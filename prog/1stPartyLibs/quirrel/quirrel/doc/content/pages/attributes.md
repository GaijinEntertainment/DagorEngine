---
title: Function attributes
group: Guides
order: 92
summary: `pure`, `fastcall` and `nodiscard`, and what each one allows the compiler to assume.
---

A signature on these pages can carry up to three attributes. They are part of the
declaration, not of the body. They tell the compiler and the VM what a call may
assume.

Script functions write them in brackets. Natives write them in the declaration
string that the binding registers.

```nut
const function [pure] square(x: int): int { return x * x }
let f = @[pure] (x) x * x
```

## pure

The result depends only on the arguments, and on state that changes only
together with a new compile of the calling script. The compiler treats a
direct call to a pure function as a constant expression, so it may fold the
call or keep the result as a constant initializer. A folded call runs at
compile time, not at run time. A call of a constant or imported pure function
with constant arguments runs at compile time in any expression: a simple
result folds, and an error fails the compile, also in a function that nothing
calls; see "Folding outside const" in
[Bindings and constants](page:language/bindings).

Mark a function pure only when it writes no state. The VM does not check this.
A pure function with a side effect may not be called at all.

An `async` function cannot be pure, because each call starts a new task. The
compiler rejects `[pure]` on it, and `sq_mark_pure_inplace` fails on it.

Most of `math` is pure. `math.sqrt` and `math.clamp` are pure. `math.rand` is
not, because it changes the seed.

## fastcall

A native calling convention. The compiler emits a dedicated call opcode that
skips the frame setup, so the native runs on the caller's frame.

A fastcall native has to be a leaf. It must not call back into the VM, and it
must push a bounded number of values.
Only the binding sets this attribute. Script code cannot add or remove it.
It is shown here because a fastcall native does not appear in a stack trace as a frame of its own.

## nodiscard

The caller must use the return value. Use this mark when a call that throws
its result away is a mistake, for example a function that returns a new value
and does not change its argument.

```nut
function [nodiscard] withItem(list, item) {
  let res = clone list
  res.append(item)
  return res
}

withItem(items, 5)               // w348: the call has no effect
let longer = withItem(items, 5)  // correct
```

The static analyzer flags a discarded result as `w348` (`discarded-nodiscard`).
The analyzer also flags a discarded `pure` call, because a pure call
changes nothing.

A `pure` function does not need this mark. Methods of the built-in types that
are useful only for their result, such as `map`, `filter` and `bindenv`, have
it. The analyzer checks these methods, and `pure` ones such as `len`, when it
knows the type of the value:

- a string, number, bool or array literal, such as `[1, 2].map(f)` or
  `"abc".len()`;
- a table literal without a computed key, a spread or a member of the method's
  name, such as `({}).keys()`;
- the result of a native that returns only a string, an integer, a float, a
  bool, an array or a thread, such as `format("%d", 1).len()`;
- a value that comes from an imported module, except a class instance;
- a variable bound to one of these, except a table literal;
- with `.$`, any table literal or a variable bound to one.

It does not check a method on a parameter, a function of the script, a class
instance or the result of another method.
