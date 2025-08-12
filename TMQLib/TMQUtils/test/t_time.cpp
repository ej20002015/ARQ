#include <gtest/gtest.h>
#include <TMQUtils/time.h>

#include <TMQUtils/error.h>

#include <regex>
#include <sstream>
#include <limits>

using namespace TMQ;
using namespace TMQ::Time;

TEST( TimeUtilsTest, ISO8601FormatValidation )
{
    std::string timestamp = Time::getTmNowAsISO8601Str();
    ASSERT_FALSE( timestamp.empty() ) << "Timestamp string should not be empty.";
    // Expected format: YYYY-MM-DDTHH:MM:SS.ffffffZ (27 chars)
    ASSERT_EQ( timestamp.length(), 27 ) << "Timestamp string should have length 27. Actual: " << timestamp;

    // 2. Regex Check: Validate against the ISO 8601 pattern
    // Pattern: YYYY-MM-DD T HH:MM:SS . ffffff Z
    const std::regex iso8601_regex( R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z)" );

    EXPECT_TRUE( std::regex_match( timestamp, iso8601_regex ) )
        << "Timestamp '" << timestamp << "' does not match expected ISO8601 format YYYY-MM-DDTHH:MM:SS.ffffffZ";
}

// Construction Tests
TEST( DateTest, DefaultConstruction )
{
    Date date;
    EXPECT_FALSE( date.isSet() );
    EXPECT_FALSE( date.isValid() );
    EXPECT_EQ( date.year(), 0 );
    EXPECT_EQ( date.month(), Month::MTH_INV );
    EXPECT_EQ( date.day(), 0 );
    EXPECT_EQ( date.weekday(), Weekday::WKD_INV );
    EXPECT_EQ( date.serial(), std::numeric_limits<int32_t>::min() );
}

TEST( DateTest, ValidDateConstruction )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    EXPECT_TRUE( date.isSet() );
    EXPECT_TRUE( date.isValid() );
    EXPECT_EQ( date.year(), 2024 );
    EXPECT_EQ( date.month(), Month::May );
    EXPECT_EQ( date.day(), 26 );
    EXPECT_EQ( date.weekday(), Weekday::Sun ); // May 26, 2024 is a Sunday
}

TEST( DateTest, InvalidDateConstruction )
{
    // Invalid dates should throw exceptions
    EXPECT_THROW( Date( Year( 2024 ), Month::Feb, Day( 30 ) ), TMQException );
    EXPECT_THROW( Date( Year( 2023 ), Month::Feb, Day( 29 ) ), TMQException ); // 2023 is not a leap year
    EXPECT_THROW( Date( Year( 2024 ), Month::Apr, Day( 31 ) ), TMQException ); // April has 30 days
}

TEST( DateTest, LeapYearHandling )
{
    // 2024 is a leap year
    EXPECT_NO_THROW( Date( Year( 2024 ), Month::Feb, Day( 29 ) ) );

    // 2023 is not a leap year
    EXPECT_THROW( Date( Year( 2023 ), Month::Feb, Day( 29 ) ), TMQException );

    // Test century years
    Date temp( Year( 1900 ), Month::Feb, Day( 28 ) );
    auto serial = temp.serial();

    EXPECT_THROW( Date( Year( 1900 ), Month::Feb, Day( 29 ) ), TMQException ); // 1900 is not a leap year
    EXPECT_NO_THROW( Date( Year( 2000 ), Month::Feb, Day( 29 ) ) ); // 2000 is a leap year
}

TEST( DateTest, SerialConstruction )
{
    // Test serial construction with known values
    Date unixEpoch( 0 ); // January 1, 1970
    EXPECT_TRUE( unixEpoch.isSet() );
    EXPECT_TRUE( unixEpoch.isValid() );
    EXPECT_EQ( unixEpoch.year(), 1970 );
    EXPECT_EQ( unixEpoch.month(), Month::Jan );
    EXPECT_EQ( unixEpoch.day(), 1 );

    // Test a known date
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    int32_t serial = date.serial();
    Date fromSerial( serial );
    EXPECT_EQ( date.year(), fromSerial.year() );
    EXPECT_EQ( date.month(), fromSerial.month() );
    EXPECT_EQ( date.day(), fromSerial.day() );
}

