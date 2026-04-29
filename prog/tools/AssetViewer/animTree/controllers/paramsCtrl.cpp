// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "paramsCtrl.h"
#include "paramsCtrlDragDropHandlers.h"
#include "../animTree.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"
#include "../animTreePanelPids.h"
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathBase.h>
#include <propPanel/control/container.h>
#include <propPanel/control/dragAndDropHandler.h>
#include <propPanel/c_constants.h> // DEFAULT_CONTROL_HEIGHT
#include <propPanel/colors.h>
#include <libTools/util/hdpiUtil.h>
#include <util/dag_lookup.h>

enum RateSrc
{
  RATE_SRC_FIXED = 0,
  RATE_SRC_PARAM = 1,
  RATE_SRC_AUTO = 2,
};

enum ScaleSrc
{
  SCALE_SRC_NONE = 0,
  SCALE_SRC_PARAM = 1,
  SCALE_SRC_AUTO = 2,
};

enum BlockType
{
  BLOCK_TYPE_CHANGE_PARAM = 0,
  BLOCK_TYPE_PARAM_REMAP = 1,
};

static RateSrc get_rate_src(const DataBlock &settings)
{
  if (settings.getStr("rateParam", nullptr) != nullptr)
    return RATE_SRC_PARAM;
  if (settings.getBool("variableChangeRate", false))
    return RATE_SRC_AUTO;
  return RATE_SRC_FIXED;
}

static ScaleSrc get_scale_src(const DataBlock &settings)
{
  if (settings.getStr("scaleParam", nullptr) != nullptr)
    return SCALE_SRC_PARAM;
  if (settings.getBool("variableScale", false))
    return SCALE_SRC_AUTO;
  return SCALE_SRC_NONE;
}

// Returns the string representation of one operand slot (p0 or p1).
// var_key (v0/v1) and float_key (p0/p1) are independent fields in the engine struct, but in practice
// they are never set together in the same block - authors use either a variable reference or a constant.
static String get_if_math_operand(const DataBlock &blk, const char *float_key, const char *var_key, const char *named_key,
  const char *slot_key)
{
  if (const char *v = blk.getStr(var_key, nullptr))
    return String(v);
  if (const char *v = blk.getStr(named_key, nullptr))
    return String(0, "@%s", v);
  if (const char *v = blk.getStr(slot_key, nullptr))
    return String(0, "#%s", v);
  return String(0, "%g", blk.getReal(float_key, 0.f));
}

static const char *MATH_OPS[] = {"=", "+", "-", "*", "/", "%", "[]", "rnd", "approach", "sin", "floor"};
static const char *COMP_OPS[] = {"=", "!=", ">", ">=", "<", "<=", "inside", "outside", "dist"};

static bool is_known_math_op(const char *op) { return lup(op, MATH_OPS, countof(MATH_OPS), -1) >= 0; }
static bool is_known_comp_op(const char *op) { return lup(op, COMP_OPS, countof(COMP_OPS), -1) >= 0; }

static String record_error(const String &msg, String &out_error)
{
  logerr("%s", msg.c_str());
  if (out_error.empty())
    out_error = msg;
  return String();
}

static String math_blk_to_string(const DataBlock &blk, int indent, String &out_error)
{
  const char *param = blk.getStr("param", nullptr);
  const char *op = blk.getStr("op", nullptr);
  const String indent_str(0, "%*s", indent * 2, "");

  if (!param || !*param)
    return record_error(String("math block: missing \"param\""), out_error);
  if (!op || !*op)
    return record_error(String(0, "math block \"%s\": missing \"op\"", param), out_error);
  if (!is_known_math_op(op))
    return record_error(String(0, "math block \"%s\": unknown op \"%s\"", param, op), out_error);

  const String p0 = get_if_math_operand(blk, "p0", "v0", "named_p0", "slot_p0");
  const String p1 = get_if_math_operand(blk, "p1", "v1", "named_p1", "slot_p1");
  // out:b=yes: result is blended with current value by controller weight (res = lerp(v, res, w)),
  // so the math effect fades in/out together with the controller instead of being applied at full strength
  const char *outSuffix = blk.getBool("out", false) ? " [out]" : "";

  if (strcmp(op, "[]") == 0)
    return String(0, "%s%s = clamp(%s, %s)%s\n", indent_str.c_str(), param, p0.c_str(), p1.c_str(), outSuffix);
  if (strcmp(op, "rnd") == 0)
    return String(0, "%s%s = rnd(%s, %s)%s\n", indent_str.c_str(), param, p0.c_str(), p1.c_str(), outSuffix);
  if (strcmp(op, "approach") == 0)
    return String(0, "%s%s = approach(%s, %s)%s\n", indent_str.c_str(), param, p0.c_str(), p1.c_str(), outSuffix);
  if (strcmp(op, "sin") == 0)
    return String(0, "%s%s = sin(%s)%s\n", indent_str.c_str(), param, p0.c_str(), outSuffix);
  if (strcmp(op, "floor") == 0)
    return String(0, "%s%s = floor(%s)%s\n", indent_str.c_str(), param, p0.c_str(), outSuffix);
  if (strcmp(op, "=") == 0)
    return String(0, "%s%s = %s%s\n", indent_str.c_str(), param, p0.c_str(), outSuffix);
  return String(0, "%s%s %s= %s%s\n", indent_str.c_str(), param, op, p0.c_str(), outSuffix);
}

