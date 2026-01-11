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
#include <any>
#include <set>
#include <format>

using namespace std::chrono_literals;

namespace ARQ
{

using StreamHeaderMap     = std::map<std::string, std::string>;
using StreamHeaderMapView = std::map<std::string_view, std::string_view>;

class StreamError
{
public:
	int32_t             code;
	std::string         message;
	bool                isFatal;
	std::optional<bool> isRetriable;
	bool                transRequiresAbort;

	bool isDefinitelyRetriable()    const { return isRetriable.has_value() && isRetriable.value(); }
	bool isDefinitelyNonRetriable() const { return isRetriable.has_value() && !isRetriable.value(); }
};

struct StreamProducerMessage
{
	std::string                            topic;
	std::optional<uint64_t>                id;
	std::optional<std::string>             key;
	std::optional<int32_t>                 partition;
	// data: 'BufferView' (zero-copy, you guarantee lifetime) or 'SharedBuffer' (safe, producer extends lifetime)
	std::variant<BufferView, SharedBuffer> data;
	StreamHeaderMap                        headers;
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

/// Opaque wrapper for innternal consumer group metadata
struct StreamGroupMetadata
{
	std::any impl;
};

using StreamTopicPartition = std::pair<std::string, int32_t>;

using StreamProducerDeliveryCallbackFunc = std::function<void( const StreamProducerMessageMetadata& messageMetadata, std::optional<StreamError> error )>;

class IStreamProducer
{
public:
	ARQCore_API virtual ~IStreamProducer() = default;

	/**
	 * @brief Asynchronously pushes a message to the local output queue.
	 * The library handles batching and network transmission in the background.
	 * @param msg The message payload and keys.
	 * @param callback Optional function called on successful delivery or failure.
	 */
	ARQCore_API virtual void send( const StreamProducerMessage& msg, const StreamProducerDeliveryCallbackFunc& callback = StreamProducerDeliveryCallbackFunc() ) = 0;

	/**
	 * @brief Blocks until all messages in the local queue are sent to the broker.
	 * @note Critical to call this before destroying the producer to prevent data loss.
	 */
	ARQCore_API virtual void flush( const std::chrono::milliseconds timeout = 10'000ms ) = 0;

	// -----------------------------------------------------------------
	// Transactional API (EOS)
	// -----------------------------------------------------------------

	/**
	 * @brief Initializes the transactional producer ID with the broker.
	 * Must be called exactly once after startup if transactions are enabled.
	 * Recovers any previous zombie transactions for this ID.
	 */
	ARQCore_API virtual void initTransactions( std::chrono::milliseconds timeout = 60s ) = 0;

	/**
	 * @brief Marks the beginning of a new atomic transaction.
	 * All 'send' calls after this are part of the transaction until committed/aborted.
	 */
	ARQCore_API virtual void beginTransaction() = 0;

	/**
	 * @brief Links the Consumer's read position to this Producer's write transaction.
	 * Ensures that the input offsets are only committed if the output messages are successfully written.
	 * @note CRITICAL: You must pass the offset of the NEXT message to read.
	 * If you just processed message with offset 100, you must pass offset 101.
	 * (e.g., offsets[partition] = msg.offset + 1;)
	 * @param offsets The map of TopicPartition -> Offset to commit.
	 * @param groupMetadata The consumer's generation ID (From IStreamConsumer::getGroupMetadata).
	 */
	ARQCore_API virtual void sendOffsetsToTransaction( const std::map<StreamTopicPartition, int64_t>& offsets, const StreamGroupMetadata& groupMetadata, std::chrono::milliseconds timeout = 60s ) = 0;

	/**
	 * @brief Atomically commits all messages and offsets in the current transaction.
	 * @throws ARQException if the commit failed (e.g., fencing error).
	 */
	ARQCore_API virtual void commitTransaction( std::chrono::milliseconds timeout = 60s ) = 0;

	/**
	 * @brief Aborts the current transaction, discarding unsent messages and rolling back state.
	 * @note Should be called in the catch block if any step of the "Consume-Process-Produce" loop fails.
	 */
	ARQCore_API virtual void abortTransaction( std::chrono::milliseconds timeout = 60s ) = 0;
};

class StreamConsumerMessageView
{
public:
	std::string_view                topic;
	int32_t                         partition;
	int64_t                         offset;
	std::optional<std::string_view> key;
	BufferView                      data;
	Time::DateTime                  timestamp;
	StreamHeaderMapView             headers;
	std::optional<StreamError>      error;
};

enum class StreamConsumerReadHeaders
{
	READ_HEADERS,
	SKIP_HEADERS,

