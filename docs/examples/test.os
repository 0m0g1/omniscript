extern "C" fn printf(fmt: char*, ...) => int;

struct Particle {
   x: float = 0;
   y: float = 0;
   constructor(x: float, y: float) => void {
      this.x = x;
      this.y = y;
   }
   log() => void {
      printf("%.2f, %.2f", this.x as double, this.y as double);
   }
}

let p1 = Particle{10, 0};
p1.log();

// // which language woud users prefer based of the syntax my language, c, c++, zig, rust? my language can be both high level and low level the user chooses what they want I don't force keep in mind all the null checks are forced during compile time for safety
// // Import standard file I/O module
// import { readFile } from "std::fs";

// // Parse space-separated integers from a file
// fn parseNumbers(filePath: string) => Result<i32[], string> {
//   try {
//     // Read file content (nullable type, compiler enforces null check)
//     let content: string? = readFile(filePath);
//     if (content == null) {
//       return result.Err("Failed to read file");
//     }
//     // Parse integers using high-level array operations
//     let numbers: i32[] = content.split(" ").map(num => {
//       let parsed: i32? = num.parseInt(); // Nullable type for parsing
//       if parsed == null {
//         return result.Err("Invalid integer: " + num);
//       }
//       return parsed; // Implicitly unwrapped after null check
//     });
//     if (numbers.length == 0) {
//       return result.Err("No valid integers found");
//     }
//     return result.Ok(numbers);
//   } catch (e) {
//     return result.Err("Error: " + e.message);
//   }
// }

// // Top-down execution for simplicity
// let result = parseNumbers("numbers.txt");
// if result.isOk() {
//   print("Numbers: ", result.unwrap());
// } else {
//   print("Error: ", result.unwrapErr());
// }

// // Declare C FFI functions
// extern "C" {
//   fn fopen(file: string, mode: string) => *FILE;
//   fn fscanf(file: *FILE, format: string, ...) => i32;
//   fn fclose(file: *FILE) => void;
// }

// // Parse space-separated integers from a file
// fn parseNumbers(filePath: string) => Result<i32[], string> {
//   // Open file with nullable pointer
//   let file: *FILE? = fopen(filePath, "r");
//   if (file == null) {
//     return result.Err("Failed to read file");
//   }
//   // Initialize dynamic array for integers
//   let numbers: i32[] = [];
//   let num: i32;
//   // Read integers until EOF or error
//   while fscanf(file, "%d", &num) == 1 {
//     numbers.push(num);
//   }
//   fclose(file);
//   if (numbers.length == 0) {
//     return result.Err("No valid integers found");
//   }
//   return result.Ok(numbers);
// }

// // Custom entry point // compiler flag
// // --entry=parseNumbers("numbers.txt")
// let result = parseNumbers("numbers.txt");
// if result.isOk() {
//   print("Numbers: ", result.unwrap());
// } else {
//   print("Error: ", result.unwrapErr());
// }

// extern "C" {
//     fn sin(x: double) => double;
//     fn printf(fmt: char*, ...) => int;
//     fn puts(str: char*) => int;
// }

// puts("hi")
// let n: int32 = 0;
// let n: double = sin(0.7);
// n++;

// extern "dependencies/openal-soft-1.24.3-bin/bin/Win64/soft_oal.dll" {
//     fn alcOpenDevice(devName: char*) => void*;
//     fn alcCreateContext(dev: void*, attrList: int*) => void*;
//     fn alcMakeContextCurrent(ctx: void*) => bool;
//     fn alcCloseDevice(dev: void*) => bool;
//     fn alcDestroyContext(ctx: void*) => void;

//     fn alGenBuffers(n: int, buffers: int*) => void;
//     fn alBufferData(buffer: int, format: int, data: char*, size: int, freq: int) => void;
//     fn alGenSources(n: int, sources: int*) => void;
//     fn alSourcei(source: int, param: int, value: int) => void;
//     fn alSourcePlay(source: int) => void;
//     fn alDeleteSources(n: int, sources: int*) => void;
//     fn alDeleteBuffers(n: int, buffers: int*) => void;
// }

// // AL Constants
// const AL_FORMAT_MONO16 = 0x1101;
// const AL_BUFFER = 0x1009;
// const PI:  float  = 3.1415927F;

// // Open audio device and context
// let device = alcOpenDevice(nullptr);
// let context = alcCreateContext(device, nullptr);
// alcMakeContextCurrent(context);

// // Create sine wave data
// let sampleRate = 44100;
// let duration = 1.0; // seconds
// let freq = 440.0; // A4 tone
// let samples = sampleRate * duration;

// let buffer = [1];
// let source = [1];

// alGenBuffers(1, &buffer[0]);
// alGenSources(1, &source[0]);

// // Generate mono 16-bit PCM sine wave
// let data: [44100]float = [samples];
// for (let i: float = 0; i < samples; i += 1) {
//     let t = i / sampleRate;
//     data[i] = (sin(2 * PI * freq * t) * 32767) as int;
// }

// let f : char = 0.1 as char;

// // Upload to OpenAL
// alBufferData(buffer[0], AL_FORMAT_MONO16, data as char*, samples * 2, sampleRate);
// alSourcei(source[0], AL_BUFFER, buffer[0]);
// alSourcePlay(source[0]);

// // Wait for it to play
// sleep(1000);

// // Cleanup
// alDeleteSources(1, &source[0]);
// alDeleteBuffers(1, &buffer[0]);
// alcDestroyContext(context);
// alcCloseDevice(device);

































