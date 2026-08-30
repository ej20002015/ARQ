using ARQ.Gateway.Configuration;
using Microsoft.Extensions.Configuration;

namespace ARQ.Gateway.Tests.Configuration;

public class ARQConfigurationTests
{
    [Fact]
    public void CreateMapsConfiguredLibrarySettings()
    {
        var configuration = new ConfigurationBuilder()
            .AddInMemoryCollection(new Dictionary<string, string?>
            {
                ["ARQLib:Env"] = "UAT",
                ["ARQLib:LogLevel"] = "INFO",
                ["ARQLib:LogLevel2"] = "WARN",
                ["ARQLib:LogDest"] = "gateway.log",
                ["ARQLib:LogDest2"] = "audit.log"
            })
            .Build();

        var result = ARQConfiguration.Create(configuration);

        Assert.Equal("UAT", result.Env);
        Assert.Equal(ARQ.LogLevel.INFO, result.LogLevel);
        Assert.Equal(ARQ.LogLevel.WARN, result.LogLevel2);
        Assert.Equal("gateway.log", result.LogDest);
        Assert.Equal("audit.log", result.LogDest2);
    }

    [Fact]
    public void CreateUsesSettingsDefaultsForUnspecifiedValues()
    {
        var configuration = new ConfigurationBuilder()
            .AddInMemoryCollection(new Dictionary<string, string?>
            {
                ["ARQLib:LogDest"] = "none"
            })
            .Build();

        var result = ARQConfiguration.Create(configuration);

        Assert.Equal("DEFAULT", result.Env);
        Assert.Equal(ARQ.LogLevel.DEBUG, result.LogLevel);
        Assert.Equal(ARQ.LogLevel.DEBUG, result.LogLevel2);
        Assert.Equal("none", result.LogDest);
        Assert.Equal("none", result.LogDest2);
    }
}
