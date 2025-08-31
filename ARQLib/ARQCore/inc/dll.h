#pragma once

#ifdef _WIN32
	#ifdef ARQCore_EXPORTS
		#define ARQCore_API __declspec( dllexport )
	#else
		#define ARQCore_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQCore_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQCore_API
	#endif
#endif