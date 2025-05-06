#pragma once
#include <TMQCore/dll.h>

#include <TMQUtils/buffer.h>

#include <array>
#include <string>
#include <chrono>
#include <shared_mutex>
#include <functional>

namespace TMQ
{

class MktDataSource
{
public:
	struct FetchData
	{
		std::string type;
		std::string instrumentID;
		std::chrono::system_clock::time_point asofTs;
		Buffer blob;
		std::string source;
		std::chrono::system_clock::time_point lastUpdatedTs;
		std::string lastUpdatedBy;
	};

	struct InsertData
	{
		std::string type;
		std::string instrumentID;
		std::chrono::system_clock::time_point asofTs;
		Buffer blob;
		std::string source;
		std::string lastUpdatedBy;
		bool active;
	};

public:
	virtual ~MktDataSource() = default;

	[[nodiscard]] TMQCore_API virtual std::vector<FetchData> fetchLatest( const std::string_view context ) = 0;
	TMQCore_API virtual void insert( const std::string_view context, const std::vector<InsertData>& insData ) = 0;
};

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