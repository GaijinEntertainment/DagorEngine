# daslib architecture notes - the shader rails: flatten, block layout, lingua franca

Companion to `ARCHITECTURE.md` in this folder; section numbers are unique across the family.

## 7. flatten

- **Predicated lowering carries one live-mask per exit flavor** - `__flat_live` for
  return, a per-loop break mask (persists across unrolled copies) and continue mask
  (re-minted per copy). A write's predicate ANDs every active mask plus the structural
  predicate; a narrow term excludes its own mask so it self-cancels. An inlined callee
  gets a fresh live mask and lowers with `ctx.loopMasks` moved OUT, so its break/continue
  can never reach the caller's loops.
- **`flatten_preshade_cse` is a joint fixpoint, not a pipeline** - extraction, regroup,
  CSE and alias elimination mutually enable each other; the `_preshader_`/`_cse_` counters
  are owned by that loop and re-seeded from surviving suffixes (per-call numbering
  re-mints a live name).
- **A CSE/regroup tally counts exactly the regions its rewrite can change** - a duplicate
  counted where the rewrite cannot reach never drops below 2 and runs the fixpoint to its
  iteration cap.
- **`__flat_ret` carries `safeWhenUninitialized` only while every write is a
  self-referential select** - a lowering change that makes the bare-decl read observable
  turns the flag into a real uninitialized read.
- **CSE is local value numbering over one converged basic block, and it is complete** -
  pure subtrees keyed by `describe()`; value-stability = reads no reassigned name; a store
  through index/field/swizzle destabilizes its base; an unrecognized node fails closed as
  mutable-reading. Uniform duplicates route to the preshader.
- **The copy-prop/CSE walks stay O(size)** - one name-to-statement index, one structural
  walk. A `string` materialized per `ExprVar` in a visitor callback breaks that: each
  `describe()` allocates a string that lives to the end of the pass, so the walk goes
  quadratic in heap bytes, not only in time.
- **`MutCollect` is what CSE trusts to say whether a name is stable, so it counts every
  store spelling, not the one the lowering emits.** CSE treats a name outside its set as
  constant for the whole block; a missed store is a shared subexpression across a mutation.
  Copies are only the visible half - `<-` also zeroes its SOURCE, `:=` lowers to a
  `builtin`clone`(dst, src)` CALL rather than an `ExprClone`, `++`/`+=` are their own
  nodes, and a by-reference
  argument writes with no assignment node anywhere. Hence the argument arm keys on the
  callee's parameter type (non-const and `ref` or a ref type), not on a node kind.
- **`delete` on a container of `ExpressionPtr` frees the BUFFER, never the nodes** -
  `delete array<T?>` frees the pointees only for das-heap `T`, and `Expression` is a
  handled C++ type whose instances are not heap chunks at all (the
  measurement: an `array<S?>` of das structs returns its pointees to `heap_bytes_allocated`,
  an `array<ExpressionPtr>` returns only the buffer and the nodes surface in the exit GC
  report). That is why `make_float_ctor`'s const-fold early return may leave its lanes
  un-consumed while the ctor path `emplace`s them away, why every `unsafe { delete args }`
  after it is sound over borrowed tree nodes, and why a struct field holding a borrowed
  node needs no `@do_not_delete`. Node lifetime belongs to the AST GC: a lane the const
  fold drops is unreachable and collected at the enclosing `ast_gc_guard`.
- **The whitelist admits value-returning primitives only.** `lower_stmt`'s fall-through arm
  lowers an unrecognized statement for its lifted sub-lets and drops the statement itself,
  which is correct exactly while every surviving call is pure - so `lift_expr` refuses a
  whitelisted call that writes through a by-reference argument (`sincos`) rather than let
  the drop delete the store. Predicating such a write would need per-out-param temps the
  lowering does not own.

## 28. shader_block_layout

- **Two rails, deliberately separate** - the LAYOUT rail admits int64/uint64 as block
  members (`compute_block_layout` special-cases them) while the ARITHMETIC rail rejects
  64-bit INT (`arith_width_ok` allows width 64 only for floats); `cpu_only_lattice_width`
  keys both emitters' fail-closed diagnostic.

## 29. shader_lingua_franca {#shader-lingua-franca}

- **Every symbol is either an exact CPU mirror of its GPU semantics or a `[sideeffects]`
  dummy every rail lowers by name** - the dummies return zero on the host, so a CPU replay
  reproduces GPU semantics only for the real-bodied set. Unsigned overloads never fold into
  signed twins (glslang picks the unsigned opcode).
- **A width-variant of a lowered-by-name symbol is one more overload here, never an emitter
  arm.** `unpack8` carries `int16 -> byte2` and `uint16 -> ubyte2` beside the 32-bit pair; every
  overload is the same `reinterpret` on the host and the same single `OpBitcast` on the SPIR-V
  rail, so the emitter matches the name and reads the width off the operand type.
