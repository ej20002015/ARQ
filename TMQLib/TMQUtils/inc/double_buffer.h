#pragma once

#include "dll.h"

#include <array>
#include <atomic>

namespace TMQ
{

template<typename T>
class DoubleBuffer
{
private:
	enum
	{
		WRITE_IDX = 1 << 0, // The current index of the buffer being written to
		NEW_DATA = 1 << 1, // Flag to indicate that new data has been written
		WRITE_BUSY = 1 << 2  // Flag to indicate that the write buffer is busy
	};

public:
	DoubleBuffer() = default;

	void write( const T& data )
	{
		// Get WRITE_IDX and set WRITE_BUSY to 1
		const uint8_t writeIdx = bufferState.fetch_or( WRITE_BUSY ) & WRITE_IDX;
		m_buffers[writeIdx] = data;
		// Set NEW_DATA to 1, set WRITE_BUSY to 0, preserve WRITE_IDX
		bufferState.store( writeIdx | NEW_DATA );
	}

	const T& read() const
	{
		uint8_t currentState = bufferState.load();

		// New data so need to flip buffers
		if( currentState & NEW_DATA )
		{
			uint8_t expectedState = currentState & ~WRITE_BUSY;    // Whatever the current state is except WRITE_BUSY is zero
			uint8_t desiredState = ( currentState & WRITE_IDX ) ^ 1; // Flip the WRITE_IDX and clear the NEW_DATA flag
			// Loop until WRITE_BUSY is zero, then set to desiredState
			while( !bufferState.compare_exchange_weak( expectedState, desiredState ) );

			currentState = desiredState;
		}

		const uint8_t readIdx = ( currentState & WRITE_IDX ) ^ 1; // Flip the WRITE_IDX
		return m_buffers[readIdx];
	}

private:
	std::array<T, 2> m_buffers;

	mutable std::atomic<uint8_t> bufferState = 0b000;
	static_assert( std::atomic<uint8_t>::is_always_lock_free );
};

TMQ_API int dummyFunction();

}