#include <ARQKafka/dll.h>

#include <ARQCore/streaming_service.h>

#include <string_view>

namespace ARQ
{

extern "C" ARQKafka_API IStreamProducer* createStreamProducer( const std::string_view dsh, const StreamProducerOptions& options );

}