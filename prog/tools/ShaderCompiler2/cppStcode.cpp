// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cppStcode.h"
#include "shLog.h"
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <EASTL/string.h>


// @TODO crossplatform everything

#if _TARGET_PC_WIN
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

enum class StcodeTargetArch
{
  DEFAULT,
  X86,
  X86_64,
  ARM64
};

#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
extern dx12::dxil::Platform targetPlatform;
#endif

bool compile_cpp_stcode = false;
bool cpp_stcode_unity_build = false;
StcodeTargetArch cpp_stcode_arch = StcodeTargetArch::DEFAULT;
StcodeShader g_cppstcode;

static eastl::string stcode_dir;
static eastl::string src_dir;

bool set_stcode_arch_from_arg(const char *str)
{
  StcodeTargetArch res;
  if (strcmp(str, "x86") == 0)
    res = StcodeTargetArch::X86;
  else if (strcmp(str, "x86_64") == 0)
    res = StcodeTargetArch::X86_64;
  else if (strcmp(str, "arm64") == 0)
    res = StcodeTargetArch::ARM64;
  else
    return false;

#if _CROSS_TARGET_DX12
  if (targetPlatform != dx12::dxil::Platform::PC && res != StcodeTargetArch::X86_64)
    return false;
#elif _CROSS_TARGET_C1 | _CROSS_TARGET_C2


#endif

  cpp_stcode_arch = res;
  return true;
}

static bool current_arch_is_valid(StcodeTargetArch desired)
{
  return cpp_stcode_arch == StcodeTargetArch::DEFAULT || cpp_stcode_arch == desired;
}

static const char *arch_name(StcodeTargetArch arch)
{
  switch (arch)
  {
    case StcodeTargetArch::DEFAULT: return "<default>";
    case StcodeTargetArch::X86: return "x86";
    case StcodeTargetArch::X86_64: return "x86_64";
    case StcodeTargetArch::ARM64: return "arm64";
  }
  return nullptr;
}

static void replace_char(char *str, size_t len, char old_ch, char new_ch)
{
  for (char *p = str; p != str + len; ++p)
  {
    if (*p == old_ch)
      *p = new_ch;
  }
}

void prepare_stcode_directory(const char *dest_dir)
{
  RET_IF_SHOULD_NOT_COMPILE();

  stcode_dir = eastl::string(eastl::string::CtorSprintf{}, "%s/stcode", dest_dir);
  dd_simplify_fname_c(stcode_dir.data());
  stcode_dir.resize(strlen(stcode_dir.c_str()));
  replace_char(stcode_dir.data(), stcode_dir.length(), '\\', '/');

  src_dir = eastl::string(eastl::string::CtorSprintf{}, "%s/src", stcode_dir.c_str());

  auto prep_dir = [](const char *dirname) {
    if (!dd_dir_exist(dirname))
      dd_mkdir(dirname);
  };

  prep_dir(stcode_dir.c_str());
  prep_dir(src_dir.c_str());
}

static void write_file(const char *fn, const char *src, size_t src_len)
{
  file_ptr_t f = df_open(fn, DF_WRITE | DF_CREATE);
  if (!f)
  {
    sh_debug(SHLOG_ERROR, "Failed to open file %s for write", fn);
    return;
  }
  int charsWritten = df_write(f, src, src_len);
  if (charsWritten != src_len)
    sh_debug(SHLOG_ERROR, "Failed write full file (%s)", fn);

  df_close(f);
  return;
}

