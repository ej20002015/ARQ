#pragma once
#include <TMQMarket/dll.h>

#include <TMQUtils/hashers.h>
#include <TMQCore/mktdata_source.h>
#include <TMQMarket/mktdata_entities.h>

#include <unordered_map>
#include <shared_mutex>
#include <chrono>

namespace TMQ
{

namespace Mkt
{

struct Context
{
	std::string tag;
	std::optional<std::chrono::year_month_day> date;

	TMQMarket_API static Context LIVE;

	std::string str() const;
};

template<c_MDEntity T>
using MktObjMap = std::unordered_map<std::string, T, TransparentStringHash, std::equal_to<>>;

template<c_MDEntity T>
class MarketStore
{
public:
	MarketStore() = default;

	[[nodiscard]] std::optional<T> get( const std::string_view iid ) const
	{
		std::shared_lock<std::shared_mutex> sl( m_mut );

		const auto it = m_objects.find( iid );
		if( it == m_objects.end() )
			return std::nullopt;

		return it->second;
	}

	void set( const std::string& iid, const T& obj )
	{
		std::unique_lock<std::shared_mutex> ul( m_mut );

		m_objects[iid] = obj;
	}

	size_t size() const noexcept
	{
		std::shared_lock<std::shared_mutex> sl( m_mut );

		return m_objects.size();
	}

	MktObjMap<T> objMap() const
	{
		std::shared_lock<std::shared_mutex> sl( m_mut );

		return m_objects;
	}

private:
	MktObjMap<T> m_objects;
	mutable std::shared_mutex m_mut;
};

class Market;

class MarketSnapshot
{
public:
	template<c_MDEntity T>
	[[nodiscard]] std::optional<T> get( const std::string_view iid ) const
	{
		if      constexpr( std::is_same_v<T, FXRate> ) return get( iid, m_fxRates );
		else if constexpr( std::is_same_v<T, EQ> )     return get( iid, m_equities );
		else
		{
			static_assert( false, "MarketSnapshot::get<T> not implemented for this MD Entity" );
		}
	}

	template<c_MDEntity T>
	[[nodiscard]] const MktObjMap<T>& getAllObjs() const
	{
		if      constexpr( std::is_same_v<T, FXRate> ) return m_fxRates;
		else if constexpr( std::is_same_v<T, EQ> )     return m_equities;
		else
		{
			static_assert( false, "MarketSnapshot::getAllObjs<T> not implemented for this MD Entity" );
		}
	}

	TMQMarket_API size_t size() const noexcept;

private:
	// Only a Market can create a MarketSnapshot
	MarketSnapshot() = default;
	friend class Market;

	template<c_MDEntity T>
	[[nodiscard]] std::optional<T> get( const std::string_view iid, const MktObjMap<T>& objs ) const
	{
		const auto it = objs.find( iid );
		if( it == objs.end() )
			return std::nullopt;

		return it->second;
	}

private:
	MktObjMap<FXRate> m_fxRates;
	MktObjMap<EQ>     m_equities;
};

class Market
{
public:
	Market() = default;

	template<c_MDEntity T>
	[[nodiscard]] std::optional<T> get( const std::string_view iid ) const
	{
		std::shared_lock<std::shared_mutex> sl( m_mut );

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
		std::shared_lock<std::shared_mutex> sl( m_mut );

		if      constexpr( std::is_same_v<T, FXRate> ) m_fxRates.set( newObj.instrumentID, newObj );
		else if constexpr( std::is_same_v<T, EQ> )     m_equities.set( newObj.instrumentID, newObj );
		else
		{
			static_assert( false, "Market::set<T> not implemented for this MD Entity" );
		}
	}

	TMQMarket_API MarketSnapshot snap() const;

private:
	MarketStore<FXRate> m_fxRates;
	MarketStore<EQ>     m_equities;

	mutable std::shared_mutex m_mut;
};

TMQMarket_API std::shared_ptr<Market> load( const Context& ctx, const std::shared_ptr<MktDataSource>& source = nullptr );
TMQMarket_API void save( const std::shared_ptr<Market> mkt, const Context& ctx, const std::shared_ptr<MktDataSource>& source = nullptr );
TMQMarket_API void save( const MarketSnapshot& mktSnapshot, const Context& ctx, const std::shared_ptr<MktDataSource>& source = nullptr );

}

}