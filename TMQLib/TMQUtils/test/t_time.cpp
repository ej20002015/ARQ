#include <gtest/gtest.h>
#include <TMQUtils/time.h>

#include <TMQUtils/error.h>

#include <regex>
#include <sstream>
#include <limits>

using namespace TMQ;
using namespace TMQ::Time;

/*
* ------------------------- Date class tests -------------------------
*/

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
    EXPECT_EQ( Date::MIN.year(), 0 );
    EXPECT_EQ( Date::MIN.month(), Month::Jan );
    EXPECT_EQ( Date::MIN.day(), 1 );

    EXPECT_TRUE( Date::MAX.isSet() );
    EXPECT_TRUE( Date::MAX.isValid() );
    EXPECT_EQ( Date::MAX.year(), 9999 );
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

/*
* ------------------------- DateTime class tests -------------------------
*/

TEST( DateTimeTest, DefaultConstruction )
{
    DateTime dateTime;
    EXPECT_FALSE( dateTime.isSet() );
    EXPECT_EQ( dateTime.date(), Date() );
    EXPECT_EQ( dateTime.hour(), Hour() );
    EXPECT_EQ( dateTime.minute(), Minute() );
    EXPECT_EQ( dateTime.second(), Second() );
    EXPECT_EQ( dateTime.millisecond(), Millisecond() );
    EXPECT_EQ( dateTime.microsecond(), Microsecond() );
}

TEST( DateTimeTest, DateOnlyConstruction )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date );

    EXPECT_TRUE( dateTime.isSet() );
    EXPECT_EQ( dateTime.date(), date );
    EXPECT_EQ( dateTime.hour(), Hour( 0 ) );
    EXPECT_EQ( dateTime.minute(), Minute( 0 ) );
    EXPECT_EQ( dateTime.second(), Second( 0 ) );
    EXPECT_EQ( dateTime.millisecond(), Millisecond( 0 ) );
    EXPECT_EQ( dateTime.microsecond(), Microsecond( 0 ) );
}

TEST( DateTimeTest, DateTimeConstruction )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );

    EXPECT_TRUE( dateTime.isSet() );
    EXPECT_EQ( dateTime.date(), date );
    EXPECT_EQ( dateTime.hour(), Hour( 14 ) );
    EXPECT_EQ( dateTime.minute(), Minute( 30 ) );
    EXPECT_EQ( dateTime.second(), Second( 45 ) );
    EXPECT_EQ( dateTime.millisecond(), Millisecond( 0 ) );
    EXPECT_EQ( dateTime.microsecond(), Microsecond( 0 ) );
}

TEST( DateTimeTest, TimeOfDayConstruction )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    TimeOfDay tod{
        .hour = Hour( 14 ),
        .minute = Minute( 30 ),
        .second = Second( 45 ),
        .millisecond = Millisecond( 123 ),
        .microsecond = Microsecond( 456 ),
        .microsecondsPast = Microseconds( 123456 )
    };

    DateTime dateTime( date, tod );

    EXPECT_TRUE( dateTime.isSet() );
    EXPECT_EQ( dateTime.date(), date );
    EXPECT_EQ( dateTime.hour(), Hour( 14 ) );
    EXPECT_EQ( dateTime.minute(), Minute( 30 ) );
    EXPECT_EQ( dateTime.second(), Second( 45 ) );
    EXPECT_EQ( dateTime.millisecond(), Millisecond( 123 ) );
    EXPECT_EQ( dateTime.microsecond(), Microsecond( 456 ) );
}

TEST( DateTimeTest, MicrosecondsConstruction )
{
    Microseconds us( 1234567890123456LL );
    DateTime dateTime( us );

    EXPECT_TRUE( dateTime.isSet() );
    EXPECT_EQ( dateTime.microsecondsSinceEpoch(), us );
}

TEST( DateTimeTest, TimePointConstruction )
{
    auto tp = std::chrono::system_clock::now();
    DateTime dateTime( tp );

    EXPECT_TRUE( dateTime.isSet() );
    // Note: Precision may be reduced to microseconds
    auto extractedTp = dateTime.tp();
    auto diffUs = std::chrono::duration_cast<std::chrono::microseconds>( tp - extractedTp ).count();
    EXPECT_LT( std::abs( diffUs ), 1 ); // Should be within 1 microsecond
}

TEST( DateTimeTest, NowUTC )
{
    DateTime now1 = DateTime::nowUTC();
    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    DateTime now2 = DateTime::nowUTC();

    EXPECT_TRUE( now1.isSet() );
    EXPECT_TRUE( now2.isSet() );
    EXPECT_LT( now1, now2 );
}