void save_compiled_cpp_stcode(StcodeShader &&cpp_shader)
{
  RET_IF_SHOULD_NOT_COMPILE();

  if (!cpp_shader.hasCode)
    return;

  constexpr char headerTemplate[] =
#include "_stcodeTemplates/codeFile.h.fmt"
    ;
  constexpr char cppTemplate[] =
#include "_stcodeTemplates/codeFile.cpp.fmt"
    ;

  StcodeStrings codeStrings = cpp_shader.code.release();
  StcodeGlobalVars::Strings globvarsStrings = cpp_shader.globVars.releaseAssembledCode();

  const eastl::string headerFileName(eastl::string::CtorSprintf{}, "%s/%s.stcode.gen.h", src_dir.c_str(),
    cpp_shader.shaderName.c_str());
  const eastl::string cppFileName(eastl::string::CtorSprintf{}, "%s/%s.stcode.gen.cpp", src_dir.c_str(),
    cpp_shader.shaderName.c_str());

  const eastl::string headerSource(eastl::string::CtorSprintf{}, headerTemplate, cpp_shader.shaderName.c_str(),
    cpp_shader.shaderName.c_str(), codeStrings.headerCode.c_str());
  const eastl::string cppSource(eastl::string::CtorSprintf{}, cppTemplate, cpp_shader.shaderName.c_str(),
    cpp_shader.shaderName.c_str(), globvarsStrings.fetchersOrFwdCppCode.c_str(), cpp_shader.shaderName.c_str(),
    codeStrings.cppCode.c_str());

  write_file(headerFileName.c_str(), headerSource.c_str(), headerSource.length());
  write_file(cppFileName.c_str(), cppSource.c_str(), cppSource.length());
}

void save_stcode_dll_main(StcodeInterface &&cpp_interface)
{
  RET_IF_SHOULD_NOT_COMPILE();

  constexpr char cppTemplate[] =
#include "_stcodeTemplates/main.cpp.fmt"
    ;

  eastl::string code = cpp_interface.releaseAssembledCode();

  const eastl::string cppFileName(eastl::string::CtorSprintf{}, "%s/stcode_main.stcode.gen.cpp", stcode_dir.c_str());

  // For unity build, paste cpps as is. Otherwise, use headers for fwd decl
  const char *includesExtForBuildType = cpp_stcode_unity_build ? "cpp" : "h";

  eastl::string includes = "";
  const eastl::string matchPath(eastl::string::CtorSprintf{}, "%s/*.stcode.gen.%s", src_dir.c_str(), includesExtForBuildType);
  alefind_t ff;
  if (dd_find_first(matchPath.c_str(), DA_FILE, &ff))
  {
    do
      includes.append_sprintf("#include \"src/%s\"\n", ff.name);
    while (dd_find_next(&ff));
  }

  const eastl::string cppSource(eastl::string::CtorSprintf{}, cppTemplate, includes.c_str(), code.c_str());

  write_file(cppFileName.c_str(), cppSource.c_str(), cppSource.length());
}

void save_stcode_global_vars(StcodeGlobalVars &&cpp_globvars)
{
  RET_IF_SHOULD_NOT_COMPILE();

  constexpr char cppTemplate[] =
#include "_stcodeTemplates/globVars.cpp.fmt"
    ;

  StcodeGlobalVars::Strings code = cpp_globvars.releaseAssembledCode();

  const eastl::string cppFileName(eastl::string::CtorSprintf{}, "%s/glob_shadervars.stcode.gen.cpp", stcode_dir.c_str());
  const eastl::string cppSource(eastl::string::CtorSprintf{}, cppTemplate, code.cppCode.c_str(), code.fetchersOrFwdCppCode.c_str());

  write_file(cppFileName.c_str(), cppSource.c_str(), cppSource.length());
}


struct DirRootInfo
{
  int rootDepthFromCwd;
  int rootDepthFromOutput;
  eastl::string pathRelToRoot;
};
static DirRootInfo get_dir_root_info(const char *path)
{
  // We presume that path is simplified
  DirRootInfo info;

  const char *p = path;

  // skip ../../../ ... trail
  constexpr char tail[] = "../";
  constexpr size_t tailLen = sizeof(tail) - 1;

  info.rootDepthFromCwd = 0;
  for (; strncmp(p, tail, tailLen) == 0; p += tailLen)
    ++info.rootDepthFromCwd;

  info.rootDepthFromOutput = 0;
  for (const char *rem = p; rem; rem = strchr(rem, '/'))
  {
    ++info.rootDepthFromOutput;
    ++rem;
  }

  info.pathRelToRoot = eastl::string(p);
  return info;
}

