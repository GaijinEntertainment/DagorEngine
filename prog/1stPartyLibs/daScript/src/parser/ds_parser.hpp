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

    #include "parser_state.h"

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

    using namespace das;

    #include "parser_impl.h"

    LineInfo tokAt ( yyscan_t scanner, const struct DAS_YYLTYPE & li );
    LineInfo tokRangeAt ( yyscan_t scanner, const struct DAS_YYLTYPE & li, const struct DAS_YYLTYPE & lie );

    struct TypePair {
        TypeDecl * firstType;
        TypeDecl * secondType;
    };


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
    DAS_STRUCT = 259,              /* "struct"  */
    DAS_CLASS = 260,               /* "class"  */
    DAS_LET = 261,                 /* "let"  */
    DAS_DEF = 262,                 /* "def"  */
    DAS_WHILE = 263,               /* "while"  */
    DAS_IF = 264,                  /* "if"  */
    DAS_STATIC_IF = 265,           /* "static_if"  */
    DAS_ELSE = 266,                /* "else"  */
    DAS_FOR = 267,                 /* "for"  */
    DAS_CATCH = 268,               /* "recover"  */
    DAS_TRUE = 269,                /* "true"  */
    DAS_FALSE = 270,               /* "false"  */
    DAS_NEWT = 271,                /* "new"  */
    DAS_TYPEINFO = 272,            /* "typeinfo"  */
    DAS_TYPE = 273,                /* "type"  */
    DAS_IN = 274,                  /* "in"  */
    DAS_IS = 275,                  /* "is"  */
    DAS_AS = 276,                  /* "as"  */
    DAS_ELIF = 277,                /* "elif"  */
    DAS_STATIC_ELIF = 278,         /* "static_elif"  */
    DAS_ARRAY = 279,               /* "array"  */
    DAS_RETURN = 280,              /* "return"  */
    DAS_NULL = 281,                /* "null"  */
    DAS_BREAK = 282,               /* "break"  */
    DAS_TRY = 283,                 /* "try"  */
    DAS_OPTIONS = 284,             /* "options"  */
    DAS_TABLE = 285,               /* "table"  */
    DAS_EXPECT = 286,              /* "expect"  */
    DAS_CONST = 287,               /* "const"  */
    DAS_REQUIRE = 288,             /* "require"  */
    DAS_OPERATOR = 289,            /* "operator"  */
    DAS_ENUM = 290,                /* "enum"  */
    DAS_FINALLY = 291,             /* "finally"  */
    DAS_DELETE = 292,              /* "delete"  */
    DAS_DEREF = 293,               /* "deref"  */
    DAS_TYPEDEF = 294,             /* "typedef"  */
    DAS_WITH = 295,                /* "with"  */
    DAS_AKA = 296,                 /* "aka"  */
    DAS_ASSUME = 297,              /* "assume"  */
    DAS_CAST = 298,                /* "cast"  */
    DAS_OVERRIDE = 299,            /* "override"  */
    DAS_ABSTRACT = 300,            /* "abstract"  */
    DAS_UPCAST = 301,              /* "upcast"  */
    DAS_ITERATOR = 302,            /* "iterator"  */
    DAS_VAR = 303,                 /* "var"  */
    DAS_ADDR = 304,                /* "addr"  */
    DAS_CONTINUE = 305,            /* "continue"  */
    DAS_WHERE = 306,               /* "where"  */
    DAS_PASS = 307,                /* "pass"  */
    DAS_REINTERPRET = 308,         /* "reinterpret"  */
    DAS_MODULE = 309,              /* "module"  */
    DAS_PUBLIC = 310,              /* "public"  */
    DAS_LABEL = 311,               /* "label"  */
    DAS_GOTO = 312,                /* "goto"  */
    DAS_IMPLICIT = 313,            /* "implicit"  */
    DAS_EXPLICIT = 314,            /* "explicit"  */
    DAS_SHARED = 315,              /* "shared"  */
    DAS_PRIVATE = 316,             /* "private"  */
    DAS_SMART_PTR = 317,           /* "smart_ptr"  */
    DAS_UNSAFE = 318,              /* "unsafe"  */
    DAS_INSCOPE = 319,             /* "inscope"  */
    DAS_STATIC = 320,              /* "static"  */
    DAS_TBOOL = 321,               /* "bool"  */
    DAS_TVOID = 322,               /* "void"  */
    DAS_TSTRING = 323,             /* "string"  */
    DAS_TAUTO = 324,               /* "auto"  */
    DAS_TINT = 325,                /* "int"  */
    DAS_TINT2 = 326,               /* "int2"  */
    DAS_TINT3 = 327,               /* "int3"  */
    DAS_TINT4 = 328,               /* "int4"  */
    DAS_TUINT = 329,               /* "uint"  */
    DAS_TBITFIELD = 330,           /* "bitfield"  */
    DAS_TUINT2 = 331,              /* "uint2"  */
    DAS_TUINT3 = 332,              /* "uint3"  */
    DAS_TUINT4 = 333,              /* "uint4"  */
    DAS_TFLOAT = 334,              /* "float"  */
    DAS_TFLOAT2 = 335,             /* "float2"  */
    DAS_TFLOAT3 = 336,             /* "float3"  */
    DAS_TFLOAT4 = 337,             /* "float4"  */
    DAS_TRANGE = 338,              /* "range"  */
    DAS_TURANGE = 339,             /* "urange"  */
    DAS_TRANGE64 = 340,            /* "range64"  */
    DAS_TURANGE64 = 341,           /* "urange64"  */
    DAS_TBLOCK = 342,              /* "block"  */
    DAS_TINT64 = 343,              /* "int64"  */
    DAS_TUINT64 = 344,             /* "uint64"  */
    DAS_TDOUBLE = 345,             /* "double"  */
    DAS_TFUNCTION = 346,           /* "function"  */
    DAS_TLAMBDA = 347,             /* "lambda"  */
    DAS_TINT8 = 348,               /* "int8"  */
    DAS_TUINT8 = 349,              /* "uint8"  */
    DAS_TINT16 = 350,              /* "int16"  */
    DAS_TUINT16 = 351,             /* "uint16"  */
    DAS_TTUPLE = 352,              /* "tuple"  */
    DAS_TVARIANT = 353,            /* "variant"  */
    DAS_GENERATOR = 354,           /* "generator"  */
    DAS_YIELD = 355,               /* "yield"  */
    DAS_SEALED = 356,              /* "sealed"  */
    ADDEQU = 357,                  /* "+="  */
    SUBEQU = 358,                  /* "-="  */
    DIVEQU = 359,                  /* "/="  */
    MULEQU = 360,                  /* "*="  */
    MODEQU = 361,                  /* "%="  */
    ANDEQU = 362,                  /* "&="  */
    OREQU = 363,                   /* "|="  */
    XOREQU = 364,                  /* "^="  */
    SHL = 365,                     /* "<<"  */
    SHR = 366,                     /* ">>"  */
    ADDADD = 367,                  /* "++"  */
    SUBSUB = 368,                  /* "--"  */
    LEEQU = 369,                   /* "<="  */
    SHLEQU = 370,                  /* "<<="  */
    SHREQU = 371,                  /* ">>="  */
    GREQU = 372,                   /* ">="  */
    EQUEQU = 373,                  /* "=="  */
    NOTEQU = 374,                  /* "!="  */
    RARROW = 375,                  /* "->"  */
    LARROW = 376,                  /* "<-"  */
    QQ = 377,                      /* "??"  */
    QDOT = 378,                    /* "?."  */
    QBRA = 379,                    /* "?["  */
    LPIPE = 380,                   /* "<|"  */
    LBPIPE = 381,                  /* " <|"  */
    LLPIPE = 382,                  /* "$ <|"  */
    LAPIPE = 383,                  /* "@ <|"  */
    LFPIPE = 384,                  /* "@@ <|"  */
    RPIPE = 385,                   /* "|>"  */
    CLONEEQU = 386,                /* ":="  */
    ROTL = 387,                    /* "<<<"  */
    ROTR = 388,                    /* ">>>"  */
    ROTLEQU = 389,                 /* "<<<="  */
    ROTREQU = 390,                 /* ">>>="  */
    MAPTO = 391,                   /* "=>"  */
    COLCOL = 392,                  /* "::"  */
    ANDAND = 393,                  /* "&&"  */
    OROR = 394,                    /* "||"  */
    XORXOR = 395,                  /* "^^"  */
    ANDANDEQU = 396,               /* "&&="  */
    OROREQU = 397,                 /* "||="  */
    XORXOREQU = 398,               /* "^^="  */
    DOTDOT = 399,                  /* ".."  */
    MTAG_E = 400,                  /* "$$"  */
    MTAG_I = 401,                  /* "$i"  */
    MTAG_V = 402,                  /* "$v"  */
    MTAG_B = 403,                  /* "$b"  */
    MTAG_A = 404,                  /* "$a"  */
    MTAG_T = 405,                  /* "$t"  */
    MTAG_C = 406,                  /* "$c"  */
    MTAG_F = 407,                  /* "$f"  */
    MTAG_DOTDOTDOT = 408,          /* "..."  */
    BRABRAB = 409,                 /* "[["  */
    BRACBRB = 410,                 /* "[{"  */
    CBRCBRB = 411,                 /* "{{"  */
    INTEGER = 412,                 /* "integer constant"  */
    LONG_INTEGER = 413,            /* "long integer constant"  */
    UNSIGNED_INTEGER = 414,        /* "unsigned integer constant"  */
    UNSIGNED_LONG_INTEGER = 415,   /* "unsigned long integer constant"  */
    UNSIGNED_INT8 = 416,           /* "unsigned int8 constant"  */
    FLOAT = 417,                   /* "floating point constant"  */
    DOUBLE = 418,                  /* "double constant"  */
    NAME = 419,                    /* "name"  */
    KEYWORD = 420,                 /* "keyword"  */
    BEGIN_STRING = 421,            /* "start of the string"  */
    STRING_CHARACTER = 422,        /* STRING_CHARACTER  */
    STRING_CHARACTER_ESC = 423,    /* STRING_CHARACTER_ESC  */
    END_STRING = 424,              /* "end of the string"  */
    BEGIN_STRING_EXPR = 425,       /* "{"  */
    END_STRING_EXPR = 426,         /* "}"  */
    END_OF_READ = 427,             /* "end of failed eader macro"  */
    SEMICOLON_CUR_CUR = 428,       /* ";}}"  */
    SEMICOLON_CUR_SQR = 429,       /* ";}]"  */
    SEMICOLON_SQR_SQR = 430,       /* ";]]"  */
    COMMA_SQR_SQR = 431,           /* ",]]"  */
    COMMA_CUR_SQR = 432,           /* ",}]"  */
    UNARY_MINUS = 433,             /* UNARY_MINUS  */
    UNARY_PLUS = 434,              /* UNARY_PLUS  */
    PRE_INC = 435,                 /* PRE_INC  */
    PRE_DEC = 436,                 /* PRE_DEC  */
    POST_INC = 437,                /* POST_INC  */
    POST_DEC = 438,                /* POST_DEC  */
    DEREF = 439                    /* DEREF  */
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
