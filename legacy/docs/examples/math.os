// Assuming 'Math' class is already initialized and available

// Accessing constants
console.log(Math.PI);        // 3.141592653589793
console.log(Math.E);         // 2.718281828459045
console.log(Math.LN2);       // 0.6931471805599453

// Using methods
// console.log(Math.abs(-42));  // 42
console.log(Math.cos(Math.PI / 3.0)); // 0.5 (Cosine of 60 degrees)
console.log(Math.sin(Math.PI / 2.0)); // 1 (Sine of 90 degrees)
console.log(Math.pow(2, 3)); // 8 (2 raised to the power of 3)

// Using random functions
console.log(Math.randint(1, 10));   // Random integer between 1 and 10
console.log(Math.randf(1.5, 2.5));  // Random float between 1.5 and 2.5
console.log(Math.randrange(1, 10)); // Random integer from 1 to 10 (exclusive)
console.log(Math.randfrange(0.5, 2.0)); // Random float from 0.5 to 2.0 (exclusive)

// Example with a derived constant
const ThreePI = Math.PI * 3;
console.log(ThreePI);  // 9.42477796076938
