---
title: Bindings and constants
group: Language
order: 20
summary: `local`, `let`, `const` and `enum`, and the scope of each one.
---

Quirrel has four ways to bind a name: `local`, `let`, `const` and `enum`. All
four are scoped to the block where they are written, from the declaration to
the end of that block.

## let and local

`local` declares a variable that can be reassigned. `let` declares a binding
that cannot be reassigned. After its one initializing assignment, a later
write to it is a compile error. Use `let` by default. Use `local` only for a
variable that you intend to reassign, such as a counter or an accumulator.

A fixed `let` binding says nothing about the object it names. `let
ammoBelt = []` still allows `ammoBelt.append(...)` afterwards. Only
`ammoBelt = otherArray` is rejected. [freeze](sym:freeze) locks the object
itself; see below.

`let name` with no initializer forward-declares the binding. Exactly one later
statement in the same block must define it. This lets two closures capture
each other before either is assigned.

{{example:language/bindings-basic}}

`local` is a variable in the usual sense. `let` names a value once and rejects
a second assignment. Most bindings never change, and `let` states this for
both the reader and the compiler. This is why `let` is the default choice.

{{example:language/bindings-let-local}}

A binding can also carry a declared type - see
[Type annotations](page:language/annotations).

Reading a forward-declared `let` before its definition runs is a compile
error, and so is a second assignment:

```nut
let target
println(target)  // error: binding 'target' cannot be used before its definition
target = "tank"
target = "plane" // error: a 'let' binding accepts a single assignment
```

## Forward declaration

A `let` can be declared with no value and defined later, on its own line. This
is the only way to write two functions that call each other: the function
written first would otherwise name something that does not exist yet.

{{example:language/bindings-forward}}

The compiler tracks the definition, and each mistake has its own message:

| Mistake | What the compiler says |
| --- | --- |
| declared but never defined | `forward declaration 'name' is never defined` |
| read before its definition | `binding 'name' cannot be used before its definition` |
| defined twice | `binding 'name' is already defined; a 'let' binding accepts a single assignment (declare it with 'local' to allow more)` |

The last message shows the difference from `local`: a forward declared `let`
still accepts exactly one assignment, so it stays a binding that never changes
after it is set. Use `local` when the value must be reassigned.

## const and global const

`const` binds a compile-time value: a number, string, `null`, or a table or
array nested from those. Every place the name is used, the compiler
substitutes the value directly. There is no variable, no stack slot and no
lookup left at runtime. A function can be `const` too, if it captures no outer
variable.

Plain `const` is lexical only and leaves no trace at runtime; not even
[getroottable](sym:getroottable) sees it. `global const` also writes the name
into the shared `consttable`, reachable through
[getconsttable](sym:getconsttable). This is how a constant is made visible
beyond the file that declares it.

{{example:language/bindings-const}}

### What the compiler will evaluate

The value is not limited to a literal. Anything the compiler can evaluate on
its own is allowed, and the result is substituted:

| A `const` may be | Example |
| --- | --- |
| a simple value | `const MAGAZINE = 30` |
| a table or array of simple values, nested | `const LOADOUT = { belts = [1, 2, 3] }` |
| another constant, or a field reached from one | `const SECOND = LOADOUT.belts[1]` |
| an arithmetic, logical or conditional expression | `const RESERVE = MAGAZINE * 4` |
| a call to a pure function | `const CAP = max(MAGAZINE, 25)` |
| a function declaration that captures nothing | `const function armorAt(angle) { ... }` |

{{example:language/bindings-const-folding}}

`max(MAGAZINE, 25)` is not called at runtime. The compiler runs it once while
compiling and stores the result. This is what the [pure](page:attributes)
attribute is for, and the compiler enforces it. A call to a function not marked
pure is rejected with
`Only calls to pure functions are allowed in constant expressions`, so
`const RANDOM = rand()` does not compile.

Because the substitution happens at compile time, a `const` costs nothing to
read, cannot be reassigned by any code, and is available in places a runtime
value is not: inside another `const`, or as an `enum` member's value. The cost
is that its value must be known without running the program, and that a change
to it requires recompiling every file that used it.

### Folding outside const

The compiler also folds the constant parts of other expressions: an operator
on constants, a string template with constant values, and a call to a pure
function when the function and all its arguments are constants. The function
can be a constant, an import, or a member of one, such as `"abc".toupper` or
`math.sqrt`. A call that uses a local, a parameter or another run-time value
stays a normal call, and so does a nullable call `f?()`.

Outside `const`, an expression with a call in it folds only when its value is
simple: `null`, a bool, a number, a string or a userpointer. When the value is
a table, an array, an instance or another object, the expression runs each
time, so a function that makes an object makes a new one on each call. Only an
explicit `const` keeps such an object, as one immutable value.

The compiler gets the result by running the call while it compiles the file.
It runs the call even when the result is an object that it does not keep. So
when the call fails, the file does not compile, even if that line never runs:

```nut
from "math" import sqrt

const DEBUG = false
if (DEBUG)
  println(sqrt("a"))  // error: the compiler runs sqrt("a"), although DEBUG is false
```

The same is true in a function that nothing calls. In some places, a `?:`
whose condition and both branches are constants, such as
`DEBUG ? sqrt("a") : 0`, runs only the branch that its condition selects. But
this depends on where the `?:` is and on its result, so do not rely on it to
skip a call that fails.

To get the error at run time instead, for example in a test, give the call a
run-time value. After `let text = "a"`, the call `sqrt(text)` stays a normal
call, and `try` can catch its error.

{{example:language/bindings-call-folding}}

## enum and global enum

`enum` groups related constants under one name, accessed as `Enum.member`.
A member with no `=` gets an integer automatically. A member with `=` takes
an integer, a float or a string literal. Like `const`, a plain `enum` is
lexical only; `global enum` also goes into the consttable.

The counter for automatic values starts at 0 and advances only when it is
used. It counts only the automatic members, in their own order, and ignores
any explicit value written between them.

{{example:language/bindings-enums}}

## Scope and shadowing

In one function, a `local`, `let`, `const` or `enum` cannot reuse a name that
is still in scope. Constants, enums and imports from outside the function
count too. A parameter, a foreach variable or a catch variable can hide a
name, but not the name of the function it is declared in. Where such a
variable hides a constant, the name is the variable, so a constant expression
there cannot use it: with a parameter `T`, `const (T + 1)` is the error
`Identifier 'T' is not a compile-time constant`.

```nut
const MAX_SQUAD = 8
let squadSize = 4
{
  let squadSize = 8  // error: conflicts with existing local variable
}
function resize() {
  let MAX_SQUAD = 4  // error: conflicts with existing constant/enum/import
}
```

{{example:language/bindings-scope}}

## freeze

[freeze](sym:freeze) marks a table, array, instance, class or userdata as
immutable and returns a reference to it. The immutable flag lives on that
reference, not on the object. Any other reference to the same object made
before the freeze - a variable it was copied to, a value already stored
elsewhere - still writes through normally, because it was never marked.
Content is shared, so a write through such a reference is visible from the
frozen one too. Only the reference returned by `freeze` refuses to write.

{{example:language/bindings-freeze}}

Assign the result back over the original name (`t = freeze(t)`) when every
access to an object must go through a frozen reference. A separate mutable
alias defeats the freeze.
