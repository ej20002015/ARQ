#ifndef date_i
#define date_i

%include <stdint.i>

%include "exception.i"
%include "StrongType_tmp_instances.i"

%{
#include <ARQUtils/time.h>
%}

#ifdef SWIGCSHARP
%rename(sub) ARQ::Time::Date::operator-;
%rename(eq) ARQ::Time::Date::operator==;
%rename(sub) ARQ::Time::DateTime::operator-;
%rename(eq) ARQ::Time::DateTime::operator==;
#endif

#ifdef SWIGPYTHON
%rename(Time_TimeZone) ARQ::Time::TimeZone;
%rename(Time_Month) ARQ::Time::Month;
%rename(Time_Weekday) ARQ::Time::Weekday;
%rename(Time_Date) ARQ::Time::Date;
    %rename(now_utc) ARQ::Time::Date::nowUTC;
    %rename(is_valid) ARQ::Time::Date::isValid;
    %rename(is_set) ARQ::Time::Date::isSet;
    %rename(add_years) ARQ::Time::Date::addYears;
    %rename(sub_years) ARQ::Time::Date::subYears;
    %rename(add_months) ARQ::Time::Date::addMonths;
    %rename(sub_months) ARQ::Time::Date::subMonths;
    %rename(add_days) ARQ::Time::Date::addDays;
    %rename(sub_days) ARQ::Time::Date::subDays;
    %rename(__sub__) ARQ::Time::Date::operator-;
    %rename(__eq__) ARQ::Time::Date::operator==;
%rename(Time_TimeOfDay) ARQ::Time::TimeOfDay;
%rename(Time_DateTime) ARQ::Time::DateTime;
    %rename(now_utc) ARQ::Time::DateTime::nowUTC;
    %rename(is_set) ARQ::Time::DateTime::isSet;
    %rename(time_of_day) ARQ::Time::DateTime::timeOfDay;
    %rename(microseconds_past) ARQ::Time::DateTime::microsecondsPast;
    %rename(microseconds_since_epoch) ARQ::Time::DateTime::microsecondsSinceEpoch;
    %rename(add_years) ARQ::Time::DateTime::addYears;
    %rename(sub_years) ARQ::Time::DateTime::subYears;
    %rename(add_months) ARQ::Time::DateTime::addMonths;
    %rename(sub_months) ARQ::Time::DateTime::subMonths;
    %rename(add_days) ARQ::Time::DateTime::addDays;
    %rename(sub_days) ARQ::Time::DateTime::subDays;
    %rename(add_hours) ARQ::Time::DateTime::addHours;
    %rename(sub_hours) ARQ::Time::DateTime::subHours;
    %rename(add_minutes) ARQ::Time::DateTime::addMinutes;
    %rename(sub_minutes) ARQ::Time::DateTime::subMinutes;
    %rename(add_seconds) ARQ::Time::DateTime::addSeconds;
    %rename(sub_seconds) ARQ::Time::DateTime::subSeconds;
    %rename(add_milliseconds) ARQ::Time::DateTime::addMilliseconds;
    %rename(sub_milliseconds) ARQ::Time::DateTime::subMilliseconds;
    %rename(add_microseconds) ARQ::Time::DateTime::addMicroseconds;
    %rename(sub_microseconds) ARQ::Time::DateTime::subMicroseconds;
    %rename(__sub__) ARQ::Time::DateTime::operator-;
    %rename(__eq__) ARQ::Time::DateTime::operator==;
    %rename(fmt_ISO8601) ARQ::Time::DateTime::fmtISO8601;
#endif

namespace ARQ
{
namespace Time
{

// Expose the enums
enum class TimeZone
{
    UTC,
    Local,
};

enum class Month
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
    Dec,
};

enum class Weekday
{
    WKD_INV = 0,
    Mon,
    Tue,
    Wed,
    Thu,
    Fri,
    Sat,
    Sun,
};

// Main Date class - only expose the methods we want
class Date
{
public:
    // Constructors
    Date( const Year y, const Month m, const Day d ) throw( ARQ::ARQException );
    Date( const Days serial ) throw( ARQ::ARQException );
    Date( const TimeZone tz );
    Date();

    static Date now();
    static Date nowUTC();

    // Query methods
    bool isValid() const noexcept;
    bool isSet() const noexcept;
    
    Year year() const noexcept;
    Month month() const noexcept;
    Day day() const noexcept;
    Weekday weekday() const noexcept;

    Days serial() const noexcept;

    // Arithmetic methods
    Date addYears( const int32_t years ) noexcept;
    Date subYears( const int32_t years ) noexcept;
    Date addMonths( const int32_t months ) noexcept;
    Date subMonths( const int32_t months ) noexcept;
    Date addDays( const int32_t days ) noexcept;
    Date subDays( const int32_t days ) noexcept;

    // Operators
    Days operator-( const Date& other ) const noexcept;
    bool operator==( const Date& other ) const noexcept;

    // Static members
    static const Date& MIN();
	static const Date& MAX();
};

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
    // Constructors
	DateTime( const Date& dt );
	DateTime( const Date& dt, const Hour h, const Minute m, const Second s );
	DateTime( const Date& dt, const TimeOfDay& tod );
	DateTime( const Microseconds us );
	DateTime();

	static DateTime nowUTC();

	// TODO: Provide timezone conversion functions

    // Query methods
	[[nodiscard]] inline bool isSet() const noexcept;

	Date         date()             const noexcept;
	TimeOfDay    timeOfDay()        const noexcept;
	Hour         hour()             const noexcept;
	Minute       minute()           const noexcept;
	Second       second()           const noexcept;
	Millisecond  millisecond()      const noexcept;
	Microsecond  microsecond()      const noexcept;
	Microseconds microsecondsPast() const noexcept;

	Microseconds microsecondsSinceEpoch() const noexcept;

    // Arithmetic methods
	DateTime addYears( const int32_t years )               const noexcept;
	DateTime subYears( const int32_t years )               const noexcept;
	DateTime addMonths( const int32_t months )             const noexcept;
	DateTime subMonths( const int32_t months )             const noexcept;
	DateTime addDays( const int32_t days )                 const noexcept;
	DateTime subDays( const int32_t days )                 const noexcept;
	DateTime addHours( const int64_t hours )               const noexcept;
	DateTime subHours( const int64_t hours )               const noexcept;
	DateTime addMinutes( const int64_t minutes )           const noexcept;
	DateTime subMinutes( const int64_t minutes )           const noexcept;
	DateTime addSeconds( const int64_t seconds )           const noexcept;
	DateTime subSeconds( const int64_t seconds )           const noexcept;
	DateTime addMilliseconds( const int64_t milliseconds ) const noexcept;
	DateTime subMilliseconds( const int64_t milliseconds ) const noexcept;
	DateTime addMicroseconds( const int64_t microseconds ) const noexcept;
	DateTime subMicroseconds( const int64_t microseconds ) const noexcept;

    // Operators
	Microseconds operator-( const DateTime& other ) const noexcept;
	bool operator ==( const DateTime& other ) const noexcept;

	std::string fmtISO8601() const;

    // Static members
	static const DateTime& MIN();
	static const DateTime& MAX();
};

}
}

#endif