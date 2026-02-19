# Control Flow

OS provides standard control flow constructs including conditional statements, loops, and branching mechanisms.

## Conditional Statements

### If Statements

#### Basic If Statement
```os
let temperature: int = 25;

if (temperature > 30) {
    printf("It's hot outside!\n");
}
```

#### If-Else Statement
```os
let age: int = 18;

if (age >= 18) {
    printf("You are an adult\n");
} else {
    printf("You are a minor\n");
}
```

#### If-Else If Chain
```os
let grade: int = 85;

if (grade >= 90) {
    printf("Grade: A\n");
} else if (grade >= 80) {
    printf("Grade: B\n");
} else if (grade >= 70) {
    printf("Grade: C\n");
} else if (grade >= 60) {
    printf("Grade: D\n");
} else {
    printf("Grade: F\n");
}
```

#### Ternary Operator
```os
let x: int = 10;
let y: int = 20;
let max = (x > y) ? x : y;

let status: char* = (age >= 18) ? "adult" : "minor";
```

### Boolean Expressions

```os
let a: int = 5;
let b: int = 10;
let c: int = 15;

// Logical AND
if (a > 0 && b > 0) {
    printf("Both are positive\n");
}

// Logical OR
if (a < 0 || b < 0) {
    printf("At least one is negative\n");
}

// Logical NOT
if (!(a > b)) {
    printf("a is not greater than b\n");
}

// Complex conditions
if ((a > 0 && b > 0) || c > 20) {
    printf("Complex condition met\n");
}
```

## Loops

### For Loops

#### Basic For Loop
```os
for (let i = 0; i < 10; i++) {
    printf("Iteration: %d\n", i);
}
```

#### For Loop with Different Step
```os
for (let i = 0; i < 100; i += 5) {
    printf("Value: %d\n", i);
}
```

#### Countdown For Loop
```os
for (let i = 10; i > 0; i--) {
    printf("Countdown: %d\n", i);
}
```

#### For Loop with Multiple Variables
```os
for (let i = 0, j = 10; i < j; i++, j--) {
    printf("i: %d, j: %d\n", i, j);
}
```

#### Nested For Loops
```os
for (let i = 0; i < 3; i++) {
    for (let j = 0; j < 3; j++) {
        printf("(%d, %d) ", i, j);
    }
    printf("\n");
}
```

### While Loops

#### Basic While Loop
```os
let count: int = 0;
while (count < 5) {
    printf("Count: %d\n", count);
    count++;
}
```

#### While Loop with Complex Condition
```os
let x: int = 1;
while (x < 1000 && x % 7 != 0) {
    x = x * 2;
    printf("x: %d\n", x);
}
```

#### Infinite While Loop (with break)
```os
let input: int = 0;
while (true) {
    printf("Enter a number (0 to exit): ");
    scanf("%d", &input);
    
    if (input == 0) {
        break;
    }
    
    printf("You entered: %d\n", input);
}
```

### Do-While Loops

```os
let number: int;
do {
    printf("Enter a positive number: ");
    scanf("%d", &number);
} while (number <= 0);

printf("You entered: %d\n", number);
```

## Loop Control Statements

### Break Statement
```os
for (let i = 0; i < 10; i++) {
    if (i == 5) {
        break;  // Exit the loop when i equals 5
    }
    printf("i: %d\n", i);
}
// Output: 0, 1, 2, 3, 4
```

### Continue Statement
```os
for (let i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;  // Skip even numbers
    }
    printf("Odd number: %d\n", i);
}
// Output: 1, 3, 5, 7, 9
```

### Labeled Break and Continue (Future Feature)
```os
outer: for (let i = 0; i < 3; i++) {
    for (let j = 0; j < 3; j++) {
        if (i == 1 && j == 1) {
            break outer;  // Break out of both loops
        }
        printf("(%d, %d) ", i, j);
    }
}
```

## Switch Statements (Future Feature)

```os
let day: int = 3;

switch (day) {
    case 1:
        printf("Monday\n");
        break;
    case 2:
        printf("Tuesday\n");
        break;
    case 3:
        printf("Wednesday\n");
        break;
    case 4:
        printf("Thursday\n");
        break;
    case 5:
        printf("Friday\n");
        break;
    case 6:
    case 7:
        printf("Weekend\n");
        break;
    default:
        printf("Invalid day\n");
        break;
}
```

