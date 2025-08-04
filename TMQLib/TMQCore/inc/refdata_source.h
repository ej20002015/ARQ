#pragma once
#include <TMQCore/dll.h>

#include <TMQCore/refdata_entities.h>
#include <TMQCore/refdata_source_interface.h>

#include <array>
#include <string>
#include <chrono>
#include <shared_mutex>
#include <functional>

namespace TMQ
{

using RefDataSourceCreateFunc = std::add_pointer<RefDataSource*( const std::string_view dsh )>::type;

class RefDataSourceFactory
{
public:
	[[nodiscard]] static std::shared_ptr<RefDataSource> create( const std::string_view dsh );
};

class GlobalRefDataSource
{
public:
	GlobalRefDataSource() = delete;

	using CreatorFunc = std::function<std::shared_ptr<RefDataSource>()>;

	[[nodiscard]] TMQCore_API static std::shared_ptr<RefDataSource> get();
	TMQCore_API static void setFunc( const CreatorFunc& creatorFunc );

private:
	static inline CreatorFunc s_globalSourceCreator;
	static inline std::shared_mutex s_mut;
};

}