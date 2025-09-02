/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_DAS_YY_DS_PARSER_HPP_INCLUDED
# define YY_DAS_YY_DS_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef DAS_YYDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define DAS_YYDEBUG 1
#  else
#   define DAS_YYDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define DAS_YYDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined DAS_YYDEBUG */
#if DAS_YYDEBUG
extern int das_yydebug;
#endif
/* "%code requires" blocks.  */

    #include "daScript/misc/platform.h"
    #include "daScript/ast/ast.h"
    #include "daScript/ast/ast_generate.h"
    #include "daScript/ast/ast_expressions.h"

    #include "../src/parser/parser_state.h"
    #include "helpers.h"
    #include "formatter.h"

#if defined(_MSC_VER) && !defined(__clang__)
    #if defined(DAS_RELWITHDEBINFO)
        #pragma optimize( "s", on )
    #endif
#endif

    enum {
        CorM_COPY   = 0,
        CorM_MOVE   = (1<<0),
        CorM_CLONE  = (1<<1)
    };

    enum {
        CorS_Struct = 0,
        CorS_Class  = 1,
        CorS_StructTemplate = 2,
        CorS_ClassTemplate  = 3
    };

    using namespace das;

    #include "../src/parser/parser_impl.h"

    LineInfo tokAt ( yyscan_t scanner, const struct DAS_YYLTYPE & li );
    LineInfo tokRangeAt ( yyscan_t scanner, const struct DAS_YYLTYPE & li, const struct DAS_YYLTYPE & lie );

    struct TypePair {
        TypeDecl * firstType;
        TypeDecl * secondType;
    };

    static bool need_wrap_current_expr = false;


