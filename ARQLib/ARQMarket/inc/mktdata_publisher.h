#pragma once

#include <ARQUtils/os.h>
#include <ARQUtils/time.h>
#include <ARQUtils/enum.h>
#include <ARQCore/serialiser.h>
#include <ARQCore/streaming_service.h>
#include <ARQMarket/mktdata_entities.h>
#include <ARQMarket/mktdata_topics.h>
#include <ARQMarket/market.h>

#include <string>
#include <shared_mutex>

namespace ARQ::MD
{

template<c_MktData T>
struct RecordMessageTraits
{
	static std::string_view type() noexcept
	{
		static const std::string_view typeStr = std::format( "MD::RecordMessage<{}>", Traits<T>::name() );
		return typeStr;
	}
};

using PublisherErrorCallback = std::function<void( const StreamProducerMessageMetadata& messageMetadata, StreamError error )>;

class Publisher
{
public:
	struct Config
	{
		std::string                 streamingServiceDSH = "Kafka";
		std::shared_ptr<Serialiser> serialiser          = SerialiserFactory::inst().create( SerialiserFactory::SerialiserImpl::Protobuf );
		std::string                 updatedByStr        = std::format( "{0}-ARQ::MD::Publisher", OS::procName() );
	};

public:
	Publisher() = default;

	ARQMarket_API void init( const Config& config );

	ARQMarket_API void registerOnErrorCallback( const PublisherErrorCallback& callback );

	template<c_MktData T>
	void publish( const MarketName& mktName, Record<T> record, const std::optional<PublisherErrorCallback>& errorCallback = std::nullopt )
	{
		// Inject metadata into the record header
		record.header.lastUpdatedTs = Time::DateTime::nowUTC();
		if( record.header.lastUpdatedBy.empty() )
			record.header.lastUpdatedBy = m_config.updatedByStr;

		RecordMessage<T> msg = {
			.record  = record,
			.mktName = mktName.str()
		};

		Buffer      buf = m_config.serialiser->serialise( msg );
		std::string key = std::format( "{0}|{1}#{2}", mktName.str(), Traits<T>::type(), record.header.id );

		publishImpl( std::move( buf ), std::move( key ), Topics<T>::updateTopic(), RecordMessageTraits<T>::type(), errorCallback );
	}

	void flush() { m_streamProducer->flush(); }

private:
	ARQMarket_API void publishImpl( Buffer&& buf, const std::string key, const std::string_view topic, const std::string_view payloadType, const std::optional<PublisherErrorCallback>& errorCallback = std::nullopt );

	void handlePublishError( const StreamProducerMessageMetadata& messageMetadata, StreamError error, const std::optional<PublisherErrorCallback>& errorCallback );

private:
	 Config m_config;

	 std::shared_ptr<IStreamProducer> m_streamProducer;

	 std::vector<PublisherErrorCallback> m_globalErrorCallbacks;
	 std::shared_mutex                   m_globalErrorCallbacksMut;
};

}
