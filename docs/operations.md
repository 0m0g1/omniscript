# Binary Expression Operators

This documentation describes the behavior of various operators in the `BinaryExpression` class. The operators are used in binary expressions and can operate on different types of data (e.g., `int`, `float`, `bool`, `std::string`, and arrays). Below are the details for each operator and its behavior when applied to different operand types.

## Supported Operators

### Arithmetic Operators

#### `+` (Addition)
- **Operands:** `int`, `float`, `std::string`
- **Behavior:**
  - **`int + int`**: Adds two integers.
  - **`float + float`**: Adds two floating-point numbers.
  - **`std::string + std::string`**: Concatenates two strings.
  - **`std::string + int/float`**: Converts the number to a string and appends it to the string.

#### `-` (Subtraction)
- **Operands:** `int`, `float`, `std::string`
- **Behavior:**
  - **`int - int`**: Subtracts one integer from another.
  - **`float - float`**: Subtracts one floating-point number from another.
  - **`std::string - std::string`**: Removes occurrences of the second string from the first string (substring removal).

#### `*` (Multiplication)
- **Operands:** `int`, `float`, `std::string` (with `int`)
- **Behavior:**
  - **`int * int`**: Multiplies two integers.
  - **`float * float`**: Multiplies two floating-point numbers.
  - **`std::string * int`**: Repeats the string `n` times (multiplying a string by an integer).
  - **`std::string * float/double`**: Repeats the string fractional times, with proper handling of negative values.

#### `/` (Division)
- **Operands:** `int`, `float`
- **Behavior:**
  - **`int / int`**: Divides one integer by another. Throws a runtime error if dividing by zero.
  - **`float / float`**: Divides one floating-point number by another. Throws a runtime error if dividing by zero.

#### `%` (Modulo)
- **Operands:** `int`
- **Behavior:**
  - **`int % int`**: Returns the remainder when dividing two integers.

### Comparison Operators

#### `==` (Equality)
- **Operands:** `int`, `float`, `bool`, `std::string`, `ArrayPrimitive`
- **Behavior:**
  - **`int == int`**: Returns `true` if both integers are equal.
  - **`float == float`**: Returns `true` if both floats are equal.
  - **`bool == bool`**: Returns `true` if both booleans are equal.
  - **`std::string == std::string`**: Returns `true` if both strings are equal.
  - **`ArrayPrimitive == ArrayPrimitive`**: Returns `true` if both arrays contain the same elements.

#### `!=` (Inequality)
- **Operands:** `int`, `float`, `bool`, `std::string`, `ArrayPrimitive`
- **Behavior:**
  - **`int != int`**: Returns `true` if the integers are not equal.
  - **`float != float`**: Returns `true` if the floats are not equal.
  - **`bool != bool`**: Returns `true` if the booleans are not equal.
  - **`std::string != std::string`**: Returns `true` if the strings are not equal.
  - **`ArrayPrimitive != ArrayPrimitive`**: Returns `true` if the arrays do not contain the same elements.

#### `<` (Less Than)
- **Operands:** `int`, `float`, `std::string`
- **Behavior:**
  - **`int < int`**: Returns `true` if the first integer is less than the second.
  - **`float < float`**: Returns `true` if the first float is less than the second.
  - **`std::string < std::string`**: Returns `true` if the first string is lexicographically smaller than the second.

#### `>` (Greater Than)
- **Operands:** `int`, `float`, `std::string`
- **Behavior:**
  - **`int > int`**: Returns `true` if the first integer is greater than the second.
  - **`float > float`**: Returns `true` if the first float is greater than the second.
  - **`std::string > std::string`**: Returns `true` if the first string is lexicographically greater than the second.

#### `<=` (Less Than or Equal To)
- **Operands:** `int`, `float`, `std::string`
- **Behavior:**
  - **`int <= int`**: Returns `true` if the first integer is less than or equal to the second.
  - **`float <= float`**: Returns `true` if the first float is less than or equal to the second.
  - **`std::string <= std::string`**: Returns `true` if the first string is lexicographically less than or equal to the second.

#### `>=` (Greater Than or Equal To)
- **Operands:** `int`, `float`, `std::string`
- **Behavior:**
  - **`int >= int`**: Returns `true` if the first integer is greater than or equal to the second.
  - **`float >= float`**: Returns `true` if the first float is greater than or equal to the second.
  - **`std::string >= std::string`**: Returns `true` if the first string is lexicographically greater than or equal to the second.

### Logical Operators

#### `&&` (Logical AND)
- **Operands:** `bool`
- **Behavior:**
  - **`bool && bool`**: Returns `true` if both booleans are `true`.

#### `||` (Logical OR)
- **Operands:** `bool`
- **Behavior:**
  - **`bool || bool`**: Returns `true` if either of the booleans is `true`.

#### `^` (Logical XOR)
- **Operands:** `bool`
- **Behavior:**
  - **`bool ^ bool`**: Returns `true` if exactly one of the booleans is `true`.

### Unary Operators (For Missing Right Operand)

#### `-` (Unary Minus)
- **Operand:** `int`, `float`
- **Behavior:**
  - **`-int`**: Negates an integer (multiplies by `-1`).
  - **`-float`**: Negates a floating-point number (multiplies by `-1`).

#### `++` (Increment)
- **Operand:** `int`
- **Behavior:**
  - **`int++`**: Increments an integer by `1`.

#### `--` (Decrement)
- **Operand:** `int`
- **Behavior:**
  - **`int--`**: Decrements an integer by `1`.

### Array Operators

#### `+` (Array Concatenation)
- **Operands:** `ArrayPrimitive`
- **Behavior:**
  - **`Array + Array`**: Concatenates two arrays by combining their elements.

#### `-` (Array Removal)
- **Operands:** `ArrayPrimitive` and any value
- **Behavior:**
  - **`Array - Value`**: Removes all occurrences of a given value from an array.

#### `+` (Array Append)
- **Operands:** `ArrayPrimitive` and any value
- **Behavior:**
  - **`Array + Value`**: Appends the value to the end of the array.
  - 
#### `+` (Array preppend)
- **Operands:** `ArrayPrimitive` and any value
- **Behavior:**
  - **`Value + Array`**: Preppends the value to the beginning of the array.

### Notes:
- **Division by Zero:** For both `int` and `float`, division by zero is not allowed and will throw a runtime error.
- **Unsupported Operations:** Some operations may be unsupported for specific data types. A runtime error will be thrown in such cases.

## Conclusion

The operators provided in this `BinaryExpression` class allow for flexible expression evaluation, with support for arithmetic, comparison, logical, and array operations. Each operator behaves differently based on the operand types, offering a wide range of operations suitable for various expressions.
