#include <ARQProtobuf/md_entity_proto_converters.h>
#include <gtest/gtest.h>

#include <ARQMarket/mktdata_entities.h>

using namespace ARQ::Proto::MD;

TEST( MktDataProtoConvertersTest, FXRate_Conversions )
{
    // --- LValue toProto ---
    ARQ::MD::FXRate cppFxRate;
    cppFxRate.mid = 1.105;
    cppFxRate.bid = 1.100;
    cppFxRate.ask = 1.110;

    ARQ::Proto::MD::FXRate protoFxRate;
    toProto( cppFxRate, &protoFxRate );

    EXPECT_DOUBLE_EQ( protoFxRate.mid(), 1.105 );
    EXPECT_DOUBLE_EQ( protoFxRate.bid(), 1.100 );
    EXPECT_DOUBLE_EQ( protoFxRate.ask(), 1.110 );

    // --- RValue toProto ---
    ARQ::Proto::MD::FXRate protoFxRateMove;
    toProto( std::move( cppFxRate ), &protoFxRateMove );
    EXPECT_DOUBLE_EQ( protoFxRateMove.mid(), 1.105 );

    // --- LValue fromProto ---
    ARQ::MD::FXRate parsedCppRate = fromProto( protoFxRate );
    EXPECT_DOUBLE_EQ( parsedCppRate.mid, 1.105 );
    EXPECT_DOUBLE_EQ( parsedCppRate.bid, 1.100 );
    EXPECT_DOUBLE_EQ( parsedCppRate.ask, 1.110 );

    // --- RValue fromProto ---
    ARQ::MD::FXRate parsedCppRateMove = fromProto( std::move( protoFxRateMove ) );
    EXPECT_DOUBLE_EQ( parsedCppRateMove.mid, 1.105 );
}

TEST( MktDataProtoConvertersTest, RecordHeader_Conversions )
{
    ARQ::MD::RecordHeader cppHeader;
    cppHeader.id = "SEQ-123";

    // --- LValue toProto ---
    ARQ::Proto::MD::RecordHeader protoHeader;
    toProto( cppHeader, &protoHeader );

    EXPECT_EQ( protoHeader.id(), "SEQ-123" );

    // --- RValue toProto ---
    ARQ::Proto::MD::RecordHeader protoHeaderMove;
    toProto( std::move( cppHeader ), &protoHeaderMove );
    EXPECT_EQ( protoHeaderMove.id(), "SEQ-123" );

    // --- LValue fromProto ---
    ARQ::MD::RecordHeader parsedHeader = fromProto( protoHeader );
    EXPECT_EQ( parsedHeader.id, "SEQ-123" );

    // --- RValue fromProto ---
    ARQ::MD::RecordHeader parsedHeaderMove = fromProto( std::move( protoHeaderMove ) );
    EXPECT_EQ( parsedHeaderMove.id, "SEQ-123" );
}

TEST( MktDataProtoConvertersTest, Record_FXRate_Conversions )
{
    ARQ::MD::Record<ARQ::MD::FXRate> cppRecord;
    cppRecord.header.id = "SEQ-123";
    cppRecord.data.mid = 1.505;
    cppRecord.data.bid = 1.500;

    // --- LValue toProto ---
    ARQ::Proto::MD::FXRateRecord protoRecord;
    toProto<ARQ::MD::FXRate>( cppRecord, &protoRecord );

    EXPECT_EQ( protoRecord.header().id(), "SEQ-123" );
    EXPECT_DOUBLE_EQ( protoRecord.data().mid(), 1.505 );

    // --- RValue toProto ---
    ARQ::Proto::MD::FXRateRecord protoRecordMove;
    toProto<ARQ::MD::FXRate>( std::move( cppRecord ), &protoRecordMove );
    EXPECT_DOUBLE_EQ( protoRecordMove.data().bid(), 1.500 );

    // --- LValue fromProto ---
    ARQ::MD::Record<ARQ::MD::FXRate> parsedRecord = fromProto<ARQ::MD::FXRate>( protoRecord );
    EXPECT_EQ( parsedRecord.header.id, "SEQ-123" );
    EXPECT_DOUBLE_EQ( parsedRecord.data.mid, 1.505 );

    // --- RValue fromProto ---
    ARQ::MD::Record<ARQ::MD::FXRate> parsedRecordMove = fromProto<ARQ::MD::FXRate>( std::move( protoRecordMove ) );
    EXPECT_DOUBLE_EQ( parsedRecordMove.data.bid, 1.500 );
}

