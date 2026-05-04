#pragma once

#include "checker_visitor.h"

namespace SQCompilation
{

struct VarScope;

enum BreakableScopeKind {
  BSK_LOOP,
  BSK_SWITCH
};

struct BreakableScope {
  BreakableScopeKind kind;

  BreakableScope *parent;

  union {
    const LoopStatement *loop;
    const SwitchStatement *swtch;
  } node;

  VarScope *loopScope;
  VarScope *exitScope;
  CheckerVisitor *visitor;

  BreakableScope(CheckerVisitor *v, const SwitchStatement *swtch)
  : visitor(v), kind(BSK_SWITCH), loopScope(nullptr), exitScope(nullptr), parent(v->breakScope)
  {
    node.swtch = swtch;
    v->breakScope = this;
  }
  BreakableScope(CheckerVisitor *v, const LoopStatement *loop, VarScope *ls, VarScope *es)
    : visitor(v), kind(BSK_LOOP), loopScope(ls), exitScope(es), parent(v->breakScope)
  {
    assert(ls/* && es*/);
    node.loop = loop;
    v->breakScope = this;
  }
  ~BreakableScope()
  {
    visitor->breakScope = parent;
  }
};


}
