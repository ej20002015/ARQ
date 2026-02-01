#include <gtest/gtest.h>
#include <ARQUtils/id.h>

#include <ARQUtils/error.h>

#include <string>
#include <set>

using namespace ARQ;
using namespace ARQ::ID;

TEST( UUIDTests, GetSessionID_IsConsistentAndValid )
{
    UUID id1 = getSessionID();
    UUID id2 = getSessionID();

    // 1. Check for consistency
    ASSERT_EQ( id1, id2 );

    // 2. Check that it's not a "nil" UUID
    std::string id_str = uuidToStr( id1 );
    ASSERT_NE( id_str, "00000000-0000-0000-0000-000000000000" );

    // 3. Check for correct string formatting
    ASSERT_EQ( id_str.length(), 36 );
}

TEST( UUIDTests, UuidNew_GeneratesUniqueIDs )
{
    UUID id1 = uuidCreate();
    UUID id2 = uuidCreate();

    ASSERT_NE( id1, id2 );

    // Generate a 1000 IDs in a tight loop and make sure all unique

    std::set<UUID> id_set;
    for( size_t i = 0; i < 1000; ++i )
        id_set.insert( uuidCreate() );

    ASSERT_EQ( id_set.size(), 1000 );
}

TEST( UUIDTests, RoundTrip_UUID_To_Str_To_UUID )
{
    // 1. Test with a newly generated ID
    UUID original_id = uuidCreate();
    std::string id_str = uuidToStr( original_id );
    UUID new_id = uuidFromStr( id_str );

    ASSERT_EQ( original_id, new_id );

    // 2. Test with the session ID
    UUID original_session_id = getSessionID();
    std::string session_str = uuidToStr( original_session_id );
    UUID new_session_id = uuidFromStr( session_str );

    ASSERT_EQ( original_session_id, new_session_id );
}

TEST( UUIDTests, RoundTrip_Str_To_UUID_To_Str )
{
    const std::string_view original_str_v7 = "0192e54a-5555-7f08-8c10-8b1a4b4f3a74";
    UUID id_v7 = uuidFromStr( original_str_v7 );
    EXPECT_EQ( uuidToStr( id_v7 ), std::string( original_str_v7 ) );
}

TEST( UUIDTests, UuidFromStr_ErrorHandling )
{
    // 1. Completely malformed string
    ASSERT_THROW( uuidFromStr( "not-a-real-uuid" ), ARQException );

    // 2. Wrong length (too short)
    ASSERT_THROW( uuidFromStr( "f47ac10b-58cc-4372-a567-0e02b2c3d47" ), ARQException );

    // 3. Wrong length (too long)
    ASSERT_THROW( uuidFromStr( "f47ac10b-58cc-4372-a567-0e02b2c3d4799" ), ARQException );

    // 4. Invalid characters
    ASSERT_THROW( uuidFromStr( "f47ac10b-58cc-4372-a567-0e02b2c3d47X" ), ARQException );

    // 5. Missing hyphens
    ASSERT_THROW( uuidFromStr( "f47ac10b58cc4372a5670e02b2c3d479" ), ARQException );

    // 6. Empty string
    ASSERT_THROW( uuidFromStr( "" ), ARQException );
}