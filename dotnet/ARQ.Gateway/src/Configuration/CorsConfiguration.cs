using Microsoft.AspNetCore.Cors.Infrastructure;
using System.Runtime.CompilerServices;

namespace ARQ.Gateway.Configuration;

public class CorsSettings
{
    public Dictionary<string, string[]> Policies { get; set; } = [];
}

public static class CorsConfiguration
{
    public static IServiceCollection AddCorsPolicies(this IServiceCollection services, IConfiguration configuration)
    {
        var corsSettings = configuration
            .GetSection("Cors")
            .Get<CorsSettings>();

        services.AddCors(options =>
        {
            if (corsSettings?.Policies != null)
            {
                foreach (var (policyName, origins) in corsSettings.Policies)
                {
                    options.AddPolicy(policyName, builder =>
                    {
                        builder
                            .WithOrigins(origins)
                            .AllowAnyHeader()
                            .AllowAnyMethod();
                    });
                }
            }
        });

        return services;
    }
}