// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/container.h>
#include <propPanel/c_indirect.h>

#include <ioSys/dag_dataBlock.h>

namespace PropPanel
{

namespace schemebasedpropertypanel
{

//===============================================================================
//  load/get property panel paramter from DataBlock
//===============================================================================

Parameter::Parameter(const DataBlock &blk)
{
  name = blk.getStr("name", "");
  caption = blk.getStr("caption", name);
  memset(&defVal, 0, sizeof(defVal));
  type = TYPE_INDENT;
  min = max = step = 0.0f;
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
        defVal.p2() = blk.getPoint2(p);
        break;
      case DataBlock::TYPE_POINT3:
        type = TYPE_POINT3;
        defVal.p3() = blk.getPoint3(p);
        break;
      case DataBlock::TYPE_E3DCOLOR:
        type = TYPE_E3DCOLOR;
        defVal.c() = blk.getE3dcolor(p);
        break;
      default: defVal.indentSize = 20;
    }
  }
}

//===================================================================

Parameter::Parameter(int indent_size)
{
  memset(&defVal, 0, sizeof(defVal));
  type = TYPE_INDENT;
  min = max = step = 0.0f;
  defVal.indentSize = indent_size;
}

//===================================================================
Parameter::~Parameter()
{
  if (type == TYPE_STRING)
    memfree((void *)defVal.s, strmem);
  memset(&defVal, 0, sizeof(defVal));
}

//===================================================================

void Parameter::addParam(ContainerPropertyControl *gr, const DataBlock &data)
{
  switch (type)
  {
    case TYPE_INDENT: gr->createIndent(getId()); return;
    case TYPE_STRING: gr->createEditBox(getId(), caption, data.getStr(name, defVal.s)); return;
    case TYPE_BOOL: gr->createCheckBox(getId(), caption, data.getBool(name, defVal.b)); return;
    case TYPE_INT: gr->createEditInt(getId(), caption, data.getInt(name, defVal.i)); return;
    case TYPE_REAL:
      gr->createEditFloat(getId(), caption, data.getReal(name, defVal.r), 4); //, true, false, 7);
      return;
    case TYPE_POINT2: gr->createPoint2(getId(), caption, data.getPoint2(name, defVal.p2()), 3); return;
    case TYPE_POINT3: gr->createPoint3(getId(), caption, data.getPoint3(name, defVal.p3()), 3); return;
    case TYPE_E3DCOLOR: gr->createColorBox(getId(), caption, data.getE3dcolor(name, defVal.c())); return;
    case TYPE_REALTRACK: gr->createTrackFloat(getId(), caption, data.getReal(name, defVal.c()), min, max, step); return;
  }
}

//===================================================================

bool Parameter::getParam(const ContainerPropertyControl *gr, DataBlock &data)
{
  Value v;

  switch (type)
  {
    case TYPE_STRING:
    {
      String buf(gr->getText(getId()));
      if (!data.getStr(name, NULL) || strcmp(buf.str(), data.getStr(name, defVal.s)) != 0)
      {
        data.setStr(name, buf.str());
        return true;
      }
    }
      return false;
    case TYPE_BOOL:
      v.b = gr->getBool(getId());
      if (data.findParam(name) == -1 || v.b != data.getBool(name, defVal.b))
      {
        data.setBool(name, v.b);
        return true;
      }
      return false;
    case TYPE_INT:
      v.i = gr->getInt(getId());
      if (data.findParam(name) == -1 || v.i != data.getInt(name, defVal.i))
      {
        data.setInt(name, v.i);
        return true;
      }
      return false;
    case TYPE_REAL:
    case TYPE_REALTRACK:
      v.r = gr->getFloat(getId());
      if (data.findParam(name) == -1 || v.r != data.getReal(name, defVal.r))
      {
        data.setReal(name, v.r);
        return true;
      }
      return false;
    case TYPE_POINT2:
      v.p2() = gr->getPoint2(getId());
      if (data.findParam(name) == -1 || v.p2() != data.getPoint2(name, defVal.p2()))
      {
        data.setPoint2(name, v.p2());
        return true;
      }
      return false;
    case TYPE_POINT3:
      v.p3() = gr->getPoint3(getId());
      if (data.findParam(name) == -1 || v.p3() != data.getPoint3(name, defVal.p3()))
      {
        data.setPoint3(name, v.p3());
        return true;
      }
      return false;
    case TYPE_E3DCOLOR:
      v.c() = gr->getColor(getId());
      if (data.findParam(name) == -1 || v.c() != data.getE3dcolor(name, defVal.c()))
      {
        data.setE3dcolor(name, v.c());
        return true;
      }
      return false;
  }

  return false;
}

