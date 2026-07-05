#include <ARQProtobuf/rd_entity_proto_converters.h>
#include <gtest/gtest.h>

#include <ARQCore/refdata_entities.h>

using namespace ARQ::Proto::RD;

TEST( RefDataProtoConvertersTest, Currency_Conversions )
{
    // --- LValue toProto ---
    ARQ::RD::Currency cppCurrency;
    cppCurrency.ccyID = "USD";
    cppCurrency.name = "US Dollar";
    cppCurrency.decimalPlaces = 2;
    cppCurrency.settlementDays = 2;

    ARQ::Proto::RD::Currency protoCurrency;
    toProto( cppCurrency, &protoCurrency );

    EXPECT_EQ( protoCurrency.ccy_id(), "USD" );
    EXPECT_EQ( protoCurrency.name(), "US Dollar" );
    EXPECT_EQ( protoCurrency.decimal_places(), 2 );
    EXPECT_EQ( protoCurrency.settlement_days(), 2 );

    // --- RValue toProto ---
    ARQ::Proto::RD::Currency protoCurrencyMove;
    toProto( std::move( cppCurrency ), &protoCurrencyMove );
    EXPECT_EQ( protoCurrencyMove.ccy_id(), "USD" );

    // --- LValue fromProto ---
    ARQ::RD::Currency parsedCppCcy = fromProto( protoCurrency );
    EXPECT_EQ( parsedCppCcy.ccyID, "USD" );
    EXPECT_EQ( parsedCppCcy.name, "US Dollar" );
    EXPECT_EQ( parsedCppCcy.decimalPlaces, 2 );
    EXPECT_EQ( parsedCppCcy.settlementDays, 2 );

    // --- RValue fromProto ---
    ARQ::RD::Currency parsedCppCcyMove = fromProto( std::move( protoCurrencyMove ) );
    EXPECT_EQ( parsedCppCcyMove.ccyID, "USD" );
}

TEST( RefDataProtoConvertersTest, User_Conversions_Optional_Set )
{
    // --- LValue toProto ---
    ARQ::RD::User cppUser;
    cppUser.userID = "JSMITH";
    cppUser.fullName = "John Smith";
    cppUser.email = "jsmith@example.com";
    cppUser.tradingDesk = "Equities"; // Optional is SET

    ARQ::Proto::RD::User protoUser;
    toProto( cppUser, &protoUser );

    EXPECT_EQ( protoUser.user_id(), "JSMITH" );
    EXPECT_EQ( protoUser.full_name(), "John Smith" );
    EXPECT_TRUE( protoUser.has_trading_desk() );
    EXPECT_EQ( protoUser.trading_desk(), "Equities" );

    // --- RValue toProto ---
    ARQ::Proto::RD::User protoUserMove;
    toProto( std::move( cppUser ), &protoUserMove );
    EXPECT_EQ( protoUserMove.user_id(), "JSMITH" );
    EXPECT_TRUE( protoUserMove.has_trading_desk() );

    // --- LValue fromProto ---
    ARQ::RD::User parsedCppUser = fromProto( protoUser );
    EXPECT_EQ( parsedCppUser.userID, "JSMITH" );
    EXPECT_TRUE( parsedCppUser.tradingDesk.has_value() );
    EXPECT_EQ( parsedCppUser.tradingDesk.value(), "Equities" );

    // --- RValue fromProto ---
    ARQ::RD::User parsedCppUserMove = fromProto( std::move( protoUserMove ) );
    EXPECT_EQ( parsedCppUserMove.userID, "JSMITH" );
    EXPECT_TRUE( parsedCppUserMove.tradingDesk.has_value() );
    EXPECT_EQ( parsedCppUserMove.tradingDesk.value(), "Equities" );
}

TEST( RefDataProtoConvertersTest, User_Conversions_Optional_Empty )
{
    // --- LValue toProto ---
    ARQ::RD::User cppUser;
    cppUser.userID = "BWAYNE";
    cppUser.fullName = "Bruce Wayne";
    cppUser.email = "bwayne@example.com";
    cppUser.tradingDesk = std::nullopt; // Optional is EMPTY

    ARQ::Proto::RD::User protoUser;
    toProto( cppUser, &protoUser );

    EXPECT_EQ( protoUser.user_id(), "BWAYNE" );
    EXPECT_FALSE( protoUser.has_trading_desk() ); // Should not be set in proto

    // --- RValue toProto ---
    ARQ::Proto::RD::User protoUserMove;
    toProto( std::move( cppUser ), &protoUserMove );
    EXPECT_FALSE( protoUserMove.has_trading_desk() );

    // --- LValue fromProto ---
    ARQ::RD::User parsedCppUser = fromProto( protoUser );
    EXPECT_EQ( parsedCppUser.userID, "BWAYNE" );
    EXPECT_FALSE( parsedCppUser.tradingDesk.has_value() ); // Should remain nullopt

    // --- RValue fromProto ---
    ARQ::RD::User parsedCppUserMove = fromProto( std::move( protoUserMove ) );
    EXPECT_FALSE( parsedCppUserMove.tradingDesk.has_value() );
}