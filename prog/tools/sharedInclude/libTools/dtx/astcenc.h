#pragma once

#if _TARGET_PC_WIN
#include <supp/_platform.h>
#undef ERROR // after #include <supp/_platform.h>
#include <process.h>
#include <stdio.h>
#define ASTCENC_DECLARE_STATIC_DATA() char ASTCEncoderHelperContext::astcencExePath[1024] = ""

#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#define strcat_s(D, N, S) strcat(D, S)
extern char **environ;
#define ASTCENC_DECLARE_STATIC_DATA()                       \
  char ASTCEncoderHelperContext::astcencExePath[1024] = ""; \
  char **environ

#endif

#if _TARGET_PC_MACOSX
#include <libproc.h>
#endif

#include <generic/dag_tab.h>
#include <image/dag_texPixel.h>
#include <image/dag_tga.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_cpuFeatures.h>
#include <debug/dag_debug.h>
#include <string.h>


struct ASTCEncoderHelperContext
{
  static constexpr unsigned ASTC_FILE_HDRSZ = 16;
  static char astcencExePath[1024];

#if !_TARGET_PC_WIN
  static constexpr unsigned L_tmpnam_s = 512;
  int tmpNameFd = -1;
  char tmpNameTemplate[252] = "";
#endif
  char tmpNameTGA[L_tmpnam_s] = "", tmpNameASTC[L_tmpnam_s] = "";

  ~ASTCEncoderHelperContext()
  {
    if (tmpNameTGA[0])
      unlink(tmpNameTGA);
    if (tmpNameASTC[0])
      unlink(tmpNameASTC);
#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
    if (tmpNameFd >= 0)
    {
      close(tmpNameFd);
      unlink(tmpNameTemplate);
    }
#endif
  }

  bool prepareTmpFilenames(const char *a_name)
  {
#if _TARGET_PC_WIN
    if (tmpnam_s(tmpNameTGA, L_tmpnam_s) != 0)
    {
      logerr("cannot create temp filename");
      return false;
    }
    strcat_s(tmpNameTGA, L_tmpnam_s, "_");
    strcat_s(tmpNameTGA, L_tmpnam_s, a_name);
#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX
    snprintf(tmpNameTemplate, sizeof(tmpNameTemplate), "/tmp/%s_XXXXXX", a_name);
    {
      tmpNameFd = mkstemp(tmpNameTemplate);
      if (tmpNameFd < 0)
      {
        logerr("cannot create temp filename");
        tmpNameTemplate[0] = '\0';
        return false;
      }
      strcpy(tmpNameTGA, tmpNameTemplate);
    }
#endif
    strcpy(tmpNameASTC, tmpNameTGA);
    strcat_s(tmpNameTGA, L_tmpnam_s, ".tga");
    strcat_s(tmpNameASTC, L_tmpnam_s, ".astc");
    return true;
  }

  bool writeTga(TexPixel32 *pix, int w, int h)
  {
    if (save_tga32(tmpNameTGA, pix, w, h, w * 4))
      return true;
    logerr("failed to write '%s' from %p %dx%d", tmpNameTGA, pix, w, h);
    return false;
  }

  bool buildOneSurface(Tab<char> &pdata, const char *astc_block, int limit_max_jobs = 0)
  {
    Tab<const char *> arguments;
    String jarg;
    arguments.push_back(astcencExePath);
    arguments.push_back("-cl");
    arguments.push_back(tmpNameTGA);
    arguments.push_back(tmpNameASTC);
    arguments.push_back(astc_block);
    arguments.push_back("-medium");
    arguments.push_back("-silent");
    if (limit_max_jobs > 0)
    {
      jarg.printf(0, "%d", limit_max_jobs);
      arguments.push_back("-j");
      arguments.push_back(jarg.str());
    }
    arguments.push_back(nullptr);
    if (!ASTCEncoderHelperContext::runAstcEnc(arguments))
    {
      unlink(tmpNameTGA);
      return false;
    }
    file_ptr_t fp = df_open(tmpNameASTC, DF_READ);
    if (!fp)
    {
      logerr("cannot read '%s', build failed?", tmpNameASTC);
      unlink(tmpNameTGA);
      return false;
    }
    unsigned packed_pos = pdata.size();
    unsigned astc_size = df_length(fp) - ASTC_FILE_HDRSZ;
    pdata.resize(packed_pos + astc_size);
    df_seek_to(fp, ASTC_FILE_HDRSZ);
    df_read(fp, &pdata[packed_pos], astc_size);
    df_close(fp);
    return true;
  }

