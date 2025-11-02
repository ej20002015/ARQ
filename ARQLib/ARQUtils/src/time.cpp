#include <ARQUtils/time.h>

#include <ARQUtils/error.h>
#include <ARQUtils/core.h>

#include <sstream>
#include <limits>
#include <format>
#include "time.h"

namespace ARQ
{

namespace Time
{

uint64_t unixTimestampMillis() noexcept
{
	using namespace std::chrono;

	return duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
}

/*
* ------------------------- Date class implementation -------------------------
*/

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
			ARQ_ASSERT( false );
	}
}

Time::Date::Date( const Year y, const Month m, const Day d )
	: m_ymd( std::chrono::year_month_day( std::chrono::year( y ), std::chrono::month( m ), std::chrono::day( d ) ) )
{
	if( !m_ymd->ok() )
		throw ARQException( std::format( "Invalid date args given: year={}, month={}, day={}", static_cast<int32_t>( y ), static_cast<int32_t>( m ), static_cast<int32_t>( d ) ) );
}

Time::Date::Date( const Days serial )
	: m_ymd( std::chrono::sys_days( std::chrono::days( serial ) ) )
{
	if( !m_ymd->ok() )
		throw ARQException( std::format( "Invalid date serial arg given: serial={} (equivalent to {:%Y%m%d})", serial.val(), m_ymd.value() ) );
}

