// import { Math, IO } from std;

// let n: f32 = std.Math->pi;
// let charp : char* = "hi";
// let text = std.string(charp);
// let test: std.string = "";

// add support for js like formatted strings
// let textChar: char* = "hello world ${}";

// let textChar: char* = "hello world";
// let text: std.string = "hello world";

extern "C" fn malloc(size: size_t) => void*;
// extern "C" fn free(ptr: pointer<void>) => void;
// extern "C" fn realloc(ptr: pointer<void>, size: usize) => pointer<void>;
// extern "C" fn calloc(count: usize, size: usize) => pointer<void>;

class String {
   private text: char*;
   private size: int = 0;

   // Constructor
   constructor(text: char* = "") => void {
      this.text = text;
      this.size = this.calculateLength();
   }

   private calculateLength() => int {
      let len: int32 = 0;
      while (this.text[len] != '\0') {
         len++;
      }
      return len;
   }

   public length() => int {
      return this.size;
   }

   public size() => int {
      return this.size;
   }
}

let textp = String("hello");
let textLength = textp.calculateLength();

// let a : i8 = 1;
// let b : i16 = 2;
// let c : i32 = 3;
// let d : i64 = 4;
// let e : i128 = 4;
// let f : i256 = 4;
// let g : i512 = 4;
// let h : i1024 = 40000000000000000000000000000000000000000000000000000000;
// let c : std::string = "hello";
   
/* This is a function */
// function changeA() {
   // let number = 5;
    // return number + a + 2 + 3;
    // return -1;
// }


// while (a > 0) {
   // a += changeA();
// }

// let a = 0;
// for (let i = 0; i < 5; i++) {
//    console.log(i);
//    a++;
// }

// console.log(a);

// changeA();
// a = (changeA() + (5 + 1) / 2 * 3 - 1);


// function ab(n = 1, b = 2) {
//    console.info(n);
//    console.info(b);
// }

// ab(n = "hell0", b = "hi");