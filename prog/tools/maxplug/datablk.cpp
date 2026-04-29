// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <unordered_map>

#include "datablk.h"
#include "debug.h"
#include "common.h"
#include "ci.h"

TMatrix TMatrix::IDENT(1), TMatrix::ZERO(0);

static const std::unordered_map<std::string, DataBlock::ParamType, CaseInsensitiveHash, CaseInsensitiveEqual> &type_map()
{
  // old C++ doesn't recognize the static initializer list for complex containers
  static std::unordered_map<std::string, DataBlock::ParamType, CaseInsensitiveHash, CaseInsensitiveEqual> tmap;
  if (tmap.empty())
  {
    tmap.emplace("t", DataBlock::ParamType::TYPE_STRING);
    tmap.emplace("i", DataBlock::ParamType::TYPE_INT);
    tmap.emplace("b", DataBlock::ParamType::TYPE_BOOL);
    tmap.emplace("c", DataBlock::ParamType::TYPE_E3DCOLOR);
    tmap.emplace("r", DataBlock::ParamType::TYPE_REAL);
    tmap.emplace("m", DataBlock::ParamType::TYPE_MATRIX);
    tmap.emplace("p2", DataBlock::ParamType::TYPE_POINT2);
    tmap.emplace("p3", DataBlock::ParamType::TYPE_POINT3);
    tmap.emplace("p4", DataBlock::ParamType::TYPE_POINT4);
    tmap.emplace("ip2", DataBlock::ParamType::TYPE_IPOINT2);
    tmap.emplace("ip3", DataBlock::ParamType::TYPE_IPOINT3);
  }
  return tmap;
}

static void makeFullPathFromRelative(std::string &path, const std::string &base_filename)
{
  if (base_filename.empty())
    return;

  if (path[0] == '/' || path[0] == '\\')
    return;

  size_t i = base_filename.find_last_of("/\\:");
  if (i == std::string::npos)
    return;

  path.insert(0, base_filename.data(), i);
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
  const char *fileName;
  int curLine;

  std::vector<std::string> includeStack;

  DataBlockParser(std::string &buf, const char *fn) :
    buffer(buf), text(buf.data()), curp(buf.data()), textend(buf.data() + buf.size()), fileName(fn), curLine(1)
  {
    std::replace(buffer.begin(), buffer.end(), EOF_CHAR, ' ');
    includeStack.emplace_back(fileName ? fileName : "");
  }

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
      if (!includeStack.empty())
      {
        includeStack.pop_back();
        if (!includeStack.empty())
          fileName = includeStack.back().data();
      }
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
      DataBlock *nb = new DataBlock(blk.nameMap);
      nb->setBlockName(name.data());
      blk.addBlock(nb);
      parse(*nb, false);
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
      blk.addParam(name.data(), type, value.data(), curLine, fileName);
    }
    else if (stricmp(name.data(), "include") == 0)
    {
      std::string value;
      getValue(value);

      buffer.erase(start - text, curp - start - 1);
      curp = start;
      *(char *)curp = EOF_CHAR;

      makeFullPathFromRelative(value, fileName);

      const char *baseFileName = fileName;

      includeStack.emplace_back(value);
      fileName = includeStack.back().data();

      std::ifstream is(value, std::ios::binary);

      if (!is)
      {
        debug("can't open include file '%s' for '%s'\n", value.data(), baseFileName);
        throw SyntaxErrorException("can't open include file");
      }

      std::string buf((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
      if (buf.empty())
        throw SyntaxErrorException("error loading include file");

      int pos = curp - text;
      buffer.insert(pos, buf);

      std::replace(buffer.begin(), buffer.end(), EOF_CHAR, ' ');

      updatePointers();
    }
    else
      throw SyntaxErrorException("syntax error");
  }
}


DataBlock *DataBlock::emptyBlock = NULL;

DataBlock::ParamType DataBlock::deserialize_param_type(const std::string &s)
{
  auto it = type_map().find(s);
  if (it != type_map().end())
    return it->second;

  return ParamType::TYPE_NONE;
}

void DataBlock::setBlockName(const char *name) { nameId = nameMap->addNameId(name); }

int DataBlock::addBlock(DataBlock *blk)
{
  if (!blk)
    return -1;

  blocks.emplace_back(blk);
  return int(blocks.size()) - 1;
}

