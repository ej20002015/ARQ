#pragma once
#include <ARQMarket/dll.h>

#include <ARQMarket/mktdata_entities.h>

#include <vector>

namespace ARQ::MD::Meta
{

struct EntityMetadata
{
	std::string_view name;
};

// Shorthand functions
ARQMarket_API const EntityMetadata& get( const std::string_view entityName );
ARQMarket_API const std::vector<EntityMetadata>&   getAll();
ARQMarket_API const std::vector<std::string_view>& getAllNames();

class EntityMetadataRegistry
{
public:
	[[nodiscard]] ARQMarket_API static const EntityMetadataRegistry& inst();

	ARQMarket_API const EntityMetadata& get( const std::string_view entityName ) const;

	[[nodiscard]] ARQMarket_API const std::vector<EntityMetadata>&   getAll()      const { return m_metadataList; };
	[[nodiscard]] ARQMarket_API const std::vector<std::string_view>& getAllNames() const { return m_entityNames; }

public:
	template<c_MktData T>
	struct Reg
	{
		Reg()
		{
			EntityMetadata meta{
				.name = Traits<T>::name()
			};

			EntityMetadataRegistry::instMut().m_metadataList.push_back( std::move( meta ) );
			EntityMetadataRegistry::instMut().m_entityNames.push_back( Traits<T>::name() );
		}
	};

private:
	[[nodiscard]] ARQMarket_API static EntityMetadataRegistry& instMut();

private:
	std::vector<EntityMetadata>   m_metadataList;
	std::vector<std::string_view> m_entityNames;
};

}

#define ARQ_REG_MD_ENTITY_META( TYPE ) \
	static ARQ::MD::Meta::EntityMetadataRegistry::Reg<TYPE> mdEntityMeta_##TYPE; \