// import "std";

// let n: int = std.Math.factorial(5);

// if (n == -1) {
//    std.IO.Console.printf("n is an invalid factorial.\nPlease input a positive integer.");
// } else {
//    std.IO.Console.printf("n is %i", n);
// }
























// std.IO.Console.printf("done");
// let variableafterifblock : int = n;

// let n : f32 = std.Math.powf(2.0, 3.0);
// std.IO.Console.printf("%.2f\n", n);


// std.IO.Console.printf("Playing melody...\n");
        
// // D Major Scale (D4 to D5)
// std.IO.Console.Beep(294, 300);  // D4
// std.IO.Console.Beep(330, 300);  // E4
// std.IO.Console.Beep(370, 300);  // F#4
// std.IO.Console.Beep(392, 300);  // G4
// std.IO.Console.Beep(440, 300);  // A4
// std.IO.Console.Beep(494, 300);  // B4
// std.IO.Console.Beep(554, 300);  // C#5
// std.IO.Console.Beep(587, 300);  // D5 (hold last note longer)
// std.IO.Console.Beep(587, 300);  // D5 (hold last note longer)


// std.IO.Console.printf("Done!");


// function powi(base: int = 1, exponent: int = 1) => int {
//    let result: int = 1;
//    for (let i = 0; i < exponent; i += 1) {
//       result *= base;
//    }
//    return result;
// }


// let n: int = pow(2, 2);
// let f: float = pow(2.0 as float, 2.0 as float);

// let pow = [powiii, powfff, powddd]


// extern "C" fn printf(fmt: char*, ...) => int;
// intrinsic fn pow(x: f32, y: f32) => f32;

// let x: f32 = pow(2.0, 2.0);
// printf("%.0f", x);

// let name : char* = "foo";
// printf("%s%s%s%s", "Hello world!!!\n", "and hello ", name, "\n");














// let x : int = 123;
// let y : uint = 456;
// let z : f16 = 456;
// let zz : f32 = 456;
// let zzz : f64 = 456;
// let zzzz : f80 = 456;
// let zzzzz : f128 = 456;

// printf("%d\n", x);
// printf("%u\n", y);
// printf("%x\n", x);
// printf("%X\n", x);
// printf("%o\n", x);
// printf("%.2f\n", z);
// printf("%.4f\n", zz);
// printf("%.4f\n", zzz);
// printf("%.4f\n", zzzz);
// printf("%.4f\n", zzzzz);


// extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll" fn glfwInit() => bool;


// let msgOk = "GLFW initialized successfully!\n";
// let msgFail = "GLFW initialization failed.\n";

// if (glfwInit()) {
//    printf(msgOk);
// } else {
//    printf(msgFail);
// }

// extern "C" fn printf(fmt: char*, ...) => int;
// extern "C" fn strcmp(a: char*, b: char*) => int;
// let name : char* = "foo";


// let test: char* = "hi";

// if (strcmp(test,"ho") == 0) {
//    printf("ho");
// } else if (strcmp(test, "hi") == 0) {
//    printf("hi");
// } else {
//    printf("neither");
// }

// import { Math } from std;

// let n: f32 = std.Math->pi;
// let charp : char* = "hi";
// let text = std.string(charp);
// let test: std.string = "";

// add support for js like formatted strings
// let textChar: char* = "hello world ${}";

// let textCar: char* = "hello world";
// let text: std.string = "hello world";



// intrinsic fn pow(x: int, y:int) => int;

// extern "C" fn malloc(size: size_t) => void*;
// extern "C" fn free(ptr: void*) => void;
// extern "C" fn realloc(ptr: void*, size: usize) => void*;
// extern "C" fn calloc(count: usize, size: usize) => void*;
// extern "C" fn printf(fmt: char*, ...) => int;

// // // let textCar: char* = malloc(5);
// // // free(textChar);

// // extern "path/to/lib.dll" fn eternal(...fmt: char*) => int;
// Todo:: a compiler flag or just a way for the current code to be turned into a dll/so library
// printf("%s%s%s", "Hello world!!!\n", "hi\n", "hello\n");
// for (let index = 0; index < 100; index++) {
   
// }
// extern "C" fn strcmp(a: char*, b: char*) => int;

// function add(...nums : int) => i32 {
//    let sum : int32 = 0;
//    for (let index : int32 = 0; index < nums_count; index++) {
//       let elem = nums[index];
//       sum += elem;
//    }
//    return sum;
// }

// let n : int32 = add(1, 2, 3);

// function main(argc : int) => i32 {
//    return 0;
// }

// class String {
//    private text: char*;
//    private size: int = 0;

//    // Constructor
//    constructor(text: char* = "") => void {
//       this.text = text;
//       this.size = this.calculateLength();

//       if (this.size > 15) {
//          // this.text = malloc(this.size + 1);
//       }
//    }

//    private calculateLength() => int {
//       let len: int32 = 0;
//       while (this.text[len] != '\0') {
//          len++;
//       }
//       return len;
//    }

//    public length() => int {
//       return this.size;
//    }

//    public size() => int {
//       return this.size;
//    }
// }

// let textp = String("hello");
// let textLength = textp.calculateLength();

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
//    std.IO.Console.log(i);
//    a++;
// }

// std.IO.Console.log(a);

// changeA();
// a = (changeA() + (5 + 1) / 2 * 3 - 1);


// function ab(n = 1, b = 2) {
//    std.IO.Console.info(n);
//    std.IO.Console.info(b);
// }

// ab(n = "hell0", b = "hi");