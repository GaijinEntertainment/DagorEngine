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

#ifndef YY_DAS2_YY_DS2_PARSER_HPP_INCLUDED
# define YY_DAS2_YY_DS2_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef DAS2_YYDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define DAS2_YYDEBUG 1
#  else
#   define DAS2_YYDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define DAS2_YYDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined DAS2_YYDEBUG */
#if DAS2_YYDEBUG
extern int das2_yydebug;
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

    LineInfo tokAt ( yyscan_t scanner, const struct DAS2_YYLTYPE & li );
    LineInfo tokRangeAt ( yyscan_t scanner, const struct DAS2_YYLTYPE & li, const struct DAS2_YYLTYPE & lie );

    struct TypePair {
        TypeDecl * firstType;
        TypeDecl * secondType;
    };

    struct EnumPair {
        string name;
        ExpressionPtr expr;
        LineInfo at;
        EnumPair ( string * n, Expression * e, const LineInfo & l )
            : name(*n), expr(e), at(l) {};
        EnumPair ( string * n, const LineInfo & l )
            : name(*n), expr(nullptr), at(l) {};
    };


/* Token kinds.  */
#ifndef DAS2_YYTOKENTYPE
# define DAS2_YYTOKENTYPE
  enum das2_yytokentype
  {
    DAS2_YYEMPTY = -2,
    DAS2_YYEOF = 0,                /* "end of file"  */
    DAS2_YYerror = 256,            /* error  */
    DAS2_YYUNDEF = 257,            /* "invalid token"  */
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
    ADDEQU = 362,                  /* "+="  */
    SUBEQU = 363,                  /* "-="  */
    DIVEQU = 364,                  /* "/="  */
    MULEQU = 365,                  /* "*="  */
    MODEQU = 366,                  /* "%="  */
    ANDEQU = 367,                  /* "&="  */
    OREQU = 368,                   /* "|="  */
    XOREQU = 369,                  /* "^="  */
    SHL = 370,                     /* "<<"  */
    SHR = 371,                     /* ">>"  */
    ADDADD = 372,                  /* "++"  */
    SUBSUB = 373,                  /* "--"  */
    LEEQU = 374,                   /* "<="  */
    SHLEQU = 375,                  /* "<<="  */
    SHREQU = 376,                  /* ">>="  */
    GREQU = 377,                   /* ">="  */
    EQUEQU = 378,                  /* "=="  */
    NOTEQU = 379,                  /* "!="  */
    RARROW = 380,                  /* "->"  */
    LARROW = 381,                  /* "<-"  */
    QQ = 382,                      /* "??"  */
    QDOT = 383,                    /* "?."  */
    QBRA = 384,                    /* "?["  */
    LPIPE = 385,                   /* "<|"  */
    RPIPE = 386,                   /* "|>"  */
    CLONEEQU = 387,                /* ":="  */
    ROTL = 388,                    /* "<<<"  */
    ROTR = 389,                    /* ">>>"  */
    ROTLEQU = 390,                 /* "<<<="  */
    ROTREQU = 391,                 /* ">>>="  */
    MAPTO = 392,                   /* "=>"  */
    COLCOL = 393,                  /* "::"  */
    ANDAND = 394,                  /* "&&"  */
    OROR = 395,                    /* "||"  */
    XORXOR = 396,                  /* "^^"  */
    ANDANDEQU = 397,               /* "&&="  */
    OROREQU = 398,                 /* "||="  */
    XORXOREQU = 399,               /* "^^="  */
    DOTDOT = 400,                  /* ".."  */
    MTAG_E = 401,                  /* "$$"  */
    MTAG_I = 402,                  /* "$i"  */
    MTAG_V = 403,                  /* "$v"  */
    MTAG_B = 404,                  /* "$b"  */
    MTAG_A = 405,                  /* "$a"  */
    MTAG_T = 406,                  /* "$t"  */
    MTAG_C = 407,                  /* "$c"  */
    MTAG_F = 408,                  /* "$f"  */
    MTAG_DOTDOTDOT = 409,          /* "..."  */
    INTEGER = 410,                 /* "integer constant"  */
    LONG_INTEGER = 411,            /* "long integer constant"  */
    UNSIGNED_INTEGER = 412,        /* "unsigned integer constant"  */
    UNSIGNED_LONG_INTEGER = 413,   /* "unsigned long integer constant"  */
    UNSIGNED_INT8 = 414,           /* "unsigned int8 constant"  */
    FLOAT = 415,                   /* "floating point constant"  */
    DOUBLE = 416,                  /* "double constant"  */
    NAME = 417,                    /* "name"  */
    BEGIN_STRING = 418,            /* "start of the string"  */
    STRING_CHARACTER = 419,        /* STRING_CHARACTER  */
    STRING_CHARACTER_ESC = 420,    /* STRING_CHARACTER_ESC  */
    END_STRING = 421,              /* "end of the string"  */
    BEGIN_STRING_EXPR = 422,       /* "{"  */
    END_STRING_EXPR = 423,         /* "}"  */
    END_OF_READ = 424,             /* "end of failed eader macro"  */
    SEMICOLON_CUR_CUR = 425,       /* ";}}"  */
    SEMICOLON_CUR_SQR = 426,       /* ";}]"  */
    SEMICOLON_SQR_SQR = 427,       /* ";]]"  */
    COMMA_SQR_SQR = 428,           /* ",]]"  */
    COMMA_CUR_SQR = 429,           /* ",}]"  */
    UNARY_MINUS = 430,             /* UNARY_MINUS  */
    UNARY_PLUS = 431,              /* UNARY_PLUS  */
    PRE_INC = 432,                 /* PRE_INC  */
    PRE_DEC = 433,                 /* PRE_DEC  */
    LLPIPE = 434,                  /* LLPIPE  */
    POST_INC = 435,                /* POST_INC  */
    POST_DEC = 436,                /* POST_DEC  */
    DEREF = 437                    /* DEREF  */
  };
  typedef enum das2_yytokentype das2_yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined DAS2_YYSTYPE && ! defined DAS2_YYSTYPE_IS_DECLARED
union DAS2_YYSTYPE
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
    EnumPair *                      pEnumPair;
    Structure *                     pStructure;
    Function *                      pFuncDecl;
    CaptureEntry *                  pCapt;
    vector<CaptureEntry> *          pCaptList;
    TypePair                        aTypePair;


};
typedef union DAS2_YYSTYPE DAS2_YYSTYPE;
# define DAS2_YYSTYPE_IS_TRIVIAL 1
# define DAS2_YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined DAS2_YYLTYPE && ! defined DAS2_YYLTYPE_IS_DECLARED
typedef struct DAS2_YYLTYPE DAS2_YYLTYPE;
struct DAS2_YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define DAS2_YYLTYPE_IS_DECLARED 1
# define DAS2_YYLTYPE_IS_TRIVIAL 1
#endif




int das2_yyparse (yyscan_t scanner);


#endif /* !YY_DAS2_YY_DS2_PARSER_HPP_INCLUDED  */
