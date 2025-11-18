#pragma once

#ifdef _WIN32
	#ifdef ARQNats_EXPORTS
		#define ARQNats_API __declspec( dllexport )
	#else
		#define ARQNats_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQNats_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQNats_API
	#endif
#endif