#pragma once

namespace SQCompilation
{


class LoopTerminatorCollector : public Visitor {
  bool _firstLevel; // means not under some condition, if or switch
  bool _inSwitch;
  bool _inTry;

  void setTerminator(const Statement *t) {
    if (terminator == nullptr)
      terminator = t;
  }

public:

  bool hasCondBreak;
  bool hasCondContinue;
  bool hasCondReturn;
  bool hasCondThrow;

  bool hasUnconditionalTerm;
  bool hasUnconditionalContinue;

  const Statement *terminator;

  LoopTerminatorCollector(bool firstLevel)
    : _firstLevel(firstLevel)
    , _inSwitch(false)
    , _inTry(false)
    , hasCondBreak(false)
    , hasCondContinue(false)
    , hasCondReturn(false)
    , hasCondThrow(false)
    , hasUnconditionalTerm(false)
    , hasUnconditionalContinue(false)
    , terminator(nullptr) {}

  void visitReturnStatement(ReturnStatement *stmt) override {
    if (_firstLevel) {
      hasUnconditionalTerm = true;
      setTerminator(stmt);
    }
    else if (!hasUnconditionalTerm)
      hasCondReturn = true;
  }

  void visitThrowStatement(ThrowStatement *stmt) override {
    if (_firstLevel && !_inTry) {
      hasUnconditionalTerm = true;
      setTerminator(stmt);
    }
    else if(!hasUnconditionalTerm)
      hasCondThrow = true;
  }

  void visitBreakStatement(BreakStatement *stmt) override {
    if (_firstLevel) {
      hasUnconditionalTerm = true;
      setTerminator(stmt);
    }
    else if (!_inSwitch && !hasUnconditionalTerm)
      hasCondBreak = true;
  }

  void visitContinueStatement(ContinueStatement *stmt) override {
    if (_firstLevel) {
      hasUnconditionalTerm = true;
      hasUnconditionalContinue = true;
      setTerminator(stmt);
    }
    else if (!hasUnconditionalTerm)
      hasCondContinue = true;
  }

  void visitIfStatement(IfStatement *stmt) override {
    bool old = _firstLevel;
    _firstLevel = false;
    Visitor::visitIfStatement(stmt);
    _firstLevel = old;
  }

  void visitLoopStatement(LoopStatement *loop) override {
    // skip - nested loops have their own break/continue/return semantics
  }

  void visitFunctionExpr(FunctionExpr *f) override {
    // skip - function expressions have their own return semantics
  }

  void visitDecl(Decl *d) override {
    // skip
  }

  void visitTryStatement(TryStatement *stmt) override {
    bool old = _inTry;
    _inTry = true;
    stmt->tryStatement()->visit(this);
    _inTry = old;
    stmt->catchStatement()->visit(this);
  }

  void visitSwitchStatement(SwitchStatement *stmt) override {
    bool oldL = _firstLevel;
    bool oldS = _inSwitch;
    _firstLevel = false;
    _inSwitch = true;
    Visitor::visitSwitchStatement(stmt);
    _inSwitch = oldS;
    _firstLevel = oldL;
  }

  bool hasUnconditionalTerminator() const {
    if (hasCondContinue)
      return false;
    return hasUnconditionalTerm;
  }
};


}
