#include <ARQUtils/os.h>

#include <ARQUtils/error.h>
#include <ARQUtils/str.h>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#include <processthreadsapi.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <pthread.h>
	#include <dlfcn.h>
#endif

#include <atomic>

namespace ARQ
{

namespace OS
{

static std::filesystem::path iProcPath()
{
	std::filesystem::path procPath;

	#ifdef _WIN32

	char buffer[4096];
	const DWORD pathLen = GetModuleFileNameA( nullptr, buffer, sizeof( buffer ) );
	if( pathLen == 0 || GetLastError() == ERROR_INSUFFICIENT_BUFFER )
		throw ARQException( "Could not get process name: GetModuleFileNameA failed" );
	else
		procPath = buffer;

	#else

	char buffer[4096];
	const ssize_t len = readlink( "/proc/self/exe", buffer, sizeof( buffer ) - 1 ); // Readlink doesn't null-terminate
	if( len == -1 )
		throw ARQException( "Could not get process name: failed to get /proc/self/exe symlink" );
	else
	{
		buffer[len] = '\0'; // Null-terminate the buffer
		procPath = buffer;
	}

	#endif

	return procPath;
}

const std::filesystem::path& procPath()
{
	static const std::filesystem::path procPath = iProcPath();
	return procPath;
}

std::string_view procName()
{
	static const std::string procName = procPath().filename().string();
	return procName;
}

static int32_t iProcID()
{
	#ifdef _WIN32
	return static_cast<int32_t>( GetCurrentProcessId() );
	#else
	return static_cast<int32_t>( getpid() );
	#endif
}

int32_t procID()
{
	static const int32_t procID = iProcID();
	return procID;
}

static thread_local std::atomic<bool> t_updatedThreadName = false;

static std::string iThreadName()
{
	std::string threadName = "unnamed_thread";

	#ifdef _WIN32

	PWSTR threadDescr = nullptr;
	HRESULT hr = GetThreadDescription( GetCurrentThread(), &threadDescr );
	if( SUCCEEDED( hr ) && threadDescr != nullptr )
	{
		std::wstring wstr = threadDescr;
		if( !wstr.empty() )
			threadName = Str::wstr2Str( wstr );
		LocalFree( threadDescr );
	}

	#else

	char buffer[256] = { 0 };
	if( pthread_getname_np( pthread_self(), buffer, sizeof( buffer ) ) == 0 && buffer[0] != '\0' )
		threadName = buffer;

	#endif

	return threadName;
}

std::string_view threadName()
{
	static thread_local std::string threadName = iThreadName();
	bool expectChanged = true;
	if( t_updatedThreadName.compare_exchange_strong( expectChanged, false ) )
		threadName = iThreadName();

	return threadName;
}

void setThreadName( const std::string_view name )
{
	static constexpr auto MAX_NAME_LEN = 15; // Linux limits thread names to 15 chars long (excl null term)
	if( name.size() > MAX_NAME_LEN )
		throw ARQException( std::format( "Length of given thread name [{0}] exceeds maximum of ", name.size(), MAX_NAME_LEN ) );

	#ifdef _WIN32

	std::wstring wideName = Str::str2Wstr( name.data() );
	HRESULT hr = SetThreadDescription( GetCurrentThread(), wideName.c_str() );
	if( FAILED( hr ) )
		throw ARQException( std::format( "Could not set thread name: SetThreadDescription failed: {0}", hr ) );

	#else

	int result = pthread_setname_np( pthread_self(), name.data() );
	if( result != 0  )
		throw ARQException( std::format( "Could not set thread name: pthread_setname_np failed: {0}", result ) );

	#endif

	t_updatedThreadName.store( true );
}

static int32_t iThreadID()
{
	#ifdef _WIN32
	return static_cast<int32_t>( GetCurrentThreadId() );
	#else
	return static_cast<int32_t>( gettid() );
	#endif
}

int32_t threadID()
{
	static const thread_local int32_t threadID = iThreadID();
	return threadID;
}

OS::DynaLib::DynaLib( const std::string_view name )
{
	load( name );
}

DynaLib::~DynaLib()
{
	unload();
}

void DynaLib::load( const std::string_view name )
{
	if( isLoaded() )
		return;

	setName( name );
	resetError();

	#ifdef _WIN32

	HINSTANCE hInst = LoadLibrary( m_name.c_str() );
	if( !hInst )
		throw ARQException( std::format( "Could not load dynalib {0}: {1}", m_name, getLastError() ) );

	m_nativeHandle = reinterpret_cast<void*>( hInst );

	#else

	void* dl = dlopen( m_name.c_str(), RTLD_LAZY );
	if( !dl )
		throw ARQException( std::format( "Could not load dynalib {0}: {1}", m_name, getLastError() ) );

	m_nativeHandle = dl;

	#endif
}

void DynaLib::unload()
{
	if( !isLoaded() )
		return;

	resetError();

	#ifdef _WIN32

	BOOL freeSuccess = FreeLibrary( reinterpret_cast<HMODULE>( m_nativeHandle ) );
	if( !freeSuccess )
		throw ARQException( std::format( "Could not unload dynalib {0}: {1}", m_name, getLastError() ) );

	#else

	int32_t errCode = dlclose( m_nativeHandle );
	if( errCode )
		throw ARQException( std::format( "Could not unload dynalib {0}: {1}", m_name, getLastError() ) );

	#endif

	m_nativeHandle = nullptr;
	m_name.clear();
}

void DynaLib::setName( const std::string_view name )
{
	#ifdef _WIN32
	m_name = std::format( "{0}.dll", name );
	#else
	m_name = std::format( "{0}.so", name );
	#endif
}

void DynaLib::resetError() const
{
	#ifdef _WIN32
	SetLastError( 0 );
	#else
	dlerror();
	#endif
}

std::string DynaLib::getLastError() const
{
	std::string errMsg;

	#ifdef _WIN32

	DWORD errorCode = GetLastError();
	if( errorCode != 0 )
	{
		LPSTR messageBuffer = nullptr;
		// Ask Win32 to allocate the buffer and format the message.
		size_t size = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errorCode, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&messageBuffer, 0, NULL );

		errMsg = std::string( messageBuffer, size );

		LocalFree( messageBuffer );

		// Remove trailing newline characters often included by FormatMessage
		while( !errMsg.empty() && ( errMsg.back() == '\n' || errMsg.back() == '\r' ) )
			errMsg.pop_back();
	}

	#else

	const char* errCStr = dlerror();
	if( errCStr )
		errMsg = errCStr;

	#endif

	return errMsg;
}

void* DynaLib::iGetFunc( const std::string_view funcName ) const
{
	if( !isLoaded() )
		throw ARQException( std::format( "Could not get {0} function from dynalib as it hasn't been loaded", funcName ) );

	resetError();
	void* funcPtr = nullptr;

	#ifdef _WIN32

	FARPROC procAddr = GetProcAddress( reinterpret_cast<HMODULE>( m_nativeHandle ), funcName.data() );
	funcPtr = static_cast<void*>( procAddr );

	#else

	funcPtr = dlsym( m_nativeHandle, funcName.data() );

	#endif

	if( !funcPtr )
		throw ARQException( std::format( "Could not get {0} function from dynalib {1}: {2}", funcName, m_name, getLastError() ) );

	return funcPtr;
}

}

}
