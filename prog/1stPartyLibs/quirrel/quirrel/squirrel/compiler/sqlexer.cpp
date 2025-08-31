/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include <ctype.h>
#include <stdlib.h>
#include <float.h>
#include "sqtable.h"
#include "sqstring.h"
#include "sqcompiler.h"
#include "sqlexer.h"

#define CUR_CHAR (_currdata)
#define RETURN_TOKEN(t) { _prevtoken = _curtoken; _prevflags = _flags; _flags = 0; _curtoken = t; return t;}
#define IS_EOB() (CUR_CHAR <= SQUIRREL_EOB)
#define NEXT() {Next();_currentcolumn++;}
#define INIT_TEMP_STRING() { _longstr.resize(0);}
#define APPEND_CHAR(c) { _longstr.push_back(c);}
#define TERMINATE_BUFFER() {_longstr.push_back(_SC('\0'));}
#define ADD_KEYWORD(key,id) _keywords->NewSlot( SQObjectPtr(SQString::Create(_sharedstate, _SC(#key))), SQObjectPtr(SQInteger(id)))

using namespace SQCompilation;

SQLexer::SQLexer(SQSharedState *ss, SQCompilationContext &ctx, Comments *comments)
    : _sharedstate(ss)
    , _longstr(ss->_alloc_ctx)
    , _ctx(ctx)
    , _comments(comments)
    , _currentComment(ss->_alloc_ctx)
{
}

SQLexer::~SQLexer()
{
    _keywords->Release();
}

using CommentVec = sqvector<CommentData>;

void SQLexer::Init(const char *sourceText, size_t sourceTextSize)
{
    _keywords = SQTable::Create(_sharedstate, 39);
    ADD_KEYWORD(while, TK_WHILE);
    ADD_KEYWORD(do, TK_DO);
    ADD_KEYWORD(if, TK_IF);
    ADD_KEYWORD(else, TK_ELSE);
    ADD_KEYWORD(break, TK_BREAK);
    ADD_KEYWORD(continue, TK_CONTINUE);
    ADD_KEYWORD(return, TK_RETURN);
    ADD_KEYWORD(null, TK_NULL);
    ADD_KEYWORD(function, TK_FUNCTION);
    ADD_KEYWORD(local, TK_LOCAL);
    ADD_KEYWORD(for, TK_FOR);
    ADD_KEYWORD(foreach, TK_FOREACH);
    ADD_KEYWORD(in, TK_IN);
    ADD_KEYWORD(typeof, TK_TYPEOF);
    ADD_KEYWORD(base, TK_BASE);
    ADD_KEYWORD(delete, TK_DELETE);
    ADD_KEYWORD(try, TK_TRY);
    ADD_KEYWORD(catch, TK_CATCH);
    ADD_KEYWORD(throw, TK_THROW);
    ADD_KEYWORD(clone, TK_CLONE);
    ADD_KEYWORD(yield, TK_YIELD);
    ADD_KEYWORD(resume, TK_RESUME);
    ADD_KEYWORD(switch, TK_SWITCH);
    ADD_KEYWORD(case, TK_CASE);
    ADD_KEYWORD(default, TK_DEFAULT);
    ADD_KEYWORD(this, TK_THIS);
    ADD_KEYWORD(class,TK_CLASS);
    ADD_KEYWORD(constructor,TK_CONSTRUCTOR);
    ADD_KEYWORD(instanceof,TK_INSTANCEOF);
    ADD_KEYWORD(true,TK_TRUE);
    ADD_KEYWORD(false,TK_FALSE);
    ADD_KEYWORD(static,TK_STATIC);
    ADD_KEYWORD(enum,TK_ENUM);
    ADD_KEYWORD(const,TK_CONST);
    ADD_KEYWORD(__LINE__,TK___LINE__);
    ADD_KEYWORD(__FILE__,TK___FILE__);
    ADD_KEYWORD(global, TK_GLOBAL);
    ADD_KEYWORD(not, TK_NOT);
    ADD_KEYWORD(let, TK_LET);

    _sourceText = sourceText;
    _sourceTextSize = sourceTextSize;
    _sourceTextPtr = 0;
    _lasttokenline = _currentline = 1;
    _lasttokencolumn = _currentcolumn = 0;
    _prevtoken = -1;
    _tokencolumn = 0;
    _tokenline = 1;
    _reached_eof = SQFalse;
    if (_comments)
      _comments->pushNewLine();
    Next();
}

