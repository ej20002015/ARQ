#pragma once

#ifdef _WIN32
	#ifdef TMQSerialisation_EXPORTS
		#define TMQSerialisation __declspec( dllexport )
	#else
		#define TMQSerialisation __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define TMQSerialisation __attribute__( ( visibility( "default" ) ) )
	#else
		#define TMQSerialisation
	#endif
#endif