
#include <assert.h>
#include <memory.h>
#include <sqconfig.h>
#include "compilationcontext.h"
#include "ast.h"
#include "sqstring.h"
#include "keyValueFile.h"
#include <sq_char_class.h>

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

static const char severityPrefixes[] = "hw";
static const char *severityNames[] = {
  "HINT", "WARNING"
};

static DiagnosticDescriptor diagnosticDescriptors[] = {
#define DEF_DIAGNOSTIC(_, severity, subsytem, num_id, text_id, fmt) { fmt, DS_##severity, DSS_##subsytem, num_id, text_id, false }
  DIAGNOSTICS
#undef DEF_DIAGNOSTIC
};

SQCompilationContext::SQCompilationContext(SQVM *vm, Arena *arena, const char *sn, const char *code, size_t csize, const Comments *comments, bool raiseError)
  : _vm(vm)
  , _arena(arena)
  , _sourceName(sn)
  , _linemap(_ss(vm)->_alloc_ctx)
  , _comments(comments)
  , _code(code)
  , _codeSize(csize)
  , _raiseError(raiseError)
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
    fprintf(ostream, "w%d (%s)\n", diag.id, diag.textId);
    fprintf(ostream, diag.format, "***", "***", "***", "***", "***", "***", "***", "***");
    fprintf(ostream, "\n\n");
  }
}

bool SQCompilationContext::enableWarning(const char *diagName, bool state) {
  for (auto &diag : diagnosticDescriptors) {
    if (strcmp(diagName, diag.textId) == 0) {
      diag.disabled = !state;
      return true;
    }
  }
  return false;
}

bool SQCompilationContext::enableWarning(int32_t id, bool state) {
  for (auto &diag : diagnosticDescriptors) {
    if (id == diag.id) {
      diag.disabled = !state;
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
    if (!sq_isspace(*l)) return false;
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

void SQCompilationContext::buildSourceWindow(int32_t line, int32_t pos, int32_t width, std::string &extraInfo) {
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
    drawUnderliner(pos, width, extraInfo);
  }

  if (l3 != nullptr && !isBlankLine(l3)) {
    extraInfo.push_back('\n');
    int32_t j = 0;
    while ((l3 + j <= lastSourceCodeChar) && l3[j] && l3[j] != '\n' && l3[j] != '\r') { //-V522
      extraInfo.push_back(l3[j++]); //-V595
    }
  }

  if (l1 || l2 || l3) {
    extraInfo.push_back('\n');
    extraInfo.push_back('\n'); // separate with extra line
  }
}

void SQCompilationContext::emitDiagnostic(const char *msg, int32_t line, int32_t pos, int32_t width,
                                          int32_t intId, const char *textId, SQMessageSeverity severity) {
  std::string extraInfo;
  buildSourceWindow(line, pos, width, extraInfo);
  const char *extra = extraInfo.empty() ? nullptr : extraInfo.c_str();

  auto diagMsgFunc = _ss(_vm)->_compilerdiaghandler;
  if (diagMsgFunc) {
    SQCompilerMessage cm;
    cm.intId = intId;
    cm.textId = textId;
    cm.line = line;
    cm.column = pos;
    cm.columnsWidth = width;
    cm.message = msg;
    cm.fileName = _sourceName;
    cm.isError = (severity == SEV_ERROR);
    diagMsgFunc(_vm, &cm);
  }

  auto messageFunc = _ss(_vm)->_compilererrorhandler;
  if (_raiseError && messageFunc) {
    messageFunc(_vm, severity, msg, _sourceName, line, pos, extra);
  }
}

void SQCompilationContext::vreportDiagnostic(enum DiagnosticsId diagId, int32_t line, int32_t pos, int32_t width, va_list vargs) {
  assert(diagId < DI_NUM_OF_DIAGNOSTICS);

  if (isDisabled(diagId, line, pos))
    return;

  auto &desc = diagnosticDescriptors[diagId];

  std::string message;
  vrenderDiagnosticHeader(diagId, message, vargs);

  SQMessageSeverity sev = (desc.severity == DS_HINT) ? SEV_HINT : SEV_WARNING;
  emitDiagnostic(message.c_str(), line, pos, width, desc.id, desc.textId, sev);
}

void SQCompilationContext::reportDiagnostic(enum DiagnosticsId diagId, int32_t line, int32_t pos, int32_t width, ...) {
  va_list vargs;
  va_start(vargs, width);
  vreportDiagnostic(diagId, line, pos, width, vargs);
  va_end(vargs);
}

void SQCompilationContext::vthrowError(int32_t line, int32_t pos, int32_t width, const char *fmt, va_list vargs) {
  std::string message;
  message.append("ERROR: ");

  char tempBuffer[2048] = { 0 };
  vsnprintf(tempBuffer, sizeof tempBuffer, fmt, vargs);
  message.append(tempBuffer);

  emitDiagnostic(message.c_str(), line, pos, width, -1, "", SEV_ERROR);

  _vm->_lasterror = SQString::Create(_ss(_vm), message.c_str(), message.length());
  throw CompilerError();
}

void SQCompilationContext::throwError(int32_t line, int32_t pos, int32_t width, const char *fmt, ...) {
  va_list vargs;
  va_start(vargs, fmt);
  vthrowError(line, pos, width, fmt, vargs);
  va_end(vargs); // unreachable; vthrowError throws
}

void SQCompilationContext::throwError(const char *fmt, ...) {
  va_list vargs;
  va_start(vargs, fmt);
  vthrowError(-1, -1, 0, fmt, vargs);
  va_end(vargs); // unreachable; vthrowError throws
}

void SQCompilationContext::throwError(const Node *n, const char *fmt, ...) {
  va_list vargs;
  va_start(vargs, fmt);
  vthrowError(n->lineStart(), n->columnStart(), n->textWidth(), fmt, vargs);
  va_end(vargs); // unreachable; vthrowError throws
}

void SQCompilationContext::reportDiagnostic(enum DiagnosticsId diagId, const Node *n, ...) {
  va_list vargs;
  va_start(vargs, n);
  vreportDiagnostic(diagId, n->lineStart(), n->columnStart(), n->textWidth(), vargs);
  va_end(vargs);
}

} // namespace SQCompilation
