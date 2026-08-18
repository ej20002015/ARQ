#pragma once
#include <ARQCore/dll.h>

#include <compare>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace ARQ
{

enum class LibComponentPhase : uint16_t
{
	FinancialFoundation = 100, // Components that provide foundational financial data and services
};

/**
 * @brief Represents the initialization order of an ARQ library component.
 * @details Components are initialized in ascending order of phase and position.
*/
struct LibComponentOrder
{
	LibComponentPhase phase;
	uint16_t          position;

	auto operator<=>( const LibComponentOrder& ) const = default;
};

/**
 * @brief Defines the interface for an ARQ library component.
 * @details Components must implement this interface to be registered and initialized by the library.
*/
class ILibComponent
{
public:
	virtual ~ILibComponent() = default;
};

using LibComponentFactory = std::function<std::unique_ptr<ILibComponent>()>;

/**
 * @brief Represents a registered ARQ library component.
 * @details Components are registered with a name, initialization order and factory function.
*/
struct LibComponentRegistration
{
	std::string_view    name;
	LibComponentOrder   order;
	LibComponentFactory factory;
};

class LibComponentRegistry
{
public:
	class Reg
	{
	public:
		/**
		 * @brief Registers an ARQ library component for initialization by libInit().
		 * @param registration The component name, initialization order and factory.
		 */
		explicit ARQCore_API Reg( LibComponentRegistration registration );

		Reg( const Reg& )            = delete;
		Reg( Reg&& )                 = delete;
		Reg& operator=( const Reg& ) = delete;
		Reg& operator=( Reg&& )      = delete;
	};

	/**
	 * @brief Gets all registered library components in initialization order.
	 * @return A copy of the component registrations sorted in ascending order.
	 */
	ARQCore_API static std::vector<LibComponentRegistration> getComponents();

private:
	static std::vector<LibComponentRegistration>& registrations();
};

}
