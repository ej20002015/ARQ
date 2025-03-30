#pragma once

#ifdef _WIN32
	#ifdef TMQMarket_EXPORTS
		#define TMQMarket_API __declspec( dllexport )
	#else
		#define TMQMarket_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define TMQMarket_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define TMQMarket_API
	#endif
#endif