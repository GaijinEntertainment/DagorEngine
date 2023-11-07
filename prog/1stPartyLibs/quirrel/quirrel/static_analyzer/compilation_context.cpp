#include "compilation_context.h"
#include <string.h>
#include <algorithm>

#ifdef _WIN32
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#else
#  include <unistd.h>
#  include <linux/limits.h>
#endif


using namespace std;

FILE * out_stream = stdout;

std::set<std::string> CompilationContext::shownMessages;
std::vector<CompilerMessage> CompilationContext::compilerMessages;
const char * CompilationContext::redirectMessagesToJson = nullptr;
int CompilationContext::errorLevel = 0;
bool CompilationContext::justParse = false;
bool CompilationContext::includeComments = false;


struct AnalyzerMessage
{
  int intId;
  const char * textId;
  const char * messageText;
};

AnalyzerMessage analyzer_messages[] =
{
  {
    190, "paren-is-function-call",
    "'(' on a new line parsed as function call."
  },
  {
    192, "statement-on-same-line",
    "Next statement on the same line after '%s' statement."
  },
  {
    200, "potentially-nulled-cmp",
    "Comparison with potentially nullable expression."
  },
  {
    201, "potentially-nulled-arith",
    "Arithmetic operation with potentially nullable expression."
  },
  {
    202, "and-or-paren",
    "Priority of the '&&' operator is higher than that of the '||' operator. Perhaps parentheses are missing?"
  },
  {
    203, "bitwise-bool-paren",
    "Result of bitwise operation used in boolean expression. Perhaps parentheses are missing?"
  },
  {
    204, "bitwise-apply-to-bool",
    "The '&' or '|' operator is applied to boolean type. You've probably forgotten to include parentheses "
    "or intended to use the '&&' or '||' operator."
  },
  {
    205, "unreachable-code",
    "Unreachable code after 'return'."
  },
  {
    206, "assigned-twice",
    "Variable is assigned twice successively."
  },
  {
    207, "identical-if-condition",
    "Conditional expressions of 'if' statements situated alongside each other are identical (if (A); if (A);)."
  },
  {
    208, "potentially-nulled-assign",
    "Assignment to potentially nullable expression."
  },
  {
    209, "assigned-to-itself",
    "The variable is assigned to itself."
  },
  {
    210, "potentially-nulled-index",
    "Potentially nullable expression used as array index."
  },
  {
    211, "duplicate-case",
    "Duplicate case value."
  },
  {
    212, "duplicate-if-expression",
    "Detected pattern 'if (A) {...} else if (A) {...}'. Branch unreachable."
  },
  {
    213, "then-and-else-equals",
    "'then' statement is equivalent to 'else' statement."
  },
  {
    214, "operator-returns-same-val",
    "Both branches of operator '?:' are equivalent."
  },
  {
    215, "ternary-priority",
    "The '?:' operator has lower priority than the '%s' operator. Perhaps the '?:' operator "
    "works in a different way than it was expected."
  },
  {
    216, "same-operands",
    "Left and right operands of '%s' operator are the same."
  },
  {
    217, "unconditional-return-loop",
    "Unconditional 'return' inside a loop."
  },
  {
    218, "unconditional-continue-loop",
    "Unconditional 'continue' inside a loop."
  },
  {
    219, "unconditional-break-loop",
    "Unconditional 'break' inside a loop."
  },
  {
    220, "potentially-nulled-container",
    "'foreach' on potentially nullable expression."
  },
  {
    221, "result-not-utilized",
    "Result of operation is not used."
  },
  {
    222, "bool-as-index",
    "Boolean used as array index."
  },
  {
    223, "compared-with-bool",
    "Comparison with boolean."
  },
  {
    224, "empty-while-loop",
    "'while' operator has an empty body."
  },
  {
    225, "all-paths-return-value",
    "Not all control paths return a value."
  },
  {
    226, "return-different-types",
    "Function can return different types."
  },
  {
    227, "ident-hides-ident",
    "%s '%s' hides %s with the same name."
  },
  {
    228, "declared-never-used",
    "%s '%s' was declared but never used."
  },
  {
    229, "copy-of-expression",
    "Duplicate expression found inside the sequance of operations."
  },
  {
    230, "trying-to-modify",
    "Trying to modify %s, '%s'."
  },
  {
    231, "format-arguments-count",
    "Format string: arguments count mismatch."
  },
  {
    232, "always-true-or-false",
    "Expression is always '%s'."
  },
  {
    233, "const-in-bool-expr",
    "Constant in a boolean expression."
  },
  {
    234, "div-by-zero",
    "Integer division by zero."
  },
  {
    235, "round-to-int",
    "Result of division will be integer."
  },
  {
    236, "shift-priority",
    "Shift operator has lower priority. Perhaps parentheses are missing?"
  },
  {
    237, "assigned-never-used",
    "%s '%s' was assigned but never used."
  },
  {
    238, "named-like-should-return",
    "Function name '%s' implies a return value, but its result is never used."
  },
  {
    239, "named-like-return-bool",
    "Function name '%s' implies a return boolean type but not all control paths returns boolean."
  },
  {
    240, "null-coalescing-priority",
    "The '??""' operator has a lower priority than the '%s' operator (a??b > c == a??""(b > c)). "
    "Perhaps the '??""' operator works in a different way than it was expected."
  },
  {
    241, "already-required",
    "Module '%s' has been required already."
  },
  {
    242, "undefined-variable",
    "Local variable '%s' is undefined."
  },
  {
    243, "ident-hides-std-function",
    "%s '%s' hides %s with the same name."
  },
  {
    244, "used-from-static",
    "Non-static class member '%s' used inside static function."
  },
  {
    245, "unknown-identifier",
    "Unknown identifier '%s'."
  },
  {
    246, "never-declared",
    "Identifier '%s' was never declared or assigned."
  },
  {
    247, "func-can-return-null",
    "Function '%s' can return null, but its result is used here."
  },
  {
    248, "call-potentially-nulled",
    "'%s' can be null, but is used as a function without checking."
  },
  {
    249, "access-potentially-nulled",
    "'%s' can be null, but is used as a container without checking."
  },
  {
    250, "cmp-with-array",
    "Comparison with an array."
  },
  {
    251, "cmp-with-table",
    "Comparison with a table."
  },
  {
    252, "undefined-const",
    "Constant '%s' is undefined."
  },
  {
    253, "const-never-declared",
    "Constant '%s' was never declared."
  },
  {
    254, "bool-passed-to-in",
    "Boolean passed to 'in' operator."
  },
  {
    255, "duplicate-function",
    "Duplicate function body. Consider functions '%s' and '%s'."
  },
  {
    256, "key-and-function-name",
    "Key and function name are not the same ('%s' and '%s')."
  },
  {
    257, "duplicate-assigned-expr",
    "Duplicate of the assigned expression."
  },
  {
    258, "similar-function",
    "Function bodies are very similar. Consider functions '%s' and '%s'."
  },
  {
    259, "similar-assigned-expr",
    "Assigned expression is very similar to one of the previous ones."
  },
  {
    260, "named-like-must-return-result",
    "Function '%s' has name like it should return a value, but not all control paths returns a value."
  },
  {
    261, "conditional-local-var",
    "Local variable declaration in a conditional statement."
  },
  {
    262, "suspicious-formatting",
    "Suspicious code formatting. Consider lines: %s, %s."
  },
  {
    263, "egyptian-braces",
    "Indentation style: 'egyptian braces' required."
  },
  {
    264, "plus-string",
    "Usage of '+' for string concatenation."
  },
  {
    265, "single-statement-function",
    "A single-statement function is not desirable."
  },
  {
    266, "forgotten-do",
    "'while' after the statement list (forgot 'do' ?)"
  },
  {
    267, "parsed-function-call",
    "'(' will be parsed as function call (forgot ',' ?)"
  },
  {
    268, "parsed-access-member",
    "'[' will be parsed as 'access to member' (forgot ',' ?)"
  },
  {
    269, "mixed-separators",
    "Mixed spaces and commas to separate %s."
  },
  {
    270, "extent-to-append",
    "Hint: it is better to use 'append(A, B, ...)' instead of 'extend([A, B, ...])'."
  },
  {
    271, "forgot-subst",
    "'{}' found inside string (forgot 'subst' or '$' ?)"
  },
  {
    272, "not-unary-op",
    "This '%s' is not unary operator. Please use ' ' after it or ',' before it for better understandability."
  },
  {
    273, "global-var-creation",
    "Creation of the global variable requires '::' before the name of the variable."
  },
  {
    274, "iterator-in-lambda",
    "Iterator '%s' is trying to be captured in lambda-function."
  },
  {
    275, "missed-break",
    "A 'break' statement is probably missing in a 'switch' statement."
  },
  {
    276, "empty-then",
    "'then' has empty body."
  },
  {
    277, "space-at-eol",
    "Whitespace at the end of line."
  },
  {
    278, "forbidden-function",
    "It is forbidden to call '%s' function."
  },
  {
    279, "mismatch-loop-variable",
    "The variable used in for-loop does not match the initialized one."
  },
  {
    280, "forbidden-parent-dir",
    "Access to the parent directory is forbidden in this function."
  },
  {
    281, "unwanted-modification",
    "Function '%s' modifies object. You probably didn't want to modify the object here."
  },
  {
    282, "inexpr-assign-priority",
    "Operator ':=' has lower priority. Perhaps parentheses are missing?"
  },
  {
    283, "useless-null-coalescing",
    "The expression to the right of the '??""' is null."
  },
  {
    284, "can-be-simplified",
    "Expression can be simplified."
  },
  {
    285, "expr-cannot-be-null",
    "The expression to the left of the '%s' cannot be null."
  },
  {
    286, "func-in-expression",
    "Function used in expression."
  },
  {
    287, "range-check",
    "It looks like the range boundaries are not checked correctly. "
    "Pay attention to checking with minimum and maximum index."
  },
  {
    288, "param-count",
    "Function '%s' is called with the wrong number of parameters. Consider the function definition in %s %s"
  },
  {
    289, "param-pos",
    "The function parameter '%s' seems to be in the wrong position. Consider the function definition in %s %s"
  },
  {
    290, "unused-func-param",
    "Unused %s '%s'. You can mark unused identifiers with a prefix underscore '_%s'."
  },
  {
    291, "invalid-underscore",
    "The name of %s '%s' is invalid. The identifier is marked as an unused with a prefix underscore, but it is used."
  },
  {
    292, "modified-container",
    "The container was modified within the loop."
  },
  {
    293, "duplicate-persist-id",
    "Duplicate id '%s' passed to 'persist'."
  },
  {
    294, "conditional-const",
    "A global constant can be defined only in file root scope."
  },
  {
    295, "undefined-global",
    "Undefined global identifier '%s'."
  },
  {
    296, "access-without-this",
    "Access to class/table member '%s' without 'this'."
  },
  {
    297, "call-from-root",
    "Function '%s' must be called from the root scope."
  },
  {
    298, "lambda-param-hides-ident",
    "Parameter of lambda hides identifier."
  },
  {
    299, "in-instead-contains",
    "Probably the 'in' operator is used instead of 'contains' method."
  },
  {
    300, "param-hides-param",
    "%s '%s' hides %s with the same name."
  },
};