static String if_condition_blk_to_string(const DataBlock &blk, int indent, String &out_error)
{
  const char *param = blk.getStr("param", nullptr);
  const char *op = blk.getStr("op", nullptr);
  const String indent_str(0, "%*s", indent * 2, "");

  if (!param || !*param)
    return record_error(String("if block: missing \"param\""), out_error);
  if (!op || !*op)
    return record_error(String(0, "if block \"%s\": missing \"op\"", param), out_error);
  if (!is_known_comp_op(op))
    return record_error(String(0, "if block \"%s\": unknown op \"%s\"", param, op), out_error);

  if (is_comp_op_needs_p1(op))
  {
    const String p0 = get_if_math_operand(blk, "p0", "v0", "named_p0", "slot_p0");
    const String p1 = get_if_math_operand(blk, "p1", "v1", "named_p1", "slot_p1");
    return String(0, "%sif (%s %s(%s, %s)) {\n", indent_str.c_str(), param, op, p0.c_str(), p1.c_str());
  }
  const String p0 = get_if_math_operand(blk, "p0", "v0", "named_p0", "slot_p0");
  return String(0, "%sif (%s %s %s) {\n", indent_str.c_str(), param, op, p0.c_str());
}

static String if_math_blk_to_string(const DataBlock &blk, int indent, String &out_error)
{
  const char *blockName = blk.getBlockName();
  if (strcmp(blockName, "math") == 0)
    return math_blk_to_string(blk, indent, out_error);
  if (strcmp(blockName, "if") != 0)
    return String();

  String result = if_condition_blk_to_string(blk, indent, out_error);
  for (int i = 0; i < blk.blockCount(); ++i)
    result += if_math_blk_to_string(*blk.getBlock(i), indent + 1, out_error);
  const String indent_str(0, "%*s", indent * 2, "");
  result += String(0, "%s}\n", indent_str.c_str());
  return result;
}

struct IfMathParser
{
  String error;

  bool setError(const String &msg)
  {
    record_error(msg, error);
    return false;
  }

  static const char *skipWs(const char *p)
  {
    while (*p && isspace((unsigned char)*p))
      ++p;
    return p;
  }

  static bool startsWithIf(const char *p) { return strncmp(p, "if", 2) == 0 && (isspace((unsigned char)p[2]) || p[2] == '('); }

  // Returns "'x'" for a real character, or "end of line" when c == '\0'.
  static String charOrEol(char c) { return c ? String(0, "'%c'", c) : String("end of line"); }

  // Try to read a named comparison operator (inside/outside/dist). Rolls back p if the identifier
  // is not a known op. Returns true if an op was consumed, false if p is left unchanged.
  static bool tryReadNamedCompOp(const char *&p, String &out)
  {
    const char *saved = p;
    if (readIdent(p, out) && is_known_comp_op(out.c_str()))
      return true;
    p = saved;
    out = String();
    return false;
  }

  // Read an identifier token (stops at whitespace or operator/punctuation chars).
  static bool readIdent(const char *&p, String &out)
  {
    p = skipWs(p);
    const char *start = p;
    while (*p && !isspace((unsigned char)*p) && !strchr("(),=+-*/%[]@#", *p))
      ++p;
    if (p == start)
      return false;
    out = String(0, "%.*s", (int)(p - start), start);
    return true;
  }

  enum class OperandType
  {
    Float,
    Ident,
    Named,
    Slot
  };

  struct Operand
  {
    String text;
    OperandType type;
  };

  // Read one operand: @name (named), #name (slot), float literal, or plain identifier (variable).
  static bool readOperand(const char *&p, Operand &out)
  {
    p = skipWs(p);
    if (*p == '@' || *p == '#')
    {
      out.type = (*p == '@') ? OperandType::Named : OperandType::Slot;
      ++p;
      return readIdent(p, out.text);
    }
    // Try float (handles sign, decimal, exponent).
    char *end = nullptr;
    strtof(p, &end);
    if (end && end > p)
    {
      out.text = String(0, "%.*s", (int)(end - p), p);
      out.type = OperandType::Float;
      p = end;
      return true;
    }
    out.type = OperandType::Ident;
    return readIdent(p, out.text);
  }

  // Write a parsed operand back into a DataBlock using the appropriate key.
  static void setOperand(DataBlock &blk, const Operand &operand, const char *p_key, const char *v_key, const char *named_key,
    const char *slot_key)
  {
    if (operand.text.empty())
      return;
    switch (operand.type)
    {
      case OperandType::Named: blk.setStr(named_key, operand.text.c_str()); break;
      case OperandType::Slot: blk.setStr(slot_key, operand.text.c_str()); break;
      case OperandType::Float: blk.setReal(p_key, strtof(operand.text.c_str(), nullptr)); break;
      case OperandType::Ident: blk.setStr(v_key, operand.text.c_str()); break;
    }
  }

  // Parse "(p0, p1)" — p must point just past the opening '('. Writes p0/v0/... and p1/v1/... into blk.
  bool parseTwoOperands(const char *&p, DataBlock &blk)
  {
    Operand p0, p1;
    if (!readOperand(p, p0))
      return setError(String("expected first value (e.g. 0.5, myParam, @namedParam)"));
    p = skipWs(p);
    if (*p != ',')
      return setError(String(0, "expected ',' to separate the two values, got %s", charOrEol(*p).c_str()));
    ++p;
    if (!readOperand(p, p1))
      return setError(String("expected second value after ',' (e.g. clamp(0.0, 1.0))"));
    p = skipWs(p);
    if (*p != ')')
      return setError(String(0, "expected ')' to close the value list, got %s", charOrEol(*p).c_str()));
    ++p;
    setOperand(blk, p0, "p0", "v0", "named_p0", "slot_p0");
    setOperand(blk, p1, "p1", "v1", "named_p1", "slot_p1");
    return true;
  }

