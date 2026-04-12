#include <gtest/gtest.h>

#include <ARQUtils/logger.h>

int main( int argc, char** argv )
{
	// since we don't init ARQ lib for utils tests, we need to create our own global logger
	auto globalLogger = std::make_unique<ARQ::Logger>();
	ARQ::Logger::setGlobalInst( globalLogger.get() );

	testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}