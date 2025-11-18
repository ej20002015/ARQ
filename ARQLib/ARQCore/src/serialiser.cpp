#include <ARQCore/serialiser.h>

#include <ARQUtils/core.h>
#include <ARQCore/dynalib_cache.h>

namespace ARQ
{

std::unordered_map<SerialiserFactory::SerialiserImpl, std::shared_ptr<Serialiser>> SerialiserFactory::s_serialisers;
std::mutex SerialiserFactory::s_serialisersMutex;

std::shared_ptr<Serialiser> SerialiserFactory::create( const SerialiserImpl impl )
{
    std::lock_guard<std::mutex> lg( s_serialisersMutex );

    if( const auto it = s_serialisers.find( impl ); it != s_serialisers.end() )
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

	return s_serialisers.emplace( impl, newSerialiser ).first->second;
}

}