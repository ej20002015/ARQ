#include "../src/proto_mktdata_market_serialisers.h"

#include <gtest/gtest.h>

using namespace ARQ;

TEST( MktDataMarketSerialisersTest, MarketUpdateBatchRoundTripsEveryField )
{
	MD::MarketUpdateBatch input;
	input.marketName     = MD::MarketName( "NYSE" );
	input.sourcePosition = { StreamTopicPartition{ "NYSE.Equities", 0 }, 10045 };

	MD::Record<MD::FXRate> fxRecord;
	fxRecord.header.id = "GBPUSD";
	fxRecord.data.mid  = 1.250;
	fxRecord.data.bid  = 1.249;
	fxRecord.data.ask  = 1.251;
	input.records.get<MD::Record<MD::FXRate>>().push_back( fxRecord );

	MD::Record<MD::EQPrice> eqRecord;
	eqRecord.header.id   = "AAPL";
	eqRecord.data.last   = 175.50;
	eqRecord.data.bid    = 175.45;
	eqRecord.data.ask    = 175.55;
	eqRecord.data.open   = 174.00;
	eqRecord.data.close  = 173.50;
	eqRecord.data.volume = 1'500'000;
	input.records.get<MD::Record<MD::EQPrice>>().push_back( eqRecord );

	Proto::MD::ProtobufTypeSerialiser_MarketUpdateBatch serialiser;
	const Buffer buffer = serialiser.serialise( input );
	MD::MarketUpdateBatch output;
	serialiser.deserialise( buffer, output );

	EXPECT_EQ( output.marketName, input.marketName );
	EXPECT_EQ( output.sourcePosition.tp, input.sourcePosition.tp );
	EXPECT_EQ( output.sourcePosition.offset, input.sourcePosition.offset );

	const auto& fxOutput = output.records.get<MD::Record<MD::FXRate>>();
	ASSERT_EQ( fxOutput.size(), 1 );
	EXPECT_EQ( fxOutput.front().header.id, fxRecord.header.id );
	EXPECT_DOUBLE_EQ( fxOutput.front().data.mid, fxRecord.data.mid );

	const auto& eqOutput = output.records.get<MD::Record<MD::EQPrice>>();
	ASSERT_EQ( eqOutput.size(), 1 );
	EXPECT_EQ( eqOutput.front().header.id, eqRecord.header.id );
	EXPECT_DOUBLE_EQ( eqOutput.front().data.last, eqRecord.data.last );
}
