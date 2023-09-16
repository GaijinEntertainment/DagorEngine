#pragma once

// test example:
//   from "a" import b.c.*, e, f.g.h
//   #strict-compile
//   import a as b

// usage:
//
//   void error_cb(void * /*user_pointer*/, const char * message, int line, int column) {....}
//   ...
//   ImportParser importParser(error_cb, nullptr);
//   const char * txt = <module code>;
//   int line = 1;
//   int col = 1;
//   vector<ImportParser::ModuleImport> modules;
//   vector<string> directives;
//   vector<pair<const char *, const char *>> keepRanges;   // .first - inclusive, .second - not inclusive
//   if (importParser.parse(&txt, line, col, modules, &directives, &keepRanges))
//     ...

#ifndef MODULE_PARSE_STL
#  include <string>
#  include <vector>
#  define MODULE_PARSE_STL std
#endif

#include <string.h>

namespace sqimportparser
{


struct ModuleImport
{
  struct Slot
  {
    MODULE_PARSE_STL::string importAsIdentifier;
    MODULE_PARSE_STL::vector<MODULE_PARSE_STL::string> path;
    int line;
    int column;
  };

  MODULE_PARSE_STL::string moduleName;
  MODULE_PARSE_STL::string moduleIdentifier;
  MODULE_PARSE_STL::vector<Slot> slots;
  int line;
  int column;
};

class ImportParser
{
public:
  typedef void (*ImportErrorCb)(void * user_pointer, const char * message, int line, int column);

private:

  const char * p = nullptr;
  const char * curLinePtr = nullptr;
  int line = -1;
  bool errors = false;
  MODULE_PARSE_STL::vector<MODULE_PARSE_STL::string> * directives = nullptr;
  MODULE_PARSE_STL::vector<MODULE_PARSE_STL::pair<const char *, const char *>> * keepRanges = nullptr;

  static bool isUtf8Bom(const char * ptr)
  {
    return (uint8_t)ptr[0] == 0xEF && (uint8_t)ptr[1] == 0xBB && (uint8_t)ptr[2] == 0xBF;
  }

  void error(const char * message)
  {
    if (errorCb && !errors)
      errorCb(cbUserPointer, message, line, int(p - curLinePtr + 1));
    errors = true;
  }

  bool accept(const char * word)
  {
    const char * cur = p;
    bool alnum = isalnum(*word);
    while (*word)
    {
      if (*cur != *word)
        return false;
      cur++;
      word++;
    }

    if (alnum && (isalnum(*cur) || *cur == '_'))
      return false;

    p = cur;
    skipSpaceAndComments();
    return true;
  }

  void expect(const char * word)
  {
    if (!accept(word))
      error((MODULE_PARSE_STL::string("expected '") + word + "'").c_str());
  }

  bool nextChar() // return true - eol
  {
    if (*p == 0)
      return false;

    if (*p == '\r' && p[1] == '\n')
      p++;

    if (*p == '\n')
    {
      p++;
      line++;
      curLinePtr = p;
      return true;
    }
    else
    {
      p++;
      return false;
    }
  }

  void skipSpaceAndComments()
  {
    bool repeat = true;
    bool insideComment = false;
    const char * commentFrom = nullptr;
    while (repeat)
    {
      repeat = false;
      while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || insideComment || (*p == '/' && p[1] == '*')))
      {
        if (!insideComment && *p == '/' && p[1] == '*')
        {
          insideComment = true;
          commentFrom = p;
          p++;
        }
        else if (insideComment && *p == '*' && p[1] == '/')
        {
          insideComment = false;
          p++;

          if (keepRanges)
            keepRanges->push_back(MODULE_PARSE_STL::make_pair(commentFrom, p + 1));
        }
        nextChar();
      }

      if (insideComment && *p == 0)
        error("unexpected end of file inside comment");

