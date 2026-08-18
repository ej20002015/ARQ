#include <ARQCore/refdata_command_processor.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <variant>

using namespace ARQ;
using namespace ARQ::RD;

namespace
{

ID::UUID makeUUID( const uint8_t marker )
{
	ID::UUID uuid;
	uuid.bytes.back() = marker;
	return uuid;
}

Time::DateTime makeUpdateTime()
{
	return Time::DateTime( Time::Microseconds( 1'234'567 ) );
}

Currency makeCurrency( const ID::UUID& uuid )
{
	return Currency{
		.uuid           = uuid,
		.ccyID          = "GBP",
		.name           = "Pound Sterling",
		.decimalPlaces  = 2,
		.settlementDays = 2
	};
}

Record<Currency> makeCurrencyRecord( const ID::UUID& uuid, const uint32_t version, const bool isActive = true )
{
	return Record<Currency>{
		.header = {
			.uuid          = uuid,
			.isActive      = isActive,
			.lastUpdatedTs = Time::DateTime( Time::Microseconds( 100 ) ),
			.lastUpdatedBy = "previous-user",
			.version       = version
		},
		.data = makeCurrency( uuid )
	};
}

}

TEST( RefDataCommandProcessorTest, CreatesFirstActiveVersionForNewUpsert )
{
	const ID::UUID uuid = makeUUID( 1 );
	const Cmd::Upsert<Currency> command{
		.targetUUID      = uuid,
		.data            = makeCurrency( uuid ),
		.updatedBy       = "test-user",
		.expectedVersion = 0
	};
	const Time::DateTime updateTime = makeUpdateTime();

	const CommandDecision<Currency> decision = RefDataCommandProcessor::process( command, std::nullopt, updateTime );

	const auto* record = std::get_if<Record<Currency>>( &decision );
	ASSERT_NE( record, nullptr );
	EXPECT_EQ( record->header.uuid, uuid );
	EXPECT_TRUE( record->header.isActive );
	EXPECT_EQ( record->header.lastUpdatedTs, updateTime );
	EXPECT_EQ( record->header.lastUpdatedBy, "test-user" );
	EXPECT_EQ( record->header.version, 1 );
	EXPECT_EQ( record->data.uuid, uuid );
	EXPECT_EQ( record->data.ccyID, "GBP" );
	EXPECT_EQ( record->data.name, "Pound Sterling" );
	EXPECT_EQ( record->data.decimalPlaces, 2 );
	EXPECT_EQ( record->data.settlementDays, 2 );
}

TEST( RefDataCommandProcessorTest, AdvancesVersionForExistingUpsertWithExpectedVersion )
{
	const ID::UUID uuid = makeUUID( 2 );
	const Cmd::Upsert<Currency> command{
		.targetUUID      = uuid,
		.data            = makeCurrency( uuid ),
		.updatedBy       = "test-user",
		.expectedVersion = 7
	};

	const CommandDecision<Currency> decision = RefDataCommandProcessor::process( command, 7, makeUpdateTime() );

	const auto* record = std::get_if<Record<Currency>>( &decision );
	ASSERT_NE( record, nullptr );
	EXPECT_EQ( record->header.version, 8 );
}

TEST( RefDataCommandProcessorTest, RejectsNewUpsertWithNonzeroExpectedVersion )
{
	const ID::UUID uuid = makeUUID( 3 );
	const Cmd::Upsert<Currency> command{
		.targetUUID      = uuid,
		.data            = makeCurrency( uuid ),
		.updatedBy       = "test-user",
		.expectedVersion = 1
	};

	const CommandDecision<Currency> decision = RefDataCommandProcessor::process( command, std::nullopt, makeUpdateTime() );

	const auto* rejection = std::get_if<CommandRejection>( &decision );
	ASSERT_NE( rejection, nullptr );
	EXPECT_EQ( rejection->message, std::format( "Version mismatch for UUID {}: CurrentVersion=None, VersionExpectedByCommand=1", uuid ) );
}

TEST( RefDataCommandProcessorTest, RejectsExistingUpsertWithMismatchedExpectedVersion )
{
	const ID::UUID uuid = makeUUID( 4 );
	const Cmd::Upsert<Currency> command{
		.targetUUID      = uuid,
		.data            = makeCurrency( uuid ),
		.updatedBy       = "test-user",
		.expectedVersion = 6
	};

	const CommandDecision<Currency> decision = RefDataCommandProcessor::process( command, 7, makeUpdateTime() );

	const auto* rejection = std::get_if<CommandRejection>( &decision );
	ASSERT_NE( rejection, nullptr );
	EXPECT_EQ( rejection->message, std::format( "Version mismatch for UUID {}: CurrentVersion=7, VersionExpectedByCommand=6", uuid ) );
}

