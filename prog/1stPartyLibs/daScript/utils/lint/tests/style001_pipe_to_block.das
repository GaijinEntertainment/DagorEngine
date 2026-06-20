options gen2
// STYLE001: unnecessary <| pipe before block argument
//
// Problem: <| pipe syntax is gen1 style and unnecessary in gen2.
//
// Bad pattern:
//   build_string() <| $(var w) { write(w, "hello") }
//
// Good pattern:
//   build_string() $(var w) { write(w, "hello") }

require strings

// bad: pipe before block with args (1 warning)
def bad_pipe_with_args() {
    let s = build_string() <| $(var w) {
        w |> write("hello")
    }
}

// good: trailing block
def good_trailing_block() {
    let s = build_string() $(var w) {
        w |> write("hello")
    }
}
