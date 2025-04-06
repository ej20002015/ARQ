#pragma once
#include <TMQSerialisation/dll.h>

#include <TMQUtils/buffer.h>
#include <TMQCore/refdata_entities.h>

namespace TMQ
{

// To be specialised for each RDEntity
template<c_RDEntity T>
[[nodiscard]] T deserialise( const BufferView buffer );

/*
* --------- User ---------
*/

TMQSerialisation [[nodiscard]] Buffer serialise( const User& user );

}