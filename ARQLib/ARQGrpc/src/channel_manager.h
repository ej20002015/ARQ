#pragma once

#include <grpcpp/grpcpp.h>

#include <unordered_map>
#include <mutex>

namespace ARQ
{

class ChannelManager
{
public:
	static ChannelManager& inst()
	{
		static ChannelManager inst;
		return inst;
	}

	[[nodiscard]] std::shared_ptr<grpc::Channel> get( const std::string_view dsh, const std::string_view service );

private:
	std::unordered_map<std::string, std::shared_ptr<grpc::Channel>> m_channelMap;
	std::mutex m_mut;
};

}