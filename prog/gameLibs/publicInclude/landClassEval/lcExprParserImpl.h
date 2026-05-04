//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <landClassEval/lcExprParser.h>
#include <stdio.h>

#ifndef LCEXPR_PARSER_INLINE
#define LCEXPR_PARSER_INLINE inline
#endif

// Landclass expression parser + compiler -- implementation.
namespace lcexpr
{

// ============================================================================
// Tag constants (inline avoids static-in-inline ODR issues)
// ============================================================================

inline const uint64_t TAG_CONST = hash_name("const");
inline const uint64_t TAG_VAR = hash_name("var");
inline const uint64_t TAG_RAMP = hash_name("ramp");
inline const uint64_t TAG_SRAMP = hash_name("smooth_ramp");
inline const uint64_t TAG_ASSIGN = hash_name("assign");
inline const uint64_t TAG_SEQ = hash_name("seq");

// ============================================================================
// Locale-independent number parsing
// ============================================================================

// Parses digits[.digits][e[+-]?digits] starting at *pp. Advances *pp past the last consumed
// char. Only '.' is recognised as a decimal separator; the C locale is not consulted.
// Caller must guarantee *pp is either a digit or '.' followed by a digit.
inline float parse_float_locale_indep(const char *&p)
{
  double val = 0.0;
  while (*p >= '0' && *p <= '9')
  {
    val = val * 10.0 + (*p - '0');
    p++;
  }
  if (*p == '.')
  {
    p++;
    double scale = 1.0;
    while (*p >= '0' && *p <= '9')
    {
      val = val * 10.0 + (*p - '0');
      scale *= 10.0;
      p++;
    }
    val /= scale;
  }
  if (*p == 'e' || *p == 'E')
  {
    const char *ePos = p;
    p++;
    bool expNeg = false;
    if (*p == '+')
      p++;
    else if (*p == '-')
    {
      expNeg = true;
      p++;
    }
    int exp = 0;
    bool hasExpDigit = false;
    while (*p >= '0' && *p <= '9')
    {
      hasExpDigit = true;
      // Cap accumulation so `exp` cannot overflow on pathological input like
      // "1e999999999999". 10000 is far beyond the double range (~10^308); once we
      // reach this cap further digits cannot change the observable result.
      if (exp < 10000)
        exp = exp * 10 + (*p - '0');
      p++;
    }
    if (!hasExpDigit)
      p = ePos; // "1e" with no trailing digits: 'e' is not part of the number
    else if (val != 0.0)
    {
      // Let pow() handle IEEE saturation. A fixed-exp clamp is wrong here because
      // the mantissa matters: 0.0001e50 should overflow to +inf (1e46) and 1e-50
      // should underflow to 0. pow(10, +large) -> inf, pow(10, -large) -> 0, and
      // val * inf / val * 0 both produce the correct saturating float cast.
      // The val == 0 branch is short-circuited to avoid 0 * inf -> NaN.
      double shift = pow(10.0, expNeg ? -(double)exp : (double)exp);
      val *= shift;
    }
  }
  return (float)val;
}

// ============================================================================
// Arena emit helpers
// ============================================================================

template <typename T>
inline uint32_t emit(Arena &arena)
{
  size_t ofs = arena.size();
  size_t aligned = (ofs + alignof(T) - 1) & ~(alignof(T) - 1);
  size_t pad = aligned - ofs;
  if (pad)
    arena.append_default(pad);
  uint8_t *p = arena.append_default(sizeof(T));
  new (p) T();
  return (uint32_t)aligned;
}

template <typename T>
inline T *at(Arena &arena, uint32_t ofs)
{
  return reinterpret_cast<T *>(arena.data() + ofs);
}

template <typename T>
uint32_t emitUnaryFn(Arena &a, const uint32_t *args)
{
  uint32_t o = emit<T>(a);
  at<T>(a, o)->n0 = args[0];
  return o;
}
template <typename T>
uint32_t emitBinaryFn(Arena &a, const uint32_t *args)
{
  uint32_t o = emit<T>(a);
  auto *p = at<T>(a, o);
  p->n0 = args[0];
  p->n1 = args[1];
  return o;
}
template <typename T>
uint32_t emitTernaryFn(Arena &a, const uint32_t *args)
{
  uint32_t o = emit<T>(a);
  auto *p = at<T>(a, o);
  p->n0 = args[0];
  p->n1 = args[1];
  p->n2 = args[2];
  return o;
}

// ============================================================================
// Default registries
// ============================================================================

LCEXPR_PARSER_INLINE FuncParseMap make_default_func_parse_map()
{
  FuncParseMap m;
  register_func_parse(m, "max", 2);
  register_func_parse(m, "min", 2);
  register_func_parse(m, "pow", 2);
  register_func_parse(m, "clamp", 3);
  register_func_parse(m, "ramp", 3);
  register_func_parse(m, "smooth_ramp", 3);
  register_func_parse(m, "lerp", 3);
  register_func_parse(m, "saturate", 1);
  register_func_parse(m, "smoothstep", 1);
  register_func_parse(m, "sqrt", 1);
  register_func_parse(m, "abs", 1);
  register_func_parse(m, "frac", 1);
  register_func_parse(m, "select", 3, true);
  return m;
}

LCEXPR_PARSER_INLINE NodeEmitMap make_default_node_emit_map()
{
  NodeEmitMap m;
  // operators
  m.emplace(hash_name("neg"), emitUnaryFn<NegNode>);
  m.emplace(hash_name("not"), emitUnaryFn<NotNode>);
  m.emplace(hash_name("add"), emitBinaryFn<AddNode>);
  m.emplace(hash_name("sub"), emitBinaryFn<SubNode>);
  m.emplace(hash_name("mul"), emitBinaryFn<MulNode>);
  m.emplace(hash_name("div"), emitBinaryFn<DivNode>);
  m.emplace(hash_name("lt"), emitBinaryFn<LtNode>);
  m.emplace(hash_name("gt"), emitBinaryFn<GtNode>);
  m.emplace(hash_name("le"), emitBinaryFn<LeNode>);
  m.emplace(hash_name("ge"), emitBinaryFn<GeNode>);
  m.emplace(hash_name("eq"), emitBinaryFn<EqNode>);
  m.emplace(hash_name("ne"), emitBinaryFn<NeNode>);
  m.emplace(hash_name("and"), emitBinaryFn<AndNode>);
  m.emplace(hash_name("or"), emitBinaryFn<OrNode>);
  // functions
  m.emplace(hash_name("max"), emitBinaryFn<MaxNode>);
  m.emplace(hash_name("min"), emitBinaryFn<MinNode>);
  m.emplace(hash_name("pow"), emitBinaryFn<PowNode>);
  m.emplace(hash_name("clamp"), emitTernaryFn<ClampNode>);
  m.emplace(hash_name("ramp"), emitTernaryFn<RampNode>);
  m.emplace(hash_name("smooth_ramp"), emitTernaryFn<SmoothRampNode>);
  m.emplace(hash_name("lerp"), emitTernaryFn<LerpNode>);
  m.emplace(hash_name("select"), emitTernaryFn<SelectNode>);
  m.emplace(hash_name("saturate"), emitUnaryFn<SaturateNode>);
  m.emplace(hash_name("smoothstep"), emitUnaryFn<SmoothstepNode>);
  m.emplace(hash_name("sqrt"), emitUnaryFn<SqrtNode>);
  m.emplace(hash_name("abs"), emitUnaryFn<AbsNode>);
  m.emplace(hash_name("frac"), emitUnaryFn<FracNode>);
  // Sequence operator (`,` / `;`): plain binary, fold-friendly when both sides are
  // const. Assign is deliberately NOT here -- AssignNode is hand-emitted in compileNode
  // because it carries varId per-node, and tryFold must not collapse it (writing to
  // vars[] is the whole point).
  m.emplace(hash_name("seq"), emitBinaryFn<SeqNode>);
  return m;
}

// ============================================================================
// Expression type annotation (compile-time only)
// ============================================================================

enum ExprType : uint8_t
{
  TYPE_FLOAT,
  TYPE_BOOL
};

// ============================================================================
// Parser -- produces IR
// ============================================================================

struct Parser
{
  static constexpr int MAX_DEPTH = 64; // max nested parens + unary minuses, bounds stack use

