#pragma once
#include <ARQUtils/dll.h>

#include <ARQUtils/str.h>
#include <ARQUtils/error.h>

namespace ARQ
{

extern "C" ARQUtils_API std::atomic<void*>* getGlobalAtomic( const char* name );

template <typename T, Str::FixedString Name>
class GlobalAccessor
{
private:
    static std::atomic<void*>& getHostAtomic()
    {
        // Grab a pointer to the one global atomic - ensures we reference the same atomic object across all dynamic libs
        // Cache for future calls
        static std::atomic<void*>* hostAtomic = getGlobalAtomic( Name.value );
        return *hostAtomic;
    }

public:
    static T* getGlobalInstPtr()
    {
        T* ptr = static_cast<T*>( getHostAtomic().load( std::memory_order_acquire ) );
        return ptr;
    }

    static T& getGlobalInst()
    {
		T* ptr = getGlobalInstPtr();
        if( !ptr )
            throw ARQException( std::format( "Attempted to access global component [{}] before initialization or after shutdown", Name.view() ) );
        return *ptr;
    }

    /**
    * @brief Just an alias for getGlobalInst()
     */
    static T& inst()
    {
        return getGlobalInst();
	}

    static void setGlobalInst( T* const inst )
    {
        getHostAtomic().store( inst, std::memory_order_release );
    }

    static bool isInstSet()
    {
        return getHostAtomic().load( std::memory_order_acquire ) != nullptr;
    }

protected:
    // Prevent direct creation of this class
    GlobalAccessor() = default;
    virtual ~GlobalAccessor() = default;
};

}