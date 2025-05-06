#pragma once

#ifdef _WIN32
	#ifdef TMQSerialisation_EXPORTS
		#define TMQSerialisation_API __declspec( dllexport )
	#else
		#define TMQSerialisation_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define TMQSerialisation_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define TMQSerialisation_API
	#endif
#endif