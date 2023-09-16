#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
  #pragma warning(disable: 4703)  // potentially uninitialized local pointer variable used.
  #pragma warning(disable: 4701)  // potentially uninitialized local variable used.
  #pragma warning(push)
#endif
//to build: devtools\re2c-1.1.1\re2c.exe -T parseExpressionLex.re > parseExpressionLex.h

/*!re2c re2c:define:YYCTYPE = "char"; */
namespace expression_parser
{

template<int base>
static bool adddgt(unsigned int &u, unsigned int  d)
{
    if (u > (ULONG_MAX - d) / base) {
        return false;
    }
    u = u * base + d;
    return true;
}

static bool lex_dec(const char *s, const char *e, unsigned int &u)
{
    for (u = 0; s < e; ++s) {
        if (!adddgt<10>(u, (unsigned char)(*s) - 0x30u)) {
            return false;
        }
    }
    //printf("DEC %d ", u);
    return true;
}

static bool lex_flt(const char *s, float &d)
{
    d = 0;
    float x = 1;
    int e = 0;
    /*!re2c
        re2c:yyfill:enable = 0;
        re2c:define:YYCURSOR = s;
    */
mant_int:
    /*!re2c
        "."   { goto mant_frac; }
        [eE]  { goto exp_sign; }
        *     { d = (d * 10) + (s[-1] - '0'); goto mant_int; }
    */
mant_frac:
    /*!re2c
        ""    { goto sfx; }
        [eE]  { goto exp_sign; }
        [0-9] { d += (x /= 10) * (s[-1] - '0'); goto mant_frac; }
    */
exp_sign:
    /*!re2c
        "+"?  { x = 1e+1; goto exp; }
        "-"   { x = 1e-1; goto exp; }
    */
exp:
    /*!re2c
        ""    { for (; e > 0; --e) d *= x;    goto sfx; }
        [0-9] { e = (e * 10) + (s[-1] - '0'); goto exp; }
    */
sfx:
    /*!re2c
        *     { goto end; }
        [fF]  { if (d > FLT_MAX) return false; goto end; }
    */
end:
    return true;
}

enum TOKEN
{
  ERROR, END, COMMA, EFUN, ID, DEC, FLT, EID, BOOL,
  CMP_EQ,
  EQ, NE, LT, LE, GT, GE, AND, OR,//functions
  OPT
};

static const char *tokens[] =
{
  "error!", "eof", ",", ")", "ident", "int", "flt", "eid", "bool",
  "==", "eq", "ne", "lt", "le", "gt", "ge", "and", "or", "opt"
};

struct Token
{
  TOKEN token = ERROR;

  union
  {
    struct {const char *s,*e;} str;//for strings
    bool b;
    unsigned int u;
    int i;
    float f;
  } val;
  Token() = default;
  Token(TOKEN tok){token = tok;}
  Token(const char *s, const char *e){token = ID;val.str.s = s;val.str.e=e;}
  Token(bool b){token = BOOL;val.b = b;}
  operator bool () const {return token!=ERROR && token!=END;}
};

static Token lex(const char *&s)
{
    const char *YYMARKER;
    const char *ds, *es, *ee, *fs, *is;
    Token error;
    start:
    /*!stags:re2c format = 'const char *@@;'; */
    /*!re2c
        re2c:yyfill:enable = 0;
        re2c:define:YYCURSOR = s;

        end = "\x00";

        *   { return error; }
        end { return Token(END); }

        // whitespaces
        wsp = ([ \t\v\n\r])+;
        wsp { goto start; }

        // character and string literals
        // integer literals
        dec = [0-9]+;
        minus = "-" wsp? ;
        @ds dec { Token tok(DEC); if (!lex_dec(ds, s, tok.val.u)) return error; return tok;}
        minus @ds dec { Token tok(DEC); if (!lex_dec(ds, s, tok.val.u)) return error; tok.val.i = -tok.val.i;return tok;}
        @es dec @ee ":eid" { Token tok(EID); if (!lex_dec(es, ee, tok.val.u)) return error; return tok;}

        // floating literals
        frc = [0-9]* "." [0-9]+ | [0-9]+ ".";
        exp = 'e' [+-]? [0-9]+;
        flt = (frc exp? | [0-9]+ exp) [fFlL]?;
        flt {  Token tok(FLT); if (!lex_flt(s, tok.val.f)) return error; return tok; }
        minus @fs flt {Token tok(FLT); if (!lex_flt(fs, tok.val.f)) return error; tok.val.f = -tok.val.f;return tok;}

        // boolean literals
        "false" {return Token(false);}
        "true"  {return Token(true);}

        // operators and punctuation (including preprocessor)
        ")"               {return Token(EFUN);}
        ","               {return Token(COMMA);}

        // identifiers
        id = [a-zA-Z_][a-zA-Z_0-9\.]*;
        @is id { return Token{is, s};}
        cmpeq = "==";
        eq = "eq" wsp? "(";
        ne = "ne" wsp? "(";
        lt = "lt" wsp? "(";
        le = "le" wsp? "(";
        gt = "gt" wsp? "(";
        ge = "ge" wsp? "(";
        and = "and" wsp? "(";
        or = "or" wsp? "(";
        opt = "opt" wsp? "(";
        eq {return Token(EQ);}
        cmpeq {return Token(CMP_EQ);}
        ne {return Token(NE);}
        lt {return Token(LT);}
        le {return Token(LE);}
        gt {return Token(GT);}
        ge {return Token(GE);}
        and {return Token(AND);}
        or {return Token(OR);}
        opt {return Token(OPT);}
    */
}

}//namespace
#ifdef _MSC_VER
  #pragma warning(pop)
#endif
