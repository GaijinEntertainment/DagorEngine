# dasLLVM architecture - the debug-info rail

Companion of `ARCHITECTURE.md` (contract: `../../ARCHITECTURE_COMMON.md`). Section 12 continues
that document's numbering. What the rail ships and what is still planned is `DEBUGGING.md`;
this document is how the emitted metadata is shaped and why.

## 12. One compile unit per generated module {#di-one-cu-per-module}

`--jit-debug` (`policies.jit_debug_info`) arms `LlvmJitFlags.need_di`, and the emitter then
carries a DIBuilder per generated LLVM module. `daslib/llvm_jit_di.das` owns the DWARF type and
expression factory - it turns a das `TypeDecl` into a `DIType` and never touches IR;
`daslib/llvm_jit.das` owns every attachment - subprograms, locations, lexical scopes and the
`dbg.declare` records. Both reach LLVM only through the C entry points bound in `bindings/`, so
the rail adds no LLVM header or library dependency anywhere.

The builder belongs to the MODULE, not to the visitor that emits one function. One DIBuilder
holds one compile unit (LLVM asserts on a second), so a builder per function would mean a
compile unit per function - and at any optimization level that inlines, the executed copy of a
function lives inside its CALLER's compile unit as a `DW_TAG_inlined_subroutine`. A debugger
finds that instance by expanding the compile unit its name index points at; with one CU per
function the caller's CU never names the callee, so `break hot` resolves only against the
out-of-line copy the inliner left dead, and never fires. With one CU per module every instance
of a function - out-of-line and inlined - sits in the same unit as its abstract origin.

The builder is created on first use for a module and finalized once, from `run_jit`
(`llvm_jit_run.das`), after the last function of that module is emitted and before anything
reads the module: `DIBuilder::finalize` is what resolves the metadata cycles and temporary nodes
a module may hold, and the LLVM verifier refuses a module that still carries one. Split mode
finalizes per partition, each partition being its own module.

The `DICompileUnit`'s own file name is the label `das-jit`; the real `.das` paths hang off the
per-function `DIFile`s. Its language is `DW_LANG_C_plus_plus`: under `DW_LANG_C` a debugger
treats an unmangled name as the whole truth about a C symbol and drops the argument types it
would otherwise print beside a frame.

The `DIType` cache holds one generation per builder. A metadata node belongs to the module its
builder writes, and a forward declaration - `LLVMDIBuilderCreateForwardDecl`, which is
`createReplaceableCompositeType` underneath - is a `distinct` node rather than a uniqued one, so
a node reused under a second builder lands in the wrong module's DIE tree. `di_cache_drop_if_stale` drops
the whole cache whenever the builder asking is not the builder it was filled for.

A `DIFile` carries the ABSOLUTE source path in its filename field and leaves its directory
field empty. A debugger resolves the source text through that one string, so a relative name
lists only from the directory the compile ran in; the directory field stays empty because
CodeView consumers join the two fields and would print `dir/dir/file`.

The artifact is a real shared object or DLL: the JIT emits an object, links it, and `dlopen`s
it. A debugger therefore needs no JIT-registration protocol - it reads the DWARF (or the PDB
lld-link writes on windows) out of a module the dynamic loader already announced. That is why
the rail is DWARF-shaped rather than built on LLVM's GDB JIT interface, which only the
in-memory `-jit-no-cache` path would need.

### 12.1 A subprogram is named as the das source names it {#di-subprogram-naming}

