#include <ARQCore/logger.h>
#include <gtest/gtest.h>

#include <ARQUtils/json.h>
#include <ARQUtils/error.h>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>

#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>

using namespace ARQ;

template<typename Mutex>
class TestSink final : public spdlog::sinks::base_sink<Mutex>
{
public:
    TestSink() = default;

    std::vector<std::string> get_messages()
    {
        std::lock_guard<std::mutex> lock( m_messagesMut );
        return m_messages;
    }

    void clear_messages()
    {
        std::lock_guard<std::mutex> lock( m_messagesMut );
        m_messages.clear();
    }

    size_t message_count()
    {
        std::lock_guard<std::mutex> lock( m_messagesMut );
        return m_messages.size();
    }

protected:
    void sink_it_( const spdlog::details::log_msg& msg ) override
    {
        std::string message_str( msg.payload.data(), msg.payload.size() );
        std::lock_guard<std::mutex> lock( m_messagesMut );
        m_messages.push_back( std::move( message_str ) );
    }

    void flush_() override {}

private:
    std::vector<std::string> m_messages;
    std::mutex m_messagesMut;
};

// Define a type alias using std::mutex for standard multithreaded safety within base_sink
using TestSink_mt = TestSink<std::mutex>;

JSON parseJson( const std::string& json_str )
{
    return JSON::parse( json_str );
}

// --- Test Fixture ---

class LoggerTest : public ::testing::Test
{
protected:
    std::shared_ptr<TestSink_mt> m_testSink;

    void SetUp() override
    {
        // 1. Create the custom sink instance
        m_testSink = std::make_shared<TestSink_mt>();

        // 2. Configure the logger specifically for testing
        LoggerConfig cfg = {
            .loggerName = "LoggerTest",
            .disableConsoleLogger = true,
            .disableFileLogger = true,
            .flushLevel = LogLevel::TRACE,
            .customSinks = { m_testSink },
        };

        ASSERT_NO_THROW( Log::init( cfg ) ) << "Log::init threw an exception during SetUp.";
        Log::setLevel( LogLevel::TRACE );
    }

    void TearDown() override
    {
        ASSERT_NO_THROW( Log::fini() );
    }

    JSON getFirstLogJson( const std::string& context_msg = "" )
    {
        Log::flush();
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        auto messages = m_testSink->get_messages();
        if( messages.empty() )
        {
            ADD_FAILURE() << "No log messages received by test sink. " << context_msg;
            return nullptr;
        }

        return parseJson( messages[0] );
    }

    std::vector<JSON> getAllLogJson( const std::string& context_msg = "" )
    {
        Log::flush();
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        auto messages = m_testSink->get_messages();
        std::vector<JSON> jsonObjects;
        if( messages.empty() )
            ADD_FAILURE() << "No log messages received by test sink. " << context_msg;

        for( std::string& msg : messages )
            jsonObjects.push_back( parseJson( msg ) );

        return jsonObjects;
    }
};

// --- Test Cases ---

TEST_F( LoggerTest, Initialization )
{
    // Verify that init and fini of logger work
    SUCCEED();
}

TEST_F( LoggerTest, BasicLogging )
{
    const std::string testMsg = "This is a basic info message.";
    const std::source_location loc = std::source_location::current();

    Log( Module::CORE ).info( "{0}", testMsg );

    auto logJSON = getFirstLogJson( "BasicLogging" );
    ASSERT_FALSE( logJSON.is_null() );

    EXPECT_EQ( logJSON["level"], "INFO" );
    EXPECT_EQ( logJSON["message"], testMsg );
    EXPECT_EQ( logJSON["module"], "CORE" );
    EXPECT_TRUE( logJSON.contains( "timestamp" ) );
    EXPECT_TRUE( logJSON.contains( "proc_id" ) );
    EXPECT_TRUE( logJSON.contains( "proc_name" ) );
    EXPECT_TRUE( logJSON.contains( "thread_id" ) );
    EXPECT_TRUE( logJSON.contains( "thread_name" ) );
    EXPECT_TRUE( logJSON.contains( "source" ) );
    EXPECT_TRUE( logJSON["source"].is_object() );
    EXPECT_EQ( logJSON["source"]["function"], loc.function_name() );
    EXPECT_FALSE( logJSON.contains( "context" ) );
    EXPECT_FALSE( logJSON.contains( "exception" ) );
}

