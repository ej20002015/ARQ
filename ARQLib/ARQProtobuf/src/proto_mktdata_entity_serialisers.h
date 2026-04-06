#pragma once
#include <ARQProtobuf/dll.h>

#include <ARQCore/serialiser.h>
#include <ARQMarket/mktdata_entities.h>

namespace ARQ::Proto::MD
{

void registerMktDataEntitySerialisers( Serialiser& serialiser );

template<ARQ::MD::c_MktData T>
class ProtobufTypeSerialiser_MDRecord : public ISerialisableType<ARQ::MD::Record<T>>
{
public:
	ARQProtobuf_API Buffer serialise( const ARQ::MD::Record<T>& obj )                      const override;
	ARQProtobuf_API void   deserialise( const BufferView buf, ARQ::MD::Record<T>& objOut ) const override;
};

template<ARQ::MD::c_MktData T>
class ProtobufTypeSerialiser_MDRecordMessage : public ISerialisableType<ARQ::MD::RecordMessage<T>>
{
public:
	ARQProtobuf_API Buffer serialise( const ARQ::MD::RecordMessage<T>& obj )                      const override;
	ARQProtobuf_API void   deserialise( const BufferView buf, ARQ::MD::RecordMessage<T>& objOut ) const override;
};

}