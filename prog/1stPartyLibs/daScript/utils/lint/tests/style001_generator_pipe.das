options gen2
// STYLE001/002: <| pipe before generator block
//
// Problem: generator<T>() <| $ { ... } uses unnecessary pipe.
//
// Bad patterns:
//   generator<int>() <| $ { yield 1; return false }
//   generator<int>() <| $() { yield 1; return false }
//
// Good pattern (once parser supports it):
//   generator<int>() { yield 1; return false }

// bad: pipe before parameterless generator block (STYLE001)
def bad_generator_pipe() : iterator<int> {
    return <- generator<int>() <| $ {
        yield 1
        yield 2
        return false
    }
}

// bad: pipe before $() generator block (STYLE002)
def bad_generator_pipe_dollar() : iterator<int> {
    return <- generator<int>() <| $() {
        yield 1
        yield 2
        return false
    }
}
