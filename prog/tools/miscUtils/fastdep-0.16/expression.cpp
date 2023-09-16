#include "expression.h"
#include "compileState.h"

Expression::Expression(const std::string &anExpression, CompileState *aState) : Text(anExpression), theState(aState), Pos(0) {}

int Expression::evaluate()
{
  Pos = 0;
  ws();
  int Result = boolean();
  if (Pos < Text.length())
    // TODO error
    ;
  return Result;
}

int Expression::boolean()
{
  int Left = equality();
  if (Pos == Text.length())
    return Left;

  if (Text[Pos] == '|')
  {
    ++Pos;
    if ((Pos == Text.length()) || (Text[Pos] != '|'))
      //  TODO error
      return Left;
    ++Pos;
    ws();
    int Right = boolean();
    return Left || Right;
  }
  if (Text[Pos] == '&')
  {
    ++Pos;
    if ((Pos == Text.length()) || (Text[Pos] != '&'))
      //  TODO error
      return Left;
    ++Pos;
    ws();
    int Right = boolean();
    return Left && Right;
  }
  return Left;
}

int Expression::equality()
{
  int Left = relational();
  if (Pos == Text.length())
    return Left;
  if (Text[Pos] == '=')
  {
    ++Pos;
    if ((Pos == Text.length()) || (Text[Pos] != '='))
      //  TODO error
      return Left;
    ++Pos;
    ws();
    int Right = relational();
    return Left == Right;
  }
  if (Text[Pos] == '!')
  {
    ++Pos;
    if ((Pos == Text.length()) || (Text[Pos] != '='))
      //  TODO error
      return Left;
    ++Pos;
    ws();
    int Right = relational();
    return Left != Right;
  }
  return Left;
}

int Expression::relational()
{
  int Left = additive();
  if (Pos == Text.length())
    return Left;
  if (Text[Pos] == '<')
  {
    ++Pos;
    if ((Pos < Text.length()) && (Text[Pos] == '='))
    {
      ++Pos;
      ws();
      if (Pos == Text.length())
        return Left; // TODO error
      int Right = additive();
      return Left <= Right;
    }
    ws();
    if (Pos == Text.length())
      return Left; // TODO error
    int Right = additive();
    return Left < Right;
  }
  if (Text[Pos] == '>')
  {
    ++Pos;
    if ((Pos < Text.length()) && (Text[Pos] == '='))
    {
      ++Pos;
      ws();
      if (Pos == Text.length())
        return Left; // TODO error
      int Right = additive();
      return Left >= Right;
    }
    ws();
    if (Pos == Text.length())
      return Left; // TODO error
    int Right = additive();
    return Left > Right;
  }
  return Left;
}

int Expression::additive()
{
  int Left = multiplicative();
  while (Pos < Text.length())
    if (Text[Pos] == '+')
    {
      ++Pos;
      ws();
      int Right = multiplicative();
      Left += Right;
    }
    else if (Text[Pos] == '-')
    {
      ++Pos;
      ws();
      int Right = multiplicative();
      Left -= Right;
    }
    else
      break;
  return Left;
}

int Expression::multiplicative()
{
  int Left = factor();
  while (Pos < Text.length())
    if (Text[Pos] == '*')
    {
      ++Pos;
      ws();
      int Right = factor();
      Left *= Right;
    }
    else if (Text[Pos] == '/')
    {
      ++Pos;
      ws();
      int Right = factor();
      if (Right == 0)
        // TODO error
        Left = 0;
      else
        Left /= Right;
    }
    else
      break;
  return Left;
}

int Expression::factor()
{
  if (Pos == Text.length())
    return 0;
  if (Text[Pos] == '!')
  {
    ++Pos;
    ws();
    return !factor();
  }
  if (Text[Pos] == '-')
  {
    ++Pos;
    ws();
    return -factor();
  }
  if (Text[Pos] == '(')
  {
    ++Pos;
    ws();
    int Result = factor();
    if ((Pos == Text.length()) || (Text[Pos] != ')'))
      // TODO error
      ;
    else
    {
      ++Pos;
      ws();
    }
    return Result;
  }
  if ((Text[Pos] >= '0') && (Text[Pos] <= '9'))
  {
    int Result = Text[Pos] - '0';
    ++Pos;
    while ((Pos < Text.length()) && ((Text[Pos] >= '0') && (Text[Pos] <= '9')))
      Result = Result * 10 + Text[Pos++] - '0';
    ws();
    return Result;
  }
  if (token("defined"))
  {
    if ((Pos == Text.length()) || (Text[Pos] != '('))
      // TODO error
      return 0;
    ++Pos;
    ws();
    unsigned int To = Pos;
    while (To < Text.length())
    {
      char c = Text[To];
      if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || c == '_')
        ++To;
      else
        break;
    }
    int Result = 0;
    if (Pos < To)
    {
      std::string Macro(Text, Pos, To - Pos);
      if (theState->isDefined(Macro))
        Result = 1;
      Pos = To;
    }
    if ((Pos == Text.length()) || (Text[Pos] != ')'))
      // TODO error
      ;
    else
    {
      ++Pos;
      ws();
    }
    return Result;
  }
  // TODO error
  return macro();
}

int Expression::macro()
{
  std::string Name;
  while (Pos < Text.length())
  {
    char c = Text[Pos];
    if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || c == '_')
    {
      Name += c;
      ++Pos;
    }
    else
      break;
  }
  if (Name != "")
  {
    ws();
    Expression E(theState->getContent(Name), theState);
    return E.evaluate();
  }
  else
    // TODO error
    return 0;
}

void Expression::ws()
{
  while ((Pos < Text.length()) && (Text[Pos] == ' '))
    ++Pos;
}

bool Expression::token(const char *aToken)
{
  unsigned int OrigPos = Pos;
  for (int i = 0; aToken[i]; ++i, ++Pos)
    if ((Pos == Text.length()) || (Text[Pos] != aToken[i]))
    {
      Pos = OrigPos;
      return false;
    }
  if (Pos < Text.length())
  {
    char c = Text[Pos];
    if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || c == '_')
    {
      Pos = OrigPos;
      return false;
    }
  }
  ws();
  return true;
}
