#include "eod_file_parser.h"

#include <ARQUtils/error.h>
#include <ARQUtils/str.h>
#include <ARQUtils/logger.h>

#include <fstream>
#include <string>

using namespace ARQ;

std::unordered_map<int64_t, EODRecord> EODParser::load( const std::filesystem::path& filepath )
{
    std::unordered_map<int64_t, EODRecord> data;
    std::ifstream file( filepath );

    if( !file.is_open() )
		throw ARQException( "EODParser: Failed to open file: " + filepath.string() );

    std::string line;
    int line_number = 0;

    while( std::getline( file, line ) )
    {
        line_number++;
        if( line.empty() ) continue; // Skip blank lines

		const auto tokens = Str::split( line, ',' );

        if( tokens.size() != 3 )
        {
			Log( Module::EXE ).warn( "EODParser: Skipping malformed line {} in {} : Expected 3 comma-separated values but got {}", line_number, filepath.string(), tokens.size());
			continue;
        }

        try
        {
            // 1. Parse Period Index
            int64_t period_index = std::stoll( tokens[0].data() );
            // 2. Parse Rate
            double rate = std::stod( tokens[1].data() );
            // 3. Parse Volatility
            double vol = std::stod( tokens[2].data() );

            data[period_index] = { rate, vol };
        }
        catch( const std::exception& e )
        {
			Log( Module::EXE ).warn( "EODParser: Skipping malformed line {} in {} : {}", line_number, filepath.string(), e.what() );
        }
    }

	Log( Module::EXE ).info( "EODParser: Successfully loaded {} records from {}", data.size(), filepath.string() );

    return data;
}