  const char *src, *pos;
  Tab<IRNode> *ir;
  const FuncParseMap *funcs;
  const NodeEmitMap *emitMap; // for const folding
  NameMap *varMap;
  uint16_t *nextVarId;
  eastl::string *err;
  bool failed;
  int depth;
  // Any varId < externalVarBase is an external (caller-registered) read-only var.
  // Anything >= externalVarBase was introduced by `var`/`let` during this parse and is
  // user-mutable. Captured at parse start from *nextVarId.
  uint16_t externalVarBase;
  // The varId of the var/let currently having its initialiser parsed, or
  // VAR_ID_OVERFLOW when no initialiser is in flight. parsePrimary uses this to reject
  // self-references like `var a = a + 1` cleanly. Saved/restored across nested var/let
  // so a sibling `var b = ...` inside the outer's initialiser does not change which
  // name is "in flight" for the outer self-reference check.
  uint16_t inFlightVarId;

  enum Tok
  {
    T_END,
    T_NUM,
    T_ID,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_LT,
    T_GT,
    T_LE,
    T_GE,
    T_EQ,
    T_NE,
    T_AND,
    T_OR,
    T_NOT,
    T_ASSIGN,
    T_LPAREN,
    T_RPAREN,
    T_COMMA,
    T_SEMI
  };
  Tok tok;
  float tokNum;
  char tokId[64];

