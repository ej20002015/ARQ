#pragma once

#ifdef _WIN32
	#ifdef ARQMarket_EXPORTS
		#define ARQMarket_API __declspec( dllexport )
	#else
		#define ARQMarket_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQMarket_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQMarket_API
	#endif
#endif