Date::Date( const std::chrono::year_month_day ymd )
	: m_ymd( ymd )
{
	if( !m_ymd->ok() )
		throw ARQException( std::format( "Invalid std::chrono:year_month_day given: year={}, month={}, day={}", static_cast<int32_t>( ymd.year() ), static_cast<uint32_t>( ymd.month() ), static_cast<uint32_t>( ymd.day() ) ) );
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
	return isSet() ? Year( static_cast<int32_t>( m_ymd->year() ) ) : Year();
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

Days Date::serial() const noexcept
{
	return isSet() ? Days( std::chrono::sys_days( *m_ymd ).time_since_epoch().count() ) : Days( std::numeric_limits<int32_t>::min() );
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

const Date& Date::MIN()
{
	static const Date MIN( Year( 0 ), Jan, Day( 1 ) );
	return MIN;
}

const Date& Date::MAX()
{
	static const Date MAX( Year( 9999 ), Dec, Day( 31 ) );
	return MAX;
}

/*
* ------------------------- DateTime class implementation -------------------------
*/

DateTime::DateTime( const Date& dt )
	: m_tp( std::chrono::sys_days( dt.ymd() ) )
{
}

DateTime::DateTime( const Date& dt, const Hour h, const Minute m, const Second s )
{
	m_tp = std::chrono::sys_days( dt.ymd() );
	*m_tp += std::chrono::hours( h );
	*m_tp += std::chrono::minutes( m );
	*m_tp += std::chrono::seconds( s );
}

DateTime::DateTime( const Date& dt, const TimeOfDay& tod )
{
	m_tp = std::chrono::sys_days( dt.ymd() );
	*m_tp += std::chrono::hours( tod.hour );
	*m_tp += std::chrono::minutes( tod.minute );
	*m_tp += std::chrono::seconds( tod.second );
	*m_tp += std::chrono::milliseconds( tod.millisecond );
	*m_tp += std::chrono::microseconds( tod.microsecond );
}

DateTime::DateTime( const Microseconds us )
	: m_tp( std::chrono::microseconds( us ) )
{
}

DateTime::DateTime( const std::chrono::system_clock::time_point tp )
	: m_tp( tp )
{
}

DateTime DateTime::nowUTC()
{
	return DateTime( floor<std::chrono::microseconds>( std::chrono::system_clock::now() ) );
}

Date DateTime::date() const noexcept
{
	return isSet() ? Date( floor<std::chrono::days>( *m_tp ) ) : Date();
}

TimeOfDay DateTime::timeOfDay() const noexcept
{
	using namespace std::chrono;

	if( !isSet() )
		return TimeOfDay();

	const auto sinceMidnight = *m_tp - floor<days>( *m_tp );
	const hh_mm_ss hms( sinceMidnight );
	const auto subSeconds = sinceMidnight - floor<seconds>( sinceMidnight );
	int64_t totalUs = duration_cast<microseconds>( subSeconds ).count();

	TimeOfDay tod;
	tod.hour = Hour( hms.hours().count() );
	tod.minute = Minute( hms.minutes().count() );
	tod.second = Second( hms.seconds().count() );
	tod.millisecond = Millisecond( static_cast<int32_t>( totalUs / 1'000 ) );
	tod.microsecondsPast = Microseconds( totalUs );
	totalUs -= tod.millisecond * 1'000;
	tod.microsecond = Microsecond( totalUs );

	return tod;
}

Hour DateTime::hour() const noexcept
{
	return isSet() ? timeOfDay().hour : Hour();
}

Minute DateTime::minute() const noexcept
{
	return isSet() ? timeOfDay().minute : Minute();
}

Second DateTime::second() const noexcept
{
	return isSet() ? timeOfDay().second : Second();
}

Millisecond DateTime::millisecond() const noexcept
{
	return isSet() ? timeOfDay().millisecond : Millisecond();
}

Microsecond DateTime::microsecond() const noexcept
{
	return isSet() ? timeOfDay().microsecond : Microsecond();
}

Microseconds DateTime::microsecondsPast() const noexcept
{
	return isSet() ? timeOfDay().microsecondsPast : Microseconds();
}

Microseconds DateTime::microsecondsSinceEpoch() const noexcept
{
	return isSet() ? Microseconds( std::chrono::duration_cast<std::chrono::microseconds>( m_tp->time_since_epoch() ).count() ) : Microseconds( std::numeric_limits<int64_t>::min() );
}

std::chrono::system_clock::time_point DateTime::tp() const noexcept
{
	return isSet() ? *m_tp : std::chrono::system_clock::time_point();
}

DateTime DateTime::addYears( const int32_t years ) const noexcept
{
	return isSet() ? DateTime( date().addYears( years ), timeOfDay() ) : *this;
}

DateTime DateTime::subYears( const int32_t years ) const noexcept
{
	return addYears( -years );
}

DateTime DateTime::addMonths( const int32_t months ) const noexcept
{
	return isSet() ? DateTime( date().addMonths( months ), timeOfDay() ) : *this;
}

DateTime DateTime::subMonths( const int32_t months ) const noexcept
{
	return addMonths( -months );
}

DateTime DateTime::addDays( const int32_t days ) const noexcept
{
	return isSet() ? DateTime( *m_tp + std::chrono::days( days ) ) : *this;
}

DateTime DateTime::subDays( const int32_t days ) const noexcept
{
	return addDays( -days );
}

DateTime DateTime::addHours( const int64_t hours ) const noexcept
{
	return isSet() ? DateTime( *m_tp + std::chrono::hours( hours ) ) : *this;
}

DateTime DateTime::subHours( const int64_t hours ) const noexcept
{
	return addHours( -hours );
}

DateTime DateTime::addMinutes( const int64_t minutes ) const noexcept
{
	return isSet() ? DateTime( *m_tp + std::chrono::minutes( minutes ) ) : *this;
}

DateTime DateTime::subMinutes( const int64_t minutes ) const noexcept
{
	return addMinutes( -minutes );
}

DateTime DateTime::addSeconds( const int64_t seconds ) const noexcept
{
	return isSet() ? DateTime( *m_tp + std::chrono::seconds( seconds ) ) : *this;
}

DateTime DateTime::subSeconds( const int64_t seconds ) const noexcept
{
	return addSeconds( -seconds );
}

DateTime DateTime::addMilliseconds( const int64_t milliseconds ) const noexcept
{
	return isSet() ? DateTime( *m_tp + std::chrono::milliseconds( milliseconds ) ) : *this;
}

DateTime DateTime::subMilliseconds( const int64_t milliseconds ) const noexcept
{
	return addMilliseconds( -milliseconds );
}

DateTime DateTime::addMicroseconds( const int64_t microseconds ) const noexcept
{
	return isSet() ? DateTime( *m_tp + std::chrono::microseconds( microseconds ) ) : *this;
}

DateTime DateTime::subMicroseconds( const int64_t microseconds ) const noexcept
{
	return addMicroseconds( -microseconds );
}

std::ostream& operator<<( std::ostream& os, const DateTime& dateTime )
{
	if( dateTime.isSet() )
		os << dateTime.m_tp.value();
	else
		os << DateTime::NOT_SET_STR;

	return os;
}

std::string DateTime::fmtISO8601() const
{
	return std::format( "{:%FT%TZ}", std::chrono::floor<std::chrono::microseconds>( tp() ) );
}

// NOTE: DateTime range is much smaller than that of Date (std::chrono::year_month_day).
// This is due to Linux std::chrono::system_clock::time_point having a restricted range.

const DateTime& DateTime::MIN()
{
	static const DateTime MIN( Date( Year( 1900 ), Month::Jan, Day( 01 ) ) );
	return MIN;
}

const DateTime& DateTime::MAX()
{
	static const DateTime MAX( Date( Year( 2200 ), Month::Dec, Day( 31 ) ), TimeOfDay{
		.hour = Hour( 23 ),
		.minute = Minute( 59 ),
		.second = Second( 59 ),
		.millisecond = Millisecond( 999 ),
		.microsecond = Microsecond( 999 ),
		.microsecondsPast = Microseconds( 999'999 )
	} );

	return MAX;
}

}

}