static eastl::string build_root_rel_path(int depth)
{
  if (depth == 0)
    return eastl::string(".");
  else
  {
    eastl::string res = "";
    for (int i = 0; i < depth - 1; ++i)
      res += "../";
    res += "..";
    return res;
  }
}

static eastl::string get_cwd()
{
  char buf[DAGOR_MAX_PATH * 4];
  if (getcwd(buf, sizeof(buf)))
  {
    dd_simplify_fname_c(buf);
    replace_char(buf, sizeof(buf), '\\', '/');
    return eastl::string(buf);
  }

  return eastl::string();
}

static eastl::string get_cwd_rel_to_root(int root_depth)
{
  if (root_depth == 0)
    return eastl::string(".");

  const eastl::string cwd = get_cwd();
  if (cwd.empty())
  {
    sh_debug(SHLOG_FATAL, "Failed to get current working directory");
    return eastl::string();
  }

  int slashCount = -1;
  for (const char *p = cwd.c_str(); p; p = strchr(p, '/'))
  {
    ++slashCount;
    ++p;
  }

  if (slashCount <= root_depth)
  {
    sh_debug(SHLOG_FATAL, "Root depth %d is deeper than absolute path %s");
    return eastl::string();
  }

  const char *p = cwd.cbegin();
  if (*p == '/') // for unix abs pathes
    ++p;
  for (int i = 0; i < slashCount - root_depth + 1; ++i)
  {
    p = strchr(p, '/');
    ++p;
  }

  return eastl::string(p);
}

static eastl::string get_dynlib_jam_out_dir(const char *out_dir, int root_depth)
{
  const eastl::string relCwd = get_cwd_rel_to_root(root_depth);
  eastl::string outDirRel(eastl::string::CtorSprintf{}, "%s/%s", relCwd.c_str(), out_dir);
  dd_simplify_fname_c(outDirRel.data());
  outDirRel.resize(strlen(outDirRel.c_str()));
  return outDirRel;
}

static eastl::string get_cpp_filename_from_source_name(const char *shader_source_name, size_t name_len)
{
  const char *begin = dd_get_fname(shader_source_name);
  const char *end = dd_get_fname_ext(shader_source_name);
  if (!end)
    end = shader_source_name + name_len;

  G_ASSERT(end - begin > 0);

  return eastl::string("src/") + eastl::string(begin, end - begin) + eastl::string(".stcode.gen.cpp");
}

