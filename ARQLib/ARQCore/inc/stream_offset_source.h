#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQCore/streaming_service.h>

#include <string>
#include <mutex>
#include <type_traits>
#include <optional>

namespace ARQ
{

class IStreamOffsetSource
{
public:
	virtual ~IStreamOffsetSource() = default;

	virtual void                                       saveOffsets( const std::string_view key, const StreamTopicPartitionOffsets& offsets ) = 0;
	virtual std::optional<StreamTopicPartitionOffsets> getOffsets( const std::string_view key)                                               = 0;
};

using StreamOffsetSourceCreateFunc = std::add_pointer<IStreamOffsetSource*( const std::string_view dsh )>::type;

class StreamOffsetSourceFactory
{
public:
	[[nodiscard]] ARQCore_API static std::shared_ptr<IStreamOffsetSource> create( const std::string_view dsh );

	ARQCore_API static void addCustomSource( const std::string_view dsh, const std::shared_ptr<IStreamOffsetSource>& source );
	ARQCore_API static void delCustomSource( const std::string_view dsh );

private:
	static std::unordered_map<std::string, std::shared_ptr<IStreamOffsetSource>, TransparentStringHash, std::equal_to<>> s_sources;
	static std::mutex s_sourcesMutex;
};

}