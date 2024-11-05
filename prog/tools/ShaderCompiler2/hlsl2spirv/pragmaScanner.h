// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string_view>
#include <utility>
#include "../fast_isalnum.h"

class PragmaDefiniton
{
  std::string_view def;

public:
  PragmaDefiniton() = default;
  ~PragmaDefiniton() = default;

  PragmaDefiniton(const PragmaDefiniton &) = default;
  PragmaDefiniton &operator=(const PragmaDefiniton &) = default;

  PragmaDefiniton(std::string_view sv) : def{sv} {}
  explicit operator bool() const { return !def.empty(); }

  class TokenList
  {
    std::string_view rest;
    std::string_view token;

    void firstToken()
    {
      while (!rest.empty())
      {
        if (fast_isspace(rest.front()))
        {
          rest.remove_prefix(1);
          continue;
        }
        break;
      }
      nextToken();
    }
    void nextToken()
    {
      if (!rest.empty())
      {
        size_t len = 0;
        for (; len < rest.length(); ++len)
        {
          if (fast_isspace(rest[len]))
            break;
        }
        token = rest.substr(0, len);
        rest = rest.substr(len);
      }
      while (!rest.empty())
      {
        if (fast_isspace(rest.front()))
        {
          rest.remove_prefix(1);
          continue;
        }
        break;
      }
    }

  public:
    TokenList() = default;
    ~TokenList() = default;

    TokenList(const TokenList &) = default;
    TokenList &operator=(const TokenList &) = default;

    TokenList(std::string_view view) : rest{view} { nextToken(); }

    std::string_view operator*() const { return token; }
    explicit operator bool() const { return !rest.empty(); }

    TokenList &operator++()
    {
      nextToken();
      return *this;
    }
  };

  TokenList tokens() const { return {def}; }
};

class PragmaScanner
{
  const char *source = "";
  const char *at = "";

  void skipSpaces()
  {
    for (; *at; ++at)
      if (0 == fast_isspace(*at))
        break;
  }

  bool nextPPStart()
  {
    while (*at)
      if ('#' == *at++)
        return true;
    return false;
  }
  bool nextIsPragma()
  {
    static const char pragma_kw[] = "pragma";
    skipSpaces();
    if (0 == strncmp(at, pragma_kw, sizeof(pragma_kw) - 1))
    {
      at += sizeof(pragma_kw) - 1;
      skipSpaces();
      return true;
    }
    return false;
  }
  std::string_view getPragmaDef()
  {
    auto from = at;
    while (*at)
      if (*at++ == '\n')
        break;
    auto to = at;
    return std::string_view(from, to - from);
  }

public:
  PragmaScanner(const char *s) : source{s}, at{s} {}
  PragmaDefiniton operator()()
  {
    while (nextPPStart())
      if (nextIsPragma())
        return getPragmaDef();
    return {};
  }
};