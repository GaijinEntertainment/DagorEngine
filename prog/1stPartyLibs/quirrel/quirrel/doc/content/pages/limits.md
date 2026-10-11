---
title: Implementation limits
group: Guides
order: 95
summary: Per-function limits of the bytecode, and the errors they report.
---

The compiler rejects a few shapes of code that the bytecode cannot express. Each
error message is clear, but it does not say which limit is behind it. This page
lists the limits.

None of them limits the size of a script. They are all per function, and
splitting the work into smaller functions clears every one of them.

## Stack slots per function

A function has **255** stack slots. `this` takes the first slot, then the
parameters, then the locals, then the temporaries an expression needs while it
is evaluated. They all come from the same budget, so a function with many
parameters has room for fewer locals.

```
too many function stack slots: cannot allocate local 'v254' at slot 255;
bytecode supports at most 255 slots per function
```

A second wording appears when the slot number itself cannot be encoded, because
`0xFF` is reserved as a marker:

```
too many function stack slots: bytecode supports at most 255 slots per function;
slot 255 cannot be encoded because 0xFF is reserved
```

A function that runs out of slots is usually too large and should be split. A
generated function that needs hundreds of values should hold them in an array
or a table, which costs one slot.

## Nesting depth

The parser has one depth budget of **500** units for the whole source tree. It
reports the same message for every construct that exhausts it:

```
AST too big. Consider simplifying it
```

A unit is a grammar rule entered, not a level of nesting in the text, so the
cost of a construct varies a lot. An expression in parentheses walks the full
precedence chain each time and costs about fifteen units per level. A statement
block costs about two. Measured against the current compiler:

| construct | levels that still compile |
| --- | --- |
| parenthesised expression, `((((1))))` | about 33 |
| nested array or table literal | about 33 |
| nested `if` | about 157 |
| nested statement block, `{ { { } } }` | about 236 |

Hand-written code does not come near these limits. Generated code does. A rule
compiler that emits one nested `if` per condition, or an expression builder
that adds parentheses at every step, hits the expression limit quickly. Emit a
flat chain, or a table the runtime walks, instead of a deeper tree.

## Active catch clauses per function

The bytecode holds at most **255** simultaneously active traps in one function,
which means that many nested `try` blocks:

```
too many active catch clauses: bytecode supports at most 255 simultaneously
active traps per function
```

Nesting `try` that deeply exhausts the parser's depth budget first, so this
message is hard to reach from source. A bytecode generator that emits traps
without matching source nesting can still hit it.
