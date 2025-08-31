#pragma once

#ifdef _WIN32
	#ifdef ARQUtils_EXPORTS
		#define ARQUtils_API __declspec( dllexport )
	#else
		#define ARQUtils_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQUtils_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQUtils_API
	#endif
#endif