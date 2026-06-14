using ARQ.Gateway.RefData.Repositories;
using ARQ.RD;
using System.Diagnostics;

namespace ARQ.Gateway.RefData.Endpoints;

public static class RefDataEndpoints
{
    public static IEndpointRouteBuilder MapRefDataEndpoints(this IEndpointRouteBuilder builder)
    {
        var grp = builder.MapGroup("/api/refdata/");

        grp.MapGet(
            "records/{entityType}",
            GetRecords);

        grp.MapGet(
            "records/{entityType}/{id}",
            GetRecord);

        grp.MapGet(
            "entities",
            GetEntities);

        grp.MapGet(
            "metadata/{entityType}",
            GetMetadata);

        return builder;
    }

    private static IResult GetRecords(string entityType, IRefDataRepository repo)
    {
        ICache? cache = repo.getCache(entityType);
        if (cache == null)
            return Results.NotFound();
    
        var records = cache.getList();
        return Results.Ok(records);
    }

    private static IResult GetRecord(string entityType, string id, IRefDataRepository repo)
    {
        ICache? cache = repo.getCache(entityType);
        if (cache == null)
            return Results.NotFound(new { Error = $"Reference data entity '{entityType}' is not supported." });

        var guid = Guid.TryParse(id, out var parsedGuid) ? parsedGuid : Guid.Empty;
        var record = cache.getRecord(guid);

        if (record == null)
            return Results.NotFound(new { Error = $"Record with ID '{id}' not found." });

        return Results.Ok(record);
    }

    private static IResult GetMetadata(string entityType, IRefDataMetaRepository repo)
    {
        try
        {
            var memberInfos = repo.GetRecordMemberInfosForEntity(entityType);
            return Results.Ok(memberInfos);
        }
        catch (ArgumentException ex)
        {
            return Results.NotFound(new { Error = ex.Message });
        }
    }

    private static IResult GetEntities(IRefDataMetaRepository repo)
    {
        var entityTypes = repo.GetEntityTypes();
        return Results.Ok(entityTypes);
    }
}