TEST( RefDataCommandProcessorTest, RejectsUpsertWhenTargetAndDataUUIDsDiffer )
{
	const ID::UUID targetUUID = makeUUID( 5 );
	const ID::UUID dataUUID   = makeUUID( 6 );
	const Cmd::Upsert<Currency> command{
		.targetUUID      = targetUUID,
		.data            = makeCurrency( dataUUID ),
		.updatedBy       = "test-user",
		.expectedVersion = 0
	};

	const CommandDecision<Currency> decision = RefDataCommandProcessor::process( command, std::nullopt, makeUpdateTime() );

	const auto* rejection = std::get_if<CommandRejection>( &decision );
	ASSERT_NE( rejection, nullptr );
	EXPECT_EQ( rejection->message, std::format( "Command target UUID {} does not match data UUID {}", targetUUID, dataUUID ) );
}

TEST( RefDataCommandProcessorTest, DeactivatesExistingRecordAndPreservesItsData )
{
	const ID::UUID uuid = makeUUID( 7 );
	const Record<Currency> currentRecord = makeCurrencyRecord( uuid, 4 );
	const Cmd::Deactivate<Currency> command{
		.targetUUID      = uuid,
		.updatedBy       = "deactivating-user",
		.expectedVersion = 4
	};
	const Time::DateTime updateTime = makeUpdateTime();

	const CommandDecision<Currency> decision = RefDataCommandProcessor::process( command, currentRecord, updateTime );

	const auto* record = std::get_if<Record<Currency>>( &decision );
	ASSERT_NE( record, nullptr );
	EXPECT_EQ( record->header.uuid, uuid );
	EXPECT_FALSE( record->header.isActive );
	EXPECT_EQ( record->header.lastUpdatedTs, updateTime );
	EXPECT_EQ( record->header.lastUpdatedBy, "deactivating-user" );
	EXPECT_EQ( record->header.version, 5 );
	EXPECT_EQ( record->data.uuid, currentRecord.data.uuid );
	EXPECT_EQ( record->data.ccyID, currentRecord.data.ccyID );
	EXPECT_EQ( record->data.name, currentRecord.data.name );
	EXPECT_EQ( record->data.decimalPlaces, currentRecord.data.decimalPlaces );
	EXPECT_EQ( record->data.settlementDays, currentRecord.data.settlementDays );
}

TEST( RefDataCommandProcessorTest, RejectsDeactivateForMissingRecord )
{
	const ID::UUID uuid = makeUUID( 8 );
	const Cmd::Deactivate<Currency> command{
		.targetUUID      = uuid,
		.updatedBy       = "test-user",
		.expectedVersion = 0
	};

	const CommandDecision<Currency> decision = RefDataCommandProcessor::process( command, std::nullopt, makeUpdateTime() );

	const auto* rejection = std::get_if<CommandRejection>( &decision );
	ASSERT_NE( rejection, nullptr );
	EXPECT_EQ( rejection->message, std::format( "Version mismatch for UUID {}: CurrentVersion=None, VersionExpectedByCommand=0", uuid ) );
}

TEST( RefDataCommandProcessorTest, RejectsDeactivateWithMismatchedExpectedVersion )
{
	const ID::UUID uuid = makeUUID( 9 );
	const Record<Currency> currentRecord = makeCurrencyRecord( uuid, 4 );
	const Cmd::Deactivate<Currency> command{
		.targetUUID      = uuid,
		.updatedBy       = "test-user",
		.expectedVersion = 3
	};

	const CommandDecision<Currency> decision = RefDataCommandProcessor::process( command, currentRecord, makeUpdateTime() );

	const auto* rejection = std::get_if<CommandRejection>( &decision );
	ASSERT_NE( rejection, nullptr );
	EXPECT_EQ( rejection->message, std::format( "Version mismatch for UUID {}: CurrentVersion=4, VersionExpectedByCommand=3", uuid ) );
}

TEST( RefDataCommandProcessorTest, PreservesOptionalUserDataWhenDeactivating )
{
	const ID::UUID uuid = makeUUID( 10 );
	const Record<User> currentRecord{
		.header = {
			.uuid          = uuid,
			.isActive      = true,
			.lastUpdatedTs = Time::DateTime( Time::Microseconds( 100 ) ),
			.lastUpdatedBy = "previous-user",
			.version       = 2
		},
		.data = {
			.uuid        = uuid,
			.userID      = "alice",
			.fullName    = "Alice Example",
			.email       = "alice@example.test",
			.tradingDesk = std::nullopt
		}
	};
	const Cmd::Deactivate<User> command{
		.targetUUID      = uuid,
		.updatedBy       = "test-user",
		.expectedVersion = 2
	};

	const CommandDecision<User> decision = RefDataCommandProcessor::process( command, currentRecord, makeUpdateTime() );

	const auto* record = std::get_if<Record<User>>( &decision );
	ASSERT_NE( record, nullptr );
	EXPECT_EQ( record->data.userID, "alice" );
	EXPECT_EQ( record->data.fullName, "Alice Example" );
	EXPECT_EQ( record->data.email, "alice@example.test" );
	EXPECT_EQ( record->data.tradingDesk, std::nullopt );
}
