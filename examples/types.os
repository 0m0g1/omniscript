// Basic integer types
let aa : int = 1;
let a : i8 = 1;
let b : i16 = 2;
let c : i32 = 3;
let d : i64 = 4;
// let e : i128 = 4;
// let f : i256 = 4;
// let g : i512 = 4;
// let h : i1024 = 40000000000000000000000000000000000000000000000000000000;

// Character and boolean
let i : char = 'a';
let j : bool = true;
let k : bool = false;

// Floating-point types
// Assignments for each floating-point type, using both base and alternative type names

// Half Precision
// let half : f16 = 3.14;  // Using 'f16'
// let half_alt : half = 2.718;  // Using 'half' (alternative name)

// Single Precision (float or f32)
let single : f32 = 3.14;  // Using 'f32'
let single_alt : float = 2.718;  // Using 'float' (alternative name)

// Double Precision (double or f64)
let double_val : f64 = 3.14;  // Using 'f64'
let double_alt : double = 2.718;  // Using 'double' (alternative name)

// Quad Precision (f128, fp128, or long double)
let quad : f128 = 1.6180339887;  // Using 'f128'
let quad_alt : fp128 = 2.718;    // Using 'fp128' (alternative name)
let quad_alt2 : long_double = 3.14159;  // Using 'long double' (alternative name)

// Extended Precision (x86_fp80 or x87_FP80)
let extended : x86_fp80 = 2.718;  // Using 'x86_fp80'
let extended_alt : x86_80bit = 3.14;  // Using 'x86_80bit' (alternative name)
let extended_alt2 : x87_FP80 = 1.618;  // Using 'x87_FP80' (alternative name)
let extended_alt3 : Intel_FP80 = 2.718;  // Using 'Intel_FP80' (alternative name)

// PPC 128-bit Precision (PPC_FP128 or PPC_Quad)
let ppc_quad : ppc_fp128 = 3.14159;  // Using 'ppc_fp128'
let ppc_quad_alt2 : PPC_Quad = 2.718;  // Using 'PPC_Quad' (alternative name)

// // // Pointers
let o : i32* = &c; // Pointer to an i32
let p : char* = &i; // Pointer to a character
let oo : i32** = &o;
let pp : char** = &p;
let ooo : i32*** = &oo; // Pointer to an i32
let ppp : char*** = &pp; // Pointer to a character
let oooo : i32**** = &ooo;
let pppp : char**** = &ppp;
let q : void* = nullptr; // Void pointer
let nullpointer : char**** = nullptr;

// // References
let r : &i32 = c; // Reference to an i32
// let rr : &&i32 = &r; // Reference to an i32
let s : &bool = j; // Reference to a bool
// let ss : &&bool = s; // Reference to a bool

// String and character arrays
// let t : char* = "Hello, OmniScript++!"; // String (C-style)
// let u : [5]char = ['H', 'e', 'l', 'l', 'o']; // Character array
// let uu : [5]i32 = [1,2,3,4,5]; // Character array
// let uuu : [5]i32 = [1,2,3,4,5]; // multidimentional array

// // Dynamic arrays (if supported)
// let v : [i32] = [1, 2, 3, 4, 5]; // Dynamic array of integers
// let w : [char] = "Dynamic String"; // Dynamic string (if OmniScript++ supports it)


// // Structs (user-defined types)
// struct Vector3 {
//     x: f64;
//     y: f64;
//     z: f64;
// }

// let v1 = Vector3{ x: 1.0, y: 2.0, z: 3.0 };
// let vx = v1.x;
// v1.x = 5.0;

// let v1 : Vector3 = { x: 1.0, y: 2.0, z: 3.0 };

// // Function pointers
// let fn : (i32, i32) -> i32 = add; // Function pointer

// // Nullable types (if supported)
// let x : ?i32 = null; // Nullable integer

// // Optional values (if supported)
// let y : Option<i32> = Some(10);
// // let z : Option<i32> = None;
// let variable : int = 10;

// function add(a: int, b: int) => i32 {
//     return a + b;
// }

// function main() => i32 {
//     return add(1, 1);
// }

// let ref : function* = add;

// let result : int = add(1, 1);
// let anchor = new heap Particle(x,  y);

// enum Fruits {
//     apple,
//     banana
// }

// let Fruits_apple : int = Fruits.apple;

// enum Fruits(lookup) {
//     apple,
//     banana
// }

// let lambda = (a: int = 1, b: int = 1) => i32 {
//     return a + b;
// }