---
title: Cheat sheet
group: Guides
order: 91
summary: The language on two pages.
layout: cheatsheet
---

The language on two pages.

## Values

| | |
| --- | --- |
| `null true false` | the three keywords |
| `42 -7 0x1F` | integer, decimal or hex |
| `1_000_000` | `_` is ignored |
| `1.5 1e3 1.0e-3` | float; `1 == 1.0`, types differ |
| `'A'` | integer 65, no char type |
| `"text"` | string |
| `@"raw\no escape"` | verbatim string |
| `$"got {n} of {t}"` | interpolation; nests, `\{` escapes |
| `[1, 2, 3]` | array |
| `{ a = 1, b = 2 }` | table |
| `[...a, v]`, `{ ...t, k = v }` | spread: copies `a` or `t` into the literal |

Value types: `null`, `bool`, `integer`, `float`.
Reference types: `string`, `array`, `table`, `function`, `class`, `instance`, `generator`, `thread`, `weakref`, `userdata`.

Falsy are only `null`, `false`, `0`, `0.0`.
`""`, `[]` and `{}` are true.

## Bindings

| | |
| --- | --- |
| `let x = 1` | bind once; the default |
| `local x = 1` | reassignable |
| `const MAX = 10` | compile time, folded |
| `enum E { A = 1 }` | compile time |
| `global const` | visible outside the file |
| `let f;` | forward declaration |

`let` fixes the *name*, not the value: `let t = {}` still allows `t.x <- 1`. Use
[freeze](sym:freeze) for the contents.

## Control flow

```nut
if (a) { } else if (b) { } else { }
while (c) { }
do { } while (c)
for (local i = 0; i < n; i++) { }
foreach (v in arr) { }
foreach (i, v in arr) { }   // index first
foreach (k, v in tbl) { }   // no order
break   continue   return
```

No `switch` by default; `#allow-switch-statement` at the top of a file turns it on.

## Functions

```nut
function add(a, b) { return a + b }
let mul = @(a, b) a * b        // lambda
function greet(n, hi = "hi") { }
function sum(...) { return vargv.len() }
function [pure] sq(x) { return x * x }
```

Types are optional, checked at compile time where inferable, else at run time
(see [builds without checks](page:language/annotations#builds-without-checks)):

```nut
function scale(v: number, by = 1.0): float {
  return v * by
}
```

`int float number bool string null table array function class instance generator
thread weakref userdata any`, joined by `|`. `number` is `int|float`.

## Classes

```nut
class Squad {
  name = "none"        // per instance
  static MAX = 12      // on the class only, read-only
  constructor(name) { this.name = name }
  function report() { return this.name }
}
class Tank(Squad) { }  // extends Squad
```

`this.` is required in a method; a bare name is not a field.
[Metamethods](page:language/metamethods) customize operators. `==` is not one.

## Arrays

| | |
| --- | --- |
| `a.append(v, w)` | not `push`; takes many |
| `a.insert(i, v)` | |
| `a.remove(i)` | |
| `a.indexof(v)` | not `find`; `null` if absent |
| `a.contains(v)` | by value |
| `a.slice(1, -1)` | negative from end; clamps |
| `a.sort()` | `sort(cmp)`, `cmp` gives -1 0 1 |
| `a.map(f)` `a.filter(f)` `a.each(f)` | `f([value], [index], [container])`, as many as `f` declares |
| `a.reduce(f)` | `f([accumulator], [value], [index], [container])` |
| `a.totable()` | values become keys, or |
| | `[[k, v], ...]` becomes slots |

## Tables

| | |
| --- | --- |
| `t.keys()` `t.values()` | |
| `t.len()` | |
| `t.x <- 1` | create/replace a slot |
| `t.x = 1` | assign; throws if absent |
| `t.$rawdelete("x")` | remove; `delete` is off |
| `k in t` | `k not in t` |

On an array, `in` tests the index, not the value.

## Strings

| | |
| --- | --- |
| `s.len()` | bytes, not characters |
| `s[0]` | the byte, an integer |
| `s.slice(a, b)` | |
| `s.indexof(sub)` | `null` if absent |
| `s.toupper()` | `.tolower()` `.strip()` |
| `s.split(",")` | to an array |
| `", ".join(arr)` | back to a string |
| `"".concat(a, b)` | |
| `s.startswith(p)` | `.endswith(p)` |
| `"{0}/{1}".subst(a, b)` | |

A string is bytes, so `slice` can cut a UTF-8 character in half.

## Null safety

| | |
| --- | --- |
| `a?.b` `a?[i]` `a?()` | `null` instead of throwing |
| `a ?? fallback` | only when `a` is `null` |

`0` and `false` survive `??`. One `?.` covers the rest of the chain.

## Modules

```nut
from "math" import clamp     // compile time
import "utils.nut" as utils
let m = require("math")      // run time
let p = require_optional("p.nut")
```

A file starts with its `#` directives, then every `import`, then the other
statements. A module returns its exports:
`return freeze({ a, b })`.

## Traps

- **`+` concatenates if either side is a string** and never throws. `1 + "2"` is
  `"12"`; `2 + 3 + "1"` is `"51"` but `"1" + 2 + 3` is `"123"`. Every other
  arithmetic operator throws. Use `$"..."`.
- **`clone` binds looser than a call.** `clone(a).append(9)` parses as
  `clone (a.append(9))` and mutates `a`. Write `(clone a).append(9)`.
- **A missing slot named like a type method is not `null`.** When `t` has no
  `filter` slot, `t?.filter` is `Table.filter`. Test with `"filter" in t`.
- **`foreach` and the callbacks disagree on order.** `foreach (i, v in arr)` puts
  the index first; `arr.map(@(v, i) ...)` puts the value first.
- **`==` ignores `_cmp`.** Two instances are equal only if they are one object.
- **Integer division truncates.** `7 / 2` is `3`, `-7 / 2` is `-3`.
- **`??` binds looser than comparison**: `a ?? b > c` is `a ?? (b > c)`.
- **Shifts bind tighter**: `1 << 2 == 4` is `(1 << 2) == 4`.
- **A mutable default parameter is shared** between calls. Never mutate it.
- **Table order is undefined** and seeded. Sort the keys when it matters.
- **No bool in arithmetic**: `1 + true` throws. Bitwise needs integers.

## Tooling

```
sq f.nut          run
sq -sa f.nut      analyzer
sq --warnings-list
sq -D:<name> -sa f.nut
```
