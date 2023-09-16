#ifdef _MSC_VER
// fopen is safe enough for this case
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include "module_exports.h"

#include <map>
#include <algorithm>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <process.h>
#  define NULL_FILE_NAME "nul"
#  define popen _popen
#  define pclose _pclose
#else
#  include <unistd.h>
#  define NULL_FILE_NAME "/dev/null"
#endif

using namespace std;

namespace moduleexports
{
  string csq_exe = "csq";
  string csq_args = "";

  static int tmp_cnt = 0;

  //  module_file_name ("" = root), identifier, parents
  map< string, map<string, vector<string> > > module_content; // module content
  map< string, map<string, vector<string> > > module_to_root; // root table for each module


  //  identifier, parents
  map<string, vector<string> > root;

  const char * dump_sorted_module_code =
    #include "dump_sorted_module.nut.inl"
    ""
    ;

  bool module_export_collector(CompilationContext & ctx, int line, int col, const char * module_name) // nullptr for roottable
  {
    string moduleNameKey = ctx.fileDir + "#";
    moduleNameKey += module_name ? module_name : "";

    auto foundModuleRoot = module_to_root.find(moduleNameKey);
    if (foundModuleRoot != module_to_root.end())
    {
      if (!module_name)
        root = foundModuleRoot->second;
      else
        root.insert(foundModuleRoot->second.begin(), foundModuleRoot->second.end());

      return true;
    }

    if (module_name && strchr(module_name, '\"'))
    {
      ctx.error(71, "Export collector: Invalid module name.", line, col);
      return false;
    }

    char uniq[16] = { 0 };

#if defined(_WIN32)
    snprintf(uniq, sizeof(uniq), "%d", _getpid());
#else
    snprintf(uniq, sizeof(uniq), "%d", getpid());
#endif

    char nutFileName[512] = { 0 };
    char outputFileName[512] = { 0 };
    snprintf(nutFileName, sizeof(nutFileName), "%s%s~nut%s.%d.tmp", ctx.fileDir.c_str(),
      ctx.fileDir.empty() ? "" : "/", uniq, tmp_cnt);
    snprintf(outputFileName, sizeof(outputFileName), "~out%s.%d.tmp", uniq, tmp_cnt);
    tmp_cnt++;

    //tmpnam(nutFileName);
    //tmpnam(outputFileName);

    FILE * fnut = fopen(nutFileName, "wt");
    if (!fnut)
    {
      CompilationContext::setErrorLevel(ERRORLEVEL_FATAL);
      ctx.error(70, "Export collector: Cannot create temporary file.", line, col);
      return false;
    }

    if (module_name)
      fprintf(fnut, "local function dump_table() { return require(\"%s\"); }\n", module_name);
    else
      fprintf(fnut, "local function dump_table() { return {} }\n");

    fprintf(fnut, "%s", dump_sorted_module_code);
    fclose(fnut);

    if (system((csq_exe + " \"" + nutFileName + "\" " + csq_args + " > " + outputFileName).c_str()))
    {
      if (system((csq_exe + " --version > " NULL_FILE_NAME).c_str()))
      {
        CompilationContext::setErrorLevel(ERRORLEVEL_FATAL);
        ctx.error(72, string("Export collector: '" + csq_exe +
          "' not found. You may disable warnings -w242 and -w246 to continue.").c_str(),
          line, col);
        remove(outputFileName);
        remove(nutFileName);
        return false;
      }

      ctx.error(73, "Export collector: code of module executed with errors:\n", line, col);
      system((csq_exe + " --dont-print-table " + nutFileName + " " + csq_args).c_str());
      remove(outputFileName);
      remove(nutFileName);
      return false;
    }

    remove(nutFileName);

    FILE * fout = fopen(outputFileName, "rt");
    if (!fout)
    {
      CompilationContext::setErrorLevel(ERRORLEVEL_FATAL);
      ctx.error(73, "Export collector: cannot open results of module execution.", line, col);
      return false;
    }


    map<string, vector<string> > moduleContent;
    map<string, vector<string> > moduleRoot;

    string emptyString = string();

    char buffer[255] = { 0 };
    bool isError = false;
    while (fgets(buffer, sizeof(buffer) - 1, fout) != NULL)
    {
      bool isRoot = !strncmp(buffer, ".R. ", 4) && !module_name;
      bool isAddRoot = !strncmp(buffer, ".A. ", 4) && module_name;
      bool isModule = !strncmp(buffer, ".M. ", 4) && module_name;
      isError |= !strncmp(buffer, ".E. ", 4);

      map<string, vector<string> > & addTo = isModule ? moduleContent : moduleRoot;

      if (isRoot || isAddRoot || isModule)
      {
        const char * s = buffer + 4 - 1;
        vector<string> names;
        do
        {
          s++;
          const char * dot = strchr(s, '.');
          if (dot)
            names.push_back(string(s, dot - s));
          else
          {
            const char * nline = strchr(s, '\n');
            if (!nline)
              names.push_back(string(s));
            else
              names.push_back(string(s, nline - s));
          }
          s = dot;
        } while (s);


        string & parent = names[0];
        string & child = names.size() > 1 ? names[1] : emptyString;

        auto it = addTo.find(parent);
        if (it == addTo.end())
        {
          vector<string> str;
          str.push_back(child);
          addTo.insert(make_pair(parent, str));
        }
        else
        {
          if (find(it->second.begin(), it->second.end(), child) == it->second.end())
            it->second.push_back(child);
        }
      }
    }

    if (!module_name)
      root = moduleRoot;
    else
      root.insert(moduleRoot.begin(), moduleRoot.end());

    module_content.insert(make_pair(moduleNameKey, moduleContent));
    module_to_root.insert(make_pair(moduleNameKey, moduleRoot));

    fclose(fout);
    remove(outputFileName);

    if (isError && moduleContent.empty() && moduleRoot.empty())
    {
      CompilationContext::setErrorLevel(ERRORLEVEL_FATAL);
      ctx.error(74, (string("Export collector: failed to require '") +
        (module_name ? module_name : "<null>") + "'.").c_str(), line, col);
      return false;
    }

    return true;
  }


