---
see_also: [debug.getstackinfos]
---

Returns the source position of the constant that the compiler evaluates, or
null when the code runs at runtime.

## Return value

A table with these keys, or `null` when the code runs at runtime:

- `func` - the name of the function that the expression is in
- `src` - its source file
- `line` - the line of the call that runs the code; for code that an operator
  runs, such as a metamethod, the first line of the whole expression

## Notes

The compiler runs a [pure](page:attributes) function to evaluate a
[constant](page:language/bindings), and to fold a call with constant arguments
in any expression. A stack trace taken in that run has no frame for the code
that uses the constant. This function gives that position,
so a diagnostic in a pure function, such as a report of a missing key, can
still name the place. Use the position for diagnostics only: the result of a
pure function must not depend on it.

Each call in a constant gives its own line, and all code that a call runs gets
the position of that call: when a pure function calls another one, both get the
same position. But a call with constant arguments in the body of a pure
function folds when that body compiles, so it gets its own line in the body.

When the evaluation compiles more code, for example with `compilestring`, the
constants of that code give their own positions, and the outer position comes
back when that compile ends.

## Example

{{example:debug.get_const_eval_pos}}
