CREATE DATABASE RefData;

CREATE TABLE RefData.Users
(
    `ID` String,
    `LastUpdatedTs` Datetime64(9) DEFAULT now64(9),
    `LastUpdatedBy` String,
    `Active` UInt8,
    `Blob` String -- TODO: Change to Bytes
)
ENGINE = MergeTree
ORDER BY (`ID`, `LastUpdatedTs`)

CREATE DATABASE MktData;

CREATE TABLE MktData.MktObjs
(
    `Type` LowCardinality(String),
    `InstrumentID` String,
    `Context` LowCardinality(String),
    `AsofTs` Datetime64(9),
    `Source` LowCardinality(String),
    `LastUpdatedTs` Datetime64(9) DEFAULT now64(9),
    `LastUpdatedBy` String,
    `Active` UInt8,
    `Blob` String -- TODO: Change to Bytes
)
ENGINE = MergeTree
ORDER BY (`Context`, `Type`, `InstrumentID`, `Source`, `AsofTs`, `LastUpdatedTs`)
PARTITION BY (`Type`, toYYYYMM(`AsofTs`))

SELECT
    Type,
    InstrumentID,
    -- Get values from the row with the maximum AsofTs, using LastUpdatedTs as a tie-breaker
    argMax(AsofTs, (AsofTs, LastUpdatedTs)) AS max_AsofTs,
    argMax(Blob, (AsofTs, LastUpdatedTs)) AS max_Blob,
    argMax(Source, (AsofTs, LastUpdatedTs)) AS max_Source,
    argMax(LastUpdatedTs, (AsofTs, LastUpdatedTs)) AS max_LastUpdatedTs,
    argMax(LastUpdatedBy, (AsofTs, LastUpdatedTs)) AS max_LastUpdatedBy,
    argMax(Active, (AsofTs, LastUpdatedTs)) AS max_Active

FROM MktData.MktObjs

WHERE
    Context = 'LIVE'

GROUP BY
    Type, InstrumentID

HAVING max_Active = 1 -- Only return objects where the winning latest version is active

ORDER BY
    Type, InstrumentID;

-- Ensure database exists (if not already done)
-- CREATE DATABASE IF NOT EXISTS MktData;

-- Use the target database
USE MktData;

-- #####################################
-- ## Test Case 1: Basic LIVE Updates ##
-- #####################################
-- FX#EURUSD from SRC_A, sequential observation times
INSERT INTO MktData.MktObjs (Type, InstrumentID, Context, AsofTs, Source, LastUpdatedBy, Active, Blob) VALUES
('FX', 'EURUSD', 'LIVE', toDateTime64('2025-04-28 10:00:00.000000000', 9), 'SRC_A', 'User1', 1, 'BlobData:EURUSD_100000'),
('FX', 'EURUSD', 'LIVE', toDateTime64('2025-04-28 10:05:00.123456789', 9), 'SRC_A', 'User1', 1, 'BlobData:EURUSD_100500'),
('FX', 'EURUSD', 'LIVE', toDateTime64('2025-04-28 10:10:00.987654321', 9), 'SRC_A', 'User1', 1, 'BlobData:EURUSD_101000');
-- Expected Latest (LIVE): The 10:10 record based on AsofTs

-- #############################################
-- ## Test Case 2: Out-of-Order LIVE Updates  ##
-- #############################################
-- Insert an older AsofTs timestamp *after* the 10:10 record was inserted.
-- Note: LastUpdatedTs will be automatically set higher than the 10:10 record's LastUpdatedTs
INSERT INTO MktData.MktObjs (Type, InstrumentID, Context, AsofTs, Source, LastUpdatedBy, Active, Blob) VALUES
('FX', 'EURUSD', 'LIVE', toDateTime64('2025-04-28 10:08:00.000000000', 9), 'SRC_A', 'User2', 1, 'BlobData:EURUSD_100800_LATE');
-- Expected Latest (LIVE): Still the 10:10 record, because query `argMax(..., (AsofTs, LastUpdatedTs))` prioritizes AsofTs.

-- ##############################################
-- ## Test Case 3: Multiple Sources LIVE       ##
-- ##############################################
-- EQ#AAPL US from SRC_A and SRC_B
INSERT INTO MktData.MktObjs (Type, InstrumentID, Context, AsofTs, Source, LastUpdatedBy, Active, Blob) VALUES
('EQ', 'AAPL US', 'LIVE', toDateTime64('2025-04-28 11:00:00.000000000', 9), 'SRC_A', 'User1', 1, 'BlobData:AAPL_A_110000'),
('EQ', 'AAPL US', 'LIVE', toDateTime64('2025-04-28 11:02:00.000000000', 9), 'SRC_B', 'User1', 1, 'BlobData:AAPL_B_110200'),
('EQ', 'AAPL US', 'LIVE', toDateTime64('2025-04-28 11:05:00.000000000', 9), 'SRC_A', 'User1', 1, 'BlobData:AAPL_A_110500'),
('EQ', 'AAPL US', 'LIVE', toDateTime64('2025-04-28 11:07:00.000000000', 9), 'SRC_B', 'User1', 1, 'BlobData:AAPL_B_110700');
-- Expected Latest (LIVE, ignoring source): The SRC_B 11:07 record (based on AsofTs)
-- Expected Latest (LIVE, per source): SRC_A=11:05 record, SRC_B=11:07 record

