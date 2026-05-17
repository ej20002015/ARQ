#pragma once
#include <ARQMarket/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQMarket/mktdata_entities.h>

#include <ankerl/unordered_dense.h>

#include <memory>
#include <tuple>
#include <mutex>

namespace ARQ::MD
{

struct MarketName
{
	std::string tag;
	Time::Date  date;

	MarketName() = default;
	MarketName( const std::string_view tag, const Time::Date date = Time::Date() )
		: tag( tag )
		, date( date )
	{}

	bool operator==( const MarketName& ) const = default;

    bool isSet() const { return !tag.empty(); }

    ARQMarket_API static MarketName fromStr( const std::string_view nameStr );

	ARQMarket_API std::string str() const;

	ARQMarket_API static const MarketName LIVE;
};

template<typename T>
class MarketImpl;

template<typename T>
class MarketSnapshotImpl;

template<c_MktData... Entities>
class MarketSnapshotImpl<EntityRecordList<Record<Entities>...>>
{
public:
	MarketSnapshotImpl() = default;
	MarketSnapshotImpl( Time::DateTime asofTs )
		: m_asofTs( asofTs )
	{}

	template<c_MktData Entity>
	OptRef<Record<Entity>> const get( const std::string_view id ) const
	{
		const auto& map = std::get<RecordMap<Entity>>( m_data );
		if( auto it = map.find( id ); it != map.end() )
			return it->second.get();
		else
			return {};
	}

	[[nodiscard]] Time::DateTime asofTs() const { return m_asofTs; }

private:
	template<c_MktData Entity> // TODO: Use mimallocator
	using RecordMap = ankerl::unordered_dense::map<std::string, std::shared_ptr<const Record<Entity>>, AnkerlTransparentStringHash, std::equal_to<>>;

private:
	template<c_MktData Entity>
	void set( const Record<Entity>& rcd )
	{
		std::get<RecordMap<Entity>>( m_data )[rcd.header.id] = std::make_shared<const Record<Entity>>( rcd );
	}

	template<c_MktData Entity>
	void erase( const std::string_view id )
	{
		std::get<RecordMap<Entity>>( m_data ).erase( id );
	}

private:
	std::tuple<RecordMap<Entities>...> m_data;
	Time::DateTime                     m_asofTs;

private:
	friend MarketImpl<EntityRecordList<Record<Entities>...>>;
};

using MarketSnapshot = MarketSnapshotImpl<AllEntityRecords>;

template<c_MktData... Entities>
class MarketImpl<EntityRecordList<Record<Entities>...>>
{
public:
	using Snapshot = MarketSnapshotImpl<EntityRecordList<Record<Entities>...>>;

public:
	MarketImpl()
		: m_currentSnapshot( std::make_shared<Snapshot>() )
	{
	}

public:
	[[nodiscard]] std::shared_ptr<const Snapshot> snapshot() const
	{
		return m_currentSnapshot.load( std::memory_order_acquire );
	}

	void update( RecordCollection&& records )
	{
		std::lock_guard<std::mutex> lock( m_writerMutex );

		// Copy current mkt state
		std::shared_ptr<Snapshot> newSnapshot = std::make_shared<Snapshot>( *m_currentSnapshot.load( std::memory_order_relaxed ) );

		// Update copy with new data
		Time::DateTime latestTs = newSnapshot->m_asofTs;
		records.visitVectors( [&newSnapshot, &latestTs] <c_MktData T> ( std::vector<Record<T>>& recs )
		{
			for( const auto& rec : recs )
			{
				if( const auto current = newSnapshot->template get<T>( rec.header.id ) )
				{
					if( rec.header.asofTs < current->header.asofTs )
						continue;
				}

				if( rec.header.isActive )
					newSnapshot->template set<T>( rec );
				else
					newSnapshot->template erase<T>( rec.header.id );

				if( rec.header.asofTs > latestTs )
					latestTs = rec.header.asofTs;
			}
		} );
		newSnapshot->m_asofTs = latestTs;

		// Overwrite mkt state wiht updated snapshot
		m_currentSnapshot.store( newSnapshot, std::memory_order_release );
	}

private:
	std::atomic<std::shared_ptr<const Snapshot>> m_currentSnapshot;
	std::mutex m_writerMutex;
};

using Market = MarketImpl<AllEntityRecords>;

}