  bool is_identifier_present_in_root(const char * name)
  {
    if (!name || !name[0])
      return false;

    auto it = root.find(name);
    return (it != root.end());
  }


  bool is_identifier_present_in_module(const char * module_name, const char * ident_name)
  {
    if (!module_name || !module_name[0] || !ident_name || !ident_name[0])
      return false;

    auto it = module_content.find(module_name);
    if (it == module_content.end())
      return false;

    auto inModuleIt = it->second.find(ident_name);
    if (inModuleIt == it->second.end())
      return false;

    return true;
  }


  bool get_module_export(const char * module_name, vector<string> & out_exports)
  {
    out_exports.clear();

    if (!module_name)
      return false;

    auto it = module_content.find(module_name);
    if (it == module_content.end())
      return false;

    for (auto && e : it->second)
      out_exports.push_back(e.first);

    return true;
  }

  vector<string> all_required_files_in_order_of_execution;
  unordered_set<string> all_required_files;
  unordered_map<string, vector<ExportedIdentifier>> all_exported_identifiers;
  static unordered_set<string> list_of_requested_module_names;
  static unordered_set<string> list_of_absolute_module_names;
  //static unordered_map<string, unordered_map<string, ExportedIdentifier>> exported_identifiers_by_module;
  unordered_map<string, string> requested_name_to_absolute_name;

  // absolute name -> list of exported identifiers
  unordered_map<string, vector<ExportedIdentifier>> all_exported_identifiers_by_module;

  vector<ExportedIdentifier> all_exported_to_root_const_from_modules;

  void add_required_file(const string & current_file_name)
  {
    if (all_required_files.find(current_file_name) == all_required_files.end())
    {
      all_required_files.insert(current_file_name);
      all_required_files_in_order_of_execution.push_back(current_file_name);
    }
  }

  string get_absolute_name(const char* requested_name)
  {
    auto it = requested_name_to_absolute_name.find(requested_name);
    if (it != requested_name_to_absolute_name.end())
      return it->second;

    // try to find by file name without path
    const char * n = requested_name;
    n = max(n, strrchr(n, '\\') + 1);
    n = max(n, strrchr(n, '/') + 1);
    int len = strlen(n);

    for (auto & it : requested_name_to_absolute_name)
    {
      const char * requested = it.first.c_str();
      requested = max(requested, strrchr(requested, '\\') + 1);
      requested = max(requested, strrchr(requested, '/') + 1);

      if (!strcmp(requested, n))
        return it.second;
    }

    return string();
  }

