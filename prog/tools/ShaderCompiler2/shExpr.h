// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  support for shader arithemtic expressions
/************************************************************************/

#include "shsyn.h"
#include "cppStcodeAssembly.h"
#include "stcodeBytecode.h"
#include <EASTL/unique_ptr.h>
#include <generic/dag_tab.h>
#include <math/dag_color.h>
#include <shaders/shExprTypes.h>
#include <util/dag_globDef.h>

class LocalVar;
class Parser;

namespace ShaderParser
{
//************************************************************************
//* forwards
//************************************************************************
typedef Tab<int> CodeTable;

/*********************************
 *
 * class Expression
 *
 *********************************/
class Expression
{
public:
  // ctor/dtor
  Expression(Symbol *parser_sym) : unaryOp(shexpr::UOP_POSITIVE), parent(NULL), parserSym(parser_sym) {}
  virtual ~Expression() {}

  // return expression type
  virtual shexpr::Type getType() const = 0;

  // return value type
  virtual shexpr::ValueType getValueType() const = 0;

  // assembly this expression
  virtual void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const;
  virtual void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const;

  // check convertion
  virtual bool canConvert(shexpr::ValueType vt) const;

  // get/set operator
  inline void setUnaryOperator(shexpr::UnaryOperator t) { unaryOp = t; }
  inline shexpr::UnaryOperator getUnaryOperator() const { return unaryOp; }

  // get/set parent
  inline Expression *getParent() const { return parent; }
  inline void SetParent(Expression *p) { parent = p; }

  // return true, if expression has only numeric constants and local variables
  virtual bool isConst() const = 0;

  // return true, if expression - is a dynamic expression
  virtual bool isDynamic() const = 0;

  // evaluate this expression as real
  virtual bool evaluate(real &out_value, Parser &parser) = 0;

  // evaluate this expression as color4
  virtual bool evaluate(Color4 &out_value, Parser &parser) = 0;

  // evaluate all avalible branches; return false if error occuried
  virtual bool collapseNumbers(Parser &parser) = 0;

  // return terminal symbol, if present
  Symbol *getParserSymbol() const { return parserSym; }
  virtual Terminal *getTerminal() const { return nullptr; } // Only works if symbol is a terminal

  virtual int getChannels() const { return 1; }

  // return pointer to symbol of subexpression that has both static and dynamic members,
  // nullptr if it does not exist
  virtual Symbol *hasDynamicAndMaterialTermsAt() const { return nullptr; }

  // add register
  Register allocateRegForResult(StcodeVMRegisterAllocator &reg_allocator) const;

  //************************************************************************
  //* helper functions
  //************************************************************************
  // return string name
  static const char *__getName(shexpr::ValueType vt);
  static const char *__getName(shexpr::BinaryOperator op);
  static const char *__getName(shexpr::ColorChannel cc);

  // assembly numeric constant
  static void assembleBytecodeForConstant(CodeTable &code, real v, int dest_reg, bool is_integer);
  static void assembleBytecodeForConstant(CodeTable &code, const Color4 &v, int dest_reg);
  static void assembleCppForConstant(StcodeExpression &cpp_expr, real v, bool is_integer);
  static void assembleCppForConstant(StcodeExpression &cpp_expr, const Color4 &v);

  // dump to debug
  void dump(int level = 0) const;

protected:
  // dump to debug
  virtual void dump_internal(int level, const char *tabs) const = 0;

private:
  Expression *parent;
  shexpr::UnaryOperator unaryOp;
  Symbol *parserSym; // for errors
}; // class Expression
//

/*********************************
 *
 * class ComplexExpression
 *
 *********************************/
class ComplexExpression : public Expression
{
public:
  // ctor/dtor
  ComplexExpression(Symbol *parser_sym, shexpr::ValueType vt, int channel = -1);
  ~ComplexExpression() override;

  ComplexExpression(ComplexExpression &&other) = default;

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_COMPLEX; }

  // return value type
  shexpr::ValueType getValueType() const override;

  // set operand
  void setOperand(shexpr::OperandType op, Expression *child);

  // add user operand
  void addOperand(Expression *child, shexpr::BinaryOperator t);

  // assembly this expression
  void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const override;
  void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const override;

  // check convertion
  bool canConvert(shexpr::ValueType vt) const override;

  // get/set operator
  void setBinOperator(shexpr::OperandType op, shexpr::BinaryOperator t);
  shexpr::BinaryOperator getBinOperator(shexpr::OperandType op) const;

  // return operator for specific expression. return UNDEFINED, if operator not found
  shexpr::BinaryOperator getBinOperator(const Expression *e) const;

  // return operand
  inline Expression *getOperand(shexpr::OperandType t) { return t < 0 ? nullptr : operands[t]; }
  inline const Expression *getOperand(shexpr::OperandType t) const { return t < 0 ? nullptr : operands[t]; }

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override;

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override;

