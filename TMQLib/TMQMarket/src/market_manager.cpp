#include <TMQMarket/market_manager.h>

#include <TMQUtils/error.h>

using namespace std::string_literals;

namespace TMQ
{

namespace Mkt
{

std::weak_ptr<Market> MarketManager::create( const std::string_view mktHandle, const Context& ctx, const std::shared_ptr<MktDataSource>& source )
{
    std::unique_lock<std::shared_mutex> ul( m_mut );

    if( m_markets.find( mktHandle ) != m_markets.end() )
        throw TMQException( "Cannot create market with handle ["s + mktHandle.data() + "] - handle already in use" );

    // TODO:
    // WILL NEED TO BE DONE ASYNCHRONOUSLY PERHAPS? Or at least need to think about locking quite a lot
    // Create empty market
    // Register to listen for updates (in background will add updates to the market)
    // Load market from the DB
    // Overlay DB market onto existing market

    // For now we just load the mkt from the DB
    auto ins = m_markets.insert( std::make_pair( mktHandle, Mkt::load( ctx, source ) ) );
    return ins.first->second;
}

std::optional<std::weak_ptr<Market>> MarketManager::get( const std::string_view mktHandle ) const
{
    std::shared_lock<std::shared_mutex> sl( m_mut );

    const auto it = m_markets.find( mktHandle );
    if( it == m_markets.end() )
        return std::nullopt;

    return it->second;
}

bool MarketManager::erase( const std::string_view mktHandle )
{
    std::unique_lock<std::shared_mutex> ul( m_mut );

    return static_cast<bool>( m_markets.erase( std::string( mktHandle ) ) );
}

void MarketManager::clear()
{
    std::unique_lock<std::shared_mutex> ul( m_mut );

    m_markets.clear();
}

}

}

