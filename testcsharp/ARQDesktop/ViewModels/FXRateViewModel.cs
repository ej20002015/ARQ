using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading;
using Avalonia.Media;
using ARQLib.Extensions;
using ARQ = ARQLib.ARQ;

namespace ARQDesktop.ViewModels;

public class FXRateViewModel : BaseViewModel, INotifyPropertyChanged
{
    private string _currencyPair = string.Empty;
    private double _bid;
    private double _ask;
    private double _mid;
    private double _spread;
    private string _source = string.Empty;
    private DateTime _lastUpdated;
    private bool _isActive;
    private IBrush _backgroundBrush = Brushes.Transparent;
    private double _previousMid;
    private Timer? _pulseTimer;

    public string CurrencyPair
    {
        get => _currencyPair;
        set => SetProperty(ref _currencyPair, value);
    }

    public double Bid
    {
        get => _bid;
        set => SetProperty(ref _bid, value);
    }

    public double Ask
    {
        get => _ask;
        set => SetProperty(ref _ask, value);
    }

    public double Mid
    {
        get => _mid;
        set
        {
            var oldMid = _mid;
            if (SetProperty(ref _mid, value))
            {
                if (oldMid != 0) // Don't pulse on initial load
                {
                    TriggerPulse(oldMid, value);
                }
                _previousMid = oldMid;
            }
        }
    }

    public double Spread
    {
        get => _spread;
        set => SetProperty(ref _spread, value);
    }

    public string Source
    {
        get => _source;
        set => SetProperty(ref _source, value);
    }

    public DateTime LastUpdated
    {
        get => _lastUpdated;
        set => SetProperty(ref _lastUpdated, value);
    }

    public string LastUpdatedDisplay => $"{(DateTime.UtcNow - LastUpdated).TotalSeconds:F1}s ago";

    public bool IsActive
    {
        get => _isActive;
        set => SetProperty(ref _isActive, value);
    }

    public IBrush BackgroundBrush
    {
        get => _backgroundBrush;
        set => SetProperty(ref _backgroundBrush, value);
    }

    private void TriggerPulse(double oldValue, double newValue)
    {
        _pulseTimer?.Dispose();

        // Determine pulse color based on price movement
        Color baseColor;
        if (newValue > oldValue)
        {
            baseColor = Color.FromArgb(180, 0, 255, 0); // Green
        }
        else if (newValue < oldValue)
        {
            baseColor = Color.FromArgb(180, 255, 0, 0); // Red
        }
        else
        {
            baseColor = Color.FromArgb(100, 128, 128, 128); // Grey
        }

        // Set initial pulse color
        BackgroundBrush = new SolidColorBrush(baseColor);

        // Create fade-out effect over 500ms
        var fadeSteps = 50; // Number of fade steps
        var fadeInterval = 10; // 25ms intervals (20 steps × 25ms = 500ms total)
        var currentStep = 0;
        var startAlpha = baseColor.A;

        _pulseTimer = new Timer(_ =>
        {
            currentStep++;

            if (currentStep >= fadeSteps)
            {
                // Animation complete - set to transparent
                BackgroundBrush = Brushes.Transparent;
                _pulseTimer?.Dispose();
                _pulseTimer = null;
            }
            else
            {
                // Calculate fade progress (0.0 to 1.0)
                var progress = (double)currentStep / fadeSteps;

                // Use easing function for smoother fade (ease-out)
                var easedProgress = 1 - Math.Pow(1 - progress, 2);

                // Calculate new alpha value
                var newAlpha = (byte)(startAlpha * (1 - easedProgress));

                // Create new color with faded alpha
                var fadedColor = Color.FromArgb(newAlpha, baseColor.R, baseColor.G, baseColor.B);

                // Update background brush on UI thread
                Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() =>
                {
                    BackgroundBrush = new SolidColorBrush(fadedColor);
                });
            }

        }, null, TimeSpan.FromMilliseconds(fadeInterval), TimeSpan.FromMilliseconds(fadeInterval));
    }

    public void UpdateFromFXRate(ARQ.MDEntities.FXRate rate)
    {
        CurrencyPair = rate.ID;
        Bid = rate.bid;
        Ask = rate.ask;
        Mid = rate.mid;
        Spread = rate.ask - rate.bid;
        Source = rate.source;
        LastUpdated = rate._lastUpdatedTs.ToDateTime();
        IsActive = rate._isActive;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public void OnPropertyChanged([CallerMemberName] string? propertyName = null)
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