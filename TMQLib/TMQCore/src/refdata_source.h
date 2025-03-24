#pragma once
#include <TMQCore/dll.h> // Used in the header only refdata.h file so need to dllexport functions

#include <TMQCore/refdata_entities.h>

#include <vector>
#include <string>
#include <chrono>

namespace TMQ
{

class RefDataSource
{
public:
	virtual ~RefDataSource() = default;

	TMQ_API virtual std::vector<std::string> fetchLatest( const std::string_view table ) = 0;
	TMQ_API virtual std::vector<std::string> fetchAsOf( const std::string_view table, const std::chrono::system_clock::time_point ts ) = 0;
};

// TODO: Have some way of setting this
TMQ_API std::shared_ptr<RefDataSource> getGlobalRefDataSource();

template <c_RDEntity T>
class TypedRDSource
{
public:
	static std::vector<T> fetchLatest( RefDataSource& source )
	{
		std::vector<std::string> rawData = source.fetchLatest( RDEntityTraits<T>::table() );
		return deserialize( rawData );
	}

	static std::vector<T> fetchAsOf( RefDataSource& source, const std::chrono::system_clock::time_point ts )
	{
		std::vector<std::string> rawData = source.fetchAsOf( RDEntityTraits<T>::table(), ts );
		return deserialize( rawData );
	}

private:
	static std::vector<T> deserialize( const std::vector<std::string>& rawData )
	{
		std::vector<T> result;
		for( const auto& blob : rawData )
			result.push_back( RDEntityTraits<T>::deserialise( blob ) );

		return result;
	}
};

class TSDBRefDataSource : public RefDataSource
{
public:
	TMQ_API std::vector<std::string> fetchLatest( const std::string_view table ) override;
	TMQ_API std::vector<std::string> fetchAsOf( const std::string_view table, const std::chrono::system_clock::time_point ts ) override;
};

}