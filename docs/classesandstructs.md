# Classes and Structs

OS supports both structs and classes for object-oriented programming, with structs being simpler data containers and classes providing full OOP features including constructors, destructors, and access control.

## Structs

Structs are lightweight data containers that can hold data and methods.

### Basic Struct Definition
```os
struct Vector3 {
    x: f32 = 0;
    y: f32 = 0;
    z: f32 = 0;
}
```

### Struct with Methods
```os
struct Vector3 {
    x: f32 = 0;
    y: f32 = 0;
    z: f32 = 0;
    
    scale(x: f32 = 1, y: f32 = 1, z: f32 = 1) => void {
        this.x *= x;
        this.y *= y;
        this.z *= z;
    }
    
    magnitude() => f32 {
        return sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
    }
    
    normalize() => void {
        let mag = this.magnitude();
        if (mag > 0) {
            this.x /= mag;
            this.y /= mag;
            this.z /= mag;
        }
    }
}
```

### Struct Instantiation
```os
// Create with initializer list
let v1 = new Vector3{ x: 1.0, y: 2.0, z: 3.0 };

// Create with defaults
let v2 = new Vector3{};

// Direct instantiation (without new)
let v3 = Vector3{ x: 5.0, y: 0.0, z: -2.0 };
```

### Using Struct Methods
```os
let v1 = new Vector3{ x: 2.0, y: 3.0, z: 4.0 };

// Call methods
v1.scale(2.0, 1.0, 1.0);
v1.normalize();

// Access fields
let x_value = v1.x;
v1.y = 10.0;
```

## Classes

Classes provide full object-oriented programming capabilities with constructors, destructors, and access control.

### Basic Class Definition
```os
class Particle {
    private x: f32 = 0;
    private y: f32 = 0;
    private z: f32 = 0;
    
    constructor(x: f32 = 0, y: f32 = 0, z: f32 = 0) => void {
        this.x = x;
        this.y = y;
        this.z = z;
    }
    
    destructor() => void {
        // Cleanup code here
        printf("Particle destroyed\n");
    }
    
    public getX() => f32 {
        return this.x;
    }
    
    public getY() => f32 {
        return this.y;
    }
    
    public getZ() => f32 {
        return this.z;
    }
    
    public setPosition(x: f32, y: f32, z: f32) => void {
        this.x = x;
        this.y = y;
        this.z = z;
    }
    
    public move(dx: f32, dy: f32, dz: f32) => void {
        this.x += dx;
        this.y += dy;
        this.z += dz;
    }
}
```

### Class Instantiation
```os
// Create with constructor parameters
let p1 = new Particle(1.0, 2.0, 3.0);

// Create with default constructor
let p2 = new Particle();

// Alternative instantiation syntax
let p3 = Particle(5.0, 10.0, 15.0);
```

### Using Class Methods
```os
let particle = new Particle(0.0, 0.0, 0.0);

// Call public methods
particle.setPosition(10.0, 20.0, 30.0);
particle.move(1.0, 0.0, -1.0);

// Get values
let z_pos = particle.getZ();
```

## Access Control

Classes support access control modifiers:

### Access Modifiers
- `private`: Only accessible within the class
- `public`: Accessible from outside the class
- `protected`: Accessible within the class and derived classes (future feature)

```os
class Example {
    private internal_data: i32 = 0;
    public external_data: i32 = 0;
    
    private internal_method() => void {
        // Only accessible within the class
    }
    
    public external_method() => void {
        // Accessible from outside
        this.internal_method(); // Can call private methods
    }
}
```

## Constructors and Destructors

### Constructor Types
```os
class MyClass {
    private value: i32;
    
    // Default constructor
    constructor() => void {
        this.value = 0;
    }
    
    // Parameterized constructor
    constructor(initial_value: i32) => void {
        this.value = initial_value;
    }
    
    // Copy constructor (future feature)
    constructor(other: &MyClass) => void {
        this.value = other.value;
    }
    
    destructor() => void {
        printf("MyClass instance destroyed with value: %d\n", this.value);
    }
}
```

## The `this` Keyword

The `this` keyword refers to the current instance:

```os
struct Point {
    x: f32 = 0;
    y: f32 = 0;
    
    set(x: f32, y: f32) => void {
        this.x = x;  // Refers to the instance's x field
        this.y = y;  // Refers to the instance's y field
    }
    
    distance_to(other: &Point) => f32 {
        let dx = this.x - other.x;
        let dy = this.y - other.y;
        return sqrt(dx * dx + dy * dy);
    }
}
```

## Static Members (Future Feature)

```os
class Counter {
    private static count: i32 = 0;
    private id: i32;
    
    constructor() => void {
        Counter.count++;
        this.id = Counter.count;
    }
    
    public static get_count() => i32 {
        return Counter.count;
    }
    
    public get_id() => i32 {
        return this.id;
    }
}
```

## Inheritance (Future Feature)

```os
class Animal {
    protected name: char* = "";
    
    constructor(name: char*) => void {
        this.name = name;
    }
    
    public virtual speak() => void {
        printf("%s makes a sound\n", this.name);
    }
}

class Dog : public Animal {
    constructor(name: char*) : Animal(name) => void {
        // Additional initialization
    }
    
    public override speak() => void {
        printf("%s barks\n", this.name);
    }
}
```

## Method Overloading

Both structs and classes support method overloading:

```os
struct Calculator {
    add(a: i32, b: i32) => i32 {
        return a + b;
    }
    
    add(a: f32, b: f32) => f32 {
        return a + b;
    }
    
    add(a: i32, b: i32, c: i32) => i32 {
        return a + b + c;
    }
}
```

## Memory Management

### Stack Allocation
```os
let local_particle = Particle(1.0, 2.0, 3.0);  // Stack allocated
```

### Heap Allocation
```os
let heap_particle = new Particle(1.0, 2.0, 3.0);  // Heap allocated
// Note: Manual memory management required
```

### Explicit Heap Allocation
```os
let heap_particle = new heap Particle(1.0, 2.0, 3.0);  // Explicitly heap allocated
```

## Best Practices

1. **Use structs for simple data containers** without complex behavior
2. **Use classes for objects with behavior** and state management
3. **Always initialize member variables** with default values
4. **Make member variables private** unless they need external access
5. **Provide public methods** for controlled access to private data
6. **Use constructors** to ensure proper initialization
7. **Implement destructors** for cleanup when needed
8. **Use meaningful names** for classes, structs, and methods

## Example: Complete Class Implementation

```os
class BankAccount {
    private balance: f64 = 0.0;
    private account_number: char* = "";
    private is_active: bool = true;
    
    constructor(account_num: char*, initial_balance: f64 = 0.0) => void {
        this.account_number = account_num;
        this.balance = initial_balance;
        this.is_active = true;
    }
    
    destructor() => void {
        printf("Account %s closed\n", this.account_number);
    }
    
    public deposit(amount: f64) => bool {
        if (!this.is_active || amount <= 0) {
            return false;
        }
        this.balance += amount;
        return true;
    }
    
    public withdraw(amount: f64) => bool {
        if (!this.is_active || amount <= 0 || amount > this.balance) {
            return false;
        }
        this.balance -= amount;
        return true;
    }
    
    public get_balance() => f64 {
        return this.balance;
    }
    
    public close_account() => void {
        this.is_active = false;
    }
}
```

---

*Class and struct features are actively being developed and may change.*