#include <ARQProtobuf/reg_type_serialisers.h>

#include "misc_type_serialisers.h"
#include "proto_refdata_entity_serialisers.h"
#include "proto_refdata_command_serialisers.h"

namespace ARQ::Proto
{

void registerTypeSerialisers( Serialiser* const serialiserPtr )
{
    registerMiscTypeSerialisers( *serialiserPtr );
    RD::registerRefDataEntitySerialisers( *serialiserPtr );
    RD::Cmd::registerRefDataCommandSerialisers( *serialiserPtr );
}

}