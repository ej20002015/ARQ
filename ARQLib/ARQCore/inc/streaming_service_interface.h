#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/buffer.h>
#include <ARQUtils/time.h>

#include <string>
#include <optional>
#include <map>
#include <chrono>
#include <functional>
#include <variant>

using namespace std::chrono_literals;

namespace ARQ
{

using StreamingHeaderMap = std::map<std::string, std::string>;

struct StreamProducerMessage
{
	std::string_view                       topic;
	std::optional<uint64_t>                id;
	std::optional<std::string_view>        key;
	std::optional<int32_t>                 partition;
	// data: 'BufferView' (zero-copy, you guarantee lifetime) or 'SharedBuffer' (safe, producer extends lifetime)
	std::variant<BufferView, SharedBuffer> data;
	StreamingHeaderMap                     headers;
};

enum class StreamMessagePersistedStatus
{
	NOT_PERSISTED,
	PERSISTED,
	UNKNOWN
};

struct StreamProducerMessageMetadata
{
	std::optional<uint64_t>      messageID;
	std::string                  topic;
	int32_t                      partition;
	std::optional<int64_t>       offset;
	size_t                       keySize;
	size_t                       valueSize;
	Time::DateTime               timestamp;
	StreamMessagePersistedStatus persistedStatus;
};

using StreamProducerDeliveryCallbackFunc = std::function<void( const StreamProducerMessageMetadata& messageMetadata, std::optional<std::string> error )>;

class IStreamProducer
{
public:
	ARQCore_API virtual ~IStreamProducer() = default;

	ARQCore_API virtual void send( const StreamProducerMessage& msg, const StreamProducerDeliveryCallbackFunc& callback = StreamProducerDeliveryCallbackFunc() ) = 0;
	ARQCore_API virtual void flush( const std::chrono::milliseconds timeout = 10'000ms )                                                                         = 0;
};

}