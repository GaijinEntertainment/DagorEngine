// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if defined(WIN32)
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <iostream>
#include <fstream>

#include "fileCache.h"
#include "fileStructure.h"
#include "checkVersion.h"

#define CWDBUFFERLENGTH 1024

static void PrintVersion()
{
  static bool shown = false;
  if (shown)
    return;
  std::cout << "Fast C/C++ dependency generator (fastdep-g), Version " << FASTDEP_VERSION_MAJOR << "." << FASTDEP_VERSION_MINOR
            << FASTDEP_VERSION_PATCHLEV << "    (GPL license)" << std::endl
            << "Copyright (C) 2001, 2002 Bart Vanhauwaert;   Modified by Nic Tiger, 2006-2008" << std::endl;
  shown = true;
}

enum
{
  COMPILER__NONE = 0,
  COMPILER_VC71,
  COMPILER_VC80,
  COMPILER_VCXENON,
};

static void (*process_compiler_option)(FileCache &Cache, const char *opt, bool dbg) = NULL;
static void (*finish_process_compiler_option)(FileCache &Cache, bool dbg) = NULL;

static void set_compiler_options_vc71(FileCache &Cache, bool dbg);
static void process_compiler_option_vc71(FileCache &Cache, const char *opt, bool dbg);
static void finish_process_compiler_option_vc71(FileCache &Cache, bool dbg);

static void set_compiler_options_vc80(FileCache &Cache, bool dbg);
static void process_compiler_option_vc80(FileCache &Cache, const char *opt, bool dbg);
static void finish_process_compiler_option_vc80(FileCache &Cache, bool dbg);

static void set_compiler_options_vcxenon(FileCache &Cache, bool dbg);
static void process_compiler_option_vcxenon(FileCache &Cache, const char *opt, bool dbg);
static void finish_process_compiler_option_vcxenon(FileCache &Cache, bool dbg);

static void set_compiler_type(FileCache &Cache, int cl_type, bool dbg)
{
  switch (cl_type)
  {
    case COMPILER_VC71:
      set_compiler_options_vc71(Cache, dbg);
      process_compiler_option = &process_compiler_option_vc71;
      finish_process_compiler_option = &finish_process_compiler_option_vc71;
      break;
    case COMPILER_VC80:
      set_compiler_options_vc80(Cache, dbg);
      process_compiler_option = &process_compiler_option_vc80;
      finish_process_compiler_option = &finish_process_compiler_option_vc80;
      break;
    case COMPILER_VCXENON:
      set_compiler_options_vcxenon(Cache, dbg);
      process_compiler_option = &process_compiler_option_vcxenon;
      finish_process_compiler_option = &finish_process_compiler_option_vcxenon;
      break;
  }
}


/*
 * Generate dependencies for all files.
 */
