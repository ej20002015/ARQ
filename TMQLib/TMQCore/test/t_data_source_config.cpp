#include <TMQCore/data_source_config.h>
#include <gtest/gtest.h>

#include <TMQUtils/error.h>

using namespace TMQ;

TEST( DataSourceConfigManagerTest, LoadFromStringValid )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.db_str]
        type = "ClickHouse"
        hostname = "host_str.example.com"
        port = 5432
        username = "user_str"
        dbName = "db_one_str"
        password = "pw_str"
        
        [data_sources.source2]
        type = "ClickHouse"
        hostname = "another@example.com"
        port = 200
    )";

    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_NO_THROW( {
        const auto& cfg = mgr.get( "db_str" );
        EXPECT_EQ( cfg.dsh, "db_str" );
        EXPECT_EQ( cfg.type, DataSourceType::ClickHouse );
        EXPECT_EQ( cfg.hostname, "host_str.example.com" );
        EXPECT_EQ( cfg.port, 5432 );
        ASSERT_TRUE( cfg.username.has_value() );
        EXPECT_EQ( cfg.username.value(), "user_str" );
        ASSERT_TRUE( cfg.dbName.has_value() );
        EXPECT_EQ( cfg.dbName.value(), "db_one_str" );
        ASSERT_TRUE( cfg.password.has_value() );
        EXPECT_EQ( cfg.password.value(), "pw_str" );
    } );

    ASSERT_NO_THROW( {
        const auto & cfg = mgr.get( "source2" );
        EXPECT_EQ( cfg.dsh, "source2" );
        EXPECT_EQ( cfg.type, DataSourceType::ClickHouse );
        EXPECT_EQ( cfg.hostname, "another@example.com" );
        EXPECT_EQ( cfg.port, 200 );
        ASSERT_TRUE( !cfg.username.has_value() );
        ASSERT_TRUE( !cfg.dbName.has_value() );
        ASSERT_TRUE( !cfg.password.has_value() );
    } );
}

TEST( DataSourceConfigManagerTest, GetNonExistentHandle )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.db_exists]
        type = "ClickHouse"
        hostname = "host_abc"
        port = 1234
    )";

    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_THROW( mgr.get( "db_does_not_exist" ), TMQException );
}

TEST( DataSourceConfigManagerTest, ParseErrorMissingRequired )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.db_no_host]
        # hostname = "missing"
        port = 5432

        [data_sources.db_ok]
        type = "ClickHouse"
        hostname = "host_c"
        port = 9999
    )";
    
    // Load should succeed overall, but will log errors for db_no_host
    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_THROW( mgr.get( "db_no_host" ), TMQ::TMQException );

    ASSERT_NO_THROW( {
        const auto & cfg = mgr.get( "db_ok" );
        EXPECT_EQ( cfg.hostname, "host_c" );
    } );
}

// Test optional field type mismatch (should log warning and return nullopt)
TEST( DataSourceConfigManagerTest, BadDataType )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.bad_data_type]
        hostname = "host"
        port = "hello"
    )";

    // Load should succeed overall, but will log errors for type mismatches
    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_THROW( mgr.get( "bad_data_type" ), TMQException );
}