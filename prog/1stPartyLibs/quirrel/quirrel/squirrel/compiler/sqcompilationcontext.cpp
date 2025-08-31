
#include <assert.h>
#include <memory.h>
#include <sqconfig.h>
#include "sqcompilationcontext.h"
#include "sqstring.h"
#include "keyValueFile.h"

namespace SQCompilation {


enum DiagnosticSubsystem {
  DSS_LEX,
  DSS_SYNTAX,
  DSS_SEMA,
  DSS_COMPILETIME,
};

struct DiagnosticDescriptor {
  const char *format;
  const enum DiagnosticSeverity severity;
  const enum DiagnosticSubsystem subsystem;
  const int32_t id;
  const char *textId;
  bool disabled;
};

static const char severityPrefixes[] = "hwe";
static const char *severityNames[] = {
  "HINT", "WARNING", "ERROR", nullptr
};

static DiagnosticDescriptor diagnosticDescriptors[] = {
#define DEF_DIAGNOSTIC(_, severity, subsytem, num_id, text_id, fmt) { _SC(fmt), DS_##severity, DSS_##subsytem, num_id, _SC(text_id), false }
  DIAGNOSTICS
#undef DEF_DIAGNOSTIC
};

SQCompilationContext::SQCompilationContext(SQVM *vm, Arena *arena, const SQChar *sn, const char *code, size_t csize, const Comments *comments, bool raiseError)
  : _vm(vm)
  , _arena(arena)
  , _sourceName(sn)
  , _linemap(_ss(vm)->_alloc_ctx)
  , _comments(comments)
  , _code(code)
  , _codeSize(csize)
  , _raiseError(raiseError)
  , _errorjmp()
{
}

SQCompilationContext::~SQCompilationContext()
{
}

Comments::~Comments() {
  SQAllocContext ctx = _commentsList._alloc_ctx;

  for (auto &lc : _commentsList) {
    for (auto &c : lc) {
      sq_vm_free(ctx, (void *)c.text, c.size + 1);
    }
  }
}

const Comments::LineCommentsList &Comments::lineComment(int line) const {
  if (_commentsList.empty())
    return emptyComments;

  line -= 1;
  assert(0 <= line && line < _commentsList.size());
  return _commentsList[line];
}

bool SQCompilationContext::do_report_missing_modules = false;

std::vector<std::string> SQCompilationContext::function_forbidden;
std::vector<std::string> SQCompilationContext::format_function_name;
std::vector<std::string> SQCompilationContext::function_can_return_string;
std::vector<std::string> SQCompilationContext::function_should_return_bool_prefix;
std::vector<std::string> SQCompilationContext::function_should_return_something_prefix;
std::vector<std::string> SQCompilationContext::function_result_must_be_utilized;
std::vector<std::string> SQCompilationContext::function_can_return_null;
std::vector<std::string> SQCompilationContext::function_calls_lambda_inplace;
std::vector<std::string> SQCompilationContext::function_takes_boolean_lambda;
std::vector<std::string> SQCompilationContext::function_forbidden_parent_dir;
std::vector<std::string> SQCompilationContext::function_modifies_object;
std::vector<std::string> SQCompilationContext::function_must_be_called_from_root;

std::vector<std::string> SQCompilationContext::std_identifier;
std::vector<std::string> SQCompilationContext::std_function;

void SQCompilationContext::resetConfig() {

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

  function_calls_lambda_inplace = {
    "findvalue",
    "findindex",
    "__update",
    "filter",
    "map",
    "reduce",
    "each",
    "sort",
    "assert",
    "persist",
    "join",
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

  std_identifier = {
    "require",
    "require_optional",
    "vargv",
    "persist",
    "getclass",
    "__name__",
    "__filename__",
    "keepref",
  };

  std_function = {
    "seterrorhandler",
    "setdebughook",
    "getstackinfos",
    "getroottable",
    "getconsttable",
    "getclass",
    "assert",
    "print",
    "error",
    "compilestring",
    "newthread",
    "suspend",
    "array",
    "type",
    "callee",
    "collectgarbage",
    "resurrectunreachable",
    "min",
    "max",
    "clamp",
  };

}

bool SQCompilationContext::loadConfigFile(const char *configFile) {
  KeyValueFile config;
  if (!config.loadFromFile(configFile)) {
    return false;
  }

  return loadConfigFile(config);
}

bool SQCompilationContext::loadConfigFile(const KeyValueFile &config) {
  do_report_missing_modules = config.getBool("report_missing_modules", false);

  //for (auto && v : config.getValuesList("format_function_name"))
  //{
  //  string functionName(v);
  //  std::transform(functionName.begin(), functionName.end(), functionName.begin(), ::tolower);
  //  format_function_name.push_back(functionName);
  //}

  for (auto && v : config.getValuesList("forbidden_function"))
    function_forbidden.push_back(v);

  for (auto && v : config.getValuesList("function_can_return_null"))
    function_can_return_null.push_back(v);

  for (auto && v : config.getValuesList("function_calls_lambda_inplace"))
    function_calls_lambda_inplace.push_back(v);

  for (auto && v : config.getValuesList("std_identifier"))
    std_identifier.push_back(v);

  for (auto && v : config.getValuesList("std_function"))
    std_function.push_back(v);

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

  return true;
}

void SQCompilationContext::buildLineMap() {
  assert(_code != NULL);

  int prev = '\n';

  for (size_t i = 0; i < _codeSize; ++i) {
    if (prev == '\n') {
      _linemap.push_back(i);
    }

    prev = _code[i];
  }
}

const char *SQCompilationContext::findLine(int lineNo) {
  if (_linemap.empty())
    buildLineMap();

  lineNo -= 1;
  if (lineNo < 0 || lineNo >= _linemap.size())
    return nullptr;

  return _code + _linemap[lineNo];
}


void SQCompilationContext::printAllWarnings(FILE *ostream) {
  for (auto &diag : diagnosticDescriptors) {
    if (diag.severity == DS_ERROR)
      continue;
    fprintf(ostream, "w%d (%s)\n", diag.id, diag.textId);
    fprintf(ostream, diag.format, "***", "***", "***", "***", "***", "***", "***", "***");
    fprintf(ostream, "\n\n");
  }
}

bool SQCompilationContext::enableWarning(const char *diagName, bool state) {
  for (auto &diag : diagnosticDescriptors) {
    if (strcmp(diagName, diag.textId) == 0) {
      if (diag.severity != DS_ERROR) {
        diag.disabled = !state;
      }
      return true;
    }
  }
  return false;
}

bool SQCompilationContext::enableWarning(int32_t id, bool state) {
  for (auto &diag : diagnosticDescriptors) {
    if (id == diag.id) {
      if (diag.severity != DS_ERROR) {
        diag.disabled = !state;
      }
      return true;
    }
  }
  return false;
}

void SQCompilationContext::switchSyntaxWarningsState(bool state) {
  for (auto &diag : diagnosticDescriptors) {
    if (diag.severity == DS_WARNING && (diag.subsystem == DSS_SYNTAX || diag.subsystem == DSS_LEX || diag.subsystem == DSS_COMPILETIME)) {
      diag.disabled = !state;
    }
  }
}

bool SQCompilationContext::isRequireDisabled(int line, int col) {
  if (!_comments)
    return false;

  auto &lineCmts = _comments->lineComment(line);
  for (auto &comment : lineCmts) {
    if (strstr(comment.text, "-skip-require"))
      return true;
  }

  return false;
}

bool SQCompilationContext::isDisabled(enum DiagnosticsId id, int line, int pos) {
  DiagnosticDescriptor &descriptor = diagnosticDescriptors[id];
  if (descriptor.severity >= DS_ERROR) return false;

  if (descriptor.disabled) return true;

  if (!_comments)
    return false;

  const Comments::LineCommentsList &lineCmts = _comments->lineComment(line);

  char suppressLineIntBuf[64] = { 0 };
  char suppressLineTextBuf[128] = { 0 };
  snprintf(suppressLineIntBuf, sizeof(suppressLineIntBuf), "-%c%d", severityPrefixes[descriptor.severity], descriptor.id);
  snprintf(suppressLineTextBuf, sizeof(suppressLineTextBuf), "-%s", descriptor.textId);

  for (auto &comment : lineCmts) {
    if (strstr(comment.text, suppressLineIntBuf))
      return true;

    if (strstr(comment.text, suppressLineTextBuf))
      return true;
  }

  char suppressFileIntBuf[64] = { 0 };
  char suppressFileTextBuf[128] = { 0 };
  snprintf(suppressFileIntBuf, sizeof(suppressFileIntBuf), "-file:%c%d", severityPrefixes[descriptor.severity], descriptor.id);
  snprintf(suppressFileTextBuf, sizeof(suppressFileTextBuf), "-file:%s", descriptor.textId);

  for (auto &lineComments : _comments->commentsList()) {
    for (auto &comment : lineComments) {
      if (strstr(comment.text, suppressFileIntBuf))
        return true;

      if (strstr(comment.text, suppressFileTextBuf))
        return true;
    }
  }

  return false;
}

static void drawUnderliner(int32_t column, int32_t width, std::string &msg)
{
  int32_t i = 0;
  while (i < column) {
    msg.push_back(' ');
    ++i;
  }

  ++i;
  msg.push_back('^');

  while ((i - column) < width) {
    msg.push_back('-');
    ++i;
  }
}


bool SQCompilationContext::isBlankLine(const char *l) {
  if (!l) return true;
  while ((l < _code + _codeSize) && *l && *l != '\n') {
    if (!isspace(*l)) return false;
    ++l;
  }
  return true;
}

void SQCompilationContext::renderDiagnosticHeader(enum DiagnosticsId diag, std::string *msg, ...) {
  va_list vargs;
  va_start(vargs, msg);
  vrenderDiagnosticHeader(diag, *msg, vargs);
  va_end(vargs);
}

void SQCompilationContext::vrenderDiagnosticHeader(enum DiagnosticsId diag, std::string &message, va_list vargs) {
  auto &desc = diagnosticDescriptors[diag];
  char tempBuffer[2048] = { 0 };

  snprintf(tempBuffer, sizeof tempBuffer, "%s: ", severityNames[desc.severity]);

  message.append(tempBuffer);

  if (desc.id > 0) {
    snprintf(tempBuffer, sizeof tempBuffer, "%c%d (%s) ", severityPrefixes[desc.severity], desc.id, desc.textId);
    message.append(tempBuffer);
  }

  vsnprintf(tempBuffer, sizeof tempBuffer, desc.format, vargs);

  message.append(tempBuffer);
}

void SQCompilationContext::vreportDiagnostic(enum DiagnosticsId diagId, int32_t line, int32_t pos, int32_t width, va_list vargs) {
  assert(diagId < DI_NUM_OF_DIAGNOSTICS);

  bool doJump = false;

  if (!isDisabled(diagId, line, pos)) {

    auto &desc = diagnosticDescriptors[diagId];
    bool isError = desc.severity >= DS_ERROR;
    std::string message;

    vrenderDiagnosticHeader(diagId, message, vargs);

    std::string extraInfo;

    const char *l1 = findLine(line - 1);
    const char *l2 = findLine(line);
    const char *l3 = findLine(line + 1);
    const char *lastSourceCodeChar = _code + _codeSize - 1;

    if (l1 != nullptr && !isBlankLine(l1)) {
      extraInfo.push_back('\n');
      int32_t j = 0;
      while ((l1 + j <= lastSourceCodeChar) && l1[j] && l1[j] != '\n' && l1[j] != '\r') { //-V522
        extraInfo.push_back(l1[j++]); //-V595
      }
    }

    if (l2 != nullptr) {
      extraInfo.push_back('\n');
      int32_t j = 0;
      while ((l2 + j <= lastSourceCodeChar) && l2[j] && l2[j] != '\n' && l2[j] != '\r') { //-V522
        extraInfo.push_back(l2[j++]); //-V595
      }

      extraInfo.push_back('\n');
      j = 0;

      drawUnderliner(pos, width, extraInfo);
    }

    if (l3 != nullptr && !isBlankLine(l3)) {
      extraInfo.push_back('\n');
      int32_t j = 0;
      while ((l3 + j <= lastSourceCodeChar) && l3[j] && l3[j] != '\n' && l3[j] != '\r') { //-V522
        extraInfo.push_back(l3[j++]); //-V595
      }
    }

    const char *extra = nullptr;
    if (l1 || l2 || l3) {
      extraInfo.push_back('\n');
      extraInfo.push_back('\n'); // separate with extra line
      extra = extraInfo.c_str();
    }

    auto messageFunc = _ss(_vm)->_compilererrorhandler;

    const char *msg = message.c_str();

    auto diagMsgFunc = _ss(_vm)->_compilerdiaghandler;
    if (diagMsgFunc) {
      SQCompilerMessage cm;
      cm.intId = desc.id;
      cm.textId = desc.textId;
      cm.line = line;
      cm.column = pos;
      cm.columnsWidth = width;
      cm.message = msg;
      cm.fileName = _sourceName;
      cm.isError = isError;
      diagMsgFunc(_vm, &cm);
    }

    if (_raiseError && messageFunc) {
      SQMessageSeverity sev = SEV_ERROR;
      if (desc.severity == DS_HINT) sev = SEV_HINT;
      else if (desc.severity == DS_WARNING) sev = SEV_WARNING;
      messageFunc(_vm, sev, msg, _sourceName, line, pos, extra);
    }
    if (isError) {
      _vm->_lasterror = SQString::Create(_ss(_vm), msg, message.length());
      doJump = true;
    }
  }
  if (doJump)
    longjmp(_errorjmp, 1);
}

void SQCompilationContext::reportDiagnostic(enum DiagnosticsId diagId, int32_t line, int32_t pos, int32_t width, ...) {
  va_list vargs;
  va_start(vargs, width);
  vreportDiagnostic(diagId, line, pos, width, vargs);
  va_end(vargs);
}

} // namespace SQCompilation
