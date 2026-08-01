// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <string_view>
#include <unordered_map>

#include "datablk.h"
#include "debug.h"
#include "common.h"
#include "ci.h"

namespace fs = std::filesystem;

TMatrix TMatrix::IDENT(1), TMatrix::ZERO(0);

static const std::unordered_map<std::string, DataBlock::ParamType, CaseInsensitiveHash, CaseInsensitiveEqual> type_map = {
  {"t", DataBlock::ParamType::TYPE_STRING},
  {"i", DataBlock::ParamType::TYPE_INT},
  {"b", DataBlock::ParamType::TYPE_BOOL},
  {"c", DataBlock::ParamType::TYPE_E3DCOLOR},
  {"r", DataBlock::ParamType::TYPE_REAL},
  {"m", DataBlock::ParamType::TYPE_MATRIX},
  {"p2", DataBlock::ParamType::TYPE_POINT2},
  {"p3", DataBlock::ParamType::TYPE_POINT3},
  {"p4", DataBlock::ParamType::TYPE_POINT4},
  {"ip2", DataBlock::ParamType::TYPE_IPOINT2},
  {"ip3", DataBlock::ParamType::TYPE_IPOINT3},
};

DataBlock::ParamType type(const DataBlock::Param &p)
{
  if (std::holds_alternative<std::string>(p))
    return DataBlock::ParamType::TYPE_STRING;
  if (std::holds_alternative<int>(p))
    return DataBlock::ParamType::TYPE_INT;
  if (std::holds_alternative<real>(p))
    return DataBlock::ParamType::TYPE_REAL;
  if (std::holds_alternative<Point2>(p))
    return DataBlock::ParamType::TYPE_POINT2;
  if (std::holds_alternative<Point3>(p))
    return DataBlock::ParamType::TYPE_POINT3;
  if (std::holds_alternative<Point4>(p))
    return DataBlock::ParamType::TYPE_POINT4;
  if (std::holds_alternative<IPoint2>(p))
    return DataBlock::ParamType::TYPE_IPOINT2;
  if (std::holds_alternative<IPoint3>(p))
    return DataBlock::ParamType::TYPE_IPOINT3;
  if (std::holds_alternative<bool>(p))
    return DataBlock::ParamType::TYPE_BOOL;
  if (std::holds_alternative<E3DCOLOR>(p))
    return DataBlock::ParamType::TYPE_E3DCOLOR;
  if (std::holds_alternative<TMatrix>(p))
    return DataBlock::ParamType::TYPE_MATRIX;
  return DataBlock::ParamType::TYPE_NONE;
}

static_assert(std::variant_size_v<DataBlock::Param> == 11,
  "DataBlock::Param alternatives changed: update type() and DataBlock::ParamType to match.");

static void makeFullPathFromRelative(std::string &path, std::string_view base_filename)
{
  if (path.empty() || base_filename.empty())
    return;

  if (path[0] == '/' || path[0] == '\\')
    return;

  if (path.size() > 1 && path[1] == ':')
    return;

  size_t i = base_filename.find_last_of("/\\:");
  if (i == std::string_view::npos)
    return;

  path.insert(0, base_filename.data(), i + 1);
}


static const char EOF_CHAR = 0;


class DataBlockParser
{
public:
  struct SyntaxErrorException : std::runtime_error
  {
    SyntaxErrorException(const char *s) : std::runtime_error(s) {}
  };

  std::string &buffer;

  const char *text, *curp, *textend;
  int curLine;

  std::vector<std::string> includeStack;

  DataBlockParser(std::string &buf, const char *fn) :
    buffer(buf), text(buf.data()), curp(buf.data()), textend(buf.data() + buf.size()), curLine(1)
  {
    std::replace(buffer.begin(), buffer.end(), EOF_CHAR, ' ');
    includeStack.emplace_back(fn ? fn : "");
  }

  const std::string &currentFile() const { return includeStack.back(); }

  void updatePointers()
  {
    int pos = curp - text;

    text = buffer.data();
    textend = text + buffer.size();
    curp = text + pos;
  }

  __forceinline bool endOfText() { return curp >= textend; }

  void skipWhite();
  bool getIdent(std::string &);
  void getValue(std::string &);
  void parse(DataBlock &, bool isTop);
};


