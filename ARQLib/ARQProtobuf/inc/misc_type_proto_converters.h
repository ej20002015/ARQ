#pragma once
#include <ARQProtobuf/dll.h>

#include <ARQCore/refdata_command_manager.h>
#include <ARQCore/streaming_service.h>

#include "proto_gen/command_manager.pb.h"
#include "proto_gen/streaming_service.pb.h"

namespace ARQ::Proto
{

namespace RD
{

// --- Converters for RD::CommandResponse ---

ARQProtobuf_API void toProto( const ARQ::RD::CommandResponse& arqObj, RefDataCommandResponse* const protoObj );

[[nodiscard]] ARQProtobuf_API ARQ::RD::CommandResponse fromProto( const RefDataCommandResponse& protoObj );

}

// --- Converters for StreamTopicPartitionOffsets ---

ARQProtobuf_API void toProto( const ARQ::StreamTopicPartitionOffsets& arqObj, StreamTopicPartitionOffsets* const protoObj );

[[nodiscard]] ARQProtobuf_API ARQ::StreamTopicPartitionOffsets fromProto( const StreamTopicPartitionOffsets& protoObj );

}