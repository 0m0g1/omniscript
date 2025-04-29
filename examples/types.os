// // Basic integer types
// let aa : int = 1;
// let a : i8 = 1;
// let b : i16 = 2;
// let c : i32 = 3;
// let d : i64 = 4;
// // let e : i128 = 4;
// // let f : i256 = 4;
// // let g : i512 = 4;
// // let h : i1024 = 40000000000000000000000000000000000000000000000000000000;

// // Character and boolean
// let i : char = 'a';
// let j : bool = true;
// let k : bool = false;

// // Floating-point types
// // Assignments for each floating-point type, using both base and alternative type names

// // Half Precision
// // let halftest : f16 = 3.14;  // Using 'f16'
// // let half_alt : half = 2.718;  // Using 'half' (alternative name)

// // Single Precision (float or f32)
// let single : f32 = 3.14;  // Using 'f32'
// let single_alt : float = 2.718;  // Using 'float' (alternative name)

// // Double Precision (double or f64)
// let double_val : f64 = 3.14;  // Using 'f64'
// let double_alt : double = 2.718;  // Using 'double' (alternative name)

// // Quad Precision (f128, fp128, or long double)
// let quad : f128 = 1.6180339887;  // Using 'f128'
// let quad_alt : fp128 = 2.718;    // Using 'fp128' (alternative name)
// let quad_alt2 : long_double = 3.14159;  // Using 'long double' (alternative name)

// // Extended Precision (x86_fp80 or x87_FP80)
// let extended : x86_fp80 = 2.718;  // Using 'x86_fp80'
// let extended_alt : x86_80bit = 3.14;  // Using 'x86_80bit' (alternative name)
// let extended_alt2 : x87_FP80 = 1.618;  // Using 'x87_FP80' (alternative name)
// let extended_alt3 : Intel_FP80 = 2.718;  // Using 'Intel_FP80' (alternative name)

// // PPC 128-bit Precision (PPC_FP128 or PPC_Quad)
// let ppc_quad : ppc_fp128 = 3.14159;  // Using 'ppc_fp128'
// let ppc_quad_alt2 : PPC_Quad = 2.718;  // Using 'PPC_Quad' (alternative name)

// // Pointers
// let o : i32* = &c; // Pointer to an i32
// let p : char* = &i; // Pointer to a character
// let oo : i32** = &o;
// let pp : char** = &p;
// let ooo : i32*** = &oo; // Pointer to an i32
// let ppp : char*** = &pp; // Pointer to a character
// let oooo : i32**** = &ooo;
// let pppp : char**** = &ppp;
// let q : void* = nullptr; // Void pointer
// let nullpointer : char**** = nullptr;

// // References
// let r : &i32 = c; // Reference to an i32
// // let rr : &&i32 = &r; // Reference to an i32
// let s : &bool = j; // Reference to a bool
// // let ss : &&bool = s; // Reference to a bool

// // String and character arrays
// let t : char* = "Hello, OmniScript++!"; // String (C-style)
// let tt : utf8 = "Hello, OmniScript++!"; // String (C-style)
// let ttt : utf16 = "Hello, OmniScript++!"; // String (C-style)
// let tttt : utf32 = "Hello, OmniScript++!"; // String (C-style)
// let u : [5]char = ['H', 'e', 'l', 'l', 'o']; // Character array
// let uu : [5]i32 = [1,2,3,4,5]; // integer arrays
// let uuu : [5]i32 = [1,2,3,4,5]; // multidimentional array

// // Dynamic arrays (if supported)
// let v : [i32] = [1, 2, 3, 4, 5]; // Dynamic array of i/ntegers
// let w : [char] = "Dynamic String"; // Dynamic string (if OmniScript++ supports it)
// enum Color {
//     Red,
//     Green,
//     Blue
// }


// enum Fruit(lookup) {
//     Apple,
//     Banana,
//     Mango
// }

// enum class VehicleType {
//     Car,
//     Truck,
//     Motorcycle
// }

// enum class Planet(lookup) {
//     Mercury,
//     Venus,
//     Earth,
//     Mars
// }


// // Structs (user-defined types)
// struct Vector3 {
//     x: f32 = 0;
//     y: f32 = 0;
//     z: f32 = 0;
// }
let index = 0;
// for (;;) {
//     let n: int32 = 0;
// }

while (index < 5) {
    // index++;
}

// work on dynamic arrays
// let v1 = Vector3{ x: 1.0, y: 2.0};

// let vx = v1.x;
// v1.x = 5.0;

// // Function pointers
// let fn : (i32, i32) -> i32 = add; // Function pointer

// // Nullable types (if supported)
// let x : ?i32 = null; // Nullable integer

// // Optional values (if supported)
// let y : Option<i32> = Some(10);
// // let z : Option<i32> = None;
// let variable : int = 10;

// function add(a: int = 1, b: int = 1) => i32 {
    // return a + b;
// }

// add(1, 1);

// <Number extends i8 | i16 | i32 | i64 | f32 | f64 | f128>
// function add(a: Number, b: Number) => Number {
//     return a + b;
// }

// function join<T>(a: T, b: T) => T {
//     return a + b;
// }

// add<i32>(5, 6);

// function main() => i32 {
//     return add(b = 1, a = 2);
//     return add<i32>(1, 1);
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

// <Number extends i8 | i16 | i32 | i64 | f32 | f64 | f128>
// let add = (a: Number, b: Number) => Number {
//     return a + b;
// }


// TODO Add support none predetermined types
// TODO LLVM automatically cleans up unused codes but not all backends do ensure that only the used type gets generated the rest to be discarded
// let add = <T>(a: T, b: T) => T {
//     return a + b;
// }
// let sth : int = add<i32>(1, 2);


// function add(a : i32 = 1, b : i32 = 1) => i32  {
//     return a + b;
// }

// function add(a : i32 = 1, b : i32 = 1, c : i32 = 0) => i32  {
//     return a + b + c;
// }

// function add(a: f32 = 1.0, b: f32 = 1.0) => f32 {
//     return a + b;
// }

// let isRaining: bool = true;
// if (isRaining) {
    
// } 

// function main() => i32 {
//     // return add(b = 1, a = 2, c = 10);
//     // return c;
//     return 0;
// }



// return add<i32>(1, 1);
// return 2;
// add(1.0, 1.0);
// add(1, 1);
