#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQUtils/os.h>
#include <ARQUtils/global_accessor.h>

#include <unordered_map>
#include <mutex>
#include <atomic>
#include <vector>

namespace ARQ
{

class DynaLibCache : public GlobalAccessor<DynaLibCache, "DynaLibCache">
{
public:
	DynaLibCache() = default;
	~DynaLibCache();

	ARQCore_API const OS::DynaLib& get( const std::string_view libName );

	DynaLibCache( const DynaLibCache& ) = delete;
	DynaLibCache& operator=( const DynaLibCache& ) = delete;

private:
	std::unordered_map<std::string, OS::DynaLib, TransparentStringHash, std::equal_to<>> m_loadedLibs;
	std::vector<std::string> m_loadedLibsOrder;
	std::mutex m_mut;
};

}
