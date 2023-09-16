#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <math/dag_TMatrix.h>
#include <math/dag_color.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point4.h>
#include <memory/dag_framemem.h>
#include <sqModules/sqModules.h>
#include <osApiWrappers/dag_direct.h>
#include <EASTL/set.h>


#include <sqrat.h>
#include <sqstdaux.h>

namespace
{
struct BlkErrReporter : public DataBlock::IErrorReporterPipe
{
  String err;
  void reportError(const char *error_text, bool serious_err) override
  {
    err.aprintf(0, "[%c] %s\n", serious_err ? 'F' : 'E', error_text);
  }
};
} // namespace


namespace bindquirrel
{

static void push_param_to_sq(HSQUIRRELVM vm, const DataBlock *blk, int index)
{
  const DataBlock *b = blk;
  using Sqrat::PushVar;
  switch (blk->getParamType(index))
  {
    case DataBlock::TYPE_STRING: sq_pushstring(vm, b->getStr(index), -1); break;
    case DataBlock::TYPE_INT: sq_pushinteger(vm, b->getInt(index)); break;
    case DataBlock::TYPE_INT64: sq_pushinteger(vm, b->getInt64(index)); break;
    case DataBlock::TYPE_REAL: sq_pushfloat(vm, b->getReal(index)); break;
    case DataBlock::TYPE_BOOL: sq_pushbool(vm, b->getBool(index)); break;
    case DataBlock::TYPE_POINT4: PushVar(vm, b->getPoint4(index)); break;
    case DataBlock::TYPE_POINT3: PushVar(vm, b->getPoint3(index)); break;
    case DataBlock::TYPE_POINT2: PushVar(vm, b->getPoint2(index)); break;
    case DataBlock::TYPE_IPOINT3: PushVar(vm, b->getIPoint3(index)); break;
    case DataBlock::TYPE_IPOINT2: PushVar(vm, b->getIPoint2(index)); break;
    case DataBlock::TYPE_MATRIX: PushVar(vm, b->getTm(index)); break;
    case DataBlock::TYPE_E3DCOLOR: PushVar(vm, Color4(b->getE3dcolor(index))); break;
    default: sq_pushnull(vm); break;
  }
}

static void push_param_type_annotation_to_sq(HSQUIRRELVM vm, const DataBlock *blk, int index)
{
  using Sqrat::PushVar;
  switch (blk->getParamType(index))
  {
    case DataBlock::TYPE_STRING: sq_pushstring(vm, "t", -1); break;
    case DataBlock::TYPE_INT: sq_pushstring(vm, "i", -1); break;
    case DataBlock::TYPE_INT64: sq_pushstring(vm, "i64", -1); break;
    case DataBlock::TYPE_REAL: sq_pushstring(vm, "r", -1); break;
    case DataBlock::TYPE_BOOL: sq_pushstring(vm, "b", -1); break;
    case DataBlock::TYPE_POINT4: sq_pushstring(vm, "p4", -1); break;
    case DataBlock::TYPE_POINT3: sq_pushstring(vm, "p3", -1); break;
    case DataBlock::TYPE_POINT2: sq_pushstring(vm, "p2", -1); break;
    case DataBlock::TYPE_IPOINT3: sq_pushstring(vm, "ip3", -1); break;
    case DataBlock::TYPE_IPOINT2: sq_pushstring(vm, "ip2", -1); break;
    case DataBlock::TYPE_MATRIX: sq_pushstring(vm, "tm", -1); break;
    case DataBlock::TYPE_E3DCOLOR: sq_pushstring(vm, "c", -1); break;
    default: sq_pushnull(vm); break;
  }
}

static SQInteger blk_get_param_value(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> self(v, 1);
  Sqrat::Var<SQInteger> index(v, 2);
  push_param_to_sq(v, self.value, index.value);
  return 1;
}

static SQInteger blk_get_param_type_annotation(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> self(v, 1);
  Sqrat::Var<SQInteger> index(v, 2);
  push_param_type_annotation_to_sq(v, self.value, index.value);
  return 1;
}

static SQInteger blk_get_int(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  SQInteger nargs = sq_gettop(v);
  Sqrat::Var<DataBlock *> self(v, 1);
  Sqrat::Var<const char *> name(v, 2);
  if (nargs == 2)
  {
    Sqrat::PushVar(v, self.value->getInt64(name.value));
  }
  else
  {
    Sqrat::Var<SQInteger> defval(v, 3);
    Sqrat::PushVar(v, self.value->getInt64(name.value, defval.value));
  }
  return 1;
}

static SQInteger blk_get(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  if (sq_gettype(v, 2) != OT_STRING)
  {
    sq_pushnull(v);
    return sq_throwobject(v);
  }

  Sqrat::Var<DataBlock *> selfSq(v, 1);
  Sqrat::Var<const char *> keySq(v, 2);

  DataBlock *self = selfSq.value;
  const char *key = keySq.value;

  int nameId = self ? self->getNameId(key) : -1;
  if (!self || nameId == -1)
  {
    sq_pushnull(v);
    return sq_throwobject(v);
  }

  const DataBlock *blk = self->getBlockByName(nameId);
  if (blk)
    Sqrat::PushVar(v, (DataBlock *)blk);
  else
  {
    int paramIndex = self->findParam(nameId);
    if (paramIndex >= 0)
      push_param_to_sq(v, self, paramIndex);
    else
    {
      sq_pushnull(v);
      return sq_throwobject(v);
    }
  }
  return 1;
}

static SQInteger blk_set(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> selfSq(v, 1);
  Sqrat::Var<const char *> keySq(v, 2);

  HSQOBJECT valObj;
  sq_getstackobj(v, 3, &valObj);

  DataBlock *self = selfSq.value;
  const char *key = keySq.value;

  switch (sq_type(valObj))
  {
    case OT_NULL: self->removeParam(key); break;
    case OT_INTEGER:
    {
      SQInteger value = sq_direct_tointeger(&valObj);
      if (value != (int32_t)value)
        self->setInt64(key, value);
      else
        self->setInt(key, value);
      break;
    }
    case OT_FLOAT: self->setReal(key, sq_direct_tofloat(&valObj)); break;
    case OT_BOOL: self->setBool(key, sq_direct_tobool(&valObj)); break;
    case OT_STRING:
    {
      const char *s = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 3, &s)));
      self->setStr(key, s);
      break;
    }
    case OT_INSTANCE:
    {
      if (Sqrat::ClassType<DataBlock>::IsObjectOfClass(&valObj))
      {
        Sqrat::Var<const DataBlock *> blkVal(v, 3);
        self->addBlock(key)->setFrom(blkVal.value);
      }
      else if (Sqrat::ClassType<IPoint2>::IsObjectOfClass(&valObj))
      {
        IPoint2 *p2Val = Sqrat::Var<IPoint2 *>(v, 3).value;
        self->addIPoint2(key, *p2Val);
      }
      else if (Sqrat::ClassType<IPoint3>::IsObjectOfClass(&valObj))
      {
        IPoint3 *p3Val = Sqrat::Var<IPoint3 *>(v, 3).value;
        self->addIPoint3(key, *p3Val);
      }
      else if (Sqrat::ClassType<Point2>::IsObjectOfClass(&valObj))
      {
        Point2 *p2Val = Sqrat::Var<Point2 *>(v, 3).value;
        self->setPoint2(key, *p2Val);
      }
      else if (Sqrat::ClassType<Point3>::IsObjectOfClass(&valObj))
      {
        Point3 *p3Val = Sqrat::Var<Point3 *>(v, 3).value;
        self->setPoint3(key, *p3Val);
      }
      else if (Sqrat::ClassType<Point4>::IsObjectOfClass(&valObj))
      {
        Point4 *p4Val = Sqrat::Var<Point4 *>(v, 3).value;
        self->setPoint4(key, *p4Val);
      }
      else if (Sqrat::ClassType<TMatrix>::IsObjectOfClass(&valObj))
      {
        TMatrix *tmVal = Sqrat::Var<TMatrix *>(v, 3).value;
        self->setTm(key, *tmVal);
      }
      else if (Sqrat::ClassType<Color4>::IsObjectOfClass(&valObj))
      {
        Color4 *col4Val = Sqrat::Var<Color4 *>(v, 3).value;
        self->setE3dcolor(key, e3dcolor(*col4Val));
      }
      else
        logerr_ctx("Invalid instance type passed as value of '%s'", key);
      break;
    }
    default: logerr_ctx("Invalid type %X passed as value of '%s'", sq_type(valObj), key);
  }

  return 0;
}

