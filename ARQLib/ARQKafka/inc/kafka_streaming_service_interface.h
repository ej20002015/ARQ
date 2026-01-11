#include <ARQKafka/dll.h>

#include <ARQCore/streaming_service.h>

#include <kafka/KafkaProducer.h>
#include <kafka/KafkaConsumer.h>

namespace ARQ
{

class KafkaStreamProducer : public IStreamProducer
{
public:
	ARQKafka_API KafkaStreamProducer( const std::string_view dsh, const StreamProducerOptions& options );

public: // IStreamProducer implementation
	ARQKafka_API void send( const StreamProducerMessage& msg, const StreamProducerDeliveryCallbackFunc& callback = StreamProducerDeliveryCallbackFunc() ) override;
	ARQKafka_API void flush( const std::chrono::milliseconds timeout = 10'000ms )                                                                         override;

	// Transactional API (EOS)
	ARQKafka_API void initTransactions( std::chrono::milliseconds timeout = 60s )                                                                                                           override;
	ARQKafka_API void beginTransaction()                                                                                                                                                    override;
	ARQKafka_API void commitTransaction( std::chrono::milliseconds timeout = 60s )                                                                                                          override;
	ARQKafka_API void abortTransaction( std::chrono::milliseconds timeout = 60s )                                                                                                           override;
	ARQKafka_API void sendOffsetsToTransaction( const std::map<StreamTopicPartition, int64_t>& offsets, const StreamGroupMetadata& groupMetadata, std::chrono::milliseconds timeout = 60s ) override;

private: // Connection
	void              connect();
	kafka::Properties buildProperties();

private:
	std::string           m_dsh;
	StreamProducerOptions m_options;

	std::unique_ptr<kafka::clients::producer::KafkaProducer> m_kafkaProducer;
};

class KafkaStreamConsumerMessageBatch : public IStreamConsumerMessageBatch
{
public:
	KafkaStreamConsumerMessageBatch( std::vector<kafka::clients::consumer::ConsumerRecord>&& records, const StreamConsumerReadHeaders readHeaders )
		: m_records( std::move( records ) )
		, m_readHeaders( readHeaders )
	{
		m_topicsCache.resize( m_records.size() );
		if( m_readHeaders == StreamConsumerReadHeaders::READ_HEADERS )
			m_headersCache.resize( m_records.size() );
	}

public: // IStreamConsumerMessageBatch implementation
	ARQKafka_API size_t                    size()                   const override { return m_records.size(); }
	ARQKafka_API bool                      empty()                  const override { return m_records.empty(); }
	ARQKafka_API StreamConsumerMessageView at( const size_t index ) const override;

private:
	std::vector<kafka::clients::consumer::ConsumerRecord> m_records;
	mutable std::vector<std::string>                      m_topicsCache;
	StreamConsumerReadHeaders                             m_readHeaders;
	mutable std::vector<kafka::Headers>                   m_headersCache;
};

class KafkaStreamConsumer : public IStreamConsumer
{
public:
	ARQKafka_API KafkaStreamConsumer( const std::string_view dsh, const StreamConsumerOptions& options );

public: // IStreamConsumer implementation
	// Standard Consumption
	ARQKafka_API void                                         subscribe( const std::set<std::string>& topics, const StreamConsumerRebalanceCallbackFunc& callback, const std::chrono::milliseconds timeout ) override;
	ARQKafka_API void                                         unsubscribe( const std::chrono::milliseconds timeout )                                                                                         override;
	ARQKafka_API std::unique_ptr<IStreamConsumerMessageBatch> poll( std::chrono::milliseconds timeout, const StreamConsumerReadHeaders readHeaders = StreamConsumerReadHeaders::DEFAULT )                    override;

	// Offset Management
	ARQKafka_API void commitOffsets()                                                                                                                                                                   override;
	ARQKafka_API void commitOffsetsAsync( const StreamConsumerOffsetCommitCallbackFunc& callback = StreamConsumerOffsetCommitCallbackFunc() )                                                           override;
	ARQKafka_API void commitOffset( const StreamConsumerMessageView& msg )                                                                                                                              override;
	ARQKafka_API void commitOffsetAsync( const StreamConsumerMessageView& msg, const StreamConsumerOffsetCommitCallbackFunc& callback = StreamConsumerOffsetCommitCallbackFunc() )                      override;
	ARQKafka_API void commitOffsets( const StreamTopicPartitionOffsets& topicPartitionOffsets )                                                                                                         override;
	ARQKafka_API void commitOffsetsAsync( const StreamTopicPartitionOffsets& topicPartitionOffsets, const StreamConsumerOffsetCommitCallbackFunc& callback = StreamConsumerOffsetCommitCallbackFunc() ) override;

	// Flow Control
	ARQKafka_API void pause()  override;
	ARQKafka_API void pause( const std::set<StreamTopicPartition>& partitions ) override;
	ARQKafka_API void resume() override;
	ARQKafka_API void resume( const std::set<StreamTopicPartition>& partitions ) override;

	// Manual Assignment & Seeking (State Restoration)
	ARQKafka_API void                           assign( const std::set<StreamTopicPartition>& partitions )                                                   override;
	ARQKafka_API std::set<StreamTopicPartition> getAssignment()                                                                                              override;
	ARQKafka_API void                           seek( const StreamTopicPartition& partition, int64_t offset, std::chrono::milliseconds timeout = 60s )       override;
	ARQKafka_API void                           seekToBeginning( std::chrono::milliseconds timeout = 60s )                                                   override;
	ARQKafka_API void                           seekToBeginning( const std::set<StreamTopicPartition>& partitions, std::chrono::milliseconds timeout = 60s ) override;
	ARQKafka_API void                           seekToEnd( std::chrono::milliseconds timeout = 60s )                                                         override;
	ARQKafka_API void                           seekToEnd( const std::set<StreamTopicPartition>& partitions, std::chrono::milliseconds timeout = 60s )       override;
	ARQKafka_API int64_t                        position( const StreamTopicPartition& partition )                                                            override;

	// Group Metadata
	ARQKafka_API StreamGroupMetadata getGroupMetadata() const override;

private: // Connection
	void              connect();
	kafka::Properties buildProperties();

private:
	std::string           m_dsh;
	StreamConsumerOptions m_options;

	std::unique_ptr<kafka::clients::consumer::KafkaConsumer> m_kafkaConsumer;
};

}