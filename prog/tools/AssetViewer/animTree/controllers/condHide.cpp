// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "condHide.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"
#include "../animTreePanelPids.h"
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_lookup.h>
#include <propPanel/control/container.h>

static const char *COMP_OPS[] = {"=", "!=", ">", ">=", "<", "<=", "inside", "outside", "dist"};
static const char *TWO_CHAR_OPS[] = {"!=", ">=", "<="};
static const char *ONE_CHAR_OPS[] = {"=", ">", "<"};

static bool is_comp_op(const char *op) { return lup(op, COMP_OPS, countof(COMP_OPS), -1) >= 0; }

static String condition_error(const char *node_name, const String &msg, String &out_error)
{
  logerr("condHide targetNode '%s': invalid condition - %s", node_name, msg.c_str());
  if (out_error.empty())
    out_error = msg;
  return String();
}

static String condition_blk_to_string(const DataBlock &blk, const char *node_name, String &out_error, bool nested = false)
{
  if (blk.isEmpty())
    return String();

  const char *opStr = blk.getStr("op", nullptr);
  if (!opStr || !*opStr)
    return condition_error(node_name,
      String("missing \"op\" field - expected a comparison (=, !=, >, >=, <, <=, inside, outside, dist) or boolean (|, &, !)"),
      out_error);

  if (is_comp_op(opStr))
  {
    const char *param = blk.getStr("param", nullptr);
    if (!param || !*param)
      return condition_error(node_name, String(0, "op \"%s\" requires a \"param\" field with the name of an linear parameter", opStr),
        out_error);
    float p0 = blk.getReal("p0", 0.f);
    if (is_comp_op_needs_p1(opStr))
    {
      float p1 = blk.getReal("p1", 0.f);
      return String(0, "%s %s(%g, %g)", param, opStr, p0, p1);
    }
    return String(0, "%s %s %g", param, opStr, p0);
  }

  if (strcmp(opStr, "!") == 0)
  {
    const DataBlock *p0 = blk.getBlockByName("p0");
    if (!p0)
      return condition_error(node_name, String("op \"!\" requires a \"p0\" sub-block containing the condition to negate"), out_error);
    return String(0, "!(%s)", condition_blk_to_string(*p0, node_name, out_error).c_str());
  }

  if (strcmp(opStr, "|") == 0 || strcmp(opStr, "&") == 0)
  {
    Tab<String> parts;
    for (int i = 0; const DataBlock *child = blk.getBlockByName(String(0, "p%d", i).str()); ++i)
      parts.push_back(condition_blk_to_string(*child, node_name, out_error, /*nested*/ true));
    if (parts.size() < 2)
      return condition_error(node_name,
        String(0, "op \"%s\" requires at least 2 sub-blocks (p0, p1, ...) with conditions, but only %d found", opStr, parts.size()),
        out_error);

    String result(nested ? "(" : "");
    for (int j = 0; j < parts.size(); ++j)
    {
      if (j > 0)
        result += String(0, " %s ", opStr);
      result += parts[j];
    }
    if (nested)
      result += ")";
    return result;
  }

  return condition_error(node_name,
    String(0, "unknown op \"%s\" - expected a comparison (=, !=, >, >=, <, <=, inside, outside, dist) or boolean (|, &, !)", opStr),
    out_error);
}

static bool is_blank(const char *s)
{
  if (!s)
    return true;
  while (*s)
    if (!isspace((unsigned char)*s++))
      return false;
  return true;
}

static void copy_condition_blk(const DataBlock &src, DataBlock &dst)
{
  const char *op = src.getStr("op", nullptr);
  if (!op)
    return;
  dst.setStr("op", op);
  if (is_comp_op(op))
  {
    dst.setStr("param", src.getStr("param", ""));
    dst.setReal("p0", src.getReal("p0", 0.f));
    if (is_comp_op_needs_p1(op))
      dst.setReal("p1", src.getReal("p1", 0.f));
  }
  else if (strcmp(op, "!") == 0)
  {
    const DataBlock *p0 = src.getBlockByName("p0");
    if (p0)
      copy_condition_blk(*p0, *dst.addNewBlock("p0"));
  }
  else
  {
    for (int i = 0; const DataBlock *child = src.getBlockByName(String(0, "p%d", i).str()); ++i)
      copy_condition_blk(*child, *dst.addNewBlock(String(0, "p%d", i).str()));
  }
}

