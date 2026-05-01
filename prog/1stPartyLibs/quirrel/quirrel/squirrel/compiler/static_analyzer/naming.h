#pragma once

#include <string>
#include <algorithm>
#include "compiler/compilationcontext.h"
#include "compiler/ast.h"
#include "config.h"
#include <sq_char_class.h>


namespace SQCompilation
{


inline const char *terminatorOpToName(TreeOp op) {
  switch (op)
  {
  case TO_BREAK: return "break";
  case TO_CONTINUE: return "continue";
  case TO_RETURN: return "return";
  case TO_THROW: return "throw";
  default:
    assert(0);
    return "<unknown terminator>";
  }
}


inline bool isValidId(const char *id) {
  assert(id != nullptr);

  if (!sq_isalpha(id[0]) && id[0] != '_')
    return false;

  for (int i = 1; id[i]; ++i) {
    char c = id[i];
    if (!sq_isalpha(c) && !sq_isdigit(c) && c != '_')
      return false;
  }

  return true;
}


inline bool hasAnyPrefix(const char *str, const std::vector<std::string> &prefixes) {
  for (auto &prefix : prefixes) {
    size_t length = prefix.length();
    bool hasPrefix = strncmp(str, prefix.c_str(), length)==0;
    if (hasPrefix) {
      char c = str[length];
      if (!c || c == '_' || c != tolower(c)) {
        return true;
      }
    }
  }

  return false;
}

inline bool hasAnyEqual(const char *str, const std::vector<std::string> &candidates) {
  for (auto &candidate : candidates) {
    if (strcmp(str, candidate.c_str()) == 0) {
      return true;
    }
  }

  return false;
}

inline bool hasAnySubstring(const char *str, const std::vector<std::string> &candidates) {
  for (auto &candidate : candidates) {
    if (strstr(str, candidate.c_str())) {
      return true;
    }
  }

  return false;
}

inline bool nameLooksLikeResultMustBeBoolean(const char *funcName) {
  if (!funcName)
    return false;

  return hasAnyPrefix(funcName, function_should_return_bool_prefix);
}

inline bool nameLooksLikeFunctionMustReturnResult(const char *funcName) {
  if (!funcName)
    return false;

  bool nameInList = nameLooksLikeResultMustBeBoolean(funcName) ||
    hasAnyPrefix(funcName, function_should_return_something_prefix);

  if (!nameInList)
    if ((strstr(funcName, "_ctor") || strstr(funcName, "Ctor")) && strstr(funcName, "set") != funcName)
      nameInList = true;

  return nameInList;
}

inline bool nameLooksLikeResultMustBeUtilized(const char *name) {
  return hasAnyEqual(name, function_result_must_be_utilized);
}

inline bool nameLooksLikeResultMustBeString(const char *name) {
  return hasAnyEqual(name, function_can_return_string);
}

inline bool canFunctionReturnNull(const char *n) {
  return hasAnyEqual(n, function_can_return_null);
}

inline bool isForbiddenFunctionName(const char *n) {
  return hasAnyEqual(n, function_forbidden);
}

inline bool nameLooksLikeMustBeCalledFromRoot(const char *n) {
  return hasAnyEqual(n, function_must_be_called_from_root);
}

inline bool nameLooksLikeForbiddenParentDir(const char *n) {
  return hasAnyEqual(n, function_forbidden_parent_dir);
}

inline bool nameLooksLikeFormatFunction(const char *n) {
  std::string transformed(n);
  std::transform(transformed.begin(), transformed.end(), transformed.begin(), ::tolower);

  return hasAnySubstring(transformed.c_str(), format_function_name);
}

inline bool nameLooksLikeModifiesObject(const char *n) {
  return hasAnyEqual(n, function_modifies_object);
}

inline bool nameLooksLikeFunctionTakeBooleanLambda(const char *n) {
  return hasAnyEqual(n, function_takes_boolean_lambda);
}

inline bool nameLooksLikeRequiresResultFromCallback(const char *n) {
  return hasAnyEqual(n, function_requires_result_from_callback);
}

inline bool nameLooksLikeIgnoresCallbackResult(const char *n) {
  return hasAnyEqual(n, function_ignores_callback_result);
}

bool looksLikeElementCount(const Expr *e);

bool isUpperCaseIdentifier(const Expr *e);

}
