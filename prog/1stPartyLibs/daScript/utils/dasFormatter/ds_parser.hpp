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
    #if !defined(_DEBUG)
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
    DAS_TFLOAT16 = 357,            /* "float16"  */
    DAS_THALF2 = 358,              /* "half2"  */
    DAS_THALF3 = 359,              /* "half3"  */
    DAS_THALF4 = 360,              /* "half4"  */
    DAS_THALF8 = 361,              /* "half8"  */
    DAS_TSHORT2 = 362,             /* "short2"  */
    DAS_TSHORT3 = 363,             /* "short3"  */
    DAS_TSHORT4 = 364,             /* "short4"  */
    DAS_TSHORT8 = 365,             /* "short8"  */
    DAS_TUSHORT2 = 366,            /* "ushort2"  */
    DAS_TUSHORT3 = 367,            /* "ushort3"  */
    DAS_TUSHORT4 = 368,            /* "ushort4"  */
    DAS_TUSHORT8 = 369,            /* "ushort8"  */
    DAS_TBYTE2 = 370,              /* "byte2"  */
    DAS_TBYTE3 = 371,              /* "byte3"  */
    DAS_TBYTE4 = 372,              /* "byte4"  */
    DAS_TBYTE8 = 373,              /* "byte8"  */
    DAS_TBYTE16 = 374,             /* "byte16"  */
    DAS_TUBYTE2 = 375,             /* "ubyte2"  */
    DAS_TUBYTE3 = 376,             /* "ubyte3"  */
    DAS_TUBYTE4 = 377,             /* "ubyte4"  */
    DAS_TUBYTE8 = 378,             /* "ubyte8"  */
    DAS_TUBYTE16 = 379,            /* "ubyte16"  */
    DAS_TTUPLE = 380,              /* "tuple"  */
    DAS_TVARIANT = 381,            /* "variant"  */
    DAS_GENERATOR = 382,           /* "generator"  */
    DAS_YIELD = 383,               /* "yield"  */
    DAS_SEALED = 384,              /* "sealed"  */
    DAS_TEMPLATE = 385,            /* "template"  */
    ADDEQU = 386,                  /* "+="  */
    SUBEQU = 387,                  /* "-="  */
    DIVEQU = 388,                  /* "/="  */
    MULEQU = 389,                  /* "*="  */
    MODEQU = 390,                  /* "%="  */
    ANDEQU = 391,                  /* "&="  */
    OREQU = 392,                   /* "|="  */
    XOREQU = 393,                  /* "^="  */
    SHL = 394,                     /* "<<"  */
    SHR = 395,                     /* ">>"  */
    ADDADD = 396,                  /* "++"  */
    SUBSUB = 397,                  /* "--"  */
    LEEQU = 398,                   /* "<="  */
    SHLEQU = 399,                  /* "<<="  */
    SHREQU = 400,                  /* ">>="  */
    GREQU = 401,                   /* ">="  */
    EQUEQU = 402,                  /* "=="  */
    NOTEQU = 403,                  /* "!="  */
    RARROW = 404,                  /* "->"  */
    LARROW = 405,                  /* "<-"  */
    QQ = 406,                      /* "??"  */
    QDOT = 407,                    /* "?."  */
    QBRA = 408,                    /* "?["  */
    LPIPE = 409,                   /* "<|"  */
    LBPIPE = 410,                  /* " <|"  */
    LLPIPE = 411,                  /* "$ <|"  */
    LAPIPE = 412,                  /* "@ <|"  */
    LFPIPE = 413,                  /* "@@ <|"  */
    RPIPE = 414,                   /* "|>"  */
    CLONEEQU = 415,                /* ":="  */
    ROTL = 416,                    /* "<<<"  */
    ROTR = 417,                    /* ">>>"  */
    ROTLEQU = 418,                 /* "<<<="  */
    ROTREQU = 419,                 /* ">>>="  */
    MAPTO = 420,                   /* "=>"  */
    COLCOL = 421,                  /* "::"  */
    ANDAND = 422,                  /* "&&"  */
    OROR = 423,                    /* "||"  */
    XORXOR = 424,                  /* "^^"  */
    ANDANDEQU = 425,               /* "&&="  */
    OROREQU = 426,                 /* "||="  */
    XORXOREQU = 427,               /* "^^="  */
    DOTDOT = 428,                  /* ".."  */
    MTAG_E = 429,                  /* "$$"  */
    MTAG_I = 430,                  /* "$i"  */
    MTAG_V = 431,                  /* "$v"  */
    MTAG_B = 432,                  /* "$b"  */
    MTAG_A = 433,                  /* "$a"  */
    MTAG_T = 434,                  /* "$t"  */
    MTAG_C = 435,                  /* "$c"  */
    MTAG_F = 436,                  /* "$f"  */
    MTAG_DOTDOTDOT = 437,          /* "..."  */
    BRABRAB = 438,                 /* "[["  */
    BRACBRB = 439,                 /* "[{"  */
    CBRCBRB = 440,                 /* "{{"  */
    OPEN_BRACE = 441,              /* "new scope"  */
    CLOSE_BRACE = 442,             /* "close scope"  */
    SEMICOLON = 443,               /* SEMICOLON  */
    INTEGER = 444,                 /* "integer constant"  */
    LONG_INTEGER = 445,            /* "long integer constant"  */
    UNSIGNED_INTEGER = 446,        /* "unsigned integer constant"  */
    UNSIGNED_LONG_INTEGER = 447,   /* "unsigned long integer constant"  */
    UNSIGNED_INT8 = 448,           /* "unsigned int8 constant"  */
    DAS_FLOAT = 449,               /* "floating point constant"  */
    DAS_FLOAT16_CONST = 450,       /* "float16 constant"  */
    DOUBLE = 451,                  /* "double constant"  */
    NAME = 452,                    /* "name"  */
    KEYWORD = 453,                 /* "keyword"  */
    TYPE_FUNCTION = 454,           /* "type function"  */
    BEGIN_STRING = 455,            /* "start of the string"  */
    STRING_CHARACTER = 456,        /* STRING_CHARACTER  */
    STRING_CHARACTER_ESC = 457,    /* STRING_CHARACTER_ESC  */
    END_STRING = 458,              /* "end of the string"  */
    BEGIN_STRING_EXPR = 459,       /* "{"  */
    END_STRING_EXPR = 460,         /* "}"  */
    END_OF_READ = 461,             /* "end of failed eader macro"  */
    SEMICOLON_CUR_CUR = 462,       /* ";}}"  */
    SEMICOLON_CUR_SQR = 463,       /* ";}]"  */
    SEMICOLON_SQR_SQR = 464,       /* ";]]"  */
    COMMA_SQR_SQR = 465,           /* ",]]"  */
    COMMA_CUR_SQR = 466,           /* ",}]"  */
    END_OF_EXPR = 467,             /* END_OF_EXPR  */
    EMPTY = 468,                   /* EMPTY  */
    UNARY_MINUS = 469,             /* UNARY_MINUS  */
    UNARY_PLUS = 470,              /* UNARY_PLUS  */
    PRE_INC = 471,                 /* PRE_INC  */
    PRE_DEC = 472,                 /* PRE_DEC  */
    POST_INC = 473,                /* POST_INC  */
    POST_DEC = 474,                /* POST_DEC  */
    DEREF = 475                    /* DEREF  */
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
    vector<tuple<string,Expression *>> *  pNameExprList;
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
    Enumeration *                   pEnumList;
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
