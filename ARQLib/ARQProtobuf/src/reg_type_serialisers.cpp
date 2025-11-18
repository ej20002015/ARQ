#include <ARQProtobuf/reg_type_serialisers.h>

#include "misc_type_serialisers.h"

namespace ARQ
{

void registerTypeSerialisers( Serialiser* const serialiserPtr )
{
    registerMiscTypeSerialisers( *serialiserPtr );
}

}