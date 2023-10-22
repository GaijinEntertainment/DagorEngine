#ifndef _ANALYSER_H_
#define _ANALYSER_H_ 1

#include "../sqast.h"
#include "../sqcompilationcontext.h"

namespace SQCompilation {

class StaticAnalyser {

  SQCompilationContext &_ctx;

public:

  StaticAnalyser(SQCompilationContext &ctx);

  void runAnalysis(RootBlock *r, const HSQOBJECT *bindings);

  static void reportGlobalNameDiagnostics(HSQUIRRELVM vm);

  static void checkTrailingWhitespaces(HSQUIRRELVM vm, const SQChar *sn, const SQChar *code, size_t codeSize);
};

}

#endif // _ANALYSER_H_
