#include <ARQUtils/instr.h>
#include <ARQUtils/enum.h>
#include <ARQCore/lib.h>
#include <ARQCore/refdata_command_manager.h>

#include <iostream>
#include <string>

using namespace ARQ;

int main( int argc, char* argv[] )
{
	LibGuard guard( argc, argv );

	RD::CommandManager cmdMgr;
	cmdMgr.init( RD::CommandManager::Config() );
	cmdMgr.start();

	while( true )
	{
		std::cout << "Creating Upsert<User> command\n" << std::flush;

		RD::Cmd::Upsert<RD::User> cmd;

		std::cout << "\ntargetUUID (leave blank for new record): " << std::flush;
		std::string targetUUIDStr;
		std::getline( std::cin, targetUUIDStr );
		if( targetUUIDStr.size() )
			cmd.targetUUID = ID::uuidFromStr( targetUUIDStr );
		else
			cmd.targetUUID = ID::uuidCreate();
		cmd.data.uuid = cmd.targetUUID;

		std::cout << "\nuserID: " << std::flush;
		std::getline( std::cin, cmd.data.userID );

		std::cout << "\nfullName: " << std::flush;
		std::getline( std::cin, cmd.data.fullName );

		std::cout << "\nemail: " << std::flush;
		std::getline( std::cin, cmd.data.email );

		std::cout << "\ntradingDesk: " << std::flush;
		std::getline( std::cin, cmd.data.tradingDesk );

		std::cout << "\nupdatedBy: " << std::flush;
		std::getline( std::cin, cmd.updatedBy );

		std::cout << "\nexpectedVersion (set as zero for new record): " << std::flush;
		std::string expectedVersionStr;
		std::getline( std::cin, expectedVersionStr );
		cmd.expectedVersion = std::stoul( expectedVersionStr );

		std::cout << "\n--------------------------------------------------\n";
		std::cout << "COMMAND PREVIEW:\n";
		std::cout << "--------------------------------------------------\n";
		std::cout << "  Type            : Upsert<User>\n";
		std::cout << "  Target UUID     : " << cmd.targetUUID << "\n";
		std::cout << "  User ID         : " << cmd.data.userID << "\n";
		std::cout << "  Full Name       : " << cmd.data.fullName << "\n";
		std::cout << "  Email           : " << cmd.data.email << "\n";
		std::cout << "  Trading Desk    : " << cmd.data.tradingDesk << "\n";
		std::cout << "  Updated By      : " << cmd.updatedBy << "\n";
		std::cout << "  Expected Ver    : " << cmd.expectedVersion << "\n";
		std::cout << "--------------------------------------------------\n";
		std::cout << "Are you sure you want to insert the above? (Y/n) ";

		std::string ack;
		std::getline( std::cin, ack );
		if( ack != "Y" && ack != "y" )
		{
			break;
		}

		std::cout << "\nPublishing command " << Time::DateTime::nowUTC().fmtISO8601() << std::endl;

		std::atomic<bool> done = false;
		Instr::Timer tm;
		cmdMgr.sendCommand( cmd, [&done, &tm] ( const RD::CommandResponse& resp )
		{
			auto micros = tm.duration<std::chrono::microseconds>();

			std::cout << "\n--------------------------------------------------\n";
			std::cout << "COMMAND RESPONSE (" << micros.count() << " us):\n";
			std::cout << "--------------------------------------------------\n";

			std::cout << "  Status : " << Enum::enum_name( resp.status ) << "\n";
			std::cout << "  CorrID : " << resp.corrID << "\n";

			if( resp.message.has_value() && !resp.message->empty() )
			{
				std::cout << "  Message: " << *resp.message << "\n";
			}

			if( resp.status == RD::CommandResponse::Status::SUCCESS )
			{
				std::cout << "  Result : [ OK ]\n";
			}
			else
			{
				std::cout << "  Result : [ FAILED ]\n";
			}
			std::cout << "--------------------------------------------------\n";

			done = true;
		}, Time::Milliseconds( 5000 ) );

		while( !done )
		{
			std::this_thread::sleep_for( 5ms );
		}

		std::cout << "Command publish done" << std::endl;

		std::cout << "Do you wish to send another command? (Y/n) ";
		std::string exit;
		std::getline( std::cin, exit );
		if( exit != "Y" && exit != "y" )
			break;
	}

	std::cout << "Exiting" << std::endl;

	cmdMgr.stop();
}