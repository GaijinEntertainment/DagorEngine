// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cppStcode.h"
#include "shLog.h"
#include "fast_isalnum.h"
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <EASTL/string.h>


#if _TARGET_PC_WIN
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

StcodeShader g_cppstcode;

static eastl::string stcode_dir;
static eastl::string src_dir;

template <typename T>
static bool item_is_in(const T &item, std::initializer_list<T> items)
{
  for (const T &ref : items)
  {
    if (ref == item)
      return true;
  }
  return false;
}

static const char *arch_name(StcodeTargetArch arch)
{
  switch (arch)
  {
    case StcodeTargetArch::DEFAULT: return "<default>";
    case StcodeTargetArch::X86: return "x86";
    case StcodeTargetArch::X86_64: return "x86_64";
    case StcodeTargetArch::ARM64: return "arm64";
    case StcodeTargetArch::ARM64_E: return "arm64e";
    case StcodeTargetArch::ARM_V7: return "armv7";
    case StcodeTargetArch::ARM_V7S: return "armv7s";
    case StcodeTargetArch::ARMEABI_V7A: return "armeabi-v7a";
    case StcodeTargetArch::ARM64_V8A: return "arm64-v8a";
    case StcodeTargetArch::I386: return "i386";
    case StcodeTargetArch::E2K: return "e2k";
  }
  return nullptr;
}

static bool validate_arch(const shc::CompilerConfig &config)
{
#if _CROSS_TARGET_DX12
  if (shc::config().targetPlatform != dx12::dxil::Platform::PC)
    return config.cppStcodeArch == StcodeTargetArch::X86_64;
  else
    return item_is_in(config.cppStcodeArch, {StcodeTargetArch::X86, StcodeTargetArch::X86_64, StcodeTargetArch::ARM64});
#elif _CROSS_TARGET_DX11
  return item_is_in(config.cppStcodeArch, {StcodeTargetArch::X86, StcodeTargetArch::X86_64, StcodeTargetArch::ARM64});
#elif _CROSS_TARGET_C1 | _CROSS_TARGET_C2

#elif _CROSS_TARGET_SPIRV
#if _TARGET_PC_WIN
  return item_is_in(config.cppStcodeArch,
    {StcodeTargetArch::DEFAULT, StcodeTargetArch::X86, StcodeTargetArch::X86_64, StcodeTargetArch::E2K, StcodeTargetArch::ARM64,
      StcodeTargetArch::ARMEABI_V7A, StcodeTargetArch::ARM64_V8A});
#elif _TARGET_PC_LINUX
  return item_is_in(config.cppStcodeArch, {StcodeTargetArch::DEFAULT, StcodeTargetArch::X86_64, StcodeTargetArch::E2K,
                                            StcodeTargetArch::ARM64, StcodeTargetArch::ARMEABI_V7A, StcodeTargetArch::ARM64_V8A});
#elif _TARGET_PC_MACOSX
  return item_is_in(config.cppStcodeArch, {StcodeTargetArch::ARMEABI_V7A, StcodeTargetArch::ARM64_V8A});
#endif
#elif _CROSS_TARGET_METAL
  return item_is_in(config.cppStcodeArch,
    {StcodeTargetArch::DEFAULT, StcodeTargetArch::X86_64, StcodeTargetArch::ARM64, StcodeTargetArch::ARM64_E, StcodeTargetArch::ARM_V7,
      StcodeTargetArch::ARM_V7S, StcodeTargetArch::I386});
#else
#error Unknown target
#endif
}

static bool validate_arch_for_platform(const shc::CompilerConfig &config);

#if _CROSS_TARGET_SPIRV

