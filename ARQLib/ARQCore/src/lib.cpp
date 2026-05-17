#include <ARQCore/lib.h>

#include <ARQUtils/os.h>
#include <ARQCore/dynalib_cache.h>

#include <thread>

namespace ARQ
{

// Static variables normally initialised by main thread
const std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();

static void addLibOptions( Cfg::IConfigWrangler& cfgWrangler, LibContext::Config& cfg )
{
	cfgWrangler.add(     cfg.env,       "--env,-E",       "Set the ARQ environment",                                                                   "ARQ_env"        );
	cfgWrangler.addEnum( cfg.logLevel,  "--log.level,-L", "Set the log level of the primary sink",                                                     "ARQ_log_level"  );
	cfgWrangler.addEnum( cfg.logLevel2, "--log.level2",   "Set the log level of the secondary sink",                                                   "ARQ_log_level2" );
	cfgWrangler.add(     cfg.logDest,   "--log.dest,-D",  "Set the log destination of the primary sink ('stdout', 'stderr', <filepath>, or 'none')",   "ARQ_log_dest"   );
	cfgWrangler.add(     cfg.logDest2,  "--log.dest2",    "Set the log destination of the secondary sink ('stdout', 'stderr', <filepath>, or 'none')", "ARQ_log_dest2"  );
}

void libInit( int argc, char* argv[], Cfg::IConfigWrangler& cfgWrangler )
{
	if( LibContext::s_inst )
		return;

	LibContext::Config cfg;

	addLibOptions( cfgWrangler, cfg );

	if( !cfgWrangler.parse( argc, argv ) )
		std::exit( cfgWrangler.printExitMsgAndGetRC() );

	LibContext::s_inst = new LibContext( cfg );
}

void libInit( int argc, char* argv[] )
{
	Cfg::ConfigWrangler cfgWrangler( "ARQ Library Initialisation", "ARQLib" );
	cfgWrangler.allowExtras();
	libInit( argc, argv, cfgWrangler );
}

void libInit( const std::vector<std::string>& args )
{
	std::vector<char*> argv;
	argv.reserve( args.size() );
	for( const auto& arg : args )
		argv.push_back( const_cast<char*>( arg.c_str() ) );
	libInit( static_cast<int>( argv.size() ), argv.data() );
}

void libShutdown()
{
	if( !LibContext::s_inst )
		return;

	delete LibContext::s_inst;
	LibContext::s_inst = nullptr;
}

LibContext* LibContext::s_inst = nullptr;

LibContext& LibContext::Inst()
{
	ARQ_ASSERT( s_inst );
	if( !s_inst )
		throw ARQException( "LibContext instance not initialized - call LibInit() first" );

	return *s_inst;
}

LibContext::LibContext( const Config& cfg )
	: m_config( cfg )
{
	if( std::this_thread::get_id() == MAIN_THREAD_ID )
		OS::setThreadName( "Main" );

	LoggerConfig logCfg{
		.primarySinkLogLevel   = m_config.logLevel,
		.secondarySinkLogLevel = m_config.logLevel2,
		.primarySinkDest       = m_config.logDest,
		.secondarySinkDest     = m_config.logDest2
	};

	// Global objects

	m_logger = std::make_unique<Logger>( logCfg );
	Logger::setGlobalInst( m_logger.get() );

	m_dynaLibCache = std::make_unique<DynaLibCache>();
	DynaLibCache::setGlobalInst( m_dynaLibCache.get() );

	// Global factories

	m_rdSourceFactory = std::make_unique<RD::SourceFactory>();
	RD::SourceFactory::setGlobalInst( m_rdSourceFactory.get() );
	m_serialiserFactory = std::make_unique<SerialiserFactory>();
	SerialiserFactory::setGlobalInst( m_serialiserFactory.get() );
	m_messagingServiceFactory = std::make_unique<MessagingServiceFactory>();
	MessagingServiceFactory::setGlobalInst( m_messagingServiceFactory.get() );
	m_streamingServiceFactory = std::make_unique<StreamingServiceFactory>();
	StreamingServiceFactory::setGlobalInst( m_streamingServiceFactory.get() );
	m_streamOffsetSourceFactory = std::make_unique<StreamOffsetSourceFactory>();
	StreamOffsetSourceFactory::setGlobalInst( m_streamOffsetSourceFactory.get() );
	m_mdMarketSourceFactory = std::make_unique<MD::MarketSourceFactory>();
	MD::MarketSourceFactory::setGlobalInst( m_mdMarketSourceFactory.get() );

	Log( Module::CORE ).info( "ARQLib initialised" );
}

LibContext::~LibContext()
{
	// Global factories

	MD::MarketSourceFactory::setGlobalInst( nullptr );
	m_mdMarketSourceFactory.reset();
	StreamOffsetSourceFactory::setGlobalInst( nullptr );
	m_streamOffsetSourceFactory.reset();
	StreamingServiceFactory::setGlobalInst( nullptr );
	m_streamingServiceFactory.reset();
	MessagingServiceFactory::setGlobalInst( nullptr );
	m_messagingServiceFactory.reset();
	SerialiserFactory::setGlobalInst( nullptr );
	m_serialiserFactory.reset();
	RD::SourceFactory::setGlobalInst( nullptr );
	m_rdSourceFactory.reset();

	// Other global objects

	DynaLibCache::setGlobalInst( nullptr );
	m_dynaLibCache.reset();

	Logger::setGlobalInst( nullptr );
	m_logger.reset();
}


}