void SQLexer::Next()
{
  if (_sourceTextPtr >= _sourceTextSize) {
      _reached_eof = SQTrue;
      _currdata = SQUIRREL_EOB;
  }
  else {
      _currdata = _sourceText[_sourceTextPtr++];
  }
}

const SQChar *SQLexer::Tok2Str(SQInteger tok)
{
    SQObjectPtr itr, key, val;
    SQInteger nitr;
    while((nitr = _keywords->Next(false,itr, key, val)) != -1) {
        itr = (SQInteger)nitr;
        if(((SQInteger)_integer(val)) == tok)
            return _stringval(key);
    }
    return NULL;
}

void SQLexer::SetStringValue() {
  _svalue = &_longstr[0];
}

void SQLexer::AddComment(enum CommentKind kind, SQInteger line, SQInteger start, SQInteger end) {
  if (!_comments)
    return;
  size_t size = _currentComment.size();
  SQChar *data = (SQChar *)sq_vm_malloc(_sharedstate->_alloc_ctx, (size + 1) * sizeof(SQChar));
  memcpy(data, &_currentComment[0], size);
  data[size] = '\0';

  CurLineComments().push_back({ kind, size, data, line, start, end });

  _currentComment.clear();
}

void SQLexer::LexBlockComment()
{
    enum CommentKind k = CK_BLOCK;
    SQInteger line = 1;
    SQInteger start = _currentcolumn;
    bool done = false;
    while(!done) {
        _currentComment.push_back(CUR_CHAR);
        switch(CUR_CHAR) {
            case _SC('*'): { NEXT(); if(CUR_CHAR == _SC('/')) { done = true; NEXT(); }}; continue;
            case _SC('\n'):
              k = CK_ML_BLOCK;
              AddComment(k, line, start, _currentcolumn);
              ++line;
              nextLine();
              NEXT();
              start = _currentcolumn;
              continue;
            case SQUIRREL_EOB:
              _ctx.reportDiagnostic(DiagnosticsId::DI_TRAILING_BLOCK_COMMENT, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
              return;
            default: NEXT();
        }
    }

    AddComment(k, k == CK_ML_BLOCK ? line : 0, start, _currentcolumn);
}

void SQLexer::LexLineComment()
{
    SQInteger start = _currentcolumn;
    do {
        NEXT();
        _currentComment.push_back(CUR_CHAR);
    } while (CUR_CHAR != _SC('\n') && (!IS_EOB()));
    AddComment(CK_LINE, 0, start, _currentcolumn);
}

SQInteger SQLexer::Lex()
{
    return LexSingleToken();
}

void SQLexer::nextLine() {
  ++_currentline;
  if (_comments)
    _comments->pushNewLine();
}

SQInteger SQLexer::LexSingleToken()
{
    _lasttokenline = _currentline;
    _lasttokencolumn = _currentcolumn;

    if (_state == LS_TEMPLATE && _expectedToken != TK_TEMPLATE_PREFIX) {
      SQInteger stype = ReadString(_SC('"'), false, false);
      if (stype != -1)
        RETURN_TOKEN(stype);
      _ctx.reportDiagnostic(DiagnosticsId::DI_LEX_ERROR_PARSE, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "the string");
    }

    while(CUR_CHAR != SQUIRREL_EOB) {
        _tokenline = _currentline;
        _tokencolumn = _currentcolumn;
        switch(CUR_CHAR){
        case _SC('\t'): case _SC('\r'): case _SC(' '): _flags |= TF_PREP_SPACE; NEXT(); continue;
        case _SC('\n'):
            nextLine();
            _prevtoken=_curtoken;
            _flags |= TF_PREP_EOL;
            _curtoken=_SC('\n');
            NEXT();
            _currentcolumn=0;
            continue;
        case _SC('/'):
            NEXT();
            switch(CUR_CHAR){
            case _SC('*'):
                NEXT();
                LexBlockComment();
                continue;
            case _SC('/'):
                LexLineComment();
                continue;
            case _SC('='):
                NEXT();
                RETURN_TOKEN(TK_DIVEQ);
                continue;
            default:
                RETURN_TOKEN('/');
            }
        case _SC('='):
            NEXT();
            if (CUR_CHAR != _SC('=')){ RETURN_TOKEN('=') }
            else { NEXT(); RETURN_TOKEN(TK_EQ); }
        case _SC('<'):
            NEXT();
            switch(CUR_CHAR) {
            case _SC('='):
                NEXT();
                if(CUR_CHAR == _SC('>')) {
                    NEXT();
                    RETURN_TOKEN(TK_3WAYSCMP);
                }
                RETURN_TOKEN(TK_LE)
                break;
            case _SC('-'): NEXT(); RETURN_TOKEN(TK_NEWSLOT); break;
            case _SC('<'): NEXT(); RETURN_TOKEN(TK_SHIFTL); break;
            }
            RETURN_TOKEN('<');
        case _SC('>'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_GE);}
            else if(CUR_CHAR == _SC('>')){
                NEXT();
                if(CUR_CHAR == _SC('>')){
                    NEXT();
                    RETURN_TOKEN(TK_USHIFTR);
                }
                RETURN_TOKEN(TK_SHIFTR);
            }
            else { RETURN_TOKEN('>') }
        case _SC('!'):
            NEXT();
            if (CUR_CHAR != _SC('=')){ RETURN_TOKEN('!')}
            else { NEXT(); RETURN_TOKEN(TK_NE); }
        case _SC('#'): {
            NEXT();
            SQInteger stype = ReadDirective();
            if (stype < 0)
                _ctx.reportDiagnostic(DiagnosticsId::DI_LEX_ERROR_PARSE, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "directive");
            RETURN_TOKEN(TK_DIRECTIVE);
            }
        case _SC('$'): {
            NEXT();
            if (CUR_CHAR == _SC('"')) {
                RETURN_TOKEN(TK_TEMPLATE_OP);
            }
            else {
                RETURN_TOKEN('$');
            }
            }
        case _SC('@'): {
            SQInteger stype;
            NEXT();
            if (CUR_CHAR == _SC('@')) {
                NEXT();
                stype = ReadString('"', true);
                if (stype != TK_STRING_LITERAL)
                    _ctx.reportDiagnostic(DiagnosticsId::DI_LEX_ERROR_PARSE, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "the docstring");
                RETURN_TOKEN(TK_DOCSTRING);
            }
            if (CUR_CHAR != _SC('"')) {
              RETURN_TOKEN('@');
            }
            if ((stype = ReadString('"', true)) != -1) {
              RETURN_TOKEN(stype);
            }
            _ctx.reportDiagnostic(DiagnosticsId::DI_LEX_ERROR_PARSE, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "the string");
            break;
            }
        case _SC('"'):
        case _SC('\''): {
            SQInteger stype;
            if((stype=ReadString(CUR_CHAR,false))!=-1){
                RETURN_TOKEN(stype);
            }
            _ctx.reportDiagnostic(DiagnosticsId::DI_LEX_ERROR_PARSE, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "the string");
            break;
            }
        case _SC('{'): case _SC('}'): case _SC('('): case _SC(')'): case _SC('['): case _SC(']'):
        case _SC(';'): case _SC(','): case _SC('^'): case _SC('~'):
            {SQInteger ret = CUR_CHAR;
            NEXT(); RETURN_TOKEN(ret); }
        case _SC('?'):
            {NEXT();
            if (CUR_CHAR == _SC('.')) {
                NEXT();
                if (CUR_CHAR == _SC('$')) {
                    NEXT();
                    RETURN_TOKEN(TK_NULLABLE_TYPE_METHOD_GETSTR);
                }
                else {
                    RETURN_TOKEN(TK_NULLGETSTR);
                }
            }
            if (CUR_CHAR == _SC('[')) { NEXT(); RETURN_TOKEN(TK_NULLGETOBJ); }
            if (CUR_CHAR == _SC('(')) { NEXT(); RETURN_TOKEN(TK_NULLCALL); }
            if (CUR_CHAR == _SC('?')) { NEXT(); RETURN_TOKEN(TK_NULLCOALESCE); }
            RETURN_TOKEN('?'); }
        case _SC('.'):
            NEXT();
            if (CUR_CHAR == _SC('$')) {
                NEXT();
                RETURN_TOKEN(TK_TYPE_METHOD_GETSTR);
            }
            if (CUR_CHAR != _SC('.')){ RETURN_TOKEN('.') }
            NEXT();
            if (CUR_CHAR != _SC('.')){
              _ctx.reportDiagnostic(DiagnosticsId::DI_INVALID_TOKEN, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "..");
            }
            NEXT();
            RETURN_TOKEN(TK_VARPARAMS);
        case _SC('&'):
            NEXT();
            if (CUR_CHAR != _SC('&')){ RETURN_TOKEN('&') }
            else { NEXT(); RETURN_TOKEN(TK_AND); }
        case _SC('|'):
            NEXT();
            if (CUR_CHAR != _SC('|')){ RETURN_TOKEN('|') }
            else { NEXT(); RETURN_TOKEN(TK_OR); }
        case _SC(':'):
            NEXT();
            if (CUR_CHAR != _SC(':')){ RETURN_TOKEN(':') }
            else { NEXT(); RETURN_TOKEN(TK_DOUBLE_COLON); }
        case _SC('*'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_MULEQ);}
            else RETURN_TOKEN('*');
        case _SC('%'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_MODEQ);}
            else RETURN_TOKEN('%');
        case _SC('-'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_MINUSEQ);}
            else if  (CUR_CHAR == _SC('-')){ NEXT(); RETURN_TOKEN(TK_MINUSMINUS);}
            else RETURN_TOKEN('-');
        case _SC('+'):
            NEXT();
            if (CUR_CHAR == _SC('=')){ NEXT(); RETURN_TOKEN(TK_PLUSEQ);}
            else if (CUR_CHAR == _SC('+')){ NEXT(); RETURN_TOKEN(TK_PLUSPLUS);}
            else RETURN_TOKEN('+');
        case SQUIRREL_EOB:
            return 0;
        default:{
                if (isdigit(CUR_CHAR)) {
                    SQInteger ret = ReadNumber();
                    RETURN_TOKEN(ret);
                }
                else if (isalpha(CUR_CHAR) || CUR_CHAR == _SC('_')) {
                    SQInteger t = ReadID();
                    RETURN_TOKEN(t);
                }
                else {
                    SQInteger c = CUR_CHAR;
                    if (iscntrl((int)c))
                        _ctx.reportDiagnostic(DiagnosticsId::DI_UNEXPECTED_CHAR, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "(control)");
                    NEXT();
                    RETURN_TOKEN(c);
                }
            }
        }
    }
    return 0;
}

