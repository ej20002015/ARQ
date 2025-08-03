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

using RefDataSourceCreateFunc = std::add_pointer<RefDataSource*()>::type;

class RefDataSourceRepo
{
public:
	RefDataSourceRepo() = delete;

	enum Type
	{
		ClickHouse,

		_SIZE
	};

public:
	[[nodiscard]] TMQCore_API static std::shared_ptr<RefDataSource> get( const Type type );

private:
	static inline std::array<std::shared_ptr<RefDataSource>, Type::_SIZE> s_sources;
	static inline std::shared_mutex s_mut;
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