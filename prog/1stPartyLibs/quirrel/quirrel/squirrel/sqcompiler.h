/*  see copyright notice in squirrel.h */
#ifndef _SQCOMPILER_H_
#define _SQCOMPILER_H_

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
  const SQChar *sourceName;
  class Comments *comments;
};

#define TK_IDENTIFIER   258
#define TK_STRING_LITERAL   259
#define TK_INTEGER  260
#define TK_FLOAT    261
#define TK_BASE 262
#define TK_DELETE   263
#define TK_EQ   264
#define TK_NE   265
#define TK_LE   266
#define TK_GE   267
#define TK_SWITCH   268
#define TK_ARROW    269
#define TK_AND  270
#define TK_OR   271
#define TK_IF   272
#define TK_ELSE 273
#define TK_WHILE    274
#define TK_BREAK    275
#define TK_FOR  276
#define TK_DO   277
#define TK_NULL 278
#define TK_FOREACH  279
#define TK_IN   280
#define TK_NEWSLOT  281
#define TK_MODULO   282
#define TK_LOCAL    283
#define TK_CLONE    284
#define TK_FUNCTION 285
#define TK_RETURN   286
#define TK_TYPEOF   287
#define TK_UMINUS   288
#define TK_PLUSEQ   289
#define TK_MINUSEQ  290
#define TK_CONTINUE 291
#define TK_YIELD 292
#define TK_TRY 293
#define TK_CATCH 294
#define TK_THROW 295
#define TK_SHIFTL 296
#define TK_SHIFTR 297
#define TK_RESUME 298
#define TK_DOUBLE_COLON 299
#define TK_CASE 300
#define TK_DEFAULT 301
#define TK_THIS 302
#define TK_PLUSPLUS 303
#define TK_MINUSMINUS 304
#define TK_3WAYSCMP 305
#define TK_USHIFTR 306
#define TK_CLASS 307
#define TK_EXTENDS 308
#define TK_CONSTRUCTOR 310
#define TK_INSTANCEOF 311
#define TK_VARPARAMS 312
#define TK___LINE__ 313
#define TK___FILE__ 314
#define TK_TRUE 315
#define TK_FALSE 316
#define TK_MULEQ 317
#define TK_DIVEQ 318
#define TK_MODEQ 319
#define TK_STATIC 322
#define TK_ENUM 323
#define TK_CONST 324
#define TK_RESERVED_001 325
#define TK_NULLGETSTR 326
#define TK_NULLGETOBJ 327
#define TK_NULLCOALESCE 328
#define TK_NULLCALL 329
#define TK_RESERVED_XXXXX 330
#define TK_GLOBAL 331
#define TK_DIRECTIVE 332
#define TK_READERMACRO 333
#define TK_NOT 334
#define TK_INEXPR_ASSIGNMENT 335
#define TK_LET 336
#define TK_TEMPLATE_PREFIX 337
#define TK_TEMPLATE_INFIX 338
#define TK_TEMPLATE_SUFFIX 339
#define TK_TEMPLATE_OP 340
#define TK_BUILT_IN_GETSTR 341
#define TK_NULLABLE_BUILT_IN_GETSTR 342

#ifndef SQ_LINE_INFO_IN_STRUCTURES
#  define SQ_LINE_INFO_IN_STRUCTURES 1
#endif

#define MAX_COMPILER_ERROR_LEN 256
#define MAX_FUNCTION_NAME_LEN 128

struct SQScope {
    SQInteger outers;
    SQInteger stacksize;
};

enum SQExpressionContext
{
  SQE_REGULAR = 0,
  SQE_IF,
  SQE_SWITCH,
  SQE_LOOP_CONDITION,
  SQE_FUNCTION_ARG,
  SQE_RVALUE,
  SQE_ARRAY_ELEM,
};


#define BEGIN_SCOPE() SQScope __oldscope__ = _scope; \
                     _scope.outers = _fs->_outers; \
                     _scope.stacksize = _fs->GetStackSize(); \
                     _scopedconsts.push_back();

#define RESOLVE_OUTERS() if(_fs->GetStackSize() != _fs->_blockstacksizes.top()) { \
                            if(_fs->CountOuters(_fs->_blockstacksizes.top())) { \
                                _fs->AddInstruction(_OP_CLOSE,0,_fs->_blockstacksizes.top()); \
                            } \
                        }

#define END_SCOPE_NO_CLOSE() {  if(_fs->GetStackSize() != _scope.stacksize) { \
                            _fs->SetStackSize(_scope.stacksize); \
                        } \
                        _scope = __oldscope__; \
                        assert(!_scopedconsts.empty()); \
                        _scopedconsts.pop_back(); \
                    }

#define END_SCOPE() {   SQInteger oldouters = _fs->_outers;\
                        if(_fs->GetStackSize() != _scope.stacksize) { \
                            _fs->SetStackSize(_scope.stacksize); \
                            if(oldouters != _fs->_outers) { \
                                _fs->AddInstruction(_OP_CLOSE,0,_scope.stacksize); \
                            } \
                        } \
                        _scope = __oldscope__; \
                        _scopedconsts.pop_back(); \
                    }

#define BEGIN_BREAKBLE_BLOCK()  SQInteger __nbreaks__=_fs->_unresolvedbreaks.size(); \
                            SQInteger __ncontinues__=_fs->_unresolvedcontinues.size(); \
                            _fs->_breaktargets.push_back(0);_fs->_continuetargets.push_back(0); \
                            _fs->_blockstacksizes.push_back(_scope.stacksize);


#define END_BREAKBLE_BLOCK(continue_target) {__nbreaks__=_fs->_unresolvedbreaks.size()-__nbreaks__; \
                    __ncontinues__=_fs->_unresolvedcontinues.size()-__ncontinues__; \
                    if(__ncontinues__>0)ResolveContinues(_fs,__ncontinues__,continue_target); \
                    if(__nbreaks__>0)ResolveBreaks(_fs,__nbreaks__); \
                    _fs->_breaktargets.pop_back();_fs->_continuetargets.pop_back(); \
                    _fs->_blockstacksizes.pop_back(); }


typedef void(*CompilerErrorFunc)(void *ud, const SQChar *s);
bool Compile(SQVM *vm, const char *sourceText, size_t sourceTextSize, const HSQOBJECT *bindings, const SQChar *sourcename, SQObjectPtr &out, bool raiseerror, bool lineinfo);

bool CompileWithAst(SQVM *vm, const char *sourceText, size_t sourceTextSize, const HSQOBJECT *bindings, const SQChar *sourcename, SQObjectPtr &out, bool raiseerror, bool lineinfo);

SqASTData *ParseToAST(SQVM *vm, const char *sourceText, size_t sourceTextSize, const SQChar *sourcename, bool preserveComments, bool raiseerror);
bool ParseAndSaveBinaryAST(SQVM *vm, const char *sourceText, size_t sourceTextSize, const SQChar *sourcename, OutputStream *ostream, bool raiseerror);
bool TranslateASTToBytecode(SQVM *vm, SqASTData *astData, const HSQOBJECT *bindings, const char *sourceText, size_t sourceTextSize, SQObjectPtr &out, bool raiseerror, bool lineinfo);
bool TranslateBinaryASTToBytecode(SQVM *vm, const uint8_t *buffer, size_t size, const HSQOBJECT *bindings, SQObjectPtr &out, bool raiseerror, bool lineinfo);
void AnalyseCode(SQVM *vm, SqASTData *astData, const HSQOBJECT *bindings, const char *sourceText, size_t sourceTextSize);

}; // SQCompilation
#endif //_SQCOMPILER_H_
