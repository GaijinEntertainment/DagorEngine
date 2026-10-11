# daslib architecture notes - the cursor module

Companion of `ARCHITECTURE.md` (this folder), which carries the module list and the numbering;
this file holds sec. 40.

## 40. ast_cursor

### 40.1 Declaration positions {#cursor-variable-position}

- **A `Variable`'s `at` is its name token - `src/parser/parser_impl.cpp` (repo root) sets
  `pVar->at` from the `tokAt(scanner, @name)` the grammar's `VariableNameAndPosition` carries -
  and a variable whose `at` has no width is matched over `length(name)` columns instead.**
  Nothing fails when one side moves alone: a wider `at` still contains the cursor on the name,
  so every cursor test stays green while hits appear for text that is not the name.
- **A `Structure`'s `at` is one column before its name and a typedef's `TypeDecl.at` is its
  right-hand type, so `name_column` finds a declared name in the line's text and falls back to
  the AST column only when the line does not spell it.** Nothing fails when one side moves
  alone: the unit test hardcodes the expected columns and the LSP protocol test computes them
  from the line text, so a parser that moves `at` onto the name changes no answer.

### 40.2 Compiler-made names {#cursor-compiler-made-names}

- **A compiler-synthesized member is `generated` and carries an apostrophe - `makeClassFinalize`
  (`src/ast/ast_generate.cpp`, repo root) names it `<Class>'__finalize` - which the lexer's NAME
  token (`src/parser/ds2_lexer.lpp`) never admits in a user identifier; it sits on the `class`
  line with no source of its own. A lambda or generator body and a generic instance are
  `generated` too and are the user's own source: the body carries `_lambda` / `_generator`, the
  instance a non-null `fromGeneric`.** Nothing fails when one side moves alone:
  `tests/daslib/ast_cursor_test.das` pins only that a `class` line yields no hit, so a
  `generated` bit on a new kind of user body silently hides every hit inside it.
- **A generator body's `for`/`let` variable becomes a state field named
  `__<name>_rename_at_<line>_<column>` - `RenameVar` in `src/ast/ast_generate.cpp`, the name
  passed through `aotSuffixNameEx`, which leaves an ASCII identifier as is - and `source_name`
  strips the wrap back to `<name>`.** Nothing fails when one side moves alone: the
  test pins only the daslib half, so a new spelling shows the mangled name in every hover and
  reference.

### 40.3 Class members {#cursor-class-fields}

- **A method call - `a->m(x)`, `a.m(x)`, or a bare `m(x)` inside the class - desugars to
  `invoke(type<T>.m, cast<auto> deref(a), x)` (`makeInvokeMethod`, `src/ast/ast_generate.cpp`,
  repo root), every node on the call's span - the `->`/`.` forms on the operator token's, with
  the method name's own position in the `ExprField`'s `atField` - and `CursorVisitor`
  (`ast_cursor.das`) never hits the `ExprRef2Value` among them and ranks a named node before a
  nameless one on the same span.** Nothing fails when one side moves alone: a new conversion
  kind the C++ emits on that span is admitted with every test green, and only the named-first
  rank keeps the field ahead of it; `tests/daslib/ast_cursor_test.das` pins the field-first
  order at one implicit and one `->` site, not the set of nodes behind it.
