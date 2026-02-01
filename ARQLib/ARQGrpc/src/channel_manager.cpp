#include "channel_manager.h"

#include <ARQUtils/logger.h>
#include <ARQCore/data_source_config.h>

namespace ARQ
{

std::shared_ptr<grpc::Channel> ChannelManager::get( const std::string_view dsh, const std::string_view service )
{
    const std::string channelID = std::string( dsh ) + "." + service.data();

    {
        std::lock_guard<std::mutex> lock( m_mut );
        const auto it = m_channelMap.find( channelID );
        if( it != m_channelMap.end() )
            return it->second;
    }

    try
    {
        Log( Module::GRPC ).info( "Creating new gRPC channel for dsh [{}], service [{}]", dsh, service );

        const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );
        const DataSourceConfig::ConnProps connProps = dsc.connPropsMap.at( service.data() );
        const std::string target = std::format( "{}:{}", connProps.hostname, connProps.port );
        
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel( target, grpc::InsecureChannelCredentials() );

        {
            std::lock_guard<std::mutex> lock( m_mut );
            m_channelMap.insert( std::make_pair( channelID, channel ) );
        }

        return channel;
    }
    catch( const ARQException& e )
    {
        throw ARQException( std::format( "Cannot create gRPC channel {}: {}", channelID, e.what() ) );
    }
    catch( const std::exception& e )
    {
        throw ARQException( std::format( "Cannot create gRPC channel {}: {}", channelID, e.what() ) );
    }
    catch( ... )
    {
        throw ARQException( std::format( "Cannot create gRPC channel {}: unknown exception occured", channelID ) );
    }
}

}