  // evaluate this expression as real
  bool evaluate(real &out_value, Parser &parser) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value, Parser &parser) override;

  // set last binary operator (used in setOperand, if operand_type is USER)
  void setLastBinOp(shexpr::BinaryOperator bop) { lastBinOp = bop; }

  // return operand count
  inline int getOperandCount() const { return operands.size(); }

  Symbol *hasDynamicAndMaterialTermsAt() const override;

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers(Parser &parser) override;

  int getCurrentChannel() const { return currentChannel; }
  void setChannels(int count) { channels = eastl::max(channels, count); }
  int getChannels() const override;

  virtual bool validate(Parser &parser) const;

protected:
  // resize operands
  void resizeOperands(int count);

  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  Tab<Expression *> operands;
  Tab<shexpr::BinaryOperator> binOp;
  shexpr::ValueType baseValueType;
  shexpr::BinaryOperator lastBinOp;
  const int currentChannel;
  int channels = 1;
}; // class ComplexExpression
//


/*********************************
 *
 * class ColorChannelExpression
 *
 *********************************/
class ColorChannelExpression : public Expression
{
public:
  // ctor/dtor
  ColorChannelExpression(Terminal *s) : Expression(s) {}

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_COLOR_CHANNEL; }

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override { return child->isConst(); }

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override { return child->isDynamic(); }

  // get/set child
  Expression *getChild() const { return child.get(); }
  void setChild(Expression *v) { child.reset(v); }

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return (Terminal *)getParserSymbol(); }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers(Parser &parser) override { return child->collapseNumbers(parser); }

protected:
  eastl::unique_ptr<Expression> child;
}; // class ColorChannelExpression
//


/*********************************
 *
 * class SingleColorChannelExpression
 *
 *********************************/
class SingleColorChannelExpression : public ColorChannelExpression
{
public:
  // ctor/dtor
  SingleColorChannelExpression(Terminal *s, shexpr::ColorChannel cc);

  // return value type
  shexpr::ValueType getValueType() const override { return shexpr::VT_REAL; }

  // assembly this expression
  void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const override;
  void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const override;

  // evaluate this expression as real
  bool evaluate(real &out_value, Parser &parser) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value, Parser &parser) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return (Terminal *)getParserSymbol(); }

  // get/set color channel
  inline shexpr::ColorChannel getColorChannel() const { return colorChannel; }
  inline void setColorChannel(shexpr::ColorChannel c)
  {
    G_ASSERT(c != shexpr::_CC_UNDEFINED);
    colorChannel = c;
  }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  shexpr::ColorChannel colorChannel = shexpr::_CC_UNDEFINED;
}; // class SingleColorChannelExpression
//


/*********************************
 *
 * class MultiColorChannelExpression
 *
 *********************************/
class MultiColorChannelExpression : public ColorChannelExpression
{
public:
  using ChannelMask = eastl::fixed_vector<shexpr::ColorChannel, 4, false>;

  // ctor/dtor
  MultiColorChannelExpression(Terminal *s, ChannelMask mask);

  // return value type
  shexpr::ValueType getValueType() const override { return shexpr::VT_COLOR4; }

  // assembly this expression
  void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const override;
  void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const override;

  // evaluate this expression as real
  bool evaluate(real &out_value, Parser &parser) override { return false; }

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value, Parser &parser) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return (Terminal *)getParserSymbol(); }

  int getChannels() const override { return channels.size(); }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  ChannelMask channels;
}; // class MultiColorChannelExpression
//


/*********************************
 *
 * class ConstRealValue
 *
 *********************************/
class ConstRealValue : public Expression
{
public:
  // ctor/dtor
  ConstRealValue(Terminal *s, real v = 0);

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_CONST_VALUE; }

  // return value type
  inline shexpr::ValueType getValueType() const override { return shexpr::VT_REAL; }

  // assembly this expression
  void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const override;
  void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const override;

  // get associated value
  inline real getReal() const { return value; }

  // set associated value
  void setValue(const real v, bool validate_nan, Parser &parser);

  // get converted value to specified type
  const Color4 &getConvertedColor() const;
  const real getConvertedReal() const;

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override { return true; }

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override { return false; }

  // evaluate this expression as real
  bool evaluate(real &out_value, Parser &parser) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value, Parser &parser) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return (Terminal *)getParserSymbol(); }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers(Parser &parser) override { return true; }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  real value;
}; // class ConstRealValue
//


/*********************************
 *
 * class ConstColor4Value
 *
 *********************************/
