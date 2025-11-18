#include "version_manager.h"

#include <ARQUtils/error.h>
#include <ARQCore/refdata_source.h>

namespace ARQ
{
namespace Grpc
{
namespace RefData
{

void VersionManagerImpl::init()
{
    m_entityMutexes.try_emplace( "Currency" );
    m_entityVers.try_emplace( "Currency" );
    m_entityMutexes.try_emplace( "User" );
    m_entityVers.try_emplace( "User" );

    // TEMP: For now we just load latest versions from ClickHouse DB
    std::shared_ptr<IRefDataSource> rdSource = RefDataSourceFactory::create( "ClickHouseDB" );
    const std::vector<RDEntities::Currency> currencies = rdSource->fetchCurrencies();
    for( const RDEntities::Currency& currency : currencies )
        setVerUnsafe( "Currency", currency.ccyID, currency._version );
    const std::vector<RDEntities::User> users = rdSource->fetchUsers();
    for( const RDEntities::User& user : users )
        setVerUnsafe( "User", user.userID, user._version );
}

std::mutex& VersionManagerImpl::getEntityMutex( const std::string_view entityName )
{
    const auto it = m_entityMutexes.find( entityName );
    if( it == m_entityMutexes.end() )
        throw ARQException( std::format( "VersionManager: Unknown refdata entity {} - cannot obtain mutex", entityName ) );

    return it->second;
}

uint32_t VersionManagerImpl::getVerUnsafe( const std::string_view entityName, const std::string_view id )
{
    const EntityID2VersionMap& verMap = getVerMap( entityName );
    const auto it = verMap.find( id );
    if( it != verMap.end() )
        return it->second;
    else
        return 0;
}

void VersionManagerImpl::setVerUnsafe( const std::string_view entityName, const std::string_view id, const uint32_t newVer )
{
    EntityID2VersionMap& verMap = getVerMap( entityName );
    const auto it = verMap.find( id );
    if( it != verMap.end() )
        it->second = newVer;
    else
        verMap.emplace( id, newVer );
}

VersionManagerImpl::EntityID2VersionMap& VersionManagerImpl::getVerMap( const std::string_view entityName )
{
    const auto it = m_entityVers.find( entityName );
    if( it == m_entityVers.end() )
        throw ARQException( std::format( "VersionManager: Unknown refdata entity {} - cannot get version map", entityName ) );

    return it->second;
}

}
}
}
