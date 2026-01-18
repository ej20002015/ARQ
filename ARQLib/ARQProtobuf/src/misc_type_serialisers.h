#pragma once
#include <ARQProtobuf/dll.h>

#include <ARQCore/serialiser.h>

#include <ARQCore/refdata_command_manager.h>

namespace ARQ::Proto
{

void registerMiscTypeSerialisers( Serialiser& serialiser );

namespace RD
{

class ProtobufTypeSerialiser_RDCommandResponse : public ISerialisableType<ARQ::RD::CommandResponse>
{
public:
	ARQProtobuf_API Buffer serialise( const ARQ::RD::CommandResponse& obj )                      const override;
	ARQProtobuf_API void   deserialise( const BufferView buf, ARQ::RD::CommandResponse& objOut ) const override;
};

}

}