static SQInteger blk_modulo(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> selfSq(v, 1);
  Sqrat::Var<const char *> keySq(v, 2);

  DataBlock *self = selfSq.value;
  const char *key = keySq.value;

  int nameId = self->getNameId(key);
  sq_newarray(v, 0);
  if (nameId >= 0)
  {
    if (self->paramExists(nameId))
    {
      for (int i = 0; i < self->paramCount(); i++)
        if (self->getParamNameId(i) == nameId)
        {
          push_param_to_sq(v, self, i);
          G_VERIFY(SQ_SUCCEEDED(sq_arrayappend(v, -2)));
        }
    }
    else
    {
      for (int i = 0; i < self->blockCount(); i++)
      {
        DataBlock *blk = self->getBlock(i);
        if (blk->getBlockNameId() == nameId)
        {
          Sqrat::PushVar(v, blk);
          G_VERIFY(SQ_SUCCEEDED(sq_arrayappend(v, -2)));
        }
      }
    }
  }

  return 1;
}

static SQInteger blk_newslot(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> selfSq(v, 1);
  Sqrat::Var<const char *> keySq(v, 2);

  HSQOBJECT valObj;
  sq_getstackobj(v, 3, &valObj);

  DataBlock *self = selfSq.value;
  const char *key = keySq.value;

  switch (sq_type(valObj))
  {
    case OT_NULL: self->removeParam(key); break;
    case OT_INTEGER:
    {
      SQInteger value = sq_direct_tointeger(&valObj);
      if (value != (int32_t)value)
        self->addInt64(key, value);
      else
        self->addInt(key, value);
      break;
    }
    case OT_FLOAT: self->addReal(key, sq_direct_tofloat(&valObj)); break;
    case OT_BOOL: self->addBool(key, sq_direct_tobool(&valObj)); break;
    case OT_STRING:
    {
      const char *s = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 3, &s)));
      self->addStr(key, s);
      break;
    }
    case OT_INSTANCE:
    {
      if (Sqrat::ClassType<DataBlock>::IsObjectOfClass(&valObj))
      {
        Sqrat::Var<const DataBlock *> blkVal(v, 3);
        self->addNewBlock(key)->setFrom(blkVal.value);
      }
      else if (Sqrat::ClassType<IPoint2>::IsObjectOfClass(&valObj))
      {
        IPoint2 *p2Val = Sqrat::Var<IPoint2 *>(v, 3).value;
        self->addIPoint2(key, *p2Val);
      }
      else if (Sqrat::ClassType<IPoint3>::IsObjectOfClass(&valObj))
      {
        IPoint3 *p3Val = Sqrat::Var<IPoint3 *>(v, 3).value;
        self->addIPoint3(key, *p3Val);
      }
      else if (Sqrat::ClassType<Point2>::IsObjectOfClass(&valObj))
      {
        Point2 *p2Val = Sqrat::Var<Point2 *>(v, 3).value;
        self->addPoint2(key, *p2Val);
      }
      else if (Sqrat::ClassType<Point3>::IsObjectOfClass(&valObj))
      {
        Point3 *p3Val = Sqrat::Var<Point3 *>(v, 3).value;
        self->addPoint3(key, *p3Val);
      }
      else if (Sqrat::ClassType<Point4>::IsObjectOfClass(&valObj))
      {
        Point4 *p4Val = Sqrat::Var<Point4 *>(v, 3).value;
        self->setPoint4(key, *p4Val);
      }
      else if (Sqrat::ClassType<TMatrix>::IsObjectOfClass(&valObj))
      {
        TMatrix *tmVal = Sqrat::Var<TMatrix *>(v, 3).value;
        self->addTm(key, *tmVal);
      }
      else if (Sqrat::ClassType<Color4>::IsObjectOfClass(&valObj))
      {
        Color4 *col4Val = Sqrat::Var<Color4 *>(v, 3).value;
        self->setE3dcolor(key, e3dcolor(*col4Val));
      }
      else
        logerr_ctx("Invalid instance type passed as value of '%s'", key);
      break;
    }
    default: logerr_ctx("Invalid type %X passed as value of '%s'", sq_type(valObj), key);
  }

  return 0;
}

