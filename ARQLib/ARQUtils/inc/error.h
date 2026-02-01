#pragma once
#include <ARQUtils/dll.h>

#include <format>
#include <string>
#include <source_location>
#include <ostream>
#include <sstream>
#include <stacktrace>

namespace ARQ
{

// Assert

#ifdef _WIN32
#define ARQ_BREAK() __debugbreak()
#else
#define ARQ_BREAK()
#endif

#ifdef NDEBUG
#define ARQ_ASSERT( x )
#else
#define ARQ_ASSERT( x ) { if( !( x ) ) { ARQ_BREAK(); } }
#endif

// Based on Peter Muldoon's OmegaException class presented at CppCon 2023
// https://www.youtube.com/watch?v=Oy-VTqz1_58&t=1579s

class ARQException
{
public:
    ARQException() = default;
    ARQException( const std::string_view str, const std::source_location& loc = std::source_location::current(), std::stacktrace trace = std::stacktrace::current() )
        : m_errStr( str )
        , m_location( loc )
        , backtrace( trace )
    {}

    std::string& what() { return m_errStr; }
    const std::string& what() const noexcept { return m_errStr; }

    const std::source_location& where() const { return m_location; }

    ARQUtils_API std::string str() const;

    const std::stacktrace& stack() const { return backtrace; }
private:
    std::string m_errStr;
    std::source_location m_location;
    std::stacktrace backtrace;
};

inline std::ostream& operator << ( std::ostream& os, const std::source_location& location )
{
    os  << location.file_name() << "("
        << location.line() << ":"
        << location.column() << "), function `"
        << location.function_name() << "`";
    return os;
}

inline std::ostream& operator << ( std::ostream& os, const std::stacktrace& backtrace )
{
    for( auto iter = backtrace.begin(); iter != ( backtrace.end() - 3 ); ++iter )
    {
        os << iter->source_file() << "(" << iter->source_line()
            << ") : " << iter->description() << "\n";
    }
    return os;
}

inline std::string fmtStacktrace( const std::stacktrace& backtrace )
{
    std::ostringstream ss;
    ss << backtrace;
    return ss.str();
}

inline std::string fmtInternalErrStr( const std::string_view errStr )
{
    return std::format( "!!! {0} !!!", errStr );
}

// Helpers for catching exceptions

#define ARQ_DO_IN_TRY( arqException, errMsg ) \
ARQException arqException;                    \
std::string  errMsg;                          \
try {                                         \

#define ARQ_END_TRY_AND_CATCH( arqException, errorMsg) } \
catch( const ARQException& e ) {                         \
	arqException = e; }                                  \
catch( const std::exception& e ) {                       \
	errorMsg = e.what(); }                               \
catch( ... ) {                                           \
	errorMsg = "Unknown exception"; }                    \

} // namespace ARQ