//===============================================================================
//  load/get property panel group to DataBlock
//===============================================================================

ParamGroup::ParamGroup(const char *_name, const char *_caption, const DataBlock &blk) : items(midmem)
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
      items[l].g = new (midmem) ParamGroup(cb.getStr("name", NULL), cb.getStr("caption", cb.getStr("name", NULL)), cb);
    }
    else if (blk.getBlock(i)->getBlockNameId() == param_nid)
    {
      l = append_items(items, 1);
      items[l].group = false;
      items[l].p = new (midmem) Parameter(*blk.getBlock(i));
    }
    else if (blk.getBlock(i)->getBlockNameId() == indent_nid)
    {
      l = append_items(items, 1);
      items[l].group = false;
      items[l].p = new (midmem) Parameter(blk.getBlock(i)->getInt("size", 4));
    }
}


//===============================================================================
ParamGroup::ParamGroup(const DataBlock &blk) : items(midmem)
{
  name = caption = blk.getBlockName();

  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock &cb = *blk.getBlock(i);
    int l = append_items(items, 1);
    items[l].group = true;
    items[l].g = new (midmem) ParamGroup(cb);
  }
  String presetCaption("");
  real presetStep = 0;
  real presetMax = 0;
  real presetMin = 0;
  for (int i = 0; i < blk.paramCount(); i++)
  {
    if (strcmp(blk.getParamName(i), "caption") == NULL)
    {
      presetCaption = blk.getStr(i);
      continue;
    }
    if (strcmp(blk.getParamName(i), "track_step") == NULL)
    {
      presetStep = blk.getReal(i);
      continue;
    }
    if (strcmp(blk.getParamName(i), "track_min") == NULL)
    {
      presetMin = blk.getReal(i);
      continue;
    }
    if (strcmp(blk.getParamName(i), "track_max") == NULL)
    {
      presetMax = blk.getReal(i);
      continue;
    }

    int l = append_items(items, 1);
    items[l].group = false;

    DataBlock param;
    param.setStr("name", blk.getParamName(i));
    if (strcmp(presetCaption, "") != NULL)
    {
      param.setStr("caption", presetCaption);
      presetCaption = "";
    }
    else
      param.setStr("caption", blk.getParamName(i));

    switch (blk.getParamType(i))
    {
      case DataBlock::TYPE_STRING: param.setStr("def", blk.getStr(blk.getParamName(i), "")); break;

      case DataBlock::TYPE_INT: param.setInt("def", blk.getInt(blk.getParamName(i), 0)); break;

      case DataBlock::TYPE_REAL: param.setReal("def", blk.getReal(blk.getParamName(i), 0)); break;

      case DataBlock::TYPE_POINT2: param.setPoint2("def", blk.getPoint2(blk.getParamName(i), Point2(0, 0))); break;

      case DataBlock::TYPE_POINT3: param.setPoint3("def", blk.getPoint3(blk.getParamName(i), Point3(0, 0, 0))); break;

      case DataBlock::TYPE_POINT4: param.setPoint4("def", blk.getPoint4(blk.getParamName(i), Point4(0, 0, 0, 0))); break;

      case DataBlock::TYPE_IPOINT2: param.setIPoint2("def", blk.getIPoint2(blk.getParamName(i), IPoint2(0, 0))); break;

      case DataBlock::TYPE_IPOINT3: param.setIPoint3("def", blk.getIPoint3(blk.getParamName(i), IPoint3(0, 0, 0))); break;

      case DataBlock::TYPE_BOOL: param.setBool("def", blk.getBool(blk.getParamName(i), 0)); break;

      case DataBlock::TYPE_E3DCOLOR: param.setE3dcolor("def", blk.getE3dcolor(blk.getParamName(i), 0)); break;
    }

    items[l].p = new (midmem) Parameter(param);
    if (presetStep)
    {
      items[l].p->setTrackBounds(presetMin, presetMax, presetStep);
      presetStep = 0;
      presetMin = 0;
      presetMax = 0;
    }
  }
}


