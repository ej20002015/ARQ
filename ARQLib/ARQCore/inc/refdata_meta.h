#pragma once

#include <ARQCore/refdata_entities.h>

#include <vector>

namespace ARQ::RD::Meta
{

struct EntityMetadata
{
	std::string_view name;
};

// Shorthand functions
const EntityMetadata& get( const std::string_view entityName );
const std::vector<EntityMetadata>& getAll();

class EntityMetadataRegistry
{
public:
	[[nodiscard]] static const EntityMetadataRegistry& inst();

	const EntityMetadata& get( const std::string_view entityName ) const;

	[[nodiscard]] const std::vector<EntityMetadata>& getAll() const { return m_metadataList; };

public:
	template<c_RefData T>
	struct Reg
	{
		Reg()
		{
			EntityMetadata meta{
				.name = Traits<T>::name()
			};

			EntityMetadataRegistry::instMut().m_metadataList.push_back( std::move( meta ) );
		}
	};

private:
	[[nodiscard]] static EntityMetadataRegistry& instMut();

private:
	std::vector<EntityMetadata> m_metadataList;
};

}

#define ARQ_REG_RD_ENTITY_META( TYPE ) \
	static ARQ::RD::Meta::EntityMetadataRegistry::Reg<TYPE> rdEntityMeta_##TYPE;