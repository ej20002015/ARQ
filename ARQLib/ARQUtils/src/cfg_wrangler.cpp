#include <ARQUtils/cfg_wrangler.h>

#include <ARQUtils/error.h>

namespace ARQ::Cfg
{

ConfigWrangler::ConfigWrangler( const std::string_view appDescription, const std::string_view appName /*= ""*/ )
{
	m_cliApp.description( appDescription.data() );
    if( appName.size() )
	    m_cliApp.name( appName.data() );

	m_cliApp.set_config( "--config,-c", "app_config.toml", "Read configuration from a toml or ini file" );
	m_cliApp.set_help_flag( "--help,-?", "Print help text" );
}

void ConfigWrangler::add( int32_t& val, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy )
{
    addImpl( val, flag, desc, env, policy );
}

void ConfigWrangler::add( double& val, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy )
{
    addImpl( val, flag, desc, env, policy );
}

void ConfigWrangler::add( bool& val, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy )
{
    addImpl( val, flag, desc, env, policy );
}

void ConfigWrangler::add( std::string& val, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy )
{
    addImpl( val, flag, desc, env, policy );
}

void ConfigWrangler::add( std::vector<std::string>& val, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy, const char delim /*= ','*/ )
{
    CLI::Option* const opt = addImpl( val, flag, desc, env, policy );
    opt->delimiter( delim );
}

void ConfigWrangler::add( std::set<std::string>& val, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy, const char delim /*= ','*/ )
{
    CLI::Option* const opt = addImpl( val, flag, desc, env, policy );
    opt->delimiter( delim );
}

void ConfigWrangler::bindEnum( const std::string_view flag, const std::map<std::string, int>& map, std::function<void( const int32_t& )> setter, int32_t currentVal, const std::string_view& desc, const std::string_view& env, ArgPolicy policy )
{
    CLI::Option* const opt = addImplWithCallback( currentVal, setter, flag, desc, env, policy );
    // Apply Map Transformer (Validates input string -> int)
    opt->transform( CLI::CheckedTransformer( map, CLI::ignore_case ) );
}

bool ConfigWrangler::parse( int argc, char* argv[] )
{
    try
    {
        m_cliApp.parse( argc, argv );
    }
    catch( const CLI::ParseError& e )
    {
        m_lastParseErrorPtr = std::make_unique<CLI::ParseError>( e );
        return false;
    }

    return true;
}

int ConfigWrangler::printExitMsgAndGetRC() const
{
    if( m_lastParseErrorPtr )
        return m_cliApp.exit( *m_lastParseErrorPtr );
    else
    {
        ARQ_ASSERT( false );
        throw ARQException( "Parsing did not error - don't call printExitMsgAndGetRC()" );
    }
}

std::string ConfigWrangler::dump() const
{
    return m_cliApp.config_to_str( true );
}

void ConfigWrangler::allowExtras()
{
    m_cliApp.allow_extras();
}

template<typename T>
CLI::Option* ConfigWrangler::addImpl( T& val, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy )
{
    CLI::Option* const opt = m_cliApp.add_option( flag.data(), val, desc.data() );
    return addImplCommon( opt, env, policy );
}

template<typename T>
CLI::Option* ConfigWrangler::addImplWithCallback( const T& currentVal, const std::function<void( const T& )>& setter, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy )
{
    CLI::Option* const opt = m_cliApp.add_option_function<T>( flag.data(), setter, desc.data() );
    opt->default_val( currentVal );
    return addImplCommon( opt, env, policy );
}

CLI::Option* ConfigWrangler::addImplCommon( CLI::Option* const opt, const std::string_view env, ArgPolicy policy )
{
    if( !env.empty() )
        opt->envname( env.data() );
    if( policy == ArgPolicy::Required )
        opt->required();

    opt->capture_default_str();

    return opt;
}

}