int main(int argc, char **argv)
{
  char cwd_buffer[CWDBUFFERLENGTH + 1];
  cwd_buffer[0] = 0;
#if defined(WIN32)
  _getcwd(cwd_buffer, CWDBUFFERLENGTH);
#else
  getcwd(cwd_buffer, CWDBUFFERLENGTH);
#endif
  FileCache Cache(cwd_buffer);
  std::string RemakeDepTarget;
  std::vector<std::string> ExtraRemakeDeps;
  std::string OutputFilename;
  bool doDebug = false, exclude_self = false;
  int optind;

  for (optind = 1; optind < argc; optind++)
    if (argv[optind][0] == '/' || argv[optind][0] == '-')
    {
      switch (argv[optind][1])
      {
        case 'D': Cache.addPreDefine(&argv[optind][2]); break;
        case 'I': Cache.addIncludeDir(cwd_buffer, &argv[optind][2]); break;
        case 'i': // -imsvc as alias for -I
          if (strncmp(&argv[optind][2], "msvc", 4) == 0)
          {
            const char *msvcDirPath = (argv[optind][6] == '\0') ? argv[++optind] : &argv[optind][6];
            Cache.addIncludeDir(cwd_buffer, msvcDirPath);
          }
          break;
        case '-':
          switch (argv[optind][0] == '-' ? argv[optind][2] : 0)
          {
            case 'o': OutputFilename = &argv[optind][3]; break;
            case 'e': Cache.SetObjectsExt(&argv[optind][3]); break;
            case 'v': PrintVersion(); return 1;
            case 'q': Cache.SetQuietModeOn(); break;
            case 'd':
              doDebug = true;
              Cache.inDebugMode();
              break;
            case 'x': Cache.addExcludeDir(cwd_buffer, &argv[optind][3]); break;
            case 'c':
              if (stricmp(&argv[optind][3], "vc71") == 0)
                set_compiler_type(Cache, COMPILER_VC71, doDebug);
              else if (stricmp(&argv[optind][3], "vc80") == 0)
                set_compiler_type(Cache, COMPILER_VC80, doDebug);
              else if (stricmp(&argv[optind][3], "vcxenon") == 0)
                set_compiler_type(Cache, COMPILER_VCXENON, doDebug);
              else
              {
                PrintVersion();
                std::cout << "fastdep-g: Command line warning: unknown cltype: <" << &argv[optind][3] << ">" << std::endl;
              }
              break;
            case 'n':
              if (strcmp(argv[optind], "--noself") == 0)
                exclude_self = true;
              else
              {
                PrintVersion();
                std::cout << "fastdep-g: Command line warning: unknown cltype: <" << &argv[optind][3] << ">" << std::endl;
              }
              break;
            case 'h':
              PrintVersion();
              std::cout << "\nfastdep [-<any>] [/<any>] [--opt] filename"
                           "\n  -<any> - any option (except -I and -D), ignored"
                           "\n           (to provide max cmdline compatibility)"
                           "\n\n  -D<macro>[=value] - define macro"
                           "\n  -I<path> or -imsvc <path> - add path to include path list"
                           "\n\n  --o<fname> - set output filename (stdout by default)"
                           "\n  --e<ext>   - set object file extension (.obj by default)"
                           "\n  --c<cltype>- set compiler type to predefine macros & resolve cmdline options"
                           "\n               valid types: VC71, VC80, VCXENON"
                           "\n               (e.g., --cVC71, should go first on options to parse CL-cmdline)"
                           "\n  --x<path>  - exclude headers found inside this path (and deeper, recursive)"
                           "\n  --v - show version"
                           "\n  --noself - exclude input file from dep list"
                           "\n  --q - set quiet mode"
                           "\n  --d - turn on internal debugging"
                           "\n  --h - this help";

              return 1;
            default:
              PrintVersion();
              std::cout << "fastdep-g: Command line warning: ignoring unknown option: " << argv[optind] << std::endl;
          }
          break;

        default:
          if (process_compiler_option)
            process_compiler_option(Cache, argv[optind], doDebug);
          break;
      }
    }
    else
      break;

#if !defined(WIN32)
  Cache.addExcludeDir("", "/usr/include");
#endif
  if (optind == argc)
  {
    PrintVersion();
    std::cout << "\nfastdep-g: Command line error: no files specified; for help type \"fastdep-g --h\"" << std::endl;
    return 0;
  }
  if (finish_process_compiler_option)
    finish_process_compiler_option(Cache, doDebug);

  std::ostream *theStream = &std::cout;
  if (OutputFilename != "")
  {
    theStream = new std::ofstream(OutputFilename.c_str());
    if (RemakeDepTarget == "")
      RemakeDepTarget = OutputFilename;
  }
  for (int i = optind; i < argc; ++i)
  {
    if (process_compiler_option)
      Cache.addPreDefine(std::string("__FILE__=") + argv[i]);

    Cache.generate(*theStream, cwd_buffer, argv[i]);
    if (RemakeDepTarget == "")
      RemakeDepTarget = argv[i];

    Cache.printAllDependenciesLine(theStream, RemakeDepTarget, exclude_self ? 1 : 0);
    for (unsigned int i = 0; i < ExtraRemakeDeps.size(); ++i)
      *theStream << RemakeDepTarget << ": " << ExtraRemakeDeps[i] << std::endl;
    break;
  }
  if (OutputFilename != "")
    delete theStream;

  return 0;
}

static bool _stdc = false, _cpp = false;
static const char *_target_cpu = "_M_IX86=600";

static void common_c_defined(FileCache &Cache)
{
  Cache.addPreDefine("__DATE__=\"?DATE?\"");
  Cache.addPreDefine("__TIME__=\"?TIME?\"");
  Cache.addPreDefine("__TIMESTAMP__=\"?STMP?\"");
  Cache.addPreDefine("__LINE__=0");
  Cache.addPreDefine("__FUNCDNAME__=\"?FDN?\"");
  Cache.addPreDefine("__FUNCSIG__=\"?FS?\"");
  Cache.addPreDefine("__FUNCTION__=\"?F?\"");
}