/* Token kinds.  */
#ifndef DAS_YYTOKENTYPE
# define DAS_YYTOKENTYPE
  enum das_yytokentype
  {
    DAS_YYEMPTY = -2,
    DAS_YYEOF = 0,                 /* "end of file"  */
    DAS_YYerror = 256,             /* error  */
    DAS_YYUNDEF = 257,             /* "invalid token"  */
    LEXER_ERROR = 258,             /* "lexer error"  */
    DAS_CAPTURE = 259,             /* "capture"  */
    DAS_STRUCT = 260,              /* "struct"  */
    DAS_CLASS = 261,               /* "class"  */
    DAS_LET = 262,                 /* "let"  */
    DAS_DEF = 263,                 /* "def"  */
    DAS_WHILE = 264,               /* "while"  */
    DAS_IF = 265,                  /* "if"  */
    DAS_STATIC_IF = 266,           /* "static_if"  */
    DAS_ELSE = 267,                /* "else"  */
    DAS_FOR = 268,                 /* "for"  */
    DAS_CATCH = 269,               /* "recover"  */
    DAS_TRUE = 270,                /* "true"  */
    DAS_FALSE = 271,               /* "false"  */
    DAS_NEWT = 272,                /* "new"  */
    DAS_TYPEINFO = 273,            /* "typeinfo"  */
    DAS_TYPE = 274,                /* "type"  */
    DAS_IN = 275,                  /* "in"  */
    DAS_IS = 276,                  /* "is"  */
    DAS_AS = 277,                  /* "as"  */
    DAS_ELIF = 278,                /* "elif"  */
    DAS_STATIC_ELIF = 279,         /* "static_elif"  */
    DAS_ARRAY = 280,               /* "array"  */
    DAS_RETURN = 281,              /* "return"  */
    DAS_NULL = 282,                /* "null"  */
    DAS_BREAK = 283,               /* "break"  */
    DAS_TRY = 284,                 /* "try"  */
    DAS_OPTIONS = 285,             /* "options"  */
    DAS_TABLE = 286,               /* "table"  */
    DAS_EXPECT = 287,              /* "expect"  */
    DAS_CONST = 288,               /* "const"  */
    DAS_REQUIRE = 289,             /* "require"  */
    DAS_OPERATOR = 290,            /* "operator"  */
    DAS_ENUM = 291,                /* "enum"  */
    DAS_FINALLY = 292,             /* "finally"  */
    DAS_DELETE = 293,              /* "delete"  */
    DAS_DEREF = 294,               /* "deref"  */
    DAS_TYPEDEF = 295,             /* "typedef"  */
    DAS_TYPEDECL = 296,            /* "typedecl"  */
    DAS_WITH = 297,                /* "with"  */
    DAS_AKA = 298,                 /* "aka"  */
    DAS_ASSUME = 299,              /* "assume"  */
    DAS_CAST = 300,                /* "cast"  */
    DAS_OVERRIDE = 301,            /* "override"  */
    DAS_ABSTRACT = 302,            /* "abstract"  */
    DAS_UPCAST = 303,              /* "upcast"  */
    DAS_ITERATOR = 304,            /* "iterator"  */
    DAS_VAR = 305,                 /* "var"  */
    DAS_ADDR = 306,                /* "addr"  */
    DAS_CONTINUE = 307,            /* "continue"  */
    DAS_WHERE = 308,               /* "where"  */
    DAS_PASS = 309,                /* "pass"  */
    DAS_REINTERPRET = 310,         /* "reinterpret"  */
    DAS_MODULE = 311,              /* "module"  */
    DAS_PUBLIC = 312,              /* "public"  */
    DAS_LABEL = 313,               /* "label"  */
    DAS_GOTO = 314,                /* "goto"  */
    DAS_IMPLICIT = 315,            /* "implicit"  */
    DAS_EXPLICIT = 316,            /* "explicit"  */
    DAS_SHARED = 317,              /* "shared"  */
    DAS_PRIVATE = 318,             /* "private"  */
    DAS_SMART_PTR = 319,           /* "smart_ptr"  */
    DAS_UNSAFE = 320,              /* "unsafe"  */
    DAS_INSCOPE = 321,             /* "inscope"  */
    DAS_STATIC = 322,              /* "static"  */
    DAS_FIXED_ARRAY = 323,         /* "fixed_array"  */
    DAS_DEFAULT = 324,             /* "default"  */
    DAS_UNINITIALIZED = 325,       /* "uninitialized"  */
    DAS_TBOOL = 326,               /* "bool"  */
    DAS_TVOID = 327,               /* "void"  */
    DAS_TSTRING = 328,             /* "string"  */
    DAS_TAUTO = 329,               /* "auto"  */
    DAS_TINT = 330,                /* "int"  */
    DAS_TINT2 = 331,               /* "int2"  */
    DAS_TINT3 = 332,               /* "int3"  */
    DAS_TINT4 = 333,               /* "int4"  */
    DAS_TUINT = 334,               /* "uint"  */
    DAS_TBITFIELD = 335,           /* "bitfield"  */
    DAS_TUINT2 = 336,              /* "uint2"  */
    DAS_TUINT3 = 337,              /* "uint3"  */
    DAS_TUINT4 = 338,              /* "uint4"  */
    DAS_TFLOAT = 339,              /* "float"  */
    DAS_TFLOAT2 = 340,             /* "float2"  */
    DAS_TFLOAT3 = 341,             /* "float3"  */
    DAS_TFLOAT4 = 342,             /* "float4"  */
    DAS_TRANGE = 343,              /* "range"  */
    DAS_TURANGE = 344,             /* "urange"  */
    DAS_TRANGE64 = 345,            /* "range64"  */
    DAS_TURANGE64 = 346,           /* "urange64"  */
    DAS_TBLOCK = 347,              /* "block"  */
    DAS_TINT64 = 348,              /* "int64"  */
    DAS_TUINT64 = 349,             /* "uint64"  */
    DAS_TDOUBLE = 350,             /* "double"  */
    DAS_TFUNCTION = 351,           /* "function"  */
    DAS_TLAMBDA = 352,             /* "lambda"  */
    DAS_TINT8 = 353,               /* "int8"  */
    DAS_TUINT8 = 354,              /* "uint8"  */
    DAS_TINT16 = 355,              /* "int16"  */
    DAS_TUINT16 = 356,             /* "uint16"  */
    DAS_TTUPLE = 357,              /* "tuple"  */
    DAS_TVARIANT = 358,            /* "variant"  */
    DAS_GENERATOR = 359,           /* "generator"  */
    DAS_YIELD = 360,               /* "yield"  */
    DAS_SEALED = 361,              /* "sealed"  */
    DAS_TEMPLATE = 362,            /* "template"  */
    ADDEQU = 363,                  /* "+="  */
    SUBEQU = 364,                  /* "-="  */
    DIVEQU = 365,                  /* "/="  */
    MULEQU = 366,                  /* "*="  */
    MODEQU = 367,                  /* "%="  */
    ANDEQU = 368,                  /* "&="  */
    OREQU = 369,                   /* "|="  */
    XOREQU = 370,                  /* "^="  */
    SHL = 371,                     /* "<<"  */
    SHR = 372,                     /* ">>"  */
    ADDADD = 373,                  /* "++"  */
    SUBSUB = 374,                  /* "--"  */
    LEEQU = 375,                   /* "<="  */
    SHLEQU = 376,                  /* "<<="  */
    SHREQU = 377,                  /* ">>="  */
    GREQU = 378,                   /* ">="  */
    EQUEQU = 379,                  /* "=="  */
    NOTEQU = 380,                  /* "!="  */
    RARROW = 381,                  /* "->"  */
    LARROW = 382,                  /* "<-"  */
    QQ = 383,                      /* "??"  */
    QDOT = 384,                    /* "?."  */
    QBRA = 385,                    /* "?["  */
    LPIPE = 386,                   /* "<|"  */
    LBPIPE = 387,                  /* " <|"  */
    LLPIPE = 388,                  /* "$ <|"  */
    LAPIPE = 389,                  /* "@ <|"  */
    LFPIPE = 390,                  /* "@@ <|"  */
    RPIPE = 391,                   /* "|>"  */
    CLONEEQU = 392,                /* ":="  */
    ROTL = 393,                    /* "<<<"  */
    ROTR = 394,                    /* ">>>"  */
    ROTLEQU = 395,                 /* "<<<="  */
    ROTREQU = 396,                 /* ">>>="  */
    MAPTO = 397,                   /* "=>"  */
    COLCOL = 398,                  /* "::"  */
    ANDAND = 399,                  /* "&&"  */
    OROR = 400,                    /* "||"  */
    XORXOR = 401,                  /* "^^"  */
    ANDANDEQU = 402,               /* "&&="  */
    OROREQU = 403,                 /* "||="  */
    XORXOREQU = 404,               /* "^^="  */
    DOTDOT = 405,                  /* ".."  */
    MTAG_E = 406,                  /* "$$"  */
    MTAG_I = 407,                  /* "$i"  */
    MTAG_V = 408,                  /* "$v"  */
    MTAG_B = 409,                  /* "$b"  */
    MTAG_A = 410,                  /* "$a"  */
    MTAG_T = 411,                  /* "$t"  */
    MTAG_C = 412,                  /* "$c"  */
    MTAG_F = 413,                  /* "$f"  */
    MTAG_DOTDOTDOT = 414,          /* "..."  */
    BRABRAB = 415,                 /* "[["  */
    BRACBRB = 416,                 /* "[{"  */
    CBRCBRB = 417,                 /* "{{"  */
    OPEN_BRACE = 418,              /* "new scope"  */
    CLOSE_BRACE = 419,             /* "close scope"  */
    SEMICOLON = 420,               /* SEMICOLON  */
    INTEGER = 421,                 /* "integer constant"  */
    LONG_INTEGER = 422,            /* "long integer constant"  */
    UNSIGNED_INTEGER = 423,        /* "unsigned integer constant"  */
    UNSIGNED_LONG_INTEGER = 424,   /* "unsigned long integer constant"  */
    UNSIGNED_INT8 = 425,           /* "unsigned int8 constant"  */
    DAS_FLOAT = 426,               /* "floating point constant"  */
    DOUBLE = 427,                  /* "double constant"  */
    NAME = 428,                    /* "name"  */
    KEYWORD = 429,                 /* "keyword"  */
    TYPE_FUNCTION = 430,           /* "type function"  */
    BEGIN_STRING = 431,            /* "start of the string"  */
    STRING_CHARACTER = 432,        /* STRING_CHARACTER  */
    STRING_CHARACTER_ESC = 433,    /* STRING_CHARACTER_ESC  */
    END_STRING = 434,              /* "end of the string"  */
    BEGIN_STRING_EXPR = 435,       /* "{"  */
    END_STRING_EXPR = 436,         /* "}"  */
    END_OF_READ = 437,             /* "end of failed eader macro"  */
    SEMICOLON_CUR_CUR = 438,       /* ";}}"  */
    SEMICOLON_CUR_SQR = 439,       /* ";}]"  */
    SEMICOLON_SQR_SQR = 440,       /* ";]]"  */
    COMMA_SQR_SQR = 441,           /* ",]]"  */
    COMMA_CUR_SQR = 442,           /* ",}]"  */
    END_OF_EXPR = 443,             /* END_OF_EXPR  */
    EMPTY = 444,                   /* EMPTY  */
    UNARY_MINUS = 445,             /* UNARY_MINUS  */
    UNARY_PLUS = 446,              /* UNARY_PLUS  */
    PRE_INC = 447,                 /* PRE_INC  */
    PRE_DEC = 448,                 /* PRE_DEC  */
    POST_INC = 449,                /* POST_INC  */
    POST_DEC = 450,                /* POST_DEC  */
    DEREF = 451                    /* DEREF  */
  };
  typedef enum das_yytokentype das_yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined DAS_YYSTYPE && ! defined DAS_YYSTYPE_IS_DECLARED
