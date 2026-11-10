module Memory {
    // External C functions
    extern "C" fn malloc(size: usize) => pointer<void>;
    extern "C" fn free(ptr: pointer<void>) => void;
    extern "C" fn realloc(ptr: pointer<void>, size: usize) => pointer<void>;
    extern "C" fn calloc(count: usize, size: usize) => pointer<void>;

    // Custom high-level wrappers
    public fn alloc<T>(count: usize = 1) => pointer<T> {
        return malloc(count * sizeof<T>()) as pointer<T>;
    }

    public fn dealloc<T>(ptr: pointer<T>) => void {
        free(ptr as pointer<void>);
    }

    public fn delete<T>(ptr: pointer<T>) => void {
        dealloc(ptr);
    }

    // Sizeof intrinsic (assuming built-in)
    private intrinsic fn sizeof<T>() => usize;
}