// DateTime Arithmetic Tests
TEST( DateTimeTest, AddYears )
{
    Date date( Year( 2024 ), Month::Feb, Day( 29 ) ); // Leap year
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );

    DateTime newDateTime = dateTime.addYears( 1 );
    EXPECT_EQ( newDateTime.date(), Date( Year( 2025 ), Month::Feb, Day( 28 ) ) ); // Adjusted for non-leap year
    EXPECT_EQ( newDateTime.hour(), Hour( 14 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 30 ) );
    EXPECT_EQ( newDateTime.second(), Second( 45 ) );

    newDateTime = newDateTime - Years( 1 );
    EXPECT_EQ( newDateTime.date(), Date( Year( 2024 ), Month::Feb, Day( 28 ) ) ); // Should stay at 28
}

TEST( DateTimeTest, AddMonths )
{
    Date date( Year( 2024 ), Month::Jan, Day( 31 ) );
    DateTime dateTime( date, Hour( 12 ), Minute( 0 ), Second( 0 ) );

    DateTime newDateTime = dateTime.addMonths( 1 );
    EXPECT_EQ( newDateTime.date(), Date( Year( 2024 ), Month::Feb, Day( 29 ) ) ); // Adjusted to last day of Feb
    EXPECT_EQ( newDateTime.hour(), Hour( 12 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 0 ) );
    EXPECT_EQ( newDateTime.second(), Second( 0 ) );
}

TEST( DateTimeTest, AddDays )
{
    Date date( Year( 2024 ), Month::Dec, Day( 30 ) );
    DateTime dateTime( date, Hour( 23 ), Minute( 59 ), Second( 59 ) );

    DateTime newDateTime = dateTime.addDays( 2 );
    EXPECT_EQ( newDateTime.date(), Date( Year( 2025 ), Month::Jan, Day( 1 ) ) );
    EXPECT_EQ( newDateTime.hour(), Hour( 23 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 59 ) );
    EXPECT_EQ( newDateTime.second(), Second( 59 ) );
}

TEST( DateTimeTest, AddHours )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 22 ), Minute( 30 ), Second( 0 ) );

    DateTime newDateTime = dateTime.addHours( 3 );
    EXPECT_EQ( newDateTime.date(), Date( Year( 2024 ), Month::May, Day( 27 ) ) );
    EXPECT_EQ( newDateTime.hour(), Hour( 1 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 30 ) );
    EXPECT_EQ( newDateTime.second(), Second( 0 ) );
}

TEST( DateTimeTest, AddMinutes )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 58 ), Second( 0 ) );

    DateTime newDateTime = dateTime.addMinutes( 5 );
    EXPECT_EQ( newDateTime.date(), Date( Year( 2024 ), Month::May, Day( 26 ) ) );
    EXPECT_EQ( newDateTime.hour(), Hour( 15 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 3 ) );
    EXPECT_EQ( newDateTime.second(), Second( 0 ) );
}

TEST( DateTimeTest, AddSeconds )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 59 ), Second( 58 ) );

    DateTime newDateTime = dateTime.addSeconds( 5 );
    EXPECT_EQ( newDateTime.date(), Date( Year( 2024 ), Month::May, Day( 26 ) ) );
    EXPECT_EQ( newDateTime.hour(), Hour( 15 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 0 ) );
    EXPECT_EQ( newDateTime.second(), Second( 3 ) );
}

TEST( DateTimeTest, AddMilliseconds )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 59 ) );

    DateTime newDateTime = dateTime.addMilliseconds( 1500 );
    EXPECT_EQ( newDateTime.hour(), Hour( 14 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 31 ) );
    EXPECT_EQ( newDateTime.second(), Second( 0 ) );
    EXPECT_EQ( newDateTime.millisecond(), Millisecond( 500 ) );
}

TEST( DateTimeTest, AddMicroseconds )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );

    DateTime newDateTime = dateTime.addMicroseconds( 1500000 ); // 1.5 seconds
    EXPECT_EQ( newDateTime.hour(), Hour( 14 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 30 ) );
    EXPECT_EQ( newDateTime.second(), Second( 46 ) );
    EXPECT_EQ( newDateTime.millisecond(), Millisecond( 500 ) );
}

TEST( DateTimeTest, SubtractionOperations )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );

    DateTime newDateTime = dateTime.subYears( 1 );
    EXPECT_EQ( newDateTime.date().year(), Year( 2023 ) );

    newDateTime = dateTime.subMonths( 2 );
    EXPECT_EQ( newDateTime.date().month(), Month::Mar );

    newDateTime = dateTime.subDays( 5 );
    EXPECT_EQ( newDateTime.date().day(), Day( 21 ) );

    newDateTime = dateTime.subHours( 2 );
    EXPECT_EQ( newDateTime.hour(), Hour( 12 ) );

    newDateTime = dateTime.subMinutes( 35 );
    EXPECT_EQ( newDateTime.minute(), Minute( 55 ) );
    EXPECT_EQ( newDateTime.hour(), Hour( 13 ) );

    newDateTime = dateTime.subSeconds( 50 );
    EXPECT_EQ( newDateTime.second(), Second( 55 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 29 ) );
}

