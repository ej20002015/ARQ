#pragma once
#include <ARQProtobuf/dll.h>

#include <ARQCore/serialiser.h>

#include <ARQCore/refdata_command_manager.h>

namespace ARQ
{

class ProtobufTypeSerialiser_RefDataCommandResponse : public ISerialisableType<RefDataCommandResponse>
{
public:
	ARQProtobuf_API Buffer serialise( const RefDataCommandResponse& obj )                      const override;
	ARQProtobuf_API void   deserialise( const BufferView buf, RefDataCommandResponse& objOut ) const override;
};

void registerMiscTypeSerialisers( Serialiser& serialiser );

}