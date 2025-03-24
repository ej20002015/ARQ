#pragma once

#include <TMQCore/data_entity.h>

namespace TMQ
{

struct RDEntity : public DataEntity
{
	bool _enabled = false;
};

template<typename T>
concept c_RDEntity = std::is_base_of_v<RDEntity, T>;

template<c_RDEntity T>
class RDEntityTraits
{
public:
	static consteval const char* const table() { static_assert( false ); }
	static std::string getID( const T& rdEntity ) { static_assert( false ); }
};

/*
* -------------- REFDATA ENTITY DEFINITIONS --------------
*/

// USER

struct User : public RDEntity
{
	std::string firstname;
	std::string surname;
	std::string desk;
	uint32_t age = 0;
};

template<>
class RDEntityTraits<User>
{
public:
	static consteval const char* const table() { return "Users"; }
	static std::string getID( const User& user ) { return user.firstname + user.surname; }
	static User deserialise( const std::string_view blob )
	{
		User user;

		size_t offset = 0;

		// Deserialize the `firstname` string
		uint32_t firstnameLength = *reinterpret_cast< const uint32_t* >( blob.data() + offset );
		offset += sizeof( uint32_t );  // move the offset past the length
		user.firstname = std::string( blob.data() + offset, firstnameLength );
		offset += firstnameLength;  // move the offset past the string

		// Deserialize the `surname` string
		uint32_t surnameLength = *reinterpret_cast< const uint32_t* >( blob.data() + offset );
		offset += sizeof( uint32_t );
		user.surname = std::string( blob.data() + offset, surnameLength );
		offset += surnameLength;

		// Deserialize the `desk` string
		uint32_t deskLength = *reinterpret_cast< const uint32_t* >( blob.data() + offset );
		offset += sizeof( uint32_t );
		user.desk = std::string( blob.data() + offset, deskLength );
		offset += deskLength;

		// Deserialize the `age` field
		user.age = *reinterpret_cast< const uint32_t* >( blob.data() + offset );
		offset += sizeof( uint32_t );

		return user;
	}
};

}