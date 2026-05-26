#pragma once

#include <ARQUtils/hashers.h>
#include <ARQUtils/logger.h>
#include <ARQCore/refdata_entities.h>
#include <ARQCore/refdata_source.h>

#include <ankerl/unordered_dense.h>

#include <vector>
#include <atomic>
#include <memory>
#include <algorithm>
#include <tuple>

namespace ARQ::RD
{

// Forward declarations
template<c_RefData T> class Cache;
template<c_RefData T> void buildCacheIndexes( Cache<T>& cache );

template<c_RefData T>
class Cache
{
public:
    using RecordMap = ankerl::unordered_dense::map<ID::UUID, Record<T>>;

public:
    explicit Cache( std::vector<Record<T>>&& records )
    {
        for( const auto& record : records )
            m_map.emplace( record.header.uuid, std::move( record ) );

		buildCacheIndexes<T>( *this );
    }

	[[nodiscard]] const RecordMap& getMap() const
	{
		return m_map;
	}

    [[nodiscard]] const std::vector<std::pair<ID::UUID, Record<T>>>& getList() const
    {
        return m_map.values();
    }

    [[nodiscard]] bool empty() const
    {
        return m_map.empty();
    }

    [[nodiscard]] size_t size() const
    {
        return m_map.size();
    }

    [[nodiscard]] OptConstRef<Record<T>> getRecord( const ID::UUID& id ) const
    {
        const auto it = m_map.find( id );
        if( it == m_map.end() )
            return nullptr;
        else
            return &( it->second );
    }

    [[nodiscard]] OptConstRef<T> get( const ID::UUID& id ) const
    {
        auto recordOpt = getRecord( id );
        return recordOpt ? &( recordOpt->data ) : nullptr;
    }

    [[nodiscard]] OptConstRef<Record<T>> getRecordByIndex( const std::string_view indexName, const std::string_view indexValue ) const
    {
        const auto it = std::find_if( m_uniqueIndices.begin(), m_uniqueIndices.end(), [&] ( const auto& pair ) { return pair.first == indexName; } );
        if( it == m_uniqueIndices.end() )
            throw ARQException( std::format( "Cache::getRecordByIndex: Given field [{}] is not a unique index for RefData entity [{}]", indexName, Traits<T>::name() ) );

        const UniqueIndexMap& indexMap = it->second;
        const auto indexIt = indexMap.find( indexValue );
        if( indexIt == indexMap.end() )
            return nullptr;
        else
            return getRecord( indexIt->second );
    }

    [[nodiscard]] OptConstRef<T> getByIndex( const std::string_view indexName, const std::string_view indexValue ) const
    {
        auto recordOpt = getRecordByIndex( indexName, indexValue );
        return recordOpt ? &( recordOpt->data ) : nullptr;
    }

    [[nodiscard]] std::vector<OptConstRef<Record<T>>> getRecordsByNonUniqIndex( const std::string_view indexName, const std::string_view indexValue ) const
    {
        const auto it = std::find_if( m_nonUniqueIndices.begin(), m_nonUniqueIndices.end(), [&] ( const auto& pair ) { return pair.first == indexName; } );
        if( it == m_nonUniqueIndices.end() )
            throw ARQException( std::format( "Cache::getRecordsByNonUniqIndex: Given field [{}] is not a non-unique index for RefData entity [{}]", indexName, Traits<T>::name() ) );

        std::vector<OptConstRef<Record<T>>> results;
        
        const NonUniqueIndexMap& indexMap = it->second;
        const auto indexIt = indexMap.find( indexValue );
        if( indexIt != indexMap.end() )
        {
            for( const auto& id : indexIt->second )
            {
                auto recordOpt = getRecord( id );
                if( recordOpt )
                    results.push_back( recordOpt );
            }
        }

		return results;
    }

    [[nodiscard]] std::vector<OptConstRef<T>> getByNonUniqIndex( const std::string_view indexName, const std::string_view indexValue ) const
    {
        auto records = getRecordsByNonUniqIndex( indexName, indexValue );

        std::vector<std::reference_wrapper<const T>> results;
		for( const auto& recordRef : records )
			results.push_back( *( recordRef.data ) );

		return results;
    }

private:
    using UniqueIndexMap    = ankerl::unordered_dense::map<std::string_view, ID::UUID, AnkerlTransparentStringHash, std::equal_to<>>;
    using NonUniqueIndexMap = ankerl::unordered_dense::map<std::string_view, std::vector<ID::UUID>, AnkerlTransparentStringHash, std::equal_to<>>;
    using UniqueIndices     = std::vector<std::pair<std::string_view, UniqueIndexMap>>;
    using NonUniqueIndices  = std::vector<std::pair<std::string_view, NonUniqueIndexMap>>;

private:
    RecordMap        m_map;
	UniqueIndices    m_uniqueIndices;
	NonUniqueIndices m_nonUniqueIndices;

private:
	friend void buildCacheIndexes<T>( Cache<T>& cache );
};

template<typename T>
class RepositoryImpl;

template<c_RefData... Entities>
class RepositoryImpl<EntityRecordList<Record<Entities>...>>
{
private:
    template<typename T>
    struct CacheSlot
    {
        std::atomic<std::shared_ptr<Cache<T>>> cache;
        std::mutex                             loadMtx;
    };

public:
    explicit RepositoryImpl( const std::string_view dsh )
        : m_rdSource( SourceFactory::inst().create( dsh ) )
    {
    }

    template<c_RefData T>
    std::shared_ptr<Cache<T>> get() const
    {
        auto& slot = std::get<CacheSlot<T>>( m_slots );
        auto ptr = slot.cache.load( std::memory_order_acquire );

        if( !ptr )
        {
            std::lock_guard<std::mutex> lg( slot.loadMtx );
            ptr = slot.cache.load( std::memory_order_acquire );
            if( !ptr )
                ptr = load<T>( slot );
        }
        return ptr;
    }

private:
    template<c_RefData T>
    std::shared_ptr<Cache<T>> load( CacheSlot<T>& cacheSlot ) const
    {
        Log( Module::REFDATA ).info( "RD::Repository: Loading Reference Data for entity [{}]", Traits<T>::name() );

        std::vector<Record<T>>    records = m_rdSource->fetch<T>();
        std::shared_ptr<Cache<T>> newCache = std::make_shared<Cache<T>>( std::move( records ) );

        cacheSlot.cache.store( newCache, std::memory_order_release );

        Log( Module::REFDATA ).info( "RD::Repository: Finished loading Reference Data for entity [{}]", Traits<T>::name() );

        return newCache;
    }

private:
    std::shared_ptr<Source> m_rdSource;

    mutable std::tuple<CacheSlot<Entities>...> m_slots;
};

using Repository = RepositoryImpl<AllEntityRecords>;

}

#include <ARQCore/refdata_cache_indexes.h>