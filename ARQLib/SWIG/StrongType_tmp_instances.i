#ifndef StrongType_tmp_instances_i
#define StrongType_tmp_instances_i

%include <stdint.i>

%{
#include <ARQUtils/types.h>
%}

#ifdef SWIGPYTHON
%rename(Time_Year) ARQ::Time::Year;
%rename(Time_Day) ARQ::Time::Day;
%rename(Time_Days) ARQ::Time::Days;
%rename(Time_Hour) ARQ::Time::Hour;
%rename(Time_Minute) ARQ::Time::Minute;
%rename(Time_Second) ARQ::Time::Second;
%rename(Time_Millisecond) ARQ::Time::Millisecond;
%rename(Time_Microsecond) ARQ::Time::Microsecond;
%rename(Time_Hours) ARQ::Time::Hours;
%rename(Time_Minutes) ARQ::Time::Minutes;
%rename(Time_Seconds) ARQ::Time::Seconds;
%rename(Time_Milliseconds) ARQ::Time::Milliseconds;
%rename(Time_Microseconds) ARQ::Time::Microseconds;
#endif

namespace ARQ
{

// Time StrongTypes
namespace Time
{

class Year
{
public:
	explicit Year( const int32_t val ) noexcept;
	Year();

	int32_t val() const noexcept;
};

class Day
{
public:
	explicit Day( const int32_t val ) noexcept;
	Day();

	int32_t val() const noexcept;
};

class Days
{
public:
	explicit Days( const int32_t val ) noexcept;
	Days();

	int32_t val() const noexcept;
};

class Hour
{
public:
	explicit Hour( const int32_t val ) noexcept;
	Hour();

	int32_t val() const noexcept;
};

class Minute
{
public:
    explicit Minute( const int32_t val ) noexcept;
    Minute();

    int32_t val() const noexcept;
};

class Second
{
public:
	explicit Second( const int32_t val ) noexcept;
	Second();

	int32_t val() const noexcept;
};

class Millisecond
{
public:
	explicit Millisecond( const int32_t val ) noexcept;
	Millisecond();

	int32_t val() const noexcept;
};

class Microsecond
{
public:
	explicit Microsecond( const int32_t val ) noexcept;
	Microsecond();

	int32_t val() const noexcept;
};

class Hours
{
public:
	explicit Hours( const int64_t val ) noexcept;
	Hours();

	int64_t val() const noexcept;
};

class Minutes
{
public:
	explicit Minutes( const int64_t val ) noexcept;
	Minutes();

	int64_t val() const noexcept;
};

class Seconds
{
public:
	explicit Seconds( const int64_t val ) noexcept;
	Seconds();

	int64_t val() const noexcept;
};

class Milliseconds
{
public:
	explicit Milliseconds( const int64_t val ) noexcept;
	Milliseconds();

	int64_t val() const noexcept;
};

class Microseconds
{
public:
	explicit Microseconds( const int64_t val ) noexcept;
	Microseconds();

	int64_t val() const noexcept;
};

}

}

#endif