#pragma once

#ifdef _WIN32
	#ifdef TMQSolace_EXPORTS
		#define TMQSolace_API __declspec( dllexport )
	#else
		#define TMQSolace_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define TMQSolace_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define TMQSolace_API
	#endif
#endif