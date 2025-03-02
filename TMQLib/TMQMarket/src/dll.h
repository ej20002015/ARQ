#pragma once

#ifdef _WIN32
	#ifdef TMQMarket_EXPORTS
		#define TMQ_API __declspec( dllexport )
	#else
		#define TMQ_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define TMQ_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define TMQ_API
	#endif
#endif