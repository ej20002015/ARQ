#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/sys.h>
#include <ARQUtils/logger.h>
#include <ARQUtils/cfg_wrangler.h>
#include <ARQCore/dynalib_cache.h>
#include <ARQCore/stream_offset_source.h>
#include <ARQCore/messaging_service.h>
#include <ARQCore/mktdata_source.h>
#include <ARQCore/refdata_source.h>
#include <ARQCore/serialiser.h>
#include <ARQCore/streaming_service.h>

#include <vector>
#include <string>

namespace ARQ
{

ARQCore_API void libInit( int argc, char* argv[], Cfg::IConfigWrangler& cfgWrangler );
ARQCore_API void libInit( int argc, char* argv[] );
ARQCore_API void libInit( const std::vector<std::string>& args );
ARQCore_API void libShutdown();

class LibGuard
{
public:
	LibGuard( int argc, char* argv[], Cfg::IConfigWrangler& cfgWrangler ) { libInit( argc, argv, cfgWrangler ); }
	LibGuard( int argc, char* argv[] )                                    { libInit( argc, argv ); }
	LibGuard( const std::vector<std::string>& args )                      { libInit( args ); }

	~LibGuard() { libShutdown(); }

	LibGuard( const LibGuard& )            = delete;
	LibGuard( LibGuard&& )                 = delete;
	LibGuard& operator=( const LibGuard& ) = delete;
	LibGuard& operator=( LibGuard&& )      = delete;
};

class LibContext
{
public:
	struct Config
	{
		std::string env       = "DEFAULT";
		LogLevel    logLevel  = DEFAULT_LOG_LEVEL;
		LogLevel    logLevel2 = DEFAULT_LOG_LEVEL;
		std::string logDest   = "stdout";
		std::string logDest2  = ( Sys::logDir() / "ARQLib.log" ).string();
	};

public:
	ARQCore_API static LibContext& Inst();

public:
	~LibContext();

	LibContext( const LibContext& )            = delete;
	LibContext( LibContext&& )                 = delete;
	LibContext& operator=( const LibContext& ) = delete;
	LibContext& operator=( LibContext&& )      = delete;

	const Config&      config() const { return m_config; }
	const std::string& env()    const { return m_config.env; }

private:
	ARQCore_API LibContext( const Config& cfg );

private:
	Config m_config;

private:
	static LibContext* s_inst;

	ARQCore_API friend void libInit( int argc, char* argv[], Cfg::IConfigWrangler& cfgWrangler );
	ARQCore_API friend void libShutdown();

private:
	// Global objects that are initialized in LibContext constructor and destroyed in LibContext destructor

	std::unique_ptr<RD::SourceFactory>         m_rdSourceFactory;
	std::unique_ptr<SerialiserFactory>         m_serialiserFactory;
	std::unique_ptr<MessagingServiceFactory>   m_messagingServiceFactory;
	std::unique_ptr<StreamingServiceFactory>   m_streamingServiceFactory;
	std::unique_ptr<StreamOffsetSourceFactory> m_streamOffsetSourceFactory;
	std::unique_ptr<MktDataSourceFactory>      m_mktDataSourceFactory;

	std::unique_ptr<DynaLibCache> m_dynaLibCache;
	std::unique_ptr<Logger> m_logger;
};

}