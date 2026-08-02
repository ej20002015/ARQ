#include <ARQCore/lib_component.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>

using namespace ARQ;

namespace
{

LibComponentRegistration registration( const std::string_view name, const uint16_t position )
{
	return LibComponentRegistration{
		.name    = name,
		.order   = {
			.phase    = LibComponentPhase::FinancialFoundation,
			.position = position,
		},
		.factory = [] () -> std::unique_ptr<ILibComponent> { return {}; },
	};
}

}

TEST( LibComponentRegistryTest, ReturnsComponentsInAscendingOrder )
{
	const size_t initialSize = LibComponentRegistry::getComponents().size();

	LibComponentRegistry::Reg third(  registration( "Third",  61002 ) );
	LibComponentRegistry::Reg first(  registration( "First",  61000 ) );
	LibComponentRegistry::Reg second( registration( "Second", 61001 ) );

	const std::vector<LibComponentRegistration> components = LibComponentRegistry::getComponents();

	ASSERT_EQ( components.size(), initialSize + 3 );
	const auto firstComponent = std::ranges::find( components, "First", &LibComponentRegistration::name );
	ASSERT_NE( firstComponent, components.end() );
	ASSERT_GE( components.end() - firstComponent, 3 );
	EXPECT_EQ( firstComponent[0].name, "First" );
	EXPECT_EQ( firstComponent[1].name, "Second" );
	EXPECT_EQ( firstComponent[2].name, "Third" );
}