#include "proto_mktdata_market_serialisers.h"

#include <ARQProtobuf/md_market_proto_converters.h>

#include "helpers.h"

namespace ARQ::Proto::MD
{

void registerMktDataMarketSerialisers( Serialiser& serialiser )
{
	serialiser.registerHandler<ARQ::MD::MarketUpdateBatch>( std::make_unique<ProtobufTypeSerialiser_MarketUpdateBatch>() );
}

Buffer ProtobufTypeSerialiser_MarketUpdateBatch::serialise( const ARQ::MD::MarketUpdateBatch& obj ) const
{
	MarketUpdateBatch protoObj;
	toProto( obj, &protoObj );
	return serialiseToBuffer( protoObj );
}

void ProtobufTypeSerialiser_MarketUpdateBatch::deserialise( const BufferView buf, ARQ::MD::MarketUpdateBatch& objOut ) const
{
	MarketUpdateBatch protoObj;
	if( !protoObj.ParseFromArray( buf.data, buf.size ) )
		throw ARQException( std::format( "Cannot deserialise buffer into MarketUpdateBatch object" ) );

	objOut = fromProto( protoObj );
}

}
