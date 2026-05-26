#pragma once

#include <ARQCore/refdata_repository.h>

namespace ARQ::RD
{

template<c_RefData T>
class Cache_Adapter
{
public:
	Cache_Adapter() = default;

	explicit Cache_Adapter( std::shared_ptr<Cache<T>> cache )
		: m_cache( std::move( cache ) )
	{
	}

	[[nodiscard]] std::vector<const Record<T>*> getList() const
	{
		std::vector<const ARQ::RD::Record<T>*> results;
		if( !m_cache )
			return results;

		const auto& kvpList = m_cache->getList();
		results.reserve( kvpList.size() );

		for( const auto& [id, record] : kvpList )
			results.push_back( &record );

		return results;
	}

	[[nodiscard]] const Record<T>* getRecord( const ID::UUID& id ) const
	{
		if( !m_cache )
			return nullptr;

		auto optRecord = m_cache->getRecord( id );
		return optRecord.has_value() ? &optRecord.value() : nullptr;
	}

private:
	std::shared_ptr<Cache<T>> m_cache;
};

class Repository_Adapter
{
public:
	explicit Repository_Adapter( const char* dsh )
		: m_repo( dsh )
	{
	}

	template<c_RefData T>
	Cache_Adapter<T> get() const
	{
		return Cache_Adapter<T>( m_repo.get<T>() );
	}

private:
	Repository m_repo;
};

}