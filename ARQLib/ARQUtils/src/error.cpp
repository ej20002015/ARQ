#include <ARQUtils/error.h>

#include <sstream>

namespace ARQ
{

std::string ARQException::str() const
{
    std::ostringstream ss;
    ss << "What: \"" << what() << "\"; Where: \"" << where() << "\"; Stack: \"" << stack() << "\"";
    return ss.str();
}

std::ostream& operator<<( std::ostream& os, const ARQException& e )
{
    os << e.str();
    return os;
}

}