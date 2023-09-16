#ifndef IF_H_
#define IF_H_

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
    virtual ~IfDef();
    virtual Control *copy() const;
    virtual bool isTrue(CompileState *aState) const;

  private:
    std::string Macro;
  };

  class IfNDef : public Control
  {
  public:
    IfNDef(const std::string &aName);
    IfNDef(const IfNDef &anOther);
    virtual ~IfNDef();
    virtual Control *copy() const;
    virtual bool isTrue(CompileState *aState) const;

  private:
    std::string Macro;
  };

  class IfTest : public Control
  {
  public:
    IfTest(const std::string &anControl);
    IfTest(const IfTest &anOther);

    virtual Control *copy() const;
    virtual bool isTrue(CompileState *aState) const;

  private:
    std::string theExpression;
  };

public:
  If(FileStructure *aStructure);
  If(const If &anOther);
  virtual ~If();

  If &operator=(const If &anOther);

  virtual Element *copy() const;
  virtual void getDependencies(CompileState *aState);

  Sequence *addIf(Control *anExpr);
  Sequence *addElse();
  Sequence *getLastScope();

private:
  std::vector<Control *> Expr;
  std::vector<Sequence *> Seq;
  Sequence *Else;
};

#endif
