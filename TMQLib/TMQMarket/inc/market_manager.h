#pragma once
#include <TMQMarket/dll.h>

#include <TMQUtils/hashers.h>
#include <TMQCore/mktdata_source.h>
#include <TMQMarket/market.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <optional>

namespace TMQ
{

namespace Mkt
{

class MarketManager
{
public:
	static MarketManager& inst()
	{
        // TODO: In constructor be able to pass in things like MktDataSource, etc.
		static auto inst = MarketManager();
		return inst;
	}

	TMQMarket_API std::weak_ptr<Market> create( const std::string_view mktHandle, const Context& ctx, const std::shared_ptr<MktDataSource>& source = nullptr );
	TMQMarket_API std::optional<std::weak_ptr<Market>> get( const std::string_view mktHandle ) const;
	TMQMarket_API bool erase( const std::string_view mktHandle );

	// TODO: Temproary measure for testing - don't think I'll need it when I've sorted out proper singleton management
	TMQMarket_API void clear();

private:
	MarketManager() = default;

private:
	std::unordered_map<std::string, std::shared_ptr<Market>, TransparentStringHash, std::equal_to<>> m_markets;
	mutable std::shared_mutex m_mut;
};

}

}