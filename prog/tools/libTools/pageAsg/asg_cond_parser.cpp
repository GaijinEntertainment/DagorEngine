#include "asg_cond_parser.h"
#include <debug/dag_debug.h>
#include <ctype.h>

char *str_to_valid_id(const char *name)
{
  static const char *remap[] = {"%_pct_", ":_semi_", ",_col_", "._dot_", "\n_ln_", "\t", " ", "\r"};
  static const int remap_sz = sizeof(remap) / sizeof(char *);

  static char id[256];
  int len = 0;

  for (; *name && len < sizeof(id) - 8; name++)
    if (isalpha(*name) || *name == '_')
      id[len++] = *name;
    else if (isdigit(*name))
    {
      if (len == 0)
        id[len++] = '_';
      id[len++] = *name;
    }
    else
    {
      bool found = false;
      for (int j = 0; j < remap_sz; j++)
        if (*name == remap[j][0])
        {
          int rlen = (int)strlen(&remap[j][1]);
          memcpy(id + len, &remap[j][1], rlen);
          len += rlen;
          found = true;
          break;
        }
      if (!found)
        id[len++] = '_';
    }

  id[len] = 0;
  return id;
}


AsgConditionStringParser::AsgConditionStringParser() : order(tmpmem), vars(tmpmem), otherCode(tmpmem) {}
AsgConditionStringParser::AsgConditionStringParser(const char *cond_str) : order(tmpmem), vars(tmpmem), otherCode(tmpmem)
{
  parse(cond_str);
}

void AsgConditionStringParser::clear()
{
  order.clear();
  vars.clear();
  otherCode.clear();
}

void AsgConditionStringParser::parse(const char *cond_str)
{
  if (!cond_str)
    return;
  const char *p = cond_str;
  const char *p2;
  const char *p3;
  const char *p4;

  p2 = strstr(p, "$(");
  while (p2)
  {
    if (p2 > p)
      addOtherCode(p, p2 - p);

    p2 += 2;
    p3 = strchr(p2, ')');
    if (p3 < p2 + 3)
    {
      debug_ctx("var usage error at %d in %s", p2 - cond_str, cond_str);
      return;
    }
    if (p2[1] != ':')
    {
      debug_ctx("var usage format error at %d in %s", p2 - cond_str, cond_str);
      return;
    }

    Var v;
    switch (*p2)
    {
      case Var::TYPE_Bit: // boolean - bit of Int param
      case Var::TYPE_BitOld:
        v.type = *p2;
        v.name = (v.type == Var::TYPE_Bit) ? "graphControlFlags" : "graphPrevControlFlags";
        v.flagName.printf(p3 - p2, "%.*s", p3 - p2 - 2, p2 + 2); //-V769
        break;
      case Var::TYPE_Scalar: // scalar  - Real param
      case Var::TYPE_Timer:  // time    - Real param with auto increment
      case Var::TYPE_Int:    // counter - Int param
        v.type = *p2;
        v.name.printf(p3 - p2, "%.*s", p3 - p2 - 2, p2 + 2);
        break;
      default: debug_ctx("unknown var type \'%c\' at %d in %s", *p2, p2 - cond_str, cond_str); return;
    }

    Piece pc;
    pc.idx = vars.size();
    vars.push_back(v);
    pc.var = true;
    order.push_back(pc);

    p = p3 + 1;
    p2 = strstr(p, "$(");
  }

  if (p[0])
    addOtherCode(p, -1);
}
void AsgConditionStringParser::addOtherCode(const char *str, int len)
{
  Piece pc;
  pc.idx = otherCode.size();
  otherCode.push_back(len != -1 ? String(len + 2, "%.*s", len, str) : String(str));
  pc.var = false;
  order.push_back(pc);
}

void AsgConditionStringParser::dump()
{
  debug_ctx("results of parsing");
  for (int i = 0; i < order.size(); i++)
  {
    int idx = order[i].idx;
    debug("  %d:  idx=%d  var=%d", i, order[i].idx, order[i].var);
    if (order[i].var)
      debug("    var:  type=%c  name=%s  flagName=%s", vars[idx].type, vars[idx].name.str(), vars[idx].flagName.str());
    else
      debug("    code: %s", otherCode[idx].str());
  }
}


AsgVarsList::AsgVarsList() : varType(tmpmem), realName(tmpmem) {}

bool AsgVarsList::registerVar(const char *_name, int type)
{
  const char *name = str_to_valid_id(_name);
  int id = varName.getNameId(name);

  if (id != -1)
  {
    if (varType[id] == type)
      return true;
    else
      return false;
  }

  id = varName.addNameId(name);
  if (id >= varType.size())
    varType.resize(id + 1);
  if (id >= realName.size())
    realName.resize(id + 1);
  varType[id] = type;
  realName[id] = _name;
  return true;
}

