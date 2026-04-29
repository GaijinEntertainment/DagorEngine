# MCP Server Roadmap

Future tools for the daslang MCP server, organized by priority and difficulty.

## Current Tools (v0.6)

| Tool | Description |
|---|---|
| `compile_check` | Compile file(s), return errors or success. Supports single file, comma-separated list, or glob pattern |
| `list_functions` | List all functions after macro expansion |
| `list_types` | List structs, classes, enums, type aliases |
| `list_requires` | List direct and transitive require dependencies |
| `list_modules` | List all available modules (builtin + daslib) |
| `list_module_api` | List functions, types, enums, globals, and annotations exported by a module (shows parent types for structs and handled types) |
| `find_symbol` | Cross-module symbol search by substring (functions, generics, structs, handled types, enums, globals, typedefs/aliases) |
| `ast_dump` | Dump AST of expression or function (S-expression or source mode). Optional `lineinfo` for source locations and `atEnclosure` |
| `program_log` | Full post-compilation program text (like `options log`) with optional function filter |
| `run_script` | Run inline code or a .das file, capture stdout |
| `run_test` | Run dastest on a test file |
| `format_file` | Format a .das file in place |
| `convert_to_gen2` | Convert gen1 (indentation) syntax to gen2 (braces/parens) |
| `goto_definition` | Resolve symbol at cursor to its definition (variable, function, field, struct, enum, typedef, builtin). Optional `no_opt` |
| `type_of` | Return resolved type of expression at cursor position. Optional `no_opt` |
| `find_references` | Find all references to symbol at cursor (calls, variables, fields, type refs, addr, enum/bitfield values, aliases, global declarations). Scope: `file` or `all`. Optional `no_opt` |
| `eval_expression` | Evaluate a daslang expression and return printed result. Supports comma-separated module imports via `require` parameter |
| `describe_type` | Describe a type's fields, methods, values, and base type. Supports structs, classes, handled types, enums, bitfields, variants, tuples, typedefs |
| `grep_usage` | Parse-aware symbol search across .das files using ast-grep + tree-sitter. Conditional on `sg` CLI being installed |
| `outline` | List all declarations (functions, structs, classes, enums, globals, typedefs) in a file or set of files using tree-sitter. No compilation needed. Conditional on `sg` CLI |

### Cross-cutting features

- **`.das_project` support** — all file-based tools accept an optional `project` parameter pointing to a `.das_project` file for custom module resolution and sandboxing
- **Request logging** — file-based logging with timestamps for debugging
- **Unified file utilities** — shared `resolve_path()`, `make_relative_path()`, `expand_glob()`, `parse_file_list()` in `tools/common.das` used by `compile_check` and `grep_usage`. Glob patterns support `!pattern` exclusion (gitignore-style)

### Cursor-based tools implementation notes

`goto_definition`, `type_of`, and `find_references` use `daslib/ast_cursor` (`find_at_cursor`) to map file+line+column to AST nodes. They support:

- **Functions:** `ExprCall` (calls), `ExprAddr` (function pointers `@@func`)
- **Variables:** `ExprVar` (local and global), including declaration-based lookup via `for_each_global`
- **Fields:** `ExprField`, `ExprSafeField` (`.field`, `?.field`)
- **Enums:** `ExprConstEnumeration` (enum values like `Color.Red`)
- **Bitfields:** `ExprConstBitfield` (bitfield values like `Flags.readable`)
- **Aliases:** `TypeDecl.alias` — covers `typedef`, `bitfield`, `variant`, `tuple` declarations
- **Structs/Classes:** type references via `TypeDecl.structType`
- **Builtins:** built-in function signatures (no source location)

The `no_opt` parameter disables compiler optimizations (`CodeOfPolicies.no_optimizations`), preserving `ExprVar` nodes for globals and `ExprConstBitfield`/`ExprConstEnumeration` nodes that would otherwise be constant-folded away.

`find_references` also supports declaration-based lookup: cursor on `def funcName`, `struct Name`, `enum Name`, `bitfield Name`, `variant Name`, `tuple Name`, or `let globalVar` declarations identifies the target via `for_each_function`/`for_each_structure`/`for_each_enumeration`/`for_each_typedef`/`for_each_global`.

