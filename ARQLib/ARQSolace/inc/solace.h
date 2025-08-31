#pragma once
#include <ARQSolace/dll.h>

#include <ARQUtils/buffer.h>

#include <string>


namespace ARQ
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
	static SolaceRet pub( const std::string_view topic, Buffer data );
};

ARQSolace_API int foo();

}