TEST( DateTimeTest, ArithmeticOnUnsetDateTime )
{
    DateTime dateTime; // Unset

    dateTime = dateTime.addYears( 1 );
    dateTime = dateTime.addHours( 5 );
    dateTime = dateTime.addMilliseconds( 1000 );

    // Should remain unset
    EXPECT_FALSE( dateTime.isSet() );
}

// DateTime Comparison Tests
TEST( DateTimeTest, Equality )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime1( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );
    DateTime dateTime2( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );
    DateTime dateTime3( date, Hour( 14 ), Minute( 30 ), Second( 46 ) );
    DateTime unsetDateTime;

    EXPECT_EQ( dateTime1, dateTime2 );
    EXPECT_NE( dateTime1, dateTime3 );
    EXPECT_NE( dateTime1, unsetDateTime );
}

TEST( DateTimeTest, Ordering )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime early( date, Hour( 14 ), Minute( 30 ), Second( 44 ) );
    DateTime late( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );
    DateTime unsetDateTime;

    EXPECT_LT( early, late );
    EXPECT_GT( late, early );
    EXPECT_LE( early, late );
    EXPECT_GE( late, early );

    // Unset datetimes should compare as less than set datetimes
    EXPECT_LT( unsetDateTime, early );
}

TEST( DateTimeTest, DateTimeDifference )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime1( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );
    DateTime dateTime2( date, Hour( 14 ), Minute( 30 ), Second( 49 ) );

    int64_t diffUs = dateTime2 - dateTime1;
    EXPECT_EQ( diffUs, 4000000LL ); // 4 seconds in microseconds

    int64_t reverseDiffUs = dateTime1 - dateTime2;
    EXPECT_EQ( reverseDiffUs, -4000000LL );
}

// DateTime TimeOfDay Tests
TEST( DateTimeTest, TimeOfDayExtraction )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    TimeOfDay originalTod{
        .hour = Hour( 14 ),
        .minute = Minute( 30 ),
        .second = Second( 45 ),
        .millisecond = Millisecond( 123 ),
        .microsecond = Microsecond( 456 ),
        .microsecondsPast = Microseconds( 123456 )
    };

    DateTime dateTime( date, originalTod );
    TimeOfDay extractedTod = dateTime.timeOfDay();

    EXPECT_EQ( extractedTod.hour, originalTod.hour );
    EXPECT_EQ( extractedTod.minute, originalTod.minute );
    EXPECT_EQ( extractedTod.second, originalTod.second );
    EXPECT_EQ( extractedTod.millisecond, originalTod.millisecond );
    EXPECT_EQ( extractedTod.microsecond, originalTod.microsecond );
}

TEST( DateTimeTest, IndividualTimeComponents )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );

    EXPECT_EQ( dateTime.hour(), Hour( 14 ) );
    EXPECT_EQ( dateTime.minute(), Minute( 30 ) );
    EXPECT_EQ( dateTime.second(), Second( 45 ) );
    EXPECT_EQ( dateTime.millisecond(), Millisecond( 0 ) );
    EXPECT_EQ( dateTime.microsecond(), Microsecond( 0 ) );
}

// DateTime Static Members Tests
TEST( DateTimeTest, StaticMembers )
{
    EXPECT_TRUE( DateTime::MIN.isSet() );
    EXPECT_EQ( DateTime::MIN.date(), Date::MIN );

    EXPECT_TRUE( DateTime::MAX.isSet() );
    EXPECT_EQ( DateTime::MAX.date(), Date::MAX );
    EXPECT_EQ( DateTime::MAX.hour(), Hour( 23 ) );
    EXPECT_EQ( DateTime::MAX.minute(), Minute( 59 ) );
    EXPECT_EQ( DateTime::MAX.second(), Second( 59 ) );

    EXPECT_LT( DateTime::MIN, DateTime::MAX );
}

// DateTime Output Tests
TEST( DateTimeTest, StreamOutput )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );
    DateTime unsetDateTime;

    std::ostringstream oss1;
    oss1 << dateTime;
    std::string dateTimeStr = oss1.str();
    EXPECT_FALSE( dateTimeStr.empty() );

    std::ostringstream oss2;
    oss2 << unsetDateTime;
    std::string unsetStr = oss2.str();
    EXPECT_EQ( unsetStr, "DATETIME_NOT_SET" );
}

