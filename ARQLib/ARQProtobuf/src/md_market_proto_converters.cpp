#include <ARQProtobuf/md_market_proto_converters.h>

#include <ARQProtobuf/md_entity_proto_converters.h>
#include <ARQProtobuf/misc_type_proto_converters.h>

namespace ARQ::Proto::MD
{

// --- Converters for MarketUpdateBatch ---

void toProto( const ARQ::MD::MarketUpdateBatch& arqObj, MarketUpdateBatch* const protoObj )
{
    RecordCollection* const protoRecords = protoObj->mutable_records();
	toProto( arqObj.records, protoRecords );

	StreamTopicPartitionOffset* const protoSourcePosition = protoObj->mutable_source_position();
	ARQ::Proto::toProto( arqObj.sourcePosition, protoSourcePosition );

	protoObj->set_mkt_name( arqObj.marketName.str() );
}

void toProto( ARQ::MD::MarketUpdateBatch&& arqObj, MarketUpdateBatch* const protoObj )
{
	RecordCollection* const protoRecords = protoObj->mutable_records();
	toProto( std::move( arqObj.records ), protoRecords);

	StreamTopicPartitionOffset* const protoSourcePosition = protoObj->mutable_source_position();
	ARQ::Proto::toProto( std::move( arqObj.sourcePosition ), protoSourcePosition );

	protoObj->set_mkt_name( std::move( arqObj.marketName.str() ) );
}

ARQ::MD::MarketUpdateBatch fromProto( const MarketUpdateBatch& protoObj )
{
	ARQ::MD::MarketUpdateBatch objOut;

	objOut.records        = fromProto( protoObj.records() );
	objOut.sourcePosition = ARQ::Proto::fromProto( protoObj.source_position() );
	objOut.marketName     = ARQ::MD::MarketName::fromStr( protoObj.mkt_name() );

	return objOut;
}

ARQ::MD::MarketUpdateBatch fromProto( MarketUpdateBatch&& protoObj )
{
	ARQ::MD::MarketUpdateBatch objOut;

	objOut.records        = fromProto( std::move( *protoObj.mutable_records() ) );
	objOut.sourcePosition = ARQ::Proto::fromProto( std::move( *protoObj.mutable_source_position() ) );
	objOut.marketName     = ARQ::MD::MarketName::fromStr( std::move( *protoObj.mutable_mkt_name() ) );

	return objOut;
}

}


