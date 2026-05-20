//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <landClassEval/lcExprNodes.h>
#include <util/dag_hashedKeyMap.h>
#include <hash/wyhash.h>
#include <EASTL/string.h>
#include <string.h>
#include <new>

// Landclass expression parser -- declarations.
// Parser produces IR (tagged tree). Compiler/fusion converts IR to arena EvalNodes.
//
// Grammar (lowest to highest precedence):
//   program ::= seq                                     -- public entry
//   seq     ::= stmt ((',' | ';') stmt)*                -- left-assoc, value = last stmt
//   stmt    ::= ('var' | 'let') ID '=' assign | assign
//   assign  ::= or ('=' assign)?                        -- right-assoc; LHS must be a
//                                                          previously-declared `var`
//                                                          (let-declared vars are
//                                                          immutable in the same parse).
//   or      ::= and ('||' and)*
//   and     ::= cmp ('&&' cmp)*
//   cmp     ::= add (('<' | '>' | '<=' | '>=' | '==' | '!=') add)?
//   add     ::= mul (('+' | '-') mul)*
//   mul     ::= unary (('*' | '/') unary)*
//   unary   ::= ('-' | '!')? primary
//   primary ::= NUMBER | ID | ID '(' assign (',' assign)* ')' | '(' seq ')'
//             | 'if' assign 'then' assign 'else' assign
//             | '{' seq '}'                              -- non-empty block, scoped vars
//
// var/let/=/,/; semantics:
//   * var <name> = expr -- declare a fresh user-MUTABLE variable.
//   * let <name> = expr -- declare a fresh user-IMMUTABLE variable. Within the same
//     parse, attempting `<name> = ...` after a let-declaration produces a parse error.
//     (Cross-parse the immutability tracking is per-parse only -- a subsequent parse
//     that reuses the NameMap loses the let attribute.)
//   * Both forms ERROR if <name> already exists (external var, prior var/let, or
//     function name) in the same visible scope. Reserved keywords (var/let/if/then/
//     else) cannot be used as variable names.
//   * <name> = expr -- assignment; LHS must be a previously-declared user var that
//     was declared via `var` (not `let`). Returns the assigned value. External
//     (caller-registered) names are read-only.
//   * `,` and `;` -- sequence (C-style comma operator); both forms are accepted and
//     identical. Evaluates left for side effects, returns right.
//   * Plain identifier reference -- ERROR if the name is not registered (no implicit
//     declaration). Function-arg commas separate arguments, NOT the sequence operator.
//
// if-then-else:
//   `if cond then a else b` evaluates to `a` when cond is non-zero, else `b`. It is
//   equivalent to `select(cond, a, b)` and lowers to the same IR/eval node, including
//   the bool-cond requirement and the all-const fold. The `then` and `else` branches
//   are each parsed as a full assignment-level expression, so binary operators bind to
//   the nearest branch (e.g. `if c then a + 1 else b` is `if c then (a+1) else b`).
//
// { block } and temp registers:
//   `{ stmts }` evaluates the inner sequence (must be non-empty -- empty `{}` is a
//   parse error) and returns the last statement's value, like parenthesised `(...)`.
//   The difference is scope: `var`/`let` declared inside `{...}` are visible only
//   within that block. The parser allocates each block-scoped variable from a
//   stack-discipline TEMP REGISTER pool: when the block exits, the slot is freed and
//   may be reused by subsequent declarations.
//
//   The caller passes `maxTempRegs` (<= MAX_TEMP_REGS) as the cap on simultaneously
//   live block-scoped variables, and gets back `outPeakTempRegs` -- the actual peak
//   number of temp slots used by this expression (post-fold). Parse fails if the peak
//   exceeds the cap. At runtime block-scoped vars share the same `vars[]` array as
//   named vars; their IDs occupy slots immediately above the named user-var range
//   ([nextVarId .. nextVarId + outPeakTempRegs - 1]), recycled per scope.
//
// Types: every value is a float. Comparisons and !, &&, || treat 0.f as false and
// non-zero as true, and produce exactly 0.f or 1.f. &&, ||, ! require bool operands.
// select() requires a bool first argument. Chained comparisons (a < b < c) are rejected.
//
// Built-in functions:
//   unary:   saturate(x), smoothstep(x), sqrt(x), abs(x), frac(x)
//   binary:  max(a,b), min(a,b), pow(a,b)
//   ternary: clamp(x,lo,hi), ramp(x,from,to), smooth_ramp(x,from,to),
//            lerp(a,b,t), select(cond_bool, then, else)
//
// Semantics / safety:
//   * pow(x, y)        := powf(fabsf(x), y); never NaN, sign of x is dropped by design.
//   * sqrt(x)          := sqrtf(max(x, 0)); negative input clamps to 0.
//   * a / b            := raw IEEE division; may produce +-inf or NaN. Use evalSafe
//                         to clamp the final result to [0, 1] and map non-finite -> 0.
//   * ramp(x,from,to)  := clamp((x - from) / (to - from), 0, 1). If |to - from| < 1e-10
//                         acts as a step function (x >= from ? 1 : 0). Inverted ranges
//                         (from > to) produce an inverted ramp.
//   * smooth_ramp      := 3t^2 - 2t^3 applied to the ramp result.
//   * x == y           := |x - y| < 1e-6 (absolute epsilon).
//   * && and ||        := short-circuit (right operand evaluated only when necessary).
//
// Numbers are parsed locale-independently; only '.' is accepted as decimal separator.
// Identifiers: ASCII letters / digits / underscore; max length 63; first char non-digit.
// Parse depth is capped at 64 nested parentheses / unary minuses to prevent stack overflow.
//
// At eval time nodes read ctx.vars[varId] unchecked. Callers must ensure
// numVars > computeMaxVarId(ir, root) (see below). The check is O(ir.size()) and is
// intended to be done once per expression, not per pixel.