-- ##############################################
-- ## Test Case 4: Correction using LastUpdatedTs ##
-- ##############################################
-- Insert a correction for the SRC_A 11:05 record *now* (later LastUpdatedTs).
-- Use the *same* AsofTs.
INSERT INTO MktData.MktObjs (Type, InstrumentID, Context, AsofTs, Source, LastUpdatedBy, Active, Blob) VALUES
('EQ', 'AAPL US', 'LIVE', toDateTime64('2025-04-28 11:07:00.000000000', 9), 'SRC_A', 'User3_Corrector', 1, 'BlobData:AAPL_A_110500_CORRECTED');
-- Expected Latest (LIVE, ignoring source): Still the SRC_B 11:07 record (highest AsofTs).
-- Expected Latest (LIVE, SRC_A only): This new "CORRECTED" record (same max AsofTs for SRC_A, but higher LastUpdatedTs tie-breaker).

-- ##############################################
-- ## Test Case 5: Active Flag Usage LIVE      ##
-- ##############################################
-- Simulate marking an older record inactive by inserting a new record with Active=0
-- (Note: Real update logic might differ, but this tests querying)
-- Let's make the 10:05 FX record inactive. We insert a new row with Active=0 and same keys/timestamps.
-- The argMax logic should then pick the status from the *latest* insertion for that timestamp.
-- This is slightly contrived for MergeTree; ReplacingMergeTree would handle this better.
-- For simplicity, let's just add an inactive record for a *new* time for EQ', 'AAPL US.
INSERT INTO MktData.MktObjs (Type, InstrumentID, Context, AsofTs, Source, LastUpdatedBy, Active, Blob) VALUES
('EQ', 'AAPL US', 'LIVE', toDateTime64('2025-04-28 11:10:00.000000000', 9), 'SRC_B', 'User4_Deactivator', 0, 'BlobData:AAPL_B_111000_INACTIVE');
-- Expected Latest (LIVE, ignoring source, ignoring Active flag): This 11:10 record (highest AsofTs).
-- Expected Latest (LIVE, ignoring source, HAVING Is_Active=1): Should return the 11:07 SRC_B record.

-- ##############################################
-- ## Test Case 6: EOD Snapshot Data           ##
-- ##############################################
-- Snapshot for yesterday, EOD 2025-04-27
INSERT INTO MktData.MktObjs (Type, InstrumentID, Context, AsofTs, Source, LastUpdatedBy, Active, Blob) VALUES
('FX', 'EURUSD', 'EOD_20250427', toDateTime64('2025-04-27 16:30:00.000000000', 9), 'SRC_A', 'EOD_Process', 1, 'BlobData:EOD_EURUSD_A_0427'),
('EQ', 'AAPL US', 'EOD_20250427', toDateTime64('2025-04-27 16:30:00.000000000', 9), 'SRC_A', 'EOD_Process', 1, 'BlobData:EOD_AAPL_A_0427_INCORRECT'),
('EQ', 'TSLA US', 'EOD_20250427', toDateTime64('2025-04-27 16:30:00.000000000', 9), 'SRC_B', 'EOD_Process', 1, 'BlobData:EOD_TSLA_B_0427');

-- ##############################################
-- ## Test Case 7: EOD Correction              ##
-- ##############################################
-- Insert the corrected EQ#AAPL US data for SRC_A for the 2025-04-27 EOD snapshot.
-- This insertion happens later (e.g., now, 2025-04-28) but refers to the same EOD AsofTs.
INSERT INTO MktData.MktObjs (Type, InstrumentID, Context, AsofTs, Source, LastUpdatedBy, Active, Blob) VALUES
('EQ', 'AAPL US', 'EOD_20250427', toDateTime64('2025-04-27 16:30:00.000000000', 9), 'SRC_A', 'User6_EODCorrector', 1, 'BlobData:EOD_AAPL_A_0427_CORRECTED');
-- Expected Latest (EOD_20250427, EQ#AAPL US, SRC_A): This "CORRECTED" record, based on `argMax(..., LastUpdatedTs)`.

-- Optional: If the correction process also marks the old one inactive.
-- This requires knowing the exact keys of the old row to update, which is complex with MergeTree.
-- Relying on argMax(..., LastUpdatedTs) during load is the simpler approach with MergeTree.
-- UPDATE MktData.MktObjs SET Active = 0 WHERE Context = 'EOD_20250427' AND ID = 'EQ#AAPL US' AND Source = 'SRC_A' AND LastUpdatedTs = <Original_LastUpdatedTs_Value>; -- UPDATEs are heavy!