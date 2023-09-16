/*  see copyright notice in squirrel.h */
#ifndef _SQLEXER_H_
#define _SQLEXER_H_

#include "sqcompilationcontext.h"

typedef unsigned char LexChar;

struct SQLexerMacroState
{
    bool insideStringInterpolation;
    SQLEXREADFUNC prevReadF;
    SQUserPointer prevUserPointer;
    LexChar prevCurrdata;
    sqvector<SQChar> macroStr;
    sqvector<SQChar> macroParams;
    int macroStrPos;

    SQLexerMacroState(SQAllocContext ctx)
        : macroStr(ctx), macroParams(ctx)
    {
        reset();
    }

    void reset()
    {
        insideStringInterpolation = false;
        prevReadF = NULL;
        prevUserPointer = NULL;
        macroStrPos = 0;
        prevCurrdata = 0;
        macroStr.resize(0);
        macroParams.resize(0);
    }

    static SQInteger macroReadF(SQUserPointer self)
    {
        SQLexerMacroState * s = (SQLexerMacroState *) self;
        return s->macroStr[s->macroStrPos++];
    }
};

using namespace SQCompilation;

enum SQLexerState {
  LS_REGULAR,
  LS_TEMPALTE
};

enum SQLexerMode {
  LM_LEGACY,
  LM_AST
};

struct SQLexer
{
    SQLexer(SQSharedState *ss, SQCompilationContext &ctx, enum SQLexerMode mode);
    ~SQLexer();
    void Init(SQSharedState *ss, const char *code, size_t codeSize);
    SQInteger Lex();
    const SQChar *Tok2Str(SQInteger tok);
private:
    SQInteger LexSingleToken();
    SQInteger GetIDType(const SQChar *s,SQInteger len);
    SQInteger ReadString(SQInteger ndelim,bool verbatim, bool advance = true);
    SQInteger ReadNumber();
    void LexBlockComment();
    void LexLineComment();
    SQInteger ReadID();
    SQInteger ReadDirective();
    void Next();
    void AppendPosDirective(sqvector<SQChar> & vec);
    bool ProcessReaderMacro();
    void ExitReaderMacro();
    static SQInteger readf(void *);
    SQInteger AddUTF8(SQUnsignedInteger ch);
    SQInteger ProcessStringHexEscape(SQChar *dest, SQInteger maxdigits);
    SQInteger _curtoken;
    SQTable *_keywords;
    SQBool _reached_eof;
    SQLexerMacroState macroState;
    SQCompilationContext &_ctx;
    const char *_sourceText;
    size_t _sourceTextSize;
    size_t _sourceTextPtr;
public:
    SQInteger _prevtoken;
    SQInteger _currentline;
    SQInteger _lasttokenline;
    SQInteger _lasttokencolumn;
    SQInteger _currentcolumn;
    SQInteger _tokencolumn;
    SQInteger _tokenline;
    SQInteger _expectedToken;
    enum SQLexerState _state;
    enum SQLexerMode _mode;
    const SQChar *_svalue;
    SQInteger _nvalue;
    SQFloat _fvalue;
    SQLEXREADFUNC _readf;
    SQUserPointer _up;
    LexChar _currdata;
    SQSharedState *_sharedstate;
    sqvector<SQChar> _longstr;
};

#endif
