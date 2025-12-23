#include <ARQkafka/dll.h>

#include <ARQCore/streaming_service.h>

#include <kafka/KafkaProducer.h>

namespace ARQ
{

class KafkaStreamProducer : public IStreamProducer
{
public:
	ARQKafka_API KafkaStreamProducer( const std::string_view dsh, const StreamProducerOptions& options );

public: // IStreamProducer implementation
	ARQKafka_API void send( const StreamProducerMessage& msg, const StreamProducerDeliveryCallbackFunc& callback = StreamProducerDeliveryCallbackFunc() ) override;
	ARQKafka_API void flush( const std::chrono::milliseconds timeout = 10'000ms )                                                                         override;

private: // Connection
	void              connect();
	kafka::Properties buildProperties();

private:
	std::string           m_dsh;
	StreamProducerOptions m_options;

	std::unique_ptr<kafka::clients::producer::KafkaProducer> m_kafkaProducer;
};

}