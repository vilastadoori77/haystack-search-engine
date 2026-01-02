import time
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed

URL = "http://localhost:8900/search?q=" + urllib.parse.quote("migration schema")
REQUESTS = 200          # total requests
CONCURRENCY = 50        # parallel requests at a time
TIMEOUT_SEC = 5

def hit(i: int):
    t0 = time.time()
    try:
        with urllib.request.urlopen(URL, timeout=TIMEOUT_SEC) as r:
            _ = r.read()  # discard body
            return (i, r.status, time.time() - t0, None)
    except Exception as e:
        return (i, None, time.time() - t0, str(e))

def main():
    print(f"URL: {URL}")
    print(f"REQUESTS={REQUESTS} CONCURRENCY={CONCURRENCY}")
    t_start = time.time()

    ok = 0
    fail = 0
    latencies = []

    with ThreadPoolExecutor(max_workers=CONCURRENCY) as ex:
        futures = [ex.submit(hit, i) for i in range(REQUESTS)]
        for f in as_completed(futures):
            i, status, dt, err = f.result()
            if status == 200:
                ok += 1
                latencies.append(dt)
            else:
                fail += 1
                print(f"[FAIL] #{i} status={status} time={dt:.3f}s err={err}")

    total = time.time() - t_start
    if latencies:
        latencies.sort()
        p50 = latencies[int(0.50 * (len(latencies)-1))]
        p95 = latencies[int(0.95 * (len(latencies)-1))]
        p99 = latencies[int(0.99 * (len(latencies)-1))]
        print(f"OK={ok} FAIL={fail} total={total:.2f}s rps={ok/total:.1f}")
        print(f"p50={p50:.3f}s p95={p95:.3f}s p99={p99:.3f}s")
    else:
        print(f"OK=0 FAIL={fail} total={total:.2f}s")

if __name__ == "__main__":
    main()