void DataBlockParser::skipWhite()
{
  for (;;)
  {
    if (endOfText())
      break;

    char c = *curp++;

    if (c == EOF_CHAR)
    {
      if (includeStack.size() > 1)
        includeStack.pop_back();
      continue;
    }

    if (c == '\r')
    {
      if (!endOfText() && *curp == '\n')
      {
        ++curp;
        ++curLine;
      }
      continue;
    }
    else if (c == '\n')
    {
      ++curLine;
      continue;
    }

    if (c == ' ' || c == '\t' || c == '\x1A')
      continue;
    else if (c == '/')
    {
      if (!endOfText())
      {
        char nc = *curp++;
        if (nc == '/')
        {
          while (!endOfText())
          {
            char cc = *curp++;
            if (cc == '\r' || cc == '\n')
              break;
          }
          continue;
        }
        else if (nc == '*')
        {
          int cnt = 1;
          while (curp + 2 < textend)
          {
            if (curp[0] == '/' && curp[1] == '*')
            {
              curp += 2;
              ++cnt;
            }
            else if (curp[0] == '*' && curp[1] == '/')
            {
              curp += 2;
              if (--cnt <= 0)
                break;
            }
            else
              ++curp;
          }
          continue;
        }
        else
          --curp;
      }

      --curp;
      break;
    }
    else
    {
      --curp;
      break;
    }
  }
}


bool DataBlockParser::getIdent(std::string &name)
{
  for (;;)
  {
    skipWhite();

    if (endOfText())
      break;

    char c = *curp;
    if (c == '_' || isalnum(c))
    {
      const char *ident = curp;
      for (++curp; !endOfText(); ++curp)
      {
        c = *curp;
        if (!(c == '_' || isalnum(c)))
          break;
      }
      int len = curp - ident;
      name = std::string(ident, len);
      return true;
    }
    else
      break;
  }
  return false;
}


void DataBlockParser::getValue(std::string &value)
{
  value.clear();

  const char *valptr = curp;
  char qc = 0;
  if (*valptr == '"' || *valptr == '\'')
  {
    qc = *valptr++;
    ++curp;
  }

  for (;;)
  {
    if (endOfText())
      throw SyntaxErrorException("unexpected EOF");

    char c = *curp;

    if (qc)
    {
      if (c == qc)
      {
        ++curp;
        skipWhite();
        if (*curp == ';')
          ++curp;
        break;
      }
      else if (c == '\r' || c == '\n' || c == EOF_CHAR)
        throw SyntaxErrorException("unclosed string");
      else if (c == '~')
      {
        ++curp;

        if (endOfText())
          throw SyntaxErrorException("unclosed string");

        c = *curp;
        if (c == 'r')
          c = '\r';
        else if (c == 'n')
          c = '\n';
        else if (c == 't')
          c = '\t';
      }
    }
    else
    {
      if (c == ';' || c == '\r' || c == '\n' || c == EOF_CHAR)
      {
        if (c == ';')
          ++curp;
        break;
      }
    }

    value += c;

    ++curp;
  }

  if (!qc)
  {
    int i;
    for (i = int(value.size() - 1); i >= 0; --i)
      if (value[i] != ' ' && value[i] != '\t')
        break;
    ++i;
    if (i < int(value.size()))
      value.erase(i);
  }
}