---

## Urgent: Developer Experience Gaps

### ~~describe_type~~ ✅

Implemented as a standalone tool. Searches all modules for a type by name and describes its fields, methods, values, base type. Supports structs, classes, handled types, enums, bitfields, variants, tuples, and typedefs. Optional `module` parameter to limit search scope.

### ~~grep_usage~~ ✅

Implemented using ast-grep + tree-sitter for parse-aware identifier search. Finds symbol occurrences excluding comments and strings — no compilation needed. Conditionally available when `sg` (ast-grep) CLI is installed. Server prints install instructions if missing.

Parameters: `symbol` (required), `directory` (optional), `glob` (optional file filter), `context_lines` (optional). Output grouped by file with line numbers.

### ~~batch_compile~~ ✅

Merged into `compile_check` — supports comma-separated file lists and glob patterns (e.g., `utils/mcp/tools/*.das`). Reports per-file pass/fail with summary.

### ~~list_annotations~~ ✅

Merged into `list_module_api` as the `annotations` section. Lists function annotations, structure annotations, call macros, reader macros, variant macros, typeinfo macros, for-loop macros, and type macros.

### ~~eval_expression~~ ✅

Evaluates a daslang expression via `let _res_ = <expr>; print("{_res_}\n")` scaffold. Supports comma-separated `require` parameter for module imports. Works with `typeinfo`, complex expressions, and library functions.

---

## Phase 2: Refactoring (Medium Priority)

### rename_symbol

**What:** Rename a function, variable, struct, field, or enum value across one or more files.

**Why:** Safe mechanical renaming is tedious and error-prone by hand. The compiler knows all references.

**Implementation approach:**
- First, use `goto_definition` to find the definition.
- Then, use `find_references` to find all usages.
- Apply text replacements at each location.
- Re-compile to verify the rename didn't break anything.
- Return the list of modified files and a diff.

**Important constraints:**
- Must handle qualified names: renaming struct `Foo` must also rename `Foo.field` access, `new Foo()`, type annotations `x : Foo`, etc.
- Method renames: `obj.method()` and `obj |> method()` and `obj->method()` all need updating.
- Must NOT rename unrelated symbols that happen to share the same name (different module, different scope).
- Should refuse to rename builtins (no source to modify).

**Parameters:**
- `file` (required) — file containing the symbol definition
- `symbol` (required) — current name
- `new_name` (required) — desired new name
- `scope` (optional) — `"file"` or `"project"`
- `dry_run` (optional, default true) — if true, return diff without modifying files

**Output:** List of changes `(file, line, old_text, new_text)` and compilation check result.

**Difficulty:** Hard. Depends on `goto_definition` and `find_references`. Safe renaming across files is complex.

### extract_function

**What:** Extract a selected range of code into a new function, automatically determining parameters and return values.

**Why:** Common refactoring. The AI can identify the code to extract but needs compiler help to determine which variables must become parameters.

**Implementation approach:**
- Parse the selected code range to identify:
  - Variables read but not defined in the selection → become function parameters
  - Variables defined in the selection and used after it → become return values
  - Variables defined and only used within the selection → stay local
- Generate the function signature with proper types (from the compiled AST).
- Replace the selected code with a call to the new function.
- Handle `var` (mutable) parameters correctly — if the selection modifies a variable used later, it needs `var` parameter.

**Challenges:**
- daslang's move semantics: if the selection moves a value (`<-`), the extraction must preserve that.
- Block/lambda captures: if extracting code that's inside a block, the new function might need different parameter passing.
- Control flow: `return`, `break`, `continue` inside the selection complicate extraction.

**Parameters:**
- `file` (required) — source file
- `start_line` (required) — first line of selection
- `end_line` (required) — last line of selection
- `function_name` (required) — name for the new function
- `dry_run` (optional, default true)

**Output:** The new function definition, the modified call site, and a compilation check.

