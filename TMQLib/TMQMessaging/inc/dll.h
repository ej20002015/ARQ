#pragma once

#ifdef _WIN32
	#ifdef TMQMessaging_EXPORTS
		#define TMQMessaging_API __declspec( dllexport )
	#else
		#define TMQMessaging_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define TMQMessaging_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define TMQMessaging_API
	#endif
#endif