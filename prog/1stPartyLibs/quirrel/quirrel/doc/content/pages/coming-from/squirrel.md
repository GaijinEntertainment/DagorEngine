---
title: Coming from Squirrel
group: Introduction
order: 4
summary: Code the compiler now rejects, and changes that compile but behave differently.
layout: mapping
---

Quirrel is Squirrel with the ambiguous parts removed. The language grew out of
Gaijin's Squirrel codebase. Nearly every change has one goal: a mistake fails at
compile time, not at run time. A Squirrel file rarely compiles unchanged, but the
compiler names what it rejects and usually names the replacement too.

Read [what changed quietly](#what-changed-quietly) first. The compiler reports
everything else.

## What the compiler rejects

| Squirrel | Quirrel |
| --- | --- |
| `class B extends A { }` | `class B(A) { }` | page:language/classes#declaring-a-class |
| `# a comment` | [`//`](page:language/lexical#comments); `#` now starts a [compiler directive](page:language/directives) |
| `0123` | `0x7B` or `123`: `leading 0 is not allowed, octal numbers are not supported` | page:language/lexical#numbers |
| `delete t.k` | `t.$rawdelete("k")`; the `delete` operator is off by default | sym:types.Table.rawdelete |
| `a.push(v)` | `a.append(v)`, and it takes several values | sym:types.Array.append |
| `a.find(v)`, `s.find(sub)` | `a.indexof(v)`, `s.indexof(sub)` | sym:types.Array.indexof |
| `f.getinfos()` | [getfuncinfos](sym:types.Function.getfuncinfos), which also works on a callable table or instance |
| `t.setdelegate(d)`, `t.getdelegate()` | gone; use a class |
| `function A::m() { }` | declare `m` in the class body, or `A.m <- function() { }` before the class is used |
| `class A { </ x = 1 /> }` | class and member attributes are gone |
| `local a = (1, 2)` | the comma operator is gone |
| `A() { x = 1 }` post-initializer | set the slots in the constructor |
| `setroottable`, `setconsttable` | gone |
| `seterrorhandler(f)` | [debug.seterrorhandler](sym:debug.seterrorhandler) |
| `rawcall` | gone |
| `switch (x) { }` | `switch` is deprecated and off by default; `#allow-switch-statement` at the top of the file turns it back on | page:language/control-flow#switch |
| `format("%d", n)` | [`from "string" import format`](page:language/modules#import-and-from-import) first, or [interpolate](page:language/strings#interpolated-strings) it as `$"{n}"` |
| `f()` inside a method, meaning `this.f()` | write `this.f()`: there is no implicit `this` lookup |

## What changed quietly

These compile and run. Test a ported file; a build alone does not find them.

- **`filter` swapped its callback arguments.** Squirrel passes `(index, value)`;
  Quirrel passes `(value, index)`. This agrees with `map` and `reduce`, and with
  `foreach`, where the index is optional. Code that filtered on the index still
  compiles and now reads the value.
- **A function or class at file scope is local.** It is no longer a slot in the root
  table, so nothing outside the file can find it by name. Return it from a module;
  see [modules](page:language/modules).
- **`const` and `enum` are local too**, unless they are declared `global const` or
  `global enum`.
- **`switch` is off by default.** Without `#allow-switch-statement` the words
  `switch`, `case` and `default` are ordinary identifiers, so the block fails to
  parse.
- **`delete` is off by default** in the same way, and the compiler reports the
  replacement.
- **A callback can take the container as a last argument.** `map`, `filter` and
  `reduce` pass it, so `this` inside the callback is no longer the array.
- **`array.append` and `array.extend` take several values**, so an accidental
  second argument is no longer ignored.
- **The static analyzer runs on request**, `sq -sa file.nut`, and has about a
  hundred separate checks. It finds unused bindings, unreachable code,
  suspicious null handling and more. Treat its output as part of the port.

## What is new

- `let` for a binding that cannot be assigned again; it is the default choice.
  `local` still works where a variable changes.
- [freeze](sym:freeze) for a table, array or instance that must not be edited, and
  `is_frozen` to ask.
- The null-safe operators `?.`, `?[`, `?()` and `??`.
- String interpolation: `$"got {n} of {total}"`, which nests.
- Destructuring: `let [a, b] = pair`, `let { x, y } = point`, the same in a
  parameter list, and the shorthand table `{ x, y }`.
- A module system with two forms: `require("m")` at run time and `import "m"` at
  compile time. Both are on the [modules](page:language/modules) page.
- `.$method` calls a type method even when a slot has the same name, so
  `data.$len()` is the length no matter what `data.len` holds.
- `not in`, `1_000_000`, docstrings written `@@"text"`, inline `const` for a
  value folded at compile time, type annotations, and compiler directives.
- `println` and `errorln`, and every print function takes several values.
- String methods moved onto the type: `strip`, `split`, `startswith`, `endswith`,
  `indexof`, `subst`, `replace`, `join`, `escape`, `hash`. The
  [string](sym:string) module keeps its own copies.
- Tables and arrays gained `each`, `map`, `filter`, `reduce`, `findvalue`,
  `findindex`, `contains`, `hasindex`, `hasvalue`, `swap`, `replace_with`,
  `totable`, `topairs`, `__merge` and `__update`.

## How to port a file

- Compile it, and fix what the compiler names, top to bottom. Most of it is
  `extends`, `#`, `push`, `find` and the implicit `this`.
- Search for `filter(` and check the argument order in every callback.
- Search for the names that other files read through the root table, and export
  them from a module.
- Run `sq -sa` over the file and read the diagnostics.
- Change `local` to `let` where nothing assigns the name again. The analyzer
  points these out.

{{example:coming-from/squirrel}}

## See also

- [Cheat sheet](page:cheatsheet) - the language on two printable pages
- [Traps](page:cheatsheet#traps) - traps for every user, not only Squirrel users
- [Bindings and constants](page:language/bindings) - `let`, `local`, `const`, `global`
- [Compiler directives](page:language/directives) - the `#` lines that turn
  features on and off in a file
- Coming from [Python](page:coming-from/python),
  [JavaScript](page:coming-from/javascript), [Lua](page:coming-from/lua)
