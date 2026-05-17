#include <ARQCore/serialiser.h>

#include <ARQUtils/core.h>
#include <ARQUtils/enum.h>
#include <ARQCore/dynalib_cache.h>


namespace ARQ
{

std::shared_ptr<Serialiser> SerialiserFactory::create( const SerialiserImpl impl )
{
    std::lock_guard<std::mutex> lg( m_serialisersMutex );

    if( const auto it = m_serialisers.find( impl ); it != m_serialisers.end() )
        return it->second;

	std::string dynaLibName;
	switch( impl )
	{
		case SerialiserImpl::Protobuf: dynaLibName = "ARQProtobuf"; break;
		default:
			ARQ_ASSERT( false );
	}

	const OS::DynaLib& lib = DynaLibCache::inst().get( dynaLibName );

	const auto registerTypesFunc = lib.getFunc<RegisterTypeSerialisersFunc>( "registerTypeSerialisers" );
	auto newSerialiser = std::make_shared<Serialiser>();
	registerTypesFunc( newSerialiser.get() );

	return m_serialisers.emplace( impl, std::move( newSerialiser ) ).first->second;
}

void SerialiserFactory::addCustomSerialiser( const SerialiserImpl impl, std::shared_ptr<Serialiser> customSerialiser )
{
	std::lock_guard<std::mutex> lg( m_serialisersMutex );

	if( !m_serialisers.emplace( impl, customSerialiser ).second )
		throw ARQException( std::format( "SerialiserFactory: Cannot add custom serialiser with impl={} as it already exists", Enum::enum_name( impl ) ) );
}

void SerialiserFactory::delCustomSerialiser( const SerialiserImpl impl )
{
	std::lock_guard<std::mutex> lg( m_serialisersMutex );

	if( const auto it = m_serialisers.find( impl ); it != m_serialisers.end() )
		m_serialisers.erase( it );
	else
		throw ARQException( std::format( "SerialiserFactory: Cannot find custom serialiser with impl={} to delete", Enum::enum_name( impl ) ) );
}

}