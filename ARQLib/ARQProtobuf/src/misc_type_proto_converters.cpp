#include <ARQProtobuf/misc_type_proto_converters.h>

#include <ARQUtils/enum.h>

#include "proto_gen/id.pb.h"

namespace ARQ::Proto
{

namespace RD
{

void toProto( const ARQ::RD::CommandResponse& arqObj, RefDataCommandResponse* const protoObj )
{
	ARQ::Proto::ID::UUID* uuidPtr = protoObj->mutable_corr_id();
	std::string* uuidBufPtr = uuidPtr->mutable_id();
	*uuidBufPtr = arqObj.corrID.toString();

	protoObj->set_status( Enum::enum_integer( arqObj.status ) );
	if( arqObj.message )
		protoObj->set_message( *arqObj.message );
}

ARQ::RD::CommandResponse fromProto( const RefDataCommandResponse& protoObj )
{
	ARQ::RD::CommandResponse arqObj;

	arqObj.corrID = ARQ::ID::UUID::fromString( protoObj.corr_id().id() );

	const auto statusOpt = Enum::enum_cast<ARQ::RD::CommandResponse::Status>( protoObj.status() );
	if( statusOpt )
		arqObj.status = statusOpt.value();
	else
		throw ARQException( std::format( "Cannot create RefDataCommandResponse from protoObj - status integer [{}] not a value in RefDataCommandResponse::Status enum", protoObj.status() ) );

	if( protoObj.has_message() )
		arqObj.message = protoObj.message();

	return arqObj;
}

}

void toProto( const ARQ::StreamTopicPartitionOffsets& arqObj, StreamTopicPartitionOffsets* const protoObj )
{
	for( const auto& [tp, offset] : arqObj )
	{
		StreamTopicPartitionOffset* protoEntry = protoObj->add_offsets();

		auto* protoTp = protoEntry->mutable_topic_partition();
		protoTp->set_topic( tp.first );
		protoTp->set_partition( tp.second );

		protoEntry->set_offset( offset );
	}
}

ARQ::StreamTopicPartitionOffsets fromProto( const StreamTopicPartitionOffsets& protoObj )
{
	ARQ::StreamTopicPartitionOffsets arqObj;

	for( const auto& entry : protoObj.offsets() )
	{
		ARQ::StreamTopicPartition key{
			entry.topic_partition().topic(),
			entry.topic_partition().partition()
		};

		arqObj.emplace( key, entry.offset() );
	}

	return arqObj;
}

}