static bool validate_arch_for_platform(const shc::CompilerConfig &config)
{
  return config.cppStcodeArch == StcodeTargetArch::DEFAULT || config.cppStcodePlatform == StcodeTargetPlatform::DEFAULT ||
         (config.cppStcodePlatform == StcodeTargetPlatform::NSWITCH && config.cppStcodeArch == StcodeTargetArch::ARM64) ||
         (config.cppStcodePlatform == StcodeTargetPlatform::ANDROID &&
           item_is_in(config.cppStcodeArch, {StcodeTargetArch::ARMEABI_V7A, StcodeTargetArch::ARM64_V8A})) ||
#if _TARGET_PC_WIN
         (config.cppStcodePlatform == StcodeTargetPlatform::PC &&
           item_is_in(config.cppStcodeArch, {StcodeTargetArch::X86, StcodeTargetArch::X86_64, StcodeTargetArch::ARM64}));
#elif _TARGET_PC_LINUX
         (config.cppStcodePlatform == StcodeTargetPlatform::PC &&
           item_is_in(config.cppStcodeArch, {StcodeTargetArch::X86_64, StcodeTargetArch::E2K}));
#elif _TARGET_PC_MACOSX
         false; // android and {armv7a, armv8a} is already listed in common code above
#endif
}

bool set_stcode_platform_from_arg(const char *str, shc::CompilerConfig &config)
{
  if (strcmp(str, "pc") == 0)
    config.cppStcodePlatform = StcodeTargetPlatform::PC;
  else if (strcmp(str, "android") == 0)
    config.cppStcodePlatform = StcodeTargetPlatform::ANDROID;
  else if (strcmp(str, "nswitch") == 0)
    config.cppStcodePlatform = StcodeTargetPlatform::NSWITCH;
  else
    return false;

  return validate_arch_for_platform(config);
}

template <typename T>
static T platform_switch(T &&if_pc, T &&if_android, T &&if_nswitch)
{
  switch (shc::config().cppStcodePlatform)
  {
    case StcodeTargetPlatform::DEFAULT:
    case StcodeTargetPlatform::PC: return eastl::forward<T>(if_pc);
    case StcodeTargetPlatform::ANDROID: return eastl::forward<T>(if_android);
    case StcodeTargetPlatform::NSWITCH: return eastl::forward<T>(if_nswitch);
  }
}

#elif _CROSS_TARGET_METAL

static bool validate_arch_for_platform(const shc::CompilerConfig &config)
{
  return config.cppStcodeArch == StcodeTargetArch::DEFAULT || config.cppStcodePlatform == StcodeTargetPlatform::DEFAULT ||
         (config.cppStcodePlatform == StcodeTargetPlatform::MACOSX &&
           item_is_in(config.cppStcodeArch, {StcodeTargetArch::X86_64, StcodeTargetArch::ARM64})) ||
         (config.cppStcodePlatform == StcodeTargetPlatform::IOS &&
           item_is_in(config.cppStcodeArch, {StcodeTargetArch::ARM64, StcodeTargetArch::ARM64_E, StcodeTargetArch::ARM_V7,
                                              StcodeTargetArch::ARM_V7S, StcodeTargetArch::I386}));
}

bool set_stcode_platform_from_arg(const char *str, shc::CompilerConfig &config)
{
  if (strcmp(str, "macosx") == 0)
    config.cppStcodePlatform = StcodeTargetPlatform::MACOSX;
  else if (strcmp(str, "ios") == 0)
    config.cppStcodePlatform = StcodeTargetPlatform::IOS;
  else
    return false;

  return validate_arch_for_platform(config);
}

static bool is_ios() { return shc::config().cppStcodePlatform == StcodeTargetPlatform::IOS; }

template <typename T>
static T if_ios(T &&if_true, T &&if_false)
{
  if (is_ios())
    return eastl::forward<T>(if_true);
  else
    return eastl::forward<T>(if_false);
}

#else

static bool validate_arch_for_platform(const shc::CompilerConfig &) { return true; }

#endif

