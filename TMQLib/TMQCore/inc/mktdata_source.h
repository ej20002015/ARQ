#pragma once
#include <TMQCore/dll.h>

#include <TMQCore/mktdata_source_interface.h>

#include <array>
#include <shared_mutex>
#include <functional>

namespace TMQ
{

using MktDataSourceCreateFunc = std::add_pointer<MktDataSource*( const std::string_view dsh )>::type;

class MktDataSourceFactory
{
public:
	[[nodiscard]] static std::shared_ptr<MktDataSource> create( const std::string_view dsh );
};

class GlobalMktDataSource
{
public:
	GlobalMktDataSource() = delete;

	using CreatorFunc = std::function<std::shared_ptr<MktDataSource>()>;

	[[nodiscard]] TMQCore_API static std::shared_ptr<MktDataSource> get();
	TMQCore_API static void setFunc( const CreatorFunc& creatorFunc );

private:
	static inline CreatorFunc s_globalSourceCreator;
	static inline std::shared_mutex s_mut;
};

}