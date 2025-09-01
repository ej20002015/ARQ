using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using ARQLib.Extensions;
using ARQ = ARQLib.ARQ;

namespace ARQDesktop.ViewModels;

public partial class MainWindowViewModel : BaseViewModel, INotifyPropertyChanged, IDisposable
{
    private readonly ARQ.Mkt.ManagedMarket _managedMarket;
    private readonly Timer? _statusUpdateTimer;
    private readonly CancellationTokenSource _cancellationTokenSource;
    private string _title = "FX Market Data";
    private string _statusText = "Initializing...";
    private bool _disposed = false;
    private int _updateCount = 0;

    public MainWindowViewModel(ARQ.Mkt.ManagedMarket managedMarket)
    {
        _managedMarket = managedMarket ?? throw new ArgumentNullException(nameof(managedMarket));
        _cancellationTokenSource = new CancellationTokenSource();

        FXRates = new ObservableCollection<FXRateViewModel>();

        // Update status every second (but don't update rates - that's done via callback now)
        _statusUpdateTimer = new Timer(UpdateStatus, null, TimeSpan.Zero, TimeSpan.FromSeconds(1));

        StatusText = "Market data loading via callbacks...";
    }

    public ObservableCollection<FXRateViewModel> FXRates { get; }

    public string Title
    {
        get => _title;
        set => SetProperty(ref _title, value);
    }

    public string StatusText
    {
        get => _statusText;
        set => SetProperty(ref _statusText, value);
    }

    /// <summary>
    /// Called by MktSubscriber when an FX rate is updated via callback
    /// This method is already dispatched to the UI thread
    /// </summary>
    public void OnFXRateUpdated(ARQ.MDEntities.FXRate updatedRate)
    {
        try
        {
            Console.WriteLine($"[MainWindowViewModel] Processing callback for {updatedRate.ID}");
            
            // Find existing rate or create new one
            var existingRate = FXRates.FirstOrDefault(r => r.CurrencyPair == updatedRate.ID);
            if (existingRate != null)
            {
                // Update existing rate - this will trigger the pulse animation
                existingRate.UpdateFromFXRate(updatedRate);
                Console.WriteLine($"[MainWindowViewModel] Updated existing rate {updatedRate.ID}");
            }
            else
            {
                // Add new rate
                var newRate = new FXRateViewModel();
                newRate.UpdateFromFXRate(updatedRate);
                FXRates.Add(newRate);
                Console.WriteLine($"[MainWindowViewModel] Added new rate {updatedRate.ID}");
            }

            _updateCount++;
            
            // Update "time ago" display for all rates
            foreach (var rate in FXRates)
            {
                rate.OnPropertyChanged(nameof(rate.LastUpdatedDisplay));
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[MainWindowViewModel] Error processing FX rate update: {ex.Message}");
            StatusText = $"Callback Error: {ex.Message}";
        }
    }

    private void UpdateStatus(object? state)
    {
        if (_disposed || _cancellationTokenSource.Token.IsCancellationRequested)
            return;

        // Update status on UI thread
        Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() =>
        {
            try
            {
                StatusText = $"Last Updated: {DateTime.Now:HH:mm:ss} - {FXRates.Count} rates active - {_updateCount} updates received via callbacks";
                    
                // Update "time ago" display for all rates
                foreach (var rate in FXRates)
                {
                    rate.OnPropertyChanged(nameof(rate.LastUpdatedDisplay));
                }
            }
            catch (Exception ex)
            {
                StatusText = $"Status Update Error: {ex.Message}";
            }
        });
    }

    public void Shutdown()
    {
        _cancellationTokenSource.Cancel();
        _statusUpdateTimer?.Dispose();
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            Shutdown();
            _cancellationTokenSource?.Dispose();
            _disposed = true;
        }
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    protected bool SetProperty<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (Equals(field, value)) return false;
        field = value;
        OnPropertyChanged(propertyName);
        return true;
    }
}