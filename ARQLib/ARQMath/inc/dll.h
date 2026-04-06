#pragma once

#ifdef _WIN32
	#ifdef ARQMath_EXPORTS
		#define ARQMath_API __declspec( dllexport )
	#else
		#define ARQMath_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQMath_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQMath_API
	#endif
#endif