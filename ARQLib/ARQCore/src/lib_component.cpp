#include <ARQCore/lib_component.h>

#include <ARQUtils/error.h>

#include <algorithm>

namespace ARQ
{

LibComponentRegistry::Reg::Reg( LibComponentRegistration registration )
{
	LibComponentRegistry::registrations().push_back( std::move( registration ) );
}

std::vector<LibComponentRegistration> LibComponentRegistry::getComponents()
{
	std::vector<LibComponentRegistration> components = registrations();

	std::ranges::sort( components, [] ( const LibComponentRegistration& lhs, const LibComponentRegistration& rhs )
	{
		return lhs.order < rhs.order;
	} );

	// Validate components: no duplicates, no missing names or factories, no duplicate names or orders
	for( size_t i = 0; i < components.size(); ++i )
	{
		const LibComponentRegistration& component = components[i];
		if( component.name.empty() )
			throw ARQException( "Cannot register an ARQ library component without a name" );
		if( !component.factory )
			throw ARQException( std::format( "ARQ library component [{}] has no factory", component.name ) );

		for( size_t j = 0; j < i; ++j )
		{
			const LibComponentRegistration& previous = components[j];
			if( component.name == previous.name )
				throw ARQException( std::format( "Duplicate ARQ library component name [{}]", component.name ) );
			if( component.order == previous.order )
				throw ARQException( std::format( "ARQ library components [{}] and [{}] have the same initialization order", previous.name, component.name ) );
		}
	}

	return components;
}

std::vector<LibComponentRegistration>& LibComponentRegistry::registrations()
{
	// Component registrations are process-lifetime metadata populated during DLL startup.
	static auto* registrations = new std::vector<LibComponentRegistration>();
	return *registrations;
}

}
