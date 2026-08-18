#include "../src/misc_type_serialisers.h"

#include <ARQProtobuf/misc_type_proto_converters.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>

using namespace ARQ;
using ::testing::HasSubstr;

namespace
{

ID::UUID makeUUID( const uint8_t marker )
{
	ID::UUID id;
	id.bytes.back() = marker;
	return id;
}

}

TEST( MiscTypeSerialisersTest, CommandResponseRoundTripsWithAndWithoutOptionalMessage )
{
	Proto::RD::ProtobufTypeSerialiser_RDCommandResponse serialiser;
	RD::CommandResponse withMessage;
	withMessage.corrID  = makeUUID( 1 );
	withMessage.status  = RD::CommandResponse::REJECTED;
	withMessage.message = "Version mismatch";

	const Buffer withMessageBuffer = serialiser.serialise( withMessage );
	RD::CommandResponse withMessageOutput;
	serialiser.deserialise( withMessageBuffer, withMessageOutput );

	EXPECT_EQ( withMessageOutput.corrID, withMessage.corrID );
	EXPECT_EQ( withMessageOutput.status, withMessage.status );
	EXPECT_EQ( withMessageOutput.message, withMessage.message );

	RD::CommandResponse withoutMessage;
	withoutMessage.corrID = makeUUID( 2 );
	withoutMessage.status = RD::CommandResponse::SUCCESS;

	const Buffer withoutMessageBuffer = serialiser.serialise( withoutMessage );
	RD::CommandResponse withoutMessageOutput;
	serialiser.deserialise( withoutMessageBuffer, withoutMessageOutput );

	EXPECT_EQ( withoutMessageOutput.corrID, withoutMessage.corrID );
	EXPECT_EQ( withoutMessageOutput.status, withoutMessage.status );
	EXPECT_FALSE( withoutMessageOutput.message.has_value() );
}

TEST( MiscTypeSerialisersTest, CommandResponseReportsMalformedWireDataAtTheParseBoundary )
{
	Proto::RD::ProtobufTypeSerialiser_RDCommandResponse serialiser;
	constexpr std::array<uint8_t, 1> MALFORMED_PROTOBUF{ 0x80 };
	const BufferView malformed( MALFORMED_PROTOBUF.data(), MALFORMED_PROTOBUF.size() );

	try
	{
		RD::CommandResponse output;
		serialiser.deserialise( malformed, output );
		FAIL() << "Expected malformed Protobuf data to be rejected";
	}
	catch( const ARQException& e )
	{
		EXPECT_THAT( std::string( e.what() ), HasSubstr( "Cannot deserialise buffer into RefData CommandResponse" ) );
	}
}

TEST( MiscTypeSerialisersTest, CommandResponseRejectsUnknownStatusValues )
{
	Proto::RD::ProtobufTypeSerialiser_RDCommandResponse serialiser;
	Proto::RefDataCommandResponse protoResponse;
	protoResponse.mutable_corr_id()->set_id( makeUUID( 3 ).toString() );
	protoResponse.set_status( 999 );
	const std::string wireData = protoResponse.SerializeAsString();
	const Buffer buffer( wireData.data(), wireData.size() );

	RD::CommandResponse output;
	EXPECT_THROW( serialiser.deserialise( buffer, output ), ARQException );
}
