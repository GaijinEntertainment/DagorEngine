#ifndef EXPRESSION_3C96529467795A46_H_
#define EXPRESSION_3C96529467795A46_H_

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

#endif
