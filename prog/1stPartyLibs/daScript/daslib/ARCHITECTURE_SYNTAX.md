# daslib architecture notes - syntax pairs

Companion to `ARCHITECTURE.md` in this folder. The pairs below are daslib facts whose other
side is a C++ source; nothing fails on a mismatch - no lint, no compile error, no default-suite
test - so the record here is what keeps the two together.

## 42. The raw assignment spellings

- **`das_source_formatter.das`, `greedy_match_safe_tokens`** lists `!==`, `!<-` and `!:=` as
  three-character tokens. The other side is the lexer, `src/parser/ds2_lexer.lpp` (repo root):
  the rules returning `NOTEQUEQU`, `NOTLARROW` and `NOTCLONEEQU`. A spelling in one list and
  not the other formats as `!= =`.
- **`ast_print.das`, the `preVisitExprCopyRight` / `preVisitExprMoveRight` /
  `preVisitExprCloneRight` overrides** print ` !== `, ` !<- ` and ` !:= ` when the node's
  `no_promotion` bit is set. The other side is the parser, `src/parser/ds2_parser.ypp` (repo
  root), the `expr_assign_no_bracket` rules that build the node from those tokens: the printer's
  output parses back to the same node.
- **`builtin.das`, the `copy_to_move` / `copy_to_move_ref` / `move_to_move` /
  `move_to_move_ref` generics** are the names inference emits for a promoted initializer - an
  initializer that inference rewrites into a call through the user's `operator =` or
  `operator <-`. The other side is `src/ast/ast_infer_type_function.cpp` (repo root):
  `promoteInitToAssign` picks the helper by whether the source's type is a reference (the
  `_ref` variant for a reference, the plain one for a value), overload resolution then picks
  the const or `var` overload by the source's constness,
  and `isPromotedInitCall` (`src/ast/ast_generate.cpp`, repo root) recognizes these names,
  `clone_to_move` and `clone_string`, so an initializer already promoted is not promoted again
  on the next inference pass.
