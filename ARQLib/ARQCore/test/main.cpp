#include <ARQCore/lib.h>
#include <t_ARQ/core.h>

#include <gtest/gtest.h>

int main( int argc, char** argv )
{
	ARQ::LibGuard guard( ARQ::getLibArgs( argc, argv, "t_ARQCore" ) );
	testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}