namespace ARQ.Gateway.Configuration;

public class ARQLibSettings
{
    public string Env { get; set; } = "DEFAULT";
    public string LogLevel { get; set; } = "DEBUG";
    public string LogLevel2 { get; set; } = "DEBUG";
    public string? LogDest { get; set; }
    public string LogDest2 { get; set; } = "none";
}

public static class ARQConfiguration
{
    public static ARQ.LibConfig Create(IConfiguration config)
    {
        var settings = config
            .GetSection("ARQLib")
            .Get<ARQLibSettings>() ?? new ARQLibSettings();

        return new ARQ.LibConfig
        {
            Env = settings.Env,
            LogLevel = Enum.Parse<ARQ.LogLevel>(settings.LogLevel),
            LogLevel2 = Enum.Parse<ARQ.LogLevel>(settings.LogLevel2),
            LogDest = settings.LogDest ?? $"{ARQ.Sys.logDir()}/ARQ.Gateway.lib.log",
            LogDest2 = settings.LogDest2
        };
    }
}