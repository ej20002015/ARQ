#include <ARQCore/refdata_meta.h>

#include <ARQUtils/error.h>

#include <algorithm>

namespace ARQ::RD::Meta
{

const EntityMetadata& get( const std::string_view entityName )
{
    return EntityMetadataRegistry::inst().get( entityName );
}

const std::vector<EntityMetadata>& getAll()
{
    return EntityMetadataRegistry::inst().getAll();
}

const EntityMetadataRegistry& EntityMetadataRegistry::inst()
{
    return instMut();
}

const EntityMetadata& EntityMetadataRegistry::get( const std::string_view entityName ) const
{
    const auto it = std::find_if( m_metadataList.begin(), m_metadataList.end(), [=] ( const EntityMetadata& meta ) { return meta.name == entityName; } );
    if( it == m_metadataList.end() )
        throw ARQException( std::format( "Cannot get metadata for refdata entity [{}] - no such refdata entity registered", entityName ) );

    return *it;
}

EntityMetadataRegistry& EntityMetadataRegistry::instMut()
{
    static EntityMetadataRegistry inst;
    return inst;
}

}