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
inline const uint64_t TAG_SELECT = hash_name("select"); // shared by select() and if-then-else
// Short-circuit boolean operators -- referenced by both the cprop and DCE passes for
// branch-merge handling (b may not run if a short-circuits).
inline const uint64_t TAG_AND = hash_name("and");
inline const uint64_t TAG_OR = hash_name("or");

// Reserved words. Must not be usable as variable names or auto-declared identifiers,
// since they alter the parse at well-known positions (var/let in parseStmt, if in
// parsePrimary, then/else as if-expression delimiters).
inline bool is_keyword(const char *s)
{
  return strcmp(s, "var") == 0 || strcmp(s, "let") == 0 || strcmp(s, "if") == 0 || strcmp(s, "then") == 0 || strcmp(s, "else") == 0;
}

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
// Parser -- produces IR
// ============================================================================
// (ExprType / TYPE_FLOAT / TYPE_BOOL / TYPE_USER_BASE live in lcExprParser.h.)

struct Parser
{
  static constexpr int MAX_DEPTH = 64; // max nested parens + unary minuses, bounds stack use

  const char *src, *pos;
  Tab<IRNode> *ir;
  const FuncParseMap *funcs;
  const NodeEmitMap *emitMap; // for const folding
  NameMap *varMap;
  uint16_t *nextVarId;
  // Optional per-varId type table (parallel to varMap). nullptr -> the untyped
  // path: var reads return TYPE_FLOAT and arg-type checks degrade accordingly.
  // Caller pre-populates for external vars; the parser writes user-var entries.
  VarTypeTable *varTypes;
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

  // ---- let / var mutability tracking (per-parse) ----
  // letFlags[vid - externalVarBase] = 1 iff the user-declared variable was introduced
  // by `let` (immutable). `var`-declared and external slots are 0. Indexed only for
  // user vars; external vars have a separate read-only check (vid < externalVarBase).
  // Lifetime: per-parse. Cross-parse we don't preserve let-ness -- semantics are
  // intentionally "let is constant inside this expression" only.
  uint8_t letFlags[MAX_VAR_ID];

  // ---- block scope tracking ({ ... }) ----
  // When parsePrimary enters `{`, push a frame with the current nextVarId and the
  // current size of blockNamesAdded. New var/let declarations inside the block push
  // their name-hashes onto blockNamesAdded so exitBlock can pop them out of varMap
  // when the block closes; nextVarId is then restored, freeing the temp slots.
  // Top-level (not-in-block) declarations do NOT touch blockNamesAdded -- their slots
  // are permanent for the parse.
  struct ScopeFrame
  {
    uint16_t savedNextVarId;
    int firstBlockNameIdx;
  };
  Tab<ScopeFrame> blockScopeStack;
  Tab<uint64_t> blockNamesAdded;

  // ---- temp register budget ----
  // maxTempRegs = caller-supplied CAP on simultaneously-live block-scoped vars
  // (<= MAX_TEMP_REGS). peakTempRegs tracks the high-water mark of in-block depth,
  // measured against the OUTERMOST block's pre-entry baseline (blockScopeStack[0].
  // savedNextVarId). Top-level decls outside any block do NOT contribute -- otherwise
  // a trailing top-level var/let that reuses a slot freed by exitBlock would shrink
  // the reported peak (since finalNextVarId would absorb the slot reuse) and silently
  // bypass the cap check. Validated against maxTempRegs after parseSeq returns.
  uint8_t maxTempRegs;
  int peakTempRegs;

  bool inBlock() const { return blockScopeStack.size() > 0; }

  void enterBlock()
  {
    ScopeFrame f;
    f.savedNextVarId = *nextVarId;
    f.firstBlockNameIdx = (int)blockNamesAdded.size();
    blockScopeStack.push_back(f);
  }

  // Symmetric counterpart to enterBlock. Removes block-scoped names from varMap and
  // restores nextVarId so the freed slots can be reused by following declarations.
  // Safe to call on the unwind path (parser failure inside a block); the outer
  // parseToIR also restores varMap from a snapshot on failure, so the cleanup here
  // is mainly to keep success-path state consistent.
  void exitBlock()
  {
    if (blockScopeStack.size() == 0)
      return;
    ScopeFrame f = blockScopeStack.back();
    blockScopeStack.pop_back();
    for (int i = f.firstBlockNameIdx; i < (int)blockNamesAdded.size(); i++)
      varMap->erase(blockNamesAdded[i]);
    blockNamesAdded.resize(f.firstBlockNameIdx);
    *nextVarId = f.savedNextVarId;
  }

  // Called immediately after register_var() succeeds in parseStmt. Records let-ness
  // for the new slot, updates the in-block peak-temp counter, and (when inside a
  // block) registers the name-hash for exitBlock cleanup.
  //
  // Peak accounting: when inside a block, the simultaneously-live block-scoped var
  // count is `*nextVarId - blockScopeStack[0].savedNextVarId`. Using the OUTERMOST
  // block's baseline (not the innermost frame and not finalNextVarId) means nested
  // block decls accumulate correctly AND a trailing top-level decl that reuses a
  // freed slot does not retroactively shrink the peak.
  void recordVarDecl(uint16_t vid, bool isLet, uint64_t nameHash)
  {
    if (vid >= externalVarBase && (vid - externalVarBase) < MAX_VAR_ID)
      letFlags[vid - externalVarBase] = isLet ? 1 : 0;
    if (inBlock())
    {
      int blockDepth = (int)*nextVarId - (int)blockScopeStack[0].savedNextVarId;
      if (blockDepth > peakTempRegs)
        peakTempRegs = blockDepth;
      blockNamesAdded.push_back(nameHash);
    }
  }

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
    T_LBRACE,
    T_RBRACE,
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
      case '{': tok = T_LBRACE; return;
      case '}': tok = T_RBRACE; return;
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
  // Bool widens to Float (runtime is identical 0.f/1.f); user types do not. Use
  // this at arithmetic / comparison / unary-neg call sites so a CURVE-typed value
  // cannot silently flow through `+`, `<`, etc. and lose its type tag.
  void requireFloat(TNode n, const char *msg)
  {
    if (!type_assignable_to(n.type, TYPE_FLOAT))
      error(msg);
  }
  // Resolves the result type of a two-branch conditional. Same type passes through
  // (covers user types like CURVE); a bool/float mix widens to float (runtime is
  // identical 0.f/1.f); anything else is a type error. Shared by the if-then-else
  // syntax and the select() function-call form so the two stay equivalent (they
  // also share TAG_SELECT in the IR).
  ExprType combineBranchTypes(ExprType a, ExprType b, const char *errMsg)
  {
    if (a == b)
      return a;
    if (type_assignable_to(a, TYPE_FLOAT) && type_assignable_to(b, TYPE_FLOAT))
      return TYPE_FLOAT;
    error(errMsg);
    return TYPE_FLOAT;
  }