  struct TNode
  {
    int idx;
    ExprType type;
  };

  // Bounds check is intentional: tryFold is reachable with a stale idx==0 after a deep
  // sub-parse hit the depth cap before emitting anything. Without `idx < ir->size()`
  // we'd read past-end of an empty Tab on the unwind path (e.g. a 200-char unary
  // chain).
  bool isConst(int idx) { return idx >= 0 && idx < (int)ir->size() && (*ir)[idx].tag == hash_name("const"); }
  float constVal(int idx) { return (*ir)[idx].constVal; }

  int addConst(float v)
  {
    int idx = (int)ir->size();
    IRNode n = {};
    n.tag = hash_name("const");
    n.constVal = v;
    ir->push_back(n);
    return idx;
  }

  int addIR(uint64_t tag, uint8_t nc = 0, int c0 = -1, int c1 = -1, int c2 = -1)
  {
    int idx = (int)ir->size();
    IRNode n = {};
    n.tag = tag;
    n.nc = nc;
    n.c[0] = c0;
    n.c[1] = c1;
    n.c[2] = c2;
    ir->push_back(n);
    return idx;
  }

  void error(const char *msg)
  {
    if (failed)
      return;
    failed = true;
    int at = (int)(pos - src);
    // Show a short window around pos with a caret on the next line. Cap the window so
    // very long inputs do not produce megabyte error strings.
    int winStart = at - 20;
    if (winStart < 0)
      winStart = 0;
    int winEnd = at + 20;
    int srcLen = (int)strlen(src);
    if (winEnd > srcLen)
      winEnd = srcLen;
    char buf[512];
    int n = snprintf(buf, sizeof(buf), "at pos %d: %s\n  %.*s\n  ", at, msg, winEnd - winStart, src + winStart);
    if (n > 0 && n < (int)sizeof(buf))
    {
      int caret = at - winStart;
      for (int i = 0; i < caret && n < (int)sizeof(buf) - 2; i++)
        buf[n++] = ' ';
      if (n < (int)sizeof(buf) - 1)
        buf[n++] = '^';
      buf[n] = 0;
    }
    *err = buf;
  }

  // Depth guard for recursive descent. Returns false when the cap is hit.
  bool enterDepth()
  {
    if (++depth > MAX_DEPTH)
    {
      error("expression too deeply nested");
      return false;
    }
    return true;
  }
  void leaveDepth() { --depth; }

