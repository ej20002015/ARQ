#pragma once

#include <TMQUtils/hashers.h>
#include <TMQUtils/time.h>
#include <TMQCore/refdata_entities.h>
#include <TMQCore/logger.h>

#include "refdata_source.h"

#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <algorithm>

namespace TMQ
{

template<c_RDEntity T>
using RDDataMap = std::unordered_map<std::string, T, TransparentStringHash, std::equal_to<>>;

template<c_RDEntity T>
class BaseRDCache
{
public:
	virtual ~BaseRDCache() = default;

	virtual std::weak_ptr<RDDataMap<T>> getData() const { return m_data; }

	virtual void reload() = 0;

protected:
	BaseRDCache( const std::shared_ptr<RefDataSource>& source = nullptr )
		: m_rdSource( source ? source : GlobalRefDataSource::get() )
	{}

protected:
	std::shared_ptr<RDDataMap<T>> m_data;
	std::shared_ptr<RefDataSource> m_rdSource;
};

template<c_RDEntity T>
class LiveRDCache : public BaseRDCache<T>
{
public:
	// Use the protected members from the base class
	using BaseRDCache<T>::m_data;
	using BaseRDCache<T>::m_rdSource;

	LiveRDCache( const std::shared_ptr<RefDataSource>& source = nullptr )
		: BaseRDCache<T>( source )
	{
		reload();
	}

	void reload() override
	{
		Log( Module::REFDATA ).debug( "Reloading LiveRDCache<{}>", RDEntityTraits<T>::name() );
		
		std::unique_lock<std::shared_mutex> lock( m_mutex );

		m_data = std::make_shared<RDDataMap<T>>();
		std::vector<T> items = TypedRDSource<T>::fetchLatest( *m_rdSource );
		m_data->reserve( items.size() );
		for( auto&& item : std::move( items ) )
		{
			std::string key = RDEntityTraits<T>::getID( item );
			m_data->emplace( std::move( key ), std::move( item ) );
		}
	}

	[[nodiscard]] std::weak_ptr<RDDataMap<T>> getData() const override
	{ 
		std::shared_lock<std::shared_mutex> lock( m_mutex );
		return m_data;
	}

private:
	mutable std::shared_mutex m_mutex;
};

template<c_RDEntity T>
class LiveRDManager
{
public:
	[[nodiscard]] static std::shared_ptr<LiveRDCache<T>> get()
	{
		static auto inst = std::make_shared<LiveRDCache<T>>();
		return inst;
	}

	static void onReload()
	{
		// TODO: Will be triggered via solace subscription

		Log( Module::REFDATA ).info( "Reloading live {} refdata", RDEntityTraits<T>::name() );

		get().reload();

		{
			std::lock_guard<std::mutex> lock( s_callbackMutex );
			for( const auto& callback : s_callbacks )
				callback();
		}
	}

	static void registerUpdateCallback( std::function<void()> callback )
	{
		std::lock_guard<std::mutex> lock( s_callbackMutex );
		s_callbacks.push_back( std::move( callback ) );
	}

private:
	static inline std::vector<std::function<void()>> s_callbacks;
	static inline std::mutex s_callbackMutex;
};

template<c_RDEntity T>
class HistoricRDCache : public BaseRDCache<T>
{
public:
	// Use the protected members from the base class
	using BaseRDCache<T>::m_data;
	using BaseRDCache<T>::m_rdSource;

	HistoricRDCache( const std::chrono::system_clock::time_point ts, const std::shared_ptr<RefDataSource>& source = nullptr )
		: BaseRDCache<T>( source )
	{
		Log( Module::REFDATA ).debug( "Loading HistoricRDCache<{0}> for ts [{1}]", RDEntityTraits<T>::name(), Time::tpToISO8601Str( ts ) );

		m_data = std::make_shared<RDDataMap<T>>();

		std::vector<T> items = TypedRDSource<T>::fetchAsOf( *m_rdSource );
		m_data->reserve( items.size() );
		for( auto&& item : std::move( items ) )
		{
			std::string key = RDEntityTraits<T>::getID( item );
			m_data->emplace( std::move( key ), std::move( item ) );
		}
	}

