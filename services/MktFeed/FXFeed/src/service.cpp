#include "service.h"

#include "eod_file_parser.h"

static void onPublishError( const StreamProducerMessageMetadata& messageMetadata, StreamError error )
{
	Log( Module::EXE ).error( "Error publishing message to topic [{}]: {}", messageMetadata.topic, error.message );
}

void FXFeedService::onStartup()
{
	MD::Publisher::Config pubConfig;
	pubConfig.streamingServiceDSH = m_config.streamSvcDSH;
	m_publisher = std::make_unique<MD::Publisher>();
	m_publisher->init( pubConfig );
	m_publisher->registerOnErrorCallback( onPublishError );

	const std::filesystem::path eodFileDir = ARQ::Sys::svcCfgDir() / "MktFeed" / "FXFeed";
	const int64_t currDayIdx = Time::DateTime::nowUTC().date() - Time::DateTime::Min().date();
    for( const auto& ccy : m_config.publishCcys )
    {
		const auto eodRatesMap = EODParser::load( eodFileDir / std::format( "EOD_{}.csv", ccy ) );

        const auto onPeriodRollover = [eodRatesMap, ccy] ( const int64_t idx )
        {
			Log( Module::EXE ).info( "Period rollover for {}: day index {}", ccy, idx );

            // Default fallback values just in case we request a date off the edge of the map
            double start = 1.0;
            double end = 1.0;
            double vol = 0.002;

            if( eodRatesMap.contains( idx ) )
            {
                start = eodRatesMap.at( idx ).rate;
                vol = eodRatesMap.at( idx ).volatility;
            }
            if( eodRatesMap.contains( idx + 1 ) )
                end = eodRatesMap.at( idx + 1 ).rate;
            else // If we don't have tomorrow's data, assume flat market for the day
                end = start;

            // Hash the currency string and combine it with the day index to get the seed
            std::size_t ccyHash = std::hash<std::string>{}( ccy );
            uint64_t uniqueSeed = static_cast<uint64_t>( idx ) ^ ( static_cast<uint64_t>( ccyHash ) << 1 );

            return Math::Stochastic::PeriodConfig{
                .startValue = start,
                .endValue = end,
                .volatility = vol,
                .seed = uniqueSeed
            };
        };

        Math::Stochastic::BrownianBridgePathGenerator::Config pathGenConfig = {
            .ticksPerStep = m_config.pathGenStepSizeMs * 1000,
            .ticksPerPeriod = 24ll * 60ll * 60ll * 1000ll * 1000ll,
            .onPeriodRollover = onPeriodRollover
        };

		m_pathGenerators[ccy] = std::make_unique<Math::Stochastic::BrownianBridgePathGenerator>( pathGenConfig );
    }
}

void FXFeedService::onShutdown()
{
	m_publisher.reset();
}

void FXFeedService::run()
{
    auto getBaseSpread = [] ( const std::string& currency )
    {
        if( currency == "JPY" ) return 0.005; // 0.5 JPY pips
        return 0.00005;                       // 0.5 Standard pips
    };

    auto next_wake_time = std::chrono::steady_clock::now();
    const auto publish_interval = std::chrono::milliseconds( m_config.publishFrequencyMs );

    while( shouldRun() )
    {
        int64_t current_usec = Time::DateTime::nowUTC().microsecondsSinceEpoch();

        for( const auto& [ccy, pathGen] : m_pathGenerators )
        {
            // Grab the mid rate
            double midRate = pathGen->getValueAtTime( current_usec );

            // Grab today's volatility from the generator's current config
            double vol = pathGen->currentPeriodConfig().volatility;

            // DYNAMIC SPREAD LOGIC: 
            // Base spread + a penalty for high volatility
            double halfSpread = getBaseSpread( ccy ) + ( vol * 10.0 * getBaseSpread( ccy ) );

            MD::FXRate rate;
            rate.mid = midRate;
            rate.bid = midRate - halfSpread;
            rate.ask = midRate + halfSpread;

			Log( Module::EXE ).debug( "Publishing {} rate update: mid={:.5f}, bid={:.5f}, ask={:.5f}", ccy, rate.mid, rate.bid, rate.ask );

			MD::Record<MD::FXRate> record;
			record.data = rate;
            record.header.id = ccy;
			record.header.asofTs = Time::DateTime::nowUTC();

            m_publisher->publish( Mkt::Name::LIVE, std::move( record ) );
        }

        next_wake_time += publish_interval;
        std::this_thread::sleep_until( next_wake_time );
    }
}

void FXFeedService::registerConfigOptions( Cfg::ConfigWrangler& cfg )
{
	cfg.add( m_config.streamSvcDSH,       "--streamServiceDSH",   "The DSH of the streaming service to use for the mktdata publisher" );
	cfg.add( m_config.publishFrequencyMs, "--publishFrequencyMs", "How often to publish new market data updates (e.g., 100ms)" );
	cfg.add( m_config.pathGenStepSizeMs,  "--pathGenStepSizeMs",  "The step size for the path generator (e.g., 100ms) - must be <= publishFrequency" );
	cfg.add( m_config.publishCcys,        "--publishCcys",        "Comma-separated list of currency codes to publish (e.g., EUR,GBP,JPY)" );
}
