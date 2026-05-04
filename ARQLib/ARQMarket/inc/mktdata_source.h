#pragma once
#include <ARQMarket/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQUtils/error.h>
#include <ARQUtils/global_accessor.h>
#include <ARQMarket/mktdata_entities.h>
#include <ARQMarket/tid.h>

#include <string>
#include <mutex>
#include <functional>
#include <unordered_map>

namespace ARQ::MD
{

class IMarketSource
{
public:
	virtual RecordCollection load( const std::string_view marketName, const TIDSet& filter = ARQ::MD::TIDSet{} ) = 0;
	virtual void             save( const std::string_view marketName, const RecordCollection& records )          = 0;
};

using MarketSourceCreateFunc = std::add_pointer<IMarketSource* ( const std::string_view dsh )>::type;

class MarketSourceFactory : public GlobalAccessor<MarketSourceFactory, "MarketSourceFactory">
{
public:
	[[nodiscard]] ARQMarket_API std::shared_ptr<IMarketSource> create( const std::string_view dsh );

	ARQMarket_API void addCustomSource( const std::string_view dsh, const std::shared_ptr<IMarketSource>& source );
	ARQMarket_API void delCustomSource( const std::string_view dsh );

private:
	std::unordered_map<std::string, std::shared_ptr<IMarketSource>, TransparentStringHash, std::equal_to<>> m_sources;
	std::mutex m_sourcesMutex;
};

}