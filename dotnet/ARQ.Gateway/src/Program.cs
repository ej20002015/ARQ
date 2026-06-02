using ARQ.Gateway.RefData.Repositories;
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
builder.Services.AddSingleton<IRefDataMetaRepository, RefDataMetaRepository>();

builder.Services.AddCors(options =>
{
    options.AddPolicy("AllowARQWebLocal", policy =>
    {
        policy.WithOrigins("http://localhost:5173")
              .AllowAnyHeader()
              .AllowAnyMethod();
    });
});

var app = builder.Build();

app.MapGet("/api/refdata/records/{entityType}", (string entityType, ARQ.RD.Repository repo) =>
{
    ICache? cache = repo.getByName(entityType);
    if (cache == null)
        return Results.NotFound();
    
    var records = cache.getList();
    return Results.Ok(records);
});

app.MapGet("/api/refdata/records/{entityType}/{id}", (string entityType, string id, Repository repo) =>
{
    ICache? cache = repo.getByName(entityType);
    if (cache == null)
        return Results.NotFound(new { Error = $"Reference data entity '{entityType}' is not supported." });

    var guid = Guid.TryParse(id, out var parsedGuid) ? parsedGuid : Guid.Empty;
    var record = cache.getRecord(guid);

    if (record == null) return Results.NotFound(new { Error = $"Record with ID '{id}' not found." });

    return Results.Ok(record);
});

app.MapGet("/api/refdata/metadata/{entityType}", (string entityType, IRefDataMetaRepository repo) =>
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
});

app.MapGet("/api/refdata/entities", (IRefDataMetaRepository repo) =>
{
    var entityTypes = repo.GetEntityTypes();
    return Results.Ok(entityTypes);
});

app.UseCors("AllowARQWebLocal");
app.Run();