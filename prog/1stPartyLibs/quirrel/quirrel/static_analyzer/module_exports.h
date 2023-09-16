#pragma once

#include "compilation_context.h"

#include <vector>
#include <string>

enum
{
  RT_NOTHING = 1 << 0,
  RT_NULL = 1 << 1,
  RT_BOOL = 1 << 2,
  RT_NUMBER = 1 << 3,
  RT_STRING = 1 << 4,
  RT_TABLE = 1 << 5,
  RT_ARRAY = 1 << 6,
  RT_CLOSURE = 1 << 7,
  RT_FUNCTION_CALL = 1 << 8,
  RT_UNRECOGNIZED = 1 << 9,
  RT_THROW = 1 << 10,
  RT_CLASS = 1 << 11,
};

namespace moduleexports
{
  extern std::string csq_exe;
  extern std::string csq_args;

  struct ExportedIdentifier
  {
    unsigned typeMask = RT_UNRECOGNIZED;
    int minParams = -1;
    int maxParams = -1;
    const char * requestedModuleName = "";
    const char * absoluteModuleName = "";
    bool isRoot = false;
    bool isConst = false;
    std::vector<std::string> params;
    std::string name;


    ExportedIdentifier() {}

    ExportedIdentifier(const ExportedIdentifier & other)
    {
      typeMask = other.typeMask;
      minParams = other.minParams;
      maxParams = other.maxParams;
      requestedModuleName = other.requestedModuleName;
      absoluteModuleName = other.absoluteModuleName;
      isRoot = other.isRoot;
      isConst = other.isConst;
      params = other.params;
      name = other.name;
    }

    ExportedIdentifier(ExportedIdentifier && other)
    {
      typeMask = other.typeMask;
      minParams = other.minParams;
      maxParams = other.maxParams;
      requestedModuleName = other.requestedModuleName;
      absoluteModuleName = other.absoluteModuleName;
      isRoot = other.isRoot;
      isConst = other.isConst;
      params = std::move(other.params);
      name = std::move(other.name);
    }

    ExportedIdentifier & operator=(const ExportedIdentifier & other)
    {
      typeMask = other.typeMask;
      minParams = other.minParams;
      maxParams = other.maxParams;
      requestedModuleName = other.requestedModuleName;
      absoluteModuleName = other.absoluteModuleName;
      isRoot = other.isRoot;
      isConst = other.isConst;
      params = other.params;
      name = other.name;
      return *this;
    }
  };

  extern std::unordered_map<std::string, std::vector<ExportedIdentifier>> all_exported_identifiers;

  //static unordered_map<string, unordered_map<string, ExportedIdentifier>> exported_identifiers_by_module;
  //extern std::unordered_map<std::string, std::string> requested_name_to_absolute_name;

  // absolute name -> list of exported identifiers
  extern std::unordered_map<std::string, std::vector<ExportedIdentifier>> all_exported_identifiers_by_module;

  extern std::vector<ExportedIdentifier> all_exported_to_root_const_from_modules;
  extern std::vector<std::string> all_required_files_in_order_of_execution;

  bool module_export_collector(CompilationContext & ctx, int line, int col, const char * module_name = nullptr); // nullptr for roottable
  bool is_identifier_present_in_root(const char * name);
  bool get_module_export(const char * module_name, std::vector<std::string> & out_exports);
  std::string get_absolute_name(const char *requested_name);

  bool gather_identifiers_from_root_files(const std::vector<std::string> & root_files, const std::vector<std::string> & output_files);
  void minimize_name(std::string & s);

}