  // Parse "(p0)" — p must point just past the opening '('. Writes p0/v0/... into blk.
  bool parseOneParenOperand(const char *&p, DataBlock &blk)
  {
    Operand p0;
    if (!readOperand(p, p0))
      return setError(String("expected a value inside parentheses (e.g. 0.5, myParam)"));
    p = skipWs(p);
    if (*p != ')')
      return setError(String(0, "expected ')' after the value, got %s", charOrEol(*p).c_str()));
    ++p;
    setOperand(blk, p0, "p0", "v0", "named_p0", "slot_p0");
    return true;
  }

  // Parse one math line into a "math" DataBlock.
  // Format: "param [op]= operand [out]" or "param = func(operands...) [out]"
  bool parseMathLine(const char *line, DataBlock &out)
  {
    const char *p = skipWs(line);
    String param;
    if (!readIdent(p, param))
      return setError(String("expected a parameter name at the start (e.g. \"myParam = 1.0\")"));
    p = skipWs(p);
    if (!*p)
      return setError(String(0,
        "\"%s\": expected an operator after the parameter name (e.g. = 1.0, += value, -= value, *= value, /= value, %%= value)",
        param.c_str()));

    out.setStr("param", param.c_str());

    if (strchr("+-*/%", *p) && p[1] == '=')
    {
      // Compound assignment: +=, -=, *=, /=, %=
      const String mathOp(0, "%c", *p);
      p += 2;
      Operand operand;
      if (!readOperand(p, operand))
        return setError(String(0, "\"%s\": expected a value after \"%s=\" (e.g. 1.0, otherParam)", param.c_str(), mathOp.c_str()));
      if (skipWs(p)[0] == '(')
        return setError(String(0, "\"%s\": function calls are not supported with \"%s=\"; use \"%s = %s(...)\" instead", param.c_str(),
          mathOp.c_str(), param.c_str(), operand.text.c_str()));
      out.setStr("op", mathOp.c_str());
      setOperand(out, operand, "p0", "v0", "named_p0", "slot_p0");
    }
    else if (*p == '=')
    {
      ++p;
      p = skipWs(p);

      struct FuncForm
      {
        const char *prefix;
        const char *op;
        bool twoOperands;
      };
      static const FuncForm FORMS[] = {
        {"clamp(", "[]", true},
        {"rnd(", "rnd", true},
        {"approach(", "approach", true},
        {"sin(", "sin", false},
        {"floor(", "floor", false},
      };
      bool matched = false;
      for (const FuncForm &f : FORMS)
      {
        const int prefLen = (int)strlen(f.prefix);
        if (strncmp(p, f.prefix, prefLen) != 0)
          continue;
        p += prefLen;
        out.setStr("op", f.op);
        if (!(f.twoOperands ? parseTwoOperands(p, out) : parseOneParenOperand(p, out)))
          return false;
        matched = true;
        break;
      }
      if (!matched)
      {
        // Direct assignment: param = operand
        Operand operand;
        if (!readOperand(p, operand))
          return setError(
            String(0, "\"%s\": expected a value or function after '=' (e.g. 1.0, otherParam, sin(x), clamp(a, b))", param.c_str()));
        if (skipWs(p)[0] == '(')
          return setError(String(0, "\"%s\": unknown function \"%s\"; valid functions: clamp, rnd, approach, sin, floor",
            param.c_str(), operand.text.c_str()));
        out.setStr("op", "=");
        setOperand(out, operand, "p0", "v0", "named_p0", "slot_p0");
      }
    }
    else
      return setError(String(0,
        "\"%s\": unexpected character '%c' after parameter name; valid operators: =, +=, -=, *=, /=, %%=", param.c_str(), *p));

    // Optional "[out]" suffix
    p = skipWs(p);
    static constexpr int OUT_SUFFIX_LEN = sizeof("[out]") - 1;
    if (strncmp(p, "[out]", OUT_SUFFIX_LEN) == 0)
    {
      out.setBool("out", true);
      p += OUT_SUFFIX_LEN;
    }
    p = skipWs(p);
    if (*p == '}')
      return setError(String(0, "closing '}' must be on its own line, not after \"%s\"", line));
    if (*p)
      return setError(String(0, "unexpected text \"%s\" after the expression; the only allowed suffix is [out]", p));
    return true;
  }

  // Parse the "if (param op p0) {" first line into out. Does not parse body or closing '}'.
  // Format: "if (param op operand) {" or "if (param inside(p0, p1)) {" for two-operand ops.
  bool parseIfFirstLine(const char *line, DataBlock &out)
  {
    const char *p = skipWs(line);
    G_ASSERTF(startsWithIf(p), "parseIfFirstLine called on non-if line: \"%s\"", p);
    p += 2; // skip "if"
    p = skipWs(p);
    if (*p != '(')
      return setError(String(0, "if: expected '(' after 'if', got %s (correct format: if (param op value) {)", charOrEol(*p).c_str()));
    ++p;

    String param;
    if (!readIdent(p, param))
      return setError(String("if: expected a parameter name after '(' (e.g. if (myParam > 0) {)"));
    p = skipWs(p);

    // Try named ops (inside, outside, dist) first, then symbol ops (!=, >=, <=, =, >, <).
    String compOp;
    if (!tryReadNamedCompOp(p, compOp))
    {
      static const char *SYM_OPS[] = {"!=", ">=", "<=", "=", ">", "<"};
      for (const char *sym : SYM_OPS)
      {
        const int len = (int)strlen(sym);
        if (strncmp(p, sym, len) == 0)
        {
          compOp = sym;
          p += len;
          break;
        }
      }
    }
    if (compOp.empty())
      return setError(
        String(0, "if \"%s\": expected a comparison operator (valid: =, !=, >, >=, <, <=, inside, outside, dist)", param.c_str()));

    out.setStr("param", param.c_str());
    out.setStr("op", compOp.c_str());

    if (is_comp_op_needs_p1(compOp.c_str()))
    {
      p = skipWs(p);
      if (*p != '(')
        return setError(String(0, "if \"%s\": operator \"%s\" requires two values in parentheses, e.g.: %s %s(0.0, 1.0)",
          param.c_str(), compOp.c_str(), param.c_str(), compOp.c_str()));
      ++p;
      if (!parseTwoOperands(p, out))
        return false;
    }
    else
    {
      Operand operand;
      if (!readOperand(p, operand))
        return setError(String(0, "if \"%s\": expected a value after \"%s\" (e.g. 0.5, otherParam)", param.c_str(), compOp.c_str()));
      setOperand(out, operand, "p0", "v0", "named_p0", "slot_p0");
    }

    p = skipWs(p);
    if (*p != ')')
      return setError(String(0, "if \"%s\": expected ')' to close the condition, got %s", param.c_str(), charOrEol(*p).c_str()));
    ++p;
    p = skipWs(p);
    if (*p != '{')
      return setError(
        String(0, "if \"%s\": expected '{' after the condition to open the block, got %s", param.c_str(), charOrEol(*p).c_str()));
    ++p;
    p = skipWs(p);
    if (*p)
      return setError(
        String(0, "if \"%s\": unexpected text \"%s\" after '{'; nothing should follow the opening brace", param.c_str(), p));
    return true;
  }

