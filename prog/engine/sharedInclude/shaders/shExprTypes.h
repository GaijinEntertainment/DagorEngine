/************************************************************************
  expression types
************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __SHEXPRTYPES_H
#define __SHEXPRTYPES_H

namespace shexpr
{
enum Type
{
  E_CONST_VALUE,   // constant real value
  E_CONST_COLOR4,  // constant color4 value
  E_COLOR_VALUE,   // color value
  E_LVAR_VALUE,    // local variable
  E_STVAR_VALUE,   // shader variable except local
  E_COMPLEX,       // +-*/
  E_COLOR_CHANNEL, // color channel modifier
  E_FUNCTION,      // standart function
};

enum ValueType
{
  VT_UNDEFINED = -1, // unknown type
  VT_REAL = 0,       // real-type value
  VT_COLOR4 = 1,     // color4-type value
  VT_TEXTURE = 2,    // texture-type value
  VT_BUFFER = 3,     // buffer-type value
};

enum UnaryOperator
{
  UOP_NEGATIVE, // negate expression's sign
  UOP_POSITIVE, // do nothing
};

enum BinaryOperator
{
  _OP_UNDEFINED = -1,
  OP_ADD = 0,
  OP_SUB = 1,
  OP_MUL = 2,
  OP_DIV = 3
};

enum OperandType
{
  _OPER_UNDEFINED = -1,
  OPER_LEFT = 0,
  OPER_USER = -2,

  OPER_R = 0,
  OPER_G = 1,
  OPER_B = 2,
  OPER_A = 3,
};

enum ColorChannel
{
  _CC_UNDEFINED = -1,
  CC_R = 0,
  CC_G = 1,
  CC_B = 2,
  CC_A = 3,
};
} // namespace shexpr


#endif //__SHEXPRTYPES_H