struct CondParser
{
  const char *pos;
  String error;

  void skipWs()
  {
    while (*pos && isspace((unsigned char)*pos))
      ++pos;
  }

  bool consume(char c)
  {
    skipWs();
    if (*pos != c)
      return false;

    ++pos;
    return true;
  }

  bool readIdent(String &out)
  {
    skipWs();
    const char *begin = pos;
    while (*pos && !isspace((unsigned char)*pos) && !strchr("(),.!|&=<>", *pos))
      ++pos;

    if (pos == begin)
      return false;

    out = String(0, "%.*s", (int)(pos - begin), begin);
    return true;
  }

  bool readFloat(float &out)
  {
    skipWs();
    char *end = nullptr;
    out = strtof(pos, &end);
    if (!end || end == pos)
      return false;

    pos = end;
    return true;
  }

  bool readCompOp(String &out)
  {
    skipWs();
    for (const char *op : TWO_CHAR_OPS)
      if (pos[0] == op[0] && pos[1] == op[1])
      {
        out = op;
        pos += 2;
        return true;
      }
    for (const char *op : ONE_CHAR_OPS)
      if (pos[0] == op[0])
      {
        out = op;
        ++pos;
        return true;
      }
    const char *saved = pos;
    String ident;
    if (readIdent(ident) && is_comp_op(ident.c_str()))
    {
      out = ident;
      return true;
    }
    pos = saved;
    return false;
  }

  // Called after boolOpChar has been consumed. Writes op/p0/p1/... into out.
  bool collectCompound(DataBlock &out, DataBlock &first, char boolOpChar)
  {
    out.setStr("op", boolOpChar == '|' ? "|" : "&");
    copy_condition_blk(first, *out.addNewBlock("p0"));
    int idx = 1;
    while (true)
    {
      DataBlock *child = out.addNewBlock(String(0, "p%d", idx++).str());
      if (!parseExpr(*child))
        return false;
      skipWs();
      if (*pos != boolOpChar)
        break;
      ++pos;
    }
    return true;
  }

  bool parseExpr(DataBlock &out)
  {
    skipWs();
    if (*pos == '(')
      return parseCompound(out);
    if (*pos == '!')
      return parseNot(out);
    return parseLeaf(out);
  }

  bool parseLeaf(DataBlock &out)
  {
    String param;
    if (!readIdent(param))
    {
      skipWs();
      if (*pos)
        error = String(0, "expected parameter name, got '%c'", *pos);
      else
        error = String("expected parameter name but reached end of input");
      return false;
    }
    String op;
    if (!readCompOp(op))
    {
      skipWs();
      if (*pos)
        error = String(0, "expected comparison operator after \"%s\" (one of: =, !=, >, >=, <, <=, inside, outside, dist), got '%c'",
          param.c_str(), *pos);
      else
        error = String(0, "expected comparison operator after \"%s\" but reached end of input", param.c_str());
      return false;
    }
    out.setStr("param", param.c_str());
    out.setStr("op", op.c_str());
    if (is_comp_op_needs_p1(op.c_str()))
    {
      if (!consume('('))
      {
        skipWs();
        error =
          String(0, "op \"%s\" requires syntax: param %s(p0, p1) - expected '(', got '%c'", op.c_str(), op.c_str(), *pos ? *pos : '?');
        return false;
      }
      float p0 = 0.f;
      if (!readFloat(p0))
      {
        error = String(0, "op \"%s\": expected number for p0 inside parentheses", op.c_str());
        return false;
      }
      if (!consume(','))
      {
        skipWs();
        error = String(0, "op \"%s\": expected ',' between p0 and p1, got '%c'", op.c_str(), *pos ? *pos : '?');
        return false;
      }
      float p1 = 0.f;
      if (!readFloat(p1))
      {
        error = String(0, "op \"%s\": expected number for p1 after ','", op.c_str());
        return false;
      }
      if (!consume(')'))
      {
        skipWs();
        error = String(0, "op \"%s\": expected ')' to close (p0, p1), got '%c'", op.c_str(), *pos ? *pos : '?');
        return false;
      }
      out.setReal("p0", p0);
      out.setReal("p1", p1);
    }
    else
    {
      float p0 = 0.f;
      if (!readFloat(p0))
      {
        skipWs();
        if (*pos)
          error = String(0, "op \"%s\": expected number after the operator, got '%c'", op.c_str(), *pos);
        else
          error = String(0, "op \"%s\": expected number after the operator but reached end of input", op.c_str());
        return false;
      }
      out.setReal("p0", p0);
    }
    return true;
  }