static bool check_name_for_bad_nexti(int id, eastl::set<int> &all_name_ids)
{
  if (all_name_ids.find(id) != all_name_ids.end())
    return false;
  all_name_ids.insert(id);
  return true;
}

static bool check_blk_for_bad_nexti(HSQUIRRELVM vm, DataBlock *self)
{
  eastl::set<int> all_name_ids;

  for (int i = 0; i < self->blockCount(); i++)
    if (!check_name_for_bad_nexti(self->getBlock(i)->getBlockNameId(), all_name_ids))
    {
      String errMsg(0, "Can't use foreach with datablock, it contains non-unique name '%s'", self->getBlock(i)->getBlockName());
      (void)sq_throwerror(vm, errMsg);
      return false;
    }

  for (int i = 0; i < self->paramCount(); i++)
    if (!check_name_for_bad_nexti(self->getParamNameId(i), all_name_ids))
    {
      String errMsg(0, "Can't use foreach with datablock, it contains non-unique name '%s'", self->getParamName(i));
      (void)sq_throwerror(vm, errMsg);
      return false;
    }

  return true;
}

static SQInteger blk_nexti(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> selfSq(v, 1);
  DataBlock *self = selfSq.value;

  if (sq_gettype(v, 2) == OT_NULL)
  {
    bool valid = check_blk_for_bad_nexti(v, self);
    if (!valid)
      return SQ_ERROR; // message is thrown inside check_blk_for_bad_nexti()
    else if (self->blockCount())
      sq_pushstring(v, self->getBlock(0)->getBlockName(), -1);
    else if (self->paramCount())
      sq_pushstring(v, self->getParamName(0), -1);
    else
      sq_pushnull(v);
    return 1;
  }
  const char *key = NULL;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 2, &key)));

  int nid = self->getNameId(key);
  if (nid < 0)
  {
    sq_pushnull(v);
    return 1;
  }
  bool hadSuchBlock = false;
  for (int i = 0; i < self->blockCount(); i++)
    if (self->getBlock(i)->getBlockNameId() == nid)
    {
      hadSuchBlock = true;
      for (int next = i + 1; next < self->blockCount(); next++)
        if (self->getBlock(next)->getBlockNameId() != nid)
        {
          sq_pushstring(v, self->getBlock(next)->getBlockName(), -1);
          return 1;
        }
    }
  if (hadSuchBlock && self->paramCount())
  {
    sq_pushstring(v, self->getParamName(0), -1);
    return 1;
  }
  for (int i = 0; i < self->paramCount(); i++)
    if (self->getParamNameId(i) == nid)
    {
      for (int next = i + 1; next < self->paramCount(); next++)
        if (self->getParamNameId(next) != nid)
        {
          sq_pushstring(v, self->getParamName(next), -1);
          return 1;
        }
    }
  sq_pushnull(v);
  return 1;
}


