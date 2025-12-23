#include <ARQCore/data_source_config.h>

#include <ARQUtils/enum.h>
#include <ARQUtils/error.h>
#include <ARQUtils/sys.h>
#include <ARQUtils/toml.h>
#include <ARQCore/logger.h>

#include <filesystem>
#include <fstream>

namespace ARQ
{

DataSourceConfigManager& DataSourceConfigManager::inst()
{
	static DataSourceConfigManager inst;
	return inst;
}

const DataSourceConfig& DataSourceConfigManager::get( const std::string_view dsh )
{
	std::call_once( m_loadFlag, [this] () { iLoad(); } );

	const auto it = m_configMap.find( dsh );
	if( it != m_configMap.end() )
		return it->second;
	else
		throw ARQException( std::format( "Cannot find DataSourceConfig for dsh {}", dsh ) );
}

void DataSourceConfigManager::load( const std::optional<std::string_view> tomlCfg )
{
	std::call_once( m_loadFlag, [this, &tomlCfg] () { iLoad( tomlCfg ); } );
}

void DataSourceConfigManager::iLoad( const std::optional<std::string_view> tomlCfg )
{
	std::string tomlCfgFileStorage;

	const std::string_view tomlCfgSV = tomlCfg.value_or( "" );
	if( tomlCfgSV.empty() )
	{
		// Need to read from file

		const std::filesystem::path configFilepath = Sys::cfgDir() / "datasources" / std::format( "{}.toml", Sys::env() );

		std::ifstream ifs( configFilepath );
		if( !ifs.is_open() )
			throw ARQException( std::format( "Failed to open data source configuration file {}", configFilepath.string() ) );

		std::stringstream buffer;
		buffer << ifs.rdbuf();
		tomlCfgFileStorage = buffer.str();

		Log( Module::CORE ).info( "Loading data source configuration from file [{}]", configFilepath.string() );
	}
	else
		Log( Module::CORE ).info( "Loading data source configuration directly from provided string" );

	parseConfig( tomlCfgSV.empty() ? std::string_view( tomlCfgFileStorage ) : tomlCfgSV );
}

template <typename T>
T getRequiredTomlValue( const toml::table& table, const std::string& key )
{
	const toml::node* node = table.get( key );
	if( !node )
		throw ARQException( std::format( "Missing required key '{}'", key ) );

	std::optional<T> value = node->value<T>();
	if( !value )
		throw ARQException( std::format( "Key '{}' has incorrect type (expected {})", key, typeid( T ).name() ) );

	return *value;
}

// Safely gets an optional value
template <typename T>
std::optional<T> getOptionalTomlValue( const toml::table& table, const std::string& key )
{
	const toml::node* node = table.get( key );
	if( !node )
		return std::nullopt;

	std::optional<T> value = node->value<T>();
	if( !value )
		throw ARQException( std::format( "Key '{}' has incorrect type (expected {})", key, typeid( T ).name() ) );

	return value;
}

void DataSourceConfigManager::parseConfig( const std::string_view tomlCfg )
{
	toml::table tbl;
	try
	{
		tbl = toml::parse( tomlCfg );
	}
	catch( const toml::parse_error& err )
	{
		std::stringstream ss;
		ss << err;
		throw ARQException( std::format( "Parsing error when reading data source configuration toml: {}", ss.str() ) );
	}

	toml::table* dataSourcesTbl = tbl["data_sources"].as_table();
	if( !dataSourcesTbl )
		throw ARQException( "No [data_sources] table found in data source configuration toml" );

	for( const auto& [key, val] : *dataSourcesTbl )
	{
		std::string dsh = std::string( key.str() );
		toml::table* sourceTable = val.as_table();

		if( !sourceTable )
		{
			Log( Module::CORE ).error( "Entry '{}' in [data_sources] is not a table. Skipping.", dsh );
			continue;
		}

		try
		{
			DataSourceConfig cfg;
			cfg.dsh = dsh;

			std::optional<DataSourceType> typeOpt = Enum::enum_cast<DataSourceType>( getRequiredTomlValue<std::string>( *sourceTable, "type" ) );
			if( !typeOpt )
			{
				Log( Module::CORE ).error( "Entry '{}' in [data_sources] has invalid type. Skipping.", dsh );
				continue;
			}
			cfg.type = *typeOpt;

			const toml::node* connPropsNode = sourceTable->get( "conn_props" );
			if( !connPropsNode )
			{
				Log( Module::CORE ).error( "Entry '{}' in [data_sources] is missing required key 'conn_props'. Skipping", dsh );
				continue;
			}

			// Parse conn_props subtable

			const toml::table* connPropsTable = connPropsNode->as_table();
			if( !connPropsTable )
			{
				Log( Module::CORE ).error( "Entry '{}' in [data_sources] has 'conn_props' key but it's not a table. Skipping", dsh );
				continue;
			}

			for( const auto& [key, val] : *connPropsTable )
			{
				const std::string connPropsName = std::string( key.str() );
				DataSourceConfig::ConnProps connProps;

				const toml::table* const connPropsEntry = val.as_table();
				if( !connPropsEntry )
					throw ARQException( std::format( "Entry '{}' in [conn_props] table for dsh '{}' is not a table.", connPropsName, dsh ) );

				connProps.hostname = getRequiredTomlValue<std::string>( *connPropsEntry, "hostname" );

				int64_t portI64 = getRequiredTomlValue<int64_t>( *connPropsEntry, "port" );
				if( portI64 <= 0 || portI64 > 65535 )
					throw ARQException( std::format( "Invalid port number {} (must be 1-65535)", portI64 ) );
				connProps.port = static_cast<uint16_t>( portI64 );

				// Optional fields

				connProps.username = getOptionalTomlValue<std::string>( *connPropsEntry, "username" );
				connProps.password = getOptionalTomlValue<std::string>( *connPropsEntry, "password" );
				connProps.dbName = getOptionalTomlValue<std::string>( *connPropsEntry, "dbName" );

				cfg.connPropsMap[connPropsName] = connProps;
			}

			// Insert into map

			auto [it, inserted] = m_configMap.emplace( dsh, std::move( cfg ) );
			if( !inserted )
			{
				Log( Module::CORE ).warn( "Duplicate data source handle [{}] found in config. Overwriting.", dsh );
				it->second = std::move( cfg );
			}

			Log( Module::CORE ).debug( "Parsed config for data source [{}]", dsh );
		}
		catch( const ARQException& specificError )
		{
			Log( Module::CORE ).error( "Failed to parse data source [{}]: {}. Skipping.", dsh, specificError.what() );
		}
		catch( const std::exception& ex )
		{
			Log( Module::CORE ).error( "Unexpected error parsing data source [{}]: {}. Skipping.", dsh, ex.what() );
		}
	}
}

}
