#include "analyzer.h"
#include <squirrel.h>

#include "sqtable.h"

#include "name_shadowing_checker.h"
#include "operator_classification.h"
#include "checker_visitor.h"
#include "global_state.h"


namespace SQCompilation {


const char *symbolContextName(SymbolKind k) {
  switch (k)
  {
  case SK_EXCEPTION: return "exception";
  case SK_FUNCTION: return "function";
  case SK_CLASS: return "class";
  case SK_TABLE: return "table";
  case SK_VAR: return "variable";
  case SK_BINDING: return "binding";
  case SK_CONST: return "const";
  case SK_ENUM: return "enum";
  case SK_ENUM_CONST: return "enum const";
  case SK_PARAM: return "parameter";
  case SK_FOREACH: return "iteration variable";
  case SK_IMPORT: return "import";
  case SK_SYNTHETIC: return "synthetic";
  default: return "<unknown>";
  }
}

void FunctionInfo::joinModifiable(const FunctionInfo *other) {
  for (auto &m : other->modifiable) {
    if (owner == m.owner)
      continue;

    addModifiable(m.name, m.owner);
  }
}

void FunctionInfo::addModifiable(const char *name, const FunctionExpr *o) {
  for (auto &m : modifiable) {
    if (m.owner == o && strcmp(name, m.name) == 0)
      return;
  }

  modifiable.push_back({ o, name });
}


//================================================================


StaticAnalyzer::StaticAnalyzer(SQCompilationContext &ctx)
  : _ctx(ctx) {
}


void StaticAnalyzer::reportGlobalNamesWarnings(HSQUIRRELVM vm) {
  auto errorFunc = _ss(vm)->_compilererrorhandler;

  if (!errorFunc)
    return;

  // 1. Check multiple declarations

  std::string message;

  for (auto it = declaredGlobals.begin(); it != declaredGlobals.end(); ++it) {
    const char *name = it->first.c_str();
    const auto &declarations = it->second;

    if (declarations.size() == 1)
      continue;

    for (int32_t i = 0; i < declarations.size(); ++i) {
      const IdLocation &loc = declarations[i];
      if (loc.diagSilenced)
        continue;

      message.clear();
      SQCompilationContext::renderDiagnosticHeader(DiagnosticsId::DI_GLOBAL_NAME_REDEF, &message, name);
      errorFunc(vm, SEV_WARNING, message.c_str(), loc.filename, loc.line, loc.column, "\n");
    }
  }

  // 2. Check undefined usages

  for (auto it = usedGlobals.begin(); it != usedGlobals.end(); ++it) {
    const std::string &id = it->first;

    bool isKnownBinding = knownBindings.find(id) != knownBindings.end();
    if (isKnownBinding)
      continue;

    if (declaredGlobals.find(id) != declaredGlobals.end())
      continue;

    const auto &usages = it->second;

    for (int32_t i = 0; i < usages.size(); ++i) {
      const IdLocation &loc = usages[i];
      if (loc.diagSilenced)
        continue;

      message.clear();
      SQCompilationContext::renderDiagnosticHeader(DiagnosticsId::DI_UNDEFINED_GLOBAL, &message, id.c_str());
      errorFunc(vm, SEV_WARNING, message.c_str(), loc.filename, loc.line, loc.column, "\n");
    }
  }
}

static bool isSpaceOrTab(char c) { return c == '\t' || c == ' '; }

void StaticAnalyzer::checkTrailingWhitespaces(HSQUIRRELVM vm, const char *sourceName, const char *code, size_t codeSize) {
  Arena arena(_ss(vm)->_alloc_ctx, "tmp");
  SQCompilationContext ctx(vm, &arena, sourceName, code, codeSize, nullptr, true);

  int32_t line = 1;
  int32_t column = 1;

  for (int32_t idx = 0; idx < codeSize - 1; ++idx, ++column) {
    if (isSpaceOrTab(code[idx])) {
      int next = code[idx + 1];
      if (!next || next == '\n' || next == '\r') {
        ctx.reportDiagnostic(DiagnosticsId::DI_SPACE_AT_EOL, line, column - 1, 1);
      }
    }
    else if (code[idx] == '\n') {
      column = 0;
      line++;
    }
  }
}

void StaticAnalyzer::mergeKnownBindings(const HSQOBJECT *bindings) {
  if (bindings && sq_istable(*bindings)) {
    SQTable *table = _table(*bindings);

    SQInteger idx = 0;
    SQObjectPtr pos(idx), key, val;

    while ((idx = table->Next(false, pos, key, val)) >= 0) {
      if (sq_isstring(key)) {
        SQInteger len = _string(key)->_len;
        const char *s = _string(key)->_val;
        knownBindings.emplace(std::string(s, s+len));
      }
      pos._unVal.nInteger = idx;
    }
  }
}

void StaticAnalyzer::runAnalysis(RootBlock *root, const HSQOBJECT *bindings)
{
  mergeKnownBindings(bindings);
  CheckerVisitor(_ctx).analyze(root, bindings);
  NameShadowingChecker(_ctx, bindings).analyze(root);
}

}