static int find_param(const DataBlock *blk, const char *param) { return blk->findParam(param); }

static bool param_exists(const DataBlock *blk, const char *param) { return blk->paramExists(param); }

static const DataBlock *get_block_by_name(const DataBlock *blk, const char *param) { return blk->getBlockByName(param); }

static void set_params_from(DataBlock *blk, const DataBlock *other) { blk->setParamsFrom(other); }

static void set_from(DataBlock *blk, const DataBlock *other) { blk->setFrom(other); }

static SQInteger blk_load_from_text(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> selfSq(v, 1);
  DataBlock *self = selfSq.value;
  const char *text = NULL;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 2, &text)));
  SQInteger text_length = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(v, 3, &text_length)));

  BlkErrReporter rep;
  DataBlock::InstallReporterRAII irep(&rep);
  sq_pushbool(v, self->loadText(text, text_length));
  if (!rep.err.empty())
    return sqstd_throwerrorf(v, "Failed to load BLK\n%s", rep.err.str());
  return 1;
}

static int set_int(DataBlock *blk, const char *key, SQInteger value)
{
  if (value != (int32_t)value)
    return blk->setInt64(key, value);
  return blk->setInt(key, value);
}

static int set_int64(DataBlock *blk, const char *key, SQInteger value) { return blk->setInt64(key, value); }

