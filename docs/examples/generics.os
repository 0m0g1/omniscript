function add<T>(a, b) {
    return a + b;
}

console.log(add<int>(1, 2));
console.log(add<string>("hello", "world"));