  // Parse one nested block (math or if) from lines[idx]; advances idx.
  bool parseNestedBlock(const Tab<String> &lines, int &idx, DataBlock &parent)
  {
    if (idx >= lines.size())
      return setError(String("unexpected end of input inside if body; expected a math line or '}'"));
    const char *trimmed = skipWs(lines[idx].c_str());
    const bool isIf = startsWithIf(trimmed);
    DataBlock *child = parent.addNewBlock(isIf ? "if" : "math");
    if (isIf)
      return parseIfBlock(lines, idx, *child);
    if (!parseMathLine(trimmed, *child))
      return false;
    ++idx;
    return true;
  }

  // Parse a full if block (first line + body + closing '}') from lines starting at idx; advances idx.
  bool parseIfBlock(const Tab<String> &lines, int &idx, DataBlock &out)
  {
    if (!parseIfFirstLine(skipWs(lines[idx].c_str()), out))
      return false;
    ++idx;
    while (idx < lines.size())
    {
      const char *trimmed = skipWs(lines[idx].c_str());
      if (*trimmed == '}')
      {
        ++idx;
        return true;
      }
      if (!parseNestedBlock(lines, idx, out))
        return false;
    }
    return setError(String("if: missing closing '}' -- every \"if (...) {\" must have a matching \"}\" on its own line"));
  }
};

static Tab<String> split_lines(const char *text)
{
  Tab<String> lines;
  const char *start = text;
  for (const char *p = text;; ++p)
  {
    if (*p == '\n' || *p == '\0')
    {
      lines.push_back(String(0, "%.*s", (int)(p - start), start));
      if (*p == '\0')
        break;
      start = p + 1;
    }
  }
  return lines;
}

// Parse the text representation of one if/math block back into a DataBlock.
// The block name ("if" or "math") is inferred from the first line.
// Returns false and fills error on parse failure.
static bool parse_if_math_text(const char *text, DataBlock &out, String &error, bool &out_is_if)
{
  Tab<String> lines = split_lines(text);
  while (!lines.empty() && IfMathParser::skipWs(lines.back().c_str())[0] == '\0')
    lines.pop_back();
  if (lines.empty())
  {
    error = "empty expression";
    return false;
  }

  IfMathParser parser;
  int idx = 0;
  const char *firstTrimmed = IfMathParser::skipWs(lines[0].c_str());
  const bool isIf = IfMathParser::startsWithIf(firstTrimmed);
  out_is_if = isIf;

  const bool parsed = isIf ? parser.parseIfBlock(lines, idx, out) : parser.parseMathLine(firstTrimmed, out);
  if (!parsed)
  {
    error = parser.error;
    return false;
  }
  if (!isIf)
    ++idx;

  // Check for unexpected trailing lines
  for (; idx < lines.size(); ++idx)
  {
    if (IfMathParser::skipWs(lines[idx].c_str())[0] != '\0')
    {
      error = String(0, "unexpected text on line %d: \"%s\"", idx + 1, lines[idx].c_str());
      logerr("%s", error.c_str());
      return false;
    }
  }
  return true;
}

static int count_if_math_fields(PropPanel::ContainerPropertyControl *panel)
{
  int count = 0;
  for (int pid = PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD; pid < PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD_SIZE; ++pid)
    if (panel->getById(pid) != nullptr)
      ++count;
  return count;
}

static int count_lines(const char *str)
{
  int count = 0;
  for (const char *p = str; *p; ++p)
    if (*p == '\n')
      ++count;
  return count;
}

