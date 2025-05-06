#pragma once
#include <TMQCore/dll.h>

#include <TMQCore/refdata_entities.h>
#include <TMQSerialisation/serialisation.h>

#include <vector>
#include <array>
#include <string>
#include <chrono>
#include <shared_mutex>
#include <functional>

namespace TMQ
{

class RefDataSource
{
public:
	struct FetchData
	{
		std::chrono::system_clock::time_point lastUpdatedTs;
		std::string lastUpdatedBy;
		Buffer blob;
	};

	struct InsertData
	{
		std::string ID;
		std::string_view lastUpdatedBy;
		bool active;
		Buffer blob;
	};

public:
	virtual ~RefDataSource() = default;

	[[nodiscard]] TMQCore_API virtual std::vector<FetchData> fetchLatest( const std::string_view table ) = 0;
	[[nodiscard]] TMQCore_API virtual std::vector<FetchData> fetchAsOf( const std::string_view table, const std::chrono::system_clock::time_point ts ) = 0;

	TMQCore_API virtual void insert( const std::string_view table, const std::vector<InsertData>& insData ) = 0;
};

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

template <c_RDEntity T>
class TypedRDSource
{
public:
	[[nodiscard]] static std::vector<T> fetchLatest( RefDataSource& source )
	{
		const auto fetchRes = source.fetchLatest( RDEntityTraits<T>::table() );
		return TypedRDSource<T>::parseFetchRes( fetchRes );
	}

	[[nodiscard]] static std::vector<T> fetchAsOf( RefDataSource& source, const std::chrono::system_clock::time_point ts )
	{
		const auto fetchRes = source.fetchAsOf( RDEntityTraits<T>::table(), ts );
		return TypedRDSource<T>::parseFetchRes( fetchRes );
	}

	static void insert( RefDataSource& source, const std::vector<T>& data )
	{
		static constexpr auto table = RDEntityTraits<T>::table();
		
		std::vector<RefDataSource::InsertData> insData;
		insData.reserve( data.size() );

		for( const T& d : data )
		{
			insData.emplace_back( RefDataSource::InsertData{
				.ID            = RDEntityTraits<T>::getID( d ),
				.lastUpdatedBy = "Evan", // TODO: Change to username
				.active        = d._active,
				.blob          = TMQ::serialise( d )
			} );
		}

		source.insert( table, insData );
	}

private:
	static std::vector<T> [[nodiscard]] parseFetchRes( const std::vector<RefDataSource::FetchData>& fetchRes )
	{
		std::vector<T> result;
		result.reserve( fetchRes.size() );
		for( const auto& resRow : fetchRes )
		{
			result.emplace_back( TMQ::deserialise<T>( resRow.blob ) );
			result.back()._lastUpdatedTs = resRow.lastUpdatedTs;
			result.back()._lastUpdatedBy = resRow.lastUpdatedBy;
			result.back()._active = true; // If returned from fetch then must be an active record
		}

		return result;
	}
};

}