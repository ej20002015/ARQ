#include <ARQMarket/mktdata_publisher.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ARQ;
using namespace ARQ::MD;

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

namespace
{

class MockStreamProducer : public IStreamProducer
{
public:
	MOCK_METHOD( void, send, ( const StreamProducerMessage&, const StreamProducerDeliveryCallbackFunc& ), ( override ) );
	MOCK_METHOD( void, flush, ( const std::chrono::milliseconds ), ( override ) );
	MOCK_METHOD( void, initTransactions, ( const std::chrono::milliseconds ), ( override ) );
	MOCK_METHOD( void, beginTransaction, (), ( override ) );
	MOCK_METHOD( void, commitTransaction, ( const std::chrono::milliseconds ), ( override ) );
	MOCK_METHOD( void, abortTransaction, ( const std::chrono::milliseconds ), ( override ) );
	MOCK_METHOD( void, sendOffsetsToTransaction, ( const StreamTopicPartitionOffsets&, const StreamGroupMetadata&, const std::chrono::milliseconds ), ( override ) );
};

class FXRateRecordMessageSerialiser : public ISerialisableType<RecordMessage<FXRate>>
{
public:
	Buffer serialise( const RecordMessage<FXRate>& ) const override
	{
		static constexpr std::string_view bytes = "serialised-observation";
		return Buffer( bytes.data(), bytes.size() );
	}

	void deserialise( const BufferView, RecordMessage<FXRate>& ) const override {}
};

class MktDataPublisherTest : public ::testing::Test
{
protected:
	static constexpr std::string_view TEST_DSH = "MktDataPublisherTest";

	void SetUp() override
	{
		m_streamProducer = std::make_shared<NiceMock<MockStreamProducer>>();
		StreamingServiceFactory::inst().addCustomStreamProducer( TEST_DSH, m_streamProducer );

		m_serialiser = std::make_shared<Serialiser>();
		m_serialiser->registerHandler<RecordMessage<FXRate>>( std::make_unique<FXRateRecordMessageSerialiser>() );

		Publisher::Config config{
			.streamingServiceDSH = std::string( TEST_DSH ),
			.serialiser          = m_serialiser
		};
		m_publisher.init( config );
	}

	void TearDown() override
	{
		StreamingServiceFactory::inst().delCustomStreamProducer( TEST_DSH );
	}

	std::shared_ptr<NiceMock<MockStreamProducer>> m_streamProducer;
	std::shared_ptr<Serialiser>                   m_serialiser;
	Publisher                                     m_publisher;
};

}

TEST( MktDataTopicsTest, MapsEntityTypesToObservationAndCurrentTopics )
{
	EXPECT_EQ( Topics<FXRate>::observationTopic(), "ARQ.MktData.Observations.FXR" );
	EXPECT_EQ( Topics<FXRate>::currentTopic(), "ARQ.MktData.Current.FXR" );
	EXPECT_EQ( getObservationTopic( "EQPrice" ), "ARQ.MktData.Observations.EQP" );
	EXPECT_EQ( getCurrentTopic( Type::EQP ), "ARQ.MktData.Current.EQP" );

	EXPECT_EQ( getTypeFromObservationTopic( "ARQ.MktData.Observations.FXR" ), Type::FXR );
	EXPECT_EQ( getTypeFromCurrentTopic( "ARQ.MktData.Current.EQP" ), Type::EQP );
}

TEST( MktDataTopicsTest, RejectsTopicsFromTheWrongStreamFamily )
{
	EXPECT_THROW( getTypeFromObservationTopic( "ARQ.MktData.Current.FXR" ), ARQException );
	EXPECT_THROW( getTypeFromCurrentTopic( "ARQ.MktData.Observations.FXR" ), ARQException );
}

TEST_F( MktDataPublisherTest, PublishesFeedRecordsAsAuthoritativeObservations )
{
	StreamProducerMessage publishedMessage;
	EXPECT_CALL( *m_streamProducer, send( _, _ ) )
		.WillOnce( Invoke( [&publishedMessage] ( const StreamProducerMessage& message, const StreamProducerDeliveryCallbackFunc& )
		{
			publishedMessage = message;
		} ) );

	Record<FXRate> record;
	record.header.id     = "EUR";
	record.header.asofTs = Time::DateTime( Time::Microseconds( 123 ) );
	record.data          = FXRate{ .mid = 1.1, .bid = 1.09, .ask = 1.11 };

	m_publisher.publish( MarketName::LIVE, std::move( record ) );

	EXPECT_EQ( publishedMessage.topic, "ARQ.MktData.Observations.FXR" );
	ASSERT_TRUE( publishedMessage.key.has_value() );
	EXPECT_EQ( *publishedMessage.key, "LIVE|FXR#EUR" );
	EXPECT_EQ( publishedMessage.headers.at( "ARQ_Type" ), "MD::RecordMessage<FXRate>" );
}
