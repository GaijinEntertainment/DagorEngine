// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/pageAsg/asg_scheme.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_e3dColor.h>
#include <libTools/util/hash.h>
#include <memory/dag_mem.h>


AsgStateGenParameter::AsgStateGenParameter(const DataBlock &blk)
{
  name = blk.getStr("name", "");
  caption = blk.getStr("caption", name);
  memset(&defVal, 0, sizeof(defVal));
  type = TYPE_INDENT;
  int p = blk.findParam("def");
  if (p == -1)
    defVal.indentSize = 20;
  else
  {
    switch (blk.getParamType(p))
    {
      case DataBlock::TYPE_STRING:
        type = TYPE_STRING;
        defVal.s = str_dup(blk.getStr(p), strmem);
        break;
      case DataBlock::TYPE_BOOL:
        type = TYPE_BOOL;
        defVal.b = blk.getBool(p);
        break;
      case DataBlock::TYPE_INT:
        type = TYPE_INT;
        defVal.i = blk.getInt(p);
        break;
      case DataBlock::TYPE_REAL:
        type = TYPE_REAL;
        defVal.r = blk.getReal(p);
        break;
      case DataBlock::TYPE_POINT2:
        type = TYPE_POINT2;
        defVal.point2() = blk.getPoint2(p);
        break;
      case DataBlock::TYPE_POINT3:
        type = TYPE_POINT3;
        defVal.point3() = blk.getPoint3(p);
        break;
      case DataBlock::TYPE_E3DCOLOR:
        type = TYPE_E3DCOLOR;
        defVal.color() = blk.getE3dcolor(p);
        break;

      default: defVal.indentSize = 20;
    }
  }
}
AsgStateGenParameter::AsgStateGenParameter(int indent_size)
{
  memset(&defVal, 0, sizeof(defVal));
  type = TYPE_INDENT;
  defVal.indentSize = indent_size;
}

AsgStateGenParameter::~AsgStateGenParameter()
{
  if (type == TYPE_STRING)
    memfree((void *)defVal.s, strmem);
  memset(&defVal, 0, sizeof(defVal));
}


void AsgStateGenParameter::generateDeclaration(FILE *fp)
{
  switch (type)
  {
    case TYPE_STRING: fprintf(fp, "const char*"); break;
    case TYPE_BOOL: fprintf(fp, "bool"); break;
    case TYPE_INT: fprintf(fp, "int"); break;
    case TYPE_REAL: fprintf(fp, "float"); break;
    case TYPE_POINT2: fprintf(fp, "Point2"); break;
    case TYPE_POINT3: fprintf(fp, "Point3"); break;
    case TYPE_E3DCOLOR: fprintf(fp, "E3DCOLOR"); break;
  }
  fprintf(fp, " %s;\n", name.str());
}
void AsgStateGenParameter::generateValue(FILE *fp, const DataBlock &data)
{
  switch (type)
  {
    case TYPE_STRING: fprintf(fp, "\"%s\"", data.getStr(name, defVal.s)); return;
    case TYPE_BOOL: fprintf(fp, "%s", data.getBool(name, defVal.b) ? "true" : "false"); return;
    case TYPE_INT: fprintf(fp, "%d", data.getInt(name, defVal.i)); return;
    case TYPE_REAL: fprintf(fp, "%.5f", data.getReal(name, defVal.r)); return;
    case TYPE_POINT2: fprintf(fp, "Point2(%.5f,%.5f)", P2D(data.getPoint2(name, defVal.point2()))); return;
    case TYPE_POINT3: fprintf(fp, "Point3(%.5f,%.5f,%.5f)", P3D(data.getPoint3(name, defVal.point3()))); return;
    case TYPE_E3DCOLOR: fprintf(fp, "0x%08X", (int)data.getE3dcolor(name, defVal.color())); return;
  }
}


