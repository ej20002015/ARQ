#include <ARQGrpc/grpc_refdata_source.h>

#include <ARQGrpc/grpc_refdata_source_interface.h>

namespace ARQ
{

extern "C" IRefDataSource* createRefDataSource( const std::string_view dsh )
{
	return new GrpcRefDataSource( dsh );
}

}
