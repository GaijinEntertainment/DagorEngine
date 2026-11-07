---
title: Control flow
group: Language
order: 40
summary: `if`, `while`, `for`, `foreach`, and `switch` (off by default).
---

Quirrel has the usual C-family statements: `if`/`else`, `while`, `do ... while`,
a C style `for`, `foreach`, and a `switch` that is off by default.

## if / else

A condition may be a plain expression, or a `local`/`let` declaration checked for
truth (`null`, integer `0` and float `0.0` are false; everything else is true).
The declaration may be followed by `;` and a separate condition. A binding
declared this way is visible in the whole chain: every `else if` and the final
`else` can see it.

{{example:language/control-flow-if-basic}}

{{example:language/control-flow-if}}

## while and do ... while

`while` checks the condition before each pass, so the body may run zero times.
`do ... while` checks it after, so the body always runs at least once.

{{example:language/control-flow-while}}

## for

`stat := 'for' '(' [init] ';' [cond] ';' [step] ')' stat`. Any part may be empty;
`for (;;) { ... }` is an infinite loop. `init` and `step` accept several
comma-separated declarations or expressions.

{{example:language/control-flow-for}}

## foreach

`foreach` iterates over an array, a table, a class, a string, or a generator. The
one-variable form binds the value. The two-variable form binds an index or key
first, then the value. The meaning of the first binding depends on the container:

| Container | one variable | two variables |
| --- | --- | --- |
| array | element | position (from 0), element |
| table, class, instance | value | key, value |
| string | character code (an integer, not a one-char string) | position (from 0), character code |

{{example:language/control-flow-foreach-array}}
{{example:language/control-flow-foreach-table}}
{{example:language/control-flow-foreach-string}}

A table's iteration order is not guaranteed, so a program that must be
deterministic sorts the keys itself, as the table sample above does.

A bound variable may also be a destructuring pattern instead of a plain name.
See [Destructuring](page:language/destructuring) for the pattern syntax and
defaults.

## break, continue, return

`break` leaves the innermost `for`, `foreach`, `while` or `do ... while` (or a
`switch`, see below). `continue` skips to the next pass of the innermost loop.
`return` leaves the function, not only the loop.

{{example:language/control-flow-loop-control}}

## Loop bodies and closures

Each pass of a loop body creates new `local`/`let` bindings. A closure created
inside the body and kept after the loop sees the value that the binding held on
the pass that created it. The bound variables of `foreach` follow the same rule,
because each pass gives them a new value (or key/value pair).

`for` is the exception. Its control variable lives in one slot for the whole
loop, and every pass changes that slot. A closure that captures the control
variable directly reads that same slot. After the loop ends, every such closure
sees the final value, not the value from its own pass. To capture the per-pass
value, copy it to a new `let` inside the body.

{{example:language/control-flow-closures}}

## switch

`switch` is disabled by default. With no directive, `switch`, `case` and
`default` are ordinary identifiers, and `switch (x) { ... }` fails to compile.
The `#allow-switch-statement` compiler directive at the top of a file turns
the keywords on for that file. See
[Compiler directives](page:language/directives).

```nut
switch (x) {
  case 1:
    println("one")
}
```

A `case` without a `break` falls through into the next `case`. Several labels
can share one body when they are written together with no code between them.
`default`, if present, must be the last clause; a `case` after it fails to
compile.

{{example:language/control-flow-switch}}

## Notes

- A `local`/`let` declared in an `if` condition, a `for` init, or bound by
  `foreach` does not leak past the statement's own body.
- `while` does not accept a declaration in its condition; only `if` does.
