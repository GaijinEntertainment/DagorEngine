# gen1-to-gen2

Converts daslang v1 syntax to gen2. Replaces the C++ `gen1-to-gen2` binary that lived here,
which carried a fork of the v1 bison grammar.

```
daslang utils/gen1-to-gen2/main.das -- -p file.das [-p other.das] [-i] [--semicolon] [--gen15] [--verify]
```

- `-p` file to convert, repeatable
- `-i` rewrite in place (default prints to stdout; more than one file needs `-i` or `--verify`)
- `--semicolon` keep the statement-terminating `;`
- `--gen15` stop at gen1.5: make-syntax only, no braces or parens
- `--verify` report what would change, write nothing

## How it works

One pass, no re-parsing.

1. **Read once.** The source is an immutable string plus a line index (`offsets.das`); every
   `LineInfo` becomes an absolute byte offset up front, so no step does line/column arithmetic.
2. **Parse with infer off.** `parse_file` (ast module) hands over the tree as parsed: make-syntax
   unlowered, generics unresolved, nothing folded or generated, and a file that does not
   type-check still yields a tree.
3. **Collect edits.** Each rule in `convert.das` records `(from, to, text)` in *original*
   offsets. Rules never write text and never see each other's output.
4. **Apply once.** `patch.das` sorts the edits and emits the result in a single forward walk.

Nesting needs no special handling. Every rule edits only the delimiters of its own construct,
which lie between its children, so a parent's edits and its children's are disjoint ranges:

```
[[Bar a=[[Foo b=2.0]]]]     inner: [[ -> Foo(    ]] -> )
                            outer: [[ -> Bar(    ]] -> )
                            one pass, four disjoint edits
```

An "after X" edit is a zero-length insert at an original offset, so it lands correctly however
much the construct it follows grew or shrank. Two edits that overlap without nesting are refused
and reported rather than merged. Two at the same offset order by rank: inner closer, outer
closer, then statement terminator.

## Comments are never lost

A comment is the one thing no tool can regenerate, so preservation is structural, not per-rule
discipline:

- untouched bytes are copied verbatim - a rule cannot lose a comment by omission
- a `remove` whose range holds a comment or string literal is refused (`patch.das`)
- gap-collapse rules require the gap to be blank (`is_blank`), which is what leaves
  `try //a` / `recover //b` alone
- an insertion anchored at a line edge steps past a block comment that ends on a later line
  (`line_edge`)
- every conversion is gated on `check_comments_preserved`: the ordered comment sequence of the
  input must equal the output's, which catches loss, duplication and reordering at once

A comment may MOVE to an adjacent anchor when the syntax around it is restructured - the gate
compares the texts, not the positions.

## Notes on v1 spans

Measured against the historical v1 corpus (`utils/lineinfo-audit --gen1`):

- a make-node's `at` covers only its two-character opener (`[[`, `[{`, `{{`); the closer is not
  in the tree, so a rule finds it from the enclosing anchor or by scanning
- multi-line v1 spans end one column past the last line - `offsets.das` clamps to the line end
- `{{ k => v }}` is already an unresolved `to_table_move` call at parse time, and
  `genFlags.generated` is false on it, so "written by hand" cannot be read off the flag; a rule
  verifies the anchor text before it patches
- a `k => v` entry reports its span from the `=>`, not from the key, so the text before the first
  value is not always the element type - only the bare `[[ ]]` form can carry one
- a block starts where its header's CODE ends, which a trailing `//` on that line puts AT the
  comment: the opening brace goes in FRONT of that anchor, and the closing brace after the
  trailing comment of the last statement

## What converts today

| family | state |
|---|---|
| make-syntax: `[[Foo ...]]`, `;`-separated instances, `[[Foo]]`, `[[Foo?]]`, `[[Foo#]]`, `[[auto ...]]`, `[[auto[] ...]]`, tuple elements | done |
| `[{ ... }]` and `{{ ... }}` wrappers, both comprehension spellings | done |
| blocks: indentation becomes braces (functions, generics, `if`/`else`, `for`, `while`, `with`, `unsafe`, `try`/`recover`) | done |
| conditions: `if`/`while`/`with` subjects and the `for` head gain parentheses | done |
| declarations: `struct`, `class`, `enum`, `bitfield` bodies; `typedef` blocks split per alias | done |
| `f <|` + an indented body (bare piped block) | done - the block gains the `$()` header gen2 needs |
| typed make-array: `[[int 1, 2]]` -> `fixed_array<int>(...)`, `[{int 1; 2}]` -> `[1, 2]` | done |
| `typeinfo(trait expr)` -> `typeinfo trait(expr)` | done |
| argument annotations: `[[optional]] a` -> `@optional a` | done |
| tuple destructuring: `var [[a, b]]` -> `var (a, b)` | done |
| piped literals: `$ <\| (a, b : int)` and `@ <\|` write themselves, and the `<\|` goes | done |
| enumeration and bitfield values: `Color green` -> `Color.green` | done |
| casts: `reinterpret<int&> x` -> `reinterpret<int&>(x)` | done |
| type functions: `take<int>(1, 2)` -> `take(type<int>, 1, 2)` | done |
| multi-value return: `return 1, "2"` -> `return (1, "2")` | done |
| `static_if cond` -> `static_if (cond)` | done |
| spoof templates (`%spoof_template~`) | **skipped** - the template body is text the parser never sees as code, so no rule can place its braces; the file is reported and left alone |
| a multi-line arithmetic RHS | **not yet** - gen2 needs it parenthesised, or the continuation lines are separate statements |
| EOL semicolons, capture lists | **not yet** |

Measured by migrating the whole historical v1 corpus in place (the 104 `.das` files under
`tests/` at `027cbc2d8^`, `.inc` fragments included) and compiling the result as gen2. Of those,
72 compile as v1 with today's compiler - the rest are deliberately-failing `failed_*` fixtures
and files the current compiler rejects for reasons that predate any conversion. **71 of those 72
convert to gen2 that compiles, and none loses a comment.** The one exception is the spoof-template
file, which is skipped by design.

An `.inc` is converted as its own file and gets no `options gen2` header - it is spliced into a
module that already carries one.

## Layout

| file | what |
|---|---|
| `offsets.das` | source as bytes: line index, `LineInfo` -> offset, span text, indent |
| `comments.das` | comment and string-literal ranges, blank-gap test, line edge |
| `patch.das` | the edit set: record, sort, apply, refuse; the conservation gate |
| `convert.das` | the rules, and `convert_file` |
| `main.das` | CLI |
| `test_engine.das` | offsets, lexical scan, edit composition |
| `test_convert.das` | end-to-end over `_fixture_gen1_basic.das` |
