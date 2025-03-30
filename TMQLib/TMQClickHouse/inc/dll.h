#pragma once

#ifdef _WIN32
	#ifdef TMQClickHouse_EXPORTS
		#define TMQClickHouse_API __declspec( dllexport )
	#else
		#define TMQClickHouse_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define TMQClickHouse_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define TMQClickHouse_API
	#endif
#endif