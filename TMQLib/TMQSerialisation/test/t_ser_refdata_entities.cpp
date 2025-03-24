#include <gtest/gtest.h>
#include <TMQSerialisation/ser_refdata_entities.h>
#include <TMQUtils/error.h>

// Helper function to create a User object
TMQ::User createTestUser()
{
    TMQ::User user;
    user.firstname = "John";
    user.surname = "Doe";
    user.desk = "A1";
    user.age = 30;
    user._lastUpdatedBy = "admin";
    user._lastUpdatedTm = std::chrono::system_clock::now();
    user._enabled = true;
    return user;
}

// Test serialise function
TEST( SerRefdataEntitiesTest, SerialiseUser )
{
    TMQ::User user = createTestUser();
    std::string serializedData = TMQ::serialise( std::move( user ) );

    // Check that the serialized data is not empty
    EXPECT_FALSE( serializedData.empty() );
}

// Test deserialise function
TEST( SerRefdataEntitiesTest, DeserialiseUser )
{
    TMQ::User user = createTestUser();
    std::string serializedData = TMQ::serialise( std::move( user ) );
    TMQ::User deserializedUser = TMQ::deserialise( serializedData );

    // Check that the deserialized data matches the original data
    EXPECT_EQ( deserializedUser.firstname, "John" );
    EXPECT_EQ( deserializedUser.surname, "Doe" );
    EXPECT_EQ( deserializedUser.desk, "A1" );
    EXPECT_EQ( deserializedUser.age, 30 );
    EXPECT_EQ( deserializedUser._lastUpdatedBy, "admin" );
    EXPECT_EQ( deserializedUser._enabled, true );
}