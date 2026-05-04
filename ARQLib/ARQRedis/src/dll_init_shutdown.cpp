#include <ARQRedis/dll.h>

#include "redis_connection.h"

namespace ARQ
{

extern "C" ARQRedis_API void arqDynaLibShutdown()
{
	RedisConnPool::inst().closeAll();
}

}