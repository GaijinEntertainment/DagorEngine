#include "importParser.h"
#include <fstream>
#include <sstream>
#include <stdio.h>

using namespace std;
using namespace sqimportparser;

static string res;

static string readFileToString(const char * path)
{
  ostringstream buf; 
  ifstream input (path); 
  buf << input.rdbuf(); 
  return buf.str();
}

static void error_cb(void * /*user_pointer*/, const char * message, int line, int column)
{
  res += "ERROR: ";
  res += message;
  res += " at line=";
  res += to_string(line);
  res += " column=";
  res += to_string(column);
}

static void remove_spaces(string & s)
{
  while (s.find("\r") != string::npos)
    s.erase(s.find("\r"), 1);
  while (s.find("\n") != string::npos)
    s.erase(s.find("\n"), 1);
  while (s.find(" ") != string::npos)
    s.erase(s.find(" "), 1);
}


int main(int argc, const char ** argv)
{
  if (argc != 2 && argc != 3)
  {
    printf("usage: importParser <test_file.nut> <expected_result.txt>\n");
    return 1;
  }

  string s = readFileToString(argv[1]);
  string expected = (argc == 3) ? readFileToString(argv[2]) : string();

  if (argc == 3 && expected.empty())
  {
    printf("'expected' file is not found or empty\n");
    return 1;
  }

  ImportParser importParser(error_cb, nullptr);
  const char * txt = s.c_str();
  int line = 1;
  int col = 1;
  vector<ModuleImport> modules;
  vector<string> directives;
  vector<pair<const char *, const char *>> keepRanges;


  if (importParser.parse(&txt, line, col, modules, &directives, &keepRanges))
  {
    for (size_t i = 0; i < modules.size(); i++)
    {
      res += modules[i].moduleName + "(" + modules[i].moduleIdentifier + "): ";
      for (size_t j = 0; j < modules[i].slots.size(); j++)
      {
        res += string("id=") + modules[i].slots[j].importAsIdentifier + ", path=";
        for (size_t k = 0; k < modules[i].slots[j].path.size(); k++)
          res += (string(k ? "." : "")) + modules[i].slots[j].path[k];
        res += "; ";
      }
      res += "\n";
    }

    for (size_t i = 0; i < directives.size(); i++)
      res += directives[i] + "\n";

    string expected2 = expected;
    string res2 = res;
    remove_spaces(expected2);
    remove_spaces(res2);

    if (!strcmp(expected2.c_str(), res2.c_str()))
    {
      printf("OK\n");
      return 0;
    }
    else
    {
      printf("FAIL\noutput:\n%s\nexpected:\n%s\n", res.c_str(), expected.c_str());
      return 1;
    }
  }
  else
  {
    if (strstr(expected.c_str(), "error"))
    {
      printf("OK\n");
      return 0;
    }
    else
    {
      printf("FAIL\noutput:\n%s\nexpected:\n%s\n", res.c_str(), expected.c_str());
      return 1;
    }
  }

  return 0;
}
