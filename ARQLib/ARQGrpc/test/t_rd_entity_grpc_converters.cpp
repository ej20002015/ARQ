#include <gtest/gtest.h>
#include <ARQGrpc/rd_entity_grpc_converters.h> // The header with your conversion functions

#include <ARQCore/refdata_entities.h>

using namespace ARQ;

// Helper function to create a sample ARQ Currency object for tests
RDEntities::Currency createSampleArqCurrency()
{
    RDEntities::Currency ccy;
    ccy._isActive = true;
    ccy._lastUpdatedTs = Time::DateTime::nowUTC();
    ccy._lastUpdatedBy = "test_user";
    ccy._version = 1;
    ccy.ccyID = "USD";
    ccy.name = "US Dollar";
    ccy.decimalPlaces = 2;
    ccy.settlementDays = 2;
    return ccy;
}

// Helper function to create a sample gRPC Currency object for tests
ARQ::Grpc::RefData::Currency createSampleGrpcCurrency()
{
    ARQ::Grpc::RefData::Currency grpcCcy;

    // Set top-level metadata fields
    grpcCcy.set__is_active( true );
    grpcCcy.set__last_updated_ts( 1672531200000000ULL ); // Same example timestamp
    grpcCcy.set__last_updated_by( "test_user" );
    grpcCcy.set__version( 1 );

    // Get a mutable pointer to the payload and set its fields
    ARQ::Grpc::RefData::CurrencyPayload* payload = grpcCcy.mutable_payload();
    payload->set_ccy_id( "USD" );
    payload->set_name( "US Dollar" );
    payload->set_decimal_places( 2 );
    payload->set_settlement_days( 2 );

    return grpcCcy;
}

// Helper function to verify the contents of a gRPC Currency object
void verifyGrpcCurrency( const ARQ::Grpc::RefData::Currency& grpcCcy, const RDEntities::Currency& arqCcy )
{
    ASSERT_EQ( grpcCcy._is_active(), arqCcy._isActive );
    ASSERT_EQ( grpcCcy._last_updated_ts(), arqCcy._lastUpdatedTs.microsecondsSinceEpoch() );
    ASSERT_EQ( grpcCcy._last_updated_by(), arqCcy._lastUpdatedBy );
    ASSERT_EQ( grpcCcy._version(), arqCcy._version );
    ASSERT_TRUE( grpcCcy.has_payload() );
    const auto& payload = grpcCcy.payload();
    ASSERT_EQ( payload.ccy_id(), arqCcy.ccyID );
    ASSERT_EQ( payload.name(), arqCcy.name );
    ASSERT_EQ( payload.decimal_places(), arqCcy.decimalPlaces );
    ASSERT_EQ( payload.settlement_days(), arqCcy.settlementDays );
}

// --- Test Cases ---

// Test conversion from ARQ struct to gRPC object using const lvalue reference
TEST( CurrencyConverterTest, ToGrpc_LValue )
{
    // Arrange
    const RDEntities::Currency arqCcy = createSampleArqCurrency();
    ARQ::Grpc::RefData::Currency grpcCcy;

    // Act
    ARQ::Grpc::RefData::toGrpc( arqCcy, &grpcCcy );

    // Assert
    verifyGrpcCurrency( grpcCcy, arqCcy );
}

// Test conversion from ARQ struct to gRPC object using rvalue reference (move semantics)
TEST( CurrencyConverterTest, ToGrpc_RValue )
{
    // Arrange
    RDEntities::Currency arqCcy = createSampleArqCurrency();
    // Keep a copy of the entity because the original will be moved from so can't be used for comparison
    const RDEntities::Currency arqCcyCopy = arqCcy;
    ARQ::Grpc::RefData::Currency grpcCcy;

    // Act
    ARQ::Grpc::RefData::toGrpc( std::move( arqCcy ), &grpcCcy );

    // Assert
    verifyGrpcCurrency( grpcCcy, arqCcyCopy );
}

// Test conversion from gRPC object to ARQ struct using const lvalue reference
TEST( CurrencyConverterTest, FromGrpc_LValue )
{
    // Arrange
    const ARQ::Grpc::RefData::Currency grpcCcy = createSampleGrpcCurrency();

    // Act
    const RDEntities::Currency arqCcy = ARQ::Grpc::RefData::fromGrpc( grpcCcy );

    // Assert
    ASSERT_EQ( arqCcy._isActive, grpcCcy._is_active() );
    ASSERT_EQ( arqCcy._lastUpdatedTs.microsecondsSinceEpoch(), grpcCcy._last_updated_ts() );
    ASSERT_EQ( arqCcy._lastUpdatedBy, grpcCcy._last_updated_by() );
    ASSERT_EQ( arqCcy._version, grpcCcy._version() );
    ASSERT_EQ( arqCcy.ccyID, grpcCcy.payload().ccy_id() );
    ASSERT_EQ( arqCcy.name, grpcCcy.payload().name() );
    ASSERT_EQ( arqCcy.decimalPlaces, grpcCcy.payload().decimal_places() );
    ASSERT_EQ( arqCcy.settlementDays, grpcCcy.payload().settlement_days() );
}

