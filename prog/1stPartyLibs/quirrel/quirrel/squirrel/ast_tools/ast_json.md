# JSON AST dump

The document that `sq_dumpastjson()` writes, for a tool that reads or rewrites
Quirrel source: a linter, an indexer, a codemod. The renderer is
`ast_json_render.h` next to this file. The `sq` interpreter writes it with
`-ast-dump-json [out-file]` for the script named on the command line, and not
for the modules that script loads; with `--parse-only` it parses and dumps
the script and runs nothing. A tool that needs the modules of a script reads
them from its `ImportStmt` nodes and dumps each in turn. The `module` of
such a node is the name as the source spells it, not a path: `sq` looks for
the file next to the importing script first and then by the name as given, and
a name of a built-in module such as `math` has no script at all.

## The document

One line of JSON for the script, ended by a line feed.

```json
{"file": "script.nut", "ast": {"type": "Block", "span": [1,0,9,0], "root": true, "statements": [...]}, "comments": [...]}
```

- `file` is the source name the parser was given, cut to 255 bytes.
- `ast` is the root node.
- `comments` is always present, and empty when the parse did not keep the
  comments (`sq` keeps them for a JSON dump). Each entry is
  `{"kind", "span", "text"}`: `kind` is `line` or `block`, `span` covers the
  comment with its delimiters, `text` is the comment without them. A line
  break in the text is `\n` whatever the line ending of the file.

## A node

```json
{"type": "CallExpr", "span": [3,9,3,17], "callee": {...}, "args": [...]}
```

- `type` is the name of the AST class: `Block`, `CallExpr`, `FunctionExpr`,
  `VarDecl`, and so on. A node kind newer than this renderer comes out under
  its internal op name instead, in capitals, with `type` and `span` alone.
- `span` is `[startLine, startColumn, endLine, endColumn]`. Lines count from 1,
  columns count from 0 and count **bytes** in the line, a UTF-8 BOM is not
  counted, and the end is exclusive. `-1` means that the position is unknown.
  A span is the span the parser records, the one the diagnostics use, so a
  `#pos:<line>:<column>` directive moves every span after it. Comment lines are
  physical and do not follow `#pos`; a comment column follows it only on the
  line of the directive itself.
- Every other key is a field of that node. A field holds a scalar, a node, or
  an array of nodes. A few fields hold a plain object instead of a node: a
  member of a table or class, a case of a switch, a catch clause, a constant of
  an enum, a slot of an import.

**A field whose value is its default is left out**, and a reader treats a
missing key as that default. The defaults are `false` for a flag, `null` for
an absent child or name (`init`, `else`, `alias`, `default`), the type `any`
for `valueType` and `resultType`, and an empty `doc`. `type`, `span` and every
array are always present. The `value` of a `LiteralExpr` is always present
too, with `kind` saying which JSON type to expect: `int`, `float`, `string`,
`bool` or `null`. A float always has a decimal point or an exponent.

The tree is what the parser builds, after its desugaring: `a not in b` is a
`UnExpr` with op `!` over a `BinExpr` with op `in`, and `$"x{a}y"` is a
`CallExpr` of the `subst` method on the literal `"x{0}y"`. A tool that needs
the surface form reads the source text by the span.

The output is valid UTF-8: a byte that is not part of a well-formed sequence
is written as `\u00XX`.

Some names in the dump are made by the compiler, and their spans are
synthesized. A destructured parameter carries `@argN` and a destructured
foreach value `@FE_VALn`, both with the span of the pattern, from the opening
bracket to the closing one; the `DestructuringDecl` of that pattern has the
same span, while the one of a `let` or `local` declaration starts at the keyword
and ends at the initializer. A lambda carries `(<file base name>:<line>)`, which is not source text
and is not unique. With closure hoisting (`-optCH`) each hoisted closure adds a
`$chN` declaration, which starts where the statement it is placed before starts
and ends at the closure itself. An enum constant without
an initializer gets a `LiteralExpr` with the span of its name. A tool that
rewrites source text by a span, or lists declarations by name, must leave these
forms alone.

## Reading it

```python
import json

with open("out.json", encoding="utf-8") as f:
    script = json.load(f)
print(script["file"], len(script["ast"]["statements"]), len(script["comments"]))
```

The text dump of `sq_dumpast()` and `-ast-dump` is for a person: its lines are
cut at 256 characters and it carries no end positions.