int DataBlock::addParam(const char *name, ParamType type, const char *value, int line, const char *filename)
{
  int nameId = nameMap->addNameId(name);

  char *org_locale = setlocale(LC_ALL, "C");

  switch (type)
  {
    case ParamType::TYPE_STRING: params.emplace_back(nameId, std::string(value)); break;

    case ParamType::TYPE_INT: params.emplace_back(nameId, std::stoi(value)); break;

    case ParamType::TYPE_REAL: params.emplace_back(nameId, std::stof(value)); break;

    case ParamType::TYPE_POINT2:
    {
      Point2 p2(0.f, 0.f);
      int res = sscanf(value, " %f , %f", &p2.x, &p2.y);
      if (res != 2)
        debug("invalid point2 value in line %d of '%s'\n", line, filename);
      params.emplace_back(nameId, p2);
    }
    break;

    case ParamType::TYPE_POINT3:
    {
      Point3 p3(0.f, 0.f, 0.f);
      int res = sscanf(value, " %f , %f , %f", &p3.x, &p3.y, &p3.z);
      if (res != 3)
        debug("invalid point3 value in line %d of '%s'\n", line, filename);
      params.emplace_back(nameId, p3);
    }
    break;

    case ParamType::TYPE_POINT4:
    {
      Point4 p4(0.f, 0.f, 0.f, 0.f);
      int res = sscanf(value, " %f , %f , %f , %f", &p4.x, &p4.y, &p4.z, &p4.w);
      if (res != 4)
        debug("invalid point4 value in line %d of '%s'\n", line, filename);
      params.emplace_back(nameId, p4);
    }
    break;

    case ParamType::TYPE_IPOINT2:
    {
      IPoint2 ip2(0.f, 0.f);
      int res = sscanf(value, " %i , %i", &ip2.x, &ip2.y);
      if (res != 2)
        debug("invalid ipoint2 value in line %d of '%s'\n", line, filename);
      params.emplace_back(nameId, ip2);
    }
    break;

    case ParamType::TYPE_IPOINT3:
    {
      IPoint3 ip3(0.f, 0.f, 0.f);
      int res = sscanf(value, " %i , %i , %i", &ip3.x, &ip3.y, &ip3.z);
      if (res != 3)
        debug("invalid ipoint3 value in line %d of '%s'\n", line, filename);
      params.emplace_back(nameId, ip3);
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
      params.emplace_back(nameId, b);
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
      params.emplace_back(nameId, c);
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

      params.emplace_back(nameId, tm);
    }
    break;

    default: debug("error");
  }

  setlocale(LC_ALL, org_locale);
  return int(params.size()) - 1;
}


DataBlock *DataBlock::addBlock(const char *name)
{
  DataBlock *blk = getBlockByName(getNameId(name));
  if (blk)
    return blk;

  return addNewBlock(name);
}


DataBlock *DataBlock::addNewBlock(const char *name)
{
  DataBlock *nb = new DataBlock(nameMap);
  nb->setBlockName(name);
  addBlock(nb);
  return nb;
}


bool DataBlock::removeBlock(const char *name)
{
  int nameId = getNameId(name);
  if (nameId < 0)
    return false;

  bool removed = false;

  for (int i = int(blocks.size()) - 1; i >= 0; --i)
    if (blocks[i] && blocks[i]->getBlockNameId() == nameId)
    {
      blocks.erase(blocks.begin() + i);
      removed = true;
    }

  return removed;
}


bool DataBlock::removeParam(const char *name)
{
  int nameId = getNameId(name);
  if (nameId < 0)
    return false;

  bool removed = false;

  for (int i = int(params.size()) - 1; i >= 0; --i)
    if (params[i].nameId == nameId)
    {
      params.erase(params.begin() + i);
      removed = true;
    }

  return removed;
}


void DataBlock::setParamsFrom(const DataBlock *blk)
{
  if (!blk)
    return;

  params.clear();

  int num = blk->paramCount();
  for (int i = 0; i < num; ++i)
  {
    const char *name = blk->getParamName(i);

    switch (blk->getParamType(i))
    {
      case ParamType::TYPE_STRING: addStr(name, blk->getStr(i)); break;
      case ParamType::TYPE_INT: addInt(name, blk->getInt(i)); break;
      case ParamType::TYPE_REAL: addReal(name, blk->getReal(i)); break;
      case ParamType::TYPE_POINT2: addPoint2(name, blk->getPoint2(i)); break;
      case ParamType::TYPE_POINT3: addPoint3(name, blk->getPoint3(i)); break;
      case ParamType::TYPE_POINT4: addPoint4(name, blk->getPoint4(i)); break;
      case ParamType::TYPE_IPOINT2: addIPoint2(name, blk->getIPoint2(i)); break;
      case ParamType::TYPE_IPOINT3: addIPoint3(name, blk->getIPoint3(i)); break;
      case ParamType::TYPE_BOOL: addBool(name, blk->getBool(i)); break;
      case ParamType::TYPE_E3DCOLOR: addE3dcolor(name, blk->getE3dcolor(i)); break;
      case ParamType::TYPE_MATRIX: addTm(name, blk->getTm(i)); break;
      default: debug("error");
    }
  }
}