TEST_F( LoggerTest, LogLevels )
{
    Log( Module::CORE ).trace( "Trace message." ); // Should log
    Log( Module::CORE ).debug( "Debug message." ); // Should log
    Log( Module::CORE ).info( "Info message." );   // Should log

    // Dynamically change level to WARN using the public setLevel method
    ASSERT_NO_THROW( Log::setLevel( LogLevel::WARN ) );

    // Verify level change took effect (using shouldLog)
    EXPECT_FALSE( Log::shouldLog( LogLevel::INFO ) );
    EXPECT_TRUE( Log::shouldLog( LogLevel::WARN ) );

    Log( Module::CORE ).info( "Info message 2." );       // Should NOT log
    Log( Module::CORE ).warn( "Warn message." );         // Should log
    Log( Module::CORE ).error( "Error message." );       // Should log
    Log( Module::CORE ).critical( "Critical message." ); // Should log

    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Async logger so wait here

    auto jsonLogs = getAllLogJson( "LogLevels" );
    // Expected: trace, debug, info1, warn, error, critical
    ASSERT_EQ( jsonLogs.size(), 6 );

    EXPECT_EQ( jsonLogs[0]["level"], "TRACE" );
    EXPECT_EQ( jsonLogs[1]["level"], "DEBUG" );
    EXPECT_EQ( jsonLogs[2]["level"], "INFO" );
    EXPECT_EQ( jsonLogs[3]["level"], "WARN" );
    EXPECT_EQ( jsonLogs[4]["level"], "ERRO" );
    EXPECT_EQ( jsonLogs[5]["level"], "CRITICAL" );
}

TEST_F( LoggerTest, PerCallContext )
{
    // Assuming JSON is nlohmann::json alias from ARQUtils/json.h
    JSON context = { { "key1", "value1" }, { "count", 123 }, { "active", true } };
    Log( Module::REFDATA, context ).info( "Message with local context." );

    auto logJSON = getFirstLogJson( "PerCallContext" );
    ASSERT_FALSE( logJSON.is_null() );
    ASSERT_TRUE( logJSON.contains( "context" ) ) << "Log entry missing 'context' field.";
    ASSERT_TRUE( logJSON["context"].is_object() );

    // Check specific context values
    EXPECT_EQ( logJSON["context"]["key1"], "value1" );
    EXPECT_EQ( logJSON["context"]["count"], 123 );
    EXPECT_EQ( logJSON["context"]["active"], true );
    EXPECT_FALSE( logJSON["context"].contains( "other_key" ) );
}

TEST_F( LoggerTest, ThreadContextScoped )
{
    const std::string threadKey = "thread_key";
    const std::string threadVal = "thread_value_123";
    const std::string outerMsg = "Message outside thread scope.";
    const std::string innerMsg = "Message within thread scope.";

    { // Start scope for RAII guard
        Log::Context::Thread::Scoped ctxGuard( { { threadKey, threadVal } } );

        // Log inside the scope
        Log( Module::CORE ).debug( "{0}", innerMsg );

        auto logJSONInner = getFirstLogJson( "ThreadContextScoped Inner" );
        ASSERT_FALSE( logJSONInner.is_null() );
        ASSERT_TRUE( logJSONInner.contains( "context" ) );
        EXPECT_EQ( logJSONInner["context"][threadKey], threadVal );
        EXPECT_EQ( logJSONInner["message"], innerMsg );

        m_testSink->clear_messages(); // Clear sink before logging outside scope
    } // ~Scoped() runs here, should remove the context

    // Log outside the scope
    Log( Module::CORE ).debug( "{0}", outerMsg );
    auto logJSONOuter = getFirstLogJson( "ThreadContextScoped Outer" );
    ASSERT_FALSE( logJSONOuter.is_null() );
    EXPECT_EQ( logJSONOuter["message"], outerMsg );

    // Context should not contain the key set inside the scope
    if( logJSONOuter.contains( "context" ) )
    {
        EXPECT_FALSE( logJSONOuter["context"].contains( threadKey ) )
            << "Thread context key '" << threadKey << "' still present outside scope.";
    }
}