  bool decodeOneSurface(const char *outNameTGA, //
    dag::ConstSpan<char> pdata, unsigned w, unsigned h, unsigned d, unsigned blk_w, unsigned blk_h, unsigned blk_d)
  {
    Tab<const char *> arguments;
    arguments.push_back(astcencExePath);
    arguments.push_back("-dl");
    arguments.push_back(tmpNameASTC);
    arguments.push_back(outNameTGA);
    arguments.push_back("-silent");
    arguments.push_back(nullptr);

    file_ptr_t fp = df_open(tmpNameASTC, DF_WRITE);
    df_write(fp, "\x13\xAB\xA1\x5C", 4);
    df_write(fp, &blk_w, 1);
    df_write(fp, &blk_h, 1);
    df_write(fp, &blk_d, 1);
    df_write(fp, &w, 3);
    df_write(fp, &h, 3);
    df_write(fp, &d, 3);
    df_write(fp, pdata.data(), pdata.size());
    df_close(fp);

    if (!ASTCEncoderHelperContext::runAstcEnc(arguments))
    {
      unlink(tmpNameASTC);
      return false;
    }
    return dd_file_exists(outNameTGA);
  }

  static bool runAstcEnc(Tab<const char *> &arguments)
  {
#if _TARGET_PC_WIN
    int ret = _spawnv(_P_WAIT, astcencExePath, (char *const *)arguments.data());
#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX
    pid_t child_pid;
    int ret = posix_spawn(&child_pid, astcencExePath, nullptr, nullptr, (char *const *)arguments.data(), environ), status = 0;
    while (ret == 0)
    {
      ret = waitpid(child_pid, &status, 0);
      if (WIFEXITED(status))
      {
        ret = WEXITSTATUS(status);
        break;
      }
      else if (ret >= 0)
        ret = 0;
    }
#endif
    if (ret != 0)
    {
      String cmdline;
      for (unsigned i = 0; i < arguments.size(); i++)
      {
        cmdline += " ";
        cmdline += arguments[i];
      }
      logerr("failed to run %s %s, errno=%d", astcencExePath, cmdline, ret);
      return false;
    }
    return true;
  }

  static void setupAstcEncExePathname()
  {
    if (astcencExePath[0])
      return;
#if _TARGET_PC_WIN
    GetModuleFileNameA(NULL, astcencExePath, sizeof(astcencExePath));
#elif _TARGET_PC_MACOSX
    if (proc_pidpath(getpid(), astcencExePath, sizeof(astcencExePath)) < 0)
      astcencExePath[0] = '\0';
#else
    if (readlink("/proc/self/exe", astcencExePath, sizeof(astcencExePath)) < 0)
      astcencExePath[0] = '\0';
#endif
    dd_simplify_fname_c(astcencExePath);
    if (char *p = strrchr(astcencExePath, '/'))
      *p = '\0';

#if _TARGET_PC_WIN
    strcat(astcencExePath, "/../bin64/astcenc");
#elif _TARGET_PC_LINUX
    strcat(astcencExePath, "/../bin-linux64/astcenc");
#elif _TARGET_PC_MACOSX
    strcat(astcencExePath, "/../bin-macosx/astcenc");
#endif
#if _TARGET_PC_WIN | _TARGET_PC_LINUX
    if (cpu_feature_avx2_checked)
      strcat(astcencExePath, "-avx2");
    else if (cpu_feature_sse41_checked && cpu_feature_popcnt_checked)
      strcat(astcencExePath, "-sse4.1");
    else
#if __e2k__
      strcat(astcencExePath, "-native");
#else
      strcat(astcencExePath, "-sse2");
#endif
#endif
#if _TARGET_PC_WIN
    strcat(astcencExePath, ".exe");
#endif

    dd_simplify_fname_c(astcencExePath);
    // debug("using %s", astcencExePath);
  }
};
