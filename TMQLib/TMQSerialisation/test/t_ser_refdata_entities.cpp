#include <gtest/gtest.h>
#include <TMQSerialisation/ser_refdata_entities.h>

// TODO: Need to redo these tests to work with the new serialisation library

// // Helper function to create a User object
// TMQ::User createTestUser()
// {
//     TMQ::User user;
//     user.firstname = "John";
//     user.surname = "Doe";
//     user.desk = "A1";
//     user.age = 30;
//     return user;
// }

// // Test serialise function
// TEST( SerRefdataEntitiesTest, SerialiseUser )
// {
//     TMQ::User user = createTestUser();
//     TMQ::Buffer serializedData = TMQ::serialise( user );

//     // Check that the serialized data is not empty
//     EXPECT_TRUE( serializedData.size );
// }

// // Test deserialise function
// TEST( SerRefdataEntitiesTest, DeserialiseUser )
// {
//     TMQ::User user = createTestUser();
//     TMQ::Buffer serializedData = TMQ::serialise( user );
//     TMQ::User deserializedUser = TMQ::deserialise<TMQ::User>( serializedData );

//     // Check that the deserialized data matches the original data
//     EXPECT_EQ( deserializedUser.firstname, "John" );
//     EXPECT_EQ( deserializedUser.surname, "Doe" );
//     EXPECT_EQ( deserializedUser.desk, "A1" );
//     EXPECT_EQ( deserializedUser.age, 30 );
// }