DataBlock *DataBlock::addNewBlock(const DataBlock *blk, const char *as_name)
{
  if (!blk)
    return NULL;

  DataBlock *newBlk = addNewBlock(as_name ? as_name : blk->getBlockName());

  newBlk->setParamsFrom(blk);

  // add sub-blocks
  int num = blk->blockCount();
  for (int i = 0; i < num; ++i)
    newBlk->addNewBlock(blk->getBlock(i));

  return newBlk;
}


void DataBlock::setFrom(const DataBlock *from)
{
  clearData();

  if (!from)
    return;

  setParamsFrom(from);

  for (int i = 0; i < from->blockCount(); ++i)
    addNewBlock(from->getBlock(i));
}


int DataBlock::setStr(const char *name, const char *value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_STRING)
    return addStr(name, value);

  params[id].set_str(value);
  return id;
}

int DataBlock::setBool(const char *name, bool value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_BOOL)
    return addBool(name, value);

  params[id].set_bool(value);
  return id;
}

int DataBlock::setInt(const char *name, int value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_INT)
    return addInt(name, value);

  params[id].set_int(value);
  return id;
}

int DataBlock::setReal(const char *name, real value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_REAL)
    return addReal(name, value);

  params[id].set_real(value);
  return id;
}

int DataBlock::setPoint2(const char *name, const Point2 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_POINT2)
    return addPoint2(name, value);

  params[id].set_pt2(value);
  return id;
}

int DataBlock::setPoint3(const char *name, const Point3 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_POINT3)
    return addPoint3(name, value);

  params[id].set_pt3(value);
  return id;
}

int DataBlock::setPoint4(const char *name, const Point4 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_POINT4)
    return addPoint4(name, value);

  params[id].set_pt4(value);
  return id;
}

int DataBlock::setIPoint2(const char *name, const IPoint2 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_IPOINT2)
    return addIPoint2(name, value);

  params[id].set_ipt2(value);
  return id;
}

int DataBlock::setIPoint3(const char *name, const IPoint3 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_IPOINT3)
    return addIPoint3(name, value);

  params[id].set_ipt3(value);
  return id;
}

int DataBlock::setE3dcolor(const char *name, const E3DCOLOR value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_E3DCOLOR)
    return addE3dcolor(name, value);

  params[id].set_color(value);
  return id;
}

int DataBlock::setTm(const char *name, const TMatrix &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != ParamType::TYPE_MATRIX)
    return addTm(name, value);

  params[id].set_tm(value);
  return id;
}


int DataBlock::addStr(const char *name, const char *value)
{
  params.emplace_back(Param(nameMap->addNameId(name), std::string(value)));
  return int(params.size()) - 1;
}

int DataBlock::addBool(const char *name, bool value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
  return int(params.size()) - 1;
}

int DataBlock::addInt(const char *name, int value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
  return int(params.size()) - 1;
}

int DataBlock::addReal(const char *name, real value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
  return int(params.size()) - 1;
}

int DataBlock::addPoint2(const char *name, const Point2 &value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
  return int(params.size()) - 1;
}

int DataBlock::addPoint3(const char *name, const Point3 &value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
  return int(params.size()) - 1;
}

int DataBlock::addPoint4(const char *name, const Point4 &value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
  return int(params.size()) - 1;
}

int DataBlock::addIPoint2(const char *name, const IPoint2 &value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
  return int(params.size()) - 1;
}

int DataBlock::addIPoint3(const char *name, const IPoint3 &value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
  return int(params.size()) - 1;
}

int DataBlock::addE3dcolor(const char *name, const E3DCOLOR value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
  return int(params.size()) - 1;
}

