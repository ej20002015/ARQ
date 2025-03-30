#pragma once
#include <TMQSerialisation/dll.h>

#include <TMQCore/refdata_entities.h>

namespace TMQ
{

// To be specialised for each RDEntity
template<c_RDEntity T>
T deserialise( const std::string_view buffer );

/*
* --------- User ---------
*/

TMQSerialisation std::string serialise( User&& user );

}