unsigned CompilationContext::defaultLangFeatures = 0;

CompilationContext::CompilationContext()
{
  isError = false;
  isWarning = false;
  outputMode = OM_FULL;
  firstLineAfterImport = 0;
  showUnchangedLocalVar = false;
  langFeatures = defaultLangFeatures;
}


CompilationContext::~CompilationContext()
{
}


static string get_absolute_file_name(const string & file_name)
{
#ifdef _WIN32
  char buf[MAX_PATH] = { 0 };
  if (GetFullPathNameA(file_name.c_str(), MAX_PATH, buf, NULL) == 0)
    return file_name;
  return buf;
#else
  char buf[PATH_MAX] = { 0 };
  if (realpath(file_name.c_str(), buf) == NULL)
    return file_name;
  return buf;
#endif
}


void CompilationContext::setFileName(const string & file_name)
{
  absoluteFileName = get_absolute_file_name(file_name);
  fileName = file_name;
  fileDir = fileName;
  size_t foundDirDelim = fileDir.find_last_of("/\\");
  if (foundDirDelim != std::string::npos)
    fileDir = fileDir.substr(0, foundDirDelim);
  else
    fileDir.clear();
}

void CompilationContext::getNearestStrings(int line_num, std::string & nearest_strings, std::string & cur_string) const
{
  int line = 1;
  int start = is_utf8_bom(code.c_str(), 0) ? 3 : 0;

  for (int i = start; i < int(code.length()); i++)
  {
    if (code[i] == 0x0d && code[i + 1] == 0x0a)
      i++;

    if (code[i] == 0x0d || code[i] == 0x0a)
      line++;

    if (line >= line_num - 1)
    {
      nearest_strings += code[i];
      if (line == line_num)
        cur_string += code[i];
    }

    if (line > line_num + 1)
      break;
  }
}