// Strong Type Tests
TEST( DateTest, StrongTypes )
{
    Year y( 2024 );
    Day d( 26 );

    // Test implicit conversion
    int32_t yearValue = y;
    int32_t dayValue = d;
    EXPECT_EQ( yearValue, 2024 );
    EXPECT_EQ( dayValue, 26 );

    // Test default construction
    Year defaultYear;
    Day defaultDay;
    EXPECT_EQ( defaultYear, 0 );
    EXPECT_EQ( defaultDay, 0 );
}

// Arithmetic Tests
TEST( DateTest, AddYears )
{
    Date date( Year( 2024 ), Month::Feb, Day( 29 ) ); // Leap year date

    Date newDate = date.addYears( 1 );
    EXPECT_EQ( newDate.year(), 2025 );
    EXPECT_EQ( newDate.month(), Month::Feb );
    EXPECT_EQ( newDate.day(), 28 ); // Should adjust to Feb 28 in non-leap year

    newDate = newDate - Years( 1 );
    EXPECT_EQ( newDate.year(), 2024 );
    EXPECT_EQ( newDate.month(), Month::Feb );
    EXPECT_EQ( newDate.day(), 28 ); // Should stay at 28
}

TEST( DateTest, AddMonths )
{
    Date date( Year( 2024 ), Month::Jan, Day( 31 ) );

    Date newDate = date.addMonths( 1 );
    EXPECT_EQ( newDate.year(), 2024 );
    EXPECT_EQ( newDate.month(), Month::Feb );
    EXPECT_EQ( newDate.day(), 29 ); // Should adjust to last day of February in leap year

    newDate = newDate + Months( 1 );
    EXPECT_EQ( newDate.year(), 2024 );
    EXPECT_EQ( newDate.month(), Month::Mar );
    EXPECT_EQ( newDate.day(), 29 ); // Should stay at 29
}

TEST( DateTest, AddDays )
{
    Date date( Year( 2024 ), Month::Dec, Day( 30 ) );

    Date newDate = date.addDays( 2 );
    EXPECT_EQ( newDate.year(), 2025 );
    EXPECT_EQ( newDate.month(), Month::Jan );
    EXPECT_EQ( newDate.day(), 1 );

    newDate = newDate + Days( -1 );
    EXPECT_EQ( newDate.year(), 2024 );
    EXPECT_EQ( newDate.month(), Month::Dec );
    EXPECT_EQ( newDate.day(), 31 );
}

TEST( DateTest, MinusOperations )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );

    Date newDate = date.subYears( 1 );
    EXPECT_EQ( newDate.year(), 2023 );

    newDate = newDate.subMonths( 2 );
    EXPECT_EQ( newDate.month(), Month::Mar );

    newDate = newDate.subDays( 5 );
    EXPECT_EQ( newDate.day(), 21 );
}

TEST( DateTest, ArithmeticOnUnsetDate )
{
    Date date; // Unset date

    date = date.addYears( 1 );
    date = date.addMonths( 1 );
    date = date.addDays( 1 );

    // Should remain unset
    EXPECT_FALSE( date.isSet() );
    EXPECT_EQ( date.serial(), std::numeric_limits<int32_t>::min() );
}

// Comparison Tests
TEST( DateTest, Equality )
{
    Date date1( Year( 2024 ), Month::May, Day( 26 ) );
    Date date2( Year( 2024 ), Month::May, Day( 26 ) );
    Date date3( Year( 2024 ), Month::May, Day( 27 ) );
    Date unsetDate;

    EXPECT_EQ( date1, date2 );
    EXPECT_NE( date1, date3 );
    EXPECT_NE( date1, unsetDate );
}

TEST( DateTest, Ordering )
{
    Date early( Year( 2024 ), Month::May, Day( 25 ) );
    Date late( Year( 2024 ), Month::May, Day( 26 ) );
    Date unsetDate;

    EXPECT_LT( early, late );
    EXPECT_GT( late, early );
    EXPECT_LE( early, late );
    EXPECT_GE( late, early );

    // Unset dates should compare as less than set dates
    EXPECT_LT( unsetDate, early );
}

