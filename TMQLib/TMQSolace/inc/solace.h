#pragma once
#include <TMQSolace/dll.h>

#include <TMQUtils/buffer.h>

#include <string>


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
	static SolaceRet pub( const std::string_view topic, Buffer data );
};

TMQSolace_API int foo();

}