dag::Expected<proc::ProcessTask, StcodeMakeTaskError> make_stcode_compilation_task(const char *out_dir, const char *lib_name,
  const ShVariantName &variant)
{
  RET_IF_SHOULD_NOT_COMPILE(dag::Unexpected(StcodeMakeTaskError::DISABLED));

  // Calculate root path, stcode path relative to root and output dir relative to root
  const auto [rootDepthFromCwd, rootDepthFromOutput, stcodeRelPath] = get_dir_root_info(stcode_dir.c_str());
  const eastl::string rootRelPath = build_root_rel_path(rootDepthFromOutput);
  const eastl::string outDirRelPath = get_dynlib_jam_out_dir(out_dir, rootDepthFromCwd);

  eastl::string sourcesList;

  if (cpp_stcode_unity_build)
    sourcesList = "";
  else
  {
    StcodeBuilder strBuilder{};
    for (const String &shname : variant.sourceFilesList)
    {
      eastl::string srcFileName = get_cpp_filename_from_source_name(shname.c_str(), shname.length());

      // @TODO: this will not work if a dshl file used to have a shader in it => generated a cpp file previously, but then was just
      // included w/ macros & shadervars. In this case one would need to manually delete the cpp file. This is very improbable, but
      // should be fixed.
      if (dd_file_exist((stcode_dir + "/" + srcFileName).c_str()))
        strBuilder.emplaceBack(eastl::move(srcFileName + "\n"));
    }

    sourcesList = strBuilder.release();
  }

  // Write jamfile
  const char jamTemplate[] =
#include "_stcodeTemplates/jamfile.fmt"
    ;

  constexpr char extension[] =
#if _TARGET_PC_WIN && (_CROSS_TARGET_C1 | _CROSS_TARGET_C2)
    ".prx"
#else // @TODO: compile .so on win too? .dylib definitely should not work, but maybe too
    DAGOR_OS_DLL_SUFFIX
#endif
    ;

  const char *customCppOpts = cpp_stcode_unity_build ? "-DROUTINE_VISIBILITY=static" : "-DROUTINE_VISIBILITY=extern";

  const eastl::string jamPath(eastl::string::CtorSprintf{}, "%s/jamfile", stcode_dir.c_str());
  const eastl::string jamContent(eastl::string::CtorSprintf{}, jamTemplate, rootRelPath.c_str(), stcodeRelPath.c_str(),
    outDirRelPath.c_str(), lib_name, extension, sourcesList.c_str(), customCppOpts);
  write_file(jamPath.c_str(), jamContent.c_str(), jamContent.length());

  proc::ProcessTask task{};
  task.isExternal = true;

  const char *targetType = "dll";
  StcodeTargetArch arch = StcodeTargetArch::DEFAULT;

#define SET_ARCH_WITH_DEF(default_)                                                   \
  do                                                                                  \
  {                                                                                   \
    arch = cpp_stcode_arch == StcodeTargetArch::DEFAULT ? default_ : cpp_stcode_arch; \
  } while (0)

  // Start proc
#if _TARGET_PC_WIN

  // @TODO: win32 w/ a compile flag
  const char *jamPlatform = nullptr;
  SET_ARCH_WITH_DEF(StcodeTargetArch::X86_64);

#if _CROSS_TARGET_DX12
  switch (targetPlatform)
  {
    case dx12::dxil::Platform::PC: jamPlatform = "windows"; break;
    case dx12::dxil::Platform::XBOX_ONE: jamPlatform = "xboxOne"; break;
    case dx12::dxil::Platform::XBOX_SCARLETT: jamPlatform = "scarlett"; break;
  }
#elif _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV
  jamPlatform = "windows";
#elif _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#endif

  G_ASSERT(jamPlatform);

  eastl::string stcodeDirWin = stcode_dir;
  replace_char(stcodeDirWin.data(), stcodeDirWin.length(), '/', '\\');

  task.argv.emplace_back("C:\\windows\\system32\\cmd.exe");
  task.argv.emplace_back("/c");
  task.argv.emplace_back("jam.exe");
  task.argv.push_back(eastl::string(eastl::string::CtorSprintf{}, "-sPlatform=%s", jamPlatform));
  task.argv.push_back(eastl::string(eastl::string::CtorSprintf{}, "-sPlatformArch=%s", arch_name(arch)));
  task.argv.push_back(eastl::string(eastl::string::CtorSprintf{}, "-sTargetType=%s", targetType));

  task.cwd.emplace(eastl::move(stcodeDirWin));

#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX

#if _TARGET_PC_LINUX && !_CROSS_TARGET_SPIRV
#error Only spirv should be compiled on linux
#elif _TARGET_PC_MAC && !_CROSS_TARGET_METAL
#error Only metal should be compiled on mac
#endif

#if _TARGET_PC_LINUX
  constexpr char jamPlatform[] = "linux";
  SET_ARCH_WITH_DEF(StcodeTargetArch::X86_64);
#else
  constexpr char jamPlatform[] = "macOS";
  SET_ARCH_WITH_DEF(StcodeTargetArch::ARM64);
#endif

  task.argv.emplace_back("jam");
  task.argv.push_back(eastl::string(eastl::string::CtorSprintf{}, "-sPlatform=%s", jamPlatform));
  task.argv.push_back(eastl::string(eastl::string::CtorSprintf{}, "-sPlatformArch=%s", arch_name(arch)));
  task.argv.push_back(eastl::string(eastl::string::CtorSprintf{}, "-sTargetType=%s", targetType));

  task.cwd.emplace(stcode_dir);

#else // Stub

  return dag::Unexpected(StcodeMakeTaskError::DISABLED);

#endif

#undef VERIFY_AND_SET_ARCH
#undef SET_ARCH_WITH_DEF

  return task;
}