void AnimTreePlugin::initIfMathFields(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings)
{
  const int ifNid = settings.getNameId("if");
  const int mathNid = settings.getNameId("math");
  int pid = PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD;
  for (int i = 0; i < settings.blockCount() && pid < PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD_SIZE; ++i)
  {
    const DataBlock *blk = settings.getBlock(i);
    if (blk->getBlockNameId() != ifNid && blk->getBlockNameId() != mathNid)
      continue;
    String error;
    String text = if_math_blk_to_string(*blk, 0, error);
    // Each serialized block ends with \n. Remove it to avoid an empty trailing line in the editbox.
    // Note: String is Tab<char> and stores the null terminator, so back() == '\0' -- use length()-based index.
    if (text.length() > 0 && text[text.length() - 1] == '\n')
      text.pop_back();

    panel->createEditBox(pid, "expression", text.c_str(), /*enabled*/ true, /*new_line*/ true,
      /*multiline*/ true, hdpi::_pxScaled(0), /*auto_height*/ true);
    panel->setDragSourceHandlerById(pid, &ifMathDragHandler);
    panel->setDropTargetHandlerById(pid, &ifMathDropHandler);
    if (!error.empty())
    {
      panel->setValueHighlightById(pid, PropPanel::ColorOverride::EDIT_BOX_WRONG_VALUE_BACKGROUND);
      panel->setTooltipId(pid, error.c_str());
      panel->setShowTooltipAlwaysById(pid, true);
    }
    ++pid;
  }
  const int ifMathCount = pid - PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD;
  panel->createButton(PID_CTRLS_PARAMS_CTRL_IF_MATH_ADD, "Add if/math");
  panel->createButton(PID_CTRLS_PARAMS_CTRL_IF_MATH_REMOVE, "Remove last", /*enabled*/ ifMathCount > 0, /*new_line*/ false);
}

// Parse and save all if/math editboxes back to settings.
// Per-field behavior:
//   - successful parse: write new block
//   - parse error: write original block (fallback)
//   - empty field with original block: write original block; if no prior error was set, mark field as invalid
//   - empty field without original block (newly added, never filled): skip
void save_if_math_fields(PropPanel::ContainerPropertyControl *panel, DataBlock &settings)
{
  const int ifNid = settings.getNameId("if");
  const int mathNid = settings.getNameId("math");

  Tab<DataBlock> origBlocks;
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    const int nid = settings.getBlock(i)->getBlockNameId();
    if (nid == ifNid || nid == mathNid)
      origBlocks.push_back(*settings.getBlock(i));
  }

  for (int i = settings.blockCount() - 1; i >= 0; --i)
  {
    const int nid = settings.getBlock(i)->getBlockNameId();
    if (nid == ifNid || nid == mathNid)
      settings.removeBlock(i);
  }

  int fieldIdx = 0;
  for (int pid = PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD; pid < PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD_SIZE; ++pid, ++fieldIdx)
  {
    if (panel->getById(pid) == nullptr)
      break;
    const SimpleString text = panel->getText(pid);
    if (text.empty())
    {
      if (fieldIdx < origBlocks.size())
      {
        settings.addNewBlock(&origBlocks[fieldIdx]);
        const char *tooltip = panel->getTooltipById(pid);
        if (!tooltip || !*tooltip)
        {
          panel->setValueHighlightById(pid, PropPanel::ColorOverride::EDIT_BOX_WRONG_VALUE_BACKGROUND);
          panel->setTooltipId(pid, "Expression can't be empty");
          panel->setShowTooltipAlwaysById(pid, true);
        }
      }
      continue;
    }
    DataBlock parseResult;
    String error;
    bool isIf = false;
    if (!parse_if_math_text(text.c_str(), parseResult, error, isIf))
    {
      logerr("paramsCtrl if/math: parse error - %s", error.c_str());
      panel->setValueHighlightById(pid, PropPanel::ColorOverride::EDIT_BOX_WRONG_VALUE_BACKGROUND);
      panel->setTooltipId(pid, String(0, "Parse error: %s", error.c_str()));
      panel->setShowTooltipAlwaysById(pid, true);
      if (fieldIdx < origBlocks.size())
        settings.addNewBlock(&origBlocks[fieldIdx]);
    }
    else
    {
      panel->setValueHighlightById(pid, PropPanel::ColorOverride::NONE);
      panel->setTooltipId(pid, "");
      panel->setShowTooltipAlwaysById(pid, false);
      settings.addNewBlock(&parseResult, isIf ? "if" : "math");
    }
  }
}

static void change_param_create_fields(PropPanel::ContainerPropertyControl *panel, const DataBlock &default_settings, bool is_editable)
{
  Tab<String> rateSrcNames{String("fixed"), String("param"), String("auto (param:CR)")};
  Tab<String> scaleSrcNames{String("none"), String("param"), String("auto (param:CRS)")};
  const RateSrc rateSrc = get_rate_src(default_settings);
  const ScaleSrc scaleSrc = get_scale_src(default_settings);
  panel->createEditBox(PID_CTRLS_PARAMS_CTRL_CHANGE_PARAM, "param", default_settings.getStr("param", ""), is_editable);
  panel->createEditFloat(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE, "changeRate", default_settings.getReal("changeRate", 0.f), 3, is_editable);
  panel->createEditFloat(PID_CTRLS_PARAMS_CTRL_CHANGE_MOD, "mod", default_settings.getReal("mod", 0.f), 3, is_editable);
  panel->createCombo(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE_SRC, "Rate source", rateSrcNames, rateSrc, is_editable);
  panel->createEditBox(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE_PARAM, "rateParam", default_settings.getStr("rateParam", ""),
    is_editable && rateSrc == RATE_SRC_PARAM);
  panel->createCombo(PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_SRC, "Scale source", scaleSrcNames, scaleSrc, is_editable);
  panel->createEditBox(PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_PARAM, "scaleParam", default_settings.getStr("scaleParam", ""),
    is_editable && scaleSrc == SCALE_SRC_PARAM);
}

