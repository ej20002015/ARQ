using System.Text.Json.Serialization;

namespace ARQ.Gateway.Configuration;

public static class JsonConfiguration
{
    public static IServiceCollection AddJsonConfiguration(this IServiceCollection services)
    {
        services.Configure<Microsoft.AspNetCore.Http.Json.JsonOptions>(options =>
        {
            // Tells the serializer to convert all enums to their string representations
            options.SerializerOptions.Converters.Add(new JsonStringEnumConverter());
        });

        return services;
    }
}