      if (!insideComment && *p == '/' && p[1] == '/')
      {
        const char * from = p;
        repeat = true;
        while (*p && *p != '\r' && *p != '\n')
          if (nextChar())
            break;

        if (keepRanges)
          keepRanges->push_back(MODULE_PARSE_STL::make_pair(from, p + 1));
      }
    }
  }


  MODULE_PARSE_STL::string parseIdentifier(bool allow_quotes = true)
  {
    if (allow_quotes && *p == '"')
    {
      p++;
      const char * from = p;
      while (*p && *p != '"')
      {
        if (*p == '\n')
        {
          error("end of line inside string");
          return MODULE_PARSE_STL::string();
        }

        p++;
      }

      if (*p == 0)
      {
        error("unexpected end of file inside string");
        return MODULE_PARSE_STL::string();
      }

      const char * to = p;
      if (*p)
        p++;

      skipSpaceAndComments();
      return MODULE_PARSE_STL::string(from, to);
    }
    else if ((isalnum(*p) && !isdigit(*p)) || *p == '_')
    {
      const char * from = p;
      while (isalnum(*p) || *p == '_')
        p++;

      const char * to = p;
      skipSpaceAndComments();
      return MODULE_PARSE_STL::string(from, to);
    }
    else
    {
      error("expected identifier");
      return MODULE_PARSE_STL::string();
    }
  }

  void parseDirective()
  {
    const char * from = p;
    p++;
    while (isalnum(*p) || *p == '-' || *p == '_' || *p == ':')
      p++;

    if (directives)
      directives->push_back(MODULE_PARSE_STL::string(from, p + 1));

    if (keepRanges)
      keepRanges->push_back(MODULE_PARSE_STL::make_pair(from, p + 1));

    skipSpaceAndComments();
  }


  MODULE_PARSE_STL::string extractFileName(const MODULE_PARSE_STL::string & s)
  {
    bool isScriptFile = s.length()>4 && strncmp(s.c_str()+s.length()-4, ".nut", 4)==0;
    if (isScriptFile)
    {
      const char * c = s.c_str();
      const char * slash1 = strrchr(c, '\\');
      const char * slash2 = strrchr(c, '/');
      const char * slash = (slash1 > slash2) ? slash1 : slash2;
      if (slash)
        slash++;
      else
        slash = c;

      const char * dot = strchr(slash, '.');
      if (dot)
        return MODULE_PARSE_STL::string(slash, dot);
      else
        return MODULE_PARSE_STL::string(slash);
    }
    else
    {
      MODULE_PARSE_STL::string result(s);
      for (int i = 0, n = int(result.length()); i < n; ++i)
      {
        char c = result[i];
        if (!isalnum(c) && c!='_')
          result[i] = '_';
      }
      return result;
    }
  }

  static void replace_range_by_spaces(char * from, char * to)
  {
    for (char * s = from; s < to; s++)
      if (*s != '\n' && *s != '\r')
        *s = ' ';
  }

  ImportErrorCb errorCb;
  void * cbUserPointer;

public:

  ImportParser(ImportErrorCb error_cb, void * cb_user_pointer)
  {
    errorCb = error_cb;
    cbUserPointer = cb_user_pointer;
  }

  // from - inclusive, to - not inclusive
  static void replaceImportBySpaces(char * from, char * to,
    const MODULE_PARSE_STL::vector<MODULE_PARSE_STL::pair<const char *, const char *>> & keep_ranges)
  {
    for (size_t i = 0; i < keep_ranges.size(); i++)
    {
      replace_range_by_spaces(from, (char *)keep_ranges[i].first);
      from = (char *)keep_ranges[i].second;
    }
    replace_range_by_spaces(from, to);
  }


  bool parse(const char ** src, int & line_, int & column_,
    MODULE_PARSE_STL::vector<ModuleImport> & result,
    MODULE_PARSE_STL::vector<MODULE_PARSE_STL::string> * directives_ = nullptr,
    MODULE_PARSE_STL::vector<MODULE_PARSE_STL::pair<const char *, const char *>> * keep_ranges_ = nullptr)
  {
    errors = false;
    directives = directives_;
    keepRanges = keep_ranges_;
    if (directives_)
      directives_->clear();
    if (keep_ranges_)
      keep_ranges_->clear();
    result.clear();
    if (isUtf8Bom(*src))
      *src += 3;
    p = *src;
    curLinePtr = p;
    line = line_;

    while (!errors)
    {
      skipSpaceAndComments();

      if (*p == '#')
      {
        while (*p == '#')
          parseDirective();
      }
      else if (accept("import"))
      {
        ModuleImport m;
        m.line = line;
        m.column = int(p - curLinePtr + 1);
        m.moduleName = parseIdentifier();

        if (accept("as"))
          m.moduleIdentifier = parseIdentifier(false);
        else
          m.moduleIdentifier = extractFileName(m.moduleName);

        result.push_back(m);
        accept(";");
      }
      else if (accept("from"))
      {
        ModuleImport m;
        m.line = line;
        m.column = int(p - curLinePtr + 1);
        m.moduleName = parseIdentifier();

        if (accept("as"))
          m.moduleIdentifier = parseIdentifier(false);
        else
          m.moduleIdentifier = extractFileName(m.moduleName);

        expect("import");

        while (!errors)
        {
          ModuleImport::Slot slot;
          slot.line = line;
          slot.column = int(p - curLinePtr + 1);
          bool importAll = false;
          while (!errors)
          {
            if (accept("*"))
            {
              slot.path.push_back(MODULE_PARSE_STL::string("*"));
              importAll = true;
              break;
            }

            slot.path.push_back(parseIdentifier());
            if (!accept("."))
              break;
          }

          if (accept("as"))
          {
            if (importAll)
              error("cannot rename '*'");

            slot.importAsIdentifier = parseIdentifier(false);
          }
          else if (!slot.path.empty() && !importAll)
          {
            slot.importAsIdentifier = slot.path.back();
          }

          m.slots.push_back(slot);

          if (!accept(","))
            break;
        }

        result.push_back(m);
        accept(";");
      }
      else
        break;
    }

    *src = p;
    line_ = line;
    column_ = int(p - curLinePtr + 1);
    return !errors;
  }
};

}
