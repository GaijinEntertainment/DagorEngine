---
title: Tables and arrays
group: Language
order: 45
summary: Tables and arrays: slots, order, growth and freezing.
---

A table is an associative container: a set of slots, each with a key and a
value. The key can be any value. An array is a sequence of values indexed
from `0`.

## Table literals

`{ key = value, ... }` builds a table. A trailing comma after the last field
is allowed, so a field added at the end changes only one line in a diff.
A key does not have to be a plain identifier: write a quoted string before a
`:` for a key that is not a valid name, or `[expr] = value` for a key
computed at construction time. Braces nest, so a literal can hold another
literal as a value.

{{example:language/containers-basic}}

{{example:language/containers-literals}}

## Array literals

`[value, ...]` builds an array. The comma between elements is optional:
`[1 2 3]` and `[1, 2, 3]` are the same array. A missing comma next to a
unary `-` or a call can merge two elements into one by accident.

## Spread

`...expr` inside a table or an array literal copies the contents of `expr`
into the literal that is being built. In a table literal the source can be a
table, a class or an instance. In an array literal it must be an array. Any
other type throws. A `null` source adds nothing, so an optional part can go
into a literal with no branch around it. A [class](page:language/classes)
body takes no spread.

The copy is shallow. A slot holding a [weakref](sym:types.WeakRef) arrives as
the value it points at, the same value a reader of that slot gets. The copy
is made in the order the literal is written: a key that comes after a spread
replaces the same key the spread brought in, and a spread that comes after a
key replaces that key.

You cannot write the same key twice. This is a compile error, in a `const`
initializer too. The check does not count the keys a spread brings in.

The result is a new container, never the source. A spread of a
[frozen](sym:freeze) source gives a copy that can be written to. Values stay
frozen if they came from a frozen source, since the immutable flag follows
the reference.

{{example:language/containers-spread}}

A `const` initializer accepts a spread too, and folds it at compile time. The
source must then be a constant of the matching type (a constant table for a
table literal, a constant array for an array literal) or a constant `null`,
which adds nothing here too.

## Reading and writing a slot

`.name` and `["expr"]` read or write an existing slot the same way on a
table, an array (with an integer index), or a class instance. Writing with a
plain `=` requires the slot to exist already. An assignment to a name that is
not there yet throws; it does not create the slot. This is by design. A typo
in a field name becomes an error instead of a new unrelated slot in the
table. `<-`,
the newslot operator, adds a new slot; see
[Operators and expressions](page:language/operators) for its full behavior,
including why it rejects a bare local variable.

{{example:language/containers-slots}}

`<-` does not work on an array, even at an index that already holds a
value: `arr[0] <- 99` throws `indexing array with integer` every time. Grow an
array with [`append`](sym:types.Array.append), [`insert`](sym:types.Array.insert)
or [`resize`](sym:types.Array.resize), and write an existing element
with plain `[i] = value`.

## Iteration order

`foreach` over a table (and [`keys`](sym:types.Table.keys),
[`values`](sym:types.Table.values), [`topairs`](sym:types.Table.topairs))
walks the slots in an order that depends on the table's internal layout, not
on insertion order. That order is reseeded for every run, so the same script
can print two different orders on two runs of the same build with no code
change. Sort the keys first when the order has to be stable; see
[Control flow](page:language/control-flow) for the full `foreach` pattern.
An array keeps the order its elements were given, since it is indexed by
position and not by a hashed key.

## delete is forbidden

The `delete` operator is forbidden by default, so `delete t.key` fails to
compile with `Usage of 'delete' operator is forbidden. Use
'o.$rawdelete("key")' instead`. Call
[`rawdelete`](sym:types.Table.rawdelete), which removes the slot and returns
the value that was in it. See
[Operators and expressions](page:language/operators) for a runnable example
of both sides of that error, and for `#allow-delete-operator`, which lifts
the restriction.

## Methods

Both types have a full set of methods reached with `.`: a table's
[`rawget`](sym:types.Table.rawget), [`rawset`](sym:types.Table.rawset),
[`rawin`](sym:types.Table.rawin), [`clear`](sym:types.Table.clear),
[`clone`](sym:types.Table.clone), [`map`](sym:types.Table.map),
[`filter`](sym:types.Table.filter) and others on the
[table](sym:types.Table) page, and an array's
[`append`](sym:types.Array.append), [`remove`](sym:types.Array.remove),
[`sort`](sym:types.Array.sort), [`slice`](sym:types.Array.slice) and others
on the [array](sym:types.Array) page. This page covers only the literal
syntax and the slot semantics the methods build on.

## Edge cases

- A key can be any value, including `null`. `{[null] = 1}` is a legal,
  one-slot table, and iteration over it works like over any other slot. Only
  the shorthand below is limited to identifiers.
- A bare name inside a table literal is shorthand for `name = name`, reading
  the value from a variable already in scope: `{ squadStrength }` is
  `{ squadStrength = squadStrength }`. This shorthand and the quoted-string
  key form are both table-only. Neither parses in a [class](page:language/classes)
  body.
- `clone t` makes a shallow copy: top-level slots are copied, but a table or
  array nested inside is shared with the original. `clone` is a keyword, so
  `t.clone()` is a parse error; write `clone t` (see
  [`clone`](sym:types.Table.clone) for the full rule, including what it does
  to a frozen table).
- [`freeze`](sym:freeze) returns an immutable reference to a table or array; see
  [Bindings and constants](page:language/bindings) for how the immutable
  flag follows the reference, not the object.
