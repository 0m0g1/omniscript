struct DynArray<T> {
    size: size_t = 0;
    capacity: size_t = 0;
    first: T*? = nullptr;
    
    // Constructor with initial capacity
    constructor(initial_capacity: size_t = 8) {
        this.capacity = initial_capacity;
        this.first = allocate<T>(this.capacity);
        this.size = 0;
    }
    
    // Destructor
    destructor() {
        if (this.first != nullptr) {
            deallocate(this.first);
        }
    }
    
    // Resize internal buffer when needed
    resize_if_needed() => void {
        if (this.size >= this.capacity) {
            const new_capacity = this.capacity * 2;
            T* new_buffer = allocate<T>(new_capacity);
            
            // Copy existing elements
            for (i: size_t = 0; i < this.size; ++i) {
                new_buffer[i] = this.first[i];
            }
            
            deallocate(this.first);
            this.first = new_buffer;
            this.capacity = new_capacity;
        }
    }
    
    append(item: T) => void {
        this.resize_if_needed();
        this.first[this.size] = item;
        ++this.size;
    }
    
    prepend(item: T) => void {
        this.resize_if_needed();
        
        // Shift all elements to the right
        for (i: size_t = this.size; i > 0; --i) {
            this.first[i] = this.first[i - 1];
        }
        
        this.first[0] = item;
        ++this.size;
    }
    
    push_back(item: T) => void { 
        this.append(item); 
    }
    
    push(item: T) => void { 
        this.prepend(item); 
    }
    
    pop() => T? {
        if (this.size == 0) {
            return null;
        }
        
        const result = this.first[0];
        
        // Shift all elements to the left
        for (i: size_t = 1; i < this.size; ++i) {
            this.first[i - 1] = this.first[i];
        }
        
        --this.size;
        return result;
    }
    
    pop_back() => T? {
        if (this.size == 0) {
            return null;
        }
        
        --this.size;
        return this.first[this.size];
    }
    
    // Check if array is empty
    is_empty() => bool {
        return this.size == 0;
    }
    
    // Get current size
    length() => size_t {
        return this.size;
    }
    
    // Clear all elements
    clear() => void {
        this.size = 0;
    }
}

// Operator overloads
DynArray<T> can + (item: T) => void {
    this.push_back(item);  // Fixed: pass the actual item, not the type
}

DynArray<T> can -- () => T? {
    return this.pop_back();  // Return the popped item
}

DynArray<T> can [] (index: size_t) => T? {
    if (index >= this.size) {  // Fixed: should be >= not >
        return null;
    }
    
    return this.first[index];  // Simplified: array access instead of pointer arithmetic
}

// Assignment operator for setting values
DynArray<T> can []= (index: size_t, value: T) => bool {
    if (index >= this.size) {
        return false;  // Index out of bounds
    }
    
    this.first[index] = value;
    return true;
}

// Iterator support (if the language supports it)
DynArray<T> can begin() => T* {
    return this.first;
}

DynArray<T> can end() => T* {
    return this.first + this.size;
}