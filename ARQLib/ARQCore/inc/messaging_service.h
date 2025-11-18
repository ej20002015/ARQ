#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQCore/messaging_service_interface.h>

#include <unordered_map>
#include <shared_mutex>

namespace ARQ
{

using MessagingServiceCreateFunc = std::add_pointer<IMessagingService* ( const std::string_view dsh )>::type;

class MessagingServiceFactory
{
public:
	[[nodiscard]] ARQCore_API static std::shared_ptr<IMessagingService> create( const std::string_view dsh );
	
	// Not threadsafe but should only really be used on startup / for testing anyway
	ARQCore_API static void addCustomService( const std::string_view dsh, const std::shared_ptr<IMessagingService>& service );
	ARQCore_API static void delCustomService( const std::string_view dsh );

private:
	static std::unordered_map<std::string, std::shared_ptr<IMessagingService>, TransparentStringHash, std::equal_to<>> s_customServices;
};

}