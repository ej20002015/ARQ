#include "misc_type_serialisers.h"

#include <ARQUtils/enum.h>

#include "proto_gen/id.pb.h"
#include "proto_gen/command_manager.pb.h"

namespace ARQ::Proto::RD
{

void registerMiscTypeSerialisers( Serialiser& serialiser )
{
	serialiser.registerHandler<ARQ::RD::CommandResponse>( std::make_unique<ProtobufTypeSerialiser_RDCommandResponse>() );
}

Buffer ProtobufTypeSerialiser_RDCommandResponse::serialise( const ARQ::RD::CommandResponse& obj ) const
{
	ARQ::Proto::RefDataCommandResponse resp;

	ARQ::Proto::ID::UUID* uuidPtr = resp.mutable_corr_id();
	std::string* uuidBufPtr = uuidPtr->mutable_id();
	*uuidBufPtr = obj.corrID.toString();

	resp.set_status( Enum::enum_integer( obj.status ) );
	if( obj.message )
		resp.set_message( *obj.message );

	Buffer respBuf( resp.ByteSizeLong() );
	resp.SerializeToArray( respBuf.data.get(), respBuf.size );
	return respBuf;
}

void ProtobufTypeSerialiser_RDCommandResponse::deserialise( const BufferView buf, ARQ::RD::CommandResponse& objOut ) const
{
	ARQ::Proto::RefDataCommandResponse resp;
	resp.ParseFromArray( buf.data, buf.size );

	objOut.corrID = ARQ::ID::UUID::fromString( resp.corr_id().id() );
	const auto statusOpt = Enum::enum_cast<ARQ::RD::CommandResponse::Status>( resp.status() );
	if( statusOpt )
		objOut.status = statusOpt.value();
	else
		throw ARQException( std::format( "Cannot deserialise buffer into RefDataCommandResponse - status integer [{}] not a value in RefDataCommandResponse::Status enum", resp.status() ) );
	if( resp.has_message() )
		objOut.message = resp.message();
}

}
