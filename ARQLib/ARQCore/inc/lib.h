#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/sys.h>
#include <ARQUtils/logger.h>
#include <ARQUtils/cfg_wrangler.h>

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
	// Library wide objects go here
	std::unique_ptr<Logger> m_logger;
};

}