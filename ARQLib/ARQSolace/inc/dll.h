#pragma once

#ifdef _WIN32
	#ifdef ARQSolace_EXPORTS
		#define ARQSolace_API __declspec( dllexport )
	#else
		#define ARQSolace_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQSolace_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQSolace_API
	#endif
#endif