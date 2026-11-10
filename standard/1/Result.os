// standard/1/Result.os
// Tier 1: Result<T, E>, written as a generic struct (no enum needed).
// E cannot be inferred from ok(...) / err(...) arguments alone, so instantiate explicitly:
//
//   function parse(s: char*) => Result<i64, i32> {
//       if (bad) return Result.err<i64, i32>(2);
//       return Result.ok<i64, i32>(42);
//   }
//   let r = parse("42");
//   if (r.isOk()) use(r.unwrap());

module Core = import "standard/1/Core.os";

struct Result<T, E> {
    okFlag: bool = false;
    value: T;
    error: E;

    isOk() => bool { return this.okFlag; }
    isErr() => bool { return !this.okFlag; }

    unwrap() => T {
        if (!this.okFlag) Core.panic("Result.unwrap called on an error");
        return this.value;
    }

    unwrapErr() => E {
        if (this.okFlag) Core.panic("Result.unwrapErr called on an ok value");
        return this.error;
    }

    unwrapOr(fallback: T) => T {
        if (this.okFlag) return this.value;
        return fallback;
    }
}

function ok<T, E>(v: T) => Result<T, E> {
    let r: Result<T, E>;
    r.okFlag = true;
    r.value = v;
    return r;
}

function err<T, E>(e: E) => Result<T, E> {
    let r: Result<T, E>;
    r.okFlag = false;
    r.error = e;
    return r;
}
