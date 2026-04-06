#include <ARQUtils/buffer.h>

namespace ARQ
{

const Buffer& Buffer::Null()
{
	static const Buffer nullBuf;
	return nullBuf;
}

}