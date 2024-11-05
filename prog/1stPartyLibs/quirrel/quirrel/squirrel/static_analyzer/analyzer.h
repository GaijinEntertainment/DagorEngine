#pragma once

#include "../sqast.h"
#include "../sqcompilationcontext.h"

namespace SQCompilation {

class StaticAnalyzer {

  SQCompilationContext &_ctx;

public:

  StaticAnalyzer(SQCompilationContext &ctx);

  void runAnalysis(RootBlock *r, const HSQOBJECT *bindings);

  static void mergeKnownBindings(const HSQOBJECT *bindings);
  static void reportGlobalNamesWarnings(HSQUIRRELVM vm);

  static void checkTrailingWhitespaces(HSQUIRRELVM vm, const SQChar *sn, const SQChar *code, size_t codeSize);
};

}