	void reload() override {}
};

template<c_RDEntity T>
class RefData
{
public:
	RefData()
		: m_dataPtr( LiveRDManager<T>::get()->getData() )
	{}

	RefData( const std::shared_ptr<BaseRDCache<T>>& rdCache )
		: m_rdCache( rdCache )
		, m_dataPtr( rdCache->getData() )
	{}

	RefData( const std::chrono::system_clock::time_point ts )
		: RefData( std::make_shared<HistoricRDCache<T>>( ts ) )
	{}

	[[nodiscard]] std::optional<std::reference_wrapper<const T>> get( const std::string_view id )
	{
		auto data = m_dataPtr.lock();
		if( !data )
		{
			reload();
			data = m_dataPtr.lock();
		}

		const auto it = data->find( id );
		if( it == data->end() )
			return std::nullopt;

		return std::cref( it->second );
	}

private:
	void reload()
	{
		if( m_rdCache )
			m_dataPtr = m_rdCache->getData();
		else
			m_dataPtr = LiveRDManager<T>::get()->getData();
	}

private:
	std::shared_ptr<BaseRDCache<T>> m_rdCache;
	std::weak_ptr<RDDataMap<T>> m_dataPtr;
};

class RefDBInserter
{
public:
	enum class StaleCheck
	{
		NONE = 0,
		FROM_LIVERD,
		FROM_LIVERD_FORCE_REFRESH
	};

public:
	RefDBInserter( const std::shared_ptr<RefDataSource>& source = nullptr, const StaleCheck staleCheck = StaleCheck::NONE )
		: m_rdSource( source ? source : GlobalRefDataSource::get() )
		, m_staleCheck( staleCheck )
	{}
	RefDBInserter( const StaleCheck staleCheck )
		: RefDBInserter( nullptr, staleCheck )
	{}

	template<c_RDEntity T>
	bool insert( const std::vector<T>& data, const std::shared_ptr<BaseRDCache<T>>& rdCache = nullptr )
	{
		Log( Module::REFDATA ).info( "Inserting refdata: {0} {1}s", data.size(), RDEntityTraits<T>::name() );

		if( m_staleCheck == StaleCheck::FROM_LIVERD_FORCE_REFRESH || m_staleCheck == StaleCheck::FROM_LIVERD )
		{
			const std::shared_ptr<BaseRDCache<T>> rdCacheRef = rdCache ? rdCache : LiveRDManager<T>::get();

			if( m_staleCheck == StaleCheck::FROM_LIVERD_FORCE_REFRESH )
				rdCacheRef->reload();

			RefData<T> rd( rdCacheRef );
			for( const T& dataItem : data )
			{
				const auto cachedData = rd.get( RDEntityTraits<T>::getID( dataItem ) );
				if( cachedData && cachedData->get()._lastUpdatedTs > dataItem._lastUpdatedTs )
				{
					Log( Module::REFDATA ).warn( "Not inserting refdata due to stale record for {0}.{1} - ts of record to insert is {2}; ts of record in cache is {3}", 
												 RDEntityTraits<T>::name(), RDEntityTraits<T>::getID( dataItem ), dataItem._lastUpdatedTs, cachedData->get()._lastUpdatedTs );
					return false;
				}
			}
		}

		TypedRDSource<T>::insert( *m_rdSource, data );
		return true;
	}

	template<c_RDEntity T>
	bool insert( const T& data, const std::shared_ptr<BaseRDCache<T>> rdCache = nullptr )
	{
		return insert( std::vector<T>{ data }, rdCache );
	}

private:
	std::shared_ptr<RefDataSource> m_rdSource;
	StaleCheck m_staleCheck;
};

}