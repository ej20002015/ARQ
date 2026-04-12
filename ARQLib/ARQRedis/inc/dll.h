#pragma once

#ifdef _WIN32
	#ifdef ARQRedis_EXPORTS
		#define ARQRedis_API __declspec( dllexport )
	#else
		#define ARQRedis_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQRedis_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQRedis_API
	#endif
#endif