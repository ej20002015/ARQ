#pragma once
#include <TMQCore/dll.h>

#include <TMQUtils/hashers.h>
#include <TMQUtils/os.h>

#include <unordered_map>
#include <mutex>

namespace TMQ
{

class DynaLibCache
{
public:
	static DynaLibCache& inst() noexcept
	{
		static DynaLibCache inst;
		return inst;
	}

	TMQCore_API const OS::DynaLib& get( const std::string_view libName );

	DynaLibCache( const DynaLibCache& ) = delete;
	DynaLibCache& operator=( const DynaLibCache& ) = delete;

private:
	DynaLibCache() = default;

private:
	std::unordered_map<std::string, OS::DynaLib, TransparentStringHash, std::equal_to<>> m_loadedLibs;
	std::mutex m_mut;
};

}