// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <util/dag_simplePBlock.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_mem.h>

#include <debug/dag_debug.h>

SimplePBlock::ParamDesc::ParamDesc()
{
  id = 0;
  type = DataBlock::TYPE_NONE;
  val.s = NULL;
}


void SimplePBlock::ParamDesc::setString(const char *_s)
{
  G_ASSERT(type == DataBlock::TYPE_STRING);
  if (val.s)
    memfree(val.s, strmem);
  val.s = str_dup(_s, strmem);
}


SimplePBlock::ParamDesc::~ParamDesc()
{
  if (type == DataBlock::TYPE_STRING && val.s)
    memfree(val.s, strmem);
}

SimplePBlock::SimplePBlock() : params(midmem) {}

// init block from blk (only from current level, no subblocks are supported)
SimplePBlock::SimplePBlock(const DataBlock &blk) : params(midmem) { loadFromBlk(blk); }


// clear all aprams
void SimplePBlock::clear() { params.clear(); }


void SimplePBlock::loadFromBlk(const DataBlock &blk)
{
  clear();
  for (int i = 0; i < blk.paramCount(); i++)
  {
    DataBlock::ParamType t = (DataBlock::ParamType)blk.getParamType(i);
    ParamId typeId = getParamId(blk.getParamName(i));

    switch (t)
    {
      case DataBlock::TYPE_STRING: setString(typeId, blk.getStr(i)); break;
      case DataBlock::TYPE_INT: setInt(typeId, blk.getInt(i)); break;
      case DataBlock::TYPE_REAL: setReal(typeId, blk.getReal(i)); break;
      case DataBlock::TYPE_POINT3: setPoint3(typeId, blk.getPoint3(i)); break;
      case DataBlock::TYPE_BOOL: setBool(typeId, blk.getBool(i)); break;
      case DataBlock::TYPE_E3DCOLOR: setE3dColor(typeId, blk.getE3dcolor(i)); break;
      default: DAG_FATAL("param type %d of param %s in %s is not supported!", t, blk.getParamName(i), blk.getBlockName());
    }
  }
}

#define CHECK_PARAM(__type)         \
  int index = getParamIndex(param); \
  if (index < 0)                    \
    return def;                     \
  G_ASSERT(params[index].type == DataBlock::__type);

#define CHECK_PARAM_S(__type)       \
  int index = getParamIndex(param); \
  if (index < 0)                    \
    return def_stor = def;          \
  G_ASSERT(params[index].type == DataBlock::__type);


// get params
real SimplePBlock::getReal(SimplePBlock::ParamId param, real def) const
{
  CHECK_PARAM(TYPE_REAL);
  return params[index].val.r;
}


int SimplePBlock::getInt(SimplePBlock::ParamId param, int def) const
{
  CHECK_PARAM(TYPE_INT);
  return params[index].val.i;
}


bool SimplePBlock::getBool(SimplePBlock::ParamId param, bool def) const
{
  CHECK_PARAM(TYPE_BOOL);
  return params[index].val.b;
}


E3DCOLOR SimplePBlock::getE3dColor(SimplePBlock::ParamId param, const E3DCOLOR def) const
{
  CHECK_PARAM(TYPE_E3DCOLOR);
  return params[index].val.c;
}


const Point3 &SimplePBlock::getPoint3(SimplePBlock::ParamId param, const Point3 &def) const
{
  static Point3 def_stor(0, 0, 0);
  CHECK_PARAM_S(TYPE_POINT3);
  return *(const Point3 *)(void *)&params[index].val.p;
}


const char *SimplePBlock::getString(SimplePBlock::ParamId param, const char *def) const
{
  CHECK_PARAM(TYPE_STRING);
  return params[index].val.s;
}


#define ADD_PARAM(__type)                   \
  int index = getParamIndex(param);         \
  if (index < 0)                            \
  {                                         \
    index = append_items(params, 1);        \
    params[index].id = param;               \
    params[index].type = DataBlock::__type; \
  }                                         \
  G_ASSERT(params[index].type == DataBlock::__type);

