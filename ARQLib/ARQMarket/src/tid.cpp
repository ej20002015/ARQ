#include <ARQMarket/tid.h>

#include <algorithm>

namespace ARQ::MD
{

bool TID::isMatch( const TID& other ) const
{
    if( type != other.type )
        return false;

    // If this item has no ID (type-level), it covers any item of the same type
    if( !id.has_value() )
        return true;

    // If the other item has no ID (type-level query), this specific item doesn't cover it
    if( !other.id.has_value() )
        return false;

    // Both have IDs, check for exact match
    return id.value() == other.id.value();
}

TIDSet::TIDSet( const std::initializer_list<TID>& items )
    : m_list( items )
{
    m_dirty = true;
}

void TIDSet::insert( const TID& item )
{
    m_list.push_back( item );
	m_dirty = true;
}

void TIDSet::erase( const TID& item )
{
    consolidate();

    if( !item.id.has_value() )
        std::erase_if( m_list, [&item] ( const TID& i ) { return i.type == item.type; } );
    else
        std::erase( m_list, item );
}

void TIDSet::merge( const TIDSet& otherSet )
{
	std::vector<TID> otherItems = otherSet.getAll();
	m_list.insert( m_list.end(), otherItems.begin(), otherItems.end() );
    m_dirty = true;
}

bool TIDSet::contains( const TID& item ) const
{
    consolidate();

    for( const TID& listItem : m_list )
    {
        if( listItem.isMatch( item ) )
            return true;
	}

    return false;
}

const std::vector<TID>& TIDSet::getAll() const
{
    consolidate();
	return m_list;
}

TIDSet::IDs TIDSet::getIDsForType( const Type type ) const
{
    consolidate();

    std::vector<std::string_view> idList;

    for( const auto& item : m_list )
    {
        // Early exit since we sort by type
        if( item.type > type )
            break;

        if( item.type == type )
        {
            // If we hit a wildcard, we can immediately stop and return All
            if( !item.id.has_value() )
                return All{};

            idList.push_back( item.id.value() );
        }
    }

    // If we found nothing, return None. Otherwise, return the Specific list
    if( idList.empty() )
        return None{};
    else
        return idList;
}

void TIDSet::consolidate() const
{
	if( !m_dirty || empty() )
        return;

	// Sort the list - type level items will be at front of their respective types, and duplicates will be adjacent
	std::sort( m_list.begin(), m_list.end() );

    auto it         = m_list.begin();
	auto resultTail = m_list.begin();
	while( ++it != m_list.end() )
    {
        if( resultTail->type == it->type && !resultTail->id.has_value() )
			continue;                             // Skip this item as it's covered by the type-level item at resultTail
        else if( *resultTail == *it )
			continue;                             // Skip this item as it's a duplicate of the item at resultTail
        else
		    *( ++resultTail ) = std::move( *it ); // This item is not covered by a previous type-level item and is not a duplicate, so we keep it
    }

    // Erase whatever is left at the end of the vector
    m_list.erase( ++resultTail, m_list.end() );
    m_dirty = false;
}

}