  bool parseNot(DataBlock &out)
  {
    G_ASSERT(*pos == '!');
    ++pos;
    if (!consume('('))
    {
      skipWs();
      error = String(0, "op \"!\": expected '(' immediately after '!', got '%c'", *pos ? *pos : '?');
      return false;
    }
    out.setStr("op", "!");
    DataBlock *p0 = out.addNewBlock("p0");
    if (!parseExpr(*p0))
      return false;
    if (!consume(')'))
    {
      skipWs();
      error = String(0, "op \"!\": expected ')' to close the negated expression, got '%c'", *pos ? *pos : '?');
      return false;
    }
    return true;
  }

  bool parseCompound(DataBlock &out)
  {
    G_ASSERT(*pos == '(');
    ++pos;
    DataBlock first;
    if (!parseExpr(first))
      return false;

    skipWs();
    if (*pos == ')')
    {
      ++pos;
      copy_condition_blk(first, out);
      return true;
    }
    if (*pos != '|' && *pos != '&')
    {
      if (*pos)
        error = String(0, "expected '|' or '&' after first sub-expression, got '%c'", *pos);
      else
        error = String("unexpected end of expression after first sub-expression");
      return false;
    }
    char boolOpChar = *pos++;
    if (!collectCompound(out, first, boolOpChar))
      return false;

    if (!consume(')'))
    {
      skipWs();
      if (*pos)
        error = String(0, "expected ')' or '%c' in compound expression, got '%c'", boolOpChar, *pos);
      else
        error = String("unexpected end of compound expression, expected ')'");
      return false;
    }
    return true;
  }
};

static bool condition_string_to_blk(const char *str, DataBlock &out, String &error)
{
  CondParser parser;
  parser.pos = str;

  DataBlock first;
  if (!parser.parseExpr(first))
  {
    error = parser.error;
    return false;
  }

  parser.skipWs();
  const char boolOpChar = *parser.pos;
  if (boolOpChar != '|' && boolOpChar != '&')
  {
    if (*parser.pos)
    {
      error = String(0, "unexpected text after condition: \"%s\"", parser.pos);
      return false;
    }
    copy_condition_blk(first, out);
    return true;
  }

  ++parser.pos; // consume boolOpChar
  if (!parser.collectCompound(out, first, boolOpChar))
  {
    error = parser.error;
    return false;
  }

  parser.skipWs();
  if (*parser.pos)
  {
    error = String(0, "unexpected text after condition: \"%s\"", parser.pos);
    return false;
  }
  return true;
}

void cond_hide_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
}

void cond_hide_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> names;
  const int targetNodeNid = settings->getNameId("targetNode");
  for (int i = 0; i < settings->blockCount(); ++i)
    if (settings->getBlock(i)->getBlockNameId() == targetNodeNid)
      names.emplace_back(settings->getBlock(i)->getStr("node", ""));

  bool isEditable = !names.empty();
  const DataBlock *defaultBlock = settings->getBlockByNameEx("targetNode");
  const bool isAlways = defaultBlock->getBool("always", false);
  panel->createList(PID_CTRLS_NODES_LIST, "Target nodes", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add target node");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_COND_HIDE_NODE, "node", defaultBlock->getStr("node", ""), isEditable);
  panel->createCheckBox(PID_CTRLS_COND_HIDE_ALWAYS, "always", isAlways, isEditable);
  panel->createCheckBox(PID_CTRLS_COND_HIDE_HIDE_ONLY, "hideOnly", defaultBlock->getBool("hideOnly", false), isEditable);
  const char *nodeName = defaultBlock->getStr("node", "");
  String condError;
  const String condStr = (isEditable && !isAlways) ? condition_blk_to_string(*defaultBlock, nodeName, condError) : String();
  panel->createEditBox(PID_CTRLS_COND_HIDE_CONDITION, "condition", condStr, isEditable && !isAlways);
  panel->setTooltipId(PID_CTRLS_COND_HIDE_CONDITION, condError.c_str());
}