static int add_int(DataBlock *blk, const char *key, SQInteger value)
{
  if (value != (int32_t)value)
    return blk->addInt64(key, value);
  return blk->addInt(key, value);
}

static IPoint3 get_IPoint3(const DataBlock *blk, const char *name, const IPoint3 &defval) { return blk->getIPoint3(name, defval); }

static int set_IPoint3(DataBlock *blk, const char *name, const IPoint3 &val) { return blk->setIPoint3(name, val); }

static int add_IPoint3(DataBlock *blk, const char *name, const IPoint3 &val) { return blk->addIPoint3(name, val); }

static inline SQInteger blk_load_impl(HSQUIRRELVM v, bool ignore_missing)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> self(v, 1);
  const char *fn;
  sq_getstring(v, 2, &fn);
  if (ignore_missing && !dd_file_exist(fn) && !::dd_file_exist(String(0, "%s.blk", fn)) && !::dd_file_exist(String(0, "%s.bin", fn)) &&
      !::dd_file_exist(String(0, "%s.blk.bin", fn)))
  {
    dblk::set_flag(*self.value, dblk::ReadFlag::ROBUST);
    sq_pushbool(v, false);
    return 1;
  }

  if (sq_gettop(v) == 3 && Sqrat::Var<SQBool>(v, 3).value) // tryLoad(fn, true) shall not throw exception on bad data
  {
    sq_pushbool(v, dblk::load(*self.value, fn, dblk::ReadFlag::ROBUST));
    return 1;
  }

  BlkErrReporter rep;
  DataBlock::InstallReporterRAII irep(&rep);
  if (dblk::load(*self.value, fn, dblk::ReadFlag::ROBUST))
  {
    sq_pushbool(v, true); // for compatibility with existing scripts
    return 1;
  }
  return sqstd_throwerrorf(v, "Failed to load '%s'%s%s", fn, rep.err.empty() ? "" : ":\n", rep.err.str());
}
static SQInteger blk_load(HSQUIRRELVM v) { return blk_load_impl(v, false); }
static SQInteger blk_try_load(HSQUIRRELVM v) { return blk_load_impl(v, true); }

static SQInteger blk_saveToTextFile(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> self(v, 1);
  const char *fname = NULL;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 2, &fname)));
  sq_pushbool(v, dblk::save_to_text_file(*self.value, fname));
  return 1;
}
static SQInteger blk_saveToTextFileCompact(HSQUIRRELVM v)
{
  if (!Sqrat::check_signature<DataBlock *>(v))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> self(v, 1);
  const char *fname = NULL;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 2, &fname)));
  sq_pushbool(v, dblk::save_to_text_file_compact(*self.value, fname));
  return 1;
}

static SQInteger blk_formatAsString(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<DataBlock *>(vm))
    return SQ_ERROR;

  Sqrat::Var<DataBlock *> var(vm, 1);
  SQInteger nargs = sq_gettop(vm);

  DynamicMemGeneralSaveCB cwr(framemem_ptr(), 0, 2 << 10);
  SQInteger max_out_line_num = -1, max_level_depth = -1, init_indent = 0;

  if (nargs >= 2)
  {
    if (sq_gettype(vm, 2) & SQOBJECT_NUMERIC)
      sq_getinteger(vm, 2, &init_indent);
    else
    {
      Sqrat::Var<Sqrat::Table> tbl(vm, 2);
#define GET_PARAM(name) name = tbl.value.RawGetSlotValue<SQInteger>(#name, name)
      GET_PARAM(init_indent);
      GET_PARAM(max_level_depth);
      GET_PARAM(max_out_line_num);
#undef GET_PARAM
    }
  }

  if (nargs >= 3)
    sq_getinteger(vm, 3, &max_level_depth);
  if (nargs >= 4)
    sq_getinteger(vm, 4, &max_out_line_num);

  dblk::print_to_text_stream_limited(*var.value, cwr, max_out_line_num, max_level_depth, init_indent);
  sq_pushstring(vm, (const char *)cwr.data(), cwr.size());
  return 1;
}


