#include <TMQUtils/error.h>

#include <sstream>

namespace TMQ
{

std::string TMQException::str() const
{
    std::ostringstream ss;
    ss << "What: \"" << what() << "\"; Where: \"" << where() << "\"";
    return ss.str();
}

//std::ostream& operator<<( std::ostream& os, const std::source_location& location )
//{
//    os 
//        << location.file_name() << "("
//        << location.line() << ":"
//        << location.column() << "), function `"
//        << location.function_name() << "`";
//    return os;
//}

std::ostream& operator<<( std::ostream& os, const TMQException& e )
{
    os << e.str();
    return os;
}

}