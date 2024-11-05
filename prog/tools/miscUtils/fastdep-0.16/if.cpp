// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "if.h"
#include "compileState.h"
#include "expression.h"
#include "fileStructure.h"
#include "sequence.h"

#include <iostream>

If::Control::~Control() {}

If::IfDef::IfDef(const std::string &aName) : Macro(aName) {}

If::IfDef::IfDef(const If::IfDef &anOther) : Macro(anOther.Macro) {}

If::IfDef::~IfDef() {}

If::Control *If::IfDef::copy() const { return new If::IfDef(*this); }

bool If::IfDef::isTrue(CompileState *aState) const { return aState->isDefined(Macro); }

If::IfNDef::IfNDef(const std::string &aName) : Macro(aName) {}

If::IfNDef::IfNDef(const If::IfNDef &anOther) : Macro(anOther.Macro) {}

If::IfNDef::~IfNDef() {}

If::Control *If::IfNDef::copy() const { return new If::IfNDef(*this); }

bool If::IfNDef::isTrue(CompileState *aState) const { return !aState->isDefined(Macro); }

If::IfTest::IfTest(const std::string &anExpression) : theExpression(anExpression)
{
  const char *s = theExpression.c_str();
  while (strchr(" \t\v", *s))
    s++;

  if (strncmp(s, "defined", 7) == 0)
  {
    s += 7;
    if (strchr(" \t\v(", *s))
    {
      while (strchr(" \t\v", *s))
        s++;
      if (*s != '(')
      {
        s--;
        *(char *)s = '(';
        theExpression += ')';
      }
    }
  }
}

If::IfTest::IfTest(const If::IfTest &anOther) : theExpression(anOther.theExpression) {}

If::Control *If::IfTest::copy() const { return new If::IfTest(*this); }

bool If::IfTest::isTrue(CompileState *aState) const
{
  Expression E(theExpression, aState);
  return E.evaluate();
}

If::If(FileStructure *aStructure) : Element(aStructure), Else(0) {}

If::If(const If &anOther) : Element(anOther), Else(0)
{
  for (unsigned int i = 0; i < anOther.Expr.size(); ++i)
    Expr.push_back(anOther.Expr[i]->copy());
  for (unsigned int j = 0; j < anOther.Seq.size(); ++j)
    Seq.push_back(dynamic_cast<Sequence *>(anOther.Seq[j]->copy()));
  if (anOther.Else)
    Else = dynamic_cast<Sequence *>(anOther.Else->copy());
}

If::~If()
{
  for (unsigned int i = 0; i < Expr.size(); ++i)
    delete Expr[i];
}

If &If::operator=(const If &anOther)
{
  if (this != &anOther)
  {
    Element::operator=(*this);
    Expr.erase(Expr.begin(), Expr.end());
    for (unsigned int i = 0; i < anOther.Expr.size(); ++i)
      Expr.push_back(anOther.Expr[i]->copy());
    for (unsigned int j = 0; j < anOther.Seq.size(); ++j)
      Seq.push_back(dynamic_cast<Sequence *>(anOther.Seq[j]->copy()));
    if (Else)
    {
      delete Else;
      Else = 0;
    }
    if (anOther.Else)
      Else = dynamic_cast<Sequence *>(anOther.Else->copy());
  }
  return *this;
}

Element *If::copy() const { return new If(*this); }

void If::getDependencies(CompileState *aState)
{
  for (unsigned int i = 0; i < Expr.size(); ++i)
    if (Expr[i]->isTrue(aState))
    {
      //			std::cout << "following in if part " << i << std::endl;
      Seq[i]->getDependencies(aState);
      return;
    }
  //	std::cout << "skipping to ...";
  if (Else)
  {
    //		std::cout << "else" << std::endl;
    Else->getDependencies(aState);
  }
  //	else
  //		std::cout << "nothing" << std::endl;
}

Sequence *If::addIf(Control *anExpr)
{
  Expr.push_back(anExpr);
  Seq.push_back(new Sequence(getStructure()));
  return Seq.back();
}

Sequence *If::addElse()
{
  if (!Else)
    Else = new Sequence(getStructure());
  return Else;
}

Sequence *If::getLastScope()
{
  if (Else)
    return Else;
  return Seq.back();
}
