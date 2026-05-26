using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace ARQ;

public static class ARQDLLResolver
{
    public struct ARQLibLocations
    {
        public string ResolvedPath { get; init; }
        public string TargetHomeDir { get; init; }
    }

    private static IntPtr _cachedEngineHandle = IntPtr.Zero;
    private static readonly object _globalLock = new object();
    private static ARQLibLocations _locations;

    public static ARQLibLocations GetARQLibLocations()
    {
        lock (_globalLock)
        {
            if (_locations.ResolvedPath != null)
                return _locations;
        }

        bool isWindows = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
        string dllName = "ARQLib_csharp.dll";

        string? resolvedPath = null;
        string? targetHomeDir = null;

        // In production environment ARQ_HOME should be set

        string? envArqHome = Environment.GetEnvironmentVariable("ARQ_HOME");
        if (!string.IsNullOrEmpty(envArqHome))
        {
            string prodPath = isWindows
                ? Path.Combine(envArqHome, "bin", dllName)
                : Path.Combine(envArqHome, "lib", dllName);

            if (!File.Exists(prodPath))
                throw new FileNotFoundException(
                    $"ARQ Engine binary ({dllName}) not found in ARQ_HOME. " +
                    $"Expected path: {prodPath}. Is ARQ_HOME set correctly?");

            resolvedPath = prodPath;
            targetHomeDir = envArqHome;
        }
        else
        {
            // Should only reach this point in development environment
            // We attempt to traverse the directory structure to find the root of the repo and then locate the built binaries based on the specified target (Debug/Release/Install)

            string baseDir = AppContext.BaseDirectory;
            DirectoryInfo? dir = new DirectoryInfo(baseDir);
            while (dir != null && dir.Name != "dotnet") { dir = dir.Parent; }

            if (dir == null)
                throw new Exception("ARQ_HOME is not set, and the local 'dotnet' repo folder could not be found!");

            if (dir.Parent == null)
                throw new Exception("ARQ_HOME is not set, and the local 'dotnet' repo folder does not have a parent directory!");

            string repoRoot = dir.Parent.FullName;

            string target = Environment.GetEnvironmentVariable("ARQ_LOCAL_TARGET") ?? "Release";

            if (target.Equals("Install", StringComparison.OrdinalIgnoreCase))
            {
                targetHomeDir = Path.Combine(repoRoot, ".install");
                resolvedPath = isWindows
                    ? Path.Combine(targetHomeDir, "bin", dllName)
                    : Path.Combine(targetHomeDir, "lib", dllName);
            }
            else // "Debug" or "Release"
            {
                targetHomeDir = Path.Combine(repoRoot, ".build");
                resolvedPath = isWindows
                    ? Path.Combine(targetHomeDir, "bin", target, dllName)
                    : Path.Combine(targetHomeDir, "lib", target, dllName);
            }

            if (!File.Exists(resolvedPath))
            {
                throw new FileNotFoundException(
                    $"ARQ Engine binary ({dllName}) not found for target '{target}'. " +
                    $"Expected path: {resolvedPath}. Did you run CMake for this target?");
            }
        }

        lock (_globalLock)
        {
            _locations = new ARQLibLocations
            {
                ResolvedPath = resolvedPath,
                TargetHomeDir = targetHomeDir
            };
        }

        return _locations;
    }

#pragma warning disable CA2255 // This is an advanced interop scenario explicitly requiring module initialization
    [ModuleInitializer]
    internal static void Initialize()
    {
        NativeLibrary.SetDllImportResolver(Assembly.GetExecutingAssembly(), ResolveARQLib);
    }
#pragma warning restore CA2255

    private static IntPtr ResolveARQLib(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
    {
        if (libraryName != "ARQLib_csharp")
            return IntPtr.Zero;

        lock (_globalLock)
        {
            if (_cachedEngineHandle != IntPtr.Zero)
                return _cachedEngineHandle;
        }

        var locations = GetARQLibLocations();
        bool isWindows = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);

        IntPtr handle = isWindows ?
            NativeLibrary.Load(locations.ResolvedPath, assembly, DllImportSearchPath.UseDllDirectoryForDependencies) :
            NativeLibrary.Load(locations.ResolvedPath, assembly, searchPath);

        if (isWindows)
        {
            string? libDir = Path.GetDirectoryName(locations.ResolvedPath);
            if (libDir == null)
                throw new Exception($"Failed to determine library directory from resolved path: {locations.ResolvedPath}");

            string currentPath = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
            if (!currentPath.Contains(libDir, StringComparison.OrdinalIgnoreCase))
                Environment.SetEnvironmentVariable("PATH", libDir + Path.PathSeparator + currentPath);
        }

        lock (_globalLock)
        {
            _cachedEngineHandle = handle;
        }

        return handle;
    }
}