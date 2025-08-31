#include <ARQCore/dynalib_cache.h>

namespace ARQ
{

const OS::DynaLib& DynaLibCache::get( const std::string_view libName )
{
    std::lock_guard<std::mutex> lg( m_mut );

    auto it = m_loadedLibs.find( libName );
    if( it == m_loadedLibs.end() )
        it = m_loadedLibs.emplace( libName, libName ).first;

    return it->second;
}

}