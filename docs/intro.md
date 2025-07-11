# OS Programming Language

## Hello World Example

```os
extern "C" fn printf(fmt: char*, ...) => int;

printf("Hello, OS!\n");
```

```os
extern "C" fn printf(fmt: char*, ...) => int;

function main() => i32 {
   printf("Hello, OS!\n");
   return 0;
}
```

## explanation 
There are two ways to run scripts in omniscript, through a main function like in c and c++ or top down like in python and javascript.
If a main function exists it will be called automatically by the compiler and all top level code will be ran at the start of the main function.
Os doesn't have a robust standard library as of yet but it can do everything in the c standard library via its powerfull foreign function interface which will talk about later.


The `extern "C" fn printf(...fmt: char*) => int` how we get hello world to be printed on the screen