class ConstColor4Value : public Expression
{
public:
  // ctor/dtor
  ConstColor4Value(Terminal *s, const Color4 &v = Color4(0, 0, 0, 0));

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_CONST_COLOR4; }

  // return value type
  inline shexpr::ValueType getValueType() const override { return shexpr::VT_COLOR4; }

  // assembly this expression
  void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const override;
  void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const override;

  // set associated value
  void setValue(const Color4 &v);

  // get converted value to specified type
  const Color4 &getConvertedColor() const;
  const real getConvertedReal() const;

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override { return true; }

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override { return false; }

  // evaluate this expression as real
  bool evaluate(real &out_value, Parser &parser) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value, Parser &parser) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return (Terminal *)getParserSymbol(); }

  int getChannels() const override { return 4; }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers(Parser &parser) override { return true; }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  Color4 value;
}; // class ConstColor4Value
//


/*********************************
 *
 * class ColorValueExpression
 *
 *********************************/
class ColorValueExpression : public ComplexExpression
{
public:
  // ctor/dtor
  ColorValueExpression(Terminal *s);

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_COLOR_VALUE; }

  // return value type
  inline shexpr::ValueType getValueType() const override { return shexpr::VT_COLOR4; }

  // check convertion
  bool canConvert(shexpr::ValueType vt) const override { return getValueType() == vt; }

  // evaluate this expression as real
  bool evaluate(real &out_value, Parser &parser) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value, Parser &parser) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return (Terminal *)getParserSymbol(); }

  // assembly this expression
  void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const override;
  void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const override;

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers(Parser &parser) override;
  int getChannels() const override { return 4; }
  bool validate(Parser &parser) const override { return true; }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;
}; // class ColorValueExpression
//


/*********************************
 *
 * class LVarValueExpression
 *
 *********************************/
class LVarValueExpression : public Expression
{
public:
  // ctor/dtor
  LVarValueExpression(Terminal *s, const LocalVar &var);

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_LVAR_VALUE; }

  // return value type
  shexpr::ValueType getValueType() const override;

  // assembly this expression
  void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const override;
  void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const override;

  // check convertion - if fail, report error & return false
  bool canConvert(shexpr::ValueType vt) const override;

  // return true, if expression has only numeric constants and constant local variables
  bool isConst() const override;

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override;

  // evaluate this expression as real
  bool evaluate(real &out_value, Parser &parser) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value, Parser &parser) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return (Terminal *)getParserSymbol(); }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers(Parser &parser) override { return true; }

  int getChannels() const override { return getValueType() == shexpr::VT_COLOR4 ? 4 : 1; }

  Symbol *hasDynamicAndMaterialTermsAt() const override;

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  const LocalVar &var;
}; // class LVarValueExpression
//

/*********************************
 *
 * class StVarValueExpression
 *
 *********************************/
class StVarValueExpression : public Expression
{
public:
  // ctor/dtor
  StVarValueExpression(Terminal *s, int var_id, shexpr::ValueType vt, bool is_dynamic, bool is_global, bool is_int);

  ~StVarValueExpression() override;

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_STVAR_VALUE; }

  // return value type
  inline shexpr::ValueType getValueType() const override { return valueType; }

  // assembly this expression
  void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const override;
  void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const override;

  // return true, if expression has only numeric constants and constant local variables
  bool isConst() const override { return false; }

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override { return isDynamicFlag; }

  // evaluate this expression as real
  bool evaluate(real &out_value, Parser &parser) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value, Parser &parser) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return (Terminal *)getParserSymbol(); }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers(Parser &parser) override { return true; }

  int getChannels() const override { return getValueType() == shexpr::VT_COLOR4 ? 4 : 1; }


protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  int varId;
  shexpr::ValueType valueType;
  bool isDynamicFlag;
  bool isGlobalFlag;
  bool isInteger;
}; // class StVarValueExpression
//

/*********************************
 *
 * class FunctionExpression
 *
 *********************************/
class FunctionExpression : public ComplexExpression
{
public:
  // ctor/dtor
  FunctionExpression(Terminal *s, int function_id, int channel = -1);

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_FUNCTION; }

  // return value type
  shexpr::ValueType getValueType() const override;

  // check convertion
  bool canConvert(shexpr::ValueType vt) const override { return Expression::canConvert(vt); }

  // evaluate this expression as real
  bool evaluate(real &out_value, Parser &parser) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value, Parser &parser) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return (Terminal *)getParserSymbol(); }

  // assembly this expression
  void assembleBytecode(CodeTable &code, Register &dest_reg, StcodeVMRegisterAllocator &reg_allocator, bool is_integer) const override;
  void assembleCpp(StcodeExpression &cpp_expr, bool is_integer) const override;

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers(Parser &parser) override;

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override;

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override;

  int getChannels() const override;

  bool validate(Parser &parser) const override { return true; }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  int func;
}; // class FunctionExpression
//
} // namespace ShaderParser
