import time
import requests
import statistics

def run_benchmark():
    # Using a Session keeps the underlying TCP connection open.
    # This prevents the 3-way TCP handshake from skewing every request.
    session = requests.Session()
    url = "http://localhost:5147/api/refdata/user"
    
    print(f"Benchmarking: {url}\n")

    # 1. The Cold Start
    t0 = time.perf_counter()
    resp = session.get(url)
    cold_time = (time.perf_counter() - t0) * 1000
    
    if resp.status_code != 200:
        print(f"Error: API returned {resp.status_code}")
        return

    print(f"Cold Start (Wire + Handshake): {cold_time:.2f} ms")
    print(f"Server-Timing Header: {resp.headers.get('Server-Timing', 'None')}\n")

    # 2. The Hot Path
    iterations = 100
    times = []

    print(f"Firing {iterations} hot requests over open TCP connection...")
    for _ in range(iterations):
        t0 = time.perf_counter()
        # We read the response content to ensure the network stream is fully flushed
        _ = session.get(url).content 
        t_end = time.perf_counter()
        times.append((t_end - t0) * 1000)

    print(f"\n--- Results ({iterations} requests) ---")
    print(f"Hot Path Average: {statistics.mean(times):.2f} ms")
    print(f"Hot Path Min:     {min(times):.2f} ms")
    print(f"Hot Path Max:     {max(times):.2f} ms")

if __name__ == "__main__":
    run_benchmark()