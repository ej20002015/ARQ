#include "../src/proto_refdata_command_serialisers.h"

#include <ARQCore/refdata_commands.h>

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

TEST( RefDataCommandSerialisersTest, UpsertCommandsRoundTripForEveryEntity )
{
	Proto::RD::Cmd::ProtobufTypeSerialiser_RDCmdUpsert<RD::Currency> currencySerialiser;
	RD::Cmd::Upsert<RD::Currency> currencyInput;
	currencyInput.targetUUID          = makeUUID( 1 );
	currencyInput.data.uuid           = currencyInput.targetUUID;
	currencyInput.data.ccyID          = "GBP";
	currencyInput.data.name           = "Pound Sterling";
	currencyInput.data.decimalPlaces  = 2;
	currencyInput.data.settlementDays = 2;
	currencyInput.updatedBy           = "alice";
	currencyInput.expectedVersion     = 4;

	const Buffer currencyBuffer = currencySerialiser.serialise( currencyInput );
	RD::Cmd::Upsert<RD::Currency> currencyOutput;
	currencySerialiser.deserialise( currencyBuffer, currencyOutput );

	EXPECT_EQ( currencyOutput.targetUUID, currencyInput.targetUUID );
	EXPECT_EQ( currencyOutput.data.uuid, currencyInput.data.uuid );
	EXPECT_EQ( currencyOutput.data.ccyID, currencyInput.data.ccyID );
	EXPECT_EQ( currencyOutput.data.name, currencyInput.data.name );
	EXPECT_EQ( currencyOutput.data.decimalPlaces, currencyInput.data.decimalPlaces );
	EXPECT_EQ( currencyOutput.data.settlementDays, currencyInput.data.settlementDays );
	EXPECT_EQ( currencyOutput.updatedBy, currencyInput.updatedBy );
	EXPECT_EQ( currencyOutput.expectedVersion, currencyInput.expectedVersion );

	RD::Cmd::Upsert<RD::User> userInput;
	userInput.targetUUID       = makeUUID( 2 );
	userInput.data.uuid        = userInput.targetUUID;
	userInput.data.userID      = "bwayne";
	userInput.data.fullName    = "Bruce Wayne";
	userInput.data.email       = "bruce@example.test";
	userInput.data.tradingDesk = "FX";
	userInput.updatedBy        = "bob";
	userInput.expectedVersion  = 9;

	Proto::RD::Cmd::ProtobufTypeSerialiser_RDCmdUpsert<RD::User> userSerialiser;
	const Buffer userBuffer = userSerialiser.serialise( userInput );
	RD::Cmd::Upsert<RD::User> userOutput;
	userSerialiser.deserialise( userBuffer, userOutput );

	EXPECT_EQ( userOutput.targetUUID, userInput.targetUUID );
	EXPECT_EQ( userOutput.data.uuid, userInput.data.uuid );
	EXPECT_EQ( userOutput.data.userID, userInput.data.userID );
	EXPECT_EQ( userOutput.data.fullName, userInput.data.fullName );
	EXPECT_EQ( userOutput.data.email, userInput.data.email );
	EXPECT_EQ( userOutput.data.tradingDesk, userInput.data.tradingDesk );
	EXPECT_EQ( userOutput.updatedBy, userInput.updatedBy );
	EXPECT_EQ( userOutput.expectedVersion, userInput.expectedVersion );
}

TEST( RefDataCommandSerialisersTest, DeactivateCommandsRoundTripForEveryEntity )
{
	Proto::RD::Cmd::ProtobufTypeSerialiser_RDCmdDeactivate<RD::Currency> currencySerialiser;
	RD::Cmd::Deactivate<RD::Currency> currencyInput;
	currencyInput.targetUUID      = makeUUID( 3 );
	currencyInput.updatedBy       = "alice";
	currencyInput.expectedVersion = 11;

	const Buffer currencyBuffer = currencySerialiser.serialise( currencyInput );
	RD::Cmd::Deactivate<RD::Currency> currencyOutput;
	currencySerialiser.deserialise( currencyBuffer, currencyOutput );

	EXPECT_EQ( currencyOutput.targetUUID, currencyInput.targetUUID );
	EXPECT_EQ( currencyOutput.updatedBy, currencyInput.updatedBy );
	EXPECT_EQ( currencyOutput.expectedVersion, currencyInput.expectedVersion );

	RD::Cmd::Deactivate<RD::User> userInput;
	userInput.targetUUID      = makeUUID( 4 );
	userInput.updatedBy       = "bob";
	userInput.expectedVersion = 12;

	Proto::RD::Cmd::ProtobufTypeSerialiser_RDCmdDeactivate<RD::User> userSerialiser;
	const Buffer userBuffer = userSerialiser.serialise( userInput );
	RD::Cmd::Deactivate<RD::User> userOutput;
	userSerialiser.deserialise( userBuffer, userOutput );

	EXPECT_EQ( userOutput.targetUUID, userInput.targetUUID );
	EXPECT_EQ( userOutput.updatedBy, userInput.updatedBy );
	EXPECT_EQ( userOutput.expectedVersion, userInput.expectedVersion );
}
