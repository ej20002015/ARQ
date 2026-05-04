#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <shared_mutex>
#include <array>
#include <mutex>

namespace ARQ
{

enum class DataSourceType
{
	ClickHouse,
	gRPC,
	NATS,
	Kafka,
	Redis,

	_SIZE_
};

static constexpr std::array<std::string_view, static_cast<size_t>( DataSourceType::_SIZE_ )> DYNA_LIB_NAMES = {
	"ARQClickHouse",
	"ARQGrpc",
	"ARQNATS",
	"ARQKafka",
	"ARQRedis",
};

static_assert(
	DYNA_LIB_NAMES.size() == static_cast<size_t>( DataSourceType::_SIZE_ ),
	"Mismatch between DataSourceType and DYNA_LIB_NAMES"
);

inline std::string_view dataSourceTypeToDynaLibName( const DataSourceType type ) { return DYNA_LIB_NAMES[static_cast<size_t>( type )]; }

struct DataSourceConfig
{
	std::string dsh;

	DataSourceType type;

	struct ConnProps
	{

		std::string hostname;
		uint16_t    port;

		std::optional<std::string> username;
		std::optional<std::string> password;
		std::optional<std::string> dbName;
	};

	std::unordered_map<std::string, ConnProps> connPropsMap;
};

class DataSourceConfigManager
{
public:
	ARQCore_API DataSourceConfigManager() = default; // Can create other instances for testing
	DataSourceConfigManager( const DataSourceConfigManager& ) = delete;
	DataSourceConfigManager& operator=( const DataSourceConfigManager& ) = delete;

	ARQCore_API static DataSourceConfigManager& inst();

	ARQCore_API const DataSourceConfig& get( const std::string_view dsh );

	ARQCore_API void load( const std::optional<std::string_view> tomlCfg = std::nullopt );

private:
	void iLoad( const std::optional<std::string_view> tomlCfg = std::nullopt );
	void parseConfig( const std::string_view tomlCfg );

private:
	std::unordered_map<std::string, DataSourceConfig, TransparentStringHash, std::equal_to<>> m_configMap;
	std::once_flag m_loadFlag;
};

}
