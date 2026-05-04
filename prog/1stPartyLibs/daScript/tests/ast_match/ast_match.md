# ast_match — AST Pattern Matching (Reverse Reification)

## Overview

`daslib/ast_match` provides compile-time pattern matching against daScript AST nodes. It uses the same tag system as `qmacro` but in reverse: instead of building AST from values, it extracts values from AST by matching structure.

## API

### `qmatch(expr, pattern) : QMatchResult`
Match a single expression against a structural pattern.
```das
var inscope expr <- qmacro(a + b)
var left : string
let r = qmatch(expr, $i(left) + b)
// r.matched == true, left == "a"
```

### `qmatch_block(expr) $(args) : RetType { stmts }` : QMatchResult
Match a block's statement list with wildcard support. Optionally match block arguments and return type.
```das
var inscope expr <- qmacro($ { if (a < 5) { return b; }; foo(); })
let r = qmatch_block(expr) $ {
    _wildcard()
    if (a < 5) { return b }
    _wildcard()
}
```

### `qmatch_function(func) $(args) : RetType { stmts }` : QMatchResult
Match a compiled function's arguments, return type, and body.
```das
var inscope func <- find_module_function_via_rtti(compiling_module(), @@target_add)
let r = qmatch_function(func) $(a, b : int) : int {
    return a + b
}
```

## Tags (Capture/Extract)

| Tag | Context | Extracts into | Example |
|-----|---------|---------------|---------|
| `$e(var)` | Expression | `ExpressionPtr` | `$e(cond) ? $e(a) : $e(b)` |
| `$v(var)` | Constant expr | `int`, `float`, `bool`, `string`, `int64`, `uint`, `double` | `a + $v(val)` where `var val : int` |
| `$i(var)` | Identifier | `string` | `$i(name) + b` extracts left var name |
| `$i(var)` | Let decl | `string` | `var $i(name) = 5` captures variable name |
| `$i(var)` | For iterator | `string` | `for ($i(name) in arr)` captures iterator name |
| `$t(var)` | Type position | `TypeDeclPtr` | `type<$t(captured)>` captures type |
| `$a(var)` | Argument list | `array<VariablePtr>` | `$(a : int; $a(rest))` captures remaining args |
| `$c(var)` | Call expr | `string` | `$c(name)(a, b)` matches call args, captures call name |
| `$f(var)` | Field expr | `string` | `obj.$f(name)` matches field value, captures field name |
| `$b(var)` | Wildcard range | `array<ExpressionPtr>` | `_wildcard($b(stmts))` captures matched statements |

## Wildcards (Block/Function patterns only)

| Wildcard | Matches | Example |
|----------|---------|---------|
| `_wildcard()` | 0 or more statements | Leading/trailing/sandwich |
| `_wildcard($b(v))` | 0 or more, captured | Statements stored in `v` |
| `_wildcard1()` | 1 or more statements | At least one before/after |
| `_wildcard1($b(v))` | 1 or more, captured | Statements stored in `v` |
| `_optional()` | 0 or 1 statement | Enforced: fails if 2+ |
| `_optional($b(v))` | 0 or 1, captured | Statement (if any) in `v` |
| `_any()` | Exactly 1 statement | |
| `_any($b(v))` | Exactly 1, captured | Single statement in `v` |

## Result Type

```das
enum QMatchError : int {
    ok
    rtti_mismatch        // wrong expression type
    field_mismatch       // field value doesn't match
    const_type_mismatch  // $v: wrong constant type
    type_mismatch        // TypeDecl structure mismatch
    list_length          // array/vector length mismatch
    wildcard_not_found   // wildcard scan failed
    null_expression      // expected expression, got null
}

struct QMatchResult {
    matched : bool
    error   : QMatchError
    expr    : Expression const?  // the mismatched node
}
```

## Type Matching

`ExprTypeDecl` nodes (`type<T>`) are matched structurally via `generate_type_match`:
- `baseType` — enum comparison
- `flags` — const, ref, temporary, smartPtr, isExplicit
- `structType`, `enumType`, `annotation` — by name
- `firstType`, `secondType` — recursive
- `argTypes` — element-wise recursive
- `dim` — fixed dimension comparison
- `dimExpr` — expression matching (supports `$e`/`$v`/`$i` captures inside type expressions)
- `$t(var)` — captures entire TypeDecl when `isTag` flag is set
- `auto` — wildcard, matches any type in that position

### `auto` (type wildcard)

