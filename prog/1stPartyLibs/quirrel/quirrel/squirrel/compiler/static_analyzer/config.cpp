#include "config.h"
#include "keyValueFile.h"


namespace SQCompilation
{

bool do_report_missing_modules = false;

std::vector<std::string> function_forbidden;
std::vector<std::string> format_function_name;
std::vector<std::string> function_can_return_string;
std::vector<std::string> function_should_return_bool_prefix;
std::vector<std::string> function_should_return_something_prefix;
std::vector<std::string> function_result_must_be_utilized;
std::vector<std::string> function_can_return_null;
std::vector<std::string> function_takes_boolean_lambda;
std::vector<std::string> function_requires_result_from_callback;
std::vector<std::string> function_ignores_callback_result;
std::vector<std::string> function_forbidden_parent_dir;
std::vector<std::string> function_modifies_object;
std::vector<std::string> function_must_be_called_from_root;


void resetAnalyzerConfig() {

  function_forbidden = {

  };

  format_function_name = {
    "prn",
    "print",
    "form",
    "fmt",
    "log",
    "dbg",
    "assert",
  };

  function_can_return_string = {
    "subst",
    "concat",
    "tostring",
    "toupper",
    "tolower",
    "slice",
    "trim",
    "join",
    "format",
    "replace"
  };

  function_should_return_bool_prefix = {
    "has",
    "Has",
    "have",
    "Have",
    "should",
    "Should",
    "need",
    "Need",
    "is",
    "Is",
    "was",
    "Was",
    "will",
    "Will",
    "contains",
    "match",
    "startswith",
    "endswith"
  };

  function_should_return_something_prefix = {
    "get",
    "Get"
  };

  function_result_must_be_utilized = {
    "__merge",
    "indexof",
    "findindex",
    "findvalue",
    "len",
    "reduce",
    "tostring",
    "tointeger",
    "tofloat",
    "slice",
    "tolower",
    "toupper"
  };

  function_can_return_null = {
    "indexof",
    "findindex",
    "findvalue",
    "require_optional"
  };

  function_takes_boolean_lambda = {
    "findvalue",
    "findindex",
    "filter"
  };

  function_ignores_callback_result = {
    "each",
    "mutate",
  };

  function_requires_result_from_callback = {
    "filter",
    "findindex",
    "findvalue",
    "map",
    "reduce",
    "sort",
    "apply",
    "modify",
  };

  function_forbidden_parent_dir = {
    "require",
    "require_optional",
  };

  function_modifies_object = {
    "extend",
    "append",
    "__update",
    "insert",
    "apply",
    "clear",
    "sort",
    "reverse",
    "resize",
    "rawdelete",
    "rawset",
    "remove"
  };

  function_must_be_called_from_root = {
    "keepref"
  };
}

bool loadAnalyzerConfigFile(const char *configFile) {
  KeyValueFile config;
  if (!config.loadFromFile(configFile)) {
    return false;
  }

  return loadAnalyzerConfigFile(config);
}

bool loadAnalyzerConfigFile(const KeyValueFile &config) {
  do_report_missing_modules = config.getBool("report_missing_modules", false);

  for (auto && v : config.getValuesList("forbidden_function"))
    function_forbidden.push_back(v);

  for (auto && v : config.getValuesList("function_can_return_null"))
    function_can_return_null.push_back(v);

  for (auto && v : config.getValuesList("function_result_must_be_utilized"))
    function_result_must_be_utilized.push_back(v);

  for (auto && v : config.getValuesList("function_can_return_string"))
    function_can_return_string.push_back(v);

  for (auto && v : config.getValuesList("function_should_return_bool_prefix"))
    function_should_return_bool_prefix.push_back(v);

  for (auto && v : config.getValuesList("function_should_return_something_prefix"))
    function_should_return_something_prefix.push_back(v);

  for (auto && v : config.getValuesList("function_forbidden_parent_dir"))
    function_forbidden_parent_dir.push_back(v);

  for (auto && v : config.getValuesList("function_modifies_object"))
    function_modifies_object.push_back(v);

  for (auto && v : config.getValuesList("function_must_be_called_from_root"))
    function_must_be_called_from_root.push_back(v);

  for (auto && v : config.getValuesList("function_requires_result_from_callback"))
    function_requires_result_from_callback.push_back(v);

  for (auto && v : config.getValuesList("function_ignores_callback_result"))
    function_ignores_callback_result.push_back(v);

  return true;
}

}
