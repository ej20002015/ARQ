#include <ARQUtils/instr.h>

namespace ARQ
{

namespace Instr
{

Timer::Timer( const StartOnConstruction soc )
{
	if( soc == YES )
		start();
}

void Timer::start()
{
	m_tStart = std::chrono::steady_clock::now();
}

void Timer::stop()
{
	if( m_tEnd != std::chrono::steady_clock::time_point() )
		m_tEnd = std::chrono::steady_clock::now();
}

}

}

