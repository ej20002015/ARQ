#include <ARQCore/dynalib_cache.h>
#include <gtest/gtest.h>

using namespace ARQ;

TEST( DynaLibCacheTests, LoadARQClickhouse )
{
    EXPECT_NO_THROW( auto t = DynaLibCache::inst().get( "ARQClickhouse" ) );
}