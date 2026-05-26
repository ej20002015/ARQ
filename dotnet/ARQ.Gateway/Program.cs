using ARQ.RD;

var builder = WebApplication.CreateBuilder(args);

var arqCfg = new ARQ.LibConfig{
    Env = "DEV",
    LogLevel = ARQ.LogLevel.DEBUG,
    LogDest = $"{ARQ.Sys.logDir()}/ARQ.Gateway.lib.log",
    LogDest2 = "none"
};

using var arq = ARQ.ARQLib.Init(arqCfg);

builder.Services.AddSingleton(provider => new ARQ.RD.Repository("ClickHouseDB"));

var app = builder.Build();

app.MapGet("/api/refdata/{entityType}", (string entityType, ARQ.RD.Repository repo) =>
{
    ICache? cache = repo.getByName(entityType);
    if (cache == null)
        return Results.NotFound();
    
    var records = cache.getList();
    return Results.Ok(records);
});

app.MapGet("/api/refdata/{entityType}/{id}", (string entityType, string id, Repository repo) =>
{
    ICache? cache = repo.getByName(entityType);
    if (cache == null)
        return Results.NotFound(new { Error = $"Reference data entity '{entityType}' is not supported." });

    var guid = Guid.TryParse(id, out var parsedGuid) ? parsedGuid : Guid.Empty;
    var record = cache.getRecord(guid);

    if (record == null) return Results.NotFound(new { Error = $"Record with ID '{id}' not found." });

    return Results.Ok(record);
});

app.Run();