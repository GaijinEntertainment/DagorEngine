// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>

#include <contentUpdater/fsUtils.h>

using updater::fs::normalize_path;
using updater::fs::simplify_path;

TEST_CASE("simplify_path converts delimiters and drops dot dirs", "[pathUtils]") { CHECK(simplify_path("a\\b//c/./d") == "a/b/c/d"); }

TEST_CASE("normalize_path ends with one delimiter", "[pathUtils]")
{
  CHECK(normalize_path("a/b") == "a/b/");
  CHECK(normalize_path("a/b/") == "a/b/");
}

TEST_CASE("normalize_path keeps a path longer than DAGOR_MAX_PATH", "[pathUtils]")
{
  eastl::string longPath;
  for (int i = 0; i < 64; i++)
    longPath += "directory_name/";
  CHECK(normalize_path(longPath.c_str()) == longPath);
}
