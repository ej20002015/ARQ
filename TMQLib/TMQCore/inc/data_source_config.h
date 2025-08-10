#pragma once

#include <TMQUtils/hashers.h>

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <shared_mutex>
#include <array>

namespace TMQ
{

struct DataSourceType
{
public:

	enum Enum
	{
		ClickHouse,

		_SIZE
	};

	static Enum fromStr( const std::string_view str );
	static std::string_view toStr( const Enum type );

private:
	static constexpr const std::array<std::string_view, static_cast<size_t>( DataSourceType::_SIZE )> TYPE_STRINGS = { "ClickHouse" };
};

struct DataSourceConfig
{
	std::string dsh;

	DataSourceType::Enum type;

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