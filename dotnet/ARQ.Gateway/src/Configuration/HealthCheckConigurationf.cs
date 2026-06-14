namespace ARQ.Gateway.Configuration;

public static class HealthCheckConfiguration
{
    public static IServiceCollection AddAppHealthChecks(this IServiceCollection services)
    {
        services.AddHealthChecks();
        return services;
    }

    public static WebApplication UseAppHealthChecks(this WebApplication app)
    {
        // Map the standard Kubernetes probes
        app.MapHealthChecks("/health/startup");
        app.MapHealthChecks("/health/ready");
        app.MapHealthChecks("/health/live");

        return app;
    }
}