//===============================================================================
ParamGroup::~ParamGroup()
{
  for (int i = items.size() - 1; i >= 0; i--)
    if (items[i].group)
      delete items[i].g;
    else
      delete items[i].p;

  clear_and_shrink(items);
}


//===============================================================================
void ParamGroup::addParams(ContainerPropertyControl *gr, const DataBlock &data)
{
  for (int i = 0; i < items.size(); i++)
    if (items[i].group)
    {
      ContainerPropertyControl *subgr = gr->createGroup(items[i].g->getId(), items[i].g->caption);
      items[i].g->addParams(subgr, *data.getBlockByNameEx(items[i].g->name));
    }
    else
      items[i].p->addParam(gr, data);
}


//===============================================================================
bool ParamGroup::getParams(const ContainerPropertyControl *gr, DataBlock &data)
{
  bool changed = false;

  for (int i = 0; i < items.size(); i++)
    if (items[i].group)
    {
      // ContainerPropertyControl *subgr = gr->getGroup ((int)items[i].g);

      DataBlock *cb = data.getBlockByName(items[i].g->name);
      if (!cb)
        cb = data.addNewBlock(items[i].g->name);

      // if ( subgr && cb )
      if (gr && cb)
      {
        // if ( items[i].g->getParams ( subgr, *cb))
        if (items[i].g->getParams(gr, *cb))
          changed = true;
      }
    }
    else if (items[i].p->getParam(gr, data))
      changed = true;

  return changed;
}
} // namespace schemebasedpropertypanel


//===============================================================================
//  sheme to load/get items from DataBlock and propeprty panel
//===============================================================================

void PropPanelScheme::load(const DataBlock &scheme_raw, bool make_on_fly)
{
  clear();
  if (!make_on_fly)
  {
    if (scheme_raw.blockCount() > 0)
    {
      rootScheme = new (midmem) schemebasedpropertypanel::ParamGroup(NULL, NULL, scheme_raw);
      if (rootScheme->items.size() == 0)
      {
        delete rootScheme;
        rootScheme = NULL;
      }
    }
    else
      rootScheme = NULL;
  }
  else
  {
    rootScheme = new (midmem) schemebasedpropertypanel::ParamGroup(scheme_raw);
    if (rootScheme->items.size() == 0)
    {
      delete rootScheme;
      rootScheme = NULL;
    }
  }
}


//===============================================================================
void PropPanelScheme::load(const String &sheme_name, const DataBlock *data_blk)
{
  DataBlock sheme_blk;
  if (sheme_blk.load(sheme_name))
    load(sheme_blk, false);
  else if (data_blk)
    load(*data_blk, true);
}


//===============================================================================
PropPanelScheme::~PropPanelScheme() { PropPanelScheme::clear(); }

//===============================================================================
void PropPanelScheme::setParams(ContainerPropertyControl *gr, const DataBlock &data)
{
  if (rootScheme && gr)
    rootScheme->addParams(gr, data);
}


//===============================================================================
bool PropPanelScheme::getParams(const ContainerPropertyControl *gr, DataBlock &data)
{
  if (rootScheme && gr)
    return rootScheme->getParams(gr, data);
  return false;
}


//===============================================================================
void PropPanelScheme::clear()
{
  if (rootScheme)
    delete rootScheme;
  rootScheme = NULL;
}

} // namespace PropPanel