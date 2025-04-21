#include <gtest/gtest.h>
#include <TMQUtils/time.h>

#include <regex>

using namespace TMQ;

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