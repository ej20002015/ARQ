#include <TMQSerialisation/ser_refdata_entities.h>

#include <TMQUtils/buffer.h>

#include <flatbuffers/flatbuffers.h>

#include <fbs_generated/user.h>

namespace TMQ
{

/*
* --------- User ---------
*/

Buffer serialise( const User& user )
{
	flatbuffers::FlatBufferBuilder builder;

	const auto fbUser = fbs::CreateUserDirect( 
		builder,
		user.firstname.c_str(),
		user.surname.c_str(),
		user.desk.c_str(),
		user.age
	);

	builder.Finish( fbUser );
	const uint8_t* const buffer = builder.GetBufferPointer();
	const size_t size = builder.GetSize();
	/*size_t _1, _2;
	builder.ReleaseRaw( _1, _2 );*/
	return Buffer( buffer, size ); // TODO: Try converting buffer to a std::unique_ptr so we can steal memory
}

template<>
User deserialise( const BufferView buffer )
{
	const uint8_t* const bufPtr = reinterpret_cast<const uint8_t*>( buffer.data );
	const fbs::User* const fbUserDeserialised = fbs::GetUser( bufPtr );

	User user = {
		std::chrono::system_clock::time_point(),
		"",
		false,
		fbUserDeserialised->firstname()->c_str(),
		fbUserDeserialised->surname()->c_str(),
		fbUserDeserialised->desk()->c_str(),
		fbUserDeserialised->age()
	};

	return user;
}

template TMQSerialisation_API User deserialise( const BufferView buffer );

}
