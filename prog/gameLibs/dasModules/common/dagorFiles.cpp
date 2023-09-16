#include <dasModules/aotDagorFiles.h>

namespace bind_dascript
{

struct DagFileAnnotation : das::ManagedStructureAnnotation<DagFile, false>
{
  DagFileAnnotation(das::ModuleLibrary &ml) : das::ManagedStructureAnnotation<DagFile, false>("DagFile", ml)
  {
    cppName = "bind_dascript::DagFile";
  }
};

const DagFile *das_df_open(const char *fname, int flags) { return (const DagFile *)df_open(fname, flags); }

int dag_df_close(const DagFile *fp) { return df_close((file_ptr_t)fp); }

void dag_df_flush(const DagFile *fp) { return df_flush((file_ptr_t)fp); }

int dag_df_seek_to(const DagFile *fp, int offset) { return df_seek_to((file_ptr_t)fp, offset); }

int dag_df_seek_rel(const DagFile *fp, int offset) { return df_seek_rel((file_ptr_t)fp, offset); }

int dag_df_seek_end(const DagFile *fp, int offset) { return df_seek_end((file_ptr_t)fp, offset); }

int dag_df_tell(const DagFile *fp) { return df_tell((file_ptr_t)fp); }

int dag_df_length(const DagFile *fp) { return df_length((file_ptr_t)fp); }

int dag_df_puts(const DagFile *fp, const char *str, das::Context *context)
{
  if (str)
  {
    int len = stringLengthSafe(*context, str);
    return df_write((file_ptr_t)fp, str, len);
  }
  else
  {
    return 0;
  }
}

char *dag_df_gets(const DagFile *fp, int maxLen, das::Context *context)
{
  void *temp = malloc(maxLen);
  char *hstr = nullptr;
  if (char *res = df_gets((char *)temp, maxLen, (file_ptr_t)fp))
  {
    hstr = context->stringHeap->allocateString(res, strlen(res));
  }
  free(temp);
  return hstr;
}

vec4f dag_builtin_df_read(das::Context &, das::SimNode_CallBase *call, vec4f *args)
{
  G_UNUSED(call);
  G_ASSERT(call->types[1]->isRef() || call->types[1]->isRefType() || call->types[1]->type == das::Type::tString);
  auto fp = das::cast<DagFile *>::to(args[0]);
  auto buf = das::cast<void *>::to(args[1]);
  auto len = das::cast<int32_t>::to(args[2]);
  if (len < 0)
    return das::cast<int32_t>::from(-1);
  int res = df_read((file_ptr_t)fp, buf, len);
  return das::cast<int32_t>::from(res);
}

vec4f dag_builtin_df_write(das::Context &, das::SimNode_CallBase *call, vec4f *args)
{
  G_UNUSED(call);
  G_ASSERT(call->types[1]->isRef() || call->types[1]->isRefType() || call->types[1]->type == das::Type::tString);
  auto fp = das::cast<DagFile *>::to(args[0]);
  auto buf = das::cast<void *>::to(args[1]);
  auto len = das::cast<int32_t>::to(args[2]);
  if (len < 0)
    return das::cast<int32_t>::from(-1);
  int res = df_write((file_ptr_t)fp, buf, len);
  return das::cast<int32_t>::from(res);
}
// loads(file,block<data>)
vec4f dag_builtin_df_load(das::Context &context, das::SimNode_CallBase *call, vec4f *args)
{
  auto fp = das::cast<DagFile *>::to(args[0]);
  int32_t len = das::cast<int32_t>::to(args[1]);
  if (len < 0)
  {
    context.throw_error_ex("can't read negative number from binary save, %d", len);
    return v_zero();
  }
  das::Block *block = das::cast<das::Block *>::to(args[2]);
  char *buf = (char *)malloc(len + 1);
  vec4f bargs[1];
  int rlen = df_read(fp, buf, len);
  if (rlen != len)
  {
    bargs[0] = v_zero();
    context.invoke(*block, bargs, nullptr, &call->debugInfo);
  }
  else
  {
    buf[rlen] = 0;
    bargs[0] = das::cast<char *>::from(buf);
    context.invoke(*block, bargs, nullptr, &call->debugInfo);
  }
  free(buf);
  return v_zero();
}

static char dagorFiles_das[] =
#include "dagorFiles.das.inl"
  ;

struct DagorStatAnnotation : das::ManagedStructureAnnotation<DagorStat, true>
{
  DagorStatAnnotation(das::ModuleLibrary &ml) : das::ManagedStructureAnnotation<DagorStat, true>("DagorStat", ml)
  {
    addField<DAS_BIND_MANAGED_FIELD(size)>("size");
    addField<DAS_BIND_MANAGED_FIELD(atime)>("atime");
    addField<DAS_BIND_MANAGED_FIELD(ctime)>("ctime");
    addField<DAS_BIND_MANAGED_FIELD(mtime)>("mtime");
  }
};

int dag_df_stat(const char *path, DagorStat &buf)
{
  if (!path)
  {
    return -1;
  }
  return df_stat(path, &buf);
}

int dag_df_fstat(const DagFile *fp, DagorStat &buf) { return df_fstat((file_ptr_t)fp, &buf); }

char *dag_df_get_real_name(const char *fname, das::Context *context)
{
  if (!fname)
  {
    return nullptr;
  }
  auto rname = df_get_real_name(fname);
  if (!rname)
    return nullptr;
  return context->stringHeap->allocateString(rname, strlen(rname));
}

class DagorFiles final : public das::Module
{
public:
  DagorFiles() : das::Module("DagorFiles")
  {
    das::ModuleLibrary lib(this);
    // file annotation
    addAnnotation(das::make_smart<DagFileAnnotation>(lib));
    addAnnotation(das::make_smart<DagorStatAnnotation>(lib));
    // file constants
    das::addConstant(*this, "DF_READ", (int32_t)DF_READ);
    das::addConstant(*this, "DF_WRITE", (int32_t)DF_WRITE);
    das::addConstant(*this, "DF_CREATE", (int32_t)DF_CREATE);
    das::addConstant(*this, "DF_RDWR", (int32_t)DF_RDWR);
    das::addConstant(*this, "DF_APPEND", (int32_t)DF_APPEND);
    das::addConstant(*this, "DF_IGNORE_MISSING", (int32_t)DF_IGNORE_MISSING);
    das::addConstant(*this, "DF_VROM_ONLY", (int32_t)DF_VROM_ONLY);
    // file functions
    das::addExtern<DAS_BIND_FUN(das_df_open)>(*this, lib, "df_open", das::SideEffects::modifyExternal, "bind_dascript::das_df_open");
    das::addExtern<DAS_BIND_FUN(dag_df_close)>(*this, lib, "df_close", das::SideEffects::modifyExternal,
      "bind_dascript::dag_df_close");
    das::addExtern<DAS_BIND_FUN(dag_df_flush)>(*this, lib, "df_flush", das::SideEffects::modifyExternal,
      "bind_dascript::dag_df_flush");
    das::addExtern<DAS_BIND_FUN(dag_df_seek_to)>(*this, lib, "df_seek_to", das::SideEffects::modifyExternal,
      "bind_dascript::dag_df_seek_to");
    das::addExtern<DAS_BIND_FUN(dag_df_seek_rel)>(*this, lib, "df_seek_rel", das::SideEffects::modifyExternal,
      "bind_dascript::dag_df_seek_rel");
    das::addExtern<DAS_BIND_FUN(dag_df_seek_end)>(*this, lib, "df_seek_end", das::SideEffects::modifyExternal,
      "bind_dascript::dag_df_seek_end");
    das::addExtern<DAS_BIND_FUN(dag_df_tell)>(*this, lib, "df_tell", das::SideEffects::modifyExternal, "bind_dascript::dag_df_tell");
    das::addExtern<DAS_BIND_FUN(dag_df_length)>(*this, lib, "df_length", das::SideEffects::modifyExternal,
      "bind_dascript::dag_df_length");
    das::addExtern<DAS_BIND_FUN(dag_df_puts)>(*this, lib, "df_puts", das::SideEffects::modifyExternal, "bind_dascript::dag_df_puts");
    das::addExtern<DAS_BIND_FUN(dag_df_gets)>(*this, lib, "df_gets", das::SideEffects::modifyExternal, "bind_dascript::dag_df_gets");
    das::addExtern<DAS_BIND_FUN(dag_df_stat)>(*this, lib, "df_stat", das::SideEffects::modifyExternal, "bind_dascript::dag_df_stat");
    das::addExtern<DAS_BIND_FUN(dag_df_fstat)>(*this, lib, "df_fstat", das::SideEffects::modifyExternal,
      "bind_dascript::dag_df_fstat");
    das::addExtern<DAS_BIND_FUN(dag_df_get_real_name)>(*this, lib, "df_get_real_name", das::SideEffects::modifyExternal,
      "bind_dascript::dag_df_get_real_name");
    // builtin file functions
    das::addInterop<dag_builtin_df_read, int, const DagFile *, vec4f, int32_t>(*this, lib, "_builtin_df_read",
      das::SideEffects::modifyExternal, "bind_dascript::dag_builtin_df_read");
    das::addInterop<dag_builtin_df_write, int, const DagFile *, vec4f, int32_t>(*this, lib, "_builtin_df_write",
      das::SideEffects::modifyExternal, "bind_dascript::dag_builtin_df_write");
    das::addInterop<dag_builtin_df_load, void, const DagFile *, int32_t, const das::Block &>(*this, lib, "_builtin_df_load",
      das::SideEffects::modifyExternal, "bind_dascript::dag_builtin_df_load");
    // add builtin module
    compileBuiltinModule("dagorFiles.das", (unsigned char *)dagorFiles_das, sizeof(dagorFiles_das));

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorFiles.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

MAKE_TYPE_FACTORY(DagFile, bind_dascript::DagFile)
MAKE_TYPE_FACTORY(DagorStat, DagorStat)


REGISTER_MODULE_IN_NAMESPACE(DagorFiles, bind_dascript);