  // Generalized const fold: emit node into a temp arena, eval, discard.
  // Replaces the first child slot with the result, trims trailing dead nodes.
  // Invariant (asserted below): recursive descent guarantees the children we fold over
  // are the most recently appended IR nodes, so ir->resize(cs[0] + 1) is safe.
  int tryFold(uint64_t tag, uint8_t nc, int c0, int c1 = -1, int c2 = -1)
  {
    // On the error-unwind path sub-parses return stale {0, FLOAT} dummies, so the
    // invariant asserted below (children are the most recently appended IR nodes)
    // no longer holds. Skip folding; the addIR fallback at the call sites is
    // harmless since parseToIR discards the IR on failure.
    if (failed)
      return -1;
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
      const bool isLet = (tokId[0] == 'l');
      const char *kw = isLet ? "let" : "var";
      next();
      if (tok != T_ID)
      {
        error("expected identifier after 'var'/'let'");
        return {0, TYPE_FLOAT};
      }
      // Reserved keywords cannot be used as variable names. Check before any name
      // hashing / registration so the error message is unambiguous (rather than
      // letting `var if = 1` register a slot named "if" and then break parsePrimary's
      // if-detection later).
      if (is_keyword(tokId))
      {
        char msg[96];
        snprintf(msg, sizeof(msg), "'%s' is a reserved keyword and cannot be used as a variable name", tokId);
        error(msg);
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
        snprintf(msg, sizeof(msg), "%s name '%s' conflicts with a function", kw, name);
        error(msg);
        return {0, TYPE_FLOAT};
      }
      if (varMap->findVal(h))
      {
        char msg[96];
        snprintf(msg, sizeof(msg), "%s name '%s' is already defined", kw, name);
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
      // Record let-ness, peak-slot, and (if inside a `{...}` block) push the name
      // onto blockNamesAdded so exitBlock removes it from varMap when the block closes.
      recordVarDecl(vid, isLet, h);
      const uint16_t prevInFlight = inFlightVarId;
      inFlightVarId = vid;
      TNode rhs = parseAssign();
      inFlightVarId = prevInFlight;
      if (failed)
        return {0, TYPE_FLOAT};
      // Inherit the new var's type from its initialiser. `var`/`let` is the only way
      // a user expression can declare a non-float typed binding (e.g. aliasing a
      // CURVE-typed external var via `let myc = slopeCurve`). Untyped path
      // (varTypes==nullptr) skips this and the slot stays implicitly FLOAT.
      if (varTypes)
      {
        if ((int)vid >= varTypes->size())
        {
          int oldSize = varTypes->size();
          varTypes->resize(vid + 1);
          for (int i = oldSize; i < (int)varTypes->size(); i++)
            (*varTypes)[i] = TYPE_FLOAT;
        }
        (*varTypes)[vid] = rhs.type;
      }
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
      // let-declared user vars are immutable for the remainder of this parse. (External
      // vars hit the previous branch; only user-declared slots reach this check.)
      if ((vid - externalVarBase) < MAX_VAR_ID && letFlags[vid - externalVarBase])
      {
        error("cannot assign to a 'let'-declared (immutable) variable");
        leaveDepth();
        return {0, TYPE_FLOAT};
      }
      G_ASSERT(lhs.idx == (int)ir->size() - 1);
      ir->pop_back();
      next();
      TNode rhs = parseAssign();
      // Type-check: rhs must be assignable to the LHS var's type. This is the only
      // place a typed user var slot can change value, so a bad assignment here would
      // otherwise silently mix types (e.g. writing a Float into a Curve slot, after
      // which curve(c, x) reads a meaningless slot index).
      ExprType lhsType = var_type_or_float(varTypes, vid);
      if (!type_assignable_to(rhs.type, lhsType))
      {
        char msg[128];
        snprintf(msg, sizeof(msg), "assignment type mismatch: expected type %u, got %u", (unsigned)lhsType, (unsigned)rhs.type);
        error(msg);
        leaveDepth();
        return {0, TYPE_FLOAT};
      }
      int idx = (int)ir->size();
      IRNode irn = {};
      irn.tag = TAG_ASSIGN;
      irn.varId = vid;
      irn.nc = 1;
      irn.c[0] = rhs.idx;
      ir->push_back(irn);
      leaveDepth();
      // The assignment expression's value carries the LHS type, not rhs.type, so a
      // bool-to-float coercion is observable downstream as a float (matching the
      // runtime, which has already widened the bool's 0.f/1.f into the float slot).
      return {idx, lhsType};
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
    const char *tag = nullptr;
    const char *errMsg = nullptr;
    switch (tok)
    {
      case T_LT:
        tag = "lt";
        errMsg = "< requires float";
        break;
      case T_GT:
        tag = "gt";
        errMsg = "> requires float";
        break;
      case T_LE:
        tag = "le";
        errMsg = "<= requires float";
        break;
      case T_GE:
        tag = "ge";
        errMsg = ">= requires float";
        break;
      case T_EQ:
        tag = "eq";
        errMsg = "== requires float";
        break;
      case T_NE:
        tag = "ne";
        errMsg = "!= requires float";
        break;
      default: return n;
    }
    next();
    TNode r = parseAdd();
    requireFloat(n, errMsg);
    requireFloat(r, errMsg);
    return binOp(tag, n, r, TYPE_BOOL);
  }
  TNode parseAdd()
  {
    TNode n = parseMul();
    while ((tok == T_PLUS || tok == T_MINUS) && !failed)
    {
      bool add = tok == T_PLUS;
      const char *errMsg = add ? "+ requires float" : "- requires float";
      next();
      TNode r = parseMul();
      requireFloat(n, errMsg);
      requireFloat(r, errMsg);
      n = binOp(add ? "add" : "sub", n, r, TYPE_FLOAT);
    }
    return n;
  }
  TNode parseMul()
  {
    TNode n = parseUnary();
    while ((tok == T_STAR || tok == T_SLASH) && !failed)
    {
      bool mul = tok == T_STAR;
      const char *errMsg = mul ? "* requires float" : "/ requires float";
      next();
      TNode r = parseUnary();
      requireFloat(n, errMsg);
      requireFloat(r, errMsg);
      n = binOp(mul ? "mul" : "div", n, r, TYPE_FLOAT);
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
      requireFloat(n, "unary - requires float");
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
    if (tok == T_ID && strcmp(tokId, "if") == 0)
    {
      // if-then-else: same IR shape as select(cond, then, else) -- shares emit/fold paths
      // and the bool-cond requirement. then/else branches are full assignment-level
      // expressions, so binary operators bind to the nearest branch:
      //   `if c then a + 1 else b` parses as `if c then (a + 1) else b`.
      next();
      TNode cond = parseAssign();
      if (failed)
        return {0, TYPE_FLOAT};
      if (tok != T_ID || strcmp(tokId, "then") != 0)
      {
        error("expected 'then' in if-expression");
        return {0, TYPE_FLOAT};
      }
      next();
      TNode thenE = parseAssign();
      if (failed)
        return {0, TYPE_FLOAT};
      if (tok != T_ID || strcmp(tokId, "else") != 0)
      {
        error("expected 'else' in if-expression");
        return {0, TYPE_FLOAT};
      }
      next();
      TNode elseE = parseAssign();
      if (failed)
        return {0, TYPE_FLOAT};
      requireBool(cond, "if-expression condition must be bool");
      ExprType branchType = combineBranchTypes(thenE.type, elseE.type, "if-expression branches must have matching type");
      int folded = tryFold(TAG_SELECT, 3, cond.idx, thenE.idx, elseE.idx);
      if (folded >= 0)
        return {folded, branchType};
      return {addIR(TAG_SELECT, 3, cond.idx, thenE.idx, elseE.idx), branchType};
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
        // Bail before the arg-type checks / degenerate-ramp check / fold: after an
        // arg parse error args[] holds stale dummy indices (possibly into an empty
        // IR, e.g. `ramp(,,)`), and the (*ir)[args[i]] reads below would be OOB.
        if (failed)
          return {0, TYPE_FLOAT};
        // select() is polymorphic in its branches: cond is bool, but args 1 and 2
        // are checked against each other instead of the registered FLOAT signature
        // so it stays equivalent to the if-then-else syntax (both share TAG_SELECT).
        // Skip the registered argTypes[1]/argTypes[2] check here; the combine call
        // below resolves the result type and emits an error on mismatch.
        const bool isSelect = (fnHash == TAG_SELECT);
        // Per-arg type check against the FuncParseInfo declared types. Float-where-
        // Bool is rejected; Bool-where-Float is accepted (implicit widening, see
        // type_assignable_to). User types must match exactly.
        for (int i = 0; i < fi->nargs; i++)
        {
          if (isSelect && i >= 1)
            continue;
          if (!type_assignable_to(argTypes[i], fi->argTypes[i]))
          {
            char msg[160];
            const char *expected = (fi->argTypes[i] == TYPE_BOOL) ? "bool" : ((fi->argTypes[i] == TYPE_FLOAT) ? "float" : nullptr);
            const char *got = (argTypes[i] == TYPE_BOOL) ? "bool" : ((argTypes[i] == TYPE_FLOAT) ? "float" : nullptr);
            // Keep the legacy "must be bool"/"must be float" wording for the FLOAT/BOOL
            // pair so existing tests and editor diagnostics stay stable. User types
            // fall through to the numeric form, which still pinpoints the offending
            // argument.
            if (expected && got)
              snprintf(msg, sizeof(msg), "%s(): arg %d must be %s, got %s", name, i + 1, expected, got);
            else
              snprintf(msg, sizeof(msg), "%s(): arg %d type mismatch (expected type %u, got %u)", name, i + 1,
                (unsigned)fi->argTypes[i], (unsigned)argTypes[i]);
            error(msg);
            return {0, TYPE_FLOAT};
          }
        }
        ExprType resultType = fi->resultType;
        if (isSelect && fi->nargs == 3)
          resultType = combineBranchTypes(argTypes[1], argTypes[2], "select(): branches must have matching type");
        // C1: ramp/smooth_ramp degenerate-from==to diagnostic. Two literal forms
        // are caught at parse time:
        //   - both args are equal-valued constants: `ramp(h, 10, 10)`
        //   - both args reference the same var slot (external or user-declared):
        //     `ramp(h, x, x)`. (cond - from) / (to - from) is 0/0 = NaN by
        //     design, no matter where x's value comes from. The user-declared
        //     case also closes a cprop bypass: without this guard, cprop
        //     replaces the two VAR(x) children with CONST(v), and the
        //     compile-time RampVarConstNode fusion (compileNode below) emits
        //     a degenerate ramp without re-running this check.
        if ((fnHash == TAG_RAMP || fnHash == TAG_SRAMP) && fi->nargs == 3 && args[1] >= 0 && args[2] >= 0)
        {
          bool sameConst = isConst(args[1]) && isConst(args[2]) && fabsf(constVal(args[1]) - constVal(args[2])) < 1e-10f;
          bool sameVar =
            (*ir)[args[1]].tag == TAG_VAR && (*ir)[args[2]].tag == TAG_VAR && (*ir)[args[1]].varId == (*ir)[args[2]].varId;
          if (sameConst || sameVar)
          {
            char msg[96];
            if (sameConst)
              snprintf(msg, sizeof(msg), "%s: from == to (%.4g), degenerate ramp", name, constVal(args[1]));
            else
              snprintf(msg, sizeof(msg), "%s: from and to are the same var, degenerate ramp", name);
            error(msg);
            return {0, TYPE_FLOAT};
          }
        }
        int folded = tryFold(fnHash, fi->nargs, args[0], args[1], args[2]);
        if (folded >= 0)
          return {folded, resultType};
        return {addIR(fnHash, fi->nargs, args[0], args[1], args[2]), resultType};
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
      return {idx, var_type_or_float(varTypes, *vidP)};
    }
    if (tok == T_LPAREN)
    {
      next();
      // Sequences are allowed inside parens, e.g. `(var t = fbm(x,y,4), t*t)`.
      TNode n = parseSeq();
      expect(T_RPAREN);
      return n;
    }
    if (tok == T_LBRACE)
    {
      // Scoped block: like (...) but var/let inside `{...}` are visible only within
      // the block. The block's value is the value of its last statement (the inner
      // parseSeq's result). Empty `{}` is a parse error -- the spec requires the block
      // to have at least one statement so the "value of last expression" is defined.
      next();
      if (tok == T_RBRACE)
      {
        error("empty '{}' block is not allowed");
        return {0, TYPE_FLOAT};
      }
      enterBlock();
      TNode n = parseSeq();
      // exitBlock unconditionally restores nextVarId / pops names so a parse failure
      // mid-block does not leave stale state behind. parseToIR's outer rollback also
      // restores varMap on failure, so a missed exitBlock here would not corrupt the
      // caller -- but consistent cleanup keeps the success path predictable.
      if (!expect(T_RBRACE))
      {
        exitBlock();
        return {0, TYPE_FLOAT};
      }
      exitBlock();
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
// Constant propagation
// ============================================================================
// Forward dataflow over the IR in execution order. For each user-var slot,
// tracks whether its current value is a known compile-time constant. A
// VarNode read of a known-const slot is rewritten in place to a ConstNode
// with that value; an op whose children become all-const is then folded by
// re-emit-and-eval into a single ConstNode (mirrors the parser's tryFold).
//
// Branch handling:
//   SeqNode(a, b)         -- walk a, then b. Linear flow.
//   SelectNode(c, t, e)   -- walk c, snapshot, walk t, save, restore, walk e,
//                            merge afterThen into afterElse. Slots that hold
//                            the same const in both branches survive; slots
//                            that disagree (or were unset in one branch) drop
//                            to unknown.
//   And/OrNode(a, b)      -- walk a, snapshot, walk b. Merge snapshot into
//                            post-b state, since b may not run (short-circuit).
//
// AssignNode(vid, n0): walk n0; if it folds to a ConstNode, set state[vid] to
// that value; otherwise mark state[vid] unknown. The AssignNode itself is
// preserved (its side effect is what subsequent reads -- and the COMMON-mode
// liveness root -- depend on; DCE removes it later when the write is dead).
//
// External slots (vid < externalVarBase) are runtime input and never tracked.
// VarNode reads of externals stay as VarNodes; this preserves the
// RampVarConstNode / SmoothRampVarConstNode fusion in compileNode.

struct LcCpropMap
{
  // Indexed by `vid - externalVarBase`. flags[i]=1 means user var i has a
  // known const value in vals[i]. External slots (vid < externalVarBase) are
  // never written here; tryGet returns false for them so external reads stay
  // as VarNode and the ramp/smooth_ramp + var fusion in compileNode applies.
  uint8_t flags[MAX_VAR_ID];
  float vals[MAX_VAR_ID];

  void clear()
  {
    for (int i = 0; i < (int)MAX_VAR_ID; i++)
      flags[i] = 0;
  }
  bool tryGet(uint16_t vid, uint16_t externalBase, float &out) const
  {
    if (vid < externalBase)
      return false;
    int idx = (int)vid - (int)externalBase;
    if (idx >= (int)MAX_VAR_ID)
      return false;
    if (!flags[idx])
      return false;
    out = vals[idx];
    return true;
  }
  void setConst(uint16_t vid, float v, uint16_t externalBase)
  {
    if (vid < externalBase)
      return;
    int idx = (int)vid - (int)externalBase;
    if (idx >= (int)MAX_VAR_ID)
      return;
    flags[idx] = 1;
    vals[idx] = v;
  }
  void setUnknown(uint16_t vid, uint16_t externalBase)
  {
    if (vid < externalBase)
      return;
    int idx = (int)vid - (int)externalBase;
    if (idx >= (int)MAX_VAR_ID)
      return;
    flags[idx] = 0;
  }
  // Intersect: keep only slots that hold the same const value in both `*this`
  // and `other`. Used at SelectNode merge points and at And/Or to fold
  // "branch may not have run" into the conservative post-state.
  // NaN equality intentionally falls through to drop -- two branches storing
  // NaN produce a slot we can't reason about (NaN != NaN), which matches the
  // "values disagree -> unknown" rule.
  void mergeFrom(const LcCpropMap &other)
  {
    for (int i = 0; i < (int)MAX_VAR_ID; i++)
    {
      if (!flags[i])
        continue;
      if (!other.flags[i] || other.vals[i] != vals[i])
        flags[i] = 0;
    }
  }
};

// All-const-children fold via emit-and-eval, mirroring the parser's tryFold
// without the "trim the IR Tab tail" step (cprop runs after parsing -- nodes
// upstream may reference these indices). The unreachable children are simply
// left in the IR; compile() walks from the root and ignores them.
//
// Skip tags whose semantics or shape forbid generic fold:
//   TAG_CONST -- already const
//   TAG_VAR   -- handled by the caller (VarNode -> ConstNode replacement)
//   TAG_ASSIGN -- has a runtime side effect; folding would lose the write
inline void lc_cprop_fold(Tab<IRNode> &ir, int idx, const NodeEmitMap &emitMap)
{
  if (idx < 0 || idx >= (int)ir.size())
    return;
  IRNode &n = ir[idx];
  if (n.tag == TAG_CONST || n.tag == TAG_VAR || n.tag == TAG_ASSIGN)
    return;
  if (n.nc == 0)
    return;
  for (int i = 0; i < n.nc; i++)
    if (ir[n.c[i]].tag != TAG_CONST)
      return;
  const NodeEmitFn *fn = emitMap.findVal(n.tag);
  if (!fn)
    return;
  Arena tmp;
  uint32_t childOfs[3] = {0, 0, 0};
  for (int i = 0; i < n.nc; i++)
  {
    childOfs[i] = emit<ConstNode>(tmp);
    at<ConstNode>(tmp, childOfs[i])->value = ir[n.c[i]].constVal;
  }
  uint32_t root = (*fn)(tmp, childOfs);
  float result = lcexpr::eval(tmp, root, nullptr, 0);
  n.tag = TAG_CONST;
  n.constVal = result;
  n.nc = 0;
  n.c[0] = n.c[1] = n.c[2] = -1;
}

inline void lc_cprop_node(Tab<IRNode> &ir, int idx, LcCpropMap &state, uint16_t externalBase, const NodeEmitMap &emitMap)
{
  if (idx < 0 || idx >= (int)ir.size())
    return;
  IRNode &n = ir[idx];
  if (n.tag == TAG_CONST)
    return;
  if (n.tag == TAG_VAR)
  {
    float v;
    if (state.tryGet(n.varId, externalBase, v))
    {
      n.tag = TAG_CONST;
      n.constVal = v;
      n.nc = 0;
      n.c[0] = n.c[1] = n.c[2] = -1;
    }
    return;
  }
  if (n.tag == TAG_ASSIGN)
  {
    lc_cprop_node(ir, n.c[0], state, externalBase, emitMap);
    if (ir[n.c[0]].tag == TAG_CONST)
      state.setConst(n.varId, ir[n.c[0]].constVal, externalBase);
    else
      state.setUnknown(n.varId, externalBase);
    return;
  }
  if (n.tag == TAG_SEQ)
  {
    lc_cprop_node(ir, n.c[0], state, externalBase, emitMap);
    lc_cprop_node(ir, n.c[1], state, externalBase, emitMap);
    lc_cprop_fold(ir, idx, emitMap);
    return;
  }
  if (n.tag == TAG_SELECT)
  {
    // Cond always runs before either branch; then/else are mutually exclusive.
    lc_cprop_node(ir, n.c[0], state, externalBase, emitMap);
    LcCpropMap snapshot = state;
    lc_cprop_node(ir, n.c[1], state, externalBase, emitMap);
    LcCpropMap afterThen = state;
    state = snapshot;
    lc_cprop_node(ir, n.c[2], state, externalBase, emitMap);
    state.mergeFrom(afterThen);
    lc_cprop_fold(ir, idx, emitMap);
    return;
  }
  if (n.tag == TAG_AND || n.tag == TAG_OR)
  {
    // a always runs; b runs only if a didn't short-circuit. Snapshot after a
    // captures the "b skipped" state; merge into post-b state to be conservative.
    lc_cprop_node(ir, n.c[0], state, externalBase, emitMap);
    LcCpropMap snapshot = state;
    lc_cprop_node(ir, n.c[1], state, externalBase, emitMap);
    state.mergeFrom(snapshot);
    lc_cprop_fold(ir, idx, emitMap);
    return;
  }
  // Generic operator: every child evaluates left-to-right unconditionally.
  for (int i = 0; i < n.nc; i++)
    lc_cprop_node(ir, n.c[i], state, externalBase, emitMap);
  lc_cprop_fold(ir, idx, emitMap);
}

// Top-level constant propagation entry. Mutates IR in place; the root index
// is unchanged because the root node is mutated (not replaced). Subsequent
// DCE will further trim writes whose only readers became consts.
LCEXPR_PARSER_INLINE int cpropIR(Tab<IRNode> &ir, int root, uint16_t externalBase, const NodeEmitMap &emitMap)
{
  if (root < 0 || ir.size() == 0)
    return root;
  LcCpropMap state;
  state.clear();
  lc_cprop_node(ir, root, state, externalBase, emitMap);
  return root;
}

// Post-cprop validation: detect any ramp/smooth_ramp whose from/to children
// resolve to the same scalar after constant propagation. Two shapes:
//   - both children TAG_CONST with equal value:
//     `var x = 10, var y = 10, ramp(h, x, y)` collapses to ramp(VAR(h),
//     CONST(10), CONST(10)); cprop_fold leaves the ramp itself alone (cond
//     is non-const), and the compileNode RampVarConstNode fusion would emit
//     a degenerate ramp.
//   - both children TAG_VAR with the same varId, where cprop did not fold
//     the var to a const (external var, or unknown user slot). Same shape
//     reaches RampVarConstNode fusion or generic eval; both produce
//     (cond - from) / (to - from) = 0/0 = NaN.
// The IR from root is tree-shaped (cprop mutates nodes in place; it does not
// introduce shared refs), so a plain pre-order walk without visited tracking
// is correct and avoids an extra Tab allocation.
inline bool lc_find_degenerate_ramp(const Tab<IRNode> &ir, int idx, const char *&outFnName, const char *&outKind)
{
  if (idx < 0 || idx >= (int)ir.size())
    return false;
  const IRNode &n = ir[idx];
  if ((n.tag == TAG_RAMP || n.tag == TAG_SRAMP) && n.nc == 3)
  {
    int a = n.c[1], b = n.c[2];
    if (a >= 0 && b >= 0)
    {
      if (ir[a].tag == TAG_CONST && ir[b].tag == TAG_CONST && fabsf(ir[a].constVal - ir[b].constVal) < 1e-10f)
      {
        outFnName = (n.tag == TAG_RAMP) ? "ramp" : "smooth_ramp";
        outKind = "from == to after constant propagation";
        return true;
      }
      if (ir[a].tag == TAG_VAR && ir[b].tag == TAG_VAR && ir[a].varId == ir[b].varId)
      {
        outFnName = (n.tag == TAG_RAMP) ? "ramp" : "smooth_ramp";
        outKind = "from and to are the same var";
        return true;
      }
    }
  }
  for (int i = 0; i < n.nc; i++)
    if (lc_find_degenerate_ramp(ir, n.c[i], outFnName, outKind))
      return true;
  return false;
}

// ============================================================================
// Dead code elimination
// ============================================================================
// Two transforms, applied in a single backward liveness pass over the IR tree:
//
//   (1) Pure-discard in SeqNode: SeqNode(a, b) where a's subtree contains no
//       AssignNode is replaced by b. The expression's only side effect is the
//       AssignNode write -- everything else is pure.
//
//   (2) Dead-store elimination: AssignNode(vid, n0) whose `vid` is not in the
//       live-out set is replaced by its rvalue child n0. The write is gone but
//       n0 still evaluates (preserving any nested side effects) and yields the
//       same scalar value the assign would have returned.
//
// Liveness at root is controlled by `treatTopLevelVarsAsExported`:
//   true  -- top-level user vars [externalVarBase, finalNextVarId) are live at
//            root, so writes to them are preserved (the daEditor's COMMON
//            expression contract: layer parses read these slots later).
//   false -- top-level user vars are local; their last writes are eliminated
//            unless an in-IR read keeps them live (the layer-parse case).
// Block-scoped temp slots (vid >= finalNextVarId) are NEVER live at root --
// they go out of scope at exitBlock during parse and no IR can reach them
// from outside their lexical block, so any unread write is safe to drop.

// 256-bit live-var set. MAX_VAR_ID = 256 covers all reachable vid values
// (register_var refuses to allocate at MAX_VAR_ID), so 4 uint64 words is exact.
struct LcDceLiveSet
{
  static constexpr int NWORDS = (MAX_VAR_ID + 63) / 64;
  uint64_t bits[NWORDS];

  void clear()
  {
    for (int i = 0; i < NWORDS; i++)
      bits[i] = 0;
  }
  bool test(uint16_t vid) const
  {
    if (vid >= MAX_VAR_ID)
      return false;
    return (bits[vid >> 6] & (uint64_t(1) << (vid & 63))) != 0;
  }
  void set(uint16_t vid)
  {
    if (vid < MAX_VAR_ID)
      bits[vid >> 6] |= uint64_t(1) << (vid & 63);
  }
  void unset(uint16_t vid)
  {
    if (vid < MAX_VAR_ID)
      bits[vid >> 6] &= ~(uint64_t(1) << (vid & 63));
  }
  void unionWith(const LcDceLiveSet &o)
  {
    for (int i = 0; i < NWORDS; i++)
      bits[i] |= o.bits[i];
  }
};

// True if the IR subtree rooted at `idx` contains any AssignNode. Uses an
// inline cache keyed by node index so a SeqNode chain doesn't re-scan the same
// subtrees. The cache uses three states: 0=unknown, 1=pure, 2=has-effect.
inline bool lc_dce_has_side_effect(const Tab<IRNode> &ir, int idx, Tab<uint8_t> &cache)
{
  if (idx < 0)
    return false;
  if (cache[idx] != 0)
    return cache[idx] == 2;
  const IRNode &n = ir[idx];
  bool eff = (n.tag == TAG_ASSIGN);
  for (int i = 0; i < n.nc && !eff; i++)
    eff = lc_dce_has_side_effect(ir, n.c[i], cache);
  cache[idx] = eff ? 2 : 1;
  return eff;
}

// Recursive backward dataflow. Returns the (possibly substituted) child index
// the parent should now point to, and writes the live-in set into `liveBefore`.
//
// Substitutions:
//   AssignNode(vid, n0) with vid not live  -> child becomes n0 directly.
//   SeqNode(a, b) with a pure              -> child becomes the (rewritten) b.
// When neither applies the original index is returned with its child links
// patched to the (possibly rewritten) sub-results.
inline int lc_dce_node(Tab<IRNode> &ir, int idx, const LcDceLiveSet &liveAfter, LcDceLiveSet &liveBefore, Tab<uint8_t> &pureCache)
{
  if (idx < 0)
  {
    liveBefore = liveAfter;
    return idx;
  }
  IRNode &n = ir[idx];

  if (n.tag == TAG_CONST)
  {
    liveBefore = liveAfter;
    return idx;
  }
  if (n.tag == TAG_VAR)
  {
    liveBefore = liveAfter;
    liveBefore.set(n.varId);
    return idx;
  }
  if (n.tag == TAG_ASSIGN)
  {
    if (!liveAfter.test(n.varId))
    {
      // Dead store: the write target isn't read anywhere downstream. Drop the
      // assign and let the rvalue stand on its own (it still yields the same
      // value the assign would have returned, and preserves any nested writes).
      return lc_dce_node(ir, n.c[0], liveAfter, liveBefore, pureCache);
    }
    // Live: vid is killed by THIS write before any subsequent read sees the
    // pre-assign value, so during n0's evaluation the OLD vid is dead unless
    // n0 itself reads it. Compute liveBefore by passing (liveAfter - {vid})
    // through n0; n0 will re-add vid if it reads it.
    LcDceLiveSet duringN0 = liveAfter;
    duringN0.unset(n.varId);
    int newC0 = lc_dce_node(ir, n.c[0], duringN0, liveBefore, pureCache);
    n.c[0] = newC0;
    return idx;
  }
  if (n.tag == TAG_SEQ)
  {
    // Eval order: a, then b. b's value is the SeqNode's value. We must process
    // BOTH children before deciding to discard a -- dead-store elimination
    // inside a's subtree may turn a previously-side-effecting Assign into a
    // pure rvalue, enabling the discard. Checking purity of the ORIGINAL c[0]
    // before recursion would miss those cascades.
    LcDceLiveSet liveBeforeB;
    int newB = lc_dce_node(ir, n.c[1], liveAfter, liveBeforeB, pureCache);
    int newA = lc_dce_node(ir, n.c[0], liveBeforeB, liveBefore, pureCache);
    if (!lc_dce_has_side_effect(ir, newA, pureCache))
    {
      // a is pure -- its evaluation has no observable effect, so it can go.
      // liveBefore was computed assuming a runs; if a is pure, its only
      // contribution to liveBefore was the vars it READ. Those reads no
      // longer happen, so the true liveBefore is liveBeforeB. (For the
      // outer cap/sizing we only care about reads of EXTERNAL slots, which
      // a pure subtree of a SeqNode has no semantic dependency on.)
      liveBefore = liveBeforeB;
      return newB;
    }
    n.c[0] = newA;
    n.c[1] = newB;
    return idx;
  }
  if (n.tag == TAG_SELECT)
  {
    // SelectNode is short-circuit: evaluates cond, then EITHER then OR else.
    // Live-after for both branches is the parent's liveAfter; liveAfter(cond)
    // is the union (cond doesn't know which branch will run).
    LcDceLiveSet liveBeforeT, liveBeforeE;
    int newT = lc_dce_node(ir, n.c[1], liveAfter, liveBeforeT, pureCache);
    int newE = lc_dce_node(ir, n.c[2], liveAfter, liveBeforeE, pureCache);
    LcDceLiveSet liveAfterC = liveBeforeT;
    liveAfterC.unionWith(liveBeforeE);
    int newC = lc_dce_node(ir, n.c[0], liveAfterC, liveBefore, pureCache);
    n.c[0] = newC;
    n.c[1] = newT;
    n.c[2] = newE;
    return idx;
  }
  if (n.tag == TAG_AND || n.tag == TAG_OR)
  {
    // Short-circuit: a always runs; b runs only if a doesn't short-circuit.
    // Conservative: liveAfter(a) = liveAfter ∪ liveBefore(b) (b might run).
    LcDceLiveSet liveBeforeB;
    int newB = lc_dce_node(ir, n.c[1], liveAfter, liveBeforeB, pureCache);
    LcDceLiveSet liveAfterA = liveAfter;
    liveAfterA.unionWith(liveBeforeB);
    int newA = lc_dce_node(ir, n.c[0], liveAfterA, liveBefore, pureCache);
    n.c[0] = newA;
    n.c[1] = newB;
    return idx;
  }
  // Generic case: every child evaluates left-to-right, no conditionality.
  // Walk children right-to-left, threading liveAfter through.
  LcDceLiveSet curLive = liveAfter;
  for (int i = n.nc - 1; i >= 0; i--)
  {
    LcDceLiveSet liveBeforeChild;
    int newC = lc_dce_node(ir, n.c[i], curLive, liveBeforeChild, pureCache);
    n.c[i] = newC;
    curLive = liveBeforeChild;
  }
  liveBefore = curLive;
  return idx;
}

// Top-level DCE entry. Returns the (possibly substituted) root index. The IR
// Tab is unchanged in size; eliminated nodes simply become unreachable from
// the new root and are ignored by subsequent compile().
LCEXPR_PARSER_INLINE int dceIR(Tab<IRNode> &ir, int root, uint16_t externalVarBase, uint16_t finalNextVarId,
  bool treatTopLevelVarsAsExported)
{
  if (root < 0 || ir.size() == 0)
    return root;
  LcDceLiveSet liveAtRoot;
  liveAtRoot.clear();
  if (treatTopLevelVarsAsExported)
    for (uint16_t v = externalVarBase; v < finalNextVarId; v++)
      liveAtRoot.set(v);
  Tab<uint8_t> pureCache;
  pureCache.resize(ir.size());
  for (auto &c : pureCache)
    c = 0;
  LcDceLiveSet liveBefore;
  return lc_dce_node(ir, root, liveAtRoot, liveBefore, pureCache);
}

// ============================================================================
// Public API
// ============================================================================

LCEXPR_PARSER_INLINE int parseToIR(const char *text, Tab<IRNode> &ir, const FuncParseMap &funcs, const NodeEmitMap &emitMap,
  NameMap &varMap, uint16_t &nextVarId, eastl::string &error, uint8_t maxTempRegs, int *outPeakTempRegs, uint32_t flags,
  VarTypeTable *varTypes)
{
  int startSize = (int)ir.size();
  NameMap savedVarMap(varMap);
  uint16_t savedNextVarId = nextVarId;
  // Snapshot varTypes so a parse failure rolls back any user-var slots the parser
  // appended for `var`/`let` declarations -- the caller's external entries stay
  // intact regardless because savedNextVarId restores the boundary.
  int savedVarTypesSize = varTypes ? varTypes->size() : 0;
  Parser p;
  p.src = text;
  p.pos = text;
  p.ir = &ir;
  p.funcs = &funcs;
  p.emitMap = &emitMap;
  p.varMap = &varMap;
  p.nextVarId = &nextVarId;
  p.varTypes = varTypes;
  p.err = &error;
  p.failed = false;
  p.depth = 0;
  // Anything already in varMap at parse start is external (read-only). User-declared
  // vars are assigned ids >= externalVarBase by register_var() during parseStmt.
  p.externalVarBase = nextVarId;
  p.inFlightVarId = lcexpr::VAR_ID_OVERFLOW;
  // letFlags is per-parse; only entries written by recordVarDecl() are meaningful, so
  // a full clear is enough (subsequent decls overwrite their own slot).
  for (int i = 0; i < (int)MAX_VAR_ID; i++)
    p.letFlags[i] = 0;
  p.maxTempRegs = maxTempRegs > MAX_TEMP_REGS ? MAX_TEMP_REGS : maxTempRegs;
  p.peakTempRegs = 0;
  p.next();
  Parser::TNode result = p.parseSeq();
  if (!p.failed && p.tok != Parser::T_END)
    p.error("unexpected token after expression");
  // peakTempRegs was accumulated by recordVarDecl while inside any `{...}` block,
  // measured against the OUTERMOST block's pre-entry baseline. Top-level decls do
  // not contribute, so a trailing top-level var/let that reuses a slot freed by
  // exitBlock cannot retroactively shrink the peak. Validated against the cap.
  if (!p.failed && p.peakTempRegs > (int)p.maxTempRegs)
  {
    char msg[128];
    snprintf(msg, sizeof(msg), "block uses %d temp registers but caller cap is %d (max %d)", p.peakTempRegs, (int)p.maxTempRegs,
      (int)MAX_TEMP_REGS);
    p.error(msg);
  }
  if (p.failed)
  {
    ir.resize(startSize);
    varMap = savedVarMap;
    nextVarId = savedNextVarId;
    if (varTypes)
      varTypes->resize(savedVarTypesSize);
    if (outPeakTempRegs)
      *outPeakTempRegs = 0;
    return -1;
  }
  if (outPeakTempRegs)
    *outPeakTempRegs = p.peakTempRegs;
  // Constant propagation runs FIRST: forward-dataflow rewrites VarNode reads of
  // slots known to hold a const, then re-folds parent ops whose children all
  // became const. This creates new dead stores (writes whose only readers were
  // replaced with consts) which DCE then sweeps. Reverse order misses the
  // cascade: DCE's pure-discard rule wouldn't fire for SeqNodes whose b-side
  // still references a var that cprop would have folded out.
  const bool exportTopLevel = (flags & PARSE_EXPORT_TOP_LEVEL_VARS) != 0;
  int cpRoot = cpropIR(ir, result.idx, p.externalVarBase, emitMap);
  // Post-cprop diagnostic: catches degenerate ramps that the parse-time check
  // misses because their from/to args only become equal after var->const
  // substitution (e.g. `var x = 10, var y = 10, ramp(h, x, y)`). Run before
  // DCE so the diagnostic reports the user's original code rather than a
  // post-DCE simplification.
  {
    const char *fnName = nullptr, *kind = nullptr;
    if (lc_find_degenerate_ramp(ir, cpRoot, fnName, kind))
    {
      char msg[128];
      snprintf(msg, sizeof(msg), "%s: %s, degenerate ramp", fnName, kind);
      error = msg;
      ir.resize(startSize);
      varMap = savedVarMap;
      nextVarId = savedNextVarId;
      if (varTypes)
        varTypes->resize(savedVarTypesSize);
      if (outPeakTempRegs)
        *outPeakTempRegs = 0;
      return -1;
    }
  }
  return dceIR(ir, cpRoot, p.externalVarBase, nextVarId, exportTopLevel);
}

LCEXPR_PARSER_INLINE uint32_t parse(const char *text, Arena &arena, const FuncParseMap &parseMap, const NodeEmitMap &emitMap,
  NameMap &varMap, uint16_t &nextVarId, eastl::string &error, int *outMaxVarId, uint8_t maxTempRegs, int *outPeakTempRegs,
  uint32_t flags, VarTypeTable *varTypes)
{
  // Snapshot the name state so a post-parseToIR compile failure rolls back atomically.
  // parseToIR itself already restores varMap / nextVarId on its own failure, but once
  // parseToIR has succeeded any new variables introduced by the text must also be
  // reverted if compile() subsequently fails.
  uint32_t startOfs = (uint32_t)arena.size();
  NameMap savedVarMap(varMap);
  uint16_t savedNextVarId = nextVarId;
  int savedVarTypesSize = varTypes ? varTypes->size() : 0;
  Tab<IRNode> ir;
  int root = parseToIR(text, ir, parseMap, emitMap, varMap, nextVarId, error, maxTempRegs, outPeakTempRegs, flags, varTypes);
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
    if (varTypes)
      varTypes->resize(savedVarTypesSize);
    if (outPeakTempRegs)
      *outPeakTempRegs = 0;
    error = "compile failed: IR tag missing from emit map";
    return PARSE_ERROR;
  }
  if (outMaxVarId)
    *outMaxVarId = computeMaxVarId(ir, root);
  return result;
}

} // namespace lcexpr