	DEFAULT = READ_HEADERS
};

class IStreamConsumerMessageBatch
{
public:
	ARQCore_API virtual ~IStreamConsumerMessageBatch() = default;

	ARQCore_API virtual size_t                         size()                    const = 0;
	ARQCore_API virtual bool                           empty()                   const = 0;
	ARQCore_API virtual StreamConsumerMessageView      at( const size_t index )  const = 0;

	struct Iterator
	{
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using value_type        = StreamConsumerMessageView;
		using pointer           = StreamConsumerMessageView*;
		using reference         = StreamConsumerMessageView;

		const IStreamConsumerMessageBatch* batch;
		size_t                             index;

		Iterator( const IStreamConsumerMessageBatch* b, size_t i )
			: batch( b ), index( i )
		{}

		bool operator!=( const Iterator& other ) const
		{
			return index != other.index;
		}

		bool operator==( const Iterator& other ) const
		{
			return index == other.index;
		}

		Iterator& operator++()
		{
			index++;
			return *this;
		}

		StreamConsumerMessageView operator*() const
		{
			return batch->at( index );
		}
	};

	
	// Support range-based for loops
	Iterator begin() const { return Iterator( this, 0 ); }
	Iterator end()   const { return Iterator( this, size() ); }
};

enum class StreamRebalanceEventType
{
	PARTITIONS_ASSIGNED,
	PARTITIONS_REVOKED
};

using StreamConsumerRebalanceCallbackFunc = std::function<void( StreamRebalanceEventType eventType, const std::set<StreamTopicPartition>& topicPartitions )>;

using StreamTopicPartitionOffsets = std::map<StreamTopicPartition, int64_t>;

using StreamConsumerOffsetCommitCallbackFunc = std::function<void( const StreamTopicPartitionOffsets& topicPartitionOffsets, const std::optional<StreamError>& error )>;

class IStreamConsumer
{
public:
	ARQCore_API virtual ~IStreamConsumer() = default;

	/**
	 * @brief Joins the Consumer Group and automatically handles partition assignment.
	 * @param topics The set of topics to subscribe to.
	 * @param callback Optional function called on partition assignment/revocation events.
	 * @param timeout Max time to block waiting for the subscribe to complete.
	 * @throws ARQException on failure.
	 * Triggers a rebalance if the group members change.
	 */
	ARQCore_API virtual void subscribe( const std::set<std::string>& topics, const StreamConsumerRebalanceCallbackFunc& callback = StreamConsumerRebalanceCallbackFunc(), const std::chrono::milliseconds timeout = 60s ) = 0;
	/**
	 * @brief Leaves the Consumer Group and stops fetching data.
	 * @param timeout Max time to block waiting for the unsubscribe to complete.
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual void unsubscribe( const std::chrono::milliseconds timeout = 60s ) = 0;

	/**
	 * @brief Fetches the next batch of messages from the broker.
	 * @param timeout Max time to block waiting for data.
	 * @param readHeaders Whether to read headers from the messages (reading headers has some overhead).
	 * @return A wrapper owning the memory of the message batch - size zero if no data.
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual std::unique_ptr<IStreamConsumerMessageBatch> poll( std::chrono::milliseconds timeout, const StreamConsumerReadHeaders readHeaders = StreamConsumerReadHeaders::DEFAULT ) = 0;

	// -----------------------------------------------------------------
	// Offset Management
	// -----------------------------------------------------------------

	/**
	 * @brief Synchronously commits the offsets of the last polled batch.
	 * Blocks until the broker acknowledges.
	 */
	ARQCore_API virtual void commitOffsets() = 0;
	/**
	 * @brief Asynchronously commits the offsets of the last polled batch.
	 * @param callback Optional function called on completion - guaranteed to be called (before closing the consumer).
	 */
	ARQCore_API virtual void commitOffsetsAsync( const StreamConsumerOffsetCommitCallbackFunc& callback = StreamConsumerOffsetCommitCallbackFunc() ) = 0;
	/**
	 * @brief Synchronously commits a specific message's offset (plus one).
	 * Useful for "At-Least-Once" processing where you process partially through a batch.
	 * @param msg The message whose offset is to be committed.
	 */
	ARQCore_API virtual void commitOffset( const StreamConsumerMessageView& msg ) = 0;
	/**
	 * @brief Asynchronously commits a specific message's offset (plus one).
	 * Useful for "At-Least-Once" processing where you process partially through a batch.
	 * @param msg The message whose offset is to be committed.
	 * @param callback Optional function called on completion - guaranteed to be called (before closing the consumer).
	 */
	ARQCore_API virtual void commitOffsetAsync( const StreamConsumerMessageView& msg, const StreamConsumerOffsetCommitCallbackFunc& callback = StreamConsumerOffsetCommitCallbackFunc() ) = 0;
	/**
	 * @brief Synchronously commits specific offsets for multiple partitions.
	 * @param topicPartitionOffsets The map of TopicPartition -> Offset to commit.
	 */
	ARQCore_API virtual void commitOffsets( const StreamTopicPartitionOffsets& topicPartitionOffsets ) = 0;
	/**
	 * @brief Asynchronously commits specific offsets for multiple partitions.
	 * @param topicPartitionOffsets The map of TopicPartition -> Offset to commit.
	 * @param callback Optional function called on completion - guaranteed to be called (before closing the consumer).
	 */
	ARQCore_API virtual void commitOffsetsAsync( const StreamTopicPartitionOffsets& topicPartitionOffsets, const StreamConsumerOffsetCommitCallbackFunc& callback = StreamConsumerOffsetCommitCallbackFunc() ) = 0;

