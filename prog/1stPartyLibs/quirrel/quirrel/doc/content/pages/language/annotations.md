---
title: Type annotations
group: Language
order: 80
summary: Type annotations on a parameter, a return type or a binding.
---

A name may carry `: Type` after it: a parameter, a return type after `):`,
a `local` or `let` declaration, a destructured field or element, or the
vararg tail. This is core syntax and is always parsed. No flag turns it on
or off in a script.

## Syntax

All fourteen types from [Values and types](page:language/types) work as
annotations. Three more exist only in annotations: `number` is short for
`int|float`, `any` accepts every value and turns the check off for that
parameter, and `userpointer` matches a raw pointer a native binding pushed.
Combine types with `|`. Parentheses may group a union. A default
value works together with a type; the common use is an optional nullable
parameter, `hp: int|null = null`.

{{example:language/annotations-syntax}}

## What the compiler does

An annotation becomes a check at the point it guards: a parameter is checked
when its function is entered, a return value when the function returns, a
declared or assigned variable when the write happens, and a destructured
field or element when the destructuring runs.

When the type of the incoming value is not known until the check runs (a
parameter, most assignments), the check is a runtime check, and a mismatch
throws. When the type is known from the expression alone (a literal assigned
directly), the compiler reports the mismatch at compile time:

```nut
function badReturn(): int {
  return "not an int"  // error: expression of type 'string' cannot be
}                       // assigned to type 'int', caught at compile time
```

{{example:language/annotations-runtime-check}}

## Builds without checks

A build that sets `SQ_RUNTIME_TYPE_CHECK` to 0, as the Dagor `rel` and `irel`
builds do, does not check annotations at runtime: a wrong type does not throw.
Its compiler also skips type inference, thus it does not report a mismatch
that only inference finds, such as a literal of a wrong type or an async call
assigned to a typed variable. Run the tests of a script on a build that
checks. The parameter checks of native functions stay, and so does the class
check of a typed `catch`.

## What it does not do

An annotation is a check, not a conversion. It never converts the value to
the declared type. `x: number` accepts an int or a float as given, so a
function that receives only ints returns an int. Quirrel has no function
overloading, so an annotation cannot select between two bodies by argument
type. There is one body for each function. The check runs only when a value
arrives. An annotation does not prove that every caller passes the right
type; it only shows that no caller so far has passed a wrong one.

{{example:language/annotations-no-coercion}}

## Declaration strings

A native function has no Quirrel source to annotate. Its binding carries the
same syntax as a string: `pure fastcall type(obj): string`,
`getbuildinfo(): table`.
There is one declaration string per native function. Every signature box on
this site is rendered from these strings. `sq --parse-types somefile.txt`
parses a file of such strings, one per line, and prints the result. This is
how the string grammar is tested:

```text
sq --parse-types decls.txt   # decls.txt holds one declaration per line

pure clampAmmo(current: int, maxAmmo: int): int

pure clampAmmo(current: int, maxAmmo: int): int
  functionName: clampAmmo
  returnTypeMask: 0x2
  objectTypeMask: 0xffffffff
  ellipsisArgTypeMask: 0x0
  requiredArgs: 2
  argCount: 2
  pure: true
  nodiscard: false
```

`--parse-types` is a tool for that string grammar. It does not enable
annotations in ordinary scripts; those are always parsed.
