#include <ARQUtils/backoff_policy.h>

#include <ARQUtils/str.h>
#include <ARQUtils/error.h>

namespace ARQ
{

BackoffPolicy::BackoffPolicy( const std::string_view spec )
{
	initFromSpecStr( spec );
}

std::optional<BackoffPolicy::Duration> BackoffPolicy::nextDelay()
{
	if( m_spec.maxAttempts && m_attempts >= m_spec.maxAttempts )
		return std::nullopt;

	const double delayMs = static_cast<double>( m_spec.initial.count() ) * std::pow( m_spec.multiplier, m_attempts );

	// Cap at Max Delay
	auto calculated = static_cast<Duration::rep>( delayMs );
	Duration next = Duration( calculated );
	if( next > m_spec.maxDelay )
		next = m_spec.maxDelay;

	++m_attempts;
	return next;
}

std::string BackoffPolicy::attemptStr() const
{
	return std::format( "Attempt {}/{}", m_attempts, m_spec.maxAttempts ? std::to_string( *m_spec.maxAttempts ) : "Inf" );
}

void BackoffPolicy::initFromSpecStr( const std::string_view specStr )
{
	const std::vector<std::string_view> tokens = Str::split( specStr, '-' );
	if( tokens.size() < 3 )
		throw ARQException( std::format( "Invalid backoff policy string - expected 'initial-mult-max[-limit]' - was only given {} elements", tokens.size() ) );

	m_spec.initial  = parseDuration( tokens[0] );
	m_spec.maxDelay = parseDuration( tokens[2] );

	if( tokens[1] == "CONSTANT" )
		m_spec.multiplier = 1.0;
	else
	{
		try
		{
			m_spec.multiplier = std::stod( tokens[1].data() );
		}
		catch( const std::exception& e )
		{
			throw ARQException( std::format( "Invalid backoff policy string - couldn't parse multiplier token '{}' as double", tokens[1] ) );
		}
	}

	if( tokens.size() > 3 )
	{
		try
		{
			m_spec.maxAttempts = static_cast<uint32_t>( std::stoul( tokens[3].data() ) );
		}
		catch( const std::exception& e )
		{
			throw ARQException( std::format( "Invalid backoff policy string - couldn't parse limit token '{}' as uint", tokens[3] ) );
		}
	}
}

BackoffPolicy::Duration BackoffPolicy::parseDuration( const std::string_view durationStr ) const
{
	int32_t val = 0;
	auto [ptr, ec] = std::from_chars( durationStr.data(), durationStr.data() + durationStr.size(), val );
	if( ec != std::errc() )
		throw ARQException( std::format( "Invalid backoff policy string - couldn't parse string '{}' as duration", durationStr ) );

	if( *ptr != '\0' )
	{
		const std::string_view suffix = ptr;
		if( suffix == "s" )
			return std::chrono::seconds( val );
		if( suffix == "m" )
			return std::chrono::minutes( val );
	}

	return std::chrono::milliseconds( val );
}



}