#include <ARQUtils/error.h>
#include <ARQUtils/enum.h>
#include <ARQUtils/time.h>
#include <ARQCore/messaging_service.h>

#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <atomic>

using namespace ARQ;

int main( int argc, char* argv[] )
{
    if( argc < 3 )
    {
        std::cerr << "Usage: " << argv[0] << " <DSH> <TOPIC>" << std::endl;
        std::cerr << "Example: " << argv[0] << " NATS quotes.us" << std::endl;
        return 1;
    }

    std::string dshId = argv[1];
    std::string targetTopic = argv[2];

    std::cout << "Initializing NATS Publisher..." << std::endl;
    std::cout << "DSH: " << dshId << " | Topic: " << targetTopic << std::endl;
    std::shared_ptr<IMessagingService> msgServ;
    ARQ_DO_IN_TRY( arqExc, errMsg );
        msgServ = MessagingServiceFactory::create( "NATS" );
    ARQ_END_TRY_AND_CATCH( arqExc, errMsg );
    if( arqExc.what().size() )
        std::cout << "Error creating nats messaging service - what: " << arqExc.what();
    else if( errMsg.size() )
        std::cout << "Error creating nats messaging service - what: " << errMsg;

    // Register Event Callback
    // This is crucial to know if we are actually connected before publishing
    std::atomic<bool> isConnected = false;

    msgServ->registerEventCallback( [&] ( const MessagingEvent event, const std::optional<int64_t> subID )
    {
        std::cout << "[NATS Event]: " << Enum::enum_name( event );
        if( subID.has_value() )
            std::cout << " (SubID: " << subID.value() << ")";
        std::cout << std::endl;

        if( event == MessagingEvent::CONN_RECONNECTED )
            isConnected = true;
        else if( event == MessagingEvent::CONN_DISCONNECTED || event == MessagingEvent::CONN_CLOSED )
            isConnected = false;
    } );

    // Publish Loop
    std::cout << "\n--- Publisher Ready ---\n";
    std::cout << "Type a message and press ENTER to publish.\n";
    std::cout << "Type 'exit' to quit.\n";

    std::string userInput;
    while( true )
    {
        std::cout << "> ";
        std::getline( std::cin, userInput );

        if( userInput == "exit" )
            break;
        if( userInput.empty() )
            continue;

        Buffer payload( reinterpret_cast<const uint8_t*>( userInput.c_str() ), userInput.size() + 1 );

        // Construct the Message
        Message msg;
        msg.topic = targetTopic;
        msg.data = std::move( payload );

        // Add Headers
        msg.headers["Sender-DSH"].push_back( dshId );
        msg.headers["Timestamp"].push_back( Time::DateTime::nowUTC().fmtISO8601() );

        ARQ_DO_IN_TRY( arqExc, errMsg );
            msgServ->publish( targetTopic, msg );
            std::cout << " [Sent]" << std::endl;
        ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

        if( arqExc.what().size() )
            std::cout << "Error publishing - what: " << arqExc.what();
        else if( errMsg.size() )
            std::cout << "Error publishing - what: " << errMsg;
    }

    std::cout << "Shutting down..." << std::endl;

    return 0;
}
