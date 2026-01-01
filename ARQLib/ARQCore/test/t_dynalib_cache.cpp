#include <ARQCore/dynalib_cache.h>
#include <gtest/gtest.h>

#include <ARQCore/streaming_service.h> // Temp
#include <ARQCore/refdata_command_manager.h>

using namespace ARQ;
using namespace std::string_view_literals; //temp

#include <iostream> //temp
#include <ARQUtils/enum.h> // Temp

TEST( DynaLibCacheTests, LoadARQClickHouse )
{
    EXPECT_NO_THROW( const auto& t = DynaLibCache::inst().get( "ARQClickHouse" ) );
}

TEST( Tmp, tmp1 )
{
	RD::Cmd::Upsert<RD::Currency> cmd;
	cmd.expectedVersion = 0;
	cmd.updatedBy = "Evan";
	cmd.targetUUID = ID::UUID::create();

	cmd.data.ccyID = "USD";
	cmd.data.name = "US Dollar";
	cmd.data.decimalPlaces = 2;
	cmd.data.settlementDays = 2;

	RD::CommandManager cmdMgr;
	cmdMgr.init( RD::CommandManager::Config() );
	cmdMgr.start();

	ID::UUID corrID = cmdMgr.sendCommand( cmd,
		[] ( const RD::CommandResponse& resp )
		{
			std::cout << "Command Response received: CorrID=" << resp.corrID
				<< ", Status=" << Enum::enum_name<RD::CommandResponse::Status>( resp.status )
				<< ", Message=" << ( resp.message ? *resp.message : "NULL" )
				<< std::endl;
		},
		Time::Milliseconds( 5000 )
	);

	while( true );
}

//TEST( Temp, temp1 ) // Temp
//{
//	std::shared_ptr<IStreamProducer> streamProducer;
//	try
//	{
//		streamProducer = StreamingServiceFactory::createProducer( "Kafka", StreamProducerOptions::OPTIMISE_LATENCY );
//	}
//	catch(const ARQException& e)
//	{
//		std::cout << e.what() << std::endl;
//	}
//
//	try
//	{
//		while( true )
//		{
//			auto tsStart = std::chrono::system_clock::now();
//
//			std::string msgStr = "A New Test message that dies";
//			Buffer msgData( reinterpret_cast<uint8_t*>( msgStr.data() ), msgStr.size() + 1 );
//			StreamProducerMessage msg;
//			msg.data = SharedBuffer( std::move( msgData ) );
//			msg.id = 1000;
//			msg.key = "TestKey";
//			msg.topic = "EvanTopicIsNotHere";
//			msg.headers["Test"] = "evan";
//
//			streamProducer->send( msg, [tsStart] ( const StreamProducerMessageMetadata& messageMetadata, std::optional<std::string> error )
//			{
//				std::cout << "Message delivered: ID=" << ( messageMetadata.messageID ? std::to_string( *messageMetadata.messageID ) : "N/A" )
//					<< ", Topic=" << messageMetadata.topic
//					<< ", Partition=" << messageMetadata.partition
//					<< ", Offset=" << ( messageMetadata.offset ? std::to_string( *messageMetadata.offset ) : "N/A" )
//					<< ", KeySize=" << messageMetadata.keySize
//					<< ", ValueSize=" << messageMetadata.valueSize
//					<< ", Timestamp=" << messageMetadata.timestamp.fmtISO8601()
//					<< ", PersistedStatus=" << Enum::enum_name<StreamMessagePersistedStatus>( messageMetadata.persistedStatus );
//
//				if( error )
//					std::cout << ", Error=" << *error;
//
//				std::cout << std::endl;
//
//				std::cout << "Total time taken: "
//					<< ( std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() - tsStart ).count() )
//					<< " ms" << std::endl;
//			} );
//
//			std::this_thread::sleep_for( 500ms );
//		}
//	}
//	catch( const ARQException& e )
//	{
//		std::cout << "ARQException caught: " << e.what() << std::endl;
//	}
//	catch( const std::exception& e )
//	{
//		std::cout << "std::exception caught: " << e.what() << std::endl;
//	}
//	catch( ... )
//	{
//		std::cout << "Unknown exception caught" << std::endl;
//	}
//
//	int y = 0;
//}
