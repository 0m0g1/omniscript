// Basic integer types
// let aa : int = 1;
// let a : i8 = 1;
// let b : i16 = 2;
// let c : i32 = 3;
// let d : i64 = 4;
// let e : i128 = 4;
// let f : i256 = 4;
// let g : i512 = 4;
// let h : i1024 = 40000000000000000000000000000000000000000000000000000000;

// // Character and boolean
// let i : char = 'a';
// let j : bool = true;
// let k : bool = false;

// // Floating-point types
// let l : f32 = 3.14;
// let m : f64 = 2.718;
// // let n : f128 = 1.6180339887;

// // // Pointers
// let o : i32* = &c; // Pointer to an i32
// let p : char* = &i; // Pointer to a character
// let oo : i32** = &o;
// let pp : char** = &p;
// let q : void* = nullptr; // Void pointer

// // References
// let r : &i32 = c; // Reference to an i32
// let s : &bool = j; // Reference to a bool

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
//     x: f32;
//     y: f32;
//     z: f32;
// }

// let v1 : Vector3 = { x: 1.0, y: 2.0, z: 3.0 };

// // Function pointers
// let fn : (i32, i32) -> i32 = add; // Function pointer

// // Nullable types (if supported)
// let x : ?i32 = null; // Nullable integer

// // Optional values (if supported)
// let y : Option<i32> = Some(10);
// let z : Option<i32> = None;
let variable : int = 10;

function add(a: int, b: int) => i32 {
    return a + b;
}

function main() => i32 {
    return add(1, 1);
}

// let ref : function* = add;

// let result : int = add(1, 1);
// let anchor = new heap Particle(x,  y);