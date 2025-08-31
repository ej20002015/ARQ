#pragma once

#ifdef _WIN32
	#ifdef ARQMessaging_EXPORTS
		#define ARQMessaging_API __declspec( dllexport )
	#else
		#define ARQMessaging_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQMessaging_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQMessaging_API
	#endif
#endif