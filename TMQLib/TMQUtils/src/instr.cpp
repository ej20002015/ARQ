#include <TMQUtils/instr.h>

namespace TMQ
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
	m_tStart = std::chrono::system_clock::now();
}

void Timer::stop()
{
	if( m_tEnd != std::chrono::system_clock::time_point() )
		m_tEnd = std::chrono::system_clock::now();
}

}

}

