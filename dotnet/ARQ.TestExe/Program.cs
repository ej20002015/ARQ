var config = new ARQ.LibConfig
{
    Env = "DEV",
    LogLevel = ARQ.LogLevel.DEBUG,
    LogDest = "C:\\tmp\\ARQ\\log\\ARQ.NET.Tests.log"
};

// The 'using' statement binds the C++ engine lifetime to this scope
using (var engine = ARQ.ARQLib.Init(config))
{
    Console.WriteLine("ARQ Engine is running natively.");

    using var repo = new ARQ.RD.Repository("ClickHouseDB");
    using var users = repo.getUsers();

    using var list = users.getList();
    List<ARQ.RD.UserRecord> userList = list.ToList();

    int y = 0;

} // <- engine.Dispose() is called here, triggering ~LibGuard() in C++

Console.WriteLine("ARQ Engine cleanly shut down.");