## Pattern Matching (Future Feature)

```os
enum Shape {
    Circle(radius: f32),
    Rectangle(width: f32, height: f32),
    Triangle(base: f32, height: f32)
}

function calculate_area(shape: Shape) => f32 {
    match (shape) {
        Circle(r) => 3.14159 * r * r,
        Rectangle(w, h) => w * h,
        Triangle(b, h) => 0.5 * b * h
    }
}
```

## Exception Handling (Future Feature)

```os
function divide(a: f32, b: f32) => f32 {
    if (b == 0.0) {
        throw "Division by zero error";
    }
    return a / b;
}

function safe_divide(a: f32, b: f32) => f32 {
    try {
        return divide(a, b);
    } catch (error: char*) {
        printf("Error: %s\n", error);
        return 0.0;
    } finally {
        printf("Division operation completed\n");
    }
}
```

## Early Returns

```os
function find_max(arr: [int], size: int) => int {
    if (size <= 0) {
        return -1;  // Early return for invalid input
    }
    
    let max = arr[0];
    for (let i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

function validate_user(age: int, name: char*) => bool {
    if (age < 0) {
        printf("Invalid age\n");
        return false;
    }
    
    if (name == nullptr || strlen(name) == 0) {
        printf("Invalid name\n");
        return false;
    }
    
    printf("User validated successfully\n");
    return true;
}
```

## Practical Examples

### Finding Prime Numbers
```os
function is_prime(n: int) => bool {
    if (n <= 1) {
        return false;
    }
    
    if (n <= 3) {
        return true;
    }
    
    if (n % 2 == 0 || n % 3 == 0) {
        return false;
    }
    
    for (let i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    
    return true;
}

function print_primes(limit: int) => void {
    printf("Prime numbers up to %d:\n", limit);
    for (let i = 2; i <= limit; i++) {
        if (is_prime(i)) {
            printf("%d ", i);
        }
    }
    printf("\n");
}
```

### Menu System
```os
function show_menu() => void {
    printf("\n=== Menu ===\n");
    printf("1. Add numbers\n");
    printf("2. Multiply numbers\n");
    printf("3. Exit\n");
    printf("Enter your choice: ");
}

function calculator() => void {
    let choice: int;
    let a: f32, b: f32;
    
    while (true) {
        show_menu();
        scanf("%d", &choice);
        
        if (choice == 3) {
            printf("Goodbye!\n");
            break;
        }
        
        if (choice < 1 || choice > 3) {
            printf("Invalid choice. Please try again.\n");
            continue;
        }
        
        printf("Enter two numbers: ");
        scanf("%f %f", &a, &b);
        
        if (choice == 1) {
            printf("Result: %.2f\n", a + b);
        } else if (choice == 2) {
            printf("Result: %.2f\n", a * b);
        }
    }
}
```

### Array Processing
```os
function process_array(arr: [int], size: int) => void {
    let sum: int = 0;
    let max: int = arr[0];
    let min: int = arr[0];
    
    for (let i = 0; i < size; i++) {
        sum += arr[i];
        
        if (arr[i] > max) {
            max = arr[i];
        }
        
        if (arr[i] < min) {
            min = arr[i];
        }
    }
    
    let average: f32 = sum as f32 / size as f32;
    
    printf("Array statistics:\n");
    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", average);
    printf("Maximum: %d\n", max);
    printf("Minimum: %d\n", min);
}
```

## Best Practices

1. **Always use braces** for control structures, even for single statements
2. **Keep conditions readable** by using parentheses and clear variable names
3. **Avoid deep nesting** by using early returns when possible
4. **Use meaningful loop variable names** instead of just `i`, `j`, `k`
5. **Initialize loop variables** properly to avoid infinite loops
6. **Handle edge cases** in conditional statements
7. **Use `break` and `continue`** judiciously to improve code clarity
8. **Consider using functions** to reduce complex nested control structures

## Performance Considerations

1. **Loop optimization**: Place invariant calculations outside loops
2. **Short-circuit evaluation**: Place likely-false conditions first in AND operations
3. **Switch vs if-else**: Use switch for multiple discrete value comparisons
4. **Avoid unnecessary comparisons** in nested loops

---

*Control flow features are actively being developed and may change.*