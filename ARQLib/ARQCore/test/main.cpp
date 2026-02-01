#include <ARQCore/lib.h>
#include <t_ARQ/core.h>

#include <gtest/gtest.h>

int main( int argc, char** argv )
{
	ARQ::LibGuard guard( { argv[0], "--log.dest2", ARQ::getLogFilepathForTests( "t_ARQCore" ).string() } );
	testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}