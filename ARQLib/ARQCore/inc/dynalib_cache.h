#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQUtils/os.h>

#include <unordered_map>
#include <mutex>

namespace ARQ
{

class DynaLibCache
{
public:
	static DynaLibCache& inst() noexcept
	{
		static DynaLibCache inst;
		return inst;
	}

	ARQCore_API const OS::DynaLib& get( const std::string_view libName );

	DynaLibCache( const DynaLibCache& ) = delete;
	DynaLibCache& operator=( const DynaLibCache& ) = delete;

private:
	DynaLibCache() = default;

private:
	std::unordered_map<std::string, OS::DynaLib, TransparentStringHash, std::equal_to<>> m_loadedLibs;
	std::mutex m_mut;
};

}