void AsgVarsList::implement_ParamIdDeclaration(FILE *fp) const
{
  if (!varName.nameCount())
    return;

  fprintf(fp, ""
              "  struct {\n"
              ""
              "    int ");
  for (int i = 0; i < varName.nameCount(); i++)
  {
    fprintf(fp, "%s", varName.getName(i));
    if (i + 1 < varName.nameCount())
      fprintf(fp, ""
                  ",\n"
                  ""
                  "        ");
  }
  fprintf(fp, ""
              ";\n  } paramId;\n\n");
}

void AsgVarsList::implement_ParamIdInit(FILE *fp, const char *translator) const
{
  if (!varName.nameCount())
    return;

  for (int i = 0; i < varName.nameCount(); i++)
  {
    fprintf(fp, "    paramId.%s = %s->addParamId(anim, \"%s\", ", varName.getName(i), translator, realName[i].str());

    switch (varType[i])
    {
      case AsgConditionStringParser::Var::TYPE_Bit:
      case AsgConditionStringParser::Var::TYPE_BitOld:
      case AsgConditionStringParser::Var::TYPE_Int: fprintf(fp, "AnimV20::IPureAnimStateHolder::PT_ScalarParamInt"); break;
      case AsgConditionStringParser::Var::TYPE_Scalar: fprintf(fp, "AnimV20::IPureAnimStateHolder::PT_ScalarParam"); break;
      case AsgConditionStringParser::Var::TYPE_Timer: fprintf(fp, "AnimV20::IPureAnimStateHolder::PT_TimeParam"); break;
      default: debug_ctx("unknown var type \'%c\'", varType[i]); return;
    }
    fprintf(fp, ");\n");
  }
}
void AsgVarsList::implement_getParam(FILE *fp, int var_n) const
{
  switch (varType[var_n])
  {
    case AsgConditionStringParser::Var::TYPE_Bit:
    case AsgConditionStringParser::Var::TYPE_BitOld:
    case AsgConditionStringParser::Var::TYPE_Int: fprintf(fp, "st->getParamInt(paramId.%s)", varName.getName(var_n)); break;
    case AsgConditionStringParser::Var::TYPE_Scalar:
    case AsgConditionStringParser::Var::TYPE_Timer: fprintf(fp, "st->getParam(paramId.%s)", varName.getName(var_n)); break;
    default: debug_ctx("unknown var type \'%c\'", varType[var_n]); return;
  }
}

int AsgVarsList::findVar(const char *_name, int type) const
{
  const char *name = str_to_valid_id(_name);
  int id = varName.getNameId(name);

  if (id == -1)
    return -1;
  if (varType[id] == type)
    return id;
  return -1;
}


AsgLocalVarsList::AsgLocalVarsList() : locals(tmpmem) {}

bool AsgLocalVarsList::registerLocalName(const char *varname, int vartype, FILE *fp, const AsgVarsList &vars, int indent)
{
  int id = vars.findVar(varname, vartype);
  if (id == -1)
    return false;

  for (int i = 0; i < locals.size(); i++)
    if (locals[i].varId == id)
      return false;

  LocalName ln;
  ln.varId = id;
  switch (vartype)
  {
    case AsgConditionStringParser::Var::TYPE_Bit:
      ln.localName = "f_";
      fprintf(fp, "%*sint ", indent, "");
      break;
    case AsgConditionStringParser::Var::TYPE_BitOld:
      ln.localName = "fo_";
      fprintf(fp, "%*sint ", indent, "");
      break;
    case AsgConditionStringParser::Var::TYPE_Int:
      ln.localName = "i_";
      fprintf(fp, "%*sint ", indent, "");
      break;
    case AsgConditionStringParser::Var::TYPE_Scalar:
      ln.localName = "s_";
      fprintf(fp, "%*sfloat ", indent, "");
      break;
    case AsgConditionStringParser::Var::TYPE_Timer:
      ln.localName = "t_";
      fprintf(fp, "%*sfloat ", indent, "");
      break;
    default: debug_ctx("unknown var type \'%c\'", vartype); return false;
  }
  ln.localName += str_to_valid_id(varname);

  fprintf(fp, "%s = ", (char *)ln.localName);
  vars.implement_getParam(fp, id);
  fprintf(fp, ";\n");

  locals.push_back(ln);
  return true;
}

const char *AsgLocalVarsList::getLocalName(const char *varname, int vartype, const AsgVarsList &vars)
{
  int id = vars.findVar(varname, vartype), i;
  if (id == -1)
    return "/*undefined*/";

  for (i = 0; i < locals.size(); i++)
    if (locals[i].varId == id)
      return locals[i].localName;

  return "/*undefined-2*/";
}
