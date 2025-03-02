#include <TMQCore/variant.h>
#include <gtest/gtest.h>

using namespace TMQ;

// Test Default Constructor
TEST( VariantTests, DefaultConstructor )
{
    Variant v;
    EXPECT_TRUE( v.is<Empty>() );
}

// Test Construction with Scalar Types
TEST( VariantTests, ScalarConstruction )
{
    Variant int32Var = 42;
    EXPECT_TRUE( int32Var.is<int32_t>() );
    EXPECT_EQ( int32Var.as<int32_t>(), 42 );

    Variant doubleVar = 3.14159;
    EXPECT_TRUE( doubleVar.is<double>() );
    EXPECT_DOUBLE_EQ( doubleVar.as<double>(), 3.14159 );

    Variant boolVar = true;
    EXPECT_TRUE( boolVar.is<bool>() );
    EXPECT_EQ( boolVar.as<bool>(), true );

    Variant stringVar = "Test String";
    EXPECT_TRUE( stringVar.is<std::string>() );
    EXPECT_EQ( stringVar.as<std::string>(), "Test String" );
}

// Test DateTime Construction
TEST( VariantTests, DateTimeConstruction )
{
    auto now = std::chrono::system_clock::now();
    Variant dateTimeVar = now;
    EXPECT_TRUE( dateTimeVar.is<std::chrono::system_clock::time_point>() );
    EXPECT_EQ( dateTimeVar.as<std::chrono::system_clock::time_point>(), now );
}

// Test mutating values
TEST( VariantTests, MutatingValues )
{
	Variant int32Var = 42;
	int32Var.as<int32_t>() = 99;
	EXPECT_EQ( int32Var.as<int32_t>(), 99 );

	Variant doubleVar = 3.14159;
	doubleVar.as<double>() = 2.71828;
	EXPECT_DOUBLE_EQ( doubleVar.as<double>(), 2.71828 );

	Variant boolVar = true;
	boolVar.as<bool>() = false;
	EXPECT_EQ( boolVar.as<bool>(), false );

    Variant stringVar = "Test String";
	stringVar.as<std::string>() = "New String";
	EXPECT_EQ( stringVar.as<std::string>(), "New String" );
}

// Test Exception Storage
TEST( VariantTests, ExceptionHandling )
{
    TMQException ex( "Error occurred", ErrCode::INVALID_INPUT );
    Variant exVar = ex;
    EXPECT_TRUE( exVar.is<TMQException>() );
    EXPECT_EQ( exVar.as<TMQException>().code(), ErrCode::INVALID_INPUT );
}

// Test Accessing Incorrect Types
TEST( VariantTests, TypeMismatchThrows )
{
    Variant int32Var = 42;
    EXPECT_THROW( auto val = int32Var.as<double>(), TMQException );
}

// Test Optional Accessor (tryAs)
TEST( VariantTests, TryAsAccessor )
{
    Variant int32Var = 123;
    auto result = int32Var.tryAs<int32_t>();
    ASSERT_TRUE( result.has_value() );
    EXPECT_EQ( result.value(), 123 );

    auto invalidResult = int32Var.tryAs<double>();
    EXPECT_FALSE( invalidResult.has_value() );
}

// Test Assigning Different Types
TEST( VariantTests, ReassignVariant )
{
    Variant var = 10;
    EXPECT_TRUE( var.is<int32_t>() );
    EXPECT_EQ( var.as<int32_t>(), 10 );

    var = "Reassigned";
    EXPECT_TRUE( var.is<std::string>() );
    EXPECT_EQ( var.as<std::string>(), "Reassigned" );
}

TEST( VariantTests, VariantEquality )
{
    // TODO: Implement this test
}

// Test Variant Array Construction
TEST( VariantArrTests, ArrayConstruction )
{
    VariantArr arr( 3, 2 ); // 3 rows, 2 columns
    EXPECT_EQ( arr.rows(), 3 );
    EXPECT_EQ( arr.cols(), 2 );
}

// Test Variant Array Element Access
TEST( VariantArrTests, ArrayElementAccess )
{
    VariantArr arr( 2, 2 );

    arr.at( 0, 0 ) = 1;
    arr.at( 0, 1 ) = 2.5;
    arr.at( 1, 0 ) = "Hello";
    arr.at( 1, 1 ) = true;

    EXPECT_EQ( arr.at( 0, 0 ).as<int32_t>(), 1 );
    EXPECT_DOUBLE_EQ( arr.at( 0, 1 ).as<double>(), 2.5 );
    EXPECT_EQ( arr.at( 1, 0 ).as<std::string>(), "Hello" );
    EXPECT_EQ( arr.at( 1, 1 ).as<bool>(), true );
}

// Test Variant Array Copy Constructor
TEST( VariantArrTests, CopyConstructor )
{
    VariantArr original( 2, 2 );
    original.at( 0, 0 ) = 42;
    original.at( 1, 1 ) = "Copy Test";

    VariantArr copy = original;

    EXPECT_EQ( copy.rows(), original.rows() );
    EXPECT_EQ( copy.cols(), original.cols() );
    EXPECT_EQ( copy.at( 0, 0 ).as<int32_t>(), 42 );
    EXPECT_EQ( copy.at( 1, 1 ).as<std::string>(), "Copy Test" );
}

// Test Variant Array Move Constructor
TEST( VariantArrTests, MoveConstructor )
{
    VariantArr original( 2, 2 );
    original.at( 0, 0 ) = 99;

    VariantArr moved = std::move( original );

    EXPECT_EQ( moved.rows(), 2 );
    EXPECT_EQ( moved.cols(), 2 );
    EXPECT_EQ( moved.at( 0, 0 ).as<int32_t>(), 99 );
}

// Test mutating elements in a Variant Array
TEST( VariantArrTests, MutatingElements )
{
	VariantArr arr( 2, 2 );
	arr.at( 0, 0 ) = 1;
	arr.at( 0, 1 ) = 2.5;
	arr.at( 1, 0 ) = "Hello";
	arr.at( 1, 1 ) = true;

	const auto& constVariant = arr.at( 0 );
	auto& variant = arr.at( 0 );
	variant = 42;

	EXPECT_EQ( arr.at( 0, 0 ).as<int32_t>(), 42 );
	EXPECT_DOUBLE_EQ( arr.at( 0, 1 ).as<double>(), 2.5 );
	EXPECT_EQ( arr.at( 1, 0 ).as<std::string>(), "Hello" );
	EXPECT_EQ( arr.at( 1, 1 ).as<bool>(), true );
}

// Test Out-of-Bounds Access
TEST( VariantArrTests, OutOfBoundsAccessThrows )
{
    VariantArr arr( 2, 2 );
    EXPECT_THROW( const auto& val = arr.at( 3, 0 ), TMQException );
    EXPECT_THROW( const auto& val = arr.at( 0, 2 ), TMQException );
}

