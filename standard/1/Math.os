module Math {
    public pow(base: int = 1, exponent: int = 1) => i32 {
        let result: int = 1;
        for (let i = 0; i < exponent; i += 1) {
            result *= base;
        }
        return result;
    }
}