namespace lcexpr
{

// ============================================================================
// Name hashing
// ============================================================================

inline uint64_t hash_name(const char *s, size_t len) { return wyhash(s, len, 0, _wyp); }
inline uint64_t hash_name(const char *s) { return hash_name(s, strlen(s)); }

typedef HashedKeyMap<uint64_t, uint16_t, 0ULL> NameMap;

// Practical limit. DIRECT_VAR(id) must not collide with PARSE_ERROR == ~0u, which forces
// MAX_VAR_ID < 0xFFFF (since DIRECT_VAR_BASE + 0xFFFF == 0xFFFFFFFF).
static constexpr uint16_t MAX_VAR_ID = 256;
static constexpr uint16_t VAR_ID_OVERFLOW = 0xFFFFu; // sentinel returned by register_var on overflow
static_assert(MAX_VAR_ID < VAR_ID_OVERFLOW, "MAX_VAR_ID must be below the overflow sentinel");
static_assert(DIRECT_VAR_BASE + MAX_VAR_ID < ~uint32_t(0), "DIRECT_VAR(MAX_VAR_ID-1) must not collide with PARSE_ERROR");

// Hard cap on the per-parse temp register budget (used by `{ block }` scoped vars).
// Callers pass 0..MAX_TEMP_REGS as `maxTempRegs` to parse(); a parse whose actual peak
// (returned via `outPeakTempRegs`) exceeds the supplied cap fails with a clear error
// rather than silently truncating.
static constexpr uint8_t MAX_TEMP_REGS = 16;

// ============================================================================
// Type system (compile-time only)
// ============================================================================
//
// Every value flowing through the parser carries an ExprType tag. At runtime
// everything is still a single float (bool nodes return 0.f / 1.f, user types
// like Curve store an integer slot index in the float slot), so the type
// system is purely for parse-time type checking.
//
// Reserved IDs:
//   TYPE_FLOAT (0)       -- all numeric values; the default for register_var.
//   TYPE_BOOL  (1)       -- comparisons / && / || / ! / select-cond. Implicitly
//                           assignable to TYPE_FLOAT (1.f / 0.f) because the
//                           runtime representation is identical.
//   TYPE_USER_BASE..TYPE_USER_BASE + MAX_USER_TYPES - 1
//                        -- caller-defined opaque types (e.g. HeightmapLand
//                           reserves one slot for "Curve"). User types are
//                           strictly equality-checked: a user-typed value
//                           cannot flow into a Float / Bool slot or vice versa.
typedef uint16_t ExprType;
static constexpr ExprType TYPE_FLOAT = 0;
static constexpr ExprType TYPE_BOOL = 1;
static constexpr ExprType TYPE_USER_BASE = 2;
static constexpr int MAX_USER_TYPES = 14;
static constexpr int MAX_TYPE_SLOTS = TYPE_USER_BASE + MAX_USER_TYPES;
static constexpr ExprType TYPE_INVALID = 0xFFFFu;

// Implicit-conversion rule: bool flows to float (the runtime value already is a
// 0.f/1.f float). All other cross-type assignments are rejected at parse time.
inline bool type_assignable_to(ExprType from, ExprType to)
{
  if (from == to)
    return true;
  if (from == TYPE_BOOL && to == TYPE_FLOAT)
    return true;
  return false;
}

// Per-parse mapping varId -> ExprType. Indexed densely by varId; unset slots
// default to TYPE_FLOAT. Caller pre-populates entries for external (read-only)
// vars before parsing; the parser appends entries for user-declared `var`/`let`
// slots and writes their inferred type.
typedef Tab<ExprType> VarTypeTable;

inline ExprType var_type_or_float(const VarTypeTable *types, uint16_t vid)
{
  if (!types || vid >= (uint16_t)types->size())
    return TYPE_FLOAT;
  return (*types)[vid];
}

// Returns VAR_ID_OVERFLOW when nextId would exceed MAX_VAR_ID. In that case nextId is left
// unchanged and nothing is added to the map, so callers observing VAR_ID_OVERFLOW can surface
// an error without corrupting the name table. The parser converts the sentinel into a clean
// error message. Callers building the NameMap directly should check the return value.
inline uint16_t register_var(NameMap &map, uint16_t &nextId, const char *name)
{
  uint64_t h = hash_name(name);
  auto r = map.emplace_if_missing(h);
  if (r.second)
  {
    if (nextId >= MAX_VAR_ID)
    {
      map.erase(h); // undo the provisional insertion so future queries fail cleanly
      return VAR_ID_OVERFLOW;
    }
    *r.first = nextId++;
  }
  return *r.first;
}

// Typed registration: like register_var but also writes `type` into `types[vid]`.
// `types` is grown to vid+1 entries (filling new slots with TYPE_FLOAT) so the
// parser can index it densely. Returns VAR_ID_OVERFLOW with no side effects on
// overflow, just like register_var.
inline uint16_t register_var_typed(NameMap &map, uint16_t &nextId, VarTypeTable &types, const char *name, ExprType type)
{
  uint16_t id = register_var(map, nextId, name);
  if (id == VAR_ID_OVERFLOW)
    return id;
  if ((int)id >= types.size())
  {
    int oldSize = types.size();
    types.resize(id + 1);
    for (int i = oldSize; i < (int)types.size(); i++)
      types[i] = TYPE_FLOAT;
  }
  types[id] = type;
  return id;
}

// ============================================================================
// Function parse info (nargs, type constraints -- no emitFn here)
// ============================================================================
//
// argTypes[i] is the EXPECTED type of the i-th argument; the parser checks each
// caller-supplied argument against this slot via type_assignable_to. resultType
// is the type the function-call expression itself produces; reads of the call
// result downstream see this type.
struct FuncParseInfo
{
  uint8_t nargs;
  ExprType resultType;
  ExprType argTypes[3];
};

typedef HashedKeyMap<uint64_t, FuncParseInfo, 0ULL> FuncParseMap;

// Back-compat shim: untyped registration where every arg and the result are
// FLOAT. `firstArgBool=true` upgrades argTypes[0] to BOOL so the existing
// select() registration keeps working without churn at the call site.
inline void register_func_parse(FuncParseMap &m, const char *name, uint8_t nargs, bool firstArgBool = false)
{
  FuncParseInfo fi;
  fi.nargs = nargs;
  fi.resultType = TYPE_FLOAT;
  for (int i = 0; i < 3; i++)
    fi.argTypes[i] = TYPE_FLOAT;
  if (firstArgBool && nargs >= 1)
    fi.argTypes[0] = TYPE_BOOL;
  m.emplace(hash_name(name), fi);
}

// Full typed registration: caller supplies the per-argument type list and the
// result type. `argTypes` may be nullptr only when nargs == 0. Reserved for
// callers that need user-type constraints (e.g. HeightmapLand's `curve(c, x)`
// where argTypes[0] is the externally-defined CURVE type).
inline void register_func_parse_typed(FuncParseMap &m, const char *name, uint8_t nargs, const ExprType *argTypes, ExprType resultType)
{
  FuncParseInfo fi;
  fi.nargs = nargs;
  fi.resultType = resultType;
  for (int i = 0; i < 3; i++)
    fi.argTypes[i] = (argTypes && i < nargs) ? argTypes[i] : TYPE_FLOAT;
  m.emplace(hash_name(name), fi);
}

FuncParseMap make_default_func_parse_map();

// ============================================================================
// Node emit registry (tag -> arena emitter, used by compiler)
// ============================================================================

typedef uint32_t (*NodeEmitFn)(Arena &arena, const uint32_t *childArenaOfs);

typedef HashedKeyMap<uint64_t, NodeEmitFn, 0ULL> NodeEmitMap;

NodeEmitMap make_default_node_emit_map();

// ============================================================================
// IR node -- lightweight tagged tree, produced by parser, consumed by fusion/compiler
// ============================================================================

struct IRNode
{
  uint64_t tag;
  float constVal; // meaningful when tag == hash_name("const")
  uint16_t varId; // meaningful when tag == hash_name("var")
  uint8_t nc;     // number of children used
  uint8_t _pad;
  int c[3]; // child indices into IR array, -1 = none
};

// ============================================================================
// Variable usage mask (computed from IR)
// Only tracks the first 32 varIds (sufficient for built-in height/angle/curvature/mask).
// ============================================================================

// Both var nodes (reads) and assign nodes (writes) carry a varId; assign nodes don't
// need a separate var-read child since the LHS slot is encoded in the assign IR node
// itself (the parser pops the var IR node when synthesising the assign). Both cases
// must contribute to the touched-var bitset so callers don't undersize vars[].
inline uint32_t computeVarMask(const Tab<IRNode> &ir, int idx)
{
  if (idx < 0)
    return 0;
  const IRNode &n = ir[idx];
  const bool touchesVar = n.tag == hash_name("var") || n.tag == hash_name("assign");
  uint32_t m = (touchesVar && n.varId < 32) ? (1u << n.varId) : 0;
  for (int i = 0; i < n.nc; i++)
    m |= computeVarMask(ir, n.c[i]);
  return m;
}

inline uint32_t varMaskFromRoot(uint32_t root)
{
  if (root >= DIRECT_VAR_BASE)
  {
    uint32_t id = root - DIRECT_VAR_BASE;
    return id < 32 ? (1u << id) : 0;
  }
  return 0;
}

// Highest varId referenced anywhere in the IR, or -1 if no variables are present.
// Callers must compare (maxVarId + 1) against numVars once before entering the per-pixel
// eval loop; nodes skip bounds checks internally.
//
// Implemented as a flat scan over the IR array rather than a recursive tree walk.
// parseToIR() rolls back the whole IR on failure, so after a successful parse every node
// in `ir` is reachable from the root, and every var/assign node is relevant. The flat
// scan is O(N) and cannot stack-overflow on a long chain like a+a+a+... where the IR
// tree is deeply left-recursive (N deep) even though parseExpr() only caps parenthesis /
// unary nesting. `idx` is kept in the signature for backwards compatibility but only
// used to short-circuit empty inputs.
//
// Both `var` (read) and `assign` (write) carry a varId. An expression that only writes
// (e.g. `var a = 1`) emits assign nodes without a corresponding var read node, so we
// must include both tags or vars[] gets sized too small.
inline int computeMaxVarId(const Tab<IRNode> &ir, int idx)
{
  if (idx < 0 || ir.size() == 0)
    return -1;
  const uint64_t varTag = hash_name("var");
  const uint64_t assignTag = hash_name("assign");
  int m = -1;
  for (const IRNode &n : ir)
    if ((n.tag == varTag || n.tag == assignTag) && (int)n.varId > m)
      m = (int)n.varId;
  return m;
}

// Same for a compiled root (handles DIRECT_VAR). Use this when only the compiled form is
// available. For arena roots a full arena walk is NOT needed: the compile-time IR walk is
// the authoritative source, so capture the max during parse and store it alongside root.
inline int maxVarIdFromRoot(uint32_t root)
{
  if (root >= DIRECT_VAR_BASE)
    return (int)(root - DIRECT_VAR_BASE);
  return -1; // caller must supply the IR-computed max for non-direct roots
}

// ============================================================================
// Parse API
// ============================================================================

static constexpr uint32_t PARSE_ERROR = ~0u;

// maxTempRegs (default 0): caller-supplied CAP on simultaneously-live block-scoped
//   variables. Parse fails if the actual peak exceeds this cap. With the default of 0,
//   only `{...}` blocks that introduce no var/let are accepted (i.e. blocks that need
//   no temp slots in vars[]).
// outPeakTempRegs (optional): MEASURED peak number of temp slots used by the expression.
//   Always 0 for expressions without `{...}`-scoped var/let declarations. The caller
//   must size vars[] >= nextVarId + outPeakTempRegs (or equivalently, >= outMaxVarId+1
//   from parse()) before eval, since temp slots live in the same vars[] buffer.
//
// flags: bitwise-OR of `lcexpr::ParseFlags` controlling parse/DCE behavior. See
//   the enum below. Default (PARSE_EXPORT_TOP_LEVEL_VARS) preserves all top-level
//   user-var writes; clear it for layer-style parses where the layer's top-level
//   user vars are not consumed by anyone past this parse's eval.

// ============================================================================
// Compile/parse flags
// ============================================================================
enum ParseFlags : uint32_t
{
  PARSE_DEFAULT = 0,
  // Treat top-level user vars (those declared via `var`/`let` at the outermost
  // depth of THIS parse, not block-scoped) as live at the root, so their writes
  // are preserved by dead-store elimination. The daEditor's COMMON expression
  // sets this -- its vars are read by subsequent layer parses against the same
  // varMap. Layer parses leave it clear because layer-local user vars have no
  // observable consumer past the layer's own eval. Block-scoped temp writes
  // are always DCE-eligible regardless of this flag.
  PARSE_EXPORT_TOP_LEVEL_VARS = 1u << 0,
};

// varTypes (optional): per-varId type table. Pre-populate entries for external
// (caller-registered) vars before parse; the parser appends entries for `var`/
// `let`-declared user vars, writing each slot's inferred type. nullptr is the
// untyped path: every var is treated as FLOAT and all function-arg type checks
// degrade to "anything assignable to FLOAT" (which currently means FLOAT or
// BOOL). New callers that need user types must supply this.
int parseToIR(const char *text, Tab<IRNode> &ir, const FuncParseMap &funcs, const NodeEmitMap &emitMap, NameMap &varMap,
  uint16_t &nextVarId, eastl::string &error, uint8_t maxTempRegs = 0, int *outPeakTempRegs = nullptr,
  uint32_t flags = PARSE_EXPORT_TOP_LEVEL_VARS, VarTypeTable *varTypes = nullptr);

// Returns PARSE_ERROR on failure (unknown tag). Any partial arena writes are left in place;
// callers that want rollback on failure should snapshot arena.size() before the call.
uint32_t compile(const Tab<IRNode> &ir, int irRoot, Arena &arena, const NodeEmitMap &emitMap);

// outMaxVarId (optional): the highest varId referenced by the expression, or -1 if none.
// Callers should verify numVars > outMaxVarId once before entering the eval loop.
// maxTempRegs / outPeakTempRegs / flags / varTypes: see parseToIR doc above.
uint32_t parse(const char *text, Arena &arena, const FuncParseMap &parseMap, const NodeEmitMap &emitMap, NameMap &varMap,
  uint16_t &nextVarId, eastl::string &error, int *outMaxVarId = nullptr, uint8_t maxTempRegs = 0, int *outPeakTempRegs = nullptr,
  uint32_t flags = PARSE_EXPORT_TOP_LEVEL_VARS, VarTypeTable *varTypes = nullptr);

} // namespace lcexpr
