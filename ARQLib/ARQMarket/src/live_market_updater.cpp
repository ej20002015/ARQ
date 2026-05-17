#include <ARQMarket/live_market_updater.h>

#include <ARQUtils/logger.h>
#include <ARQMarket/mktdata_topics.h>

namespace ARQ::MD
{

void LiveMarketUpdater::start()
{
	const bool hasBaseline = m_mktSrcDSH.size();
	m_state = hasBaseline ? State::BUFFERING : State::LIVE;

	const std::string mktNameStr = m_mktName.str();
	m_msgSub = m_msgSvc->subscribe( std::string( SUB_TOPIC_PFX ) + mktNameStr, shared_from_this() );

	if( hasBaseline )
	{
		auto offsetSrc = StreamOffsetSourceFactory::inst().create( m_mktSrcDSH );
		auto mktSrc    = MarketSourceFactory::inst().create( m_mktSrcDSH );

		// Load baseline market along with its offsets

		auto offsets = offsetSrc->getOffsets( std::format( "{}:{}", MARKETS_KEY_NAMESPACE, mktNameStr ) );
		if( offsets )
			m_offsets = convertOffsets( std::move( *offsets ) );
		auto records = mktSrc->load( mktNameStr, m_mktSrcTIDSet );
		m_mkt->update( std::move( records ) );

		// Grab lock to prevent new updates being buffered
		// Iterate through buffered updates and apply to the market
		{
			std::lock_guard<std::mutex> lg( m_bufferedUpdatesMutex );
			for( auto& updateBatch : m_bufferedUpdates )
				applyUpdate( std::move( updateBatch ) );

			m_bufferedUpdates.clear();
		}

		m_state = State::LIVE;
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
			m_bufferedUpdates.emplace_back( std::move( batch ) );
			break;
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
	MktEntity2OffsetsMap offsetUpdates;
	MktEntity2OffsetsMap batchOffsets = convertOffsets( std::move( updateBatch.offsets ) );
	updateBatch.records.visitVectors( [this, &batchOffsets, &offsetUpdates] <c_MktData T> ( std::vector<Record<T>>& newRecords )
	{
		// Drop records if offsets are old
		StreamTopicPartitionOffsets& tpOffsetsInBatch = batchOffsets[Traits<T>::typeEnum()];
		if( auto it = m_offsets.find( Traits<T>::typeEnum() ); it != m_offsets.end() )
		{
			if( !isOffsetsNewer( it->second, tpOffsetsInBatch ) )
			{
				newRecords.clear();
				return;
			}
		}

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

		offsetUpdates[Traits<T>::typeEnum()] = std::move( tpOffsetsInBatch );
	} );

	// Update mkt state and offsets
	if( offsetUpdates.size() )
	{
		m_mkt->update( std::move( updateBatch.records ) );
		for( auto& [k, v] : offsetUpdates )
			m_offsets.insert_or_assign( k, std::move( v ) );
	}
}

LiveMarketUpdater::MktEntity2OffsetsMap LiveMarketUpdater::convertOffsets( StreamTopicPartitionOffsets&& tpOffsets ) const
{
	LiveMarketUpdater::MktEntity2OffsetsMap map;

	for( auto& [tp, offset] : tpOffsets )
	{
		const auto mktEntityType = getTypeFromUpdateTopic( tp.first );
		StreamTopicPartitionOffsets& innerMap = map[mktEntityType];
		innerMap.emplace( tp, offset );
	}

	return map;
}

bool LiveMarketUpdater::isOffsetsNewer( const StreamTopicPartitionOffsets& lhs, const StreamTopicPartitionOffsets& rhs ) const
{
	// If the rhs contains AT LEAST ONE offset that is newer than lhs, return true
	for( const auto& [tp, offset] : rhs )
	{
		if( const auto it = lhs.find( tp ); it == lhs.end() || offset > it->second )
			return true;
	}

	return false;
}

}