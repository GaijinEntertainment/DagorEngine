#pragma once

#include <string>
#include <vector>

class KeyValueFile;

namespace SQCompilation
{

extern bool do_report_missing_modules;

extern std::vector<std::string> function_can_return_string;
extern std::vector<std::string> function_should_return_bool_prefix;
extern std::vector<std::string> function_should_return_something_prefix;
extern std::vector<std::string> function_result_must_be_utilized;
extern std::vector<std::string> function_can_return_null;
extern std::vector<std::string> function_takes_boolean_lambda;
extern std::vector<std::string> function_requires_result_from_callback;
extern std::vector<std::string> function_ignores_callback_result;
extern std::vector<std::string> format_function_name;
extern std::vector<std::string> function_forbidden;
extern std::vector<std::string> function_forbidden_parent_dir;
extern std::vector<std::string> function_modifies_object;
extern std::vector<std::string> function_must_be_called_from_root;

void resetAnalyzerConfig();
bool loadAnalyzerConfigFile(const KeyValueFile &configBlk);
bool loadAnalyzerConfigFile(const char *configFile);

}