static void change_param_fill_fields(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings, bool is_editable)
{
  const RateSrc rateSrc = get_rate_src(settings);
  const ScaleSrc scaleSrc = get_scale_src(settings);
  panel->setText(PID_CTRLS_PARAMS_CTRL_CHANGE_PARAM, settings.getStr("param", ""));
  panel->setFloat(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE, settings.getReal("changeRate", 0.f));
  panel->setFloat(PID_CTRLS_PARAMS_CTRL_CHANGE_MOD, settings.getReal("mod", 0.f));
  panel->setInt(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE_SRC, rateSrc);
  panel->setText(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE_PARAM, settings.getStr("rateParam", ""));
  panel->setInt(PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_SRC, scaleSrc);
  panel->setText(PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_PARAM, settings.getStr("scaleParam", ""));
  for (int pid = PID_CTRLS_PARAMS_CTRL_CHANGE_PARAM; pid <= PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_PARAM; ++pid)
    panel->setEnabledById(pid, is_editable);
  panel->setEnabledById(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE_PARAM, is_editable && rateSrc == RATE_SRC_PARAM);
  panel->setEnabledById(PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_PARAM, is_editable && scaleSrc == SCALE_SRC_PARAM);
}

static void change_param_save_fields(PropPanel::ContainerPropertyControl *panel, DataBlock &settings)
{
  const RateSrc rateSrc = static_cast<RateSrc>(panel->getInt(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE_SRC));
  const ScaleSrc scaleSrc = static_cast<ScaleSrc>(panel->getInt(PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_SRC));
  settings.setStr("param", panel->getText(PID_CTRLS_PARAMS_CTRL_CHANGE_PARAM).c_str());
  settings.setReal("changeRate", panel->getFloat(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE));
  const float mod = panel->getFloat(PID_CTRLS_PARAMS_CTRL_CHANGE_MOD);
  if (!is_equal_float(mod, 0.f))
    settings.setReal("mod", mod);
  if (rateSrc == RATE_SRC_PARAM)
    settings.setStr("rateParam", panel->getText(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE_PARAM).c_str());
  else if (rateSrc == RATE_SRC_AUTO)
    settings.setBool("variableChangeRate", true);
  if (scaleSrc == SCALE_SRC_PARAM)
    settings.setStr("scaleParam", panel->getText(PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_PARAM).c_str());
  else if (scaleSrc == SCALE_SRC_AUTO)
    settings.setBool("variableScale", true);
}

static int count_param_remap_mappings(PropPanel::ContainerPropertyControl *group)
{
  int count = 0;
  for (int pid = PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD; pid < PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD_SIZE; ++pid)
    if (group->getById(pid) != nullptr)
      ++count;
  return count;
}

static void init_param_remap_mappings(PropPanel::ContainerPropertyControl *group, const DataBlock &settings, bool is_editable)
{
  const int mappingNid = settings.getNameId("paramMapping");
  int pid = PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD;
  auto add_mapping = [&](Point2 value) {
    if (pid >= PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD_SIZE)
      return;
    group->createPoint2(pid, "paramMapping", value, /*prec*/ 2, is_editable);
    group->moveById(pid, PID_CTRLS_PARAMS_CTRL_REMAP_ADD_MAPPING);
    ++pid;
  };
  for (int i = 0; i < settings.paramCount(); ++i)
  {
    if (settings.getParamNameId(i) != mappingNid)
      continue;
    if (settings.getParamType(i) == DataBlock::TYPE_POINT4)
    {
      const Point4 p4 = settings.getPoint4(i);
      add_mapping(Point2(p4.x, p4.y));
      add_mapping(Point2(p4.z, p4.w));
    }
    else if (settings.getParamType(i) == DataBlock::TYPE_POINT2)
    {
      add_mapping(settings.getPoint2(i));
    }
  }
  const int count = pid - PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD;
  group->setEnabledById(PID_CTRLS_PARAMS_CTRL_REMAP_REMOVE_MAPPING, is_editable && count > 0);
}

static void param_remap_create_fields(PropPanel::ContainerPropertyControl *group, const DataBlock &default_settings, bool is_editable)
{
  group->createEditBox(PID_CTRLS_PARAMS_CTRL_REMAP_PARAM, "param", default_settings.getStr("param", ""), is_editable);
  group->createEditBox(PID_CTRLS_PARAMS_CTRL_REMAP_DEST_PARAM, "destParam", default_settings.getStr("destParam", ""), is_editable);
  group->createButton(PID_CTRLS_PARAMS_CTRL_REMAP_ADD_MAPPING, "Add mapping", is_editable);
  group->createButton(PID_CTRLS_PARAMS_CTRL_REMAP_REMOVE_MAPPING, "Remove last mapping", /*enabled*/ false, /*new_line*/ false);
  init_param_remap_mappings(group, default_settings, is_editable);
}

static void param_remap_fill_fields(PropPanel::ContainerPropertyControl *group, const DataBlock &settings, bool is_editable)
{
  group->setText(PID_CTRLS_PARAMS_CTRL_REMAP_PARAM, settings.getStr("param", ""));
  group->setText(PID_CTRLS_PARAMS_CTRL_REMAP_DEST_PARAM, settings.getStr("destParam", ""));
  group->setEnabledById(PID_CTRLS_PARAMS_CTRL_REMAP_PARAM, is_editable);
  group->setEnabledById(PID_CTRLS_PARAMS_CTRL_REMAP_DEST_PARAM, is_editable);
  group->setEnabledById(PID_CTRLS_PARAMS_CTRL_REMAP_ADD_MAPPING, is_editable);
  remove_fields(group, PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD, PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD_SIZE);
  init_param_remap_mappings(group, settings, is_editable);
}

static void param_remap_save_fields(PropPanel::ContainerPropertyControl *panel, DataBlock &settings)
{
  settings.setStr("param", panel->getText(PID_CTRLS_PARAMS_CTRL_REMAP_PARAM).c_str());
  settings.setStr("destParam", panel->getText(PID_CTRLS_PARAMS_CTRL_REMAP_DEST_PARAM).c_str());
  for (int pid = PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD; pid < PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD_SIZE; ++pid)
  {
    if (panel->getById(pid) == nullptr)
      break;
    settings.addPoint2("paramMapping", panel->getPoint2(pid));
  }
}

