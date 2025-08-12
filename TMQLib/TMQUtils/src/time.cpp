#include <TMQUtils/time.h>

#include <TMQUtils/error.h>
#include <TMQUtils/core.h>

#include <sstream>
#include <limits>
#include <format>

namespace TMQ
{

namespace Time
{

TMQUtils_API std::string tpToISO8601Str( const std::chrono::system_clock::time_point tp )
{
	using namespace std::chrono;

	auto now_time_t = system_clock::to_time_t( tp );
	auto micros = duration_cast<microseconds>( tp.time_since_epoch() ) % 1'000'000;

	std::ostringstream oss;
	oss << std::put_time( std::gmtime( &now_time_t ), "%FT%T" );
	oss << '.' << std::setw( 6 ) << std::setfill( '0' ) << micros.count() << 'Z';
	return oss.str();
}

std::string getTmNowAsISO8601Str()
{
	using namespace std::chrono;

	auto now = system_clock::now();
	return tpToISO8601Str( now );
}

Time::Date::Date( const Year y, const Month m, const Day d )
	: m_ymd( std::chrono::year_month_day( std::chrono::year( y ), std::chrono::month( m ), std::chrono::day( d ) ) )
{
	if( !m_ymd->ok() )
		throw TMQException( std::format( "Invalid date args given: year={}, month={}, day={}", static_cast<int32_t>( y ), static_cast<int32_t>( m ), static_cast<int32_t>( d ) ) );
}

Time::Date::Date( const int32_t serial )
	: m_ymd( std::chrono::sys_days( std::chrono::days( serial ) ) )
{
	if( !m_ymd->ok() )
		throw TMQException( std::format( "Invalid date serial arg given: serial={} (equivalent to {:%Y%m%d})", serial, m_ymd.value() ) );
}

Date::Date( const TimeZone tz )
{
	using namespace std::chrono;

	switch( tz )
	{
		case TimeZone::UTC:
			m_ymd = year_month_day( floor<days>( system_clock::now() ) );
			break;
		case TimeZone::Local:
			m_ymd = year_month_day( floor<days>( zoned_time( current_zone(), system_clock::now() ).get_local_time() ) );
			break;
		default:
			TMQ_ASSERT( false );
	}
}

Date::Date( const std::chrono::year_month_day ymd )
	: m_ymd( ymd )
{
	if( !m_ymd->ok() )
		throw TMQException( std::format( "Invalid std::chrono:year_month_day given: year={}, month={}, day={}", static_cast<int32_t>( ymd.year() ), static_cast<uint32_t>( ymd.month() ), static_cast<uint32_t>( ymd.day() ) ) );
}

Date Date::now()
{
	return Date( TimeZone::Local );
}

Date Date::nowUTC()
{
	return Date( TimeZone::UTC );
}

Year Date::year() const noexcept
{
	return isSet() ? Year(static_cast<int32_t>( m_ymd->year() ) ) : Year();
}

Month Date::month() const noexcept
{
	return isSet() ? static_cast<Month>( static_cast<uint32_t>( m_ymd->month() ) ) : Month::MTH_INV;
}

Day Date::day() const noexcept
{
	return isSet() ? Day( static_cast<int32_t>( static_cast<uint32_t>( m_ymd->day() ) ) ) : Day();
}

Weekday Date::weekday() const noexcept
{
	return isSet() ? static_cast<Weekday>( std::chrono::weekday( *m_ymd ).iso_encoding() ) : Weekday::WKD_INV;
}

int32_t Date::serial() const noexcept
{
	return isSet() ? std::chrono::sys_days( *m_ymd ).time_since_epoch().count() : std::numeric_limits<int32_t>::min();
}

std::chrono::year_month_day Date::ymd() const noexcept
{
	return isSet() ? *m_ymd : std::chrono::year_month_day();
}

Date Date::addYears( const int32_t years ) const noexcept
{
	if( !isSet() )
		return *this;

	Date dt;
	dt.m_ymd = m_ymd.value() + std::chrono::years( years );
	if( !dt.m_ymd->ok() )
	{
		// Must have advanced from Feb 29th to a non-leap-year
		dt.m_ymd = dt.m_ymd->year() / std::chrono::February / std::chrono::day( 28 );
	}

	return dt;
}

Date Date::subYears( const int32_t years ) const noexcept
{
	return addYears( -years );
}

Date Date::addMonths( const int32_t months ) const noexcept
{
	if( !isSet() )
		return *this;

	Date dt;
	dt.m_ymd = m_ymd.value() + std::chrono::months( months );
	if( !dt.m_ymd->ok() )
	{
		// Must be past the final day of the month
		std::chrono::year_month_day_last last = dt.m_ymd->year() / dt.m_ymd->month() / std::chrono::last;
		dt.m_ymd = dt.m_ymd->year() / dt.m_ymd->month() / last.day();
	}

	return dt;
}

Date Date::subMonths( const int32_t months ) const noexcept
{
	return addMonths( -months );
}

Date Date::addDays( const int32_t days ) const noexcept
{
	if( !isSet() )
		return *this;

	Date dt( std::chrono::sys_days( *m_ymd ) + std::chrono::days( days ) );

	return dt;
}

Date Date::subDays( const int32_t days ) const noexcept
{
	return addDays( -days );
}

std::ostream& operator<<( std::ostream& os, const Date& date )
{
	if( date.isSet() )
		os << date.m_ymd.value();
	else
		os << Date::NOT_SET_STR;

	return os;
}

const Date Date::MIN( Year( static_cast<int32_t>( std::chrono::year::min() ) ), Jan, Day( 01 ) );
const Date Date::MAX( Year( static_cast<int32_t>( std::chrono::year::max() ) ), Dec, Day( 31 ) );

}

}