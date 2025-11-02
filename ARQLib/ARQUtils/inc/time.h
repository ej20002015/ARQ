#pragma once
#include <ARQUtils/dll.h>

#include "types.h"

#include <string>
#include <chrono>
#include <optional>
#include <compare>
#include <format> 

namespace ARQ
{

namespace Time
{

uint64_t unixTimestampMillis() noexcept;

template<typename Dur>
concept c_Duration = requires {
	typename Dur::rep;
	typename Dur::period;
};

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
	ARQUtils_API explicit Date( const TimeZone tz );
	ARQUtils_API explicit Date( const Year y, const Month m, const Day d );
	ARQUtils_API explicit Date( const Days serial );
	ARQUtils_API explicit Date( const std::chrono::year_month_day ymd );
	Date() = default;

	static Date now();
	static Date nowUTC();

	[[nodiscard]] inline bool isValid() const noexcept { return m_ymd.has_value() ? m_ymd->ok() : false; };

	[[nodiscard]] inline bool isSet() const noexcept { return m_ymd.has_value(); }

	[[nodiscard]] ARQUtils_API Year    year()    const noexcept;
	[[nodiscard]] ARQUtils_API Month   month()   const noexcept;
	[[nodiscard]] ARQUtils_API Day     day()     const noexcept;
	[[nodiscard]] ARQUtils_API Weekday weekday() const noexcept;

	[[nodiscard]] ARQUtils_API Days serial() const noexcept;

	[[nodiscard]] ARQUtils_API std::chrono::year_month_day ymd() const noexcept;

	[[nodiscard]] ARQUtils_API Date addYears( const int32_t years )   const noexcept;
	[[nodiscard]] ARQUtils_API Date subYears( const int32_t years )   const noexcept;
	[[nodiscard]] ARQUtils_API Date addMonths( const int32_t months ) const noexcept;
	[[nodiscard]] ARQUtils_API Date subMonths( const int32_t months ) const noexcept;
	[[nodiscard]] ARQUtils_API Date addDays( const int32_t days )     const noexcept;
	[[nodiscard]] ARQUtils_API Date subDays( const int32_t days )     const noexcept;

	inline Date operator+( const Years years )   const { return addYears( years ); }
	inline Date operator-( const Years years )   const { return subYears( years ); }
	inline Date operator+( const Months months ) const { return addMonths( months ); }
	inline Date operator-( const Months months ) const { return subMonths( months ); }
	inline Date operator+( const Days days )     const { return addDays( days ); }
	inline Date operator-( const Days days )     const { return subDays( days ); }

	inline Days operator-( const Date& other ) const noexcept { return Days( serial() - other.serial() ); }

	inline bool operator ==( const Date& other ) const noexcept { return m_ymd == other.m_ymd; }
	inline std::strong_ordering operator<=>( const Date& other ) const noexcept { return m_ymd <=> other.m_ymd; };

	ARQUtils_API friend std::ostream& operator<<( std::ostream& os, const Date& date );
	
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
	ARQUtils_API static const Date& MIN();
	ARQUtils_API static const Date& MAX();

private:
	static constexpr auto NOT_SET_STR = "DATE_NOT_SET";

private:
	std::optional<std::chrono::year_month_day> m_ymd;
};

struct HourTag {};
using Hour = StrongType<HourTag, int32_t>;
struct MinuteTag {};
using Minute = StrongType<MinuteTag, int32_t>;
struct SecondTag {};
using Second = StrongType<SecondTag, int32_t>;
struct MillisecondTag {};
using Millisecond = StrongType<MillisecondTag, int32_t>;
struct MicrosecondTag {};
using Microsecond = StrongType<MicrosecondTag, int32_t>;

struct HoursTag {};
using Hours = StrongType<HoursTag, int64_t>;
struct MinutesTag {};
using Minutes = StrongType<MinutesTag, int64_t>;
struct SecondsTag {};
using Seconds = StrongType<SecondsTag, int64_t>;
struct MillisecondsTag {};
using Milliseconds = StrongType<MillisecondsTag, int64_t>;
struct MicrosecondsTag {};
using Microseconds = StrongType<MicrosecondsTag, int64_t>;

struct TimeOfDay
{
	Hour         hour;             // 0-23
	Minute       minute;           // 0-59
	Second       second;           // 0-59
	Millisecond  millisecond;      // 0-999
	Microsecond  microsecond;      // 0-999
	Microseconds microsecondsPast; // 0-999'999
};

class DateTime
{
public:
	ARQUtils_API explicit DateTime( const Date& dt );
	ARQUtils_API explicit DateTime( const Date& dt, const Hour h, const Minute m, const Second s );
	ARQUtils_API explicit DateTime( const Date& dt, const TimeOfDay& tod );
	ARQUtils_API explicit DateTime( const Microseconds us );
	ARQUtils_API explicit DateTime( const std::chrono::system_clock::time_point tp );
	DateTime() = default;

	static DateTime nowUTC();

	// TODO: Provide timezone conversion functions

	[[nodiscard]] inline bool isSet() const noexcept { return m_tp.has_value(); }

	[[nodiscard]] ARQUtils_API Date         date()             const noexcept;
	[[nodiscard]] ARQUtils_API TimeOfDay    timeOfDay()        const noexcept;
	[[nodiscard]] ARQUtils_API Hour         hour()             const noexcept;
	[[nodiscard]] ARQUtils_API Minute       minute()           const noexcept;
	[[nodiscard]] ARQUtils_API Second       second()           const noexcept;
	[[nodiscard]] ARQUtils_API Millisecond  millisecond()      const noexcept;
	[[nodiscard]] ARQUtils_API Microsecond  microsecond()      const noexcept;
	[[nodiscard]] ARQUtils_API Microseconds microsecondsPast() const noexcept;

