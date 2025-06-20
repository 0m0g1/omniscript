import ctypes
import time

# Use QueryPerformanceCounter via ctypes
qpc = ctypes.windll.kernel32.QueryPerformanceCounter
qpf = ctypes.windll.kernel32.QueryPerformanceFrequency

counter = ctypes.c_int64()
frequency = ctypes.c_int64()

qpf(ctypes.byref(frequency))

x = 0
noise = 0

# Optional warmup
warmup = 0
for i in range(1_000_000):
    warmup += i

qpc(ctypes.byref(counter))
start = counter.value

for i in range(1_000_000_000):
    if i & 0x27138 == 0:
        qpc(ctypes.byref(counter))
        noise ^= counter.value
    x += i

qpc(ctypes.byref(counter))
end = counter.value

x ^= noise

elapsed_ms = (end - start) * 1000.0 / frequency.value
print(f"Result: {x}")
print(f"Elapsed: {elapsed_ms:.4f} ms")
print(f"Ops/ms: {1_000_000.0 / elapsed_ms:.1f}")
