#pragma once

#include <ARQMarket/mktdata_entities.h>

#include <map>
#include <string>

class CurrentMarketSelector
{
private:
	struct SelectionKey
	{
		ARQ::MD::Type type;
		std::string id;

		auto operator<=>( const SelectionKey& ) const = default;
	};

public:
	bool push( const ARQ::MD::Type type, const std::string_view id, const ARQ::Time::DateTime asofTs )
	{
		const SelectionKey key{ type, std::string( id ) };
		auto [it, inserted] = m_selectedAsofs.try_emplace( key, asofTs );
		if( inserted )
			return true;
		else if( asofTs >= it->second )
		{
			it->second = asofTs;
			return true;
		}
		else
			return false;
	}

private:
	std::map<SelectionKey, ARQ::Time::DateTime> m_selectedAsofs;
};
