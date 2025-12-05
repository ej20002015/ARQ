#include <ARQUtils/error.h>
#include <ARQUtils/enum.h>
#include <ARQUtils/time.h>
#include <ARQCore/messaging_service.h>

#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <atomic>
#include <thread>

using namespace ARQ;

// -----------------------------------------------------------------------------
// Concrete Handler Implementation
// -----------------------------------------------------------------------------
class ConsoleListener : public ISubscriptionHandler
{
public:
    void onMsg( Message&& msg ) override
    {
        std::string payloadStr = msg.data.toString();

        std::cout << "\n[MSG Received] Topic: " << msg.topic << "\n";
        std::cout << "  > Payload: " << payloadStr << "\n";

        if( !msg.headers.empty() )
        {
            std::cout << "  > Headers:\n";
            for( const auto& [key, values] : msg.headers )
            {
                std::cout << "    - " << key << ": ";
                for( const auto& val : values )
                    std::cout << val << " ";
                std::cout << "\n";
            }
        }
    }

    [[nodiscard]] std::string_view getDesc() const override
    {
        return "ConsoleListener";
    }

    [[nodiscard]] SubscriptionOptions getSubOptions() override
    {
        // We return NONE (0) because DEFAULT includes DISABLE_HEADERS.
        // We want to see the headers sent by the publisher.
        return SubscriptionOptions::NONE;
    }
};

// -----------------------------------------------------------------------------
// Main Application
// -----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
    if( argc < 3 )
    {
        std::cerr << "Usage: " << argv[0] << " <DSH> <TOPIC_PATTERN>" << std::endl;
        std::cerr << "Example: " << argv[0] << " NATS quotes.>" << std::endl;
        return 1;
    }

    std::string dsh = argv[1];
    std::string topicPattern = argv[2];

    std::cout << "Initializing NATS Subscriber..." << std::endl;
    std::cout << "DSH: " << dsh << " | Topic pattern: " << topicPattern << std::endl;

    std::shared_ptr<IMessagingService> msgServ;
    ARQ_DO_IN_TRY( arqExc, errMsg );
        msgServ = MessagingServiceFactory::create( dsh );
    ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

    if( arqExc.what().size() )
    {
        std::cout << "Error creating nats messaging service - what: " << arqExc.what() << std::endl;
        return 1;
    }
    else if( errMsg.size() )
    {
        std::cout << "Error creating nats messaging service - what: " << errMsg << std::endl;
        return 1;
    }

    // Register Event Callback
    msgServ->registerEventCallback( [&] ( const MessagingEvent event, const std::optional<int64_t> subID )
    {
        std::cout << "[NATS Event]: " << Enum::enum_name( event );
        if( subID.has_value() )
            std::cout << " (SubID: " << subID.value() << ")";
        std::cout << std::endl;
    } );

    // Create the handler
    auto handler = std::make_shared<ConsoleListener>();
    std::unique_ptr<ISubscription> subscription;

    // Perform Subscription
    ARQ_DO_IN_TRY( arqExc1, errMsg1 );
    subscription = msgServ->subscribe( topicPattern, handler );
    std::cout << "Subscribed successfully. Subscription ID: " << subscription->getID() << std::endl;
    ARQ_END_TRY_AND_CATCH( arqExc1, errMsg1 );

    if( arqExc1.what().size() )
        std::cout << "Error subscribing - what: " << arqExc1.what() << std::endl;
    else if( errMsg1.size() )
        std::cout << "Error subscribing - what: " << errMsg1 << std::endl;

    // Keep Alive Loop
    std::cout << "\n--- Subscriber Ready ---\n";
    std::cout << "Press ENTER to quit.\n";

    // Simple blocking read to keep the main thread alive
    std::cin.get();

    std::cout << "Unsubscribing and shutting down..." << std::endl;

    // Explicitly unsubscribe, though destruction of 'subscription' ptr usually handles this
    if( subscription )
        subscription->drainAndUnsubscribe();

    return 0;
}