  void next()
  {
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r')
      pos++;
    if (!*pos)
    {
      tok = T_END;
      return;
    }
    if ((*pos >= '0' && *pos <= '9') || (*pos == '.' && pos[1] >= '0' && pos[1] <= '9'))
    {
      tok = T_NUM;
      tokNum = parse_float_locale_indep(pos);
      return;
    }
    if ((*pos >= 'a' && *pos <= 'z') || (*pos >= 'A' && *pos <= 'Z') || *pos == '_')
    {
      tok = T_ID;
      int len = 0;
      bool overflowed = false;
      while ((*pos >= 'a' && *pos <= 'z') || (*pos >= 'A' && *pos <= 'Z') || (*pos >= '0' && *pos <= '9') || *pos == '_')
      {
        if (len < 63)
          tokId[len++] = *pos;
        else
          overflowed = true;
        pos++;
      }
      tokId[len] = 0;
      if (overflowed)
      {
        error("identifier too long (max 63 chars)");
        tok = T_END;
      }
      return;
    }
    if (pos[0] == '<' && pos[1] == '=')
    {
      tok = T_LE;
      pos += 2;
      return;
    }
    if (pos[0] == '>' && pos[1] == '=')
    {
      tok = T_GE;
      pos += 2;
      return;
    }
    if (pos[0] == '=' && pos[1] == '=')
    {
      tok = T_EQ;
      pos += 2;
      return;
    }
    if (pos[0] == '!' && pos[1] == '=')
    {
      tok = T_NE;
      pos += 2;
      return;
    }
    if (pos[0] == '&' && pos[1] == '&')
    {
      tok = T_AND;
      pos += 2;
      return;
    }
    if (pos[0] == '|' && pos[1] == '|')
    {
      tok = T_OR;
      pos += 2;
      return;
    }
    switch (*pos++)
    {
      case '+': tok = T_PLUS; return;
      case '-': tok = T_MINUS; return;
      case '*': tok = T_STAR; return;
      case '/': tok = T_SLASH; return;
      case '<': tok = T_LT; return;
      case '>': tok = T_GT; return;
      case '!': tok = T_NOT; return;
      case '=': tok = T_ASSIGN; return; // `==` is consumed earlier; lone `=` is assignment.
      case '(': tok = T_LPAREN; return;
      case ')': tok = T_RPAREN; return;
      case ',': tok = T_COMMA; return;
      case ';': tok = T_SEMI; return;
      default: error("unexpected character"); tok = T_END;
    }
  }

  bool expect(Tok t)
  {
    if (tok != t)
    {
      error("unexpected token");
      return false;
    }
    next();
    return true;
  }
  void requireBool(TNode n, const char *msg)
  {
    if (n.type != TYPE_BOOL)
      error(msg);
  }

  // Generalized const fold: emit node into a temp arena, eval, discard.
  // Replaces the first child slot with the result, trims trailing dead nodes.
  // Invariant (asserted below): recursive descent guarantees the children we fold over
  // are the most recently appended IR nodes, so ir->resize(cs[0] + 1) is safe.
  int tryFold(uint64_t tag, uint8_t nc, int c0, int c1 = -1, int c2 = -1)
  {
    int cs[] = {c0, c1, c2};
    for (int i = 0; i < nc; i++)
      if (!isConst(cs[i]))
        return -1;
    const NodeEmitFn *fn = emitMap->findVal(tag);
    if (!fn)
      return -1;
    G_ASSERT(cs[nc - 1] == (int)ir->size() - 1);
    Arena tmp;
    uint32_t childOfs[3];
    for (int i = 0; i < nc; i++)
    {
      childOfs[i] = emit<ConstNode>(tmp);
      at<ConstNode>(tmp, childOfs[i])->value = constVal(cs[i]);
    }
    uint32_t root = (*fn)(tmp, childOfs);
    float result = lcexpr::eval(tmp, root, nullptr, 0);
    int idx = cs[0];
    (*ir)[idx].constVal = result;
    ir->resize(idx + 1);
    return idx;
  }

  TNode binOp(const char *tag, TNode l, TNode r, ExprType t)
  {
    int folded = tryFold(hash_name(tag), 2, l.idx, r.idx);
    if (folded >= 0)
      return {folded, t};
    return {addIR(hash_name(tag), 2, l.idx, r.idx), t};
  }

  // Top-level entry: parses one or more statements separated by `,` / `;`. The whole
  // sequence's value is the last statement's value; preceding statements run for their
  // side effects (typically var/let or assignment). parseSeq is also what `(...)` and
  // function-argument boundaries delegate to when sequences are syntactically allowed.
  //
  // Function arguments use parseAssign instead, because the comma there separates args.
  TNode parseSeq()
  {
    if (!enterDepth())
      return {0, TYPE_FLOAT};
    TNode n = parseStmt();
    while ((tok == T_COMMA || tok == T_SEMI) && !failed)
    {
      next();
      TNode r = parseStmt();
      // Const-fold (c1, c2) -> c2 when both sides are constants. Plain SeqNode emit
      // would do the same, but tryFold replaces the whole subtree with a single const,
      // saving an arena node and a virtual call at eval time.
      int folded = tryFold(TAG_SEQ, 2, n.idx, r.idx);
      if (folded >= 0)
        n = {folded, r.type};
      else
        n = {addIR(TAG_SEQ, 2, n.idx, r.idx), r.type};
    }
    leaveDepth();
    return n;
  }