int DataBlock::addTm(const char *name, const TMatrix &value)
{
  params.emplace_back(Param(nameMap->addNameId(name), value));
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

  return true;
}

bool DataBlock::loadText(char *text, int len, const char *filename)
{
  std::string buf(text, len);
  return loadText(buf, filename);
}


bool DataBlock::load(const std::wstring &fname)
{
  reset();
  if (fname.empty())
    return false;

  std::string fileName = wideToStr(fname.data()).data(); // FIXME

  std::ifstream is(fileName, std::ios::binary);
  if (!is)
  {
    debug("can't open include file '%s'\n", fname);
    return false;
  }

  return loadFromStream(is, fileName.data());
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
  for (const Param &p : params)
  {
    os << std::string(level * 2, ' ');
    writeString(os, getName(p.nameId));

    // clang-format off
    switch (p.type)
    {
      case ParamType::TYPE_STRING:
        os << ":t=";
        writeStringValue(os, p.as_c_str());
        break;

      case ParamType::TYPE_BOOL:
        os << ":b=" << (p.as_bool() ? "yes" : "no");
        break;

      case ParamType::TYPE_INT:
        os << ":i=" << p.as_int();
        break;

      case ParamType::TYPE_REAL:
        os << ":r=" << p.as_real();
        break;

      case ParamType::TYPE_POINT2:
        os << ":p2=" << p.as_pt2().x << ", " << p.as_pt2().y;
        break;

      case ParamType::TYPE_POINT3:
        os << ":p3=" << p.as_pt3().x << ", " << p.as_pt3().y << ", " << p.as_pt3().z;
        break;

      case ParamType::TYPE_POINT4:
        os << ":p4=" << p.as_pt4().x << ", " << p.as_pt4().y << ", " << p.as_pt4().z << ", " << p.as_pt4().w;
        break;

      case ParamType::TYPE_IPOINT2:
        os << ":ip2=" << p.as_ipt2().x << ", " << p.as_ipt2().y;
        break;

      case ParamType::TYPE_IPOINT3:
        os << ":ip3=" << p.as_ipt3().x << ", " << p.as_ipt3().y << ", " << p.as_ipt3().z;
      break;

      case ParamType::TYPE_E3DCOLOR:
        os << ":c=" << p.as_color().r << ", " << p.as_color().g << ", " << p.as_color().b << ", " << p.as_color().a;
        break;

      case ParamType::TYPE_MATRIX:
      {
        os << ":m=[";
        os << "[" << p.as_tm().getcol(0).x << ", " << p.as_tm().getcol(0).y << ", " << p.as_tm().getcol(0).z << "]";
        os << "[" << p.as_tm().getcol(1).x << ", " << p.as_tm().getcol(1).y << ", " << p.as_tm().getcol(1).z << "]";
        os << "[" << p.as_tm().getcol(2).x << ", " << p.as_tm().getcol(2).y << ", " << p.as_tm().getcol(2).z << "]";
        os << "[" << p.as_tm().getcol(3).x << ", " << p.as_tm().getcol(3).y << ", " << p.as_tm().getcol(3).z << "]";
        os << "]";
        break;
      }

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

bool DataBlock::saveToTextFile(const std::wstring &filename) const
{
  std::ofstream os(filename, std::ios::binary);
  if (!os)
  {
    debug(_T("cant open '%s' file for writing"), filename.data());
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
  return params[i].type;
}

int DataBlock::getParamNameId(int i) const
{
  if (i < 0 || i >= int(params.size()))
    return -1;
  return params[i].nameId;
}

const char *DataBlock::getStr(int i, const char *def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_STRING)
    return def;
  return params[i].as_c_str();
}

int DataBlock::getInt(int i, int def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_INT)
    return def;
  return params[i].as_int();
}

bool DataBlock::getBool(int i, bool def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_BOOL)
    return def;
  return params[i].as_bool();
}

real DataBlock::getReal(int i, real def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_REAL)
    return def;
  return params[i].as_real();
}

Point2 DataBlock::getPoint2(int i, const Point2 &def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_POINT2)
    return def;
  return params[i].as_pt2();
}

Point3 DataBlock::getPoint3(int i, const Point3 &def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_POINT3)
    return def;
  return params[i].as_pt3();
}

Point4 DataBlock::getPoint4(int i, const Point4 &def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_POINT4)
    return def;
  return params[i].as_pt4();
}

IPoint2 DataBlock::getIPoint2(int i, const IPoint2 &def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_IPOINT2)
    return def;
  return params[i].as_ipt2();
}