TEST( MktDataProtoConvertersTest, RecordMessage_FXRate_Conversions )
{
    ARQ::MD::RecordMessage<ARQ::MD::FXRate> cppMessage;
    cppMessage.record.header.id = "MSG-456";
    cppMessage.record.data.mid = 1.605;
    cppMessage.mktName = "NYSE";

    // --- LValue toProto ---
    ARQ::Proto::MD::FXRateRecordMessage protoMessage;
    toProto<ARQ::MD::FXRate>( cppMessage, &protoMessage );

    EXPECT_EQ( protoMessage.record().header().id(), "MSG-456" );
    EXPECT_DOUBLE_EQ( protoMessage.record().data().mid(), 1.605 );
    EXPECT_EQ( protoMessage.mkt_name(), "NYSE" );

    // --- RValue toProto ---
    ARQ::Proto::MD::FXRateRecordMessage protoMessageMove;
    toProto<ARQ::MD::FXRate>( std::move( cppMessage ), &protoMessageMove );
    EXPECT_EQ( protoMessageMove.mkt_name(), "NYSE" );

    // --- LValue fromProto ---
    ARQ::MD::RecordMessage<ARQ::MD::FXRate> parsedMessage = fromProto<ARQ::MD::FXRate>( protoMessage );
    EXPECT_EQ( parsedMessage.record.header.id, "MSG-456" );
    EXPECT_EQ( parsedMessage.mktName, "NYSE" );

    // --- RValue fromProto ---
    ARQ::MD::RecordMessage<ARQ::MD::FXRate> parsedMessageMove = fromProto<ARQ::MD::FXRate>( std::move( protoMessageMove ) );
    EXPECT_EQ( parsedMessageMove.mktName, "NYSE" );
}

TEST( MktDataProtoConvertersTest, RecordCollection_Conversions )
{
    ARQ::MD::RecordCollection cppColl;

    // Add an FXRate record to the collection
    ARQ::MD::Record<ARQ::MD::FXRate> cppRecord;
    cppRecord.header.id = "BATCH-001";
    cppRecord.data.mid = 1.705;
    cppRecord.data.bid = 1.700;
    cppColl.get<ARQ::MD::Record<ARQ::MD::FXRate>>().push_back( cppRecord );

    // --- LValue toProto ---
    ARQ::Proto::MD::RecordCollection protoColl;
    toProto( cppColl, &protoColl );

    // Verify it was mapped to the correct repeated field via ProtoTraits
    ASSERT_EQ( protoColl.fx_rates_size(), 1 );
    EXPECT_EQ( protoColl.fx_rates( 0 ).header().id(), "BATCH-001" );
    EXPECT_DOUBLE_EQ( protoColl.fx_rates( 0 ).data().mid(), 1.705 );

    // --- RValue toProto ---
    ARQ::Proto::MD::RecordCollection protoCollMove;
    toProto( std::move( cppColl ), &protoCollMove );
    ASSERT_EQ( protoCollMove.fx_rates_size(), 1 );
    EXPECT_DOUBLE_EQ( protoCollMove.fx_rates( 0 ).data().bid(), 1.700 );

    // --- LValue fromProto ---
    ARQ::MD::RecordCollection parsedColl = fromProto( protoColl );
    ASSERT_EQ( parsedColl.get<ARQ::MD::Record<ARQ::MD::FXRate>>().size(), 1 );
    EXPECT_EQ( parsedColl.get<ARQ::MD::Record<ARQ::MD::FXRate>>()[0].header.id, "BATCH-001" );
    EXPECT_DOUBLE_EQ( parsedColl.get<ARQ::MD::Record<ARQ::MD::FXRate>>()[0].data.mid, 1.705 );

    // --- RValue fromProto ---
    ARQ::MD::RecordCollection parsedCollMove = fromProto( std::move( protoCollMove ) );
    ASSERT_EQ( parsedCollMove.get<ARQ::MD::Record<ARQ::MD::FXRate>>().size(), 1 );
    EXPECT_EQ( parsedCollMove.get<ARQ::MD::Record<ARQ::MD::FXRate>>()[0].header.id, "BATCH-001" );
    EXPECT_DOUBLE_EQ( parsedCollMove.get<ARQ::MD::Record<ARQ::MD::FXRate>>()[0].data.bid, 1.700 );
}

