/*  see copyright notice in squirrel.h */
#ifndef _SQLEXER_H_
#define _SQLEXER_H_

#include "compilationcontext.h"
#include "sourceloc.h"

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
    using LexChar = unsigned char;

    SQLexer(SQSharedState *ss, SQCompilation::SQCompilationContext &ctx, SQCompilation::Comments *comments);
    ~SQLexer();
    void Init(const char *code, size_t codeSize);
    SQInteger Lex();
    const char *Tok2Str(SQInteger tok);
    void SetStringValue();
private:
    void nextLine();
    SQInteger LexSingleToken();
    SQInteger GetIDType(const char *s,SQInteger len);
    SQInteger ReadString(SQInteger ndelim,bool verbatim, bool advance = true);
    SQInteger ReadNumber();
    void LexBlockComment();
    void LexLineComment();
    SQInteger ReadID();
    SQInteger ReadDirective();
    void Next();
    SQInteger AddUTF8(SQUnsignedInteger ch);
    SQInteger ProcessStringHexEscape(char *dest, SQInteger maxdigits);
    SQCompilation::Comments::LineCommentsList &CurLineComments() { assert(_comments);  return _comments->commentsList().back(); }

    void AddComment(enum SQCompilation::CommentKind kind, SQInteger line, SQInteger start, SQInteger end);

private:
    SQInteger _curtoken = -1;
    SQTable *_keywords = nullptr;
    SQBool _reached_eof = SQFalse;
    SQCompilation::SQCompilationContext &_ctx;
    const char *_sourceText = nullptr;
    size_t _sourceTextSize = 0;
    size_t _sourceTextPtr = 0;
    SQCompilation::Comments *_comments = nullptr;
    sqvector<char> _currentComment;

public:
    SQCompilation::SourceSpan tokenSpan() const {
        return {
            {static_cast<int32_t>(_tokenline), static_cast<int32_t>(_tokencolumn)},
            {static_cast<int32_t>(_currentline), static_cast<int32_t>(_currentcolumn)}
        };
    }

    SQCompilation::SourceLoc tokenStart() const {
        return {static_cast<int32_t>(_tokenline), static_cast<int32_t>(_tokencolumn)};
    }

    SQCompilation::SourceLoc currentPos() const {
        return {static_cast<int32_t>(_currentline), static_cast<int32_t>(_currentcolumn)};
    }

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
    const char *_svalue = nullptr;
    SQInteger _nvalue = 0;
    SQFloat _fvalue = 0.0f;
    LexChar _currdata = 0;
    SQSharedState *_sharedstate = nullptr;
    sqvector<char> _longstr;
};

#endif
