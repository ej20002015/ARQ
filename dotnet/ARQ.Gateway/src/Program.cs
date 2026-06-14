using ARQ.Gateway.Configuration;
using ARQ.Gateway.RefData;
using ARQ.Gateway.RefData.Endpoints;

/* ---------- App Builder ------------*/

var builder = WebApplication.CreateBuilder(args);

// Global services
builder.Services.AddJsonConfiguration();
builder.Services.AddCorsPolicies(builder.Configuration);

// Feature specific services
builder.Services.AddRefData();

/* ---------- App Runtime ------------*/

var app = builder.Build();

// ARQ library init
var arqCfg = ARQConfiguration.Create(builder.Configuration);
using var arq = ARQ.ARQLib.Init(arqCfg);

// Feature endpoint registration
app.MapRefDataEndpoints();

// Global config
app.UseCors("ARQWebLocal");

app.Run();