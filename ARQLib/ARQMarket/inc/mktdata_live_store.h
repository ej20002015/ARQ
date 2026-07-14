#pragma once
#include <ARQMarket/dll.h>

#include <ARQUtils/global_accessor.h>
#include <ARQUtils/hashers.h>
#include <ARQCore/streaming_service.h>
#include <ARQMarket/market.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace ARQ::MD
{

struct MarketUpdateBatch
{
	MarketName                  marketName;
	RecordCollection            records;
	StreamTopicPartitionOffsets offsets;
};

class ILiveMarketStore
{
public:
	virtual ~ILiveMarketStore() = default;

	virtual void apply( const MarketUpdateBatch& updateBatch ) = 0;
};

using LiveMarketStoreCreateFunc = std::add_pointer<ILiveMarketStore*( const std::string_view dsh )>::type;

class LiveMarketStoreFactory : public GlobalAccessor<LiveMarketStoreFactory, "LiveMarketStoreFactory">
{
public:
	[[nodiscard]] ARQMarket_API std::shared_ptr<ILiveMarketStore> create( const std::string_view dsh );

	ARQMarket_API void addCustomStore( const std::string_view dsh, const std::shared_ptr<ILiveMarketStore>& store );
	ARQMarket_API void delCustomStore( const std::string_view dsh );

private:
	std::unordered_map<std::string, std::shared_ptr<ILiveMarketStore>, TransparentStringHash, std::equal_to<>> m_stores;
	std::mutex m_storesMutex;
};

}

ARQ_REG_TYPE( ARQ::MD::MarketUpdateBatch )