#pragma once

#ifdef _WIN32
	#ifdef ARQProtobuf_EXPORTS
		#define ARQProtobuf_API __declspec( dllexport )
	#else
		#define ARQProtobuf_API __declspec( dllimport )
	#endif
#else
	#ifdef __GNUC__
		#define ARQProtobuf_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define ARQProtobuf_API
	#endif
#endif