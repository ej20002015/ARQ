#include <gtest/gtest.h>
#include <TMQSerialisation/ser_mktdata_entities.h>

using namespace TMQ;
using namespace std::chrono_literals;

FXRate createTestFXRate()
{
    FXRate rate;
    rate.instrumentID = "GBPUSD";
    rate.rate = 1.0;
    rate.bid = 0.9;
    rate.ask = 1.1;
    rate.source = "Test";
    rate.asofTs = std::chrono::system_clock::time_point() + 1h;

    return rate;
}

TEST( SerMktdataEntitiesTest, SerialiseFXRate )
{
    FXRate rate = createTestFXRate();
    Buffer serializedData = serialise( rate );

    // Check that the serialized data is not empty
    EXPECT_TRUE( serializedData.size );
}

TEST( SerMktdataEntitiesTest, DeserialiseFXRate )
{
    FXRate rate = createTestFXRate();
    Buffer serializedData = serialise( rate );
    FXRate deserialisedRate = deserialise<FXRate>( serializedData );

    // Check that the deserialized data matches the original data
    EXPECT_EQ( deserialisedRate.instrumentID, rate.instrumentID );
    EXPECT_DOUBLE_EQ( deserialisedRate.rate, rate.rate );
    EXPECT_DOUBLE_EQ( deserialisedRate.bid, rate.bid );
    EXPECT_DOUBLE_EQ( deserialisedRate.ask, rate.ask );
    EXPECT_EQ( deserialisedRate.source, rate.source );
    EXPECT_EQ( deserialisedRate.asofTs, rate.asofTs );
}