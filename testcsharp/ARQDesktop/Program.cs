using ARQDesktop.Views;
using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Controls.Shapes;
using System;
using System.Reflection.Metadata;
using System.Threading;
using System.Threading.Tasks;
using ARQ = ARQLib.ARQ;
using ARQLib.Extensions;
using ARQDesktop.ViewModels;

namespace ARQDesktop
{
    public class MktSubscriber : ARQ.Mkt.VirtualSubscriber
    {
        private readonly MainWindowViewModel _viewModel;

        public MktSubscriber(string description, MainWindowViewModel viewModel) : base(description)
        {
            _viewModel = viewModel ?? throw new ArgumentNullException(nameof(viewModel));
        }

        public override void onFXRateUpdate(ARQ.MDEntities.FXRate updatedObj)
        {
            // Push the update to the UI on the UI thread
            ARQ.MDEntities.FXRate rateCopy = updatedObj.Clone();
            Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() =>
            {
                _viewModel.OnFXRateUpdated(rateCopy);
            });
        }
    }

    internal sealed class Program
    {
        private static MktDataUpdater? _updater;
        private static ARQ.Mkt.ManagedMarket? _managedMkt;
        private static MktSubscriber? _subscriber;
        private static MainWindowViewModel? _mainViewModel;

        // Initialization code. Don't use any Avalonia, third-party APIs or any
        // SynchronizationContext-reliant code before AppMain is called: things aren't initialized
        // yet and stuff might break.
        [STAThread]
        public static void Main(string[] args) => BuildAvaloniaApp()
            .StartWithClassicDesktopLifetime(args);

        // Avalonia configuration, don't remove; also used by visual designer.
        public static AppBuilder BuildAvaloniaApp()
            => AppBuilder.Configure<App>()
                .UsePlatformDetect()
                .WithInterFont()
                .LogToTrace()
                .AfterSetup(SetupApplication);

        private static void SetupApplication(AppBuilder builder)
        {
            if (Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
            {
                // Set up Ctrl+C handling and initialize market data
                var cancellationTokenSource = new CancellationTokenSource();

                // Initialize market data
                _managedMkt = new ARQ.Mkt.ManagedMarket("ClickHouseDB", ARQ.Mkt.Name.LIVE);

                // Create main window with view model FIRST
                _mainViewModel = new MainWindowViewModel(_managedMkt);
                desktop.MainWindow = new MainWindow(_mainViewModel);

                // Create market subscriber with reference to view model
                _subscriber = new MktSubscriber("DesktopAppSubscriber", _mainViewModel);
                _managedMkt.subscribe(_subscriber, ARQ.Mkt.ConsolidatingTIDSet.ALL_TYPES);

                // Start the market data updater on a background thread
                Console.WriteLine("Starting background market data updates...");
                _updater = new MktDataUpdater(_managedMkt);

                // Background task to periodically show market status (optional monitoring)
                var monitoringTask = Task.Run(async () =>
                {
                    while (!cancellationTokenSource.Token.IsCancellationRequested)
                    {
                        try
                        {
                            await Task.Delay(10000, cancellationTokenSource.Token); // Every 10 seconds

                            var snap = _managedMkt.snap();
                            var fxRates = snap.getAllFXRates();

                            Console.WriteLine($"[Background Monitor] {DateTime.Now:HH:mm:ss} - {fxRates.Count} rates active");
                        }
                        catch (OperationCanceledException)
                        {
                            break;
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine($"[Background Monitor] Error: {ex.Message}");
                        }
                    }
                });

                // Handle application shutdown
                desktop.ShutdownRequested += (sender, e) =>
                {
                    Console.WriteLine("Application shutdown requested...");
                    cancellationTokenSource.Cancel();

                    _mainViewModel?.Shutdown();

                    Console.WriteLine("Stopping background services...");
                    _updater?.Stop();

                    // Wait for background services to stop
                    Task.Run(async () =>
                    {
                        try
                        {
                            if (_updater != null)
                            {
                                await _updater.WaitForCompletion();
                                _updater.Dispose();
                            }
                            await monitoringTask;
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine($"Error during shutdown: {ex.Message}");
                        }
                    });
                };

                Console.WriteLine("Avalonia UI application started successfully!");
            }
        }
    }
}