	[[nodiscard]] ARQUtils_API Microseconds microsecondsSinceEpoch() const noexcept;

	[[nodiscard]] ARQUtils_API std::chrono::system_clock::time_point tp() const noexcept;

	[[nodiscard]] ARQUtils_API DateTime addYears( const int32_t years )               const noexcept;
	[[nodiscard]] ARQUtils_API DateTime subYears( const int32_t years )               const noexcept;
	[[nodiscard]] ARQUtils_API DateTime addMonths( const int32_t months )             const noexcept;
	[[nodiscard]] ARQUtils_API DateTime subMonths( const int32_t months )             const noexcept;
	[[nodiscard]] ARQUtils_API DateTime addDays( const int32_t days )                 const noexcept;
	[[nodiscard]] ARQUtils_API DateTime subDays( const int32_t days )                 const noexcept;
	[[nodiscard]] ARQUtils_API DateTime addHours( const int64_t hours )               const noexcept;
	[[nodiscard]] ARQUtils_API DateTime subHours( const int64_t hours )               const noexcept;
	[[nodiscard]] ARQUtils_API DateTime addMinutes( const int64_t minutes )           const noexcept;
	[[nodiscard]] ARQUtils_API DateTime subMinutes( const int64_t minutes )           const noexcept;
	[[nodiscard]] ARQUtils_API DateTime addSeconds( const int64_t seconds )           const noexcept;
	[[nodiscard]] ARQUtils_API DateTime subSeconds( const int64_t seconds )           const noexcept;
	[[nodiscard]] ARQUtils_API DateTime addMilliseconds( const int64_t milliseconds ) const noexcept;
	[[nodiscard]] ARQUtils_API DateTime subMilliseconds( const int64_t milliseconds ) const noexcept;
	[[nodiscard]] ARQUtils_API DateTime addMicroseconds( const int64_t microseconds ) const noexcept;
	[[nodiscard]] ARQUtils_API DateTime subMicroseconds( const int64_t microseconds ) const noexcept;

	inline DateTime operator+( const Years years )               const { return addYears( years ); }
	inline DateTime operator-( const Years years )               const { return subYears( years ); }
	inline DateTime operator+( const Months months )             const { return addMonths( months ); }
	inline DateTime operator-( const Months months )             const { return subMonths( months ); }
	inline DateTime operator+( const Days days )                 const { return addDays( days ); }
	inline DateTime operator-( const Days days )                 const { return subDays( days ); }
	inline DateTime operator+( const Hours hours )               const { return addHours( hours ); }
	inline DateTime operator-( const Hours hours )               const { return subHours( hours ); }
	inline DateTime operator+( const Minutes minutes )           const { return addMinutes( minutes ); }
	inline DateTime operator-( const Minutes minutes )           const { return subMinutes( minutes ); }
	inline DateTime operator+( const Seconds seconds )           const { return addSeconds( seconds ); }
	inline DateTime operator-( const Seconds seconds )           const { return subSeconds( seconds ); }
	inline DateTime operator+( const Milliseconds milliseconds ) const { return addMilliseconds( milliseconds ); }
	inline DateTime operator-( const Milliseconds milliseconds ) const { return subMilliseconds( milliseconds ); }
	inline DateTime operator+( const Microseconds microseconds ) const { return addMicroseconds( microseconds ); }
	inline DateTime operator-( const Microseconds microseconds ) const { return subMicroseconds( microseconds ); }

	inline Microseconds operator-( const DateTime& other ) const noexcept { return Microseconds( microsecondsSinceEpoch() - other.microsecondsSinceEpoch() ); }

	inline bool operator ==( const DateTime& other ) const noexcept { return m_tp == other.m_tp; }
	inline std::strong_ordering operator<=>( const DateTime& other ) const noexcept { return m_tp <=> other.m_tp; };

	ARQUtils_API friend std::ostream& operator<<( std::ostream& os, const DateTime& date );

	std::string fmtISO8601() const;

	friend struct std::formatter<DateTime>;

public:

	struct Hasher
	{
		size_t operator()( const DateTime& dateTime ) const noexcept
		{
			return std::hash<int64_t>()( dateTime.microsecondsSinceEpoch() );
		}
	};

public:
	ARQUtils_API static const DateTime& MIN();
	ARQUtils_API static const DateTime& MAX();

private:
	static constexpr auto NOT_SET_STR = "DATETIME_NOT_SET";

private:
	std::optional<std::chrono::system_clock::time_point> m_tp;
};

}

}

// Specialization of std::formatter for Date and DateTime classes

namespace std
{

template <typename CharT>
struct formatter<ARQ::Time::Date, CharT> : formatter<std::chrono::year_month_day, CharT>
{
	template <typename FormatContext>
	auto format( const ARQ::Time::Date& date, FormatContext& ctx ) const -> decltype( ctx.out() )
	{
		if( date.isSet() )
			return formatter<std::chrono::year_month_day, CharT>::format( date.m_ymd.value(), ctx );
		else
			return std::format_to( ctx.out(), "{}", ARQ::Time::Date::NOT_SET_STR );
	}
};

template <typename CharT>
struct formatter<ARQ::Time::DateTime, CharT> : formatter<std::chrono::system_clock::time_point, CharT>
{
	template <typename FormatContext>
	auto format( const ARQ::Time::DateTime& dateTime, FormatContext& ctx ) const -> decltype( ctx.out() )
	{
		if( dateTime.isSet() )
			return formatter<std::chrono::system_clock::time_point, CharT>::format( dateTime.m_tp.value(), ctx );
		else
			return std::format_to( ctx.out(), "{}", ARQ::Time::DateTime::NOT_SET_STR );
	}
};

}