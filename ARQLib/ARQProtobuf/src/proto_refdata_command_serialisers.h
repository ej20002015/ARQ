#pragma once
#include <ARQProtobuf/dll.h>

#include <ARQCore/serialiser.h>
#include <ARQCore/refdata_commands.h>

namespace ARQ::Proto::RD::Cmd
{

void registerRefDataCommandSerialisers( Serialiser& serialiser );

template<ARQ::RD::c_RefData T>
class ProtobufTypeSerialiser_RDCmdUpsert : public ISerialisableType<ARQ::RD::Cmd::Upsert<T>>
{
public:
	ARQProtobuf_API Buffer serialise( const ARQ::RD::Cmd::Upsert<T>& obj )                      const override;
	ARQProtobuf_API void   deserialise( const BufferView buf, ARQ::RD::Cmd::Upsert<T>& objOut ) const override;
};

template<ARQ::RD::c_RefData T>
class ProtobufTypeSerialiser_RDCmdDeactivate : public ISerialisableType<ARQ::RD::Cmd::Deactivate<T>>
{
public:
	ARQProtobuf_API Buffer serialise( const ARQ::RD::Cmd::Deactivate<T>& obj )                      const override;
	ARQProtobuf_API void   deserialise( const BufferView buf, ARQ::RD::Cmd::Deactivate<T>& objOut ) const override;
};

}