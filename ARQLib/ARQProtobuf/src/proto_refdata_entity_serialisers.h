#pragma once
#include <ARQProtobuf/dll.h>

#include <ARQCore/serialiser.h>
#include <ARQCore/refdata_entities.h>

namespace ARQ::Proto::RD
{

void registerRefDataEntitySerialisers( Serialiser& serialiser );

template<ARQ::RD::c_RefData T>
class ProtobufTypeSerialiser_RDRecord : public ISerialisableType<ARQ::RD::Record<T>>
{
public:
	ARQProtobuf_API Buffer serialise( const ARQ::RD::Record<T>& obj )                      const override;
	ARQProtobuf_API void   deserialise( const BufferView buf, ARQ::RD::Record<T>& objOut ) const override;
};

}