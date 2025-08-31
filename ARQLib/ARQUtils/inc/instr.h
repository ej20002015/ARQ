#pragma once
#include <ARQUtils/dll.h>

#include <ARQUtils/time.h>

#include <chrono>

namespace ARQ
{

namespace Instr
{

class Timer
{
public:
	enum StartOnConstruction
	{
		YES,
		NO
	};

	Timer( const StartOnConstruction soc = YES );

	Timer( const Timer& ) = delete;
	Timer& operator=( const Timer& ) = delete;

	void start();
	void stop();

	template<Time::c_Duration Duration = std::chrono::nanoseconds>
	Duration duration()
	{ 
		stop();
		return std::chrono::duration_cast<Duration>( m_tEnd - m_tStart );
	}

private:
	std::chrono::steady_clock::time_point m_tStart;
	std::chrono::steady_clock::time_point m_tEnd;
};

}

}