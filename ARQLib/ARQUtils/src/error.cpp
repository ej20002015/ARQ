#include <ARQUtils/error.h>

#include <sstream>

namespace ARQ
{

std::string ARQException::str() const
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

std::ostream& operator<<( std::ostream& os, const ARQException& e )
{
    os << e.str();
    return os;
}

}