Use `auto` in any type position within a pattern to match any type:
```das
qmatch(expr, type<auto>)                   // matches any type
qmatch(expr, type<array<auto>>)            // matches array<int>, array<float>, ...
qmatch(expr, type<table<string; auto>>)    // matches table with string keys, any value
qmatch(expr, type<table<auto; auto>>)      // matches any table
```

`auto` can be combined with `$t` — use `auto` when you don't need the matched type, `$t(var)` when you want to capture it.

## Argument & Return Type Matching

### qmatch_block
Pattern block arguments match the actual ExprBlock's arguments (name, type). Pattern returnType matches the actual block's returnType. Omitted = match any.

### qmatch_function
Pattern block arguments match Function's arguments (filtering hidden `fakeContext`/`fakeLineInfo` args). Pattern returnType matches Function's `result` field. Omitted = match any.

## Architecture

### Compile-time: Macro code generation
- `generate_match(pattern, actual_var, body, index, at)` — recursive, walks pattern AST via annotation reflection (`get_expression_annotation` + `for_each_field`)
- `generate_type_match(pat_type, td_var, expr_var, body, index, at)` — hardcoded TypeDecl field matching
- `generate_block_match(pat_block, actual_var, body, index, at)` — statement list matching with wildcards + scan
- `generate_pattern_args_match(pat_block, target_var, ret_field, expr_var, body, index, at)` — argument name/type + return type matching

### Runtime helpers
- `qm_run(expr, block)` — clones expression, invokes matching block
- `qm_run_function(func, block)` — passes body + Function? to matching block
- `qm_scan(blk_expr, pos, len, matcher)` — forward scan for wildcard matching
- `qm_extract(expr, var)` — overloaded for int/float/bool/string/int64/uint/double
- `qm_extract_name(expr)` — extracts identifier name from common expression types
- `qm_extract_type(expr, var)` — extracts TypeDeclPtr from ExprTypeDecl
- `qm_extract_call_name(expr)` — extracts call name from ExprCall
- `qm_extract_field_name(expr)` — extracts field name from ExprField/ExprSafeField/variant
- `qm_extract_stmts(blk_expr, from, to, result)` — extracts statement range [from, to) into `array<ExpressionPtr>`
- `qm_count_real_args(args)` / `qm_real_arg_index(args, idx)` — filter fakeContext/fakeLineInfo
- `qm_extract_remaining_args(args, from_logical, result)` — collect remaining real args into `array<VariablePtr>`

### Field handling
- `smart_ptr<Expression>` — recursive `generate_match`
- `smart_ptr<TypeDecl>` — recursive `generate_type_match`
- `das_string` — literal comparison
- `dasvector<smart_ptr<Expression>>` — length check + element-wise matching
- `dasvector<smart_ptr<Variable>>` — name (with `$i` tag support), type, init expression
- `dasvector<das_string>` — length check + element-wise string comparison
- `int`, `uint`, `bool` — literal comparison
- `qm_skip_field(rtti, field)` — per-type skip list for compiler-internal fields

### Special expression handlers
- `ExprConst*` — RTTI + value comparison
- `ExprTypeDecl` — delegates to `generate_type_match`
- `ExprFor` — custom handler for `iteratorsTags` ($i capture on iterators)
- `ExprTag` with `$i` + `value=ExprLet` — clears placeholder name, matches ExprLet, captures variable name

## Test Files (153 tests)

| File | Tests | Coverage |
|------|-------|----------|
| `test_ast_match.das` | 43 | Core qmatch + qmatch_block wildcards |
| `test_qmatch_basic.das` | 8 | Constants, RTTI mismatch, cross-type, null, op2 |
| `test_capture_e.das` | 6 | `$e` on children, whole expr, clone independence |
| `test_capture_v.das` | 12 | `$v` int/float/bool/string, wrong type, in compound |
| `test_capture_i.das` | 11 | `$i` on ExprVar, op2, const, let decl, for iterator |
| `test_capture_cfb.das` | 15 | `$c` call name, `$f` field name, `$b` statement range |
| `test_control_flow.das` | 19 | if/else, while, for, return, mixed with wildcards |
| `test_qmatch_function.das` | 3 | Compiled function body matching |
| `test_type_match.das` | 17 | TypeDecl matching, `$t` capture |
| `test_args_return.das` | 19 | Function/block arg names, types, return type, `$a` capture |

## Not Yet Implemented

- `_optional()` scan path same as `_wildcard()` (no 0-or-1 limit enforced)
- Semantic pattern expansion (comprehension/generator recipes)
- `[ast_check]` compile-time annotation
