#pragma once

#ifdef _WIN32
	#ifdef ARQClickHouse_EXPORTS
		#define ARQClickHouse_API __declspec( dllexport )
	#else
		#define ARQClickHouse_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQClickHouse_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQClickHouse_API
	#endif
#endif