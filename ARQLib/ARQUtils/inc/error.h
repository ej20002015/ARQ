#pragma once

#include <format>
#include <string>
#include <source_location>
#include <ostream>
// TODO: Get stackstraces on exception
//#include <stacktrace>

namespace ARQ
{

// Based on Peter Muldoon's OmegaException class presented at CppCon 2023
// TODO: add support for data
// TODO: add stacktraces
// https://www.youtube.com/watch?v=Oy-VTqz1_58&t=1579s

class ARQException
{
public:
    ARQException() = default;
    ARQException( const std::string_view str, const std::source_location& loc = std::source_location::current()/*, std::stacktrace trace = std::stacktrace::current()*/ )
        : m_errStr( str )
        , m_location( loc )
        //, backtrace( trace )
    {}

    std::string& what() { return m_errStr; }
    const std::string& what() const noexcept { return m_errStr; }

    const std::source_location& where() const { return m_location; }

    std::string str() const;

    /*const std::stacktrace& stack() const { return backtrace; }*/
private:
    std::string m_errStr;
    std::source_location m_location;
    /*std::stacktrace backtrace;*/
};

inline std::ostream& operator << ( std::ostream& os, const std::source_location& location )
{
    os  << location.file_name() << "("
        << location.line() << ":"
        << location.column() << "), function `"
        << location.function_name() << "`";
    return os;
}

//std::ostream& operator << ( std::ostream& os, const std::stacktrace& backtrace )
//{
//    for( auto iter = backtrace.begin(); iter != ( backtrace.end() - 3 ); ++iter )
//    {
//        os << iter->source_file() << "(" << iter->source_line()
//            << ") : " << iter->description() << "\n";
//    }
//    return os;
//}

inline std::string fmtInternalErrStr( const std::string_view errStr )
{
    return std::format( "!!! {0} !!!", errStr );
}

} // namespace ARQ