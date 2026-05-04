/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#ifndef NO_COMPILER
#include <stdarg.h>
#include <algorithm>
#include "opcodes.h"
#include "sqstring.h"
#include "sqfuncproto.h"
#include "compiler.h"
#include "sqfuncstate.h"
#include "optimizer.h"
#include "lexer.h"
#include "sqvm.h"
#include "sqtable.h"
#include "ast.h"
#include "parser.h"
#include "codegen.h"
#include "sqtypeparser.h"
#include "optimizations/closureHoisting.h"
#include "compilationcontext.h"
#include "static_analyzer/analyzer.h"


namespace SQCompilation {

static RootBlock *parseASTImpl(Arena *astArena, SQVM *vm, const char *sourceText, size_t sourceTextSize, const char *sourcename, Comments *comments, bool raiseerror) {
  Arena parseArena(_ss(vm)->_alloc_ctx, "Parser");
  SQCompilationContext ctx(vm, &parseArena, sourcename, sourceText, sourceTextSize, comments, raiseerror);
  SQParser p(vm, sourceText, sourceTextSize, sourcename, astArena, ctx, comments);

  RootBlock *r = p.parse();

  if (r) {
    ClosureHoistingOpt opt(_ss(vm), astArena);
    opt.run(r);
  }

  return r;
}

SqASTData *ParseToAST(SQVM *vm, const char *sourceText, size_t sourceTextSize, const char *sourcename, bool preserveComments, bool raiseerror) {

    SqASTData *astData = sq_allocateASTData(vm);

    if (sourcename) {
        strncpy(astData->sourceName, sourcename, sizeof(astData->sourceName));
        astData->sourceName[sizeof(astData->sourceName) / sizeof(astData->sourceName[0]) - 1] = 0;
    }

    astData->root = parseASTImpl(astData->astArena, vm, sourceText, sourceTextSize, astData->sourceName, preserveComments ? astData->comments : nullptr, raiseerror);
    if (astData->root)
        return astData;

    sq_releaseASTData(vm, astData);
    return nullptr;
}

bool Compile(SQVM *vm, const char *sourceText, size_t sourceTextSize, const HSQOBJECT *bindings, const char *sourcename, SQObjectPtr &out, bool raiseerror)
{
    Arena astArena(_ss(vm)->_alloc_ctx, "AST");

    RootBlock *r = parseASTImpl(&astArena, vm, sourceText, sourceTextSize, sourcename, nullptr, raiseerror);

    if (!r)
      return false;

    Arena cgArena(_ss(vm)->_alloc_ctx, "Codegen");
    SQCompilationContext ctx(vm, &cgArena, sourcename, sourceText, sourceTextSize, nullptr, raiseerror);
    CodeGenVisitor codegen(&cgArena, bindings, vm, sourcename, ctx);

    return codegen.generate(r, out);
}

static bool TranslateASTToBytecodeImpl(SQVM *vm, RootBlock *ast, const HSQOBJECT *bindings, const char *sourcename, const char *sourceText, size_t sourceTextSize, SQObjectPtr &out, const Comments *comments, bool raiseerror)
{
    Arena cgArena(_ss(vm)->_alloc_ctx, "Codegen");
    SQCompilationContext ctx(vm, &cgArena, sourcename, sourceText, sourceTextSize, comments, raiseerror);
    CodeGenVisitor codegen(&cgArena, bindings, vm, sourcename, ctx);

    return codegen.generate(ast, out);
}

bool TranslateASTToBytecode(SQVM *vm, SqASTData *astData, const HSQOBJECT *bindings, const char *sourceText, size_t sourceTextSize, SQObjectPtr &out, bool raiseerror)
{
    return TranslateASTToBytecodeImpl(vm, astData->root, bindings, astData->sourceName, sourceText, sourceTextSize, out, astData->comments, raiseerror);
}

void AnalyzeCode(SQVM *vm, SqASTData *astData, const HSQOBJECT *bindings, const char *sourceText, size_t sourceTextSize)
{
    Arena saArena(_ss(vm)->_alloc_ctx, "Analyzer");
    SQCompilationContext ctx(vm, &saArena, astData->sourceName, sourceText, sourceTextSize, astData->comments, true);

    RootBlock *ast = astData->root;
    StaticAnalyzer sa(ctx);

    sa.runAnalysis(ast, bindings);
}

bool AstGetImports(HSQUIRRELVM v, SQCompilation::SqASTData *astData, SQInteger *num, SQModuleImport **imports)
{
    if (!astData || !astData->root || !num || !imports)
        return false;

    const ArenaVector<Statement *> &stmts = astData->root->statements();
    size_t importCount = 0;
    for (size_t i = 0; i < stmts.size(); i++) {
        TreeOp op = stmts[i]->op();
        if (op == TO_IMPORT)
            importCount++;
          else if (op != TO_DIRECTIVE)
            break;
    }

    if (importCount == 0) {
        *num = 0;
        *imports = nullptr;
        return true;
    }

    SQModuleImport *importArrayOut = (SQModuleImport *)sq_vm_malloc(_ss(v)->_alloc_ctx, sizeof(SQModuleImport) * importCount);
    memset(importArrayOut, 0, sizeof(SQModuleImport) * importCount);

    size_t importIdx = 0;
    for (size_t i = 0; i < stmts.size(); i++) {
        if (stmts[i]->op() == TO_DIRECTIVE)
            continue;
        if (stmts[i]->op() != TO_IMPORT)
            break;

        ImportStmt *importStmt = static_cast<ImportStmt *>(stmts[i]);
        size_t numSlots = importStmt->slots.size();

        SQModuleImport &imp =  importArrayOut[importIdx];
        imp.name = importStmt->moduleName;
        imp.alias = importStmt->moduleAlias;
        imp.numSlots = (int)numSlots;
        imp.slots = numSlots ? importStmt->slots.begin() : nullptr;
        imp.line = importStmt->lineStart();
        imp.nameColumn = importStmt->nameCol;
        imp.aliasColumn = importStmt->aliasCol;

        importIdx++;
    }

    *num = (SQInteger)importCount;
    *imports = importArrayOut;

    return true;
}

void AstFreeImports(HSQUIRRELVM v, SQInteger num, SQModuleImport *imports)
{
    if (!imports || num <= 0)
        return;

    sq_vm_free(_ss(v)->_alloc_ctx, imports, sizeof(SQModuleImport) * num);
}

}; // SQCompilation

#endif
