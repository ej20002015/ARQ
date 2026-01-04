#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQUtils/error.h>
#include <ARQCore/refdata_entities.h>

#include <string>
#include <vector>
#include <mutex>
#include <functional>

namespace ARQ::RD
{

class Source;

class SourceConcept
{
public:
	virtual ~SourceConcept() = default;

protected:
	const std::string_view dsh() const { return m_dsh; }

protected:
	std::string m_dsh;

private:
	void setDSH( const std::string_view dsh ) { m_dsh = dsh; }

	friend class Source;
};

template<c_RefData T>
class IEntitySource : public SourceConcept
{
public:
	virtual std::vector<Record<T>> fetch()                                      const = 0;
	virtual void                   insert( const std::vector<Record<T>>& data )       = 0;
};

class Source
{
public:
	template<typename T>
	std::vector<Record<T>> fetch() const;

	template<typename T>
	void insert( const std::vector<Record<T>> data );

	template<typename T>
	void registerEntitySource( std::unique_ptr<IEntitySource<T>> entitySource );

	void setDSH( const std::string_view dsh );

private:
	template<typename T>
	IEntitySource<T>& getEntitySource() const;

private:
	std::unordered_map<std::string_view, std::unique_ptr<SourceConcept>> m_entitySources;
	std::string m_dsh;
};

using RegisterEntitySourcesFunc = std::add_pointer<void( Source* const source )>::type;

class SourceFactory
{
public:
	[[nodiscard]] ARQCore_API static std::shared_ptr<Source> create( const std::string_view dsh );

	ARQCore_API static void addCustomSource( const std::string_view dsh, const std::shared_ptr<Source>& source );
	ARQCore_API static void delCustomSource( const std::string_view dsh );

private:
	static std::unordered_map<std::string, std::shared_ptr<Source>, TransparentStringHash, std::equal_to<>> s_sources;
	static std::mutex s_sourcesMutex;
};

template<typename T>
std::vector<Record<T>> Source::fetch() const
{
	const IEntitySource<T>& entitySource = getEntitySource<T>();
	return entitySource.fetch();
}

template<typename T>
void Source::insert( const std::vector<Record<T>> data )
{
	IEntitySource<T>& entitySource = getEntitySource<T>();
	entitySource.insert( data );
}

template<typename T>
void Source::registerEntitySource( std::unique_ptr<IEntitySource<T>> entitySource )
{
	constexpr std::string_view typeName = ARQType<T>::name();
	m_entitySources[typeName] = std::move( entitySource );
}

template<typename T>
IEntitySource<T>& Source::getEntitySource() const
{
	constexpr std::string_view typeName = ARQType<T>::name();
	const auto it = m_entitySources.find( typeName );
	if( it != m_entitySources.end() )
		return static_cast<IEntitySource<T>&>( *it->second );
	else
		throw ARQException( std::format( "RD::Source: No entity source registered for type [{}]", typeName ) );
}

}