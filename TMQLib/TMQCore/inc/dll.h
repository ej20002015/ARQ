#pragma once

#ifdef _WIN32
	#ifdef TMQCore_EXPORTS
		#define TMQCore_API __declspec( dllexport )
	#else
		#define TMQCore_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define TMQCore_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define TMQCore_API
	#endif
#endif