union DAS_YYSTYPE
{

    char                            ch;
    bool                            b;
    int32_t                         i;
    uint32_t                        ui;
    int64_t                         i64;
    uint64_t                        ui64;
    double                          d;
    double                          fd;
    string *                        s;
    vector<string> *                pNameList;
    vector<VariableNameAndPosition> * pNameWithPosList;
    VariableDeclaration *           pVarDecl;
    vector<VariableDeclaration*> *  pVarDeclList;
    TypeDecl *                      pTypeDecl;
    vector<Expression *> *          pTypeDeclList;
    Expression *                    pExpression;
    Type                            type;
    AnnotationArgument *            aa;
    AnnotationArgumentList *        aaList;
    AnnotationDeclaration *         fa;
    AnnotationList *                faList;
    MakeStruct *                    pMakeStruct;
    Enumeration *                   pEnum;
    Structure *                     pStructure;
    Function *                      pFuncDecl;
    CaptureEntry *                  pCapt;
    vector<CaptureEntry> *          pCaptList;
    TypePair                        aTypePair;
    vector<LineInfo> *              positions;


};
typedef union DAS_YYSTYPE DAS_YYSTYPE;
# define DAS_YYSTYPE_IS_TRIVIAL 1
# define DAS_YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined DAS_YYLTYPE && ! defined DAS_YYLTYPE_IS_DECLARED
typedef struct DAS_YYLTYPE DAS_YYLTYPE;
struct DAS_YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define DAS_YYLTYPE_IS_DECLARED 1
# define DAS_YYLTYPE_IS_TRIVIAL 1
#endif




int das_yyparse (yyscan_t scanner);


#endif /* !YY_DAS_YY_DS_PARSER_HPP_INCLUDED  */