IPoint3 DataBlock::getIPoint3(int i, const IPoint3 &def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_IPOINT3)
    return def;
  return params[i].as_ipt3();
}

E3DCOLOR DataBlock::getE3dcolor(int i, const E3DCOLOR &def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_E3DCOLOR)
    return def;
  return params[i].as_color();
}

TMatrix DataBlock::getTm(int i, const TMatrix &def) const
{
  if (i < 0 || i >= int(params.size()) || params[i].type != ParamType::TYPE_MATRIX)
    return def;
  return params[i].as_tm();
}


int DataBlock::findParam(int nid, int after) const
{
  for (int i = after + 1; i < int(params.size()); ++i)
    if (params[i].nameId == nid)
      return i;
  return -1;
}

const DataBlock::Param *DataBlock::getParam(int name_id) const
{
  if (name_id < 0 || name_id >= int(params.size()))
    return 0;
  return &params[name_id];
}

const char *DataBlock::getStr(const char *name, const char *def) const { return getStr(findParam(name), def); }

int DataBlock::getInt(const char *name, int def) const { return getInt(findParam(name), def); }

bool DataBlock::getBool(const char *name, bool def) const { return getBool(findParam(name), def); }

real DataBlock::getReal(const char *name, real def) const { return getReal(findParam(name), def); }

Point2 DataBlock::getPoint2(const char *name, const Point2 &def) const { return getPoint2(findParam(name), def); }

Point3 DataBlock::getPoint3(const char *name, const Point3 &def) const { return getPoint3(findParam(name), def); }

Point4 DataBlock::getPoint4(const char *name, const Point4 &def) const { return getPoint4(findParam(name), def); }

IPoint2 DataBlock::getIPoint2(const char *name, const IPoint2 &def) const { return getIPoint2(findParam(name), def); }

IPoint3 DataBlock::getIPoint3(const char *name, const IPoint3 &def) const { return getIPoint3(findParam(name), def); }

E3DCOLOR DataBlock::getE3dcolor(const char *name, const E3DCOLOR &def) const { return getE3dcolor(findParam(name), def); }

TMatrix DataBlock::getTm(const char *name, const TMatrix &def) const { return getTm(findParam(name), def); }


DataBlock::Param::Param(int id, const std::string &s) : nameId(id), type(DataBlock::ParamType::TYPE_STRING)
{
  new (&data) std::string(s);
}

DataBlock::Param::Param(int id, int i) : nameId(id), type(DataBlock::ParamType::TYPE_INT) { new (&data) int(i); }
DataBlock::Param::Param(int id, real r) : nameId(id), type(DataBlock::ParamType::TYPE_REAL) { new (&data) real(r); }

DataBlock::Param::Param(int id, const Point2 &p2) : nameId(id), type(DataBlock::ParamType::TYPE_POINT2) { new (&data) Point2(p2); }
DataBlock::Param::Param(int id, const Point3 &p3) : nameId(id), type(DataBlock::ParamType::TYPE_POINT3) { new (&data) Point3(p3); }
DataBlock::Param::Param(int id, const Point4 &p4) : nameId(id), type(DataBlock::ParamType::TYPE_POINT4) { new (&data) Point4(p4); }

DataBlock::Param::Param(int id, const IPoint2 &ip2) : nameId(id), type(DataBlock::ParamType::TYPE_IPOINT2)
{
  new (&data) IPoint2(ip2);
}
DataBlock::Param::Param(int id, const IPoint3 &ip3) : nameId(id), type(DataBlock::ParamType::TYPE_IPOINT3)
{
  new (&data) IPoint3(ip3);
}

DataBlock::Param::Param(int id, bool b) : nameId(id), type(DataBlock::ParamType::TYPE_BOOL) { new (&data) bool(b); }

DataBlock::Param::Param(int id, const E3DCOLOR &c) : nameId(id), type(DataBlock::ParamType::TYPE_E3DCOLOR) { new (&data) E3DCOLOR(c); }
DataBlock::Param::Param(int id, const TMatrix &tm) : nameId(id), type(DataBlock::ParamType::TYPE_MATRIX) { new (&data) TMatrix(tm); }


DataBlock::Param::Param(const Param &p)
{
  nameId = p.nameId;
  type = p.type;

  // std::string has a non-trival copy constructor
  if (type == ParamType::TYPE_STRING)
    new (&data) std::string(p.as_c_str());
  else
    memcpy(&data, &p.data, sizeof(data));
}

