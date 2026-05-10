#pragma once
#include <ARQProtobuf/dll.h>

#include <ARQMarket/managed_market.h>

#include <proto_gen/mktdata_markets.pb.h>

namespace ARQ::Proto::MD
{

// --- Converters for MarketUpdateBatch ---

ARQProtobuf_API void toProto( const ARQ::MD::MarketUpdateBatch& arqObj, MarketUpdateBatch* const protoObj );
ARQProtobuf_API void toProto( ARQ::MD::MarketUpdateBatch&& arqObj, MarketUpdateBatch* const protoObj );

[[nodiscard]] ARQProtobuf_API ARQ::MD::MarketUpdateBatch fromProto( const MarketUpdateBatch& protoObj );
[[nodiscard]] ARQProtobuf_API ARQ::MD::MarketUpdateBatch fromProto( MarketUpdateBatch&& protoObj );

}