#include <ARQCore/data_source_config.h>
#include <gtest/gtest.h>

#include <ARQUtils/error.h>

using namespace ARQ;

TEST( DataSourceConfigManagerTest, LoadFromStringValid )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.db_str]
        type = "ClickHouse"
        [data_sources.db_str.conn_props.Main]
        hostname = "host_str.example.com"
        port = 5432
        username = "user_str"
        dbName = "db_one_str"
        password = "pw_str"
        
        [data_sources.source2]
        type = "ClickHouse"
        [data_sources.source2.conn_props.Main]
        hostname = "another@example.com"
        port = 200
    )";

    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_NO_THROW( {
        const auto& cfg = mgr.get( "db_str" );
        const auto& connProps = cfg.connPropsMap.at( "Main" );
        EXPECT_EQ( cfg.dsh, "db_str" );
        EXPECT_EQ( cfg.type, DataSourceType::ClickHouse );
        EXPECT_EQ( connProps.hostname, "host_str.example.com" );
        EXPECT_EQ( connProps.port, 5432 );
        ASSERT_TRUE( connProps.username.has_value() );
        EXPECT_EQ( connProps.username.value(), "user_str" );
        ASSERT_TRUE( connProps.dbName.has_value() );
        EXPECT_EQ( connProps.dbName.value(), "db_one_str" );
        ASSERT_TRUE( connProps.password.has_value() );
        EXPECT_EQ( connProps.password.value(), "pw_str" );
    } );

    ASSERT_NO_THROW( {
        const auto & cfg = mgr.get( "source2" );
        const auto& connProps = cfg.connPropsMap.at( "Main" );
        EXPECT_EQ( cfg.dsh, "source2" );
        EXPECT_EQ( cfg.type, DataSourceType::ClickHouse );
        EXPECT_EQ( connProps.hostname, "another@example.com" );
        EXPECT_EQ( connProps.port, 200 );
        ASSERT_TRUE( !connProps.username.has_value() );
        ASSERT_TRUE( !connProps.dbName.has_value() );
        ASSERT_TRUE( !connProps.password.has_value() );
    } );
}

TEST( DataSourceConfigManagerTest, GetNonExistentHandle )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.db_exists]
        type = "ClickHouse"
        [data_sources.db_exists.conn_props.Main]
        hostname = "host_abc"
        port = 1234
    )";

    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_THROW( mgr.get( "db_does_not_exist" ), ARQException );
}

TEST( DataSourceConfigManagerTest, ParseErrorMissingRequired )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.db_no_host]
        type = "ClickHouse"
        [data_sources.db_no_host.conn_props.Main]
        # hostname = "missing"
        port = 5432

        [data_sources.db_ok]
        type = "ClickHouse"
        [data_sources.db_ok.conn_props.Main]
        hostname = "host_c"
        port = 9999
    )";
    
    // Load should succeed overall, but will log errors for db_no_host
    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_THROW( mgr.get( "db_no_host" ), ARQ::ARQException );

    ASSERT_NO_THROW( {
        const auto & cfg = mgr.get( "db_ok" );
        const auto& connProps = cfg.connPropsMap.at( "Main" );
        EXPECT_EQ( connProps.hostname, "host_c" );
    } );
}

// Test optional field type mismatch (should log warning and return nullopt)
TEST( DataSourceConfigManagerTest, BadDataType )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.bad_data_type]
        type = "ClickHouse"
        [data_sources.bad_data_type.conn_props.Main]
        hostname = "host"
        port = "hello"
    )";

    // Load should succeed overall, but will log errors for type mismatches
    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_THROW( mgr.get( "bad_data_type" ), ARQException );
}

TEST( DataSourceConfigManagerTest, MissingConnPropsKey )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.missing_conn_props]
        type = "ClickHouse"
    )";

    // Load should succeed overall, but will log errors for no conn_props key
    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_THROW( mgr.get( "missing_conn_props" ), ARQException );
}

TEST( DataSourceConfigManagerTest, MultipleConnPropsEntries )
{
    const std::string tomlContent = R"(
        [data_sources]
        [data_sources.multiple_conn_props]
        type = "ClickHouse"
        [data_sources.multiple_conn_props.conn_props.First]
        hostname = "host_str.example.com"
        port = 5432
        username = "user_str"
        dbName = "db_one_str"
        password = "pw_str"
        [data_sources.multiple_conn_props.conn_props.Second]
        hostname = "second_host_str.example.com"
        port = 5433
    )";

    DataSourceConfigManager mgr;
    ASSERT_NO_THROW( mgr.load( tomlContent ) );

    ASSERT_NO_THROW( {
        const auto& cfg = mgr.get( "multiple_conn_props" );
        EXPECT_EQ( cfg.dsh, "multiple_conn_props" );
        EXPECT_EQ( cfg.type, DataSourceType::ClickHouse );
        const auto& connPropsFirst = cfg.connPropsMap.at( "First" );
        EXPECT_EQ( connPropsFirst.hostname, "host_str.example.com" );
        EXPECT_EQ( connPropsFirst.port, 5432 );
        ASSERT_TRUE( connPropsFirst.username.has_value() );
        EXPECT_EQ( connPropsFirst.username.value(), "user_str" );
        ASSERT_TRUE( connPropsFirst.dbName.has_value() );
        EXPECT_EQ( connPropsFirst.dbName.value(), "db_one_str" );
        ASSERT_TRUE( connPropsFirst.password.has_value() );
        EXPECT_EQ( connPropsFirst.password.value(), "pw_str" );
        const auto& connPropsSecond = cfg.connPropsMap.at( "Second" );
        EXPECT_EQ( connPropsSecond.hostname, "second_host_str.example.com" );
        EXPECT_EQ( connPropsSecond.port, 5433 );
        ASSERT_FALSE( connPropsSecond.username.has_value() );
    } );
}