TEST( DateTest, DateDifference )
{
    Date date1( Year( 2024 ), Month::May, Day( 26 ) );
    Date date2( Year( 2024 ), Month::May, Day( 30 ) );

    EXPECT_EQ( date2 - date1, 4 );
    EXPECT_EQ( date1 - date2, -4 );
}

// Weekday Tests
TEST( DateTest, WeekdayCalculation )
{
    // Test known weekdays
    Date monday( Year( 2024 ), Month::May, Day( 20 ) );    // Monday
    Date tuesday( Year( 2024 ), Month::May, Day( 21 ) );   // Tuesday
    Date sunday( Year( 2024 ), Month::May, Day( 26 ) );    // Sunday

    EXPECT_EQ( monday.weekday(), Weekday::Mon );
    EXPECT_EQ( tuesday.weekday(), Weekday::Tue );
    EXPECT_EQ( sunday.weekday(), Weekday::Sun );
}

// Serial Tests
TEST( DateTest, SerialRoundTrip )
{
    Date original( Year( 2024 ), Month::May, Day( 26 ) );
    int32_t serial = original.serial();
    Date reconstructed( serial );

    EXPECT_EQ( original, reconstructed );
    EXPECT_EQ( original.year(), reconstructed.year() );
    EXPECT_EQ( original.month(), reconstructed.month() );
    EXPECT_EQ( original.day(), reconstructed.day() );
}

TEST( DateTest, SerialProgression )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    int32_t baseSerial = date.serial();

    date = date.addDays( 1 );
    EXPECT_EQ( date.serial(), baseSerial + 1 );

    date = date.addDays( -2 );
    EXPECT_EQ( date.serial(), baseSerial - 1 );
}

// Static Members Tests
TEST( DateTest, StaticMembers )
{
    EXPECT_TRUE( Date::MIN.isSet() );
    EXPECT_TRUE( Date::MIN.isValid() );
    EXPECT_EQ( Date::MIN.year(), -32767 );
    EXPECT_EQ( Date::MIN.month(), Month::Jan );
    EXPECT_EQ( Date::MIN.day(), 1 );

    EXPECT_TRUE( Date::MAX.isSet() );
    EXPECT_TRUE( Date::MAX.isValid() );
    EXPECT_EQ( Date::MAX.year(), 32767 );
    EXPECT_EQ( Date::MAX.month(), Month::Dec );
    EXPECT_EQ( Date::MAX.day(), 31 );

    EXPECT_LT( Date::MIN, Date::MAX );
}

// Output Tests
TEST( DateTest, StreamOutput )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    Date unsetDate;

    std::ostringstream oss1;
    oss1 << date;
    std::string dateStr = oss1.str();
    EXPECT_FALSE( dateStr.empty() );
    EXPECT_NE( dateStr.find( "2024" ), std::string::npos );

    std::ostringstream oss2;
    oss2 << unsetDate;
    std::string unsetStr = oss2.str();
    EXPECT_EQ( unsetStr, "DATE_NOT_SET" );
}

// Formatting Tests
TEST( DateTest, StdFormatting )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    Date unsetDate;

    // Test formatting of valid date
    std::string formatted = std::format( "{}", date );
    EXPECT_FALSE( formatted.empty() );
    EXPECT_NE( formatted.find( "2024" ), std::string::npos );
    EXPECT_NE( formatted.find( "05" ), std::string::npos ); // Month should be zero-padded
    EXPECT_NE( formatted.find( "26" ), std::string::npos );

    // Test formatting of unset date
    std::string unsetFormatted = std::format( "{}", unsetDate );
    EXPECT_EQ( unsetFormatted, "DATE_NOT_SET" );

    // Test custom format specifications
    std::string customFormatted = std::format( "{:%Y-%h-%d}", date );
    EXPECT_EQ( customFormatted, "2024-May-26" );
}

// Hash Tests
TEST( DateTest, Hashing )
{
    Date date1( Year( 2024 ), Month::May, Day( 26 ) );
    Date date2( Year( 2024 ), Month::May, Day( 26 ) );
    Date date3( Year( 2024 ), Month::May, Day( 27 ) );

    Date::Hasher hasher;

    EXPECT_EQ( hasher( date1 ), hasher( date2 ) ); // Same dates should have same hash
    EXPECT_NE( hasher( date1 ), hasher( date3 ) ); // Different dates should have different hash
}