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
//                                                          user-mutable var (var/let)
//   or      ::= and ('||' and)*
//   and     ::= cmp ('&&' cmp)*
//   cmp     ::= add (('<' | '>' | '<=' | '>=' | '==' | '!=') add)?
//   add     ::= mul (('+' | '-') mul)*
//   mul     ::= unary (('*' | '/') unary)*
//   unary   ::= ('-' | '!')? primary
//   primary ::= NUMBER | ID | ID '(' assign (',' assign)* ')' | '(' seq ')'
//
// var/let/=/,/; semantics:
//   * var <name> = expr / let <name> = expr -- declare a fresh user-mutable variable.
//     ERROR if <name> already exists (external var, prior var/let, or function name).
//   * <name> = expr -- assignment; LHS must be a previously-declared user var. Returns
//     the assigned value. External (caller-registered) names are read-only.
//   * `,` and `;` -- sequence (C-style comma operator); both forms are accepted and
//     identical. Evaluates left for side effects, returns right.
//   * Plain identifier reference -- ERROR if the name is not registered (no implicit
//     declaration). Function-arg commas separate arguments, NOT the sequence operator.
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

// ============================================================================
// Function parse info (nargs, type constraints -- no emitFn here)
// ============================================================================

struct FuncParseInfo
{
  uint8_t nargs;
  bool firstArgBool;
};

typedef HashedKeyMap<uint64_t, FuncParseInfo, 0ULL> FuncParseMap;

inline void register_func_parse(FuncParseMap &m, const char *name, uint8_t nargs, bool firstArgBool = false)
{
  m.emplace(hash_name(name), FuncParseInfo{nargs, firstArgBool});
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

int parseToIR(const char *text, Tab<IRNode> &ir, const FuncParseMap &funcs, const NodeEmitMap &emitMap, NameMap &varMap,
  uint16_t &nextVarId, eastl::string &error);

// Returns PARSE_ERROR on failure (unknown tag). Any partial arena writes are left in place;
// callers that want rollback on failure should snapshot arena.size() before the call.
uint32_t compile(const Tab<IRNode> &ir, int irRoot, Arena &arena, const NodeEmitMap &emitMap);

// outMaxVarId (optional): the highest varId referenced by the expression, or -1 if none.
// Callers should verify numVars > outMaxVarId once before entering the eval loop.
uint32_t parse(const char *text, Arena &arena, const FuncParseMap &parseMap, const NodeEmitMap &emitMap, NameMap &varMap,
  uint16_t &nextVarId, eastl::string &error, int *outMaxVarId = nullptr);

} // namespace lcexpr
