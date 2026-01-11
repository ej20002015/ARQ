#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/hashers.h>
#include <ARQCore/streaming_service_interface.h>

#include <functional>
#include <memory>

namespace ARQ
{

class StreamOptions
{
public:
	StreamOptions( const std::string_view name )
		: m_name( name )
	{
	}

	void setOptionOverride( const std::string_view option, const std::string_view value ) { m_optionOverrides.emplace( option, value ); }

	[[nodiscard]] const std::unordered_map<std::string, std::string>& optionOverrides() const { return m_optionOverrides; }
	[[nodiscard]] const std::string&                                  name()            const { return m_name; }

private:
	std::unordered_map<std::string, std::string> m_optionOverrides;
	std::string m_name;
};

class StreamProducerOptions : public StreamOptions
{
public:
	enum class Preset
	{
		Standard,
		LowLatency,
		HighThroughput,

		DEFAULT = Standard
	};

public:
	StreamProducerOptions( const std::string_view name,
						   const Preset preset = Preset::Standard )
		: StreamOptions( name )
		, m_preset( preset )
	{
	}

	[[nodiscard]] Preset preset() const { return m_preset; }

private:
	Preset m_preset;
};

class StreamConsumerOptions : public StreamOptions
{
public:
	// Controls how we pull data from the broker (Network/Buffering)
	enum class FetchPreset
	{
		Standard,       // Balanced (Default)
		LowLatency,     // Aggressive fetching, min buffering (e.g., Realtime Trading)
		HighThroughput, // Max buffering, large batches (e.g., Archiving/Analytics)

		DEFAULT = Standard
	};

	// Controls whether offsets are committed automatically in the background
	enum class AutoCommitOffsets
	{
		Enabled,
		Disabled, // Disable when using manual offset management

		DEFAULT = Enabled
	};

	// Controls where we start if no committed offset exists
	enum class AutoOffsetReset
	{
		Latest,   // Start from the end of the stream
		Earliest, // Start from the beginning of the stream
		None,     // If no offset exists, do not start

		DEFAULT = Latest
	};

public:
	StreamConsumerOptions( const std::string_view name,
						   const std::string_view groupID,
						   const FetchPreset fetchPreset = FetchPreset::DEFAULT,
						   const AutoCommitOffsets autoCommitOffsets = AutoCommitOffsets::DEFAULT,
						   const AutoOffsetReset autoOffsetReset = AutoOffsetReset::DEFAULT )
		: StreamOptions( name )
		, m_groupID( groupID )
		, m_fetchPreset( fetchPreset )
		, m_autoCommitOffsets( autoCommitOffsets )
		, m_autoOffsetReset( autoOffsetReset )
	{
	}

	[[nodiscard]] const std::string& groupID()           const { return m_groupID; }
	[[nodiscard]] FetchPreset        fetchPreset()       const { return m_fetchPreset; }
	[[nodiscard]] AutoOffsetReset    autoOffsetReset()   const { return m_autoOffsetReset; }
	[[nodiscard]] AutoCommitOffsets  autoCommitOffsets() const { return m_autoCommitOffsets; }

private:
	std::string       m_groupID;
	FetchPreset	      m_fetchPreset;
	AutoCommitOffsets m_autoCommitOffsets;
	AutoOffsetReset   m_autoOffsetReset;
};

using CreateStreamProducerFunc = std::add_pointer<IStreamProducer* ( const std::string_view dsh, const StreamProducerOptions& options )>::type;
using CreateStreamConsumerFunc = std::add_pointer<IStreamConsumer* ( const std::string_view dsh, const StreamConsumerOptions& options )>::type;

class StreamingServiceFactory
{
public:
	[[nodiscard]] ARQCore_API static std::shared_ptr<IStreamProducer> createProducer( const std::string_view dsh, const StreamProducerOptions& options );
	[[nodiscard]] ARQCore_API static std::shared_ptr<IStreamConsumer> createConsumer( const std::string_view dsh, const StreamConsumerOptions& options );

	// Not threadsafe but should only really be used on startup / for testing anyway
	ARQCore_API static void addCustomStreamProducer( const std::string_view dsh, const std::shared_ptr<IStreamProducer>& streamProducer );
	ARQCore_API static void delCustomStreamProducer( const std::string_view dsh );
	ARQCore_API static void addCustomStreamConsumer( const std::string_view dsh, const std::shared_ptr<IStreamConsumer>& streamConsumer );
	ARQCore_API static void delCustomStreamConsumer( const std::string_view dsh );

private:
	static std::unordered_map<std::string, std::shared_ptr<IStreamProducer>, TransparentStringHash, std::equal_to<>> s_customStreamProducers;
	static std::unordered_map<std::string, std::shared_ptr<IStreamConsumer>, TransparentStringHash, std::equal_to<>> s_customStreamConsumers;
};

}