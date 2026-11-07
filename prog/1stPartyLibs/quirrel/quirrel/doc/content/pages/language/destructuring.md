---
title: Destructuring
group: Language
order: 60
summary: Patterns that bind a table's keys or an array's positions to names.
---

A `local` or `let` declaration can bind a table's keys or an array's positions
directly to named bindings.

## Table by key, array by position

`let { a, b } = table` binds `a` and `b` from the keys of the same name.
`let [a, b] = array` binds them from positions 0 and 1. `local` destructures the
same way. Commas between fields are optional in both forms.

{{example:language/destructuring-basics}}

## Defaults, and a key with no default

`let { a = 0 } = table` binds `a` from the key `a`. It uses the default
expression only if that key is missing. An array pattern takes a default the
same way for a missing position. A field with no default throws if its key or
position is missing; it does not become `null`.

{{example:language/destructuring-defaults}}

A default applies only when the key or position is missing. A key that is
present and holds `null` is not missing: it binds `null`, and the default is not
used.

## Type hints in a pattern

A field may carry a type hint, written the same way as a parameter annotation. A
hint and a default can appear together, in that order.

{{example:language/destructuring-hints}}

In a build that checks annotations, the hint is checked against the value that
arrives, and a mismatch throws
`type 'string' differs from the declared type 'int'`. Because a present `null` is
not missing, it reaches the hint and the hint rejects it: `{ crew : int = 2 }`
does not turn a `null` crew into `2`. A
[build without checks](page:language/annotations#builds-without-checks) binds
the value as it arrives.

## There is no rename syntax

Unlike JavaScript, a table pattern cannot bind a key under a different name.
`{ key }` always binds a variable named `key`; writing `=` after a field name
supplies its default, not an alias:

```nut
// this does NOT rename hitPoints to hp - it tries to use an undeclared
// variable named hp as hitPoints's default, and fails to compile
let { hitPoints = hp } = vehicle
```

To bind under a different name, destructure first and then assign:
`let { hitPoints } = vehicle; let hp = hitPoints`. Only the `from "module"
import` statement below renames directly, with `as`.

## A pattern is one level deep

A field inside `{ }` or `[ ]` is always a plain name, with an optional type hint
and default. It is never another `{ }` or `[ ]` pattern. There is no nested
destructuring. To reach into a nested table or array, destructure one level,
then destructure the result:

{{example:language/destructuring-nested}}

## Destructuring in a from-import

`from "module" import a, b as c` imports specific exports of a module by name.
An optional `as` binds an export under a different local name. `*` imports
every export. `import "module"` binds the whole module under one name. Most
game code uses the `from` form to import a few names from a shared module.

{{example:language/destructuring-import}}

## Destructuring in a foreach

A `foreach` binder accepts a pattern, so the fields of each element can be bound
directly. An index or key may still come first, and an array element
destructures by position.

{{example:language/destructuring-foreach}}

## Where else a pattern is accepted

A pattern is accepted in a `let`/`local` declaration, in a `foreach` binder, in a
`from ... import` list, and in a function parameter list; see
[Functions](page:language/functions). It is not accepted anywhere else. A bare
`{ name, strength } = squad` assignment does not parse, because a pattern only
introduces new bindings.
