#include <TMQClickHouse/test.h>

#include <TMQClickHouse/query.h>
#include <TMQUtils/error.h>

#include <iostream>
#include <vector>
#include <string>

#include <chrono>
#include <sstream>
#include <clickhouse/client.h>

using namespace clickhouse;

namespace TMQ
{

struct RDEntity
{
    std::chrono::system_clock::time_point lastUpdated;
};

// Dummy User struct as per your definition
struct User : public RDEntity
{
    std::string firstname;
    std::string surname;
    std::string desk;
    uint32_t age;
};

template<typename T>
class RDEntityTraits
{
public:
    static consteval const char* const schemaName() { static_assert( false ); }
    static std::string getID( const T& rdEntity ) { static_assert( false ); }
};

// RDEntityTraits specialization for User
template<>
class RDEntityTraits<User>
{
public:
    static consteval const char* const schemaName() { return "Users"; }
    static std::string getID( const User& user ) { return user.firstname + user.surname; }

    // Serialize the User into a raw binary buffer (just for testing)
    static std::string serialize( const User& user )
    {
        std::ostringstream oss;

        // Serialize the `firstname` string
        uint32_t firstnameLength = user.firstname.size();
        oss.write( reinterpret_cast< const char* >( &firstnameLength ), sizeof( uint32_t ) );
        oss.write( user.firstname.c_str(), firstnameLength );

        // Serialize the `surname` string
        uint32_t surnameLength = user.surname.size();
        oss.write( reinterpret_cast< const char* >( &surnameLength ), sizeof( uint32_t ) );
        oss.write( user.surname.c_str(), surnameLength );

        // Serialize the `desk` string
        uint32_t deskLength = user.desk.size();
        oss.write( reinterpret_cast< const char* >( &deskLength ), sizeof( uint32_t ) );
        oss.write( user.desk.c_str(), deskLength );

        // Serialize the `age` field
        oss.write( reinterpret_cast< const char* >( &user.age ), sizeof( uint32_t ) );

        return oss.str();  // Return the raw serialized data as a string
    }
};

// Function to create a dummy user
User createDummyUser( int index )
{
    User user;
    user.firstname = "First" + std::to_string( index );
    user.surname = "Last" + std::to_string( index );
    user.desk = "Desk" + std::to_string( index );
    user.age = 20 + index;
    user.lastUpdated = std::chrono::system_clock::now();
    return user;
}

// Function to insert dummy users into ClickHouse
void insertUsersIntoClickHouse( clickhouse::Client& client, const std::vector<User>& users )
{
    // Prepare the ClickHouse insert query
    clickhouse::Block block;
    auto idCol = std::make_shared<clickhouse::ColumnString>();
    auto tsCol = std::make_shared<clickhouse::ColumnDateTime>();
    auto activeCol = std::make_shared<clickhouse::ColumnUInt8>();
    auto blobCol = std::make_shared<clickhouse::ColumnString>();


    for( const auto& user : users )
    {
        // Prepare the values to insert
        std::string serializedBlob = RDEntityTraits<User>::serialize( user );
        std::string userID = RDEntityTraits<User>::getID( user );

        // Push the values into the respective columns
        idCol->Append( userID );     // ID
        tsCol->Append( std::chrono::system_clock::to_time_t( user.lastUpdated ) ); // Ts
        activeCol->Append( 1 );           // Active (dummy active value)
        blobCol->Append( serializedBlob ); // Blob (Serialized user)
    }

    block.AppendColumn( "ID", idCol );
    block.AppendColumn( "Ts", tsCol );
    block.AppendColumn( "Active", activeCol );
    block.AppendColumn( "Blob", blobCol );

    // Perform the insert operation
    client.Insert( "RefData.Users", block );
}

void doTest()
{
    try
    {
        // Create ClickHouse client and connect to your ClickHouse server
        clickhouse::Client client( clickhouse::ClientOptions().SetHost( "localhost" ) );

        // Create some dummy users
        std::vector<User> users;
        for( int i = 0; i < 10; ++i )
        {
            users.push_back( createDummyUser( i ) );
        }

        // Insert users into the ClickHouse database
        insertUsersIntoClickHouse( client, users );

        std::cout << "Dummy users inserted into ClickHouse." << std::endl;
    }
    catch( const TMQException& e )
    {
        std::cerr << "Error occured running test: " << e.what();
        std::cerr << "\tLocation: " << e.where();
    }
}

}
