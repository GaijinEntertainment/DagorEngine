/************************************************************************
  shader boolean variable (for boolean expr eval)
/************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __SHVARBOOL_H
#define __SHVARBOOL_H

class ShVarBool
{
public:
  bool isConst; // if true, value is never changing in the shader
  bool value;   // boolean value

  inline ShVarBool() : isConst(false), value(true) {}

  inline ShVarBool(const ShVarBool &other) { operator=(other); }

  inline explicit ShVarBool(const bool b, bool is_cost) : isConst(is_cost), value(b) {}

  inline ShVarBool &operator=(const ShVarBool &other)
  {
    isConst = other.isConst;
    value = other.value;
    return *this;
  }

  inline ShVarBool operator!() const { return ShVarBool(!value, isConst); }

  inline ShVarBool operator&&(const ShVarBool &other)
  {
    if (isConst)
      return value ? other : *this;
    if (other.isConst)
      return other.value ? *this : other;
    return ShVarBool(value && other.value, isConst && other.isConst);
  }

  inline ShVarBool operator||(const ShVarBool &other) { return ShVarBool(value || other.value, isConst && other.isConst); }
};

#endif //__SHVARBOOL_H