TEST( MktDataProtoConvertersTest, EQPrice_Conversions_Optional_Set )
{
    // --- LValue toProto ---
    ARQ::MD::EQPrice cppPrice;
    cppPrice.last = 150.50; // Assuming standard fields exist alongside vwap
    cppPrice.vwap = 150.25; // Optional is SET

    ARQ::Proto::MD::EQPrice protoPrice;
    toProto( cppPrice, &protoPrice );

    EXPECT_DOUBLE_EQ( protoPrice.last(), 150.50 );
    EXPECT_TRUE( protoPrice.has_vwap() );
    EXPECT_DOUBLE_EQ( protoPrice.vwap(), 150.25 );

    // --- RValue toProto ---
    ARQ::Proto::MD::EQPrice protoPriceMove;
    toProto( std::move( cppPrice ), &protoPriceMove );
    EXPECT_TRUE( protoPriceMove.has_vwap() );
    EXPECT_DOUBLE_EQ( protoPriceMove.vwap(), 150.25 );

    // --- LValue fromProto ---
    ARQ::MD::EQPrice parsedCppPrice = fromProto( protoPrice );
    EXPECT_DOUBLE_EQ( parsedCppPrice.last, 150.50 );
    EXPECT_TRUE( parsedCppPrice.vwap.has_value() );
    EXPECT_DOUBLE_EQ( parsedCppPrice.vwap.value(), 150.25 );

    // --- RValue fromProto ---
    ARQ::MD::EQPrice parsedCppPriceMove = fromProto( std::move( protoPriceMove ) );
    EXPECT_TRUE( parsedCppPriceMove.vwap.has_value() );
    EXPECT_DOUBLE_EQ( parsedCppPriceMove.vwap.value(), 150.25 );
}

TEST( MktDataProtoConvertersTest, EQPrice_Conversions_Optional_Empty )
{
    // --- LValue toProto ---
    ARQ::MD::EQPrice cppPrice;
    cppPrice.last = 150.50;
    cppPrice.vwap = std::nullopt; // Optional is EMPTY

    ARQ::Proto::MD::EQPrice protoPrice;
    toProto( cppPrice, &protoPrice );

    EXPECT_DOUBLE_EQ( protoPrice.last(), 150.50 );
    EXPECT_FALSE( protoPrice.has_vwap() ); // Should not be set in proto

    // --- RValue toProto ---
    ARQ::Proto::MD::EQPrice protoPriceMove;
    toProto( std::move( cppPrice ), &protoPriceMove );
    EXPECT_FALSE( protoPriceMove.has_vwap() );

    // --- LValue fromProto ---
    ARQ::MD::EQPrice parsedCppPrice = fromProto( protoPrice );
    EXPECT_DOUBLE_EQ( parsedCppPrice.last, 150.50 );
    EXPECT_FALSE( parsedCppPrice.vwap.has_value() ); // Should remain nullopt

    // --- RValue fromProto ---
    ARQ::MD::EQPrice parsedCppPriceMove = fromProto( std::move( protoPriceMove ) );
    EXPECT_FALSE( parsedCppPriceMove.vwap.has_value() );
}

TEST( MktDataProtoConvertersTest, Record_EQPrice_Conversions )
{
    ARQ::MD::Record<ARQ::MD::EQPrice> cppRecord;
    cppRecord.header.id = "SEQ-789";
    cppRecord.data.last = 200.00;
    cppRecord.data.vwap = 199.50; // Optional is SET

    // --- LValue toProto ---
    ARQ::Proto::MD::EQPriceRecord protoRecord;
    toProto<ARQ::MD::EQPrice>( cppRecord, &protoRecord );

    EXPECT_EQ( protoRecord.header().id(), "SEQ-789" );
    EXPECT_TRUE( protoRecord.data().has_vwap() );
    EXPECT_DOUBLE_EQ( protoRecord.data().vwap(), 199.50 );

    // --- LValue fromProto ---
    ARQ::MD::Record<ARQ::MD::EQPrice> parsedRecord = fromProto<ARQ::MD::EQPrice>( protoRecord );
    EXPECT_EQ( parsedRecord.header.id, "SEQ-789" );
    EXPECT_TRUE( parsedRecord.data.vwap.has_value() );
    EXPECT_DOUBLE_EQ( parsedRecord.data.vwap.value(), 199.50 );
}