SQInteger SQLexer::GetIDType(const SQChar *s,SQInteger len)
{
    SQObjectPtr t;
    if(_keywords->GetStr(s,len, t)) {
        return SQInteger(_integer(t));
    }
    return TK_IDENTIFIER;
}

SQInteger SQLexer::AddUTF8(SQUnsignedInteger ch)
{
    if (ch < 0x80) {
        APPEND_CHAR((char)ch);
        return 1;
    }
    if (ch < 0x800) {
        APPEND_CHAR((SQChar)((ch >> 6) | 0xC0));
        APPEND_CHAR((SQChar)((ch & 0x3F) | 0x80));
        return 2;
    }
    if (ch < 0x10000) {
        APPEND_CHAR((SQChar)((ch >> 12) | 0xE0));
        APPEND_CHAR((SQChar)(((ch >> 6) & 0x3F) | 0x80));
        APPEND_CHAR((SQChar)((ch & 0x3F) | 0x80));
        return 3;
    }
    if (ch < 0x110000) {
        APPEND_CHAR((SQChar)((ch >> 18) | 0xF0));
        APPEND_CHAR((SQChar)(((ch >> 12) & 0x3F) | 0x80));
        APPEND_CHAR((SQChar)(((ch >> 6) & 0x3F) | 0x80));
        APPEND_CHAR((SQChar)((ch & 0x3F) | 0x80));
        return 4;
    }
    return 0;
}