static BlockType get_block_type(const DataBlock *blk)
{
  if (!blk)
    return BLOCK_TYPE_CHANGE_PARAM;
  const int changeParamNid = blk->getNameId("changeParam");
  return blk->getBlockNameId() == changeParamNid ? BLOCK_TYPE_CHANGE_PARAM : BLOCK_TYPE_PARAM_REMAP;
}

static BlockType get_selected_block_type(PropPanel::ContainerPropertyControl *panel)
{
  return static_cast<BlockType>(panel->getInt(PID_CTRLS_PARAMS_CTRL_BLOCK_TYPE_COMBO_SELECT));
}

void params_ctrl_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
}

void AnimTreePlugin::paramsCtrlInitBlockSettings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> names;
  const int changeParamNid = settings->getNameId("changeParam");
  const int paramRemapNid = settings->getNameId("paramRemap");
  const DataBlock *firstBlock = nullptr;

  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *block = settings->getBlock(i);
    if (block->getBlockNameId() == changeParamNid || block->getBlockNameId() == paramRemapNid)
    {
      names.emplace_back(block->getStr("param", ""));
      if (!firstBlock)
        firstBlock = block;
    }
  }

  const bool isEditable = !names.empty();
  const BlockType firstBlockType = get_block_type(firstBlock);
  Tab<String> blockTypeNames{String("changeParam"), String("paramRemap")};
  initIfMathFields(panel, *settings);
  panel->createList(PID_CTRLS_NODES_LIST, "Params", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createCombo(PID_CTRLS_PARAMS_CTRL_BLOCK_TYPE_COMBO_SELECT, "Block type", blockTypeNames, firstBlockType, isEditable);

  if (firstBlockType == BLOCK_TYPE_CHANGE_PARAM)
    change_param_create_fields(panel, *settings->getBlockByNameEx("changeParam"), isEditable);
  else
    param_remap_create_fields(panel, *settings->getBlockByNameEx("paramRemap"), isEditable);
}

void params_ctrl_set_selected_change_param(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedSettings = &DataBlock::emptyBlock;
  const int changeParamNid = settings->getNameId("changeParam");
  const int paramRemapNid = settings->getNameId("paramRemap");

  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *block = settings->getBlock(i);
    if ((block->getBlockNameId() == changeParamNid || block->getBlockNameId() == paramRemapNid) &&
        name == block->getStr("param", nullptr))
    {
      selectedSettings = block;
      break;
    }
  }

  const bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  const BlockType selectedSettingsType = get_block_type(selectedSettings);
  const bool typeChanged = get_selected_block_type(panel) != selectedSettingsType;

  panel->setInt(PID_CTRLS_PARAMS_CTRL_BLOCK_TYPE_COMBO_SELECT, selectedSettingsType);
  panel->setEnabledById(PID_CTRLS_PARAMS_CTRL_BLOCK_TYPE_COMBO_SELECT, isEditable);

  if (typeChanged)
  {
    PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
    remove_fields(panel, PID_CTRLS_PARAMS_CTRL_CHANGE_PARAM, PID_CTRLS_ACTION_FIELD_SIZE);
    if (selectedSettingsType == BLOCK_TYPE_CHANGE_PARAM)
      change_param_create_fields(group, *selectedSettings, isEditable);
    else
      param_remap_create_fields(group, *selectedSettings, isEditable);
    group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save");
  }
  else if (selectedSettingsType == BLOCK_TYPE_CHANGE_PARAM)
    change_param_fill_fields(panel, *selectedSettings, isEditable);
  else
  {
    PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
    param_remap_fill_fields(group, *selectedSettings, isEditable);
  }
}

void params_ctrl_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  save_if_math_fields(panel, *settings);

  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  const BlockType blockType = get_selected_block_type(panel);
  const int changeParamNid = settings->getNameId("changeParam");
  const int paramRemapNid = settings->getNameId("paramRemap");
  DataBlock *selectedSettings = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *block = settings->getBlock(i);
    if ((block->getBlockNameId() == changeParamNid || block->getBlockNameId() == paramRemapNid) &&
        listName == block->getStr("param", nullptr))
    {
      selectedSettings = block;
      break;
    }
  }
  if (!selectedSettings)
    selectedSettings = settings->addNewBlock(blockType == BLOCK_TYPE_CHANGE_PARAM ? "changeParam" : "paramRemap");
  for (int i = selectedSettings->paramCount() - 1; i >= 0; --i)
    selectedSettings->removeParam(i);

  if (blockType == BLOCK_TYPE_CHANGE_PARAM)
    change_param_save_fields(panel, *selectedSettings);
  else
    param_remap_save_fields(panel, *selectedSettings);

  const int paramPid = blockType == BLOCK_TYPE_CHANGE_PARAM ? PID_CTRLS_PARAMS_CTRL_CHANGE_PARAM : PID_CTRLS_PARAMS_CTRL_REMAP_PARAM;
  const SimpleString param = panel->getText(paramPid);
  if (listName != param)
    panel->setText(PID_CTRLS_NODES_LIST, param.c_str());
}

void params_ctrl_remove_change_param(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  const int changeParamNid = settings->getNameId("changeParam");
  const int paramRemapNid = settings->getNameId("paramRemap");
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *block = settings->getBlock(i);
    if ((block->getBlockNameId() == changeParamNid || block->getBlockNameId() == paramRemapNid) &&
        removeName == block->getStr("param", nullptr))
    {
      settings->removeBlock(i);
      break;
    }
  }
}

