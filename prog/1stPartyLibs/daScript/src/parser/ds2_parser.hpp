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

    #include "parser_impl.h"

    LineInfo tokAt ( yyscan_t scanner, const struct DAS2_YYLTYPE & li );
    LineInfo tokRangeAt ( yyscan_t scanner, const struct DAS2_YYLTYPE & li, const struct DAS2_YYLTYPE & lie );

    struct TypePair {
        TypeDecl * firstType;
        TypeDecl * secondType;
    };

    struct EnumPair {
        string name;
        ExpressionPtr expr = nullptr;
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
    NOTDOT = 409,                  /* "!."  */
    NOTQDOT = 410,                 /* "!?."  */
    NOTBRA = 411,                  /* "!["  */
    NOTQBRA = 412,                 /* "!?["  */
    NOTQQ = 413,                   /* "!??"  */
    NOTEQUEQU = 414,               /* "!=="  */
    NOTLARROW = 415,               /* "!<-"  */
    NOTCLONEEQU = 416,             /* "!:="  */
    NOTIS = 417,                   /* "!is"  */
    NOTAS = 418,                   /* "!as"  */
    NOTQAS = 419,                  /* "!?as"  */
    LPIPE = 420,                   /* "<|"  */
    RPIPE = 421,                   /* "|>"  */
    CLONEEQU = 422,                /* ":="  */
    ROTL = 423,                    /* "<<<"  */
    ROTR = 424,                    /* ">>>"  */
    ROTLEQU = 425,                 /* "<<<="  */
    ROTREQU = 426,                 /* ">>>="  */
    MAPTO = 427,                   /* "=>"  */
    DOUBLE_AT = 428,               /* "@@"  */
    AT_FIELD = 429,                /* "@field"  */
    COLCOL = 430,                  /* "::"  */
    ANDAND = 431,                  /* "&&"  */
    OROR = 432,                    /* "||"  */
    XORXOR = 433,                  /* "^^"  */
    ANDANDEQU = 434,               /* "&&="  */
    OROREQU = 435,                 /* "||="  */
    XORXOREQU = 436,               /* "^^="  */
    DOTDOT = 437,                  /* ".."  */
    MTAG_E = 438,                  /* "$$"  */
    MTAG_I = 439,                  /* "$i"  */
    MTAG_V = 440,                  /* "$v"  */
    MTAG_B = 441,                  /* "$b"  */
    MTAG_A = 442,                  /* "$a"  */
    MTAG_T = 443,                  /* "$t"  */
    MTAG_C = 444,                  /* "$c"  */
    MTAG_F = 445,                  /* "$f"  */
    MTAG_DOTDOTDOT = 446,          /* "..."  */
    INTEGER = 447,                 /* "integer constant"  */
    LONG_INTEGER = 448,            /* "long integer constant"  */
    UNSIGNED_INTEGER = 449,        /* "unsigned integer constant"  */
    UNSIGNED_LONG_INTEGER = 450,   /* "unsigned long integer constant"  */
    UNSIGNED_INT8 = 451,           /* "unsigned int8 constant"  */
    DAS_FLOAT = 452,               /* "floating point constant"  */
    DAS_FLOAT16_CONST = 453,       /* "float16 constant"  */
    DOUBLE = 454,                  /* "double constant"  */
    NAME = 455,                    /* "name"  */
    DAS_EMIT_COMMA = 456,          /* "new line, comma"  */
    DAS_EMIT_SEMICOLON = 457,      /* "new line, semicolon"  */
    BEGIN_STRING = 458,            /* "start of the string"  */
    STRING_CHARACTER = 459,        /* STRING_CHARACTER  */
    STRING_CHARACTER_ESC = 460,    /* STRING_CHARACTER_ESC  */
    END_STRING = 461,              /* "end of the string"  */
    BEGIN_STRING_EXPR = 462,       /* "{"  */
    END_STRING_EXPR = 463,         /* "}"  */
    END_OF_READ = 464,             /* "end of failed eader macro"  */
    UNARY_MINUS = 465,             /* UNARY_MINUS  */
    UNARY_PLUS = 466,              /* UNARY_PLUS  */
    PRE_INC = 467,                 /* PRE_INC  */
    PRE_DEC = 468,                 /* PRE_DEC  */
    LLPIPE = 469,                  /* LLPIPE  */
    POST_INC = 470,                /* POST_INC  */
    POST_DEC = 471,                /* POST_DEC  */
    DEREF = 472                    /* DEREF  */
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
