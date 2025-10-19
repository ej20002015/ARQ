#include "refdata_reader_service.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <ARQCore/refdata_source.h>

#include <iostream>

int main()
{
    std::string server_address = std::format( "0.0.0.0:{}", 50051 );
    std::shared_ptr<ARQ::RefDataSource> chRDSource = ARQ::RefDataSourceFactory::create( "ClickHouseDB" );
    ARQ::Grpc::RefData::RefDataReaderServiceImpl service( std::move( chRDSource ) );

    grpc::EnableDefaultHealthCheckService( true );
    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort( server_address, grpc::InsecureServerCredentials() );
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService( &service );
    // Finally assemble the server.
    std::unique_ptr<grpc::Server> server( builder.BuildAndStart() );
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}
