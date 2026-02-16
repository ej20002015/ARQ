#include "service.h"

using namespace ARQ;

int main( int argc, char** argv )
{
	return ServiceRunner::run<RefDataAuditProjectorService>( argc, argv );
}