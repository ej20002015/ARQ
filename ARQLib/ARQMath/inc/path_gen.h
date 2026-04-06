#pragma once
#include <ARQMath/dll.h>

#include <cstdint>
#include <vector>
#include <random>
#include <functional>

namespace ARQ::Math::Stochastic
{

struct PeriodConfig
{
	double   startValue = 0.0;
	double   endValue   = 0.0;
	double   volatility = 0.0;
	uint64_t seed       = 0;
};

class BrownianBridgePathGenerator
{
public:
	using onPeriodRollover = std::function<PeriodConfig( const int64_t currentPeriodIndex )>;

	struct Config
	{
		int64_t          ticksPerStep;
		int64_t          ticksPerPeriod;
		onPeriodRollover onPeriodRollover;
	};

public:
	ARQMath_API explicit BrownianBridgePathGenerator( const Config& config );

	[[nodiscard]] ARQMath_API double getValueAtTime( const int64_t currTick );

	[[nodiscard]] ARQMath_API const PeriodConfig& currentPeriodConfig() const { return m_currentPeriodConfig; }

private:
	void genPathForPeriod();

private:
	Config   m_config;

	int64_t      m_stepsPerPeriod;
	int64_t      m_currentPeriodIndex;
	PeriodConfig m_currentPeriodConfig;

	std::vector<double> m_pathValues;

	std::mt19937 m_rng;
};

}