/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#ifndef NO_COMPILER
#include <stdarg.h>
#include <ctype.h>
#include <setjmp.h>
#include <algorithm>
#include "sqopcodes.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "sqcompiler.h"
#include "sqfuncstate.h"
#include "sqoptimizer.h"
#include "sqlexer.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqast.h"
#include "sqastparser.h"
#include "sqastrender.h"
#include "sqastcodegen.h"
#include "optimizations/closureHoisting.h"
#include "sqbinaryast.h"
#include "sqcompilationcontext.h"
#include "static_analyzer/analyzer.h"

namespace SQCompilation {

static RootBlock *parseASTImpl(Arena *astArena, SQVM *vm, const char *sourceText, size_t sourceTextSize, const SQChar *sourcename, Comments *comments, bool raiseerror) {
  Arena parseArena(_ss(vm)->_alloc_ctx, "Parser");
  SQCompilationContext ctx(vm, &parseArena, sourcename, sourceText, sourceTextSize, comments, raiseerror);
  SQParser p(vm, sourceText, sourceTextSize, sourcename, astArena, ctx, comments);

  RootBlock *r = p.parse();

  if (r) {
    ClosureHoistingOpt opt(_ss(vm), astArena);
    opt.run(r, sourcename);
  }

  return r;
}

SqASTData *ParseToAST(SQVM *vm, const char *sourceText, size_t sourceTextSize, const SQChar *sourcename, bool preserveComments, bool raiseerror) {

  void *arenaPtr = sq_vm_malloc(_ss(vm)->_alloc_ctx, sizeof(Arena));
  Arena *astArena = new (arenaPtr) Arena(_ss(vm)->_alloc_ctx, "AST");
  Comments *comments = nullptr;
  if (preserveComments) {
    void *commentsPtr = sq_vm_malloc(_ss(vm)->_alloc_ctx, sizeof(Comments));
    comments = new (commentsPtr) Comments(_ss(vm)->_alloc_ctx);
  }

  SqASTData *astData = (SqASTData *)sq_vm_malloc(_ss(vm)->_alloc_ctx, sizeof(SqASTData));
  if (sourcename) {
    strncpy(astData->sourceName, sourcename, sizeof(astData->sourceName));
    astData->sourceName[sizeof(astData->sourceName) / sizeof(astData->sourceName[0]) - 1] = 0;
  }
  else
    astData->sourceName[0] = 0;

  RootBlock *r = parseASTImpl(astArena, vm, sourceText, sourceTextSize, astData->sourceName, comments, raiseerror);

  if (!r) {
    if (comments) {
      comments->~Comments();
      sq_vm_free(_ss(vm)->_alloc_ctx, comments, sizeof(Comments));
    }
    astArena->~Arena();
    sq_vm_free(_ss(vm)->_alloc_ctx, astArena, sizeof(Arena));
    sq_vm_free(_ss(vm)->_alloc_ctx, astData, sizeof(SqASTData));
    return nullptr;
  }

  astData->astArena = astArena;
  astData->root = r;
  astData->comments = comments;

  return astData;
}

bool Compile(SQVM *vm, const char *sourceText, size_t sourceTextSize, const HSQOBJECT *bindings, const SQChar *sourcename, SQObjectPtr &out, bool raiseerror, bool lineinfo)
{
    Arena astArena(_ss(vm)->_alloc_ctx, "AST");

    RootBlock *r = parseASTImpl(&astArena, vm, sourceText, sourceTextSize, sourcename, nullptr, raiseerror);

    if (!r)
      return false;

    Arena cgArena(_ss(vm)->_alloc_ctx, "Codegen");
    SQCompilationContext ctx(vm, &cgArena, sourcename, sourceText, sourceTextSize, nullptr, raiseerror);
    CodegenVisitor codegen(&cgArena, bindings, vm, sourcename, ctx, lineinfo);

    return codegen.generate(r, out);
}

static bool TranslateASTToBytecodeImpl(SQVM *vm, RootBlock *ast, const HSQOBJECT *bindings, const SQChar *sourcename, const char *sourceText, size_t sourceTextSize, SQObjectPtr &out, const Comments *comments, bool raiseerror, bool lineinfo)
{
    Arena cgArena(_ss(vm)->_alloc_ctx, "Codegen");
    SQCompilationContext ctx(vm, &cgArena, sourcename, sourceText, sourceTextSize, comments, raiseerror);
    CodegenVisitor codegen(&cgArena, bindings, vm, sourcename, ctx, lineinfo);

    return codegen.generate(ast, out);
}

bool TranslateASTToBytecode(SQVM *vm, SqASTData *astData, const HSQOBJECT *bindings, const char *sourceText, size_t sourceTextSize, SQObjectPtr &out, bool raiseerror, bool lineinfo)
{
    return TranslateASTToBytecodeImpl(vm, astData->root, bindings, astData->sourceName, sourceText, sourceTextSize, out, astData->comments, raiseerror, lineinfo);
}

void AnalyzeCode(SQVM *vm, SqASTData *astData, const HSQOBJECT *bindings, const char *sourceText, size_t sourceTextSize)
{
    Arena saArena(_ss(vm)->_alloc_ctx, "Analyzer");
    SQCompilationContext ctx(vm, &saArena, astData->sourceName, sourceText, sourceTextSize, astData->comments, true);

    RootBlock *ast = astData->root;
    StaticAnalyzer sa(ctx);

    sa.runAnalysis(ast, bindings);
}

bool TranslateBinaryASTToBytecode(SQVM *vm, const uint8_t *buffer, size_t size, const HSQOBJECT *bindings, SQObjectPtr &out, bool raiseerror, bool lineinfo)
{
    Arena astArena(_ss(vm)->_alloc_ctx, "AST");

    MemoryInputStream mis(buffer, size);
    SQASTReader reader(_ss(vm)->_alloc_ctx, vm, &astArena, &mis, raiseerror);
    const SQChar *sourcename = NULL;
    RootBlock *r = reader.readAst(sourcename);

    if (!r) {
      return false;
    }

    return TranslateASTToBytecodeImpl(vm, r, bindings, sourcename, NULL, 0, out, nullptr, raiseerror, lineinfo);
}

bool ParseAndSaveBinaryAST(SQVM *vm, const char *sourceText, size_t sourceTextSize, const SQChar *sourcename, OutputStream *ostream, bool raiseerror) {
    Arena astArena(_ss(vm)->_alloc_ctx, "AST");
    Comments comments(_ss(vm)->_alloc_ctx);

    RootBlock *r = parseASTImpl(&astArena, vm, sourceText, sourceTextSize, sourcename, nullptr, raiseerror);

    if (!r) {
        return false;
    }

    SQASTWriter writer(_ss(vm)->_alloc_ctx, ostream);
    writer.writeAST(r, sourcename);

    return true;
}

}; // SQCompilation

#endif
