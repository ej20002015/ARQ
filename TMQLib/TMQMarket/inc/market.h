#pragma once
#include <TMQMarket/dll.h>

#include <TMQMarket/mktdata_entities.h>
#include <TMQUtils/hashers.h>

#include <unordered_map>
#include <shared_mutex>
#include <chrono>

namespace TMQ
{

namespace Mkt
{

template<c_MDEntity T>
class MarketStore
{
public:
	MarketStore() = default;

	[[nodiscard]] std::optional<std::reference_wrapper<const T>> get( const std::string_view iid ) const
	{
		std::shared_lock<std::shared_mutex> sl( m_mut );

		const auto it = m_objects.find( iid );
		if( it == m_objects.end() )
			return std::nullopt;

		return std::cref( it->second );
	}

	void set( const std::string& iid, const T& obj )
	{
		std::unique_lock<std::shared_mutex> ul( m_mut );

		m_objects[iid] = obj;
	}

private:
	std::unordered_map<std::string, T, TransparentStringHash, std::equal_to<>> m_objects;
	mutable std::shared_mutex m_mut;
};

class Market
{
public:
	Market() = default;

	template<c_MDEntity T>
	[[nodiscard]] std::optional<std::reference_wrapper<const T>> get( const std::string_view iid ) const
	{
		if      constexpr( std::is_same_v<T, FXRate> ) return m_fxRates.get( iid );
		else if constexpr( std::is_same_v<T, EQ> )     return m_equities.get( iid );
		else
		{
			static_assert( false, "Market::get<T> not implemented for this MD Entity" );
		}
	}

	template<c_MDEntity T>
	void set( const T& newObj )
	{
		if      constexpr( std::is_same_v<T, FXRate> ) return m_fxRates.set( newObj.instrumentID, newObj );
		else if constexpr( std::is_same_v<T, EQ> )     return m_equities.set( newObj.instrumentID, newObj );
		else
		{
			static_assert( false, "Market::set<T> not implemented for this MD Entity" );
		}
	}

private:
	MarketStore<FXRate> m_fxRates;
	MarketStore<EQ>     m_equities;
};

struct Context
{
	std::string tag;
	std::optional<std::chrono::year_month_day> date;

	//inline static Context LIVE = Context{ "LIVE", std::nullopt };
};

TMQMarket_API Market load( Context mktCtx ); // TODO
TMQMarket_API void save( const Market& mkt );

}

}