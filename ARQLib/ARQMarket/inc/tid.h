#pragma once
#include <ARQMarket/dll.h>

#include <ARQMarket/mktdata_entities.h>

#include <optional>
#include <vector>
#include <variant>

namespace ARQ::MD
{

struct TID
{
    TID( const Type type, std::optional<std::string> id = std::nullopt )
        : type( type )
        , id( id )
    {}

    TID()
        : type( Type::_NOTSET_ )
        , id( std::nullopt )
    {}

    // Will auto generate ==, !=, <, >, <=, >= 
    // Compares 'type' first, and if they are equal, compares 'id'
    auto operator<=>( const TID& ) const = default;

    ARQMarket_API bool isMatch( const TID& other ) const;

    Type                       type;
    std::optional<std::string> id;
};

class TIDSet
{
public:
    struct None {};
    struct All  {};
	using  IDList = std::vector<std::string_view>;

	using IDs = std::variant<None, All, IDList>;

    TIDSet() = default;
    ARQMarket_API TIDSet( const std::initializer_list<TID>& items );

    ARQMarket_API void insert( const TID& item );
    ARQMarket_API void erase( const TID& item );
    ARQMarket_API void merge( const TIDSet& otherSet );
	              void clear() { m_list.clear(); }

    ARQMarket_API bool                    contains( const TID& item )      const;
	              bool                    empty()                          const { return m_list.empty(); }
    ARQMarket_API const std::vector<TID>& getAll()                         const;
	ARQMarket_API IDs                     getIDsForType( const Type type ) const;

private:
    void consolidate() const;

private:
    mutable std::vector<TID> m_list;
    mutable bool             m_dirty = false;
};

}