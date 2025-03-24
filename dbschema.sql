CREATE DATABASE RefData;

CREATE TABLE RefData.Users
(
    ID String,
    Ts DateTime,
    Active UInt8,
    Blob String
)
ENGINE = MergeTree()
ORDER BY (ID, Ts);