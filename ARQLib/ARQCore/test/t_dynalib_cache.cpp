#include <ARQCore/dynalib_cache.h>
#include <gtest/gtest.h>

#include <ARQCore/streaming_service.h> // Temp
#include <ARQCore/refdata_command_manager.h> // Temp
#include <ARQCore/refdata_repository.h> // Temp

using namespace ARQ;
using namespace std::string_view_literals; //temp

#include <iostream> //temp
#include <ARQUtils/enum.h> // Temp
#include <atomic>

TEST( DynaLibCacheTests, LoadARQClickHouse )
{
    EXPECT_NO_THROW( const auto& t = DynaLibCache::inst().get( "ARQClickHouse" ) );
}

//TEST( Tmp, tmp3 )
//{
//    static constexpr auto TOPIC = "ARQ.RefData.Commands.Upsert.Currency";
//
//    std::shared_ptr<IStreamConsumer> streamConsumer;
//	try
//	{
//		StreamConsumerOptions opts( "test", "ARQ.RefData.CommandConsumers", StreamConsumerOptions::FetchPreset::DEFAULT, StreamConsumerOptions::AutoCommitOffsets::Disabled );
//		streamConsumer = StreamingServiceFactory::createConsumer( "Kafka", opts );
//	}
//	catch(const ARQException& e)
//	{
//		std::cout << e.what() << std::endl;
//	}
//
//	streamConsumer->subscribe( { TOPIC } );
//	streamConsumer->seekToBeginning();
//	const auto messagesPtr = streamConsumer->poll( 100ms );
//	const auto& messages = *messagesPtr;
//
//	for( const StreamConsumerMessageView& msg : messages )
//	{
//		std::string headersStr;
//		for( const auto& [key, val] : msg.headers )
//			headersStr += std::format( "{}: {}, ", key, val );
//
//		std::cout << "Message consumed: "
//			<< ", Topic=" << msg.topic
//			<< ", Partition=" << msg.partition
//			<< ", Offset=" << msg.offset
//			<< ", Key=" << msg.key.value_or( "N/A" )
//			<< ", Timestamp=" << msg.timestamp.fmtISO8601()
//			<< ", headers=" << headersStr
//			<< ", Error" << ( msg.error ? msg.error->message : "N/A" );
//	}
//
//	streamConsumer->unsubscribe();
//}

//TEST( Tmp, tmp2 )
//{
//	auto source = RD::SourceFactory::create( "ClickHouseDB" );
//	RD::Record<RD::User> newUser;
//	newUser.header.uuid = ID::UUID::create();
//	newUser.header.isActive = true;
//	newUser.header.lastUpdatedTs = Time::DateTime::nowUTC();
//	newUser.header.lastUpdatedBy = "Calum";
//	newUser.header.version = 0;
//	newUser.data.uuid = newUser.header.uuid;
//	newUser.data.userID = "cwallbridge";
//	newUser.data.fullName = "Calum Wallbridge";
//	newUser.data.email = "calum.wallbridge@example.com";
//	newUser.data.tradingDesk = "DOGE Trading";
//	source->insert<RD::User>( { newUser } );
//	
//
//	//RD::Repository repo( "ClickHouseDB" );
//	//auto userCache = repo.get<RD::User>();
//	//auto userOpt = userCache->getByIndex( "userID", "cwallbridge" ); // TODO: Make this a compile time thing. Maybe the index's can be enum values?
//	//auto userOpt2 = userCache->getByIndex( "userID", "ejames" );
//	//auto list = userCache->getByNonUniqIndex( "tradingDesk", "DOGE Trading" );
//
//	//
//	//std::vector<RD::Record<RD::Currency>> currencies = source->fetch<RD::Currency>();
//}

TEST( Tmp, tmp1 )
{
	RD::Cmd::Upsert<RD::Currency> cmd;
	cmd.expectedVersion = 0;
	cmd.updatedBy = "Evan";
	cmd.targetUUID = /*ID::UUID::fromString( "019bd2b3-0f21-7b24-9dd2-09fca89e52a5" )*/ID::UUID::create();

	cmd.data.uuid = cmd.targetUUID;
	cmd.data.ccyID = "GBP";
	cmd.data.name = "Great British Pound";
	cmd.data.decimalPlaces = 2;
	cmd.data.settlementDays = 2;

	RD::CommandManager cmdMgr;
	cmdMgr.init( RD::CommandManager::Config() );
	cmdMgr.start();

	std::atomic<bool> run = true;

	ID::UUID corrID = cmdMgr.sendCommand( cmd,
		[&run] ( const RD::CommandResponse& resp )
		{
			std::cout << "Command Response received: CorrID=" << resp.corrID
				<< ", Status=" << Enum::enum_name<RD::CommandResponse::Status>( resp.status )
				<< ", Message=" << ( resp.message ? *resp.message : "NULL" )
				<< std::endl;
			run = false;
		},
		Time::Milliseconds( 5000 )
	);

	while( run );
}

//TEST( Tmp, temp1 ) // Temp
//{
//	std::shared_ptr<IStreamProducer> streamProducer;
//	try
//	{
//		streamProducer = StreamingServiceFactory::createProducer( "Kafka", StreamProducerOptions( "Test" ));
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
//			streamProducer->send( msg, [tsStart] ( const StreamProducerMessageMetadata& messageMetadata, std::optional<StreamError> error )
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
//					std::cout << ", Error=" << error->message;
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
