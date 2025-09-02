//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <catch2/catch_tostring.hpp>

#include <EASTL/string.h>
#include <EASTL/string_view.h>


// =======================================================
// Provide custom to string formatting for EASTL types
//
// Catch2 manual about custom type formatting:
// https://github.com/catchorg/Catch2/blob/devel/docs/tostring.md
// =======================================================


namespace Catch
{


template <>
struct StringMaker<eastl::string>
{
  static std::string convert(const eastl::string &value) { return {value.data(), value.size()}; }
};

template <>
struct StringMaker<eastl::string_view>
{
  static std::string convert(const eastl::string_view &value) { return {value.data(), value.size()}; }
};


} // namespace Catch
