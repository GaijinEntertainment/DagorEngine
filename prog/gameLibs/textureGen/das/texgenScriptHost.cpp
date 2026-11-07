// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "texgenScriptHost_api.h"
#include "texgenModule_api.h"
#include <gameLibs/textureGen/graph/texgenGraph_api.h>
#include <daScript/daScript.h>
#include <daScript/daScriptModule.h>
#include <daScript/misc/sysos.h>
#include <daScript/simulate/fs_file_info.h>
#include <string.h>

static void pull_texgen_modules()
{
  NEED_MODULE(Module_BuiltIn);
  NEED_MODULE(Module_Math);
  NEED_MODULE(Module_Strings);
  NEED_MODULE(TexgenModule);
  NEED_FUSION;
}

namespace texgen
{
void texgen_das_pull_modules() { pull_texgen_modules(); }

// Every program requires the das part of the builtin module; the host carries it, so no das root is needed on disk.
// Bytes, as one 80 KB string literal is over the MSVC limit.
static const unsigned char builtin_das[] = {
#include "builtin.das.inl"
};

namespace
{
constexpr const char *NODES_MODULE = "texgen_nodes";

// Serves the nodes module and the one script; no other file is read and only the sandbox modules resolve.
class SandboxFileAccess final : public das::FsFileAccess
{
public:
  explicit SandboxFileAccess(const char *script_path) : scriptPath(script_path) {}

  void serve(const char *name, const char *text)
  {
    setFileInfo(name, das::make_unique<das::TextFileInfo>(text, uint32_t(strlen(text)), false));
  }

  das::ModuleInfo getModuleInfo(const das::string &req, const das::string &from) const override
  {
    if (req == NODES_MODULE)
    {
      das::ModuleInfo info;
      info.moduleName = NODES_MODULE;
      info.fileName = NODES_DAS;
      return info;
    }
    return das::FileAccess::getModuleInfo(req, from);
  }

  // lint = false would skip the unsafe check.
  bool isOptionAllowed(const das::string &opt, const das::string &from) const override
  {
    return from != scriptPath || opt == "gen2" || opt == "indenting";
  }

  bool isModuleAllowed(const das::string &mod, const das::string &) const override
  {
    static const char *const allowed[] = {"$", "builtin", "math", "strings", "texgen", NODES_MODULE};
    for (const char *name : allowed)
      if (mod == name)
        return true;
    return false;
  }

protected:
  das::FileInfo *getNewFileInfo(const das::string &file_name) override
  {
    return file_name == scriptPath ? das::FsFileAccess::getNewFileInfo(file_name) : nullptr;
  }

private:
  das::string scriptPath;
};

void append_at(eastl::string &errors, const das::LineInfo &at, const char *text)
{
  eastl::string t(text);
  while (!t.empty() && (t.back() == '\n' || t.back() == ' '))
    t.pop_back();
  if (at.fileInfo)
    errors.append_sprintf("%s:%d: %s\n", at.fileInfo->name.c_str(), int(at.line), t.c_str());
  else
    errors.append_sprintf("%s\n", t.c_str());
}

void append_program_errors(const das::Program &program, eastl::string &errors)
{
  for (const das::Error &e : program.errors)
  {
    eastl::string text(e.what.c_str());
    if (!e.extra.empty())
      text.append_sprintf("\n%s", e.extra.c_str());
    append_at(errors, e.at, text.c_str());
  }
}
} // namespace

bool texgen_run_script(const char *script_path_or_name, const char *script_text, const char *nodes_module_das_text, Graph &out,
  eastl::string &errors)
{
  out = Graph{};
  texgen_graph_reset();
  auto access = das::make_smart<SandboxFileAccess>(script_path_or_name);
  access->serve((das::getDasRoot() + "/daslib/builtin.das").c_str(), reinterpret_cast<const char *>(builtin_das));
  access->serve(NODES_DAS, nodes_module_das_text ? nodes_module_das_text : "");
  if (script_text)
    access->serve(script_path_or_name, script_text);

  das::TextWriter logs;
  das::ModuleGroup group;
  das::CodeOfPolicies policies;
  policies.version_2_syntax = true;
  policies.gen2_make_syntax = true;
  policies.no_unsafe = true;
  policies.disable_auto_inline = true;
  das::ProgramPtr program = das::compileDaScript(script_path_or_name, access, logs, group, policies);
  if (!program || program->failed())
  {
    if (program)
      append_program_errors(*program, errors);
    else
      errors.append_sprintf("%s: does not compile\n", script_path_or_name);
    return false;
  }

  das::Context ctx(program->getContextStackSize());
  if (!program->simulate(ctx, logs))
  {
    append_program_errors(*program, errors);
    return false;
  }
  das::SimFunction *fn = nullptr;
  for (das::SimFunction *f : ctx.findFunctions("graph"))
    if (f->debugInfo && f->debugInfo->count == 0 && !f->cmres)
      fn = f;
  if (!fn)
  {
    errors.append_sprintf("%s: no [export] def graph() that takes no arguments and returns no structure\n", script_path_or_name);
    return false;
  }

  ctx.evalWithCatch(fn, nullptr);
  if (const char *exc = ctx.getException())
  {
    append_at(errors, ctx.exceptionAt, exc);
    return false;
  }
  out = eastl::move(texgen_graph());
  return validate(out, errors);
}
} // namespace texgen
