#pragma once

#ifdef _WIN32
	#ifdef ARQSerialisation_EXPORTS
		#define ARQSerialisation_API __declspec( dllexport )
	#else
		#define ARQSerialisation_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQSerialisation_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQSerialisation_API
	#endif
#endif