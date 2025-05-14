// import { Math } from std;

// let n: f32 = std.Math->pi;

// let textChar: char* = "hello world";
// let text: std.string = "hello world";

class Particle {
   x: f32 = 1.0;
   z: f32 = 3.0;

   constructor(x: f32 = 0, z: f32 = 0) => void {
      this.x = x;
      this.z = z;
   }

    // Allowed internal access to private member
   getX() => f32 {
      return this.x;
   }
}

let p = Particle(10.0, 30.0);

// ✅ Valid access to public member
// let a = p.z;       // OK

// ✅ Valid access via method
// let b = p.getX();  // OK

// ❌ Invalid access to private member (should fail)
// let c = p.x;       // ← This line should throw a compile-time error or crash at runtime





























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