#pragma once
#include <ARQMarket/dll.h>

#include <ARQCore/messaging_service.h>
#include <ARQCore/stream_offset_source.h>
#include <ARQCore/serialiser.h>
#include <ARQMarket/mktdata_source.h>
#include <ARQMarket/market.h>

namespace ARQ::MD
{

struct MarketUpdateBatch
{
	MarketName                  marketName;
	RecordCollection            records;
	StreamTopicPartitionOffsets offsets;
};

class LiveMarketUpdater : public ISubscriptionHandler,
	                      public std::enable_shared_from_this<LiveMarketUpdater>
{
public:
	struct Params
	{
		const std::shared_ptr<Market>& mkt;
		const std::string_view         mktSrcDSH;
		const std::string_view         msgSvcDSH;
		const MarketName&              mktName;
		const TIDSet&                  mktSrcTIDSet;
		const TIDSet&                  msgTIDSet;
	};

	enum class State
	{
		INIT,
		BUFFERING,
		LIVE
	};

private:
	struct Passkey {};

public:
	LiveMarketUpdater( Passkey, Params&& params )
		: m_mkt( params.mkt )
		, m_mktSrcDSH( params.mktSrcDSH )
		, m_msgSvc( MessagingServiceFactory::inst().create( params.msgSvcDSH ) )
		, m_mktName( params.mktName )
		, m_mktSrcTIDSet( params.mktSrcTIDSet )
		, m_msgTIDSet( params.msgTIDSet )
		, m_desc( "LiveMarketUpdater for Mkt: " + m_mktName.str() )
		, m_state( State::INIT )
		, m_serialiser( SerialiserFactory::inst().create( SerialiserFactory::SerialiserImpl::Protobuf ) )
	{
	}

	static std::shared_ptr<LiveMarketUpdater> create( Params&& params )
	{
		return std::make_shared<LiveMarketUpdater>( Passkey(), std::move( params ) );
	}

	ARQMarket_API void start();
	ARQMarket_API void stop();

	[[nodiscard]] ARQMarket_API const std::shared_ptr<Market>& market() { return m_mkt; }

	static constexpr std::string_view SUB_TOPIC_PFX = "ARQ.MktData.Updates.";

private: // ISubscriptionHandler implementation
	ARQMarket_API void             onMsg( Message&& msg )       override;
	              std::string_view getDesc()              const override { return m_desc; };

private:
	void applyUpdate( MarketUpdateBatch&& updateBatch );

	using MktEntity2OffsetsMap = std::unordered_map<Type, StreamTopicPartitionOffsets>;

	MktEntity2OffsetsMap convertOffsets( StreamTopicPartitionOffsets&& tpOffsets ) const;
	bool                 isOffsetsNewer( const StreamTopicPartitionOffsets& lhs, const StreamTopicPartitionOffsets& rhs ) const;

private:
	std::shared_ptr<Market>            m_mkt;
	std::string                        m_mktSrcDSH;
	std::shared_ptr<IMessagingService> m_msgSvc;
	MarketName                         m_mktName;
	TIDSet                             m_mktSrcTIDSet;
	TIDSet                             m_msgTIDSet;
	std::string                        m_desc;

	std::atomic<State>                 m_state;
	MktEntity2OffsetsMap               m_offsets;

	std::shared_ptr<Serialiser>        m_serialiser;
	std::unique_ptr<ISubscription>     m_msgSub;
	std::vector<MarketUpdateBatch>     m_bufferedUpdates;
	std::mutex                         m_bufferedUpdatesMutex;
};

}

ARQ_REG_TYPE( ARQ::MD::MarketUpdateBatch )