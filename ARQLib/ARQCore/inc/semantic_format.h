#pragma once

namespace ARQ
{

/**
 * @brief Set of formats to apply when displaying values in the UI.
 */
enum class SemanticFormat
{
    String,
    UUID,            // Display as standard 36-character UUID string
    Integer,
    Decimal,         // Raw float/double, no forced UI rounding
    Decimal2DP,
    Decimal4DP,
    Percentage,      // Native 0.05 displays as 5%
    BasisPoints,     // Native 0.0005 displays as 5 bps
    Boolean,
    Date,            // YYYY-MM-DD
    DateTime         // Full timestamp
};

}