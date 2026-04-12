#include <ARQUtils/global_accessor.h>

#include <mutex>
#include <unordered_map>

namespace ARQ
{

extern "C" ARQUtils_API std::atomic<void*>* getGlobalAtomic( const char* name )
{
    // These static variables live permanently inside the ARQUtils.dll memory space
    static std::mutex s_mut;
    static std::unordered_map<std::string, std::atomic<void*>> s_atomics;

    std::lock_guard<std::mutex> lg( s_mut );

    // Returns the memory address of the atomic associated with this name
    // If it doesn't exist yet, will be inited to nullptr and returned
    return &s_atomics[name];
}

}