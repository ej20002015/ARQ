#pragma once

#include <ARQCore/service_base.h>
#include <ARQMarket/mktdata_publisher.h>
#include <ARQMath/path_gen.h>

#include <vector>
#include <unordered_map>

using namespace ARQ;

class FXFeedService : public ServiceBase
{
public:
	std::string_view serviceName()        override { return "FXFeedService"; };
	std::string_view serviceDescription() override { return "FX Feed Service which publishes canned Forex rates at a configurable frequency. "
														    "The rates are generated using a Brownian Bridge process calibrated to historical EOD rates and volatilities."; }

	void onStartup()  override;
	void onShutdown() override;

	void run() override;

	void registerConfigOptions( Cfg::ConfigWrangler& cfg ) override;

private:
	struct Config
	{
		std::string              streamSvcDSH       = "Kafka";
		int64_t                  publishFrequencyMs = 500;
		int64_t                  pathGenStepSizeMs  = 100;
		std::vector<std::string> publishCcys        = { "EUR", "GBP" };
	} m_config;

private:
	std::unique_ptr<MD::Publisher> m_publisher;
	std::unordered_map<std::string, std::unique_ptr<Math::Stochastic::BrownianBridgePathGenerator>> m_pathGenerators;
};