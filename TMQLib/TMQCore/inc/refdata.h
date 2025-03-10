#include <TMQUtils/hashers.h>
#include <TMQUtils/buffer.h>

#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <functional>

// TEST
#include <iostream>

namespace TMQ
{

struct RDEntity
{
	std::string ID;
	std::chrono::system_clock::time_point lastUpdated;
};

template<typename T>
concept c_RDEntity = std::is_base_of_v<RDEntity, T>;

template<c_RDEntity T>
class RDEntityTraits
{
public:
	static consteval const char* const schemaName() { static_assert( false ); }
};

struct User : public RDEntity
{
	std::string firstname;
	std::string surname;
	std::string desk;
	uint32_t age;
};

template<>
class RDEntityTraits<User>
{
public:
	static consteval const char* const schemaName() { return "Users"; }
};

class RefDataSource
{
public:
	virtual ~RefDataSource() = default;

	virtual std::vector<OwningBuffer> fetchLatest() = 0;
	virtual std::vector<OwningBuffer> fetchAsOf( const std::chrono::system_clock::time_point ts ) = 0;
};

class TSDBRefDataSource : public RefDataSource
{
public:
	std::vector<OwningBuffer> fetchLatest()
	{
		return std::vector<OwningBuffer>();
	}

	std::vector<OwningBuffer> fetchAsOf( const std::chrono::system_clock::time_point ts )
	{
		return std::vector<OwningBuffer>();
	}
};

template<c_RDEntity T>
using RDDataMap = std::unordered_map<std::string, T, TransparentStringHash, std::equal_to<>>;

// TODO: Have some way of setting this
static std::shared_ptr<RefDataSource> getGlobalRefDataSource()
{
	static auto globalSource = std::make_shared<TSDBRefDataSource>();
	return globalSource;
}

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

template <c_RDEntity T>
class TypedRefDataSource
{
public:
	static std::vector<T> fetchLatest( RefDataSource& source )
	{
		std::vector<OwningBuffer> rawData = source.fetchLatest();
		return deserialize<T>( rawData );
	}

	static std::vector<T> fetchAsOf( RefDataSource& source, const std::chrono::system_clock::time_point ts )
	{
		std::vector<OwningBuffer> rawData = source.fetchAsOf( ts );
		return deserialize<T>( rawData );
	}

private:
	static std::vector<T> deserialize( const std::vector<OwningBuffer>& rawData )
	{
		std::vector<T> result;
		for( const auto& blob : rawData )
		{
			//TODO
		}
		return result;
	}
};

template<c_RDEntity T>
class LiveRDCache : public BaseRDCache<T>
{
public:
	// Use the protected members from the base class
	using BaseRDCache<T>::m_data;
	using BaseRDCache<T>::m_rdSource;

	LiveRDCache();

	void reload()
	{
		std::unique_lock<std::shared_mutex> lock( m_mutex );

		std::vector<T> records = TypedRefDataSource<T>::fetchLatest( *m_rdSource );
		m_data = std::make_shared<RDDataMap<T>>();
		for( const auto record : records )
		{
			m_data->insert( std::make_pair( record.ID, record ) );
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
inline LiveRDCache<T>::LiveRDCache()
{
	this->m_data = std::make_shared<RDDataMap<T>>();
	this->m_data->insert( std::make_pair( "Evan", User() ) );
}

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

	HistoricRDCache( const std::chrono::system_clock::time_point ts );
};

template<c_RDEntity T>
inline HistoricRDCache<T>::HistoricRDCache( const std::chrono::system_clock::time_point ts )
{
	TypedRefDataSource<T>::fetchAsOf( *m_rdSource );
}

template<c_RDEntity T>
class RefData
{
public:
	RefData()
		: m_data( LiveRDManager<T>::get().getData() )
	{}

	RefData( const std::chrono::system_clock::time_point ts )
		: RefData( std::make_shared<HistoricRDCache<T>>( ts ) )
	{}

	RefData( const std::shared_ptr<BaseRDCache<T>>& rdCache )
		: m_rdCache( rdCache )
		, m_data( rdCache->getData() )
	{}

	[[nodiscard]] std::optional<std::reference_wrapper<const T>> get( const std::string_view id )
	{
		auto data = m_data.lock();
		if( !data )
		{
			reload();
			data = m_data.lock();
		}

		const auto it = data->find( id );
		if( it == data->end() )
			return std::nullopt;

		return std::cref( it->second );
	}

private:
	void reload()
	{
		m_data = m_rdCache->getData();
	}

private:
	std::shared_ptr<BaseRDCache<T>> m_rdCache;
	std::weak_ptr<RDDataMap<T>> m_data;
};

}