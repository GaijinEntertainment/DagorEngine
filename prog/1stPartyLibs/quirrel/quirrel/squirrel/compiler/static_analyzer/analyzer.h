#pragma once

#include "compiler/ast.h"
#include "compiler/compilationcontext.h"

namespace SQCompilation {

class StaticAnalyzer {

  SQCompilationContext &_ctx;

public:

  StaticAnalyzer(SQCompilationContext &ctx);

  void runAnalysis(RootBlock *r, const HSQOBJECT *bindings);

  static void mergeKnownBindings(const HSQOBJECT *bindings);
  static void reportGlobalNamesWarnings(HSQUIRRELVM vm);

  static void checkTrailingWhitespaces(HSQUIRRELVM vm, const char *sn, const char *code, size_t codeSize);
};

}
