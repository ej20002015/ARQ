#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQCore/streaming_service_interface.h>

#include <functional>
#include <memory>

namespace ARQ
{

class StreamProducerOptions
{
public:
	enum class Preset
	{
		DEFAULT,
		OPTIMISE_LATENCY,
		OPTIMISE_THROUGHPUT
	};

public:
	StreamProducerOptions( const Preset preset = Preset::DEFAULT )
		: m_preset( preset )
	{
	}

	void setOptionOverride( const std::string_view option, const std::string_view value )
	{
		m_optionOverrides.emplace( option, value );
	}

	[[nodiscard]] Preset getPreset() const
	{
		return m_preset;
	}
	[[nodiscard]] const std::unordered_map<std::string, std::string>& getOptionOverrides() const
	{
		return m_optionOverrides;
	}

public: // Static Presets
	ARQCore_API static const StreamProducerOptions OPTIMISE_LATENCY;
	ARQCore_API static const StreamProducerOptions OPTIMISE_THROUGHPUT;
	ARQCore_API static const StreamProducerOptions DEFAULT;

private:
	Preset                                       m_preset;
	std::unordered_map<std::string, std::string> m_optionOverrides;
};

using CreateStreamProducerFunc = std::add_pointer<IStreamProducer* ( const std::string_view dsh, const StreamProducerOptions& options )>::type;

class StreamingServiceFactory
{
public:
	[[nodiscard]] ARQCore_API static std::shared_ptr<IStreamProducer> createProducer( const std::string_view dsh, const StreamProducerOptions& options );

	// Not threadsafe but should only really be used on startup / for testing anyway
	ARQCore_API static void addCustomStreamProducer( const std::string_view dsh, const std::shared_ptr<IStreamProducer>& streamProducer );
	ARQCore_API static void delCustomStreamProducer( const std::string_view dsh );

private:
	static std::unordered_map<std::string, std::shared_ptr<IStreamProducer>, TransparentStringHash, std::equal_to<>> s_customStreamProducers;
};

}