#include <cstdint>
#include <memory>

namespace TMQ
{

struct OwningBuffer
{
	std::unique_ptr<uint8_t[]> data = nullptr;
	size_t size                     = 0;

	OwningBuffer() = default;
	OwningBuffer( uint8_t* data, const size_t size )
		: data( data )
		, size( size )
	{
	}
};

struct BufferView
{
	const uint8_t* data = nullptr;
	size_t size = 0;

	BufferView() = default;
	BufferView( const uint8_t* data, const size_t size )
		: data( data )
		, size( size )
	{}
};

}