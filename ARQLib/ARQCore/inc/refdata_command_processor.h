#pragma once

#include <ARQCore/refdata_commands.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace ARQ::RD
{

struct CommandRejection
{
	std::string message;
};

template<c_RefData T>
using CommandDecision = std::variant<Record<T>, CommandRejection>;

class RefDataCommandProcessor
{
public:
	RefDataCommandProcessor() = delete;

	template<c_RefData T>
	static CommandDecision<T> process( const Cmd::Upsert<T>& command, const std::optional<uint32_t>& currentVersion, const Time::DateTime& updateTime )
	{
		if( command.targetUUID != command.data.uuid )
		{
			return CommandRejection{
				.message = std::format( "Command target UUID {} does not match data UUID {}", command.targetUUID, command.data.uuid )
			};
		}

		if( !isValidUpsertVersion( currentVersion, command.expectedVersion ) )
			return rejectVersionMismatch( command.targetUUID, currentVersion, command.expectedVersion );

		Record<T> newRecord;
		newRecord.header.uuid          = command.targetUUID;
		newRecord.header.isActive      = true;
		newRecord.header.lastUpdatedTs = updateTime;
		newRecord.header.lastUpdatedBy = command.updatedBy;
		newRecord.header.version       = currentVersion.value_or( 0 ) + 1;
		newRecord.data                 = command.data;

		return newRecord;
	}

	template<c_RefData T>
	static CommandDecision<T> process( const Cmd::Deactivate<T>& command, const std::optional<Record<std::type_identity_t<T>>>& currentRecord, const Time::DateTime& updateTime )
	{
		const std::optional<uint32_t> currentVersion = currentRecord ? std::make_optional( currentRecord->header.version ) : std::nullopt;

		if( !currentVersion || command.expectedVersion != *currentVersion )
			return rejectVersionMismatch( command.targetUUID, currentVersion, command.expectedVersion );

		Record<T> newRecord;
		newRecord.header.uuid          = command.targetUUID;
		newRecord.header.isActive      = false;
		newRecord.header.lastUpdatedTs = updateTime;
		newRecord.header.lastUpdatedBy = command.updatedBy;
		newRecord.header.version       = *currentVersion + 1;
		newRecord.data                 = currentRecord->data;

		return newRecord;
	}

private:
	static bool isValidUpsertVersion( const std::optional<uint32_t>& currentVersion, const uint32_t expectedVersion )
	{
		return ( !currentVersion && expectedVersion == 0 ) || ( currentVersion && expectedVersion == *currentVersion );
	}

	static CommandRejection rejectVersionMismatch( const ID::UUID& targetUUID, const std::optional<uint32_t>& currentVersion, const uint32_t expectedVersion )
	{
		return CommandRejection{
			.message = std::format( "Version mismatch for UUID {}: CurrentVersion={}, VersionExpectedByCommand={}",
									targetUUID, currentVersion ? std::to_string( *currentVersion ) : "None", expectedVersion )
		};
	}
};

}
