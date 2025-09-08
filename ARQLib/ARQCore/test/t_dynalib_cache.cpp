#include <ARQCore/dynalib_cache.h>
#include <gtest/gtest.h>

using namespace ARQ;

TEST( DynaLibCacheTests, LoadARQClickhouse )
{
    EXPECT_NO_THROW( const auto& t = DynaLibCache::inst().get( "ARQClickHouse" ) );
}