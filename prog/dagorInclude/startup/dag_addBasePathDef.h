//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_globalSettings.h>
#include <stdio.h>
#if _TARGET_PC_WIN
#include <direct.h>
#elif _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_C3
#include <unistd.h>
#endif
#if _TARGET_XBOX
#include <osApiWrappers/gdk/storage.h>
#endif

#if _TARGET_APPLE
#ifndef DAGOR_MACOSX_CONTENTS
#define DAGOR_MACOSX_CONTENTS "../Resources/game/"
#endif
#endif

#if _TARGET_TVOS
#include <dirent.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4505)
#endif

static void dagor_add_cwd_base_path()
{
#if (_TARGET_PC_WIN | _TARGET_PC_LINUX) && defined(__UNLIMITED_BASE_PATH)
  dd_add_base_path("");

#elif _TARGET_PC_WIN || defined(__CURDIR_BASE_PATH)
  char buf[DAGOR_MAX_PATH];
  if (getcwd(buf, DAGOR_MAX_PATH))
  {
    dd_append_slash_c(buf);
    dd_simplify_fname_c(buf);
    if (dd_add_base_path(buf))
      debug("addpath: %s", buf);
  }
#else

#endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if _TARGET_PC_WIN
#define IS_ABSOLUTE_PATH(p) ((p)[0] && (p)[1] == ':')
#elif _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_PC_ANDROID | _TARGET_C3
#define IS_ABSOLUTE_PATH(p) ((p)[0] && (p)[0] == '/')
#else
#define IS_ABSOLUTE_PATH(p) 1
#endif

#if __UNLIMITED_BASE_PATH
static void dagor_change_root_directory(const char *new_root = NULL)
{
  // Do not change working directory unless explicitly requested.
  // NOTE: This define is set by tools that work outside of their binary's directory.
  if (new_root)
    G_VERIFY(chdir(new_root) == 0);
}
#elif _TARGET_PC_WIN
static void dagor_change_root_directory(const char *new_root = NULL)
{
  if (!new_root)
  {
    wchar_t processPath[512];
    if (size_t moduleLen = GetModuleFileNameW(NULL, processPath, countof(processPath)))
      for (wchar_t *p = processPath + moduleLen; p > processPath; p--)
        if (*p == '\\' || *p == '/')
        {
          *p = 0;
          SetCurrentDirectoryW(processPath);
          break;
        }
#ifdef __DAGOR_OVERRIDE_DEFAULT_ROOT_RELATIVE_PATH
    chdir(__DAGOR_OVERRIDE_DEFAULT_ROOT_RELATIVE_PATH);
#endif
  }
  else
    chdir(new_root);
}
#elif _TARGET_PC_LINUX
static void dagor_change_root_directory(const char *new_root = NULL)
{
  if (!new_root)
  {
    char processPath[DAGOR_MAX_PATH];
    int sz = readlink("/proc/self/exe", processPath, sizeof(processPath) - 1);
    G_ASSERT(sz > 0 && sz < sizeof(processPath));
    processPath[sz] = '\0';
    char *delim = strrchr(processPath, '/');
    if (delim)
      *delim = '\0';
    G_VERIFY(chdir(processPath) == 0);
#ifdef __DAGOR_OVERRIDE_DEFAULT_ROOT_RELATIVE_PATH
    G_VERIFY(chdir(__DAGOR_OVERRIDE_DEFAULT_ROOT_RELATIVE_PATH) == 0);
#endif
  }
  else
    G_VERIFY(chdir(new_root) == 0);
}
#elif _TARGET_APPLE | _TARGET_ANDROID | _TARGET_C3
static void dagor_change_root_directory(const char *new_root = NULL)
{
  if (!new_root)
  {
    char executable_path[DAGOR_MAX_PATH];
    dd_get_fname_location(executable_path, dgs_argv[0]);

    char cwd[DAGOR_MAX_PATH] = {0};
    if (!IS_ABSOLUTE_PATH(executable_path))
      if (getcwd(cwd, sizeof(cwd)))
        dd_append_slash_c(cwd);

    strcat(cwd, executable_path);

#if _TARGET_IOS | _TARGET_TVOS
    if (strstr(cwd, ".app/"))
      strcat(cwd, "game/");
#elif _TARGET_PC_MACOSX
    if (strstr(cwd, ".app/Contents/MacOS/"))
      strcat(cwd, DAGOR_MACOSX_CONTENTS);
#endif

    dd_simplify_fname_c(cwd);
    chdir(cwd);
#ifdef __DAGOR_OVERRIDE_DEFAULT_ROOT_RELATIVE_PATH
    chdir(__DAGOR_OVERRIDE_DEFAULT_ROOT_RELATIVE_PATH);
#endif
  }
  else
    chdir(new_root);
#if _TARGET_TVOS
  char cwd[DAGOR_MAX_PATH] = {0};
  getcwd(cwd, sizeof(cwd));
  debug("dagor_change_root_directory  to \"%s\"", cwd);
#endif
}
#endif

#define NEED_ABSOLUTE_ADDPATH (_TARGET_PC) | (_TARGET_ANDROID)

#if !_TARGET_XBOX
static void dagor_init_base_path()
{
#if NEED_ABSOLUTE_ADDPATH || (defined(__ADD_EXE_BASE_PATH) && _TARGET_PC_WIN)
  char cwd[DAGOR_MAX_PATH];
#endif
#if NEED_ABSOLUTE_ADDPATH
  G_VERIFY(getcwd(cwd, sizeof(cwd)) != NULL);
  dd_append_slash_c(cwd);
#endif

  int it = 1;
  for (const char *ap; (ap = ::dgs_get_argv("addpath", it)) != NULL;)
    if (*ap)
    {
      char buf[DAGOR_MAX_PATH] = {0};
#if NEED_ABSOLUTE_ADDPATH
      if (!IS_ABSOLUTE_PATH(ap))
        strcpy(buf, cwd);
#endif
      strcat(buf, ap);
      dd_append_slash_c(buf);
      dd_simplify_fname_c(buf);
      dd_add_base_path(buf);
    }

  dd_add_base_path("");

#if defined(__ADD_EXE_BASE_PATH) && _TARGET_PC_WIN
  char exe_dir[DAGOR_MAX_PATH];
  dd_get_fname_location(exe_dir, dgs_argv[0]);
  if (IS_ABSOLUTE_PATH(exe_dir))
    cwd[0] = 0;
  strcat(cwd, exe_dir);
  dd_simplify_fname_c(cwd);
  dd_add_base_path(cwd);
#endif
}
#endif

#undef NEED_ABSOLUTE_ADDPATH

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4505)
#endif
// NOTE: Deprecated and left for compatibility reasons only.
static void dagor_add_base_path_default()
{
#if _TARGET_C1 | _TARGET_C2













#elif _TARGET_XBOX
  dd_add_base_path(gdk::get_pls_path(), true);
  dd_add_base_path("G:\\");

#elif _TARGET_ANDROID
  char path[DAGOR_MAX_PATH];
  if (dagor_android_external_path)
  {
    strcpy(path, dagor_android_external_path);
    strcat(path, "/");
    dd_add_base_path(path);
    debug("addpath: %s", path);
  }

#elif (_TARGET_PC | _TARGET_IOS | _TARGET_TVOS) && !_TARGET_PC_WIN
  char exe_dir[DAGOR_MAX_PATH];
  dd_get_fname_location(exe_dir, dgs_argv[0]);
  char cwd[DAGOR_MAX_PATH * 2];
  cwd[0] = 0;
  if (!IS_ABSOLUTE_PATH(exe_dir))
  {
    if (getcwd(cwd, DAGOR_MAX_PATH))
      dd_append_slash_c(cwd);
  }
  strcat(cwd, exe_dir);
  dd_simplify_fname_c(cwd);
#if _TARGET_IOS | _TARGET_TVOS
  if (strstr(cwd, ".app/"))
  {
    strcat(cwd, "game/");
    dd_simplify_fname_c(cwd);
  }
#elif _TARGET_PC_MACOSX
  if (strstr(cwd, ".app/Contents/MacOS/"))
  {
    strcat(cwd, DAGOR_MACOSX_CONTENTS);
    dd_simplify_fname_c(cwd);
  }
#endif
  if (dd_add_base_path(cwd))
    debug("addpath: %s", cwd);
#endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#undef IS_ABSOLUTE_PATH
