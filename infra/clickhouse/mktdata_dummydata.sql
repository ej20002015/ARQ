-- =================================================================
--  Dummy Data for MktData.FXRates
-- =================================================================

INSERT INTO MktData.FXRates 
(`MarketName`, `ID`, `Source`, `AsofTs`, `Mid`, `Bid`, `Ask`, `_IsActive`, `_LastUpdatedBy`) 
VALUES
-- LIVE Market Data
('LIVE', 'EUR/USD', 'Bloomberg', '2025-07-27 14:30:00.000', 1.0845, 1.0844, 1.0846, 1, 'market_data_feed'),
('LIVE', 'GBP/USD', 'Bloomberg', '2025-07-27 14:30:15.000', 1.2765, 1.2764, 1.2766, 1, 'market_data_feed'),
('LIVE', 'USD/JPY', 'Bloomberg', '2025-07-27 14:30:30.000', 154.25, 154.23, 154.27, 1, 'market_data_feed'),
('LIVE', 'USD/CHF', 'Reuters', '2025-07-27 14:31:00.000', 0.8721, 0.8720, 0.8722, 1, 'market_data_feed'),
('LIVE', 'AUD/USD', 'Reuters', '2025-07-27 14:31:15.000', 0.6598, 0.6597, 0.6599, 1, 'market_data_feed'),
('LIVE', 'USD/CAD', 'ICE', '2025-07-27 14:31:30.000', 1.3856, 1.3855, 1.3857, 1, 'market_data_feed'),
('LIVE', 'GBP/USD', 'Bloomberg', '2025-07-27 14:32:00.000', 1.2770, 1.2769, 1.2771, 1, 'market_data_feed'),

-- Historical EOD Market Data (July 24th)
('EOD_20250724', 'EUR/USD', 'Bloomberg', '2025-07-24 21:00:00.000', 1.0823, 1.0822, 1.0824, 1, 'eod_data_loader'),
('EOD_20250724', 'GBP/USD', 'Bloomberg', '2025-07-24 21:00:00.000', 1.2742, 1.2741, 1.2743, 1, 'eod_data_loader'),
('EOD_20250724', 'USD/JPY', 'Bloomberg', '2025-07-24 21:00:00.000', 153.89, 153.87, 153.91, 1, 'eod_data_loader'),
('EOD_20250724', 'USD/CHF', 'Reuters', '2025-07-24 21:00:00.000', 0.8709, 0.8708, 0.8710, 1, 'eod_data_loader'),
('EOD_20250724', 'AUD/USD', 'Reuters', '2025-07-24 21:00:00.000', 0.6612, 0.6611, 0.6613, 1, 'eod_data_loader'),
('EOD_20250724', 'USD/CAD', 'ICE', '2025-07-24 21:00:00.000', 1.3834, 1.3833, 1.3835, 1, 'eod_data_loader'),

-- Stress Test Scenario Data
('VAR_STRESS_1', 'EUR/USD', 'Risk_System', '2025-07-27 00:00:00.000', 1.0500, 1.0499, 1.0501, 1, 'risk_scenario_gen'),
('VAR_STRESS_1', 'GBP/USD', 'Risk_System', '2025-07-27 00:00:00.000', 1.2400, 1.2399, 1.2401, 1, 'risk_scenario_gen'),
('VAR_STRESS_1', 'USD/JPY', 'Risk_System', '2025-07-27 00:00:00.000', 160.00, 159.98, 160.02, 1, 'risk_scenario_gen'),
('VAR_STRESS_1', 'USD/CHF', 'Risk_System', '2025-07-27 00:00:00.000', 0.9000, 0.8999, 0.9001, 1, 'risk_scenario_gen'),

-- Historical Data (July 23rd EOD)
('EOD_20250723', 'EUR/USD', 'Bloomberg', '2025-07-23 21:00:00.000', 1.0801, 1.0800, 1.0802, 1, 'eod_data_loader'),
('EOD_20250723', 'GBP/USD', 'Bloomberg', '2025-07-23 21:00:00.000', 1.2718, 1.2717, 1.2719, 1, 'eod_data_loader'),
('EOD_20250723', 'USD/JPY', 'Bloomberg', '2025-07-23 21:00:00.000', 153.12, 153.10, 153.14, 1, 'eod_data_loader');