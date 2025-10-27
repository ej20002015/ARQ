#include <ARQCore/dynalib_cache.h>
#include <gtest/gtest.h>

#include <iostream>

using namespace ARQ;

TEST( DynaLibCacheTests, LoadARQClickHouse )
{
    EXPECT_NO_THROW( const auto& t = DynaLibCache::inst().get( "ARQClickHouse" ) );
}