bool set_stcode_arch_from_arg(const char *str, shc::CompilerConfig &config_rw)
{
  StcodeTargetArch res = StcodeTargetArch::DEFAULT;
  for (StcodeTargetArch arch : ALL_STCODE_ARCHS)
  {
    if (arch != StcodeTargetArch::DEFAULT && strcmp(str, arch_name(arch)) == 0)
    {
      res = arch;
      break;
    }
  }
  if (res == StcodeTargetArch::DEFAULT) // Not found or invalid
    return false;


  config_rw.cppStcodeArch = res;

  return validate_arch(config_rw) && validate_arch_for_platform(config_rw);
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

void save_stcode_dll_main(StcodeInterface &&cpp_interface, uint64_t stcode_hash)
{
  RET_IF_SHOULD_NOT_COMPILE();

  constexpr char cppTemplate[] =
#include "_stcodeTemplates/main.cpp.fmt"
    ;

  eastl::string code = cpp_interface.releaseAssembledCode();

  const eastl::string cppFileName(eastl::string::CtorSprintf{}, "%s/stcode_main.stcode.gen.cpp", stcode_dir.c_str());

  // For unity build, paste cpps as is. Otherwise, use headers for fwd decl
  const char *includesExtForBuildType = shc::config().cppStcodeUnityBuild ? "cpp" : "h";

  eastl::string includes = "";
  const eastl::string matchPath(eastl::string::CtorSprintf{}, "%s/*.stcode.gen.%s", src_dir.c_str(), includesExtForBuildType);
  alefind_t ff;
  if (dd_find_first(matchPath.c_str(), DA_FILE, &ff))
  {
    do
      includes.append_sprintf("#include \"src/%s\"\n", ff.name);
    while (dd_find_next(&ff));
  }

  const eastl::string cppSource(eastl::string::CtorSprintf{}, cppTemplate, includes.c_str(), code.c_str(), stcode_hash);

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

static eastl::string remove_lowest_dir(const char *path, size_t path_len)
{
  const char *p = strchr(path, '/');
  return p ? eastl::string{p + 1, path_len - (path - p) - 1} : eastl::string{path, path_len};
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

static bool path_is_abs(const char *path)
{
  G_ASSERT(path);
#if _TARGET_PC_WIN
  if (strchr(path, ':'))
    return true;
#endif
  return path[0] == '/';
}

struct JamCompilationDirs
{
  eastl::string root;
  eastl::string location;
  eastl::string outDir;
};

JamCompilationDirs generate_dirs_with_provided_root(const char *engine_root, const char *out_dir)
{
  const eastl::string cwd = get_cwd();

  auto toAbs = [&cwd](const char *src) {
    eastl::string res{src};
    if (!path_is_abs(src))
      res = cwd + "/" + res;
    dd_simplify_fname_c(res.data());
    res.resize(strlen(res.c_str()));
    if (res.back() == '/' && res.size() > 0)
      res.pop_back();
    return res;
  };

  auto tracebackPathFrom = [](const char *from) {
    int dirCount = 0;
    for (const char *p = from; p; p = strchr(p, '/'))
    {
      ++dirCount;
      ++p;
    }

    constexpr char UNIT[] = "../";
    constexpr size_t UNIT_LEN = sizeof(UNIT) - 1;
    eastl::string res{};
    res.resize(UNIT_LEN * dirCount);
    for (int i = 0; i < dirCount; ++i)
      memcpy(res.data() + UNIT_LEN * i, UNIT, UNIT_LEN);

    res.pop_back();
    return res;
  };

  auto makeRelPath = [&](const char *from, const char *to) {
    const char *pf = from, *pt = to;
    for (; *pf && *pt && *pf == *pt; ++pf, ++pt)
      ;

    if (*pf == '\0' && *pt == '\0')
      return eastl::string{"./"};
    else if (*pf == '\0' && *pt == '/')
      return eastl::string{pt + 1};
    else if (*pf == '/' && *pt == '\0')
      return tracebackPathFrom(pf + 1);
    else
    {
      do
      {
        --pf, --pt;
      } while (*pf != '/' && *pt != '/');
      return tracebackPathFrom(pf + 1) + "/" + eastl::string{pt + 1};
    }
  };

  const eastl::string outDirAbs = toAbs(out_dir);
  const eastl::string engineRootAbs = toAbs(engine_root);
  const eastl::string stcodeDirAbs = toAbs(stcode_dir.c_str());

  JamCompilationDirs res{};
  res.root = makeRelPath(stcodeDirAbs.c_str(), engineRootAbs.c_str());
  res.location = makeRelPath(engineRootAbs.c_str(), stcodeDirAbs.c_str());
  res.outDir = makeRelPath(engineRootAbs.c_str(), outDirAbs.c_str());
  return res;
}

JamCompilationDirs generate_dirs_with_inferred_root_from_rel_path(const char *out_dir)
{
  sh_debug(SHLOG_NORMAL,
    "[WARNING] engineRootDir var not found in blk, trying to generate stcode compilation paths from output path (%s). Note that "
    "this path must be relative to cwd and passing through engine root, or the build will fail!",
    stcode_dir.c_str());

  if (path_is_abs(stcode_dir.c_str()))
  {
    sh_debug(SHLOG_FATAL, "output path (%s) can not be absolute when inferring root path", stcode_dir.c_str());
    return {};
  }

  const auto [rootDepthFromCwd, rootDepthFromOutput, stcodeRelPath] = get_dir_root_info(stcode_dir.c_str());
  return {build_root_rel_path(rootDepthFromOutput), eastl::move(stcodeRelPath), get_dynlib_jam_out_dir(out_dir, rootDepthFromCwd)};
}

dag::Expected<proc::ProcessTask, StcodeMakeTaskError> make_stcode_compilation_task(const char *out_dir, const char *lib_name,
  const ShVariantName &variant)
{
  RET_IF_SHOULD_NOT_COMPILE(dag::Unexpected(StcodeMakeTaskError::DISABLED));

  // Calculate root path, stcode path relative to root and output dir relative to root
  const char *engineRoot = shc::config().engineRootDir.empty() ? nullptr : shc::config().engineRootDir.c_str();
  auto [rootRelPath, stcodeRelPath, outDirRelPath] =
    engineRoot ? generate_dirs_with_provided_root(engineRoot, out_dir) : generate_dirs_with_inferred_root_from_rel_path(out_dir);

  eastl::string sourcesList;

  if (shc::config().cppStcodeUnityBuild)
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
  constexpr char jamTemplate[] =
#include "_stcodeTemplates/jamfile.fmt"
    ;

  // Override inappropriate target mangling when autocompleting target for linux pc and macos
  const char *extension =
#if _TARGET_PC_LINUX & _CROSS_TARGET_SPIRV
    platform_switch<const char *>(".so", "", "")
#elif _TARGET_PC_MACOSX & _CROSS_TARGET_METAL
    if_ios<const char *>("", ".dylib")
#else
    ""
#endif
    ;

  eastl::string targetRelName{};

  // @HACK Due to jam limits on string size, the name needs to be mangled to be shortened
  {
    String targetRelNameBase{remove_lowest_dir(stcodeRelPath.c_str(), stcodeRelPath.length()).c_str()};
    targetRelNameBase.replace("/stcode", "");
    replace_char(targetRelNameBase.data(), targetRelNameBase.length(), '/', '~');

    // Now reduce each word to first three letters
    constexpr int REDUCED_TOKEN_SIZE = 3;

    targetRelName.reserve(targetRelNameBase.size());
    int charStreak = 0;
    for (char c : targetRelNameBase)
    {
      if (fast_isalnum(c))
        ++charStreak;
      else
        charStreak = 0;
      if (charStreak < REDUCED_TOKEN_SIZE)
        targetRelName.push_back(c);
    }
  }

  const char *customCppOpts = shc::config().cppStcodeUnityBuild ? "-DROUTINE_VISIBILITY=static" : "-DROUTINE_VISIBILITY=extern";

  const eastl::string jamPath(eastl::string::CtorSprintf{}, "%s/jamfile", stcode_dir.c_str());
  const eastl::string jamContent(eastl::string::CtorSprintf{}, jamTemplate, rootRelPath.c_str(), stcodeRelPath.c_str(),
    outDirRelPath.c_str(), targetRelName.c_str(), lib_name, extension, sourcesList.c_str(), customCppOpts);
  write_file(jamPath.c_str(), jamContent.c_str(), jamContent.length());

  proc::ProcessTask task{};
  task.isExternal = true;

  const char *targetType = "dll";
  StcodeTargetArch arch = StcodeTargetArch::DEFAULT;

#define SET_ARCH_WITH_DEF(default_) \
  arch = (shc::config().cppStcodeArch == StcodeTargetArch::DEFAULT ? default_ : shc::config().cppStcodeArch)

  // Start proc
#if _TARGET_PC_WIN

  const char *jamPlatform = nullptr;

#if _CROSS_TARGET_SPIRV
  SET_ARCH_WITH_DEF(platform_switch(StcodeTargetArch::X86_64, StcodeTargetArch::ARM64_V8A, StcodeTargetArch::ARM64));
#else
  SET_ARCH_WITH_DEF(StcodeTargetArch::X86_64);
#endif

#if _CROSS_TARGET_DX12
  switch (shc::config().targetPlatform)
  {
    case dx12::dxil::Platform::PC: jamPlatform = "windows"; break;
    case dx12::dxil::Platform::XBOX_ONE: jamPlatform = "xboxOne"; break;
    case dx12::dxil::Platform::XBOX_SCARLETT: jamPlatform = "scarlett"; break;
  }
#elif _CROSS_TARGET_DX11
  jamPlatform = "windows";
#elif _CROSS_TARGET_SPIRV
  jamPlatform = platform_switch<const char *>("windows", "android", "nswitch");
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
  const char *jamPlatform = platform_switch<const char *>("linux", "android", "nswitch");
  SET_ARCH_WITH_DEF(platform_switch(StcodeTargetArch::X86_64, StcodeTargetArch::ARM64_V8A, StcodeTargetArch::ARM64));
#else
#if _CROSS_TARGET_METAL
  const char *jamPlatform = if_ios<const char *>("iOS", "macOS");
  SET_ARCH_WITH_DEF(if_ios(StcodeTargetArch::ARM64, StcodeTargetArch::X86_64));
#else
  const char *jamPlatform = "android";
  SET_ARCH_WITH_DEF(StcodeTargetArch::ARM64_V8A);
#endif
#endif

  task.argv.emplace_back("jam");
  task.argv.push_back(eastl::string(eastl::string::CtorSprintf{}, "-sPlatform=%s", jamPlatform));
  task.argv.push_back(eastl::string(eastl::string::CtorSprintf{}, "-sPlatformArch=%s", arch_name(arch)));
  task.argv.push_back(eastl::string(eastl::string::CtorSprintf{}, "-sTargetType=%s", targetType));

#if _TARGET_PC_MACOSX && _CROSS_TARGET_METAL
  if (is_ios())
    task.argv.push_back(eastl::string("-sBuildAsFramework=yes"));
#endif

  task.cwd.emplace(stcode_dir);

#else // Stub

  return dag::Unexpected(StcodeMakeTaskError::DISABLED);

#endif

#undef VERIFY_AND_SET_ARCH
#undef SET_ARCH_WITH_DEF

  return task;
}

void cleanup_after_stcode_compilation(const char *out_dir, const char *lib_name)
{
  G_UNUSED(out_dir);
  G_UNUSED(lib_name);

  constexpr char DEBINFO_EXT[] =
#if _TARGET_PC_WIN
    ".pdb"
#elif _TARGET_PC_LINUX
    ".so.debuginfo"
#else // _TARGET_PC_MACOSX
    ".dSYM"
#endif
    ;

#if !(_CROSS_TARGET_C1 | _CROSS_TARGET_C2)
#if _CROSS_TARGET_SPIRV
  if (shc::config().cppStcodeDeleteDebugInfo && shc::config().usePcToken)
#else
  if (shc::config().cppStcodeDeleteDebugInfo)
#endif
  {
    // @TODO: add more configurations, now it's hardcoded dev anyway
    eastl::string pdbPath = eastl::string{out_dir} + "/" + eastl::string{lib_name} + "-dev" + DEBINFO_EXT;
    dd_erase(pdbPath.c_str());
  }
#endif
}