  static unsigned string_to_type(const char * s)
  {
    if (!strcmp(s, "function"))
      return RT_CLOSURE;
    else if (!strcmp(s, "native"))
      return RT_CLOSURE;
    else if (!strcmp(s, "table"))
      return RT_TABLE;
    else if (!strcmp(s, "array"))
      return RT_ARRAY;
    else if (!strcmp(s, "class"))
      return RT_CLASS;
    else if (!strcmp(s, "instance"))
      return RT_UNRECOGNIZED;
    else if (!strcmp(s, "null"))
      return RT_NULL;
    else if (!strcmp(s, "integer"))
      return RT_NUMBER;
    else if (!strcmp(s, "float"))
      return RT_NUMBER;
    else if (!strcmp(s, "bool"))
      return RT_BOOL;
    else if (!strcmp(s, "string"))
      return RT_STRING;
    else
      return RT_UNRECOGNIZED;
  }

  static void split_by_space(char * str, vector<const char *> & result)
  {
    result.clear();
    char * s = str;
    while (s && *s)
    {
      while (*s && isspace(*s))
        s++;
      if (!*s)
        break;
      result.push_back(s);
      while (*s && !isspace(*s))
        s++;
      if (!*s)
        break;
      *s = 0;
      s++;
    }
  }

  static bool gather_single_file(FILE * f, const char * exe_string, bool & read_something)
  {
    string buf;
    buf.resize(20000);
    bool insideModuleContent = false;
    bool insideRoot = false;
    bool insideConst = false;
    const char * currentRequestedModuleName = nullptr;
    const char * currentAbsoluteModuleName = nullptr;

    while (fgets(&buf[0], buf.size() - 2, f) != NULL)
    {
      char * newLine = strchr(&buf[0], '\n');
      if (newLine)
        *newLine = 0;

      if (!strcmp(&buf[0], "### BEGIN EXPORT CONTENT"))
      {
        insideModuleContent = true;
        read_something = true;
        continue;
      }

      if (!strcmp(&buf[0], "### END EXPORT CONTENT"))
      {
        insideModuleContent = false;
        continue;
      }

      if (!insideModuleContent)
        continue;

      if (strncmp(&buf[0], "### SCRIPT MODULE ", 18) == 0)
      {
        insideRoot = false;
        insideConst = false;

        vector <const char *> names;
        split_by_space(&buf[18], names); // 0 - requested name, 1 - resolved name, 2 - absolute name

        requested_name_to_absolute_name.insert({ names[0], names[2] });

        auto result = list_of_requested_module_names.insert(names[0]);
        currentRequestedModuleName = result.first->c_str();
        auto it = list_of_absolute_module_names.insert(names[2]);
        currentAbsoluteModuleName = it.first->c_str();
        continue;
      }

      if (strncmp(&buf[0], "### NATIVE MODULE ", 18) == 0)
      {
        insideRoot = false;
        insideConst = false;
        string n = &buf[18];

        requested_name_to_absolute_name.insert({n, n});

        auto result = list_of_requested_module_names.insert(n);
        currentRequestedModuleName = result.first->c_str();
        auto it = list_of_absolute_module_names.insert(n);
        currentAbsoluteModuleName = it.first->c_str();
        continue;
      }

      if (!strcmp(&buf[0], "### MODULE EXPORT TO ROOT"))
      {
        insideRoot = true;
        insideConst = false;
        continue;
      }

      if (!strcmp(&buf[0], "### MODULE EXPORT TO CONST"))
      {
        insideRoot = false;
        insideConst = true;
        continue;
      }

      if (!strcmp(&buf[0], "### INITIAL CONST"))
      {
        insideRoot = false;
        insideConst = true;
        currentRequestedModuleName = list_of_requested_module_names.find("const table")->c_str();
        currentAbsoluteModuleName = list_of_absolute_module_names.find("const table")->c_str();
        continue;
      }

      if (!strcmp(&buf[0], "### INITIAL ROOT"))
      {
        insideRoot = true;
        insideConst = false;
        currentRequestedModuleName = list_of_requested_module_names.find("root table")->c_str();
        currentAbsoluteModuleName = list_of_absolute_module_names.find("root table")->c_str();
        continue;
      }

      if (strncmp(&buf[0], "### PROCESS FILE ", 17) == 0)
      {
        const char * name = &buf[17];
        add_required_file(name);
        continue;
      }

      if (buf[0] == '#')
      {
        CompilationContext::globalError((string("Export collector: Invalid line: '") + &buf[0] +
          "' in result of execution of: '" + exe_string + "'").c_str());
        return false;
      }

      vector<const char *> tokens;
      split_by_space(&buf[0], tokens);

      if (tokens.size() < 2)
      {
        CompilationContext::globalError((string("Export collector: Invalid line: '") + &buf[0] +
          "' in result of execution of: '" + exe_string + "'").c_str());
        return false;
      }

      ExportedIdentifier ident;
      ident.name = tokens[0];
      ident.isRoot = insideRoot;
      ident.isConst = insideConst;
      ident.requestedModuleName = currentRequestedModuleName;
      ident.absoluteModuleName = currentAbsoluteModuleName;
      ident.typeMask = string_to_type(tokens[1]);
      if (ident.typeMask == RT_CLOSURE)
      {
        ident.minParams = atoi(tokens[2]);
        ident.maxParams = atoi(tokens[3]);
        for (size_t i = 4; i < tokens.size(); i++)
        {
          string s = tokens[i];
          minimize_name(s);
          if (s.empty())
            ident.params.push_back(tokens[i]);
          else
            ident.params.push_back(s);
        }
      }
      else
      {
        for (size_t i = 2; i < tokens.size(); i++)
          ident.params.push_back(tokens[i]);
      }

      {
        auto it = all_exported_identifiers.find(tokens[0]);
        if (it == all_exported_identifiers.end())
          it = all_exported_identifiers.insert(make_pair(tokens[0], vector<ExportedIdentifier>())).first;
        it->second.push_back(ident);
      }


      auto it = all_exported_identifiers_by_module.find(currentAbsoluteModuleName);
      if (it == all_exported_identifiers_by_module.end())
        it = all_exported_identifiers_by_module.insert(make_pair(currentAbsoluteModuleName, vector<ExportedIdentifier>())).first;
      it->second.push_back(ident);

      if (ident.isConst || ident.isRoot)
        all_exported_to_root_const_from_modules.push_back(ident);
    }

    return true;
  }

