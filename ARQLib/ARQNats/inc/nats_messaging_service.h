#pragma once
#include <ARQNats/dll.h>

#include <ARQCore/messaging_service.h>

namespace ARQ
{

extern "C" ARQNats_API IMessagingService* createMessagingService( const std::string_view dsh );

}