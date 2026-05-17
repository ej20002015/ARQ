#pragma once
#include <ARQProtobuf/dll.h>

#include <ARQCore/serialiser.h>
#include <ARQMarket/market_live.h>

namespace ARQ::Proto::MD
{

void registerMktDataMarketSerialisers( Serialiser& serialiser );

class ProtobufTypeSerialiser_MarketUpdateBatch : public ISerialisableType<ARQ::MD::MarketUpdateBatch>
{
public:
	ARQProtobuf_API Buffer serialise( const ARQ::MD::MarketUpdateBatch& obj )                      const override;
	ARQProtobuf_API void   deserialise( const BufferView buf, ARQ::MD::MarketUpdateBatch& objOut ) const override;
};

}