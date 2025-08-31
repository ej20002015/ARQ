#include <gtest/gtest.h>
#include <ARQSerialisation/ser_refdata_entities.h>

// TODO: Need to redo these tests to work with the new serialisation library

// // Helper function to create a User object
// ARQ::User createTestUser()
// {
//     ARQ::User user;
//     user.firstname = "John";
//     user.surname = "Doe";
//     user.desk = "A1";
//     user.age = 30;
//     return user;
// }

// // Test serialise function
// TEST( SerRefdataEntitiesTest, SerialiseUser )
// {
//     ARQ::User user = createTestUser();
//     ARQ::Buffer serializedData = ARQ::serialise( user );

//     // Check that the serialized data is not empty
//     EXPECT_TRUE( serializedData.size );
// }

// // Test deserialise function
// TEST( SerRefdataEntitiesTest, DeserialiseUser )
// {
//     ARQ::User user = createTestUser();
//     ARQ::Buffer serializedData = ARQ::serialise( user );
//     ARQ::User deserializedUser = ARQ::deserialise<ARQ::User>( serializedData );

//     // Check that the deserialized data matches the original data
//     EXPECT_EQ( deserializedUser.firstname, "John" );
//     EXPECT_EQ( deserializedUser.surname, "Doe" );
//     EXPECT_EQ( deserializedUser.desk, "A1" );
//     EXPECT_EQ( deserializedUser.age, 30 );
// }