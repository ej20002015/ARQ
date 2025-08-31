#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQCore/refdata_entities.h>
#include <ARQCore/refdata_source_interface.h>

#include <array>
#include <string>
#include <chrono>
#include <shared_mutex>
#include <functional>

namespace ARQ
{

using RefDataSourceCreateFunc = std::add_pointer<RefDataSource*( const std::string_view dsh )>::type;

class RefDataSourceFactory
{
public:
	[[nodiscard]] static std::shared_ptr<RefDataSource> create( const std::string_view dsh );

	ARQCore_API static void addCustomSource( const std::string_view dsh, const std::shared_ptr<RefDataSource>& source );
	ARQCore_API static void delCustomSource( const std::string_view dsh );

private:
	static inline std::unordered_map<std::string, std::shared_ptr<RefDataSource>, TransparentStringHash, std::equal_to<>> s_customSources;
};

class GlobalRefDataSource
{
public:
	GlobalRefDataSource() = delete;

	using CreatorFunc = std::function<std::shared_ptr<RefDataSource>()>;

	[[nodiscard]] ARQCore_API static std::shared_ptr<RefDataSource> get();
	ARQCore_API static void setFunc( const CreatorFunc& creatorFunc );

private:
	static inline CreatorFunc s_globalSourceCreator;
	static inline std::shared_mutex s_mut;
};

}