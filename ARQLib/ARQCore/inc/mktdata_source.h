#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQCore/mktdata_source_interface.h>

#include <shared_mutex>
#include <functional>
#include <unordered_map>

namespace ARQ
{

using MktDataSourceCreateFunc = std::add_pointer<IMktDataSource*( const std::string_view dsh )>::type;

class MktDataSourceFactory
{
public:
	[[nodiscard]] static std::shared_ptr<IMktDataSource> create( const std::string_view dsh );

	// Not threadsafe but should only really be used on startup / for testing anyway
	ARQCore_API static void addCustomSource( const std::string_view dsh, const std::shared_ptr<IMktDataSource>& source );
	ARQCore_API static void delCustomSource( const std::string_view dsh );

private:
	static std::unordered_map<std::string, std::shared_ptr<IMktDataSource>, TransparentStringHash, std::equal_to<>> s_customSources;
};

class GlobalMktDataSource
{
public:
	GlobalMktDataSource() = delete;

	using CreatorFunc = std::function<std::shared_ptr<IMktDataSource>()>;

	[[nodiscard]] ARQCore_API static std::shared_ptr<IMktDataSource> get();
	ARQCore_API static void setFunc( const CreatorFunc& creatorFunc );

private:
	static inline CreatorFunc s_globalSourceCreator;
	static inline std::shared_mutex s_mut;
};

}