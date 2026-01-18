#include <ARQUtils/buffer.h>

namespace ARQ::Proto
{

template<typename ProtoType>
Buffer serialiseToBuffer( const ProtoType& protoObj )
{
	Buffer buf( protoObj.ByteSizeLong() );
	protoObj.SerializeToArray( buf.data.get(), buf.size );
	return buf;
}

}