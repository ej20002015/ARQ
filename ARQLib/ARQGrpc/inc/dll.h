#pragma once

#ifdef _WIN32
	#ifdef ARQGrpc_EXPORTS
		#define ARQGrpc_API __declspec( dllexport )
	#else
		#define ARQGrpc_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQGrpc_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQGrpc_API
	#endif
#endif