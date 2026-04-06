#include <ARQMath/path_gen.h>

#include <ARQUtils/error.h>

namespace ARQ::Math::Stochastic
{

BrownianBridgePathGenerator::BrownianBridgePathGenerator( const Config& config )
	: m_config( config )
	, m_stepsPerPeriod( config.ticksPerPeriod / config.ticksPerStep )
	, m_currentPeriodIndex( -1 ) // Force first getValueAtTime call to trigger period rollover
{
	if( m_config.ticksPerPeriod % m_config.ticksPerStep )
		throw ARQException( "ticksPerPeriod must be an integer multiple of ticksPerStep" );
	if( !m_config.onPeriodRollover )
		throw ARQException( "onPeriodRollover callback must be provided" );
}

double BrownianBridgePathGenerator::getValueAtTime( const int64_t currTick )
{
	const int64_t periodIndex = currTick / m_config.ticksPerPeriod;
	if( periodIndex > m_currentPeriodIndex )
	{
		m_currentPeriodIndex = periodIndex;
		m_currentPeriodConfig = m_config.onPeriodRollover( periodIndex );
		genPathForPeriod();
	}

	const int64_t stepIndex = ( currTick % m_config.ticksPerPeriod ) / m_config.ticksPerStep;
	return m_pathValues[stepIndex];
}

void BrownianBridgePathGenerator::genPathForPeriod()
{
	m_rng.seed( m_currentPeriodConfig.seed );

	std::normal_distribution<double> dist( 0.0, 1.0 );
	std::vector<double> wienerValues( m_stepsPerPeriod, 0.0 );
	double currW = 0;
	const double sqrtTm = std::sqrt( 1.0 / m_stepsPerPeriod );
	for( size_t i = 1; i < m_stepsPerPeriod; ++i )
	{
		currW += dist( m_rng ) * sqrtTm;
		wienerValues[i] = currW;
	}

	const double w_T = wienerValues.back();
	const double p_0 = m_currentPeriodConfig.startValue;
	const double p_T = m_currentPeriodConfig.endValue;

	m_pathValues.assign( m_stepsPerPeriod, 0.0 );
	for( size_t i = 0; i < m_stepsPerPeriod; ++i )
	{
		const double t = static_cast<double>( i );
		const double T = static_cast<double>( m_stepsPerPeriod - 1 ); // -1 because t starts at 0

		const double drift = p_0 + ( t / T ) * ( p_T - p_0 );
		const double randomness = wienerValues[i] - ( t / T ) * w_T;
		m_pathValues[i] = drift + m_currentPeriodConfig.volatility * randomness;
	}
}

}