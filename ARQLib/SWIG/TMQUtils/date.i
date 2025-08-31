#ifndef date_i
#define date_i

%include <stdint.i>

%include "exception.i"

%{
#include <ARQUtils/time.h>
%}

#ifdef SWIGCSHARP
%rename(sub) ARQ::Time::Date::operator-;
%rename(eq) ARQ::Time::Date::operator==;
#endif

#ifdef SWIGPYTHON
%rename(__sub__) ARQ::Time::Date::operator-;
%rename(__eq__) ARQ::Time::Date::operator==;
#endif

namespace ARQ {
namespace Time {

// Expose the enums
enum TimeZone {
    UTC,
    Local
};

enum Month {
    MTH_INV = 0,
    Jan = 1,
    Feb = 2,
    Mar = 3,
    Apr = 4,
    May = 5,
    Jun = 6,
    Jul = 7,
    Aug = 8,
    Sep = 9,
    Oct = 10,
    Nov = 11,
    Dec = 12
};

enum Weekday {
    WKD_INV = 0,
    Mon = 1,
    Tue = 2,
    Wed = 3,
    Thu = 4,
    Fri = 5,
    Sat = 6,
    Sun = 7
};

// Strong type wrappers
class Year {
public:
    explicit Year(int32_t value) noexcept;
    Year() noexcept;
};

class Day {
public:
    explicit Day(int32_t value) noexcept;
    Day() noexcept;
};

// Main Date class - only expose the methods we want
class Date {
public:
    // Constructors
    Date(const Year y, const Month m, const Day d) throw(ARQ::ARQException);
    Date(const int32_t serial) throw(ARQ::ARQException);
    Date(const TimeZone tz);
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
    int32_t serial() const noexcept;

    // Arithmetic methods
    Date addYears(const int32_t years) noexcept;
    Date subYears(const int32_t years) noexcept;
    Date addMonths(const int32_t months) noexcept;
    Date subMonths(const int32_t months) noexcept;
    Date addDays(const int32_t days) noexcept;
    Date subDays(const int32_t days) noexcept;

    // Operators
    int32_t operator-(const Date& other) const noexcept;
    bool operator==(const Date& other) const noexcept;
    
    // Static members
    static const Date MIN;
    static const Date MAX;
};

// TODO: Need to add the StrongType template

}
}

%extend ARQ::Time::Year {
    int32_t toInt() const {
        return static_cast<int32_t>(*$self);
    }
}

%extend ARQ::Time::Day {
    int32_t toInt() const {
        return static_cast<int32_t>(*$self);
    }
}

#endif