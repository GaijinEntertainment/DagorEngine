#pragma once

#include <libTools/pageAsg/asg_decl.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>
#include <stdio.h>

class AsgConditionStringParser
{
public:
  struct Piece
  {
    int idx;
    bool var; // true=var, false=other code
  };
  struct Var
  {
    int type;
    String name;
    String flagName; // for bitfields only

    enum
    {
      TYPE_Bit = 'B',
      TYPE_BitOld = 'O',
      TYPE_Scalar = 'S',
      TYPE_Timer = 'T',
      TYPE_Int = 'I',
    };
  };

  Tab<Piece> order;
  Tab<Var> vars;
  Tab<String> otherCode;

public:
  AsgConditionStringParser();
  AsgConditionStringParser(const char *cond_str);
  ~AsgConditionStringParser() { clear(); }

  void clear();
  void parse(const char *cond_str);

  void dump();

private:
  void addOtherCode(const char *str, int len);
};


class AsgVarsList
{
  NameMap varName;
  Tab<int> varType;
  Tab<String> realName;

public:
  AsgVarsList();

  bool registerVar(const char *name, int type);

  void implement_ParamIdDeclaration(FILE *fp) const;
  void implement_ParamIdInit(FILE *fp, const char *translator) const;
  void implement_getParam(FILE *fp, int var_n) const;

  int findVar(const char *name, int type) const;
};


class AsgLocalVarsList
{
  struct LocalName
  {
    String localName;
    int varId;
  };
  Tab<LocalName> locals;

public:
  AsgLocalVarsList();

  bool registerLocalName(const char *varname, int vartype, FILE *fp, const AsgVarsList &vars, int indent = 4);
  const char *getLocalName(const char *varname, int vartype, const AsgVarsList &vars);
};


extern char *str_to_valid_id(const char *name);