static const char * only_file_name_and_ext(const char * path)
{
  const char * s1 = strrchr(path, '/');
  const char * s2 = strrchr(path, '\\');
  if (s2 > s1)
    s1 = s2;

  return s1 ? s1 + 1 : path;
}


void CompilationContext::error(int error_code, const char * error, int line, int col)
{
  if (isError)
    return;

  setErrorLevel(ERRORLEVEL_ERROR);

  isError = true;

  string hash = std::to_string(error_code) + std::to_string(line) + "_" + std::to_string(col) +
    "_" + only_file_name_and_ext(fileName.c_str());
  if (shownMessages.find(hash) != shownMessages.end())
    return;

  shownMessages.insert(hash);
  std::string nearestStrings, curString;
  getNearestStrings(line, nearestStrings, curString);

  if (!redirectMessagesToJson)
  {
    if (outputMode == OM_1_LINE)
      fprintf(out_stream, "ERR: e%d %s  %s:%d:%d\n", error_code, error, only_file_name_and_ext(fileName.c_str()), line, col);
    else if (outputMode == OM_2_LINES)
      fprintf(out_stream, "ERROR: e%d %s\n  %s:%d:%d\n", error_code, error, fileName.c_str(), line, col);
    else
      fprintf(out_stream, "ERROR: e%d %s\nat %s:%d:%d\n%s\n\n\n", error_code, error, fileName.c_str(), line, col, nearestStrings.c_str());
  }
  shownWarningsAndErrors.push_back(error_code);

  CompilerMessage cm;
  cm.line = line;
  cm.column = col;
  cm.intId = error_code;
  cm.isError = true;
  cm.message = error;
  cm.fileName = fileName;
  compilerMessages.push_back(cm);
}


