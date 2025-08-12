#pragma once
#include <TMQUtils/dll.h>

#include "types.h"

#include <string>
#include <chrono>
#include <optional>
#include <compare>
#include <format> 

namespace TMQ
{

namespace Time
{

TMQUtils_API std::string tpToISO8601Str( const std::chrono::system_clock::time_point tp );
TMQUtils_API std::string getTmNowAsISO8601Str();

template<typename Dur>
concept c_Duration = requires {
	typename Dur::rep;
	typename Dur::period;
};

template<c_Duration Duration = std::chrono::nanoseconds>
inline uint64_t tpToLong( const std::chrono::system_clock::time_point tp )
{
	return std::chrono::duration_cast<Duration>( tp.time_since_epoch() ).count();
}

template<c_Duration Duration = std::chrono::nanoseconds>
inline std::chrono::system_clock::time_point longToTp( const uint64_t lng )
{
	return std::chrono::system_clock::time_point(
		std::chrono::duration_cast<std::chrono::system_clock::duration>( Duration( lng ) )
	);
}

enum TimeZone
{
	UTC,
	Local
};

enum Month
{
	MTH_INV = 0,
	Jan,
	Feb,
	Mar,
	Apr,
	May,
	Jun,
	Jul,
	Aug,
	Sep,
	Oct,
	Nov,
	Dec
};

enum Weekday
{
	WKD_INV = 0,
	Mon,
	Tue,
	Wed,
	Thu,
	Fri,
	Sat,
	Sun
};

struct YearTag {};
using Year = StrongType<YearTag, int32_t>;
struct DayTag {};
using Day = StrongType<DayTag, int32_t>;

struct YearsTag {};
using Years = StrongType<YearsTag, int32_t>;
struct MonthsTag {};
using Months = StrongType<MonthsTag, int32_t>;
struct DaysTag {};
using Days = StrongType<DaysTag, int32_t>;

class Date
{
public:
	TMQUtils_API explicit Date( const Year y, const Month m, const Day d );
	TMQUtils_API explicit Date( const int32_t serial );
	TMQUtils_API explicit Date( const TimeZone tz );
	TMQUtils_API explicit Date( const std::chrono::year_month_day ymd );
	Date() = default;

	static Date now();
	static Date nowUTC();

	[[nodiscard]] inline bool isValid() const noexcept { return m_ymd.has_value() ? m_ymd->ok() : false; };

	[[nodiscard]] inline bool isSet() const noexcept { return m_ymd.has_value(); }

	[[nodiscard]] TMQUtils_API Year    year()    const noexcept;
	[[nodiscard]] TMQUtils_API Month   month()   const noexcept;
	[[nodiscard]] TMQUtils_API Day     day()     const noexcept;
	[[nodiscard]] TMQUtils_API Weekday weekday() const noexcept;

	[[nodiscard]] TMQUtils_API int32_t serial() const noexcept;

	[[nodiscard]] TMQUtils_API std::chrono::year_month_day ymd() const noexcept;

	TMQUtils_API Date addYears( const int32_t years ) const noexcept;
	TMQUtils_API Date subYears( const int32_t years ) const noexcept;
	TMQUtils_API Date addMonths( const int32_t months ) const noexcept;
	TMQUtils_API Date subMonths( const int32_t months ) const noexcept;
	TMQUtils_API Date addDays( const int32_t days ) const noexcept;
	TMQUtils_API Date subDays( const int32_t days ) const noexcept;

	inline Date operator+( const Years years )   const { return addYears( years ); }
	inline Date operator-( const Years years )   const { return subYears( years ); }
	inline Date operator+( const Months months ) const { return addMonths( months ); }
	inline Date operator-( const Months months ) const { return subMonths( months ); }
	inline Date operator+( const Days days )     const { return addDays( days ); }
	inline Date operator-( const Days days )     const { return subDays( days ); }

	inline int32_t operator-( const Date& other ) const noexcept { return serial() - other.serial(); }

	inline bool operator ==( const Date& other ) const noexcept { return m_ymd == other.m_ymd; }
	inline std::strong_ordering operator<=>( const Date& other ) const noexcept { return m_ymd <=> other.m_ymd; };

	TMQUtils_API friend std::ostream& operator<<( std::ostream& os, const Date& date );
	
	friend struct std::formatter<Date>;

public:

	struct Hasher
	{
		size_t operator()( const Date& date ) const noexcept
		{
			return std::hash<int32_t>()( date.serial() );
		}
	};

public:
	TMQUtils_API static const Date MIN;
	TMQUtils_API static const Date MAX;

private:
	std::optional<std::chrono::year_month_day> m_ymd;

private:
	static constexpr auto NOT_SET_STR = "DATE_NOT_SET";
};

}

}

// Specialization of std::formatter for Date class
namespace std
{

template <typename CharT>
struct formatter<TMQ::Time::Date, CharT> : formatter<std::chrono::year_month_day, CharT>
{
	template <typename FormatContext>
	auto format( const TMQ::Time::Date& date, FormatContext& ctx ) const -> decltype( ctx.out() )
	{
		if( date.isSet() )
			return formatter<std::chrono::year_month_day, CharT>::format( date.m_ymd.value(), ctx );
		else
			return std::format_to( ctx.out(), "{}", TMQ::Time::Date::NOT_SET_STR );
	}
};

}