#include <ARQCore/refdata_command_manager.h>
#include <gtest/gtest.h>

using namespace ARQ;

// TEST( SKIP_RefDataCurrencyInsert, CurrencyInsert )
// {
// 	try
// 	{
// 		RD::CommandManager cmdMgr;
// 		cmdMgr.init( RD::CommandManager::Config() );
// 		cmdMgr.start();

// 		RD::Cmd::Upsert<RD::Currency> cmd;

// 		/*/// The immutable, globally unique system identifier (Machine Key)
// 		ID::UUID uuid;
// 		/// The 3-letter ISO 4217 currency code (e.g., USD).
// 		std::string ccyID;
// 		/// The full currency name (e.g., US Dollar).
// 		std::string name;
// 		/// Number of decimal places for standard formatting.
// 		uint8_t decimalPlaces;
// 		/// Standard number of days for spot settlement (commonly 2).
// 		uint8_t settlementDays;*/

// 		cmd.data = {
// 			.uuid = ID::UUID::create(),
// 			.ccyID = "JPY",
// 			.name = "Japanese Yen",
// 			.decimalPlaces = 2,
// 			.settlementDays = 2
// 		};

// 		cmd.targetUUID = cmd.data.uuid;
// 		cmd.expectedVersion = 0;
// 		cmd.updatedBy = "ejames";

// 		cmdMgr.sendCommand( cmd, [] ( const RD::CommandResponse& resp )
// 		{
// 			std::cout << "Command response received: corrID=" << resp.corrID.toString() << ", status=" << resp.status << ", message=" << ( resp.message ? *resp.message : "nullopt" ) << std::endl;
// 		} );

// 		cmdMgr.stop();
// 	}
// 	catch( const ARQException& ex )
// 	{
// 		std::cerr << "ARQException thrown: " << ex.what() << std::endl;
// 	}
// }