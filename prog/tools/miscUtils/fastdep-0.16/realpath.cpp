#ifdef WIN32
#include <direct.h>
#include "os.h"
/*
 * realpath.c -- canonicalize pathname by removing symlinks
 * Copyright (C) 1993 Rick Sladkey <jrs@world.std.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
// #include <unistd.h>
#include <stdio.h>
#include <string.h>
// #include <strings.h>
#include <limits.h> /* for PATH_MAX */
// #include <sys/param.h>			/* for MAXPATHLEN */
#include <errno.h>

#include <sys/stat.h> /* for S_IFLNK */

#ifndef PATH_MAX
#ifdef _POSIX_VERSION
#define PATH_MAX _POSIX_PATH_MAX
#else
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

#define MAX_READLINKS 32

#if defined __STDC__ || defined WIN32
char *realpath(const char *path, char resolved_path[])
#else
char *realpath(path, resolved_path) const char *path;
char resolved_path[];
#endif
{
  strcpy(resolved_path, path);
  simplify_fname_c(resolved_path);
  return resolved_path;

  // until fixed
  char copy_path[PATH_MAX];
  char link_path[PATH_MAX];
  char got_path[PATH_MAX];
  char *new_path = got_path;
  char *max_path;
  int readlinks = 0;
  int n;

  /* Make a copy of the source path since we may need to modify it. */
  if (strlen(path) >= PATH_MAX - 2)
  {
#ifdef WIN32
    errno = ENOENT;
#else
    __set_errno(ENAMETOOLONG);
#endif
    return NULL;
  }
  strcpy(copy_path, path);
  path = copy_path;
  max_path = copy_path + PATH_MAX - 2;
  /* If it's a relative pathname use getwd for starters. */
  if (is_path_relative(path))
  {
    /* Ohoo... */
#define HAVE_GETCWD
#ifdef HAVE_GETCWD
#ifdef WIN32
    _getcwd(new_path, PATH_MAX - 1);
#else
    getcwd(new_path, PATH_MAX - 1);
#endif
#else
    getwd(new_path);
#endif
    new_path += strlen(new_path);
    if (new_path[-1] != cPathSep)
      *new_path++ = cPathSep;
  }
  else
  {
    *new_path++ = cPathSep;
    path++;
  }
  /* Expand each slash-separated pathname component. */
  while (*path != '\0')
  {
    /* Ignore stray "/". */
    if (*path == cPathSep)
    {
      path++;
      continue;
    }
    if (*path == '.')
    {
      /* Ignore ".". */
      if (path[1] == '\0' || path[1] == cPathSep)
      {
        path++;
        continue;
      }
      if (path[1] == '.')
      {
        if (path[2] == '\0' || path[2] == cPathSep)
        {
          path += 2;
          /* Ignore ".." at root. */
          if (new_path == got_path + 1)
            continue;
          /* Handle ".." by backing up. */
          while ((--new_path)[-1] != cPathSep)
            ;
          continue;
        }
      }
    }
    /* Safely copy the next pathname component. */
    while (*path != '\0' && *path != cPathSep)
    {
      if (path > max_path)
      {
#ifdef WIN32
        errno = ENOENT;
#else
        __set_errno(ENAMETOOLONG);
#endif //__set_errno(ENAMETOOLONG);
        return NULL;
      }
      *new_path++ = *path++;
    }
#ifdef S_IFLNK
    /* Protect against infinite loops. */
    if (readlinks++ > MAX_READLINKS)
    {
      __set_errno(ELOOP);
      return NULL;
    }
    /* See if latest pathname component is a symlink. */
    *new_path = '\0';
    n = readlink(got_path, link_path, PATH_MAX - 1);
    if (n < 0)
    {
      /* EINVAL means the file exists but isn't a symlink. */
      if (errno != EINVAL)
      {
        /* Make sure it's null terminated. */
        *new_path = '\0';
        strcpy(resolved_path, got_path);
        return NULL;
      }
    }
    else
    {
      /* Note: readlink doesn't add the null byte. */
      link_path[n] = '\0';
      if (*link_path == cPathSep)
        /* Start over for an absolute symlink. */
        new_path = got_path;
      else
        /* Otherwise back up over this component. */
        while (*(--new_path) != cPathSep)
          ;
      /* Safe sex check. */
      if (strlen(path) + n >= PATH_MAX - 2)
      {
        __set_errno(ENAMETOOLONG);
        return NULL;
      }
      /* Insert symlink contents into path. */
      strcat(link_path, path);
      strcpy(copy_path, link_path);
      path = copy_path;
    }
#endif /* S_IFLNK */
    *new_path++ = cPathSep;
  }
  /* Delete trailing slash but don't whomp a lone slash. */
  if (new_path != got_path + 1 && new_path[-1] == cPathSep)
    new_path--;
  /* Make sure it's null terminated. */
  *new_path = '\0';
  strcpy(resolved_path, got_path);
  return resolved_path;
}

