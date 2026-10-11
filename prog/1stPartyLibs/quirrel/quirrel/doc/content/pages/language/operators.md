---
title: Operators and expressions
group: Language
order: 35
summary: Operator precedence, values that count as false, and the null-safe operators.
---

Most operators do what the same symbol does in C. The differences are which
values count as false, what `??` and `?.` check, and some precedence traps.

## All operators

Every operator the compiler accepts, with the metamethod that overloads it where
there is one. Each links to the section that explains it.

{{operators}}

There is no bitwise compound assignment: `&=`, `|=`, `^=`, `<<=` and `>>=` are all
compile errors. Write `flags = flags & mask` instead.

## Arithmetic

`+ - * /` and `%` work on `int` and `float`. Division of two `int` truncates
toward zero. An `int` mixed with a `float` gives a `float`. `%` keeps the
sign of the left operand, like C; it is not the mathematical modulo.

`+` also concatenates: if either side is a string, it converts the other side
to a string, so `1 + "2"` is `"12"` and not an error. Prefer `$"...{...}"`
interpolation to build strings.

{{example:language/operators-arithmetic}}

## Comparison and three-way compare

`== != < <= > >=` compare two values and return a `bool`, not `0`/`1`.
`<=>` is the three-way compare. It returns an `int` that is negative, zero,
or positive when the left side is less than, equal to, or greater than the
right. This is the result [sort](sym:types.Array.sort) expects from a
comparator.

{{example:language/operators-compare}}

## Logical operators

`&&`, `||` and `!` treat `null`, `false`, the integer `0` and the float `0.0`
as false. Everything else is true, including `""`, `[]` and `{}`. Quirrel
does not treat an empty string, array or table as false.

`&&` and `||` short-circuit and evaluate to one of their operands, not
always a `bool`. `&&` evaluates its left side; if that is false, it stops
and yields it; otherwise it evaluates and yields the right side. `||` is the
mirror image. Only `!` always produces a `bool`.

{{example:language/operators-logical}}

## Null-coalescing and null-safe access

`??` looks like `||` but tests strictly for `null`. The false-but-valid
values above (`0`, `0.0`, `false`) pass through it. A plain `||` used as a
default replaces a real `0` or `false` with the fallback; `??` does not.

`?.` and `?[` are the null-safe forms of `.` and `[`. If the value on the
left is `null`, the whole expression is `null` and nothing throws. Once one
of them fires on a `null`, the compiler treats the rest of the chain (further
`.`, `[`, or a call) as null-safe too. `a?.b.c[0]()` needs only one `?.`, not
one at every step.

{{example:language/operators-nullsafe}}

## in, instanceof, typeof

`key in container` and `key not in container` test whether a slot exists.
On a table this means the key is present. On an array it means the key is a
valid index. This is a common trap: `20 in [10, 20, 30]` is `false` because
`20` is not a valid index, even though it is one of the values. Use
[contains](sym:types.Array.contains) to search by value.