  // Statement: either a `var`/`let` declaration, or an assignment expression. Both
  // produce a single IR node usable as a sequence element.
  TNode parseStmt()
  {
    if (failed)
      return {0, TYPE_FLOAT};
    if (tok == T_ID && (strcmp(tokId, "var") == 0 || strcmp(tokId, "let") == 0))
    {
      next();
      if (tok != T_ID)
      {
        error("expected identifier after 'var'/'let'");
        return {0, TYPE_FLOAT};
      }
      char name[64];
      int nameLen = (int)strlen(tokId);
      memcpy(name, tokId, nameLen + 1);
      next();
      if (!expect(T_ASSIGN))
        return {0, TYPE_FLOAT};
      // Reject names that collide with a builtin/user function or with an existing
      // variable (external or already-declared). A var/let must introduce a fresh name.
      uint64_t h = hash_name(name, nameLen);
      if (funcs->findVal(h))
      {
        char msg[96];
        snprintf(msg, sizeof(msg), "var name '%s' conflicts with a function", name);
        error(msg);
        return {0, TYPE_FLOAT};
      }
      if (varMap->findVal(h))
      {
        char msg[96];
        snprintf(msg, sizeof(msg), "var name '%s' is already defined", name);
        error(msg);
        return {0, TYPE_FLOAT};
      }
      // Reserve the new name BEFORE parsing the initialiser. Two reasons:
      //  1. A nested same-name `var a = (var a = 1, a)` is rejected by the existing
      //     "already defined" check on the inner var (without the reservation,
      //     register_var() would silently reuse the inner slot for the outer name).
      //  2. A self-reference `var a = a + 1` is caught at parsePrimary via inFlightVarId
      //     (a clearer error than letting `a` resolve to its own freshly-allocated slot
      //     and read undefined memory at eval time).
      uint16_t vid = register_var(*varMap, *nextVarId, name);
      if (vid == VAR_ID_OVERFLOW)
      {
        char msg[96];
        snprintf(msg, sizeof(msg), "too many variables (limit %d)", (int)MAX_VAR_ID);
        error(msg);
        return {0, TYPE_FLOAT};
      }
      const uint16_t prevInFlight = inFlightVarId;
      inFlightVarId = vid;
      TNode rhs = parseAssign();
      inFlightVarId = prevInFlight;
      if (failed)
        return {0, TYPE_FLOAT};
      // The fresh user-var slot starts undefined on the caller's side; we always emit an
      // AssignNode so the runtime store happens. tryFold would lose the side effect.
      int idx = (int)ir->size();
      IRNode irn = {};
      irn.tag = TAG_ASSIGN;
      irn.varId = vid;
      irn.nc = 1;
      irn.c[0] = rhs.idx;
      ir->push_back(irn);
      return {idx, rhs.type};
    }
    return parseAssign();
  }

  // Assignment: parseOr followed by an optional `= parseAssign`. Right-associative so
  // `a = b = c` chains as `a = (b = c)`. The LHS of `=` must be a single user-mutable
  // variable; anything else is a parse error.
  TNode parseAssign()
  {
    if (!enterDepth())
      return {0, TYPE_FLOAT};
    TNode lhs = parseOr();
    if (tok == T_ASSIGN && !failed)
    {
      // Validate LHS is a bare user-var node, then drop it and emit an assign in its
      // place. Recursive descent guarantees the var node is the most recent IR entry,
      // so a single pop_back is enough to remove it.
      if (lhs.idx < 0 || (*ir)[lhs.idx].tag != TAG_VAR)
      {
        error("assignment target must be a variable");
        leaveDepth();
        return {0, TYPE_FLOAT};
      }
      uint16_t vid = (*ir)[lhs.idx].varId;
      if (vid < externalVarBase)
      {
        error("cannot assign to external (read-only) variable");
        leaveDepth();
        return {0, TYPE_FLOAT};
      }
      G_ASSERT(lhs.idx == (int)ir->size() - 1);
      ir->pop_back();
      next();
      TNode rhs = parseAssign();
      int idx = (int)ir->size();
      IRNode irn = {};
      irn.tag = TAG_ASSIGN;
      irn.varId = vid;
      irn.nc = 1;
      irn.c[0] = rhs.idx;
      ir->push_back(irn);
      leaveDepth();
      return {idx, rhs.type};
    }
    leaveDepth();
    return lhs;
  }

  // Backwards-compatible name kept for legacy callers / inner expression contexts that
  // want "everything except sequence" -- function args, ternary slots, etc. parseSeq is
  // the new public entry.
  TNode parseExpr() { return parseAssign(); }

