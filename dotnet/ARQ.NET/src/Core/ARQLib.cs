using System;
using System.Collections.Generic;

namespace ARQ;

public static class ARQLib
{
    /// <summary>
    /// Initializes the ARQ library with the provided configuration. If no configuration is provided, it will use default settings.
    /// </summary>
    /// <remarks>
    /// Return value must be wrapped in a 'using' statement to guarantee safe shutdown.<br/>
    /// Must be called before using any other ARQ library functions.
    /// </remarks>
    /// <param name="config">The configuration for the ARQ library.</param>
    /// <returns>A guard object for managing the lifecycle of the ARQ library - Must be disposed to shut down the library.</returns>
    public static LibGuard Init(LibConfig? config = null)
    {
        var args = new List<string>();
        
        // argv[0] is conventionally the program name in C/C++
        args.Add( "ARQ.NET" );

        if (!string.IsNullOrWhiteSpace(config?.Env))
        {
            args.Add("--env");
            args.Add(config.Env);
        }
        if (config?.LogLevel.HasValue == true)
        {
            args.Add("--log.level");
            args.Add(config.LogLevel.Value.ToString().ToUpperInvariant());
        }
        if (config?.LogLevel2.HasValue == true)
        {
            args.Add("--log.level2");
            args.Add(config.LogLevel2.Value.ToString().ToUpperInvariant());
        }
        if (!string.IsNullOrWhiteSpace(config?.LogDest))
        {
            args.Add("--log.dest");
            args.Add(config.LogDest);
        }
        if (!string.IsNullOrWhiteSpace(config?.LogDest2))
        {
            args.Add("--log.dest2");
            args.Add(config.LogDest2);
        }
        if (!string.IsNullOrWhiteSpace(config?.Home))
        {
            args.Add("--home");
            if (config.Home == "<default>")
            {
                var libLocations = ARQDLLResolver.GetARQLibLocations();
                args.Add(libLocations.TargetHomeDir);
            }
            else
                args.Add(config.Home);
        }

        var vec = new Collections.StringVector(args);
        return new LibGuard(vec);
    }
}