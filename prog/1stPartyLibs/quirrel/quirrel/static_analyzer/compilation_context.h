#pragma once

#include <set>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <stdio.h>
#include "helpers/importParser/importParser.h"
#include "quirrel_parser.h"

bool is_utf8_bom(const char * ptr, int i);

extern FILE * out_stream;

struct Token;

enum OutputMode
{
  OM_1_LINE, // 1 line per warning
  OM_2_LINES,
  OM_FULL,
};

enum ErrorLevels
{
  ERRORLEVEL_WARNING = 2,
  ERRORLEVEL_ERROR = 3,
  ERRORLEVEL_FATAL = 4,
};

enum SQLangFeature
{
  LF_STRICT_BOOL = 0x01,
  LF_NO_PLUS_CONCAT = 0x10,
  LF_FORBID_ROOT_TABLE = 0x40,
  LF_TOOLS_COMPILE_CHECK = 0x1000,
  LF_DISABLE_OPTIMIZER = 0x2000,

  LF_STRICT = LF_STRICT_BOOL |
              LF_NO_PLUS_CONCAT |
              LF_FORBID_ROOT_TABLE
};


struct CompilerMessage
{
  int line;
  int column;
  int intId;
  const char * textId;
  std::string message;
  std::string fileName;
  bool isError;

  CompilerMessage()
  {
    line = 0;
    column = 0;
    intId = 0;
    textId = "";
    isError = false;
  }
};

class CompilationContext
{
  std::vector<int> suppressWarnings;
  static std::set<std::string> shownMessages;
  static int errorLevel;

public:

  NodeList nodeList;
  std::unordered_set<std::string> stringList;
  std::string fileName;
  std::string fileDir;
  std::string absoluteFileName;
  std::string code;
  std::vector<int> shownWarningsAndErrors;
  std::vector<sqimportparser::ModuleImport> imports;
  static std::vector<CompilerMessage> compilerMessages;
  static const char * redirectMessagesToJson;
  static void setErrorLevel(int error_level);
  static int getErrorLevel();
  static void clearErrorLevel();
  int firstLineAfterImport;
  bool isError;
  bool isWarning;
  bool showUnchangedLocalVar;
  static bool justParse;
  static bool includeComments;
  unsigned langFeatures;
  static unsigned defaultLangFeatures;
  std::unordered_map<const Token *, bool> unchangedVar;
  OutputMode outputMode;

  CompilationContext();
  ~CompilationContext();
  void setFileName(const std::string & file_name);
  void getNearestStrings(int line_num, std::string & nearest_strings, std::string & cur_string) const;
  void error(int error_code, const char * error, int line, int col);
  static void globalError(const char * error);
  void warning(const char * text_id, int line, int col);
  void warning(const char * text_id, const Token & tok, const char * arg0 = "???", const char * arg1 = "???",
    const char * arg2 = "???", const char * arg3 = "???");
  void offsetToLineAndCol(int offset, int & line, int & col) const;
  void clearSuppressedWarnings();
  void suppressWaring(int intId);
  void suppressWaring(const char * textId);
  bool isWarningSuppressed(const char * text_id);
  void inverseWarningsSuppression();
  static void printAllWarningsList();
};

