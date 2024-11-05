// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <windows.h>

using namespace std;


// trim from start (in place)
static inline void ltrim(std::string &s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string &s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s)
{
  ltrim(s);
  rtrim(s);
}

inline bool ends_with(std::string const &value, std::string const &ending)
{
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool replace_string(string &subject, const string &search, const string &replace, vector<char> &next_symbols, bool is_prefix)
{
  bool anyReplace = false;
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != string::npos)
  {
    bool symbolFound = is_prefix;
    for (char sym : next_symbols)
      if (subject[pos + search.length()] == sym)
        symbolFound = true;

    if (!symbolFound)
    {
      pos++;
      continue;
    }

    anyReplace = true;
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
  return anyReplace;
}

bool replace_all(string &data, const string &toSearch, const string &replaceStr)
{
  bool wasReplaced = false;
  size_t pos = data.find(toSearch);
  while (pos != string::npos)
  {
    wasReplaced = true;
    data.replace(pos, toSearch.size(), replaceStr);
    pos = data.find(toSearch, pos + replaceStr.size());
  }

  return wasReplaced;
}


void read_file_lines(const char *file_name, vector<string> &lines)
{
  lines.clear();

  ifstream file(file_name);
  if (!file.is_open())
  {
    printf("[E] cannot open file '%s'\n", file_name);
    exit(1);
  }

  std::string line;
  while (getline(file, line))
    lines.push_back(line);

  file.close();
}

void write_lines_to_file(const char *file_name, const vector<string> &lines)
{
  FILE *f = fopen(file_name, "wt");
  if (!f)
  {
    printf("[E] cannot open file '%s' for write.\n", file_name);
    exit(1);
  }

  for (const string &s : lines)
    fprintf(f, "%s\n", s.c_str());

  fclose(f);
}

string get_temp_file_name()
{
  static int cnt = 0;
  char buf[MAX_PATH] = {0};
  GetTempPathA(MAX_PATH, buf);
  string s = string(buf) + "replacer" + to_string(cnt++) + ".tmp";
  remove(s.c_str());
  return s;
}


bool find_in_file_fast(const char *file_name, const char *substring)
{
  FILE *f = fopen(file_name, "rb");
  if (!f)
  {
    printf("[E] cannot open file '%s'.\n", file_name);
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = (char *)malloc(fsize + 1);
  fread(buf, 1, fsize, f);
  fclose(f);
  buf[fsize] = 0;

  int sublength = strlen(substring);
  bool found = false;

  for (int i = 0; i < fsize - sublength; i++)
  {
    if (buf[i] == substring[0])
      if (strncmp(buf + i, substring, sublength) == 0)
      {
        found = true;
        break;
      }
  }

  free(buf);

  return found;
}

void execute(const string &cmd)
{
  int res = system(cmd.c_str());
  if (res)
  {
    printf("[E] failed to execute '%s'\n", cmd.c_str());
    exit(1);
  }
}


void usage()
{
  printf("Usage: replacer <file_extension> <replace_in_directory> <replace_pairs.txt> [--remove-spaces]\n");
  printf("\n--remove-spaces - remove spaces in all text fields (slow)\n\n");
  printf("Example: replacer .dag D:\\dagor2\\enlisted\\develop\\assets replace_pairs.txt\n"); // FIXME_BROKEN_DEP
  printf("         and file \"replace_pairs.txt\" is something like this:\n");
  printf("is_african_pattern_atlas_tex_n.tga europe_pattern_tex_n.tga\n");
  printf("is_african_tex_n.tga europe_tex_n.tga\n");
  printf("is_african_* europe_*\n\n");
}


static bool remove_spaces = false;
static bool is_dag_files = false;
static int argc = 0;
static char **argv = nullptr;

int DagorWinMain(bool)
{
  argc = __argc;
  argv = __argv;

  if (argc == 1)
  {
    usage();
    return 0;
  }

  if (argc < 4 || argc > 5)
  {
    usage();
    return 1;
  }

  string ext = argv[1];
  string directory = argv[2];
  string replacePairsFileName = argv[3];

  if (argc == 5)
  {
    if (!strcmp(argv[4], "--remove-spaces"))
      remove_spaces = true;
    else
    {
      printf("[E] Error: unknown option '%s'", argv[4]);
      exit(1);
    }
  }


  vector<pair<string, string>> replacePairs;
  vector<string> pairLines;
  read_file_lines(replacePairsFileName.c_str(), pairLines);
  for (string &p : pairLines)
    if (p.length() > 3)
    {
      size_t pos = p.find(' ');
      bool error = false;
      if (pos == string::npos)
        error = true;
      else if (strchr(p.c_str() + pos + 1, ' '))
        error = true;

      if (error)
      {
        printf("[E] Error in '%s'. Each line must consist 2 names separated by one 'space'.\n"
               "Note: You cannot use 'space' in replace strings, use '{SPACE}' instead.\n",
          replacePairsFileName.c_str());
        exit(1);
      }

      string left(p.c_str(), pos);
      string right(p.c_str() + pos + 1, p.length() - pos - 1);
      trim(left);
      trim(right);
      replace_all(left, string("{SPACE}"), string(" "));
      replace_all(right, string("{SPACE}"), string(" "));
      replacePairs.push_back(make_pair(left, right));
    }


  is_dag_files = !!strstr(ext.c_str(), "dag");

  vector<string> fileNames;
  string tmpList = get_temp_file_name();
  execute(string("dir. /s /b ") + directory + "\\ > " + tmpList);
  read_file_lines(tmpList.c_str(), fileNames);
  remove(tmpList.c_str());

  string temp = get_temp_file_name();

  vector<string> fromList;
  vector<string> toList;
  vector<bool> isPrefix;
  vector<string> prefixes = {" ", "=", "\\", "/", "\""};
  vector<char> allowed_next_symbols = {'.', '\n', '\0', '\"', ' '};
  for (auto &p : replacePairs)
    for (string &prefix : prefixes)
    {
      string a = prefix + p.first;
      string b = prefix + p.second;
      if (a.back() == '*')
      {
        a.resize(a.length() - 1);
        isPrefix.push_back(true);
        if (b.back() == '*')
          b.resize(b.length() - 1);
      }
      else
        isPrefix.push_back(false);


      fromList.push_back(a);
      toList.push_back(b);
    }

  if (!fromList.size() && remove_spaces)
  {
    fromList.push_back(string("/X"));
    toList.push_back(string("/X"));
  }


  int processed = 0;
  int total = 0;
  int changed = 0;
  for (const string &fn : fileNames)
    if (strstr(fn.c_str(), "\\.") == nullptr && ends_with(fn, ext))
      total++;


  for (const string &fn : fileNames)
  {
    if (strstr(fn.c_str(), "\\.") != nullptr || !ends_with(fn, ext))
      continue;

    processed++;
    if (processed % 1024 == 1023)
      printf("=== processed %d%%\n", processed * 100 / (total + 1));


    bool anyReplaceInFile = false;

    bool found = remove_spaces;
    if (!found)
      for (size_t i = 0; i < fromList.size(); i += prefixes.size())
        if (find_in_file_fast(fn.c_str(), fromList[i].c_str() + 1))
        {
          found = true;
          break;
        }

    if (!found)
      continue;


    // for each file:
    if (!fromList.empty())
    {
      vector<string> fileLines;

      if (is_dag_files)
      {
        execute(string("mat_remap-dev -q " + fn + " " + temp));
        read_file_lines(temp.c_str(), fileLines);
      }
      else // text file
      {
        read_file_lines(fn.c_str(), fileLines);
      }


      bool replaced = false;
      for (auto &s : fileLines)
      {
        for (size_t k = 0; k < fromList.size(); k++)
          replaced |= replace_string(s, fromList[k], toList[k], allowed_next_symbols, isPrefix[k]);

        if (remove_spaces)
          replaced |= replace_all(s, string(" "), string(""));
      }

      if (replaced)
      {
        anyReplaceInFile = true;
        if (is_dag_files)
        {
          write_lines_to_file(temp.c_str(), fileLines);
          execute(string("mat_remap-dev -q " + fn + " " + temp));
          remove(temp.c_str());
        }
        else // text file
        {
          write_lines_to_file(fn.c_str(), fileLines);
        }

        printf("=== replaced in '%s'\n\n\n", fn.c_str());
      }
      else
      {
        if (is_dag_files)
        {
          remove(temp.c_str());
        }
      }
    }

    if (anyReplaceInFile)
      changed++;
  }


  // rename files

  for (auto &replacePair : replacePairs)
    for (const string &fn : fileNames)
    {
      const char *n = strstr(fn.c_str(), replacePair.first.c_str());
      if (n && n > fn.c_str() && !strchr(n, '\\') && strstr(fn.c_str(), "\\.") == nullptr)
      {
        char nextSymbol = n[replacePair.first.length()];
        if (n[-1] == '\\' && (!nextSymbol || nextSymbol == '.'))
        {
          string maskFrom = string(fn.c_str(), (n - fn.c_str()) + replacePair.first.length());
          string suffix = nextSymbol == '.' ? ".*" : "";
          string cmd = string("rename ") + maskFrom + suffix + " " + replacePair.second + suffix + " 2>nul";
          system(cmd.c_str());
        }
      }
    }

  if (total > 0)
    printf("=== processed 100%% (%d files), %d files changed\n\n", total, changed);

  return 0;
}
