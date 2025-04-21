#pragma once

#include <cstdint>
#include <memory>
#include <utility>

namespace TMQ
{

struct Buffer
{
	std::unique_ptr<uint8_t[]> data = nullptr;
	size_t size                     = 0;

	Buffer() = default;

	// Construct zero-initialised buffer of a given size
	explicit Buffer( const size_t size )
		: data( std::make_unique<uint8_t[]>(size))
		, size( size )
	{}

	// Construct buffer by copying the given data
	explicit Buffer( const uint8_t* dataToCopy, const size_t size )
		: Buffer( size )
	{
		if( dataToCopy )
			std::copy_n( dataToCopy, size, data.get() );
	}

	// Construct buffer by taking ownership of given data
	explicit Buffer( std::unique_ptr<uint8_t[]> data, const size_t size )
		: data( std::move( data ) )
		, size( size )
	{}

	~Buffer() = default;

	Buffer( const Buffer& ) = delete;
	Buffer& operator=( const Buffer& ) = delete;

	Buffer( Buffer&& other ) noexcept
		: data( std::move( other.data ) )
		, size( std::exchange( other.size, 0 ) )
	{
	}

	Buffer& operator=( Buffer&& other ) noexcept
	{
		if( this != &other )
		{
			data = std::move( other.data );
			size = std::exchange( other.size, 0 );
		}

		return *this;
	}
};

struct BufferView
{
	const uint8_t* data = nullptr;
	size_t size         = 0;

	BufferView() = default;

	// Construct BufferView from raw data pointer
	explicit BufferView( const uint8_t* data, const size_t size )
		: data( data )
		, size( size )
	{}

	// Construct BufferView from Buffer
	BufferView( const Buffer& buffer )
		: data( buffer.data.get() )
		, size( buffer.size )
	{}

};

}