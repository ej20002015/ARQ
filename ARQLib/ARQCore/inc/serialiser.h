#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/buffer.h>
#include <ARQUtils/error.h>
#include <ARQCore/type_registry.h>

#include <unordered_map>
#include <string>
#include <mutex>

namespace ARQ
{

class SerialisableTypeConcept
{
public:
	virtual ~SerialisableTypeConcept() = default;
};

template<typename T>
class ISerialisableType : public SerialisableTypeConcept
{
public:
	virtual Buffer serialise( const T& obj )                      const = 0;
	virtual void   deserialise( const BufferView buf, T& objOut ) const = 0;
};

class Serialiser
{
public:
	template<typename T>
	Buffer serialise( const T& obj ) const;
	
	template<typename T>
	T deserialise( const BufferView buf ) const;

	template<typename T>
	void registerHandler( std::unique_ptr<ISerialisableType<T>> handler );

private:
	template<typename T>
	const ISerialisableType<T>& getTypeSerialiser() const;

private:
	std::unordered_map<std::string_view, std::unique_ptr<SerialisableTypeConcept>> m_typeSerialisers;
};

using RegisterTypeSerialisersFunc = std::add_pointer<void( Serialiser* const serialiser )>::type;

class SerialiserFactory
{
public:
	enum class SerialiserImpl
	{
		Protobuf
	};

public:
	[[nodiscard]] ARQCore_API static std::shared_ptr<Serialiser> create( const SerialiserImpl impl );

private:
	static std::unordered_map<SerialiserImpl, std::shared_ptr<Serialiser>> s_serialisers;
	static std::mutex s_serialisersMutex;
};

template<typename T>
Buffer Serialiser::serialise( const T& obj ) const
{
	const ISerialisableType<T>& typeSerialiser = getTypeSerialiser<T>();
	return typeSerialiser.serialise( obj );
}

template<typename T>
T Serialiser::deserialise( const BufferView buf ) const
{
	T obj;
	const ISerialisableType<T>& typeSerialiser = getTypeSerialiser<T>();
	typeSerialiser.deserialise( buf, obj );
	return obj;
}

template<typename T>
void Serialiser::registerHandler( std::unique_ptr<ISerialisableType<T>> handler )
{
	constexpr std::string_view typeName = ARQType<T>::name();
	m_typeSerialisers[typeName] = std::move( handler );
}

template<typename T>
const ISerialisableType<T>& Serialiser::getTypeSerialiser() const
{
	constexpr std::string_view typeName = ARQType<T>::name();
	const auto it = m_typeSerialisers.find( typeName );
	if( it != m_typeSerialisers.end() )
		return static_cast<const ISerialisableType<T>&>( *it->second );
	else
		throw ARQException( std::format( "No type serialiser registered for type [{}]", typeName ) );
}

}