  bool gather_identifiers_from_root_files(const std::vector<std::string> & root_files,
    const std::vector<std::string> & output_files)
  {
    all_exported_to_root_const_from_modules.clear();
    all_exported_identifiers.clear();
    list_of_absolute_module_names.clear();
    list_of_requested_module_names.clear();
    list_of_absolute_module_names.insert("root table");
    list_of_absolute_module_names.insert("const table");
    list_of_requested_module_names.insert("root table");
    list_of_requested_module_names.insert("const table");

    for (const string & s : root_files)
    {
      string exeString = csq_exe + string(" --export-modules-content ") + csq_args + " " + s;
      FILE * f = popen(exeString.c_str(), "r");
      if (!f)
      {
        CompilationContext::globalError((string("Export collector: Failed to execute: ") + exeString).c_str());
        system(exeString.c_str());
        return false;
      }

      bool readSomething = false;
      bool res = gather_single_file(f, exeString.c_str(), readSomething);

      pclose(f);

      if (!readSomething)
      {
        CompilationContext::globalError((string("Export collector: Failed to execute: ") + exeString).c_str());
        system(exeString.c_str());
        return false;
      }

      if (!res)
        return false;
    }

    for (const string & s : output_files)
    {
      FILE * f = fopen(s.c_str(), "r");
      if (!f)
      {
        CompilationContext::globalError((string("Export collector: Failed to open file: ") + s).c_str());
        return false;
      }

      bool readSomething = false;
      bool res = gather_single_file(f, s.c_str(), readSomething);

      fclose(f);
    }

    return true;
  }


  void minimize_name(std::string & s)
  {
    int len = s.length();
    int to = 0;

    for (int from = 0; from <= len; from++)
      if (s[from] != '_')
      {
        s[to] = std::tolower(s[from]);
        to++;
      }
  }


} // namespace