TEST_F( LoggerTest, GlobalContextScoped )
{
    const std::string globalKey = "global_process_id";
    const int globalVal = 9876;
    const std::string outerMsg = "Message outside global scope.";
    const std::string innterMsg = "Message within global scope.";

    { // Start scope for RAII guard
        Log::Context::Global::Scoped ctxGuard( { { globalKey, globalVal } } );

        // Log inside the scope
        Log( Module::CORE ).warn( "{0}", innterMsg );

        auto logJSONInner = getFirstLogJson( "GlobalContextScoped Inner" );
        ASSERT_FALSE( logJSONInner.is_null() );
        ASSERT_TRUE( logJSONInner.contains( "context" ) );
        EXPECT_EQ( logJSONInner["context"][globalKey], globalVal );
        EXPECT_EQ( logJSONInner["message"], innterMsg );

        m_testSink->clear_messages();
    } // ~Scoped() runs here, should restore previous global state for this key

    // Log outside the scope
    Log( Module::CORE ).warn( "{0}", outerMsg );
    auto logJSONOuter = getFirstLogJson( "GlobalContextScoped Outer" );
    ASSERT_FALSE( logJSONOuter.is_null() );
    EXPECT_EQ( logJSONOuter["message"], outerMsg );

    // Context should not contain the key set inside the scope (or it should be restored)
    if( logJSONOuter.contains( "context" ) )
    {
        EXPECT_FALSE( logJSONOuter["context"].contains( globalKey ) )
            << "Global context key '" << globalKey << "' still present outside scope.";
    }
}

TEST_F( LoggerTest, ContextPrecedence )
{
    const std::string key = "shared_key";

    // Set global, then thread, then provide local context
    Log::Context::Global::Scoped g_guard( { { key, "GLOBAL_VAL" } } );
    Log::Context::Thread::Scoped t_guard( { { key, "THREAD_VAL" } } );
    JSON local_ctx = { {key, "LOCAL_VAL"} };

    Log( Module::CORE, local_ctx ).info( "Testing context precedence." );

    auto logJSON = getFirstLogJson( "ContextPrecedence" );
    ASSERT_FALSE( logJSON.is_null() );
    ASSERT_TRUE( logJSON.contains( "context" ) );

    // Expect the LOCAL value to win
    EXPECT_EQ( logJSON["context"][key], "LOCAL_VAL" );

    m_testSink->clear_messages();

    Log( Module::CORE ).info( "Testing context precedence without local." );

    auto logJSON2 = getFirstLogJson( "ContextPrecedence without local" );
    ASSERT_FALSE( logJSON2.is_null() );
    ASSERT_TRUE( logJSON2.contains( "context" ) );

    // Expect the THREAD value to win
    EXPECT_EQ( logJSON2["context"][key], "THREAD_VAL" );
}

TEST_F( LoggerTest, ExceptionLogging )
{
    const std::string exceptionWhat = "Database connection failed.";
    const std::string logMsg = "Failed during user lookup.";
    const auto exceptionLoc = std::source_location::current();
    ARQException testException( exceptionWhat, exceptionLoc ); // Assuming constructor

    // Log the exception using the dedicated overload
    Log( Module::CORE ).error( testException, "{0}", logMsg );

    auto logJSON = getFirstLogJson( "ExceptionLogging" );
    ASSERT_FALSE( logJSON.is_null() );

    // Check standard fields
    EXPECT_EQ( logJSON["level"], "ERRO" );
    EXPECT_EQ( logJSON["message"], logMsg );

    // Check exception field
    ASSERT_TRUE( logJSON.contains( "exception" ) ) << "Log entry missing 'exception' field.";
    ASSERT_TRUE( logJSON["exception"].is_object() );

    // Check exception 'what'
    EXPECT_EQ( logJSON["exception"]["what"], exceptionWhat );

    // Check exception 'where' structure
    ASSERT_TRUE( logJSON["exception"].contains( "where" ) ) << "Exception details missing 'where' field.";
    ASSERT_TRUE( logJSON["exception"]["where"].is_object() );
    EXPECT_TRUE( logJSON["exception"]["where"].contains( "file" ) );
    EXPECT_TRUE( logJSON["exception"]["where"].contains( "line" ) );
    EXPECT_TRUE( logJSON["exception"]["where"].contains( "function" ) );

    // Verify the source location matches where the exception was created
    EXPECT_EQ( logJSON["exception"]["where"]["file"], exceptionLoc.file_name() );
    EXPECT_EQ( logJSON["exception"]["where"]["line"], exceptionLoc.line() );
    EXPECT_EQ( logJSON["exception"]["where"]["function"], exceptionLoc.function_name() );
}

