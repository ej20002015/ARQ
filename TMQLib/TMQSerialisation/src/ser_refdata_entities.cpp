#include <TMQSerialisation/ser_refdata_entities.h>

#include <TMQUtils/error.h>
#include "utilities.h"

#include <flatbuffers/flatbuffers.h>

#include <fbs_generated/user.h>

namespace TMQ
{

std::string serialise( User&& user )
{
	flatbuffers::FlatBufferBuilder builder;

	const auto fbUser = fbs::CreateUserDirect( 
		builder,
		user._lastUpdatedBy.c_str(),
		tpToLong( user._lastUpdatedTm ),
		user._active,
		user.firstname.c_str(),
		user.surname.c_str(),
		user.desk.c_str(),
		user.age
	);

	builder.Finish( fbUser );
	const uint8_t* const buffer = builder.GetBufferPointer();
	const size_t size = builder.GetSize();

    return std::string( reinterpret_cast<const char*>( buffer ), size );
}

template<>
User deserialise( const std::string_view buffer )
{
	const uint8_t* const bufPtr = reinterpret_cast<const uint8_t*>( buffer.data() );
	const fbs::User* const fbUserDeserialised = fbs::GetUser( bufPtr );

	User user = {
		fbUserDeserialised->_lastUpdatedBy()->c_str(),
		longToTp( fbUserDeserialised->_lastUpdatedTm() ),
		fbUserDeserialised->_active(),
		fbUserDeserialised->firstname()->c_str(),
		fbUserDeserialised->surname()->c_str(),
		fbUserDeserialised->desk()->c_str(),
		fbUserDeserialised->age()
	};

	return user;
}

template TMQSerialisation User deserialise( const std::string_view buffer );

}
