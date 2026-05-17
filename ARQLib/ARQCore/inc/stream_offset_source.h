#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQUtils/global_accessor.h>
#include <ARQCore/streaming_service.h>

#include <string>
#include <mutex>
#include <type_traits>
#include <optional>

namespace ARQ
{

static constexpr std::string_view MARKETS_KEY_NAMESPACE = "Markets";

class IStreamOffsetSource
{
public:
	virtual ~IStreamOffsetSource() = default;

	virtual void                                       saveOffsets( const std::string_view key, const StreamTopicPartitionOffsets& offsets ) = 0;
	virtual std::optional<StreamTopicPartitionOffsets> getOffsets( const std::string_view key)                                               = 0;
};

using StreamOffsetSourceCreateFunc = std::add_pointer<IStreamOffsetSource*( const std::string_view dsh )>::type;

class StreamOffsetSourceFactory : public GlobalAccessor<StreamOffsetSourceFactory, "StreamOffsetSourceFactory">
{
public:
	[[nodiscard]] ARQCore_API std::shared_ptr<IStreamOffsetSource> create( const std::string_view dsh );

	ARQCore_API void addCustomSource( const std::string_view dsh, const std::shared_ptr<IStreamOffsetSource>& source );
	ARQCore_API void delCustomSource( const std::string_view dsh );

private:
	std::unordered_map<std::string, std::shared_ptr<IStreamOffsetSource>, TransparentStringHash, std::equal_to<>> m_sources;
	std::mutex m_sourcesMutex;
};

}