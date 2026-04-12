#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQUtils/os.h>

#include <unordered_map>
#include <mutex>
#include <atomic>
#include <vector>

namespace ARQ
{

class DynaLibCache
{
public:
	ARQCore_API static DynaLibCache* globalInst();
	            static void          setGlobalInst( DynaLibCache* const inst );

	ARQCore_API static const OS::DynaLib& get( const std::string_view libName );
	ARQCore_API        const OS::DynaLib& iGet( const std::string_view libName );
	
	DynaLibCache() = default;
	~DynaLibCache();

	DynaLibCache( const DynaLibCache& ) = delete;
	DynaLibCache& operator=( const DynaLibCache& ) = delete;

private:
	ARQCore_API static std::atomic<DynaLibCache*> s_globalInst;

private:
	std::unordered_map<std::string, OS::DynaLib, TransparentStringHash, std::equal_to<>> m_loadedLibs;
	std::vector<std::string> m_loadedLibsOrder;
	std::mutex m_mut;
};

}
