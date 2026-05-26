namespace ARQ;

public enum LogLevel
{
    CRITICAL,
    ERRO,
    WARN,
    INFO,
    DEBUG,
    TRACE
}

public class LibConfig
{
    public string? Env { get; set; }
    public LogLevel? LogLevel { get; set; }
    public LogLevel? LogLevel2 { get; set; }
    public string? LogDest { get; set; } 
    public string? LogDest2 { get; set; }
    public string? Home { get; set;  } = "<default>";
}