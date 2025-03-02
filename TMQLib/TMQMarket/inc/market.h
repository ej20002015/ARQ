#pragma once

#include "dll.h"

#include "TMQMarket/types.h"
#include <TMQUtils/hashers.h>

#include <unordered_map>
#include <mutex>
#include <functional>
#include <set>

namespace TMQ
{

template<MktType::c_MktType T>
class MarketStore
{
public:
	MarketStore() = default;

	[[nodiscard]] std::optional<std::reference_wrapper<const T>> get( const std::string_view id ) const
	{
		std::lock_guard<std::mutex> lock( m_mutex );

		const auto it = m_objects.find( id );
		if( it == m_objects.end() )
			return std::nullopt;

		return std::cref( it->second );
	}

	void set( const std::string& id, const T& obj )
	{
		std::lock_guard<std::mutex> lock( m_mutex );

		m_objects[id] = obj;
	}

private:
	std::unordered_map<std::string, T, TransparentStringHash, std::equal_to<>> m_objects;
	mutable std::mutex m_mutex;
};

class Market
{
public:
	class Listener
	{
	public:
		virtual void onMktUpdate( const MktType::Type type, const std::string_view id ) = 0;
	};

public:
	Market() = default;

	/*
	* MktType getters and setters
	*/

	[[nodiscard]] std::optional<std::reference_wrapper<const MktType::FXPair>> getFxPair( const std::string_view id ) const
	{
		return m_fxPairs.get( id );
	}

	void setFxPair( const std::string& id, const MktType::FXPair& fxPair )
	{
		m_fxPairs.set( id, fxPair );
		notifyListeners( MktType::FX_PAIR, id );
	}

	[[nodiscard]] std::optional<std::reference_wrapper<const MktType::Equity>> getEquity( const std::string_view id ) const
	{
		return m_equities.get( id );
	}

	void setEquity( const std::string& id, const MktType::Equity& equity )
	{
		m_equities.set( id, equity );
		notifyListeners( MktType::EQUITY, id );
	}

	/* 
	* Mkt listener
	*/
	
	void registerListener( Listener* const listener )
	{
		std::lock_guard<std::mutex> lock( m_listenerMutex );
		m_listeners.insert( listener );
	}

private:
	void notifyListeners( const MktType::Type type, const std::string_view id ) const
	{
		std::lock_guard<std::mutex> lock( m_listenerMutex );

		for( Listener* const listener : m_listeners )
			listener->onMktUpdate( type, id );
	}

private:
	MarketStore<MktType::FXPair> m_fxPairs;
	MarketStore<MktType::Equity> m_equities;

	mutable std::mutex m_listenerMutex;
	std::set<Listener*> m_listeners;
};

TMQ_API std::string MktDummyFunction();

}