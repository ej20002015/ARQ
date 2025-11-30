#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <algorithm>

namespace ARQ
{

struct Buffer
{
	std::unique_ptr<uint8_t[]> data = nullptr;
	size_t size                     = 0;

	Buffer() = default;

	// Construct zero-initialised buffer of a given size
	explicit Buffer( const size_t size )
		: size( size )
	{
		if( size )
			data = std::make_unique<uint8_t[]>( size );
	}

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

	Buffer( const Buffer& other )
		: Buffer( size )
	{
		if( other.data )
			std::copy_n( other.data.get(), size, data.get() );
	}

	Buffer& operator=( const Buffer& other )
	{
		if( this != &other )
		{
			size = other.size;

			data.release();
			if( other.data )
				std::copy_n( other.data.get(), size, data.get() );
		}

		return *this;
	}

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

	uint8_t* getDataPtr() const noexcept
	{
		return data.get();
	}

	template<typename T>
	T getDataPtrAs() const noexcept
	{
		return reinterpret_cast<T>( data.get() );
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