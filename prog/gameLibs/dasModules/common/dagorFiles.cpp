// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_basePath.h>
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

char *dag_df_gets(const DagFile *fp, int maxLen, das::Context *context, das::LineInfoArg *at)
{
  if (maxLen < 0)
  {
    context->throw_error_at(at, "can't read negative amount of data %d", maxLen);
    return nullptr;
  }
  if (maxLen == 0)
    return nullptr;
  void *temp = malloc(maxLen);
  char *hstr = nullptr;
  if (char *res = df_gets((char *)temp, maxLen, (file_ptr_t)fp))
  {
    hstr = context->allocateString(res, strlen(res), at);
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

int dag_builtin_df_write(const DagFile *fp, const void *buf, int32_t len) { return df_write((file_ptr_t)fp, buf, len); }


int dag_builtin_df_load(const DagFile *fp, int32_t len,
  const das::TBlock<void, const das::TTemporary<const das::TArray<uint8_t>>> &block, das::Context *context, das::LineInfoArg *at)
{
  if (len < 0)
  {
    context->throw_error_at(at, "can't read negative amount of data %d", len);
    return 0;
  }
  char *buf = (char *)malloc(len);
  int rlen = df_read((file_ptr_t)fp, buf, len);
  das::Array arr;
  arr.data = buf;
  arr.size = rlen;
  arr.capacity = rlen;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  free(buf);
  return rlen;
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

char *dag_df_get_real_name(const char *fname, das::Context *context, das::LineInfoArg *at)
{
  if (!fname)
  {
    return nullptr;
  }
  auto rname = df_get_real_name(fname);
  if (!rname)
    return nullptr;
  return context->allocateString(rname, strlen(rname), at);
}

char *das_dd_get_named_mount_path(const char *mount_name, das::Context *context, das::LineInfoArg *at)
{
  const char *path = ::dd_get_named_mount_path(mount_name ? mount_name : "", -1);
  return path ? context->allocateString(path, strlen(path), at) : nullptr;
}


char *das_dd_resolve_named_mount(char *path, das::Context *context, das::LineInfoArg *at)
{
  if (!path)
    return nullptr;

  das::string tmp_dir_path;
  if (dd_resolve_named_mount(tmp_dir_path, path))
    return context->allocateString(tmp_dir_path.c_str(), tmp_dir_path.length(), at);

  return nullptr;
}

char *das_dd_get_named_mount_by_path(char *path, das::Context *context, das::LineInfoArg *at)
{
  if (!path)
    return nullptr;
  const char *res = dd_get_named_mount_by_path(path);
  return res ? context->allocateString(res, strlen(res), at) : nullptr;
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
    das::addExtern<DAS_BIND_FUN(dag_df_gets)>(*this, lib, "df_gets", das::SideEffects::modifyExternal, "bind_dascript::dag_df_gets");
    das::addExtern<DAS_BIND_FUN(dag_df_stat)>(*this, lib, "df_stat", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::dag_df_stat");
    das::addExtern<DAS_BIND_FUN(dag_df_fstat)>(*this, lib, "df_fstat", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::dag_df_fstat");
    das::addExtern<DAS_BIND_FUN(dag_df_get_real_name)>(*this, lib, "df_get_real_name", das::SideEffects::modifyExternal,
      "bind_dascript::dag_df_get_real_name");
    // builtin file functions
    das::addInterop<dag_builtin_df_read, int, const DagFile *, vec4f, int32_t>(*this, lib, "_builtin_df_read",
      das::SideEffects::modifyExternal, "bind_dascript::dag_builtin_df_read");
    das::addExtern<DAS_BIND_FUN(dag_builtin_df_load)>(*this, lib, "df_read", das::SideEffects::modifyExternal,
      "bind_dascript::dag_builtin_df_load");

    das::addExtern<DAS_BIND_FUN(dag_builtin_df_write)>(*this, lib, "_builtin_df_write", das::SideEffects::modifyExternal,
      "bind_dascript::dag_builtin_df_write");

    das::addExtern<DAS_BIND_FUN(das_dd_get_named_mount_path)>(*this, lib, "dd_get_named_mount_path", das::SideEffects::accessExternal,
      "bind_dascript::das_dd_get_named_mount_path");

    das::addExtern<DAS_BIND_FUN(das_dd_resolve_named_mount)>(*this, lib, "dd_resolve_named_mount", das::SideEffects::accessExternal,
      "bind_dascript::das_dd_resolve_named_mount");

    das::addExtern<DAS_BIND_FUN(das_dd_get_named_mount_by_path)>(*this, lib, "dd_get_named_mount_by_path",
      das::SideEffects::accessExternal, "bind_dascript::das_dd_get_named_mount_by_path");

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