void cond_hide_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  const int targetNodeNid = settings->getNameId("targetNode");
  DataBlock *selectedBlock = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *child = settings->getBlock(i);
    if (child->getBlockNameId() == targetNodeNid && listName == child->getStr("node", nullptr))
      selectedBlock = child;
  }

  const SimpleString nodeValue = panel->getText(PID_CTRLS_COND_HIDE_NODE);
  const bool alwaysValue = panel->getBool(PID_CTRLS_COND_HIDE_ALWAYS);
  const bool hideOnlyValue = panel->getBool(PID_CTRLS_COND_HIDE_HIDE_ONLY);
  const SimpleString condStr = panel->getText(PID_CTRLS_COND_HIDE_CONDITION);
  if (!selectedBlock)
    selectedBlock = settings->addNewBlock("targetNode");

  // Parse condition before modifying selectedBlock so savedCondStr is still valid on error
  DataBlock condBlock;
  String parseError;
  const bool condEmpty = is_blank(condStr.c_str());
  const bool condOk = condEmpty || condition_string_to_blk(condStr.c_str(), condBlock, parseError);

  if (condOk)
  {
    for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
      selectedBlock->removeParam(i);
    for (int i = selectedBlock->blockCount() - 1; i >= 0; --i)
      selectedBlock->removeBlock(i);
  }
  else
  {
    String condError;
    const String savedCondStr = condition_blk_to_string(*selectedBlock, selectedBlock->getStr("node", ""), condError);
    const String tooltipMsg = String(0, "Parse error: %s\nLast saved condition: %s", parseError.c_str(), savedCondStr.c_str());
    panel->setValueHighlightById(PID_CTRLS_COND_HIDE_CONDITION, PropPanel::ColorOverride::EDIT_BOX_WRONG_VALUE_BACKGROUND);
    panel->setTooltipId(PID_CTRLS_COND_HIDE_CONDITION, tooltipMsg.c_str());
    logerr("condHide targetNode '%s': condition parse error - %s", nodeValue.c_str(), parseError.c_str());
    panel->setShowTooltipAlwaysById(PID_CTRLS_COND_HIDE_CONDITION, true);
    selectedBlock->removeParam("always");
    selectedBlock->removeParam("hideOnly");
  }

  selectedBlock->setStr("node", nodeValue.c_str());
  if (alwaysValue)
    selectedBlock->setBool("always", alwaysValue);
  if (hideOnlyValue)
    selectedBlock->setBool("hideOnly", hideOnlyValue);

  if (condOk)
  {
    if (!condEmpty)
      copy_condition_blk(condBlock, *selectedBlock);
    panel->setValueHighlightById(PID_CTRLS_COND_HIDE_CONDITION, PropPanel::ColorOverride::NONE);
    panel->setTooltipId(PID_CTRLS_COND_HIDE_CONDITION, "");
  }

  if (listName != nodeValue)
    panel->setText(PID_CTRLS_NODES_LIST, nodeValue.c_str());
}

void cond_hide_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  const int targetNodeNid = settings->getNameId("targetNode");
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (child->getBlockNameId() == targetNodeNid && name == child->getStr("node", nullptr))
      selectedBlock = child;
  }
  panel->setText(PID_CTRLS_COND_HIDE_NODE, name);
  const bool isAlways = selectedBlock->getBool("always", false);
  panel->setBool(PID_CTRLS_COND_HIDE_ALWAYS, isAlways);
  panel->setBool(PID_CTRLS_COND_HIDE_HIDE_ONLY, selectedBlock->getBool("hideOnly", false));
  const char *nodeName = selectedBlock->getStr("node", "");
  String condError;
  const String condStr = isAlways ? String() : condition_blk_to_string(*selectedBlock, nodeName, condError);
  panel->setText(PID_CTRLS_COND_HIDE_CONDITION, condStr);
  panel->setTooltipId(PID_CTRLS_COND_HIDE_CONDITION, condError.c_str());

  const bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  for (int i = PID_CTRLS_COND_HIDE_NODE; i <= PID_CTRLS_COND_HIDE_HIDE_ONLY; ++i)
    panel->setEnabledById(i, isEditable);
  panel->setEnabledById(PID_CTRLS_COND_HIDE_CONDITION, isEditable && !isAlways);
}

void cond_hide_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  const int targetNodeNid = settings->getNameId("targetNode");
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (child->getBlockNameId() == targetNodeNid && removeName == child->getStr("node", nullptr))
    {
      settings->removeBlock(i);
      break;
    }
  }
}