AsgStateGenParamGroup::AsgStateGenParamGroup(const char *_name, const char *_caption, const DataBlock &blk) : items(midmem)
{
  if (_name)
    name = _name;
  if (_caption)
    caption = _caption;

  int group_nid = blk.getNameId("group"), param_nid = blk.getNameId("param"), indent_nid = blk.getNameId("indent"), l;

  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == group_nid)
    {
      const DataBlock &cb = *blk.getBlock(i);
      l = append_items(items, 1);
      items[l].group = true;
      items[l].g = new (midmem) AsgStateGenParamGroup(cb.getStr("name", NULL), cb.getStr("caption", cb.getStr("name", NULL)), cb);
    }
    else if (blk.getBlock(i)->getBlockNameId() == param_nid)
    {
      l = append_items(items, 1);
      items[l].group = false;
      items[l].p = new (midmem) AsgStateGenParameter(*blk.getBlock(i));
    }
    else if (blk.getBlock(i)->getBlockNameId() == indent_nid)
    {
      l = append_items(items, 1);
      items[l].group = false;
      items[l].p = new (midmem) AsgStateGenParameter(blk.getBlock(i)->getInt("size", 4));
    }
}
AsgStateGenParamGroup::~AsgStateGenParamGroup()
{
  for (int i = items.size() - 1; i >= 0; i--)
    if (items[i].group)
      delete items[i].g;
    else
      delete items[i].p;

  clear_and_shrink(items);
}

void AsgStateGenParamGroup::generateClassCode(FILE *fp, AsgStateGenParamGroup *g, int indent, bool skip_use)
{
  for (int i = 0; i < g->items.size(); i++)
    if (g->items[i].group)
    {
      fprintf(fp, "%*sstruct {\n", indent, "");
      generateClassCode(fp, g->items[i].g, indent + 2, false);
      fprintf(fp, "%*s} %s;\n", indent, "", g->items[i].g->name.str());
    }
    else
    {
      if (skip_use && strcmp(g->items[i].p->name, "use") == 0)
        continue;
      fprintf(fp, "%*s", indent, "");
      g->items[i].p->generateDeclaration(fp);
    }
}
bool AsgStateGenParamGroup::generateClassCode(const char *fname, AsgStateGenParamGroup *pg)
{
  FILE *fp = fopen(fname, "wt");
  if (!fp)
    return false;
  fprintf(fp, "#pragma once\n\n");

  if (pg)
    pg->generateStructDecl(fp);

  fclose(fp);
  return true;
}
void AsgStateGenParamGroup::generateStructDecl(FILE *fp)
{
  fprintf(fp, "enum {\n");
  for (int i = 0; i < items.size(); i++)
    if (items[i].group)
      fprintf(fp, "  EUID_%s = 0x%08X,\n", items[i].g->name.str(), get_hash(items[i].g->name, 0));
  fprintf(fp, "};\n\n");

  for (int i = 0; i < items.size(); i++)
    if (items[i].group)
    {
      fprintf(fp, "struct %s {\n", items[i].g->name.str());
      generateClassCode(fp, items[i].g, 2, true);
      fprintf(fp, "};\n");
    }
}

void AsgStateGenParamGroup::generateStaticVar(FILE *fp, AsgStateGenParamGroup *g, const DataBlock &data, bool skip_use)
{
  for (int i = 0; i < g->items.size(); i++)
    if (g->items[i].group)
    {
      const DataBlock *cb = data.getBlockByNameEx(g->items[i].g->name);
      fprintf(fp, "{ ");
      generateStaticVar(fp, g->items[i].g, *cb, false);
      fprintf(fp, "}, ");
    }
    else
    {
      if (skip_use && strcmp(g->items[i].p->name, "use") == 0)
        continue;
      g->items[i].p->generateValue(fp, data);
      fprintf(fp, ", ");
    }
}
void AsgStateGenParamGroup::generateStaticVar(FILE *fp, int state_id, const DataBlock &data)
{
  for (int i = 0; i < items.size(); i++)
    if (items[i].group)
    {
      const DataBlock *cb = data.getBlockByNameEx(items[i].g->name);
      if (!cb->getBool("use", false))
        continue;
      fprintf(fp, "  static %s state%d_ed_%d = { ", items[i].g->name.str(), state_id, i);
      generateStaticVar(fp, items[i].g, *cb, true);
      fprintf(fp, " };\n");
    }
}
void AsgStateGenParamGroup::generateFuncCode(FILE *fp, int state_id, const DataBlock &data)
{
  for (int i = 0; i < items.size(); i++)
    if (items[i].group)
    {
      const DataBlock *cb = data.getBlockByNameEx(items[i].g->name);
      if (!cb->getBool("use", false))
        continue;
      fprintf(fp,
        "    if (ex_props_class == EUID_%s) {\n"
        "      *ex_props = &state%d_ed_%d;\n"
        "      return true;\n"
        "    }\n",
        items[i].g->name.str(), state_id, i);
    }
}
