// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  shader boolean variable (for boolean expr eval)
/************************************************************************/

class ShVarBool
{
public:
  bool isConst; // if true, value is never changing in the shader
  bool value;   // boolean value

  inline ShVarBool() : isConst(false), value(true) {}

  inline ShVarBool(const ShVarBool &other) { operator=(other); }

  inline explicit ShVarBool(const bool b, bool is_const) : isConst(is_const), value(b) {}

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
