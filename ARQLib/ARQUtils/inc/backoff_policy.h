#pragma once
#include <ARQUtils/dll.h>

#include <string>
#include <chrono>
#include <format>
#include <optional>
#include <cstdint>

using namespace std::chrono_literals;

namespace ARQ
{

/**
 * @brief Implements an exponential backoff strategy for retry logic.
 * * This utility calculates the delay duration for subsequent retry attempts,
 * supporting an initial delay, a growth multiplier, a maximum delay cap,
 * and an optional maximum attempt limit.
 */
class BackoffPolicy
{
public:

	using Duration = std::chrono::milliseconds;

	/**
	 * @brief Configuration specification for the backoff strategy.
	 */
	struct Spec
	{
		Duration                initial     = 100ms; ///< The duration of the first delay.
		double                  multiplier  = 4;     ///< The factor by which the delay increases each attempt.
		Duration                maxDelay    = 5s;    ///< The upper limit for the calculated delay.
		std::optional<uint32_t> maxAttempts = 10;    ///< Optional limit on the number of retry attempts.
	};

	/**
	 * @brief User-friendly help text describing the string format for command-line arguments (use in e.g. cmd line arg help).
	 */
	static constexpr std::string_view HelpText =
		"Backoff Policy Format: 'initial-multiplier-max[-limit]'\n"
		"  initial:    Start duration (e.g., '100ms', '1s')\n"
		"  multiplier: Growth factor (e.g., '2.0' for double) or 'CONSTANT'\n"
		"  max:        Cap on duration (e.g., '10s', '1m')\n"
		"  limit:      (Optional) Max retry attempts before failing\n"
		"\n"
		"Examples:\n"
		"  '100ms-2.0-10s'     -> Start 100ms, double delay until 10s. Infinite retries.\n"
		"  '500ms-1.5-1m-5'    -> Start 500ms, x1.5 growth. Fail after 5 attempts.\n"
		"  '5s-CONSTANT-5s'    -> Retry every 5s forever.";

public:
	/*
	 * @brief Constructs the policy with the default spec.
	 */
	BackoffPolicy() = default;

	/**
	 * @brief Constructs the policy by parsing a specification string.
	 * @param specStr The policy string (e.g., "100ms-2.0-10s").
	 * @throws ARQException If the string format is invalid.
	 */
	ARQUtils_API explicit BackoffPolicy( const std::string_view specStr );

	/**
	 * @brief Constructs the policy from a structured specification.
	 * @param spec The configuration struct.
	 */
	BackoffPolicy( const Spec& spec )
		: m_spec( spec )
	{
	}

	/**
	 * @brief Calculates the delay for the next retry attempt.
	 * * Increments the internal attempt counter.
	 * @return The duration to wait, or std::nullopt if the max attempt limit has been reached.
	 */
	[[nodiscard]] ARQUtils_API std::optional<Duration> nextDelay();
	
	/**
	 * @brief Returns a string representation of the current attempt progress.
	 * @return A string like "Attempt 3/5" or "Attempt 3/Inf".
	 */
	[[nodiscard]] ARQUtils_API std::string attemptStr() const;

	/**
	 * @brief Resets the attempt counter to zero.
	 * Should be called after a successful operation or before starting a new sequence.
	 */
	void reset() { m_attempts = 0; }

private:
	void     initFromSpecStr( const std::string_view specStr );
	Duration parseDuration( const std::string_view durationStr ) const;

private:
	Spec     m_spec;
	uint32_t m_attempts = 0;
};

}