// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#ifdef WIN32
static const char cPathSep = '\\';
static const char *sPathSep = "\\";

static inline bool is_path_relative(const char *path)
{
  if (*path == '\\' || *path == '/')
    return false;
  if (*path && path[1] == ':')
    return false;
  return true;
}

#else
static const char cPathSep = '/';
static const char *sPathSep = "/";

static inline bool is_path_relative(const char *path) { return *path != '/'; }

#endif

void simplify_fname_c(char *s);
char *get_fname_location(char *buf, const char *filename);
