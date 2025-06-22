#pragma once
#include <TMQUtils/dll.h>

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

class Year
{
public:
	explicit Year(int32_t value) noexcept : 
		m_value(value) 
	{}
	Year() noexcept = default;
	
	operator int32_t() const noexcept { return m_value; }
	
private:
	int32_t m_value = 0;
};

class Day
{
public:
	explicit Day(int32_t value) noexcept
		: m_value(value) 
	{}
	Day() noexcept = default;
	
	operator int32_t() const noexcept { return m_value; }
	
private:
	int32_t m_value = 0;
};

class Date
{
public:
	TMQUtils_API Date( const Year y, const Month m, const Day d );
	TMQUtils_API Date( const int32_t serial );
	TMQUtils_API Date( const TimeZone tz );
	Date() = default;

	[[nodiscard]] inline bool isValid() const noexcept { return m_ymd.has_value() ? m_ymd->ok() : false; };

	[[nodiscard]] inline bool isSet() const noexcept { return m_ymd.has_value(); }

	[[nodiscard]] TMQUtils_API Year    year()    const noexcept;
	[[nodiscard]] TMQUtils_API Month   month()   const noexcept;
	[[nodiscard]] TMQUtils_API Day     day()     const noexcept;
	[[nodiscard]] TMQUtils_API Weekday weekday() const noexcept;

	[[nodiscard]] TMQUtils_API int32_t serial() const noexcept;

	TMQUtils_API void addYears( const int32_t years ) noexcept;
	TMQUtils_API void subYears( const int32_t years ) noexcept;
	TMQUtils_API void addMonths( const int32_t months ) noexcept;
	TMQUtils_API void subMonths( const int32_t months ) noexcept;
	TMQUtils_API void addDays( const int32_t days ) noexcept;
	TMQUtils_API void subDays( const int32_t days ) noexcept;

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