// set params
void SimplePBlock::setReal(SimplePBlock::ParamId param, real v)
{
  ADD_PARAM(TYPE_REAL);
  params[index].val.r = v;
}


void SimplePBlock::setInt(SimplePBlock::ParamId param, int v)
{
  ADD_PARAM(TYPE_INT);
  params[index].val.i = v;
}


void SimplePBlock::setBool(SimplePBlock::ParamId param, bool v)
{
  ADD_PARAM(TYPE_BOOL);
  params[index].val.b = v;
}


void SimplePBlock::setE3dColor(SimplePBlock::ParamId param, const E3DCOLOR v)
{
  ADD_PARAM(TYPE_E3DCOLOR);
  params[index].val.c = v;
}


void SimplePBlock::setPoint3(ParamId param, const Point3 &v)
{
  ADD_PARAM(TYPE_POINT3);
  params[index].val.p[0] = v.x;
  params[index].val.p[1] = v.y;
  params[index].val.p[2] = v.z;
}


void SimplePBlock::setString(ParamId param, const char *v)
{
  ADD_PARAM(TYPE_STRING);
  params[index].setString(v);
}


// get param id from string
SimplePBlock::ParamId SimplePBlock::getParamId(const char *s) const
{
  char buf[4] = {' ', ' ', ' ', ' '};
  if (s)
  {
    int i = 0;
    while (*s && i < 4)
    {
      buf[i] = *s;
      ++s;
      ++i;
    }
  }

  return MAKE4C(buf[0], buf[1], buf[2], buf[3]);
}


int SimplePBlock::getParamIndex(SimplePBlock::ParamId param) const
{
  for (int i = 0; i < params.size(); i++)
  {
    if (params[i].id == param)
      return i;
  }

  return -1;
}

// assigment
SimplePBlock::SimplePBlock(const SimplePBlock &other) : params(midmem) { operator=(other); }


SimplePBlock &SimplePBlock::operator=(const SimplePBlock &other)
{
  clear_and_shrink(params);
  params.resize(other.params.size());
  for (int i = 0; i < other.params.size(); i++)
  {
    if (other.params[i].type == DataBlock::TYPE_STRING)
    {
      params[i].setString(other.params[i].val.s);
    }
    else
    {
      params[i] = other.params[i];
    }
  }
  return *this;
}

// debugging
void SimplePBlock::dump(String &ret) const
{
  ret.printf(128, "count=%d", params.size());
  E3DCOLOR c;

  for (int i = 0; i < params.size(); i++)
  {
    switch (params[i].type)
    {
      case DataBlock::TYPE_STRING: ret.aprintf(1024, " %c%c%c%c(%d):s=%s", DUMP4C(params[i].id), i, params[i].val.s); break;
      case DataBlock::TYPE_INT: ret.aprintf(1024, " %c%c%c%c(%d):i=%i", DUMP4C(params[i].id), i, params[i].val.i); break;
      case DataBlock::TYPE_REAL: ret.aprintf(1024, " %c%c%c%c(%d):r=%.4f", DUMP4C(params[i].id), i, params[i].val.r); break;
      case DataBlock::TYPE_POINT3:
        ret.aprintf(1024, " %c%c%c%c(%d):p=(%.4f %.4f %.4f)", DUMP4C(params[i].id), i, params[i].val.p[0], params[i].val.p[1],
          params[i].val.p[2]);
        break;
      case DataBlock::TYPE_BOOL:
        ret.aprintf(1024, " %c%c%c%c(%d):b=%s", DUMP4C(params[i].id), i, params[i].val.b ? "true" : "false");
        break;
      case DataBlock::TYPE_E3DCOLOR:
        c = reinterpret_cast<const E3DCOLOR &>(params[i].val.c);
        ret.aprintf(1024, " %c%c%c%c(%d):c=(%d %d %d %d)", DUMP4C(params[i].id), i, c.r, c.g, c.b, c.a);
        break;
      default: ret.aprintf(1024, " %c%c%c%c(%d):???=???", DUMP4C(params[i].id), i);
    }
  }
}
