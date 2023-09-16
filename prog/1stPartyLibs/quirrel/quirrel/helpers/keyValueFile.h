#pragma once

#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include <streambuf>


//file example:
//
//  ; comment
//  include ../some_file.inc
//  boolean_value = yes   ; comment
//  code = fn1(); fn2();  ; comment
//
//  ; use '\' for concatination with the next line
//  multiline = line1 \
//              line1 \  ; valid comment
//              line1 \
//              line1
//
//  list = value1  ; call getValuesList("list") to get all these values in std::vector
//  list = value2
//  list = value3  value4 value5
//  list = value6
//
//

class KeyValueFile
{
public:
  typedef void (*PrintErrorFunc)(const char *);
  PrintErrorFunc printErrorFunc = nullptr;

  struct StringKeyValue
  {
    std::string key;
    std::string value;
  };

private:
  std::string fileName;
  std::vector<StringKeyValue> kv;
  int includeDepth = 0;

  bool isUtf8Bom(const char * it)
  {
    return (
      (unsigned char)(*it++) == 0xef &&
      (unsigned char)(*it++) == 0xbb &&
      (unsigned char)(*it)   == 0xbf
    );
  }

  bool isSpace(char c) const
  {
    return c == ' ' || c == '\t';
  }

  std::string trimStr(std::string s)
  {
    int from = 0;
    int to = int(s.length()) - 1;
    while (to > 0 && isSpace(s[to]))
      to--;
    while (from <= to && isSpace(s[from]))
      from++;
    if (to < from)
      return std::string("");
    return std::string(&(s[from]), to - from + 1);
  }

  void errorParam(const char * msg, const char * key_name) const
  {
    if (printErrorFunc)
    {
      std::string err = std::string("ERROR: ") + msg + ", at file '" + fileName +  "', key '" + std::string(key_name) + "'\n";
      printErrorFunc(err.c_str());
    }
  }

  bool includeFile(const char * inc_file_name, int include_depth)
  {
    if (include_depth >= 16)
    {
      if (printErrorFunc)
        printErrorFunc(
          (std::string("ERROR: Maximum include depth exceeded on file '") + inc_file_name + "'\n").c_str());
      return false;
    }

    int len = int(fileName.length()) - 1;
    while (len >= 0)
    {
      if (fileName[len] == '\\' || fileName[len] == '/')
        break;
      len--;
    }

    std::string combinedFileName(fileName.c_str(), len + 1);
    combinedFileName += inc_file_name;

    KeyValueFile kvFile;
    kvFile.includeDepth = include_depth + 1;
    kvFile.printErrorFunc = printErrorFunc;

    bool ok = kvFile.loadFromFile(combinedFileName.c_str());

    if (ok)
      for (auto && value : kvFile.kv)
        kv.push_back(value);

    return ok;
  }

public:

  bool loadFromFile(const char * file_name)
  {
    if (!file_name)
      return false;

    std::ifstream stream(file_name);
    if (stream.fail())
    {
      if (printErrorFunc)
        printErrorFunc(
          (std::string("ERROR: Cannot open file '") + file_name + "'\n").c_str());
      return false;
    }

    std::string buf((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    return loadFromString(buf.c_str(), file_name);
  }


  bool loadFromString(const char * data, const char * debug_file_name = "")
  {
    fileName = debug_file_name;

    if (!data)
      return false;

    if (isUtf8Bom(data))
      data += 3;
    int len = int(strlen(data));

    StringKeyValue x;
    bool comment = false;
    bool readKey = true;
    bool eqFound = false;
    bool error = false;
    bool include = false;
    int line = 1;

    for (int i = 0; i <= len; i++)
    {
      char c = data[i];

      if (c == '\\')
      {
        if (i == len - 1)
          i++;
        else
        {
          bool commentAfterConcat = false;
          for (int j = i + 1; j < len; j++)
          {
            if (data[j] == ';' && isSpace(data[j - 1]))
              commentAfterConcat = true;

            if (!commentAfterConcat && data[j] > ' ')
              break;

            bool concat = false;
            if (data[j] == 0x0d && data[j + 1] == 0x0a)
            {
              line++;
              i = j + 2;
              concat = true;
            }
            else if (data[j] == 0x0a)
            {
              line++;
              i = j + 1;
              concat = true;
            }

            if (concat)
            {
              if (comment)
                if (printErrorFunc)
                  printErrorFunc((std::string("ERROR: Line concatenation inside comment. At file '") +
                    fileName + "' line " + std::to_string(line - 1) + "\n").c_str());

              if (data[i] > ' ' && data[i] != ';')
                if (printErrorFunc)
                  printErrorFunc((
                    std::string("ERROR: The next line after the line concatenation symbol '\\' must be started with space. At file '") +
                    fileName + "' line " + std::to_string(line) + "\n").c_str());

              c = data[i];
              break;
            }
          }
        }
      }

      if (c == 0x0d || c == 0x0a || c == 0)
      {
        x.key = trimStr(x.key);
        x.value = trimStr(x.value);
        if (!x.key.empty() && !x.value.empty() && eqFound && !include)
        {
          //printf("%s = \"%s\"\n", x.key.c_str(), x.value.c_str());
          kv.push_back(x);
        }
        else if (!x.key.empty())
        {
          if (include)
          {
            if (!debug_file_name || !debug_file_name[0])
            {
              if (printErrorFunc)
                printErrorFunc(
                  (std::string("ERROR: File name must be passed to loadFromString() when you are using 'include'. At file '") +
                    fileName + "' line " + std::to_string(line) + "\n").c_str());
              error = true;
            }
            else
            {
              bool ok = includeFile(x.value.c_str(), includeDepth);
              error |= !ok;
            }
          }
          else if (!eqFound)
          {
            if (printErrorFunc)
              printErrorFunc(
                (std::string("ERROR: Expected '=' at file '") + fileName + "' line " + std::to_string(line) + "\n").c_str());
            error = true;
          }
          else if (x.value.empty())
          {
            if (printErrorFunc)
              printErrorFunc(
                (std::string("ERROR: Expected value after '=' at file '") + fileName + "' line " + std::to_string(line) + "\n").c_str());
            error = true;
          }
        }

        comment = false;
        readKey = true;
        eqFound = false;
        include = false;
        x.key.clear();
        x.value.clear();

        if (c == 0x0a)
          line++;
      }

      if (c == ';' && (i == 0 || isSpace(data[i - 1]) || data[i - 1] == 0x0a))
        comment = true;

      if (comment)
        continue;

      if (readKey)
      {
        if (!isSpace(c) && c > ' ' && c != '=')
          x.key += c;
        else if (!x.key.empty())
        {
          readKey = false;
          if (c == '=')
            eqFound = true;

          if (x.key == "include")
            include = true;
        }
      }
      else
      {
        if (c == '=' && !eqFound)
          eqFound = true;
        else if ((eqFound && c >= ' ') || include)
          x.value += c;
      }
    }

    return !error;
  }


  int count() const
  {
    return int(kv.size());
  }


  const char * getStr(const char * key, const char * default_value = "") const
  {
    if (!key)
      return default_value;

    const char *res = default_value;

    for (int i = 0; i < int(kv.size()); i++)
      if (!strcmp(key, kv[i].key.c_str()))
      {
        if (res != default_value)
          errorParam("More than one key found", key);

        res = kv[i].value.c_str();
      }

    return res;
  }


  bool getBool(const char * key, bool default_value) const
  {
    if (!key)
      return default_value;

    for (int i = 0; i < int(kv.size()); i++)
      if (!strcmp(key, kv[i].key.c_str()))
      {
        if (kv[i].value.empty())
          errorParam("Value is empty", key);
        const char * v = kv[i].value.c_str();
        bool isTrue = !strcmp(v, "1") || !strncmp(v, "yes", 3) || !strncmp(v, "true", 4);
        bool isFalse = !strcmp(v, "0") || !strncmp(v, "no", 2) || !strncmp(v, "false", 5);
        if (!isTrue && !isFalse)
          errorParam("Invalid boolean value, expected yes,true,1 or no,false,0", key);

        return isTrue;
      }

    return default_value;
  }


  std::vector<std::string> getValuesList(const char * key) const // separated by spaces
  {
    std::vector<std::string> res;

    for (auto && x : kv)
      if (!strcmp(key, x.key.c_str()))
      {
        const char * s = x.value.c_str();
        int len = int(x.value.length());
        int i = 0;
        while (i < len)
        {
          while (i < len && (isSpace(s[i]) || s[i] < ' '))
            i++;

          int wordStart = i;

          while (i < len && s[i] > ' ')
            i++;

          if (i != wordStart)
            res.push_back(std::string(s + wordStart, i - wordStart));
        }
      }

    return res;
  }


  const StringKeyValue & keyValue(int index) const
  {
    return kv[index];
  }


  const std::string getFileName() const
  {
    return fileName;
  }
};
