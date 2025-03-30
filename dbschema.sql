CREATE DATABASE RefData;

CREATE TABLE RefData.Users
(
    `ID` String,
    `LastUpdatedTs` Datetime64(9) DEFAULT now64(9),
    `LastUpdatedBy` String,
    `Active` UInt8,
    `Blob` String
)
ENGINE = MergeTree
ORDER BY (`ID`, `LastUpdatedTs`)