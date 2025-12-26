#pragma once

#ifdef _WIN32
#ifdef ARQKafka_EXPORTS
#define ARQKafka_API __declspec( dllexport )
#else
#define ARQKafka_API __declspec( dllimport )
#endif
#else
#ifdef __GNUC__
#define ARQKafka_API __attribute__( ( visibility( "default" ) ) )
#else
#define ARQKafka_API
#endif
#endif