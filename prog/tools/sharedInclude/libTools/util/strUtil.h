//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>


inline bool is_empty_string(const char *str) { return !(str && *str != '\0'); }


//--- filenames & paths functions
// detects full path like D:\projects
bool is_full_path(const char *path);

// splits path to base path and filename
void split_path(const char *path, String &location, String &filename);

// makes path "path" absolute
[[nodiscard]] String make_path_absolute(const char *path);

// makes path "path" relative to "base" path
// returns relative "path" or full "path" if unable to make "path" relative
String make_path_relative(const char *path, const char *base);

// makes full path from "dir" and "fname"
// if "fname" is a full path, function returns "fname" only
String make_full_path(const char *dir, const char *fname);

// tries to smart find file with path "path" in catalog "base"
// if "path" is "C:\foo_dir\file.foo" and "base" is "D:\base"
// and full path to necessary file is "D:\base\foo_dir\file.foo",
//"new_path" will set to "D:\base\foo_dir\file.foo"
bool find_in_base(const char *path, const char *base, String &new_path);

// such as previous "find_in_base", but returns "path" if file with path "path"
// exists, and "def_path" if unable to find file
String find_in_base_smart(const char *path, const char *base, const char *def_path);

// trims full path to location (folder with trailing slash)
void location_from_path(String &full_path);

// returns file extension (without dot) or NULL
const char *get_file_ext(const char *path);

// returns file name from path
const char *get_file_name(const char *path);

// returns file name w/o extension
String get_file_name_wo_ext(const char *path);

// append slash, if there is no slash
void append_slash(String &);

// adds "./" before path if it is no one slash found there
// in other case function returns "path"
String add_dot_slash(const char *path);

// makes "path" 'good' :) simplifies and make path local (if needed)
String make_good_path(const char *path);

// converts path like "foo.ltinput.ltinput.dat" to "foo.ltinput.dat"
String unify_path_from_win32_dlg(const char *path, const char *def_ext);

// converts slashes ('/') in MS-style slashes ('\')
inline char *make_ms_slashes(char *path)
{
  for (char *c = path; *c; ++c)
    if (*c == '/')
      *c = '\\';
  return path;
}

// converts MS-style slashes ('\') in true slashes ('/')
inline char *make_slashes(char *path)
{
  for (char *c = path; *c; ++c)
    if (*c == '\\')
      *c = '/';
  return path;
}


// replaces end zeros in strings like "12.345000" NULL characters ("12.345")
// returns modified string (argument "s")
const char *remove_end_zeros(char *s);


// translates bytes to Kb
String bytes_to_kb(uint64_t bytes);

// translates bytes to Mb
String bytes_to_mb(uint64_t bytes);


// compares the end of str with trail_str
bool trail_strcmp(const char *str, const char *trail_str);

// compares the end of str with trail_str (case ignored)
bool trail_stricmp(const char *str, const char *trail_str);

// compares the end of target with trail_str (case ignored) and removes that trailing str
// in target if it coinsides
void remove_trailing_string(String &target, const char *trail_str);

// remove spaces from left and right side of string (resizing string)
void trim(String &str);

// fills all ending spaces with '\0' and returns pointer to first non-space char
const char *trim(char *str);


//--- some functions... may be, useless
// Append number to name or increment existing.
void make_next_numbered_name(String &name, int num_digits = 3);

// Converts name so that it contains only A-Z, a-z, 0-9 or _.
void make_ident_like_name(String &name);

// Append suffix to target, unless it already has that suffix (case ignored).
void ensure_trailing_string(String &target, const char *suffix);

// replaces '/', '\' and ':' to convert pathname to valid filename
void cvt_pathname_to_valid_filename(String &inout_name);

// generate words list by string. Example: in: "p1 p2", out: "p1", "p2"
void string_to_words(const String &str, Tab<String> &list);

// Returns if text contains a wildcard character ('*' or '?').
bool str_has_wildcard_character(const char *text);

// Returns true if text matches the specified wildcard pattern.
//
// * matches zero or more characters
// ? matches exactly one character
//
// For example:
// test* is matched by test and test_something but not Atest
// test? is matched by test1 but not test or test11
bool str_matches_wildcard_pattern(const char *text, const char *pattern);
