#include <ARQCore/refdata_meta.h>

#include <gtest/gtest.h>

#include <set>
#include <string_view>

using namespace ARQ;
using namespace ARQ::RD;

namespace
{

void expectMemberInfoEqual( const MemberInfo& actual, const MemberInfo& expected )
{
	EXPECT_EQ( actual.name, expected.name );
	EXPECT_EQ( actual.comment, expected.comment );
	EXPECT_EQ( actual.physicalType, expected.physicalType );
	EXPECT_EQ( actual.indexType, expected.indexType );
	EXPECT_EQ( actual.format, expected.format );
	EXPECT_EQ( actual.uiReadOnly, expected.uiReadOnly );
	EXPECT_EQ( actual.isPrimaryKey, expected.isPrimaryKey );
	EXPECT_EQ( actual.isOptional, expected.isOptional );
}

template<c_RefData T>
void expectEntityMetadataMatchesTraits()
{
	const Meta::EntityMetadata& metadata = Meta::get( Traits<T>::name() );

	EXPECT_EQ( metadata.name, Traits<T>::name() );
	ASSERT_EQ( metadata.membersInfo.size(), Traits<T>::membersInfo.size() );
	for( size_t i = 0; i < metadata.membersInfo.size(); ++i )
		expectMemberInfoEqual( metadata.membersInfo[i], Traits<T>::membersInfo[i] );
}

}

TEST( RefDataMetadataTest, RegistryContainsEachSupportedEntityExactlyOnce )
{
	const auto& allMetadata = Meta::getAll();
	const auto& allNames = Meta::getAllNames();
	const std::set<std::string_view> expectedNames{ "Currency", "User" };
	const std::set<std::string_view> actualNames( allNames.begin(), allNames.end() );

	ASSERT_EQ( allMetadata.size(), expectedNames.size() );
	ASSERT_EQ( allNames.size(), expectedNames.size() );
	EXPECT_EQ( actualNames, expectedNames );
	for( size_t i = 0; i < allMetadata.size(); ++i )
	{
		EXPECT_EQ( allMetadata[i].name, allNames[i] );
		EXPECT_EQ( &Meta::get( allNames[i] ), &allMetadata[i] );
	}
}

TEST( RefDataMetadataTest, EntityMetadataMatchesCompileTimeTraits )
{
	expectEntityMetadataMatchesTraits<Currency>();
	expectEntityMetadataMatchesTraits<User>();
}

TEST( RefDataMetadataTest, HeaderMetadataMatchesCanonicalDefinition )
{
	const auto& headerMetadata = Meta::getHeaderMemberInfos();

	ASSERT_EQ( headerMetadata.size(), recordHeaderMembersInfo.size() );
	for( size_t i = 0; i < headerMetadata.size(); ++i )
		expectMemberInfoEqual( headerMetadata[i], recordHeaderMembersInfo[i] );
}

TEST( RefDataMetadataTest, UnknownEntityNameThrows )
{
	EXPECT_THROW( Meta::get( "UnknownEntity" ), ARQException );
}
