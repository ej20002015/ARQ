#pragma once
#include <ARQUtils/dll.h>

#include <ARQUtils/cli.h>
#include <ARQUtils/core.h>
#include <ARQUtils/enum.h>
#include <ARQUtils/str.h>

namespace ARQ::Cfg
{

enum class ArgPolicy
{
    Optional,
    Required
};

class IConfigWrangler
{
public:
	virtual ~IConfigWrangler() = default;
	
    ARQUtils_API virtual void add( int32_t& val,                  const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional ) = 0;
    ARQUtils_API virtual void add( double& val,                   const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional ) = 0;
    ARQUtils_API virtual void add( bool& val,                     const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional ) = 0;
    ARQUtils_API virtual void add( std::string& val,              const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional ) = 0;
    ARQUtils_API virtual void add( std::vector<std::string>& val, const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional, const char delim = ',' ) = 0;
    ARQUtils_API virtual void add( std::set<std::string>& val,    const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional, const char delim = ',' ) = 0;

    template<c_Enum T>
    void addEnum( T& val, const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional )
    {
        std::map<std::string, int32_t> intMap;

        for( const auto& [enumVal, enumName] : Enum::enum_entries<T>() )
        {
			if( Str::contains( enumName, "0x" ) )
                continue; // magic_enum can give garbage entries - skip these
            intMap[enumName.data()] = static_cast<int32_t>( enumVal );
        }

        const auto setter = [&val]( const int32_t& intVal )
        {
            val = static_cast<T>( intVal );
		};

        bindEnum( flag, intMap, setter, static_cast<int32_t>( val ), desc, env, policy );
    }

    ARQUtils_API virtual void bindEnum( const std::string_view flag, const std::map<std::string, int>& map, std::function<void( const int32_t& )> setter, int32_t currentVal, const std::string_view& desc, const std::string_view& env, ArgPolicy policy ) = 0;
};

class ConfigWrangler : public IConfigWrangler
{
public: // Constructor
    ARQUtils_API ConfigWrangler( const std::string_view appDescription, const std::string_view appName = "" );

public: // IConfigWrangler overrides
    ARQUtils_API void add( int32_t& val,                  const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional ) override;
    ARQUtils_API void add( double& val,                   const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional ) override;
    ARQUtils_API void add( bool& val,                     const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional ) override;
    ARQUtils_API void add( std::string& val,              const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional ) override;
    ARQUtils_API void add( std::vector<std::string>& val, const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional, const char delim = ',' ) override;
    ARQUtils_API void add( std::set<std::string>& val,    const std::string_view flag, const std::string_view desc, const std::string_view env = "", ArgPolicy policy = ArgPolicy::Optional, const char delim = ',' ) override;

    ARQUtils_API void bindEnum( const std::string_view flag, const std::map<std::string, int>& map, std::function<void( const int32_t& )> setter, int32_t currentVal, const std::string_view& help, const std::string_view& env, ArgPolicy policy ) override;

public: // Methods
	ARQUtils_API bool parse( int argc, char* argv[] );

    ARQUtils_API int printExitMsgAndGetRC() const;

    ARQUtils_API std::string dump() const;

private:
    template<typename T>
	CLI::Option* addImpl( T& val, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy );
    template<typename T>
    CLI::Option* addImplWithCallback( const T& currentVal, const std::function<void( const T& )>& setter, const std::string_view flag, const std::string_view desc, const std::string_view env, ArgPolicy policy );
    
    CLI::Option* addImplCommon( CLI::Option* const opt, const std::string_view env, ArgPolicy policy );

private:
	CLI::App                         m_cliApp;
	std::unique_ptr<CLI::ParseError> m_lastParseErrorPtr;

    std::vector<std::shared_ptr<int32_t>> m_enumStorage;
};

}