#pragma once

#ifdef _WIN32
	#ifdef TMQUtils_EXPORTS
		#define TMQUtils_API __declspec( dllexport )
	#else
		#define TMQUtils_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define TMQUtils_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define TMQUtils_API
	#endif
#endif