A `DISubprogram` carries the das function's own name in `DW_AT_name` and NO
`DW_AT_linkage_name`. That is what makes `break hot` work. The generated symbol
(`_anon_...`hot Ci implementation`) is nothing anyone would type, and it is not a mangling any
debugger can decode - while gdb, for a symbol that has a linkage name, uses that name as the
lookup key and the das name becomes unreachable. The object symbol table still carries the
generated symbol, so a debugger that wants it has it, tied to the DIE by address.

Both halves of a generated function pair carry that same das name - the impl and the wrapper
the interpreter calls. The reason is that the impl is `alwaysinline`: once the wrapper has
inlined it, the standalone impl body is dead code that still owns a full line table, so a
breakpoint resolved against the impl alone never fires. Naming both gives the debugger a
location list that includes the copy that actually runs.

An impl's `DISubprogram` claims local-to-unit only outside split mode. Split mode gives an impl
external-hidden linkage so a caller in another partition can bind it, and the flag has to match
the linkage the symbol really has. A wrapper is the exported symbol in either mode, so its
subprogram never claims it.

A block or lambda body is a generated function too, and the das source has no name for it. Its
subprogram is named `<host function>_block_at_<line>`, which is also the name a `bt` frame
prints. Without a subprogram of its own, nothing inside a block body carries a line at all -
LLVM drops a `!dbg` whose scope is another function.

### 12.2 A variable's location is its stack slot {#di-variable-locations}

Every das local, loop variable and argument gets a `DILocalVariable` plus a `dbg.declare`
record naming the slot it lives in. `dbg.declare` states an ADDRESS, so the emitter passes the
`alloca` - or, for a `ref` argument, the incoming pointer, since the parameter value IS the
variable's address. The one indirect case is a das slot that holds a POINTER to the value
rather than the value: a `ref` local's `alloca`. Its location is one `DW_OP_deref` past the
slot, and without that expression a debugger prints the pointer where the value was asked for.

A debug build promotes EVERY argument to a stack slot, the same way `hasMakeBlock` already
does. A parameter that stays an SSA value has no address for `dbg.declare` to name; with the
slot in place `SROA` re-promotes it to a register and rewrites the metadata itself, so the
promotion costs nothing past `-O0` and is what makes `frame variable` answer at every level.

The subprogram is created before the entry block is built, because the entry block is where the
argument variables are declared and a `DILocalVariable` has to name the scope it belongs to.

**An instruction the emitter hoists or defers carries no stale line.** `break file:line` takes
the LOWEST address a line owns, so one misattributed instruction moves the stop to somewhere the
line's own statement has not run. Three shapes are ruled:

- **The prologue carries no location at all** - not the entry block's frame slots, not the
  argument stores, not the branch that closes it. LLVM marks `prologue_end` at the first
  instruction that has one, so a location there stops a debugger ahead of the argument stores
  where every argument reads as garbage. Worse, a frame slot is hoisted to the entry block from
  wherever the das statement that needs it sits, so a `let` inside a nested block would give its
  own line to a prologue instruction and `break` on that line would resolve to function entry.
  `at_function_entry` clears the location around every hoisted slot and restores it after.
- **A wrapper's entry block is the exception, and carries the declaration line on purpose.**
  A wrapper holds no das statement - it marshals arguments, calls the impl and returns - so
  there is no statement whose line it could steal, and a frame for it needs a line to print.
- **A loop's latch carries the loop's own line**, not the last statement of its body. The latch
  is emitted after the body and would otherwise inherit that statement's location - and then a
  breakpoint on the body's last line stops in the PREVIOUS iteration's increment, with every
  value one step stale.

The first real location a function carries is therefore the one its body's first statement sets.

### 12.3 A das block is a lexical scope {#di-scopes}

Each open das block pushes a `DILexicalBlock` on the scope stack, innermost last, with the
subprogram at the bottom. A `DILocalVariable` names the scope it was declared in, and that is
what makes a shadowed name resolve to the right slot instead of the outermost one. The stack is
per generated function - a nested block body is emitted by its own visitor instance and starts
clean.

When a block's line info belongs to a different file than its parent scope - what an inlined
`require`d module produces - the pushed scope is a `DILexicalBlockFile` instead. A `DILocation`
takes its file from its scope, so without the re-point every line of the included body would be
reported against the host function's file.

### 12.4 A das global is described where its address is known {#di-globals}

A das global has no static address: it lives at `context->globals + stackTop`, and the same
jitted module serves every Context of the program, each with its own base. A
`DIGlobalVariableExpression` would therefore need one address to stand for all of them, and
whichever Context wrote it, the value printed in the others would be confidently wrong.

The emitter does not need a static address, because it already computes the right one. Every
function that touches a global materializes a pointer to it - through `jit_get_global_mnh` at
the use site, or from `context->globals` in the entry block under `options solid_context` - and
that pointer is derived from the `ctx` the call actually received. The global is described as a
variable of that function, located at that pointer, once per function that mentions it. The
location is per-Context correct by construction.

The cost is scope rather than accuracy: a global reads in any frame whose function touches it,
and not in a frame that never mentions it. That is the honest shape for a variable whose address
is a function of the running context, and it is the shape a debugger user experiences anyway -
the value is asked for from inside a frame.

### 12.5 A composite DIType is built with no metadata cycle {#di-composite-types}

A das structure, tuple or variant becomes a real `DW_TAG_structure_type` with one member per
field, at the field's own byte offset - independent of the `[N x i8]` blob the value has as an
LLVM type, since DWARF describes the layout the emitted code uses rather than the IR type.

A variant's members are the `int` tag at offset zero and every arm at one shared offset past it -
that is the layout a das variant has, and the tag says which arm is live.

The metadata is built acyclic, and that is a constraint rather than a preference: only
`DIBuilder::finalize` can resolve a metadata cycle, the module verifier runs per function long
before the module is finished, and an unresolved node fails it. Two shapes would close a cycle
and neither is emitted. A member's scope is the FILE, not the composite that holds it - the DIE
nesting already says which type a member belongs to. And a pointer field reaching back into the
type whose members are being built takes a forward declaration of that type as its pointee: the
declaration shares the definition's unique id, which is what a debugger reconciles the two by.

`LLVMDIBuilderCreateStructType` is the one entry point the generated `bindings/llvm_func.das`
does not carry, so `llvm_jit_di.das` declares it itself against the same `LLVM.dll`. Its
das-side name is deliberately distinct from the C spelling, so a regenerated binding that adds
the plain name cannot collide with it.

The rest of the lattice maps by shape: a workhorse scalar to a `DW_TAG_base_type` with the
matching DWARF encoding, a `string` to `char *` (which is the shape every debugger already
prints as text), a vector or range to a vector-of-subrange over its lane scalar, a `dim` to an
array type with one subrange per dimension, an enumeration to a `DW_TAG_enumeration_type` with
the entries whose values fold to constants, and everything whose interior the rail does not
describe - a handled C++ type, a container, a block, a lambda - to a named opaque forward
declaration of the right size, so a debugger prints the name and the address rather than
nothing.
