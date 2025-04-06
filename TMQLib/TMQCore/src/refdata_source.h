#pragma once
#include <TMQCore/dll.h> // Used in the header only refdata.h file so need to dllexport functions

#include <TMQCore/refdata_entities.h>
#include <TMQSerialisation/serialisation.h>

#include <vector>
#include <string>
#include <chrono>

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
		std::string lastUpdatedBy;
		bool active;
		std::string blob;
	};

public:
	virtual ~RefDataSource() = default;

	TMQCore_API virtual [[nodiscard]] std::vector<FetchData> fetchLatest( const std::string_view table ) = 0;
	TMQCore_API virtual [[nodiscard]] std::vector<FetchData> fetchAsOf( const std::string_view table, const std::chrono::system_clock::time_point ts ) = 0;
	TMQCore_API virtual void insert( const std::string_view table, const std::vector<InsertData>& insData ) = 0;
};

// TODO: Have some way of setting this
TMQCore_API std::shared_ptr<RefDataSource> getGlobalRefDataSource();

template <c_RDEntity T>
class TypedRDSource
{
public:
	static [[nodiscard]] std::vector<T> fetchLatest( RefDataSource& source )
	{
		const auto fetchRes = source.fetchLatest( RDEntityTraits<T>::table() );
		return TypedRDSource<T>::parseFetchRes( fetchRes );
	}

	static [[nodiscard]] std::vector<T> fetchAsOf( RefDataSource& source, const std::chrono::system_clock::time_point ts )
	{
		const auto fetchRes = source.fetchAsOf( RDEntityTraits<T>::table(), ts );
		return TypedRDSource<T>::parseFetchRes( fetchRes );
	}

	static void insert( RefDataSource& source, const std::vector<T>& data )
	{
		static constexpr auto table = RDEntityTraits<T>::table();
		
		std::vector<RefDataSource::InsertData> insData;

		for( const T& d : data )
		{
			const Buffer buf = TMQ::serialise( d );
			std::string bufStr( reinterpret_cast<const char*>( buf.data.get() ), buf.size ); // TODO: don't copy

			insData.emplace_back( RefDataSource::InsertData {
				RDEntityTraits<T>::getID( d ),
				"Evan", // TODO: Change to username
				d._active,
				bufStr
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

class TSDBRefDataSource : public RefDataSource
{
public:
	TMQCore_API [[nodiscard]] std::vector<FetchData> fetchLatest( const std::string_view table ) override;
	TMQCore_API [[nodiscard]] std::vector<FetchData> fetchAsOf( const std::string_view table, const std::chrono::system_clock::time_point ts ) override;

	TMQCore_API void insert( const std::string_view table, const std::vector<InsertData>& insData ) override;
};

}