function add(a : i32 = 1, b : i32 = 1) => i32  {
    return a + b;
}

function add(a : i32 = 1, b : i32 = 1, c : i32 = 0) => i32  {
    return a + b + c;
}

function add(a: f32 = 1.0, b: f32 = 1.0) => f32 {
    return a + b;
}

function main() => i32 {
    return add(b = 1, a = 2, c = 10);
}
