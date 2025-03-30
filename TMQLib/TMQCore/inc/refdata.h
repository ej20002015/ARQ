#pragma once

#include <TMQUtils/hashers.h>
#include <TMQUtils/buffer.h>
#include <TMQCore/refdata_entities.h>

#include "refdata_source.h"

#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <functional>

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

protected:
	BaseRDCache( const std::shared_ptr<RefDataSource>& source = nullptr )
		: m_rdSource( source ? source : getGlobalRefDataSource() )
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

	void reload()
	{
		std::unique_lock<std::shared_mutex> lock( m_mutex );

		std::vector<T> records = TypedRDSource<T>::fetchLatest( *m_rdSource );
		m_data = std::make_shared<RDDataMap<T>>();
		for( const auto record : records )
		{
			m_data->insert( std::make_pair( RDEntityTraits<T>::getID( record ), record ) );
		}
	}

	std::weak_ptr<RDDataMap<T>> getData() const override
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
	static LiveRDCache<T>& get()
	{
		static LiveRDCache<T> inst;
		return inst;
	}

	static void onReload()
	{
		// TODO: Will be triggered via solace subscription
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
		s_callbacks.push_back( callback );
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
		std::vector<T> records = TypedRDSource<T>::fetchAsOf( *m_rdSource );
		m_data = std::make_shared<RDDataMap<T>>();
		for( const auto record : records )
		{
			m_data->insert( std::make_pair( RDEntityTraits<T>::getID( record ), record ) );
		}
	}
};

template<c_RDEntity T>
class RefData
{
public:
	RefData()
		: m_dataPtr( LiveRDManager<T>::get().getData() )
	{}

	RefData( const std::chrono::system_clock::time_point ts )
		: RefData( std::make_shared<HistoricRDCache<T>>( ts ) )
	{}

	RefData( const std::shared_ptr<BaseRDCache<T>>& rdCache )
		: m_rdCache( rdCache )
		, m_dataPtr( rdCache->getData() )
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
			m_dataPtr = LiveRDManager<T>::get().getData();
	}

private:
	std::shared_ptr<BaseRDCache<T>> m_rdCache;
	std::weak_ptr<RDDataMap<T>> m_dataPtr;
};

class RefDBInserter
{
public:
	RefDBInserter( const std::shared_ptr<RefDataSource>& source = nullptr )
		: m_rdSource( source ? source : getGlobalRefDataSource() )
	{}

	template<c_RDEntity T>
	void insert( std::vector<T>&& data )
	{
		// TODO: Check timestamps from LiveRDCache to see if data is stale - do a reload as well
		TypedRDSource<T>::insert( *m_rdSource, std::move( data ) );
	}

	template<c_RDEntity T>
	void insert( T&& data )
	{
		TypedRDSource<T>::insert( *m_rdSource, std::vector<T>{ std::move( data ) } );
	}

private:
	std::shared_ptr<RefDataSource> m_rdSource;
};

}