// VC 7.1 options processing
static void set_compiler_options_vc71(FileCache &Cache, bool dbg)
{
  common_c_defined(Cache);
  Cache.addPreDefine("_MSC_VER=1310");
  Cache.addPreDefine("_MSC_FULL_VER=13103077");
  Cache.addPreDefine("_WIN32=1");
}
static void process_compiler_option_vc71(FileCache &Cache, const char *opt, bool dbg)
{
  if (strcmp(&opt[1], "J") == 0)
    Cache.addPreDefine("_CHAR_UNSIGNED=1");
  else if (strcmp(&opt[1], "GR") == 0)
    Cache.addPreDefine("_CPPRTTI=1");
  else if (strcmp(&opt[1], "GX") == 0 || strncmp(&opt[1], "EH", 2) == 0)
    Cache.addPreDefine("_CPPUNWIND=1");
  else if (strcmp(&opt[1], "LDd") == 0 || strcmp(&opt[1], "MDd") == 0 || strcmp(&opt[1], "MLd") == 0 || strcmp(&opt[1], "MTd") == 0)
    Cache.addPreDefine("_DEBUG=1");
  else if (strcmp(&opt[1], "Za") == 0)
    _stdc = true;
  else if (strcmp(&opt[1], "Ze") == 0)
    _stdc = false;
  else if (strcmp(&opt[1], "TP") == 0)
    _cpp = true;
  else if (strcmp(&opt[1], "TC") == 0)
    _cpp = false;
  else if (stricmp(&opt[1], "RTC") == 0)
    Cache.addPreDefine("__MSVC_RUNTIME_CHECKS=1");
  else if (stricmp(&opt[1], "Zc:wchar_t") == 0)
    Cache.addPreDefine("_NATIVE_WCHAR_T_DEFINED=1");
  else if (stricmp(&opt[1], "G3") == 0)
    _target_cpu = "_M_IX86=300";
  else if (stricmp(&opt[1], "G4") == 0)
    _target_cpu = "_M_IX86=400";
  else if (stricmp(&opt[1], "G5") == 0)
    _target_cpu = "_M_IX86=500";
  else if (stricmp(&opt[1], "G6") == 0 || stricmp(&opt[1], "GB") == 0)
    _target_cpu = "_M_IX86=600";
  else if (strnicmp(&opt[1], "FI", 2) == 0)
    Cache.addAutoInc(&opt[3]);


  if (strcmp(&opt[1], "MD") == 0 || strcmp(&opt[1], "MDd") == 0)
    Cache.addPreDefine("_DLL=1");

  if (strcmp(&opt[1], "MD") == 0 || strcmp(&opt[1], "MDd") == 0 || strcmp(&opt[1], "MT") == 0 || strcmp(&opt[1], "MTd") == 0)
    Cache.addPreDefine("_MT=1");
}
static void finish_process_compiler_option_vc71(FileCache &Cache, bool dbg)
{
  if (!_stdc)
    Cache.addPreDefine("_MSC_EXTENSIONS=1");
  else if (_stdc && !_cpp)
    Cache.addPreDefine("__STDC__=1");
  if (_cpp)
    Cache.addPreDefine("__cplusplus=1");

  Cache.addPreDefine(_target_cpu);
}

// VC 8.0 options processing
static void set_compiler_options_vc80(FileCache &Cache, bool dbg)
{
  common_c_defined(Cache);
  Cache.addPreDefine("_MSC_VER=1400");
  Cache.addPreDefine("_MSC_FULL_VER=140050727");
  Cache.addPreDefine("_WIN32=1");
}
static void process_compiler_option_vc80(FileCache &Cache, const char *opt, bool dbg)
{
  process_compiler_option_vc71(Cache, opt, dbg);
}
static void finish_process_compiler_option_vc80(FileCache &Cache, bool dbg) { finish_process_compiler_option_vc71(Cache, dbg); }


// VC for xbox360 (XENON) options processing
static void set_compiler_options_vcxenon(FileCache &Cache, bool dbg)
{
  common_c_defined(Cache);
  Cache.addPreDefine("_MSC_VER=1400");
  Cache.addPreDefine("_MSC_FULL_VER=14002712");
  Cache.addPreDefine("_WIN32=1");
  Cache.addPreDefine("_M_PPC=5401");
  Cache.addPreDefine("_M_PPCBE=1");
}
static void process_compiler_option_vcxenon(FileCache &Cache, const char *opt, bool dbg)
{
  process_compiler_option_vc71(Cache, opt, dbg);
}
static void finish_process_compiler_option_vcxenon(FileCache &Cache, bool dbg)
{
  if (!_stdc)
    Cache.addPreDefine("_MSC_EXTENSIONS=1");
  else if (_stdc && !_cpp)
    Cache.addPreDefine("__STDC__=1");
  if (_cpp)
    Cache.addPreDefine("__cplusplus=1");
}