**Difficulty:** Very hard. Requires deep AST analysis of data flow. Could start with a simpler version that only handles straightforward cases (no control flow escapes, no moves).

### inline_function

**What:** Replace a function call with the function's body, substituting parameters.

**Why:** The inverse of extract — useful for removing unnecessary abstraction.

**Difficulty:** Hard. Needs parameter substitution, handling of return statements, variable name conflicts.

---

## Phase 3: Intelligence (Medium Priority)

### explain_error

**What:** Given a compiler error message, provide a detailed explanation with fix suggestions.

**Why:** daslang error messages can be cryptic, especially for type mismatches, move semantics, and macro expansion errors. The AI can provide better explanations but needs structured error info.

**Implementation approach:**
- Parse the error output from `compile_check` into structured fields: error code, file, line, message, context.
- Match against known error patterns and provide:
  - Plain-English explanation
  - Common causes
  - Suggested fixes with code snippets
- Could maintain a knowledge base of error patterns (similar to Rust's error index).

**Parameters:**
- `file` (required) — file with the error
- `error_text` (optional) — specific error message to explain (if not provided, compile and explain all errors)

**Output:** Structured explanation per error: what it means, why it happens, how to fix it.

**Difficulty:** Medium. Pattern matching on error messages is straightforward. Quality depends on the knowledge base.

### type_search

**What:** Search for functions by their type signature (like Haskell's Hoogle or Gleam's Gloogle).

**Why:** "I have a `string` and want an `int` — what function does that?" This is incredibly useful for discovering API.

**Implementation approach:**
- Index all functions across registered modules by parameter types and return type.
- Parse a query like `string -> int` or `(array<int>, int) -> bool`.
- Match functions where parameter/return types are compatible (exact match, then subtype match).
- Rank by relevance: exact matches first, then partial matches.

**Parameters:**
- `query` (required) — type signature query (e.g., `string -> int`, `float, float -> float`)
- `module` (optional) — limit search to a specific module

**Output:** List of matching functions with their full signatures and module names.

**Difficulty:** Medium-hard. Parsing type queries and matching against `TypeDecl` requires a type compatibility checker.

### try_fix

**What:** Given a file with compilation errors, attempt automatic fixes and return the corrected code.

**Why:** Many errors have mechanical fixes: missing `var`, wrong type cast, missing `require`, missing parentheses.

**Implementation approach:**
- Compile the file, collect errors.
- For each error, apply pattern-matched fixes:
  - "expecting T, got U" → insert cast or conversion
  - "undefined symbol X" → add `require` for the module containing X (using `find_symbol`)
  - "variable not found" → suggest `var` declaration
  - "can't copy" → suggest `:=` or `<-`
- Re-compile after each fix to verify it works.
- Iterate until no more auto-fixable errors remain.

**Parameters:**
- `file` (required) — file to fix
- `dry_run` (optional, default true)
- `max_iterations` (optional, default 5)

**Output:** Diff of changes made, remaining errors (if any), compilation result.

**Difficulty:** Hard. Each error pattern needs a specific fixer. Interactions between fixes can cause cascading issues.

---

## Phase 4: Project-Level (Lower Priority, Some Deferred)

### workspace_index

**What:** Build an in-memory index of all `.das` files in a project directory for fast cross-file operations.

**Why:** Many tools (find_references, rename_symbol) need to know about all files in a project. Compiling each file on every query is too slow.

**Implementation approach:**
- Scan a directory tree for `.das` files.
- Compile each file and cache: function/type/global definitions, require graph, symbol locations.
- Invalidate cache entries when files change (check mtime).
- Other tools query the index instead of compiling from scratch.

**Difficulty:** Hard. Cache invalidation is the main challenge (macro expansion means a change in one file can affect others).

### dependency_graph

**What:** Visualize the full dependency graph of a file or project.

**Why:** Understanding module dependencies helps with architecture decisions, finding circular deps, and planning refactors.

**Implementation approach:**
- Already have `list_requires` for single-file deps. Extend to:
  - Build a graph across all files in a project
  - Detect cycles
  - Output in DOT format or structured JSON
  - Show which deps are direct vs transitive

**Parameters:**
- `file` or `root` — starting point
- `format` — `"text"`, `"json"`, or `"dot"`
- `depth` (optional) — max depth to traverse

**Difficulty:** Medium. Building on `list_requires` infrastructure.

### package_search

**What:** Search a package registry for daslang packages by name, description, or functionality.

**Status:** **Deferred** — waiting for the daslang package manager to be implemented.

### scaffold

**What:** Generate boilerplate code for common patterns: new module, test file, C++ binding, etc.

**Why:** Ensures generated code follows project conventions (correct `options`, `require`s, annotations).

**Templates:**
- `module` — new daslib module with proper header
- `test` — test file with `[test]` functions and fixture setup
- `binding` — C++ module binding skeleton (`.das_module` + C++ stub)
- `tutorial` — tutorial .das file with comments

**Difficulty:** Easy-medium. Template-based generation. The AI already does this well, so the value-add is mainly in ensuring conventions.

---

## Implementation Priority

Recommended order based on value/effort ratio:

1. ~~**goto_definition**~~ ✅ Implemented
2. ~~**type_of**~~ ✅ Implemented
3. ~~**find_references**~~ ✅ Implemented (file + all-modules scope, declaration lookup)
4. ~~**program_log**~~ ✅ Implemented (full program text, optional function filter)
5. ~~**ast_dump with LineInfo**~~ ✅ Implemented (`lineinfo` parameter, shows `atEnclosure`)
6. ~~**dot-call LineInfo fix**~~ ✅ Fixed (`atEnclosure` on dot-call/arrow-call expressions in parser + inference)
7. ~~**.das_project support**~~ ✅ Implemented (per-tool `project` parameter)
8. ~~**describe_type**~~ ✅ Implemented (fields, methods, values, base types for all type kinds)
9. ~~**grep_usage**~~ ✅ Implemented (ast-grep + tree-sitter, conditional on `sg` CLI)
9b. ~~**outline**~~ ✅ Implemented (tree-sitter kind-based rules, conditional on `sg` CLI)
10. ~~**batch_compile**~~ ✅ Implemented (merged into `compile_check` with comma-separated and glob support)
11. ~~**list_annotations**~~ ✅ Implemented (merged into `list_module_api` as `annotations` section)
12. ~~**eval_expression**~~ ✅ Implemented (expression eval with `require` support)
13. **explain_error** — high value, relatively easy
14. **dependency_graph** — medium value, easy (extends `list_requires`)
15. **type_search** — high value for API discovery, medium-hard effort
16. **rename_symbol** — high value, depends on goto_definition + find_references (both done)
17. **try_fix** — medium-high value, hard
18. **extract_function** — medium value, very hard
19. **workspace_index** — enabler for cross-file tools at scale
20. **scaffold** — low priority (AI already generates good code)
21. **package_search** — deferred until package manager exists

## Architecture Notes

### Position-to-AST Mapping

Cursor-based tools (goto_definition, type_of, find_references) use `daslib/ast_cursor` module:

- `find_at_cursor(program, file, line, col)` returns an array of `CursorHit` from innermost to outermost expression
- Each `CursorHit` has: `expr` (the expression), `func` (enclosing function), `rtti` (node type name), `name` (symbol name if applicable)
- `compile_program(file, export_all, no_opt, project)` in `tools/common.das` wraps compilation with proper CodeOfPolicies

### Cross-File Compilation

For project-level tools, we need to compile multiple files. Options:
- **Sequential:** compile each file independently. Simple but slow.
- **Cached:** maintain a `ModuleGroup` across compilations. Faster but complex lifetime management.
- **Indexed:** pre-scan files for `require` statements (text-level), build dependency order, compile in order. Best for large projects.

### Tool Composition

Many complex tools are compositions of simpler ones:
- `rename_symbol` = `goto_definition` + `find_references` + text replacement + `compile_check`
- `try_fix` = `compile_check` + error pattern matching + text edit + `compile_check`
- `extract_function` = AST analysis + code generation + `compile_check`

Building the foundational tools well creates a platform for everything else.
