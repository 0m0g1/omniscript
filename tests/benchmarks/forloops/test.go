package main

import (
	"fmt"
	"syscall"
	"unsafe"
)

var (
	kernel32                  = syscall.NewLazyDLL("kernel32.dll")
	procQueryPerformanceCounter = kernel32.NewProc("QueryPerformanceCounter")
	procQueryPerformanceFrequency = kernel32.NewProc("QueryPerformanceFrequency")
)

func queryPerformanceCounter() int64 {
	var counter int64
	procQueryPerformanceCounter.Call(uintptr(unsafe.Pointer(&counter)))
	return counter
}

func queryPerformanceFrequency() int64 {
	var freq int64
	procQueryPerformanceFrequency.Call(uintptr(unsafe.Pointer(&freq)))
	return freq
}

func main() {
	freq := queryPerformanceFrequency()

	var x int64 = 0
	var noise int64 = 0

	// Optional warmup
	var warmup int64 = 0
	for i := int64(0); i < 1_000_000; i++ {
		warmup += i
	}

	start := queryPerformanceCounter()

	for i := int64(0); i < 1_000_000_000; i++ {
		if i&0x27138 == 0 {
			temp := queryPerformanceCounter()
			noise ^= temp
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
