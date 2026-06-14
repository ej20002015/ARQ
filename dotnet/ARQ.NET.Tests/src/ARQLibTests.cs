namespace ARQ.NET.Tests;

public class ARQLibTests
{
    [Fact]
    public void LibInitialisationSucceeds()
    {
        var exception = Record.Exception(() =>
        {
            var config = new ARQ.LibConfig
            {
                LogDest = "None",
                LogDest2 = "None",
            };

            using (var engine = ARQ.ARQLib.Init(config))
            {
                // Run a safe function with no side-effects to make sure interop is working
                string logDir = ARQ.Sys.logDir();
            }
        });

        Assert.Null(exception); // Proves the DLL loaded and didn't throw
    }
}
