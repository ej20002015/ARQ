#pragma once

namespace ARQ
{

/**
 * @brief Set of physical data types that can be used by entity fields.
 * Represents the underlying storage type for a field, independent of how it may be semantically formatted in the UI.
 * @note This should always be kept in sync with the types in /codegen/definitions/types.toml
 */
enum class PhysicalType
{
    String,
    Double,
    DateTime,
    Boolean,
    UInt8,
    UInt32,
    Int32,
    Int64,
    UUID,
};

}