void CompilationContext::globalError(const char * error)
{
  CompilationContext::setErrorLevel(ERRORLEVEL_FATAL);

  if (!redirectMessagesToJson)
    fprintf(out_stream, "ERROR: %s\n", error);

  CompilerMessage cm;
  cm.line = 0;
  cm.column = 0;
  cm.intId = 0;
  cm.isError = true;
  cm.message = error;
  cm.fileName = "";
  compilerMessages.push_back(cm);
}


void CompilationContext::warning(const char * text_id, const Token & tok, const char * arg0, const char * arg1,
  const char * arg2, const char * arg3)
{
  if (isError)
    return;

  int warningCode = -1;
  int found = 0;
  int msgIndex = 0;
  for (int i = 0; i < sizeof(analyzer_messages) / sizeof(analyzer_messages[0]); i++)
    if (!strcmp(text_id, analyzer_messages[i].textId))
    {
      msgIndex = i;
      found++;
      warningCode = analyzer_messages[i].intId;
    }

  if (!found)
  {
    error(20, (string("Internal error: message text id not found: ") + text_id).c_str(), 0, 0);
    return;
  }

  if (found != 1)
  {
    error(21, (string("Internal error: duplicated message text id: ") + text_id).c_str(), 0, 0);
    return;
  }

  for (size_t i = 0; i < suppressWarnings.size(); i++)
    if (warningCode == suppressWarnings[i])
      return;

  char suppressLineIntBuf[64];
  char suppressLineTextBuf[128];
  snprintf(suppressLineIntBuf, sizeof(suppressLineIntBuf), "-w%d", warningCode);
  snprintf(suppressLineTextBuf, sizeof(suppressLineTextBuf), "-%s", text_id);

  char suppressFileIntBuf[64];
  char suppressFileTextBuf[128];
  snprintf(suppressFileIntBuf, sizeof(suppressFileIntBuf), "-file:w%d", warningCode);
  snprintf(suppressFileTextBuf, sizeof(suppressFileTextBuf), "-file:%s", text_id);

  int line = tok.line;
  int col = tok.column;

  if (size_t(arg0) == 1)
  {
    line = int(size_t(arg1));
    col = int(size_t(arg2));
    arg0 = nullptr;
    arg1 = nullptr;
    arg2 = nullptr;
  }

  std::string nearestStrings, curString;
  getNearestStrings(line, nearestStrings, curString);

  if (strstr(curString.c_str(), suppressLineIntBuf) || strstr(curString.c_str(), suppressLineTextBuf))
    return;

  if (strstr(code.c_str(), suppressFileIntBuf) || strstr(code.c_str(), suppressFileTextBuf))
    return;


  string hash = std::string(text_id) + std::to_string(line) + "_" + std::to_string(col) +
    "_" + only_file_name_and_ext(fileName.c_str());
  if (shownMessages.find(hash) != shownMessages.end())
    return;

  shownMessages.insert(hash);

  char warningText[512] = { 0 };
  snprintf(warningText, sizeof(warningText), analyzer_messages[msgIndex].messageText, arg0, arg1, arg2, arg3);

  if (!redirectMessagesToJson)
  {
    if (outputMode == OM_1_LINE)
      fprintf(out_stream, "WARN: %s  %s:%d:%d\n", text_id, only_file_name_and_ext(fileName.c_str()), line, col);
    else if (outputMode == OM_2_LINES)
      fprintf(out_stream, "WARNING: w%d (%s)  %s\n  %s:%d:%d\n", warningCode, text_id, warningText, fileName.c_str(), line, col);
    else
      fprintf(out_stream, "WARNING: w%d (%s)  %s\nat %s:%d:%d\n%s\n\n\n", warningCode, text_id, warningText, fileName.c_str(),
        line, col, nearestStrings.c_str());
  }

  isWarning = true;
  setErrorLevel(ERRORLEVEL_WARNING);
  shownWarningsAndErrors.push_back(warningCode);

  CompilerMessage cm;
  cm.line = line;
  cm.column = col;
  cm.intId = warningCode;
  cm.textId = text_id;
  cm.isError = false;
  cm.message = warningText;
  cm.fileName = fileName;

  compilerMessages.push_back(cm);
}


