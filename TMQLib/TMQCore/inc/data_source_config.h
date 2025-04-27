#pragma once

#include <TMQUtils/hashers.h>

#include <string>
#include <optional>
#include <unordered_map>
#include <shared_mutex>

namespace TMQ
{

struct DataSourceConfig
{
	std::string handle;

	std::string hostname;
	uint16_t port;

	std::optional<std::string> username;
	std::optional<std::string> password;
	std::optional<std::string> dbName;
};

class DataSourceConfigManager
{
public:
	DataSourceConfigManager() = default; // Can create other instances for testing
	DataSourceConfigManager( const DataSourceConfigManager& ) = delete;
	DataSourceConfigManager& operator=( const DataSourceConfigManager& ) = delete;

	static DataSourceConfigManager& inst()
	{
		static DataSourceConfigManager inst;
		return inst;
	}

	const DataSourceConfig& get( const std::string_view handle );

	void load( const std::optional<std::string_view> tomlCfg = std::nullopt );

private:
	void iLoad( const std::optional<std::string_view> tomlCfg = std::nullopt );
	void parseConfig( const std::string_view tomlCfg );

private:
	std::unordered_map<std::string, DataSourceConfig, TransparentStringHash, std::equal_to<>> m_configMap;
	std::once_flag m_loadFlag;
};

}