// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <sqplus.h>

class ScriptEditor;

struct SubClass
{
  String name;
  String group;
  DataBlock blk;
  Tab<SubClass *> children;
  SubClass *parent;
  SquirrelObject sqClass;

  Tab<String> closures;
  String autoGenClosure;

  SubClass(const char *name_) :
    name(name_),
    from(tmpmem),
    till(tmpmem),
    closures(tmpmem),
    parent(NULL),
    autoGenFrom(-1),
    autoGenTill(-1),
    children(tmpmem),
    childNames(tmpmem)
  {}
  SubClass() :
    from(tmpmem), till(tmpmem), closures(tmpmem), parent(NULL), autoGenFrom(-1), autoGenTill(-1), children(tmpmem), childNames(tmpmem)
  {}

  void findChildren(ScriptEditor *se, int len_file);
  void findEdges(ScriptEditor *se, int len_file);
  void findAutoGenCode(ScriptEditor *se, const char *autogen_name);
  void findOverride(ScriptEditor *se, const char *override_name);
  void buildOverrides(ScriptEditor *se);
  void buildAutoGen(ScriptEditor *se);
  void buildChildren(Tab<SubClass *> &sub_classes);

private:
  int braceMin, braceMax;
  Tab<int> from, till;          // temp
  int autoGenFrom, autoGenTill; // temp
  Tab<String> childNames;       // temp
};
