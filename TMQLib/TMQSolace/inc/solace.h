#include <string>

#include <TMQUtils/buffer.h>

#include "dll.h"

namespace TMQ
{

enum class SolaceRet
{
	SUCCESS,
	FAILURE
};

using SolaceSubCallback = void ( * )( std::string_view, BufferView );

class Solace
{
	static SolaceRet sub( const std::string_view subTopic, SolaceSubCallback callback );
	static SolaceRet pub( const std::string_view topic, OwningBuffer data );
};

TMQ_API int foo();

}