// DateTime Formatting Tests
TEST( DateTimeTest, StdFormatting )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );
    DateTime unsetDateTime;

    // Test formatting of valid datetime
    std::string formatted = std::format( "{}", dateTime );
    EXPECT_FALSE( formatted.empty() );

    // Test formatting of unset datetime
    std::string unsetFormatted = std::format( "{}", unsetDateTime );
    EXPECT_EQ( unsetFormatted, "DATETIME_NOT_SET" );
}

TEST( DateTimeTest, ISO8601FormatValidation )
{
    std::string timestamp = DateTime::nowUTC().fmtISO8601();
    ASSERT_FALSE( timestamp.empty() ) << "Timestamp string should not be empty.";
    // Expected format: YYYY-MM-DDTHH:MM:SS.ffffffZ (27 chars)
    ASSERT_EQ( timestamp.length(), 27 ) << "Timestamp string should have length 27. Actual: " << timestamp;

    // 2. Regex Check: Validate against the ISO 8601 pattern
    // Pattern: YYYY-MM-DD T HH:MM:SS . ffffff Z
    const std::regex iso8601_regex( R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z)" );

    EXPECT_TRUE( std::regex_match( timestamp, iso8601_regex ) )
        << "Timestamp '" << timestamp << "' does not match expected ISO8601 format YYYY-MM-DDTHH:MM:SS.ffffffZ";
}

// DateTime Hash Tests
TEST( DateTimeTest, Hashing )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime1( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );
    DateTime dateTime2( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );
    DateTime dateTime3( date, Hour( 14 ), Minute( 30 ), Second( 46 ) );

    DateTime::Hasher hasher;

    EXPECT_EQ( hasher( dateTime1 ), hasher( dateTime2 ) ); // Same datetimes should have same hash
    EXPECT_NE( hasher( dateTime1 ), hasher( dateTime3 ) ); // Different datetimes should have different hash
}

// DateTime Edge Cases
TEST( DateTimeTest, MidnightTransition )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 23 ), Minute( 59 ), Second( 59 ) );

    DateTime nextSecond = dateTime.addSeconds( 1 );
    EXPECT_EQ( nextSecond.date(), Date( Year( 2024 ), Month::May, Day( 27 ) ) );
    EXPECT_EQ( nextSecond.hour(), Hour( 0 ) );
    EXPECT_EQ( nextSecond.minute(), Minute( 0 ) );
    EXPECT_EQ( nextSecond.second(), Second( 0 ) );
}

TEST( DateTimeTest, YearBoundaryTransition )
{
    Date date( Year( 2024 ), Month::Dec, Day( 31 ) );
    DateTime dateTime( date, Hour( 23 ), Minute( 59 ), Second( 59 ) );

    DateTime nextSecond = dateTime.addSeconds( 1 );
    EXPECT_EQ( nextSecond.date(), Date( Year( 2025 ), Month::Jan, Day( 1 ) ) );
    EXPECT_EQ( nextSecond.hour(), Hour( 0 ) );
    EXPECT_EQ( nextSecond.minute(), Minute( 0 ) );
    EXPECT_EQ( nextSecond.second(), Second( 0 ) );
}

// DateTime Precision Tests
TEST( DateTimeTest, SubSecondPrecision )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime baseTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );

    DateTime withMillis = baseTime.addMilliseconds( 123 );
    EXPECT_EQ( withMillis.millisecond(), Millisecond( 123 ) );

    DateTime withMicros = withMillis.addMicroseconds( 456 );
    EXPECT_EQ( withMicros.microsecond(), Microsecond( 456 ) );
}

// DateTime Microsecond Boundary Tests
TEST( DateTimeTest, MicrosecondBoundary )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );

    // Test adding exactly 1 second worth of microseconds
    DateTime newDateTime = dateTime.addMicroseconds( 1000000 );
    EXPECT_EQ( newDateTime.hour(), Hour( 14 ) );
    EXPECT_EQ( newDateTime.minute(), Minute( 30 ) );
    EXPECT_EQ( newDateTime.second(), Second( 46 ) );
    EXPECT_EQ( newDateTime.millisecond(), Millisecond( 0 ) );
    EXPECT_EQ( newDateTime.microsecond(), Microsecond( 0 ) );
}

TEST( DateTimeTest, MaxMicrosecondPrecision )
{
    Date date( Year( 2024 ), Month::May, Day( 26 ) );
    DateTime dateTime( date, Hour( 14 ), Minute( 30 ), Second( 45 ) );

    // Test maximum sub-second precision
    DateTime precise = dateTime.addMicroseconds( 999999 ); // 999.999 milliseconds
    EXPECT_EQ( precise.millisecond(), Millisecond( 999 ) );
    EXPECT_EQ( precise.microsecond(), Microsecond( 999 ) );
    EXPECT_EQ( precise.microsecondsPast(), Microseconds( 999'999 ) );
}