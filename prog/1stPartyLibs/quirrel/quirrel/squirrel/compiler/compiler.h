/*  see copyright notice in squirrel.h */
#pragma once

struct SQVM;
class OutputStream;
class Arena;

namespace SQCompilation
{

class RootBlock;
class Comments;

struct SqASTData {
  Arena *astArena;
  RootBlock *root;
  char sourceName[256];
  Comments *comments;
};


typedef void(*CompilerErrorFunc)(void *ud, const char *s);
bool Compile(SQVM *vm, const char *sourceText, size_t sourceTextSize, const HSQOBJECT *bindings, const char *sourcename, SQObjectPtr &out, bool raiseerror);

SqASTData *ParseToAST(SQVM *vm, const char *sourceText, size_t sourceTextSize, const char *sourcename, bool preserveComments, bool raiseerror);
bool TranslateASTToBytecode(SQVM *vm, SqASTData *astData, const HSQOBJECT *bindings, const char *sourceText, size_t sourceTextSize, SQObjectPtr &out, bool raiseerror);
void AnalyzeCode(SQVM *vm, SqASTData *astData, const HSQOBJECT *bindings, const char *sourceText, size_t sourceTextSize);
bool AstGetImports(HSQUIRRELVM v, SQCompilation::SqASTData *astData, SQInteger *num, SQModuleImport **imports);
void AstFreeImports(HSQUIRRELVM v, SQInteger num, SQModuleImport *imports);

}; // SQCompilation
