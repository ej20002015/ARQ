#pragma once
#include <ARQCore/dll.h>

#include <ARQCore/refdata_entities.h>

#include <vector>

namespace ARQ::RD::Meta
{

struct EntityMetadata
{
	std::string_view name;
};

// Shorthand functions
ARQCore_API const EntityMetadata& get( const std::string_view entityName );
ARQCore_API const std::vector<EntityMetadata>& getAll();

class EntityMetadataRegistry
{
public:
	[[nodiscard]] ARQCore_API static const EntityMetadataRegistry& inst();

	ARQCore_API const EntityMetadata& get( const std::string_view entityName ) const;

	[[nodiscard]] ARQCore_API const std::vector<EntityMetadata>& getAll() const { return m_metadataList; };

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
	[[nodiscard]] ARQCore_API static EntityMetadataRegistry& instMut();

private:
	std::vector<EntityMetadata> m_metadataList;
};

}

#define ARQ_REG_RD_ENTITY_META( TYPE ) \
	static ARQ::RD::Meta::EntityMetadataRegistry::Reg<TYPE> rdEntityMeta_##TYPE;