#include <ARQMarket/market_live.h>

#include <ARQUtils/logger.h>

namespace ARQ::MD
{

void LiveMarketUpdater::start()
{
	const bool hasBaseline = m_mktSrcDSH.size();
	m_state = hasBaseline ? State::BUFFERING : State::LIVE;

	const std::string mktNameStr = m_mktName.str();
	auto subHandler = std::make_shared<SubHandler>( weak_from_this(), "LiveMarketUpdater for Mkt: " + mktNameStr );
	m_msgSub = m_msgSvc->subscribe( std::string( SUB_TOPIC_PFX ) + mktNameStr, std::move( subHandler ) );

	if( hasBaseline )
	{
		auto offsetSrc = StreamOffsetSourceFactory::inst().create( m_mktSrcDSH );
		auto mktSrc    = MarketSourceFactory::inst().create( m_mktSrcDSH );

		// Load baseline market along with its offsets

		auto srcOffsets = offsetSrc->getOffsets( std::format( "{}:{}", MARKETS_KEY_NAMESPACE, mktNameStr ) );
		if( srcOffsets )
			m_offsets = std::move( *srcOffsets );
		auto records = mktSrc->load( mktNameStr, m_mktSrcTIDSet );
		m_mkt->update( std::move( records ) );

		// Grab lock to prevent new updates being buffered
		// Iterate through buffered updates and apply to the market
		// Then set state to LIVE so that new updates are applied directly to the market
		{
			std::lock_guard<std::mutex> lg( m_bufferedUpdatesMutex );
			for( auto& updateBatch : m_bufferedUpdates )
				applyUpdate( std::move( updateBatch ) );

			m_bufferedUpdates.clear();
			
			m_state = State::LIVE;
		}
	}
}

void LiveMarketUpdater::stop()
{
	if( m_msgSub )
		m_msgSub->unsubscribe();
}

void LiveMarketUpdater::onMsg( Message&& msg )
{
	MarketUpdateBatch batch;
	try
	{
		batch = m_serialiser->deserialise<MarketUpdateBatch>( msg.data );
	}
	catch( const ARQException& e )
	{
		Log( Module::MKT ).error( e, "LiveMarketUpdater: Error when attempting to deserialise message on topic [{}] into a MarketUpdateBatch object ", msg.topic );
		return;
	}

	switch( m_state )
	{
		case State::BUFFERING:
		{
			std::lock_guard<std::mutex> lg( m_bufferedUpdatesMutex );
			if( m_state == State::BUFFERING )
			{
				m_bufferedUpdates.emplace_back( std::move( batch ) );
				break;
			}
			// If state changed to LIVE while we were waiting for the lock, fall through to apply the update
		}
		case State::LIVE:
			applyUpdate( std::move( batch ) );
			break;
		default:
			break;
	}
}

void LiveMarketUpdater::applyUpdate( MarketUpdateBatch&& updateBatch )
{
	// Drop records if offsets are old
	if( auto it = m_offsets.find( updateBatch.sourcePosition.tp ); it != m_offsets.end() )
	{
		const auto& [tp, currentOffset] = *it;
		if( updateBatch.sourcePosition.offset <= currentOffset )
			return;
	}

	updateBatch.records.visitVectors( [this, &updateBatch] <c_MktData T> ( std::vector<Record<T>>&newRecords )
	{
		// Filter update by TIDSet
		if( !m_msgTIDSet.empty() && newRecords.size() )
		{
			auto allowedIDs = m_msgTIDSet.getIDsForType( Traits<T>::typeEnum() );

			if( std::holds_alternative<TIDSet::None>( allowedIDs ) )
				newRecords.clear();
			else if( auto* listPtr = std::get_if<TIDSet::IDList>( &allowedIDs ) )
			{
				std::erase_if( newRecords, [&listPtr] ( const Record<T>& rec )
				{
					return std::find( listPtr->begin(), listPtr->end(), rec.header.id ) == listPtr->end();
				} );
			}
		}
	} );

	// Update mkt state
	if( updateBatch.records.size() )
		m_mkt->update( std::move( updateBatch.records ) );
	m_offsets[updateBatch.sourcePosition.tp] = updateBatch.sourcePosition.offset;
}

}