static void set_parseIncludesAsParams(bool opt) { DataBlock::parseIncludesAsParams = opt; }
static bool get_parseIncludesAsParams() { return DataBlock::parseIncludesAsParams; }
static void set_parseOverridesNotApply(bool opt) { DataBlock::parseOverridesNotApply = opt; }
static bool get_parseOverridesNotApply() { return DataBlock::parseOverridesNotApply; }
static void set_parseOverridesIgnored(bool opt) { DataBlock::parseOverridesIgnored = opt; }
static bool get_parseOverridesIgnored() { return DataBlock::parseOverridesIgnored; }
static void set_parseCommentsAsParams(bool opt) { DataBlock::parseCommentsAsParams = opt; }
static bool get_parseCommentsAsParams() { return DataBlock::parseCommentsAsParams; }

static SQInteger datablock_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  if (top > 1 && !Sqrat::check_signature<DataBlock *>(vm, 2))
    return sq_throwerror(vm, "Copy constructor requires DataBlock as argument");

  DataBlock *blk = nullptr;
  if (top == 1)
    blk = new DataBlock();
  else
  {
    Sqrat::Var<const DataBlock *> src(vm, 2);
    blk = new DataBlock(*src.value);
  }
  Sqrat::ClassType<DataBlock>::SetManagedInstance(vm, 1, blk);
  return 0;
}


