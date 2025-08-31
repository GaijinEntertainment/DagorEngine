// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "element.h"

#include <string>
#include <vector>

class CompileState;
class FileStructure;
class Sequence;

class If : public Element
{
public:
  class Control
  {
  public:
    virtual ~Control() = 0;
    virtual Control *copy() const = 0;
    virtual bool isTrue(CompileState *aState) const = 0;
  };

  class IfDef : public Control
  {
  public:
    IfDef(const std::string &aName);
    IfDef(const IfDef &anOther);
    ~IfDef() override;
    Control *copy() const override;
    bool isTrue(CompileState *aState) const override;

  private:
    std::string Macro;
  };

  class IfNDef : public Control
  {
  public:
    IfNDef(const std::string &aName);
    IfNDef(const IfNDef &anOther);
    ~IfNDef() override;
    Control *copy() const override;
    bool isTrue(CompileState *aState) const override;

  private:
    std::string Macro;
  };

  class IfTest : public Control
  {
  public:
    IfTest(const std::string &anControl);
    IfTest(const IfTest &anOther);

    Control *copy() const override;
    bool isTrue(CompileState *aState) const override;

  private:
    std::string theExpression;
  };

public:
  If(FileStructure *aStructure);
  If(const If &anOther);
  ~If() override;

  If &operator=(const If &anOther);

  Element *copy() const override;
  void getDependencies(CompileState *aState) override;

  Sequence *addIf(Control *anExpr);
  Sequence *addElse();
  Sequence *getLastScope();

private:
  std::vector<Control *> Expr;
  std::vector<Sequence *> Seq;
  Sequence *Else;
};
