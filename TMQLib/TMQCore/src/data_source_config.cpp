#include <TMQCore/data_source_config.h>

#include <TMQUtils/error.h>
#include <TMQUtils/sys.h>
#include <TMQUtils/toml.h>
#include <TMQCore/logger.h>

#include <filesystem>
#include <fstream>

namespace TMQ
{

const DataSourceConfig& DataSourceConfigManager::get( const std::string_view handle )
{
	std::call_once( m_loadFlag, [this] () { iLoad(); } );

	const auto it = m_configMap.find( handle );
	if( it != m_configMap.end() )
		return it->second;
	else
		throw TMQException( std::format( "Cannot find DataSourceConfig for handle {}", handle ) );
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
			throw TMQException( std::format( "Failed to open data source configuration file {}", configFilepath.string() ) );

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
		throw TMQException( std::format( "Missing required key '{}'", key ) );

	std::optional<T> value = node->value<T>();
	if( !value )
		throw TMQException( std::format( "Key '{}' has incorrect type (expected {})", key, typeid( T ).name() ) );

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
		throw TMQException( std::format( "Key '{}' has incorrect type (expected {})", key, typeid( T ).name() ) );

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
		throw TMQException( std::format( "Parsing error when reading data source configuration toml: {}", ss.str() ) );
	}

	toml::table* dataSourcesTbl = tbl["data_sources"].as_table();
	if( !dataSourcesTbl )
		throw TMQException( "No [data_sources] table found in data source configuration toml" );

	for( const auto [key, val] : *dataSourcesTbl )
	{
		std::string handle = std::string( key.str() );
		toml::table* sourceTable = val.as_table();

		if( !sourceTable )
		{
			Log( Module::CORE ).error( "Entry '{}' in [data_sources] is not a table. Skipping.", handle );
			continue;
		}

		try
		{
			DataSourceConfig cfg;
			cfg.handle = handle;

			// Required fields

			cfg.hostname = getRequiredTomlValue<std::string>( *sourceTable, "hostname" );

			int64_t portI64 = getRequiredTomlValue<int64_t>( *sourceTable, "port" );
			if( portI64 <= 0 || portI64 > 65535 )
				throw TMQException( std::format( "Invalid port number {} (must be 1-65535)", portI64 ) );
			cfg.port = static_cast<uint16_t>( portI64 );

			// Optional fields

			cfg.username = getOptionalTomlValue<std::string>( *sourceTable, "username" );
			cfg.password = getOptionalTomlValue<std::string>( *sourceTable, "password" );
			cfg.dbName   = getOptionalTomlValue<std::string>( *sourceTable, "dbName" );

			// Insert into map

			auto [it, inserted] = m_configMap.emplace( handle, std::move( cfg ) );
			if( !inserted )
			{
				Log( Module::CORE ).warn( "Duplicate data source handle [{}] found in config. Overwriting.", handle );
				it->second = std::move( cfg );
			}

			Log( Module::CORE ).debug( "Parsed config for data source [{}]", handle );
		}
		catch( const TMQException& specificError )
		{
			Log( Module::CORE ).error( "Failed to parse data source [{}]: {}. Skipping.", handle, specificError.what() );
		}
		catch( const std::exception& ex )
		{
			Log( Module::CORE ).error( "Unexpected error parsing data source [{}]: {}. Skipping.", handle, ex.what() );
		}
	}
}

}
