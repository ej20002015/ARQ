#ifndef market_i
#define market_i

%include <std_string.i>
%include <std_string_view.i>

%include "../TMQUtils/date.i"

%{
	#include <TMQMarket/market.h>
%}

namespace TMQ {
namespace Mkt {

struct Context
{
	std::string tag;
	TMQ::Time::Date date;

	Context( const std::string_view tag );
	Context( const std::string_view tag, const TMQ::Time::Date date );

	std::string str() const;

	static const Context LIVE;
};

}
}

#endif