	// -----------------------------------------------------------------
	// Flow Control
	// -----------------------------------------------------------------

	/**
	 * @brief Pauses fetching from assigned partitions without leaving the group.
	 * poll() will return zero records until resume() is called.
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual void pause() = 0;
	/**
	 * @brief Pauses fetching from specified partitions without leaving the group.
	 * @param partitions The set of partitions to pause.
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual void pause( const std::set<StreamTopicPartition>& partitions ) = 0;
	/**
	 * @brief Resumes fetching for paused partitions.
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual void resume() = 0;
	/**
	 * @brief Resumes fetching from specified partitions.
	 * @param partitions The set of partitions to resume fetching.
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual void resume( const std::set<StreamTopicPartition>& partitions ) = 0;

	// -----------------------------------------------------------------
	// Manual Assignment & Seeking (State Restoration)
	// -----------------------------------------------------------------

	/**
	 * @brief Manually assigns specific partitions to this consumer.
	 * WARNING: Disables automatic consumer group rebalancing.
	 * @throws ARQException on failure - will throw if subscribe was called previously (without a subsequent call to unsubscribe).
	 */
	ARQCore_API virtual void assign( const std::set<StreamTopicPartition>& partitions ) = 0;
	/**
	 * @brief Returns the current set of partitions assigned to this consumer.
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual std::set<StreamTopicPartition> getAssignment() = 0;

	/**
	 * @brief Seeks to a specific absolute offset.
	 * Offset will be used on the next poll().
	 * @throws ARQException on failure.
	 * @param timeout Max time to block waiting for the seek to complete.
	 */
	ARQCore_API virtual void seek( const StreamTopicPartition& partition, int64_t offset, std::chrono::milliseconds timeout = 60s ) = 0;
	/**
	 * @brief Seeks to the earliest offsets in the current partition assignment.
	 * Offsets will be used on the next poll().
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual void seekToBeginning( std::chrono::milliseconds timeout = 60s ) = 0;
	/**
	 * @brief Seeks to the earliest offset in the partitions.
	 * Offset will be used on the next poll().
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual void seekToBeginning( const std::set<StreamTopicPartition>& partitions, std::chrono::milliseconds timeout = 60s ) = 0;
	/**
	 * @brief Seeks to the latest offsets in the current partition assignment (live tail).
	 * Offsets will be used on the next poll().
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual void seekToEnd( std::chrono::milliseconds timeout = 60s ) = 0;
	/**
	 * @brief Seeks to the latest offset in the partitions (live tail).
	 * Offset will be used on the next poll().
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual void seekToEnd( const std::set<StreamTopicPartition>& partitions, std::chrono::milliseconds timeout = 60s ) = 0;

	/**
	 * @brief Returns the current local offset position (the next offset to be fetched).
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual int64_t position( const StreamTopicPartition& partition ) = 0;

	/**
	 * @brief Gets the opaque metadata required for transactional commits.
	 * Pass the result of this to Producer::sendOffsetsToTransaction.
	 * @throws ARQException on failure.
	 */
	ARQCore_API virtual StreamGroupMetadata getGroupMetadata() const = 0;
};

}

template<>
struct std::formatter<ARQ::StreamTopicPartition>
{
	constexpr auto parse( std::format_parse_context& ctx )
	{
		return ctx.begin();
	}

	template<typename FormatContext>
	auto format( const ARQ::StreamTopicPartition& p, FormatContext& ctx ) const
	{
		return std::format_to( ctx.out(), "{}:{}", p.first, p.second );
	}
};