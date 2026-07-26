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

void toProto( const ARQ::StreamTopicPartitionOffset& arqObj, StreamTopicPartitionOffset* const protoObj )
{
	auto* protoTp = protoObj->mutable_topic_partition();
	protoTp->set_topic( arqObj.tp.first );
	protoTp->set_partition( arqObj.tp.second );
	protoObj->set_offset( arqObj.offset );
}

ARQ::StreamTopicPartitionOffset fromProto( const StreamTopicPartitionOffset& protoObj )
{
	ARQ::StreamTopicPartitionOffset arqObj;

	arqObj.tp.first = protoObj.topic_partition().topic();
	arqObj.tp.second = protoObj.topic_partition().partition();
	arqObj.offset = protoObj.offset();

	return arqObj;
}

}
