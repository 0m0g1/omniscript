// Define a function to compute the factorial of a number
function factorial(n) {
    if (n == 0) {
        return 1; // Base case: 0! = 1
    }
    return n * factorial(n - 1); // Recursive case: n! = n * (n-1)!
}

// Test the factorial function
let number = 5; // Input number
let result = factorial(number); // Calculate factorial
console.log("Factorial of " + number + " is " + result);
