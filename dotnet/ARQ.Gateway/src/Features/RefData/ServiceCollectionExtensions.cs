using ARQ.Gateway.RefData.Repositories;

namespace ARQ.Gateway.RefData;

public static class ServiceCollectionExtensions
{
    public static IServiceCollection AddRefData(this IServiceCollection services)
    {
        services.AddSingleton<IRefDataRepository>(_ => new RefDataRepository("ClickHouseDB"));
        services.AddSingleton<IRefDataMetaRepository, RefDataMetaRepository>();

        return services;
    }
}