#include "../src/proto_refdata_entity_serialisers.h"

#include <ARQCore/refdata_entities.h>

#include <gtest/gtest.h>

using namespace ARQ;

namespace
{

ID::UUID makeUUID( const uint8_t marker )
{
	ID::UUID id;
	id.bytes.back() = marker;
	return id;
}

}

TEST( RefDataEntitySerialisersTest, CurrencyRecordRoundTripsEveryField )
{
	Proto::RD::ProtobufTypeSerialiser_RDRecord<RD::Currency> serialiser;
	RD::Record<RD::Currency> input;
	input.header.uuid          = makeUUID( 1 );
	input.header.isActive      = false;
	input.header.lastUpdatedTs = Time::DateTime( Time::Microseconds( 1'725'000'000'123'456 ) );
	input.header.lastUpdatedBy = "alice";
	input.header.version       = 7;
	input.data.uuid            = input.header.uuid;
	input.data.ccyID           = "GBP";
	input.data.name            = "Pound Sterling";
	input.data.decimalPlaces   = 2;
	input.data.settlementDays  = 2;

	const Buffer buffer = serialiser.serialise( input );
	RD::Record<RD::Currency> output;
	serialiser.deserialise( buffer, output );

	EXPECT_EQ( output.header.uuid, input.header.uuid );
	EXPECT_EQ( output.header.isActive, input.header.isActive );
	EXPECT_EQ( output.header.lastUpdatedTs, input.header.lastUpdatedTs );
	EXPECT_EQ( output.header.lastUpdatedBy, input.header.lastUpdatedBy );
	EXPECT_EQ( output.header.version, input.header.version );
	EXPECT_EQ( output.data.uuid, input.data.uuid );
	EXPECT_EQ( output.data.ccyID, input.data.ccyID );
	EXPECT_EQ( output.data.name, input.data.name );
	EXPECT_EQ( output.data.decimalPlaces, input.data.decimalPlaces );
	EXPECT_EQ( output.data.settlementDays, input.data.settlementDays );
}

TEST( RefDataEntitySerialisersTest, UserRecordPreservesOptionalFieldPresence )
{
	Proto::RD::ProtobufTypeSerialiser_RDRecord<RD::User> serialiser;
	RD::Record<RD::User> input;
	input.header.uuid          = makeUUID( 2 );
	input.header.isActive      = true;
	input.header.lastUpdatedTs = Time::DateTime( Time::Microseconds( 1'725'000'000'654'321 ) );
	input.header.lastUpdatedBy = "bob";
	input.header.version       = 3;
	input.data.uuid            = input.header.uuid;
	input.data.userID          = "bwayne";
	input.data.fullName        = "Bruce Wayne";
	input.data.email           = "bruce@example.test";
	input.data.tradingDesk     = std::nullopt;

	const Buffer buffer = serialiser.serialise( input );
	RD::Record<RD::User> output;
	serialiser.deserialise( buffer, output );

	EXPECT_EQ( output.header.uuid, input.header.uuid );
	EXPECT_EQ( output.header.lastUpdatedTs, input.header.lastUpdatedTs );
	EXPECT_EQ( output.header.version, input.header.version );
	EXPECT_EQ( output.data.uuid, input.data.uuid );
	EXPECT_EQ( output.data.userID, input.data.userID );
	EXPECT_EQ( output.data.fullName, input.data.fullName );
	EXPECT_EQ( output.data.email, input.data.email );
	EXPECT_FALSE( output.data.tradingDesk.has_value() );
}
