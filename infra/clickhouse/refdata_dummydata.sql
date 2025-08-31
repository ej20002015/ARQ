-- =================================================================
--  Dummy Data for RefData.Currencies
-- =================================================================

INSERT INTO RefData.Currencies (CcyID, Name, DecimalPlaces, SettlementDays, _IsActive, _LastUpdatedBy) VALUES
('USD', 'US Dollar', 2, 2, 1, 'System'),
('EUR', 'Euro', 2, 2, 1, 'System'),
('JPY', 'Japanese Yen', 2, 2, 1, 'System'),
('GBP', 'British Pound', 2, 2, 1, 'System'),
('AUD', 'Australian Dollar', 2, 2, 1, 'System'),
('CAD', 'Canadian Dollar', 2, 2, 1, 'System'),
('CHF', 'Swiss Franc', 2, 2, 1, 'System'),
('CNH', 'Chinese Yuan Offshore', 2, 2, 1, 'System'),
('HKD', 'Hong Kong Dollar', 2, 2, 1, 'System'),
('SGD', 'Singapore Dollar', 2, 2, 1, 'System'),
('NZD', 'New Zealand Dollar', 2, 2, 1, 'System'),
('SEK', 'Swedish Krona', 2, 2, 1, 'System'),
('NOK', 'Norwegian Krone', 2, 2, 1, 'System'),
('MXN', 'Mexican Peso', 2, 2, 1, 'System'),
('ZAR', 'South African Rand', 2, 2, 1, 'System');


-- =================================================================
--  Dummy Data for RefData.Users
-- =================================================================

INSERT INTO RefData.Users (UserID, FullName, Email, TradingDesk, _IsActive, _LastUpdatedBy) VALUES
('jdoe', 'John Doe', 'john.doe@example.com', 'FX G10', 1, 'Admin'),
('asmith', 'Alice Smith', 'alice.smith@example.com', 'Rates', 1, 'Admin'),
('bwilliams', 'Bob Williams', 'bob.williams@example.com', 'Commodities', 1, 'Admin'),
('cjones', 'Charlie Jones', 'charlie.jones@example.com', 'Equities', 1, 'Admin'),
('davisd', 'Diana Davis', 'diana.davis@example.com', 'FX G10', 1, 'Admin'),
('emiller', 'Ethan Miller', 'ethan.miller@example.com', 'Rates', 1, 'Admin'),
('fgarcia', 'Fiona Garcia', 'fiona.garcia@example.com', 'Emerging Markets', 1, 'Admin'),
('gwilson', 'George Wilson', 'george.wilson@example.com', 'FX Options', 1, 'Admin'),
('hmoore', 'Helen Moore', 'helen.moore@example.com', 'Commodities', 0, 'Admin'),
('itaylor', 'Ian Taylor', 'ian.taylor@example.com', 'Credit', 1, 'Admin'),
('ejames', 'Evan James', 'evan.james@investec.com', 'Equities', 1, 'Admin');

INSERT INTO RefData.Users (userID, fullName, email, tradingDesk, IsActive, LastUpdatedBy) VALUES ('ejames', 'Evan James', 'evan.james@investec.com', 'Equities', 0, 'Admin');