using ARQLib.Extensions;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using ARQ = ARQLib.ARQ;

public class MktDataUpdater : IDisposable
{
    private readonly ARQ.Mkt.ManagedMarket _managedMarket;
    private readonly CancellationTokenSource _cancellationTokenSource;
    private readonly Task _updateTask;
    private readonly Random _random = new Random();
    private volatile bool _disposed = false;

    public MktDataUpdater(ARQ.Mkt.ManagedMarket managedMarket)
    {
        _managedMarket = managedMarket ?? throw new ArgumentNullException(nameof(managedMarket));
        _cancellationTokenSource = new CancellationTokenSource();

        // Start the update task
        _updateTask = Task.Run(() => UpdateLoop(_cancellationTokenSource.Token));
    }

    private async Task UpdateLoop(CancellationToken cancellationToken)
    {
        Console.WriteLine("Market data updater started...");

        // Currency pairs to update
        var currencyPairs = new[]
        {
            "EURUSD", "GBPUSD", "USDJPY", "USDCHF", "USDCAD",
            "AUDUSD", "NZDUSD", "EURGBP", "EURJPY", "GBPJPY"
        };

        // Initial base rates (realistic-ish values)
        var baseRates = new Dictionary<string, double>
        {
            { "EURUSD", 1.0850 }, { "GBPUSD", 1.2650 }, { "USDJPY", 149.50 },
            { "USDCHF", 0.8750 }, { "USDCAD", 1.3450 }, { "AUDUSD", 0.6550 },
            { "NZDUSD", 0.6150 }, { "EURGBP", 0.8580 }, { "EURJPY", 162.20 },
            { "GBPJPY", 189.10 }
        };

        try
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                // Calculate how many pairs to update (20% of total)
                var updateCount = Math.Max(1, (int)Math.Ceiling(currencyPairs.Length * 0.2));

                // Randomly select 20% of the currency pairs to update
                var pairsToUpdate = currencyPairs
                    .OrderBy(x => _random.Next())
                    .Take(updateCount)
                    .ToArray();

                Console.WriteLine($"[MktDataUpdater] Updating {pairsToUpdate.Length} of {currencyPairs.Length} pairs: [{string.Join(", ", pairsToUpdate)}]");

                // Update only the selected currency pairs
                foreach (var pair in pairsToUpdate)
                {
                    if (cancellationToken.IsCancellationRequested)
                        break;

                    try
                    {
                        using var rate = CreateRandomRate(pair, baseRates[pair]);
                        _managedMarket.onFXRateUpdate(rate);

                        Console.WriteLine($"Updated {pair}: bid={rate.bid:F4}, ask={rate.ask:F4}, mid={rate.mid:F4}");
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Error updating {pair}: {ex.Message}");
                    }
                }

                // Wait for quarter second (250ms) before next update cycle
                await Task.Delay(250, cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
            Console.WriteLine("Market data updater cancelled.");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Market data updater error: {ex.Message}");
        }
        finally
        {
            Console.WriteLine("Market data updater stopped.");
        }
    }

    private ARQ.MDEntities.FXRate CreateRandomRate(string currencyPair, double baseRate)
    {
        var rate = new ARQ.MDEntities.FXRate();

        // Add some random variation (±0.5%)
        var variation = (_random.NextDouble() - 0.5) * 0.01;
        var currentRate = baseRate * (1 + variation);

        // Create bid/ask spread (typically 0.1-0.3%)
        var spread = baseRate * (0.001 + _random.NextDouble() * 0.002);

        rate.ID = currencyPair;
        rate.source = "MARKET_SIMULATOR";
        rate.bid = currentRate - spread / 2;
        rate.ask = currentRate + spread / 2;
        rate.mid = (rate.bid + rate.ask) / 2.0;
        rate.asofTs = DateTime.UtcNow.ToARQDateTime();
        rate._isActive = true;
        rate._lastUpdatedBy = "MarketDataUpdater";
        rate._lastUpdatedTs = DateTime.UtcNow.ToARQDateTime();

        return rate;
    }

    public void Stop()
    {
        _cancellationTokenSource?.Cancel();
    }

    public async Task WaitForCompletion()
    {
        if (_updateTask != null)
        {
            await _updateTask;
        }
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            _cancellationTokenSource?.Cancel();

            try
            {
                _updateTask?.Wait(TimeSpan.FromSeconds(5));
            }
            catch (AggregateException)
            {
                // Task was cancelled, which is expected
            }

            _cancellationTokenSource?.Dispose();
            _disposed = true;
        }
    }
}