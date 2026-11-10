struct DynArray<T> {
    size: size_t = 0;
    capacity: size_t = 0;
    first: unique_ptr<T[]> = null;
    
    // Constructor with initial capacity
    constructor(initial_capacity: size_t = 8) {
        this.capacity = initial_capacity;
        this.first = unique_array<T>(this.capacity);
        this.size = 0;
    }
    
    // Destructor
    destructor() {
        // Smart pointer handles cleanup automatically
        // No explicit deallocation needed
    }
    
    // Resize internal buffer when needed
    resize_if_needed() => void {
        if (this.size >= this.capacity) {
            const new_capacity = this.capacity * 2;
            unique_ptr<T[]> new_buffer = unique_array<T>(new_capacity);
            
            if (new_buffer == null) {
                // Handle allocation failure
                return;
            }
            
            // Copy existing elements
             for (i in 1...this.size - 1) {
                new_buffer[i] = this.first[i];
            }
            
            // Smart pointer automatically cleans up old buffer
            this.first = move(new_buffer);
            this.capacity = new_capacity;
        }
    }
    
    append(item: T) => void {
        this.resize_if_needed();
        if (this.first) {
            this.first[this.size] = item;
            ++this.size;
        }
    }
    
    prepend(item: T) => void {
        this.resize_if_needed();
        
        if (this.first) {
            // Shift all elements to the right
            for (i in this.size...1) {
                this.first[i] = this.first[i - 1];
            }
            
            this.first[0] = item;
            ++this.size;
        }
    }
    
    push_back(item: T) => void { 
        this.append(item); 
    }
    
    push(item: T) => void { 
        this.prepend(item); 
    }
    
    pop() => T? {
        if (this.size == 0 || !this.first) {
            return null;
        }
        
        const result = this.first[0];
        
        // Shift all elements to the left
        for (i in 1...this.size - 1) {
            this.first[i - 1] = this.first[i];
        }
        
        --this.size;
        return result;
    }
    
    pop_back() => T? {
        if (this.size == 0 || !this.first) {
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
    this.push_back(item);
}

DynArray<T> can -- () => T? {
    return this.pop_back();
}

DynArray<T> can [] (index: size_t) => T? {
    if (index >= this.size || !this.first) {
        return null;
    }
    
    return this.first[index];
}

// Assignment operator for setting values
DynArray<T> can []= (index: size_t, value: T) => bool {
    if (index >= this.size || !this.first) {
        return false;  // Index out of bounds or null buffer
    }
    
    this.first[index] = value;
    return true;
}

// Iterator support
DynArray<T> can begin() => T*? {
    return this.first.get();
}

DynArray<T> can end() => T*? {
    if (!this.first) {
        return null;
    }
    return this.first.get() + this.size;
}

// Usage examples with OS smart pointers
example_usage() => void {
    // Create a dynamic array using shared smart pointer
    arr: shared_ptr<DynArray<int>> = shared<DynArray<int>>(16);
    
    if (!arr == null) {
        // Add some elements
        arr.push_back(10);
        arr.push_back(20);
        arr.push_back(30);
        
        // Use operator overloads
        arr + 40;  // Add element
        
        // Access elements
        first_elem: int? = arr[0];  // Returns 10
        
        // Pop element
        last_elem: int? = arr--;  // Returns 40
        
        // Create another reference to the same array
        arr2: shared_ptr<DynArray<int>> = arr;  // Reference count increases
        
        // Both arr and arr2 can access the same data
        arr2.push_back(50);
        
        // Automatic cleanup when both references go out of scope
    }
    
    // Stack-allocated version with internal smart pointers
    stack_arr: DynArray<int> = DynArray<int>(8);
    stack_arr.push_back(1);
    stack_arr.push_back(2);
    // Smart pointer members handle internal memory cleanup automatically
}

// Unique pointer version for exclusive ownership
unique_example() => void {
    // Using unique pointer for single ownership
    arr: unique_ptr<DynArray<int>> = unique<DynArray<int>>(12);
    
    arr.push_back(100);
    arr.push_back(200);
    
    // Transfer ownership using move semantics
    moved_arr: unique_ptr<DynArray<int>> = move(arr);
    // arr is now null, moved_arr owns the resource
    
    if (!moved_arr == null) {
        moved_arr.push_back(300);
    }
    
    // Automatic cleanup when moved_arr goes out of scope
}

// Weak pointer example for breaking cycles
weak_example() => void {
    arr: shared_ptr<DynArray<int>> = shared<DynArray<int>>(5);
    
    // Create a weak reference that doesn't affect reference count
    weak_ref: weak_ptr<DynArray<int>> = arr.weak();
    
    // Check if the original object still exists
    if (strong_ref: shared_ptr<DynArray<int>> = weak_ref.lock(); !strong_ref == null) {
        strong_ref.push_back(42);
    }
    
    // When arr goes out of scope, weak_ref will become invalid
}