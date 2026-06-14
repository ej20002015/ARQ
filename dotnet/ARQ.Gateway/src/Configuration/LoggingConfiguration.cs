using Microsoft.AspNetCore.HttpLogging;
using Serilog;
using Serilog.Events;

namespace ARQ.Gateway.Configuration;

public static class LoggingConfiguration
{
    public static WebApplicationBuilder AddLoggingConfiguration(this WebApplicationBuilder builder)
    {
        builder.Host.UseSerilog((context, loggerConfiguration) =>
        {
            loggerConfiguration.ReadFrom.Configuration(context.Configuration);
        });

        return builder;
    }

    public static WebApplication UseAppLogging(this WebApplication app)
    {
        app.UseSerilogRequestLogging(options =>
        {
            options.MessageTemplate = "HTTP {RequestMethod} {RequestPath} responded {StatusCode} in {Elapsed:0.0000} ms";

            options.GetLevel = (httpContext, elapsed, ex) =>
            {
                if (ex != null || httpContext.Response.StatusCode > 499)
                    return LogEventLevel.Error;

                if (httpContext.Request.Path.StartsWithSegments("/health"))
                    return LogEventLevel.Verbose; // Hide health checks from standard output

                return LogEventLevel.Information;
            };
        });

        return app;
    }
}