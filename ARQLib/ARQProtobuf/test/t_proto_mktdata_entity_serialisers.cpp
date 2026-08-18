#include "../src/proto_mktdata_entity_serialisers.h"

#include <gtest/gtest.h>

using namespace ARQ;

TEST( MktDataEntitySerialisersTest, FXRateRecordRoundTripsEveryField )
{
	Proto::MD::ProtobufTypeSerialiser_MDRecord<MD::FXRate> serialiser;
	MD::Record<MD::FXRate> input;
	input.header.id            = "GBPUSD";
	input.header.asofTs        = Time::DateTime( Time::Microseconds( 1'725'000'000'123'456 ) );
	input.header.isActive      = false;
	input.header.lastUpdatedTs = Time::DateTime( Time::Microseconds( 1'725'000'000'654'321 ) );
	input.header.lastUpdatedBy = "market-loader";
	input.data.mid             = 1.2500;
	input.data.bid             = 1.2495;
	input.data.ask             = 1.2505;

	const Buffer buffer = serialiser.serialise( input );
	MD::Record<MD::FXRate> output;
	serialiser.deserialise( buffer, output );

	EXPECT_EQ( output.header.id, input.header.id );
	EXPECT_EQ( output.header.asofTs, input.header.asofTs );
	EXPECT_EQ( output.header.isActive, input.header.isActive );
	EXPECT_EQ( output.header.lastUpdatedTs, input.header.lastUpdatedTs );
	EXPECT_EQ( output.header.lastUpdatedBy, input.header.lastUpdatedBy );
	EXPECT_DOUBLE_EQ( output.data.mid, input.data.mid );
	EXPECT_DOUBLE_EQ( output.data.bid, input.data.bid );
	EXPECT_DOUBLE_EQ( output.data.ask, input.data.ask );
}

TEST( MktDataEntitySerialisersTest, EQPriceRecordMessageRoundTripsEveryField )
{
	Proto::MD::ProtobufTypeSerialiser_MDRecordMessage<MD::EQPrice> serialiser;
	MD::RecordMessage<MD::EQPrice> input;
	input.mktName                     = "LSE";
	input.record.header.id            = "VOD.L";
	input.record.header.asofTs        = Time::DateTime( Time::Microseconds( 1'725'000'100'123'456 ) );
	input.record.header.isActive      = true;
	input.record.header.lastUpdatedTs = Time::DateTime( Time::Microseconds( 1'725'000'100'654'321 ) );
	input.record.header.lastUpdatedBy = "market-loader";
	input.record.data.last            = 72.14;
	input.record.data.bid             = 72.12;
	input.record.data.ask             = 72.16;
	input.record.data.open            = 71.80;
	input.record.data.close           = 71.95;
	input.record.data.volume          = 1'250'000;
	input.record.data.vwap            = 72.03;

	const Buffer buffer = serialiser.serialise( input );
	MD::RecordMessage<MD::EQPrice> output;
	serialiser.deserialise( buffer, output );

	EXPECT_EQ( output.mktName, input.mktName );
	EXPECT_EQ( output.record.header.id, input.record.header.id );
	EXPECT_EQ( output.record.header.asofTs, input.record.header.asofTs );
	EXPECT_EQ( output.record.header.isActive, input.record.header.isActive );
	EXPECT_EQ( output.record.header.lastUpdatedTs, input.record.header.lastUpdatedTs );
	EXPECT_EQ( output.record.header.lastUpdatedBy, input.record.header.lastUpdatedBy );
	EXPECT_DOUBLE_EQ( output.record.data.last, input.record.data.last );
	EXPECT_DOUBLE_EQ( output.record.data.bid, input.record.data.bid );
	EXPECT_DOUBLE_EQ( output.record.data.ask, input.record.data.ask );
	EXPECT_DOUBLE_EQ( output.record.data.open, input.record.data.open );
	EXPECT_DOUBLE_EQ( output.record.data.close, input.record.data.close );
	EXPECT_EQ( output.record.data.volume, input.record.data.volume );
	EXPECT_EQ( output.record.data.vwap, input.record.data.vwap );
}