DataBlock::Param::~Param()
{
  // std::string has a non-trivial destructor
  if (type == ParamType::TYPE_STRING)
    reinterpret_cast<std::string *>(&data)->~basic_string();
}


const char *DataBlock::Param::as_c_str() const
{
  assert(type == ParamType::TYPE_STRING);
  return reinterpret_cast<const std::string *>(&data)->c_str();
}

int DataBlock::Param::as_int() const
{
  assert(type == ParamType::TYPE_INT);
  return *reinterpret_cast<const int *>(&data);
}

real DataBlock::Param::as_real() const
{
  assert(type == ParamType::TYPE_REAL);
  return *reinterpret_cast<const float *>(&data);
}

const Point2 &DataBlock::Param::as_pt2() const
{
  assert(type == ParamType::TYPE_POINT2);
  return *reinterpret_cast<const Point2 *>(&data);
}

const Point3 &DataBlock::Param::as_pt3() const
{
  assert(type == ParamType::TYPE_POINT3);
  return *reinterpret_cast<const Point3 *>(&data);
}

const Point4 &DataBlock::Param::as_pt4() const
{
  assert(type == ParamType::TYPE_POINT4);
  return *reinterpret_cast<const Point4 *>(&data);
}

const IPoint2 &DataBlock::Param::as_ipt2() const
{
  assert(type == ParamType::TYPE_IPOINT2);
  return *reinterpret_cast<const IPoint2 *>(&data);
}

const IPoint3 &DataBlock::Param::as_ipt3() const
{
  assert(type == ParamType::TYPE_IPOINT3);
  return *reinterpret_cast<const IPoint3 *>(&data);
}

bool DataBlock::Param::as_bool() const
{
  assert(type == ParamType::TYPE_BOOL);
  return *reinterpret_cast<const bool *>(&data);
}

const E3DCOLOR &DataBlock::Param::as_color() const
{
  assert(type == ParamType::TYPE_E3DCOLOR);
  return *reinterpret_cast<const E3DCOLOR *>(&data);
}

const TMatrix &DataBlock::Param::as_tm() const
{
  assert(type == ParamType::TYPE_MATRIX);
  return *reinterpret_cast<const TMatrix *>(&data);
}


void DataBlock::Param::set_str(const std::string &s)
{
  assert(type == ParamType::TYPE_STRING);
  *reinterpret_cast<std::string *>(&data) = s;
}

void DataBlock::Param::set_int(int i)
{
  assert(type == ParamType::TYPE_INT);
  *reinterpret_cast<int *>(&data) = i;
}

void DataBlock::Param::set_real(real r)
{
  assert(type == ParamType::TYPE_REAL);
  *reinterpret_cast<real *>(&data) = r;
}

void DataBlock::Param::set_pt2(const Point2 &p2)
{
  assert(type == ParamType::TYPE_POINT2);
  *reinterpret_cast<Point2 *>(&data) = p2;
}

void DataBlock::Param::set_pt3(const Point3 &p3)
{
  assert(type == ParamType::TYPE_POINT3);
  *reinterpret_cast<Point3 *>(&data) = p3;
}

void DataBlock::Param::set_pt4(const Point4 &p4)
{
  assert(type == ParamType::TYPE_POINT4);
  *reinterpret_cast<Point4 *>(&data) = p4;
}

void DataBlock::Param::set_ipt2(const IPoint2 &ip2)
{
  assert(type == ParamType::TYPE_IPOINT2);
  *reinterpret_cast<IPoint2 *>(&data) = ip2;
}

void DataBlock::Param::set_ipt3(const IPoint3 &ip3)
{
  assert(type == ParamType::TYPE_IPOINT3);
  *reinterpret_cast<IPoint3 *>(&data) = ip3;
}

void DataBlock::Param::set_bool(bool b)
{
  assert(type == ParamType::TYPE_BOOL);
  *reinterpret_cast<bool *>(&data) = b;
}

void DataBlock::Param::set_color(const E3DCOLOR &c)
{
  assert(type == ParamType::TYPE_E3DCOLOR);
  *reinterpret_cast<E3DCOLOR *>(&data) = c;
}

void DataBlock::Param::set_tm(const TMatrix &tm)
{
  assert(type == ParamType::TYPE_MATRIX);
  *reinterpret_cast<TMatrix *>(&data) = tm;
}
