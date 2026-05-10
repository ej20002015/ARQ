#include <ARQProtobuf/md_market_proto_converters.h>
#include <gtest/gtest.h>

using namespace ARQ::Proto::MD;

class MktDataBatchConvertersTest : public ::testing::Test
{
protected:
    // Helper to generate a populated batch for testing
    ARQ::MD::MarketUpdateBatch createPopulatedBatch()
    {
        ARQ::MD::MarketUpdateBatch cppBatch;
        cppBatch.marketName = ARQ::Mkt::Name( "NYSE" );

        // 1. Add an FXRate Record
        ARQ::MD::Record<ARQ::MD::FXRate> fxRecord;
        fxRecord.header.id = "GBPUSD";
        fxRecord.data.mid = 1.250;
        fxRecord.data.bid = 1.249;
        fxRecord.data.ask = 1.251;
        cppBatch.records.get<ARQ::MD::Record<ARQ::MD::FXRate>>().push_back( fxRecord );

        // 2. Add an EQPrice Record
        ARQ::MD::Record<ARQ::MD::EQPrice> eqRecord;
        eqRecord.header.id = "AAPL";
        eqRecord.data.last = 175.50;
        eqRecord.data.bid = 175.45;
        eqRecord.data.ask = 175.55;
        eqRecord.data.open = 174.00;
        eqRecord.data.close = 173.50;
        eqRecord.data.volume = 1500000;
        cppBatch.records.get<ARQ::MD::Record<ARQ::MD::EQPrice>>().push_back( eqRecord );

        // 3. Add Offsets 
        cppBatch.offsets[ARQ::StreamTopicPartition{ "NYSE.Equities", 0 }] = 10045;
        cppBatch.offsets[ARQ::StreamTopicPartition{ "NYSE.Equities", 1 }] = 20089;

        return cppBatch;
    }
};

TEST_F( MktDataBatchConvertersTest, MarketUpdateBatch_LValue_Conversions )
{
    ARQ::MD::MarketUpdateBatch cppBatch = createPopulatedBatch();

    // --- C++ to Proto (LValue) ---
    ARQ::Proto::MD::MarketUpdateBatch protoBatch;
    toProto( cppBatch, &protoBatch );

    // Verify Market Name
    EXPECT_EQ( protoBatch.mkt_name(), "NYSE" );

    // Verify Records were routed to the correct protobuf repeated fields
    ASSERT_EQ( protoBatch.records().fx_rates_size(), 1 );
    EXPECT_DOUBLE_EQ( protoBatch.records().fx_rates( 0 ).data().bid(), 1.249 );

    ASSERT_EQ( protoBatch.records().eq_prices_size(), 1 );
    EXPECT_DOUBLE_EQ( protoBatch.records().eq_prices( 0 ).data().last(), 175.50 );
    EXPECT_EQ( protoBatch.records().eq_prices( 0 ).data().volume(), 1500000 );

    // Verify Offsets map size (assuming Protobuf map semantics)
    EXPECT_EQ( protoBatch.offsets().offsets().size(), 2 );

    // --- Proto to C++ (LValue) ---
    ARQ::MD::MarketUpdateBatch parsedBatch = fromProto( protoBatch );

    EXPECT_EQ( parsedBatch.marketName.str(), "NYSE" );

    ASSERT_EQ( parsedBatch.records.get<ARQ::MD::Record<ARQ::MD::FXRate>>().size(), 1 );
    EXPECT_EQ( parsedBatch.records.get<ARQ::MD::Record<ARQ::MD::FXRate>>()[0].header.id, "GBPUSD" );
    EXPECT_DOUBLE_EQ( parsedBatch.records.get<ARQ::MD::Record<ARQ::MD::FXRate>>()[0].data.mid, 1.250 );
    ASSERT_EQ( parsedBatch.records.get<ARQ::MD::Record<ARQ::MD::EQPrice>>().size(), 1 );
    EXPECT_EQ( parsedBatch.records.get<ARQ::MD::Record<ARQ::MD::EQPrice>>()[0].header.id, "AAPL" );
    EXPECT_DOUBLE_EQ( parsedBatch.records.get<ARQ::MD::Record<ARQ::MD::EQPrice>>()[0].data.last, 175.50 );

    EXPECT_EQ( parsedBatch.offsets.size(), 2 );
}

TEST_F( MktDataBatchConvertersTest, MarketUpdateBatch_RValue_Conversions )
{
    ARQ::MD::MarketUpdateBatch cppBatch = createPopulatedBatch();

    // --- C++ to Proto (RValue) ---
    ARQ::Proto::MD::MarketUpdateBatch protoBatchMove;
    // Moving cppBatch here, so it should not be used afterward!
    toProto( std::move( cppBatch ), &protoBatchMove );

    EXPECT_EQ( protoBatchMove.mkt_name(), "NYSE" );
    ASSERT_EQ( protoBatchMove.records().eq_prices_size(), 1 );
    EXPECT_DOUBLE_EQ( protoBatchMove.records().eq_prices( 0 ).data().open(), 174.00 );
    EXPECT_EQ( protoBatchMove.offsets().offsets().size(), 2 );

    // --- Proto to C++ (RValue) ---
    ARQ::MD::MarketUpdateBatch parsedBatchMove = fromProto( std::move( protoBatchMove ) );
    EXPECT_EQ( parsedBatchMove.marketName.str(), "NYSE" );

    ASSERT_EQ( parsedBatchMove.records.get<ARQ::MD::Record<ARQ::MD::FXRate>>().size(), 1 );
    EXPECT_DOUBLE_EQ( parsedBatchMove.records.get<ARQ::MD::Record<ARQ::MD::FXRate>>()[0].data.ask, 1.251 );

    ASSERT_EQ( parsedBatchMove.records.get<ARQ::MD::Record<ARQ::MD::EQPrice>>().size(), 1 );
    EXPECT_DOUBLE_EQ( parsedBatchMove.records.get<ARQ::MD::Record<ARQ::MD::EQPrice>>()[0].data.close, 173.50 );

    EXPECT_EQ( parsedBatchMove.offsets.size(), 2 );
}