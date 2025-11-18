#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQCore/refdata_entities.h>
#include <ARQCore/refdata_source_interface.h>

#include <string>
#include <chrono>
#include <shared_mutex>
#include <functional>

namespace ARQ
{

using RefDataSourceCreateFunc = std::add_pointer<IRefDataSource*( const std::string_view dsh )>::type;

class RefDataSourceFactory
{
public:
	[[nodiscard]] static std::shared_ptr<IRefDataSource> create( const std::string_view dsh );

	// Not threadsafe but should only really be used on startup / for testing anyway
	ARQCore_API static void addCustomSource( const std::string_view dsh, const std::shared_ptr<IRefDataSource>& source );
	ARQCore_API static void delCustomSource( const std::string_view dsh );

private:
	static std::unordered_map<std::string, std::shared_ptr<IRefDataSource>, TransparentStringHash, std::equal_to<>> s_customSources;
};

class GlobalRefDataSource
{
public:
	GlobalRefDataSource() = delete;

	using CreatorFunc = std::function<std::shared_ptr<IRefDataSource>()>;

	[[nodiscard]] ARQCore_API static std::shared_ptr<IRefDataSource> get();
	ARQCore_API static void setFunc( const CreatorFunc& creatorFunc );

private:
	static inline CreatorFunc s_globalSourceCreator;
	static inline std::shared_mutex s_mut;
};

}