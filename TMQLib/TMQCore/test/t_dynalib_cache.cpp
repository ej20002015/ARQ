#include <TMQCore/dynalib_cache.h>
#include <gtest/gtest.h>

using namespace TMQ;

TEST( DynaLibCacheTests, LoadTMQClickhouse )
{
    EXPECT_NO_THROW( auto t = DynaLibCache::inst().get( "TMQClickhouse" ) );
}