void params_ctrl_change_rate_src_changed(PropPanel::ContainerPropertyControl *panel)
{
  const RateSrc rateSrc = static_cast<RateSrc>(panel->getInt(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE_SRC));
  panel->setEnabledById(PID_CTRLS_PARAMS_CTRL_CHANGE_RATE_PARAM, rateSrc == RATE_SRC_PARAM);
}

void params_ctrl_change_scale_src_changed(PropPanel::ContainerPropertyControl *panel)
{
  const ScaleSrc scaleSrc = static_cast<ScaleSrc>(panel->getInt(PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_SRC));
  panel->setEnabledById(PID_CTRLS_PARAMS_CTRL_CHANGE_SCALE_PARAM, scaleSrc == SCALE_SRC_PARAM);
}

void params_ctrl_block_type_changed(PropPanel::ContainerPropertyControl *group, const DataBlock *settings)
{
  const bool isEditable = group->getInt(PID_CTRLS_NODES_LIST) >= 0;
  const BlockType selectedSettingsType = get_selected_block_type(group);
  const SimpleString name = group->getText(PID_CTRLS_NODES_LIST);
  const int changeParamNid = settings->getNameId("changeParam");
  const int paramRemapNid = settings->getNameId("paramRemap");
  const int targetNid = selectedSettingsType == BLOCK_TYPE_PARAM_REMAP ? paramRemapNid : changeParamNid;

  const DataBlock *selectedSettings = &DataBlock::emptyBlock;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *block = settings->getBlock(i);
    if (block->getBlockNameId() == targetNid && name == block->getStr("param", nullptr))
    {
      selectedSettings = block;
      break;
    }
  }

  if (selectedSettingsType == BLOCK_TYPE_CHANGE_PARAM)
    change_param_create_fields(group, *selectedSettings, isEditable);
  else
    param_remap_create_fields(group, *selectedSettings, isEditable);
}

void params_ctrl_add_mapping(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  const int count = count_param_remap_mappings(group);
  if (count >= PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD_SIZE - PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD)
    return;
  const int pid = PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD + count;
  group->createPoint2(pid, "paramMapping");
  group->moveById(pid, PID_CTRLS_PARAMS_CTRL_REMAP_ADD_MAPPING);
  group->setEnabledById(PID_CTRLS_PARAMS_CTRL_REMAP_REMOVE_MAPPING, true);
}

void params_ctrl_remove_mapping(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  for (int pid = PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD_SIZE - 1; pid >= PID_CTRLS_PARAMS_CTRL_REMAP_MAPPING_FIELD; --pid)
  {
    if (group->getById(pid) != nullptr)
    {
      group->removeById(pid);
      const int count = count_param_remap_mappings(group);
      group->setEnabledById(PID_CTRLS_PARAMS_CTRL_REMAP_REMOVE_MAPPING, count > 0);
      return;
    }
  }
}

void AnimTreePlugin::paramsCtrlAddIfMath(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  int nextPid = PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD;
  while (nextPid < PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD_SIZE && group->getById(nextPid) != nullptr)
    ++nextPid;
  if (nextPid >= PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD_SIZE)
  {
    logerr("paramsCtrl: if/math fields limit (%d) reached",
      PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD_SIZE - PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD);
    return;
  }
  group->createEditBox(nextPid, "expression", "", /*enabled*/ true, /*new_line*/ true,
    /*multiline*/ true, hdpi::_pxScaled(0), /*auto_height*/ true);
  group->setDragSourceHandlerById(nextPid, &ifMathDragHandler);
  group->setDropTargetHandlerById(nextPid, &ifMathDropHandler);
  group->moveById(nextPid, PID_CTRLS_PARAMS_CTRL_IF_MATH_ADD, /*after*/ false);
  group->setEnabledById(PID_CTRLS_PARAMS_CTRL_IF_MATH_REMOVE, true);
}

static bool params_ctrl_remove_if_math(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const int uiCountBefore = count_if_math_fields(panel);
  for (int pid = PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD_SIZE - 1; pid >= PID_CTRLS_PARAMS_CTRL_IF_MATH_FIELD; --pid)
  {
    if (panel->getById(pid) == nullptr)
      continue;
    panel->removeById(pid);
    panel->setEnabledById(PID_CTRLS_PARAMS_CTRL_IF_MATH_REMOVE, count_if_math_fields(panel) > 0);
    break;
  }

  if (!settings)
    return false;

  const int ifNid = settings->getNameId("if");
  const int mathNid = settings->getNameId("math");
  int blkCount = 0;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const int nid = settings->getBlock(i)->getBlockNameId();
    if (nid == ifNid || nid == mathNid)
      ++blkCount;
  }

  // If uiCountBefore > blkCount the removed field was newly added (not yet saved) -- nothing to remove from settings.
  if (uiCountBefore > blkCount)
    return false;

  for (int i = settings->blockCount() - 1; i >= 0; --i)
  {
    const int nid = settings->getBlock(i)->getBlockNameId();
    if (nid == ifNid || nid == mathNid)
    {
      settings->removeBlock(i);
      return true;
    }
  }
  return false;
}

void AnimTreePlugin::paramsCtrlRemoveIfMath(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;
  TLeafHandle parent = tree->getParentLeaf(leaf);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);
  if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
    if (DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf)))
      if (params_ctrl_remove_if_math(panel, settings))
        saveProps(props, fullPath);
}

void AnimTreePlugin::changeParamsCtrlBlockType(PropPanel::ContainerPropertyControl *panel)
{
  remove_fields(panel, PID_CTRLS_PARAMS_CTRL_CHANGE_PARAM, PID_CTRLS_ACTION_FIELD_SIZE);
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;
  TLeafHandle parent = tree->getParentLeaf(leaf);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);
  if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
    if (DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf)))
      params_ctrl_block_type_changed(group, settings);
  group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save");
}
