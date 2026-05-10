#include "misc_type_serialisers.h"

#include <ARQUtils/enum.h>

#include <ARQProtobuf/misc_type_proto_converters.h>

#include "proto_gen/command_manager.pb.h"

namespace ARQ::Proto
{

void registerMiscTypeSerialisers( Serialiser& serialiser )
{
	serialiser.registerHandler<ARQ::RD::CommandResponse>( std::make_unique<RD::ProtobufTypeSerialiser_RDCommandResponse>() );
}

namespace RD
{

Buffer ProtobufTypeSerialiser_RDCommandResponse::serialise( const ARQ::RD::CommandResponse& obj ) const
{
	ARQ::Proto::RefDataCommandResponse resp;
	toProto( obj, &resp );

	Buffer respBuf( resp.ByteSizeLong() );
	resp.SerializeToArray( respBuf.data.get(), respBuf.size );
	return respBuf;
}

void ProtobufTypeSerialiser_RDCommandResponse::deserialise( const BufferView buf, ARQ::RD::CommandResponse& objOut ) const
{
	ARQ::Proto::RefDataCommandResponse resp;
	resp.ParseFromArray( buf.data, buf.size );

	objOut = fromProto( std::move( resp ) );
}

}

}
