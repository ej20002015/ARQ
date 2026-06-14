/**
 * Represents how to format a field when displaying in the UI.
 * Should be in sync with SemanticFormat enum in ARQLib/ARQCore/inc/semantic_format.h
 */
export type SemanticFormat =
    | "String"
    | "Integer"
    | "Decimal"
    | "Decimal2DP"
    | "Decimal4DP"
    | "Percentage"
    | "BasisPoints"
    | "Boolean"
    | "Date"
    | "DateTime"
    | "UUID";

/**
 * Indicates whether a refdata field is an index, and if so what type
 */
export type IndexType =
    | "None"
    | "Unique"
    | "NonUnique";

/**
 * Represents the underlying storage type for a field.
 * Should be in sync with PhysicalType enum in ARQLib/ARQCore/inc/physical_type.h
 */
export type PhysicalType =
    | "String"
    | "Double"
    | "DateTime"
    | "Boolean"
    | "UInt8"
    | "UInt32"
    | "Int32"
    | "Int64"
    | "UUID";

/**
 * Represents metadata about a single field/column in a reference data entity
 */
export interface RefDataFieldMetadata {
    name:         string;
    comment:      string;
    type:         PhysicalType;
    indexType:    IndexType;
    format:       SemanticFormat;
    uiReadOnly:   boolean;
    isPrimaryKey: boolean;
}

/**
 * Represents the response from the refdata metadata endpoint
 */
export interface RefDataMetadataResponse {
    data: RefDataFieldMetadata[];
    header: RefDataFieldMetadata[];
}