void CompilationContext::warning(const char * text_id, int line, int col)
{
  static Token emptyToken;
  warning(text_id, emptyToken, (const char *)(1), (const char *)(size_t(line)), (const char *)(size_t(col)));
}


void CompilationContext::offsetToLineAndCol(int offset, int & line, int & col) const
{
  line = 1;
  col = 1;
  int start = is_utf8_bom(code.c_str(), 0) ? 3 : 0;

  for (int i = start; i < int(code.length()); i++)
  {
    if (offset - 1 < i)
      break;

    col++;

    if (code[i] == 0x0d && code[i + 1] == 0x0a)
      i++;

    if (code[i] == 0x0d || code[i] == 0x0a)
    {
      line++;
      col = 1;
    }
  }
}


void CompilationContext::clearSuppressedWarnings()
{
  suppressWarnings.clear();
}


void CompilationContext::suppressWaring(int int_id)
{
  for (size_t i = 0; i < suppressWarnings.size(); i++)
    if (int_id == suppressWarnings[i])
    {
      error(31, (string("Warning is already suppressed: w") + to_string(int_id)).c_str(), 0, 0);
      return;
    }

  suppressWarnings.push_back(int_id);
}


void CompilationContext::suppressWaring(const char * text_id)
{
  for (int i = 0; i < sizeof(analyzer_messages) / sizeof(analyzer_messages[0]); i++)
    if (!strcmp(text_id, analyzer_messages[i].textId))
    {
      suppressWaring(analyzer_messages[i].intId);
      return;
    }

  if (text_id && text_id[0] == 'w')
    text_id++;  // skip first 'w'

  for (int i = 0; i < sizeof(analyzer_messages) / sizeof(analyzer_messages[0]); i++)
    if (!strcmp(text_id, analyzer_messages[i].textId))
    {
      suppressWaring(analyzer_messages[i].intId);
      return;
    }

  error(30, (string("Cannot suppress warning, text id not found: ") + text_id).c_str(), 0, 0);
}


bool CompilationContext::isWarningSuppressed(const char * text_id)
{
  int id = -1;
  for (int i = 0; i < sizeof(analyzer_messages) / sizeof(analyzer_messages[0]); i++)
    if (!strcmp(text_id, analyzer_messages[i].textId))
    {
      id = analyzer_messages[i].intId;
      break;
    }

  return (std::find(suppressWarnings.begin(), suppressWarnings.end(), id) != suppressWarnings.end());
}


void CompilationContext::inverseWarningsSuppression()
{
  std::vector<int> newSuppressWarnings;
  for (int i = 0; i < sizeof(analyzer_messages) / sizeof(analyzer_messages[0]); i++)
  {
    bool found = false;
    for (size_t j = 0; j < suppressWarnings.size(); j++)
    {
      if (analyzer_messages[i].intId == suppressWarnings[j])
      {
        found = true;
        break;
      }
    }

    if (!found)
      newSuppressWarnings.push_back(analyzer_messages[i].intId);
  }

  suppressWarnings = newSuppressWarnings;
}


void CompilationContext::printAllWarningsList()
{
  for (int i = 0; i < sizeof(analyzer_messages) / sizeof(analyzer_messages[0]); i++)
  {
    fprintf(out_stream, "w%d (%s)\n", analyzer_messages[i].intId, analyzer_messages[i].textId);
    fprintf(out_stream, analyzer_messages[i].messageText, "***", "***", "***", "***", "***", "***", "***", "***");
    fprintf(out_stream, "\n\n");
  }
}

void CompilationContext::setErrorLevel(int error_level)
{
  if (error_level > errorLevel)
    errorLevel = error_level;
}

int CompilationContext::getErrorLevel()
{
  return errorLevel;
}

void CompilationContext::clearErrorLevel()
{
  errorLevel = 0;
}


