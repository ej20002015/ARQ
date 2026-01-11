#include <ARQKafka/kafka_streaming_service.h>

#include <ARQKafka/kafka_streaming_service_interface.h>

namespace ARQ
{

IStreamProducer* createStreamProducer( const std::string_view dsh, const StreamProducerOptions& options )
{
	return new KafkaStreamProducer( dsh, options );
}

IStreamConsumer* createStreamConsumer( const std::string_view dsh, const StreamConsumerOptions& options )
{
	return new KafkaStreamConsumer( dsh, options );
}

}