// --- Optional Multi-threaded Test ---
// This test is basic and mainly checks for deadlocks or crashes during concurrent logging
// and context manipulation, rather than strict output verification.
TEST_F( LoggerTest, MultiThreadedBasic )
{
    const int numThreads = 4;
    const int logsPerThread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> errCount = 0;

    for( int i = 0; i < numThreads; ++i )
    {
        threads.emplace_back( [i, logsPerThread, &errCount] ()
        {
            try
            {
                // Use scoped thread context
                Log::Context::Thread::Scoped t_guard( { { "thread_idx", i } } );

                for( int j = 0; j < logsPerThread; ++j )
                {
                    // Mix log levels and context types
                    if( j % 3 == 0 )
                    {
                        JSON localCtx = { { "loop_idx", j }, { "rand_val", rand() % 100 } };
                        Log( Module::CORE, localCtx ).info( "MT Log iter {} from thread {}", j, i );
                    }
                    else if( j % 3 == 1 )
                        Log( Module::REFDATA ).debug( "MT Debug iter {} from thread {}", j, i );
                    else
                        Log( Module::CORE ).warn( "MT Warn iter {} from thread {}", j, i );
                    
                    // Small sleep to allow interleaving, but not too long
                    if( j % 10 == 0 ) std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
            }
            catch( const std::exception& e )
            {
                std::cerr << "!!! Exception in test thread " << i << ": " << e.what() << std::endl;
                errCount++;
            }
            catch( ... )
            {
                std::cerr << "!!! Unknown exception in test thread " << i << std::endl;
                errCount++;
            }
        } );
    }

    // Wait for all threads to complete
    for( auto& t : threads )
    {
        if( t.joinable() )
            t.join();
    }

    // Basic checks after threads finish
    EXPECT_EQ( errCount.load(), 0 ) << "Exceptions occurred in logging threads.";

    // Check total number of messages logged
    Log::flush();
    size_t final_count = m_testSink->message_count();
    EXPECT_EQ( final_count, numThreads * logsPerThread )
        << "Incorrect total number of log messages received.";

    // Spot check a few messages for content (harder due to non-determinism)
    // Check if at least one message contains context from thread 0
    auto allLogs = m_testSink->get_messages();
    bool foundThread0Ctx = false;
    for( const auto& msg_str : allLogs )
    {
        auto json_obj = parseJson( msg_str );
        if( !json_obj.is_null() && json_obj.contains( "context" ) && json_obj["context"].contains( "thread_idx" ) )
        {
            if( json_obj["context"]["thread_idx"].get<int>() == 0 )
            {
                foundThread0Ctx = true;
                break;
            }
        }
    }
    EXPECT_TRUE( foundThread0Ctx ) << "Did not find context from thread 0 in logs.";
}

TEST_F( LoggerTest, PerfTest )
{
    auto tmStart = std::chrono::system_clock::now();
    for( uint32_t i = 0; i < 1000; i++ )
    {
        Log( Module::CORE ).info( "Hello this is a test: num {0}", i );
    }
    auto tmEnd = std::chrono::system_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>( tmEnd - tmStart );
    std::cout << dur.count() << "microseconds" << std::endl;
}