#include "../src/misc_type_serialisers.h"
#include <gtest/gtest.h>

using namespace ARQ;

TEST( SerialiserTest, RefDataCommandResponse )
{
	ProtobufTypeSerialiser_RefDataCommandResponse typeSerialiser;

	RefDataCommandResponse resp;
	resp.status = RefDataCommandResponse::SUCCESS;
	resp.corrID = ID::UUID::create();
	resp.message = "Hello there";

	const Buffer buf = typeSerialiser.serialise( resp );
	RefDataCommandResponse desResp;
	typeSerialiser.deserialise( buf, desResp );
	
	ASSERT_EQ( resp.status, desResp.status );
	ASSERT_EQ( resp.corrID, desResp.corrID );
	ASSERT_EQ( resp.message, desResp.message );
}