// Test conversion from gRPC object to ARQ struct using rvalue reference (move semantics)
TEST( CurrencyConverterTest, FromGrpc_RValue )
{
    // Arrange
    ARQ::Grpc::RefData::Currency grpcCcy = createSampleGrpcCurrency();
    // Keep a copy of the grpc object because the original will be moved from so can't be used for comparison
    const ARQ::Grpc::RefData::Currency grpcCcyCopy = grpcCcy;

    // Act
    const RDEntities::Currency arqCcy = ARQ::Grpc::RefData::fromGrpc( std::move( grpcCcy ) );

    // Assert
    ASSERT_EQ( arqCcy._isActive, grpcCcyCopy._is_active() );
    ASSERT_EQ( arqCcy._lastUpdatedTs.microsecondsSinceEpoch(), grpcCcyCopy._last_updated_ts() );
    ASSERT_EQ( arqCcy._lastUpdatedBy, grpcCcyCopy._last_updated_by() );
    ASSERT_EQ( arqCcy._version, grpcCcyCopy._version() );
    ASSERT_EQ( arqCcy.ccyID, grpcCcyCopy.payload().ccy_id() );
    ASSERT_EQ( arqCcy.name, grpcCcyCopy.payload().name() );
    ASSERT_EQ( arqCcy.decimalPlaces, grpcCcyCopy.payload().decimal_places() );
    ASSERT_EQ( arqCcy.settlementDays, grpcCcyCopy.payload().settlement_days() );
}

// Test a full round trip: ARQ -> gRPC -> ARQ
TEST( CurrencyConverterTest, RoundTripConversion )
{
    // Arrange: Create the original ARQ entity
    const RDEntities::Currency arqCcyOriginal = createSampleArqCurrency();
    ARQ::Grpc::RefData::Currency grpcCcy;

    // Act I: Convert from ARQ to gRPC
    ARQ::Grpc::RefData::toGrpc( arqCcyOriginal, &grpcCcy );

    // Act II: Convert from gRPC back to ARQ
    const RDEntities::Currency arqCcyFinal = ARQ::Grpc::RefData::fromGrpc( grpcCcy );

    // Member-by-member comparison (TODO: Define equality operators for the entities):
    ASSERT_EQ( arqCcyFinal._isActive, arqCcyOriginal._isActive );
    ASSERT_EQ( arqCcyFinal._lastUpdatedTs, arqCcyOriginal._lastUpdatedTs );
    ASSERT_EQ( arqCcyFinal._lastUpdatedBy, arqCcyOriginal._lastUpdatedBy );
    ASSERT_EQ( arqCcyFinal._version, arqCcyOriginal._version );
    ASSERT_EQ( arqCcyFinal.ccyID, arqCcyOriginal.ccyID );
    ASSERT_EQ( arqCcyFinal.name, arqCcyOriginal.name );
    ASSERT_EQ( arqCcyFinal.decimalPlaces, arqCcyOriginal.decimalPlaces );
    ASSERT_EQ( arqCcyFinal.settlementDays, arqCcyOriginal.settlementDays );
}

// Test a full round trip using move semantics: ARQ -> gRPC -> ARQ
TEST( CurrencyConverterTest, RoundTripConversion_MoveSemantics )
{
    // Arrange: Create the original ARQ entity and store its values for later comparison
    RDEntities::Currency arqCcyOriginal = createSampleArqCurrency();
    const RDEntities::Currency arqCcyOriginalCopy = arqCcyOriginal;
    ARQ::Grpc::RefData::Currency grpcCcy;

    // Act I: Convert from ARQ to gRPC using move
    ARQ::Grpc::RefData::toGrpc( std::move( arqCcyOriginal ), &grpcCcy );

    // Act II: Convert from gRPC back to ARQ using move
    const RDEntities::Currency arqCcyFinal = ARQ::Grpc::RefData::fromGrpc( std::move( grpcCcy ) );

    // Member-by-member comparison (TODO: Define equality operators for the entities):
    ASSERT_EQ( arqCcyFinal._isActive, arqCcyOriginalCopy._isActive );
    ASSERT_EQ( arqCcyFinal._lastUpdatedTs, arqCcyOriginalCopy._lastUpdatedTs );
    ASSERT_EQ( arqCcyFinal._lastUpdatedBy, arqCcyOriginalCopy._lastUpdatedBy );
    ASSERT_EQ( arqCcyFinal._version, arqCcyOriginalCopy._version );
    ASSERT_EQ( arqCcyFinal.ccyID, arqCcyOriginalCopy.ccyID );
    ASSERT_EQ( arqCcyFinal.name, arqCcyOriginalCopy.name );
    ASSERT_EQ( arqCcyFinal.decimalPlaces, arqCcyOriginalCopy.decimalPlaces );
    ASSERT_EQ( arqCcyFinal.settlementDays, arqCcyOriginalCopy.settlementDays );
}