void sqrat_bind_datablock(SqModules *module_mgr, bool allow_file_access)
{
  G_ASSERT(module_mgr);
  HSQUIRRELVM vm = module_mgr->getVM();

#define FUNC(f) .Func(#f, &DataBlock::f)
  ///@class DataBlock=DataBlock
  Sqrat::Class<DataBlock> sqDataBlock(vm, "DataBlock");
  (void)sqDataBlock.SquirrelCtor(datablock_ctor, -1, "xx")
    .SquirrelFunc("formatAsString", blk_formatAsString, -1, "x t|n n n")
    .Func("getBlock", (const DataBlock *(DataBlock::*)(uint32_t) const) & DataBlock::getBlock)
    .Func("addBlock", (DataBlock * (DataBlock::*)(const char *)) & DataBlock::addBlock)
    .Func("addNewBlock", (DataBlock * (DataBlock::*)(const char *)) & DataBlock::addNewBlock)
    .Func("getStr", (const char *(DataBlock::*)(const char *, const char *) const) & DataBlock::getStr)
    .Func("getBool", (bool(DataBlock::*)(const char *, bool) const) & DataBlock::getBool)
    .Func("getReal", (float(DataBlock::*)(const char *, float) const) & DataBlock::getReal)
    .Func("getPoint4", (Point4(DataBlock::*)(const char *, const Point4 &) const) & DataBlock::getPoint4)
    .Func("getPoint3", (Point3(DataBlock::*)(const char *, const Point3 &) const) & DataBlock::getPoint3)
    .Func("getPoint2", (Point2(DataBlock::*)(const char *, const Point2 &) const) & DataBlock::getPoint2)
    .Func("getTm", (TMatrix(DataBlock::*)(const char *, const TMatrix &) const) & DataBlock::getTm)
    .Func("setStr", (int(DataBlock::*)(const char *, const char *)) & DataBlock::setStr)
    .Func("setBool", (int(DataBlock::*)(const char *, bool)) & DataBlock::setBool)
    .Func("setReal", (int(DataBlock::*)(const char *, float)) & DataBlock::setReal)
    .Func("setPoint4", (int(DataBlock::*)(const char *, const Point4 &)) & DataBlock::setPoint4)
    .Func("setPoint3", (int(DataBlock::*)(const char *, const Point3 &)) & DataBlock::setPoint3)
    .Func("setPoint2", (int(DataBlock::*)(const char *, const Point2 &)) & DataBlock::setPoint2)
    .Func("setTm", (int(DataBlock::*)(const char *, const TMatrix &)) & DataBlock::setTm)
    .Func("addStr", (int(DataBlock::*)(const char *, const char *)) & DataBlock::addStr)
    .Func("addBool", (int(DataBlock::*)(const char *, bool)) & DataBlock::addBool)
    .Func("addReal", (int(DataBlock::*)(const char *, float)) & DataBlock::addReal)
    .Func("addPoint3", (int(DataBlock::*)(const char *, const Point3 &)) & DataBlock::addPoint3)
    .Func("addPoint4", (int(DataBlock::*)(const char *, const Point4 &)) & DataBlock::addPoint4)
    .Func("addPoint2", (int(DataBlock::*)(const char *, const Point2 &)) & DataBlock::addPoint2)
    .Func("addTm", (int(DataBlock::*)(const char *, const TMatrix &)) & DataBlock::addTm)
    .Func("removeParam", (bool(DataBlock::*)(const char *)) & DataBlock::removeParam)
    .Func("removeParamById", (bool(DataBlock::*)(uint32_t)) & DataBlock::removeParam)
    .Func("removeBlock", (bool(DataBlock::*)(const char *)) & DataBlock::removeBlock)
    .Func("removeBlockById", (bool(DataBlock::*)(uint32_t)) & DataBlock::removeBlock)

    .GlobalFunc("findParam", find_param)
    .GlobalFunc("paramExists", param_exists)
    .GlobalFunc("getBlockByName", get_block_by_name)
    .GlobalFunc("setParamsFrom", set_params_from)
    .GlobalFunc("setFrom", set_from)
    .GlobalFunc("setInt", set_int)
    .GlobalFunc("setInt64", set_int64)
    .GlobalFunc("addInt", add_int)
    .GlobalFunc("getIPoint3", get_IPoint3)
    .GlobalFunc("setIPoint3", set_IPoint3)
    .GlobalFunc("addIPoint3", add_IPoint3)

      FUNC(clearData) FUNC(reset) FUNC(getBlockName) FUNC(changeBlockName) FUNC(blockCount) FUNC(paramCount) FUNC(getParamName)

    .SquirrelFunc("getParamTypeAnnotation", blk_get_param_type_annotation, 2, "xi")
    .SquirrelFunc("getParamValue", blk_get_param_value, 2, "xi")
    .SquirrelFunc("getInt", blk_get_int, -2, "xs")
    .SquirrelFunc("_set", blk_set, 3, "xs")
    .SquirrelFunc("_get", blk_get, 2, "x")
    .SquirrelFunc("_modulo", blk_modulo, 2, "xs")
    .SquirrelFunc("_newslot", blk_newslot, 3, "xs")
    .SquirrelFunc("_nexti", blk_nexti, 0)
    .SquirrelFunc("loadFromText", blk_load_from_text, 3, "xsi")

    .StaticFunc("set_root", DataBlock::setRootIncludeResolver)
    .StaticFunc("setParseIncludesAsParams", set_parseIncludesAsParams)
    .StaticFunc("getParseIncludesAsParams", get_parseIncludesAsParams)
    .StaticFunc("setParseOverridesNotApply", set_parseOverridesNotApply)
    .StaticFunc("getParseOverridesNotApply", get_parseOverridesNotApply)
    .StaticFunc("setParseOverridesIgnored", set_parseOverridesIgnored)
    .StaticFunc("getParseOverridesIgnored", get_parseOverridesIgnored)
    .StaticFunc("setParseCommentsAsParams", set_parseCommentsAsParams)
    .StaticFunc("getParseCommentsAsParams", get_parseCommentsAsParams); //-V1071

  if (allow_file_access)
  {
    (void)sqDataBlock.SquirrelFunc("load", blk_load, 2, "xs")
      .SquirrelFunc("tryLoad", blk_try_load, -2, "xs")
      .SquirrelFunc("saveToTextFile", blk_saveToTextFile, 2, "xs")
      .SquirrelFunc("saveToTextFileCompact", blk_saveToTextFileCompact, 2, "xs"); //-V1071
  }

#undef FUNC

  Sqrat::Object classObj(sqDataBlock.GetObject(), vm);
  module_mgr->addNativeModule("DataBlock", classObj);
}

} // namespace bindquirrel
