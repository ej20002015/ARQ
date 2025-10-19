#pragma once

#include <string>
#include <mutex>
#include <unordered_map>

#include <ARQUtils/hashers.h>

namespace ARQ
{
namespace Grpc
{
namespace RefData
{

class VersionManager
{
public:
	virtual void init() = 0;

	virtual std::mutex& getEntityMutex( const std::string_view entityName ) = 0;

	virtual uint32_t getVerUnsafe( const std::string_view entityName, const std::string_view id ) = 0;
	virtual void setVerUnsafe( const std::string_view entityName, const std::string_view id, const uint32_t newVer ) = 0;
};

class VersionManagerImpl : public VersionManager
{
public:
	void init();

	std::mutex& getEntityMutex( const std::string_view entityName ) override;

	uint32_t getVerUnsafe( const std::string_view entityName, const std::string_view id ) override;
	void setVerUnsafe( const std::string_view entityName, const std::string_view id, const uint32_t newVer ) override;

private:
	using EntityID2VersionMap = std::unordered_map<std::string, uint32_t, TransparentStringHash, std::equal_to<>>;

	EntityID2VersionMap& getVerMap( const std::string_view entityName );

private:
	std::unordered_map<std::string, std::mutex, TransparentStringHash, std::equal_to<>> m_entityMutexes;
	std::unordered_map<std::string, EntityID2VersionMap, TransparentStringHash, std::equal_to<>> m_entityVers;
};

}
}
}