void DataBlockParser::parse(DataBlock &blk, bool isTop)
{
  for (;;)
  {
    skipWhite();

    if (endOfText())
      break;


    if (*curp == '}')
    {
      if (isTop)
        throw SyntaxErrorException("unexpected '}' in top block");
      ++curp;
      break;
    }

    const char *start = curp;

    std::string name;
    if (!getIdent(name))
      throw SyntaxErrorException("expected identifier");

    skipWhite();
    if (endOfText())
      throw SyntaxErrorException("unexpected EOF");

    if (*curp == '{')
    {
      ++curp;
      auto nb = std::make_unique<DataBlock>(blk.nameMap);
      nb->setBlockName(name.data());
      parse(*nb, false);
      blk.addBlock(std::move(nb));
    }
    else if (*curp == ':')
    {
      ++curp;
      std::string typeName;
      if (!getIdent(typeName))
        throw SyntaxErrorException("expected type identifier");

      DataBlock::ParamType type = DataBlock::deserialize_param_type(typeName);

      skipWhite();

      if (endOfText())
        throw SyntaxErrorException("unexpected EOF");

      if (*curp++ != '=')
        throw SyntaxErrorException("expected '='");

      skipWhite();

      if (endOfText())
        throw SyntaxErrorException("unexpected EOF");

      std::string value;
      getValue(value);
      blk.addParam(name.data(), type, value.data(), curLine, currentFile().c_str());
    }
    else if (stricmp(name.data(), "include") == 0)
    {
      std::string value;
      getValue(value);

      buffer.erase(start - text, curp - start - 1);
      curp = start;
      *(char *)curp = EOF_CHAR;

      makeFullPathFromRelative(value, currentFile());

      const std::string baseFileName = currentFile();

      includeStack.emplace_back(value);

      std::ifstream is(fs::path(strToWide(value)), std::ios::binary);

      if (!is)
      {
        debug("can't open include file '%s' for '%s'\n", value.data(), baseFileName.c_str());
        throw SyntaxErrorException("can't open include file");
      }

      std::string buf((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
      if (buf.empty())
        throw SyntaxErrorException("error loading include file");

      std::replace(buf.begin(), buf.end(), EOF_CHAR, ' ');

      int pos = curp - text;
      buffer.insert(pos, buf);

      updatePointers();
    }
    else
      throw SyntaxErrorException("syntax error");
  }
}


DataBlock::ParamType DataBlock::deserialize_param_type(std::string_view s)
{
  auto it = type_map.find(s);
  if (it != type_map.end())
    return it->second;

  return ParamType::TYPE_NONE;
}

void DataBlock::setBlockName(const char *name) { nameId = nameMap->addNameId(name); }

int DataBlock::addBlock(std::unique_ptr<DataBlock> blk)
{
  if (!blk)
    return -1;

  blocks.push_back(std::move(blk));
  return int(blocks.size()) - 1;
}

int DataBlock::addParam(const char *name, ParamType type, const char *value, int line, const char *filename)
{
  nameIds.emplace_back(nameMap->addNameId(name));

  char *org_locale = setlocale(LC_ALL, "C");

  switch (type)
  {
    case ParamType::TYPE_STRING: params.emplace_back(std::string(value)); break;

    case ParamType::TYPE_INT: params.emplace_back(std::stoi(value)); break;

    case ParamType::TYPE_REAL: params.emplace_back(std::stof(value)); break;

    case ParamType::TYPE_POINT2:
    {
      Point2 p2(0.f, 0.f);
      int res = sscanf(value, " %f , %f", &p2.x, &p2.y);
      if (res != 2)
        debug("invalid point2 value in line %d of '%s'\n", line, filename);
      params.emplace_back(p2);
    }
    break;

    case ParamType::TYPE_POINT3:
    {
      Point3 p3(0.f, 0.f, 0.f);
      int res = sscanf(value, " %f , %f , %f", &p3.x, &p3.y, &p3.z);
      if (res != 3)
        debug("invalid point3 value in line %d of '%s'\n", line, filename);
      params.emplace_back(p3);
    }
    break;

    case ParamType::TYPE_POINT4:
    {
      Point4 p4(0.f, 0.f, 0.f, 0.f);
      int res = sscanf(value, " %f , %f , %f , %f", &p4.x, &p4.y, &p4.z, &p4.w);
      if (res != 4)
        debug("invalid point4 value in line %d of '%s'\n", line, filename);
      params.emplace_back(p4);
    }
    break;

    case ParamType::TYPE_IPOINT2:
    {
      IPoint2 ip2(0.f, 0.f);
      int res = sscanf(value, " %i , %i", &ip2.x, &ip2.y);
      if (res != 2)
        debug("invalid ipoint2 value in line %d of '%s'\n", line, filename);
      params.emplace_back(ip2);
    }
    break;

    case ParamType::TYPE_IPOINT3:
    {
      IPoint3 ip3(0.f, 0.f, 0.f);
      int res = sscanf(value, " %i , %i , %i", &ip3.x, &ip3.y, &ip3.z);
      if (res != 3)
        debug("invalid ipoint3 value in line %d of '%s'\n", line, filename);
      params.emplace_back(ip3);
    }
    break;

    case ParamType::TYPE_BOOL:
    {
      bool b = false;
      if (stricmp(value, "yes") == 0 || stricmp(value, "on") == 0 || stricmp(value, "true") == 0 || stricmp(value, "1") == 0)
        b = true;
      else if (stricmp(value, "no") == 0 || stricmp(value, "off") == 0 || stricmp(value, "false") == 0 || stricmp(value, "0") == 0)
        b = false;
      else
      {
        b = false;
        debug("invalid boolean value '%s' in line %d of '%s'\n", value, line, filename);
      }
      params.emplace_back(b);
    }
    break;

    case ParamType::TYPE_E3DCOLOR:
    {
      int r = 255, g = 255, b = 255, a = 255;
      int res = sscanf(value, " %d , %d , %d , %d", &r, &g, &b, &a);
      //== check value range
      if (res < 3)
        debug("invalid e3dcolor value in line %d of '%s'\n", line, filename);

      E3DCOLOR c;
      c.r = r;
      c.g = g;
      c.b = b;
      c.a = a;
      params.emplace_back(c);
    }
    break;

    case ParamType::TYPE_MATRIX:
    {
      TMatrix tm = TMatrix::IDENT;
      int res = sscanf(value,
        "[[ %f , %f , %f ] [ %f , %f , %f ] "
        "[ %f , %f , %f ] [ %f , %f , %f ]]",
        &tm.m[0][0], &tm.m[0][1], &tm.m[0][2], &tm.m[1][0], &tm.m[1][1], &tm.m[1][2], &tm.m[2][0], &tm.m[2][1], &tm.m[2][2],
        &tm.m[3][0], &tm.m[3][1], &tm.m[3][2]);

      if (res != 12)
        debug("invalid TMatrix value in line %d of '%s'\n", line, filename);

      params.emplace_back(tm);
    }
    break;

    default:
      nameIds.pop_back();
      debug("addBlock error: unknown param type");
      break;
  }

  setlocale(LC_ALL, org_locale);
  return int(params.size()) - 1;
}


int DataBlock::setStr(const char *name, const char *value)
{
  int id = findParam(name);
  if (id < 0 || type(params[id]) != ParamType::TYPE_STRING)
    return addStr(name, value);

  params[id] = std::string(value);
  return id;
}

int DataBlock::setBool(const char *name, bool value)
{
  int id = findParam(name);
  if (id < 0 || type(params[id]) != ParamType::TYPE_BOOL)
    return addBool(name, value);

  params[id] = value;
  return id;
}

int DataBlock::setInt(const char *name, int value)
{
  int id = findParam(name);
  if (id < 0 || type(params[id]) != ParamType::TYPE_INT)
    return addInt(name, value);

  params[id] = value;
  return id;
}

int DataBlock::setReal(const char *name, real value)
{
  int id = findParam(name);
  if (id < 0 || type(params[id]) != ParamType::TYPE_REAL)
    return addReal(name, value);

  params[id] = value;
  return id;
}

int DataBlock::setPoint3(const char *name, const Point3 &value)
{
  int id = findParam(name);
  if (id < 0 || type(params[id]) != ParamType::TYPE_POINT3)
    return addPoint3(name, value);

  params[id] = value;
  return id;
}


int DataBlock::addStr(const char *name, const char *value)
{
  params.emplace_back(std::string(value));
  nameIds.emplace_back(nameMap->addNameId(name));
  return int(params.size()) - 1;
}

int DataBlock::addBool(const char *name, bool value)
{
  params.emplace_back(value);
  nameIds.emplace_back(nameMap->addNameId(name));
  return int(params.size()) - 1;
}

int DataBlock::addInt(const char *name, int value)
{
  params.emplace_back(value);
  nameIds.emplace_back(nameMap->addNameId(name));
  return int(params.size()) - 1;
}

int DataBlock::addReal(const char *name, real value)
{
  params.emplace_back(value);
  nameIds.emplace_back(nameMap->addNameId(name));
  return int(params.size()) - 1;
}

int DataBlock::addPoint3(const char *name, const Point3 &value)
{
  params.emplace_back(value);
  nameIds.emplace_back(nameMap->addNameId(name));
  return int(params.size()) - 1;
}


DataBlock::DataBlock(std::shared_ptr<NameMap> nm) : nameMap(nm), nameId(-1) {}

DataBlock::~DataBlock() {}

// reset class (clear all data & names)
void DataBlock::reset()
{
  nameId = -1;
  nameMap->clear();
  clearData();
}


// delete all children
void DataBlock::clearData()
{
  params.clear();
  nameIds.clear();
  blocks.clear();
}


bool DataBlock::loadText(std::string &text, const char *filename)
{
  reset();
  DataBlockParser parser(text, filename);

  try
  {
    parser.parse(*this, true);
  }
  catch (DataBlockParser::SyntaxErrorException e)
  {
    debug("DataBlock error in line %d of '%s':\n  %s\n", parser.curLine, filename ? filename : "<unknown>", e.what());

    if (!paramCount())
      reset();

    return false;
  }
  catch (std::invalid_argument &e)
  {
    debug("DataBlock error in line %d of '%s': invalid numeric value:\n  %s\n", parser.curLine, filename ? filename : "<unknown>",
      e.what());

    if (!paramCount())
      reset();

    return false;
  }
  catch (std::out_of_range &e)
  {
    debug("DataBlock error in line %d of '%s': numeric value out of range:\n  %s\n", parser.curLine, filename ? filename : "<unknown>",
      e.what());

    if (!paramCount())
      reset();

    return false;
  }

  return true;
}

bool DataBlock::loadText(const char *text, int len, const char *filename)
{
  std::string buf(text, len);
  return loadText(buf, filename);
}


bool DataBlock::load(const fs::path &fname)
{
  reset();
  if (fname.empty())
    return false;

  std::ifstream is(fname, std::ios::binary);

  const std::string narrow_name = wideToStr(fname.native());

  if (!is)
  {
    debug("can't open include file '%s'\n", narrow_name.c_str());
    return false;
  }

  return loadFromStream(is, narrow_name.c_str());
}


bool DataBlock::loadFromStream(std::ifstream &is, const char *fname)
{
  reset();
  std::string text((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
  return loadText(text, fname);
}


// Saving

static void writeString(std::ofstream &os, const char *s)
{
  if (s && *s)
    os << s;
}

static void writeStringValue(std::ofstream &os, const char *s)
{
  if (!s)
    s = "";

  os << '"';
  for (; *s; ++s)
  {
    char c = *s;
    if (c == '~')
      os << "~~";
    else if (c == '"')
      os << "~\"";
    else if (c == '\r')
      os << "~r";
    else if (c == '\n')
      os << "~n";
    else if (c == '\t')
      os << "~t";
    else
      os << c;
  }
  os << '"';
}


void DataBlock::saveText(std::ofstream &os, int level) const
{
  for (size_t i = 0; i < params.size(); ++i)
  {
    const Param &p = params[i];

    os << std::string(level * 2, ' ');
    writeString(os, getName(nameIds[i]));

    // clang-format off
    switch (type(p))
    {
      case ParamType::TYPE_STRING:
        os << ":t=";
        writeStringValue(os, std::get<std::string>(p).data());
        break;

      case ParamType::TYPE_BOOL:
        os << ":b=" << (std::get<bool>(p) ? "yes" : "no");
        break;

      case ParamType::TYPE_INT:
        os << ":i=" << std::get<int>(p);
        break;

      case ParamType::TYPE_REAL:
        os << ":r=" << std::get<real>(p);
        break;

      case ParamType::TYPE_POINT2:
      {
        auto &p2 = std::get<Point2>(p);
        os << ":p2=" << p2.x << ", " << p2.y;
      }
        break;

      case ParamType::TYPE_POINT3:
      {
        auto &p3 = std::get<Point3>(p);
        os << ":p3=" << p3.x << ", " << p3.y << ", " << p3.z;
      }
        break;

      case ParamType::TYPE_POINT4:
      {
        auto &p4 = std::get<Point4>(p);
        os << ":p4=" << p4.x << ", " << p4.y << ", " << p4.z << ", " << p4.w;
      }
        break;

      case ParamType::TYPE_IPOINT2:
      {
        auto &ip2 = std::get<IPoint2>(p);
        os << ":ip2=" << ip2.x << ", " << ip2.y;
      }
        break;

      case ParamType::TYPE_IPOINT3:
      {
        auto &ip3 = std::get<IPoint3>(p);
        os << ":ip3=" << ip3.x << ", " << ip3.y << ", " << ip3.z;
      }
        break;

      case ParamType::TYPE_E3DCOLOR:
      {
        auto &c = std::get<E3DCOLOR>(p);
        os << ":c=" << c.r << ", " << c.g << ", " << c.b << ", " << c.a;
      }
        break;

      case ParamType::TYPE_MATRIX:
      {
        auto &m = std::get<TMatrix>(p);
        os << ":m=[";
        os << "[" << m.getcol(0).x << ", " << m.getcol(0).y << ", " << m.getcol(0).z << "]";
        os << "[" << m.getcol(1).x << ", " << m.getcol(1).y << ", " << m.getcol(1).z << "]";
        os << "[" << m.getcol(2).x << ", " << m.getcol(2).y << ", " << m.getcol(2).z << "]";
        os << "[" << m.getcol(3).x << ", " << m.getcol(3).y << ", " << m.getcol(3).z << "]";
        os << "]";
      }
        break;

      default: debug("unknown type");
    }
    os << "\r\n";
  }
  // clang-format on

  if (!params.empty() && !blocks.empty())
    os << std::string(level * 2, ' ') << "\r\n";

  for (const auto &b : blocks)
  {
    os << std::string(level * 2, ' ');
    writeString(os, getName(b->nameId));
    os << "{\r\n";

    b->saveText(os, level + 1);

    os << std::string(level * 2, ' ') << "}\r\n";

    if (&b != &blocks.back())
      os << "\r\n";
  }
}

bool DataBlock::saveToTextFile(const fs::path &filename) const
{
  std::ofstream os(filename, std::ios::binary);
  if (!os)
  {
    debug(_T("cant open '%s' file for writing"), filename.c_str());
    return false;
  }

  saveText(os);
  return true;
}

// Names

int DataBlock::getNameId(const char *name) const { return nameMap->getNameId(name); }

const char *DataBlock::getName(int nid) const { return nameMap->getName(nid); }

// Sub-blocks

DataBlock *DataBlock::getBlock(int i) const
{
  if (i < 0 || i >= int(blocks.size()))
    return NULL;
  return blocks[i].get();
}

DataBlock *DataBlock::getBlockByName(int nid, int after) const
{
  for (int i = after + 1; i < int(blocks.size()); ++i)
    if (blocks[i] && blocks[i]->nameId == nid)
      return blocks[i].get();
  return NULL;
}

// Parameters

DataBlock::ParamType DataBlock::getParamType(int i) const
{
  if (i < 0 || i >= int(params.size()))
    return ParamType::TYPE_NONE;
  return type(params[i]);
}

int DataBlock::getParamNameId(int i) const
{
  if (i < 0 || i >= int(nameIds.size()))
    return -1;
  return nameIds[i];
}

const char *DataBlock::getStr(int i, const char *def) const
{
  if (i < 0 || i >= int(params.size()) || type(params[i]) != ParamType::TYPE_STRING)
    return def;
  return std::get<std::string>(params[i]).c_str();
}

int DataBlock::getInt(int i, int def) const
{
  if (i < 0 || i >= int(params.size()) || type(params[i]) != ParamType::TYPE_INT)
    return def;
  return std::get<int>(params[i]);
}

bool DataBlock::getBool(int i, bool def) const
{
  if (i < 0 || i >= int(params.size()) || type(params[i]) != ParamType::TYPE_BOOL)
    return def;
  return std::get<bool>(params[i]);
}

real DataBlock::getReal(int i, real def) const
{
  if (i < 0 || i >= int(params.size()) || type(params[i]) != ParamType::TYPE_REAL)
    return def;
  return std::get<real>(params[i]);
}

Point3 DataBlock::getPoint3(int i, const Point3 &def) const
{
  if (i < 0 || i >= int(params.size()) || type(params[i]) != ParamType::TYPE_POINT3)
    return def;
  return std::get<Point3>(params[i]);
}


int DataBlock::findParam(int nid, int after) const
{
  for (int i = after + 1; i < int(nameIds.size()); ++i)
    if (nameIds[i] == nid)
      return i;
  return -1;
}

std::optional<std::reference_wrapper<const DataBlock::Param>> DataBlock::getParam(int param_number) const
{
  if (param_number < 0 || param_number >= int(params.size()))
    return std::nullopt;
  return params[param_number];
}

const char *DataBlock::getStr(const char *name, const char *def) const { return getStr(findParam(name), def); }

int DataBlock::getInt(const char *name, int def) const { return getInt(findParam(name), def); }

bool DataBlock::getBool(const char *name, bool def) const { return getBool(findParam(name), def); }

real DataBlock::getReal(const char *name, real def) const { return getReal(findParam(name), def); }

Point3 DataBlock::getPoint3(const char *name, const Point3 &def) const { return getPoint3(findParam(name), def); }
