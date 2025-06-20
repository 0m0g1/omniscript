package main

import (
    "fmt"
    "syscall"
    "unsafe"
)

var (
    k32  = syscall.NewLazyDLL("kernel32.dll")
    qpc  = k32.NewProc("QueryPerformanceCounter")
    qpf  = k32.NewProc("QueryPerformanceFrequency")
)

func queryPerformanceCounter() int64 {
    var v int64
    qpc.Call(uintptr(unsafe.Pointer(&v)))
    return v
}

func queryPerformanceFrequency() int64 {
    var v int64
    qpf.Call(uintptr(unsafe.Pointer(&v)))
    return v
}

func main() {
    freq := queryPerformanceFrequency()

    warmup := int64(0)
    warmupNoise := int64(0)
    for i := int64(0); i < 1_000_000; i++ {
        if i%1_000_000_001 == 0 {
            t := queryPerformanceCounter()
            warmupNoise ^= t
        }
        warmup += i
    }

    noise := int64(0)
    x := warmup ^ warmupNoise

    start := queryPerformanceCounter()
    for i := int64(0); i < 1_000_000_000; i++ {
        if i%1_000_000_001 == 0 {
            t := queryPerformanceCounter()
            noise ^= t
        }
        x += i
    }
    end := queryPerformanceCounter()

    x ^= noise

    elapsedMs := float64(end-start) * 1000.0 / float64(freq)
    fmt.Printf("Result: %d\n", x)
    fmt.Printf("Elapsed: %.4f ms\n", elapsedMs)
    fmt.Printf("Ops/ms: %.1f\n", 1_000_000.0/elapsedMs)
}
