/*  see copyright notice in squirrel.h */
#ifndef _SQLEXER_H_
#define _SQLEXER_H_

#include "sqcompilationcontext.h"

typedef unsigned char LexChar;

using namespace SQCompilation;

enum SQLexerState {
  LS_REGULAR,
  LS_TEMPLATE
};

enum SQTokenFlags {
  TF_PREP_EOL = 1 << 0,             // end of line after this token
  TF_PREP_SPACE = 1 << 1,           // space after this token
};

struct SQLexer
{
    SQLexer(SQSharedState *ss, SQCompilationContext &ctx, Comments *comments);
    ~SQLexer();
    void Init(const char *code, size_t codeSize);
    SQInteger Lex();
    const SQChar *Tok2Str(SQInteger tok);
    void SetStringValue();
private:
    void nextLine();
    SQInteger LexSingleToken();
    SQInteger GetIDType(const SQChar *s,SQInteger len);
    SQInteger ReadString(SQInteger ndelim,bool verbatim, bool advance = true);
    SQInteger ReadNumber();
    void LexBlockComment();
    void LexLineComment();
    SQInteger ReadID();
    SQInteger ReadDirective();
    void Next();
    SQInteger AddUTF8(SQUnsignedInteger ch);
    SQInteger ProcessStringHexEscape(SQChar *dest, SQInteger maxdigits);
    Comments::LineCommentsList &CurLineComments() { assert(_comments);  return _comments->commentsList().back(); }

    void AddComment(enum CommentKind kind, SQInteger line, SQInteger start, SQInteger end);

private:
    SQInteger _curtoken = -1;
    SQTable *_keywords = nullptr;
    SQBool _reached_eof = SQFalse;
    SQCompilationContext &_ctx;
    const char *_sourceText = nullptr;
    size_t _sourceTextSize = 0;
    size_t _sourceTextPtr = 0;
    Comments *_comments = nullptr;
    sqvector<SQChar> _currentComment;

public:
    SQInteger _prevtoken = -1;
    SQInteger _currentline = 1;
    SQInteger _lasttokenline = -1;
    SQInteger _lasttokencolumn = 0;
    SQInteger _currentcolumn = 0;
    SQInteger _tokencolumn = 0;
    SQInteger _tokenline = 1;
    SQInteger _expectedToken = -1;
    unsigned _prevflags = 0;
    unsigned _flags = 0;
    enum SQLexerState _state = LS_REGULAR;
    const SQChar *_svalue = nullptr;
    SQInteger _nvalue = 0;
    SQFloat _fvalue = 0.0f;
    LexChar _currdata = 0;
    SQSharedState *_sharedstate = nullptr;
    sqvector<SQChar> _longstr;
};

#endif
