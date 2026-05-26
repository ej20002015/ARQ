#pragma once

#ifdef _WIN32
	#ifdef ARQAdapt_EXPORTS
		#define ARQAdapt_API __declspec( dllexport )
	#else
		#define ARQAdapt_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQAdapt_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQAdapt_API
	#endif
#endif