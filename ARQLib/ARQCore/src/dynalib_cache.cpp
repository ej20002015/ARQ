#include <ARQCore/dynalib_cache.h>

#include <ARQUtils/logger.h>
#include <ARQCore/dynalib_hooks.h>

namespace ARQ
{

std::atomic<DynaLibCache*> DynaLibCache::s_globalInst = nullptr;

DynaLibCache* DynaLibCache::globalInst()
{
    return s_globalInst.load();
}

void DynaLibCache::setGlobalInst( DynaLibCache* const inst )
{
	s_globalInst.store( inst );
}

const OS::DynaLib& DynaLibCache::get( const std::string_view libName )
{
	if( globalInst() )
        return globalInst()->iGet(libName);
    else
        throw ARQException( std::format( "Cannot get dynalib [{}] as DynaLibCache global instance has not been set", libName ) );
}

const OS::DynaLib& DynaLibCache::iGet( const std::string_view libName )
{
    std::lock_guard<std::mutex> lg( m_mut );

    auto it = m_loadedLibs.find( libName );
    if( it == m_loadedLibs.end() )
    {
        Log( Module::CORE ).info( "Loading dynalib [{}]", libName );
        it = m_loadedLibs.emplace( libName, libName ).first;
        m_loadedLibsOrder.emplace_back( libName );

        // Call the init function if it exists
        if( auto initFunc = it->second.getFunc<ARQDynaLibInitFunc>( "arqDynaLibInit", DoThrow::NO ) )
        {
            Log( Module::CORE ).info( "Calling init function of dynalib [{}]", libName );

            ARQ_DO_IN_TRY( arqExc, errMsg );
                initFunc();
            ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

            if( arqExc.what().size() )
				throw ARQException( std::format( "Exception thrown when calling init function of dynalib [{}]: {}", libName, arqExc.what() ) );
            else if( errMsg.size() )
                throw ARQException( std::format( "Exception thrown when calling init function of dynalib [{}]: {}", libName, errMsg ) );
        }
    }

    return it->second;
}

DynaLibCache::~DynaLibCache()
{
    std::lock_guard<std::mutex> lg( m_mut );

    for( auto it = m_loadedLibsOrder.rbegin(); it != m_loadedLibsOrder.rend(); ++it )
    {
        const auto& libName = *it;
        const auto libIt = m_loadedLibs.find( libName );
        if( libIt != m_loadedLibs.end() )
        {
            // Call the shutdown function if it exists
            if( auto shutdownFunc = libIt->second.getFunc<ARQDynaLibShutdownFunc>( "arqDynaLibShutdown", DoThrow::NO ) )
            {
                Log( Module::CORE ).info( "Calling shutdown function of dynalib [{}]", libName );

                ARQ_DO_IN_TRY( arqExc, errMsg );
                    shutdownFunc();
                ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

                if( arqExc.what().size() )
					Log( Module::CORE ).error( arqExc, "Exception thrown when calling shutdown function of dynalib [{}]", libName );
                else if( errMsg.size() )
                    Log( Module::CORE ).error( "Exception thrown when calling shutdown function of dynalib [{}] - what: {}", libName, errMsg );
            }

            Log( Module::CORE ).info( "Unloading dynalib [{}]", libName );
            m_loadedLibs.erase( libIt );
        }
    }
}

}