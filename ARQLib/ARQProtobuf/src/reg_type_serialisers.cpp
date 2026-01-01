#include <ARQProtobuf/reg_type_serialisers.h>

#include "misc_type_serialisers.h"
#include "proto_refdata_command_serialisers.h"

namespace ARQ::Proto
{

void registerTypeSerialisers( Serialiser* const serialiserPtr )
{
    RD::registerMiscTypeSerialisers( *serialiserPtr );
    RD::Cmd::registerRefDataCommandSerialisers( *serialiserPtr );
}

}