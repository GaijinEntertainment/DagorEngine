// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class CompileState;

#include <string>

class Expression
{
public:
  Expression(const std::string &anExpression, CompileState *aState);

  int evaluate();

private:
  int boolean();
  int equality();
  int relational();
  int additive();
  int multiplicative();
  int factor();

  int macro();

  void ws();
  bool token(const char *aToken);

  std::string Text;
  CompileState *theState;
  unsigned int Pos;
};