SQInteger SQLexer::ProcessStringHexEscape(SQChar *dest, SQInteger maxdigits)
{
    NEXT();
    if (!isxdigit(CUR_CHAR))
        _ctx.reportDiagnostic(DiagnosticsId::DI_HEX_NUMBERS_EXPECTED, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
    SQInteger n = 0;
    while (isxdigit(CUR_CHAR) && n < maxdigits) {
        dest[n] = CUR_CHAR;
        n++;
        NEXT();
    }
    dest[n] = 0;
    return n;
}

SQInteger SQLexer::ReadString(SQInteger ndelim,bool verbatim, bool advance)
{
    SQInteger t = TK_STRING_LITERAL;
    if (_state == LS_TEMPLATE && ndelim != _SC('\"')) {
        _ctx.reportDiagnostic(DiagnosticsId::DI_EXPECTED_LEX, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "string");
        return -1;
    }
    INIT_TEMP_STRING();
    if (advance)
      NEXT();
    if(IS_EOB()) return -1;
    for(;;) {
        while(CUR_CHAR != ndelim) {
            SQInteger x = CUR_CHAR;
            switch (x) {
            case SQUIRREL_EOB:
                _ctx.reportDiagnostic(DiagnosticsId::DI_UNFINISHED_STRING, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
                return -1;
            case _SC('\n'):
                if(!verbatim)
                    _ctx.reportDiagnostic(DiagnosticsId::DI_NEWLINE_IN_CONST, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
                APPEND_CHAR(CUR_CHAR); NEXT();
                nextLine();
                break;
            case _SC('\\'):
                if(verbatim) {
                    APPEND_CHAR('\\'); NEXT();
                }
                else {
                    NEXT();
                    switch(CUR_CHAR) {
                    case _SC('x'):  {
                        const SQInteger maxdigits = sizeof(SQChar) * 2;
                        SQChar temp[maxdigits + 1];
                        ProcessStringHexEscape(temp, maxdigits);
                        SQChar *stemp;
                        APPEND_CHAR((SQChar)strtoul(temp, &stemp, 16));
                    }
                    break;
                    case _SC('U'):
                    case _SC('u'):  {
                        const SQInteger maxdigits = CUR_CHAR == 'u' ? 4 : 8;
                        SQChar temp[8 + 1];
                        ProcessStringHexEscape(temp, maxdigits);
                        SQChar *stemp;
                        AddUTF8(strtoul(temp, &stemp, 16));
                    }
                    break;
                    case _SC('t'): APPEND_CHAR(_SC('\t')); NEXT(); break;
                    case _SC('a'): APPEND_CHAR(_SC('\a')); NEXT(); break;
                    case _SC('b'): APPEND_CHAR(_SC('\b')); NEXT(); break;
                    case _SC('n'): APPEND_CHAR(_SC('\n')); NEXT(); break;
                    case _SC('r'): APPEND_CHAR(_SC('\r')); NEXT(); break;
                    case _SC('v'): APPEND_CHAR(_SC('\v')); NEXT(); break;
                    case _SC('f'): APPEND_CHAR(_SC('\f')); NEXT(); break;
                    case _SC('0'): APPEND_CHAR(_SC('\0')); NEXT(); break;
                    case _SC('\\'): APPEND_CHAR(_SC('\\')); NEXT(); break;
                    case _SC('"'): APPEND_CHAR(_SC('"')); NEXT(); break;
                    case _SC('\''): APPEND_CHAR(_SC('\'')); NEXT(); break;
                    case _SC('{'): case _SC('}'):
                        if (_state == LS_TEMPLATE) {
                          APPEND_CHAR(CUR_CHAR);
                          NEXT();
                          break;
                        }
                    // fall through -V796
                    default:
                        _ctx.reportDiagnostic(DiagnosticsId::DI_UNRECOGNISED_ESCAPER, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
                    break;
                    }
                }
                break;
            case _SC('{'):
                if (_state == LS_TEMPLATE) {
                    APPEND_CHAR(CUR_CHAR);
                    NEXT();
                    assert(_expectedToken > 0);
                    t = _expectedToken;
                    goto loop_exit;
                }
            default:
                APPEND_CHAR(CUR_CHAR);
                NEXT();
            }
        }
        NEXT();

        if (_state == LS_TEMPLATE)
          t = TK_TEMPLATE_SUFFIX;

        if(verbatim && CUR_CHAR == '"') { //double quotation
            APPEND_CHAR(CUR_CHAR);
            NEXT();
        }
        else {
            break;
        }
    }

loop_exit:
    TERMINATE_BUFFER();
    SQInteger len = _longstr.size()-1;
    if(ndelim == _SC('\'')) {
        assert(_state != LS_TEMPLATE);
        if(len == 0)
            _ctx.reportDiagnostic(DiagnosticsId::DI_EMPTY_LITERAL, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
        if(len > 1)
            _ctx.reportDiagnostic(DiagnosticsId::DI_TOO_LONG_LITERAL, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
        _nvalue = _longstr[0];
        return TK_INTEGER;
    }
    SetStringValue();
    return t;
}

static void LexHexadecimal(const SQChar *s,SQUnsignedInteger *res)
{
    *res = 0;
    while(*s != 0)
    {
        if(isdigit(*s)) *res = (*res)*16+((*s++)-'0');
        else if(isxdigit(*s)) *res = (*res)*16+(toupper(*s++)-'A'+10);
        else { assert(0); }
    }
}

#define INT_OVERFLOW_THRESHOLD (~SQUnsignedInteger(0) / 10)
#define INT_OVERFLOW_DIGIT (~SQUnsignedInteger(0) % 10)

static bool LexInteger(const SQChar *s, SQUnsignedInteger *res)
{

    SQUnsignedInteger x = 0;
    while(*s != 0)
    {
        SQUnsignedInteger digit = (*s++) - '0';
        if (x > INT_OVERFLOW_THRESHOLD || (x == INT_OVERFLOW_THRESHOLD && digit > INT_OVERFLOW_DIGIT))
            return false;
        x = x * 10 + digit;
    }
    *res = x;
    return x <= (~SQUnsignedInteger(0) >> 1);
}

static SQInteger isexponent(SQInteger c) { return c == 'e' || c=='E'; }


#define MAX_HEX_DIGITS (sizeof(SQInteger)*2)
#define NUM_NEXT() { do NEXT() while (CUR_CHAR==_SC('_')); }
SQInteger SQLexer::ReadNumber()
{
#define TINT 1
#define TFLOAT 2
#define THEX 3
#define TSCIENTIFIC 4

    SQInteger type = TINT, firstchar = CUR_CHAR;
    SQChar *sTemp;
    volatile SQFloat value;
    INIT_TEMP_STRING();
    NUM_NEXT();
    if(firstchar == _SC('0') && isdigit(CUR_CHAR))
        _ctx.reportDiagnostic(DiagnosticsId::DI_OCTAL_NOT_SUPPORTED, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
    if(firstchar == _SC('0') && (toupper(CUR_CHAR) == _SC('X')) ) {
        NUM_NEXT();
        type = THEX;
        while(isxdigit(CUR_CHAR)) {
            APPEND_CHAR(CUR_CHAR);
            NUM_NEXT();
        }
        if(_longstr.size() > MAX_HEX_DIGITS)
            _ctx.reportDiagnostic(DiagnosticsId::DI_HEX_TOO_MANY_DIGITS, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
        if(_longstr.size() == 0)
            _ctx.reportDiagnostic(DiagnosticsId::DI_HEX_DIGITS_EXPECTED, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
    }
    else {
        APPEND_CHAR((SQChar)firstchar);
        bool hasExp = false;
        bool hasDot = false;
        while (CUR_CHAR == _SC('.') || isalnum(CUR_CHAR)) {
            if(CUR_CHAR == _SC('.')) {
                if(hasDot || hasExp)
                    _ctx.reportDiagnostic(DiagnosticsId::DI_MALFORMED_NUMBER, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
                hasDot = true;
                type = TFLOAT;
            }
            else if(isexponent(CUR_CHAR)) {
                if(hasExp)
                    _ctx.reportDiagnostic(DiagnosticsId::DI_MALFORMED_NUMBER, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
                hasExp = true;
                type = TSCIENTIFIC;
                APPEND_CHAR(CUR_CHAR);
                NEXT();
                if(CUR_CHAR == '+' || CUR_CHAR == '-'){
                    APPEND_CHAR(CUR_CHAR);
                    NEXT();
                }
                if(!isdigit(CUR_CHAR))
                    _ctx.reportDiagnostic(DiagnosticsId::DI_FP_EXP_EXPECTED, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
            }
            if(!isdigit(CUR_CHAR) && !isexponent(CUR_CHAR) && CUR_CHAR != '.')
                _ctx.reportDiagnostic(DiagnosticsId::DI_MALFORMED_NUMBER, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);

            APPEND_CHAR(CUR_CHAR);
            NUM_NEXT();
        }
    }
    TERMINATE_BUFFER();
    switch(type) {
    case TSCIENTIFIC:
    case TFLOAT:
#if SQ_USE_STD_FROM_CHARS
        {
            auto ret = std::from_chars(&_longstr[0], &_longstr[0] + _longstr.size(), _fvalue);
            if (ret.ec == std::errc::result_out_of_range)
                _ctx.reportDiagnostic(_fvalue == 0 ? DiagnosticsId::DI_LITERAL_UNDERFLOW : DiagnosticsId::DI_LITERAL_OVERFLOW,
                    _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "float");

            for (const char * c = ret.ptr; c < &_longstr[0] + _longstr.size() - 1; ++c)
                if (*c != '0')
                {
                    _ctx.reportDiagnostic(DiagnosticsId::DI_MALFORMED_NUMBER, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn);
                    break;
                }
        }
#else
        value = (SQFloat)strtod(&_longstr[0], &sTemp);
        _fvalue = value;
        if(value == 0)
        {
            for(int i = 0; i < _longstr.size(); i++)
            {
                SQChar c = _longstr[i];
                if (!c || c == 'e' || c == 'E')
                    break;
                if (c != '.' && c != '0')
                    _ctx.reportDiagnostic(DiagnosticsId::DI_LITERAL_UNDERFLOW, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "float");
            }
        }
#endif

        if(sizeof(_fvalue) == sizeof(float))
        {
            if (_fvalue >= FLT_MAX)
                _ctx.reportDiagnostic(DiagnosticsId::DI_LITERAL_OVERFLOW, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "float");
        }
        else if(sizeof(_fvalue) == sizeof(double))
        {
            if (_fvalue >= DBL_MAX)
                _ctx.reportDiagnostic(DiagnosticsId::DI_LITERAL_OVERFLOW, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "float");
        }
        return TK_FLOAT;
    case TINT:
        if(!LexInteger(&_longstr[0],(SQUnsignedInteger *)&_nvalue))
            _ctx.reportDiagnostic(DiagnosticsId::DI_LITERAL_OVERFLOW, _tokenline, _tokencolumn, _currentcolumn - _tokencolumn, "integer");
        return TK_INTEGER;
    case THEX:
        LexHexadecimal(&_longstr[0],(SQUnsignedInteger *)&_nvalue);
        return TK_INTEGER;
    }
    return 0;
}

SQInteger SQLexer::ReadID()
{
    SQInteger res;
    INIT_TEMP_STRING();
    do {
        APPEND_CHAR(CUR_CHAR);
        NEXT();
    } while(isalnum(CUR_CHAR) || CUR_CHAR == _SC('_'));
    TERMINATE_BUFFER();
    res = GetIDType(&_longstr[0],_longstr.size() - 1);
    if(res == TK_IDENTIFIER || res == TK_CONSTRUCTOR) {
        SetStringValue();
    }
    return res;
}

SQInteger SQLexer::ReadDirective()
{
    INIT_TEMP_STRING();
    do {
        APPEND_CHAR(CUR_CHAR);
        NEXT();
    } while(isalnum(CUR_CHAR) || CUR_CHAR == _SC('_') || CUR_CHAR == _SC('-') || CUR_CHAR == _SC(':'));
    TERMINATE_BUFFER();
    if (!_longstr[0])
        return -1;
    SetStringValue();
    return TK_DIRECTIVE;
}
