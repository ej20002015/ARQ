import urllib.request
import json
import csv
import math
from datetime import datetime, timedelta

def calc_std_dev(data):
    if len(data) < 2: return 0.0
    mean = sum(data) / len(data)
    variance = sum((x - mean) ** 2 for x in data) / (len(data) - 1)
    return math.sqrt(variance)

# https://api.frankfurter.dev/v2/rates?base=USD&quotes=EUR,GBP,JPY&from=2024-01-01&to=2026-01-01 in the browser to get the raw data

with open("raw_frankfurter_data.json", "r") as f:
    data = json.load(f)

# 2. Reorganize into a dictionary by currency
rates_by_ccy = {'EUR': {}, 'GBP': {}, 'JPY': {}}
for row in data:
    rates_by_ccy[row['quote']][row['date']] = row['rate']

today = datetime.utcnow().date()
EPOCH = datetime(1970, 1, 1).date()
ROLLING_WINDOW = 20

for ccy, rates in rates_by_ccy.items():
    if not rates: continue
        
    sorted_dates = sorted(rates.keys())
    
    # --- PHASE A: Calculate Returns and Volatility on TRADING DAYS only ---
    trading_data = [] # List of tuples: (date_str, price, daily_vol)
    
    # Store rolling log returns
    log_returns = []
    
    for i, date_str in enumerate(sorted_dates):
        raw_rate = rates[date_str]
        
        # Invert EUR and GBP to standard market convention
        price = 1.0 / raw_rate if ccy in ['EUR', 'GBP'] else raw_rate
        
        if i == 0:
            trading_data.append((date_str, price, 0.002)) # Default fallback vol for day 1
            continue
            
        # Calculate log return: ln(Price_today / Price_yesterday)
        prev_price = trading_data[-1][1]
        daily_return = math.log(price / prev_price)
        log_returns.append(daily_return)
        
        # Maintain rolling window
        if len(log_returns) > ROLLING_WINDOW:
            log_returns.pop(0)
            
        # Calculate volatility (standard deviation of daily returns)
        # If we don't have enough data yet, use what we have, or fallback to the previous day's vol
        if len(log_returns) >= 2:
            vol = calc_std_dev(log_returns)
        else:
            vol = trading_data[-1][2]
            
        trading_data.append((date_str, price, vol))
        
    # --- PHASE B: Map to Continuous Calendar and Shift to Present ---
    # Convert trading_data back to a dict for easy lookup
    trading_dict = {row[0]: {'price': row[1], 'vol': row[2]} for row in trading_data}
    
    start_date = datetime.strptime(sorted_dates[0], "%Y-%m-%d").date()
    max_date = datetime.strptime(sorted_dates[-1], "%Y-%m-%d").date()
    shift_days = (today - start_date).days / 2 # Shift halfway to the present for a more recent dataset
    
    filled_data = {}
    last_price = trading_data[0][1]
    last_vol = trading_data[0][2]
    
    curr_date = start_date
    while curr_date <= max_date:
        curr_date_str = curr_date.strftime("%Y-%m-%d")
        
        # Forward-fill if the date is missing (weekends/holidays)
        if curr_date_str in trading_dict:
            last_price = trading_dict[curr_date_str]['price']
            last_vol = trading_dict[curr_date_str]['vol']
            
            # Floor the vol to a minimum so the generator never completely freezes
            last_vol = max(last_vol, 0.0005) 
        
        shifted_date = curr_date + timedelta(days=shift_days)
        period_index = (shifted_date - EPOCH).days
        
        filled_data[period_index] = (last_price, last_vol)
        curr_date += timedelta(days=1)
        
    # --- PHASE C: Export ---
    filename = f"EOD_{ccy}USD.csv" if ccy in ['EUR', 'GBP'] else f"EOD_USD{ccy}.csv"
    with open(filename, "w", newline="") as f:
        writer = csv.writer(f)
        for p_idx, (p, v) in filled_data.items():
            writer.writerow([p_idx, round(p, 5), round(v, 6)])
            
    print(f"Saved {filename} with dynamically calculated daily volatility.")