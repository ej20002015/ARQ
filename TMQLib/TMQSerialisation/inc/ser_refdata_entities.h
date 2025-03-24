#pragma once
#include <TMQSerialisation/dll.h>

#include <TMQCore/refdata_entities.h>

namespace TMQ
{

/*
* --------- User ---------
*/

TMQ_API std::string serialise( User&& user );
TMQ_API User deserialise( const std::string& buffer );

}