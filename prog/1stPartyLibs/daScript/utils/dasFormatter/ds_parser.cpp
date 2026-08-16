/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         DAS_YYSTYPE
#define YYLTYPE         DAS_YYLTYPE
/* Substitute the variable and function names.  */
#define yyparse         das_yyparse
#define yylex           das_yylex
#define yyerror         das_yyerror
#define yydebug         das_yydebug
#define yynerrs         das_yynerrs

/* First part of user prologue.  */

    #define das_yyparse fmt_yyparse // We can't change flex prefix, since we have only one lexer file, seems like to define is the best way

    #include "daScript/misc/platform.h"
    #include "daScript/simulate/debug_info.h"
    #include "daScript/ast/compilation_errors.h"

    #ifdef _MSC_VER
    #pragma warning(disable:4262)
    #pragma warning(disable:4127)
    #pragma warning(disable:4702)
    #endif

    using namespace das;

    union DAS_YYSTYPE;
    struct DAS_YYLTYPE;

    #define YY_NO_UNISTD_H
    #include "../src/parser/lex.yy.h"

    void das_yyerror ( DAS_YYLTYPE * lloc, yyscan_t scanner, const string & error );
    void das_yyfatalerror ( DAS_YYLTYPE * lloc, yyscan_t scanner, const string & error, CompilationError cerr );
    int yylex ( DAS_YYSTYPE *lvalp, DAS_YYLTYPE *llocp, yyscan_t scanner );
    void yybegin ( const char * str );

    void das_yybegin_reader ( yyscan_t yyscanner );
    void das_yyend_reader ( yyscan_t yyscanner );
    void das_accept_sequence ( yyscan_t yyscanner, const char * seq, size_t seqLen, int lineNo, FileInfo * info );

    namespace das { class Module; }
    void das_collect_keywords ( das::Module * mod, yyscan_t yyscanner );
    void das_strfmt ( yyscan_t yyscanner );

    #undef yyextra
    #define yyextra (*((das::DasParserState **)(scanner)))


# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "ds_parser.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_LEXER_ERROR = 3,                /* "lexer error"  */
  YYSYMBOL_DAS_CAPTURE = 4,                /* "capture"  */
  YYSYMBOL_DAS_STRUCT = 5,                 /* "struct"  */
  YYSYMBOL_DAS_CLASS = 6,                  /* "class"  */
  YYSYMBOL_DAS_LET = 7,                    /* "let"  */
  YYSYMBOL_DAS_DEF = 8,                    /* "def"  */
  YYSYMBOL_DAS_WHILE = 9,                  /* "while"  */
  YYSYMBOL_DAS_IF = 10,                    /* "if"  */
  YYSYMBOL_DAS_STATIC_IF = 11,             /* "static_if"  */
  YYSYMBOL_DAS_ELSE = 12,                  /* "else"  */
  YYSYMBOL_DAS_FOR = 13,                   /* "for"  */
  YYSYMBOL_DAS_CATCH = 14,                 /* "recover"  */
  YYSYMBOL_DAS_TRUE = 15,                  /* "true"  */
  YYSYMBOL_DAS_FALSE = 16,                 /* "false"  */
  YYSYMBOL_DAS_NEWT = 17,                  /* "new"  */
  YYSYMBOL_DAS_TYPEINFO = 18,              /* "typeinfo"  */
  YYSYMBOL_DAS_TYPE = 19,                  /* "type"  */
  YYSYMBOL_DAS_IN = 20,                    /* "in"  */
  YYSYMBOL_DAS_IS = 21,                    /* "is"  */
  YYSYMBOL_DAS_AS = 22,                    /* "as"  */
  YYSYMBOL_DAS_ELIF = 23,                  /* "elif"  */
  YYSYMBOL_DAS_STATIC_ELIF = 24,           /* "static_elif"  */
  YYSYMBOL_DAS_ARRAY = 25,                 /* "array"  */
  YYSYMBOL_DAS_RETURN = 26,                /* "return"  */
  YYSYMBOL_DAS_NULL = 27,                  /* "null"  */
  YYSYMBOL_DAS_BREAK = 28,                 /* "break"  */
  YYSYMBOL_DAS_TRY = 29,                   /* "try"  */
  YYSYMBOL_DAS_OPTIONS = 30,               /* "options"  */
  YYSYMBOL_DAS_TABLE = 31,                 /* "table"  */
  YYSYMBOL_DAS_EXPECT = 32,                /* "expect"  */
  YYSYMBOL_DAS_CONST = 33,                 /* "const"  */
  YYSYMBOL_DAS_REQUIRE = 34,               /* "require"  */
  YYSYMBOL_DAS_OPERATOR = 35,              /* "operator"  */
  YYSYMBOL_DAS_ENUM = 36,                  /* "enum"  */
  YYSYMBOL_DAS_FINALLY = 37,               /* "finally"  */
  YYSYMBOL_DAS_DELETE = 38,                /* "delete"  */
  YYSYMBOL_DAS_DEREF = 39,                 /* "deref"  */
  YYSYMBOL_DAS_TYPEDEF = 40,               /* "typedef"  */
  YYSYMBOL_DAS_TYPEDECL = 41,              /* "typedecl"  */
  YYSYMBOL_DAS_WITH = 42,                  /* "with"  */
  YYSYMBOL_DAS_AKA = 43,                   /* "aka"  */
  YYSYMBOL_DAS_ASSUME = 44,                /* "assume"  */
  YYSYMBOL_DAS_CAST = 45,                  /* "cast"  */
  YYSYMBOL_DAS_OVERRIDE = 46,              /* "override"  */
  YYSYMBOL_DAS_ABSTRACT = 47,              /* "abstract"  */
  YYSYMBOL_DAS_UPCAST = 48,                /* "upcast"  */
  YYSYMBOL_DAS_ITERATOR = 49,              /* "iterator"  */
  YYSYMBOL_DAS_VAR = 50,                   /* "var"  */
  YYSYMBOL_DAS_ADDR = 51,                  /* "addr"  */
  YYSYMBOL_DAS_CONTINUE = 52,              /* "continue"  */
  YYSYMBOL_DAS_WHERE = 53,                 /* "where"  */
  YYSYMBOL_DAS_PASS = 54,                  /* "pass"  */
  YYSYMBOL_DAS_REINTERPRET = 55,           /* "reinterpret"  */
  YYSYMBOL_DAS_MODULE = 56,                /* "module"  */
  YYSYMBOL_DAS_PUBLIC = 57,                /* "public"  */
  YYSYMBOL_DAS_LABEL = 58,                 /* "label"  */
  YYSYMBOL_DAS_GOTO = 59,                  /* "goto"  */
  YYSYMBOL_DAS_IMPLICIT = 60,              /* "implicit"  */
  YYSYMBOL_DAS_EXPLICIT = 61,              /* "explicit"  */
  YYSYMBOL_DAS_SHARED = 62,                /* "shared"  */
  YYSYMBOL_DAS_PRIVATE = 63,               /* "private"  */
  YYSYMBOL_DAS_SMART_PTR = 64,             /* "smart_ptr"  */
  YYSYMBOL_DAS_UNSAFE = 65,                /* "unsafe"  */
  YYSYMBOL_DAS_INSCOPE = 66,               /* "inscope"  */
  YYSYMBOL_DAS_STATIC = 67,                /* "static"  */
  YYSYMBOL_DAS_FIXED_ARRAY = 68,           /* "fixed_array"  */
  YYSYMBOL_DAS_DEFAULT = 69,               /* "default"  */
  YYSYMBOL_DAS_UNINITIALIZED = 70,         /* "uninitialized"  */
  YYSYMBOL_DAS_TBOOL = 71,                 /* "bool"  */
  YYSYMBOL_DAS_TVOID = 72,                 /* "void"  */
  YYSYMBOL_DAS_TSTRING = 73,               /* "string"  */
  YYSYMBOL_DAS_TAUTO = 74,                 /* "auto"  */
  YYSYMBOL_DAS_TINT = 75,                  /* "int"  */
  YYSYMBOL_DAS_TINT2 = 76,                 /* "int2"  */
  YYSYMBOL_DAS_TINT3 = 77,                 /* "int3"  */
  YYSYMBOL_DAS_TINT4 = 78,                 /* "int4"  */
  YYSYMBOL_DAS_TUINT = 79,                 /* "uint"  */
  YYSYMBOL_DAS_TBITFIELD = 80,             /* "bitfield"  */
  YYSYMBOL_DAS_TUINT2 = 81,                /* "uint2"  */
  YYSYMBOL_DAS_TUINT3 = 82,                /* "uint3"  */
  YYSYMBOL_DAS_TUINT4 = 83,                /* "uint4"  */
  YYSYMBOL_DAS_TFLOAT = 84,                /* "float"  */
  YYSYMBOL_DAS_TFLOAT2 = 85,               /* "float2"  */
  YYSYMBOL_DAS_TFLOAT3 = 86,               /* "float3"  */
  YYSYMBOL_DAS_TFLOAT4 = 87,               /* "float4"  */
  YYSYMBOL_DAS_TRANGE = 88,                /* "range"  */
  YYSYMBOL_DAS_TURANGE = 89,               /* "urange"  */
  YYSYMBOL_DAS_TRANGE64 = 90,              /* "range64"  */
  YYSYMBOL_DAS_TURANGE64 = 91,             /* "urange64"  */
  YYSYMBOL_DAS_TBLOCK = 92,                /* "block"  */
  YYSYMBOL_DAS_TINT64 = 93,                /* "int64"  */
  YYSYMBOL_DAS_TUINT64 = 94,               /* "uint64"  */
  YYSYMBOL_DAS_TDOUBLE = 95,               /* "double"  */
  YYSYMBOL_DAS_TFUNCTION = 96,             /* "function"  */
  YYSYMBOL_DAS_TLAMBDA = 97,               /* "lambda"  */
  YYSYMBOL_DAS_TINT8 = 98,                 /* "int8"  */
  YYSYMBOL_DAS_TUINT8 = 99,                /* "uint8"  */
  YYSYMBOL_DAS_TINT16 = 100,               /* "int16"  */
  YYSYMBOL_DAS_TUINT16 = 101,              /* "uint16"  */
  YYSYMBOL_DAS_TFLOAT16 = 102,             /* "float16"  */
  YYSYMBOL_DAS_THALF2 = 103,               /* "half2"  */
  YYSYMBOL_DAS_THALF3 = 104,               /* "half3"  */
  YYSYMBOL_DAS_THALF4 = 105,               /* "half4"  */
  YYSYMBOL_DAS_THALF8 = 106,               /* "half8"  */
  YYSYMBOL_DAS_TSHORT2 = 107,              /* "short2"  */
  YYSYMBOL_DAS_TSHORT3 = 108,              /* "short3"  */
  YYSYMBOL_DAS_TSHORT4 = 109,              /* "short4"  */
  YYSYMBOL_DAS_TSHORT8 = 110,              /* "short8"  */
  YYSYMBOL_DAS_TUSHORT2 = 111,             /* "ushort2"  */
  YYSYMBOL_DAS_TUSHORT3 = 112,             /* "ushort3"  */
  YYSYMBOL_DAS_TUSHORT4 = 113,             /* "ushort4"  */
  YYSYMBOL_DAS_TUSHORT8 = 114,             /* "ushort8"  */
  YYSYMBOL_DAS_TBYTE2 = 115,               /* "byte2"  */
  YYSYMBOL_DAS_TBYTE3 = 116,               /* "byte3"  */
  YYSYMBOL_DAS_TBYTE4 = 117,               /* "byte4"  */
  YYSYMBOL_DAS_TBYTE8 = 118,               /* "byte8"  */
  YYSYMBOL_DAS_TBYTE16 = 119,              /* "byte16"  */
  YYSYMBOL_DAS_TUBYTE2 = 120,              /* "ubyte2"  */
  YYSYMBOL_DAS_TUBYTE3 = 121,              /* "ubyte3"  */
  YYSYMBOL_DAS_TUBYTE4 = 122,              /* "ubyte4"  */
  YYSYMBOL_DAS_TUBYTE8 = 123,              /* "ubyte8"  */
  YYSYMBOL_DAS_TUBYTE16 = 124,             /* "ubyte16"  */
  YYSYMBOL_DAS_TTUPLE = 125,               /* "tuple"  */
  YYSYMBOL_DAS_TVARIANT = 126,             /* "variant"  */
  YYSYMBOL_DAS_GENERATOR = 127,            /* "generator"  */
  YYSYMBOL_DAS_YIELD = 128,                /* "yield"  */
  YYSYMBOL_DAS_SEALED = 129,               /* "sealed"  */
  YYSYMBOL_DAS_TEMPLATE = 130,             /* "template"  */
  YYSYMBOL_ADDEQU = 131,                   /* "+="  */
  YYSYMBOL_SUBEQU = 132,                   /* "-="  */
  YYSYMBOL_DIVEQU = 133,                   /* "/="  */
  YYSYMBOL_MULEQU = 134,                   /* "*="  */
  YYSYMBOL_MODEQU = 135,                   /* "%="  */
  YYSYMBOL_ANDEQU = 136,                   /* "&="  */
  YYSYMBOL_OREQU = 137,                    /* "|="  */
  YYSYMBOL_XOREQU = 138,                   /* "^="  */
  YYSYMBOL_SHL = 139,                      /* "<<"  */
  YYSYMBOL_SHR = 140,                      /* ">>"  */
  YYSYMBOL_ADDADD = 141,                   /* "++"  */
  YYSYMBOL_SUBSUB = 142,                   /* "--"  */
  YYSYMBOL_LEEQU = 143,                    /* "<="  */
  YYSYMBOL_SHLEQU = 144,                   /* "<<="  */
  YYSYMBOL_SHREQU = 145,                   /* ">>="  */
  YYSYMBOL_GREQU = 146,                    /* ">="  */
  YYSYMBOL_EQUEQU = 147,                   /* "=="  */
  YYSYMBOL_NOTEQU = 148,                   /* "!="  */
  YYSYMBOL_RARROW = 149,                   /* "->"  */
  YYSYMBOL_LARROW = 150,                   /* "<-"  */
  YYSYMBOL_QQ = 151,                       /* "??"  */
  YYSYMBOL_QDOT = 152,                     /* "?."  */
  YYSYMBOL_QBRA = 153,                     /* "?["  */
  YYSYMBOL_LPIPE = 154,                    /* "<|"  */
  YYSYMBOL_LBPIPE = 155,                   /* " <|"  */
  YYSYMBOL_LLPIPE = 156,                   /* "$ <|"  */
  YYSYMBOL_LAPIPE = 157,                   /* "@ <|"  */
  YYSYMBOL_LFPIPE = 158,                   /* "@@ <|"  */
  YYSYMBOL_RPIPE = 159,                    /* "|>"  */
  YYSYMBOL_CLONEEQU = 160,                 /* ":="  */
  YYSYMBOL_ROTL = 161,                     /* "<<<"  */
  YYSYMBOL_ROTR = 162,                     /* ">>>"  */
  YYSYMBOL_ROTLEQU = 163,                  /* "<<<="  */
  YYSYMBOL_ROTREQU = 164,                  /* ">>>="  */
  YYSYMBOL_MAPTO = 165,                    /* "=>"  */
  YYSYMBOL_COLCOL = 166,                   /* "::"  */
  YYSYMBOL_ANDAND = 167,                   /* "&&"  */
  YYSYMBOL_OROR = 168,                     /* "||"  */
  YYSYMBOL_XORXOR = 169,                   /* "^^"  */
  YYSYMBOL_ANDANDEQU = 170,                /* "&&="  */
  YYSYMBOL_OROREQU = 171,                  /* "||="  */
  YYSYMBOL_XORXOREQU = 172,                /* "^^="  */
  YYSYMBOL_DOTDOT = 173,                   /* ".."  */
  YYSYMBOL_MTAG_E = 174,                   /* "$$"  */
  YYSYMBOL_MTAG_I = 175,                   /* "$i"  */
  YYSYMBOL_MTAG_V = 176,                   /* "$v"  */
  YYSYMBOL_MTAG_B = 177,                   /* "$b"  */
  YYSYMBOL_MTAG_A = 178,                   /* "$a"  */
  YYSYMBOL_MTAG_T = 179,                   /* "$t"  */
  YYSYMBOL_MTAG_C = 180,                   /* "$c"  */
  YYSYMBOL_MTAG_F = 181,                   /* "$f"  */
  YYSYMBOL_MTAG_DOTDOTDOT = 182,           /* "..."  */
  YYSYMBOL_BRABRAB = 183,                  /* "[["  */
  YYSYMBOL_BRACBRB = 184,                  /* "[{"  */
  YYSYMBOL_CBRCBRB = 185,                  /* "{{"  */
  YYSYMBOL_OPEN_BRACE = 186,               /* "new scope"  */
  YYSYMBOL_CLOSE_BRACE = 187,              /* "close scope"  */
  YYSYMBOL_SEMICOLON = 188,                /* SEMICOLON  */
  YYSYMBOL_INTEGER = 189,                  /* "integer constant"  */
  YYSYMBOL_LONG_INTEGER = 190,             /* "long integer constant"  */
  YYSYMBOL_UNSIGNED_INTEGER = 191,         /* "unsigned integer constant"  */
  YYSYMBOL_UNSIGNED_LONG_INTEGER = 192,    /* "unsigned long integer constant"  */
  YYSYMBOL_UNSIGNED_INT8 = 193,            /* "unsigned int8 constant"  */
  YYSYMBOL_DAS_FLOAT = 194,                /* "floating point constant"  */
  YYSYMBOL_DAS_FLOAT16_CONST = 195,        /* "float16 constant"  */
  YYSYMBOL_DOUBLE = 196,                   /* "double constant"  */
  YYSYMBOL_NAME = 197,                     /* "name"  */
  YYSYMBOL_KEYWORD = 198,                  /* "keyword"  */
  YYSYMBOL_TYPE_FUNCTION = 199,            /* "type function"  */
  YYSYMBOL_BEGIN_STRING = 200,             /* "start of the string"  */
  YYSYMBOL_STRING_CHARACTER = 201,         /* STRING_CHARACTER  */
  YYSYMBOL_STRING_CHARACTER_ESC = 202,     /* STRING_CHARACTER_ESC  */
  YYSYMBOL_END_STRING = 203,               /* "end of the string"  */
  YYSYMBOL_BEGIN_STRING_EXPR = 204,        /* "{"  */
  YYSYMBOL_END_STRING_EXPR = 205,          /* "}"  */
  YYSYMBOL_END_OF_READ = 206,              /* "end of failed eader macro"  */
  YYSYMBOL_207_begin_of_code_block_ = 207, /* "begin of code block"  */
  YYSYMBOL_208_end_of_code_block_ = 208,   /* "end of code block"  */
  YYSYMBOL_209_end_of_expression_ = 209,   /* "end of expression"  */
  YYSYMBOL_SEMICOLON_CUR_CUR = 210,        /* ";}}"  */
  YYSYMBOL_SEMICOLON_CUR_SQR = 211,        /* ";}]"  */
  YYSYMBOL_SEMICOLON_SQR_SQR = 212,        /* ";]]"  */
  YYSYMBOL_COMMA_SQR_SQR = 213,            /* ",]]"  */
  YYSYMBOL_COMMA_CUR_SQR = 214,            /* ",}]"  */
  YYSYMBOL_END_OF_EXPR = 215,              /* END_OF_EXPR  */
  YYSYMBOL_EMPTY = 216,                    /* EMPTY  */
  YYSYMBOL_217_ = 217,                     /* ','  */
  YYSYMBOL_218_ = 218,                     /* '='  */
  YYSYMBOL_219_ = 219,                     /* '?'  */
  YYSYMBOL_220_ = 220,                     /* ':'  */
  YYSYMBOL_221_ = 221,                     /* '|'  */
  YYSYMBOL_222_ = 222,                     /* '^'  */
  YYSYMBOL_223_ = 223,                     /* '&'  */
  YYSYMBOL_224_ = 224,                     /* '<'  */
  YYSYMBOL_225_ = 225,                     /* '>'  */
  YYSYMBOL_226_ = 226,                     /* '-'  */
  YYSYMBOL_227_ = 227,                     /* '+'  */
  YYSYMBOL_228_ = 228,                     /* '*'  */
  YYSYMBOL_229_ = 229,                     /* '/'  */
  YYSYMBOL_230_ = 230,                     /* '%'  */
  YYSYMBOL_UNARY_MINUS = 231,              /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 232,               /* UNARY_PLUS  */
  YYSYMBOL_233_ = 233,                     /* '~'  */
  YYSYMBOL_234_ = 234,                     /* '!'  */
  YYSYMBOL_PRE_INC = 235,                  /* PRE_INC  */
  YYSYMBOL_PRE_DEC = 236,                  /* PRE_DEC  */
  YYSYMBOL_POST_INC = 237,                 /* POST_INC  */
  YYSYMBOL_POST_DEC = 238,                 /* POST_DEC  */
  YYSYMBOL_DEREF = 239,                    /* DEREF  */
  YYSYMBOL_240_ = 240,                     /* '.'  */
  YYSYMBOL_241_ = 241,                     /* '['  */
  YYSYMBOL_242_ = 242,                     /* ']'  */
  YYSYMBOL_243_ = 243,                     /* '('  */
  YYSYMBOL_244_ = 244,                     /* ')'  */
  YYSYMBOL_245_ = 245,                     /* '$'  */
  YYSYMBOL_246_ = 246,                     /* '@'  */
  YYSYMBOL_247_ = 247,                     /* '#'  */
  YYSYMBOL_YYACCEPT = 248,                 /* $accept  */
  YYSYMBOL_program = 249,                  /* program  */
  YYSYMBOL_semicolon = 250,                /* semicolon  */
  YYSYMBOL_top_level_reader_macro = 251,   /* top_level_reader_macro  */
  YYSYMBOL_optional_public_or_private_module = 252, /* optional_public_or_private_module  */
  YYSYMBOL_module_name = 253,              /* module_name  */
  YYSYMBOL_optional_not_required = 254,    /* optional_not_required  */
  YYSYMBOL_module_declaration = 255,       /* module_declaration  */
  YYSYMBOL_character_sequence = 256,       /* character_sequence  */
  YYSYMBOL_string_constant = 257,          /* string_constant  */
  YYSYMBOL_format_string = 258,            /* format_string  */
  YYSYMBOL_optional_format_string = 259,   /* optional_format_string  */
  YYSYMBOL_260_1 = 260,                    /* $@1  */
  YYSYMBOL_string_builder_body = 261,      /* string_builder_body  */
  YYSYMBOL_string_builder = 262,           /* string_builder  */
  YYSYMBOL_reader_character_sequence = 263, /* reader_character_sequence  */
  YYSYMBOL_expr_reader = 264,              /* expr_reader  */
  YYSYMBOL_265_2 = 265,                    /* $@2  */
  YYSYMBOL_options_declaration = 266,      /* options_declaration  */
  YYSYMBOL_require_declaration = 267,      /* require_declaration  */
  YYSYMBOL_keyword_or_name = 268,          /* keyword_or_name  */
  YYSYMBOL_require_module_name = 269,      /* require_module_name  */
  YYSYMBOL_require_module = 270,           /* require_module  */
  YYSYMBOL_is_public_module = 271,         /* is_public_module  */
  YYSYMBOL_expect_declaration = 272,       /* expect_declaration  */
  YYSYMBOL_expect_list = 273,              /* expect_list  */
  YYSYMBOL_expect_error = 274,             /* expect_error  */
  YYSYMBOL_expression_label = 275,         /* expression_label  */
  YYSYMBOL_expression_goto = 276,          /* expression_goto  */
  YYSYMBOL_elif_or_static_elif = 277,      /* elif_or_static_elif  */
  YYSYMBOL_expression_else = 278,          /* expression_else  */
  YYSYMBOL_if_or_static_if = 279,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 280, /* expression_else_one_liner  */
  YYSYMBOL_281_3 = 281,                    /* $@3  */
  YYSYMBOL_expression_if_one_liner = 282,  /* expression_if_one_liner  */
  YYSYMBOL_expression_if_then_else = 283,  /* expression_if_then_else  */
  YYSYMBOL_284_4 = 284,                    /* $@4  */
  YYSYMBOL_expression_for_loop = 285,      /* expression_for_loop  */
  YYSYMBOL_286_5 = 286,                    /* $@5  */
  YYSYMBOL_287_6 = 287,                    /* $@6  */
  YYSYMBOL_expression_unsafe = 288,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 289,    /* expression_while_loop  */
  YYSYMBOL_expression_with = 290,          /* expression_with  */
  YYSYMBOL_expression_with_alias = 291,    /* expression_with_alias  */
  YYSYMBOL_292_7 = 292,                    /* $@7  */
  YYSYMBOL_293_8 = 293,                    /* $@8  */
  YYSYMBOL_annotation_argument_value = 294, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 295, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 296, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 297,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 298, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 299,   /* metadata_argument_list  */
  YYSYMBOL_annotation_declaration_name = 300, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 301, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 302,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 303,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 304, /* optional_annotation_list  */
  YYSYMBOL_optional_function_argument_list = 305, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 306,   /* optional_function_type  */
  YYSYMBOL_function_name = 307,            /* function_name  */
  YYSYMBOL_optional_template = 308,        /* optional_template  */
  YYSYMBOL_global_function_declaration = 309, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 310, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 311, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 312,     /* function_declaration  */
  YYSYMBOL_313_9 = 313,                    /* $@9  */
  YYSYMBOL_open_block = 314,               /* open_block  */
  YYSYMBOL_close_block = 315,              /* close_block  */
  YYSYMBOL_expression_block = 316,         /* expression_block  */
  YYSYMBOL_expr_call_pipe = 317,           /* expr_call_pipe  */
  YYSYMBOL_expression_any = 318,           /* expression_any  */
  YYSYMBOL_expressions = 319,              /* expressions  */
  YYSYMBOL_expr_keyword = 320,             /* expr_keyword  */
  YYSYMBOL_optional_expr_list = 321,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_list_in_braces = 322, /* optional_expr_list_in_braces  */
  YYSYMBOL_optional_expr_map_tuple_list = 323, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 324, /* type_declaration_no_options_list  */
  YYSYMBOL_expression_keyword = 325,       /* expression_keyword  */
  YYSYMBOL_326_10 = 326,                   /* $@10  */
  YYSYMBOL_327_11 = 327,                   /* $@11  */
  YYSYMBOL_328_12 = 328,                   /* $@12  */
  YYSYMBOL_329_13 = 329,                   /* $@13  */
  YYSYMBOL_expr_pipe = 330,                /* expr_pipe  */
  YYSYMBOL_name_in_namespace = 331,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 332,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 333,     /* new_type_declaration  */
  YYSYMBOL_334_14 = 334,                   /* $@14  */
  YYSYMBOL_335_15 = 335,                   /* $@15  */
  YYSYMBOL_expr_new = 336,                 /* expr_new  */
  YYSYMBOL_expression_break = 337,         /* expression_break  */
  YYSYMBOL_expression_continue = 338,      /* expression_continue  */
  YYSYMBOL_expression_return_no_pipe = 339, /* expression_return_no_pipe  */
  YYSYMBOL_expression_return = 340,        /* expression_return  */
  YYSYMBOL_expression_yield_no_pipe = 341, /* expression_yield_no_pipe  */
  YYSYMBOL_expression_yield = 342,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 343,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 344,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 345,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 346,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 347,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 348, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 349,           /* expression_let  */
  YYSYMBOL_expr_cast = 350,                /* expr_cast  */
  YYSYMBOL_351_16 = 351,                   /* $@16  */
  YYSYMBOL_352_17 = 352,                   /* $@17  */
  YYSYMBOL_353_18 = 353,                   /* $@18  */
  YYSYMBOL_354_19 = 354,                   /* $@19  */
  YYSYMBOL_355_20 = 355,                   /* $@20  */
  YYSYMBOL_356_21 = 356,                   /* $@21  */
  YYSYMBOL_expr_type_decl = 357,           /* expr_type_decl  */
  YYSYMBOL_358_22 = 358,                   /* $@22  */
  YYSYMBOL_359_23 = 359,                   /* $@23  */
  YYSYMBOL_expr_type_info = 360,           /* expr_type_info  */
  YYSYMBOL_expr_list = 361,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 362,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 363,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 364,            /* capture_entry  */
  YYSYMBOL_capture_list = 365,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 366,    /* optional_capture_list  */
  YYSYMBOL_expr_block = 367,               /* expr_block  */
  YYSYMBOL_expr_full_block = 368,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 369, /* expr_full_block_assumed_piped  */
  YYSYMBOL_370_24 = 370,                   /* $@24  */
  YYSYMBOL_expr_numeric_const = 371,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign = 372,              /* expr_assign  */
  YYSYMBOL_expr_assign_pipe_right = 373,   /* expr_assign_pipe_right  */
  YYSYMBOL_expr_assign_pipe = 374,         /* expr_assign_pipe  */
  YYSYMBOL_expr_named_call = 375,          /* expr_named_call  */
  YYSYMBOL_expr_method_call = 376,         /* expr_method_call  */
  YYSYMBOL_func_addr_name = 377,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 378,           /* func_addr_expr  */
  YYSYMBOL_379_25 = 379,                   /* $@25  */
  YYSYMBOL_380_26 = 380,                   /* $@26  */
  YYSYMBOL_381_27 = 381,                   /* $@27  */
  YYSYMBOL_382_28 = 382,                   /* $@28  */
  YYSYMBOL_expr_field = 383,               /* expr_field  */
  YYSYMBOL_384_29 = 384,                   /* $@29  */
  YYSYMBOL_385_30 = 385,                   /* $@30  */
  YYSYMBOL_expr_call = 386,                /* expr_call  */
  YYSYMBOL_expr2 = 387,                    /* expr2  */
  YYSYMBOL_388_31 = 388,                   /* $@31  */
  YYSYMBOL_389_32 = 389,                   /* $@32  */
  YYSYMBOL_390_33 = 390,                   /* $@33  */
  YYSYMBOL_391_34 = 391,                   /* $@34  */
  YYSYMBOL_392_35 = 392,                   /* $@35  */
  YYSYMBOL_393_36 = 393,                   /* $@36  */
  YYSYMBOL_expr = 394,                     /* expr  */
  YYSYMBOL_expr_mtag = 395,                /* expr_mtag  */
  YYSYMBOL_optional_field_annotation = 396, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 397,        /* optional_override  */
  YYSYMBOL_optional_constant = 398,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 399, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 400, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 401, /* structure_variable_declaration  */
  YYSYMBOL_opt_sem = 402,                  /* opt_sem  */
  YYSYMBOL_struct_variable_declaration_list = 403, /* struct_variable_declaration_list  */
  YYSYMBOL_404_37 = 404,                   /* $@37  */
  YYSYMBOL_405_38 = 405,                   /* $@38  */
  YYSYMBOL_406_39 = 406,                   /* $@39  */
  YYSYMBOL_407_40 = 407,                   /* $@40  */
  YYSYMBOL_function_argument_declaration_no_type = 408, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 409, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 410,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 411,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 412,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 413,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 414,             /* variant_type  */
  YYSYMBOL_variant_type_list = 415,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 416,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 417,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 418, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 419, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 420,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 421,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 422,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 423, /* let_variable_name_with_pos_list  */
  YYSYMBOL_let_variable_declaration = 424, /* let_variable_declaration  */
  YYSYMBOL_global_variable_declaration_list = 425, /* global_variable_declaration_list  */
  YYSYMBOL_426_41 = 426,                   /* $@41  */
  YYSYMBOL_optional_shared = 427,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 428, /* optional_public_or_private_variable  */
  YYSYMBOL_global_let = 429,               /* global_let  */
  YYSYMBOL_430_42 = 430,                   /* $@42  */
  YYSYMBOL_enum_list = 431,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 432, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 433,             /* single_alias  */
  YYSYMBOL_434_43 = 434,                   /* $@43  */
  YYSYMBOL_alias_list = 435,               /* alias_list  */
  YYSYMBOL_alias_declaration = 436,        /* alias_declaration  */
  YYSYMBOL_437_44 = 437,                   /* $@44  */
  YYSYMBOL_optional_public_or_private_enum = 438, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 439,                /* enum_name  */
  YYSYMBOL_enum_declaration = 440,         /* enum_declaration  */
  YYSYMBOL_optional_structure_parent = 441, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 442,          /* optional_sealed  */
  YYSYMBOL_structure_name = 443,           /* structure_name  */
  YYSYMBOL_class_or_struct = 444,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 445, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 446, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 447,    /* structure_declaration  */
  YYSYMBOL_448_45 = 448,                   /* $@45  */
  YYSYMBOL_449_46 = 449,                   /* $@46  */
  YYSYMBOL_variable_name_with_pos_list = 450, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 451,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 452, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 453, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 454,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 455,            /* bitfield_bits  */
  YYSYMBOL_commas = 456,                   /* commas  */
  YYSYMBOL_bitfield_alias_bits = 457,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_basic_type_declaration = 458, /* bitfield_basic_type_declaration  */
  YYSYMBOL_bitfield_type_declaration = 459, /* bitfield_type_declaration  */
  YYSYMBOL_460_47 = 460,                   /* $@47  */
  YYSYMBOL_461_48 = 461,                   /* $@48  */
  YYSYMBOL_c_or_s = 462,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 463,          /* table_type_pair  */
  YYSYMBOL_dim_list = 464,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 465, /* type_declaration_no_options  */
  YYSYMBOL_466_49 = 466,                   /* $@49  */
  YYSYMBOL_467_50 = 467,                   /* $@50  */
  YYSYMBOL_468_51 = 468,                   /* $@51  */
  YYSYMBOL_469_52 = 469,                   /* $@52  */
  YYSYMBOL_470_53 = 470,                   /* $@53  */
  YYSYMBOL_471_54 = 471,                   /* $@54  */
  YYSYMBOL_472_55 = 472,                   /* $@55  */
  YYSYMBOL_473_56 = 473,                   /* $@56  */
  YYSYMBOL_474_57 = 474,                   /* $@57  */
  YYSYMBOL_475_58 = 475,                   /* $@58  */
  YYSYMBOL_476_59 = 476,                   /* $@59  */
  YYSYMBOL_477_60 = 477,                   /* $@60  */
  YYSYMBOL_478_61 = 478,                   /* $@61  */
  YYSYMBOL_479_62 = 479,                   /* $@62  */
  YYSYMBOL_480_63 = 480,                   /* $@63  */
  YYSYMBOL_481_64 = 481,                   /* $@64  */
  YYSYMBOL_482_65 = 482,                   /* $@65  */
  YYSYMBOL_483_66 = 483,                   /* $@66  */
  YYSYMBOL_484_67 = 484,                   /* $@67  */
  YYSYMBOL_485_68 = 485,                   /* $@68  */
  YYSYMBOL_486_69 = 486,                   /* $@69  */
  YYSYMBOL_487_70 = 487,                   /* $@70  */
  YYSYMBOL_488_71 = 488,                   /* $@71  */
  YYSYMBOL_489_72 = 489,                   /* $@72  */
  YYSYMBOL_490_73 = 490,                   /* $@73  */
  YYSYMBOL_491_74 = 491,                   /* $@74  */
  YYSYMBOL_492_75 = 492,                   /* $@75  */
  YYSYMBOL_type_declaration = 493,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 494,  /* tuple_alias_declaration  */
  YYSYMBOL_495_76 = 495,                   /* $@76  */
  YYSYMBOL_496_77 = 496,                   /* $@77  */
  YYSYMBOL_497_78 = 497,                   /* $@78  */
  YYSYMBOL_498_79 = 498,                   /* $@79  */
  YYSYMBOL_variant_alias_declaration = 499, /* variant_alias_declaration  */
  YYSYMBOL_500_80 = 500,                   /* $@80  */
  YYSYMBOL_501_81 = 501,                   /* $@81  */
  YYSYMBOL_502_82 = 502,                   /* $@82  */
  YYSYMBOL_503_83 = 503,                   /* $@83  */
  YYSYMBOL_bitfield_alias_declaration = 504, /* bitfield_alias_declaration  */
  YYSYMBOL_505_84 = 505,                   /* $@84  */
  YYSYMBOL_506_85 = 506,                   /* $@85  */
  YYSYMBOL_make_decl = 507,                /* make_decl  */
  YYSYMBOL_make_struct_fields = 508,       /* make_struct_fields  */
  YYSYMBOL_make_variant_dim = 509,         /* make_variant_dim  */
  YYSYMBOL_make_struct_single = 510,       /* make_struct_single  */
  YYSYMBOL_make_struct_dim = 511,          /* make_struct_dim  */
  YYSYMBOL_make_struct_dim_list = 512,     /* make_struct_dim_list  */
  YYSYMBOL_make_struct_dim_decl = 513,     /* make_struct_dim_decl  */
  YYSYMBOL_optional_make_struct_dim_decl = 514, /* optional_make_struct_dim_decl  */
  YYSYMBOL_optional_block = 515,           /* optional_block  */
  YYSYMBOL_optional_trailing_semicolon_cur_cur = 516, /* optional_trailing_semicolon_cur_cur  */
  YYSYMBOL_optional_trailing_semicolon_cur_sqr = 517, /* optional_trailing_semicolon_cur_sqr  */
  YYSYMBOL_optional_trailing_semicolon_sqr_sqr = 518, /* optional_trailing_semicolon_sqr_sqr  */
  YYSYMBOL_optional_trailing_delim_sqr_sqr = 519, /* optional_trailing_delim_sqr_sqr  */
  YYSYMBOL_optional_trailing_delim_cur_sqr = 520, /* optional_trailing_delim_cur_sqr  */
  YYSYMBOL_use_initializer = 521,          /* use_initializer  */
  YYSYMBOL_make_struct_decl = 522,         /* make_struct_decl  */
  YYSYMBOL_523_86 = 523,                   /* $@86  */
  YYSYMBOL_524_87 = 524,                   /* $@87  */
  YYSYMBOL_525_88 = 525,                   /* $@88  */
  YYSYMBOL_526_89 = 526,                   /* $@89  */
  YYSYMBOL_527_90 = 527,                   /* $@90  */
  YYSYMBOL_528_91 = 528,                   /* $@91  */
  YYSYMBOL_529_92 = 529,                   /* $@92  */
  YYSYMBOL_530_93 = 530,                   /* $@93  */
  YYSYMBOL_make_tuple = 531,               /* make_tuple  */
  YYSYMBOL_make_map_tuple = 532,           /* make_map_tuple  */
  YYSYMBOL_make_tuple_call = 533,          /* make_tuple_call  */
  YYSYMBOL_534_94 = 534,                   /* $@94  */
  YYSYMBOL_535_95 = 535,                   /* $@95  */
  YYSYMBOL_make_dim = 536,                 /* make_dim  */
  YYSYMBOL_make_dim_decl = 537,            /* make_dim_decl  */
  YYSYMBOL_538_96 = 538,                   /* $@96  */
  YYSYMBOL_539_97 = 539,                   /* $@97  */
  YYSYMBOL_540_98 = 540,                   /* $@98  */
  YYSYMBOL_541_99 = 541,                   /* $@99  */
  YYSYMBOL_542_100 = 542,                  /* $@100  */
  YYSYMBOL_543_101 = 543,                  /* $@101  */
  YYSYMBOL_544_102 = 544,                  /* $@102  */
  YYSYMBOL_545_103 = 545,                  /* $@103  */
  YYSYMBOL_546_104 = 546,                  /* $@104  */
  YYSYMBOL_547_105 = 547,                  /* $@105  */
  YYSYMBOL_make_table = 548,               /* make_table  */
  YYSYMBOL_expr_map_tuple_list = 549,      /* expr_map_tuple_list  */
  YYSYMBOL_make_table_decl = 550,          /* make_table_decl  */
  YYSYMBOL_array_comprehension_where = 551, /* array_comprehension_where  */
  YYSYMBOL_optional_comma = 552,           /* optional_comma  */
  YYSYMBOL_array_comprehension = 553       /* array_comprehension  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined DAS_YYLTYPE_IS_TRIVIAL && DAS_YYLTYPE_IS_TRIVIAL \
             && defined DAS_YYSTYPE_IS_TRIVIAL && DAS_YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   15621

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  248
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  306
/* YYNRULES -- Number of rules.  */
#define YYNRULES  1015
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1875

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   475


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   234,     2,   247,   245,   230,   223,     2,
     243,   244,   228,   227,   217,   226,   240,   229,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   220,   209,
     224,   218,   225,   219,   246,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   241,     2,   242,   222,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   207,   221,   208,   233,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   210,   211,   212,   213,   214,   215,   216,   231,
     232,   235,   236,   237,   238,   239
};

#if DAS_YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   610,   610,   611,   616,   617,   618,   619,   620,   621,
     622,   623,   624,   625,   626,   627,   628,   632,   635,   638,
     644,   645,   646,   650,   651,   655,   656,   660,   679,   680,
     681,   682,   686,   687,   691,   692,   696,   697,   697,   701,
     706,   715,   730,   746,   751,   759,   759,   800,   830,   834,
     835,   836,   840,   843,   847,   851,   855,   859,   865,   874,
     877,   883,   884,   888,   892,   893,   897,   900,   906,   912,
     915,   921,   922,   926,   927,   928,   938,   951,   952,   956,
     957,   957,   963,   964,   965,   966,   967,   971,   981,   991,
     991,   999,   999,  1003,  1003,  1012,  1020,  1032,  1042,  1042,
    1046,  1046,  1052,  1053,  1054,  1055,  1056,  1057,  1061,  1066,
    1074,  1075,  1076,  1080,  1081,  1082,  1083,  1084,  1085,  1086,
    1087,  1088,  1094,  1097,  1103,  1106,  1109,  1115,  1116,  1117,
    1118,  1122,  1135,  1153,  1156,  1164,  1175,  1186,  1197,  1200,
    1207,  1211,  1218,  1219,  1223,  1224,  1225,  1229,  1232,  1236,
    1243,  1247,  1248,  1249,  1250,  1251,  1252,  1253,  1254,  1255,
    1256,  1257,  1258,  1259,  1260,  1261,  1262,  1263,  1264,  1265,
    1266,  1267,  1268,  1269,  1270,  1271,  1272,  1273,  1274,  1275,
    1276,  1277,  1278,  1279,  1280,  1281,  1282,  1283,  1284,  1285,
    1286,  1287,  1288,  1289,  1290,  1291,  1292,  1293,  1294,  1295,
    1296,  1297,  1298,  1299,  1300,  1301,  1302,  1303,  1304,  1305,
    1306,  1307,  1308,  1309,  1310,  1311,  1312,  1313,  1314,  1315,
    1316,  1317,  1318,  1319,  1320,  1321,  1322,  1323,  1324,  1325,
    1326,  1327,  1328,  1329,  1330,  1331,  1332,  1333,  1334,  1335,
    1336,  1337,  1338,  1339,  1340,  1341,  1342,  1343,  1344,  1345,
    1346,  1347,  1348,  1349,  1350,  1351,  1352,  1353,  1354,  1355,
    1356,  1357,  1358,  1359,  1360,  1361,  1362,  1363,  1364,  1365,
    1366,  1367,  1368,  1369,  1370,  1371,  1372,  1373,  1374,  1375,
    1376,  1377,  1381,  1382,  1386,  1405,  1406,  1407,  1411,  1417,
    1417,  1434,  1435,  1438,  1439,  1442,  1449,  1473,  1491,  1500,
    1506,  1507,  1508,  1509,  1510,  1511,  1512,  1513,  1514,  1515,
    1516,  1517,  1518,  1519,  1520,  1521,  1522,  1523,  1524,  1525,
    1526,  1527,  1531,  1536,  1542,  1548,  1560,  1561,  1565,  1566,
    1570,  1571,  1575,  1579,  1586,  1586,  1586,  1592,  1592,  1592,
    1601,  1635,  1643,  1650,  1657,  1663,  1664,  1675,  1679,  1682,
    1690,  1690,  1690,  1693,  1699,  1702,  1706,  1710,  1717,  1724,
    1730,  1734,  1738,  1741,  1744,  1752,  1755,  1758,  1766,  1769,
    1777,  1780,  1783,  1791,  1803,  1804,  1805,  1809,  1810,  1814,
    1815,  1819,  1824,  1832,  1843,  1849,  1864,  1876,  1879,  1885,
    1885,  1885,  1888,  1888,  1888,  1893,  1893,  1893,  1901,  1901,
    1901,  1907,  1921,  1937,  1955,  1965,  1976,  1991,  1994,  2000,
    2001,  2008,  2019,  2020,  2021,  2025,  2026,  2027,  2028,  2029,
    2033,  2038,  2046,  2047,  2060,  2064,  2074,  2081,  2088,  2088,
    2097,  2098,  2099,  2100,  2101,  2102,  2103,  2104,  2108,  2109,
    2110,  2111,  2112,  2113,  2114,  2115,  2116,  2117,  2118,  2119,
    2120,  2121,  2122,  2123,  2124,  2125,  2126,  2130,  2140,  2149,
    2158,  2163,  2164,  2165,  2166,  2167,  2168,  2169,  2170,  2171,
    2172,  2173,  2174,  2175,  2176,  2177,  2178,  2179,  2184,  2190,
    2201,  2206,  2216,  2220,  2227,  2230,  2230,  2230,  2235,  2235,
    2235,  2248,  2252,  2256,  2261,  2268,  2276,  2281,  2288,  2288,
    2288,  2295,  2299,  2309,  2318,  2327,  2331,  2334,  2340,  2341,
    2342,  2343,  2344,  2345,  2346,  2347,  2348,  2349,  2350,  2351,
    2352,  2353,  2354,  2355,  2356,  2357,  2358,  2359,  2360,  2361,
    2362,  2363,  2364,  2365,  2366,  2367,  2368,  2375,  2376,  2377,
    2378,  2379,  2380,  2381,  2382,  2383,  2384,  2385,  2386,  2393,
    2394,  2395,  2396,  2397,  2398,  2399,  2400,  2416,  2417,  2418,
    2419,  2420,  2423,  2426,  2427,  2427,  2427,  2430,  2435,  2439,
    2443,  2443,  2443,  2448,  2451,  2455,  2455,  2455,  2460,  2463,
    2464,  2465,  2466,  2467,  2468,  2469,  2470,  2471,  2473,  2477,
    2485,  2490,  2494,  2503,  2504,  2505,  2506,  2507,  2508,  2509,
    2513,  2517,  2521,  2525,  2529,  2533,  2537,  2541,  2545,  2552,
    2553,  2562,  2566,  2567,  2568,  2572,  2573,  2577,  2578,  2579,
    2583,  2584,  2588,  2599,  2600,  2601,  2602,  2607,  2610,  2610,
    2614,  2614,  2633,  2632,  2648,  2647,  2661,  2670,  2682,  2691,
    2701,  2702,  2703,  2704,  2705,  2709,  2712,  2721,  2722,  2726,
    2729,  2732,  2747,  2756,  2757,  2761,  2764,  2767,  2780,  2781,
    2785,  2791,  2797,  2806,  2809,  2816,  2819,  2825,  2826,  2827,
    2831,  2832,  2836,  2843,  2848,  2857,  2863,  2874,  2877,  2882,
    2887,  2895,  2905,  2916,  2919,  2919,  2939,  2940,  2944,  2945,
    2946,  2950,  2957,  2957,  2976,  2979,  2995,  3015,  3016,  3017,
    3022,  3022,  3052,  3055,  3062,  3072,  3072,  3076,  3077,  3078,
    3082,  3092,  3112,  3135,  3136,  3140,  3141,  3145,  3151,  3152,
    3153,  3154,  3158,  3159,  3160,  3164,  3167,  3178,  3183,  3178,
    3203,  3210,  3215,  3224,  3230,  3241,  3242,  3243,  3244,  3245,
    3246,  3247,  3248,  3249,  3250,  3251,  3252,  3253,  3254,  3255,
    3256,  3257,  3258,  3259,  3260,  3261,  3262,  3263,  3264,  3265,
    3266,  3267,  3268,  3269,  3270,  3271,  3272,  3273,  3274,  3275,
    3276,  3277,  3278,  3279,  3280,  3281,  3282,  3283,  3284,  3285,
    3286,  3287,  3288,  3289,  3290,  3294,  3295,  3296,  3297,  3298,
    3299,  3300,  3301,  3305,  3316,  3320,  3327,  3339,  3346,  3352,
    3362,  3363,  3368,  3373,  3387,  3397,  3407,  3417,  3427,  3440,
    3441,  3442,  3443,  3444,  3448,  3452,  3452,  3452,  3466,  3467,
    3471,  3475,  3482,  3485,  3491,  3492,  3493,  3494,  3495,  3505,
    3508,  3508,  3508,  3512,  3517,  3524,  3524,  3531,  3535,  3539,
    3544,  3549,  3554,  3559,  3563,  3567,  3572,  3576,  3580,  3585,
    3585,  3585,  3591,  3598,  3598,  3598,  3603,  3603,  3603,  3609,
    3609,  3609,  3614,  3619,  3619,  3619,  3624,  3624,  3624,  3633,
    3638,  3638,  3638,  3643,  3643,  3643,  3652,  3657,  3657,  3657,
    3662,  3662,  3662,  3671,  3671,  3671,  3677,  3677,  3677,  3686,
    3689,  3700,  3716,  3716,  3721,  3730,  3716,  3759,  3759,  3764,
    3774,  3759,  3803,  3803,  3803,  3856,  3857,  3858,  3859,  3860,
    3864,  3871,  3878,  3884,  3890,  3897,  3904,  3910,  3919,  3922,
    3928,  3936,  3941,  3948,  3953,  3960,  3965,  3971,  3972,  3976,
    3977,  3982,  3983,  3987,  3988,  3992,  3993,  3997,  3998,  3999,
    4003,  4004,  4005,  4009,  4010,  4014,  4047,  4086,  4105,  4125,
    4145,  4166,  4166,  4166,  4174,  4174,  4174,  4181,  4181,  4181,
    4192,  4192,  4192,  4203,  4207,  4213,  4229,  4235,  4241,  4247,
    4247,  4247,  4261,  4266,  4273,  4293,  4321,  4345,  4345,  4345,
    4355,  4355,  4355,  4369,  4369,  4369,  4383,  4392,  4392,  4392,
    4412,  4419,  4419,  4419,  4429,  4434,  4441,  4444,  4450,  4470,
    4492,  4500,  4520,  4545,  4546,  4550,  4551,  4556,  4566,  4569,
    4572,  4575,  4583,  4592,  4604,  4614
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "\"lexer error\"",
  "\"capture\"", "\"struct\"", "\"class\"", "\"let\"", "\"def\"",
  "\"while\"", "\"if\"", "\"static_if\"", "\"else\"", "\"for\"",
  "\"recover\"", "\"true\"", "\"false\"", "\"new\"", "\"typeinfo\"",
  "\"type\"", "\"in\"", "\"is\"", "\"as\"", "\"elif\"", "\"static_elif\"",
  "\"array\"", "\"return\"", "\"null\"", "\"break\"", "\"try\"",
  "\"options\"", "\"table\"", "\"expect\"", "\"const\"", "\"require\"",
  "\"operator\"", "\"enum\"", "\"finally\"", "\"delete\"", "\"deref\"",
  "\"typedef\"", "\"typedecl\"", "\"with\"", "\"aka\"", "\"assume\"",
  "\"cast\"", "\"override\"", "\"abstract\"", "\"upcast\"", "\"iterator\"",
  "\"var\"", "\"addr\"", "\"continue\"", "\"where\"", "\"pass\"",
  "\"reinterpret\"", "\"module\"", "\"public\"", "\"label\"", "\"goto\"",
  "\"implicit\"", "\"explicit\"", "\"shared\"", "\"private\"",
  "\"smart_ptr\"", "\"unsafe\"", "\"inscope\"", "\"static\"",
  "\"fixed_array\"", "\"default\"", "\"uninitialized\"", "\"bool\"",
  "\"void\"", "\"string\"", "\"auto\"", "\"int\"", "\"int2\"", "\"int3\"",
  "\"int4\"", "\"uint\"", "\"bitfield\"", "\"uint2\"", "\"uint3\"",
  "\"uint4\"", "\"float\"", "\"float2\"", "\"float3\"", "\"float4\"",
  "\"range\"", "\"urange\"", "\"range64\"", "\"urange64\"", "\"block\"",
  "\"int64\"", "\"uint64\"", "\"double\"", "\"function\"", "\"lambda\"",
  "\"int8\"", "\"uint8\"", "\"int16\"", "\"uint16\"", "\"float16\"",
  "\"half2\"", "\"half3\"", "\"half4\"", "\"half8\"", "\"short2\"",
  "\"short3\"", "\"short4\"", "\"short8\"", "\"ushort2\"", "\"ushort3\"",
  "\"ushort4\"", "\"ushort8\"", "\"byte2\"", "\"byte3\"", "\"byte4\"",
  "\"byte8\"", "\"byte16\"", "\"ubyte2\"", "\"ubyte3\"", "\"ubyte4\"",
  "\"ubyte8\"", "\"ubyte16\"", "\"tuple\"", "\"variant\"", "\"generator\"",
  "\"yield\"", "\"sealed\"", "\"template\"", "\"+=\"", "\"-=\"", "\"/=\"",
  "\"*=\"", "\"%=\"", "\"&=\"", "\"|=\"", "\"^=\"", "\"<<\"", "\">>\"",
  "\"++\"", "\"--\"", "\"<=\"", "\"<<=\"", "\">>=\"", "\">=\"", "\"==\"",
  "\"!=\"", "\"->\"", "\"<-\"", "\"??\"", "\"?.\"", "\"?[\"", "\"<|\"",
  "\" <|\"", "\"$ <|\"", "\"@ <|\"", "\"@@ <|\"", "\"|>\"", "\":=\"",
  "\"<<<\"", "\">>>\"", "\"<<<=\"", "\">>>=\"", "\"=>\"", "\"::\"",
  "\"&&\"", "\"||\"", "\"^^\"", "\"&&=\"", "\"||=\"", "\"^^=\"", "\"..\"",
  "\"$$\"", "\"$i\"", "\"$v\"", "\"$b\"", "\"$a\"", "\"$t\"", "\"$c\"",
  "\"$f\"", "\"...\"", "\"[[\"", "\"[{\"", "\"{{\"", "\"new scope\"",
  "\"close scope\"", "SEMICOLON", "\"integer constant\"",
  "\"long integer constant\"", "\"unsigned integer constant\"",
  "\"unsigned long integer constant\"", "\"unsigned int8 constant\"",
  "\"floating point constant\"", "\"float16 constant\"",
  "\"double constant\"", "\"name\"", "\"keyword\"", "\"type function\"",
  "\"start of the string\"", "STRING_CHARACTER", "STRING_CHARACTER_ESC",
  "\"end of the string\"", "\"{\"", "\"}\"",
  "\"end of failed eader macro\"", "\"begin of code block\"",
  "\"end of code block\"", "\"end of expression\"", "\";}}\"", "\";}]\"",
  "\";]]\"", "\",]]\"", "\",}]\"", "END_OF_EXPR", "EMPTY", "','", "'='",
  "'?'", "':'", "'|'", "'^'", "'&'", "'<'", "'>'", "'-'", "'+'", "'*'",
  "'/'", "'%'", "UNARY_MINUS", "UNARY_PLUS", "'~'", "'!'", "PRE_INC",
  "PRE_DEC", "POST_INC", "POST_DEC", "DEREF", "'.'", "'['", "']'", "'('",
  "')'", "'$'", "'@'", "'#'", "$accept", "program", "semicolon",
  "top_level_reader_macro", "optional_public_or_private_module",
  "module_name", "optional_not_required", "module_declaration",
  "character_sequence", "string_constant", "format_string",
  "optional_format_string", "$@1", "string_builder_body", "string_builder",
  "reader_character_sequence", "expr_reader", "$@2", "options_declaration",
  "require_declaration", "keyword_or_name", "require_module_name",
  "require_module", "is_public_module", "expect_declaration",
  "expect_list", "expect_error", "expression_label", "expression_goto",
  "elif_or_static_elif", "expression_else", "if_or_static_if",
  "expression_else_one_liner", "$@3", "expression_if_one_liner",
  "expression_if_then_else", "$@4", "expression_for_loop", "$@5", "$@6",
  "expression_unsafe", "expression_while_loop", "expression_with",
  "expression_with_alias", "$@7", "$@8", "annotation_argument_value",
  "annotation_argument_value_list", "annotation_argument_name",
  "annotation_argument", "annotation_argument_list",
  "metadata_argument_list", "annotation_declaration_name",
  "annotation_declaration_basic", "annotation_declaration",
  "annotation_list", "optional_annotation_list",
  "optional_function_argument_list", "optional_function_type",
  "function_name", "optional_template", "global_function_declaration",
  "optional_public_or_private_function", "function_declaration_header",
  "function_declaration", "$@9", "open_block", "close_block",
  "expression_block", "expr_call_pipe", "expression_any", "expressions",
  "expr_keyword", "optional_expr_list", "optional_expr_list_in_braces",
  "optional_expr_map_tuple_list", "type_declaration_no_options_list",
  "expression_keyword", "$@10", "$@11", "$@12", "$@13", "expr_pipe",
  "name_in_namespace", "expression_delete", "new_type_declaration", "$@14",
  "$@15", "expr_new", "expression_break", "expression_continue",
  "expression_return_no_pipe", "expression_return",
  "expression_yield_no_pipe", "expression_yield", "expression_try_catch",
  "kwd_let_var_or_nothing", "kwd_let", "optional_in_scope",
  "tuple_expansion", "tuple_expansion_variable_declaration",
  "expression_let", "expr_cast", "$@16", "$@17", "$@18", "$@19", "$@20",
  "$@21", "expr_type_decl", "$@22", "$@23", "expr_type_info", "expr_list",
  "block_or_simple_block", "block_or_lambda", "capture_entry",
  "capture_list", "optional_capture_list", "expr_block", "expr_full_block",
  "expr_full_block_assumed_piped", "$@24", "expr_numeric_const",
  "expr_assign", "expr_assign_pipe_right", "expr_assign_pipe",
  "expr_named_call", "expr_method_call", "func_addr_name",
  "func_addr_expr", "$@25", "$@26", "$@27", "$@28", "expr_field", "$@29",
  "$@30", "expr_call", "expr2", "$@31", "$@32", "$@33", "$@34", "$@35",
  "$@36", "expr", "expr_mtag", "optional_field_annotation",
  "optional_override", "optional_constant",
  "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "opt_sem", "struct_variable_declaration_list", "$@37", "$@38", "$@39",
  "$@40", "function_argument_declaration_no_type",
  "function_argument_declaration_type", "function_argument_list",
  "tuple_type", "tuple_type_list", "tuple_alias_type_list", "variant_type",
  "variant_type_list", "variant_alias_type_list", "copy_or_move",
  "variable_declaration_no_type", "variable_declaration_type",
  "variable_declaration", "copy_or_move_or_clone", "optional_ref",
  "let_variable_name_with_pos_list", "let_variable_declaration",
  "global_variable_declaration_list", "$@41", "optional_shared",
  "optional_public_or_private_variable", "global_let", "$@42", "enum_list",
  "optional_public_or_private_alias", "single_alias", "$@43", "alias_list",
  "alias_declaration", "$@44", "optional_public_or_private_enum",
  "enum_name", "enum_declaration", "optional_structure_parent",
  "optional_sealed", "structure_name", "class_or_struct",
  "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@45", "$@46", "variable_name_with_pos_list", "basic_type_declaration",
  "enum_basic_type_declaration", "structure_type_declaration",
  "auto_type_declaration", "bitfield_bits", "commas",
  "bitfield_alias_bits", "bitfield_basic_type_declaration",
  "bitfield_type_declaration", "$@47", "$@48", "c_or_s", "table_type_pair",
  "dim_list", "type_declaration_no_options", "$@49", "$@50", "$@51",
  "$@52", "$@53", "$@54", "$@55", "$@56", "$@57", "$@58", "$@59", "$@60",
  "$@61", "$@62", "$@63", "$@64", "$@65", "$@66", "$@67", "$@68", "$@69",
  "$@70", "$@71", "$@72", "$@73", "$@74", "$@75", "type_declaration",
  "tuple_alias_declaration", "$@76", "$@77", "$@78", "$@79",
  "variant_alias_declaration", "$@80", "$@81", "$@82", "$@83",
  "bitfield_alias_declaration", "$@84", "$@85", "make_decl",
  "make_struct_fields", "make_variant_dim", "make_struct_single",
  "make_struct_dim", "make_struct_dim_list", "make_struct_dim_decl",
  "optional_make_struct_dim_decl", "optional_block",
  "optional_trailing_semicolon_cur_cur",
  "optional_trailing_semicolon_cur_sqr",
  "optional_trailing_semicolon_sqr_sqr", "optional_trailing_delim_sqr_sqr",
  "optional_trailing_delim_cur_sqr", "use_initializer", "make_struct_decl",
  "$@86", "$@87", "$@88", "$@89", "$@90", "$@91", "$@92", "$@93",
  "make_tuple", "make_map_tuple", "make_tuple_call", "$@94", "$@95",
  "make_dim", "make_dim_decl", "$@96", "$@97", "$@98", "$@99", "$@100",
  "$@101", "$@102", "$@103", "$@104", "$@105", "make_table",
  "expr_map_tuple_list", "make_table_decl", "array_comprehension_where",
  "optional_comma", "array_comprehension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1604)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-881)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1604,   132, -1604, -1604,    84,   -93,   453,   173, -1604,   -51,
      67,    67,    67, -1604, -1604,   -64,   179, -1604, -1604, -1604,
     576, -1604, -1604, -1604,   573, -1604,   109, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604,    56, -1604,   -28,
      59,    52, -1604,   171, -1604, -1604, -1604,   387,   346, -1604,
      35, -1604, -1604, -1604,    67,    67, -1604, -1604,   109, -1604,
   -1604, -1604, -1604, -1604,   365,   308, -1604, -1604, -1604, -1604,
     179,   179,   179,   360, -1604,   797,   201, -1604, -1604,   513,
     518,   527,   328,   506, -1604,   693,    73,    84,   520,   -93,
     453,   453,   575,   453,   600, -1604,   918,   918, -1604,   636,
     593,   138,   576,   862,   646,   692,   715, -1604,   743,   678,
   -1604, -1604,   -27,    84,   179,   179,   179,   179, -1604, -1604,
   -1604, -1604,   910, -1604, -1604,   753, -1604, -1604, -1604, -1604,
   -1604,   173, -1604, -1604, -1604, -1604, -1604,   919,    49,   696,
   -1604, -1604, -1604, -1604,   575,   575,   575,   921, -1604, -1604,
   -1604, -1604,   795, -1604, -1604, -1604, -1604,   593, -1604, -1604,
   -1604,   758, -1604, -1604, -1604, -1604, -1604,   784, -1604,   116,
   -1604,    72,   828,   797, -1604, -1604, -1604, -1604, -1604,     1,
     875, -1604,  -102, -1604, -1604, -1604,   925, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604,   194,   808, -1604,   793, -1604, -1604,
     948, -1604,   800,   173,   173, -1604, -1604, 15424,   909, -1604,
   -1604,   825, -1604,   597,    84,    84,   -60,   152, -1604, -1604,
   -1604,    49, -1604, -1604, 12228, -1604,   830,   173, -1604, -1604,
   14494, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,  1005,
    1008, -1604,   782,   173, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604,   173,   643,   841,   173, -1604,  -102,   240, -1604,
      84, -1604,   823,  1031,   809, -1604, -1604,   858,   859,   868,
     850,   871,   876, -1604, -1604, -1604,   860, -1604, -1604, -1604,
   -1604, -1604,   652, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604,   880, -1604, -1604, -1604,   882,   883,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604,   885,   894,   886,
     -64, -1604, -1604, -1604, -1604, -1604,   798,   878, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604,   922,   937, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604,   941,   897, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604,  1125, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604,   952,   911, -1604, -1604,   210,   -37,
   -1604, -1604, -1604,   549, -1604,   -64, -1604, -1604, -1604,   152,
     915, -1604, 11104,   954,   961, 12228, -1604,   -25, -1604, -1604,
   -1604, 11104, -1604, -1604,   962,   936,   -70,   -29,   149, -1604,
   -1604, 11104,   154, -1604, -1604, -1604,    21, -1604, -1604, -1604,
      12,  6692, -1604,   920, 11876,   699, 12053,   563, -1604, -1604,
   -1604, -1604,   965,   656,  1502,   923, -1604,   104,   955,   501,
     924, 12228, 12228, -1604,  3201,   643, 11104, -1604, -1604,    47,
     593, -1604,   946,   947, -1604, -1604,   928,   -92,   949,    42,
   -1604,   574,   929,   950,   951,   933,   953,   938,   618,   956,
   -1604,   625,   958,   959, 11104, 11104,   942,   943,   957,   963,
     964,   968, -1604, 11526, 11701,  6926, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604,   969,   972, -1604,  1152, 11104, 11104,
   11104, 11104, 11104,  6228, 11104, -1604,   971, -1604, -1604,  7160,
   -1604,   111, -1604, -1604, -1604, -1604,   960, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604,  2397, -1604,   975, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604,  1136,   990, -1604, -1604, -1604,  4828,
   12228, 12228, 12228,  2623, 12228, 12228,   978,   967, 12228,   782,
   12228,   782, 12228,   782, 12403,  1001,  2785, -1604, 11104, -1604,
   -1604, -1604, -1604, -1604,   973, -1604, -1604, 13969, 11104, -1604,
     798,   261, -1604,   -43, -1604, -1604,   216, -1604,   878,   597,
     982,   216, -1604,   597, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   11104, -1604, -1604,   639,   -88,   -88,   -88, -1604,   878,   878,
   -1604, 11104, -1604, -1604,   976,  4132, -1604,   173,  7392, -1604,
   11104,  1007, -1604,   576,  1016,  7624,   144,   999,  4364,   281,
     281,   281, -1604,  7856, -1604,   576,   576, 11104,  1214, -1604,
   -1604, -1604, -1604, -1604, -1604,  1188, -1604, -1604, -1604,   -42,
   -1604,   576,   576,   576,   576, -1604,   576, -1604, -1604,  1161,
   -1604,   306, -1604, 14686,   675, -1604,   593, -1604,   179,  1220,
   -1604,  -102, -1604, -1604, -1604, -1604,   987, -1604, -1604,   -64,
     626, -1604,  1011,  1012,  1014, -1604, 11104, 12228, 11104, 11104,
   -1604, -1604, 11104, -1604, 11104, -1604, 11104, -1604, -1604, 11104,
   -1604, 12228,   624,   624, 11104, 11104, 11104, 11104, 11104, 11104,
     639,  3433,   639,  3666,   639,  1074, -1604,   707, -1604, -1604,
     845,   997,   624,   624,     7,   624,   624,   180,  1228,  1000,
    1026, 14686,  1026,   310,   639,   597, -1604,  1027, -1604,  5060,
      57, 15087, 15142, 11104, 11104, -1604, -1604, 11104, 11104, 11104,
   11104,  1048, 11104,   676, 11104, 11104, 11104, 11104, 11104, 11104,
   11104, 11104, 11104,  8088, 11104, 11104, 11104, 11104, 11104, 11104,
   11104, 11104, 11104, 11104, 15323, 11104, -1604,  8320,  1050, -1604,
    4828, -1604,  1093, 11523,   623,   754,  1032,   411, -1604,   810,
     813, -1604, -1604,  1061,   816,   -37,   837,   -37,   843,   -37,
   -1604,   332, -1604,   472, -1604, 12228,  1044, -1604, -1604, 14076,
   -1604, -1604, 11104,  1045, 12228, -1604, -1604, 12228, -1604, -1604,
    2892,  1021,  1225, -1604, -1604,   245, -1604, -1604, -1604,   173,
     639,  1028,  4828, -1604,  1052,  2053, 11698,  1256, 11104, -1604,
    1075,   173,  1055, -1604,  1054,  1086, -1604, -1604, 12228,  4828,
   -1604, 11523,  1030, -1604,   960, -1604, -1604, -1604,   173, -1604,
   -1604,   173, -1604,   173, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604,   -90,   281, -1604,  5292,  5292,  5292,  5292,  5292,
    5292,  5292,  5292,  5292,  5292,  5292, 11104,  5292,  5292,  5292,
    5292,  5292,  5292, -1604, -1604,  1080,   287,   917,  1212,   576,
   12228, 12228, 12228,  6460,  8552,  1083, 11104, 12228, -1604, -1604,
   -1604, 12228,  1026,  1914,  1046, 12391, 12228, 12228, 12514, 12228,
   12549, 12228,  1026, 12228, 12403,  1026,  1001,   342, 12656, 12691,
   12798, 12833, 12940, 12975,    20,   281,  1049,    15,  3899,  5526,
    8784,  1128,  1077,    14,   442,  1078,   263,    28,  9016,    14,
     672,    33, 11104,  1088, -1604, 11104, -1604, 12228, 12228, -1604,
   11104,   142,   639,   639,    39,   193, -1604, 11104, -1604,  1053,
    1057,  1058,   174, -1604, -1604,    43, -1604, 11104, -1604,   -54,
    5760, -1604,   203,  1081,  1059,  1062,   -12,   782,  1082,  1064,
   -1604, -1604,  1085,  1068, -1604, -1604,    77,    77,  1230,  1230,
    2447,  2447,  1090,   488,  1094, -1604, 14111,   212,   212,   975,
      77,    77,  2106, 14916,  1173, 14782, 15269, 14647,  2354,  1610,
    1355,  1230,  1230,   230,   230,   488,   488,   488,   704, 11104,
    1095,  1098,   730, 11104,  1301,  1100, 14218, -1604,   311, -1604,
   -1604, 11873, 11104, 11104, 11104, 11104, 11104, 11104, 11104, 11104,
   11104, 11104, 11104, 11104, 11104, 11104, 11104, 11104, 11104, -1604,
   -1604, -1604, -1604, 12228, -1604, -1604, -1604,   499, -1604,  1092,
   -1604,  1103, -1604,  1104, -1604, 12403, -1604,  1001,   510,   878,
   -1604,  1118, -1604,   114, -1604,   878,   878, -1604, 11104,  1121,
   -1604,  1139, -1604, 12228, -1604, 11104, -1604,    46,   639, -1604,
    1052, 11104,   173, -1604,  1145, -1604, -1604, -1604, -1604,  1063,
   -1604, 11523, -1604,    57, -1604,    98, 11104, -1604,   960,  1147,
    1147, -1604, -1604, -1604,   281,   281,   281, -1604, -1604,   -42,
   -1604,   -42, -1604,   -42, -1604,   -42, -1604,   -42, -1604,   -42,
   -1604,   -42, -1604,   -42, -1604,   -42, -1604,   -42, -1604,   -42,
   -1604, -1604,   -42, -1604,   -42, -1604,   -42, -1604,   -42, -1604,
     -42, -1604,   -42,  1146,   576, -1604, -1604,   300, -1604,    51,
     593,  1401,  1650,   854,   733,   336,  1122,  1123,  1168,  1124,
     323,  1130,   855, 12228, 12403,  1001,  1769,  1131,  1135, 12228,
   -1604, -1604,  1939,  2158, -1604,  2175, -1604,  2410,  1143,  2636,
     538,  1144,   544,    57, -1604, -1604, -1604, -1604, -1604,  1137,
   11104, -1604, 11104, 11104, 11104,  5994, 13969,     3, 11104,   751,
     733,   442, -1604, -1604,  1148, -1604, 11104, -1604,  1151, 11104,
   -1604, 11104,   733,   822,  1162, -1604, -1604, 11104, -1604, -1604,
   -1604,   545,   612,  1185,    48,    60, 11104,   639,    63, 14686,
   -1604, 11104, 11104, 12228,   782, 11104, -1604,   198, -1604,  1164,
     322, 11336, -1604,   751, -1604, -1604,   -12,  1209,  1213,  1166,
    1215,  1218, -1604,   355,   -37, -1604, 11104, -1604, 11104,  9248,
   11104, -1604,  1187,  1174, -1604, -1604, 11104,  1177, -1604, 14253,
   11104,  9480,  1178, -1604, 14360, -1604,  9712, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
     878, -1604, -1604,  1219, -1604,  1226, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604,  1179, 12228, -1604, -1604,
    1045, 13082, -1604,  1381,   189, -1604, 11104,    74, -1604, 12228,
   11104,    57,   782,   173, -1604, -1604,   468, 11104, -1604,  1416,
    3201,    57, -1604,   402,   381, -1604, -1604, -1604, 12228,   593,
    1396,    51, -1604, -1604,   917, -1604, -1604, -1604, -1604,  1186,
   -1604, -1604, -1604,   653, -1604,  1190,  1234, -1604, -1604,  2646,
     665,   683, -1604, -1604, 11104,  2676, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604,  1192,  9944,   682, 13117, -1604,
   -1604,    14,   442, -1604,  1194,   248,  1077, -1604, -1604, -1604,
   -1604,  1078,   734,    14,  1197, -1604, -1604, -1604, -1604,   735,
   -1604, -1604, -1604,  1235, 11104, 11104,   738,    80, 11104, 13224,
   13259,  2847,   -37,   745, -1604,  1199,  5760,   389, -1604, -1604,
    1248, -1604, -1604,   -12,  1205,   333, 12228, 13366, 12228, 13401,
   -1604,   394, 13508, -1604, 11104, 14820, 11104, -1604, 13543,  5760,
   -1604,   405, 11104, -1604, -1604, -1604,   414, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604,   878, -1604,  1253, 11104,   406, 11104,
     350,   576,   844,   -37, -1604, -1604,   173, -1604,   576, -1604,
     782,  1254,  1210,   -86,   350, -1604, -1604, -1604,  1396,   639,
    1211,  1221, -1604, -1604, 11104,  1266, 11104,  1240, -1604, -1604,
   -1604, -1604,  1223,  1224,  1231, 11104, 11104, 11104,  1232,  1383,
    1233,  1236, 10176, -1604,   473, 11104,   255,   442, -1604, 11104,
   11104, 11104, 11104,   822, -1604, 11104, 11104,  1179, -1604, -1604,
     524,   531, 11104, 11104,   752, -1604, -1604, -1604,  1244, 11104,
   -1604,   417, -1604,  1229, -1604, -1604, 10408, -1604, -1604,  2924,
   -1604,   856, -1604, -1604, -1604, 12228, 13650, 13685, -1604,   498,
   -1604, 13792, -1604, -1604, -1604, -1604,   535, -1604, -1604, -1604,
     333,   133,  4596, -1604,   -37, -1604,   277, 12228,   -25,   593,
   15424, -1604, -1604, -1604, -1604,  1383,  1383,  1237,  1252,  1241,
    1243,  1245,  1246,  1247, 11104, -1604, 11104, -1604, -1604, -1604,
   11104, -1604, -1604,  1383,  1383, -1604, 13827, -1604, 14537, 11104,
   11104, -1604, 13934, -1604, -1604, 14537, -1604,   576, -1604, -1604,
    1277,  1275,  1278, 14537,   557, 11104,   537, -1604,   576,  1249,
   -1604, 11104, -1604, -1604, -1604,   863, -1604, -1604,  1255, -1604,
     173, -1604,   468, -1604, 10640, 10872, -1604, -1604, -1604, -1604,
   -1604, -1604,   173, 12228,   -25,  1037, 11104, -1604,   576, 15424,
     -34,   -34, -1604, 11104, -1604, 11104,  1383,  1383,   733,  1261,
    1274,  1026,   -34,   733, -1604,  1438,  1258, -1604, -1604,   290,
    1302,   597, -1604, 11104, 11104,  1279,  1310, 14537, -1604,   537,
     597, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, 11104,
   11104, -1604, -1604,  1037, 11104, 11104,   576,   593,   173,   733,
    1077,  1305, -1604,  1280,  1281,  1283,  1285,   -34,   -34,  1077,
    1286, -1604, -1604,  1288,  1289,  1290, 11104,  1293, 11104, 11104,
    1294,   597,   576, 14537, -1604, 11104,  1295, -1604, -1604, -1604,
   -1604, 11104,   576,   576, -1604, -1604,   593,   578,  1296, -1604,
   -1604, -1604, -1604, -1604,  1298,  1299, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604,  1330,  1303, 14537, -1604,
     576, -1604, -1604, -1604, -1604,   733, -1604, -1604, -1604, -1604,
    1307, -1604,   579, -1604, -1604
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   142,     1,   377,     0,     0,     0,   705,   378,     0,
     697,   697,   697,    17,    18,     0,     0,    16,    15,     3,
       0,    10,     9,     8,     0,     7,   686,     6,    11,     5,
       4,    13,    12,    14,   111,   112,   110,   120,   122,    47,
      66,    63,    64,     0,    49,    50,    51,     0,     0,    52,
      61,    48,   292,   291,   697,   697,    24,    23,   686,   699,
     698,   902,   892,   897,     0,   345,    45,   128,   129,   130,
       0,     0,     0,   131,   133,   140,     0,   127,    19,   719,
     718,   282,   707,   722,   687,   688,     0,     0,     0,     0,
       0,     0,    53,     0,     0,    62,     0,     0,    59,     0,
     626,   697,     0,    20,     0,     0,     0,   347,     0,     0,
     139,   134,     0,     0,     0,     0,     0,     0,   143,   721,
     720,   283,   285,   709,   708,     0,   724,   723,   727,   690,
     689,   692,   118,   119,   116,   117,   114,     0,     0,     0,
     113,   123,    67,    65,    55,    56,    54,    61,    58,    57,
     700,   623,   624,   702,   294,   293,   704,   626,   706,    21,
      22,    25,   903,   893,   898,   346,    43,    46,   138,     0,
     135,   136,   137,   141,   287,   286,   289,   284,   710,     0,
     715,   683,   609,    28,    29,    33,     0,   106,   107,   104,
     105,   103,   102,   108,     0,     0,    60,     0,   625,   703,
       0,    27,   809,     0,     0,    44,   132,     0,     0,   694,
     716,     0,   728,   684,     0,     0,   611,     0,    30,    31,
      32,     0,   121,   115,     0,    26,     0,     0,   894,   899,
       0,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,     0,
       0,   150,   144,     0,   785,   788,   791,   792,   786,   789,
     787,   790,     0,     0,   713,   725,   691,   609,     0,   124,
       0,   126,     0,   672,   670,   693,   109,     0,     0,     0,
       0,     0,     0,   735,   778,   736,   794,   737,   741,   742,
     743,   744,   784,   748,   749,   750,   751,   752,   753,   754,
     779,   780,   781,   782,   862,   740,   747,   783,   869,   876,
     738,   745,   739,   746,   755,   756,   757,   758,   759,   760,
     761,   762,   763,   764,   765,   766,   767,   768,   769,   770,
     771,   772,   773,   774,   775,   776,   777,     0,     0,     0,
       0,   793,   824,   827,   825,   826,   889,   701,   812,   813,
     810,   811,   802,   649,   655,   228,   229,   226,   153,   154,
     156,   155,   157,   158,   159,   160,   186,   187,   184,   185,
     177,   188,   189,   178,   175,   176,   227,   210,     0,   225,
     190,   191,   192,   193,   164,   165,   166,   161,   162,   163,
     174,     0,   180,   181,   179,   172,   173,   168,   167,   169,
     170,   171,   152,   151,   209,     0,   182,   183,   609,   147,
     322,   290,   694,   626,   711,     0,   717,   627,   729,     0,
       0,   125,     0,     0,     0,     0,   671,     0,   830,   853,
     856,     0,   859,   849,     0,     0,   863,   870,   877,   883,
     886,     0,   328,   839,   844,   838,     0,   852,   848,   841,
       0,     0,   843,   828,     0,     0,   895,   900,   230,   231,
     224,   208,   232,   211,   194,     0,   145,   376,   640,   641,
       0,     0,     0,   288,     0,     0,     0,   695,   714,   630,
     626,   610,     0,     0,   554,   555,     0,     0,     0,     0,
     549,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     784,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   598,     0,     0,     0,   430,   432,   431,   433,
     434,   435,   436,   437,     0,     0,    39,   291,     0,     0,
       0,     0,     0,   326,     0,   412,   413,   552,   551,   330,
     591,   508,   582,   581,   580,   579,   142,   585,   550,   584,
     583,   557,   509,   558,     0,   510,     0,   553,   905,   909,
     906,   907,   908,   674,   675,     0,   668,   669,   667,     0,
       0,     0,     0,     0,     0,     0,     0,   815,     0,   144,
       0,   144,     0,   144,     0,     0,     0,   835,   326,   834,
     846,   847,   840,   842,     0,   845,   829,     0,     0,   891,
     890,   805,   904,   345,   818,   819,     0,   650,   645,     0,
       0,     0,   656,     0,   233,   213,   214,   216,   215,   217,
     218,   219,   220,   212,   221,   222,   223,   197,   198,   200,
     199,   201,   202,   203,   204,   195,   196,   205,   206,   207,
       0,   374,   375,     0,   609,   609,   609,   146,   149,   148,
     324,     0,    77,    78,    91,   362,   360,     0,     0,   100,
       0,     0,   361,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   300,     0,   301,     0,     0,     0,     0,   317,
     312,   309,   308,   310,   311,   295,   344,   323,   303,   591,
     302,     0,    85,    86,    83,   315,    84,   316,   318,   380,
     307,     0,   304,   592,   438,   712,   626,   628,     0,     0,
     726,   609,   685,   951,   954,   350,   354,   353,   359,     0,
       0,   398,     0,     0,     0,   987,     0,     0,   330,     0,
     389,   392,     0,   395,     0,   991,     0,   960,   969,     0,
     957,     0,   537,   538,     0,     0,     0,     0,     0,     0,
       0,   929,     0,     0,     0,   967,   994,     0,   334,   337,
       0,     0,   514,   513,   547,   512,   511,     0,     0,     0,
    1005,   407,  1005,   414,     0,     0,   996,  1005,   589,     0,
     422,     0,     0,     0,     0,   539,   540,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   498,     0,   673,     0,     0,   677,
       0,   682,     0,   681,     0,     0,     0,   820,   833,     0,
       0,   795,   814,     0,     0,   147,     0,   147,     0,   147,
     647,     0,   653,     0,   796,     0,  1005,   837,   822,     0,
     803,   800,     0,   804,     0,   651,   896,     0,   657,   901,
       0,     0,   730,   637,   638,   660,   642,   643,   644,     0,
       0,     0,     0,   366,   363,   592,   438,     0,     0,   348,
       0,     0,     0,   321,     0,     0,    70,    95,     0,     0,
     371,   368,   413,   425,   142,   343,   341,   342,     0,   319,
     320,     0,    89,     0,   428,   298,   306,   313,   314,   365,
     370,   379,     0,     0,   305,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   297,   696,     0,     0,   617,   620,     0,
       0,     0,     0,   943,     0,     0,     0,     0,   977,   980,
     983,     0,  1005,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1005,     0,     0,  1005,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   345,     0,     0,
       0,   963,   921,   929,     0,   972,     0,     0,     0,   929,
       0,     0,     0,     0,   932,     0,   999,     0,     0,    42,
       0,    40,     0,     0,     0,     0,   974,  1006,   327,     0,
       0,     0,   485,   482,   484,     0,   998,  1006,   331,     0,
     326,   501,     0,  1005,     0,     0,     0,   144,     0,     0,
     568,   567,     0,     0,   569,   573,   515,   516,   528,   529,
     526,   527,     0,   563,     0,   545,     0,   586,   587,   588,
     517,   518,   533,   534,   535,   536,     0,     0,   531,   532,
     530,   524,   525,   520,   519,   521,   522,   523,     0,     0,
       0,   491,     0,     0,     0,     0,     0,   506,     0,   676,
     679,   438,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   680,
     831,   854,   857,     0,   860,   850,   797,     0,   864,     0,
     871,     0,   878,     0,   884,     0,   887,     0,     0,   332,
    1006,     0,   823,   808,   801,   646,   652,   639,     0,     0,
     659,     0,   658,     0,   661,     0,    96,     0,     0,   367,
     364,     0,     0,   349,     0,    97,    98,    68,    69,     0,
     372,   369,   414,   422,   325,    73,     0,   322,   142,     0,
       0,   388,   387,   340,     0,     0,     0,   460,   469,   448,
     470,   449,   472,   451,   471,   450,   473,   452,   463,   442,
     464,   443,   465,   444,   474,   453,   475,   454,   462,   440,
     441,   476,   455,   477,   456,   466,   445,   467,   446,   468,
     447,   461,   439,     0,   143,   618,   619,   620,   621,   612,
     626,     0,     0,     0,   944,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1000,   559,     0,     0,   560,     0,   590,     0,     0,     0,
       0,     0,     0,   422,   593,   594,   595,   596,   597,     0,
       0,   930,     0,     0,     0,     0,   407,   929,     0,     0,
       0,     0,   938,   939,     0,   946,     0,   936,     0,     0,
     975,     0,     0,     0,     0,   934,   976,     0,   966,   931,
     995,     0,     0,    36,     0,     0,     0,     0,     0,   408,
     556,     0,     0,     0,   144,     0,   997,     0,   502,     0,
       0,     0,   505,  1006,   920,   503,     0,     0,     0,     0,
       0,     0,   420,     0,   147,   564,     0,   570,     0,     0,
       0,   543,     0,     0,   574,   578,     0,     0,   546,     0,
       0,     0,     0,   492,     0,   499,     0,   541,   507,   678,
     448,   449,   451,   450,   452,   442,   443,   444,   453,   454,
     440,   455,   456,   445,   446,   447,   439,   832,   855,   858,
     821,   861,   851,     0,   816,     0,   865,   867,   872,   874,
     879,   881,   885,   648,   888,   654,   328,     0,   329,   806,
     807,     0,   732,   733,   663,   662,     0,     0,   373,     0,
       0,   422,   144,     0,    71,    72,    73,     0,    88,    79,
       0,   422,   381,     0,     0,   459,   457,   458,     0,   626,
     615,   612,   613,   614,   617,   631,   952,   955,   351,     0,
     356,   357,   355,     0,   401,     0,     0,   404,   399,     0,
       0,     0,   988,   986,   330,     0,   390,   393,   396,   992,
     990,   961,   970,   968,   958,     0,     0,     0,     0,   911,
     910,   929,     0,   964,     0,     0,   922,   945,   937,   965,
     935,   973,     0,   929,     0,   941,   942,   949,   933,     0,
     335,   338,    37,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   147,     0,   504,     0,   326,     0,   417,   418,
       0,   416,   415,     0,     0,     0,     0,     0,     0,     0,
     480,     0,     0,   575,     0,   548,     0,   544,     0,   326,
     493,     0,     0,   542,   500,   496,     0,   799,   817,   798,
     868,   875,   882,   836,   333,   731,     0,     0,     0,     0,
       0,     0,     0,   147,    74,    87,     0,    80,     0,   296,
     144,     0,     0,   670,     0,   636,   616,   632,   615,     0,
       0,     0,   352,   358,     0,     0,     0,     0,   400,   978,
     981,   984,     0,     0,     0,     0,     0,     0,     0,   943,
       0,     0,     0,   599,     0,     0,     0,     0,   947,     0,
       0,     0,     0,     0,   940,     0,     0,   328,    34,    41,
       0,     0,     0,     0,     0,   483,   608,   486,     0,     0,
     478,     0,   424,     0,   421,   423,     0,   409,   427,     0,
     607,     0,   605,   481,   602,     0,     0,     0,   601,     0,
     494,     0,   497,   734,   664,    92,     0,   101,    99,   299,
       0,    73,     0,    90,   147,   382,   670,     0,     0,   626,
       0,   634,   666,   665,   622,   943,   943,     0,     0,     0,
       0,     0,     0,     0,   326,  1001,   330,   391,   394,   397,
       0,   944,   962,   943,   943,   561,     0,   600,  1003,     0,
       0,   948,     0,   913,   912,  1003,   950,  1003,   336,   339,
      38,     0,     0,  1003,     0,     0,     0,   489,  1003,     0,
     419,     0,   410,   565,   571,     0,   606,   604,     0,   603,
       0,   426,    73,    75,   362,     0,    81,    85,    86,    83,
      84,    82,     0,     0,     0,     0,     0,   629,     0,     0,
     928,   928,   402,     0,   405,     0,   943,   943,   918,     0,
       0,  1005,   928,   918,   562,     0,     0,   915,   914,     0,
       0,     0,    35,     0,     0,     0,     0,  1003,   487,     0,
       0,   479,   411,   566,   572,   576,   495,    94,    76,     0,
       0,   368,   429,     0,     0,     0,     0,   626,     0,     0,
     925,  1005,   927,     0,     0,     0,     0,   928,   928,   919,
       0,   989,  1002,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1003,  1003,  1007,     0,     0,   490,  1014,   577,
     369,     0,     0,     0,   386,   633,   626,     0,  1006,   926,
     953,   956,   403,   406,     0,     0,   985,   993,   971,   959,
    1004,  1012,   917,   916,  1013,  1015,     0,     0,  1003,  1011,
       0,   385,   384,   635,   923,     0,   979,   982,  1010,  1008,
       0,   383,     0,  1009,   924
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1604, -1604,    -1, -1604, -1604, -1604, -1604, -1604,   750,  1458,
   -1604, -1604, -1604, -1604, -1604, -1604,  1546, -1604, -1604, -1604,
     610,   500, -1604,  1403, -1604, -1604,  1462, -1604, -1604, -1604,
   -1390, -1604, -1604, -1604,  -108, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604,  1334, -1604, -1604,   -46,
     -44, -1604, -1604, -1604,   629,   818,  -500,  -588,  -832, -1604,
   -1604, -1604, -1604, -1597, -1604, -1604,    -5,  -209,  -275,  -493,
   -1604,   370, -1604,  -612, -1356,  -751,  -161,  -284, -1604, -1604,
   -1604, -1604,  -567,     0, -1604, -1604, -1604, -1604, -1604,  -104,
     -99,   -98, -1604,   -97, -1604, -1604, -1604,  1565, -1604,   378,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604,  -403,   -91,   812,    53,   234, -1137,  -671,
   -1604,  -706, -1604, -1604,  -494,   399, -1604, -1604, -1604, -1603,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,  1126,
   -1604, -1604, -1604, -1604, -1604, -1604,  1406, -1604,  -168,   131,
      -4,   145,   338, -1604,  -154, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604,   456,  -459,  -955, -1604,  -475,  -954, -1604,  -864,
      -2,    11, -1604,  -590, -1487, -1604,  -421, -1604, -1604,  1528,
   -1604, -1604, -1604,  1149,  1132,   285, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604,  -722,   455, -1604,  1076, -1604, -1604,   440, -1604,
    1272, -1604, -1604, -1604,  -409, -1604, -1604,  -435, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604,  -173, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,  1087,
    -757,  -165,  -930,  -765, -1604, -1604, -1220,  -975, -1604, -1604,
   -1604, -1257,   -14, -1180, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604,   302,  -544, -1604, -1604, -1604,   819, -1604,
   -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604, -1604,
   -1604, -1604, -1604, -1381,  -785, -1604
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,   635,    18,   161,    58,   201,    19,   186,   192,
    1710,  1503,  1618,   790,   567,   167,   568,   109,    21,    22,
      49,    50,    51,    98,    23,    41,    42,   705,   706,  1427,
    1428,   707,  1568,  1662,   708,   709,  1186,   710,   900,   901,
     711,   712,   713,   714,  1420,   910,   193,   194,    37,    38,
      39,   216,    73,    74,    75,    76,    24,   439,   503,   282,
     122,    25,   176,   283,   177,   207,   569,   156,   923,  1197,
     717,   504,   718,   799,   619,   805,  1148,   570,  1027,  1616,
    1028,  1617,   720,   571,   721,   746,   972,  1582,   572,   722,
     723,   724,   725,   726,   727,   728,   673,   729,   942,  1433,
    1191,   730,   573,   986,  1595,   987,  1596,   989,  1597,   574,
     977,  1588,   575,   800,  1638,   576,  1342,  1343,  1057,   925,
     577,   963,  1188,   578,   852,  1198,   732,   579,   580,  1044,
     581,  1323,  1716,  1324,  1779,   582,  1104,  1544,   583,   733,
    1526,  1783,  1528,  1784,  1645,  1829,   785,   585,   497,  1444,
    1577,  1237,  1239,   969,   153,   509,   965,   741,  1670,  1749,
     498,   499,   500,   870,   871,   486,   872,   873,   487,  1284,
     893,   894,  1674,   599,   457,   304,   305,   213,   297,    85,
     131,    27,   182,   293,    99,   100,   197,   101,    28,    55,
     125,   179,    29,   446,   211,   212,    83,   128,   448,    30,
     180,   295,   895,   586,   292,   373,   374,  1137,   883,   485,
     227,   375,   863,  1548,  1145,   856,   483,   376,   600,  1387,
     875,   605,  1392,   601,  1388,   602,  1389,   604,  1391,   608,
    1396,   609,  1550,   610,  1398,   611,  1551,   612,  1400,   613,
    1552,   614,  1402,   615,  1404,   638,    31,   105,   203,   383,
     639,    32,   106,   204,   384,   643,    33,   104,   202,   587,
    1800,  1810,  1054,  1013,  1801,  1802,  1803,  1014,  1026,  1306,
    1300,  1295,  1497,  1247,   588,   970,  1580,   971,  1581,   996,
    1601,   993,  1599,  1015,   806,   589,   994,  1600,  1016,   590,
    1253,  1681,  1254,  1682,  1255,  1683,   981,  1592,   991,  1598,
     787,   807,   591,  1766,  1038,   592
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      17,   786,    54,   199,   296,   850,   876,   984,   441,   739,
     731,   716,   641,   935,   217,    66,    77,  1039,  1019,    78,
     595,   865,  1048,   867,  1012,   869,  1012,   636,   510,   926,
     927,  1165,   851,  1139,  1487,  1141,  1565,  1143,  1291,  1270,
    1280,   141,  1272,  1246,  1303,   622,  1422,   752,  1301,   630,
    1553,   377,  1053,  1307,   620,  -142,  1005,    94,  1004,  1316,
    1017,  1055,  1021,  1325,   187,   188,  1416,  1005,  1504,   169,
      77,    77,    77,  1748,    64,  1034,   810,   637,   642,  -866,
    1505,   214,  1045,  1508,   444,   302,  1668,   737,   132,   133,
     495,  1151,    95,  1189,  1559,   214,    40,  1442,   811,   812,
    1623,   158,    64,    34,    35,    65,   716,   303,   781,   783,
    1423,   671,   501,  1778,    77,    77,    77,    77,   903,  1329,
    -873,  1424,  1425,   108,    59,   596,   181,  1006,    13,   449,
      60,   920,     2,    65,  1667,   597,  1475,   456,  1337,     3,
     114,   115,   116,  1327,   215,  1423,    56,  1006,  1338,    14,
    -866,   749,  1798,  1190,   672,  -866,  1424,  1425,   215,   823,
     824,   802,     4,  1327,     5,  1160,     6,   753,   754,   299,
     298,    84,     7,  -866,   209,  1283,  1827,   884,  1167,  1744,
    1443,   108,     8,   502,  1006,  1339,   300,    52,     9,    87,
    1328,  -873,   716,   598,    57,    59,  -873,  1257,   228,   229,
    1327,    60,    13,   565,   922,   716,  1340,  1268,    53,  1799,
    1271,  1341,    10,    67,  -873,   301,  1053,   168,   815,   816,
     719,   208,   382,    14,   371,  1608,   821,   885,   822,   823,
     824,   825,   888,  1162,   154,   623,   826,  1161,   189,   114,
    1056,   116,    68,   190,   621,  1161,   191,   844,   845,   137,
    1161,   811,   812,   624,   451,   155,  1161,    11,    12,   625,
    1161,  1709,   134,  1161,    96,  1161,   755,   135,  1334,    89,
     136,  1733,  1193,   137,    86,    97,   632,  1161,   440,    88,
    1161,    36,   904,  1110,  1562,   756,  1426,   442,   738,   507,
     447,  1161,  1053,  1330,  1570,   715,   735,  1161,  -880,  1460,
     740,  1461,  1409,   839,   840,   841,   842,   843,   808,    69,
    1314,  1315,  1482,  1318,  1449,   719,   138,   844,   845,   139,
      13,  1732,   983,  -488,  1770,   154,  1771,   302,   678,   679,
      52,   881,  1775,    87,  1281,  1169,   997,  1780,    70,  1160,
     102,    14,  1788,   218,   219,    64,   155,  1440,  1160,   303,
    1701,    53,  1180,   982,   809,   891,   742,   716,  1283,    52,
     206,   821,    15,   992,   823,   824,   995,  1238,   891,  -880,
     472,   815,   816,    16,  -880,   473,    65,   892,   617,   821,
      53,   822,   823,   824,   825,   123,   157,   764,   495,   826,
     892,   124,  -880,   214,  -488,  1160,  1826,   618,  1160,  -488,
      90,   719,   474,   475,    13,  1160,  1052,  1162,  1610,   716,
     484,   221,   907,    71,   719,  1699,  1162,  -488,   117,  1692,
    1331,   917,    72,  1033,  1183,    14,   716,   854,   855,   857,
     886,   859,   860,   634,   889,   864,  1317,   866,   222,   868,
    1160,  1856,  1857,   118,  1108,   508,  1417,  1332,  1133,   880,
    1818,    13,   844,   845,   496,   371,   215,    87,   841,   842,
     843,   943,  1161,  1162,  1147,  1163,  1162,    52,  1164,  1344,
     844,   845,    14,  1162,   108,  1297,    64,  1870,   881,   882,
    1423,  1310,   450,  1179,   371,  1040,   371,  1053,    53,   476,
    1041,  1424,  1425,   477,    13,  1750,  1751,  1743,  1636,  1170,
     456,   371,   371,  1326,   117,  1298,  1607,    65,  1162,   811,
     812,    13,  1525,  1762,  1763,    14,   371,   750,  1613,    52,
      13,  1192,  1481,  1435,  1436,  1437,   565,   922,  1171,  1234,
    1012,  1804,    14,  1486,  1042,  1241,  1242,  1493,    13,  1289,
      53,    14,  1814,   371,   371,  1012,  1256,    92,  1455,   634,
    1557,  1262,  1263,  1171,  1265,  1368,  1267,  1144,  1269,    14,
      43,   478,   107,   126,  1515,   479,   719,  1273,   480,   127,
    1245,   484,  1523,   968,  1259,    93,  1807,  1808,    79,    80,
    1450,    81,   964,   481,    44,    45,    46,  1844,  1845,   482,
     144,   145,    52,   146,   849,  1507,  1046,  1524,  1571,    13,
     371,   371,   371,   113,   371,   371,  1523,   802,   371,    82,
     371,  1171,   371,    53,   371,   802,    91,    47,   719,  1571,
      14,  1611,  1171,  1171,  1166,  1573,    43,    48,   634,   815,
     816,  1171,   484,  1632,  1289,   719,  1175,   821,  1643,   822,
     823,   824,   825,   119,  1572,   811,   812,   826,   120,  1650,
      44,    45,    46,  1184,  1292,  1293,  1185,   121,  1652,  1719,
      13,   719,   719,   719,   719,   719,   719,   719,   719,   719,
     719,   719,  1405,   719,   719,   719,   719,   719,   719,   372,
    1628,    14,   440,    47,  1294,   -82,  1403,    13,  1431,   634,
    1171,   440,   913,    48,   440,   440,   440,  1146,    13,   110,
     111,   112,  1149,    64,   929,   930,   148,   149,    14,   142,
     675,  1155,  1040,  1593,  1156,  1289,  1393,  1697,   676,    14,
     936,   937,   938,   939,  1394,   940,    13,   634,   844,   845,
     944,  1660,    13,    13,    65,  1406,  1512,   151,    77,  1407,
    1728,  1171,  1700,   170,   171,   172,   173,    14,  1171,   974,
     129,    13,  1171,    14,    14,   634,   130,   371,   152,  1631,
     640,   634,   634,  1472,    13,   815,   816,   506,  1711,  1474,
    1500,   371,    14,   821,  1171,  1712,   823,   824,   825,  1730,
     634,   151,  1649,   826,   154,    14,  1025,   645,   646,   647,
     648,   649,   650,   651,   652,  1289,  1289,   147,   757,  1243,
      13,  1776,   152,  1043,  1252,   155,   945,   946,   947,   948,
     949,   950,   951,   952,   891,    97,   653,   758,  1459,   953,
     954,    14,  1864,  1874,  1465,   955,   654,   655,   656,   634,
     154,   473,  1742,   150,  1563,   956,   892,  1501,   957,   958,
     443,    13,   765,   162,   484,   959,   960,   961,  1130,   768,
     975,   155,  1129,    13,  1149,  1149,  1659,  1074,   474,   475,
      13,   766,    14,  1147,   844,   845,  1311,  1312,   769,   976,
     634,    13,   226,  1075,    14,   371,  -809,  1477,  1584,   166,
    1304,    14,   634,  1305,   371,  1357,   154,   371,  1511,   163,
    1590,  1605,    14,   962,   440,    13,   631,  1418,  1492,  1171,
     634,  1358,  1407,  1407,  1499,  1819,   440,   155,  1591,   378,
     372,  1362,   164,  1506,  1006,  1023,    14,  1024,   371,   159,
     565,   922,  1513,   440,   379,   160,   440,  1363,  1187,   380,
    1327,   381,  1484,   512,   513,  1760,   731,   716,   440,   372,
     165,   372,   195,  1612,  1615,   476,  1531,  1622,  1485,   477,
     178,  1171,  1171,   519,  1629,  1171,   372,   372,  1541,   521,
    1390,  1715,  1171,  1546,   114,   115,   116,   174,  1240,  1171,
     371,   371,   371,   175,  1235,   484,  1813,   371,    95,  1131,
    1236,   371,  1664,   198,   284,   205,   371,   371,   285,   371,
    1414,   371,   200,   371,   371,   114,   528,   529,   372,   372,
     440,  1809,   286,   287,   210,   223,  1809,   288,   289,   290,
     291,   224,  1290,  1558,   225,  1299,  1839,   478,  1290,  1299,
     226,   479,   294,   473,   480,   438,   454,   371,   371,   455,
    1494,   484,   456,  1495,   484,  1134,  1496,   484,  1135,   481,
     436,  1138,  1837,   437,  1585,   482,   183,   184,  1029,  1030,
     474,   475,  1147,   531,   532,   372,   372,   372,   484,   372,
     372,   445,  1140,   372,   484,   372,   452,   372,  1142,   372,
     473,  1707,  1759,  1604,   453,   484,   484,   484,  1746,  1448,
    1458,  1724,   458,   459,   484,  1718,  1445,  1602,  1785,   565,
     922,  1639,   460,   461,    64,   462,   473,   474,   475,   484,
     463,  1620,  1621,   464,   466,  1624,   467,   468,  1872,   469,
    1369,   543,   544,   545,    52,    44,    45,    46,   470,   488,
     183,   184,   185,   474,   475,    65,   218,   219,   220,   471,
     896,   897,   898,   371,   489,   557,  1395,   476,   490,   491,
     596,   477,    61,    62,    63,   371,   719,   492,  1564,   493,
     597,   593,   745,   494,  1794,  1795,  1656,   511,   594,   606,
     607,   628,   644,   371,   674,   791,   670,   440,   677,   563,
     743,   744,   759,   751,   760,   761,   762,   763,    13,   848,
     767,   764,   770,   771,   476,   774,   775,   596,   477,   440,
     440,   440,   862,   788,   811,   812,   789,   597,   640,    14,
     776,    16,   887,  1831,   912,   914,   777,   778,   598,   478,
     476,   779,   372,   479,   477,   877,   480,   803,   847,   -93,
    1714,  1569,   861,   918,   932,   933,   372,   941,   967,  1822,
     973,   481,  1745,  1439,  1554,   978,   979,   482,   980,  1022,
    1032,  1035,  1036,  1037,  1047,  1072,  1560,  1109,   943,  1456,
    1637,   811,   812,   371,   371,   598,   478,  1132,  1136,   371,
     479,  1150,  1154,   480,  1158,  1574,  1061,  1065,  1159,  1171,
    1172,  1168,  1174,  1176,  1177,  1178,  1182,  1233,   481,  1238,
    1250,  1079,   478,  1655,   482,  1575,   479,  1761,  1421,   480,
    1260,  1661,  1282,  1288,  1289,  1296,  1309,  1320,  1333,  1105,
    1321,  1322,  1365,  1335,   481,  1336,  1345,  1346,  1793,  1347,
     482,  1348,   813,   814,   815,   816,   817,  1397,  1412,   818,
     819,   820,   821,   371,   822,   823,   824,   825,  1399,  1401,
     372,   904,   826,  1349,   827,   828,  1413,  1350,  1360,   372,
     829,  1361,   372,  1366,  1432,  1200,  1202,  1204,  1206,  1208,
    1210,  1212,  1214,  1216,  1218,  1641,  1221,  1223,  1225,  1227,
    1229,  1231,  1408,  1419,  1438,  1453,  1451,  1452,  1454,   813,
     814,   815,   816,   372,  1457,  1463,   811,   812,  1464,   821,
    1476,   822,   823,   824,   825,  1637,  1170,  1470,  1473,   826,
    1488,   827,   828,  1490,   834,   835,   836,   837,   838,   839,
     840,   841,   842,   843,  1498,  1502,  1518,   371,  1514,  1520,
    1519,  1533,  1521,   844,   845,  1522,  1547,  1534,   440,   371,
    1536,  1542,   618,  1549,  1556,   372,   372,   372,  1567,  1576,
    1583,  1587,   372,  1586,   473,  1602,   372,  1609,   371,  1614,
    1619,   372,   372,  1630,   372,  1633,   372,  1635,   372,   372,
    1653,  1665,  1666,  1691,  1675,  1787,   839,   840,   841,   842,
     843,   474,   475,  1678,  1676,  1680,  1684,  1792,  1685,  1717,
     844,   845,  1725,  1720,  1686,  1690,  1693,  1753,  1772,  1694,
    1290,  1752,   372,   372,  1773,  1754,  1755,  1774,  1756,  1757,
    1758,  1816,  1290,  1781,   813,   814,   815,   816,   817,  1786,
    1817,   818,   819,   820,   821,  1811,   822,   823,   824,   825,
    1820,   924,   924,   924,   826,  1747,   827,   828,  1812,  1825,
     440,  1824,  1838,  1836,  1840,  1841,   371,  1842,   371,  1843,
    1846,   934,  1847,  1848,  1849,  1851,  1854,  1859,  1868,  1865,
    1031,  1355,  1866,  1867,   140,  1869,   934,    20,   476,  1873,
     196,   143,   477,   440,  1736,   306,   966,  1430,  1737,  1657,
    1658,   440,  1821,  1738,  1739,  1740,    26,  1663,  1434,  1731,
    1517,  1828,  1578,  1669,  1671,  1441,  1634,  1672,   584,   837,
     838,   839,   840,   841,   842,   843,   103,   603,   372,  1579,
    1673,   505,   747,  1410,   465,   844,   845,   616,  1815,  1706,
     372,  1491,  1020,   748,     0,     0,     0,   627,     0,     0,
       0,     0,  1855,     0,     0,     0,     0,     0,   372,     0,
     478,     0,     0,     0,   479,     0,  1446,   480,     0,     0,
       0,   811,   812,   657,   658,   659,   660,   661,   662,   663,
     664,     0,   481,  1835,     0,   371,     0,     0,   482,     0,
       0,     0,   665,     0,     0,   440,     0,     0,     0,     0,
     772,   773,   666,     0,     0,   934,     0,   371,     0,     0,
       0,     0,   667,   668,   669,     0,     0,     0,     0,     0,
       0,     0,  1863,   473,   792,   793,   794,   795,   796,   801,
     801,     0,     0,     0,     0,     0,     0,  1765,     0,     0,
       0,     0,     0,     0,  1765,     0,  1765,     0,   372,   372,
     474,   475,  1765,     0,   372,     0,  1043,  1765,   934,     0,
       0,     0,     0,     0,     0,   440,     0,     0,     0,     0,
       0,     0,     0,   934,     0,     0,     0,   440,     0,     0,
       0,     0,     0,   371,   801,     0,     0,  1797,     0,   813,
     814,   815,   816,   817,   879,   924,   818,   819,   820,   821,
       0,   822,   823,   824,   825,     0,     0,     0,     0,   826,
       0,   827,   828,     0,     0,     0,  1765,     0,   372,  1043,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   440,     0,  1834,   890,   476,     0,     0,
       0,   477,   473,     0,     0,     0,     0,     0,     0,     0,
       0,   905,     0,     0,     0,     0,     0,   924,     0,     0,
       0,  1765,  1765,     0,     0,     0,     0,     0,     0,   474,
     475,  1861,  1862,   836,   837,   838,   839,   840,   841,   842,
     843,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     844,   845,     0,     0,     0,     0,     0,  1765,     0,  1871,
       0,     0,   372,     0,     0,     0,     0,     0,     0,   478,
       0,     0,     0,   479,   372,  1447,   480,     0,     0,     0,
       0,     0,   801,     0,     0,   985,     0,     0,   988,     0,
     990,   481,   801,   372,     0,   801,     0,   482,     0,     0,
     998,   999,  1000,  1001,  1002,  1003,     0,     0,     0,     0,
     734,     0,   736,     0,     0,     0,   476,     0,     0,     0,
     477,     0,     0,   934,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   801,     0,     0,     0,  1066,
    1067,     0,     0,  1068,  1069,  1070,  1071,   473,  1073,     0,
    1076,  1077,  1078,  1080,  1081,  1082,  1083,  1084,  1085,  1087,
    1088,  1089,  1090,  1091,  1092,  1093,  1094,  1095,  1096,  1097,
       0,  1106,   473,   801,   474,   475,     0,     0,     0,     0,
       0,   372,     0,   372,     0,     0,     0,     0,   478,     0,
       0,     0,   479,   934,  1462,   480,     0,     0,     0,   474,
     475,     0,     0,     0,     0,   853,   924,   924,   924,     0,
     481,   934,     0,   934,     0,   934,   482,   934,     0,   934,
       0,   934,     0,   934,     0,   934,     0,   934,   905,   934,
       0,   934,     0,     0,   934,     0,   934,     0,   934,     0,
     934,     0,   934,     0,   934,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   476,     0,  -407,     0,   477,     0,     0,     0,     0,
       0,     0,     0,     0,   811,   812,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   476,   899,     0,     0,
     477,   906,     0,     0,   909,     0,   911,     0,     0,   801,
     372,   916,    13,     0,   921,     0,     0,     0,     0,   928,
       0,     0,     0,   931,     0,     0,     0,     0,     0,     0,
       0,     0,   372,    14,     0,     0,     0,   811,   812,     0,
       0,   634,     0,   478,   792,  1286,   801,   479,     0,  1258,
     480,     0,     0,     0,   801,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   481,     0,     0,   478,     0,
       0,   482,   479,  1319,  1466,   480,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   801,     0,     0,     0,
     481,     0,     0,     0,     0,     0,   482,  1011,     0,  1011,
       0,   473,   813,   814,   815,   816,   817,     0,   372,   818,
     819,   820,   821,     0,   822,   823,   824,   825,   473,     0,
       0,     0,   826,     0,   827,   828,     0,     0,   474,   475,
     829,   830,   831,     0,     0,  1359,   832,     0,     0,  1364,
       0,     0,     0,     0,     0,   474,   475,     0,     0,     0,
       0,  -407,     0,     0,     0,   813,   814,   815,   816,   817,
       0,     0,   818,   819,   820,   821,  1111,   822,   823,   824,
     825,     0,  -407,     0,     0,   826,     0,   827,   828,     0,
    -407,     0,   833,     0,   834,   835,   836,   837,   838,   839,
     840,   841,   842,   843,  1411,     0,     0,     0,  1153,     0,
       0,     0,     0,   844,   845,     0,     0,  1319,     0,     0,
       0,     0,     0,     0,     0,   476,     0,     0,   906,   477,
       0,     0,     0,     0,  1173,     0,     0,     0,     0,     0,
       0,     0,   476,     0,     0,  1181,   477,   834,   835,   836,
     837,   838,   839,   840,   841,   842,   843,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   844,   845,     0,     0,
       0,  1199,  1201,  1203,  1205,  1207,  1209,  1211,  1213,  1215,
    1217,  1219,  1220,  1222,  1224,  1226,  1228,  1230,  1232,     0,
       0,     0,     0,     0,   934,   811,   812,   478,     0,     0,
    1249,   479,  1251,  1467,   480,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   478,     0,     0,     0,   479,   481,
    1468,   480,     0,     0,     0,   482,   801,     0,  1478,     0,
       0,   801,     0,     0,     0,     0,   481,     0,   811,   812,
       0,     0,   482,     0,     0,     0,     0,   801,  1308,     0,
       0,     0,     0,   801,     0,     0,  1313,     0,     0,     0,
       0,     0,   801,   473,     0,     0,     0,  1509,  1510,     0,
       0,   801,     0,     0,     0,     0,     0,  1319,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   811,   812,
     474,   475,  1527,     0,  1529,   801,  1532,     0,     0,     0,
       0,     0,  1535,     0,     0,     0,  1538,   801,     0,     0,
       0,     0,   801,   813,   814,   815,   816,   817,     0,     0,
     818,   819,   820,   821,     0,   822,   823,   824,   825,     0,
       0,     0,     0,   826,     0,   827,   828,     0,  1370,  1371,
    1372,  1373,  1374,  1375,  1376,  1377,  1378,  1379,  1380,  1381,
    1382,  1383,  1384,  1385,  1386,     0,   813,   814,   815,   816,
     817,     0,   801,   818,   819,   820,   821,     0,   822,   823,
     824,   825,     0,     0,     0,     0,   826,   476,   827,   828,
       0,   477,     0,     0,   829,   830,   831,     0,     0,     0,
     832,  1415,     0,     0,     0,     0,   835,   836,   837,   838,
     839,   840,   841,   842,   843,     0,   813,   814,   815,   816,
     817,     0,  1429,   818,   844,   845,   821,     0,   822,   823,
     824,   825,   801,     0,     0,     0,   826,     0,   827,   828,
       0,     0,     0,     0,     0,     0,   833,     0,   834,   835,
     836,   837,   838,   839,   840,   841,   842,   843,     0,   478,
     801,   801,     0,   479,   801,  1469,   480,   844,   845,     0,
       0,   846,   801,     0,   811,   812,     0,     0,     0,     0,
       0,   481,     0,     0,     0,     0,     0,   482,     0,     0,
    1646,     0,  1647,     0,     0,   801,     0,     0,  1651,   473,
       0,   837,   838,   839,   840,   841,   842,   843,     0,   473,
       0,     0,     0,     0,     0,   801,     0,   844,   845,  1479,
    1480,     0,     0,     0,  1483,     0,   474,   475,     0,     0,
       0,     0,  1489,     0,     0,  1011,   474,   475,     0,   473,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1696,     0,
       0,  1698,     0,     0,     0,  1702,   474,   475,  1705,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1713,   801,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   813,   814,   815,   816,   817,     0,     0,   818,
     819,   820,   821,     0,   822,   823,   824,   825,     0,     0,
       0,     0,   826,   476,   827,   828,     0,   477,     0,     0,
     829,   830,   831,   476,     0,     0,   832,   477,     0,     0,
       0,     0,     0,     0,     0,     0,   811,   812,     0,     0,
     801,     0,     0,     0,     0,     0,   801,     0,     0,     0,
       0,     0,     0,   476,     0,     0,  1561,   477,     0,     0,
       0,     0,     0,  1566,     0,     0,   734,     0,     0,     0,
       0,  1777,   833,     0,   834,   835,   836,   837,   838,   839,
     840,   841,   842,   843,     0,   478,     0,     0,     0,   479,
     801,  1471,   480,   844,   845,   478,     0,   858,     0,   479,
       0,  1589,   480,     0,     0,     0,     0,   481,     0,     0,
     473,     0,     0,   482,     0,     0,     0,   481,     0,     0,
       0,     0,     0,   482,     0,   478,     0,     0,     0,   479,
    1823,  1594,   480,     0,     0,     0,     0,   474,   475,     0,
       0,     0,     0,   811,   812,   801,     0,   481,     0,     0,
       0,     0,     0,   482,   813,   814,   815,   816,   817,     0,
       0,   818,   819,   820,   821,     0,   822,   823,   824,   825,
       0,     0,     0,     0,   826,     0,   827,   828,     0,     0,
       0,  1858,   829,   830,   831,     0,     0,   473,   832,     0,
       0,     0,     0,  1654,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   474,   475,     0,     0,     0,     0,
    1677,     0,  1679,     0,   476,     0,     0,     0,   477,     0,
       0,  1687,  1688,  1689,   833,     0,   834,   835,   836,   837,
     838,   839,   840,   841,   842,   843,  1703,  1704,     0,     0,
       0,     0,  1708,     0,     0,   844,   845,     0,     0,   874,
       0,   813,   814,   815,   816,   817,     0,     0,   818,   819,
     820,   821,  1722,   822,   823,   824,   825,     0,     0,     0,
       0,   826,     0,   827,   828,     0,     0,     0,     0,   829,
     830,   831,     0,     0,     0,   832,   478,     0,  1741,     0,
     479,   476,  1627,   480,     0,   477,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   481,     0,
       0,     0,     0,     0,   482,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1767,  1768,     0,     0,     0,
       0,   833,     0,   834,   835,   836,   837,   838,   839,   840,
     841,   842,   843,     0,     0,     0,     0,  1782,     0,     0,
       0,     0,   844,   845,     0,     0,  1157,     0,     0,     0,
       0,  1791,     0,   478,     0,     0,     0,   479,     0,  1723,
     480,     0,  1796,     0,     0,     0,     0,     0,     0,  1805,
       0,  1806,     0,     0,     0,   481,     0,     0,     0,     0,
       0,   482,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1830,     0,     0,     0,
    1832,  1833,   680,     0,     0,     0,   512,   513,     3,     0,
     681,   682,   683,     0,   684,     0,   514,   515,   516,   517,
     518,     0,  1850,     0,  1852,  1853,   519,   685,   520,   686,
     687,     0,   521,     0,     0,     0,     0,  1860,     0,   688,
     522,   689,     0,   690,     0,   691,   523,     0,     0,   524,
       0,     8,   525,   692,     0,   693,   526,     0,     0,   694,
     695,     0,     0,     0,     0,     0,   696,     0,     0,   528,
     529,     0,   313,   314,   315,     0,   317,   318,   319,   320,
     321,   530,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,     0,   335,   336,   337,     0,     0,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   531,   532,   697,   698,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   534,   535,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   699,   700,   701,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   536,   537,   538,   539,   540,
       0,   541,     0,   542,   543,   544,   545,    52,   154,   702,
     546,   547,   548,   549,   550,   551,   552,   553,    65,   703,
     555,   556,     0,     0,     0,     0,     0,     0,   557,   155,
     704,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   558,   559,   560,
       0,    15,     0,     0,   561,   562,     0,     0,   512,   513,
       0,     0,   563,     0,   564,     0,   565,   566,   514,   515,
     516,   517,   518,     0,     0,     0,     0,     0,   519,     0,
     520,     0,     0,     0,   521,     0,   473,     0,     0,     0,
       0,     0,   522,     0,     0,     0,     0,     0,   523,     0,
       0,   524,     0,     0,   525,     0,  1005,     0,   526,     0,
       0,     0,     0,   474,   475,     0,     0,     0,   527,     0,
       0,   528,   529,     0,   313,   314,   315,     0,   317,   318,
     319,   320,   321,   530,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,     0,   335,   336,   337,     0,
       0,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   531,   532,
     533,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   534,   535,     0,     0,     0,     0,
     476,     0,     0,     0,   477,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
       0,     0,     0,     0,     0,     0,     0,   536,   537,   538,
     539,   540,     0,   541,  1006,   542,   543,   544,   545,    52,
       0,     0,   546,   547,   548,   549,   550,   551,   552,   553,
    1007,   554,   555,   556,     0,     0,     0,     0,     0,     0,
     557,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   478,     0,     0,     0,   479,     0,     0,  1008,
     559,   560,     0,    15,     0,     0,   561,   562,     0,     0,
       0,   512,   513,     0,  1009,     0,  1010,     0,   565,   566,
     482,   514,   515,   516,   517,   518,     0,     0,     0,     0,
       0,   519,     0,   520,     0,     0,     0,   521,     0,   473,
       0,     0,     0,     0,     0,   522,     0,     0,     0,     0,
       0,   523,     0,     0,   524,     0,     0,   525,     0,     0,
       0,   526,     0,     0,     0,     0,   474,   475,     0,     0,
       0,   527,     0,     0,   528,   529,     0,   313,   314,   315,
       0,   317,   318,   319,   320,   321,   530,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   335,
     336,   337,     0,     0,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   531,   532,   533,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   534,   535,     0,
       0,     0,     0,   476,     0,     0,     0,   477,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     536,   537,   538,   539,   540,     0,   541,  1006,   542,   543,
     544,   545,    52,     0,     0,   546,   547,   548,   549,   550,
     551,   552,   553,  1007,   554,   555,   556,     0,     0,     0,
       0,     0,     0,   557,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   478,     0,     0,     0,   479,
       0,     0,  1008,   559,   560,     0,    15,     0,     0,   561,
     562,     0,     0,     0,   512,   513,     0,  1009,     0,  1018,
       0,   565,   566,   482,   514,   515,   516,   517,   518,     0,
       0,     0,     0,     0,   519,     0,   520,     0,     0,     0,
     521,     0,   622,     0,     0,     0,     0,     0,   522,     0,
       0,     0,     0,     0,   523,     0,     0,   524,     0,     0,
     525,     0,     0,     0,   526,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   527,     0,     0,   528,   529,     0,
     313,   314,   315,     0,   317,   318,   319,   320,   321,   530,
     323,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,     0,   335,   336,   337,     0,     0,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   531,   532,   533,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     534,   535,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    64,     0,     0,     0,     0,
       0,     0,     0,   536,   537,   538,   539,   540,     0,   541,
       0,   542,   543,   544,   545,    52,     0,     0,   546,   547,
     548,   549,   550,   551,   552,   553,    65,   554,   555,   556,
       0,     0,     0,     0,     0,     0,   557,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   623,     0,     0,   558,   559,   560,     0,    15,
       0,     0,   561,   562,     0,     0,     0,   512,   513,     0,
    1285,     0,   564,     0,   565,   566,   625,   514,   515,   516,
     517,   518,     0,     0,     0,     0,     0,   519,     0,   520,
       0,     0,     0,   521,     0,     0,     0,     0,     0,     0,
       0,   522,     0,     0,     0,     0,     0,   523,     0,     0,
     524,     0,     0,   525,     0,     0,     0,   526,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   527,     0,     0,
     528,   529,     0,   313,   314,   315,     0,   317,   318,   319,
     320,   321,   530,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   335,   336,   337,     0,     0,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   531,   532,   697,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   534,   535,     0,     0,     0,     0,     0,
       0,     0,   902,     0,     0,     0,     0,     0,   699,   700,
     701,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   536,   537,   538,   539,
     540,     0,   541,     0,   542,   543,   544,   545,    52,     0,
       0,   546,   547,   548,   549,   550,   551,   552,   553,    65,
     554,   555,   556,     0,     0,     0,     0,     0,     0,   557,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   558,   559,
     560,     0,    15,     0,     0,   561,   562,     0,     0,   512,
     513,     0,     0,   563,     0,   564,     0,   565,   566,   514,
     515,   516,   517,   518,     0,     0,     0,     0,     0,   519,
       0,   520,     0,     0,     0,   521,     0,     0,     0,     0,
       0,     0,     0,   522,     0,     0,     0,     0,     0,   523,
       0,     0,   524,     0,     0,   525,     0,     0,     0,   526,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   527,
       0,     0,   528,   529,     0,   313,   314,   315,     0,   317,
     318,   319,   320,   321,   530,   323,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,     0,   335,   336,   337,
       0,     0,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   531,
     532,   697,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   534,   535,     0,     0,     0,
       0,     0,     0,     0,   919,     0,     0,     0,     0,     0,
     699,   700,   701,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   536,   537,
     538,   539,   540,     0,   541,     0,   542,   543,   544,   545,
      52,     0,     0,   546,   547,   548,   549,   550,   551,   552,
     553,    65,   554,   555,   556,     0,     0,     0,     0,     0,
       0,   557,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     558,   559,   560,     0,    15,     0,     0,   561,   562,     0,
       0,   512,   513,     0,     0,   563,     0,   564,     0,   565,
     566,   514,   515,   516,   517,   518,     0,     0,     0,     0,
       0,   519,  1734,   520,   686,     0,     0,   521,     0,     0,
       0,     0,     0,     0,     0,   522,     0,     0,     0,     0,
       0,   523,     0,     0,   524,     0,     0,   525,   692,     0,
       0,   526,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   527,     0,     0,   528,   529,     0,   313,   314,   315,
       0,   317,   318,   319,   320,   321,   530,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   335,
     336,   337,     0,     0,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   531,   532,   533,  1735,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   534,   535,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     536,   537,   538,   539,   540,     0,   541,     0,   542,   543,
     544,   545,    52,     0,     0,   546,   547,   548,   549,   550,
     551,   552,   553,    65,   554,   555,   556,     0,     0,     0,
       0,     0,     0,   557,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   558,   559,   560,     0,    15,     0,     0,   561,
     562,     0,     0,   512,   513,     0,     0,   563,     0,   564,
       0,   565,   566,   514,   515,   516,   517,   518,     0,     0,
       0,     0,     0,   519,     0,   520,     0,     0,     0,   521,
       0,     0,     0,     0,     0,     0,     0,   522,     0,     0,
       0,     0,     0,   523,     0,     0,   524,     0,     0,   525,
       0,     0,     0,   526,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   527,     0,     0,   528,   529,     0,   313,
     314,   315,     0,   317,   318,   319,   320,   321,   530,   323,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
       0,   335,   336,   337,     0,     0,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   531,   532,   697,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   534,
     535,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   699,   700,   701,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   536,   537,   538,   539,   540,     0,   541,     0,
     542,   543,   544,   545,    52,     0,     0,   546,   547,   548,
     549,   550,   551,   552,   553,    65,   554,   555,   556,     0,
       0,     0,     0,     0,     0,   557,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   558,   559,   560,     0,    15,     0,
       0,   561,   562,     0,     0,   512,   513,     0,     0,   563,
       0,   564,     0,   565,   566,   514,   515,   516,   517,   518,
       0,     0,     0,     0,     0,   519,     0,   520,     0,     0,
       0,   521,     0,     0,     0,     0,     0,     0,     0,   522,
       0,     0,     0,     0,     0,   523,     0,     0,   524,     0,
       0,   525,     0,     0,     0,   526,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   527,     0,     0,   528,   529,
    1049,   313,   314,   315,     0,   317,   318,   319,   320,   321,
     530,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,     0,   335,   336,   337,     0,     0,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   531,   532,   533,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   534,   535,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   536,   537,   538,   539,   540,     0,
     541,  1006,   542,   543,   544,   545,    52,     0,     0,   546,
     547,   548,   549,   550,   551,   552,   553,  1007,   554,   555,
     556,     0,     0,     0,     0,     0,     0,   557,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   558,   559,   560,     0,
      15,     0,     0,   561,   562,     0,     0,   512,   513,     0,
       0,  1050,     0,   564,  1051,   565,   566,   514,   515,   516,
     517,   518,     0,     0,     0,     0,     0,   519,     0,   520,
       0,     0,     0,   521,     0,     0,     0,     0,     0,     0,
       0,   522,     0,     0,     0,     0,     0,   523,     0,     0,
     524,     0,     0,   525,     0,     0,     0,   526,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   527,     0,     0,
     528,   529,     0,   313,   314,   315,     0,   317,   318,   319,
     320,   321,   530,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   335,   336,   337,     0,     0,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   531,   532,   697,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   534,   535,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1194,  1195,
    1196,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   536,   537,   538,   539,
     540,     0,   541,     0,   542,   543,   544,   545,    52,     0,
       0,   546,   547,   548,   549,   550,   551,   552,   553,    65,
     554,   555,   556,     0,     0,     0,     0,     0,     0,   557,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   558,   559,
     560,     0,    15,     0,     0,   561,   562,     0,     0,     0,
       0,   512,   513,   563,     0,   564,     0,   565,   566,   797,
       0,   514,   515,   516,   517,   518,     0,     0,     0,     0,
       0,   519,     0,   520,     0,     0,     0,   521,     0,     0,
       0,     0,     0,     0,     0,   522,     0,     0,     0,     0,
       0,   523,     0,     0,   524,   798,     0,   525,     0,     0,
       0,   526,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   527,     0,     0,   528,   529,     0,   313,   314,   315,
       0,   317,   318,   319,   320,   321,   530,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   335,
     336,   337,     0,     0,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   531,   532,   533,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   534,   535,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     536,   537,   538,   539,   540,     0,   541,     0,   542,   543,
     544,   545,    52,     0,     0,   546,   547,   548,   549,   550,
     551,   552,   553,    65,   554,   555,   556,     0,     0,     0,
       0,     0,     0,   557,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   558,   559,   560,     0,    15,     0,     0,   561,
     562,     0,     0,     0,     0,   512,   513,   563,   626,   564,
       0,   565,   566,   797,     0,   514,   515,   516,   517,   518,
       0,     0,     0,     0,     0,   519,     0,   520,     0,     0,
       0,   521,     0,     0,     0,     0,     0,     0,     0,   522,
       0,     0,     0,     0,     0,   523,     0,     0,   524,   798,
       0,   525,     0,     0,     0,   526,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   527,     0,     0,   528,   529,
       0,   313,   314,   315,     0,   317,   318,   319,   320,   321,
     530,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,     0,   335,   336,   337,     0,     0,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   531,   532,   533,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   534,   535,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   536,   537,   538,   539,   540,     0,
     541,  1006,   542,   543,   544,   545,    52,     0,     0,   546,
     547,   548,   549,   550,   551,   552,   553,  1007,   554,   555,
     556,     0,     0,     0,     0,     0,     0,   557,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   558,   559,   560,     0,
      15,     0,     0,   561,   562,     0,     0,     0,     0,   512,
     513,   563,     0,   564,     0,   565,   566,   797,     0,   514,
     515,   516,   517,   518,     0,     0,     0,     0,     0,   519,
       0,   520,     0,     0,     0,   521,     0,     0,     0,     0,
       0,     0,     0,   522,     0,     0,     0,     0,     0,   523,
       0,     0,   524,   798,     0,   525,     0,     0,     0,   526,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   527,
       0,     0,   528,   529,     0,   313,   314,   315,     0,   317,
     318,   319,   320,   321,   530,   323,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,     0,   335,   336,   337,
       0,     0,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   531,
     532,   533,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   534,   535,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   536,   537,
     538,   539,   540,     0,   541,     0,   542,   543,   544,   545,
      52,     0,     0,   546,   547,   548,   549,   550,   551,   552,
     553,    65,   554,   555,   556,     0,     0,     0,     0,     0,
       0,   557,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     558,   559,   560,     0,    15,     0,     0,   561,   562,     0,
       0,     0,     0,   512,   513,   563,   877,   564,     0,   565,
     566,   797,     0,   514,   515,   516,   517,   518,     0,     0,
       0,     0,     0,   519,     0,   520,     0,     0,     0,   521,
       0,     0,     0,     0,     0,     0,     0,   522,     0,     0,
       0,     0,     0,   523,     0,     0,   524,   798,     0,   525,
       0,     0,     0,   526,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   527,     0,     0,   528,   529,     0,   313,
     314,   315,     0,   317,   318,   319,   320,   321,   530,   323,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
       0,   335,   336,   337,     0,     0,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   531,   532,   533,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   534,
     535,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   536,   537,   538,   539,   540,     0,   541,     0,
     542,   543,   544,   545,    52,     0,     0,   546,   547,   548,
     549,   550,   551,   552,   553,    65,   554,   555,   556,     0,
       0,     0,     0,     0,     0,   557,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   558,   559,   560,     0,    15,     0,
       0,   561,   562,     0,     0,   512,   513,     0,     0,   563,
       0,   564,     0,   565,   566,   514,   515,   516,   517,   518,
       0,     0,     0,     0,     0,   519,     0,   520,     0,     0,
       0,   521,     0,     0,     0,     0,     0,     0,     0,   522,
       0,     0,     0,     0,     0,   523,     0,     0,   524,     0,
       0,   525,     0,     0,     0,   526,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   527,     0,     0,   528,   529,
    1244,   313,   314,   315,     0,   317,   318,   319,   320,   321,
     530,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,     0,   335,   336,   337,     0,     0,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   531,   532,   533,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   534,   535,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   536,   537,   538,   539,   540,     0,
     541,  1006,   542,   543,   544,   545,    52,     0,     0,   546,
     547,   548,   549,   550,   551,   552,   553,  1007,   554,   555,
     556,     0,     0,     0,     0,     0,     0,   557,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   558,   559,   560,     0,
      15,     0,     0,   561,   562,     0,     0,   512,   513,     0,
       0,   563,     0,   564,     0,   565,   566,   514,   515,   516,
     517,   518,     0,     0,     0,     0,     0,   519,     0,   520,
       0,     0,     0,   521,     0,     0,     0,     0,     0,     0,
       0,   522,     0,     0,     0,     0,     0,   523,     0,     0,
     524,     0,     0,   525,     0,     0,     0,   526,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   527,     0,     0,
     528,   529,     0,   313,   314,   315,     0,   317,   318,   319,
     320,   321,   530,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   335,   336,   337,     0,     0,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   531,   532,   533,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   534,   535,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   536,   537,   538,   539,
     540,     0,   541,     0,   542,   543,   544,   545,    52,     0,
       0,   546,   547,   548,   549,   550,   551,   552,   553,    65,
     554,   555,   556,     0,     0,     0,     0,     0,     0,   557,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   558,   559,
     560,     0,    15,     0,     0,   561,   562,     0,     0,     0,
       0,   512,   513,   563,   626,   564,     0,   565,   566,   784,
       0,   514,   515,   516,   517,   518,     0,     0,     0,     0,
       0,   519,     0,   520,     0,     0,     0,   521,     0,     0,
       0,     0,     0,     0,     0,   522,     0,     0,     0,     0,
       0,   523,     0,     0,   524,     0,     0,   525,     0,     0,
       0,   526,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   527,     0,     0,   528,   529,     0,   313,   314,   315,
       0,   317,   318,   319,   320,   321,   530,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   335,
     336,   337,     0,     0,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   531,   532,   533,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   534,   535,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     536,   537,   538,   539,   540,     0,   541,     0,   542,   543,
     544,   545,    52,     0,     0,   546,   547,   548,   549,   550,
     551,   552,   553,    65,   554,   555,   556,     0,     0,     0,
       0,     0,     0,   557,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   558,   559,   560,     0,    15,     0,     0,   561,
     562,     0,     0,     0,     0,   512,   513,   563,     0,   564,
       0,   565,   566,   804,     0,   514,   515,   516,   517,   518,
       0,     0,     0,     0,     0,   519,     0,   520,     0,     0,
       0,   521,     0,     0,     0,     0,     0,     0,     0,   522,
       0,     0,     0,     0,     0,   523,     0,     0,   524,     0,
       0,   525,     0,     0,     0,   526,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   527,     0,     0,   528,   529,
       0,   313,   314,   315,     0,   317,   318,   319,   320,   321,
     530,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,     0,   335,   336,   337,     0,     0,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   531,   532,   533,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   534,   535,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   536,   537,   538,   539,   540,     0,
     541,     0,   542,   543,   544,   545,    52,     0,     0,   546,
     547,   548,   549,   550,   551,   552,   553,    65,   554,   555,
     556,     0,     0,     0,     0,     0,     0,   557,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   558,   559,   560,     0,
      15,     0,     0,   561,   562,     0,     0,   512,   513,     0,
       0,   563,     0,   564,     0,   565,   566,   514,   515,   516,
     517,   518,     0,     0,     0,     0,     0,   519,     0,   520,
       0,     0,     0,   521,     0,     0,     0,     0,     0,     0,
       0,   522,     0,     0,     0,     0,     0,   523,     0,     0,
     524,     0,     0,   525,     0,     0,     0,   526,     0,     0,
       0,     0,     0,   908,     0,     0,     0,   527,     0,     0,
     528,   529,     0,   313,   314,   315,     0,   317,   318,   319,
     320,   321,   530,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   335,   336,   337,     0,     0,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   531,   532,   533,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   534,   535,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   536,   537,   538,   539,
     540,     0,   541,     0,   542,   543,   544,   545,    52,     0,
       0,   546,   547,   548,   549,   550,   551,   552,   553,    65,
     554,   555,   556,     0,     0,     0,     0,     0,     0,   557,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   558,   559,
     560,     0,    15,     0,     0,   561,   562,     0,     0,   512,
     513,     0,     0,   563,     0,   564,     0,   565,   566,   514,
     515,   516,   517,   518,     0,     0,     0,     0,     0,   519,
       0,   520,     0,     0,     0,   521,     0,     0,     0,     0,
       0,     0,     0,   522,     0,     0,     0,     0,     0,   523,
       0,     0,   524,     0,     0,   525,     0,     0,     0,   526,
       0,     0,   915,     0,     0,     0,     0,     0,     0,   527,
       0,     0,   528,   529,     0,   313,   314,   315,     0,   317,
     318,   319,   320,   321,   530,   323,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,     0,   335,   336,   337,
       0,     0,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   531,
     532,   533,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   534,   535,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   536,   537,
     538,   539,   540,     0,   541,     0,   542,   543,   544,   545,
      52,     0,     0,   546,   547,   548,   549,   550,   551,   552,
     553,    65,   554,   555,   556,     0,     0,     0,     0,     0,
       0,   557,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     558,   559,   560,     0,    15,     0,     0,   561,   562,     0,
       0,   512,   513,     0,     0,   563,     0,   564,     0,   565,
     566,   514,   515,   516,   517,   518,     0,     0,     0,     0,
       0,   519,     0,   520,     0,     0,     0,   521,     0,     0,
       0,     0,     0,     0,     0,   522,     0,     0,     0,     0,
       0,   523,     0,     0,   524,     0,     0,   525,     0,     0,
       0,   526,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   527,     0,     0,   528,   529,     0,   313,   314,   315,
       0,   317,   318,   319,   320,   321,   530,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   335,
     336,   337,     0,     0,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   531,   532,   533,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   534,   535,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     536,   537,   538,   539,   540,     0,   541,     0,   542,   543,
     544,   545,    52,     0,     0,   546,   547,   548,   549,   550,
     551,   552,   553,    65,   554,   555,   556,     0,     0,     0,
       0,     0,     0,   557,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     788,     0,   558,   559,   560,     0,    15,     0,     0,   561,
     562,     0,     0,   512,   513,     0,     0,   563,     0,   564,
       0,   565,   566,   514,   515,   516,   517,   518,     0,     0,
    1086,     0,     0,   519,     0,   520,     0,     0,     0,   521,
       0,     0,     0,     0,     0,     0,     0,   522,     0,     0,
       0,     0,     0,   523,     0,     0,   524,     0,     0,   525,
       0,     0,     0,   526,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   527,     0,     0,   528,   529,     0,   313,
     314,   315,     0,   317,   318,   319,   320,   321,   530,   323,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
       0,   335,   336,   337,     0,     0,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   531,   532,   533,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   534,
     535,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   536,   537,   538,   539,   540,     0,   541,     0,
     542,   543,   544,   545,    52,     0,     0,   546,   547,   548,
     549,   550,   551,   552,   553,    65,   554,   555,   556,     0,
       0,     0,     0,     0,     0,   557,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   558,   559,   560,     0,    15,     0,
       0,   561,   562,     0,     0,   512,   513,     0,     0,   563,
       0,   564,     0,   565,   566,   514,   515,   516,   517,   518,
       0,     0,     0,     0,     0,   519,     0,   520,     0,     0,
       0,   521,     0,     0,     0,     0,     0,     0,     0,   522,
       0,     0,     0,     0,     0,   523,     0,     0,   524,     0,
       0,   525,     0,     0,     0,   526,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   527,     0,     0,   528,   529,
       0,   313,   314,   315,     0,   317,   318,   319,   320,   321,
     530,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,     0,   335,   336,   337,     0,     0,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   531,   532,   533,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   534,   535,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   536,   537,   538,   539,   540,     0,
     541,     0,   542,   543,   544,   545,    52,     0,     0,   546,
     547,   548,   549,   550,   551,   552,   553,    65,   554,   555,
     556,     0,     0,     0,     0,     0,     0,   557,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   558,   559,   560,     0,
      15,     0,     0,   561,   562,     0,     0,   512,   513,     0,
       0,   563,     0,   564,  1107,   565,   566,   514,   515,   516,
     517,   518,     0,     0,     0,     0,     0,   519,     0,   520,
       0,     0,     0,   521,     0,     0,     0,     0,     0,     0,
       0,   522,     0,     0,     0,     0,     0,   523,     0,     0,
     524,     0,     0,   525,     0,     0,     0,   526,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   527,     0,     0,
     528,   529,     0,   313,   314,   315,     0,   317,   318,   319,
     320,   321,   530,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   335,   336,   337,     0,     0,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   531,   532,   533,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   534,   535,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   536,   537,   538,   539,
     540,     0,   541,     0,   542,   543,   544,   545,    52,     0,
       0,   546,   547,   548,   549,   550,   551,   552,   553,    65,
     554,   555,   556,     0,     0,     0,     0,     0,     0,   557,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1248,     0,   558,   559,
     560,     0,    15,     0,     0,   561,   562,     0,     0,   512,
     513,     0,     0,   563,     0,   564,     0,   565,   566,   514,
     515,   516,   517,   518,     0,     0,     0,     0,     0,   519,
       0,   520,     0,     0,     0,   521,     0,     0,     0,     0,
       0,     0,     0,   522,     0,     0,     0,     0,     0,   523,
       0,     0,   524,     0,     0,   525,     0,     0,     0,   526,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   527,
       0,     0,   528,   529,     0,   313,   314,   315,     0,   317,
     318,   319,   320,   321,   530,   323,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,     0,   335,   336,   337,
       0,     0,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   531,
     532,   533,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   534,   535,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   536,   537,
     538,   539,   540,     0,   541,     0,   542,   543,   544,   545,
      52,     0,     0,   546,   547,   548,   549,   550,   551,   552,
     553,    65,   554,   555,   556,     0,     0,     0,     0,     0,
       0,   557,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     558,   559,   560,     0,    15,     0,     0,   561,   562,     0,
       0,   512,   513,     0,     0,   563,     0,   564,  1287,   565,
     566,   514,   515,   516,   517,   518,     0,     0,     0,     0,
       0,   519,     0,   520,     0,     0,     0,   521,     0,     0,
       0,     0,     0,     0,     0,   522,     0,     0,     0,     0,
       0,   523,     0,     0,   524,     0,     0,   525,     0,     0,
       0,   526,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   527,     0,     0,   528,   529,     0,   313,   314,   315,
       0,   317,   318,   319,   320,   321,   530,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   335,
     336,   337,     0,     0,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   531,   532,   533,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   534,   535,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     536,   537,   538,   539,   540,     0,   541,     0,   542,   543,
     544,   545,    52,     0,     0,   546,   547,   548,   549,   550,
     551,   552,   553,    65,   554,   555,   556,     0,     0,     0,
       0,     0,     0,   557,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   558,   559,   560,     0,    15,     0,     0,   561,
     562,     0,     0,   512,   513,     0,     0,   563,     0,   564,
    1302,   565,   566,   514,   515,   516,   517,   518,     0,     0,
       0,     0,     0,   519,     0,   520,     0,     0,     0,   521,
       0,     0,     0,     0,     0,     0,     0,   522,     0,     0,
       0,     0,     0,   523,     0,     0,   524,     0,     0,   525,
       0,     0,     0,   526,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   527,     0,     0,   528,   529,     0,   313,
     314,   315,     0,   317,   318,   319,   320,   321,   530,   323,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
       0,   335,   336,   337,     0,     0,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   531,   532,   533,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   534,
     535,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   536,   537,   538,   539,   540,     0,   541,     0,
     542,   543,   544,   545,    52,     0,     0,   546,   547,   548,
     549,   550,   551,   552,   553,    65,   554,   555,   556,     0,
       0,     0,     0,     0,     0,   557,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   558,   559,   560,     0,    15,     0,
       0,   561,   562,     0,     0,   512,   513,     0,     0,   563,
       0,   564,  1530,   565,   566,   514,   515,   516,   517,   518,
       0,     0,     0,     0,     0,   519,     0,   520,     0,     0,
       0,   521,     0,     0,     0,     0,     0,     0,     0,   522,
       0,     0,     0,     0,     0,   523,     0,     0,   524,     0,
       0,   525,     0,     0,     0,   526,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   527,     0,     0,   528,   529,
       0,   313,   314,   315,     0,   317,   318,   319,   320,   321,
     530,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,     0,   335,   336,   337,     0,     0,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   531,   532,   533,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   534,   535,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   536,   537,   538,   539,   540,     0,
     541,     0,   542,   543,   544,   545,    52,     0,     0,   546,
     547,   548,   549,   550,   551,   552,   553,    65,   554,   555,
     556,     0,     0,     0,     0,     0,     0,   557,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   558,   559,   560,     0,
      15,     0,     0,   561,   562,     0,     0,   512,   513,     0,
       0,  1539,     0,   564,  1540,   565,   566,   514,   515,   516,
     517,   518,     0,     0,     0,     0,     0,   519,     0,   520,
       0,     0,     0,   521,     0,     0,     0,     0,     0,     0,
       0,   522,     0,     0,     0,     0,     0,   523,     0,     0,
     524,     0,     0,   525,     0,     0,     0,   526,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   527,     0,     0,
     528,   529,     0,   313,   314,   315,     0,   317,   318,   319,
     320,   321,   530,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   335,   336,   337,     0,     0,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   531,   532,   533,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   534,   535,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   536,   537,   538,   539,
     540,     0,   541,     0,   542,   543,   544,   545,    52,     0,
       0,   546,   547,   548,   549,   550,   551,   552,   553,    65,
     554,   555,   556,     0,     0,     0,     0,     0,     0,   557,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   558,   559,
     560,     0,    15,     0,     0,   561,   562,     0,     0,   512,
     513,     0,     0,   563,     0,   564,  1545,   565,   566,   514,
     515,   516,   517,   518,     0,     0,     0,     0,     0,   519,
       0,   520,     0,     0,     0,   521,     0,     0,     0,     0,
       0,     0,     0,   522,     0,     0,     0,     0,     0,   523,
       0,     0,   524,     0,     0,   525,     0,     0,     0,   526,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   527,
       0,     0,   528,   529,     0,   313,   314,   315,     0,   317,
     318,   319,   320,   321,   530,   323,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,     0,   335,   336,   337,
       0,     0,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   531,
     532,   533,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   534,   535,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   536,   537,
     538,   539,   540,     0,   541,     0,   542,   543,   544,   545,
      52,     0,     0,   546,   547,   548,   549,   550,   551,   552,
     553,    65,   554,   555,   556,     0,     0,     0,     0,     0,
       0,   557,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     558,   559,   560,     0,    15,     0,     0,   561,   562,     0,
       0,   512,   513,     0,     0,   563,     0,   564,  1603,   565,
     566,   514,   515,   516,   517,   518,     0,     0,     0,     0,
       0,   519,     0,   520,     0,     0,     0,   521,     0,     0,
       0,     0,     0,     0,     0,   522,     0,     0,     0,     0,
       0,   523,     0,     0,   524,     0,     0,   525,     0,     0,
       0,   526,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   527,     0,     0,   528,   529,     0,   313,   314,   315,
       0,   317,   318,   319,   320,   321,   530,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   335,
     336,   337,     0,     0,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   531,   532,   533,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   534,   535,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     536,   537,   538,   539,   540,     0,   541,     0,   542,   543,
     544,   545,    52,     0,     0,   546,   547,   548,   549,   550,
     551,   552,   553,    65,   554,   555,   556,     0,     0,     0,
       0,     0,     0,   557,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   558,   559,   560,     0,    15,     0,     0,   561,
     562,     0,     0,   512,   513,     0,     0,   563,     0,   564,
    1695,   565,   566,   514,   515,   516,   517,   518,     0,     0,
       0,     0,     0,   519,     0,   520,     0,     0,     0,   521,
       0,     0,     0,     0,     0,     0,     0,   522,     0,     0,
       0,     0,     0,   523,     0,     0,   524,     0,     0,   525,
       0,     0,     0,   526,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   527,     0,     0,   528,   529,     0,   313,
     314,   315,     0,   317,   318,   319,   320,   321,   530,   323,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
       0,   335,   336,   337,     0,     0,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   531,   532,   533,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   534,
     535,     0,     0,     0,     0,     0,     0,     0,  1721,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   536,   537,   538,   539,   540,     0,   541,     0,
     542,   543,   544,   545,    52,     0,     0,   546,   547,   548,
     549,   550,   551,   552,   553,    65,   554,   555,   556,     0,
       0,     0,     0,     0,     0,   557,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   558,   559,   560,     0,    15,     0,
       0,   561,   562,     0,     0,   512,   513,     0,     0,   563,
       0,   564,     0,   565,   566,   514,   515,   516,   517,   518,
       0,     0,     0,     0,     0,   519,     0,   520,     0,     0,
       0,   521,     0,     0,     0,     0,     0,     0,     0,   522,
       0,     0,     0,     0,     0,   523,     0,     0,   524,     0,
       0,   525,     0,     0,     0,   526,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   527,     0,     0,   528,   529,
       0,   313,   314,   315,     0,   317,   318,   319,   320,   321,
     530,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,     0,   335,   336,   337,     0,     0,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   531,   532,   533,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   534,   535,     0,     0,     0,     0,     0,     0,     0,
    1789,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   536,   537,   538,   539,   540,     0,
     541,     0,   542,   543,   544,   545,    52,     0,     0,   546,
     547,   548,   549,   550,   551,   552,   553,    65,   554,   555,
     556,     0,     0,     0,     0,     0,     0,   557,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   558,   559,   560,     0,
      15,     0,     0,   561,   562,     0,     0,   512,   513,     0,
       0,   563,     0,   564,     0,   565,   566,   514,   515,   516,
     517,   518,     0,     0,     0,     0,     0,   519,     0,   520,
       0,     0,     0,   521,     0,     0,     0,     0,     0,     0,
       0,   522,     0,     0,     0,     0,     0,   523,     0,     0,
     524,     0,     0,   525,     0,     0,     0,   526,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   527,     0,     0,
     528,   529,     0,   313,   314,   315,     0,   317,   318,   319,
     320,   321,   530,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   335,   336,   337,     0,     0,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   531,   532,   533,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   534,   535,     0,     0,     0,     0,     0,
       0,     0,  1790,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   536,   537,   538,   539,
     540,     0,   541,     0,   542,   543,   544,   545,    52,     0,
       0,   546,   547,   548,   549,   550,   551,   552,   553,    65,
     554,   555,   556,     0,     0,     0,     0,     0,     0,   557,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   558,   559,
     560,     0,    15,     0,     0,   561,   562,     0,     0,   512,
     513,     0,     0,   563,     0,   564,     0,   565,   566,   514,
     515,   516,   517,   518,     0,     0,     0,     0,     0,   519,
       0,   520,     0,     0,     0,   521,     0,     0,     0,     0,
       0,     0,     0,   522,     0,     0,     0,     0,     0,   523,
       0,     0,   524,     0,     0,   525,     0,     0,     0,   526,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   527,
       0,     0,   528,   529,     0,   313,   314,   315,     0,   317,
     318,   319,   320,   321,   530,   323,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,     0,   335,   336,   337,
       0,     0,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   531,
     532,   533,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   534,   535,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   536,   537,
     538,   539,   540,     0,   541,     0,   542,   543,   544,   545,
      52,     0,     0,   546,   547,   548,   549,   550,   551,   552,
     553,    65,   554,   555,   556,     0,     0,     0,     0,     0,
       0,   557,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     558,   559,   560,     0,    15,     0,     0,   561,   562,     0,
       0,   512,   513,     0,     0,   563,     0,   564,     0,   565,
     566,   514,   515,   516,   517,   518,     0,     0,     0,     0,
       0,   519,     0,   520,     0,     0,     0,   521,     0,     0,
       0,     0,     0,     0,     0,   522,     0,     0,     0,     0,
       0,   523,     0,     0,   524,     0,     0,   525,     0,     0,
       0,   526,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   527,     0,     0,   528,   529,     0,   313,   314,   315,
       0,   317,   318,   319,   320,   321,   530,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,     0,   335,
     336,   337,     0,     0,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   531,   532,   533,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   534,   535,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     536,   537,   538,   539,   540,     0,   541,     0,   542,   543,
     544,   545,    52,     0,     0,   546,   547,   548,   549,   550,
     551,   552,   553,    65,   554,   555,   556,     0,     0,   780,
       0,     0,     0,   557,     0,   307,     0,     0,     0,     0,
       0,   308,     0,     0,     0,     0,     0,   309,     0,     0,
       0,     0,   558,   559,   560,     0,    15,   310,     0,   561,
     562,     0,     0,     0,     0,   311,     0,  1516,     0,   564,
       0,   565,   566,     0,     0,     0,     0,     0,     0,     0,
     312,     0,     0,     0,     0,     0,     0,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,     0,  1112,  1113,  1114,  1115,  1116,  1117,
    1118,  1119,     0,     0,     0,     0,     0,  1120,  1121,     0,
       0,     0,     0,  1122,     0,     0,     0,     0,  -438,     0,
       0,     0,     0,   956,     0,     0,  1123,  1124,     0,     0,
       0,     0,    64,  1125,  1126,  1127,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   369,     0,     0,     0,     0,
       0,     0,     0,     0,   782,     0,     0,     0,     0,     0,
     307,     0,     0,    65,     0,     0,   308,     0,     0,     0,
       0,     0,   309,     0,     0,     0,     0,     0,     0,     0,
       0,  1128,   310,     0,     0,     0,     0,     0,     0,     0,
     311,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   312,     0,     0,   565,   922,
       0,   370,   313,   314,   315,   316,   317,   318,   319,   320,
     321,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,     0,  1112,
    1113,  1114,  1115,  1116,  1117,  1118,  1119,     0,     0,     0,
       0,     0,  1120,  1121,     0,     0,     0,     0,  1122,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   956,     0,
       0,  1123,  1124,     0,     0,     0,     0,    64,  1125,  1126,
    1127,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     369,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   307,     0,     0,    65,     0,
       0,   308,     0,     0,     0,     0,     0,   309,     0,     0,
       0,     0,     0,     0,     0,     0,  1128,   310,     0,     0,
       0,     0,     0,     0,     0,   311,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     312,     0,     0,   565,   922,     0,   370,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,     0,  1112,  1113,  1114,  1115,  1116,  1117,
    1118,  1119,     0,     0,     0,     0,     0,  1120,  1121,     0,
       0,     0,     0,  1122,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   956,     0,     0,  1123,  1124,     0,     0,
       0,     0,    64,  1125,  1126,  1127,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   369,     0,     0,     0,     0,
       0,    13,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   307,    65,     0,     0,     0,     0,   308,     0,
       0,     0,    14,     0,   309,     0,     0,     0,     0,     0,
       0,  1128,     0,     0,   310,     0,     0,     0,     0,     0,
       0,     0,   311,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   312,   565,   922,
       0,   370,     0,   629,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   369,     0,     0,     0,     0,     0,     0,     0,
       0,    13,     0,     0,     0,     0,     0,   307,     0,     0,
     633,     0,     0,   308,     0,     0,     0,     0,     0,   309,
       0,     0,    14,     0,     0,     0,     0,     0,     0,   310,
     634,     0,     0,     0,     0,     0,     0,   311,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   312,     0,     0,     0,     0,     0,   370,   313,
     314,   315,   316,   317,   318,   319,   320,   321,   322,   323,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   369,     0,     0,
       0,     0,   811,   812,     0,     0,     0,     0,     0,     0,
       0,     0,   307,     0,     0,    65,     0,     0,   308,     0,
       0,     0,     0,     0,   309,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   310,     0,     0,     0,     0,     0,
       0,     0,   311,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   312,     0,     0,
       0,     0,     0,   370,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     813,   814,   815,   816,   817,   811,   812,   818,   819,   820,
     821,     0,   822,   823,   824,   825,     0,     0,     0,     0,
     826,     0,   827,   828,     0,     0,     0,     0,   829,   830,
     831,     0,     0,     0,   832,     0,     0,     0,     0,    64,
     811,   812,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   369,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     633,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     833,     0,   834,   835,   836,   837,   838,   839,   840,   841,
     842,   843,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   844,   845,     0,     0,  1261,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   370,     0,
       0,     0,     0,   813,   814,   815,   816,   817,     0,     0,
     818,   819,   820,   821,     0,   822,   823,   824,   825,     0,
       0,     0,     0,   826,     0,   827,   828,   811,   812,     0,
       0,   829,   830,   831,     0,     0,     0,   832,   813,   814,
     815,   816,   817,     0,     0,   818,   819,   820,   821,     0,
     822,   823,   824,   825,     0,     0,     0,     0,   826,     0,
     827,   828,   811,   812,     0,     0,   829,   830,   831,     0,
       0,     0,   832,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   833,     0,   834,   835,   836,   837,   838,
     839,   840,   841,   842,   843,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   844,   845,     0,     0,  1264,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   833,     0,
     834,   835,   836,   837,   838,   839,   840,   841,   842,   843,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   844,
     845,     0,     0,  1266,     0,   813,   814,   815,   816,   817,
       0,     0,   818,   819,   820,   821,     0,   822,   823,   824,
     825,     0,     0,     0,     0,   826,     0,   827,   828,   811,
     812,     0,     0,   829,   830,   831,     0,     0,     0,   832,
     813,   814,   815,   816,   817,     0,     0,   818,   819,   820,
     821,     0,   822,   823,   824,   825,     0,     0,     0,     0,
     826,     0,   827,   828,   811,   812,     0,     0,   829,   830,
     831,     0,     0,     0,   832,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   833,     0,   834,   835,   836,
     837,   838,   839,   840,   841,   842,   843,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   844,   845,     0,     0,
    1274,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     833,     0,   834,   835,   836,   837,   838,   839,   840,   841,
     842,   843,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   844,   845,     0,     0,  1275,     0,   813,   814,   815,
     816,   817,     0,     0,   818,   819,   820,   821,     0,   822,
     823,   824,   825,     0,     0,     0,     0,   826,     0,   827,
     828,   811,   812,     0,     0,   829,   830,   831,     0,     0,
       0,   832,   813,   814,   815,   816,   817,     0,     0,   818,
     819,   820,   821,     0,   822,   823,   824,   825,     0,     0,
       0,     0,   826,     0,   827,   828,   811,   812,     0,     0,
     829,   830,   831,     0,     0,     0,   832,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   833,     0,   834,
     835,   836,   837,   838,   839,   840,   841,   842,   843,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   844,   845,
       0,     0,  1276,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   833,     0,   834,   835,   836,   837,   838,   839,
     840,   841,   842,   843,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   844,   845,     0,     0,  1277,     0,   813,
     814,   815,   816,   817,     0,     0,   818,   819,   820,   821,
       0,   822,   823,   824,   825,     0,     0,     0,     0,   826,
       0,   827,   828,   811,   812,     0,     0,   829,   830,   831,
       0,     0,     0,   832,   813,   814,   815,   816,   817,     0,
       0,   818,   819,   820,   821,     0,   822,   823,   824,   825,
       0,     0,     0,     0,   826,     0,   827,   828,   811,   812,
       0,     0,   829,   830,   831,     0,     0,     0,   832,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   833,
       0,   834,   835,   836,   837,   838,   839,   840,   841,   842,
     843,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     844,   845,     0,     0,  1278,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   833,     0,   834,   835,   836,   837,
     838,   839,   840,   841,   842,   843,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   844,   845,     0,     0,  1279,
       0,   813,   814,   815,   816,   817,     0,     0,   818,   819,
     820,   821,     0,   822,   823,   824,   825,     0,     0,     0,
       0,   826,     0,   827,   828,   811,   812,     0,     0,   829,
     830,   831,     0,     0,     0,   832,   813,   814,   815,   816,
     817,     0,     0,   818,   819,   820,   821,     0,   822,   823,
     824,   825,     0,     0,     0,     0,   826,     0,   827,   828,
     811,   812,     0,     0,   829,   830,   831,     0,     0,     0,
     832,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   833,     0,   834,   835,   836,   837,   838,   839,   840,
     841,   842,   843,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   844,   845,     0,     0,  1555,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   833,     0,   834,   835,
     836,   837,   838,   839,   840,   841,   842,   843,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   844,   845,     0,
       0,  1606,     0,   813,   814,   815,   816,   817,     0,     0,
     818,   819,   820,   821,     0,   822,   823,   824,   825,     0,
       0,     0,     0,   826,     0,   827,   828,   811,   812,     0,
       0,   829,   830,   831,     0,     0,     0,   832,   813,   814,
     815,   816,   817,     0,     0,   818,   819,   820,   821,     0,
     822,   823,   824,   825,     0,     0,     0,     0,   826,     0,
     827,   828,   811,   812,     0,     0,   829,   830,   831,     0,
       0,     0,   832,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   833,     0,   834,   835,   836,   837,   838,
     839,   840,   841,   842,   843,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   844,   845,     0,     0,  1625,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   833,     0,
     834,   835,   836,   837,   838,   839,   840,   841,   842,   843,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   844,
     845,     0,     0,  1626,     0,   813,   814,   815,   816,   817,
       0,     0,   818,   819,   820,   821,     0,   822,   823,   824,
     825,     0,     0,     0,     0,   826,     0,   827,   828,   811,
     812,     0,     0,   829,   830,   831,     0,     0,     0,   832,
     813,   814,   815,   816,   817,     0,     0,   818,   819,   820,
     821,     0,   822,   823,   824,   825,     0,     0,     0,     0,
     826,     0,   827,   828,   811,   812,     0,     0,   829,   830,
     831,     0,     0,     0,   832,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   833,     0,   834,   835,   836,
     837,   838,   839,   840,   841,   842,   843,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   844,   845,     0,     0,
    1640,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     833,     0,   834,   835,   836,   837,   838,   839,   840,   841,
     842,   843,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   844,   845,     0,     0,  1642,     0,   813,   814,   815,
     816,   817,     0,     0,   818,   819,   820,   821,     0,   822,
     823,   824,   825,     0,     0,     0,     0,   826,     0,   827,
     828,   811,   812,     0,     0,   829,   830,   831,     0,     0,
       0,   832,   813,   814,   815,   816,   817,     0,     0,   818,
     819,   820,   821,     0,   822,   823,   824,   825,     0,     0,
       0,     0,   826,     0,   827,   828,   811,   812,     0,     0,
     829,   830,   831,     0,     0,     0,   832,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   833,     0,   834,
     835,   836,   837,   838,   839,   840,   841,   842,   843,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   844,   845,
       0,     0,  1644,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   833,     0,   834,   835,   836,   837,   838,   839,
     840,   841,   842,   843,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   844,   845,     0,     0,  1648,     0,   813,
     814,   815,   816,   817,     0,     0,   818,   819,   820,   821,
       0,   822,   823,   824,   825,     0,     0,     0,     0,   826,
       0,   827,   828,   811,   812,     0,     0,   829,   830,   831,
       0,     0,     0,   832,   813,   814,   815,   816,   817,     0,
       0,   818,   819,   820,   821,     0,   822,   823,   824,   825,
       0,     0,     0,     0,   826,     0,   827,   828,   811,   812,
       0,     0,   829,   830,   831,     0,     0,     0,   832,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   833,
       0,   834,   835,   836,   837,   838,   839,   840,   841,   842,
     843,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     844,   845,     0,     0,  1726,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   833,     0,   834,   835,   836,   837,
     838,   839,   840,   841,   842,   843,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   844,   845,     0,     0,  1727,
       0,   813,   814,   815,   816,   817,     0,     0,   818,   819,
     820,   821,     0,   822,   823,   824,   825,     0,     0,     0,
       0,   826,     0,   827,   828,   811,   812,     0,     0,   829,
     830,   831,     0,     0,     0,   832,   813,   814,   815,   816,
     817,     0,     0,   818,   819,   820,   821,     0,   822,   823,
     824,   825,     0,     0,     0,     0,   826,     0,   827,   828,
     811,   812,     0,     0,   829,   830,   831,     0,     0,     0,
     832,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   833,     0,   834,   835,   836,   837,   838,   839,   840,
     841,   842,   843,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   844,   845,     0,     0,  1729,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   833,     0,   834,   835,
     836,   837,   838,   839,   840,   841,   842,   843,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   844,   845,     0,
       0,  1764,     0,   813,   814,   815,   816,   817,     0,     0,
     818,   819,   820,   821,     0,   822,   823,   824,   825,     0,
       0,     0,     0,   826,     0,   827,   828,   811,   812,     0,
       0,   829,   830,   831,     0,     0,     0,   832,   813,   814,
     815,   816,   817,     0,     0,   818,   819,   820,   821,     0,
     822,   823,   824,   825,     0,     0,     0,     0,   826,     0,
     827,   828,   811,   812,     0,     0,   829,   830,   831,     0,
       0,     0,   832,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   833,     0,   834,   835,   836,   837,   838,
     839,   840,   841,   842,   843,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   844,   845,     0,     0,  1769,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   833,     0,
     834,   835,   836,   837,   838,   839,   840,   841,   842,   843,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   844,
     845,   878,     0,     0,     0,   813,   814,   815,   816,   817,
       0,     0,   818,   819,   820,   821,     0,   822,   823,   824,
     825,     0,     0,     0,     0,   826,     0,   827,   828,   811,
     812,     0,     0,   829,   830,   831,     0,     0,     0,   832,
     813,   814,   815,   816,   817,     0,     0,   818,   819,   820,
     821,     0,   822,   823,   824,   825,     0,     0,     0,     0,
     826,     0,   827,   828,   811,   812,     0,     0,   829,   830,
     831,     0,     0,     0,   832,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   833,     0,   834,   835,   836,
     837,   838,   839,   840,   841,   842,   843,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   844,   845,  1152,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     833,     0,   834,   835,   836,   837,   838,   839,   840,   841,
     842,   843,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   844,   845,  1351,     0,     0,     0,   813,   814,   815,
     816,   817,     0,     0,   818,   819,   820,   821,     0,   822,
     823,   824,   825,     0,     0,     0,     0,   826,     0,   827,
     828,   811,   812,     0,     0,   829,   830,   831,     0,     0,
       0,   832,   813,   814,   815,   816,   817,     0,     0,   818,
     819,   820,   821,     0,   822,   823,   824,   825,     0,     0,
       0,     0,   826,     0,   827,   828,     0,     0,     0,     0,
     829,   830,   831,     0,     0,     0,   832,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   833,     0,   834,
     835,   836,   837,   838,   839,   840,   841,   842,   843,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   844,   845,
    1367,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   833,     0,   834,   835,   836,   837,   838,   839,
     840,   841,   842,   843,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   844,   845,  1537,     0,     0,     0,   813,
     814,   815,   816,   817,     0,     0,   818,   819,   820,   821,
       0,   822,   823,   824,   825,   385,   386,     0,     0,   826,
       0,   827,   828,     0,     0,     0,     0,   829,   830,   831,
       0,     0,   387,   832,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   811,   812,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   833,
       0,   834,   835,   836,   837,   838,   839,   840,   841,   842,
     843,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     844,   845,  1543,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   388,   389,   390,   391,   392,
     393,   394,   395,   396,   397,   398,   399,   400,   401,   402,
     403,   404,   405,     0,     0,   406,   407,   408,     0,     0,
       0,     0,     0,     0,   409,   410,   411,   412,   413,     0,
       0,   414,   415,   416,   417,   418,   419,   420,   811,   812,
       0,     0,     0,     0,     0,     0,   813,   814,   815,   816,
     817,     0,     0,   818,   819,   820,   821,     0,   822,   823,
     824,   825,     0,     0,     0,     0,   826,     0,   827,   828,
       0,     0,     0,     0,   829,   830,   831,   811,   812,     0,
     832,     0,     0,   421,     0,   422,   423,   424,   425,   426,
     427,   428,   429,   430,   431,    13,     0,   432,   433,     0,
       0,     0,     0,     0,   434,   435,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    14,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   833,     0,   834,   835,
     836,   837,   838,   839,   840,   841,   842,   843,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   844,   845,     0,
       0,     0,     0,     0,     0,     0,   813,   814,   815,   816,
     817,     0,     0,   818,   819,   820,   821,     0,   822,   823,
     824,   825,     0,   811,   812,     0,   826,     0,   827,   828,
       0,     0,     0,     0,   829,   830,   831,     0,     0,     0,
     832,     0,     0,     0,     0,   813,   814,   815,   816,   817,
       0,     0,   818,   819,   820,   821,     0,   822,   823,   824,
     825,   811,   812,     0,     0,   826,     0,   827,   828,     0,
       0,     0,     0,   829,   830,   831,     0,     0,     0,   832,
       0,     0,     0,     0,     0,     0,   833,  1356,   834,   835,
     836,   837,   838,   839,   840,   841,   842,   843,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   844,   845,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   833,     0,   834,   835,   836,
     837,   838,   839,   840,   841,   842,   843,     0,     0,     0,
       0,   813,   814,   815,   816,   817,   844,   845,   818,   819,
     820,   821,     0,   822,   823,   824,   825,   811,   812,     0,
       0,   826,     0,   827,   828,     0,     0,     0,     0,   829,
     830,   831,     0,     0,     0,  -881,     0,     0,     0,   813,
     814,   815,   816,   817,     0,     0,   818,   819,   820,   821,
       0,   822,   823,   824,   825,     0,     0,     0,     0,   826,
       0,   827,   828,     0,     0,     0,     0,   829,   830,   831,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   833,     0,   834,   835,   836,   837,   838,   839,   840,
     841,   842,   843,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   844,   845,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   833,
       0,   834,   835,   836,   837,   838,   839,   840,   841,   842,
     843,     0,     0,     0,     0,   813,   814,   815,   816,   817,
     844,   845,   818,   819,   820,   821,     0,   822,   823,   824,
     825,     0,     0,     0,     0,   826,     0,   827,   828,     0,
       0,     0,     0,   829,     0,   831,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1058,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   834,   835,   836,
     837,   838,   839,   840,   841,   842,   843,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   844,   845,   313,   314,
     315,  1062,   317,   318,   319,   320,   321,   530,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,     0,
     335,   336,   337,     0,     0,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,     0,   313,   314,   315,     0,   317,   318,   319,
     320,   321,   530,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   335,   336,   337,     0,     0,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,     0,  1059,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1060,     0,     0,     0,  1352,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1063,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1064,
     313,   314,   315,     0,   317,   318,   319,   320,   321,   530,
     323,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,     0,   335,   336,   337,     0,     0,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   313,   314,   315,     0,   317,   318,
     319,   320,   321,   530,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,     0,   335,   336,   337,     0,
       0,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,     0,     0,
    1353,     0,     0,     0,     0,     0,     0,     0,     0,   230,
       0,     0,     0,     0,     0,     0,  1354,     0,     0,     0,
       0,     0,     0,     0,     0,  1098,  1099,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   231,     0,   232,     0,   233,
     234,   235,   236,   237,  1100,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,     0,   249,   250,   251,
    1101,     0,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1102,  1103,   279,   280,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   281
};

static const yytype_int16 yycheck[] =
{
       1,   545,     7,   157,   213,   595,   618,   758,   283,   509,
     504,   504,   487,   719,   182,    15,    16,   802,   783,    20,
     455,   609,   807,   611,   781,   613,   783,   486,   449,   700,
     701,   895,   599,   865,  1291,   867,  1426,   869,  1013,   994,
      20,    87,   996,   973,  1019,    33,  1183,     5,    20,   484,
    1406,   224,   809,    20,    33,     8,    53,    22,   780,    20,
     782,     4,   784,    20,    15,    16,    20,    53,    20,   113,
      70,    71,    72,  1670,   166,   797,   576,   486,   487,   149,
      20,   183,   804,    20,   293,   175,  1573,    40,    15,    16,
     178,   876,    57,   183,    20,   183,   189,    46,    21,    22,
      20,   102,   166,    19,    20,   197,   599,   197,   543,   544,
      12,     7,   149,  1716,   114,   115,   116,   117,   685,  1049,
     149,    23,    24,   166,    57,   150,   131,   181,   188,   297,
      63,   698,     0,   197,   220,   160,  1273,   223,   150,     7,
     167,   168,   169,   197,   246,    12,   197,   181,   160,   209,
     220,   243,  1749,   243,    50,   225,    23,    24,   246,   152,
     153,   564,    30,   197,    32,   150,    34,   125,   126,   215,
     214,    62,    40,   243,   179,   160,  1779,   220,   900,  1666,
     129,   166,    50,   220,   181,   197,   246,   186,    56,   217,
     244,   220,   685,   218,   245,    57,   225,   982,   203,   204,
     197,    63,   188,   245,   246,   698,   218,   992,   207,   243,
     995,   223,    80,    34,   243,   216,   973,   244,   141,   142,
     504,   220,   227,   209,   224,  1482,   149,   636,   151,   152,
     153,   154,   641,   218,   187,   223,   159,   217,   189,   167,
     183,   169,    63,   194,   223,   217,   197,   240,   241,   200,
     217,    21,    22,   241,   300,   208,   217,   125,   126,   247,
     217,  1617,   189,   217,   229,   217,   224,   194,  1053,   217,
     197,  1661,   943,   200,   218,   240,   485,   217,   283,   220,
     217,   197,   685,   850,  1421,   243,   188,   292,   241,   443,
     295,   217,  1049,  1050,  1431,   504,   505,   217,   149,  1254,
     509,  1255,   188,   226,   227,   228,   229,   230,   197,   130,
    1032,  1033,  1287,  1035,  1244,   599,   243,   240,   241,   246,
     188,   188,   757,   149,  1705,   187,  1707,   175,   501,   502,
     186,   217,  1713,   217,  1005,   902,   771,  1718,   159,   150,
      55,   209,  1732,   201,   202,   166,   208,    47,   150,   197,
    1607,   207,   919,   756,   243,   175,   510,   850,   160,   186,
     244,   149,   230,   766,   152,   153,   769,    67,   175,   220,
     370,   141,   142,   241,   225,    33,   197,   197,   224,   149,
     207,   151,   152,   153,   154,    57,   101,   243,   178,   159,
     197,    63,   243,   183,   220,   150,  1777,   243,   150,   225,
     229,   685,    60,    61,   188,   150,   809,   218,   160,   902,
     221,   217,   687,   234,   698,   160,   218,   243,   217,  1599,
     217,   696,   243,   243,   924,   209,   919,   600,   601,   602,
     639,   604,   605,   217,   643,   608,   243,   610,   244,   612,
     150,  1822,  1823,   242,   847,   445,  1168,   244,   857,   188,
     160,   188,   240,   241,   244,   455,   246,   217,   228,   229,
     230,   155,   217,   218,   873,   220,   218,   186,   223,  1057,
     240,   241,   209,   218,   166,   212,   166,  1858,   217,   218,
      12,  1025,   242,   918,   484,   175,   486,  1244,   207,   147,
     180,    23,    24,   151,   188,  1675,  1676,   220,   165,   902,
     223,   501,   502,  1047,   217,   242,  1481,   197,   218,    21,
      22,   188,  1344,  1693,  1694,   209,   516,   517,  1493,   186,
     188,   942,  1287,  1194,  1195,  1196,   245,   246,   217,   242,
    1287,  1751,   209,  1290,   224,   970,   971,  1302,   188,   217,
     207,   209,  1762,   543,   544,  1302,   981,    47,   225,   217,
    1414,   986,   987,   217,   989,   244,   991,   225,   993,   209,
     173,   219,   197,    57,   242,   223,   850,   225,   226,    63,
     973,   221,   217,   741,   983,   229,  1756,  1757,     5,     6,
     244,     8,   736,   241,   197,   198,   199,  1807,  1808,   247,
      90,    91,   186,    93,   595,  1317,   805,   242,   217,   188,
     600,   601,   602,   243,   604,   605,   217,  1010,   608,    36,
     610,   217,   612,   207,   614,  1018,   229,   230,   902,   217,
     209,  1485,   217,   217,   899,   244,   173,   240,   217,   141,
     142,   217,   221,   244,   217,   919,   911,   149,   244,   151,
     152,   153,   154,   130,   242,    21,    22,   159,   130,   244,
     197,   198,   199,   928,   212,   213,   931,   130,   244,   242,
     188,   945,   946,   947,   948,   949,   950,   951,   952,   953,
     954,   955,  1147,   957,   958,   959,   960,   961,   962,   224,
    1512,   209,   687,   230,   242,    10,  1145,   188,  1188,   217,
     217,   696,   693,   240,   699,   700,   701,   225,   188,    70,
      71,    72,   875,   166,   705,   706,    96,    97,   209,   189,
     209,   884,   175,  1464,   887,   217,   217,   244,   217,   209,
     721,   722,   723,   724,   225,   726,   188,   217,   240,   241,
     731,  1563,   188,   188,   197,   225,  1324,   188,   738,  1148,
     242,   217,  1606,   114,   115,   116,   117,   209,   217,   749,
      57,   188,   217,   209,   209,   217,    63,   757,   209,  1516,
     197,   217,   217,   225,   188,   141,   142,   218,   244,   225,
     225,   771,   209,   149,   217,   244,   152,   153,   154,   244,
     217,   188,  1539,   159,   187,   209,   787,   131,   132,   133,
     134,   135,   136,   137,   138,   217,   217,   197,   224,   972,
     188,   244,   209,   803,   977,   208,   131,   132,   133,   134,
     135,   136,   137,   138,   175,   240,   160,   243,  1253,   144,
     145,   209,   244,   244,  1259,   150,   170,   171,   172,   217,
     187,    33,  1664,   197,  1422,   160,   197,   225,   163,   164,
     197,   188,   224,   197,   221,   170,   171,   172,   225,   224,
     224,   208,   853,   188,  1027,  1028,  1562,   181,    60,    61,
     188,   243,   209,  1272,   240,   241,  1027,  1028,   243,   243,
     217,   188,   220,   197,   209,   875,   224,  1280,   225,   201,
     208,   209,   217,   211,   884,   181,   187,   887,  1323,   197,
     225,   209,   209,   218,   899,   188,   197,  1172,  1301,   217,
     217,   197,  1311,  1312,  1307,  1769,   911,   208,   225,    79,
     455,   181,   197,  1316,   181,   208,   209,   210,   918,    57,
     245,   246,  1325,   928,    94,    63,   931,   197,   933,    99,
     197,   101,   181,     5,     6,  1686,  1430,  1430,   943,   484,
     197,   486,   246,   209,   209,   147,  1349,   209,   197,   151,
     197,   217,   217,    25,   209,   217,   501,   502,  1361,    31,
    1133,   209,   217,  1366,   167,   168,   169,    57,   969,   217,
     970,   971,   972,    63,    57,   221,  1761,   977,    57,   225,
      63,   981,  1570,   188,    75,   201,   986,   987,    79,   989,
    1163,   991,   234,   993,   994,   167,    68,    69,   543,   544,
    1005,  1758,    93,    94,   129,   197,  1763,    98,    99,   100,
     101,   218,  1013,  1416,    66,  1016,  1801,   219,  1019,  1020,
     220,   223,   197,    33,   226,   243,   217,  1027,  1028,   220,
     208,   221,   223,   211,   221,   225,   214,   221,   225,   241,
      35,   225,  1799,    35,  1453,   247,   201,   202,   203,   204,
      60,    61,  1461,   125,   126,   600,   601,   602,   221,   604,
     605,   220,   225,   608,   221,   610,   243,   612,   225,   614,
      33,  1615,  1684,  1476,    43,   221,   221,   221,  1668,   225,
     225,   225,   224,   224,   221,  1629,  1240,   243,   225,   245,
     246,  1526,   224,   243,   166,   224,    33,    60,    61,   221,
     224,  1504,  1505,   243,   224,  1508,   224,   224,  1865,   224,
    1111,   183,   184,   185,   186,   197,   198,   199,   224,   197,
     201,   202,   203,    60,    61,   197,   201,   202,   203,   243,
     674,   675,   676,  1133,   197,   207,  1137,   147,   197,   242,
     150,   151,    10,    11,    12,  1145,  1430,    22,  1423,   197,
     160,   197,   224,   242,  1744,  1745,  1559,   242,   197,   197,
     224,   241,   197,  1163,   209,    13,   243,  1172,   244,   241,
     224,   224,   243,   224,   224,   224,   243,   224,   188,    43,
     224,   243,   224,   224,   147,   243,   243,   150,   151,  1194,
    1195,  1196,   225,   224,    21,    22,   224,   160,   197,   209,
     243,   241,   220,  1793,   197,   189,   243,   243,   218,   219,
     147,   243,   757,   223,   151,   242,   226,   246,   243,   243,
    1623,  1430,   244,   224,    10,    37,   771,    66,     8,  1773,
     243,   241,  1667,  1234,  1407,   224,   224,   247,   224,   165,
     243,    13,   242,   217,   217,   197,  1419,   197,   155,  1250,
    1525,    21,    22,  1253,  1254,   218,   219,   225,   197,  1259,
     223,   217,   217,   226,   243,  1438,   811,   812,    43,   217,
      14,   243,   197,   218,   220,   189,   246,   197,   241,    67,
     197,   826,   219,  1558,   247,  1439,   223,  1690,   225,   226,
     244,  1566,   243,   165,   217,   217,   208,   244,   217,   844,
     243,   243,     1,   244,   241,   243,   224,   243,  1743,   224,
     247,   243,   139,   140,   141,   142,   143,   225,   197,   146,
     147,   148,   149,  1323,   151,   152,   153,   154,   225,   225,
     875,  1734,   159,   243,   161,   162,   197,   243,   243,   884,
     167,   243,   887,   243,   197,   946,   947,   948,   949,   950,
     951,   952,   953,   954,   955,  1528,   957,   958,   959,   960,
     961,   962,   244,   218,   218,   197,   244,   244,   244,   139,
     140,   141,   142,   918,   244,   244,    21,    22,   243,   149,
     243,   151,   152,   153,   154,  1660,  1789,   244,   244,   159,
     242,   161,   162,   242,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   242,   220,   197,  1407,   244,   243,
     197,   224,   197,   240,   241,   197,   197,   243,  1423,  1419,
     243,   243,   243,   197,    43,   970,   971,   972,    12,    33,
     244,   197,   977,   243,    33,   243,   981,   243,  1438,   242,
     205,   986,   987,   244,   989,   197,   991,   242,   993,   994,
     197,   197,   242,    70,   243,  1730,   226,   227,   228,   229,
     230,    60,    61,   197,   243,   225,   243,  1742,   244,   225,
     240,   241,  1645,   244,   243,   243,   243,   225,   201,   243,
    1481,   244,  1027,  1028,   209,   244,   243,   209,   243,   243,
     243,    53,  1493,   244,   139,   140,   141,   142,   143,   244,
     242,   146,   147,   148,   149,   244,   151,   152,   153,   154,
     208,   699,   700,   701,   159,  1669,   161,   162,   244,   209,
    1525,   242,   217,  1798,   244,   244,  1526,   244,  1528,   244,
     244,   719,   244,   244,   244,   242,   242,   242,   208,   243,
     790,  1086,   244,   244,    86,   242,   734,     1,   147,   242,
     147,    89,   151,  1558,  1662,   221,   738,  1187,  1662,  1560,
    1561,  1566,  1771,  1662,  1662,  1662,     1,  1568,  1190,  1660,
    1336,  1780,  1441,  1574,  1578,  1237,  1523,  1579,   452,   224,
     225,   226,   227,   228,   229,   230,    58,   461,  1133,  1444,
    1579,   442,   516,  1153,   322,   240,   241,   471,  1763,  1613,
    1145,  1299,   783,   516,    -1,    -1,    -1,   481,    -1,    -1,
      -1,    -1,  1821,    -1,    -1,    -1,    -1,    -1,  1163,    -1,
     219,    -1,    -1,    -1,   223,    -1,   225,   226,    -1,    -1,
      -1,    21,    22,   131,   132,   133,   134,   135,   136,   137,
     138,    -1,   241,  1797,    -1,  1645,    -1,    -1,   247,    -1,
      -1,    -1,   150,    -1,    -1,  1660,    -1,    -1,    -1,    -1,
     534,   535,   160,    -1,    -1,   853,    -1,  1667,    -1,    -1,
      -1,    -1,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1836,    33,   558,   559,   560,   561,   562,   563,
     564,    -1,    -1,    -1,    -1,    -1,    -1,  1698,    -1,    -1,
      -1,    -1,    -1,    -1,  1705,    -1,  1707,    -1,  1253,  1254,
      60,    61,  1713,    -1,  1259,    -1,  1716,  1718,   906,    -1,
      -1,    -1,    -1,    -1,    -1,  1730,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   921,    -1,    -1,    -1,  1742,    -1,    -1,
      -1,    -1,    -1,  1743,   618,    -1,    -1,  1748,    -1,   139,
     140,   141,   142,   143,   628,   943,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,
      -1,   161,   162,    -1,    -1,    -1,  1777,    -1,  1323,  1779,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1798,    -1,  1796,   670,   147,    -1,    -1,
      -1,   151,    33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   685,    -1,    -1,    -1,    -1,    -1,  1005,    -1,    -1,
      -1,  1822,  1823,    -1,    -1,    -1,    -1,    -1,    -1,    60,
      61,  1832,  1833,   223,   224,   225,   226,   227,   228,   229,
     230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     240,   241,    -1,    -1,    -1,    -1,    -1,  1858,    -1,  1860,
      -1,    -1,  1407,    -1,    -1,    -1,    -1,    -1,    -1,   219,
      -1,    -1,    -1,   223,  1419,   225,   226,    -1,    -1,    -1,
      -1,    -1,   756,    -1,    -1,   759,    -1,    -1,   762,    -1,
     764,   241,   766,  1438,    -1,   769,    -1,   247,    -1,    -1,
     774,   775,   776,   777,   778,   779,    -1,    -1,    -1,    -1,
     504,    -1,   506,    -1,    -1,    -1,   147,    -1,    -1,    -1,
     151,    -1,    -1,  1111,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   809,    -1,    -1,    -1,   813,
     814,    -1,    -1,   817,   818,   819,   820,    33,   822,    -1,
     824,   825,   826,   827,   828,   829,   830,   831,   832,   833,
     834,   835,   836,   837,   838,   839,   840,   841,   842,   843,
      -1,   845,    33,   847,    60,    61,    -1,    -1,    -1,    -1,
      -1,  1526,    -1,  1528,    -1,    -1,    -1,    -1,   219,    -1,
      -1,    -1,   223,  1181,   225,   226,    -1,    -1,    -1,    60,
      61,    -1,    -1,    -1,    -1,   599,  1194,  1195,  1196,    -1,
     241,  1199,    -1,  1201,    -1,  1203,   247,  1205,    -1,  1207,
      -1,  1209,    -1,  1211,    -1,  1213,    -1,  1215,   902,  1217,
      -1,  1219,    -1,    -1,  1222,    -1,  1224,    -1,  1226,    -1,
    1228,    -1,  1230,    -1,  1232,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   147,    -1,    10,    -1,   151,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   147,   681,    -1,    -1,
     151,   685,    -1,    -1,   688,    -1,   690,    -1,    -1,   973,
    1645,   695,   188,    -1,   698,    -1,    -1,    -1,    -1,   703,
      -1,    -1,    -1,   707,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1667,   209,    -1,    -1,    -1,    21,    22,    -1,
      -1,   217,    -1,   219,  1008,  1009,  1010,   223,    -1,   225,
     226,    -1,    -1,    -1,  1018,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   241,    -1,    -1,   219,    -1,
      -1,   247,   223,  1037,   225,   226,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1050,    -1,    -1,    -1,
     241,    -1,    -1,    -1,    -1,    -1,   247,   781,    -1,   783,
      -1,    33,   139,   140,   141,   142,   143,    -1,  1743,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,    33,    -1,
      -1,    -1,   159,    -1,   161,   162,    -1,    -1,    60,    61,
     167,   168,   169,    -1,    -1,  1099,   173,    -1,    -1,  1103,
      -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,
      -1,   188,    -1,    -1,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,   850,   151,   152,   153,
     154,    -1,   209,    -1,    -1,   159,    -1,   161,   162,    -1,
     217,    -1,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,  1158,    -1,    -1,    -1,   882,    -1,
      -1,    -1,    -1,   240,   241,    -1,    -1,  1171,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,    -1,    -1,   902,   151,
      -1,    -1,    -1,    -1,   908,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   147,    -1,    -1,   919,   151,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   240,   241,    -1,    -1,
      -1,   945,   946,   947,   948,   949,   950,   951,   952,   953,
     954,   955,   956,   957,   958,   959,   960,   961,   962,    -1,
      -1,    -1,    -1,    -1,  1562,    21,    22,   219,    -1,    -1,
     974,   223,   976,   225,   226,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   219,    -1,    -1,    -1,   223,   241,
     225,   226,    -1,    -1,    -1,   247,  1280,    -1,  1282,    -1,
      -1,  1285,    -1,    -1,    -1,    -1,   241,    -1,    21,    22,
      -1,    -1,   247,    -1,    -1,    -1,    -1,  1301,  1022,    -1,
      -1,    -1,    -1,  1307,    -1,    -1,  1030,    -1,    -1,    -1,
      -1,    -1,  1316,    33,    -1,    -1,    -1,  1321,  1322,    -1,
      -1,  1325,    -1,    -1,    -1,    -1,    -1,  1331,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      60,    61,  1346,    -1,  1348,  1349,  1350,    -1,    -1,    -1,
      -1,    -1,  1356,    -1,    -1,    -1,  1360,  1361,    -1,    -1,
      -1,    -1,  1366,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    -1,
      -1,    -1,    -1,   159,    -1,   161,   162,    -1,  1112,  1113,
    1114,  1115,  1116,  1117,  1118,  1119,  1120,  1121,  1122,  1123,
    1124,  1125,  1126,  1127,  1128,    -1,   139,   140,   141,   142,
     143,    -1,  1416,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    -1,    -1,    -1,    -1,   159,   147,   161,   162,
      -1,   151,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,
     173,  1165,    -1,    -1,    -1,    -1,   222,   223,   224,   225,
     226,   227,   228,   229,   230,    -1,   139,   140,   141,   142,
     143,    -1,  1186,   146,   240,   241,   149,    -1,   151,   152,
     153,   154,  1476,    -1,    -1,    -1,   159,    -1,   161,   162,
      -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,    -1,   219,
    1504,  1505,    -1,   223,  1508,   225,   226,   240,   241,    -1,
      -1,   244,  1516,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,   241,    -1,    -1,    -1,    -1,    -1,   247,    -1,    -1,
    1534,    -1,  1536,    -1,    -1,  1539,    -1,    -1,  1542,    33,
      -1,   224,   225,   226,   227,   228,   229,   230,    -1,    33,
      -1,    -1,    -1,    -1,    -1,  1559,    -1,   240,   241,  1283,
    1284,    -1,    -1,    -1,  1288,    -1,    60,    61,    -1,    -1,
      -1,    -1,  1296,    -1,    -1,  1299,    60,    61,    -1,    33,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1602,    -1,
      -1,  1605,    -1,    -1,    -1,  1609,    60,    61,  1612,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1622,  1623,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,    -1,    -1,
      -1,    -1,   159,   147,   161,   162,    -1,   151,    -1,    -1,
     167,   168,   169,   147,    -1,    -1,   173,   151,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
    1684,    -1,    -1,    -1,    -1,    -1,  1690,    -1,    -1,    -1,
      -1,    -1,    -1,   147,    -1,    -1,  1420,   151,    -1,    -1,
      -1,    -1,    -1,  1427,    -1,    -1,  1430,    -1,    -1,    -1,
      -1,  1715,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,    -1,   219,    -1,    -1,    -1,   223,
    1734,   225,   226,   240,   241,   219,    -1,   244,    -1,   223,
      -1,   225,   226,    -1,    -1,    -1,    -1,   241,    -1,    -1,
      33,    -1,    -1,   247,    -1,    -1,    -1,   241,    -1,    -1,
      -1,    -1,    -1,   247,    -1,   219,    -1,    -1,    -1,   223,
    1774,   225,   226,    -1,    -1,    -1,    -1,    60,    61,    -1,
      -1,    -1,    -1,    21,    22,  1789,    -1,   241,    -1,    -1,
      -1,    -1,    -1,   247,   139,   140,   141,   142,   143,    -1,
      -1,   146,   147,   148,   149,    -1,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    -1,    -1,
      -1,  1825,   167,   168,   169,    -1,    -1,    33,   173,    -1,
      -1,    -1,    -1,  1557,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,    -1,
    1584,    -1,  1586,    -1,   147,    -1,    -1,    -1,   151,    -1,
      -1,  1595,  1596,  1597,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,  1610,  1611,    -1,    -1,
      -1,    -1,  1616,    -1,    -1,   240,   241,    -1,    -1,   244,
      -1,   139,   140,   141,   142,   143,    -1,    -1,   146,   147,
     148,   149,  1636,   151,   152,   153,   154,    -1,    -1,    -1,
      -1,   159,    -1,   161,   162,    -1,    -1,    -1,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,   219,    -1,  1662,    -1,
     223,   147,   225,   226,    -1,   151,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   241,    -1,
      -1,    -1,    -1,    -1,   247,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1699,  1700,    -1,    -1,    -1,
      -1,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,   230,    -1,    -1,    -1,    -1,  1721,    -1,    -1,
      -1,    -1,   240,   241,    -1,    -1,   244,    -1,    -1,    -1,
      -1,  1735,    -1,   219,    -1,    -1,    -1,   223,    -1,   225,
     226,    -1,  1746,    -1,    -1,    -1,    -1,    -1,    -1,  1753,
      -1,  1755,    -1,    -1,    -1,   241,    -1,    -1,    -1,    -1,
      -1,   247,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1790,    -1,    -1,    -1,
    1794,  1795,     1,    -1,    -1,    -1,     5,     6,     7,    -1,
       9,    10,    11,    -1,    13,    -1,    15,    16,    17,    18,
      19,    -1,  1816,    -1,  1818,  1819,    25,    26,    27,    28,
      29,    -1,    31,    -1,    -1,    -1,    -1,  1831,    -1,    38,
      39,    40,    -1,    42,    -1,    44,    45,    -1,    -1,    48,
      -1,    50,    51,    52,    -1,    54,    55,    -1,    -1,    58,
      59,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   156,   157,   158,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,
      -1,   180,    -1,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,   208,
     209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   226,   227,   228,
      -1,   230,    -1,    -1,   233,   234,    -1,    -1,     5,     6,
      -1,    -1,   241,    -1,   243,    -1,   245,   246,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    33,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    53,    -1,    55,    -1,
      -1,    -1,    -1,    60,    61,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,
     147,    -1,    -1,    -1,   151,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,
     177,   178,    -1,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,
     207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   219,    -1,    -1,    -1,   223,    -1,    -1,   226,
     227,   228,    -1,   230,    -1,    -1,   233,   234,    -1,    -1,
      -1,     5,     6,    -1,   241,    -1,   243,    -1,   245,   246,
     247,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    33,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,   147,    -1,    -1,    -1,   151,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     174,   175,   176,   177,   178,    -1,   180,   181,   182,   183,
     184,   185,   186,    -1,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,    -1,    -1,    -1,   223,
      -1,    -1,   226,   227,   228,    -1,   230,    -1,    -1,   233,
     234,    -1,    -1,    -1,     5,     6,    -1,   241,    -1,   243,
      -1,   245,   246,   247,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,   180,
      -1,   182,   183,   184,   185,   186,    -1,    -1,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   223,    -1,    -1,   226,   227,   228,    -1,   230,
      -1,    -1,   233,   234,    -1,    -1,    -1,     5,     6,    -1,
     241,    -1,   243,    -1,   245,   246,   247,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,   156,   157,
     158,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,   186,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   226,   227,
     228,    -1,   230,    -1,    -1,   233,   234,    -1,    -1,     5,
       6,    -1,    -1,   241,    -1,   243,    -1,   245,   246,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,
     156,   157,   158,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
     186,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     226,   227,   228,    -1,   230,    -1,    -1,   233,   234,    -1,
      -1,     5,     6,    -1,    -1,   241,    -1,   243,    -1,   245,
     246,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    26,    27,    28,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    52,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     174,   175,   176,   177,   178,    -1,   180,    -1,   182,   183,
     184,   185,   186,    -1,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   226,   227,   228,    -1,   230,    -1,    -1,   233,
     234,    -1,    -1,     5,     6,    -1,    -1,   241,    -1,   243,
      -1,   245,   246,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   156,   157,   158,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   174,   175,   176,   177,   178,    -1,   180,    -1,
     182,   183,   184,   185,   186,    -1,    -1,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   226,   227,   228,    -1,   230,    -1,
      -1,   233,   234,    -1,    -1,     5,     6,    -1,    -1,   241,
      -1,   243,    -1,   245,   246,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      70,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,
     180,   181,   182,   183,   184,   185,   186,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   226,   227,   228,    -1,
     230,    -1,    -1,   233,   234,    -1,    -1,     5,     6,    -1,
      -1,   241,    -1,   243,   244,   245,   246,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   156,   157,
     158,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,   186,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   226,   227,
     228,    -1,   230,    -1,    -1,   233,   234,    -1,    -1,    -1,
      -1,     5,     6,   241,    -1,   243,    -1,   245,   246,    13,
      -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    49,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     174,   175,   176,   177,   178,    -1,   180,    -1,   182,   183,
     184,   185,   186,    -1,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   226,   227,   228,    -1,   230,    -1,    -1,   233,
     234,    -1,    -1,    -1,    -1,     5,     6,   241,   242,   243,
      -1,   245,   246,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    49,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,
     180,   181,   182,   183,   184,   185,   186,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   226,   227,   228,    -1,
     230,    -1,    -1,   233,   234,    -1,    -1,    -1,    -1,     5,
       6,   241,    -1,   243,    -1,   245,   246,    13,    -1,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    49,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
     186,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     226,   227,   228,    -1,   230,    -1,    -1,   233,   234,    -1,
      -1,    -1,    -1,     5,     6,   241,   242,   243,    -1,   245,
     246,    13,    -1,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    49,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   174,   175,   176,   177,   178,    -1,   180,    -1,
     182,   183,   184,   185,   186,    -1,    -1,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   226,   227,   228,    -1,   230,    -1,
      -1,   233,   234,    -1,    -1,     5,     6,    -1,    -1,   241,
      -1,   243,    -1,   245,   246,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      70,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,
     180,   181,   182,   183,   184,   185,   186,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   226,   227,   228,    -1,
     230,    -1,    -1,   233,   234,    -1,    -1,     5,     6,    -1,
      -1,   241,    -1,   243,    -1,   245,   246,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,   186,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   226,   227,
     228,    -1,   230,    -1,    -1,   233,   234,    -1,    -1,    -1,
      -1,     5,     6,   241,   242,   243,    -1,   245,   246,    13,
      -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     174,   175,   176,   177,   178,    -1,   180,    -1,   182,   183,
     184,   185,   186,    -1,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   226,   227,   228,    -1,   230,    -1,    -1,   233,
     234,    -1,    -1,    -1,    -1,     5,     6,   241,    -1,   243,
      -1,   245,   246,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,
     180,    -1,   182,   183,   184,   185,   186,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   226,   227,   228,    -1,
     230,    -1,    -1,   233,   234,    -1,    -1,     5,     6,    -1,
      -1,   241,    -1,   243,    -1,   245,   246,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    61,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,   186,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   226,   227,
     228,    -1,   230,    -1,    -1,   233,   234,    -1,    -1,     5,
       6,    -1,    -1,   241,    -1,   243,    -1,   245,   246,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
     186,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     226,   227,   228,    -1,   230,    -1,    -1,   233,   234,    -1,
      -1,     5,     6,    -1,    -1,   241,    -1,   243,    -1,   245,
     246,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     174,   175,   176,   177,   178,    -1,   180,    -1,   182,   183,
     184,   185,   186,    -1,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     224,    -1,   226,   227,   228,    -1,   230,    -1,    -1,   233,
     234,    -1,    -1,     5,     6,    -1,    -1,   241,    -1,   243,
      -1,   245,   246,    15,    16,    17,    18,    19,    -1,    -1,
      22,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   174,   175,   176,   177,   178,    -1,   180,    -1,
     182,   183,   184,   185,   186,    -1,    -1,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   226,   227,   228,    -1,   230,    -1,
      -1,   233,   234,    -1,    -1,     5,     6,    -1,    -1,   241,
      -1,   243,    -1,   245,   246,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,
     180,    -1,   182,   183,   184,   185,   186,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   226,   227,   228,    -1,
     230,    -1,    -1,   233,   234,    -1,    -1,     5,     6,    -1,
      -1,   241,    -1,   243,   244,   245,   246,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,   186,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   224,    -1,   226,   227,
     228,    -1,   230,    -1,    -1,   233,   234,    -1,    -1,     5,
       6,    -1,    -1,   241,    -1,   243,    -1,   245,   246,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
     186,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     226,   227,   228,    -1,   230,    -1,    -1,   233,   234,    -1,
      -1,     5,     6,    -1,    -1,   241,    -1,   243,   244,   245,
     246,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     174,   175,   176,   177,   178,    -1,   180,    -1,   182,   183,
     184,   185,   186,    -1,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   226,   227,   228,    -1,   230,    -1,    -1,   233,
     234,    -1,    -1,     5,     6,    -1,    -1,   241,    -1,   243,
     244,   245,   246,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   174,   175,   176,   177,   178,    -1,   180,    -1,
     182,   183,   184,   185,   186,    -1,    -1,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   226,   227,   228,    -1,   230,    -1,
      -1,   233,   234,    -1,    -1,     5,     6,    -1,    -1,   241,
      -1,   243,   244,   245,   246,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,
     180,    -1,   182,   183,   184,   185,   186,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   226,   227,   228,    -1,
     230,    -1,    -1,   233,   234,    -1,    -1,     5,     6,    -1,
      -1,   241,    -1,   243,   244,   245,   246,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,   186,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   226,   227,
     228,    -1,   230,    -1,    -1,   233,   234,    -1,    -1,     5,
       6,    -1,    -1,   241,    -1,   243,   244,   245,   246,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
     186,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     226,   227,   228,    -1,   230,    -1,    -1,   233,   234,    -1,
      -1,     5,     6,    -1,    -1,   241,    -1,   243,   244,   245,
     246,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     174,   175,   176,   177,   178,    -1,   180,    -1,   182,   183,
     184,   185,   186,    -1,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   226,   227,   228,    -1,   230,    -1,    -1,   233,
     234,    -1,    -1,     5,     6,    -1,    -1,   241,    -1,   243,
     244,   245,   246,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   174,   175,   176,   177,   178,    -1,   180,    -1,
     182,   183,   184,   185,   186,    -1,    -1,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   226,   227,   228,    -1,   230,    -1,
      -1,   233,   234,    -1,    -1,     5,     6,    -1,    -1,   241,
      -1,   243,    -1,   245,   246,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,
     180,    -1,   182,   183,   184,   185,   186,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   226,   227,   228,    -1,
     230,    -1,    -1,   233,   234,    -1,    -1,     5,     6,    -1,
      -1,   241,    -1,   243,    -1,   245,   246,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,   186,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   226,   227,
     228,    -1,   230,    -1,    -1,   233,   234,    -1,    -1,     5,
       6,    -1,    -1,   241,    -1,   243,    -1,   245,   246,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
     186,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     226,   227,   228,    -1,   230,    -1,    -1,   233,   234,    -1,
      -1,     5,     6,    -1,    -1,   241,    -1,   243,    -1,   245,
     246,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     174,   175,   176,   177,   178,    -1,   180,    -1,   182,   183,
     184,   185,   186,    -1,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    13,
      -1,    -1,    -1,   207,    -1,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,   226,   227,   228,    -1,   230,    41,    -1,   233,
     234,    -1,    -1,    -1,    -1,    49,    -1,   241,    -1,   243,
      -1,   245,   246,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      64,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,    -1,   131,   132,   133,   134,   135,   136,
     137,   138,    -1,    -1,    -1,    -1,    -1,   144,   145,    -1,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,   155,    -1,
      -1,    -1,    -1,   160,    -1,    -1,   163,   164,    -1,    -1,
      -1,    -1,   166,   170,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    13,    -1,    -1,    -1,    -1,    -1,
      19,    -1,    -1,   197,    -1,    -1,    25,    -1,    -1,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   218,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,   245,   246,
      -1,   245,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,    -1,   131,
     132,   133,   134,   135,   136,   137,   138,    -1,    -1,    -1,
      -1,    -1,   144,   145,    -1,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   160,    -1,
      -1,   163,   164,    -1,    -1,    -1,    -1,   166,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,   197,    -1,
      -1,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   218,    41,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      64,    -1,    -1,   245,   246,    -1,   245,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,    -1,   131,   132,   133,   134,   135,   136,
     137,   138,    -1,    -1,    -1,    -1,    -1,   144,   145,    -1,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   160,    -1,    -1,   163,   164,    -1,    -1,
      -1,    -1,   166,   170,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,
      -1,   188,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,   197,    -1,    -1,    -1,    -1,    25,    -1,
      -1,    -1,   209,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,   218,    -1,    -1,    41,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,   245,   246,
      -1,   245,    -1,   247,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   188,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,
     197,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,    31,
      -1,    -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,    41,
     217,    -1,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,   245,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    -1,    -1,   197,    -1,    -1,    25,    -1,
      -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,   245,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     139,   140,   141,   142,   143,    21,    22,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,
     159,    -1,   161,   162,    -1,    -1,    -1,    -1,   167,   168,
     169,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,   166,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     197,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   240,   241,    -1,    -1,   244,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   245,    -1,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    -1,
      -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,
      -1,   167,   168,   169,    -1,    -1,    -1,   173,   139,   140,
     141,   142,   143,    -1,    -1,   146,   147,   148,   149,    -1,
     151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,
     161,   162,    21,    22,    -1,    -1,   167,   168,   169,    -1,
      -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,    -1,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   240,   241,    -1,    -1,   244,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,
     241,    -1,    -1,   244,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,
      22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,
     159,    -1,   161,   162,    21,    22,    -1,    -1,   167,   168,
     169,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   240,   241,    -1,    -1,
     244,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   240,   241,    -1,    -1,   244,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,
     162,    21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,
      -1,   173,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,    -1,    -1,
      -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,
     167,   168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,   241,
      -1,    -1,   244,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   240,   241,    -1,    -1,   244,    -1,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,
      -1,   161,   162,    21,    22,    -1,    -1,   167,   168,   169,
      -1,    -1,    -1,   173,   139,   140,   141,   142,   143,    -1,
      -1,   146,   147,   148,   149,    -1,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,
      -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
      -1,   221,   222,   223,   224,   225,   226,   227,   228,   229,
     230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     240,   241,    -1,    -1,   244,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   240,   241,    -1,    -1,   244,
      -1,   139,   140,   141,   142,   143,    -1,    -1,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,
      -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,   139,   140,   141,   142,
     143,    -1,    -1,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,
      21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,
     173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,   230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   240,   241,    -1,    -1,   244,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,   241,    -1,
      -1,   244,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    -1,
      -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,
      -1,   167,   168,   169,    -1,    -1,    -1,   173,   139,   140,
     141,   142,   143,    -1,    -1,   146,   147,   148,   149,    -1,
     151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,
     161,   162,    21,    22,    -1,    -1,   167,   168,   169,    -1,
      -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,    -1,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   240,   241,    -1,    -1,   244,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,
     241,    -1,    -1,   244,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,
      22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,
     159,    -1,   161,   162,    21,    22,    -1,    -1,   167,   168,
     169,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   240,   241,    -1,    -1,
     244,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   240,   241,    -1,    -1,   244,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,
     162,    21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,
      -1,   173,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,    -1,    -1,
      -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,
     167,   168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,   241,
      -1,    -1,   244,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   240,   241,    -1,    -1,   244,    -1,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,
      -1,   161,   162,    21,    22,    -1,    -1,   167,   168,   169,
      -1,    -1,    -1,   173,   139,   140,   141,   142,   143,    -1,
      -1,   146,   147,   148,   149,    -1,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,
      -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
      -1,   221,   222,   223,   224,   225,   226,   227,   228,   229,
     230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     240,   241,    -1,    -1,   244,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   240,   241,    -1,    -1,   244,
      -1,   139,   140,   141,   142,   143,    -1,    -1,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,
      -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,   139,   140,   141,   142,
     143,    -1,    -1,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,
      21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,
     173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,   230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   240,   241,    -1,    -1,   244,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,   241,    -1,
      -1,   244,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    -1,
      -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,
      -1,   167,   168,   169,    -1,    -1,    -1,   173,   139,   140,
     141,   142,   143,    -1,    -1,   146,   147,   148,   149,    -1,
     151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,
     161,   162,    21,    22,    -1,    -1,   167,   168,   169,    -1,
      -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,    -1,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   240,   241,    -1,    -1,   244,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,
     241,   242,    -1,    -1,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,
      22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,
     159,    -1,   161,   162,    21,    22,    -1,    -1,   167,   168,
     169,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   240,   241,   242,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   240,   241,   242,    -1,    -1,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,
     162,    21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,
      -1,   173,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,    -1,    -1,
      -1,    -1,   159,    -1,   161,   162,    -1,    -1,    -1,    -1,
     167,   168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,   241,
     242,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   240,   241,   242,    -1,    -1,    -1,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    21,    22,    -1,    -1,   159,
      -1,   161,   162,    -1,    -1,    -1,    -1,   167,   168,   169,
      -1,    -1,    38,   173,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
      -1,   221,   222,   223,   224,   225,   226,   227,   228,   229,
     230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     240,   241,   242,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,    -1,    -1,   151,   152,   153,    -1,    -1,
      -1,    -1,    -1,    -1,   160,   161,   162,   163,   164,    -1,
      -1,   167,   168,   169,   170,   171,   172,   173,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,   139,   140,   141,   142,
     143,    -1,    -1,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,
      -1,    -1,    -1,    -1,   167,   168,   169,    21,    22,    -1,
     173,    -1,    -1,   219,    -1,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   188,    -1,   233,   234,    -1,
      -1,    -1,    -1,    -1,   240,   241,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   209,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,   241,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   139,   140,   141,   142,
     143,    -1,    -1,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    -1,    21,    22,    -1,   159,    -1,   161,   162,
      -1,    -1,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,
     173,    -1,    -1,    -1,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    21,    22,    -1,    -1,   159,    -1,   161,   162,    -1,
      -1,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
      -1,    -1,    -1,    -1,    -1,    -1,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,   241,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,    -1,    -1,    -1,
      -1,   139,   140,   141,   142,   143,   240,   241,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,    21,    22,    -1,
      -1,   159,    -1,   161,   162,    -1,    -1,    -1,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,
      -1,   161,   162,    -1,    -1,    -1,    -1,   167,   168,   169,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,   230,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   240,   241,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
      -1,   221,   222,   223,   224,   225,   226,   227,   228,   229,
     230,    -1,    -1,    -1,    -1,   139,   140,   141,   142,   143,
     240,   241,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,    -1,
      -1,    -1,    -1,   167,    -1,   169,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   240,   241,    71,    72,
      73,    19,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,   181,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   197,    -1,    -1,    -1,    19,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   181,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   197,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,    -1,
     181,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,
      -1,    -1,    -1,    -1,    -1,    -1,   197,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   152,   153,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    -1,    73,    -1,    75,
      76,    77,    78,    79,   181,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
     197,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   240,   241,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   197
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   249,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   125,   126,   188,   209,   230,   241,   250,   251,   255,
     264,   266,   267,   272,   304,   309,   345,   429,   436,   440,
     447,   494,   499,   504,    19,    20,   197,   296,   297,   298,
     189,   273,   274,   173,   197,   198,   199,   230,   240,   268,
     269,   270,   186,   207,   314,   437,   197,   245,   253,    57,
      63,   432,   432,   432,   166,   197,   331,    34,    63,   130,
     159,   234,   243,   300,   301,   302,   303,   331,   250,     5,
       6,     8,    36,   444,    62,   427,   218,   217,   220,   217,
     229,   229,   269,   229,    22,    57,   229,   240,   271,   432,
     433,   435,   433,   427,   505,   495,   500,   197,   166,   265,
     302,   302,   302,   243,   167,   168,   169,   217,   242,   130,
     130,   130,   308,    57,    63,   438,    57,    63,   445,    57,
      63,   428,    15,    16,   189,   194,   197,   200,   243,   246,
     257,   297,   189,   274,   269,   269,   269,   197,   268,   268,
     197,   188,   209,   402,   187,   208,   315,   433,   250,    57,
      63,   252,   197,   197,   197,   197,   201,   263,   244,   298,
     302,   302,   302,   302,    57,    63,   310,   312,   197,   439,
     448,   314,   430,   201,   202,   203,   256,    15,    16,   189,
     194,   197,   257,   294,   295,   246,   271,   434,   188,   402,
     234,   254,   506,   496,   501,   201,   244,   313,   220,   314,
     129,   442,   443,   425,   183,   246,   299,   396,   201,   202,
     203,   217,   244,   197,   218,    66,   220,   458,   314,   314,
      35,    71,    73,    75,    76,    77,    78,    79,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    93,
      94,    95,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   141,
     142,   197,   307,   311,    75,    79,    93,    94,    98,    99,
     100,   101,   452,   431,   197,   449,   315,   426,   298,   297,
     246,   250,   175,   197,   423,   424,   294,    19,    25,    31,
      41,    49,    64,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   179,
     245,   331,   451,   453,   454,   459,   465,   493,    79,    94,
      99,   101,   314,   497,   502,    21,    22,    38,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   151,   152,   153,   160,
     161,   162,   163,   164,   167,   168,   169,   170,   171,   172,
     173,   219,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   233,   234,   240,   241,    35,    35,   243,   305,
     314,   316,   314,   197,   315,   220,   441,   314,   446,   396,
     242,   297,   243,    43,   217,   220,   223,   422,   224,   224,
     224,   243,   224,   224,   243,   458,   224,   224,   224,   224,
     224,   243,   331,    33,    60,    61,   147,   151,   219,   223,
     226,   241,   247,   464,   221,   457,   413,   416,   197,   197,
     197,   242,    22,   197,   242,   178,   244,   396,   408,   409,
     410,   149,   220,   306,   319,   431,   218,   402,   331,   403,
     424,   242,     5,     6,    15,    16,    17,    18,    19,    25,
      27,    31,    39,    45,    48,    51,    55,    65,    68,    69,
      80,   125,   126,   127,   141,   142,   174,   175,   176,   177,
     178,   180,   182,   183,   184,   185,   189,   190,   191,   192,
     193,   194,   195,   196,   198,   199,   200,   207,   226,   227,
     228,   233,   234,   241,   243,   245,   246,   262,   264,   314,
     325,   331,   336,   350,   357,   360,   363,   368,   371,   375,
     376,   378,   383,   386,   387,   395,   451,   507,   522,   533,
     537,   550,   553,   197,   197,   465,   150,   160,   218,   421,
     466,   471,   473,   387,   475,   469,   197,   224,   477,   479,
     481,   483,   485,   487,   489,   491,   387,   224,   243,   322,
      33,   223,    33,   223,   241,   247,   242,   387,   241,   247,
     465,   197,   315,   197,   217,   250,   411,   462,   493,   498,
     197,   414,   462,   503,   197,   131,   132,   133,   134,   135,
     136,   137,   138,   160,   170,   171,   172,   131,   132,   133,
     134,   135,   136,   137,   138,   150,   160,   170,   171,   172,
     243,     7,    50,   344,   209,   209,   217,   244,   493,   493,
       1,     9,    10,    11,    13,    26,    28,    29,    38,    40,
      42,    44,    52,    54,    58,    59,    65,   127,   128,   156,
     157,   158,   188,   198,   209,   275,   276,   279,   282,   283,
     285,   288,   289,   290,   291,   315,   317,   318,   320,   325,
     330,   332,   337,   338,   339,   340,   341,   342,   343,   345,
     349,   372,   374,   387,   394,   315,   394,    40,   241,   304,
     315,   405,   402,   224,   224,   224,   333,   453,   507,   243,
     331,   224,     5,   125,   126,   224,   243,   224,   243,   243,
     224,   224,   243,   224,   243,   224,   243,   224,   224,   243,
     224,   224,   387,   387,   243,   243,   243,   243,   243,   243,
      13,   465,    13,   465,    13,   394,   532,   548,   224,   224,
     261,    13,   387,   387,   387,   387,   387,    13,    49,   321,
     361,   387,   361,   246,    13,   323,   532,   549,   197,   243,
     304,    21,    22,   139,   140,   141,   142,   143,   146,   147,
     148,   149,   151,   152,   153,   154,   159,   161,   162,   167,
     168,   169,   173,   219,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   240,   241,   244,   243,    43,   250,
     421,   330,   372,   394,   493,   493,   463,   493,   244,   493,
     493,   244,   225,   460,   493,   305,   493,   305,   493,   305,
     411,   412,   414,   415,   244,   468,   321,   242,   242,   387,
     188,   217,   218,   456,   220,   462,   315,   220,   462,   315,
     387,   175,   197,   418,   419,   450,   410,   410,   410,   394,
     286,   287,   150,   330,   361,   387,   394,   316,    61,   394,
     293,   394,   197,   250,   189,    58,   394,   316,   224,   150,
     330,   394,   246,   316,   363,   367,   367,   367,   394,   250,
     250,   394,    10,    37,   363,   369,   250,   250,   250,   250,
     250,    66,   346,   155,   250,   131,   132,   133,   134,   135,
     136,   137,   138,   144,   145,   150,   160,   163,   164,   170,
     171,   172,   218,   369,   402,   404,   303,     8,   396,   401,
     523,   525,   334,   243,   331,   224,   243,   358,   224,   224,
     224,   544,   361,   465,   323,   387,   351,   353,   387,   355,
     387,   546,   361,   529,   534,   361,   527,   465,   387,   387,
     387,   387,   387,   387,   450,    53,   181,   197,   226,   241,
     243,   394,   508,   511,   515,   531,   536,   450,   243,   511,
     536,   450,   165,   208,   210,   250,   516,   326,   328,   203,
     204,   256,   243,   243,   450,    13,   242,   217,   552,   552,
     175,   180,   224,   331,   377,   450,   315,   217,   552,    70,
     241,   244,   361,   508,   510,     4,   183,   366,    19,   181,
     197,   451,    19,   181,   197,   451,   387,   387,   387,   387,
     387,   387,   197,   387,   181,   197,   387,   387,   387,   451,
     387,   387,   387,   387,   387,   387,    22,   387,   387,   387,
     387,   387,   387,   387,   387,   387,   387,   387,   152,   153,
     181,   197,   240,   241,   384,   451,   387,   244,   361,   197,
     330,   394,   131,   132,   133,   134,   135,   136,   137,   138,
     144,   145,   150,   163,   164,   170,   171,   172,   218,   250,
     225,   225,   225,   462,   225,   225,   197,   455,   225,   306,
     225,   306,   225,   306,   225,   462,   225,   462,   324,   493,
     217,   552,   242,   394,   217,   493,   493,   244,   243,    43,
     150,   217,   218,   220,   223,   417,   316,   450,   243,   330,
     361,   217,    14,   394,   197,   316,   218,   220,   189,   465,
     330,   394,   246,   304,   316,   316,   284,   314,   370,   183,
     243,   348,   424,   367,   156,   157,   158,   317,   373,   394,
     373,   394,   373,   394,   373,   394,   373,   394,   373,   394,
     373,   394,   373,   394,   373,   394,   373,   394,   373,   394,
     394,   373,   394,   373,   394,   373,   394,   373,   394,   373,
     394,   373,   394,   197,   242,    57,    63,   399,    67,   400,
     250,   465,   465,   493,    70,   361,   510,   521,   224,   394,
     197,   394,   493,   538,   540,   542,   465,   552,   225,   462,
     244,   244,   465,   465,   244,   465,   244,   465,   552,   465,
     412,   552,   415,   225,   244,   244,   244,   244,   244,   244,
      20,   367,   243,   160,   417,   241,   387,   244,   165,   217,
     250,   515,   212,   213,   242,   519,   217,   212,   242,   250,
     518,    20,   244,   515,   208,   211,   517,    20,   394,   208,
     532,   324,   324,   394,   450,   450,    20,   243,   450,   387,
     244,   243,   243,   379,   381,    20,   532,   197,   244,   510,
     508,   217,   244,   217,   552,   244,   243,   150,   160,   197,
     218,   223,   364,   365,   305,   224,   243,   224,   243,   243,
     243,   242,    19,   181,   197,   451,   220,   181,   197,   387,
     243,   243,   181,   197,   387,     1,   243,   242,   244,   250,
     394,   394,   394,   394,   394,   394,   394,   394,   394,   394,
     394,   394,   394,   394,   394,   394,   394,   467,   472,   474,
     493,   476,   470,   217,   225,   250,   478,   225,   482,   225,
     486,   225,   490,   411,   492,   414,   225,   462,   244,   188,
     456,   387,   197,   197,   493,   394,    20,   450,   316,   218,
     292,   225,   366,    12,    23,    24,   188,   277,   278,   394,
     319,   304,   197,   347,   347,   367,   367,   367,   218,   250,
      47,   400,    46,   129,   397,   402,   225,   225,   225,   510,
     244,   244,   244,   197,   244,   225,   250,   244,   225,   465,
     412,   415,   225,   244,   243,   465,   225,   225,   225,   225,
     244,   225,   225,   244,   225,   366,   243,   361,   387,   394,
     394,   511,   515,   394,   181,   197,   508,   519,   242,   394,
     242,   531,   361,   511,   208,   211,   214,   520,   242,   361,
     225,   225,   220,   259,    20,    20,   361,   450,    20,   387,
     387,   465,   305,   361,   244,   242,   241,   365,   197,   197,
     243,   197,   197,   217,   242,   306,   388,   387,   390,   387,
     244,   361,   387,   224,   243,   387,   243,   242,   387,   241,
     244,   361,   243,   242,   385,   244,   361,   197,   461,   197,
     480,   484,   488,   322,   493,   244,    43,   417,   361,    20,
     493,   394,   366,   305,   316,   278,   394,    12,   280,   315,
     366,   217,   242,   244,   493,   402,    33,   398,   397,   399,
     524,   526,   335,   244,   225,   462,   243,   197,   359,   225,
     225,   225,   545,   323,   225,   352,   354,   356,   547,   530,
     535,   528,   243,   244,   361,   209,   244,   515,   519,   243,
     160,   417,   209,   515,   242,   209,   327,   329,   260,   205,
     361,   361,   209,    20,   361,   244,   244,   225,   306,   209,
     244,   508,   244,   197,   364,   242,   165,   316,   362,   465,
     244,   493,   244,   244,   244,   392,   387,   387,   244,   508,
     244,   387,   244,   197,   394,   316,   361,   250,   250,   369,
     306,   316,   281,   250,   305,   197,   242,   220,   422,   250,
     406,   398,   418,   419,   420,   243,   243,   394,   197,   394,
     225,   539,   541,   543,   243,   244,   243,   394,   394,   394,
     243,    70,   521,   243,   243,   244,   387,   244,   387,   160,
     417,   519,   387,   394,   394,   387,   520,   532,   394,   322,
     258,   244,   244,   387,   361,   209,   380,   225,   532,   242,
     244,   150,   394,   225,   225,   493,   244,   244,   242,   244,
     244,   362,   188,   278,    26,   128,   282,   337,   338,   339,
     341,   394,   306,   220,   422,   465,   421,   402,   311,   407,
     521,   521,   244,   225,   244,   243,   243,   243,   243,   321,
     323,   361,   521,   521,   244,   250,   551,   394,   394,   244,
     551,   551,   201,   209,   209,   551,   244,   387,   377,   382,
     551,   244,   394,   389,   391,   225,   244,   316,   278,   150,
     150,   394,   316,   465,   421,   421,   394,   250,   311,   243,
     508,   512,   513,   514,   514,   394,   394,   521,   521,   508,
     509,   244,   244,   552,   514,   509,    53,   242,   160,   417,
     208,   315,   532,   387,   242,   209,   551,   377,   315,   393,
     394,   421,   394,   394,   250,   402,   316,   508,   217,   552,
     244,   244,   244,   244,   514,   514,   244,   244,   244,   244,
     394,   242,   394,   394,   242,   315,   551,   551,   387,   242,
     394,   250,   250,   402,   244,   243,   244,   244,   208,   242,
     551,   250,   508,   242,   244
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   248,   249,   249,   249,   249,   249,   249,   249,   249,
     249,   249,   249,   249,   249,   249,   249,   250,   250,   251,
     252,   252,   252,   253,   253,   254,   254,   255,   256,   256,
     256,   256,   257,   257,   258,   258,   259,   260,   259,   261,
     261,   261,   262,   263,   263,   265,   264,   266,   267,   268,
     268,   268,   269,   269,   269,   269,   269,   269,   269,   270,
     270,   271,   271,   272,   273,   273,   274,   274,   275,   276,
     276,   277,   277,   278,   278,   278,   278,   279,   279,   280,
     281,   280,   282,   282,   282,   282,   282,   283,   283,   284,
     283,   286,   285,   287,   285,   288,   289,   290,   292,   291,
     293,   291,   294,   294,   294,   294,   294,   294,   295,   295,
     296,   296,   296,   297,   297,   297,   297,   297,   297,   297,
     297,   297,   298,   298,   299,   299,   299,   300,   300,   300,
     300,   301,   301,   302,   302,   302,   302,   302,   302,   302,
     303,   303,   304,   304,   305,   305,   305,   306,   306,   306,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   308,   308,   309,   310,   310,   310,   311,   313,
     312,   314,   314,   315,   315,   316,   316,   317,   317,   317,
     318,   318,   318,   318,   318,   318,   318,   318,   318,   318,
     318,   318,   318,   318,   318,   318,   318,   318,   318,   318,
     318,   318,   319,   319,   319,   320,   321,   321,   322,   322,
     323,   323,   324,   324,   326,   327,   325,   328,   329,   325,
     330,   330,   330,   330,   330,   331,   331,   331,   332,   332,
     334,   335,   333,   333,   336,   336,   336,   336,   336,   336,
     337,   338,   339,   339,   339,   340,   340,   340,   341,   341,
     342,   342,   342,   343,   344,   344,   344,   345,   345,   346,
     346,   347,   347,   348,   348,   348,   348,   349,   349,   351,
     352,   350,   353,   354,   350,   355,   356,   350,   358,   359,
     357,   360,   360,   360,   360,   360,   360,   361,   361,   362,
     362,   362,   363,   363,   363,   364,   364,   364,   364,   364,
     365,   365,   366,   366,   366,   367,   367,   368,   370,   369,
     371,   371,   371,   371,   371,   371,   371,   371,   372,   372,
     372,   372,   372,   372,   372,   372,   372,   372,   372,   372,
     372,   372,   372,   372,   372,   372,   372,   373,   373,   373,
     373,   374,   374,   374,   374,   374,   374,   374,   374,   374,
     374,   374,   374,   374,   374,   374,   374,   374,   375,   375,
     376,   376,   377,   377,   378,   379,   380,   378,   381,   382,
     378,   383,   383,   383,   383,   383,   383,   383,   384,   385,
     383,   386,   386,   386,   386,   386,   386,   386,   387,   387,
     387,   387,   387,   387,   387,   387,   387,   387,   387,   387,
     387,   387,   387,   387,   387,   387,   387,   387,   387,   387,
     387,   387,   387,   387,   387,   387,   387,   387,   387,   387,
     387,   387,   387,   387,   387,   387,   387,   387,   387,   387,
     387,   387,   387,   387,   387,   387,   387,   387,   387,   387,
     387,   387,   387,   387,   388,   389,   387,   387,   387,   387,
     390,   391,   387,   387,   387,   392,   393,   387,   387,   387,
     387,   387,   387,   387,   387,   387,   387,   387,   387,   387,
     387,   387,   394,   395,   395,   395,   395,   395,   395,   395,
     395,   395,   395,   395,   395,   395,   395,   395,   395,   396,
     396,   396,   397,   397,   397,   398,   398,   399,   399,   399,
     400,   400,   401,   402,   402,   402,   402,   403,   404,   403,
     405,   403,   406,   403,   407,   403,   403,   408,   409,   409,
     410,   410,   410,   410,   410,   411,   411,   412,   412,   413,
     413,   413,   414,   415,   415,   416,   416,   416,   417,   417,
     418,   418,   418,   419,   419,   420,   420,   421,   421,   421,
     422,   422,   423,   423,   423,   423,   423,   424,   424,   424,
     424,   424,   424,   425,   426,   425,   427,   427,   428,   428,
     428,   429,   430,   429,   431,   431,   431,   432,   432,   432,
     434,   433,   435,   435,   436,   437,   436,   438,   438,   438,
     439,   440,   440,   441,   441,   442,   442,   443,   444,   444,
     444,   444,   445,   445,   445,   446,   446,   448,   449,   447,
     450,   450,   450,   450,   450,   451,   451,   451,   451,   451,
     451,   451,   451,   451,   451,   451,   451,   451,   451,   451,
     451,   451,   451,   451,   451,   451,   451,   451,   451,   451,
     451,   451,   451,   451,   451,   451,   451,   451,   451,   451,
     451,   451,   451,   451,   451,   451,   451,   451,   451,   451,
     451,   451,   451,   451,   451,   452,   452,   452,   452,   452,
     452,   452,   452,   453,   454,   454,   454,   455,   455,   455,
     456,   456,   457,   457,   457,   457,   457,   457,   457,   458,
     458,   458,   458,   458,   459,   460,   461,   459,   462,   462,
     463,   463,   464,   464,   465,   465,   465,   465,   465,   465,
     466,   467,   465,   465,   465,   468,   465,   465,   465,   465,
     465,   465,   465,   465,   465,   465,   465,   465,   465,   469,
     470,   465,   465,   471,   472,   465,   473,   474,   465,   475,
     476,   465,   465,   477,   478,   465,   479,   480,   465,   465,
     481,   482,   465,   483,   484,   465,   465,   485,   486,   465,
     487,   488,   465,   489,   490,   465,   491,   492,   465,   493,
     493,   493,   495,   496,   497,   498,   494,   500,   501,   502,
     503,   499,   505,   506,   504,   507,   507,   507,   507,   507,
     508,   508,   508,   508,   508,   508,   508,   508,   509,   509,
     510,   511,   511,   512,   512,   513,   513,   514,   514,   515,
     515,   516,   516,   517,   517,   518,   518,   519,   519,   519,
     520,   520,   520,   521,   521,   522,   522,   522,   522,   522,
     522,   523,   524,   522,   525,   526,   522,   527,   528,   522,
     529,   530,   522,   531,   531,   531,   532,   532,   533,   534,
     535,   533,   536,   536,   537,   537,   537,   538,   539,   537,
     540,   541,   537,   542,   543,   537,   537,   544,   545,   537,
     537,   546,   547,   537,   548,   548,   549,   549,   550,   550,
     550,   550,   550,   551,   551,   552,   552,   553,   553,   553,
     553,   553,   553,   553,   553,   553
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     1,     1,     2,
       0,     1,     1,     1,     1,     0,     2,     5,     1,     1,
       2,     2,     3,     2,     0,     2,     0,     0,     3,     0,
       2,     5,     3,     1,     2,     0,     4,     2,     2,     1,
       1,     1,     1,     2,     3,     3,     3,     3,     3,     2,
       4,     0,     1,     2,     1,     3,     1,     3,     3,     3,
       2,     1,     1,     0,     2,     4,     5,     1,     1,     0,
       0,     3,     1,     1,     1,     1,     1,     5,     4,     0,
       6,     0,     6,     0,     8,     2,     3,     3,     0,     6,
       0,     6,     1,     1,     1,     1,     1,     1,     1,     3,
       1,     1,     1,     3,     3,     5,     3,     3,     3,     3,
       1,     5,     1,     3,     2,     3,     2,     1,     1,     1,
       1,     1,     4,     1,     2,     3,     3,     3,     3,     2,
       1,     3,     0,     3,     0,     2,     3,     0,     2,     2,
       1,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     3,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     3,     2,
       2,     3,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     3,     2,     2,     2,     2,     2,
       3,     3,     3,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     1,     4,     0,     1,     1,     3,     0,
       4,     1,     1,     1,     1,     3,     7,     2,     2,     6,
       1,     1,     1,     1,     1,     2,     2,     1,     1,     1,
       1,     1,     1,     2,     2,     1,     1,     1,     1,     2,
       2,     2,     0,     2,     2,     3,     0,     2,     0,     4,
       0,     2,     1,     3,     0,     0,     7,     0,     0,     7,
       3,     2,     2,     2,     1,     1,     3,     2,     2,     3,
       0,     0,     5,     1,     2,     5,     5,     5,     6,     2,
       1,     1,     1,     2,     3,     2,     2,     3,     2,     3,
       2,     2,     3,     4,     1,     1,     0,     1,     1,     1,
       0,     1,     3,     9,     8,     8,     7,     3,     3,     0,
       0,     7,     0,     0,     7,     0,     0,     7,     0,     0,
       6,     5,     8,    10,     5,     8,    10,     1,     3,     1,
       2,     3,     1,     1,     2,     2,     2,     2,     2,     4,
       1,     3,     0,     4,     4,     1,     6,     6,     0,     7,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     6,     8,
       5,     6,     1,     4,     3,     0,     0,     8,     0,     0,
       9,     3,     4,     5,     6,     8,     5,     6,     0,     0,
       5,     3,     4,     4,     5,     4,     3,     4,     1,     1,
       1,     2,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     4,     5,     4,     5,     3,     4,     2,     5,     1,
       1,     1,     1,     1,     1,     1,     4,     1,     1,     4,
       4,     7,     8,     3,     0,     0,     8,     3,     3,     3,
       0,     0,     8,     3,     4,     0,     0,     9,     4,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     3,     2,
       4,     1,     1,     4,     4,     4,     4,     4,     1,     6,
       7,     6,     6,     7,     7,     6,     7,     6,     6,     0,
       4,     1,     0,     1,     1,     0,     1,     0,     1,     1,
       0,     1,     5,     1,     1,     2,     0,     0,     0,     8,
       0,     5,     0,    10,     0,    11,     6,     3,     3,     4,
       1,     1,     3,     3,     3,     1,     3,     1,     3,     0,
       2,     3,     3,     1,     3,     0,     2,     3,     1,     1,
       1,     2,     3,     3,     5,     1,     1,     1,     1,     1,
       0,     1,     1,     4,     3,     3,     5,     4,     6,     5,
       5,     4,     4,     0,     0,     5,     0,     1,     0,     1,
       1,     6,     0,     6,     0,     3,     5,     0,     1,     1,
       0,     5,     2,     3,     4,     0,     4,     0,     1,     1,
       1,     7,     9,     0,     2,     0,     1,     3,     1,     1,
       2,     2,     0,     1,     1,     0,     3,     0,     0,     7,
       1,     4,     3,     3,     5,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     4,     4,     1,     3,     3,
       1,     2,     0,     3,     3,     2,     5,     5,     4,     0,
       2,     2,     2,     2,     4,     0,     0,     7,     1,     1,
       1,     3,     3,     4,     1,     1,     1,     1,     2,     3,
       0,     0,     6,     4,     3,     0,     7,     4,     2,     2,
       3,     2,     3,     2,     2,     3,     3,     3,     2,     0,
       0,     6,     2,     0,     0,     6,     0,     0,     6,     0,
       0,     6,     1,     0,     0,     6,     0,     0,     7,     1,
       0,     0,     6,     0,     0,     7,     1,     0,     0,     6,
       0,     0,     7,     0,     0,     6,     0,     0,     6,     1,
       3,     3,     0,     0,     0,     0,    10,     0,     0,     0,
       0,    10,     0,     0,     9,     1,     1,     1,     1,     1,
       3,     3,     5,     5,     6,     6,     8,     8,     0,     1,
       2,     1,     3,     3,     5,     1,     2,     1,     0,     0,
       2,     2,     1,     2,     1,     2,     1,     2,     1,     1,
       2,     1,     1,     0,     1,     5,     4,     6,     7,     5,
       7,     0,     0,    10,     0,     0,    10,     0,     0,    10,
       0,     0,     7,     1,     3,     3,     3,     1,     5,     0,
       0,    10,     1,     3,     3,     4,     4,     0,     0,    11,
       0,     0,    11,     0,     0,    10,     5,     0,     0,     9,
       5,     0,     0,    10,     1,     3,     1,     3,     3,     3,
       4,     7,     9,     0,     3,     0,     1,     9,    11,    12,
      11,    10,    10,    10,     9,    10
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = DAS_YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == DAS_YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (&yylloc, scanner, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use DAS_YYerror or DAS_YYUNDEF. */
#define YYERRCODE DAS_YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if DAS_YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined DAS_YYLTYPE_IS_TRIVIAL && DAS_YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t scanner)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  YY_USE (scanner);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t scanner)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, scanner);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule, yyscan_t scanner)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]), scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, scanner); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !DAS_YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !DAS_YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, yyscan_t scanner)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  YY_USE (scanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_NAME: /* "name"  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_KEYWORD: /* "keyword"  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_TYPE_FUNCTION: /* "type function"  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_module_name: /* module_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_character_sequence: /* character_sequence  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_string_constant: /* string_constant  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_format_string: /* format_string  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_optional_format_string: /* optional_format_string  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_string_builder_body: /* string_builder_body  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_string_builder: /* string_builder  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_reader: /* expr_reader  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_keyword_or_name: /* keyword_or_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_require_module_name: /* require_module_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_expression_label: /* expression_label  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_goto: /* expression_goto  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_else: /* expression_else  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_else_one_liner: /* expression_else_one_liner  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_if_one_liner: /* expression_if_one_liner  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_if_then_else: /* expression_if_then_else  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_for_loop: /* expression_for_loop  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_unsafe: /* expression_unsafe  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_while_loop: /* expression_while_loop  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_with: /* expression_with  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_with_alias: /* expression_with_alias  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_annotation_argument_value: /* annotation_argument_value  */
            { delete ((*yyvaluep).aa); }
        break;

    case YYSYMBOL_annotation_argument_value_list: /* annotation_argument_value_list  */
            { delete ((*yyvaluep).aaList); }
        break;

    case YYSYMBOL_annotation_argument_name: /* annotation_argument_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_annotation_argument: /* annotation_argument  */
            { delete ((*yyvaluep).aa); }
        break;

    case YYSYMBOL_annotation_argument_list: /* annotation_argument_list  */
            { delete ((*yyvaluep).aaList); }
        break;

    case YYSYMBOL_metadata_argument_list: /* metadata_argument_list  */
            { delete ((*yyvaluep).aaList); }
        break;

    case YYSYMBOL_annotation_declaration_name: /* annotation_declaration_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_annotation_declaration_basic: /* annotation_declaration_basic  */
            { /* gc owns AnnotationDeclaration */ }
        break;

    case YYSYMBOL_annotation_declaration: /* annotation_declaration  */
            { /* gc owns AnnotationDeclaration */ }
        break;

    case YYSYMBOL_annotation_list: /* annotation_list  */
            { delete ((*yyvaluep).faList); }
        break;

    case YYSYMBOL_optional_annotation_list: /* optional_annotation_list  */
            { delete ((*yyvaluep).faList); }
        break;

    case YYSYMBOL_optional_function_argument_list: /* optional_function_argument_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_optional_function_type: /* optional_function_type  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_function_name: /* function_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_function_declaration_header: /* function_declaration_header  */
            { ((*yyvaluep).pFuncDecl)->delRef(); }
        break;

    case YYSYMBOL_function_declaration: /* function_declaration  */
            { ((*yyvaluep).pFuncDecl)->delRef(); }
        break;

    case YYSYMBOL_expression_block: /* expression_block  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_call_pipe: /* expr_call_pipe  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_any: /* expression_any  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expressions: /* expressions  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_keyword: /* expr_keyword  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_optional_expr_list: /* optional_expr_list  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_optional_expr_list_in_braces: /* optional_expr_list_in_braces  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_optional_expr_map_tuple_list: /* optional_expr_map_tuple_list  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_type_declaration_no_options_list: /* type_declaration_no_options_list  */
            { deleteTypeDeclarationList(((*yyvaluep).pTypeDeclList)); }
        break;

    case YYSYMBOL_expression_keyword: /* expression_keyword  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_pipe: /* expr_pipe  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_name_in_namespace: /* name_in_namespace  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_expression_delete: /* expression_delete  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_new_type_declaration: /* new_type_declaration  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_expr_new: /* expr_new  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_break: /* expression_break  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_continue: /* expression_continue  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_return_no_pipe: /* expression_return_no_pipe  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_return: /* expression_return  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_yield_no_pipe: /* expression_yield_no_pipe  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_yield: /* expression_yield  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_try_catch: /* expression_try_catch  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_tuple_expansion: /* tuple_expansion  */
            { delete ((*yyvaluep).pNameList); }
        break;

    case YYSYMBOL_tuple_expansion_variable_declaration: /* tuple_expansion_variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_expression_let: /* expression_let  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_cast: /* expr_cast  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_type_decl: /* expr_type_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_type_info: /* expr_type_info  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_list: /* expr_list  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_block_or_simple_block: /* block_or_simple_block  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_capture_entry: /* capture_entry  */
            { delete ((*yyvaluep).pCapt); }
        break;

    case YYSYMBOL_capture_list: /* capture_list  */
            { delete ((*yyvaluep).pCaptList); }
        break;

    case YYSYMBOL_optional_capture_list: /* optional_capture_list  */
            { delete ((*yyvaluep).pCaptList); }
        break;

    case YYSYMBOL_expr_block: /* expr_block  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_full_block: /* expr_full_block  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_full_block_assumed_piped: /* expr_full_block_assumed_piped  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_numeric_const: /* expr_numeric_const  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_assign: /* expr_assign  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_assign_pipe_right: /* expr_assign_pipe_right  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_assign_pipe: /* expr_assign_pipe  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_named_call: /* expr_named_call  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_method_call: /* expr_method_call  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_func_addr_name: /* func_addr_name  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_func_addr_expr: /* func_addr_expr  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_field: /* expr_field  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_call: /* expr_call  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr2: /* expr2  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr: /* expr  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_mtag: /* expr_mtag  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_optional_field_annotation: /* optional_field_annotation  */
            { delete ((*yyvaluep).aaList); }
        break;

    case YYSYMBOL_structure_variable_declaration: /* structure_variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_struct_variable_declaration_list: /* struct_variable_declaration_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_function_argument_declaration_no_type: /* function_argument_declaration_no_type  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_function_argument_declaration_type: /* function_argument_declaration_type  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_function_argument_list: /* function_argument_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_tuple_type: /* tuple_type  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_tuple_type_list: /* tuple_type_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_tuple_alias_type_list: /* tuple_alias_type_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_variant_type: /* variant_type  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_variant_type_list: /* variant_type_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_variant_alias_type_list: /* variant_alias_type_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_variable_declaration_no_type: /* variable_declaration_no_type  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_variable_declaration_type: /* variable_declaration_type  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_variable_declaration: /* variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_let_variable_name_with_pos_list: /* let_variable_name_with_pos_list  */
            { delete ((*yyvaluep).pNameWithPosList); }
        break;

    case YYSYMBOL_let_variable_declaration: /* let_variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_global_variable_declaration_list: /* global_variable_declaration_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_enum_list: /* enum_list  */
            { delete ((*yyvaluep).pEnumList); }
        break;

    case YYSYMBOL_alias_list: /* alias_list  */
            { delete ((*yyvaluep).positions); }
        break;

    case YYSYMBOL_enum_name: /* enum_name  */
            { /* $$->delRef(); // if enum rule returns, module already has the link */ }
        break;

    case YYSYMBOL_optional_structure_parent: /* optional_structure_parent  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_optional_struct_variable_declaration_list: /* optional_struct_variable_declaration_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_variable_name_with_pos_list: /* variable_name_with_pos_list  */
            { delete ((*yyvaluep).pNameWithPosList); }
        break;

    case YYSYMBOL_structure_type_declaration: /* structure_type_declaration  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_auto_type_declaration: /* auto_type_declaration  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_bitfield_bits: /* bitfield_bits  */
            { delete ((*yyvaluep).pNameList); }
        break;

    case YYSYMBOL_bitfield_alias_bits: /* bitfield_alias_bits  */
            { deleteNameExprList(((*yyvaluep).pNameExprList)); }
        break;

    case YYSYMBOL_bitfield_type_declaration: /* bitfield_type_declaration  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_table_type_pair: /* table_type_pair  */
            { delete ((*yyvaluep).aTypePair).firstType; delete ((*yyvaluep).aTypePair).secondType; }
        break;

    case YYSYMBOL_dim_list: /* dim_list  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_type_declaration_no_options: /* type_declaration_no_options  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_type_declaration: /* type_declaration  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_make_decl: /* make_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_struct_fields: /* make_struct_fields  */
            { /* gc owns MakeStruct */ }
        break;

    case YYSYMBOL_make_variant_dim: /* make_variant_dim  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_struct_single: /* make_struct_single  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_struct_dim: /* make_struct_dim  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_struct_dim_list: /* make_struct_dim_list  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_struct_dim_decl: /* make_struct_dim_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_optional_make_struct_dim_decl: /* optional_make_struct_dim_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_optional_block: /* optional_block  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_struct_decl: /* make_struct_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_tuple: /* make_tuple  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_map_tuple: /* make_map_tuple  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_tuple_call: /* make_tuple_call  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_dim: /* make_dim  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_dim_decl: /* make_dim_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_table: /* make_table  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_map_tuple_list: /* expr_map_tuple_list  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_table_decl: /* make_table_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_array_comprehension_where: /* array_comprehension_where  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_array_comprehension: /* array_comprehension  */
            { delete ((*yyvaluep).pExpression); }
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (yyscan_t scanner)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined DAS_YYLTYPE_IS_TRIVIAL && DAS_YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = DAS_YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == DAS_YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc, scanner);
    }

  if (yychar <= DAS_YYEOF)
    {
      yychar = DAS_YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == DAS_YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = DAS_YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = DAS_YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 3: /* program: program module_declaration  */
                                   {
            if ( yyextra->das_has_type_declarations ) {
                das_yyerror(scanner,"module name has to be first declaration",tokAt(scanner,(yylsp[0])), CompilationError::syntax_error);
            }
        }
    break;

  case 4: /* program: program structure_declaration  */
                                                { yyextra->das_has_type_declarations = true; }
    break;

  case 5: /* program: program enum_declaration  */
                                                { yyextra->das_has_type_declarations = true; }
    break;

  case 6: /* program: program global_let  */
                                                { yyextra->das_has_type_declarations = true; }
    break;

  case 7: /* program: program global_function_declaration  */
                                                { yyextra->das_has_type_declarations = true; }
    break;

  case 11: /* program: program alias_declaration  */
                                                { yyextra->das_has_type_declarations = true; }
    break;

  case 12: /* program: program variant_alias_declaration  */
                                                { yyextra->das_has_type_declarations = true; }
    break;

  case 13: /* program: program tuple_alias_declaration  */
                                                { yyextra->das_has_type_declarations = true; }
    break;

  case 14: /* program: program bitfield_alias_declaration  */
                                                { yyextra->das_has_type_declarations = true; }
    break;

  case 17: /* semicolon: SEMICOLON  */
                       {
        format::try_semicolon_at_eol(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
    }
    break;

  case 18: /* semicolon: "end of expression"  */
          {}
    break;

  case 19: /* top_level_reader_macro: expr_reader semicolon  */
                                   {
        delete (yyvsp[-1].pExpression);    // we do nothing, we don't even attemp to 'visit'
    }
    break;

  case 20: /* optional_public_or_private_module: %empty  */
                        { (yyval.b) = yyextra->g_Program->policies.default_module_public; }
    break;

  case 21: /* optional_public_or_private_module: "public"  */
                        { (yyval.b) = true; }
    break;

  case 22: /* optional_public_or_private_module: "private"  */
                        { (yyval.b) = false; }
    break;

  case 23: /* module_name: '$'  */
                    { (yyval.s) = new string("$"); }
    break;

  case 24: /* module_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 25: /* optional_not_required: %empty  */
        { (yyval.b) = false; }
    break;

  case 26: /* optional_not_required: '!' "inscope"  */
                        { (yyval.b) = true; }
    break;

  case 27: /* module_declaration: "module" module_name optional_shared optional_public_or_private_module optional_not_required  */
                                                                                                                                    {
        yyextra->g_Program->thisModuleName = *(yyvsp[-3].s);
        yyextra->g_Program->thisModule->isPublic = (yyvsp[-1].b);
        yyextra->g_Program->thisModule->isModule = true;
        yyextra->g_Program->thisModule->visibleEverywhere = (yyvsp[0].b);
        if ( yyextra->g_Program->thisModule->name.empty() ) {
            yyextra->g_Program->library.renameModule(yyextra->g_Program->thisModule.get(),*(yyvsp[-3].s));
        } else if ( yyextra->g_Program->thisModule->name != *(yyvsp[-3].s) ){
            das_yyerror(scanner,"this module already has a name " + yyextra->g_Program->thisModule->name,tokAt(scanner,(yylsp[-3])),
                CompilationError::module_already_has_a_name);
        }
        if ( !yyextra->g_Program->policies.ignore_shared_modules ) {
            yyextra->g_Program->promoteToBuiltin = (yyvsp[-2].b);
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 28: /* character_sequence: STRING_CHARACTER  */
                                                                                  { (yyval.s) = new string(); *(yyval.s) += (yyvsp[0].ch); }
    break;

  case 29: /* character_sequence: STRING_CHARACTER_ESC  */
                                                                                  { (yyval.s) = new string(); *(yyval.s) += "\\\\"; }
    break;

  case 30: /* character_sequence: character_sequence STRING_CHARACTER  */
                                                                                  { (yyval.s) = (yyvsp[-1].s); *(yyvsp[-1].s) += (yyvsp[0].ch); }
    break;

  case 31: /* character_sequence: character_sequence STRING_CHARACTER_ESC  */
                                                                                  { (yyval.s) = (yyvsp[-1].s); *(yyvsp[-1].s) += "\\\\"; }
    break;

  case 32: /* string_constant: "start of the string" character_sequence "end of the string"  */
                                                           { (yyval.s) = (yyvsp[-1].s); }
    break;

  case 33: /* string_constant: "start of the string" "end of the string"  */
                                                           { (yyval.s) = new string(); }
    break;

  case 34: /* format_string: %empty  */
        { (yyval.s) = new string(); }
    break;

  case 35: /* format_string: format_string STRING_CHARACTER  */
                                                 { (yyval.s) = (yyvsp[-1].s); (yyvsp[-1].s)->push_back((yyvsp[0].ch)); }
    break;

  case 36: /* optional_format_string: %empty  */
        { (yyval.s) = new string(""); }
    break;

  case 37: /* $@1: %empty  */
            { das_strfmt(scanner); }
    break;

  case 38: /* optional_format_string: ':' $@1 format_string  */
                                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 39: /* string_builder_body: %empty  */
        {
        (yyval.pExpression) = new ExprStringBuilder();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 40: /* string_builder_body: string_builder_body character_sequence  */
                                                                                  {
        bool err;
        auto esconst = unescapeString(*(yyvsp[0].s),&err);
        if ( err ) das_yyerror(scanner,"invalid escape sequence",tokAt(scanner,(yylsp[-1])), CompilationError::invalid_escape_sequence);
        auto sc = new ExprConstString(tokAt(scanner,(yylsp[0])),esconst);
        delete (yyvsp[0].s);
        static_cast<ExprStringBuilder *>((yyvsp[-1].pExpression))->elements.push_back(sc);
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 41: /* string_builder_body: string_builder_body "{" expr optional_format_string "}"  */
                                                                                                                                     {
        auto se = (yyvsp[-2].pExpression);
        if ( !(yyvsp[-1].s)->empty() ) {
            auto call_fmt = new ExprCall(tokAt(scanner,(yylsp[-1])), "_::fmt");
            call_fmt->arguments.push_back(new ExprConstString(tokAt(scanner,(yylsp[-1])),":" + *(yyvsp[-1].s)));
            call_fmt->arguments.push_back(se);
            se = call_fmt;
        }
        static_cast<ExprStringBuilder *>((yyvsp[-4].pExpression))->elements.push_back(se);
        (yyval.pExpression) = (yyvsp[-4].pExpression);
        delete (yyvsp[-1].s);
    }
    break;

  case 42: /* string_builder: "start of the string" string_builder_body "end of the string"  */
                                                                   {
        auto strb = static_cast<ExprStringBuilder *>((yyvsp[-1].pExpression));
        if ( strb->elements.size()==0 ) {
            (yyval.pExpression) = new ExprConstString(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),"");
            delete (yyvsp[-1].pExpression);
        } else if ( strb->elements.size()==1 && strb->elements[0]->rtti_isStringConstant() ) {
            auto sconst = static_cast<ExprConstString*>(strb->elements[0]);
            (yyval.pExpression) = new ExprConstString(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),sconst->text);
            delete (yyvsp[-1].pExpression);
        } else {
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    }
    break;

  case 43: /* reader_character_sequence: STRING_CHARACTER  */
                               {
        if ( !yyextra->g_ReaderMacro->accept(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, (yyvsp[0].ch), tokAt(scanner,(yylsp[0]))) ) {
            das_yyend_reader(scanner);
        }
    }
    break;

  case 44: /* reader_character_sequence: reader_character_sequence STRING_CHARACTER  */
                                                                {
        if ( !yyextra->g_ReaderMacro->accept(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, (yyvsp[0].ch), tokAt(scanner,(yylsp[0]))) ) {
            das_yyend_reader(scanner);
        }
    }
    break;

  case 45: /* $@2: %empty  */
                                        {
        auto macros = yyextra->g_Program->getReaderMacro(*(yyvsp[0].s));
        if ( macros.size()==0 ) {
            das_yyerror(scanner,"reader macro " + *(yyvsp[0].s) + " not found",tokAt(scanner,(yylsp[0])),
                CompilationError::unsupported_read_macro);
        } else if ( macros.size()>1 ) {
            string options;
            for ( auto & x : macros ) {
                options += "\t" + x->module->name + "::" + x->name + "\n";
            }
            das_yyerror(scanner,"too many options for the reader macro " + *(yyvsp[0].s) +  "\n" + options, tokAt(scanner,(yylsp[0])),
                CompilationError::unsupported_read_macro);
        } else if ( yychar != '~' ) {
            das_yyerror(scanner,"expecting ~ after the reader macro", tokAt(scanner,(yylsp[0])),
                CompilationError::syntax_error);
        } else {
            yyextra->g_ReaderMacro = macros.back();
            yyextra->g_ReaderExpr = new ExprReader(tokAt(scanner,(yylsp[-1])),yyextra->g_ReaderMacro);
            yyclearin ;
            das_yybegin_reader(scanner);
        }
    }
    break;

  case 46: /* expr_reader: '%' name_in_namespace $@2 reader_character_sequence  */
                                     {
        yyextra->g_ReaderExpr->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[0]));
        (yyval.pExpression) = yyextra->g_ReaderExpr;
        int thisLine = 0;
        FileInfo * info = nullptr;
        if (!format::is_formatter_enabled()) {
        if ( auto seqt = yyextra->g_ReaderMacro->suffix(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, thisLine, info, tokAt(scanner,(yylsp[0]))) ) {
            das_accept_sequence(scanner,seqt,strlen(seqt),thisLine,info);
            yylloc.first_column = (yylsp[0]).first_column;
            yylloc.first_line = (yylsp[0]).first_line;
            yylloc.last_column = (yylsp[0]).last_column;
            yylloc.last_line = (yylsp[0]).last_line;
        }
        }
        delete (yyvsp[-2].s);
        yyextra->g_ReaderMacro = nullptr;
        yyextra->g_ReaderExpr = nullptr;
    }
    break;

  case 47: /* options_declaration: "options" annotation_argument_list  */
                                                   {
        for ( auto & opt : *(yyvsp[0].aaList) ) {
            if ( opt.name=="indenting" && opt.type==Type::tInt ) {
                if (opt.iValue != 0 && opt.iValue != 2 && opt.iValue != 4 && opt.iValue != 8) { //this is error
                    yyextra->das_tab_size = yyextra->das_def_tab_size;
                } else {
                    yyextra->das_tab_size = opt.iValue ? opt.iValue : yyextra->das_def_tab_size;//0 is default
                }
                yyextra->g_FileAccessStack.back()->tabSize = yyextra->das_tab_size;
            } else if ( opt.name=="gen2_make_syntax" && opt.type==Type::tBool ) {
                yyextra->das_gen2_make_syntax = opt.bValue;
            }
        }
        for ( auto & opt : *(yyvsp[0].aaList) ) {
            if ( yyextra->g_Access->isOptionAllowed(opt.name, yyextra->g_Program->thisModule->fileName) ) {
                if ( yyextra->g_Access->isOptionBlocked(opt.name, yyextra->g_Program->thisModule->fileName) ) {
                    // blocked: ok to write, silently ignored (not applied)
                } else {
                    yyextra->g_Program->options.push_back(opt);
                }
            } else {
                das_yyerror(scanner,"option " + opt.name + " is not allowed here",
                    tokAt(scanner,(yylsp[0])), CompilationError::invalid_option);
            }
        }
        delete (yyvsp[0].aaList);
    }
    break;

  case 49: /* keyword_or_name: "name"  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 50: /* keyword_or_name: "keyword"  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 51: /* keyword_or_name: "type function"  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 52: /* require_module_name: keyword_or_name  */
                              {
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 53: /* require_module_name: '%' require_module_name  */
                                     {
        *(yyvsp[0].s) = "%" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 54: /* require_module_name: '.' '/' require_module_name  */
                                         {
        *(yyvsp[0].s) = "./" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 55: /* require_module_name: ".." '/' require_module_name  */
                                            {
        *(yyvsp[0].s) = "../" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 56: /* require_module_name: '%' '/' require_module_name  */
                                         {
        *(yyvsp[0].s) = "%/" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 57: /* require_module_name: require_module_name '.' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += ".";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 58: /* require_module_name: require_module_name '/' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += "/";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 59: /* require_module: require_module_name is_public_module  */
                                                         {
        ast_requireModule(scanner,(yyvsp[-1].s),nullptr,(yyvsp[0].b),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 60: /* require_module: require_module_name "as" "name" is_public_module  */
                                                                              {
        ast_requireModule(scanner,(yyvsp[-3].s),(yyvsp[-1].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 61: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 62: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 66: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 67: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 68: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 69: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 70: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 71: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 72: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 73: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 74: /* expression_else: "else" expression_block  */
                                                           { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 75: /* expression_else: elif_or_static_elif expr expression_block expression_else  */
                                                                                          {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-2])), (yyvsp[-2].pExpression)->at);
        if (!format::is_else_newline() && (yyvsp[0].pExpression) != nullptr) {
            format::skip_spaces_or_print(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), yyextra->das_tab_size);
        }

        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-3].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 76: /* expression_else: elif_or_static_elif expr expression_block SEMICOLON expression_else  */
                                                                                                    {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-3])), (yyvsp[-3].pExpression)->at);
        if (!format::is_else_newline() && (yyvsp[0].pExpression) != nullptr) {
            format::skip_spaces_or_print(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-2])), tokAt(scanner,(yylsp[0])), yyextra->das_tab_size);
        }

        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-4].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 77: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 78: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 79: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 80: /* $@3: %empty  */
                      { yyextra->das_need_oxford_comma = true; }
    break;

  case 81: /* expression_else_one_liner: "else" $@3 expression_if_one_liner  */
                                                                                                 {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 82: /* expression_if_one_liner: expr  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 83: /* expression_if_one_liner: expression_return_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 84: /* expression_if_one_liner: expression_yield_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 85: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 86: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 87: /* expression_if_then_else: if_or_static_if expr expression_block SEMICOLON expression_else  */
                                                                                                {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-3])), (yyvsp[-3].pExpression)->at);
        if (!format::is_else_newline() && (yyvsp[0].pExpression) != nullptr) {
            format::skip_spaces_or_print(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-2])), tokAt(scanner,(yylsp[0])), yyextra->das_tab_size);
        }

        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-4].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 88: /* expression_if_then_else: if_or_static_if expr expression_block expression_else  */
                                                                                      {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-2])), (yyvsp[-2].pExpression)->at);
        if (!format::is_else_newline() && (yyvsp[0].pExpression) != nullptr) {
            format::skip_spaces_or_print(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), yyextra->das_tab_size);
        }

        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-3].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 89: /* $@4: %empty  */
                                                     { yyextra->das_need_oxford_comma = true; }
    break;

  case 90: /* expression_if_then_else: expression_if_one_liner "if" $@4 expr expression_else_one_liner semicolon  */
                                                                                                                                                         {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-2])), (yyvsp[-2].pExpression)->at);

        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-4])),(yyvsp[-2].pExpression),ast_wrapInBlock((yyvsp[-5].pExpression)),(yyvsp[-1].pExpression) ? ast_wrapInBlock((yyvsp[-1].pExpression)) : nullptr);
    }
    break;

  case 91: /* $@5: %empty  */
                     { yyextra->das_need_oxford_comma=false; }
    break;

  case 92: /* expression_for_loop: "for" $@5 variable_name_with_pos_list "in" expr_list expression_block  */
                                                                                                                                                 {
        format::wrap_par_expr(tokRangeAt(scanner, (yylsp[-3]), (yylsp[-1])), tokRangeAt(scanner, (yylsp[-3]), (yylsp[-1])));
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-3].pNameWithPosList),(yyvsp[-1].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 93: /* $@6: %empty  */
                     { // Had to add to successfully convert to v2 syntax, just copied from ds2_parser
                             yyextra->das_keyword = true;
     }
    break;

  case 94: /* expression_for_loop: "for" $@6 '(' variable_name_with_pos_list "in" expr_list ')' expression_block  */
                                                                                                  {
        yyextra->das_keyword = false;
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-4].pNameWithPosList),(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 95: /* expression_unsafe: "unsafe" expression_block  */
                                                 {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-1])));
        pUnsafe->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 96: /* expression_while_loop: "while" expr expression_block  */
                                                               {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-1])), (yyvsp[-1].pExpression)->at);

        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-2])));
        pWhile->cond = (yyvsp[-1].pExpression);
        pWhile->body = (yyvsp[0].pExpression);
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 97: /* expression_with: "with" expr expression_block  */
                                                         {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-1])), (yyvsp[-1].pExpression)->at);
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-2])));
        pWith->with = (yyvsp[-1].pExpression);
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 98: /* $@7: %empty  */
                                        { yyextra->das_need_oxford_comma=true; }
    break;

  case 99: /* expression_with_alias: "assume" "name" '=' $@7 expr semicolon  */
                                                                                                         {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-5])), *(yyvsp[-4].s), ExpressionPtr((yyvsp[-1].pExpression)));
        delete (yyvsp[-4].s);
    }
    break;

  case 100: /* $@8: %empty  */
                         { yyextra->das_force_oxford_comma=true;}
    break;

  case 101: /* expression_with_alias: "typedef" $@8 "name" '=' type_declaration semicolon  */
                                                                                                                   {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-5])), *(yyvsp[-3].s), TypeDeclPtr((yyvsp[-1].pTypeDecl)));
    }
    break;

  case 102: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 103: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 104: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 105: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 106: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 107: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 108: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 109: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 110: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 111: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 112: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 113: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 114: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 115: /* annotation_argument: annotation_argument_name '=' '@' '@' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[0].s); delete (yyvsp[-4].s); }
    break;

  case 116: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[-2].s); }
    break;

  case 117: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[-2].s); }
    break;

  case 118: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[-2].s); }
    break;

  case 119: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[-2].s); }
    break;

  case 120: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 121: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                               {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokRangeAt(scanner,(yylsp[-4]),(yylsp[0]))); delete (yyvsp[-4].s); }
    }
    break;

  case 122: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 123: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 124: /* metadata_argument_list: '@' annotation_argument  */
                                      {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 125: /* metadata_argument_list: metadata_argument_list '@' annotation_argument  */
                                                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 126: /* metadata_argument_list: metadata_argument_list semicolon  */
                                               {
        (yyval.aaList) = (yyvsp[-1].aaList);
    }
    break;

  case 127: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 128: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 129: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 130: /* annotation_declaration_name: "template"  */
                                    { (yyval.s) = new string("template"); }
    break;

  case 131: /* annotation_declaration_basic: annotation_declaration_name  */
                                          {
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner,(yylsp[0]));
        if ( yyextra->g_Access->isAnnotationAllowed(*(yyvsp[0].s), yyextra->g_Program->thisModuleName) ) {
            if ( auto ann = findAnnotation(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0]))) ) {
                (yyval.fa)->annotation = ann;
            }
        } else {
            das_yyerror(scanner,"annotation " + *(yyvsp[0].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[0])), CompilationError::invalid_annotation);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 132: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
                                                                                 {
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner,(yylsp[-3]));
        if ( yyextra->g_Access->isAnnotationAllowed(*(yyvsp[-3].s), yyextra->g_Program->thisModuleName) ) {
            if ( auto ann = findAnnotation(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3]))) ) {
                (yyval.fa)->annotation = ann;
            }
        } else {
            das_yyerror(scanner,"annotation " + *(yyvsp[-3].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[-3])), CompilationError::invalid_annotation);
        }
        swap ( (yyval.fa)->arguments, *(yyvsp[-1].aaList) );
        delete (yyvsp[-1].aaList);
        delete (yyvsp[-3].s);
    }
    break;

  case 133: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 134: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 135: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation); }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::And,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 136: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
                                                                            {
        if ( !(yyvsp[-2].fa)->annotation || !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation); }
        if ( !(yyvsp[0].fa)->annotation || !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Or,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 137: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation); }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Xor,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 138: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 139: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 140: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 141: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 142: /* optional_annotation_list: %empty  */
                                        { (yyval.faList) = nullptr; }
    break;

  case 143: /* optional_annotation_list: '[' annotation_list ']'  */
                                        { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 144: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 145: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 146: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 147: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 148: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 149: /* optional_function_type: "->" type_declaration  */
                                           {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 150: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 151: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 152: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 153: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 154: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 155: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 156: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 157: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 158: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 159: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 160: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 161: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 162: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 163: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 164: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 165: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 166: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 167: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 168: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 169: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 170: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 171: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 172: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 173: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 174: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 175: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 176: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 177: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 178: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 179: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 180: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 181: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 182: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 183: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 184: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 185: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 186: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 187: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 188: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 189: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 190: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 191: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 192: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 193: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 194: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 195: /* function_name: "operator" '[' ']' "<-"  */
                                    { (yyval.s) = new string("[]<-"); }
    break;

  case 196: /* function_name: "operator" '[' ']' ":="  */
                                      { (yyval.s) = new string("[]:="); }
    break;

  case 197: /* function_name: "operator" '[' ']' "+="  */
                                     { (yyval.s) = new string("[]+="); }
    break;

  case 198: /* function_name: "operator" '[' ']' "-="  */
                                     { (yyval.s) = new string("[]-="); }
    break;

  case 199: /* function_name: "operator" '[' ']' "*="  */
                                     { (yyval.s) = new string("[]*="); }
    break;

  case 200: /* function_name: "operator" '[' ']' "/="  */
                                     { (yyval.s) = new string("[]/="); }
    break;

  case 201: /* function_name: "operator" '[' ']' "%="  */
                                     { (yyval.s) = new string("[]%="); }
    break;

  case 202: /* function_name: "operator" '[' ']' "&="  */
                                     { (yyval.s) = new string("[]&="); }
    break;

  case 203: /* function_name: "operator" '[' ']' "|="  */
                                     { (yyval.s) = new string("[]|="); }
    break;

  case 204: /* function_name: "operator" '[' ']' "^="  */
                                     { (yyval.s) = new string("[]^="); }
    break;

  case 205: /* function_name: "operator" '[' ']' "&&="  */
                                        { (yyval.s) = new string("[]&&="); }
    break;

  case 206: /* function_name: "operator" '[' ']' "||="  */
                                        { (yyval.s) = new string("[]||="); }
    break;

  case 207: /* function_name: "operator" '[' ']' "^^="  */
                                        { (yyval.s) = new string("[]^^="); }
    break;

  case 208: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 209: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 210: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 211: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 212: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 213: /* function_name: "operator" '.' "name" "+="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`+="); delete (yyvsp[-1].s); }
    break;

  case 214: /* function_name: "operator" '.' "name" "-="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`-="); delete (yyvsp[-1].s); }
    break;

  case 215: /* function_name: "operator" '.' "name" "*="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`*="); delete (yyvsp[-1].s); }
    break;

  case 216: /* function_name: "operator" '.' "name" "/="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`/="); delete (yyvsp[-1].s); }
    break;

  case 217: /* function_name: "operator" '.' "name" "%="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`%="); delete (yyvsp[-1].s); }
    break;

  case 218: /* function_name: "operator" '.' "name" "&="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&="); delete (yyvsp[-1].s); }
    break;

  case 219: /* function_name: "operator" '.' "name" "|="  */
                                          { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`|="); delete (yyvsp[-1].s); }
    break;

  case 220: /* function_name: "operator" '.' "name" "^="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^="); delete (yyvsp[-1].s); }
    break;

  case 221: /* function_name: "operator" '.' "name" "&&="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&&="); delete (yyvsp[-1].s); }
    break;

  case 222: /* function_name: "operator" '.' "name" "||="  */
                                            { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`||="); delete (yyvsp[-1].s); }
    break;

  case 223: /* function_name: "operator" '.' "name" "^^="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^^="); delete (yyvsp[-1].s); }
    break;

  case 224: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 225: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 226: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 227: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 228: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 229: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 230: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 231: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 232: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 233: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 234: /* function_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 235: /* function_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 236: /* function_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 237: /* function_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 238: /* function_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 239: /* function_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 240: /* function_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 241: /* function_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 242: /* function_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 243: /* function_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 244: /* function_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 245: /* function_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 246: /* function_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 247: /* function_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 248: /* function_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 249: /* function_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 250: /* function_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 251: /* function_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 252: /* function_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 253: /* function_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 254: /* function_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 255: /* function_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 256: /* function_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 257: /* function_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 258: /* function_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 259: /* function_name: "float16"  */
                     { (yyval.s) = new string("float16"); }
    break;

  case 260: /* function_name: "half2"  */
                     { (yyval.s) = new string("half2"); }
    break;

  case 261: /* function_name: "half3"  */
                     { (yyval.s) = new string("half3"); }
    break;

  case 262: /* function_name: "half4"  */
                     { (yyval.s) = new string("half4"); }
    break;

  case 263: /* function_name: "half8"  */
                     { (yyval.s) = new string("half8"); }
    break;

  case 264: /* function_name: "short2"  */
                     { (yyval.s) = new string("short2"); }
    break;

  case 265: /* function_name: "short3"  */
                     { (yyval.s) = new string("short3"); }
    break;

  case 266: /* function_name: "short4"  */
                     { (yyval.s) = new string("short4"); }
    break;

  case 267: /* function_name: "short8"  */
                     { (yyval.s) = new string("short8"); }
    break;

  case 268: /* function_name: "ushort2"  */
                     { (yyval.s) = new string("ushort2"); }
    break;

  case 269: /* function_name: "ushort3"  */
                     { (yyval.s) = new string("ushort3"); }
    break;

  case 270: /* function_name: "ushort4"  */
                     { (yyval.s) = new string("ushort4"); }
    break;

  case 271: /* function_name: "ushort8"  */
                     { (yyval.s) = new string("ushort8"); }
    break;

  case 272: /* function_name: "byte2"  */
                     { (yyval.s) = new string("byte2"); }
    break;

  case 273: /* function_name: "byte3"  */
                     { (yyval.s) = new string("byte3"); }
    break;

  case 274: /* function_name: "byte4"  */
                     { (yyval.s) = new string("byte4"); }
    break;

  case 275: /* function_name: "byte8"  */
                     { (yyval.s) = new string("byte8"); }
    break;

  case 276: /* function_name: "byte16"  */
                     { (yyval.s) = new string("byte16"); }
    break;

  case 277: /* function_name: "ubyte2"  */
                     { (yyval.s) = new string("ubyte2"); }
    break;

  case 278: /* function_name: "ubyte3"  */
                     { (yyval.s) = new string("ubyte3"); }
    break;

  case 279: /* function_name: "ubyte4"  */
                     { (yyval.s) = new string("ubyte4"); }
    break;

  case 280: /* function_name: "ubyte8"  */
                     { (yyval.s) = new string("ubyte8"); }
    break;

  case 281: /* function_name: "ubyte16"  */
                     { (yyval.s) = new string("ubyte16"); }
    break;

  case 282: /* optional_template: %empty  */
                                        { (yyval.b) = false; }
    break;

  case 283: /* optional_template: "template"  */
                                        { (yyval.b) = true; }
    break;

  case 284: /* global_function_declaration: optional_annotation_list "def" optional_template function_declaration  */
                                                                                                              {
        (yyvsp[0].pFuncDecl)->atDecl = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
        (yyvsp[0].pFuncDecl)->isTemplate = (yyvsp[-1].b);
        assignDefaultArguments((yyvsp[0].pFuncDecl));
        runFunctionAnnotations(scanner, yyextra, (yyvsp[0].pFuncDecl), (yyvsp[-3].faList), tokAt(scanner,(yylsp[-3])));
        if ( (yyvsp[0].pFuncDecl)->isGeneric() ) {
            implAddGenericFunction(scanner,(yyvsp[0].pFuncDecl));
        } else {
            if ( !yyextra->g_Program->addFunction((yyvsp[0].pFuncDecl)) ) {
                das_yyerror(scanner,"function is already defined " +
                    (yyvsp[0].pFuncDecl)->getMangledName(),(yyvsp[0].pFuncDecl)->at,
                        CompilationError::function_already_declared);
            }
        }
        (yyvsp[0].pFuncDecl)->delRef();
    }
    break;

  case 285: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 286: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 287: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 288: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 289: /* $@9: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 290: /* function_declaration: optional_public_or_private_function $@9 function_declaration_header expression_block  */
                                                                {
        (yyvsp[-1].pFuncDecl)->body = (yyvsp[0].pExpression);
        (yyvsp[-1].pFuncDecl)->privateFunction = !(yyvsp[-3].b);
        (yyval.pFuncDecl) = (yyvsp[-1].pFuncDecl);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
        }
    }
    break;

  case 291: /* open_block: "begin of code block"  */
          { (yyval.ui) = 0xdeadbeef; }
    break;

  case 292: /* open_block: "new scope"  */
                        { (yyval.ui) = (yyvsp[0].i); }
    break;

  case 293: /* close_block: "end of code block"  */
          { (yyval.ui) = 0xdeadbeef; }
    break;

  case 294: /* close_block: "close scope"  */
                         { (yyval.ui) = (yyvsp[0].i); }
    break;

  case 295: /* expression_block: open_block expressions close_block  */
                                                                  {
        auto prev_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(prev_loc, (yyvsp[-2].ui), format::get_substring(prev_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size, format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
    }
    break;

  case 296: /* expression_block: open_block expressions close_block "finally" open_block expressions close_block  */
                                                                                                                                  {
        auto prev_loc = format::Pos::from(tokAt(scanner,(yylsp[-6])));
        if (format::is_replace_braces() && (yyvsp[-6].ui) != 0xdeadbeef && format::prepare_rule(prev_loc)) {
            handle_brace(prev_loc, (yyvsp[-6].ui), format::get_substring(prev_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-5])))),
                         yyextra->das_tab_size, format::Pos::from_last(tokAt(scanner,(yylsp[-5]))));
            auto prev_loc_f = format::Pos::from(tokAt(scanner,(yylsp[-2])));
            assert((yyvsp[-2].ui) != 0xdeadbeef);
            {
                const auto internal_f = format::get_substring(prev_loc_f, format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
                format::get_writer() << " finally {" << internal_f << "\n" << string((yyvsp[-2].ui) * yyextra->das_tab_size, ' ') + "}";
                format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
            }
        }

        auto pB = (ExprBlock *) (yyvsp[-5].pExpression);
        auto pF = (ExprBlock *) (yyvsp[-1].pExpression);
        swap ( pB->finalList, pF->list );
        (yyval.pExpression) = (yyvsp[-5].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-6]),(yylsp[0]));
        delete (yyvsp[-1].pExpression);
    }
    break;

  case 297: /* expr_call_pipe: expr expr_full_block_assumed_piped  */
                                                      {
        if ( (yyvsp[-1].pExpression)->rtti_isCallLikeExpr() ) {
            auto start = format::Pos::from_last(tokAt(scanner, (yylsp[-1])));
            start.column -= 1; // drop )
            if (format::is_replace_braces() && format::prepare_rule(start)) {
                if (!((ExprLooksLikeCall *)(yyvsp[-1].pExpression))->arguments.empty()) {
                    format::get_writer() << ", ";
                }
                format::get_writer() << format::get_substring(tokAt(scanner, (yylsp[0]))) << ");";
                format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[0]))));
            }
            ((ExprLooksLikeCall *)(yyvsp[-1].pExpression))->arguments.push_back((yyvsp[0].pExpression));
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else {
            (yyval.pExpression) = (yyvsp[-1].pExpression);
            delete (yyvsp[0].pExpression);
        }
    }
    break;

  case 298: /* expr_call_pipe: expression_keyword expr_full_block_assumed_piped  */
                                                                    {
        if ( (yyvsp[-1].pExpression)->rtti_isCallLikeExpr() ) {
            ((ExprLooksLikeCall *)(yyvsp[-1].pExpression))->arguments.push_back((yyvsp[0].pExpression));
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else {
            (yyval.pExpression) = (yyvsp[-1].pExpression);
            delete (yyvsp[0].pExpression);
        }
    }
    break;

  case 299: /* expr_call_pipe: "generator" '<' type_declaration_no_options '>' optional_capture_list expr_full_block_assumed_piped  */
                                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-3].pTypeDecl),(yyvsp[-1].pCaptList),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 300: /* expression_any: SEMICOLON  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 301: /* expression_any: "end of expression"  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 302: /* expression_any: expr_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 303: /* expression_any: expr_keyword  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 304: /* expression_any: expr_assign_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 305: /* expression_any: expr_assign semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 306: /* expression_any: expression_delete semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 307: /* expression_any: expression_let  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 308: /* expression_any: expression_while_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 309: /* expression_any: expression_unsafe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 310: /* expression_any: expression_with  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 311: /* expression_any: expression_with_alias  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 312: /* expression_any: expression_for_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 313: /* expression_any: expression_break semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 314: /* expression_any: expression_continue semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 315: /* expression_any: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 316: /* expression_any: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 317: /* expression_any: expression_if_then_else  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 318: /* expression_any: expression_try_catch  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 319: /* expression_any: expression_label semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 320: /* expression_any: expression_goto semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 321: /* expression_any: "pass" semicolon  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 322: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 323: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 324: /* expressions: expressions error  */
                                 {
        delete (yyvsp[-1].pExpression); (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 325: /* expr_keyword: "keyword" expr expression_block  */
                                                           {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-1])), (yyvsp[-1].pExpression)->at); // wrap match (expr)
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s));
        pCall->arguments.push_back((yyvsp[-1].pExpression));
        auto resT = new TypeDecl(Type::autoinfer);
        auto blk = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,resT,(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),LineInfo());
        pCall->arguments.push_back(blk);
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 326: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 327: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 328: /* optional_expr_list_in_braces: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 329: /* optional_expr_list_in_braces: '(' optional_expr_list optional_comma ')'  */
                                                             { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 330: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 331: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 332: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 333: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 334: /* $@10: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 335: /* $@11: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 336: /* expression_keyword: "keyword" '<' $@10 type_declaration_no_options_list '>' $@11 expr  */
                                                                                                                                                     {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 337: /* $@12: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 338: /* $@13: %empty  */
                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 339: /* expression_keyword: "type function" '<' $@12 type_declaration_no_options_list '>' $@13 optional_expr_list_in_braces  */
                                                                                                                                                                                   {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 340: /* expr_pipe: expr_assign " <|" expr_block  */
                                                        {
        Expression * pipeCall = (yyvsp[-2].pExpression)->tail();
        if ( pipeCall->rtti_isCallLikeExpr() ) {
            auto pCall = (ExprLooksLikeCall *) pipeCall;
            pCall->arguments.push_back((yyvsp[0].pExpression));
            (yyval.pExpression) = (yyvsp[-2].pExpression);
        } else if ( pipeCall->rtti_isVar() ) {
            // a += b <| c
            auto pVar = (ExprVar *) pipeCall;
            auto pCall = yyextra->g_Program->makeCall(pVar->at,pVar->name);
            pCall->arguments.push_back((yyvsp[0].pExpression));
            if ( !(yyvsp[-2].pExpression)->swap_tail(pVar,pCall) ) {
                delete pVar;
                (yyval.pExpression) = pCall;
            } else {
                (yyval.pExpression) = (yyvsp[-2].pExpression);
            }
        } else if ( pipeCall->rtti_isMakeStruct() ) {
            auto pMS = (ExprMakeStruct *) pipeCall;
            if ( pMS->block ) {
                das_yyerror(scanner,"can't pipe into [[ make structure ]]. it already has where closure",
                    tokAt(scanner,(yylsp[-1])),CompilationError::cant_pipe);
                delete (yyvsp[0].pExpression);
            } else {
                pMS->block = (yyvsp[0].pExpression);
            }
            (yyval.pExpression) = (yyvsp[-2].pExpression);
        } else {
            das_yyerror(scanner,"can only pipe into function call or [[ make structure ]]",
                tokAt(scanner,(yylsp[-1])),CompilationError::cant_pipe);
            delete (yyvsp[0].pExpression);
            (yyval.pExpression) = (yyvsp[-2].pExpression);
        }
    }
    break;

  case 341: /* expr_pipe: "@ <|" expr_block  */
                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
            format::get_writer() << "@";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[-1]))));
        }

        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 342: /* expr_pipe: "@@ <|" expr_block  */
                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
            format::get_writer() << "@@";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[-1]))));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 343: /* expr_pipe: "$ <|" expr_block  */
                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
            format::get_writer() << "$";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[-1]))));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 344: /* expr_pipe: expr_call_pipe  */
                             {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 345: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 346: /* name_in_namespace: "name" "::" "name"  */
                                               {
            auto ita = yyextra->das_module_alias.find(*(yyvsp[-2].s));
            if ( ita == yyextra->das_module_alias.end() ) {
                *(yyvsp[-2].s) += "::";
            } else {
                *(yyvsp[-2].s) = ita->second + "::";
            }
            *(yyvsp[-2].s) += *(yyvsp[0].s);
            delete (yyvsp[0].s);
            (yyval.s) = (yyvsp[-2].s);
        }
    break;

  case 347: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 348: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 349: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 350: /* $@14: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 351: /* $@15: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 352: /* new_type_declaration: '<' $@14 type_declaration '>' $@15  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 353: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 354: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 355: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 356: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 357: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 358: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 359: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 360: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 361: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 362: /* expression_return_no_pipe: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 363: /* expression_return_no_pipe: "return" expr_list  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),sequenceToTuple((yyvsp[0].pExpression)));
    }
    break;

  case 364: /* expression_return_no_pipe: "return" "<-" expr_list  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),sequenceToTuple((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 365: /* expression_return: expression_return_no_pipe semicolon  */
                                                    {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 366: /* expression_return: "return" expr_pipe  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 367: /* expression_return: "return" "<-" expr_pipe  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 368: /* expression_yield_no_pipe: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 369: /* expression_yield_no_pipe: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 370: /* expression_yield: expression_yield_no_pipe semicolon  */
                                                   {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 371: /* expression_yield: "yield" expr_pipe  */
                                          {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 372: /* expression_yield: "yield" "<-" expr_pipe  */
                                                 {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 373: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                              {
        const auto end_block = format::Pos::from_last(tokAt(scanner, (yylsp[-2])));
        const auto start = format::Pos::from(tokAt(scanner, (yylsp[-3])));
        if (format::is_replace_braces()) {
            format::skip_spaces_or_print(tokAt(scanner, (yylsp[-3])), tokAt(scanner, (yylsp[-2])), tokAt(scanner, (yylsp[-1])), yyextra->das_tab_size);
        }

        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 374: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 375: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 376: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 377: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 378: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 379: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 380: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 381: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 382: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 383: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                                                            {
        // std::cout << "case11" << std::endl;
        format::replace_with(false,
                             format::Pos::from(tokAt(scanner,(yylsp[-8]))),
                             format::substring_between(tokAt(scanner, (yylsp[-8])), tokAt(scanner, (yylsp[-6]))),
                             format::Pos::from_last(tokAt(scanner,(yylsp[-5]))), "(", ")");
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-7].pNameList),tokAt(scanner,(yylsp[-7])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 384: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 385: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr semicolon  */
                                                                                                                                                {
        // std::cout << "case12" << std::endl;
        format::replace_with(false,
                              format::Pos::from(tokAt(scanner,(yylsp[-7]))),
                              format::substring_between(tokAt(scanner, (yylsp[-7])), tokAt(scanner, (yylsp[-5]))),
                              format::Pos::from_last(tokAt(scanner,(yylsp[-4]))), "(", ")");

        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-6]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 386: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr semicolon  */
                                                                                                        {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-5]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameList),tokAt(scanner,(yylsp[-5])),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 387: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 388: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 389: /* $@16: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 390: /* $@17: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 391: /* expr_cast: "cast" '<' $@16 type_declaration_no_options '>' $@17 expr  */
                                                                                                                                                {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
    }
    break;

  case 392: /* $@18: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 393: /* $@19: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 394: /* expr_cast: "upcast" '<' $@18 type_declaration_no_options '>' $@19 expr  */
                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 395: /* $@20: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 396: /* $@21: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 397: /* expr_cast: "reinterpret" '<' $@20 type_declaration_no_options '>' $@21 expr  */
                                                                                                                                                       {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 398: /* $@22: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 399: /* $@23: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 400: /* expr_type_decl: "type" '<' $@22 type_declaration '>' $@23  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 401: /* expr_type_info: "typeinfo" '(' name_in_namespace expr ')'  */
                                                                                {
            format::replace_with(false,
                                 format::Pos::from(tokAt(scanner,(yylsp[-3]))),
                                 format::get_substring(tokAt(scanner,(yylsp[-2]))),
                                 format::Pos::from(tokAt(scanner,(yylsp[-1]))), " ", "(");
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-2].s),ptd->typeexpr);
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-2].s),(yyvsp[-1].pExpression));
            }
            delete (yyvsp[-2].s);
    }
    break;

  case 402: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" '>' expr ')'  */
                                                                                                             {
            format::replace_with(false,
                                 format::Pos::from(tokAt(scanner,(yylsp[-6]))),
                                 format::get_substring(tokRangeAt(scanner,(yylsp[-5]),(yylsp[-2]))),
                                 format::Pos::from(tokAt(scanner,(yylsp[-1]))), " ", "(");

            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-5].s),ptd->typeexpr,*(yyvsp[-3].s));
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-5].s),(yyvsp[-1].pExpression),*(yyvsp[-3].s));
            }
            delete (yyvsp[-5].s);
            delete (yyvsp[-3].s);
    }
    break;

  case 403: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" c_or_s "name" '>' expr ')'  */
                                                                                                                                    {
            format::replace_with(false,
                                 format::Pos::from(tokAt(scanner,(yylsp[-8]))),
                                 format::get_substring(tokRangeAt(scanner,(yylsp[-7]),(yylsp[-2]))),
                                 format::Pos::from(tokAt(scanner,(yylsp[-1]))), " ", "(");

            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-7].s),ptd->typeexpr,*(yyvsp[-5].s),*(yyvsp[-3].s));
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-7].s),(yyvsp[-1].pExpression),*(yyvsp[-5].s),*(yyvsp[-3].s));
            }
            delete (yyvsp[-7].s);
            delete (yyvsp[-5].s);
            delete (yyvsp[-3].s);
    }
    break;

  case 404: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
                                                                          {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-3].s),ptd->typeexpr);
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-3].s),(yyvsp[-1].pExpression));
            }
            delete (yyvsp[-3].s);
    }
    break;

  case 405: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
                                                                                                {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-6].s),ptd->typeexpr,*(yyvsp[-4].s));
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-6].s),(yyvsp[-1].pExpression),*(yyvsp[-4].s));
            }
            delete (yyvsp[-6].s);
            delete (yyvsp[-4].s);
    }
    break;

  case 406: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" semicolon "name" '>' '(' expr ')'  */
                                                                                                                           {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-8].s),ptd->typeexpr,*(yyvsp[-6].s),*(yyvsp[-4].s));
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-8].s),(yyvsp[-1].pExpression),*(yyvsp[-6].s),*(yyvsp[-4].s));
            }
            delete (yyvsp[-8].s);
            delete (yyvsp[-6].s);
            delete (yyvsp[-4].s);
    }
    break;

  case 407: /* expr_list: expr2  */
                       {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 408: /* expr_list: expr_list ',' expr2  */
                                             {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 409: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 410: /* block_or_simple_block: "=>" expr  */
                                        {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 411: /* block_or_simple_block: "=>" "<-" expr  */
                                               {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 412: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 413: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 414: /* block_or_lambda: '@' '@'  */
                { (yyval.i) = 2;   /* local function */ }
    break;

  case 415: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 416: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 417: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 418: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 419: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 420: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 421: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 422: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 423: /* optional_capture_list: "[[" capture_list ']' ']'  */
                                                              {
        if (format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-3]))))) {
            format::get_writer()
                      << "capture("
                      << format::substring_between(tokAt(scanner, (yylsp[-3])), tokAt(scanner, (yylsp[-2])))
                      << format::get_substring(tokAt(scanner, (yylsp[-2])))
                      << format::substring_between(tokAt(scanner, (yylsp[-2])), tokAt(scanner, (yylsp[-1])))
                      << ")";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[0]))));
        }

         ; (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 424: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 425: /* expr_block: expression_block  */
                                            {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))))) {
            format::get_writer() << "$() ";
            format::finish_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))));
        }

        ExprBlock * closure = (ExprBlock *) (yyvsp[0].pExpression);
        (yyval.pExpression) = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),(yyvsp[0].pExpression));
        closure->returnType = new TypeDecl(Type::autoinfer);
    }
    break;

  case 426: /* expr_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 427: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 428: /* $@24: %empty  */
                             {  yyextra->das_need_oxford_comma = false; }
    break;

  case 429: /* expr_full_block_assumed_piped: block_or_lambda $@24 optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type expression_block  */
                                                                                       {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 430: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 431: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 432: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 433: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 434: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 435: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 436: /* expr_numeric_const: "float16 constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat16(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 437: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 438: /* expr_assign: expr  */
                                             { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 439: /* expr_assign: expr '=' expr  */
                                             { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 440: /* expr_assign: expr "<-" expr  */
                                             { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 441: /* expr_assign: expr ":=" expr  */
                                             { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 442: /* expr_assign: expr "&=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 443: /* expr_assign: expr "|=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 444: /* expr_assign: expr "^=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 445: /* expr_assign: expr "&&=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 446: /* expr_assign: expr "||=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 447: /* expr_assign: expr "^^=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 448: /* expr_assign: expr "+=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 449: /* expr_assign: expr "-=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 450: /* expr_assign: expr "*=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 451: /* expr_assign: expr "/=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 452: /* expr_assign: expr "%=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 453: /* expr_assign: expr "<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 454: /* expr_assign: expr ">>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 455: /* expr_assign: expr "<<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 456: /* expr_assign: expr ">>>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 457: /* expr_assign_pipe_right: "@ <|" expr_block  */
                                         {
            if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
                auto tok = tokAt(scanner, (yylsp[0]));
                tok.column += 1;
                format::get_writer() << "@" << format::get_substring(tok) << ";";
                format::finish_rule(format::Pos::from_last(tok));
            }

            (yyval.pExpression) = (yyvsp[0].pExpression);
        }
    break;

  case 458: /* expr_assign_pipe_right: "@@ <|" expr_block  */
                                         {
            if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
                auto tok = tokAt(scanner, (yylsp[0]));
                tok.column += 1;
                format::get_writer() << "@@" << format::get_substring(tok) << ";";
                format::finish_rule(format::Pos::from_last(tok));
            }
            (yyval.pExpression) = (yyvsp[0].pExpression);
        }
    break;

  case 459: /* expr_assign_pipe_right: "$ <|" expr_block  */
                                         {
            if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
                auto tok = tokAt(scanner, (yylsp[0]));
                tok.column += 1;
                format::get_writer() << "$" << format::get_substring(tok) << ";";
                format::finish_rule(format::Pos::from_last(tok));
            }
            (yyval.pExpression) = (yyvsp[0].pExpression);
        }
    break;

  case 460: /* expr_assign_pipe_right: expr_call_pipe  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 461: /* expr_assign_pipe: expr '=' expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 462: /* expr_assign_pipe: expr "<-" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 463: /* expr_assign_pipe: expr "&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 464: /* expr_assign_pipe: expr "|=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 465: /* expr_assign_pipe: expr "^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 466: /* expr_assign_pipe: expr "&&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 467: /* expr_assign_pipe: expr "||=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 468: /* expr_assign_pipe: expr "^^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 469: /* expr_assign_pipe: expr "+=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 470: /* expr_assign_pipe: expr "-=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 471: /* expr_assign_pipe: expr "*=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 472: /* expr_assign_pipe: expr "/=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 473: /* expr_assign_pipe: expr "%=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 474: /* expr_assign_pipe: expr "<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 475: /* expr_assign_pipe: expr ">>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 476: /* expr_assign_pipe: expr "<<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 477: /* expr_assign_pipe: expr ">>>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 478: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 479: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 480: /* expr_method_call: expr2 "->" "name" '(' ')'  */
                                                          {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 481: /* expr_method_call: expr2 "->" "name" '(' expr_list ')'  */
                                                                               {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 482: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 483: /* func_addr_name: "$i" '(' expr2 ')'  */
                                           {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 484: /* func_addr_expr: '@' '@' func_addr_name  */
                                          {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 485: /* $@25: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 486: /* $@26: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 487: /* func_addr_expr: '@' '@' '<' $@25 type_declaration_no_options '>' $@26 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 488: /* $@27: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 489: /* $@28: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 490: /* func_addr_expr: '@' '@' '<' $@27 optional_function_argument_list optional_function_type '>' $@28 func_addr_name  */
                                                                                                                                                                                     {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value));
        expr->funcType = new TypeDecl(Type::tFunction);
        expr->funcType->firstType = (yyvsp[-3].pTypeDecl);
        if ( (yyvsp[-4].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, expr->funcType, (yyvsp[-4].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-4].pVarDeclList));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 491: /* expr_field: expr2 '.' "name"  */
                                               {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 492: /* expr_field: expr2 '.' '.' "name"  */
                                                   {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 493: /* expr_field: expr2 '.' "name" '(' ')'  */
                                                       {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 494: /* expr_field: expr2 '.' "name" '(' expr_list ')'  */
                                                                            {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 495: /* expr_field: expr2 '.' "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                        {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 496: /* expr_field: expr2 '.' basic_type_declaration '(' ')'  */
                                                                         {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 497: /* expr_field: expr2 '.' basic_type_declaration '(' expr_list ')'  */
                                                                                              {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 498: /* $@29: %empty  */
                                { yyextra->das_suppress_errors=true; }
    break;

  case 499: /* $@30: %empty  */
                                                                             { yyextra->das_suppress_errors=false; }
    break;

  case 500: /* expr_field: expr2 '.' $@29 error $@30  */
                                                                                                                     {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), "");
        yyerrok;
    }
    break;

  case 501: /* expr_call: name_in_namespace '(' ')'  */
                                               {
            (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),*(yyvsp[-2].s));
            delete (yyvsp[-2].s);
    }
    break;

  case 502: /* expr_call: name_in_namespace '(' "uninitialized" ')'  */
                                                          {
            auto dd = new ExprMakeStruct(tokAt(scanner,(yylsp[-3])));
            dd->at = tokAt(scanner,(yylsp[-3]));
            dd->makeType = new TypeDecl(Type::alias);
            dd->makeType->alias = *(yyvsp[-3].s);
            dd->useInitializer = false;
            dd->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = dd;
    }
    break;

  case 503: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
                                                               {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = new TypeDecl(Type::alias);
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType->alias = *(yyvsp[-3].s);
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 504: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
                                                                                 {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = new TypeDecl(Type::alias);
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType->alias = *(yyvsp[-4].s);
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-4].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 505: /* expr_call: name_in_namespace '(' expr_list ')'  */
                                                                    {
            (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),*(yyvsp[-3].s)),(yyvsp[-1].pExpression));
            delete (yyvsp[-3].s);
    }
    break;

  case 506: /* expr_call: basic_type_declaration '(' ')'  */
                                                    {
        (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-2].type)));
    }
    break;

  case 507: /* expr_call: basic_type_declaration '(' expr_list ')'  */
                                                                         {
        (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-3].type))),(yyvsp[-1].pExpression));
    }
    break;

  case 508: /* expr2: name_in_namespace  */
                                              { need_wrap_current_expr = true; (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 509: /* expr2: expr_field  */
                                              { need_wrap_current_expr = true; (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0])); (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 510: /* expr2: expr_mtag  */
                                              { need_wrap_current_expr = true; (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0])); (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 511: /* expr2: '!' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"!",(yyvsp[0].pExpression)); }
    break;

  case 512: /* expr2: '~' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"~",(yyvsp[0].pExpression)); }
    break;

  case 513: /* expr2: '+' expr2  */
                                                   { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"+",(yyvsp[0].pExpression)); }
    break;

  case 514: /* expr2: '-' expr2  */
                                                   { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"-",(yyvsp[0].pExpression)); }
    break;

  case 515: /* expr2: expr2 "<<" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 516: /* expr2: expr2 ">>" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 517: /* expr2: expr2 "<<<" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 518: /* expr2: expr2 ">>>" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 519: /* expr2: expr2 '+' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 520: /* expr2: expr2 '-' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 521: /* expr2: expr2 '*' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 522: /* expr2: expr2 '/' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 523: /* expr2: expr2 '%' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 524: /* expr2: expr2 '<' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 525: /* expr2: expr2 '>' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 526: /* expr2: expr2 "==" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 527: /* expr2: expr2 "!=" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 528: /* expr2: expr2 "<=" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 529: /* expr2: expr2 ">=" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 530: /* expr2: expr2 '&' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 531: /* expr2: expr2 '|' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 532: /* expr2: expr2 '^' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 533: /* expr2: expr2 "&&" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 534: /* expr2: expr2 "||" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 535: /* expr2: expr2 "^^" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 536: /* expr2: expr2 ".." expr2  */
                                               {
        need_wrap_current_expr = true;
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 537: /* expr2: "++" expr2  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"++", (yyvsp[0].pExpression)); }
    break;

  case 538: /* expr2: "--" expr2  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"--", (yyvsp[0].pExpression)); }
    break;

  case 539: /* expr2: expr2 "++"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 540: /* expr2: expr2 "--"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 541: /* expr2: expr2 '[' expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprAt(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 542: /* expr2: expr2 '.' '[' expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprAt(tokRangeAt(scanner,(yylsp[-4]), (yylsp[0])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 543: /* expr2: expr2 "?[" expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeAt(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 544: /* expr2: expr2 '.' "?[" expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeAt(tokRangeAt(scanner,(yylsp[-4]), (yylsp[0])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 545: /* expr2: expr2 "?." "name"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeField(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 546: /* expr2: expr2 '.' "?." "name"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeField(tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 547: /* expr2: '*' expr2  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 548: /* expr2: expr2 '?' expr2 ':' expr2  */
                                                             {
        need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp3(tokRangeAt(scanner,(yylsp[-4]), (yylsp[0])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 549: /* expr2: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 550: /* expr2: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 551: /* expr2: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 552: /* expr2: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 553: /* expr2: make_decl  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 554: /* expr2: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 555: /* expr2: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 556: /* expr2: '(' expr_list optional_comma ')'  */
                                                         {
            if ( (yyvsp[-2].pExpression)->rtti_isSequence() ) {
                auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-2])));
                mkt->values = sequenceToList((yyvsp[-2].pExpression));
                mkt->shorthandRecordNames = ast_tupleCollectShorthandNames(mkt->values);
                (yyval.pExpression) = mkt;
            } else if ( (yyvsp[-1].b) ) {
                auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-2])));
                mkt->values.push_back((yyvsp[-2].pExpression));
                mkt->shorthandRecordNames = ast_tupleCollectShorthandNames(mkt->values);
                (yyval.pExpression) = mkt;
            } else {
                (yyvsp[-2].pExpression)->at = tokAt(scanner, (yylsp[-2]));
                (yyval.pExpression) = (yyvsp[-2].pExpression);
            }
        }
    break;

  case 557: /* expr2: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 558: /* expr2: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 559: /* expr2: "deref" '(' expr2 ')'  */
                                                    { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 560: /* expr2: "addr" '(' expr2 ')'  */
                                                    { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 561: /* expr2: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 562: /* expr2: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr2 ')'  */
                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 563: /* expr2: expr2 "??" expr2  */
                                                     { (yyval.pExpression) = new ExprNullCoalescing(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 564: /* $@31: %empty  */
                                                { yyextra->das_arrow_depth ++; }
    break;

  case 565: /* $@32: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 566: /* expr2: expr2 "is" "type" '<' $@31 type_declaration_no_options '>' $@32  */
                                                                                                                                                             {
        (yyval.pExpression) = new ExprIs(tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 567: /* expr2: expr2 "is" basic_type_declaration  */
                                                                {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 568: /* expr2: expr2 "is" "name"  */
                                               {
        (yyval.pExpression) = new ExprIsVariant(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 569: /* expr2: expr2 "as" "name"  */
                                               {
        (yyval.pExpression) = new ExprAsVariant(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 570: /* $@33: %empty  */
                                                { yyextra->das_arrow_depth ++; }
    break;

  case 571: /* $@34: %empty  */
                                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 572: /* expr2: expr2 "as" "type" '<' $@33 type_declaration '>' $@34  */
                                                                                                                                                  {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 573: /* expr2: expr2 "as" basic_type_declaration  */
                                                                {
        (yyval.pExpression) = new ExprAsVariant(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 574: /* expr2: expr2 '?' "as" "name"  */
                                                   {
        (yyval.pExpression) = new ExprSafeAsVariant(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 575: /* $@35: %empty  */
                                                    { yyextra->das_arrow_depth ++; }
    break;

  case 576: /* $@36: %empty  */
                                                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 577: /* expr2: expr2 '?' "as" "type" '<' $@35 type_declaration '>' $@36  */
                                                                                                                                                      {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokRangeAt(scanner,(yylsp[-8]), (yylsp[-1])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 578: /* expr2: expr2 '?' "as" basic_type_declaration  */
                                                                    {
        (yyval.pExpression) = new ExprSafeAsVariant(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 579: /* expr2: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 580: /* expr2: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 581: /* expr2: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 582: /* expr2: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 583: /* expr2: expr_method_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); (yyval.pExpression)->at = tokAt(scanner, (yylsp[0])); }
    break;

  case 584: /* expr2: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 585: /* expr2: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 586: /* expr2: expr2 "<|" expr2  */
                                                  { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); }
    break;

  case 587: /* expr2: expr2 "|>" expr2  */
                                                  { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-2]), (yylsp[0]))); (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])); }
    break;

  case 588: /* expr2: expr2 "|>" basic_type_declaration  */
                                                           {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])));
    }
    break;

  case 589: /* expr2: name_in_namespace "name"  */
                                                  {
        if (format::prepare_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-1]))))) {
            format::get_writer() << "." << format::get_substring(tokAt(scanner,(yylsp[0])));
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        (yyval.pExpression) = ast_NameName(scanner,(yyvsp[-1].s),(yyvsp[0].s),tokRangeAt(scanner,(yylsp[-1]),(yylsp[0])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 590: /* expr2: "unsafe" '(' expr2 ')'  */
                                          {
        (yyvsp[-1].pExpression)->alwaysSafe = true;
        (yyvsp[-1].pExpression)->userSaidItsSafe = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 591: /* expr2: expression_keyword  */
                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 592: /* expr: expr2  */
                                       {
        if (need_wrap_current_expr) {
            format::wrap_par_expr_newline(tokAt(scanner,(yylsp[0])), (yyvsp[0].pExpression)->at);
            need_wrap_current_expr = false;
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 593: /* expr_mtag: "$$" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 594: /* expr_mtag: "$i" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 595: /* expr_mtag: "$v" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 596: /* expr_mtag: "$b" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 597: /* expr_mtag: "$a" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 598: /* expr_mtag: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 599: /* expr_mtag: "$c" '(' expr2 ')' '(' ')'  */
                                                             {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 600: /* expr_mtag: "$c" '(' expr2 ')' '(' expr_list ')'  */
                                                                                 {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 601: /* expr_mtag: expr2 '.' "$f" '(' expr2 ')'  */
                                                                  {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 602: /* expr_mtag: expr2 "?." "$f" '(' expr2 ')'  */
                                                                   {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 603: /* expr_mtag: expr2 '.' '.' "$f" '(' expr2 ')'  */
                                                                      {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 604: /* expr_mtag: expr2 '.' "?." "$f" '(' expr2 ')'  */
                                                                       {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 605: /* expr_mtag: expr2 "as" "$f" '(' expr2 ')'  */
                                                                     {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 606: /* expr_mtag: expr2 '?' "as" "$f" '(' expr2 ')'  */
                                                                         {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 607: /* expr_mtag: expr2 "is" "$f" '(' expr2 ')'  */
                                                                     {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 608: /* expr_mtag: '@' '@' "$c" '(' expr2 ')'  */
                                                          {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 609: /* optional_field_annotation: %empty  */
                                                        { (yyval.aaList) = nullptr; }
    break;

  case 610: /* optional_field_annotation: "[[" annotation_argument_list ']' ']'  */
                                                                     {
        if (format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-3]))))) {
            for (const auto &arg: *(yyvsp[-2].aaList)) {
                format::get_writer() << "@" << format::get_substring(arg.at);
            }
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[0]))));
        }
        (yyval.aaList) = (yyvsp[-2].aaList); /*this one is gone when BRABRA is disabled*/
    }
    break;

  case 611: /* optional_field_annotation: metadata_argument_list  */
                                                        { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 612: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 613: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 614: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 615: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 616: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 617: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 618: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 619: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 620: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 621: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 622: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 623: /* opt_sem: SEMICOLON  */
                { (yyval.b) = false; }
    break;

  case 624: /* opt_sem: "end of expression"  */
          { (yyval.b) = true; }
    break;

  case 625: /* opt_sem: "end of expression" SEMICOLON  */
                    { (yyval.b) = true; }
    break;

  case 626: /* opt_sem: %empty  */
                  {(yyval.b) = false; }
    break;

  case 627: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 628: /* $@37: %empty  */
                                                               { yyextra->das_force_oxford_comma=true;}
    break;

  case 629: /* struct_variable_declaration_list: struct_variable_declaration_list "typedef" $@37 "name" '=' type_declaration semicolon opt_sem  */
                                                                                                                                                                 {
        (yyval.pVarDeclList) = (yyvsp[-7].pVarDeclList);
        ast_structureAlias(scanner,(yyvsp[-4].s),(yyvsp[-2].pTypeDecl),tokAt(scanner,(yylsp[-6])));
    }
    break;

  case 630: /* $@38: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 631: /* struct_variable_declaration_list: struct_variable_declaration_list $@38 structure_variable_declaration semicolon opt_sem  */
                                                             {
        (yyval.pVarDeclList) = (yyvsp[-4].pVarDeclList);
        if ( (yyvsp[-2].pVarDecl) ) (yyvsp[-4].pVarDeclList)->push_back((yyvsp[-2].pVarDecl));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *((yyvsp[-2].pVarDecl)->pNameList) ) {
                    crd->afterStructureField(nl.name.c_str(), nl.at);
                }
            }
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterStructureFields(tak);
        }
    }
    break;

  case 632: /* $@39: %empty  */
                                                                                                                     {
                yyextra->das_force_oxford_comma=true;
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 633: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable "abstract" optional_constant $@39 function_declaration_header semicolon opt_sem  */
                                                                  {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure->isTemplate;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-9].pVarDeclList),(yyvsp[-8].faList),(yyvsp[-6].b),(yyvsp[-4].b), (yyvsp[-2].pFuncDecl));
            }
    break;

  case 634: /* $@40: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 635: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@40 function_declaration_header expression_block opt_sem  */
                                                                                {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure->isTemplate;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-10].pVarDeclList),(yyvsp[-9].faList),(yyvsp[-6].b),(yyvsp[-7].b),(yyvsp[-5].i),(yyvsp[-4].b),(yyvsp[-2].pFuncDecl),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-8]),(yylsp[-1])),tokAt(scanner,(yylsp[-9])));
            }
    break;

  case 636: /* struct_variable_declaration_list: struct_variable_declaration_list '[' annotation_list ']' semicolon opt_sem  */
                                                                                               {
        das_yyerror(scanner,"structure field or class method annotation expected to remain on the same line with the field or the class",
            tokAt(scanner,(yylsp[-3])), CompilationError::syntax_error);
        delete (yyvsp[-3].faList);
        (yyval.pVarDeclList) = (yyvsp[-5].pVarDeclList);
    }
    break;

  case 637: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
                                                                                                          {
            (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
            if ( (yyvsp[-1].b) ) {
                (yyvsp[0].pVarDecl)->pTypeDecl->constant = true;
            } else {
                (yyvsp[0].pVarDecl)->pTypeDecl->removeConstant = true;
            }
            (yyvsp[0].pVarDecl)->annotation = (yyvsp[-2].aaList);
        }
    break;

  case 638: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
                                                                                                       {
            (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
            if ( (yyvsp[-1].b) ) {
                (yyvsp[0].pVarDecl)->pTypeDecl->constant = true;
            } else {
                (yyvsp[0].pVarDecl)->pTypeDecl->removeConstant = true;
            }
            (yyvsp[0].pVarDecl)->annotation = (yyvsp[-2].aaList);
        }
    break;

  case 639: /* function_argument_declaration_type: "$a" '(' expr2 ')'  */
                                      {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 640: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 641: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 642: /* function_argument_list: function_argument_declaration_no_type "end of expression" function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 643: /* function_argument_list: function_argument_declaration_type "end of expression" function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 644: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 645: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 646: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 647: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 648: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                          { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 649: /* tuple_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 650: /* tuple_alias_type_list: tuple_alias_type_list c_or_s  */
                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 651: /* tuple_alias_type_list: tuple_alias_type_list tuple_type c_or_s  */
                                                            {
        (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[-1].pVarDecl));
        /*
        if ( !yyextra->g_CommentReaders.empty() ) {
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *($decl->pNameList) ) {
                    crd->afterVariantEntry(nl.name.c_str(), nl.at);
                }
            }
        }
        */
    }
    break;

  case 652: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 653: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 654: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 655: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 656: /* variant_alias_type_list: variant_alias_type_list c_or_s  */
                                           {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 657: /* variant_alias_type_list: variant_alias_type_list variant_type c_or_s  */
                                                                {
        (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[-1].pVarDecl));
        if ( !yyextra->g_CommentReaders.empty() ) {
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *((yyvsp[-1].pVarDecl)->pNameList) ) {
                    crd->afterVariantEntry(nl.name.c_str(), nl.at);
                }
            }
        }
    }
    break;

  case 658: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 659: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 660: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 661: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 662: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 663: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 664: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 665: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 666: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 667: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 668: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 669: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 670: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 671: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 672: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 673: /* let_variable_name_with_pos_list: "$i" '(' expr2 ')'  */
                                      {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 674: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
                                         {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 675: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 676: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 677: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options semicolon  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 678: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 679: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 680: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr semicolon  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 681: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr  */
                                                                                                                  {
        // Until absence of semicolon with lambda is not fixed
        format::try_semicolon_at_eol(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-3]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 682: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr_pipe  */
                                                                                                           {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-3]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 683: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 684: /* $@41: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 685: /* global_variable_declaration_list: global_variable_declaration_list $@41 optional_field_annotation let_variable_declaration opt_sem  */
                                                                              {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders )
                for ( auto & nl : *((yyvsp[-1].pVarDecl)->pNameList) )
                    crd->afterGlobalVariable(nl.name.c_str(),tak);
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterGlobalVariables(tak);
        }
        (yyval.pVarDeclList) = (yyvsp[-4].pVarDeclList);
        (yyvsp[-1].pVarDecl)->annotation = (yyvsp[-2].aaList);
        (yyvsp[-4].pVarDeclList)->push_back((yyvsp[-1].pVarDecl));
    }
    break;

  case 686: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 687: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 688: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 689: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 690: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 691: /* global_let: kwd_let optional_shared optional_public_or_private_variable open_block global_variable_declaration_list close_block  */
                                                                                                                                                                   {
        handle_brace(format::Pos::from(tokAt(scanner, (yylsp[-2]))), (yyvsp[-2].ui),
                     format::get_substring(tokRangeAt(scanner, (yylsp[-2]), (yylsp[0]))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 692: /* $@42: %empty  */
                                                                                        {
        yyextra->das_force_oxford_comma=true;
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 693: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@42 optional_field_annotation let_variable_declaration  */
                                                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders )
                for ( auto & nl : *((yyvsp[0].pVarDecl)->pNameList) )
                    crd->afterGlobalVariable(nl.name.c_str(),tak);
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterGlobalVariables(tak);
        }
        ast_globalLet(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].aaList),(yyvsp[0].pVarDecl));
    }
    break;

  case 694: /* enum_list: %empty  */
        {
        (yyval.pEnumList) = new Enumeration();
    }
    break;

  case 695: /* enum_list: enum_list "name" opt_sem  */
                                              {
        format::skip_token(true, false, tokAt(scanner,(yylsp[0])));
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        if ( !(yyvsp[-2].pEnumList)->add(*(yyvsp[-1].s),nullptr,tokAt(scanner,(yylsp[-1]))) ) {
            das_yyerror(scanner,"enumeration already declared " + *(yyvsp[-1].s), tokAt(scanner,(yylsp[-1])),
                CompilationError::enumeration_value_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tokName = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) {
                crd->afterEnumerationEntry((yyvsp[-1].s)->c_str(), tokName);
            }
        }
        delete (yyvsp[-1].s);
        (yyval.pEnumList) = (yyvsp[-2].pEnumList);
    }
    break;

  case 696: /* enum_list: enum_list "name" '=' expr opt_sem  */
                                                              {
        format::skip_token(true, false, tokAt(scanner,(yylsp[0])));
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        if ( !(yyvsp[-4].pEnumList)->add(*(yyvsp[-3].s),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-3]))) ) {
            das_yyerror(scanner,"enumeration value already declared " + *(yyvsp[-3].s), tokAt(scanner,(yylsp[-3])),
                CompilationError::enumeration_value_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tokName = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) {
                crd->afterEnumerationEntry((yyvsp[-3].s)->c_str(), tokName);
            }
        }
        delete (yyvsp[-3].s);
        (yyval.pEnumList) = (yyvsp[-4].pEnumList);
    }
    break;

  case 697: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 698: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 699: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 700: /* $@43: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 701: /* single_alias: optional_public_or_private_alias "name" $@43 '=' type_declaration  */
                                  {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        (yyvsp[0].pTypeDecl)->isPrivateAlias = !(yyvsp[-4].b);
        if ( (yyvsp[0].pTypeDecl)->baseType == Type::alias ) {
            das_yyerror(scanner,"alias cannot be defined in terms of another alias "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::invalid_type);
        }
        (yyvsp[0].pTypeDecl)->alias = *(yyvsp[-3].s);
        if ( !yyextra->g_Program->addAlias((yyvsp[0].pTypeDecl)) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterAlias((yyvsp[-3].s)->c_str(),pubename);
        }
        if ((yylsp[-4]).first_column == (yylsp[-4]).last_column) {
            (yyloc).first_line = (yylsp[-3]).first_line;
            (yyloc).first_column = (yylsp[-3]).first_column;
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 702: /* alias_list: single_alias opt_sem  */
                                    {
        (yyval.positions) = new vector<LineInfo>(1, tokAt(scanner, (yylsp[-1])));
    }
    break;

  case 703: /* alias_list: alias_list single_alias opt_sem  */
                                                       {
        (yyvsp[-2].positions)->emplace_back(tokAt(scanner, (yylsp[-1])));
        (yyval.positions) = (yyvsp[-2].positions);
    }
    break;

  case 704: /* alias_declaration: "typedef" open_block alias_list close_block  */
                                                                           {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-3]))))) {
            // todo: comments here and all such places, same rule
            for (const auto single: *(yyvsp[-1].positions)) {
                format::get_writer() << "typedef " << format::get_substring(single) << '\n';
            }
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[0]))));
        }

    }
    break;

  case 705: /* $@44: %empty  */
                    { yyextra->das_force_oxford_comma=true;}
    break;

  case 707: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 708: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 709: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 710: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 711: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name open_block enum_list close_block  */
                                                                                                                                                              {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
        const auto first_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(first_loc, (yyvsp[-2].ui),
                     format::get_substring(first_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-3].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-6].faList),tokAt(scanner,(yylsp[-6])),(yyvsp[-4].b),(yyvsp[-3].pEnum),(yyvsp[-1].pEnumList),Type::tInt);
    }
    break;

  case 712: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name ':' enum_basic_type_declaration open_block enum_list close_block  */
                                                                                                                                                                                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-5]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
        const auto first_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(first_loc, (yyvsp[-2].ui),
                     format::get_substring(first_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-5].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-8].faList),tokAt(scanner,(yylsp[-8])),(yyvsp[-6].b),(yyvsp[-5].pEnum),(yyvsp[-1].pEnumList),(yyvsp[-3].type));
    }
    break;

  case 713: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 714: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 715: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 716: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 717: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 718: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 719: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 720: /* class_or_struct: "class" "template"  */
                                  { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 721: /* class_or_struct: "struct" "template"  */
                                  { (yyval.i) = CorS_StructTemplate; }
    break;

  case 722: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 723: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 724: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 725: /* optional_struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 726: /* optional_struct_variable_declaration_list: open_block struct_variable_declaration_list close_block  */
                                                                                      {
        const auto prev_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(prev_loc, (yyvsp[-2].ui),
                     format::get_substring(prev_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 727: /* $@45: %empty  */
                                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 728: /* $@46: %empty  */
                         {
        if ( (yyvsp[0].pStructure) ) {
            (yyvsp[0].pStructure)->isClass = (yyvsp[-3].i)==CorS_Class || (yyvsp[-3].i)==CorS_ClassTemplate;
            (yyvsp[0].pStructure)->isTemplate = (yyvsp[-3].i)==CorS_ClassTemplate || (yyvsp[-3].i)==CorS_StructTemplate;
            (yyvsp[0].pStructure)->privateStructure = !(yyvsp[-2].b);
        }
    }
    break;

  case 729: /* structure_declaration: optional_annotation_list class_or_struct optional_public_or_private_structure $@45 structure_name $@46 optional_struct_variable_declaration_list  */
                                                      {
        if ( (yyvsp[-2].pStructure) ) {
            ast_structureDeclaration ( scanner, (yyvsp[-6].faList), tokAt(scanner,(yylsp[-5])), (yyvsp[-2].pStructure), tokAt(scanner,(yylsp[-2])), (yyvsp[0].pVarDeclList) );
            if ( !yyextra->g_CommentReaders.empty() ) {
                auto tak = tokAt(scanner,(yylsp[-5]));
                for ( auto & crd : yyextra->g_CommentReaders ) crd->afterStructure((yyvsp[-2].pStructure),tak);
            }
        } else {
            deleteVariableDeclarationList((yyvsp[0].pVarDeclList));
        }
    }
    break;

  case 730: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 731: /* variable_name_with_pos_list: "$i" '(' expr2 ')'  */
                                      {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 732: /* variable_name_with_pos_list: "name" "aka" "name"  */
                                         {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 733: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 734: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 735: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 736: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 737: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 738: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 739: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 740: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 741: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 742: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 743: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 744: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 745: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 746: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 747: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 748: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 749: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 750: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 751: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 752: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 753: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 754: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 755: /* basic_type_declaration: "float16"  */
                        { (yyval.type) = Type::tFloat16; }
    break;

  case 756: /* basic_type_declaration: "half2"  */
                        { (yyval.type) = Type::tHalf2; }
    break;

  case 757: /* basic_type_declaration: "half3"  */
                        { (yyval.type) = Type::tHalf3; }
    break;

  case 758: /* basic_type_declaration: "half4"  */
                        { (yyval.type) = Type::tHalf4; }
    break;

  case 759: /* basic_type_declaration: "half8"  */
                        { (yyval.type) = Type::tHalf8; }
    break;

  case 760: /* basic_type_declaration: "short2"  */
                        { (yyval.type) = Type::tShort2; }
    break;

  case 761: /* basic_type_declaration: "short3"  */
                        { (yyval.type) = Type::tShort3; }
    break;

  case 762: /* basic_type_declaration: "short4"  */
                        { (yyval.type) = Type::tShort4; }
    break;

  case 763: /* basic_type_declaration: "short8"  */
                        { (yyval.type) = Type::tShort8; }
    break;

  case 764: /* basic_type_declaration: "ushort2"  */
                        { (yyval.type) = Type::tUShort2; }
    break;

  case 765: /* basic_type_declaration: "ushort3"  */
                        { (yyval.type) = Type::tUShort3; }
    break;

  case 766: /* basic_type_declaration: "ushort4"  */
                        { (yyval.type) = Type::tUShort4; }
    break;

  case 767: /* basic_type_declaration: "ushort8"  */
                        { (yyval.type) = Type::tUShort8; }
    break;

  case 768: /* basic_type_declaration: "byte2"  */
                        { (yyval.type) = Type::tByte2; }
    break;

  case 769: /* basic_type_declaration: "byte3"  */
                        { (yyval.type) = Type::tByte3; }
    break;

  case 770: /* basic_type_declaration: "byte4"  */
                        { (yyval.type) = Type::tByte4; }
    break;

  case 771: /* basic_type_declaration: "byte8"  */
                        { (yyval.type) = Type::tByte8; }
    break;

  case 772: /* basic_type_declaration: "byte16"  */
                        { (yyval.type) = Type::tByte16; }
    break;

  case 773: /* basic_type_declaration: "ubyte2"  */
                        { (yyval.type) = Type::tUByte2; }
    break;

  case 774: /* basic_type_declaration: "ubyte3"  */
                        { (yyval.type) = Type::tUByte3; }
    break;

  case 775: /* basic_type_declaration: "ubyte4"  */
                        { (yyval.type) = Type::tUByte4; }
    break;

  case 776: /* basic_type_declaration: "ubyte8"  */
                        { (yyval.type) = Type::tUByte8; }
    break;

  case 777: /* basic_type_declaration: "ubyte16"  */
                        { (yyval.type) = Type::tUByte16; }
    break;

  case 778: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 779: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 780: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 781: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 782: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 783: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 784: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 785: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 786: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 787: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 788: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 789: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 790: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 791: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 792: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 793: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 794: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 795: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 796: /* auto_type_declaration: "$t" '(' expr2 ')'  */
                                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::alias);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = "``MACRO``TAG``";
        (yyval.pTypeDecl)->isTag = true;
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner, (yylsp[-1]));
        (yyval.pTypeDecl)->firstType->typeMacroExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 797: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 798: /* bitfield_bits: bitfield_bits semicolon "name"  */
                                                 {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 799: /* bitfield_bits: bitfield_bits ',' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 802: /* bitfield_alias_bits: %empty  */
       {
        auto pSL = new vector<tuple<string,Expression *>>();
        (yyval.pNameExprList) = pSL;

    }
    break;

  case 803: /* bitfield_alias_bits: bitfield_alias_bits "name" SEMICOLON  */
                                                       {
        if (format::enum_bitfield_with_comma() && format::is_replace_braces() && format::prepare_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-1]))))) {
            format::get_writer() << ",";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        }
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pNameExprList) = (yyvsp[-2].pNameExprList);
        (yyval.pNameExprList)->emplace_back(*(yyvsp[-1].s),nullptr);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[-1].s)->c_str(),atvname);
        }
        delete (yyvsp[-1].s);
    }
    break;

  case 804: /* bitfield_alias_bits: bitfield_alias_bits "name" commas  */
                                                    {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pNameExprList) = (yyvsp[-2].pNameExprList);
        (yyval.pNameExprList)->emplace_back(*(yyvsp[-1].s),nullptr);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[-1].s)->c_str(),atvname);
        }
        delete (yyvsp[-1].s);
    }
    break;

  case 805: /* bitfield_alias_bits: bitfield_alias_bits "name"  */
                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.pNameExprList) = (yyvsp[-1].pNameExprList);
        (yyval.pNameExprList)->emplace_back(*(yyvsp[0].s),nullptr);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[0].s)->c_str(),atvname);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 806: /* bitfield_alias_bits: bitfield_alias_bits "name" '=' expr SEMICOLON  */
                                                                       {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        (yyval.pNameExprList) = (yyvsp[-4].pNameExprList);
        (yyval.pNameExprList)->emplace_back(*(yyvsp[-3].s),(yyvsp[-1].pExpression));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[-3].s)->c_str(),atvname);
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 807: /* bitfield_alias_bits: bitfield_alias_bits "name" '=' expr commas  */
                                                                    {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        (yyval.pNameExprList) = (yyvsp[-4].pNameExprList);
        (yyval.pNameExprList)->emplace_back(*(yyvsp[-3].s),(yyvsp[-1].pExpression));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[-3].s)->c_str(),atvname);
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 808: /* bitfield_alias_bits: bitfield_alias_bits "name" '=' expr  */
                                                             {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        (yyval.pNameExprList) = (yyvsp[-3].pNameExprList);
        (yyval.pNameExprList)->emplace_back(*(yyvsp[-2].s),(yyvsp[0].pExpression));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[-2].s)->c_str(),atvname);
        }
        delete (yyvsp[-2].s);
    }
    break;

  case 809: /* bitfield_basic_type_declaration: %empty  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 810: /* bitfield_basic_type_declaration: ':' "uint8"  */
                             { (yyval.type) = Type::tBitfield8; }
    break;

  case 811: /* bitfield_basic_type_declaration: ':' "uint16"  */
                             { (yyval.type) = Type::tBitfield16; }
    break;

  case 812: /* bitfield_basic_type_declaration: ':' "uint"  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 813: /* bitfield_basic_type_declaration: ':' "uint64"  */
                             { (yyval.type) = Type::tBitfield64; }
    break;

  case 814: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' '>'  */
                                                                          {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-2].type));
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-2]));
    }
    break;

  case 815: /* $@47: %empty  */
                                                                     { yyextra->das_arrow_depth ++; }
    break;

  case 816: /* $@48: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 817: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' $@47 bitfield_bits '>' $@48  */
                                                                                                                                                             {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-5].type));
            (yyval.pTypeDecl)->argNames = *(yyvsp[-2].pNameList);
            auto maxBits = (yyval.pTypeDecl)->maxBitfieldBits();
            if ( (yyval.pTypeDecl)->argNames.size()>maxBits ) {
                das_yyerror(scanner,"only " + to_string(maxBits) + " different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-5])),
                    CompilationError::invalid_type);
            }
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
            delete (yyvsp[-2].pNameList);
    }
    break;

  case 820: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 821: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 822: /* dim_list: '[' expr2 ']'  */
                              {
        (yyval.pTypeDecl) = appendDimExpr(nullptr, (yyvsp[-1].pExpression), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 823: /* dim_list: dim_list '[' expr2 ']'  */
                                             {
        (yyval.pTypeDecl) = appendDimExpr((yyvsp[-3].pTypeDecl), (yyvsp[-1].pExpression), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 824: /* type_declaration_no_options: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 825: /* type_declaration_no_options: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 826: /* type_declaration_no_options: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 827: /* type_declaration_no_options: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 828: /* type_declaration_no_options: type_declaration_no_options dim_list  */
                                                                {
        if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeDecl ) {
            das_yyerror(scanner,"type declaration can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_type);
        } else if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeMacro ) {
            das_yyerror(scanner,"macro can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_type);
        }
        (yyval.pTypeDecl) = attachDimChain((yyvsp[0].pTypeDecl), (yyvsp[-1].pTypeDecl));
    }
    break;

  case 829: /* type_declaration_no_options: type_declaration_no_options '[' ']'  */
                                                      {
        (yyval.pTypeDecl) = appendAutoDim((yyvsp[-2].pTypeDecl), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 830: /* $@49: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 831: /* $@50: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 832: /* type_declaration_no_options: "type" '<' $@49 type_declaration '>' $@50  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 833: /* type_declaration_no_options: "typedecl" '(' expr2 ')'  */
                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->typeMacroExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 834: /* type_declaration_no_options: '$' name_in_namespace optional_expr_list_in_braces  */
                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-1]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = sequenceToList((yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-1])), *(yyvsp[-1].s)));
        delete (yyvsp[-1].s);
    }
    break;

  case 835: /* $@51: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 836: /* type_declaration_no_options: '$' name_in_namespace '<' $@51 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 837: /* type_declaration_no_options: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 838: /* type_declaration_no_options: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 839: /* type_declaration_no_options: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 840: /* type_declaration_no_options: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 841: /* type_declaration_no_options: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 842: /* type_declaration_no_options: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 843: /* type_declaration_no_options: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 844: /* type_declaration_no_options: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 845: /* type_declaration_no_options: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 846: /* type_declaration_no_options: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 847: /* type_declaration_no_options: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 848: /* type_declaration_no_options: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 849: /* $@52: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 850: /* $@53: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 851: /* type_declaration_no_options: "smart_ptr" '<' $@52 type_declaration '>' $@53  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 852: /* type_declaration_no_options: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 853: /* $@54: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 854: /* $@55: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 855: /* type_declaration_no_options: "array" '<' $@54 type_declaration '>' $@55  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 856: /* $@56: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 857: /* $@57: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 858: /* type_declaration_no_options: "table" '<' $@56 table_type_pair '>' $@57  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 859: /* $@58: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 860: /* $@59: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 861: /* type_declaration_no_options: "iterator" '<' $@58 type_declaration '>' $@59  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 862: /* type_declaration_no_options: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 863: /* $@60: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 864: /* $@61: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 865: /* type_declaration_no_options: "block" '<' $@60 type_declaration '>' $@61  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 866: /* $@62: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 867: /* $@63: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 868: /* type_declaration_no_options: "block" '<' $@62 optional_function_argument_list optional_function_type '>' $@63  */
                                                                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
        if ( (yyvsp[-3].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-3].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        }
    }
    break;

  case 869: /* type_declaration_no_options: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 870: /* $@64: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 871: /* $@65: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 872: /* type_declaration_no_options: "function" '<' $@64 type_declaration '>' $@65  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 873: /* $@66: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 874: /* $@67: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 875: /* type_declaration_no_options: "function" '<' $@66 optional_function_argument_list optional_function_type '>' $@67  */
                                                                                                                                                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
        if ( (yyvsp[-3].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-3].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        }
    }
    break;

  case 876: /* type_declaration_no_options: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 877: /* $@68: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 878: /* $@69: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 879: /* type_declaration_no_options: "lambda" '<' $@68 type_declaration '>' $@69  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 880: /* $@70: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 881: /* $@71: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 882: /* type_declaration_no_options: "lambda" '<' $@70 optional_function_argument_list optional_function_type '>' $@71  */
                                                                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
        if ( (yyvsp[-3].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-3].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        }
    }
    break;

  case 883: /* $@72: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 884: /* $@73: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 885: /* type_declaration_no_options: "tuple" '<' $@72 tuple_type_list '>' $@73  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 886: /* $@74: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 887: /* $@75: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 888: /* type_declaration_no_options: "variant" '<' $@74 variant_type_list '>' $@75  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 889: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 890: /* type_declaration: type_declaration '|' type_declaration_no_options  */
                                                                     {
        if ( (yyvsp[-2].pTypeDecl)->baseType==Type::option ) {
            (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
            (yyval.pTypeDecl)->argTypes.push_back((yyvsp[0].pTypeDecl));
        } else {
            (yyval.pTypeDecl) = new TypeDecl(Type::option);
            (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
            (yyval.pTypeDecl)->argTypes.push_back((yyvsp[-2].pTypeDecl));
            (yyval.pTypeDecl)->argTypes.push_back((yyvsp[0].pTypeDecl));
        }
    }
    break;

  case 891: /* type_declaration: type_declaration '|' '#'  */
                                             {
        if ( (yyvsp[-2].pTypeDecl)->baseType==Type::option ) {
            (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
            (yyval.pTypeDecl)->argTypes.push_back(new TypeDecl(*(yyvsp[-2].pTypeDecl)->argTypes.back()));
            (yyvsp[-2].pTypeDecl)->argTypes.back()->temporary ^= true;
        } else {
            (yyval.pTypeDecl) = new TypeDecl(Type::option);
            (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
            (yyval.pTypeDecl)->argTypes.push_back((yyvsp[-2].pTypeDecl));
            (yyval.pTypeDecl)->argTypes.push_back(new TypeDecl(*(yyvsp[-2].pTypeDecl)));
            (yyval.pTypeDecl)->argTypes.back()->temporary ^= true;
        }
    }
    break;

  case 892: /* $@76: %empty  */
                                                          { yyextra->das_need_oxford_comma=false; }
    break;

  case 893: /* $@77: %empty  */
                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 894: /* $@78: %empty  */
                      {
        if (format::is_replace_braces() && (yyvsp[0].ui) != 0xdeadbeef && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))))) {
            format::get_writer() << " {";
            format::finish_rule(format::Pos::from(tokAt(scanner,(yylsp[0]))));
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 895: /* $@79: %empty  */
                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
    }
    break;

  case 896: /* tuple_alias_declaration: "tuple" optional_public_or_private_alias $@76 "name" $@77 open_block $@78 tuple_alias_type_list $@79 close_block  */
                         {
        if (format::is_replace_braces() && (yyvsp[-4].ui) != 0xdeadbeef && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))))) {
            format::get_writer() << "\n" << string((yyvsp[0].ui) * yyextra->das_tab_size, ' ') + "}";
            format::finish_rule(format::Pos::from(tokAt(scanner,(yylsp[0]))));
        }
        auto vtype = new TypeDecl(Type::tTuple);
        vtype->alias = *(yyvsp[-6].s);
        vtype->at = tokAt(scanner,(yylsp[-6]));
        vtype->isPrivateAlias = !(yyvsp[-8].b);
        varDeclToTypeDecl(scanner, vtype, (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-6].s),tokAt(scanner,(yylsp[-6])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTuple((yyvsp[-6].s)->c_str(),atvname);
        }
        delete (yyvsp[-6].s);
    }
    break;

  case 897: /* $@80: %empty  */
                                                            { yyextra->das_need_oxford_comma=false; }
    break;

  case 898: /* $@81: %empty  */
                                                                                                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 899: /* $@82: %empty  */
                      {
        if (format::is_replace_braces() && (yyvsp[0].ui) != 0xdeadbeef && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))))) {
            format::get_writer() << " {";
            format::finish_rule(format::Pos::from(tokAt(scanner,(yylsp[0]))));
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 900: /* $@83: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
    }
    break;

  case 901: /* variant_alias_declaration: "variant" optional_public_or_private_alias $@80 "name" $@81 open_block $@82 variant_alias_type_list $@83 close_block  */
                         {
        if (format::is_replace_braces() && (yyvsp[0].ui) != 0xdeadbeef && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))))) {
            format::get_writer() << "\n" << string((yyvsp[0].ui) * yyextra->das_tab_size, ' ') + "}";
            format::finish_rule(format::Pos::from(tokAt(scanner,(yylsp[0]))));
        }
        auto vtype = new TypeDecl(Type::tVariant);
        vtype->alias = *(yyvsp[-6].s);
        vtype->at = tokAt(scanner,(yylsp[-6]));
        vtype->isPrivateAlias = !(yyvsp[-8].b);
        varDeclToTypeDecl(scanner, vtype, (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-6].s),tokAt(scanner,(yylsp[-6])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariant((yyvsp[-6].s)->c_str(),atvname);
        }
        delete (yyvsp[-6].s);
    }
    break;

  case 902: /* $@84: %empty  */
                                                             { yyextra->das_need_oxford_comma=false; }
    break;

  case 903: /* $@85: %empty  */
                                                                                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 904: /* bitfield_alias_declaration: "bitfield" optional_public_or_private_alias $@84 "name" $@85 bitfield_basic_type_declaration open_block bitfield_alias_bits close_block  */
                                                                                                              {
        const auto prev_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(prev_loc, (yyvsp[-2].ui),
                     format::get_substring(prev_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-5]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-5]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
        auto btype = new TypeDecl((yyvsp[-3].type));
        btype->alias = *(yyvsp[-5].s);
        btype->at = tokAt(scanner,(yylsp[-5]));
        btype->isPrivateAlias = !(yyvsp[-7].b);
        for ( auto & p : *(yyvsp[-1].pNameExprList) ) {
            if ( !get<1>(p) ) {
                btype->argNames.push_back(get<0>(p));
            }
        }
        auto maxBits = btype->maxBitfieldBits();
        if ( btype->argNames.size()>maxBits ) {
            das_yyerror(scanner,"only " + to_string(maxBits) + " different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-5])),
                CompilationError::invalid_type);
        }
        for ( auto & p : *(yyvsp[-1].pNameExprList) ) {
            if ( get<1>(p) ) {
                ast_globalBitfieldConst ( scanner, btype, (yyvsp[-7].b), get<0>(p), get<1>(p) );
            }
        }
        if ( !yyextra->g_Program->addAlias(btype) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-5].s),tokAt(scanner,(yylsp[-5])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-5]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfield((yyvsp[-5].s)->c_str(),atvname);
        }
        delete (yyvsp[-5].s);
        delete (yyvsp[-1].pNameExprList);
    }
    break;

  case 905: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 906: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 907: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 908: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 909: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 910: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 911: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 912: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 913: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 914: /* make_struct_fields: "$f" '(' expr2 ')' copy_or_move expr  */
                                                                            {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-5]), (yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 915: /* make_struct_fields: "$f" '(' expr2 ')' ":=" expr  */
                                                                   {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner, (yylsp[-5]), (yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 916: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr2 ')' copy_or_move expr  */
                                                                                                        {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-5]),(yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 917: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr2 ')' ":=" expr  */
                                                                                               {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-5]), (yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 918: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 919: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 920: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 921: /* make_struct_dim: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 922: /* make_struct_dim: make_struct_dim semicolon make_struct_fields  */
                                                               {
        ((ExprMakeStruct *) (yyvsp[-2].pExpression))->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 923: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 924: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 925: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 926: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 927: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 928: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 929: /* optional_block: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 930: /* optional_block: "where" expr_block  */
                                  { (yyvsp[0].pExpression)->at = tokAt(scanner, (yylsp[0])); (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 943: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 944: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 945: /* make_struct_decl: "[[" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                     {
        // std::cout << "case1" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-4]))))) {
            auto type = format::type_to_string((yyvsp[-3].pTypeDecl), tokAt(scanner,(yylsp[-3]))).value_or("");
            const bool is_initialized = false;
            const auto before = format::substring_between(tokAt(scanner,(yylsp[-4])), tokAt(scanner, (yylsp[-3])));
            const auto internal = format::handle_msd(tokAt(scanner, (yylsp[-3])),
                                                     static_cast<ExprMakeStruct*>((yyvsp[-2].pExpression)),
                                                     tokAt(scanner, (yyvsp[-1].pExpression) != nullptr ? (yylsp[-1]) : (yylsp[0])),
                                                     type, is_initialized);
            if (static_cast<ExprMakeStruct*>((yyvsp[-2].pExpression))->structs.size() == 1) {
                // single struct
                if (type.find('[') != size_t(-1)) {
                    format::get_writer() << "fixed_array(" << before << internal << ")";
                } else {
                    format::get_writer() << before << internal;
                }
                if ((yyvsp[-1].pExpression) != nullptr) {
                    format::get_writer() << " <| " << format::get_substring((yyvsp[-1].pExpression)->at);
                }
            } else {
                // array of structs
    //            const auto internal = format::get_substring(format::Pos::from(tokAt(scanner,@msd)),
    //                                                          format::Pos::from(tokAt(scanner,@end)));
                format::get_writer() << "fixed_array(" << internal << ")";
            }
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-4]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 946: /* make_struct_decl: "[[" type_declaration_no_options optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                {
        // // std::cout << "case2" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-3]))))) {
            auto type = format::type_to_string((yyvsp[-2].pTypeDecl), tokAt(scanner,(yylsp[-2])));
            das_hash_set<string> always_init = {
                // Is there a method to describe all this types?
                "struct<array<",
                "array<",
                "bitfield",
                "string",
                "int",
                "float",
                "variant",
                "tuple",
            };
            const auto before = format::get_substring(format::Pos::from_last(tokAt(scanner,(yylsp[-3]))),
                                                      format::Pos::from(tokAt(scanner, (yylsp[-2]))));
            const auto after = format::get_substring(format::Pos::from_last(tokAt(scanner,(yylsp[-2]))),
                                                      format::Pos::from(tokAt(scanner, (yylsp[0]))));
            string suffix = ")";
            if ((yyvsp[-2].pTypeDecl)->isPointer()) {
                format::get_writer() << "default<" << type.value() << ">";
                suffix.clear();
            } else if (any_of(always_init.begin(), always_init.end(),
                            [&type](auto t){ return type.value_or("").find(t) == 0; })) {
                format::get_writer() << type.value_or("") << "(";
            } else {
                format::get_writer() << type.value_or("") << "("; // Possible uninitialized
            }
            format::get_writer() << before << after << suffix;
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        auto msd = new ExprMakeStruct();
        msd->makeType = (yyvsp[-2].pTypeDecl);
        msd->block = (yyvsp[-1].pExpression);
        msd->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = msd;
    }
    break;

  case 947: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                            {
        // std::cout << "case3" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-5]))))) {
            const auto type_name = format::type_to_string((yyvsp[-4].pTypeDecl), tokAt(scanner,(yylsp[-4])));
            auto maybe_init = (format::can_default_init(type_name.value_or(""))) ? "" : ""; // Possible uninitialized
            format::get_writer() << type_name.value_or("") << "("
                                 << format::substring_between(tokAt(scanner,(yylsp[-5])), tokAt(scanner, (yylsp[-4])))
                                 << format::substring_between(tokAt(scanner,(yylsp[-3])), tokAt(scanner, (yylsp[-2])))
                                 << format::substring_between(tokAt(scanner,(yylsp[-2])), tokAt(scanner, (yylsp[0])))
                                 << maybe_init << ")";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }
        auto msd = new ExprMakeStruct();
        msd->makeType = (yyvsp[-4].pTypeDecl);
        msd->useInitializer = true;
        msd->block = (yyvsp[-1].pExpression);
        msd->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pExpression) = msd;
    }
    break;

  case 948: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                                                {
        // std::cout << "case4" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-6]))))) {
            auto type_name = format::type_to_string((yyvsp[-5].pTypeDecl), tokAt(scanner,(yylsp[-5]))).value_or("");
            const auto internal = format::handle_msd(tokAt(scanner, (yylsp[-3])),
                                                     static_cast<ExprMakeStruct*>((yyvsp[-2].pExpression)),
                                                     tokAt(scanner, (yylsp[0])),
                                                     type_name);
            format::get_writer() << format::substring_between(tokAt(scanner, (yylsp[-6])), tokAt(scanner, (yylsp[-5])))
                                 << format::substring_between(tokAt(scanner, (yylsp[-4])), tokAt(scanner, (yylsp[-3])))
                                 << internal;
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-5].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 949: /* make_struct_decl: "[{" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
                                                                                                                                          {
        // std::cout << "case6" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-4]))))) {
            auto type_name = format::type_to_string((yyvsp[-3].pTypeDecl), tokAt(scanner,(yylsp[-3]))).value_or("");
            format::get_writer() << "[" << format::substring_between(tokAt(scanner, (yylsp[-4])), tokAt(scanner, (yylsp[-3])))
                                     << format::handle_msd(tokAt(scanner, (yylsp[-3])),
                                                              static_cast<ExprMakeStruct*>((yyvsp[-2].pExpression)),
                                                              tokAt(scanner, (yylsp[0])),
                                                              type_name)
                                 << "]";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-4]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_array_move");
        tam->arguments.push_back((yyvsp[-2].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 950: /* make_struct_decl: "[{" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
                                                                                                                                                          {
        // std::cout << "case7" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-6]))))) {
            auto type_name = format::type_to_string((yyvsp[-5].pTypeDecl), tokAt(scanner,(yylsp[-5]))).value_or("");
            format::get_writer() << "[" << format::substring_between(tokAt(scanner, (yylsp[-6])), tokAt(scanner, (yylsp[-5])))
                                   << format::handle_msd(tokAt(scanner, (yylsp[-3])),
                                                         static_cast<ExprMakeStruct*>((yyvsp[-2].pExpression)),
                                                         tokAt(scanner, (yylsp[0])),
                                                         type_name)
                                   << "]";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-5].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-6]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),"to_array_move");
        tam->arguments.push_back((yyvsp[-2].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 951: /* $@86: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 952: /* $@87: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 953: /* make_struct_decl: "struct" '<' $@86 type_declaration_no_options '>' $@87 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                            {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 954: /* $@88: %empty  */
                            { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 955: /* $@89: %empty  */
                                                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 956: /* make_struct_decl: "class" '<' $@88 type_declaration_no_options '>' $@89 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                          {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 957: /* $@90: %empty  */
                               { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 958: /* $@91: %empty  */
                                                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 959: /* make_struct_decl: "variant" '<' $@90 variant_type_list '>' $@91 '(' use_initializer make_variant_dim ')'  */
                                                                                                                                                                                                                        {
        auto mkt = new TypeDecl(Type::tVariant);
        mkt->at = tokAt(scanner,(yylsp[-9]));
        varDeclToTypeDecl(scanner, mkt, (yyvsp[-6].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-6].pVarDeclList));
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = mkt;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 960: /* $@92: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 961: /* $@93: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 962: /* make_struct_decl: "default" '<' $@92 type_declaration_no_options '>' $@93 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 963: /* make_tuple: expr  */
                  {
        (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 964: /* make_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 965: /* make_tuple: make_tuple ',' expr  */
                                      {
        (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0]));
        ExprMakeTuple * mt;
        if ( (yyvsp[-2].pExpression)->rtti_isMakeTuple() ) {
            mt = static_cast<ExprMakeTuple *>((yyvsp[-2].pExpression));
        } else {
            mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-2])));
            mt->values.push_back((yyvsp[-2].pExpression));
        }
        mt->values.push_back((yyvsp[0].pExpression));
        mt->at = format::concat(mt->at, tokAt(scanner, (yylsp[0])));
        (yyval.pExpression) = mt;
    }
    break;

  case 966: /* make_map_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 967: /* make_map_tuple: expr  */
                 {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 968: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = new TypeDecl(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 969: /* $@94: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 970: /* $@95: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 971: /* make_tuple_call: "tuple" '<' $@94 tuple_type_list '>' $@95 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                 {
        auto mkt = new TypeDecl(Type::tTuple);
        mkt->at = tokAt(scanner,(yylsp[-9]));
        varDeclToTypeDecl(scanner, mkt, (yyvsp[-6].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-6].pVarDeclList));
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = mkt;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceTuple = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 972: /* make_dim: make_tuple  */
                        {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 973: /* make_dim: make_dim semicolon make_tuple  */
                                                {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 974: /* make_dim_decl: '[' optional_expr_list ']'  */
                                                    {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-2])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = new TypeDecl(Type::autoinfer);
            mka->gen2 = true;
            auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_array_move");
            tam->arguments.push_back(mka);
            (yyval.pExpression) = tam;
        } else {
            auto mks = new ExprMakeStruct();
            mks->at = tokAt(scanner,(yylsp[-2]));
            mks->makeType = new TypeDecl(Type::tArray);
            mks->makeType->firstType = new TypeDecl(Type::autoinfer);
            mks->useInitializer = true;
            mks->alwaysUseInitializer = true;
            (yyval.pExpression) = mks;
        }

    }
    break;

  case 975: /* make_dim_decl: "[[" type_declaration_no_options make_dim optional_trailing_semicolon_sqr_sqr  */
                                                                                                                   {
        // std::cout << "case13" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-3]))))) {
            auto type_name = format::type_to_string((yyvsp[-2].pTypeDecl), tokAt(scanner,(yylsp[-2])));
            auto internal = format::handle_mka(tokAt(scanner, (yylsp[-2])),
                                               static_cast<ExprMakeArray*>((yyvsp[-1].pExpression)),
                                               tokAt(scanner, (yylsp[0])));
            const auto before = format::substring_between(tokAt(scanner, (yylsp[-3])), tokAt(scanner, (yylsp[-2])));
            if (static_cast<ExprMakeArray*>((yyvsp[-1].pExpression))->values.size() == 1) {
                // single element
                if (type_name.value_or("").find('[') != size_t(-1)) {
                    format::get_writer() << "fixed_array(" << before << internal << ")";
                } else {
                    format::get_writer() << before << internal;
                }
            } else {
                format::get_writer() << "fixed_array";
                if (!(yyvsp[-2].pTypeDecl)->isAuto()) {
                    format::get_writer() << "<" << type_name.value().substr(0, type_name.value().find('[')) << ">";
                }
                format::get_writer() << "(" << before << internal << ")";
            }
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-2].pTypeDecl);
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 976: /* make_dim_decl: "[{" type_declaration_no_options make_dim optional_trailing_semicolon_cur_sqr  */
                                                                                                                   {
        // std::cout << "case8" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-3]))))) {
            string prefix = "[";
            string suffix = "]";
            if (!(yyvsp[-2].pTypeDecl)->isAuto()) {
                prefix = "array<" + format::get_substring(tokAt(scanner,(yylsp[-2]))) + ">(";
                suffix = ")";
            }
            format::get_writer() << prefix
                                 << format::substring_between(tokAt(scanner, (yylsp[-3])), tokAt(scanner, (yylsp[-2])))
                                 << format::handle_mka(tokAt(scanner, (yylsp[-2])),
                                                                 static_cast<ExprMakeArray*>((yyvsp[-1].pExpression)),
                                                                 tokAt(scanner, (yylsp[0])))
                                 << suffix;
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-2].pTypeDecl);
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 977: /* $@96: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 978: /* $@97: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 979: /* make_dim_decl: "array" "struct" '<' $@96 type_declaration_no_options '>' $@97 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-10]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-10])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 980: /* $@98: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 981: /* $@99: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 982: /* make_dim_decl: "array" "tuple" '<' $@98 tuple_type_list '>' $@99 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                     {
        auto mkt = new TypeDecl(Type::tTuple);
        mkt->at = tokAt(scanner,(yylsp[-10]));
        varDeclToTypeDecl(scanner, mkt, (yyvsp[-6].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-6].pVarDeclList));
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-10]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = mkt;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceTuple = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-10])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 983: /* $@100: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 984: /* $@101: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 985: /* make_dim_decl: "array" "variant" '<' $@100 variant_type_list '>' $@101 '(' make_variant_dim ')'  */
                                                                                                                                                                      {
        auto mkt = new TypeDecl(Type::tVariant);
        mkt->at = tokAt(scanner,(yylsp[-9]));
        varDeclToTypeDecl(scanner, mkt, (yyvsp[-5].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-5].pVarDeclList));
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = mkt;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-9])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 986: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
                                                                   {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        mka->gen2 = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_array_move");
        tam->arguments.push_back(mka);
        (yyval.pExpression) = tam;
    }
    break;

  case 987: /* $@102: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 988: /* $@103: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 989: /* make_dim_decl: "array" '<' $@102 type_declaration_no_options '>' $@103 '(' optional_expr_list ')'  */
                                                                                                                                                                        {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-8])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = (yyvsp[-5].pTypeDecl);
            mka->gen2 = true;
            auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-8])),"to_array_move");
            tam->arguments.push_back(mka);
            (yyval.pExpression) = tam;
        } else {
            auto msd = new ExprMakeStruct();
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->makeType = new TypeDecl(Type::tArray);
            msd->makeType->firstType = (yyvsp[-5].pTypeDecl);
            msd->at = tokAt(scanner,(yylsp[-5]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 990: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 991: /* $@104: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 992: /* $@105: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 993: /* make_dim_decl: "fixed_array" '<' $@104 type_declaration_no_options '>' $@105 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 994: /* make_table: make_map_tuple  */
                            {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 995: /* make_table: make_table semicolon make_map_tuple  */
                                                      {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 996: /* expr_map_tuple_list: make_map_tuple  */
                                {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 997: /* expr_map_tuple_list: expr_map_tuple_list ',' make_map_tuple  */
                                                                {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 998: /* make_table_decl: open_block optional_expr_map_tuple_list close_block  */
                                                                              {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-2])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = new TypeDecl(Type::autoinfer);
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto mks = new ExprMakeStruct();
            mks->at = tokAt(scanner,(yylsp[-2]));
            mks->makeType = new TypeDecl(Type::tTable);
            mks->makeType->firstType = new TypeDecl(Type::autoinfer);
            mks->makeType->secondType = new TypeDecl(Type::autoinfer);
            mks->useInitializer = true;
            mks->alwaysUseInitializer = true;
            (yyval.pExpression) = mks;
        }

    }
    break;

  case 999: /* make_table_decl: "{{" make_table optional_trailing_semicolon_cur_cur  */
                                                                                {
        // std::cout << "case10" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-2]))))) {
            format::get_writer() << "{"
                                 << format::substring_between(tokAt(scanner, (yylsp[-2])), tokAt(scanner, (yylsp[-1])))
                                 << format::convert_to_string(((ExprMakeArray *)(yyvsp[-1].pExpression))->values, ",", ";")
                                 << format::substring_between(tokAt(scanner, (yylsp[-1])), tokAt(scanner, (yylsp[0])))
                                 << "}";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        auto mkt = new TypeDecl(Type::tFixedArray);
        mkt->fixedDim = TypeDecl::dimAuto;
        mkt->at = tokAt(scanner,(yylsp[-2]));
        mkt->firstType = new TypeDecl(Type::autoinfer);
        mkt->firstType->at = mkt->at;
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = mkt;
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-2]));
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_table_move");
        ttm->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = ttm;
    }
    break;

  case 1000: /* make_table_decl: "table" '(' optional_expr_map_tuple_list ')'  */
                                                                       {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-1].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 1001: /* make_table_decl: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
                                                                                                                 {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-6])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = (yyvsp[-4].pTypeDecl);
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto msd = new ExprMakeStruct();
            msd->at = tokAt(scanner,(yylsp[-6]));
            msd->makeType = new TypeDecl(Type::tTable);
            msd->makeType->firstType = (yyvsp[-4].pTypeDecl);
            msd->makeType->secondType = new TypeDecl(Type::tVoid);
            msd->at = tokAt(scanner,(yylsp[-6]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 1002: /* make_table_decl: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
                                                                                                                                                             {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-8])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = new TypeDecl(Type::tTuple);
            mka->makeType->argTypes.push_back((yyvsp[-6].pTypeDecl));
            mka->makeType->argTypes.push_back((yyvsp[-4].pTypeDecl));
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-8])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto msd = new ExprMakeStruct();
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->makeType = new TypeDecl(Type::tTable);
            msd->makeType->firstType = (yyvsp[-6].pTypeDecl);
            msd->makeType->secondType = (yyvsp[-4].pTypeDecl);
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 1003: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 1004: /* array_comprehension_where: semicolon "where" expr  */
                                          { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 1005: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 1006: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 1007: /* array_comprehension: '[' "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-6]))))) {
            format::get_writer() << "(" << format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-6]))),
                                                                 format::Pos::from_last(tokAt(scanner,(yylsp[-4])))) << ")";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-4]))));
        }

        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 1008: /* array_comprehension: '[' "for" '(' variable_name_with_pos_list "in" expr_list ')' "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 1009: /* array_comprehension: '[' "iterator" "for" '(' variable_name_with_pos_list "in" expr_list ')' "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                                         {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 1010: /* array_comprehension: "begin of code block" "for" '(' variable_name_with_pos_list "in" expr_list ')' "end of expression" make_map_tuple array_comprehension_where "end of code block"  */
                                                                                                                                                                     {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
    }
    break;

  case 1011: /* array_comprehension: '[' "iterator" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                                  {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-6]))))) {
            format::get_writer() << "(" << format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-6]))),
                                                                 format::Pos::from_last(tokAt(scanner,(yylsp[-4])))) << ")";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-4]))));
        }
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 1012: /* array_comprehension: "[[" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where ']' ']'  */
                                                                                                                                                                         {
        // std::cout << "case5" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-9]))))) {
            auto internal = format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-8]))), format::Pos::from_last(tokAt(scanner,(yylsp[-2]))));
            format::get_writer() << "[iterator " << internal << "]";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),true,false);
    }
    break;

  case 1013: /* array_comprehension: "[{" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where "end of code block" ']'  */
                                                                                                                                                                                 {
        // std::cout << "case9" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-9]))))) {
            auto internal = format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-8]))), format::Pos::from_last(tokAt(scanner,(yylsp[-2]))));
            format::get_writer() << "[" << format::substring_between(tokAt(scanner, (yylsp[-9])), tokAt(scanner, (yylsp[-8])))
                                 << internal
                                 << format::substring_between(tokAt(scanner, (yylsp[-2])), tokAt(scanner, (yylsp[-1]))) << "]";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),false,false);
    }
    break;

  case 1014: /* array_comprehension: open_block "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where close_block  */
                                                                                                                                                                                        {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-6]))))) {
            format::get_writer() << "(" << format::substring_between(tokAt(scanner, (yylsp[-8])), tokAt(scanner, (yylsp[-7])))
                                 << format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-6]))),
                                                                 format::Pos::from_last(tokAt(scanner,(yylsp[-4]))))
                                 << format::substring_between(tokAt(scanner, (yylsp[-1])), tokAt(scanner, (yylsp[0]))) << ")";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-4]))));
        }
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
    }
    break;

  case 1015: /* array_comprehension: "{{" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where close_block close_block  */
                                                                                                                                                                                                       {
        // std::cout << "case12" << std::endl;
        if (format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-9]))))) {
            auto internal = format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-8]))), format::Pos::from_last(tokAt(scanner,(yylsp[-2]))));
            format::get_writer() << "[" << format::substring_between(tokAt(scanner, (yylsp[-9])), tokAt(scanner, (yylsp[-8])))
                                 << internal
                                 << format::substring_between(tokAt(scanner, (yylsp[-2])), tokAt(scanner, (yylsp[-1]))) << "]";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),true,true);
    }
    break;



      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == DAS_YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (&yylloc, scanner, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= DAS_YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == DAS_YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, scanner);
          yychar = DAS_YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, scanner, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != DAS_YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}


