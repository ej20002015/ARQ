#pragma once
#include <ARQProtobuf/dll.h>

#include <ARQCore/serialiser.h>

namespace ARQ
{

extern "C" ARQProtobuf_API void registerTypeSerialisers( Serialiser* const serialiserPtr );

}