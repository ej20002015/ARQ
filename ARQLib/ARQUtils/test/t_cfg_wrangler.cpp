#include <ARQUtils/cfg_wrangler.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using namespace ARQ;
using ::testing::HasSubstr;

namespace
{

enum class RunMode
{
	FAST,
	SAFE
};

bool parse( Cfg::ConfigWrangler& config, const std::initializer_list<std::string_view> arguments )
{
	std::vector<std::string> ownedArguments{ "config-wrangler-test" };
	ownedArguments.reserve( arguments.size() + 1 );
	for( const std::string_view argument : arguments )
		ownedArguments.emplace_back( argument );

	std::vector<char*> argv;
	argv.reserve( ownedArguments.size() );
	for( std::string& argument : ownedArguments )
		argv.push_back( argument.data() );

	return config.parse( static_cast<int>( argv.size() ), argv.data() );
}

class ScopedEnvironmentVariable
{
public:
	ScopedEnvironmentVariable( std::string name, const std::string_view value )
		: m_name( std::move( name ) )
	{
		if( const char* const previous = std::getenv( m_name.c_str() ) )
			m_previous = previous;
		set( value );
	}

	~ScopedEnvironmentVariable()
	{
		if( m_previous )
			set( *m_previous );
		else
			unset();
	}

	ScopedEnvironmentVariable( const ScopedEnvironmentVariable& )            = delete;
	ScopedEnvironmentVariable( ScopedEnvironmentVariable&& )                 = delete;
	ScopedEnvironmentVariable& operator=( const ScopedEnvironmentVariable& ) = delete;
	ScopedEnvironmentVariable& operator=( ScopedEnvironmentVariable&& )      = delete;

private:
	void set( const std::string_view value ) const
	{
#ifdef _WIN32
		_putenv_s( m_name.c_str(), std::string( value ).c_str() );
#else
		setenv( m_name.c_str(), std::string( value ).c_str(), 1 );
#endif
	}

	void unset() const
	{
#ifdef _WIN32
		_putenv_s( m_name.c_str(), "" );
#else
		unsetenv( m_name.c_str() );
#endif
	}

	std::string                m_name;
	std::optional<std::string> m_previous;
};

}

TEST( ConfigWranglerTest, ParsesEverySupportedScalarType )
{
	Cfg::ConfigWrangler config( "Test scalar configuration", "config-wrangler-test" );
	int32_t int32Value = 0;
	int64_t int64Value = 0;
	double doubleValue = 0.0;
	bool boolValue = false;
	std::string stringValue;

	config.add( int32Value, "--int32", "32-bit integer" );
	config.add( int64Value, "--int64", "64-bit integer" );
	config.add( doubleValue, "--double", "floating-point value" );
	config.add( boolValue, "--bool", "boolean value" );
	config.add( stringValue, "--string", "string value" );

	ASSERT_TRUE( parse( config, { "--int32", "-42", "--int64", "9876543210", "--double", "3.125", "--bool", "true", "--string", "hello" } ) );

	EXPECT_EQ( int32Value, -42 );
	EXPECT_EQ( int64Value, 9'876'543'210 );
	EXPECT_DOUBLE_EQ( doubleValue, 3.125 );
	EXPECT_TRUE( boolValue );
	EXPECT_EQ( stringValue, "hello" );
}

TEST( ConfigWranglerTest, ParsesDelimitedVectorsAndSets )
{
	Cfg::ConfigWrangler config( "Test collection configuration" );
	std::vector<std::string> vectorValue;
	std::set<std::string> setValue;

	config.add( vectorValue, "--vector", "vector value", "", Cfg::ArgPolicy::Optional, ';' );
	config.add( setValue, "--set", "set value", "", Cfg::ArgPolicy::Optional, '|' );

	ASSERT_TRUE( parse( config, { "--vector", "alpha;beta;gamma", "--set", "red|blue|red" } ) );

	EXPECT_THAT( vectorValue, ::testing::ElementsAre( "alpha", "beta", "gamma" ) );
	EXPECT_THAT( setValue, ::testing::UnorderedElementsAre( "blue", "red" ) );
}

TEST( ConfigWranglerTest, ParsesEnumsCaseInsensitivelyAndRejectsUnknownValues )
{
	RunMode mode = RunMode::FAST;
	Cfg::ConfigWrangler validConfig( "Test enum configuration" );
	validConfig.addEnum( mode, "--mode", "run mode" );

	ASSERT_TRUE( parse( validConfig, { "--mode", "safe" } ) );
	EXPECT_EQ( mode, RunMode::SAFE );

	Cfg::ConfigWrangler invalidConfig( "Test enum configuration" );
	invalidConfig.addEnum( mode, "--mode", "run mode" );
	EXPECT_FALSE( parse( invalidConfig, { "--mode", "unsupported" } ) );
}

TEST( ConfigWranglerTest, RequiredOptionsFailCleanlyWhenMissing )
{
	Cfg::ConfigWrangler config( "Test required configuration" );
	std::string value;
	config.add( value, "--required", "required value", "", Cfg::ArgPolicy::Required );

	ASSERT_FALSE( parse( config, {} ) );

	testing::internal::CaptureStdout();
	testing::internal::CaptureStderr();
	const int exitCode = config.printExitMsgAndGetRC();
	static_cast<void>( testing::internal::GetCapturedStdout() );
	static_cast<void>( testing::internal::GetCapturedStderr() );
	EXPECT_NE( exitCode, 0 );
}

TEST( ConfigWranglerTest, ExtrasAreRejectedUnlessExplicitlyAllowed )
{
	Cfg::ConfigWrangler strictConfig( "Test strict configuration" );
	EXPECT_FALSE( parse( strictConfig, { "unexpected" } ) );

	Cfg::ConfigWrangler permissiveConfig( "Test permissive configuration" );
	permissiveConfig.allowExtras();
	EXPECT_TRUE( parse( permissiveConfig, { "unexpected" } ) );
}

TEST( ConfigWranglerTest, ReadsEnvironmentValuesAndCommandLineTakesPrecedence )
{
	const ScopedEnvironmentVariable environment( "ARQ_TEST_CONFIG_WRANGLER_COUNT", "41" );

	Cfg::ConfigWrangler environmentConfig( "Test environment configuration" );
	int32_t environmentValue = 0;
	environmentConfig.add( environmentValue, "--count", "count", "ARQ_TEST_CONFIG_WRANGLER_COUNT" );
	ASSERT_TRUE( parse( environmentConfig, {} ) );
	EXPECT_EQ( environmentValue, 41 );

	Cfg::ConfigWrangler commandLineConfig( "Test environment configuration" );
	int32_t commandLineValue = 0;
	commandLineConfig.add( commandLineValue, "--count", "count", "ARQ_TEST_CONFIG_WRANGLER_COUNT" );
	ASSERT_TRUE( parse( commandLineConfig, { "--count", "42" } ) );
	EXPECT_EQ( commandLineValue, 42 );
	EXPECT_THAT( commandLineConfig.dump(), HasSubstr( "count=42" ) );
}