  TNode parseOr()
  {
    TNode n = parseAnd();
    while (tok == T_OR && !failed)
    {
      next();
      TNode r = parseAnd();
      requireBool(n, "|| requires bool");
      requireBool(r, "|| requires bool");
      n = binOp("or", n, r, TYPE_BOOL);
    }
    return n;
  }
  TNode parseAnd()
  {
    TNode n = parseCmp();
    while (tok == T_AND && !failed)
    {
      next();
      TNode r = parseCmp();
      requireBool(n, "&& requires bool");
      requireBool(r, "&& requires bool");
      n = binOp("and", n, r, TYPE_BOOL);
    }
    return n;
  }
  TNode parseCmp()
  {
    TNode n = parseAdd();
    switch (tok)
    {
      case T_LT: next(); return binOp("lt", n, parseAdd(), TYPE_BOOL);
      case T_GT: next(); return binOp("gt", n, parseAdd(), TYPE_BOOL);
      case T_LE: next(); return binOp("le", n, parseAdd(), TYPE_BOOL);
      case T_GE: next(); return binOp("ge", n, parseAdd(), TYPE_BOOL);
      case T_EQ: next(); return binOp("eq", n, parseAdd(), TYPE_BOOL);
      case T_NE: next(); return binOp("ne", n, parseAdd(), TYPE_BOOL);
      default: return n;
    }
  }
  TNode parseAdd()
  {
    TNode n = parseMul();
    while ((tok == T_PLUS || tok == T_MINUS) && !failed)
    {
      bool add = tok == T_PLUS;
      next();
      n = binOp(add ? "add" : "sub", n, parseMul(), TYPE_FLOAT);
    }
    return n;
  }
  TNode parseMul()
  {
    TNode n = parseUnary();
    while ((tok == T_STAR || tok == T_SLASH) && !failed)
    {
      bool mul = tok == T_STAR;
      next();
      n = binOp(mul ? "mul" : "div", n, parseUnary(), TYPE_FLOAT);
    }
    return n;
  }
  TNode parseUnary()
  {
    if (tok == T_MINUS && !failed)
    {
      next();
      if (!enterDepth())
        return {0, TYPE_FLOAT};
      TNode n = parseUnary();
      leaveDepth();
      if (failed)
        return {0, TYPE_FLOAT};
      int f = tryFold(hash_name("neg"), 1, n.idx);
      return f >= 0 ? TNode{f, TYPE_FLOAT} : TNode{addIR(hash_name("neg"), 1, n.idx), TYPE_FLOAT};
    }
    if (tok == T_NOT && !failed)
    {
      next();
      if (!enterDepth())
        return {0, TYPE_FLOAT};
      TNode n = parseUnary();
      leaveDepth();
      if (failed)
        return {0, TYPE_FLOAT};
      requireBool(n, "! requires bool");
      int f = tryFold(hash_name("not"), 1, n.idx);
      return f >= 0 ? TNode{f, TYPE_BOOL} : TNode{addIR(hash_name("not"), 1, n.idx), TYPE_BOOL};
    }
    return parsePrimary();
  }
  TNode parsePrimary()
  {
    if (failed)
      return {0, TYPE_FLOAT};
    if (tok == T_NUM)
    {
      int idx = addIR(hash_name("const"));
      (*ir)[idx].constVal = tokNum;
      next();
      return {idx, TYPE_FLOAT};
    }
    if (tok == T_ID)
    {
      char name[64];
      int nameLen = (int)strlen(tokId);
      memcpy(name, tokId, nameLen + 1);
      next();
      if (tok == T_LPAREN)
      {
        uint64_t fnHash = hash_name(name, nameLen);
        const FuncParseInfo *fi = funcs->findVal(fnHash);
        if (!fi)
        {
          char msg[96];
          snprintf(msg, sizeof(msg), "unknown function '%s'", name);
          error(msg);
          return {0, TYPE_FLOAT};
        }
        next();
        int args[3] = {-1, -1, -1};
        ExprType argTypes[3] = {};
        for (int i = 0; i < fi->nargs; i++)
        {
          if (i > 0 && !expect(T_COMMA))
            return {0, TYPE_FLOAT};
          TNode arg = parseExpr();
          args[i] = arg.idx;
          argTypes[i] = arg.type;
        }
        expect(T_RPAREN);
        if (fi->firstArgBool)
          requireBool({args[0], argTypes[0]}, "select() first argument must be bool");
        // C1: ramp/smooth_ramp with constant from==to is likely a mistake
        if ((fnHash == TAG_RAMP || fnHash == TAG_SRAMP) && fi->nargs == 3 && isConst(args[1]) && isConst(args[2]) &&
            fabsf(constVal(args[1]) - constVal(args[2])) < 1e-10f)
        {
          char msg[96];
          snprintf(msg, sizeof(msg), "%s: from == to (%.4g), degenerate ramp", name, constVal(args[1]));
          error(msg);
          return {0, TYPE_FLOAT};
        }
        int folded = tryFold(fnHash, fi->nargs, args[0], args[1], args[2]);
        if (folded >= 0)
          return {folded, TYPE_FLOAT};
        return {addIR(fnHash, fi->nargs, args[0], args[1], args[2]), TYPE_FLOAT};
      }
      // Lookup-only: an unknown identifier is an error. New names must be introduced
      // explicitly via `var <name> = ...` / `let <name> = ...` at statement level.
      uint64_t h = hash_name(name, nameLen);
      const uint16_t *vidP = varMap->findVal(h);
      if (!vidP)
      {
        char msg[96];
        snprintf(msg, sizeof(msg), "unknown identifier '%s'", name);
        error(msg);
        return {0, TYPE_FLOAT};
      }
      // Self-reference inside the var/let's own initialiser. The slot is reserved but
      // unwritten at this point; resolving the read would either alias an unrelated
      // value (uninitialised stack slot) or emit a load that the runtime traps on.
      if (*vidP == inFlightVarId)
      {
        char msg[96];
        snprintf(msg, sizeof(msg), "var '%s' cannot reference itself in its own initializer", name);
        error(msg);
        return {0, TYPE_FLOAT};
      }
      int idx = addIR(TAG_VAR);
      (*ir)[idx].varId = *vidP;
      return {idx, TYPE_FLOAT};
    }
    if (tok == T_LPAREN)
    {
      next();
      // Sequences are allowed inside parens, e.g. `(var t = fbm(x,y,4), t*t)`.
      TNode n = parseSeq();
      expect(T_RPAREN);
      return n;
    }
    error("expected expression");
    return {0, TYPE_FLOAT};
  }
};

// ============================================================================
// Compiler: IR -> Arena EvalNodes
// ============================================================================

LCEXPR_PARSER_INLINE uint32_t compileNode(const Tab<IRNode> &ir, int idx, Arena &arena, const NodeEmitMap &emitMap,
  Tab<uint32_t> &compiled)
{
  if (compiled[idx] != PARSE_ERROR)
    return compiled[idx];
  const IRNode &n = ir[idx];

  if (n.tag == TAG_CONST)
  {
    uint32_t o = emit<ConstNode>(arena);
    at<ConstNode>(arena, o)->value = n.constVal;
    return compiled[idx] = o;
  }
  if (n.tag == TAG_VAR)
  {
    uint32_t o = emit<VarNode>(arena);
    at<VarNode>(arena, o)->varId = n.varId;
    return compiled[idx] = o;
  }
  // Assign carries varId + a single child expression. The emit map cannot represent
  // this shape (varId is per-node state, not in args[]) so we hand-roll the emit here
  // -- analogous to RampVarConstNode below.
  if (n.tag == TAG_ASSIGN)
  {
    uint32_t childOfs = compileNode(ir, n.c[0], arena, emitMap, compiled);
    if (childOfs == PARSE_ERROR)
      return PARSE_ERROR;
    uint32_t o = emit<AssignNode>(arena);
    auto *p = at<AssignNode>(arena, o);
    p->n0 = childOfs;
    p->varId = n.varId;
    return compiled[idx] = o;
  }
  // Subtree fusion: ramp(var, const, const) / smooth_ramp(var, const, const)
  // Emit optimized node with varId + embedded constants, skip child compilation.
  if ((n.tag == TAG_RAMP || n.tag == TAG_SRAMP) && n.nc == 3 && ir[n.c[0]].tag == TAG_VAR && ir[n.c[1]].tag == TAG_CONST &&
      ir[n.c[2]].tag == TAG_CONST)
  {
    if (n.tag == TAG_RAMP)
    {
      uint32_t o = emit<RampVarConstNode>(arena);
      auto *p = at<RampVarConstNode>(arena, o);
      p->varId = ir[n.c[0]].varId;
      p->from = ir[n.c[1]].constVal;
      p->to = ir[n.c[2]].constVal;
      return compiled[idx] = o;
    }
    else
    {
      uint32_t o = emit<SmoothRampVarConstNode>(arena);
      auto *p = at<SmoothRampVarConstNode>(arena, o);
      p->varId = ir[n.c[0]].varId;
      p->from = ir[n.c[1]].constVal;
      p->to = ir[n.c[2]].constVal;
      return compiled[idx] = o;
    }
  }

  // compile children first; propagate PARSE_ERROR up
  uint32_t childOfs[3] = {0, 0, 0};
  for (int i = 0; i < n.nc; i++)
  {
    childOfs[i] = compileNode(ir, n.c[i], arena, emitMap, compiled);
    if (childOfs[i] == PARSE_ERROR)
      return PARSE_ERROR;
  }

  const NodeEmitFn *fn = emitMap.findVal(n.tag);
  if (!fn)
    return PARSE_ERROR; // unknown tag: caller must roll back the arena
  return compiled[idx] = (*fn)(arena, childOfs);
}

LCEXPR_PARSER_INLINE uint32_t compile(const Tab<IRNode> &ir, int irRoot, Arena &arena, const NodeEmitMap &emitMap)
{
  // Single var node -> DIRECT_VAR, no arena allocation
  if (ir[irRoot].tag == TAG_VAR)
    return DIRECT_VAR(ir[irRoot].varId);

  Tab<uint32_t> compiled;
  compiled.resize(ir.size());
  for (auto &c : compiled)
    c = PARSE_ERROR;
  return compileNode(ir, irRoot, arena, emitMap, compiled);
}

// ============================================================================
// Public API
// ============================================================================

LCEXPR_PARSER_INLINE int parseToIR(const char *text, Tab<IRNode> &ir, const FuncParseMap &funcs, const NodeEmitMap &emitMap,
  NameMap &varMap, uint16_t &nextVarId, eastl::string &error)
{
  int startSize = (int)ir.size();
  NameMap savedVarMap(varMap);
  uint16_t savedNextVarId = nextVarId;
  Parser p;
  p.src = text;
  p.pos = text;
  p.ir = &ir;
  p.funcs = &funcs;
  p.emitMap = &emitMap;
  p.varMap = &varMap;
  p.nextVarId = &nextVarId;
  p.err = &error;
  p.failed = false;
  p.depth = 0;
  // Anything already in varMap at parse start is external (read-only). User-declared
  // vars are assigned ids >= externalVarBase by register_var() during parseStmt.
  p.externalVarBase = nextVarId;
  p.inFlightVarId = lcexpr::VAR_ID_OVERFLOW;
  p.next();
  Parser::TNode result = p.parseSeq();
  if (!p.failed && p.tok != Parser::T_END)
    p.error("unexpected token after expression");
  if (p.failed)
  {
    ir.resize(startSize);
    varMap = savedVarMap;
    nextVarId = savedNextVarId;
    return -1;
  }
  return result.idx;
}

LCEXPR_PARSER_INLINE uint32_t parse(const char *text, Arena &arena, const FuncParseMap &parseMap, const NodeEmitMap &emitMap,
  NameMap &varMap, uint16_t &nextVarId, eastl::string &error, int *outMaxVarId)
{
  // Snapshot the name state so a post-parseToIR compile failure rolls back atomically.
  // parseToIR itself already restores varMap / nextVarId on its own failure, but once
  // parseToIR has succeeded any new variables introduced by the text must also be
  // reverted if compile() subsequently fails.
  uint32_t startOfs = (uint32_t)arena.size();
  NameMap savedVarMap(varMap);
  uint16_t savedNextVarId = nextVarId;
  Tab<IRNode> ir;
  int root = parseToIR(text, ir, parseMap, emitMap, varMap, nextVarId, error);
  if (root < 0)
  {
    arena.resize(startOfs);
    return PARSE_ERROR;
  }
  uint32_t result = compile(ir, root, arena, emitMap);
  if (result == PARSE_ERROR)
  {
    arena.resize(startOfs);
    varMap = savedVarMap;
    nextVarId = savedNextVarId;
    error = "compile failed: IR tag missing from emit map";
    return PARSE_ERROR;
  }
  if (outMaxVarId)
    *outMaxVarId = computeMaxVarId(ir, root);
  return result;
}

} // namespace lcexpr
