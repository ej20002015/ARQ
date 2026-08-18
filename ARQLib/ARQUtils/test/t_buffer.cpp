#include <ARQUtils/buffer.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>

using namespace ARQ;

namespace
{

Buffer makeBuffer( const std::initializer_list<uint8_t> bytes )
{
	return Buffer( bytes.begin(), bytes.size() );
}

void expectBytes( const BufferView buffer, const std::initializer_list<uint8_t> expected )
{
	ASSERT_EQ( buffer.size, expected.size() );
	ASSERT_NE( buffer.data, nullptr );
	EXPECT_TRUE( std::equal( expected.begin(), expected.end(), buffer.data ) );
}

template<typename T>
concept ConstObjectExposesMutableTypedPointer = requires( const T& buffer )
{
	buffer.template getDataPtrAs<uint8_t*>();
};

}

TEST( BufferTest, CopyConstructionCreatesAnIndependentCopy )
{
	Buffer source = makeBuffer( { 1, 2, 3 } );
	Buffer copy( source );

	source.data[0] = 9;

	expectBytes( copy, { 1, 2, 3 } );
	EXPECT_NE( copy.data.get(), source.data.get() );
}

TEST( BufferTest, CopyAssignmentReplacesExistingStorageWithAnIndependentCopy )
{
	Buffer source = makeBuffer( { 1, 2, 3 } );
	Buffer target = makeBuffer( { 8, 9 } );

	target = source;
	source.data[1] = 7;

	expectBytes( target, { 1, 2, 3 } );
	EXPECT_NE( target.data.get(), source.data.get() );
}

TEST( BufferTest, CopyAssignmentFromEmptyBufferClearsDestination )
{
	Buffer source;
	Buffer target = makeBuffer( { 1, 2, 3 } );

	target = source;

	EXPECT_EQ( target.size, 0 );
	EXPECT_EQ( target.data, nullptr );
}

TEST( BufferTest, SelfAssignmentPreservesContents )
{
	Buffer buffer = makeBuffer( { 1, 2, 3 } );
	const uint8_t* const originalData = buffer.data.get();

	buffer = buffer;

	expectBytes( buffer, { 1, 2, 3 } );
	EXPECT_EQ( buffer.data.get(), originalData );
}

TEST( BufferTest, MoveOperationsTransferOwnershipAndClearSource )
{
	Buffer source = makeBuffer( { 1, 2, 3 } );
	const uint8_t* const sourceData = source.data.get();

	Buffer moved( std::move( source ) );

	EXPECT_EQ( moved.data.get(), sourceData );
	expectBytes( moved, { 1, 2, 3 } );
	EXPECT_EQ( source.size, 0 );
	EXPECT_EQ( source.data, nullptr );

	Buffer assigned = makeBuffer( { 8, 9 } );
	const uint8_t* const movedData = moved.data.get();
	assigned = std::move( moved );

	EXPECT_EQ( assigned.data.get(), movedData );
	expectBytes( assigned, { 1, 2, 3 } );
	EXPECT_EQ( moved.size, 0 );
	EXPECT_EQ( moved.data, nullptr );
}

TEST( SharedBufferTest, TakesOwnershipFromBufferAndSharesItsLifetime )
{
	Buffer source = makeBuffer( { 1, 2, 3 } );
	const uint8_t* const sourceData = source.data.get();

	SharedBuffer shared( std::move( source ) );
	SharedBuffer copy = shared;

	EXPECT_EQ( shared.data.get(), sourceData );
	EXPECT_EQ( copy.data.get(), sourceData );
	EXPECT_EQ( source.size, 0 );
	EXPECT_EQ( source.data, nullptr );

	shared = SharedBuffer{};
	expectBytes( copy, { 1, 2, 3 } );
}

TEST( SharedBufferTest, MoveOperationsTransferOwnershipAndClearSource )
{
	SharedBuffer source( makeBuffer( { 1, 2, 3 } ) );
	const uint8_t* const sourceData = source.data.get();

	SharedBuffer moved( std::move( source ) );

	EXPECT_EQ( moved.data.get(), sourceData );
	expectBytes( moved, { 1, 2, 3 } );
	EXPECT_EQ( source.size, 0 );
	EXPECT_EQ( source.data, nullptr );

	SharedBuffer assigned( makeBuffer( { 8, 9 } ) );
	assigned = std::move( moved );

	EXPECT_EQ( assigned.data.get(), sourceData );
	expectBytes( assigned, { 1, 2, 3 } );
	EXPECT_EQ( moved.size, 0 );
	EXPECT_EQ( moved.data, nullptr );
}

TEST( BufferTest, DataAccessorsRespectConstQualification )
{
	static_assert( std::is_same_v<decltype( std::declval<Buffer&>().getDataPtr() ), uint8_t*> );
	static_assert( std::is_same_v<decltype( std::declval<const Buffer&>().getDataPtr() ), const uint8_t*> );
	static_assert( std::is_same_v<decltype( std::declval<SharedBuffer&>().getDataPtr() ), uint8_t*> );
	static_assert( std::is_same_v<decltype( std::declval<const SharedBuffer&>().getDataPtr() ), const uint8_t*> );
	static_assert( !ConstObjectExposesMutableTypedPointer<Buffer> );
	static_assert( !ConstObjectExposesMutableTypedPointer<SharedBuffer> );

	Buffer buffer = makeBuffer( { 1 } );
	SharedBuffer sharedBuffer( makeBuffer( { 2 } ) );

	EXPECT_EQ( std::as_const( buffer ).getDataPtr(), buffer.getDataPtr() );
	EXPECT_EQ( std::as_const( sharedBuffer ).getDataPtr(), sharedBuffer.getDataPtr() );
}

TEST( BufferTest, EmptyBuffersConvertToEmptyStrings )
{
	const Buffer buffer;
	const SharedBuffer sharedBuffer;

	static_assert( !noexcept( buffer.toString() ) );
	static_assert( !noexcept( sharedBuffer.toString() ) );
	EXPECT_TRUE( buffer.toString().empty() );
	EXPECT_TRUE( sharedBuffer.toString().empty() );
}

TEST( BufferViewTest, ImplicitConversionAcceptsOnlyLvalueOwners )
{
	static_assert( std::is_convertible_v<Buffer&, BufferView> );
	static_assert( std::is_convertible_v<const Buffer&, BufferView> );
	static_assert( !std::is_convertible_v<Buffer&&, BufferView> );
	static_assert( std::is_convertible_v<SharedBuffer&, BufferView> );
	static_assert( std::is_convertible_v<const SharedBuffer&, BufferView> );
	static_assert( !std::is_convertible_v<SharedBuffer&&, BufferView> );

	Buffer buffer = makeBuffer( { 1 } );
	SharedBuffer sharedBuffer( makeBuffer( { 2 } ) );
	const BufferView bufferView = buffer;
	const BufferView sharedBufferView = sharedBuffer;

	expectBytes( bufferView, { 1 } );
	expectBytes( sharedBufferView, { 2 } );
}

TEST( BufferTest, ViewsAndStringsPreserveBinaryData )
{
	constexpr std::array<uint8_t, 4> bytes{ 'A', 0, 'B', 0xFF };
	const std::string expected( reinterpret_cast<const char*>( bytes.data() ), bytes.size() );
	Buffer buffer( bytes.data(), bytes.size() );

	const BufferView view = buffer;

	expectBytes( view, { 'A', 0, 'B', 0xFF } );
	EXPECT_EQ( buffer.toString(), expected );
}
