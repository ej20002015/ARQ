#pragma once
#include <TMQCore/dll.h>

#include <TMQCore/mktdata_source_interface.h>

#include <array>
#include <shared_mutex>
#include <functional>

namespace TMQ
{

using MktDataSourceCreateFunc = std::add_pointer<MktDataSource* ( )>::type;

class MktDataSourceRepo
{
public:
	MktDataSourceRepo() = delete;

	enum Type
	{
		ClickHouse,

		_SIZE
	};

public:
	[[nodiscard]] TMQCore_API static std::shared_ptr<MktDataSource> get( const Type type );

private:
	static inline std::array<std::shared_ptr<MktDataSource>, Type::_SIZE> s_sources;
	static inline std::shared_mutex s_mut;
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