#define PATH_DELIM      '/'
#define PATH_DELIM_BACK '\\'
void simplify_fname_c(char *s)
{
  if (!s)
    return;
  int i, len = strlen(s);
  for (;;)
  {
    bool ok = false;
    // remove leading spaces
    for (i = 0; i < len; ++i)
      if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
        break;
    if (i > 0)
    {
      len -= i;
      memmove(s, s + i, len + 1);
      ok = true;
    }
    // remove trailing spaces
    for (i = len - 1; i >= 0; --i)
      if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
        break;
    if (i < len - 1)
    {
      s[len = i + 1] = 0;
      ok = true;
    }
    // remove quotes
    for (i = 0; i < len; ++i)
      if (s[i] == '"')
        break;
    if (i < len)
    {
      int qs = i;
      for (++i; i < len; ++i)
        if (s[i] == '"')
          break;
      if (i < len)
      {
        memmove(s + qs, s + qs + 1, len - qs);
        --len;
        --i;
        memmove(s + i, s + i + 1, len - i);
        --len;
        ok = true;
      }
    }
    if (!ok)
      break;
  }

  // replace all back slashes with normal ones
  for (i = 1; i < len; ++i)
    if (s[i] == PATH_DELIM_BACK)
      s[i] = PATH_DELIM;

  // remove extra slashes
  for (i = 1; i < len; ++i)
    if (s[i] == PATH_DELIM)
      if (s[i - 1] == PATH_DELIM)
      {
        memmove(s + i, s + i + 1, len - i);
        --len;
        --i;
      }

  // remove extra cur-dirs
  if (len > 0)
  {
    if (s[0] == '.' && s[1] != '.')
    {
      int e = 1;
      if (s[1] == PATH_DELIM)
        ++e;
      len -= e;
      memmove(s, s + e, len + 1);
    }
  }

  for (i = 1; i < len; ++i)
    if (s[i] == '.' && (s[i - 1] == PATH_DELIM || s[i - 1] == ':'))
      if (s[i + 1] == PATH_DELIM || s[i + 1] == 0)
      {
        int e = 1;
        if (s[i + 1] == PATH_DELIM)
          ++e;
        len -= e;
        memmove(s + i, s + i + e, len + 1 - i);
        --i;
      }

  // remove extra up-dirs
  for (i = 2; i < len; ++i)
    if (s[i - 2] == PATH_DELIM && s[i - 1] == '.' && s[i] == '.')
      if (s[i + 1] == PATH_DELIM || s[i + 1] == 0)
      {
        int j;
        for (j = i - 3; j >= 0; --j)
          if (s[j] == PATH_DELIM || s[j] == ':')
            break;

        if (++j < i - 2)
        {
          int e = i + 1 - j;

          if (e != 5 || s[j] != '.' || s[j + 1] != '.')
          {
            if (s[i + 1] == PATH_DELIM)
              ++e;

            len -= e;
            memmove(s + j, s + j + e, len + 1 - j);
            i = j;
          }
        }
      }
}

char *get_fname_location(char *buf, const char *filename)
{
  strcpy(buf, filename);
  simplify_fname_c(buf);

  char *p = strrchr(buf, PATH_DELIM);
  if (!p)
    p = strchr(buf, ':');

  if (p)
    p++;
  else
  {
    buf[0] = '.';
    p = buf + 1;
  }

  *p = '\0';
  return buf;
}
#undef PATH_DELIM
#undef PATH_DELIM_BACK

#endif