`instanceof` tests whether an instance was made from a class or one of its
subclasses. `typeof` returns the type name as a string (`"table"`,
`"instance"`, `"array"`, ...), through the `_typeof`
[metamethod](page:language/metamethods#type-name-and-text) when the class defines
one. [type](sym:type) answers the same question as a function. It can be passed
as a callback, and it reports the plain type regardless of `_typeof`.

{{example:language/operators-membership}}

## The .$ type-method operator

A table's own slots and its built-in methods share one namespace, so a slot named
`len` or `rawdelete` hides the method of that name. `o.$name(...)` calls the
built-in type method directly and never sees the slot. The null-safe form,
`o?.$name(...)`, works the same way.

{{example:language/operators-typemethod}}

Plain `o.name(...)` is safe on data whose keys you control. On data parsed from
a file, received over a network, or passed in by a caller, use `.$`, because the
data cannot break it. The compiler's message for the forbidden `delete` operator
uses this form: `Use 'o.$rawdelete("key")' instead`. See
[types.Table.rawdelete](sym:types.Table.rawdelete).

## clone

`clone value` makes a shallow copy of a table, array, or instance: the
top-level slots are copied, but a container nested inside is shared with the
original.

`clone` is a keyword, not an identifier, so there is no `.clone()` method by
default:

```nut
let copy = original.clone()  // parse error: expected 'IDENTIFIER'
```

The `.$` form fails the same way, because the keyword is the problem, not the
slot lookup. Write `clone original` instead, or `original["clone"]()`.

With [`#forbid-clone-operator`](page:language/directives#delete-and-clone)
the operator is off and `clone` is an ordinary identifier, so
`original.clone()` and `original.$clone()` compile and call the
[clone](sym:types.Table.clone) type method.

{{example:language/operators-clone}}

## The newslot operator, <-

`<-` adds a new field to a table or instance. If the slot already exists,
`<-` behaves like `=`. The reverse is not true: plain `=` never creates a
slot. An assignment to a field that does not exist yet throws. `<-` is the
only way to add a slot.

`<-` needs a table or instance slot on its left, not a bare variable. `local
a; a <- 1` is a compile error ("can't 'create' a local slot"), because a
local binding is not a container that can hold new slots.

{{example:language/operators-newslot}}

## Compound assignment and increment/decrement

`+= -= *= /= %=` read the current value, combine it, and write it back, so
(like plain `=`) they need the slot to exist already. `++` and `--` work as
in C. As a statement the two forms do the same thing. As an expression the
prefix form yields the value after the step and the postfix form yields the
value before it.

{{example:language/operators-compound}}

## Overloading an operator

A class defines a metamethod to give its instances behaviour for an operator. The
name is the operator's metamethod from the table above, and it is an ordinary
method on the class.

{{example:language/operators-overloading}}

Three things to watch in that class:

- **A class cannot name itself inside its own body.** The binding does not exist
  until the declaration finishes, so `MyPoint2(...)` inside `_add` is
  `Unknown variable [MyPoint2]`. Use `this.getclass()`. It also works when a
  subclass inherits the metamethod. A
  [forward declaration](page:language/bindings) works too when the name is
  needed.
- **One `_cmp` drives the ordering comparisons.** `<`, `<=`, `>` and `>=` all go
  through it. Return a negative number, zero, or a positive one, as `<=>` does.
  `==` and `!=` are not among them. They compare raw identity, and no metamethod
  can change that.
- **`_typeof` changes `typeof`, not [type](sym:type).** `typeof muzzle` gives
  `MyPoint2` while `type(muzzle)` still gives `instance`, because `type` ignores
  metamethods by design.

An operator with no metamethod on the class throws; there is no default
fallback. An instance that defines `_add` but not `_sub` cannot be subtracted.

[Metamethods](page:language/metamethods) gives the contract of each one: what it
receives, which operand supplies it, and what the VM does with the returned
value.

## Precedence traps

- `??` binds looser than the comparison operators, so `a ?? b > c`
  parses as `a ?? (b > c)`, not `(a ?? b) > c`.
- Shift operators (`<< >> >>>`) bind tighter than comparisons, so
  `1 << 2 == 4` parses as `(1 << 2) == 4`.
- `&&` binds tighter than `||`, as in most languages. Parenthesize a
  mixed chain when the reading is not obvious.
- Assignment is a statement, not a nestable expression: `p = q = 5` is a
  compile error ("'=' inside 'expression' is forbidden"). Write two
  statements instead.

{{example:language/operators-precedence}}

## delete

The `delete` operator is forbidden by default:

```nut
delete tbl.someKey
// Usage of 'delete' operator is forbidden. Use 'o.$rawdelete("key")' instead
```

Use `rawdelete`. It does the same removal as a regular method call and
returns the removed value.

{{example:language/operators-rawdelete}}

See [Errors and exceptions](page:language/errors) for how a caller reacts
to the errors shown above.
