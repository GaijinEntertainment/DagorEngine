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
#define YYSTYPE         DAS2_YYSTYPE
#define YYLTYPE         DAS2_YYLTYPE
/* Substitute the variable and function names.  */
#define yyparse         das2_yyparse
#define yylex           das2_yylex
#define yyerror         das2_yyerror
#define yydebug         das2_yydebug
#define yynerrs         das2_yynerrs

/* First part of user prologue.  */

    #include "daScript/misc/platform.h"
    #include "daScript/simulate/debug_info.h"
    #include "daScript/ast/compilation_errors.h"

    #ifdef _MSC_VER
    #pragma warning(disable:4262)
    #pragma warning(disable:4127)
    #pragma warning(disable:4702)
    #endif

    using namespace das;

    union DAS2_YYSTYPE;
    struct DAS2_YYLTYPE;

    #define YY_NO_UNISTD_H
    #include "lex2.yy.h"

    void das2_yyerror ( DAS2_YYLTYPE * lloc, yyscan_t scanner, const string & error );
    void das2_yyfatalerror ( DAS2_YYLTYPE * lloc, yyscan_t scanner, const string & error, CompilationError cerr );
    int yylex ( DAS2_YYSTYPE *lvalp, DAS2_YYLTYPE *llocp, yyscan_t scanner );
    void yybegin ( const char * str );

    void das2_yybegin_reader ( yyscan_t yyscanner );
    void das2_yyend_reader ( yyscan_t yyscanner );
    void das2_accept_sequence ( yyscan_t yyscanner, const char * seq, size_t seqLen, int lineNo, FileInfo * info );
    void das2_strfmt ( yyscan_t yyscanner );

    namespace das { class Module; }
    void das2_collect_keywords ( das::Module * mod, yyscan_t yyscanner );

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

#include "ds2_parser.hpp"
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
  YYSYMBOL_NOTDOT = 154,                   /* "!."  */
  YYSYMBOL_NOTQDOT = 155,                  /* "!?."  */
  YYSYMBOL_NOTBRA = 156,                   /* "!["  */
  YYSYMBOL_NOTQBRA = 157,                  /* "!?["  */
  YYSYMBOL_NOTQQ = 158,                    /* "!??"  */
  YYSYMBOL_NOTEQUEQU = 159,                /* "!=="  */
  YYSYMBOL_NOTLARROW = 160,                /* "!<-"  */
  YYSYMBOL_NOTCLONEEQU = 161,              /* "!:="  */
  YYSYMBOL_NOTIS = 162,                    /* "!is"  */
  YYSYMBOL_NOTAS = 163,                    /* "!as"  */
  YYSYMBOL_NOTQAS = 164,                   /* "!?as"  */
  YYSYMBOL_LPIPE = 165,                    /* "<|"  */
  YYSYMBOL_RPIPE = 166,                    /* "|>"  */
  YYSYMBOL_CLONEEQU = 167,                 /* ":="  */
  YYSYMBOL_ROTL = 168,                     /* "<<<"  */
  YYSYMBOL_ROTR = 169,                     /* ">>>"  */
  YYSYMBOL_ROTLEQU = 170,                  /* "<<<="  */
  YYSYMBOL_ROTREQU = 171,                  /* ">>>="  */
  YYSYMBOL_MAPTO = 172,                    /* "=>"  */
  YYSYMBOL_DOUBLE_AT = 173,                /* "@@"  */
  YYSYMBOL_AT_FIELD = 174,                 /* "@field"  */
  YYSYMBOL_COLCOL = 175,                   /* "::"  */
  YYSYMBOL_ANDAND = 176,                   /* "&&"  */
  YYSYMBOL_OROR = 177,                     /* "||"  */
  YYSYMBOL_XORXOR = 178,                   /* "^^"  */
  YYSYMBOL_ANDANDEQU = 179,                /* "&&="  */
  YYSYMBOL_OROREQU = 180,                  /* "||="  */
  YYSYMBOL_XORXOREQU = 181,                /* "^^="  */
  YYSYMBOL_DOTDOT = 182,                   /* ".."  */
  YYSYMBOL_MTAG_E = 183,                   /* "$$"  */
  YYSYMBOL_MTAG_I = 184,                   /* "$i"  */
  YYSYMBOL_MTAG_V = 185,                   /* "$v"  */
  YYSYMBOL_MTAG_B = 186,                   /* "$b"  */
  YYSYMBOL_MTAG_A = 187,                   /* "$a"  */
  YYSYMBOL_MTAG_T = 188,                   /* "$t"  */
  YYSYMBOL_MTAG_C = 189,                   /* "$c"  */
  YYSYMBOL_MTAG_F = 190,                   /* "$f"  */
  YYSYMBOL_MTAG_DOTDOTDOT = 191,           /* "..."  */
  YYSYMBOL_INTEGER = 192,                  /* "integer constant"  */
  YYSYMBOL_LONG_INTEGER = 193,             /* "long integer constant"  */
  YYSYMBOL_UNSIGNED_INTEGER = 194,         /* "unsigned integer constant"  */
  YYSYMBOL_UNSIGNED_LONG_INTEGER = 195,    /* "unsigned long integer constant"  */
  YYSYMBOL_UNSIGNED_INT8 = 196,            /* "unsigned int8 constant"  */
  YYSYMBOL_DAS_FLOAT = 197,                /* "floating point constant"  */
  YYSYMBOL_DAS_FLOAT16_CONST = 198,        /* "float16 constant"  */
  YYSYMBOL_DOUBLE = 199,                   /* "double constant"  */
  YYSYMBOL_NAME = 200,                     /* "name"  */
  YYSYMBOL_DAS_EMIT_COMMA = 201,           /* "new line, comma"  */
  YYSYMBOL_DAS_EMIT_SEMICOLON = 202,       /* "new line, semicolon"  */
  YYSYMBOL_BEGIN_STRING = 203,             /* "start of the string"  */
  YYSYMBOL_STRING_CHARACTER = 204,         /* STRING_CHARACTER  */
  YYSYMBOL_STRING_CHARACTER_ESC = 205,     /* STRING_CHARACTER_ESC  */
  YYSYMBOL_END_STRING = 206,               /* "end of the string"  */
  YYSYMBOL_BEGIN_STRING_EXPR = 207,        /* "{"  */
  YYSYMBOL_END_STRING_EXPR = 208,          /* "}"  */
  YYSYMBOL_END_OF_READ = 209,              /* "end of failed eader macro"  */
  YYSYMBOL_210_ = 210,                     /* ','  */
  YYSYMBOL_211_ = 211,                     /* '='  */
  YYSYMBOL_212_ = 212,                     /* '?'  */
  YYSYMBOL_213_ = 213,                     /* ':'  */
  YYSYMBOL_214_ = 214,                     /* '|'  */
  YYSYMBOL_215_ = 215,                     /* '^'  */
  YYSYMBOL_216_ = 216,                     /* '&'  */
  YYSYMBOL_217_ = 217,                     /* '<'  */
  YYSYMBOL_218_ = 218,                     /* '>'  */
  YYSYMBOL_219_ = 219,                     /* '-'  */
  YYSYMBOL_220_ = 220,                     /* '+'  */
  YYSYMBOL_221_ = 221,                     /* '*'  */
  YYSYMBOL_222_ = 222,                     /* '/'  */
  YYSYMBOL_223_ = 223,                     /* '%'  */
  YYSYMBOL_UNARY_MINUS = 224,              /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 225,               /* UNARY_PLUS  */
  YYSYMBOL_226_ = 226,                     /* '~'  */
  YYSYMBOL_227_ = 227,                     /* '!'  */
  YYSYMBOL_PRE_INC = 228,                  /* PRE_INC  */
  YYSYMBOL_PRE_DEC = 229,                  /* PRE_DEC  */
  YYSYMBOL_LLPIPE = 230,                   /* LLPIPE  */
  YYSYMBOL_POST_INC = 231,                 /* POST_INC  */
  YYSYMBOL_POST_DEC = 232,                 /* POST_DEC  */
  YYSYMBOL_DEREF = 233,                    /* DEREF  */
  YYSYMBOL_234_ = 234,                     /* '.'  */
  YYSYMBOL_235_ = 235,                     /* '['  */
  YYSYMBOL_236_ = 236,                     /* ']'  */
  YYSYMBOL_237_ = 237,                     /* '('  */
  YYSYMBOL_238_ = 238,                     /* ')'  */
  YYSYMBOL_239_ = 239,                     /* '$'  */
  YYSYMBOL_240_ = 240,                     /* '@'  */
  YYSYMBOL_241_ = 241,                     /* ';'  */
  YYSYMBOL_242_ = 242,                     /* '{'  */
  YYSYMBOL_243_ = 243,                     /* '}'  */
  YYSYMBOL_244_ = 244,                     /* '#'  */
  YYSYMBOL_YYACCEPT = 245,                 /* $accept  */
  YYSYMBOL_program = 246,                  /* program  */
  YYSYMBOL_COMMA = 247,                    /* COMMA  */
  YYSYMBOL_SEMICOLON = 248,                /* SEMICOLON  */
  YYSYMBOL_top_level_reader_macro = 249,   /* top_level_reader_macro  */
  YYSYMBOL_optional_public_or_private_module = 250, /* optional_public_or_private_module  */
  YYSYMBOL_module_name = 251,              /* module_name  */
  YYSYMBOL_optional_not_required = 252,    /* optional_not_required  */
  YYSYMBOL_module_declaration = 253,       /* module_declaration  */
  YYSYMBOL_character_sequence = 254,       /* character_sequence  */
  YYSYMBOL_string_constant = 255,          /* string_constant  */
  YYSYMBOL_format_string = 256,            /* format_string  */
  YYSYMBOL_optional_format_string = 257,   /* optional_format_string  */
  YYSYMBOL_258_1 = 258,                    /* $@1  */
  YYSYMBOL_string_builder_body = 259,      /* string_builder_body  */
  YYSYMBOL_string_builder = 260,           /* string_builder  */
  YYSYMBOL_reader_character_sequence = 261, /* reader_character_sequence  */
  YYSYMBOL_expr_reader = 262,              /* expr_reader  */
  YYSYMBOL_263_2 = 263,                    /* $@2  */
  YYSYMBOL_options_declaration = 264,      /* options_declaration  */
  YYSYMBOL_require_declaration = 265,      /* require_declaration  */
  YYSYMBOL_require_module_name = 266,      /* require_module_name  */
  YYSYMBOL_optional_require_guard = 267,   /* optional_require_guard  */
  YYSYMBOL_require_module = 268,           /* require_module  */
  YYSYMBOL_is_public_module = 269,         /* is_public_module  */
  YYSYMBOL_expect_declaration = 270,       /* expect_declaration  */
  YYSYMBOL_expect_list = 271,              /* expect_list  */
  YYSYMBOL_expect_error = 272,             /* expect_error  */
  YYSYMBOL_expression_label = 273,         /* expression_label  */
  YYSYMBOL_expression_goto = 274,          /* expression_goto  */
  YYSYMBOL_elif_or_static_elif = 275,      /* elif_or_static_elif  */
  YYSYMBOL_emit_semis = 276,               /* emit_semis  */
  YYSYMBOL_optional_emit_semis = 277,      /* optional_emit_semis  */
  YYSYMBOL_expression_else = 278,          /* expression_else  */
  YYSYMBOL_279_3 = 279,                    /* $@3  */
  YYSYMBOL_280_4 = 280,                    /* $@4  */
  YYSYMBOL_if_or_static_if = 281,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 282, /* expression_else_one_liner  */
  YYSYMBOL_expression_if_one_liner = 283,  /* expression_if_one_liner  */
  YYSYMBOL_semis = 284,                    /* semis  */
  YYSYMBOL_optional_semis = 285,           /* optional_semis  */
  YYSYMBOL_expression_if_block = 286,      /* expression_if_block  */
  YYSYMBOL_287_5 = 287,                    /* $@5  */
  YYSYMBOL_288_6 = 288,                    /* $@6  */
  YYSYMBOL_289_7 = 289,                    /* $@7  */
  YYSYMBOL_expression_else_block = 290,    /* expression_else_block  */
  YYSYMBOL_291_8 = 291,                    /* $@8  */
  YYSYMBOL_292_9 = 292,                    /* $@9  */
  YYSYMBOL_293_10 = 293,                   /* $@10  */
  YYSYMBOL_expression_if_then_else = 294,  /* expression_if_then_else  */
  YYSYMBOL_295_11 = 295,                   /* $@11  */
  YYSYMBOL_296_12 = 296,                   /* $@12  */
  YYSYMBOL_expression_if_then_else_oneliner = 297, /* expression_if_then_else_oneliner  */
  YYSYMBOL_for_variable_name_with_pos_list = 298, /* for_variable_name_with_pos_list  */
  YYSYMBOL_expression_for_loop = 299,      /* expression_for_loop  */
  YYSYMBOL_300_13 = 300,                   /* $@13  */
  YYSYMBOL_expression_unsafe = 301,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 302,    /* expression_while_loop  */
  YYSYMBOL_303_14 = 303,                   /* $@14  */
  YYSYMBOL_with_keyword_on = 304,          /* with_keyword_on  */
  YYSYMBOL_expression_with = 305,          /* expression_with  */
  YYSYMBOL_expression_with_alias = 306,    /* expression_with_alias  */
  YYSYMBOL_annotation_argument_value = 307, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 308, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 309, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 310,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 311, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 312,   /* metadata_argument_list  */
  YYSYMBOL_optional_for_annotations = 313, /* optional_for_annotations  */
  YYSYMBOL_annotation_declaration_name = 314, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 315, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 316,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 317,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 318, /* optional_annotation_list  */
  YYSYMBOL_optional_annotation_list_with_emit_semis = 319, /* optional_annotation_list_with_emit_semis  */
  YYSYMBOL_optional_function_argument_list = 320, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 321,   /* optional_function_type  */
  YYSYMBOL_function_name = 322,            /* function_name  */
  YYSYMBOL_das_type_name = 323,            /* das_type_name  */
  YYSYMBOL_optional_template = 324,        /* optional_template  */
  YYSYMBOL_global_function_declaration = 325, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 326, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 327, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 328,     /* function_declaration  */
  YYSYMBOL_329_15 = 329,                   /* $@15  */
  YYSYMBOL_expression_block_finally = 330, /* expression_block_finally  */
  YYSYMBOL_331_16 = 331,                   /* $@16  */
  YYSYMBOL_332_17 = 332,                   /* $@17  */
  YYSYMBOL_expression_block = 333,         /* expression_block  */
  YYSYMBOL_334_18 = 334,                   /* $@18  */
  YYSYMBOL_335_19 = 335,                   /* $@19  */
  YYSYMBOL_expr_call_pipe_no_bracket = 336, /* expr_call_pipe_no_bracket  */
  YYSYMBOL_expression_any = 337,           /* expression_any  */
  YYSYMBOL_338_20 = 338,                   /* $@20  */
  YYSYMBOL_339_21 = 339,                   /* $@21  */
  YYSYMBOL_expressions = 340,              /* expressions  */
  YYSYMBOL_optional_expr_list = 341,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_map_tuple_list = 342, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 343, /* type_declaration_no_options_list  */
  YYSYMBOL_name_in_namespace = 344,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 345,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 346,     /* new_type_declaration  */
  YYSYMBOL_347_22 = 347,                   /* $@22  */
  YYSYMBOL_348_23 = 348,                   /* $@23  */
  YYSYMBOL_expr_new = 349,                 /* expr_new  */
  YYSYMBOL_expression_break = 350,         /* expression_break  */
  YYSYMBOL_expression_continue = 351,      /* expression_continue  */
  YYSYMBOL_expression_return = 352,        /* expression_return  */
  YYSYMBOL_expression_yield = 353,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 354,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 355,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 356,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 357,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 358,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 359, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 360,           /* expression_let  */
  YYSYMBOL_expr_cast = 361,                /* expr_cast  */
  YYSYMBOL_362_24 = 362,                   /* $@24  */
  YYSYMBOL_363_25 = 363,                   /* $@25  */
  YYSYMBOL_364_26 = 364,                   /* $@26  */
  YYSYMBOL_365_27 = 365,                   /* $@27  */
  YYSYMBOL_366_28 = 366,                   /* $@28  */
  YYSYMBOL_367_29 = 367,                   /* $@29  */
  YYSYMBOL_expr_type_decl = 368,           /* expr_type_decl  */
  YYSYMBOL_369_30 = 369,                   /* $@30  */
  YYSYMBOL_370_31 = 370,                   /* $@31  */
  YYSYMBOL_expr_type_info = 371,           /* expr_type_info  */
  YYSYMBOL_expr_list = 372,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 373,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 374,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 375,            /* capture_entry  */
  YYSYMBOL_capture_list = 376,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 377,    /* optional_capture_list  */
  YYSYMBOL_expr_full_block = 378,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 379, /* expr_full_block_assumed_piped  */
  YYSYMBOL_expr_numeric_const = 380,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign_no_bracket = 381,   /* expr_assign_no_bracket  */
  YYSYMBOL_expr_named_call = 382,          /* expr_named_call  */
  YYSYMBOL_expr_method_call_no_bracket = 383, /* expr_method_call_no_bracket  */
  YYSYMBOL_func_addr_name = 384,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 385,           /* func_addr_expr  */
  YYSYMBOL_386_32 = 386,                   /* $@32  */
  YYSYMBOL_387_33 = 387,                   /* $@33  */
  YYSYMBOL_388_34 = 388,                   /* $@34  */
  YYSYMBOL_389_35 = 389,                   /* $@35  */
  YYSYMBOL_expr_field_no_bracket = 390,    /* expr_field_no_bracket  */
  YYSYMBOL_391_36 = 391,                   /* $@36  */
  YYSYMBOL_392_37 = 392,                   /* $@37  */
  YYSYMBOL_expr_call = 393,                /* expr_call  */
  YYSYMBOL_expr = 394,                     /* expr  */
  YYSYMBOL_expr_no_bracket = 395,          /* expr_no_bracket  */
  YYSYMBOL_396_38 = 396,                   /* $@38  */
  YYSYMBOL_397_39 = 397,                   /* $@39  */
  YYSYMBOL_398_40 = 398,                   /* $@40  */
  YYSYMBOL_399_41 = 399,                   /* $@41  */
  YYSYMBOL_400_42 = 400,                   /* $@42  */
  YYSYMBOL_401_43 = 401,                   /* $@43  */
  YYSYMBOL_402_44 = 402,                   /* $@44  */
  YYSYMBOL_403_45 = 403,                   /* $@45  */
  YYSYMBOL_404_46 = 404,                   /* $@46  */
  YYSYMBOL_405_47 = 405,                   /* $@47  */
  YYSYMBOL_406_48 = 406,                   /* $@48  */
  YYSYMBOL_407_49 = 407,                   /* $@49  */
  YYSYMBOL_408_50 = 408,                   /* $@50  */
  YYSYMBOL_409_51 = 409,                   /* $@51  */
  YYSYMBOL_expr_generator = 410,           /* expr_generator  */
  YYSYMBOL_expr_mtag_no_bracket = 411,     /* expr_mtag_no_bracket  */
  YYSYMBOL_optional_field_annotation = 412, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 413,        /* optional_override  */
  YYSYMBOL_optional_constant = 414,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 415, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 416, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 417, /* structure_variable_declaration  */
  YYSYMBOL_struct_variable_declaration_list = 418, /* struct_variable_declaration_list  */
  YYSYMBOL_419_52 = 419,                   /* $@52  */
  YYSYMBOL_420_53 = 420,                   /* $@53  */
  YYSYMBOL_421_54 = 421,                   /* $@54  */
  YYSYMBOL_function_argument_declaration_no_type = 422, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 423, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 424,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 425,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 426,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 427,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 428,             /* variant_type  */
  YYSYMBOL_variant_type_list = 429,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 430,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 431,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 432, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 433, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 434,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 435,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 436,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 437, /* let_variable_name_with_pos_list  */
  YYSYMBOL_global_let_variable_name_with_pos_list = 438, /* global_let_variable_name_with_pos_list  */
  YYSYMBOL_variable_declaration_list = 439, /* variable_declaration_list  */
  YYSYMBOL_let_variable_declaration = 440, /* let_variable_declaration  */
  YYSYMBOL_global_let_variable_declaration = 441, /* global_let_variable_declaration  */
  YYSYMBOL_optional_shared = 442,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 443, /* optional_public_or_private_variable  */
  YYSYMBOL_global_variable_declaration_list = 444, /* global_variable_declaration_list  */
  YYSYMBOL_445_55 = 445,                   /* $@55  */
  YYSYMBOL_global_let = 446,               /* global_let  */
  YYSYMBOL_447_56 = 447,                   /* $@56  */
  YYSYMBOL_enum_expression = 448,          /* enum_expression  */
  YYSYMBOL_commas = 449,                   /* commas  */
  YYSYMBOL_enum_list = 450,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 451, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 452,             /* single_alias  */
  YYSYMBOL_453_57 = 453,                   /* $@57  */
  YYSYMBOL_alias_declaration = 454,        /* alias_declaration  */
  YYSYMBOL_distinct_alias = 455,           /* distinct_alias  */
  YYSYMBOL_456_58 = 456,                   /* $@58  */
  YYSYMBOL_optional_public_or_private_enum = 457, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 458,                /* enum_name  */
  YYSYMBOL_optional_enum_basic_type_declaration = 459, /* optional_enum_basic_type_declaration  */
  YYSYMBOL_optional_commas = 460,          /* optional_commas  */
  YYSYMBOL_emit_commas = 461,              /* emit_commas  */
  YYSYMBOL_optional_emit_commas = 462,     /* optional_emit_commas  */
  YYSYMBOL_enum_declaration = 463,         /* enum_declaration  */
  YYSYMBOL_464_59 = 464,                   /* $@59  */
  YYSYMBOL_465_60 = 465,                   /* $@60  */
  YYSYMBOL_466_61 = 466,                   /* $@61  */
  YYSYMBOL_optional_structure_parent = 467, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 468,          /* optional_sealed  */
  YYSYMBOL_structure_name = 469,           /* structure_name  */
  YYSYMBOL_class_or_struct = 470,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 471, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 472, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 473,    /* structure_declaration  */
  YYSYMBOL_474_62 = 474,                   /* $@62  */
  YYSYMBOL_475_63 = 475,                   /* $@63  */
  YYSYMBOL_476_64 = 476,                   /* $@64  */
  YYSYMBOL_variable_name_with_pos_list = 477, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 478,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 479, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 480, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 481,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 482,            /* bitfield_bits  */
  YYSYMBOL_bitfield_alias_bits = 483,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_basic_type_declaration = 484, /* bitfield_basic_type_declaration  */
  YYSYMBOL_bitfield_type_declaration = 485, /* bitfield_type_declaration  */
  YYSYMBOL_486_65 = 486,                   /* $@65  */
  YYSYMBOL_487_66 = 487,                   /* $@66  */
  YYSYMBOL_c_or_s = 488,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 489,          /* table_type_pair  */
  YYSYMBOL_dim_list = 490,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 491, /* type_declaration_no_options  */
  YYSYMBOL_optional_expr_list_in_braces = 492, /* optional_expr_list_in_braces  */
  YYSYMBOL_type_declaration_no_options_no_dim = 493, /* type_declaration_no_options_no_dim  */
  YYSYMBOL_494_67 = 494,                   /* $@67  */
  YYSYMBOL_495_68 = 495,                   /* $@68  */
  YYSYMBOL_496_69 = 496,                   /* $@69  */
  YYSYMBOL_497_70 = 497,                   /* $@70  */
  YYSYMBOL_498_71 = 498,                   /* $@71  */
  YYSYMBOL_499_72 = 499,                   /* $@72  */
  YYSYMBOL_500_73 = 500,                   /* $@73  */
  YYSYMBOL_501_74 = 501,                   /* $@74  */
  YYSYMBOL_502_75 = 502,                   /* $@75  */
  YYSYMBOL_503_76 = 503,                   /* $@76  */
  YYSYMBOL_504_77 = 504,                   /* $@77  */
  YYSYMBOL_505_78 = 505,                   /* $@78  */
  YYSYMBOL_506_79 = 506,                   /* $@79  */
  YYSYMBOL_507_80 = 507,                   /* $@80  */
  YYSYMBOL_508_81 = 508,                   /* $@81  */
  YYSYMBOL_509_82 = 509,                   /* $@82  */
  YYSYMBOL_510_83 = 510,                   /* $@83  */
  YYSYMBOL_511_84 = 511,                   /* $@84  */
  YYSYMBOL_512_85 = 512,                   /* $@85  */
  YYSYMBOL_513_86 = 513,                   /* $@86  */
  YYSYMBOL_514_87 = 514,                   /* $@87  */
  YYSYMBOL_515_88 = 515,                   /* $@88  */
  YYSYMBOL_516_89 = 516,                   /* $@89  */
  YYSYMBOL_517_90 = 517,                   /* $@90  */
  YYSYMBOL_518_91 = 518,                   /* $@91  */
  YYSYMBOL_519_92 = 519,                   /* $@92  */
  YYSYMBOL_520_93 = 520,                   /* $@93  */
  YYSYMBOL_521_94 = 521,                   /* $@94  */
  YYSYMBOL_type_declaration = 522,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 523,  /* tuple_alias_declaration  */
  YYSYMBOL_524_95 = 524,                   /* $@95  */
  YYSYMBOL_525_96 = 525,                   /* $@96  */
  YYSYMBOL_526_97 = 526,                   /* $@97  */
  YYSYMBOL_527_98 = 527,                   /* $@98  */
  YYSYMBOL_variant_alias_declaration = 528, /* variant_alias_declaration  */
  YYSYMBOL_529_99 = 529,                   /* $@99  */
  YYSYMBOL_530_100 = 530,                  /* $@100  */
  YYSYMBOL_531_101 = 531,                  /* $@101  */
  YYSYMBOL_532_102 = 532,                  /* $@102  */
  YYSYMBOL_bitfield_alias_declaration = 533, /* bitfield_alias_declaration  */
  YYSYMBOL_534_103 = 534,                  /* $@103  */
  YYSYMBOL_535_104 = 535,                  /* $@104  */
  YYSYMBOL_536_105 = 536,                  /* $@105  */
  YYSYMBOL_537_106 = 537,                  /* $@106  */
  YYSYMBOL_make_decl = 538,                /* make_decl  */
  YYSYMBOL_make_decl_no_bracket = 539,     /* make_decl_no_bracket  */
  YYSYMBOL_make_struct_fields = 540,       /* make_struct_fields  */
  YYSYMBOL_make_variant_dim = 541,         /* make_variant_dim  */
  YYSYMBOL_make_struct_single = 542,       /* make_struct_single  */
  YYSYMBOL_make_struct_dim_list = 543,     /* make_struct_dim_list  */
  YYSYMBOL_make_struct_dim_decl = 544,     /* make_struct_dim_decl  */
  YYSYMBOL_optional_make_struct_dim_decl = 545, /* optional_make_struct_dim_decl  */
  YYSYMBOL_use_initializer = 546,          /* use_initializer  */
  YYSYMBOL_make_struct_decl = 547,         /* make_struct_decl  */
  YYSYMBOL_548_107 = 548,                  /* $@107  */
  YYSYMBOL_549_108 = 549,                  /* $@108  */
  YYSYMBOL_550_109 = 550,                  /* $@109  */
  YYSYMBOL_551_110 = 551,                  /* $@110  */
  YYSYMBOL_552_111 = 552,                  /* $@111  */
  YYSYMBOL_553_112 = 553,                  /* $@112  */
  YYSYMBOL_554_113 = 554,                  /* $@113  */
  YYSYMBOL_555_114 = 555,                  /* $@114  */
  YYSYMBOL_556_115 = 556,                  /* $@115  */
  YYSYMBOL_557_116 = 557,                  /* $@116  */
  YYSYMBOL_make_tuple_call = 558,          /* make_tuple_call  */
  YYSYMBOL_559_117 = 559,                  /* $@117  */
  YYSYMBOL_560_118 = 560,                  /* $@118  */
  YYSYMBOL_make_dim_decl = 561,            /* make_dim_decl  */
  YYSYMBOL_562_119 = 562,                  /* $@119  */
  YYSYMBOL_563_120 = 563,                  /* $@120  */
  YYSYMBOL_564_121 = 564,                  /* $@121  */
  YYSYMBOL_565_122 = 565,                  /* $@122  */
  YYSYMBOL_566_123 = 566,                  /* $@123  */
  YYSYMBOL_567_124 = 567,                  /* $@124  */
  YYSYMBOL_568_125 = 568,                  /* $@125  */
  YYSYMBOL_569_126 = 569,                  /* $@126  */
  YYSYMBOL_570_127 = 570,                  /* $@127  */
  YYSYMBOL_571_128 = 571,                  /* $@128  */
  YYSYMBOL_expr_map_tuple_list = 572,      /* expr_map_tuple_list  */
  YYSYMBOL_push_table_nesting = 573,       /* push_table_nesting  */
  YYSYMBOL_make_table_decl = 574,          /* make_table_decl  */
  YYSYMBOL_make_table_call = 575,          /* make_table_call  */
  YYSYMBOL_array_comprehension_where = 576, /* array_comprehension_where  */
  YYSYMBOL_optional_comma = 577,           /* optional_comma  */
  YYSYMBOL_table_comprehension = 578,      /* table_comprehension  */
  YYSYMBOL_array_comprehension = 579       /* array_comprehension  */
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
         || (defined DAS2_YYLTYPE_IS_TRIVIAL && DAS2_YYLTYPE_IS_TRIVIAL \
             && defined DAS2_YYSTYPE_IS_TRIVIAL && DAS2_YYSTYPE_IS_TRIVIAL)))

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
#define YYLAST   12759

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  245
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  335
/* YYNRULES -- Number of rules.  */
#define YYNRULES  1052
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1878

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   472


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
       2,     2,     2,   227,     2,   244,   239,   223,   216,     2,
     237,   238,   221,   220,   210,   219,   234,   222,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   213,   241,
     217,   211,   218,   212,   240,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   235,     2,   236,   215,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   242,   214,   243,   226,     2,     2,     2,
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
     205,   206,   207,   208,   209,   224,   225,   228,   229,   230,
     231,   232,   233
};

#if DAS2_YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   605,   605,   606,   611,   612,   613,   614,   615,   616,
     617,   618,   619,   620,   621,   622,   623,   627,   628,   632,
     633,   637,   643,   644,   645,   649,   650,   654,   655,   659,
     678,   679,   680,   681,   685,   686,   690,   691,   695,   696,
     696,   700,   705,   714,   729,   745,   750,   758,   758,   803,
     821,   825,   828,   832,   836,   840,   844,   850,   859,   860,
     864,   867,   870,   876,   877,   881,   885,   886,   890,   893,
     899,   905,   908,   914,   915,   919,   920,   924,   925,   929,
     930,   930,   934,   934,   943,   944,   948,   949,   955,   956,
     957,   958,   959,   963,   964,   968,   969,   973,   975,   973,
     987,   987,   995,   997,   995,  1009,  1009,  1017,  1019,  1017,
    1030,  1037,  1044,  1049,  1058,  1066,  1072,  1076,  1084,  1094,
    1094,  1103,  1111,  1111,  1127,  1133,  1140,  1159,  1163,  1170,
    1171,  1172,  1173,  1174,  1175,  1179,  1184,  1192,  1193,  1194,
    1195,  1199,  1200,  1201,  1202,  1203,  1204,  1205,  1206,  1207,
    1213,  1216,  1222,  1225,  1231,  1234,  1237,  1243,  1244,  1245,
    1246,  1250,  1268,  1291,  1294,  1304,  1319,  1334,  1349,  1352,
    1359,  1363,  1370,  1371,  1375,  1376,  1380,  1381,  1382,  1386,
    1390,  1394,  1401,  1405,  1406,  1407,  1408,  1409,  1410,  1411,
    1412,  1413,  1414,  1415,  1416,  1417,  1418,  1419,  1420,  1421,
    1422,  1423,  1424,  1425,  1426,  1427,  1428,  1429,  1430,  1431,
    1432,  1433,  1434,  1435,  1436,  1437,  1438,  1439,  1440,  1441,
    1442,  1443,  1444,  1445,  1446,  1447,  1448,  1449,  1450,  1451,
    1452,  1453,  1454,  1455,  1456,  1457,  1458,  1459,  1460,  1461,
    1462,  1463,  1464,  1465,  1466,  1467,  1468,  1469,  1470,  1471,
    1472,  1473,  1474,  1475,  1476,  1477,  1478,  1479,  1480,  1481,
    1482,  1483,  1484,  1485,  1486,  1487,  1488,  1489,  1490,  1491,
    1492,  1493,  1494,  1498,  1499,  1500,  1501,  1502,  1503,  1504,
    1505,  1506,  1507,  1508,  1509,  1510,  1511,  1512,  1513,  1514,
    1515,  1516,  1517,  1518,  1519,  1520,  1521,  1522,  1523,  1524,
    1525,  1526,  1527,  1528,  1529,  1530,  1531,  1532,  1533,  1534,
    1535,  1536,  1537,  1538,  1539,  1540,  1541,  1542,  1543,  1544,
    1545,  1549,  1550,  1554,  1573,  1574,  1575,  1579,  1585,  1585,
    1602,  1605,  1607,  1605,  1619,  1621,  1619,  1636,  1654,  1672,
    1690,  1701,  1702,  1703,  1704,  1705,  1706,  1707,  1708,  1709,
    1710,  1711,  1712,  1713,  1714,  1715,  1716,  1717,  1718,  1719,
    1720,  1722,  1720,  1737,  1742,  1748,  1754,  1755,  1759,  1760,
    1764,  1768,  1775,  1776,  1787,  1791,  1794,  1802,  1802,  1802,
    1805,  1811,  1814,  1818,  1822,  1829,  1836,  1842,  1846,  1850,
    1853,  1856,  1864,  1867,  1875,  1881,  1882,  1883,  1887,  1888,
    1892,  1893,  1897,  1902,  1910,  1917,  1929,  1933,  1936,  1946,
    1946,  1946,  1949,  1949,  1949,  1954,  1954,  1954,  1962,  1962,
    1962,  1968,  1978,  1989,  2004,  2007,  2010,  2013,  2019,  2020,
    2028,  2040,  2041,  2042,  2046,  2047,  2048,  2049,  2050,  2054,
    2059,  2067,  2068,  2072,  2079,  2083,  2090,  2091,  2092,  2093,
    2094,  2095,  2096,  2097,  2101,  2102,  2103,  2104,  2105,  2106,
    2107,  2108,  2109,  2110,  2111,  2112,  2113,  2114,  2115,  2116,
    2117,  2118,  2119,  2120,  2121,  2122,  2123,  2124,  2125,  2126,
    2130,  2136,  2143,  2155,  2161,  2169,  2177,  2188,  2200,  2204,
    2211,  2214,  2214,  2214,  2219,  2219,  2219,  2232,  2236,  2240,
    2244,  2250,  2258,  2264,  2272,  2280,  2291,  2300,  2306,  2314,
    2314,  2314,  2321,  2325,  2334,  2342,  2350,  2354,  2357,  2365,
    2366,  2367,  2374,  2375,  2376,  2377,  2378,  2379,  2380,  2381,
    2382,  2383,  2384,  2385,  2386,  2387,  2388,  2389,  2390,  2391,
    2392,  2393,  2394,  2395,  2396,  2397,  2398,  2399,  2400,  2401,
    2402,  2403,  2404,  2405,  2406,  2407,  2408,  2409,  2415,  2416,
    2417,  2418,  2419,  2434,  2443,  2444,  2445,  2446,  2447,  2448,
    2449,  2450,  2451,  2452,  2453,  2454,  2455,  2456,  2457,  2457,
    2457,  2465,  2466,  2467,  2472,  2475,  2475,  2475,  2478,  2483,
    2487,  2487,  2487,  2492,  2499,  2505,  2509,  2509,  2509,  2514,
    2517,  2523,  2523,  2523,  2530,  2535,  2539,  2539,  2539,  2544,
    2547,  2553,  2553,  2553,  2560,  2565,  2566,  2567,  2568,  2569,
    2570,  2571,  2572,  2573,  2575,  2579,  2580,  2585,  2591,  2597,
    2606,  2609,  2612,  2621,  2622,  2623,  2624,  2625,  2626,  2627,
    2631,  2635,  2639,  2643,  2647,  2651,  2655,  2659,  2663,  2668,
    2672,  2677,  2681,  2686,  2693,  2694,  2698,  2699,  2700,  2704,
    2705,  2709,  2710,  2711,  2715,  2716,  2720,  2732,  2735,  2736,
    2740,  2740,  2759,  2758,  2773,  2772,  2789,  2801,  2810,  2820,
    2821,  2822,  2823,  2824,  2828,  2831,  2840,  2841,  2845,  2848,
    2852,  2865,  2874,  2875,  2879,  2882,  2886,  2899,  2900,  2904,
    2909,  2914,  2922,  2925,  2932,  2935,  2941,  2942,  2943,  2947,
    2948,  2952,  2959,  2964,  2973,  2979,  2983,  2994,  3001,  3010,
    3013,  3016,  3023,  3027,  3033,  3044,  3047,  3052,  3063,  3064,
    3068,  3069,  3070,  3074,  3077,  3080,  3080,  3100,  3103,  3103,
    3121,  3126,  3134,  3135,  3139,  3142,  3155,  3172,  3173,  3174,
    3179,  3179,  3205,  3206,  3213,  3213,  3235,  3236,  3237,  3241,
    3251,  3254,  3260,  3261,  3265,  3266,  3270,  3271,  3275,  3277,
    3282,  3275,  3298,  3299,  3303,  3304,  3308,  3314,  3315,  3316,
    3317,  3321,  3322,  3323,  3327,  3330,  3336,  3338,  3343,  3336,
    3364,  3371,  3376,  3385,  3391,  3395,  3406,  3407,  3408,  3409,
    3410,  3411,  3412,  3413,  3414,  3415,  3416,  3417,  3418,  3419,
    3420,  3421,  3422,  3423,  3424,  3425,  3426,  3427,  3428,  3429,
    3430,  3431,  3432,  3433,  3434,  3435,  3436,  3437,  3438,  3439,
    3440,  3441,  3442,  3443,  3444,  3445,  3446,  3447,  3448,  3449,
    3450,  3451,  3452,  3453,  3454,  3455,  3459,  3460,  3461,  3462,
    3463,  3464,  3465,  3466,  3470,  3481,  3485,  3492,  3504,  3511,
    3517,  3526,  3531,  3541,  3551,  3561,  3574,  3575,  3576,  3577,
    3578,  3582,  3586,  3586,  3586,  3600,  3601,  3605,  3610,  3617,
    3620,  3623,  3626,  3632,  3635,  3649,  3650,  3654,  3655,  3656,
    3657,  3658,  3658,  3658,  3662,  3667,  3674,  3681,  3681,  3688,
    3688,  3695,  3699,  3703,  3708,  3713,  3718,  3723,  3727,  3731,
    3736,  3740,  3744,  3749,  3749,  3749,  3755,  3762,  3762,  3762,
    3767,  3767,  3767,  3773,  3773,  3773,  3778,  3784,  3784,  3784,
    3789,  3789,  3789,  3798,  3804,  3804,  3804,  3809,  3809,  3809,
    3818,  3824,  3824,  3824,  3829,  3829,  3829,  3838,  3838,  3838,
    3844,  3844,  3844,  3853,  3856,  3867,  3883,  3885,  3890,  3895,
    3883,  3921,  3923,  3928,  3934,  3921,  3960,  3962,  3967,  3972,
    3960,  4013,  4014,  4015,  4016,  4017,  4018,  4019,  4023,  4024,
    4025,  4026,  4027,  4031,  4038,  4045,  4051,  4057,  4064,  4071,
    4077,  4086,  4089,  4095,  4103,  4108,  4115,  4120,  4126,  4127,
    4131,  4132,  4136,  4136,  4136,  4144,  4144,  4144,  4151,  4151,
    4151,  4161,  4161,  4161,  4168,  4168,  4168,  4179,  4185,  4185,
    4185,  4198,  4217,  4217,  4217,  4227,  4227,  4227,  4240,  4240,
    4240,  4253,  4262,  4262,  4262,  4282,  4289,  4289,  4289,  4299,
    4302,  4313,  4319,  4342,  4350,  4370,  4395,  4396,  4400,  4401,
    4406,  4409,  4419
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
  "\"!=\"", "\"->\"", "\"<-\"", "\"??\"", "\"?.\"", "\"?[\"", "\"!.\"",
  "\"!?.\"", "\"![\"", "\"!?[\"", "\"!??\"", "\"!==\"", "\"!<-\"",
  "\"!:=\"", "\"!is\"", "\"!as\"", "\"!?as\"", "\"<|\"", "\"|>\"",
  "\":=\"", "\"<<<\"", "\">>>\"", "\"<<<=\"", "\">>>=\"", "\"=>\"",
  "\"@@\"", "\"@field\"", "\"::\"", "\"&&\"", "\"||\"", "\"^^\"",
  "\"&&=\"", "\"||=\"", "\"^^=\"", "\"..\"", "\"$$\"", "\"$i\"", "\"$v\"",
  "\"$b\"", "\"$a\"", "\"$t\"", "\"$c\"", "\"$f\"", "\"...\"",
  "\"integer constant\"", "\"long integer constant\"",
  "\"unsigned integer constant\"", "\"unsigned long integer constant\"",
  "\"unsigned int8 constant\"", "\"floating point constant\"",
  "\"float16 constant\"", "\"double constant\"", "\"name\"",
  "\"new line, comma\"", "\"new line, semicolon\"",
  "\"start of the string\"", "STRING_CHARACTER", "STRING_CHARACTER_ESC",
  "\"end of the string\"", "\"{\"", "\"}\"",
  "\"end of failed eader macro\"", "','", "'='", "'?'", "':'", "'|'",
  "'^'", "'&'", "'<'", "'>'", "'-'", "'+'", "'*'", "'/'", "'%'",
  "UNARY_MINUS", "UNARY_PLUS", "'~'", "'!'", "PRE_INC", "PRE_DEC",
  "LLPIPE", "POST_INC", "POST_DEC", "DEREF", "'.'", "'['", "']'", "'('",
  "')'", "'$'", "'@'", "';'", "'{'", "'}'", "'#'", "$accept", "program",
  "COMMA", "SEMICOLON", "top_level_reader_macro",
  "optional_public_or_private_module", "module_name",
  "optional_not_required", "module_declaration", "character_sequence",
  "string_constant", "format_string", "optional_format_string", "$@1",
  "string_builder_body", "string_builder", "reader_character_sequence",
  "expr_reader", "$@2", "options_declaration", "require_declaration",
  "require_module_name", "optional_require_guard", "require_module",
  "is_public_module", "expect_declaration", "expect_list", "expect_error",
  "expression_label", "expression_goto", "elif_or_static_elif",
  "emit_semis", "optional_emit_semis", "expression_else", "$@3", "$@4",
  "if_or_static_if", "expression_else_one_liner",
  "expression_if_one_liner", "semis", "optional_semis",
  "expression_if_block", "$@5", "$@6", "$@7", "expression_else_block",
  "$@8", "$@9", "$@10", "expression_if_then_else", "$@11", "$@12",
  "expression_if_then_else_oneliner", "for_variable_name_with_pos_list",
  "expression_for_loop", "$@13", "expression_unsafe",
  "expression_while_loop", "$@14", "with_keyword_on", "expression_with",
  "expression_with_alias", "annotation_argument_value",
  "annotation_argument_value_list", "annotation_argument_name",
  "annotation_argument", "annotation_argument_list",
  "metadata_argument_list", "optional_for_annotations",
  "annotation_declaration_name", "annotation_declaration_basic",
  "annotation_declaration", "annotation_list", "optional_annotation_list",
  "optional_annotation_list_with_emit_semis",
  "optional_function_argument_list", "optional_function_type",
  "function_name", "das_type_name", "optional_template",
  "global_function_declaration", "optional_public_or_private_function",
  "function_declaration_header", "function_declaration", "$@15",
  "expression_block_finally", "$@16", "$@17", "expression_block", "$@18",
  "$@19", "expr_call_pipe_no_bracket", "expression_any", "$@20", "$@21",
  "expressions", "optional_expr_list", "optional_expr_map_tuple_list",
  "type_declaration_no_options_list", "name_in_namespace",
  "expression_delete", "new_type_declaration", "$@22", "$@23", "expr_new",
  "expression_break", "expression_continue", "expression_return",
  "expression_yield", "expression_try_catch", "kwd_let_var_or_nothing",
  "kwd_let", "optional_in_scope", "tuple_expansion",
  "tuple_expansion_variable_declaration", "expression_let", "expr_cast",
  "$@24", "$@25", "$@26", "$@27", "$@28", "$@29", "expr_type_decl", "$@30",
  "$@31", "expr_type_info", "expr_list", "block_or_simple_block",
  "block_or_lambda", "capture_entry", "capture_list",
  "optional_capture_list", "expr_full_block",
  "expr_full_block_assumed_piped", "expr_numeric_const",
  "expr_assign_no_bracket", "expr_named_call",
  "expr_method_call_no_bracket", "func_addr_name", "func_addr_expr",
  "$@32", "$@33", "$@34", "$@35", "expr_field_no_bracket", "$@36", "$@37",
  "expr_call", "expr", "expr_no_bracket", "$@38", "$@39", "$@40", "$@41",
  "$@42", "$@43", "$@44", "$@45", "$@46", "$@47", "$@48", "$@49", "$@50",
  "$@51", "expr_generator", "expr_mtag_no_bracket",
  "optional_field_annotation", "optional_override", "optional_constant",
  "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "struct_variable_declaration_list", "$@52", "$@53", "$@54",
  "function_argument_declaration_no_type",
  "function_argument_declaration_type", "function_argument_list",
  "tuple_type", "tuple_type_list", "tuple_alias_type_list", "variant_type",
  "variant_type_list", "variant_alias_type_list", "copy_or_move",
  "variable_declaration_no_type", "variable_declaration_type",
  "variable_declaration", "copy_or_move_or_clone", "optional_ref",
  "let_variable_name_with_pos_list",
  "global_let_variable_name_with_pos_list", "variable_declaration_list",
  "let_variable_declaration", "global_let_variable_declaration",
  "optional_shared", "optional_public_or_private_variable",
  "global_variable_declaration_list", "$@55", "global_let", "$@56",
  "enum_expression", "commas", "enum_list",
  "optional_public_or_private_alias", "single_alias", "$@57",
  "alias_declaration", "distinct_alias", "$@58",
  "optional_public_or_private_enum", "enum_name",
  "optional_enum_basic_type_declaration", "optional_commas", "emit_commas",
  "optional_emit_commas", "enum_declaration", "$@59", "$@60", "$@61",
  "optional_structure_parent", "optional_sealed", "structure_name",
  "class_or_struct", "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@62", "$@63", "$@64", "variable_name_with_pos_list",
  "basic_type_declaration", "enum_basic_type_declaration",
  "structure_type_declaration", "auto_type_declaration", "bitfield_bits",
  "bitfield_alias_bits", "bitfield_basic_type_declaration",
  "bitfield_type_declaration", "$@65", "$@66", "c_or_s", "table_type_pair",
  "dim_list", "type_declaration_no_options",
  "optional_expr_list_in_braces", "type_declaration_no_options_no_dim",
  "$@67", "$@68", "$@69", "$@70", "$@71", "$@72", "$@73", "$@74", "$@75",
  "$@76", "$@77", "$@78", "$@79", "$@80", "$@81", "$@82", "$@83", "$@84",
  "$@85", "$@86", "$@87", "$@88", "$@89", "$@90", "$@91", "$@92", "$@93",
  "$@94", "type_declaration", "tuple_alias_declaration", "$@95", "$@96",
  "$@97", "$@98", "variant_alias_declaration", "$@99", "$@100", "$@101",
  "$@102", "bitfield_alias_declaration", "$@103", "$@104", "$@105",
  "$@106", "make_decl", "make_decl_no_bracket", "make_struct_fields",
  "make_variant_dim", "make_struct_single", "make_struct_dim_list",
  "make_struct_dim_decl", "optional_make_struct_dim_decl",
  "use_initializer", "make_struct_decl", "$@107", "$@108", "$@109",
  "$@110", "$@111", "$@112", "$@113", "$@114", "$@115", "$@116",
  "make_tuple_call", "$@117", "$@118", "make_dim_decl", "$@119", "$@120",
  "$@121", "$@122", "$@123", "$@124", "$@125", "$@126", "$@127", "$@128",
  "expr_map_tuple_list", "push_table_nesting", "make_table_decl",
  "make_table_call", "array_comprehension_where", "optional_comma",
  "table_comprehension", "array_comprehension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1705)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-945)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1705,   212, -1705, -1705,    77,  -121,  -127,   806, -1705,    18,
   -1705, -1705, -1705, -1705,   593,   131, -1705, -1705, -1705, -1705,
      86,    86,    86, -1705,   366, -1705,    75, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,   -18, -1705,
      13,    71,   146, -1705,   -99,   334, -1705, -1705, -1705,   175,
      86,    86, -1705, -1705,    75,   806,   806,   806,   231,   288,
   -1705, -1705, -1705, -1705,   131,   131,   131,   211, -1705,   828,
     325, -1705, -1705, -1705, -1705,   344, -1705,   621, -1705,   814,
      48,    77,   389,  -121,   337, -1705,   367,   351,   307,   388,
      52,   459, -1705, -1705,   857,   478,   486,   525, -1705,   532,
     416, -1705, -1705,   -89,    77,   131,   131,   131,   131,   564,
   -1705,   858,   861,   572,   666,   865, -1705, -1705,   536, -1705,
   -1705,   620, -1705, -1705, -1705,   829,   169, -1705, -1705, -1705,
   -1705,   -99,   -99,   627,   -99,   673,   692,   642,   725, -1705,
   -1705, -1705,   670, -1705, -1705,   686,   717,   564,   564, -1705,
   -1705,   736, -1705,   180, -1705,    22,   771,   828, -1705,   757,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705,   769, -1705, -1705,
   -1705, -1705, -1705, -1705,   798, -1705, -1705, -1705, -1705,   832,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705,   199,   627,   627,
     627, -1705, -1705,   926,   926,   784, 10811,   934, -1705,   676,
     825, -1705, -1705, -1705, -1705, -1705, 12257, -1705,   797,   899,
     -10,    77,   868,   839, -1705, -1705, -1705,   169, -1705, -1705,
   -1705, 10811,   843,   844,   846,   827,   849,   850, -1705, -1705,
   -1705,   836, -1705, -1705, -1705, -1705, -1705,   507, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,   859,
   -1705, -1705, -1705,   862,   863, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705,   867,   869,   841,   593,   187, -1705, -1705, -1705,
   -1705,   248,   852,   871, -1705, -1705, -1705, -1705, -1705, -1705,
     887, -1705,   848,   854,  2071, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705,  1056,  1057, -1705,   860, -1705,   564,  1154,
     825, -1705,   894,   564, -1705, -1705,   798,   564,    77, -1705,
     649, -1705, -1705,   871, -1705, -1705, -1705,  9376, -1705, -1705,
     898,   882,   -27,   133,   208, -1705, -1705,  9376,   205, -1705,
    6868, -1705, -1705, -1705,    11, -1705, -1705, -1705,    23, -1705,
    7096,   873, 10465, -1705,   864, -1705, -1705, 12385, 12505, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
     901,   874, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705,  1080, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,   903,   877,
   -1705, -1705,   -64,   -68,   -94, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705,   880,   896, -1705,   -69, -1705,   564,
     923, 10811, -1705,   -42, 10811, 10811, 10811,   907,   913, -1705,
   -1705,    36,   593,   914,    32, -1705,   206,   895,   916,   919,
     313,   921,   902,   378,   925, -1705,   427,    47,   928, 10282,
   10282,   363,   910,   912,   915,   917,   918,   930, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, 10282, 10282,
   10282, 10282, 10282,  4360,  5272, -1705, -1705, -1705, -1705, -1705,
   -1705,   931, -1705, -1705, -1705, -1705,   935, -1705, -1705,   -87,
     -87, -1705,   -87,   -87,   936, 10970, -1705, -1705,   938, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, 10811, 10811,   939,
     932, 10811,   860, 10811,   860, 10811,   860, 10981,   951,   942,
   -1705,  6868, -1705, 10811,  9376,   943,   985, -1705, -1705, -1705,
   -1705, -1705,   937, -1705, -1705,   964,  7324, -1705,   248, -1705,
   10981,   951, -1705, -1705, -1705, -1705, -1705, -1705, 12559,   774,
    1466,   959, -1705,   109,   958,   157,   967, 10811, 10811, -1705,
    9830, -1705, -1705, -1705, -1705,   593, -1705,   691,   970,  1110,
     681, -1705, -1705, -1705,   250, -1705, -1705, -1705,  9376,   314,
     565,   992,   446, -1705, -1705, -1705, -1705,   974, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705,   557, -1705,  1001,
    1003,  1004, -1705,  6868, 10811,  9376,  9376, -1705, -1705, -1705,
    9376, -1705,  9376, -1705,  6868, -1705, -1705,  6868,  1010, -1705,
   10811,  1060,  1060,   997,  1002,   238, -1705, -1705,  9376,  9376,
    9376,  9376,  9376,  9376,   635,  1060,  1060,   454,  1060,  1060,
    1005,  1225,  1008,  1006,   333,   985,  1030,  1011,   564,  3904,
     131,  1237, -1705, -1705,   935, -1705, -1705, -1705, -1705, -1705,
   11712, 11767, 10282, 10282, -1705, -1705, 10282, 10282, 10282, 10282,
    1045, 10282,   608,  9376,   636,   643,  9376,  9376, 10282, 11897,
   11952, 12082, 10282, 10282, 10282, 10282,  9376, 10282, 10282, 10282,
   10282, 10056, 10282, 10282, 10282, 10282, 10282, 10282, 10282, 10282,
   10282, 10282, 12329,  9376,  5500,   669,   713, -1705, -1705,  1046,
     721,   -68,   724,   -68,   742,   -68,   -93, -1705,   302,   871,
    1037, -1705,   335, -1705, 10811,   985,   467,   871, -1705, -1705,
    7552, -1705, -1705, -1705, -1705,  1020,  1063, -1705,    86, -1705,
      86, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,  9376,
   -1705, -1705,   296,   -48,   -48,   -48, -1705,   871,   871, 10282,
   11093, -1705,  1066, -1705, -1705, -1705, -1705,  9376,  1067,   382,
   10811,   -42, -1705,  9376,    86, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, 10811, 10811, 10811, 10811,  4588,  1069,  9376, 10811,
   -1705, -1705, -1705, 10811,   985,   103, -1705,  1070,  1048, 10811,
   10811, 10811,  1052, 10811,  1053, 10811,   985, 10811, 10981,   985,
   -1705,   951,   359,  9376,  9376, 10811,   860,  1055,  1058,  1059,
    1061,  1062,  1064, -1705,  9376,   809,   116,  1071, -1705,  9376,
   -1705,  9376, -1705,  9376,  1068,   644, -1705, -1705,  7780,   117,
    4132, -1705,   200,  1072,   430,  1074,   860,  2340,  1237,  1081,
    1075, -1705, -1705,  1084,  1076, -1705, -1705,   824,   824,  1119,
    1119, 11547, 11547,  1077,   595,  1082, -1705,  1073,  1085,  1086,
    1087, -1705,  1089,  1090,   595,  1112,  1093, -1705, -1705,  1114,
    1098, -1705, -1705,  1120,  1099, -1705, -1705,   482,   482,   938,
     824,   824, 11093, -1705, -1705, 11370,  1312, 11280, 11093, 12137,
    1774, 10628, 10473, 11460,  1119,  1119,   963,   963,   595,   595,
     595,   648,  9376,  1106,  1107,   657,  9376,  1303,  1108,  1091,
   -1705,   220, -1705, -1705, -1705,   123, -1705,  1129, -1705,  1130,
   -1705,  1131, 10811, -1705, 10981, 10811, -1705,   951,   480,  1118,
    1113, 10811,  9376, -1705, -1705,  1096,   470, -1705, 10639, -1705,
     143, -1705,  1123,  1121,  1314, -1705, -1705,   149, -1705, -1705,
   -1705, 11190,  2583,  1151, -1705,   470,    30,  1125, -1705,  1127,
    1323,   250,  9376,    86, -1705, -1705, -1705, -1705,   871,   380,
     523,   743,   664,   247,  1133,  1134,   513,  1135,   744, 10811,
   10981,   951,   856,  1136,  1138, 10811,  9376,  1140, -1705,   960,
    1170,  1204, -1705,  1218, -1705,  1299,  1141,  1365,   563,  1142,
   10811,   614,  1237,  1146,  1149,  1411,   -68, -1705, -1705, -1705,
   -1705, -1705,  1139,  1177,  1157,  1348,  1195,    25,   116,  1159,
   -1705, -1705, -1705,  1162,   162,  1163,  1158,  1070,   185, -1705,
    1164,   490,  5728, -1705, -1705, -1705,   -60,   -68, -1705,  8008,
   -1705,  1165,  8236,  1206,  1211, -1705,    86,  1212,  8464,    -7,
    8692, -1705, -1705, -1705,    86,    86,  1405, -1705,  1007, -1705,
   -1705,  1406, -1705, -1705,  1409,  1379, -1705,    86, -1705,    86,
      86,    86,    86,    86, -1705,  1358, -1705,    86,  2705,   860,
   -1705,  9376, -1705,  9376,  4816,  9376, -1705,  9376,  5956,  9376,
   -1705, -1705, -1705,  9376, -1705,  9376, -1705,  9376,  1214,  1191,
   -1705, -1705, 10282,  1192, -1705,  1196,  9376,  5044,  1198, -1705,
    1197, -1705,  6184, -1705,  7552, -1705, -1705, -1705,  1238, -1705,
    1239, -1705, -1705, -1705, -1705, -1705, -1705,   871, -1705, -1705,
     871, -1705, -1705,  1113, -1705, -1705,   871, -1705,  9376, -1705,
      -4, -1705, -1705, -1705,  1199, -1705,  1202, -1705,  9376,  1241,
     606, 10811, -1705,  9376,  1213,  9376,   613, -1705,  1247, -1705,
   -1705,  1441,   798, -1705,  9376,  1273, -1705,  9376,    86, -1705,
   -1705, -1705, -1705,  1244, -1705, -1705, -1705,  1242,  1283, -1705,
   -1705,  1489,   617,   678, -1705, -1705,  9376,  1507, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,  1547,
   -1705,   112, -1705, -1705, -1705,  1266,  6412, -1705,  1277,  9376,
    1286, -1705,   277,  6868,   270,    27,   244,  9376,  9376,  9376,
     116, -1705, -1705, -1705,   644,  1249,  4132,   304,  1293,  1294,
    1259,  1297,  1298, -1705,   336,   564,  9376, -1705,  1485,  9376,
   -1705,  1289,  1290, -1705,  1291,  1310, -1705,  1165,  9376, -1705,
   -1705, -1705, -1705,  1268, -1705, -1705,  1269,   -57,   -57,  1270,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705,   -95, -1705, 10282,
   10282, 10282, 10282, 10282, 10282, 10282, 10282, 10282, 10282,  9376,
   10282,  9376, 10282, 10282, 10282, 10282, 10282, 10282, 10282, 10282,
     -68, 10811,  1265, 10811,  1271,  4132, -1705,   341,   362,  1275,
    1276, -1705,   375,  1281, 10811,  1282, 10811,  1285, 10811,  1287,
   -1705,  9376, 11190,  9376, -1705,  1300,  4132, -1705,   381,   387,
    9376, -1705, -1705, -1705,   408, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705,  1313, -1705,  1296, -1705, -1705,  1304, -1705,
    1307,  1493,   -70, -1705,  1500, -1705, -1705,  1302,  1330,   866,
    1481,    86,  1315, -1705,    86, -1705,  1318,  1319, -1705, -1705,
    9376,  1334, -1705, -1705, -1705, -1705,  1320,  1321,  1324,  1326,
    1327,  1329,  1332,  1335,  1490,  1337, -1705,  1339,  8920,  1165,
     534, -1705, -1705,   411, -1705, -1705,  1333, -1705,  1378, -1705,
     412,  1342,  1539,  1195,  6868,  9376,  9376,  1347, -1705, -1705,
      28, -1705,   509, -1705, -1705, -1705,  1386, -1705, -1705,   -60,
   -1705,   -94, -1705,  1165, -1705, 10811,  9376, -1705, -1705, -1705,
   -1705,  2936,  9376,  9376,    77,   868,  1350,  1351,  9148,  1195,
   -1705, -1705,   -69, 10970, 10970, 10970, 10970, 10970, 10970, 10970,
   10970, 10970, 10970, 10970, -1705, -1705, 10970, 10970, -1705, -1705,
   10970, 10970, 10970, 10970, 10970, 10970, 10970, 10970,   564,  1620,
   -1705,   750, -1705,   533,  6640, -1705, -1705, -1705, -1705, -1705,
   -1705,  1754, -1705,   753, -1705,   756, -1705, 10811,  1352,  1353,
   -1705,   546,  6640, -1705, -1705,  1354, -1705,  9376, -1705, -1705,
    9376,  1389,  9376, -1705, -1705, -1705, 10811, -1705, -1705,   273,
   -1705,    38, -1705, -1705, -1705,  1490,  1490,  1355,  1357,  1359,
    1368,  1373,  6868, -1705,  9376,  9376,  9376,  9376,  9376,  6868,
   -1705, -1705,  1490,  1374,  1490, -1705,  1375, -1705, -1705,   534,
   -1705,  1391, -1705, -1705,  1371,  9376,  1415,   419,   420, -1705,
   -1705,   315,  6868,  1380,  1381, -1705, -1705, -1705,   871, -1705,
    1377,  1384,  1394,   553,   116,  9376,   -99,  1396,   435,    91,
   -1705,   -94, -1705, -1705,  1397,   466, -1705, -1705, -1705,   780,
   -1705, -1705,  1400,   473, -1705, -1705,  1412, -1705, -1705,  1382,
    -111,  1584,    38, -1705, -1705,   866,   249,   249, -1705,  9376,
    1490,  1490,   664,  1413,  1414,  1418,  1419,  1421,  1427,   985,
     249,  1490,   664, -1705, -1705, -1705,  9376,  1428, -1705, -1705,
    1402,  9376,  9376,   491, -1705, -1705,  1500,  1632,   564, -1705,
      49,  1429,   554,   564,   126, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705,  1584,   296,   664,  1439,  1458, -1705,
    1433,  1435,  1437,   249,   249,  1439,  1447, -1705, -1705, -1705,
   -1705, -1705, -1705,  1449,  1450,   664,  1451,  1438, -1705,  9376,
   -1705, -1705,  1456, -1705,  9604,    86, -1705,  6868,   564,   564,
    1165, 10811,   -42, -1705,  3179, 12257, -1705, -1705, -1705, -1705,
     495,  1454, -1705, -1705, -1705, -1705,  1461,  1462, -1705, -1705,
   -1705,  1464, -1705,  1639,  1468,  1438,  9376, -1705, -1705, -1705,
   -1705, -1705, 10970, -1705,  1467,   500,  1165,  1165, -1705,   265,
    9376,  1463,    86, 12257, -1705,   664, -1705, -1705, -1705,  9376,
   -1705,  1474,  1438, -1705,   788,  9604,   564, -1705, -1705,  9376,
      86, -1705, -1705,   564,   502, -1705, -1705,  1469, -1705,   564,
   -1705, -1705,  1476, -1705,    86,  1165,    86, -1705,   -94, -1705,
   -1705,  3422, -1705,  9376, -1705, -1705, -1705, -1705,  1471,  1473,
    1478,  1500, -1705, -1705,  9604,   564, -1705, -1705,    86, -1705,
    3665, -1705,  1473,  1475,   788,  1500, -1705, -1705
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   174,     1,   398,     0,     0,    58,   747,   399,     0,
     966,   956,   961,    20,     0,     0,    19,    16,    15,     3,
       0,     0,     0,     8,   786,     7,   728,     6,    11,     5,
       4,    13,    12,    14,   138,   139,   140,   137,   148,   150,
      49,    68,    65,    66,     0,     0,    50,   749,   748,     0,
       0,     0,    26,    25,   728,   747,   747,   747,     0,   372,
      47,   158,   159,   160,     0,     0,     0,   161,   163,   170,
       0,   157,    21,    10,     9,   321,   768,     0,   729,   730,
       0,     0,     0,     0,     0,    51,     0,     0,    59,     0,
      63,   750,   752,   753,    22,     0,     0,     0,   374,     0,
       0,   169,   164,     0,     0,     0,     0,     0,     0,    77,
     322,   324,   756,   778,   777,   781,   732,   731,   738,   146,
     147,     0,   144,   145,   142,     0,     0,   141,   151,    69,
      67,     0,     0,    52,     0,     0,     0,     0,     0,    64,
      60,   754,     0,    23,    24,    27,   866,    77,    77,   373,
      45,    48,   168,     0,   165,   166,   167,   171,    75,    78,
     175,   326,   325,   328,   323,   758,   757,     0,   780,   779,
     783,   782,   787,   733,   654,   143,    30,    31,    35,     0,
     133,   134,   131,   132,   130,   129,   135,     0,    54,    55,
      53,    57,    56,    63,    63,     0,     0,     0,    29,     0,
     766,   957,   962,    46,   162,    76,     0,   759,   760,   774,
     735,     0,   655,     0,    32,    33,    34,     0,   149,    62,
      61,     0,     0,     0,     0,     0,     0,     0,   796,   839,
     797,   855,   798,   802,   803,   804,   805,   845,   809,   810,
     811,   812,   813,   814,   815,   840,   841,   842,   843,   926,
     801,   808,   844,   933,   940,   799,   806,   800,   807,   816,
     817,   818,   819,   820,   821,   822,   823,   824,   825,   826,
     827,   828,   829,   830,   831,   832,   833,   834,   835,   836,
     837,   838,     0,     0,     0,     0,   854,   887,   890,   888,
     889,   953,   883,   751,    28,   869,   870,   867,   868,   764,
     767,   967,     0,     0,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,   319,   320,     0,     0,   182,   176,   272,    77,     0,
     766,   775,     0,    77,   737,   734,   654,    77,     0,   717,
     709,   739,   136,   755,   891,   917,   920,     0,   923,   913,
       0,     0,   927,   934,   941,   947,   950,     0,   885,   897,
     366,   903,   908,   902,     0,   916,   912,   905,     0,   907,
       0,   884,     0,   765,     0,   958,   963,   263,   264,   261,
     187,   188,   190,   189,   191,   192,   193,   194,   220,   221,
     218,   219,   211,   222,   223,   212,   209,   210,   185,   262,
     245,     0,   260,   224,   225,   226,   227,   198,   199,   200,
     195,   196,   197,   208,   184,     0,   214,   215,   213,   206,
     207,   202,   201,   203,   204,   205,   186,   183,   244,     0,
     216,   217,   654,   179,     0,   846,   849,   852,   853,   847,
     850,   848,   851,   761,     0,   772,   788,     0,   152,    77,
       0,     0,   710,     0,     0,     0,     0,     0,     0,   528,
     529,     0,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   845,     0,     0,     0,     0,
       0,   433,     0,     0,     0,     0,     0,     0,   638,   446,
     448,   447,   449,   450,   451,   452,   453,    41,     0,     0,
       0,     0,     0,   366,     0,   431,   432,  1041,   526,   525,
     625,   523,   618,   617,   616,   615,   172,   621,   524,   620,
     619,   573,   530,   574,     0,   519,   581,   531,     0,   527,
     978,   980,   979,   520,   982,   981,   521,     0,     0,     0,
     872,     0,   176,     0,   176,     0,   176,     0,     0,     0,
     899,     0,   896,     0,     0,     0,  1048,   424,   910,   911,
     904,   906,     0,   909,   880,     0,     0,   955,   954,   968,
     688,   694,   265,   267,   266,   268,   259,   243,   269,   246,
     228,     0,   177,   397,   679,   680,     0,     0,     0,   327,
       0,   334,   428,   329,   769,     0,   776,     0,     0,   711,
     709,   736,   153,   718,     0,   707,   708,   706,     0,     0,
       0,     0,   877,  1002,  1005,   377,   854,   381,   380,   386,
     971,   977,   972,   973,   974,   976,   975,     0,   418,     0,
       0,     0,  1032,     0,     0,     0,     0,   409,   412,   578,
       0,   415,     0,  1036,     0,  1014,  1018,     0,     0,  1008,
       0,   558,   559,     0,     0,   491,   488,   490,     0,     0,
       0,     0,     0,     0,     0,   535,   534,   575,   533,   532,
       0,     0,     0,     0,   372,  1048,  1048,     0,    77,     0,
       0,   441,   433,   363,   172,   340,   338,   339,   337,   894,
       0,     0,     0,     0,   560,   561,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   509,     0,     0,     0,     0,   856,   871,     0,
       0,   179,     0,   179,     0,   179,   372,   686,     0,   684,
       0,   692,     0,   857,     0,  1048,     0,   370,   425,   895,
    1049,   367,   901,   879,   882,     0,   861,   689,    95,   695,
      95,   270,   271,   248,   249,   251,   250,   252,   253,   254,
     255,   247,   256,   257,   258,   232,   233,   235,   234,   236,
     237,   238,   239,   230,   231,   240,   241,   242,   229,     0,
     395,   396,     0,   654,   654,   654,   178,   181,   180,     0,
     429,   363,   744,   773,   784,   667,   789,     0,     0,     0,
       0,     0,   725,     0,     0,   892,   918,   921,    18,    17,
     875,   876,     0,     0,     0,     0,  1000,     0,     0,     0,
    1022,  1025,  1028,     0,  1048,     0,  1039,  1048,     0,     0,
       0,     0,     0,     0,     0,     0,  1048,     0,     0,  1048,
    1011,     0,     0,     0,     0,     0,   176,     0,     0,     0,
       0,     0,     0,    44,     0,    42,     0,     0,  1021,     0,
     698,     0,   697,     0,     0,  1049,   993,   563,   368,     0,
     366,   512,     0,     0,     0,     0,   176,     0,   441,     0,
       0,   589,   588,     0,     0,   595,   599,   536,   537,   549,
     550,   547,   548,     0,   582,     0,   570,     0,     0,   499,
       0,   572,     0,     0,   583,     0,     0,   594,   593,     0,
       0,   600,   604,     0,     0,   610,   614,   622,   623,   624,
     538,   539,   627,   628,   629,   554,   555,   556,   557,     0,
       0,   552,   553,   551,   545,   546,   541,   540,   542,   543,
     544,     0,     0,     0,   497,     0,     0,     0,     0,     0,
     517,     0,   924,   914,   858,     0,   928,     0,   935,     0,
     942,     0,     0,   948,     0,     0,   951,     0,     0,     0,
     885,     0,     0,   426,   881,   862,   762,    93,    96,   959,
      96,   964,     0,     0,   790,   676,   677,   699,   681,   683,
     682,   430,     0,   740,   745,   762,   670,     0,   713,     0,
     714,     0,     0,     0,   727,   893,   919,   922,   878,     0,
       0,     0,  1001,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1049,     0,   576,     0,
       0,     0,   577,     0,   626,     0,     0,     0,     0,     0,
       0,     0,   441,     0,     0,     0,   179,   633,   634,   635,
     636,   637,     0,    38,     0,   111,     0,     0,     0,     0,
     984,   983,   562,     0,     0,     0,     0,  1048,     0,   513,
       0,     0,     0,   516,   514,   173,     0,   179,   365,   389,
     387,     0,     0,     0,     0,   388,     0,     0,     0,    77,
       0,   360,   445,   341,     0,     0,     0,   354,     0,   355,
     349,     0,   346,   345,     0,     0,   347,     0,   364,     0,
      91,    92,    89,    90,   356,   401,   344,     0,   454,   176,
     585,     0,   596,     0,     0,     0,   567,     0,     0,     0,
     566,   569,   590,     0,   601,     0,   611,     0,     0,     0,
     605,   609,     0,     0,   571,     0,     0,     0,     0,   498,
       0,   510,     0,   564,     0,   518,   925,   915,     0,   873,
       0,   929,   931,   936,   938,   943,   945,   685,   949,   687,
     691,   952,   693,   885,   886,   898,   371,   427,     0,   742,
     763,   969,    94,   690,     0,   696,     0,   678,     0,     0,
       0,     0,   700,     0,     0,     0,   763,   770,     0,   668,
     785,     0,   654,   712,     0,     0,   722,     0,     0,   726,
    1003,  1006,   378,     0,   383,   384,   382,     0,     0,   421,
     419,     0,     0,     0,  1033,  1031,   368,     0,  1040,  1043,
     410,   413,   579,   416,  1037,  1035,  1015,  1019,  1017,     0,
    1009,    77,   489,   653,   492,     0,     0,    39,     0,     0,
       0,   402,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1042,   369,   515,     0,     0,   366,     0,     0,     0,
       0,     0,     0,   439,     0,    77,     0,   390,     0,     0,
     375,     0,     0,   359,     0,     0,    72,     0,     0,   392,
     363,   357,   358,     0,    84,    85,     0,   154,   154,     0,
     348,   343,   350,   351,   352,   353,   400,   654,   342,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     179,     0,     0,     0,     0,   366,   483,     0,     0,     0,
       0,   502,     0,     0,     0,     0,     0,     0,     0,     0,
     606,     0,   584,     0,   568,     0,   366,   500,     0,     0,
       0,   565,   511,   507,     0,   860,   874,   859,   932,   939,
     946,   900,   863,   864,   743,     0,   960,   965,     0,   792,
       0,   793,   702,   701,   330,   741,   746,     0,     0,   661,
     664,     0,     0,   716,     0,   724,     0,     0,   379,   385,
       0,     0,   420,  1023,  1026,  1029,     0,     0,     0,     0,
       0,     0,     0,     0,  1000,     0,  1012,     0,     0,     0,
       0,   495,   639,     0,    36,    43,     0,   113,     0,   114,
       0,     0,   115,     0,     0,     0,     0,     0,   986,   985,
       0,   480,     0,   482,   436,   437,     0,   435,   434,     0,
     442,     0,   391,     0,   376,     0,     0,    70,    71,   121,
     393,     0,     0,     0,     0,   156,     0,     0,     0,     0,
     719,   407,     0,   471,   472,   474,   473,   475,   465,   466,
     467,   476,   477,   456,   457,   458,   460,   461,   462,   463,
     464,   459,   478,   479,   468,   469,   470,   455,    77,     0,
     651,     0,   647,     0,     0,   484,   487,   642,   644,   503,
     646,     0,   652,     0,   648,     0,   650,     0,     0,     0,
     641,     0,     0,   501,   506,     0,   508,     0,   970,   791,
       0,     0,     0,   331,   336,   771,     0,   662,   663,   664,
     665,   656,   671,   715,   723,  1000,  1000,     0,     0,     0,
       0,     0,   366,  1044,   368,     0,     0,     0,     0,     0,
    1001,  1016,  1000,     0,  1000,   630,     0,   632,   493,     0,
     640,    40,   112,   403,     0,     0,     0,     0,     0,   988,
     987,     0,     0,     0,     0,   440,   443,   394,   128,   127,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     406,     0,   586,   597,     0,     0,   591,   602,   612,     0,
     649,   645,     0,     0,   643,   865,     0,   795,   703,     0,
       0,   659,   656,   657,   658,   661,   999,   999,   422,     0,
    1000,  1000,   991,     0,     0,     0,     0,     0,     0,  1048,
     999,  1000,   991,   631,   496,    37,     0,     0,   117,   118,
       0,     0,     0,     0,   481,   438,   330,    86,    77,   155,
       0,     0,     0,    77,   709,   408,   720,   721,   444,   587,
     598,   485,   486,   592,   603,   613,   607,   504,   505,   794,
     363,   669,   660,   672,   659,     0,     0,   996,  1048,   998,
       0,     0,     0,   999,   999,   992,     0,  1034,  1045,   411,
     414,   580,   417,     0,     0,   991,     0,  1046,   116,     0,
     990,   989,     0,   362,     0,     0,   108,     0,    77,    77,
       0,     0,     0,   608,     0,     0,   674,   705,   704,   666,
       0,  1049,   997,  1004,  1007,   423,     0,     0,  1030,  1038,
    1020,     0,  1010,     0,     0,  1046,     0,    87,    91,    92,
      89,    90,    88,   110,   100,     0,     0,     0,   125,     0,
       0,     0,     0,     0,   994,     0,  1024,  1027,  1013,     0,
    1050,     0,  1046,    97,    79,     0,    77,   123,   126,     0,
       0,   333,   673,    77,     0,  1047,  1051,     0,   363,    77,
      73,    74,     0,   109,     0,     0,     0,   405,     0,   995,
    1052,     0,    80,     0,   101,   120,   404,   675,     0,   105,
       0,   330,   102,    81,     0,    77,    99,   363,     0,    82,
       0,   106,   105,     0,    79,   330,    83,   104
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1705, -1705,  -983,    -1, -1705, -1705, -1705, -1705, -1705,  1017,
    1637, -1705, -1705, -1705, -1705, -1705, -1705,  1719, -1705, -1705,
   -1705,   -32, -1705, -1705,   837, -1705, -1705,  1638, -1705, -1705,
   -1705, -1705,  -143,  -150, -1705, -1705, -1705, -1705, -1704,   929,
     940, -1705, -1705, -1705, -1705,  -144, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1096, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705,  1513, -1705, -1705,   -45,  -101,  -299,   383, -1705,
   -1705,   588,  1022,  1021,   688,  -546,  -748, -1705,  -332, -1705,
   -1705, -1705, -1485, -1705, -1705, -1574, -1705, -1705, -1115, -1705,
   -1705, -1705, -1705, -1705, -1705,  -827,  -383, -1241,   952,   -13,
   -1705, -1705, -1705, -1705, -1705, -1696, -1692, -1691, -1687, -1705,
   -1705,  1737, -1705, -1278, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705,  -475, -1472,   396,
     240, -1705,  -863, -1705,   472, -1705, -1705, -1705, -1705, -1412,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,   499,
    1132, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705,  -163,    69,     9,
      70,   158, -1705, -1705, -1705, -1705, -1705, -1705, -1705,   236,
    -578,  -790, -1705,  -584,  -845, -1705, -1003,    14,    15, -1705,
    -626,  -621, -1705, -1705, -1705, -1467, -1705,  1692, -1705, -1705,
   -1705, -1705, -1705,   508,   703, -1705,   998, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705,   710, -1705,  1401, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705,   -78, -1705,  1278, -1705, -1705, -1705,  1520,
   -1705, -1705, -1705,  -632, -1705, -1705,  -362,  -963, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705,  -190, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705,  -792, -1579,  -691, -1705, -1705,
   -1563, -1425,  1279, -1705, -1705, -1705, -1705, -1705, -1705, -1705,
   -1705, -1705, -1705,  1288, -1705, -1705,  1292, -1705, -1705, -1705,
   -1705, -1705, -1705, -1705, -1705, -1705, -1705,  1095, -1705,  -476,
    1295, -1605,  -673,  1301,  -467
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,   860,   861,    18,   145,    54,   198,    19,   179,
     185,  1621,  1298,  1474,   694,   538,   151,   539,   100,    21,
      22,    88,    45,    46,   140,    23,    42,    43,  1144,  1145,
    1842,   159,   160,  1843,  1859,  1872,  1346,  1765,  1146,  1028,
    1029,  1824,  1838,  1858,  1825,  1863,  1867,  1873,  1864,  1147,
    1148,  1804,  1149,  1107,  1150,  1151,  1152,  1153,  1154,  1155,
    1156,  1157,   186,   187,    38,    39,    40,   212,  1516,    67,
      68,    69,    70,   711,    24,   463,   619,   356,   357,   111,
      25,   163,   358,   164,   206,  1584,  1669,  1811,   622,   841,
    1244,   540,  1158,  1340,  1640,   927,   702,  1116,   786,   541,
    1159,   647,   865,  1448,   542,  1160,  1161,  1162,  1163,  1164,
     832,  1165,  1357,  1302,  1521,  1166,   543,   879,  1459,   880,
    1460,   883,  1462,   544,   869,  1452,   545,   586,   623,   546,
    1323,  1324,   926,   547,   715,   548,  1167,   549,   550,   687,
     551,   895,  1470,   896,  1619,   552,   997,  1412,   553,   587,
     555,   881,  1461,  1381,  1719,  1394,  1723,  1383,  1720,  1396,
    1724,  1567,  1773,  1398,  1725,   556,   557,   613,  1675,  1733,
    1589,  1591,  1441,  1046,  1252,  1775,  1813,   614,   615,   616,
     777,   778,   798,   781,   782,   800,   913,  1035,  1036,  1779,
     638,   483,   630,   370,  1649,   631,   371,    79,   118,   210,
     366,    27,   174,  1044,  1230,  1045,    49,    50,   142,    28,
      51,   195,   167,   208,   360,  1231,   300,   301,    29,   112,
     842,  1437,   626,   362,   363,   115,   172,   846,    30,    77,
     209,   627,  1037,   558,   473,   288,   289,  1005,  1026,   200,
     290,   769,  1416,  1014,   641,   401,   291,   582,   292,   484,
    1055,   583,   784,   568,  1207,   485,  1056,   486,  1057,   567,
    1206,   571,  1211,   572,  1418,   573,  1213,   574,  1419,   575,
    1215,   576,  1420,   577,  1218,   578,  1221,   779,    31,    56,
     302,   600,  1234,    32,    57,   303,   601,  1236,    33,    55,
     404,   796,  1425,   649,   559,   706,  1746,   707,  1738,  1739,
    1740,  1065,   560,   863,  1446,   864,  1447,   891,  1467,  1090,
    1613,   887,  1464,   561,   888,  1465,   562,  1069,  1599,  1070,
    1600,  1071,  1601,   873,  1456,   885,  1463,  1117,   708,   563,
     564,  1794,   791,   565,   566
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      17,    60,    71,   153,   201,   202,   293,   585,   853,   851,
     862,   213,  1305,    90,  1042,   653,  1328,   799,   923,    72,
      73,    74,   797,  1007,   656,  1009,   771,  1011,   773,  1636,
     775,   373,   914,   916,  1243,  1457,   128,   659,  -174,  1611,
     598,   487,   488,  1229,   588,  1303,  1091,  1484,  1632,    92,
      93,    71,    71,    71,   133,  1650,   590,  1225,  1618,   705,
    1797,   494,  1229,   119,   120,  1169,   678,   496,  1798,  1767,
    1248,    41,  1799,  1800,   138,   603,   605,  1801,   620,   211,
     910,   617,    99,    84,  1673,    44,   712,   105,   106,   107,
    1318,    13,    71,    71,    71,    71,    34,    35,  1088,   188,
     189,    85,   190,   402,   503,   504,   785,  1319,   635,   139,
     211,  1309,  1019,  1756,  1741,   628,   830,   211,   287,   634,
    1012,  1844,  -930,   611,    86,   636,   211,  1754,  1121,  1798,
      16,   629,  1763,  1799,  1800,    87,   391,    78,  1801,   611,
    1320,   912,  1519,   287,   402,   618,    36,  1520,   621,   152,
    1017,  1321,   535,   536,  1021,   713,  1322,   660,   661,   831,
    1868,   506,   507,   392,   393,    61,   367,  1674,  1798,   637,
    1676,  1677,  1799,  1800,   612,  1064,  1791,  1801,  1514,  1718,
    1786,  1787,  1717,   286,   180,   181,  -930,  1690,   874,  1692,
    1821,  -930,    13,    80,    62,   158,  1423,   858,   105,   886,
     107,  1073,   889,   477,  1077,  1627,   859,  1694,   286,   365,
    -930,    58,     2,  1086,  1490,   464,  1089,  1837,    52,     3,
     476,   121,  1509,    81,   478,  1052,  1273,   589,  1120,  1291,
     672,    16,  1249,   364,   922,  1304,    59,  1304,  1304,   591,
     122,  1648,     4,  1075,     5,   123,     6,  1424,   124,   662,
     394,   125,     7,   645,   395,  1743,  1744,    53,   592,  1304,
    1421,    63,     8,  1424,   679,    15,  1755,   593,     9,   663,
     973,   533,   388,  1250,   135,   628,   802,    37,   537,   974,
    1272,   391,  -937,   391,    82,   126,   136,  1866,    13,  1001,
    1812,   629,    10,    13,   639,   640,   642,    64,   391,   910,
    1104,  1877,   875,  1486,   858,    13,    58,   703,   392,   393,
     392,   393,   910,   859,   158,   396,  1105,  1118,   892,   397,
    1671,  1074,   398,   479,   287,   392,   393,    16,  1833,  1308,
    1317,    59,    16,  1208,  1715,   910,   632,    11,    12,  1771,
    1590,  1209,   482,   780,    16,    13,  -937,   399,  1295,  1468,
    1096,  -937,   911,  1106,  1617,  1119,    83,  -944,    65,  1240,
     912,   182,  1241,  1684,  1210,  1242,   183,   834,    66,   184,
    -937,  1263,   125,   912,    75,    91,  1857,   765,   766,  1325,
    1127,   770,  1388,   772,    16,   774,  1021,  -494,  1637,   286,
      81,  1063,   391,   787,   910,   394,   912,   394,   835,   395,
     635,   395,    76,   287,   389,  1409,   287,   287,   287,   217,
    1122,  1485,   394,   391,    13,   635,   395,   636,   204,   392,
     393,  -944,   580,   664,   390,  1257,  -944,   837,   838,  1582,
    1204,    98,   636,  1222,  1268,    14,  1219,   218,  1123,   703,
     392,   393,   581,   665,  1312,  -944,  1235,    15,   104,  1118,
    1233,  -494,    13,    16,  1481,   912,  -494,  1204,  1205,  1017,
     396,   637,   396,    99,   397,   910,   397,   398,   286,   398,
    1482,   286,   286,   286,   110,  -494,   637,   396,   646,   657,
    1033,   397,  1701,   910,   398,  1264,  1736,  1478,  1051,   287,
     287,    16,   399,   287,   399,   287,  1034,   287,   686,   287,
     911,  1059,  1060,   858,    13,   287,   394,  1483,    99,   399,
     395,  1072,   859,  1511,  1314,  1479,    84,  1079,  1080,  1081,
    1013,  1083,   287,  1085,  1492,  1087,   912,   394,   402,   135,
     669,   395,   855,  1095,    85,   108,   858,    13,    58,   287,
     287,   136,  1493,    16,   912,   859,  1499,   683,  1710,    84,
     670,  1554,   684,  1016,   286,   286,   391,    86,   286,   131,
     286,   109,   286,    59,   286,   918,  1049,    85,    87,    89,
     286,   396,  1314,   134,  1500,   397,    16,  1092,   398,  1555,
     685,   129,  1050,   392,   393,  1204,   287,   286,   137,   132,
      86,  1572,   396,  1553,   787,   673,   397,  1314,  1260,   398,
    1556,    87,   287,   399,   286,   286,   732,   733,   734,   735,
     736,   737,   843,  1559,  1571,   674,   720,   721,  1204,  1573,
     150,  1204,  1204,  1380,   399,  1574,   113,   114,  1702,  1478,
    1204,   730,  1548,   852,   732,   733,   734,   735,   736,   737,
     108,  1017,   932,   936,   676,  1478,  1576,   858,    13,  1620,
    1624,   286,   101,   102,   103,  1808,   859,  1699,  1700,   141,
     402,   958,   962,   966,   677,   969,  1125,   286,   858,    13,
     394,   858,  1058,  1714,   395,  1061,  1314,   859,   146,  1068,
     859,   858,    13,  1314,   998,  1020,   147,    16,   762,   763,
     859,  1827,  1828,   154,   155,   156,   157,    71,  1223,  1387,
    1314,  1204,   168,  1392,  1722,  1314,   287,  1271,    16,    58,
    1204,  1728,  1314,  1277,   858,    13,   762,   763,   683,  1314,
     199,    16,  1408,   859,  -866,   148,  1315,  1414,  1289,  1762,
    1855,  1267,   149,  1814,    59,   396,   724,   725,  1826,   397,
    1849,  1261,   398,  1314,   730,  1633,   731,   732,   733,   734,
     735,   736,   737,   738,    16,   295,  1314,   739,   740,   741,
     742,   743,  1655,    81,   858,    13,   158,   399,    58,  1654,
     296,   286,   287,   859,   867,   297,   135,   298,   173,   402,
    1663,  1287,  1662,   856,   287,   287,   287,   287,   136,  1709,
    1430,   287,  1769,    59,   868,   287,   169,  1027,   945,  1027,
    1839,   287,   287,   287,    16,   287,  1431,   287,   946,   287,
     287,  1840,  1841,  1043,   858,   858,    13,   287,   858,    13,
     175,  1473,  1217,   859,   859,  1220,   948,   859,  1480,   762,
     763,  1226,  1290,   950,  1113,  1454,   949,   286,  1193,   176,
     177,   903,   904,   951,  1114,   720,   721,  1198,  1194,   286,
     286,   286,   286,  1054,   703,    16,   286,  1199,    16,   480,
     286,   136,   481,    47,  1118,   482,   286,   286,   286,    48,
     286,   116,   286,   191,   286,   286,   554,   117,   193,   858,
      13,   196,   286,   402,  1737,  1737,   579,  1002,   859,   391,
    1745,   849,   192,  1534,   850,  1538,  1455,   482,  1737,   595,
    1745,  1191,  1535,  1774,  1539,   803,   804,   805,   806,   807,
     808,   809,   810,   197,   143,   161,   392,   393,   165,    16,
     144,   162,   170,  1587,   166,   194,  1143,   402,   171,  1588,
     199,  1003,   844,   845,   287,   402,   287,   287,   402,  1006,
     203,   811,  1008,   287,  1780,   714,   714,   105,   714,   714,
     287,  1737,  1737,   812,   813,   814,   402,   402,   402,   205,
    1010,  1262,  1270,  1745,   402,   724,   725,   402,  1653,   207,
     402,  1657,   211,   730,  1658,   731,   732,   733,   734,   735,
     736,   737,   738,   139,   720,   721,   739,   740,   741,   742,
     743,   287,   287,   391,   402,   221,  1337,   287,  1726,   286,
     294,   286,   286,   394,   105,   106,   107,   395,   286,  1628,
     359,  1851,   287,   214,   215,   286,  1753,  1344,  1345,  1549,
     392,   393,   716,  1834,   717,   718,   299,  1232,   361,  1232,
     219,   220,  1561,   176,   177,   178,   214,   215,   216,   369,
    1870,  1143,   368,   757,   758,   759,   760,   761,  1515,  1515,
    1256,  1432,  1259,    95,    96,    97,   286,   286,   762,   763,
     374,   375,   286,   376,   377,  1782,   378,   379,   396,  1038,
    1039,  1040,   397,   380,  1274,   398,   382,   286,   387,   383,
     384,   720,   721,   788,   385,   402,   386,   400,   403,  1440,
     405,   460,   461,  1772,   475,   795,   406,   462,   569,   570,
     399,   606,   608,   609,   724,   725,   599,   394,   596,   625,
     607,   395,   730,   610,   731,   732,   733,   734,   735,   736,
     737,   738,   624,   633,   643,   739,   740,   741,   742,   743,
     644,   658,   666,   667,  1689,  1333,   668,   854,   671,   672,
     720,   721,   675,  1341,  1342,   680,  1810,   688,  1469,   689,
     768,   780,   690,   848,   691,   692,  1350,  1703,  1351,  1352,
    1353,  1354,  1355,   287,   876,   878,  1358,   693,   709,   882,
     710,   884,   396,   792,   719,   764,   397,   767,  1280,   398,
     783,   789,  1501,  1829,   759,   760,   761,   897,   898,   899,
     900,   901,   902,  1551,  1522,   790,   829,   762,   763,   833,
     793,   724,   725,   391,   399,   836,  1563,   847,  1565,   730,
     857,   866,   732,   733,   734,   735,   736,   737,   870,  1683,
     871,   872,   739,   740,   741,   742,   743,   890,   286,   465,
     392,   393,   947,   466,   893,   952,   953,   391,   907,   894,
     915,   925,   906,   909,   908,   943,  1004,   467,   468,   917,
    1015,   391,   469,   470,   471,   472,  1024,  1445,   722,   723,
     724,   725,   999,  1025,   392,   393,  1043,  1048,   730,  1066,
     731,   732,   733,   734,   735,   736,   737,   738,   392,   393,
    1076,   739,   740,   741,   742,   743,  1078,   744,   745,  1023,
    1082,  1084,  1805,  1097,   762,   763,  1098,  1099,  1170,  1100,
    1101,  1172,  1102,   287,  1201,   287,  1112,  1228,  1108,  1176,
    1124,  1126,  1171,  1173,  1174,  1638,   287,   394,   287,  1175,
     287,   395,  1177,  1178,  1179,  1180,  1181,  1203,  1032,  1182,
    1183,  1184,   391,   720,   721,  1185,  1187,  1186,   757,   758,
     759,   760,   761,  1196,  1197,  1202,  1047,  1212,  1214,  1216,
     581,   394,  1053,   762,   763,   395,  1224,  1239,  1238,   392,
     393,  1237,  1245,  1253,  1254,   394,  1255,  1067,   286,   395,
     286,  1265,  1266,  1269,  1275,  1276,  1296,  1659,  1279,  1285,
    1288,   286,   396,   286,  1292,   286,   397,  1293,  1281,   398,
    1297,  1300,  1093,  1094,  1299,  1301,  1670,  1306,   391,  1307,
    1310,  1311,  1313,  1103,  1334,  1651,  1331,   621,  1109,  1809,
    1110,  1332,  1111,  1643,   399,  1343,   396,   876,  1348,  1347,
     397,  1349,  1282,   398,  1356,   392,   393,   287,  1401,  1403,
     396,  1400,  1404,  1411,   397,  1410,  1283,   398,  1415,  1417,
    1592,  1429,  1426,  1594,   391,  1427,   394,  1438,   399,  1439,
     395,   722,   723,   724,   725,   726,  1434,   686,   727,   728,
     729,   730,   399,   731,   732,   733,   734,   735,   736,   737,
     738,   392,   393,  1443,   739,   740,   741,   742,   743,  1450,
     744,   745,  1449,  1451,  1471,  1475,  1477,  1491,   747,   287,
     749,  1195,   286,  1494,  1495,  1200,  1496,  1497,  1498,  1503,
    1505,  1506,  1508,  1550,  1507,  1512,  1513,  1518,   287,  1552,
    1143,   396,   394,  1557,  1558,   397,   395,  1284,   398,  1560,
    1562,  1227,   391,  1564,  1577,  1566,   752,   753,   754,   755,
     756,   757,   758,   759,   760,   761,  1581,  1583,  1570,  1578,
     391,  1586,  1579,   399,  1580,  1585,   762,   763,  1590,   392,
     393,  1258,  1598,  1593,   286,  1595,  1596,  1602,   394,  1603,
    1610,  1604,   395,  1605,  1606,  1766,  1607,   392,   393,  1608,
    1770,  1622,  1609,   286,  1612,  1278,  1614,   396,  1623,  1625,
     391,   397,  1626,  1286,   398,  1631,  1634,  1644,  1645,  1667,
    1660,  1661,  1664,  1678,  1679,  1695,  1680,   815,   816,   817,
     818,   819,   820,   821,   822,  1681,   686,   392,   393,   399,
    1682,  1691,  1696,  1693,  1712,  1698,   823,  1732,  1704,  1705,
    1706,  1023,  1707,   396,  1730,  1806,  1807,   397,  1327,  1294,
     398,  1330,  1708,   824,  1713,  1721,   394,  1336,  1727,  1339,
     395,   681,   682,  1759,  1764,   825,   826,   827,  1716,  1314,
    1729,  1747,  1748,   391,   394,   399,  1749,  1750,   395,  1751,
     695,   696,   697,   698,   699,  1752,  1758,  1768,  1781,  1731,
    1382,  1783,  1384,  1784,  1389,  1785,  1390,   828,  1393,  1793,
     392,   393,  1395,  1845,  1397,  1788,  1399,  1789,  1790,  1792,
    1848,  1815,  1819,   287,   394,  1405,  1852,  1796,   395,  1816,
    1817,   396,  1818,  1023,  1820,   397,  1831,  1453,   398,  1823,
    1836,   905,  1850,  1853,  1861,  1862,  1865,   127,  1875,   396,
      20,   130,  1869,   397,  1876,  1458,   398,  1422,  1874,  1030,
     372,  1517,   924,   399,  1251,   928,  1018,  1428,    26,  1635,
    1031,  1734,  1433,  1776,  1435,  1735,    94,  1672,  1246,  1777,
    1778,   399,   840,  1442,  1436,  1247,  1444,   381,   286,   396,
     877,   474,     0,   397,  1803,  1466,   398,   394,     0,   648,
     650,   395,     0,  1143,     0,   876,     0,     0,     0,   651,
       0,     0,     0,   652,     0,     0,   654,   391,     0,     0,
       0,   399,   655,     0,     0,   720,   721,     0,  1476,     0,
       0,     0,     0,     0,     0,     0,  1487,  1488,  1489,     0,
       0,  1832,     0,     0,   392,   393,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1502,     0,     0,  1504,  1847,
       0,     0,   396,     0,     0,     0,   397,  1510,  1652,   398,
       0,     0,     0,  1854,     0,  1856,     0,     0,     0,     0,
    1143,     0,     0,     0,   937,   938,     0,     0,   939,   940,
     941,   942,     0,   944,   399,     0,     0,  1871,     0,  1143,
     954,     0,     0,     0,   967,   968,   970,   971,   972,   975,
     976,   977,   978,   980,   981,   982,   983,   984,   985,   986,
     987,   988,   989,   990,     0,     0,     0,     0,     0,     0,
    1568,   394,  1569,     0,     0,   395,     0,     0,     0,  1575,
       0,     0,     0,   722,   723,   724,   725,   726,     0,     0,
     727,   728,   729,   730,     0,   731,   732,   733,   734,   735,
     736,   737,   738,     0,     0,     0,   739,   740,   741,   742,
     743,     0,   744,   745,     0,     0,   746,     0,     0,  1597,
     747,   748,   749,     0,     0,     0,   750,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   396,  1616,     0,     0,
     397,  1041,  1656,   398,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1629,  1630,   751,  1192,   752,   753,
     754,   755,   756,   757,   758,   759,   760,   761,   399,     0,
       0,     0,     0,     0,     0,  1639,     0,     0,   762,   763,
       0,  1641,  1642,     0,     0,     0,     0,  1647,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1023,     0,     0,     0,     0,     0,  1168,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1023,     0,     0,     0,     0,  1665,     0,     0,  1666,
       0,  1668,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   407,   408,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   876,  1685,  1686,  1687,  1688,     0,   409,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1697,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1711,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1168,     0,     0,     0,  1742,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1757,     0,     0,     0,     0,
    1760,  1761,   410,   411,   412,   413,   414,   415,   416,   417,
     418,   419,   420,   421,   422,   423,   424,   425,   426,   427,
       0,   428,   429,   430,   431,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   432,   433,
     434,   435,   436,     0,     0,     0,     0,   437,   438,   439,
     440,   441,   442,   443,     0,     0,     0,     0,  1795,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   444,   445,     0,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,  1822,     0,   456,   457,     0,
       0,     0,     0,     0,     0,   458,   459,     0,     0,  1830,
       0,     0,     0,     0,     0,     0,     0,     0,  1835,     0,
       0,     0,     0,     0,  1402,     0,     0,     0,  1846,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1128,     0,     0,     0,   487,   488,     3,     0,  -122,
    -107,  -107,  1860,  -119,     0,   489,   490,   491,   492,   493,
       0,     0,     0,     0,     0,   494,  1129,   495,  1130,  1131,
       0,   496,     0,     0,     0,     0,     0,     0,  1132,   497,
    1133,     0,  -124,     0,  1134,   498,     0,     0,   499,     0,
       8,   500,  1135,     0,  1136,   501,     0,     0,  1137,  1138,
       0,     0,     0,     0,     0,  1139,     0,     0,   503,   504,
       0,   228,   229,   230,     0,   232,   233,   234,   235,   236,
     505,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,     0,   250,   251,   252,     0,     0,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   506,   507,   508,  1140,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   509,   510,     0,     0,     0,     0,     0,     0,     0,
       0,  1523,  1524,  1525,  1526,  1527,  1528,  1529,  1530,  1531,
    1532,  1533,  1536,  1537,  1540,  1541,  1542,  1543,  1544,  1545,
    1546,  1547,     0,   511,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,   515,   516,     0,   517,
       0,   518,   519,   520,   521,   522,   523,   524,   525,   526,
      59,     0,    13,   527,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
     529,   530,     0,    14,     0,     0,   531,   532,     0,     0,
       0,     0,     0,     0,     0,   533,     0,   534,     0,   535,
     536,    16,  1141,  1142,  1128,     0,     0,     0,   487,   488,
       3,     0,  -122,  -107,  -107,     0,  -119,     0,   489,   490,
     491,   492,   493,     0,     0,     0,     0,     0,   494,  1129,
     495,  1130,  1131,     0,   496,     0,     0,     0,     0,     0,
       0,  1132,   497,  1133,     0,  -124,     0,  1134,   498,     0,
       0,   499,     0,     8,   500,  1135,     0,  1136,   501,     0,
       0,  1137,  1138,  1168,     0,     0,     0,     0,  1139,     0,
       0,   503,   504,     0,   228,   229,   230,     0,   232,   233,
     234,   235,   236,   505,   238,   239,   240,   241,   242,   243,
     244,   245,   246,   247,   248,     0,   250,   251,   252,     0,
       0,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   506,   507,
     508,  1140,     0,     0,     0,   -88,     0,     0,     0,     0,
       0,     0,     0,     0,   509,   510,   720,   721,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   511,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   512,   513,   514,   515,
     516,     0,   517,     0,   518,   519,   520,   521,   522,   523,
     524,   525,   526,    59,     0,    13,   527,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   528,   529,   530,     0,    14,     0,     0,   531,
     532,     0,     0,     0,     0,     0,     0,     0,   533,     0,
     534,     0,   535,   536,    16,  1141,  -335,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1359,  1360,  1361,  1362,
    1363,  1364,  1365,  1366,   722,   723,   724,   725,   726,  1367,
    1368,   727,   728,   729,   730,  1369,   731,   732,   733,   734,
     735,   736,   737,   738,  1370,  1371,  1372,   739,   740,   741,
     742,   743,  1373,   744,   745,  1374,  1375,   746,     0,     0,
       0,   747,   748,   749,  1376,  1377,  1378,   750,     0,     0,
       0,     0,     0,     0,     0,     0,  1802,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1168,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1379,   751,     0,   752,
     753,   754,   755,   756,   757,   758,   759,   760,   761,     0,
       0,     0,     0,     0,     0,     0,     0,  1128,     0,   762,
     763,   487,   488,     3,     0,  -122,  -107,  -107,     0,  -119,
       0,   489,   490,   491,   492,   493,     0,  1802,     0,     0,
       0,   494,  1129,   495,  1130,  1131,     0,   496,     0,     0,
       0,     0,     0,     0,  1132,   497,  1133,     0,  -124,     0,
    1134,   498,     0,  1168,   499,     0,     8,   500,  1135,     0,
    1136,   501,     0,     0,  1137,  1138,  1802,     0,     0,     0,
       0,  1139,  1168,     0,   503,   504,     0,   228,   229,   230,
       0,   232,   233,   234,   235,   236,   505,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,     0,   250,
     251,   252,     0,     0,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   506,   507,   508,  1140,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   511,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,   515,   516,     0,   517,     0,   518,   519,   520,
     521,   522,   523,   524,   525,   526,    59,     0,    13,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,   529,   530,     0,    14,
       0,     0,   531,   532,     0,     0,     0,     0,     0,     0,
       0,   533,     0,   534,     0,   535,   536,    16,  1141,  -361,
    1128,     0,     0,     0,   487,   488,     3,     0,  -122,  -107,
    -107,     0,  -119,     0,   489,   490,   491,   492,   493,     0,
       0,     0,     0,     0,   494,  1129,   495,  1130,  1131,     0,
     496,     0,     0,     0,     0,     0,     0,  1132,   497,  1133,
       0,  -124,     0,  1134,   498,     0,     0,   499,     0,     8,
     500,  1135,     0,  1136,   501,     0,     0,  1137,  1138,     0,
       0,     0,     0,     0,  1139,     0,     0,   503,   504,     0,
     228,   229,   230,     0,   232,   233,   234,   235,   236,   505,
     238,   239,   240,   241,   242,   243,   244,   245,   246,   247,
     248,     0,   250,   251,   252,     0,     0,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   506,   507,   508,  1140,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     509,   510,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   511,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   512,   513,   514,   515,   516,     0,   517,     0,
     518,   519,   520,   521,   522,   523,   524,   525,   526,    59,
       0,    13,   527,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   528,   529,
     530,     0,    14,     0,     0,   531,   532,     0,     0,     0,
       0,     0,     0,     0,   533,     0,   534,     0,   535,   536,
      16,  1141,  -332,  1128,     0,     0,     0,   487,   488,     3,
       0,  -122,  -107,  -107,     0,  -119,     0,   489,   490,   491,
     492,   493,     0,     0,     0,     0,     0,   494,  1129,   495,
    1130,  1131,     0,   496,     0,     0,     0,     0,     0,     0,
    1132,   497,  1133,     0,  -124,     0,  1134,   498,     0,     0,
     499,     0,     8,   500,  1135,     0,  1136,   501,     0,     0,
    1137,  1138,     0,     0,     0,     0,     0,  1139,     0,     0,
     503,   504,     0,   228,   229,   230,     0,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   506,   507,   508,
    1140,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   509,   510,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   511,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,   515,   516,
       0,   517,     0,   518,   519,   520,   521,   522,   523,   524,
     525,   526,    59,     0,    13,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   528,   529,   530,     0,    14,     0,     0,   531,   532,
       0,     0,     0,     0,     0,     0,     0,   533,     0,   534,
       0,   535,   536,    16,  1141,   -98,  1128,     0,     0,     0,
     487,   488,     3,     0,  -122,  -107,  -107,     0,  -119,     0,
     489,   490,   491,   492,   493,     0,     0,     0,     0,     0,
     494,  1129,   495,  1130,  1131,     0,   496,     0,     0,     0,
       0,     0,     0,  1132,   497,  1133,     0,  -124,     0,  1134,
     498,     0,     0,   499,     0,     8,   500,  1135,     0,  1136,
     501,     0,     0,  1137,  1138,     0,     0,     0,     0,     0,
    1139,     0,     0,   503,   504,     0,   228,   229,   230,     0,
     232,   233,   234,   235,   236,   505,   238,   239,   240,   241,
     242,   243,   244,   245,   246,   247,   248,     0,   250,   251,
     252,     0,     0,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     506,   507,   508,  1140,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   509,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   511,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   512,   513,
     514,   515,   516,     0,   517,     0,   518,   519,   520,   521,
     522,   523,   524,   525,   526,    59,     0,    13,   527,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   528,   529,   530,     0,    14,     0,
       0,   531,   532,     0,     0,     0,     0,     0,     0,     0,
     533,     0,   534,     0,   535,   536,    16,  1141,  -103,   487,
     488,     0,     0,     0,     0,     0,     0,     0,     0,   489,
     490,   491,   492,   493,     0,     0,     0,     0,     0,   494,
       0,   495,     0,     0,     0,   496,     0,     0,     0,     0,
       0,     0,     0,   497,     0,     0,     0,     0,     0,   498,
       0,     0,   499,     0,     0,   500,     0,     0,     0,   501,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
       0,     0,   503,   504,   919,   228,   229,   230,     0,   232,
     233,   234,   235,   236,   505,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,     0,   250,   251,   252,
       0,     0,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   506,
     507,   508,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   509,   510,     0,     0,     0,
       0,     0,     0,     0,   584,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   511,     0,    58,
       0,     0,     0,     0,     0,     0,     0,   512,   513,   514,
     515,   516,     0,   517,   703,   518,   519,   520,   521,   522,
     523,   524,   525,   526,   704,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   528,   529,   530,     0,    14,     0,     0,
     531,   532,     0,     0,     0,     0,     0,   487,   488,   920,
       0,   534,   921,   535,   536,   700,   537,   489,   490,   491,
     492,   493,     0,     0,     0,     0,     0,   494,     0,   495,
       0,     0,     0,   496,     0,     0,     0,     0,     0,     0,
       0,   497,     0,     0,     0,     0,     0,   498,     0,     0,
     499,   701,     0,   500,     0,     0,     0,   501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,     0,     0,
     503,   504,     0,   228,   229,   230,     0,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   506,   507,   508,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   509,   510,     0,     0,     0,     0,     0,
       0,     0,   584,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   511,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,   515,   516,
       0,   517,   703,   518,   519,   520,   521,   522,   523,   524,
     525,   526,   704,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   528,   529,   530,     0,    14,     0,     0,   531,   532,
       0,     0,     0,     0,     0,   487,   488,   533,     0,   534,
       0,   535,   536,   700,   537,   489,   490,   491,   492,   493,
       0,     0,     0,     0,     0,   494,     0,   495,     0,     0,
       0,   496,     0,     0,     0,     0,     0,     0,     0,   497,
       0,     0,     0,     0,     0,   498,     0,     0,   499,   701,
       0,   500,     0,     0,     0,   501,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,     0,     0,   503,   504,
       0,   228,   229,   230,     0,   232,   233,   234,   235,   236,
     505,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,     0,   250,   251,   252,     0,     0,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   506,   507,   508,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   509,   510,     0,     0,     0,     0,     0,     0,     0,
     584,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   511,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,   515,   516,     0,   517,
       0,   518,   519,   520,   521,   522,   523,   524,   525,   526,
      59,     0,     0,   527,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
     529,   530,     0,    14,     0,     0,   531,   532,     0,     0,
       0,     0,     0,   487,   488,   533,     0,   534,     0,   535,
     536,     0,   537,   489,   490,   491,   492,   493,     0,     0,
       0,     0,     0,   494,     0,   495,     0,     0,     0,   496,
       0,     0,     0,     0,     0,     0,     0,   497,     0,     0,
       0,     0,     0,   498,     0,     0,   499,     0,     0,   500,
       0,     0,     0,   501,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,     0,     0,   503,   504,  1062,   228,
     229,   230,     0,   232,   233,   234,   235,   236,   505,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
       0,   250,   251,   252,     0,     0,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   506,   507,   508,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   509,
     510,     0,     0,     0,     0,     0,     0,     0,   584,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   511,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,   515,   516,     0,   517,   703,   518,
     519,   520,   521,   522,   523,   524,   525,   526,   704,     0,
       0,   527,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,   529,   530,
       0,    14,     0,     0,   531,   532,     0,     0,     0,     0,
       0,   487,   488,   533,     0,   534,     0,   535,   536,     0,
     537,   489,   490,   491,   492,   493,     0,     0,     0,     0,
       0,   494,     0,   495,     0,     0,     0,   496,     0,     0,
       0,     0,     0,     0,     0,   497,     0,     0,     0,     0,
       0,   498,     0,     0,   499,     0,     0,   500,     0,     0,
       0,   501,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,     0,     0,   503,   504,     0,   228,   229,   230,
       0,   232,   233,   234,   235,   236,   505,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,     0,   250,
     251,   252,     0,     0,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   506,   507,   508,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,     0,
       0,     0,     0,     0,     0,     0,   584,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   511,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,   515,   516,     0,   517,   703,   518,   519,   520,
     521,   522,   523,   524,   525,   526,   704,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,   529,   530,     0,    14,
       0,     0,   531,   532,     0,     0,     0,     0,     0,   487,
     488,  1385,     0,   534,  1386,   535,   536,     0,   537,   489,
     490,   491,   492,   493,     0,     0,     0,     0,     0,   494,
       0,   495,     0,     0,     0,   496,     0,     0,     0,     0,
       0,     0,     0,   497,     0,     0,     0,     0,     0,   498,
       0,     0,   499,     0,     0,   500,     0,     0,     0,   501,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
       0,     0,   503,   504,     0,   228,   229,   230,     0,   232,
     233,   234,   235,   236,   505,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,     0,   250,   251,   252,
       0,     0,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   506,
     507,   508,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   509,   510,     0,     0,     0,
       0,     0,     0,     0,   584,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   511,     0,    58,
       0,     0,     0,     0,     0,     0,     0,   512,   513,   514,
     515,   516,     0,   517,   703,   518,   519,   520,   521,   522,
     523,   524,   525,   526,   704,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   528,   529,   530,     0,    14,     0,     0,
     531,   532,     0,     0,     0,     0,     0,   487,   488,  1406,
       0,   534,  1407,   535,   536,     0,   537,   489,   490,   491,
     492,   493,     0,     0,     0,     0,     0,   494,     0,   495,
       0,     0,     0,   496,     0,     0,     0,     0,     0,     0,
       0,   497,     0,     0,     0,     0,     0,   498,     0,     0,
     499,     0,     0,   500,     0,     0,     0,   501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,     0,     0,
     503,   504,     0,   228,   229,   230,     0,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   506,   507,   508,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   509,   510,     0,     0,     0,     0,     0,
       0,     0,   584,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   511,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,   515,   516,
       0,   517,   703,   518,   519,   520,   521,   522,   523,   524,
     525,   526,   704,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   528,   529,   530,     0,    14,     0,     0,   531,   532,
       0,     0,     0,     0,     0,   487,   488,   533,     0,   534,
       0,   535,   536,     0,   537,   489,   490,   491,   492,   493,
       0,     0,     0,     0,     0,   494,     0,   495,     0,     0,
       0,   496,     0,     0,     0,     0,     0,     0,     0,   497,
       0,     0,     0,     0,     0,   498,     0,     0,   499,     0,
       0,   500,     0,     0,     0,   501,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,     0,     0,   503,   504,
       0,   228,   229,   230,     0,   232,   233,   234,   235,   236,
     505,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,     0,   250,   251,   252,     0,     0,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   506,   507,   508,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   509,   510,     0,     0,     0,     0,     0,     0,     0,
     584,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   511,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,   515,   516,     0,   517,
       0,   518,   519,   520,   521,   522,   523,   524,   525,   526,
      59,     0,     0,   527,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
     529,   530,     0,    14,     0,     0,   531,   532,     0,     0,
       0,     0,     0,   487,   488,   533,     0,   534,  1000,   535,
     536,     0,   537,   489,   490,   491,   492,   493,     0,     0,
       0,     0,     0,   494,     0,   495,     0,     0,     0,   496,
       0,     0,     0,     0,     0,     0,     0,   497,     0,     0,
       0,     0,     0,   498,     0,     0,   499,     0,     0,   500,
       0,     0,     0,   501,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,     0,     0,   503,   504,     0,   228,
     229,   230,     0,   232,   233,   234,   235,   236,   505,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
       0,   250,   251,   252,     0,     0,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   506,   507,   508,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   509,
     510,     0,     0,     0,     0,     0,     0,     0,  1022,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   511,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,   515,   516,     0,   517,   703,   518,
     519,   520,   521,   522,   523,   524,   525,   526,   704,     0,
       0,   527,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,   529,   530,
       0,    14,     0,     0,   531,   532,     0,     0,     0,     0,
       0,   487,   488,  1316,     0,   534,     0,   535,   536,     0,
     537,   489,   490,   491,   492,   493,     0,     0,     0,     0,
       0,   494,     0,   495,     0,     0,     0,   496,     0,     0,
       0,     0,     0,     0,     0,   497,     0,     0,     0,     0,
       0,   498,     0,     0,   499,     0,     0,   500,     0,     0,
       0,   501,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,     0,     0,   503,   504,     0,   228,   229,   230,
       0,   232,   233,   234,   235,   236,   505,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,     0,   250,
     251,   252,     0,     0,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   506,   507,   508,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,     0,
       0,     0,     0,     0,     0,     0,   584,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   511,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,   515,   516,     0,   517,     0,   518,   519,   520,
     521,   522,   523,   524,   525,   526,    59,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,   529,   530,     0,    14,
       0,     0,   531,   532,     0,     0,     0,     0,     0,   487,
     488,   533,     0,   534,  1391,   535,   536,     0,   537,   489,
     490,   491,   492,   493,     0,     0,     0,     0,     0,   494,
       0,   495,     0,     0,     0,   496,     0,     0,     0,     0,
       0,     0,     0,   497,     0,     0,     0,     0,     0,   498,
       0,     0,   499,     0,     0,   500,     0,     0,     0,   501,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
       0,     0,   503,   504,     0,   228,   229,   230,     0,   232,
     233,   234,   235,   236,   505,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,     0,   250,   251,   252,
       0,     0,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   506,
     507,   508,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   509,   510,     0,     0,     0,
       0,     0,     0,     0,   584,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   511,     0,    58,
       0,     0,     0,     0,     0,     0,     0,   512,   513,   514,
     515,   516,     0,   517,     0,   518,   519,   520,   521,   522,
     523,   524,   525,   526,    59,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   528,   529,   530,     0,    14,     0,     0,
     531,   532,     0,     0,     0,     0,     0,   487,   488,   533,
       0,   534,  1413,   535,   536,     0,   537,   489,   490,   491,
     492,   493,     0,     0,     0,     0,     0,   494,     0,   495,
       0,     0,     0,   496,     0,     0,     0,     0,     0,     0,
       0,   497,     0,     0,     0,     0,     0,   498,     0,     0,
     499,     0,     0,   500,     0,     0,     0,   501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,     0,     0,
     503,   504,     0,   228,   229,   230,     0,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   506,   507,   508,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   509,   510,     0,     0,     0,     0,     0,
       0,     0,   584,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   511,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,   515,   516,
       0,   517,     0,   518,   519,   520,   521,   522,   523,   524,
     525,   526,    59,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   528,   529,   530,     0,    14,     0,     0,   531,   532,
       0,     0,     0,     0,     0,   487,   488,   533,     0,   534,
    1472,   535,   536,     0,   537,   489,   490,   491,   492,   493,
       0,     0,     0,     0,     0,   494,     0,   495,     0,     0,
       0,   496,     0,     0,     0,     0,     0,     0,     0,   497,
       0,     0,     0,     0,     0,   498,     0,     0,   499,     0,
       0,   500,     0,     0,     0,   501,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,     0,     0,   503,   504,
       0,   228,   229,   230,     0,   232,   233,   234,   235,   236,
     505,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,     0,   250,   251,   252,     0,     0,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   506,   507,   508,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   509,   510,     0,     0,     0,     0,     0,     0,     0,
    1022,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   511,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,   515,   516,     0,   517,
     703,   518,   519,   520,   521,   522,   523,   524,   525,   526,
     704,     0,     0,   527,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
     529,   530,     0,    14,     0,     0,   531,   532,     0,     0,
       0,     0,     0,   487,   488,   533,     0,   534,     0,   535,
     536,     0,   537,   489,   490,   491,   492,   493,     0,     0,
       0,     0,     0,   494,     0,   495,     0,     0,     0,   496,
       0,     0,     0,     0,     0,     0,     0,   497,     0,     0,
       0,     0,     0,   498,     0,     0,   499,     0,     0,   500,
       0,     0,     0,   501,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,     0,     0,   503,   504,     0,   228,
     229,   230,     0,   232,   233,   234,   235,   236,   505,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
       0,   250,   251,   252,     0,     0,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   506,   507,   508,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   509,
     510,     0,     0,     0,     0,     0,     0,     0,   584,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   511,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,   515,   516,     0,   517,     0,   518,
     519,   520,   521,   522,   523,   524,   525,   526,    59,     0,
       0,   527,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,   529,   530,
       0,    14,     0,     0,   531,   532,     0,     0,     0,     0,
       0,   487,   488,   533,     0,   534,     0,   535,   536,     0,
     537,   489,   490,   491,   492,   493,     0,     0,     0,     0,
       0,   494,     0,   495,     0,     0,     0,   496,     0,     0,
       0,     0,     0,     0,     0,   497,     0,     0,     0,     0,
       0,   498,     0,     0,   499,     0,     0,   500,     0,     0,
       0,   501,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,     0,     0,   503,   504,     0,   228,   229,   230,
       0,   232,   233,   234,   235,   236,   505,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,     0,   250,
     251,   252,     0,     0,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   506,   507,   508,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   511,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,   515,   516,     0,   517,     0,   518,   519,   520,
     521,   522,   523,   524,   525,   526,    59,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,   529,   530,     0,    14,
       0,     0,   531,   532,     0,     0,     0,     0,     0,   487,
     488,   533,   594,   534,     0,   535,   536,     0,   537,   489,
     490,   491,   492,   493,     0,     0,     0,     0,     0,   494,
       0,   495,     0,     0,     0,   496,     0,     0,     0,     0,
       0,     0,     0,   497,     0,     0,     0,     0,     0,   498,
       0,     0,   499,     0,     0,   500,     0,     0,     0,   501,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
       0,     0,   503,   504,     0,   228,   229,   230,     0,   232,
     233,   234,   235,   236,   505,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,     0,   250,   251,   252,
       0,     0,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   506,
     507,   508,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   509,   510,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   511,     0,    58,
       0,     0,     0,     0,     0,     0,     0,   512,   513,   514,
     515,   516,     0,   517,     0,   518,   519,   520,   521,   522,
     523,   524,   525,   526,    59,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   528,   529,   530,     0,    14,     0,     0,
     531,   532,     0,     0,     0,     0,     0,   487,   488,   533,
     794,   534,     0,   535,   536,     0,   537,   489,   490,   491,
     492,   493,     0,     0,     0,     0,     0,   494,     0,   495,
       0,     0,     0,   496,     0,     0,     0,     0,     0,     0,
       0,   497,     0,     0,     0,     0,     0,   498,     0,     0,
     499,     0,     0,   500,     0,     0,     0,   501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,     0,     0,
     503,   504,     0,   228,   229,   230,     0,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   506,   507,   508,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   509,   510,     0,     0,     0,     0,     0,
       0,     0,  1022,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   511,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,   515,   516,
       0,   517,     0,   518,   519,   520,   521,   522,   523,   524,
     525,   526,    59,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   528,   529,   530,     0,    14,     0,     0,   531,   532,
       0,     0,     0,     0,     0,   487,   488,   533,     0,   534,
       0,   535,   536,  1115,   537,   489,   490,   491,   492,   493,
       0,     0,     0,     0,     0,   494,     0,   495,     0,     0,
       0,   496,     0,     0,     0,     0,     0,     0,     0,   497,
       0,     0,     0,     0,     0,   498,     0,     0,   499,     0,
       0,   500,     0,     0,     0,   501,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,     0,     0,   503,   504,
       0,   228,   229,   230,     0,   232,   233,   234,   235,   236,
     505,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,     0,   250,   251,   252,     0,     0,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   506,   507,   508,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   509,   510,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   511,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,   515,   516,     0,   517,
       0,   518,   519,   520,   521,   522,   523,   524,   525,   526,
      59,     0,     0,   527,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
     529,   530,     0,    14,     0,     0,   531,   532,     0,     0,
       0,     0,     0,   487,   488,   533,     0,   534,     0,   535,
     536,     0,   537,   489,   490,   491,   492,   493,     0,     0,
       0,     0,     0,   494,     0,   495,     0,     0,     0,   496,
       0,     0,     0,     0,     0,     0,     0,   497,     0,     0,
       0,     0,     0,   498,     0,     0,   499,     0,     0,   500,
       0,     0,     0,   501,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,     0,     0,   503,   504,     0,   228,
     229,   230,     0,   232,   233,   234,   235,   236,   505,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
       0,   250,   251,   252,     0,     0,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   506,   507,   508,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   509,
     510,     0,     0,     0,     0,     0,     0,     0,  1326,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   511,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,   515,   516,     0,   517,     0,   518,
     519,   520,   521,   522,   523,   524,   525,   526,    59,     0,
       0,   527,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,   529,   530,
       0,    14,     0,     0,   531,   532,     0,     0,     0,     0,
       0,   487,   488,   533,     0,   534,     0,   535,   536,     0,
     537,   489,   490,   491,   492,   493,     0,     0,     0,     0,
       0,   494,     0,   495,     0,     0,     0,   496,     0,     0,
       0,     0,     0,     0,     0,   497,     0,     0,     0,     0,
       0,   498,     0,     0,   499,     0,     0,   500,     0,     0,
       0,   501,     0,     0,     0,     0,     0,  1329,     0,     0,
       0,   502,     0,     0,   503,   504,     0,   228,   229,   230,
       0,   232,   233,   234,   235,   236,   505,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,     0,   250,
     251,   252,     0,     0,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   506,   507,   508,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   511,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,   515,   516,     0,   517,     0,   518,   519,   520,
     521,   522,   523,   524,   525,   526,    59,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,   529,   530,     0,    14,
       0,     0,   531,   532,     0,     0,     0,     0,     0,   487,
     488,   533,     0,   534,     0,   535,   536,     0,   537,   489,
     490,   491,   492,   493,     0,     0,     0,     0,     0,   494,
       0,   495,     0,     0,     0,   496,     0,     0,     0,     0,
       0,     0,     0,   497,     0,     0,     0,     0,     0,   498,
       0,     0,   499,     0,     0,   500,     0,     0,     0,   501,
       0,     0,  1335,     0,     0,     0,     0,     0,     0,   502,
       0,     0,   503,   504,     0,   228,   229,   230,     0,   232,
     233,   234,   235,   236,   505,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,     0,   250,   251,   252,
       0,     0,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   506,
     507,   508,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   509,   510,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   511,     0,    58,
       0,     0,     0,     0,     0,     0,     0,   512,   513,   514,
     515,   516,     0,   517,     0,   518,   519,   520,   521,   522,
     523,   524,   525,   526,    59,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   528,   529,   530,     0,    14,     0,     0,
     531,   532,     0,     0,     0,     0,     0,   487,   488,   533,
       0,   534,     0,   535,   536,     0,   537,   489,   490,   491,
     492,   493,     0,     0,     0,     0,     0,   494,     0,   495,
       0,     0,     0,   496,     0,     0,     0,     0,     0,     0,
       0,   497,     0,     0,     0,     0,     0,   498,     0,     0,
     499,     0,     0,   500,     0,     0,     0,   501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,     0,     0,
     503,   504,     0,   228,   229,   230,     0,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   506,   507,   508,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   509,   510,     0,     0,     0,     0,     0,
       0,     0,  1338,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   511,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,   515,   516,
       0,   517,     0,   518,   519,   520,   521,   522,   523,   524,
     525,   526,    59,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   528,   529,   530,     0,    14,     0,     0,   531,   532,
       0,     0,     0,     0,     0,   487,   488,   533,     0,   534,
       0,   535,   536,     0,   537,   489,   490,   491,   492,   493,
       0,     0,     0,     0,     0,   494,     0,   495,     0,     0,
       0,   496,     0,     0,     0,     0,     0,     0,     0,   497,
       0,     0,     0,     0,     0,   498,     0,     0,   499,     0,
       0,   500,     0,     0,     0,   501,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,     0,     0,   503,   504,
       0,   228,   229,   230,     0,   232,   233,   234,   235,   236,
     505,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,     0,   250,   251,   252,     0,     0,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   506,   507,   508,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   509,   510,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   511,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,   515,   516,     0,   517,
       0,   518,   519,   520,   521,   522,   523,   524,   525,   526,
      59,     0,     0,   527,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
     529,   530,     0,    14,     0,     0,   531,   532,     0,     0,
       0,     0,     0,   487,   488,   533,     0,   534,  1615,   535,
     536,     0,   537,   489,   490,   491,   492,   493,     0,     0,
       0,     0,     0,   494,     0,   495,     0,     0,     0,   496,
       0,     0,     0,     0,     0,     0,     0,   497,     0,     0,
       0,     0,     0,   498,     0,     0,   499,     0,     0,   500,
       0,     0,     0,   501,  1646,     0,     0,     0,     0,     0,
       0,     0,     0,   502,     0,     0,   503,   504,     0,   228,
     229,   230,     0,   232,   233,   234,   235,   236,   505,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
       0,   250,   251,   252,     0,     0,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   506,   507,   508,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   509,
     510,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   511,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,   515,   516,     0,   517,     0,   518,
     519,   520,   521,   522,   523,   524,   525,   526,    59,     0,
       0,   527,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,   529,   530,
       0,    14,     0,     0,   531,   532,     0,     0,     0,     0,
       0,   487,   488,   533,     0,   534,     0,   535,   536,     0,
     537,   489,   490,   491,   492,   493,     0,     0,     0,     0,
       0,   494,     0,   495,     0,     0,     0,   496,     0,     0,
       0,     0,     0,     0,     0,   497,     0,     0,     0,     0,
       0,   498,     0,     0,   499,     0,     0,   500,     0,     0,
       0,   501,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,     0,     0,   503,   504,     0,   228,   229,   230,
       0,   232,   233,   234,   235,   236,   505,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,     0,   250,
     251,   252,     0,     0,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   506,   507,   508,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   511,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,   515,   516,     0,   517,     0,   518,   519,   520,
     521,   522,   523,   524,   525,   526,    59,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,   529,   530,     0,    14,
       0,     0,   531,   532,     0,     0,     0,     0,     0,   487,
     488,   533,     0,   534,     0,   535,   536,     0,   537,   489,
     490,   491,   492,   493,     0,     0,     0,     0,     0,   494,
    1129,   495,  1130,     0,     0,   496,     0,     0,     0,     0,
       0,     0,     0,   497,     0,     0,     0,     0,     0,   498,
       0,     0,   499,     0,     0,   500,  1135,     0,     0,   501,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
       0,     0,   503,   504,     0,   228,   229,   230,     0,   232,
     233,   234,   235,   236,   505,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,     0,   250,   251,   252,
       0,     0,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   506,
     507,   508,  1140,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   509,   510,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   511,     0,    58,
       0,     0,     0,     0,     0,     0,     0,   512,   513,   514,
     515,   516,     0,   517,     0,   518,   519,   520,   521,   522,
     523,   524,   525,   526,    59,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   528,   529,   530,     0,    14,     0,     0,
     531,   532,     0,     0,     0,   487,   488,     0,     0,   533,
       0,   534,     0,   535,   536,   489,   490,   491,   492,   493,
       0,     0,     0,     0,     0,   494,     0,   495,     0,     0,
       0,   496,     0,     0,     0,     0,     0,     0,     0,   497,
       0,     0,     0,     0,     0,   498,     0,     0,   499,     0,
       0,   500,     0,     0,     0,   501,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,     0,     0,   503,   504,
       0,   228,   229,   230,     0,   232,   233,   234,   235,   236,
     505,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,     0,   250,   251,   252,     0,     0,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   506,   507,   508,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   509,   510,     0,     0,     0,     0,     0,     0,     0,
     839,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   511,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,   515,   516,     0,   517,
       0,   518,   519,   520,   521,   522,   523,   524,   525,   526,
      59,     0,     0,   527,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
     529,   530,     0,    14,     0,     0,   531,   532,     0,     0,
       0,   487,   488,     0,     0,   533,     0,   534,     0,   535,
     536,   489,   490,   491,   492,   493,     0,     0,   979,     0,
       0,   494,     0,   495,     0,     0,     0,   496,     0,     0,
       0,     0,     0,     0,     0,   497,     0,     0,     0,     0,
       0,   498,     0,     0,   499,     0,     0,   500,     0,     0,
       0,   501,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,     0,     0,   503,   504,     0,   228,   229,   230,
       0,   232,   233,   234,   235,   236,   505,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,     0,   250,
     251,   252,     0,     0,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   506,   507,   508,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   509,   510,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   511,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,   515,   516,     0,   517,     0,   518,   519,   520,
     521,   522,   523,   524,   525,   526,    59,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,   529,   530,     0,    14,
       0,     0,   531,   532,     0,     0,     0,   487,   488,     0,
       0,   533,     0,   534,     0,   535,   536,   489,   490,   491,
     492,   493,     0,     0,     0,     0,     0,   494,     0,   495,
       0,     0,     0,   496,     0,     0,     0,     0,     0,     0,
       0,   497,     0,     0,     0,     0,     0,   498,     0,     0,
     499,     0,     0,   500,     0,     0,     0,   501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,     0,     0,
     503,   504,     0,   228,   229,   230,     0,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   506,   507,   508,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   509,   510,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   511,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,   515,   516,
       0,   517,     0,   518,   519,   520,   521,   522,   523,   524,
     525,   526,    59,     0,   222,   527,     0,     0,     0,     0,
     223,     0,     0,     0,   720,   721,   224,     0,     0,     0,
       0,   528,   529,   530,     0,    14,   225,     0,   531,   532,
       0,     0,     0,     0,   226,     0,     0,   533,     0,   534,
       0,   535,   536,     0,     0,     0,     0,     0,     0,   227,
       0,     0,     0,     0,     0,     0,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   722,   723,   724,   725,   726,     0,     0,   727,
     728,   729,   730,     0,   731,   732,   733,   734,   735,   736,
     737,   738,     0,     0,     0,   739,   740,   741,   742,   743,
      58,   744,   745,     0,     0,     0,     0,     0,     0,   720,
     721,     0,     0,   284,     0,     0,     0,     0,   222,     0,
       0,     0,     0,     0,   223,    59,     0,     0,     0,     0,
     224,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     225,     0,     0,     0,     0,     0,     0,     0,   226,   754,
     755,   756,   757,   758,   759,   760,   761,     0,     0,     0,
       0,     0,     0,   227,   285,     0,     0,   762,   763,   597,
     228,   229,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,     0,   722,   723,   724,
     725,   726,     0,     0,   727,   728,   729,   730,     0,   731,
     732,   733,   734,   735,   736,   737,   738,     0,     0,     0,
     739,   740,   741,   742,   743,     0,   744,   745,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    58,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   284,     0,     0,
     222,     0,     0,     0,     0,     0,   223,     0,     0,   776,
       0,    13,   224,   753,   754,   755,   756,   757,   758,   759,
     760,   761,   225,     0,     0,     0,     0,     0,     0,     0,
     226,     0,   762,   763,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   227,     0,     0,   285,     0,
      16,     0,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,     0,     0,
       0,   720,   721,     0,     0,     0,     0,     0,     0,   284,
     222,     0,     0,     0,     0,     0,   223,     0,     0,     0,
       0,    59,   224,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   225,     0,     0,     0,     0,     0,     0,     0,
     226,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   227,     0,     0,     0,     0,
     285,     0,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,     0,   722,
     723,   724,   725,   726,   720,   721,   727,   728,   729,   730,
       0,   731,   732,   733,   734,   735,   736,   737,   738,     0,
       0,     0,   739,   740,   741,   742,   743,     0,   744,   745,
       0,     0,   746,     0,     0,     0,   747,   748,   749,     0,
       0,     0,   750,     0,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   284,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   776,   751,     0,   752,   753,   754,   755,   756,   757,
     758,   759,   760,   761,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   762,   763,     0,     0,     0,     0,
       0,   720,   721,     0,     0,     0,     0,     0,     0,     0,
     285,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   722,   723,   724,   725,   726,     0,     0,   727,
     728,   729,   730,     0,   731,   732,   733,   734,   735,   736,
     737,   738,     0,     0,     0,   739,   740,   741,   742,   743,
       0,   744,   745,     0,     0,  -945,     0,     0,     0,   747,
     748,   749,     0,     0,     0,  -945,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   720,   721,     0,     0,   751,     0,   752,   753,   754,
     755,   756,   757,   758,   759,   760,   761,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   762,   763,   722,
     723,   724,   725,   726,     0,     0,   727,   728,   729,   730,
       0,   731,   732,   733,   734,   735,   736,   737,   738,     0,
       0,     0,   739,   740,   741,   742,   743,     0,   744,   745,
       0,     0,     0,     0,     0,     0,   747,   748,   749,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   720,   721,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   751,     0,   752,   753,   754,   755,   756,   757,
     758,   759,   760,   761,     0,     0,     0,     0,     0,   722,
     723,   724,   725,   726,   762,   763,   727,   728,   729,   730,
       0,   731,   732,   733,   734,   735,   736,   737,   738,     0,
       0,     0,   739,   740,   741,   742,   743,     0,   744,   745,
       0,     0,     0,     0,     0,     0,   747,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   720,   721,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   752,   753,   754,   755,   756,   757,
     758,   759,   760,   761,     0,     0,     0,     0,     0,   722,
     723,   724,   725,   726,   762,   763,   727,   728,   729,   730,
       0,   731,   732,   733,   734,   735,   736,   737,   738,     0,
       0,     0,   739,   740,   741,   742,   743,     0,   744,   745,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   720,   721,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   752,   753,   754,   755,   756,   757,
     758,   759,   760,   761,     0,     0,     0,     0,     0,   722,
     723,   724,   725,   726,   762,   763,   727,   728,   729,   730,
       0,   731,   732,   733,   734,   735,   736,   737,   738,     0,
       0,     0,   739,   740,   741,   742,   743,     0,   744,   745,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   755,   756,   757,
     758,   759,   760,   761,     0,     0,   722,   723,   724,   725,
     726,     0,     0,   727,   762,   763,   730,     0,   731,   732,
     733,   734,   735,   736,   737,   738,     0,     0,     0,   739,
     740,   741,   742,   743,     0,   744,   745,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   929,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   755,   756,   757,   758,   759,   760,
     761,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   762,   763,   228,   229,   230,   933,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,     0,   228,   229,
     230,     0,   232,   233,   234,   235,   236,   505,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,     0,
     250,   251,   252,     0,     0,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   930,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   931,     0,     0,     0,   955,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   934,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   935,   228,   229,
     230,   959,   232,   233,   234,   235,   236,   505,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,     0,
     250,   251,   252,     0,     0,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,     0,   228,   229,   230,     0,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   956,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   957,     0,     0,
       0,   963,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   960,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   961,   228,   229,   230,  1188,   232,   233,   234,
     235,   236,   505,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,   250,   251,   252,     0,     0,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,     0,   228,   229,
     230,     0,   232,   233,   234,   235,   236,   505,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,     0,
     250,   251,   252,     0,     0,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   964,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   965,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   304,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1189,   305,     0,
     306,     0,   307,   308,   309,   310,   311,  1190,   312,   313,
     314,   315,   316,   317,   318,   319,   320,   321,   322,     0,
     323,   324,   325,     0,     0,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   353,   354,
     228,   229,   230,     0,   232,   233,   234,   235,   236,   505,
     238,   239,   240,   241,   242,   243,   244,   245,   246,   247,
     248,     0,   250,   251,   252,     0,     0,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,     0,     0,   305,   355,   306,     0,
     307,   308,   309,   310,   311,     0,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,     0,   323,   324,
     325,   991,   992,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   993,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   994,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   995,   996,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   305,     0,   306,     0,
     307,   308,   309,   310,   311,   602,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,     0,   323,   324,
     325,     0,     0,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     305,     0,   306,     0,   307,   308,   309,   310,   311,     0,
     312,   313,   314,   315,   316,   317,   318,   319,   320,   321,
     322,     0,   323,   324,   325,     0,     0,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   604,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   801
};

static const yytype_int16 yycheck[] =
{
       1,    14,    15,   104,   147,   148,   196,   390,   634,   630,
     642,   174,  1108,    45,   841,   491,  1131,   601,   709,    20,
      21,    22,   600,   771,   491,   773,   572,   775,   574,  1501,
     576,   221,   705,   706,  1037,  1276,    81,     5,     8,  1464,
     402,     5,     6,  1026,    33,    20,   891,    20,    20,    50,
      51,    64,    65,    66,    86,  1522,    33,  1020,  1470,   534,
    1764,    25,  1045,    15,    16,   928,    19,    31,  1764,    20,
      40,   192,  1764,  1764,    22,   407,   408,  1764,   172,   174,
     150,   149,   175,   182,    46,   212,   173,   176,   177,   178,
     150,   202,   105,   106,   107,   108,    19,    20,   888,   131,
     132,   200,   134,   214,    68,    69,   581,   167,   150,    57,
     174,  1114,   785,  1692,  1677,   184,     7,   174,   196,   481,
     213,  1825,   149,   187,   223,   167,   174,  1690,   920,  1825,
     241,   200,  1706,  1825,  1825,   234,    33,    62,  1825,   187,
     200,   211,   237,   221,   214,   213,    69,   242,   242,   238,
     782,   211,   239,   240,   786,   242,   216,   125,   126,    50,
    1864,   125,   126,    60,    61,    34,   211,   129,  1864,   211,
    1595,  1596,  1864,  1864,   238,   866,  1755,  1864,   235,  1651,
    1743,  1744,  1649,   196,    15,    16,   213,  1612,   663,  1614,
    1795,   218,   202,   211,    63,   202,   200,   201,   176,   674,
     178,   874,   677,   366,   877,  1483,   210,  1619,   221,   210,
     237,   175,     0,   886,  1310,   358,   889,  1822,   200,     7,
     363,   173,  1337,   210,   367,   851,  1071,   216,   919,  1092,
     237,   241,   202,   243,   709,   210,   200,   210,   210,   216,
     192,  1519,    30,   875,    32,   197,    34,  1230,   200,   217,
     147,   203,    40,   217,   151,  1680,  1681,   239,   235,   210,
    1223,   130,    50,  1246,   217,   235,  1691,   244,    56,   237,
     746,   235,   285,   243,   222,   184,   608,   200,   242,   746,
    1070,    33,   149,    33,   213,   237,   234,  1861,   202,   764,
    1775,   200,    80,   202,   484,   485,   486,   166,    33,   150,
     184,  1875,   664,  1306,   201,   202,   175,   190,    60,    61,
      60,    61,   150,   210,   202,   212,   200,   200,   680,   216,
      47,   218,   219,   368,   402,    60,    61,   241,  1813,   167,
    1122,   200,   241,   210,   243,   150,   479,   125,   126,   213,
      67,   218,   216,   200,   241,   202,   213,   244,  1096,   237,
     896,   218,   167,   237,  1469,   238,   210,   149,   227,   210,
     211,   192,   213,  1604,   241,   216,   197,   210,   237,   200,
     237,  1062,   203,   211,     8,   200,  1848,   567,   568,  1127,
     926,   571,  1174,   573,   241,   575,  1018,   149,  1503,   402,
     210,   866,    33,   583,   150,   147,   211,   147,   241,   151,
     150,   151,    36,   481,   217,  1197,   484,   485,   486,   210,
     210,   167,   147,    33,   202,   150,   151,   167,   238,    60,
      61,   213,   217,   217,   237,  1051,   218,   617,   618,  1432,
     210,   200,   167,  1017,  1066,   223,  1014,   238,   238,   190,
      60,    61,   237,   237,  1117,   237,  1030,   235,   237,   200,
    1028,   213,   202,   241,   184,   211,   218,   210,   238,  1091,
     212,   211,   212,   175,   216,   150,   216,   219,   481,   219,
     200,   484,   485,   486,   130,   237,   211,   212,   491,   492,
     184,   216,   167,   150,   219,   238,   237,   210,   850,   567,
     568,   241,   244,   571,   244,   573,   200,   575,   511,   577,
     167,   863,   864,   201,   202,   583,   147,   237,   175,   244,
     151,   873,   210,  1340,   210,   238,   182,   879,   880,   881,
     218,   883,   600,   885,  1316,   887,   211,   147,   214,   222,
     217,   151,   218,   895,   200,   210,   201,   202,   175,   617,
     618,   234,   238,   241,   211,   210,   210,   184,  1644,   182,
     237,   210,   189,   218,   567,   568,    33,   223,   571,   222,
     573,   236,   575,   200,   577,   708,   184,   200,   234,   235,
     583,   212,   210,   222,   238,   216,   241,   218,   219,   238,
     217,   192,   200,    60,    61,   210,   664,   600,   200,   222,
     223,   210,   212,  1385,   784,   217,   216,   210,   218,   219,
     238,   234,   680,   244,   617,   618,   152,   153,   154,   155,
     156,   157,   625,   238,  1406,   237,    21,    22,   210,   238,
     204,   210,   210,  1169,   244,   238,     5,     6,  1631,   210,
     210,   149,  1380,   634,   152,   153,   154,   155,   156,   157,
     210,  1273,   720,   721,   217,   210,   238,   201,   202,   238,
     238,   664,    64,    65,    66,  1770,   210,   238,   238,   200,
     214,   739,   740,   741,   237,   743,   236,   680,   201,   202,
     147,   201,   862,   238,   151,   865,   210,   210,   200,   869,
     210,   201,   202,   210,   762,   218,   200,   241,   234,   235,
     210,  1806,  1807,   105,   106,   107,   108,   710,   218,  1174,
     210,   210,   130,  1178,   238,   210,   784,  1069,   241,   175,
     210,   238,   210,  1075,   201,   202,   234,   235,   184,   210,
     213,   241,  1197,   210,   217,   200,   236,  1202,  1090,   238,
    1845,   218,   200,   238,   200,   212,   141,   142,   238,   216,
     238,   218,   219,   210,   149,   236,   151,   152,   153,   154,
     155,   156,   157,   158,   241,    79,   210,   162,   163,   164,
     165,   166,  1554,   210,   201,   202,   202,   244,   175,   236,
      94,   784,   850,   210,   217,    99,   222,   101,   242,   214,
    1572,   218,   236,   218,   862,   863,   864,   865,   234,   236,
     184,   869,   238,   200,   237,   873,   130,   798,   190,   800,
      12,   879,   880,   881,   241,   883,   200,   885,   200,   887,
     888,    23,    24,   200,   201,   201,   202,   895,   201,   202,
     200,  1296,  1012,   210,   210,  1015,   190,   210,  1303,   234,
     235,  1021,   218,   190,   190,   218,   200,   850,   190,   204,
     205,   206,   207,   200,   200,    21,    22,   190,   200,   862,
     863,   864,   865,   854,   190,   241,   869,   200,   241,   210,
     873,   234,   213,    57,   200,   216,   879,   880,   881,    63,
     883,    57,   885,   200,   887,   888,   377,    63,   236,   201,
     202,   211,   895,   214,  1676,  1677,   387,   218,   210,    33,
    1682,   210,   200,  1369,   213,  1371,   218,   216,  1690,   400,
    1692,   979,  1369,  1730,  1371,   131,   132,   133,   134,   135,
     136,   137,   138,   227,    57,    57,    60,    61,    57,   241,
      63,    63,    57,    57,    63,   200,   927,   214,    63,    63,
     213,   218,   241,   242,  1012,   214,  1014,  1015,   214,   218,
     204,   167,   218,  1021,  1736,   549,   550,   176,   552,   553,
    1028,  1743,  1744,   179,   180,   181,   214,   214,   214,   202,
     218,   218,   218,  1755,   214,   141,   142,   214,   218,   200,
     214,   218,   174,   149,   218,   151,   152,   153,   154,   155,
     156,   157,   158,    57,    21,    22,   162,   163,   164,   165,
     166,  1069,  1070,    33,   214,   211,  1139,  1075,   218,  1012,
      66,  1014,  1015,   147,   176,   177,   178,   151,  1021,  1484,
     213,  1838,  1090,   204,   205,  1028,  1689,    10,    11,  1381,
      60,    61,   550,  1815,   552,   553,   201,  1028,   129,  1030,
     193,   194,  1394,   204,   205,   206,   204,   205,   206,   200,
    1867,  1042,   174,   219,   220,   221,   222,   223,  1347,  1348,
    1051,  1241,  1053,    55,    56,    57,  1069,  1070,   234,   235,
     217,   217,  1075,   217,   237,  1738,   217,   217,   212,   833,
     834,   835,   216,   237,   218,   219,   217,  1090,   237,   217,
     217,    21,    22,   584,   217,   214,   217,   235,   201,  1252,
     242,    35,    35,  1714,   200,   596,   242,   237,   200,   217,
     244,   200,    22,   200,   141,   142,   242,   147,   235,   213,
     236,   151,   149,   236,   151,   152,   153,   154,   155,   156,
     157,   158,   242,   200,   217,   162,   163,   164,   165,   166,
     217,   217,   237,   217,  1609,  1136,   217,   638,   217,   237,
      21,    22,   217,  1144,  1145,   217,  1772,   237,  1291,   237,
     218,   200,   237,    43,   237,   237,  1157,  1632,  1159,  1160,
    1161,  1162,  1163,  1241,   665,   666,  1167,   237,   237,   670,
     235,   672,   212,   236,   238,   237,   216,   238,   218,   219,
     238,   238,  1325,  1809,   221,   222,   223,   688,   689,   690,
     691,   692,   693,  1383,  1357,   210,   237,   234,   235,   241,
     236,   141,   142,    33,   244,   238,  1396,   237,  1398,   149,
     218,   237,   152,   153,   154,   155,   156,   157,   217,  1602,
     217,   217,   162,   163,   164,   165,   166,   217,  1241,    75,
      60,    61,   733,    79,   237,   736,   737,    33,    13,   237,
     210,     4,   237,   237,   236,   200,   200,    93,    94,   238,
     213,    33,    98,    99,   100,   101,   236,  1258,   139,   140,
     141,   142,   763,   200,    60,    61,   200,   200,   149,   200,
     151,   152,   153,   154,   155,   156,   157,   158,    60,    61,
     210,   162,   163,   164,   165,   166,   238,   168,   169,   790,
     238,   238,  1767,   238,   234,   235,   238,   238,   217,   238,
     238,   217,   238,  1381,     1,  1383,   238,   211,   237,   236,
     238,   237,   237,   237,   237,  1505,  1394,   147,  1396,   237,
    1398,   151,   237,   237,   237,   236,   236,   236,   829,   217,
     237,   217,    33,    21,    22,   237,   237,   217,   219,   220,
     221,   222,   223,   237,   237,   237,   847,   218,   218,   218,
     237,   147,   853,   234,   235,   151,   238,    43,   237,    60,
      61,   238,   211,   238,   237,   147,    43,   868,  1381,   151,
    1383,   238,   238,   238,   238,   237,   237,  1567,   238,   238,
     238,  1394,   212,  1396,   238,  1398,   216,   238,   218,   219,
     213,    43,   893,   894,   237,   200,  1586,   238,    33,   237,
     237,   243,   238,   904,   192,  1548,   200,   242,   909,  1771,
     911,   200,   913,  1514,   244,    10,   212,   918,     9,    13,
     216,    42,   218,   219,    66,    60,    61,  1505,   237,   237,
     212,   217,   236,   236,   216,   237,   218,   219,   200,   200,
    1441,   200,   243,  1444,    33,   243,   147,   200,   244,     8,
     151,   139,   140,   141,   142,   143,   243,  1470,   146,   147,
     148,   149,   244,   151,   152,   153,   154,   155,   156,   157,
     158,    60,    61,   200,   162,   163,   164,   165,   166,   237,
     168,   169,   238,   200,   218,   208,   200,   238,   176,  1567,
     178,   992,  1505,   200,   200,   996,   237,   200,   200,    14,
     211,   211,   192,   238,   213,   237,   237,   237,  1586,   238,
    1511,   212,   147,   238,   238,   216,   151,   218,   219,   238,
     238,  1022,    33,   238,   211,   238,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,    43,    37,   238,   243,
      33,   211,   238,   244,   237,   243,   234,   235,    67,    60,
      61,  1052,   218,   238,  1567,   237,   237,   237,   147,   238,
      70,   237,   151,   237,   237,  1708,   237,    60,    61,   237,
    1713,   238,   237,  1586,   237,  1076,   237,   212,   200,   237,
      33,   216,    43,   218,   219,   238,   200,   237,   237,   200,
     238,   238,   238,   238,   237,   204,   237,   131,   132,   133,
     134,   135,   136,   137,   138,   237,  1619,    60,    61,   244,
     237,   237,   241,   238,  1646,   200,   150,    33,   238,   238,
     243,  1122,   238,   212,   242,  1768,  1769,   216,  1129,   218,
     219,  1132,   238,   167,   238,   238,   147,  1138,   238,  1140,
     151,   509,   510,   241,    12,   179,   180,   181,  1649,   210,
     238,   238,   238,    33,   147,   244,   238,   238,   151,   238,
     528,   529,   530,   531,   532,   238,   238,   238,   210,  1670,
    1171,   238,  1173,   238,  1175,   238,  1177,   211,  1179,   241,
      60,    61,  1183,  1826,  1185,   238,  1187,   238,   238,   238,
    1833,   237,    53,  1771,   147,  1196,  1839,   241,   151,   238,
     238,   212,   238,  1204,   236,   216,   243,   218,   219,   242,
     236,   694,   243,   237,   243,   242,   238,    80,   243,   212,
       1,    83,  1865,   216,  1874,   218,   219,  1228,  1872,   800,
     217,  1348,   710,   244,  1046,   714,   784,  1238,     1,  1499,
     800,  1672,  1243,  1734,  1245,  1675,    54,  1589,  1045,  1735,
    1735,   244,   620,  1254,  1246,  1045,  1257,   237,  1771,   212,
     665,   360,    -1,   216,  1765,   218,   219,   147,    -1,   491,
     491,   151,    -1,  1774,    -1,  1276,    -1,    -1,    -1,   491,
      -1,    -1,    -1,   491,    -1,    -1,   491,    33,    -1,    -1,
      -1,   244,   491,    -1,    -1,    21,    22,    -1,  1299,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1307,  1308,  1309,    -1,
      -1,  1812,    -1,    -1,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1326,    -1,    -1,  1329,  1830,
      -1,    -1,   212,    -1,    -1,    -1,   216,  1338,   218,   219,
      -1,    -1,    -1,  1844,    -1,  1846,    -1,    -1,    -1,    -1,
    1851,    -1,    -1,    -1,   722,   723,    -1,    -1,   726,   727,
     728,   729,    -1,   731,   244,    -1,    -1,  1868,    -1,  1870,
     738,    -1,    -1,    -1,   742,   743,   744,   745,   746,   747,
     748,   749,   750,   751,   752,   753,   754,   755,   756,   757,
     758,   759,   760,   761,    -1,    -1,    -1,    -1,    -1,    -1,
    1401,   147,  1403,    -1,    -1,   151,    -1,    -1,    -1,  1410,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,   155,
     156,   157,   158,    -1,    -1,    -1,   162,   163,   164,   165,
     166,    -1,   168,   169,    -1,    -1,   172,    -1,    -1,  1450,
     176,   177,   178,    -1,    -1,    -1,   182,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   212,  1468,    -1,    -1,
     216,   839,   218,   219,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1485,  1486,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   244,    -1,
      -1,    -1,    -1,    -1,    -1,  1506,    -1,    -1,   234,   235,
      -1,  1512,  1513,    -1,    -1,    -1,    -1,  1518,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1554,    -1,    -1,    -1,    -1,    -1,   927,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1572,    -1,    -1,    -1,    -1,  1577,    -1,    -1,  1580,
      -1,  1582,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1604,  1605,  1606,  1607,  1608,    -1,    38,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1625,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1645,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1042,    -1,    -1,    -1,  1679,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1696,    -1,    -1,    -1,    -1,
    1701,  1702,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
      -1,   150,   151,   152,   153,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,
     169,   170,   171,    -1,    -1,    -1,    -1,   176,   177,   178,
     179,   180,   181,   182,    -1,    -1,    -1,    -1,  1759,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   211,   212,    -1,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,  1796,    -1,   226,   227,    -1,
      -1,    -1,    -1,    -1,    -1,   234,   235,    -1,    -1,  1810,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1819,    -1,
      -1,    -1,    -1,    -1,  1192,    -1,    -1,    -1,  1829,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,     5,     6,     7,    -1,     9,
      10,    11,  1853,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    38,    39,
      40,    -1,    42,    -1,    44,    45,    -1,    -1,    48,    -1,
      50,    51,    52,    -1,    54,    55,    -1,    -1,    58,    59,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1359,  1360,  1361,  1362,  1363,  1364,  1365,  1366,  1367,
    1368,  1369,  1370,  1371,  1372,  1373,  1374,  1375,  1376,  1377,
    1378,  1379,    -1,   173,    -1,   175,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   183,   184,   185,   186,   187,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,   202,   203,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
     220,   221,    -1,   223,    -1,    -1,   226,   227,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   235,    -1,   237,    -1,   239,
     240,   241,   242,   243,     1,    -1,    -1,    -1,     5,     6,
       7,    -1,     9,    10,    11,    -1,    13,    -1,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    29,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    39,    40,    -1,    42,    -1,    44,    45,    -1,
      -1,    48,    -1,    50,    51,    52,    -1,    54,    55,    -1,
      -1,    58,    59,  1511,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,    -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   141,   142,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,   175,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   183,   184,   185,   186,
     187,    -1,   189,    -1,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,    -1,   202,   203,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   219,   220,   221,    -1,   223,    -1,    -1,   226,
     227,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   235,    -1,
     237,    -1,   239,   240,   241,   242,   243,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,    -1,    -1,
      -1,   176,   177,   178,   179,   180,   181,   182,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1764,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1774,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   211,   212,    -1,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,   234,
     235,     5,     6,     7,    -1,     9,    10,    11,    -1,    13,
      -1,    15,    16,    17,    18,    19,    -1,  1825,    -1,    -1,
      -1,    25,    26,    27,    28,    29,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    38,    39,    40,    -1,    42,    -1,
      44,    45,    -1,  1851,    48,    -1,    50,    51,    52,    -1,
      54,    55,    -1,    -1,    58,    59,  1864,    -1,    -1,    -1,
      -1,    65,  1870,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,
      -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,
     184,   185,   186,   187,    -1,   189,    -1,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,   202,   203,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,   220,   221,    -1,   223,
      -1,    -1,   226,   227,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   235,    -1,   237,    -1,   239,   240,   241,   242,   243,
       1,    -1,    -1,    -1,     5,     6,     7,    -1,     9,    10,
      11,    -1,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    40,
      -1,    42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,
      51,    52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   173,    -1,   175,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   183,   184,   185,   186,   187,    -1,   189,    -1,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      -1,   202,   203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,   220,
     221,    -1,   223,    -1,    -1,   226,   227,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   235,    -1,   237,    -1,   239,   240,
     241,   242,   243,     1,    -1,    -1,    -1,     5,     6,     7,
      -1,     9,    10,    11,    -1,    13,    -1,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    29,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      38,    39,    40,    -1,    42,    -1,    44,    45,    -1,    -1,
      48,    -1,    50,    51,    52,    -1,    54,    55,    -1,    -1,
      58,    59,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   173,    -1,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   183,   184,   185,   186,   187,
      -1,   189,    -1,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,   202,   203,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   219,   220,   221,    -1,   223,    -1,    -1,   226,   227,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   235,    -1,   237,
      -1,   239,   240,   241,   242,   243,     1,    -1,    -1,    -1,
       5,     6,     7,    -1,     9,    10,    11,    -1,    13,    -1,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    26,    27,    28,    29,    -1,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    39,    40,    -1,    42,    -1,    44,
      45,    -1,    -1,    48,    -1,    50,    51,    52,    -1,    54,
      55,    -1,    -1,    58,    59,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,
     175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,   184,
     185,   186,   187,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,   202,   203,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   219,   220,   221,    -1,   223,    -1,
      -1,   226,   227,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     235,    -1,   237,    -1,   239,   240,   241,   242,   243,     5,
       6,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    70,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,   184,   185,
     186,   187,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,   203,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,   220,   221,    -1,   223,    -1,    -1,
     226,   227,    -1,    -1,    -1,    -1,    -1,     5,     6,   235,
      -1,   237,   238,   239,   240,    13,   242,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    49,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   173,    -1,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   183,   184,   185,   186,   187,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,   203,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   219,   220,   221,    -1,   223,    -1,    -1,   226,   227,
      -1,    -1,    -1,    -1,    -1,     5,     6,   235,    -1,   237,
      -1,   239,   240,    13,   242,    15,    16,    17,    18,    19,
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
     150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   173,    -1,   175,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   183,   184,   185,   186,   187,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
     220,   221,    -1,   223,    -1,    -1,   226,   227,    -1,    -1,
      -1,    -1,    -1,     5,     6,   235,    -1,   237,    -1,   239,
     240,    -1,   242,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    70,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   173,    -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   183,   184,   185,   186,   187,    -1,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,   220,   221,
      -1,   223,    -1,    -1,   226,   227,    -1,    -1,    -1,    -1,
      -1,     5,     6,   235,    -1,   237,    -1,   239,   240,    -1,
     242,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,
      -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,
     184,   185,   186,   187,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,   203,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,   220,   221,    -1,   223,
      -1,    -1,   226,   227,    -1,    -1,    -1,    -1,    -1,     5,
       6,   235,    -1,   237,   238,   239,   240,    -1,   242,    15,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,   184,   185,
     186,   187,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,   203,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,   220,   221,    -1,   223,    -1,    -1,
     226,   227,    -1,    -1,    -1,    -1,    -1,     5,     6,   235,
      -1,   237,   238,   239,   240,    -1,   242,    15,    16,    17,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   173,    -1,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   183,   184,   185,   186,   187,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,   203,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   219,   220,   221,    -1,   223,    -1,    -1,   226,   227,
      -1,    -1,    -1,    -1,    -1,     5,     6,   235,    -1,   237,
      -1,   239,   240,    -1,   242,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   173,    -1,   175,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   183,   184,   185,   186,   187,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
     220,   221,    -1,   223,    -1,    -1,   226,   227,    -1,    -1,
      -1,    -1,    -1,     5,     6,   235,    -1,   237,   238,   239,
     240,    -1,   242,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   173,    -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   183,   184,   185,   186,   187,    -1,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,   220,   221,
      -1,   223,    -1,    -1,   226,   227,    -1,    -1,    -1,    -1,
      -1,     5,     6,   235,    -1,   237,    -1,   239,   240,    -1,
     242,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,
      -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,
     184,   185,   186,   187,    -1,   189,    -1,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,   203,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,   220,   221,    -1,   223,
      -1,    -1,   226,   227,    -1,    -1,    -1,    -1,    -1,     5,
       6,   235,    -1,   237,   238,   239,   240,    -1,   242,    15,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,   184,   185,
     186,   187,    -1,   189,    -1,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,   203,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,   220,   221,    -1,   223,    -1,    -1,
     226,   227,    -1,    -1,    -1,    -1,    -1,     5,     6,   235,
      -1,   237,   238,   239,   240,    -1,   242,    15,    16,    17,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   173,    -1,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   183,   184,   185,   186,   187,
      -1,   189,    -1,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,   203,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   219,   220,   221,    -1,   223,    -1,    -1,   226,   227,
      -1,    -1,    -1,    -1,    -1,     5,     6,   235,    -1,   237,
     238,   239,   240,    -1,   242,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   173,    -1,   175,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   183,   184,   185,   186,   187,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
     220,   221,    -1,   223,    -1,    -1,   226,   227,    -1,    -1,
      -1,    -1,    -1,     5,     6,   235,    -1,   237,    -1,   239,
     240,    -1,   242,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   173,    -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   183,   184,   185,   186,   187,    -1,   189,    -1,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,   220,   221,
      -1,   223,    -1,    -1,   226,   227,    -1,    -1,    -1,    -1,
      -1,     5,     6,   235,    -1,   237,    -1,   239,   240,    -1,
     242,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,
      -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,
     184,   185,   186,   187,    -1,   189,    -1,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,   203,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,   220,   221,    -1,   223,
      -1,    -1,   226,   227,    -1,    -1,    -1,    -1,    -1,     5,
       6,   235,   236,   237,    -1,   239,   240,    -1,   242,    15,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,   184,   185,
     186,   187,    -1,   189,    -1,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,   203,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,   220,   221,    -1,   223,    -1,    -1,
     226,   227,    -1,    -1,    -1,    -1,    -1,     5,     6,   235,
     236,   237,    -1,   239,   240,    -1,   242,    15,    16,    17,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   173,    -1,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   183,   184,   185,   186,   187,
      -1,   189,    -1,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,   203,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   219,   220,   221,    -1,   223,    -1,    -1,   226,   227,
      -1,    -1,    -1,    -1,    -1,     5,     6,   235,    -1,   237,
      -1,   239,   240,    13,   242,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   173,    -1,   175,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   183,   184,   185,   186,   187,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
     220,   221,    -1,   223,    -1,    -1,   226,   227,    -1,    -1,
      -1,    -1,    -1,     5,     6,   235,    -1,   237,    -1,   239,
     240,    -1,   242,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   173,    -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   183,   184,   185,   186,   187,    -1,   189,    -1,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,   220,   221,
      -1,   223,    -1,    -1,   226,   227,    -1,    -1,    -1,    -1,
      -1,     5,     6,   235,    -1,   237,    -1,   239,   240,    -1,
     242,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    61,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,
      -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,
     184,   185,   186,   187,    -1,   189,    -1,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,   203,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,   220,   221,    -1,   223,
      -1,    -1,   226,   227,    -1,    -1,    -1,    -1,    -1,     5,
       6,   235,    -1,   237,    -1,   239,   240,    -1,   242,    15,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,   184,   185,
     186,   187,    -1,   189,    -1,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,   203,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,   220,   221,    -1,   223,    -1,    -1,
     226,   227,    -1,    -1,    -1,    -1,    -1,     5,     6,   235,
      -1,   237,    -1,   239,   240,    -1,   242,    15,    16,    17,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   173,    -1,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   183,   184,   185,   186,   187,
      -1,   189,    -1,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,   203,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   219,   220,   221,    -1,   223,    -1,    -1,   226,   227,
      -1,    -1,    -1,    -1,    -1,     5,     6,   235,    -1,   237,
      -1,   239,   240,    -1,   242,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   173,    -1,   175,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   183,   184,   185,   186,   187,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
     220,   221,    -1,   223,    -1,    -1,   226,   227,    -1,    -1,
      -1,    -1,    -1,     5,     6,   235,    -1,   237,   238,   239,
     240,    -1,   242,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    56,    -1,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   173,    -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   183,   184,   185,   186,   187,    -1,   189,    -1,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,   220,   221,
      -1,   223,    -1,    -1,   226,   227,    -1,    -1,    -1,    -1,
      -1,     5,     6,   235,    -1,   237,    -1,   239,   240,    -1,
     242,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,
      -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,
     184,   185,   186,   187,    -1,   189,    -1,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,   203,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,   220,   221,    -1,   223,
      -1,    -1,   226,   227,    -1,    -1,    -1,    -1,    -1,     5,
       6,   235,    -1,   237,    -1,   239,   240,    -1,   242,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    52,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,   184,   185,
     186,   187,    -1,   189,    -1,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,   203,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,   220,   221,    -1,   223,    -1,    -1,
     226,   227,    -1,    -1,    -1,     5,     6,    -1,    -1,   235,
      -1,   237,    -1,   239,   240,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   173,    -1,   175,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   183,   184,   185,   186,   187,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,
     220,   221,    -1,   223,    -1,    -1,   226,   227,    -1,    -1,
      -1,     5,     6,    -1,    -1,   235,    -1,   237,    -1,   239,
     240,    15,    16,    17,    18,    19,    -1,    -1,    22,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,
      -1,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,
     184,   185,   186,   187,    -1,   189,    -1,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,   203,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,   220,   221,    -1,   223,
      -1,    -1,   226,   227,    -1,    -1,    -1,     5,     6,    -1,
      -1,   235,    -1,   237,    -1,   239,   240,    15,    16,    17,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   173,    -1,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   183,   184,   185,   186,   187,
      -1,   189,    -1,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    19,   203,    -1,    -1,    -1,    -1,
      25,    -1,    -1,    -1,    21,    22,    31,    -1,    -1,    -1,
      -1,   219,   220,   221,    -1,   223,    41,    -1,   226,   227,
      -1,    -1,    -1,    -1,    49,    -1,    -1,   235,    -1,   237,
      -1,   239,   240,    -1,    -1,    -1,    -1,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,   155,   156,
     157,   158,    -1,    -1,    -1,   162,   163,   164,   165,   166,
     175,   168,   169,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    -1,    -1,   188,    -1,    -1,    -1,    -1,    19,    -1,
      -1,    -1,    -1,    -1,    25,   200,    -1,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,   216,
     217,   218,   219,   220,   221,   222,   223,    -1,    -1,    -1,
      -1,    -1,    -1,    64,   239,    -1,    -1,   234,   235,   244,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,   155,   156,   157,   158,    -1,    -1,    -1,
     162,   163,   164,   165,   166,    -1,   168,   169,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   188,    -1,    -1,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,   200,
      -1,   202,    31,   215,   216,   217,   218,   219,   220,   221,
     222,   223,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    -1,   234,   235,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,   239,    -1,
     241,    -1,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,   188,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,
      -1,   200,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
     239,    -1,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,    -1,   139,
     140,   141,   142,   143,    21,    22,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,   155,   156,   157,   158,    -1,
      -1,    -1,   162,   163,   164,   165,   166,    -1,   168,   169,
      -1,    -1,   172,    -1,    -1,    -1,   176,   177,   178,    -1,
      -1,    -1,   182,    -1,    -1,    -1,   175,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   188,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,   212,    -1,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   234,   235,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     239,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,   155,   156,
     157,   158,    -1,    -1,    -1,   162,   163,   164,   165,   166,
      -1,   168,   169,    -1,    -1,   172,    -1,    -1,    -1,   176,
     177,   178,    -1,    -1,    -1,   182,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,   212,    -1,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   234,   235,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,   155,   156,   157,   158,    -1,
      -1,    -1,   162,   163,   164,   165,   166,    -1,   168,   169,
      -1,    -1,    -1,    -1,    -1,    -1,   176,   177,   178,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   212,    -1,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,    -1,    -1,    -1,    -1,    -1,   139,
     140,   141,   142,   143,   234,   235,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,   155,   156,   157,   158,    -1,
      -1,    -1,   162,   163,   164,   165,   166,    -1,   168,   169,
      -1,    -1,    -1,    -1,    -1,    -1,   176,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,    -1,    -1,    -1,    -1,    -1,   139,
     140,   141,   142,   143,   234,   235,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,   155,   156,   157,   158,    -1,
      -1,    -1,   162,   163,   164,   165,   166,    -1,   168,   169,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,    -1,    -1,    -1,    -1,    -1,   139,
     140,   141,   142,   143,   234,   235,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,   155,   156,   157,   158,    -1,
      -1,    -1,   162,   163,   164,   165,   166,    -1,   168,   169,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,   218,   219,
     220,   221,   222,   223,    -1,    -1,   139,   140,   141,   142,
     143,    -1,    -1,   146,   234,   235,   149,    -1,   151,   152,
     153,   154,   155,   156,   157,   158,    -1,    -1,    -1,   162,
     163,   164,   165,   166,    -1,   168,   169,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,   218,   219,   220,   221,   222,
     223,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   234,   235,    71,    72,    73,    19,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   190,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   200,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   190,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,    71,    72,
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
     118,   119,   120,   121,   122,   123,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   190,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,    -1,    -1,
      -1,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   190,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   200,    71,    72,    73,    19,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   190,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    35,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   190,    71,    -1,
      73,    -1,    75,    76,    77,    78,    79,   200,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    71,   200,    73,    -1,
      75,    76,    77,    78,    79,    -1,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,   152,   153,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   190,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   234,   235,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    73,    -1,
      75,    76,    77,    78,    79,   200,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
      71,    -1,    73,    -1,    75,    76,    77,    78,    79,    -1,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   246,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   125,   126,   202,   223,   235,   241,   248,   249,   253,
     262,   264,   265,   270,   319,   325,   356,   446,   454,   463,
     473,   523,   528,   533,    19,    20,    69,   200,   309,   310,
     311,   192,   271,   272,   212,   267,   268,    57,    63,   451,
     452,   455,   200,   239,   251,   534,   524,   529,   175,   200,
     344,    34,    63,   130,   166,   227,   237,   314,   315,   316,
     317,   344,   248,   248,   248,     8,    36,   474,    62,   442,
     211,   210,   213,   210,   182,   200,   223,   234,   266,   235,
     266,   200,   248,   248,   442,   451,   451,   451,   200,   175,
     263,   316,   316,   316,   237,   176,   177,   178,   210,   236,
     130,   324,   464,     5,     6,   470,    57,    63,   443,    15,
      16,   173,   192,   197,   200,   203,   237,   255,   310,   192,
     272,   222,   222,   266,   222,   222,   234,   200,    22,    57,
     269,   200,   453,    57,    63,   250,   200,   200,   200,   200,
     204,   261,   238,   311,   316,   316,   316,   316,   202,   276,
     277,    57,    63,   326,   328,    57,    63,   457,   130,   130,
      57,    63,   471,   242,   447,   200,   204,   205,   206,   254,
      15,    16,   192,   197,   200,   255,   307,   308,   266,   266,
     266,   200,   200,   236,   200,   456,   211,   227,   252,   213,
     484,   277,   277,   204,   238,   202,   329,   200,   458,   475,
     444,   174,   312,   412,   204,   205,   206,   210,   238,   269,
     269,   211,    19,    25,    31,    41,    49,    64,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   188,   239,   344,   478,   480,   481,
     485,   491,   493,   522,    66,    79,    94,    99,   101,   201,
     461,   462,   525,   530,    35,    71,    73,    75,    76,    77,
      78,    79,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    93,    94,    95,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   141,   142,   200,   322,   323,   327,   213,
     459,   129,   468,   469,   243,   248,   445,   310,   174,   200,
     438,   441,   307,   522,   217,   217,   217,   237,   217,   217,
     237,   484,   217,   217,   217,   217,   217,   237,   344,   217,
     237,    33,    60,    61,   147,   151,   212,   216,   219,   244,
     235,   490,   214,   201,   535,   242,   242,    21,    22,    38,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   150,   151,
     152,   153,   167,   168,   169,   170,   171,   176,   177,   178,
     179,   180,   181,   182,   211,   212,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   226,   227,   234,   235,
      35,    35,   237,   320,   277,    75,    79,    93,    94,    98,
      99,   100,   101,   479,   462,   200,   277,   412,   277,   310,
     210,   213,   216,   436,   494,   500,   502,     5,     6,    15,
      16,    17,    18,    19,    25,    27,    31,    39,    45,    48,
      51,    55,    65,    68,    69,    80,   125,   126,   127,   141,
     142,   173,   183,   184,   185,   186,   187,   189,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   203,   219,   220,
     221,   226,   227,   235,   237,   239,   240,   242,   260,   262,
     336,   344,   349,   361,   368,   371,   374,   378,   380,   382,
     383,   385,   390,   393,   394,   395,   410,   411,   478,   539,
     547,   558,   561,   574,   575,   578,   579,   504,   498,   200,
     217,   506,   508,   510,   512,   514,   516,   518,   520,   394,
     217,   237,   492,   496,   150,   341,   372,   394,    33,   216,
      33,   216,   235,   244,   236,   394,   235,   244,   491,   242,
     526,   531,   200,   323,   200,   323,   200,   236,    22,   200,
     236,   187,   238,   412,   422,   423,   424,   149,   213,   321,
     172,   242,   333,   373,   242,   213,   467,   476,   184,   200,
     437,   440,   277,   200,   491,   150,   167,   211,   435,   522,
     522,   489,   522,   217,   217,   217,   344,   346,   480,   538,
     547,   558,   561,   574,   575,   578,   579,   344,   217,     5,
     125,   126,   217,   237,   217,   237,   237,   217,   217,   217,
     237,   217,   237,   217,   237,   217,   217,   237,    19,   217,
     217,   395,   395,   184,   189,   217,   344,   384,   237,   237,
     237,   237,   237,   237,   259,   395,   395,   395,   395,   395,
      13,    49,   341,   190,   200,   372,   540,   542,   573,   237,
     235,   318,   173,   242,   374,   379,   379,   379,   379,   238,
      21,    22,   139,   140,   141,   142,   143,   146,   147,   148,
     149,   151,   152,   153,   154,   155,   156,   157,   158,   162,
     163,   164,   165,   166,   168,   169,   172,   176,   177,   178,
     182,   212,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   234,   235,   237,   522,   522,   238,   218,   486,
     522,   320,   522,   320,   522,   320,   200,   425,   426,   522,
     200,   428,   429,   238,   497,   372,   343,   522,   394,   238,
     210,   577,   236,   236,   236,   394,   536,   425,   427,   428,
     430,   200,   323,   131,   132,   133,   134,   135,   136,   137,
     138,   167,   179,   180,   181,   131,   132,   133,   134,   135,
     136,   137,   138,   150,   167,   179,   180,   181,   211,   237,
       7,    50,   355,   241,   210,   241,   238,   522,   522,   150,
     395,   334,   465,   344,   241,   242,   472,   237,    43,   210,
     213,   436,   248,   435,   394,   218,   218,   218,   201,   210,
     247,   248,   488,   548,   550,   347,   237,   217,   237,   369,
     217,   217,   217,   568,   372,   491,   394,   572,   394,   362,
     364,   396,   394,   366,   394,   570,   372,   556,   559,   372,
     217,   552,   491,   237,   237,   386,   388,   394,   394,   394,
     394,   394,   394,   206,   207,   254,   237,    13,   236,   237,
     150,   167,   211,   431,   577,   210,   577,   238,   277,    70,
     235,   238,   372,   542,   317,     4,   377,   340,   318,    19,
     190,   200,   478,    19,   190,   200,   478,   395,   395,   395,
     395,   395,   395,   200,   395,   190,   200,   394,   190,   200,
     190,   200,   394,   394,   395,    19,   190,   200,   478,    19,
     190,   200,   478,    19,   190,   200,   478,   395,   395,   478,
     395,   395,   395,   574,   579,   395,   395,   395,   395,    22,
     395,   395,   395,   395,   395,   395,   395,   395,   395,   395,
     395,   152,   153,   190,   200,   234,   235,   391,   478,   394,
     238,   372,   218,   218,   200,   482,   218,   321,   218,   321,
     218,   321,   213,   218,   488,   213,   218,   488,   343,   577,
     218,   488,   150,   394,   236,   200,   483,   248,   284,   285,
     284,   285,   394,   184,   200,   432,   433,   477,   424,   424,
     424,   395,   340,   200,   448,   450,   418,   394,   200,   184,
     200,   491,   435,   394,   248,   495,   501,   503,   522,   491,
     491,   522,    70,   372,   542,   546,   200,   394,   522,   562,
     564,   566,   491,   577,   218,   488,   210,   577,   238,   491,
     491,   491,   238,   491,   238,   491,   577,   491,   426,   577,
     554,   429,   218,   394,   394,   491,   320,   238,   238,   238,
     238,   238,   238,   394,   184,   200,   237,   298,   237,   394,
     394,   394,   238,   190,   200,    13,   342,   572,   200,   238,
     542,   540,   210,   238,   238,   236,   237,   320,     1,    26,
      28,    29,    38,    40,    44,    52,    54,    58,    59,    65,
     128,   242,   243,   248,   273,   274,   283,   294,   295,   297,
     299,   300,   301,   302,   303,   304,   305,   306,   337,   345,
     350,   351,   352,   353,   354,   356,   360,   381,   395,   377,
     217,   237,   217,   237,   237,   237,   236,   237,   237,   237,
     236,   236,   217,   237,   217,   237,   217,   237,    19,   190,
     200,   478,   213,   190,   200,   394,   237,   237,   190,   200,
     394,     1,   237,   236,   210,   238,   505,   499,   210,   218,
     241,   507,   218,   511,   218,   515,   218,   522,   519,   425,
     522,   521,   428,   218,   238,   492,   522,   394,   211,   247,
     449,   460,   248,   425,   527,   428,   532,   238,   237,    43,
     210,   213,   216,   431,   335,   211,   449,   460,    40,   202,
     243,   319,   419,   238,   237,    43,   248,   435,   394,   248,
     218,   218,   218,   542,   238,   238,   238,   218,   488,   238,
     218,   491,   426,   429,   218,   238,   237,   491,   394,   238,
     218,   218,   218,   218,   218,   238,   218,   218,   238,   491,
     218,   377,   238,   238,   218,   321,   237,   213,   257,   237,
      43,   200,   358,    20,   210,   298,   238,   237,   167,   431,
     237,   243,   577,   238,   210,   236,   235,   540,   150,   167,
     200,   211,   216,   375,   376,   321,   150,   394,   333,    61,
     394,   200,   200,   248,   192,    58,   394,   277,   150,   394,
     338,   248,   248,    10,    10,    11,   281,    13,     9,    42,
     248,   248,   248,   248,   248,   248,    66,   357,   248,   131,
     132,   133,   134,   135,   136,   137,   138,   144,   145,   150,
     159,   160,   161,   167,   170,   171,   179,   180,   181,   211,
     320,   398,   394,   402,   394,   235,   238,   372,   540,   394,
     394,   238,   372,   394,   400,   394,   404,   394,   408,   394,
     217,   237,   395,   237,   236,   394,   235,   238,   372,   540,
     237,   236,   392,   238,   372,   200,   487,   200,   509,   513,
     517,   492,   394,   200,   247,   537,   243,   243,   394,   200,
     184,   200,   522,   394,   243,   394,   448,   466,   200,     8,
     412,   417,   394,   200,   394,   248,   549,   551,   348,   238,
     237,   200,   370,   218,   218,   218,   569,   342,   218,   363,
     365,   397,   367,   571,   557,   560,   218,   553,   237,   277,
     387,   218,   238,   372,   258,   208,   394,   200,   210,   238,
     372,   184,   200,   237,    20,   167,   431,   394,   394,   394,
     298,   238,   540,   238,   200,   200,   237,   200,   200,   210,
     238,   277,   394,    14,   394,   211,   211,   213,   192,   333,
     394,   340,   237,   237,   235,   312,   313,   313,   237,   237,
     242,   359,   412,   395,   395,   395,   395,   395,   395,   395,
     395,   395,   395,   395,   574,   579,   395,   395,   574,   579,
     395,   395,   395,   395,   395,   395,   395,   395,   321,   491,
     238,   522,   238,   540,   210,   238,   238,   238,   238,   238,
     238,   491,   238,   522,   238,   522,   238,   406,   394,   394,
     238,   540,   210,   238,   238,   394,   238,   211,   243,   238,
     237,    43,   431,    37,   330,   243,   211,    57,    63,   415,
      67,   416,   248,   238,   248,   237,   237,   394,   218,   563,
     565,   567,   237,   238,   237,   237,   237,   237,   237,   237,
      70,   546,   237,   555,   237,   238,   394,   333,   384,   389,
     238,   256,   238,   200,   238,   237,    43,   358,   372,   394,
     394,   238,    20,   236,   200,   375,   373,   333,   522,   394,
     339,   394,   394,   311,   237,   237,    56,   394,   358,   439,
     440,   277,   218,   218,   236,   540,   218,   218,   218,   522,
     238,   238,   236,   540,   238,   394,   394,   200,   394,   331,
     522,    47,   416,    46,   129,   413,   546,   546,   238,   237,
     237,   237,   237,   341,   342,   394,   394,   394,   394,   372,
     546,   237,   546,   238,   384,   204,   241,   394,   200,   238,
     238,   167,   431,   372,   238,   238,   243,   238,   238,   236,
     298,   394,   266,   238,   238,   243,   248,   440,   373,   399,
     403,   238,   238,   401,   405,   409,   218,   238,   238,   238,
     242,   248,    33,   414,   413,   415,   237,   540,   543,   544,
     545,   545,   394,   546,   546,   540,   541,   238,   238,   238,
     238,   238,   238,   577,   545,   546,   541,   394,   238,   241,
     394,   394,   238,   330,    12,   282,   277,    20,   238,   238,
     277,   213,   436,   407,   340,   420,   414,   432,   433,   434,
     540,   210,   577,   238,   238,   238,   545,   545,   238,   238,
     238,   541,   238,   241,   576,   394,   241,   283,   350,   351,
     352,   353,   395,   248,   296,   372,   277,   277,   333,   491,
     435,   332,   327,   421,   238,   237,   238,   238,   238,    53,
     236,   576,   394,   242,   286,   289,   238,   333,   333,   435,
     394,   243,   248,   327,   540,   394,   236,   576,   287,    12,
      23,    24,   275,   278,   283,   277,   394,   248,   277,   238,
     243,   340,   277,   237,   248,   333,   248,   373,   288,   279,
     394,   243,   242,   290,   293,   238,   330,   291,   283,   277,
     340,   248,   280,   292,   290,   243,   278,   330
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   245,   246,   246,   246,   246,   246,   246,   246,   246,
     246,   246,   246,   246,   246,   246,   246,   247,   247,   248,
     248,   249,   250,   250,   250,   251,   251,   252,   252,   253,
     254,   254,   254,   254,   255,   255,   256,   256,   257,   258,
     257,   259,   259,   259,   260,   261,   261,   263,   262,   264,
     265,   266,   266,   266,   266,   266,   266,   266,   267,   267,
     268,   268,   268,   269,   269,   270,   271,   271,   272,   272,
     273,   274,   274,   275,   275,   276,   276,   277,   277,   278,
     279,   278,   280,   278,   281,   281,   282,   282,   283,   283,
     283,   283,   283,   284,   284,   285,   285,   287,   288,   286,
     289,   286,   291,   292,   290,   293,   290,   295,   296,   294,
     297,   298,   298,   298,   298,   298,   298,   298,   298,   300,
     299,   301,   303,   302,   304,   305,   305,   306,   306,   307,
     307,   307,   307,   307,   307,   308,   308,   309,   309,   309,
     309,   310,   310,   310,   310,   310,   310,   310,   310,   310,
     311,   311,   312,   312,   313,   313,   313,   314,   314,   314,
     314,   315,   315,   316,   316,   316,   316,   316,   316,   316,
     317,   317,   318,   318,   319,   319,   320,   320,   320,   321,
     321,   321,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   323,   323,   323,   323,   323,   323,   323,
     323,   323,   323,   323,   323,   323,   323,   323,   323,   323,
     323,   323,   323,   323,   323,   323,   323,   323,   323,   323,
     323,   323,   323,   323,   323,   323,   323,   323,   323,   323,
     323,   323,   323,   323,   323,   323,   323,   323,   323,   323,
     323,   324,   324,   325,   326,   326,   326,   327,   329,   328,
     330,   331,   332,   330,   334,   335,   333,   336,   336,   336,
     336,   337,   337,   337,   337,   337,   337,   337,   337,   337,
     337,   337,   337,   337,   337,   337,   337,   337,   337,   337,
     338,   339,   337,   340,   340,   340,   341,   341,   342,   342,
     343,   343,   344,   344,   344,   345,   345,   347,   348,   346,
     346,   349,   349,   349,   349,   349,   349,   350,   351,   352,
     352,   352,   353,   353,   354,   355,   355,   355,   356,   356,
     357,   357,   358,   358,   359,   359,   360,   360,   360,   362,
     363,   361,   364,   365,   361,   366,   367,   361,   369,   370,
     368,   371,   371,   371,   372,   372,   372,   372,   373,   373,
     373,   374,   374,   374,   375,   375,   375,   375,   375,   376,
     376,   377,   377,   378,   379,   379,   380,   380,   380,   380,
     380,   380,   380,   380,   381,   381,   381,   381,   381,   381,
     381,   381,   381,   381,   381,   381,   381,   381,   381,   381,
     381,   381,   381,   381,   381,   381,   381,   381,   381,   381,
     382,   382,   382,   383,   383,   383,   383,   383,   384,   384,
     385,   386,   387,   385,   388,   389,   385,   390,   390,   390,
     390,   390,   390,   390,   390,   390,   390,   390,   390,   391,
     392,   390,   393,   393,   393,   393,   393,   393,   393,   394,
     394,   394,   395,   395,   395,   395,   395,   395,   395,   395,
     395,   395,   395,   395,   395,   395,   395,   395,   395,   395,
     395,   395,   395,   395,   395,   395,   395,   395,   395,   395,
     395,   395,   395,   395,   395,   395,   395,   395,   395,   395,
     395,   395,   395,   395,   395,   395,   395,   395,   395,   395,
     395,   395,   395,   395,   395,   395,   395,   395,   396,   397,
     395,   395,   395,   395,   395,   398,   399,   395,   395,   395,
     400,   401,   395,   395,   395,   395,   402,   403,   395,   395,
     395,   404,   405,   395,   395,   395,   406,   407,   395,   395,
     395,   408,   409,   395,   395,   395,   395,   395,   395,   395,
     395,   395,   395,   395,   395,   395,   395,   395,   395,   395,
     410,   410,   410,   411,   411,   411,   411,   411,   411,   411,
     411,   411,   411,   411,   411,   411,   411,   411,   411,   411,
     411,   411,   411,   411,   412,   412,   413,   413,   413,   414,
     414,   415,   415,   415,   416,   416,   417,   418,   418,   418,
     419,   418,   420,   418,   421,   418,   422,   423,   423,   424,
     424,   424,   424,   424,   425,   425,   426,   426,   427,   427,
     427,   428,   429,   429,   430,   430,   430,   431,   431,   432,
     432,   432,   433,   433,   434,   434,   435,   435,   435,   436,
     436,   437,   437,   437,   437,   437,   437,   438,   438,   439,
     439,   439,   440,   440,   440,   441,   441,   441,   442,   442,
     443,   443,   443,   444,   444,   445,   444,   446,   447,   446,
     448,   448,   449,   449,   450,   450,   450,   451,   451,   451,
     453,   452,   454,   454,   456,   455,   457,   457,   457,   458,
     459,   459,   460,   460,   461,   461,   462,   462,   464,   465,
     466,   463,   467,   467,   468,   468,   469,   470,   470,   470,
     470,   471,   471,   471,   472,   472,   474,   475,   476,   473,
     477,   477,   477,   477,   477,   477,   478,   478,   478,   478,
     478,   478,   478,   478,   478,   478,   478,   478,   478,   478,
     478,   478,   478,   478,   478,   478,   478,   478,   478,   478,
     478,   478,   478,   478,   478,   478,   478,   478,   478,   478,
     478,   478,   478,   478,   478,   478,   478,   478,   478,   478,
     478,   478,   478,   478,   478,   478,   479,   479,   479,   479,
     479,   479,   479,   479,   480,   481,   481,   481,   482,   482,
     482,   483,   483,   483,   483,   483,   484,   484,   484,   484,
     484,   485,   486,   487,   485,   488,   488,   489,   489,   490,
     490,   490,   490,   491,   491,   492,   492,   493,   493,   493,
     493,   494,   495,   493,   493,   493,   493,   496,   493,   497,
     493,   493,   493,   493,   493,   493,   493,   493,   493,   493,
     493,   493,   493,   498,   499,   493,   493,   500,   501,   493,
     502,   503,   493,   504,   505,   493,   493,   506,   507,   493,
     508,   509,   493,   493,   510,   511,   493,   512,   513,   493,
     493,   514,   515,   493,   516,   517,   493,   518,   519,   493,
     520,   521,   493,   522,   522,   522,   524,   525,   526,   527,
     523,   529,   530,   531,   532,   528,   534,   535,   536,   537,
     533,   538,   538,   538,   538,   538,   538,   538,   539,   539,
     539,   539,   539,   540,   540,   540,   540,   540,   540,   540,
     540,   541,   541,   542,   543,   543,   544,   544,   545,   545,
     546,   546,   548,   549,   547,   550,   551,   547,   552,   553,
     547,   554,   555,   547,   556,   557,   547,   558,   559,   560,
     558,   561,   562,   563,   561,   564,   565,   561,   566,   567,
     561,   561,   568,   569,   561,   561,   570,   571,   561,   572,
     572,   573,   574,   575,   575,   575,   576,   576,   577,   577,
     578,   578,   579
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     3,
       3,     2,     2,     2,     2,     2,     2,     1,     1,     1,
       1,     2,     0,     1,     1,     1,     1,     0,     2,     5,
       1,     1,     2,     2,     3,     2,     0,     2,     0,     0,
       3,     0,     2,     5,     3,     1,     2,     0,     4,     2,
       2,     1,     2,     3,     3,     3,     3,     3,     0,     2,
       3,     5,     5,     0,     1,     2,     1,     3,     1,     3,
       3,     3,     2,     1,     1,     1,     2,     0,     1,     0,
       0,     4,     0,     8,     1,     1,     0,     2,     1,     1,
       1,     1,     1,     1,     2,     0,     1,     0,     0,     6,
       0,     3,     0,     0,     6,     0,     3,     0,     0,     9,
       7,     1,     4,     3,     3,     3,     6,     5,     5,     0,
      10,     3,     0,     8,     0,     7,     8,     4,     4,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     1,     1,
       1,     3,     3,     4,     3,     3,     3,     3,     1,     5,
       1,     3,     3,     4,     0,     3,     1,     1,     1,     1,
       1,     1,     4,     1,     2,     3,     3,     3,     3,     2,
       1,     3,     0,     3,     0,     4,     0,     2,     3,     0,
       2,     2,     1,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     3,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     3,     2,     2,     3,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     3,
       2,     2,     2,     2,     2,     3,     3,     3,     3,     3,
       4,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     1,     4,     0,     1,     1,     3,     0,     5,
       0,     0,     0,     6,     0,     0,     6,     2,     2,     2,
       2,     1,     2,     2,     1,     1,     1,     1,     2,     1,
       2,     2,     2,     2,     1,     1,     1,     2,     2,     2,
       0,     0,     6,     0,     2,     2,     0,     2,     0,     2,
       1,     3,     1,     3,     2,     2,     3,     0,     0,     5,
       1,     2,     5,     5,     5,     6,     2,     1,     1,     1,
       2,     3,     2,     3,     4,     1,     1,     0,     1,     1,
       1,     0,     1,     3,     8,     7,     4,     3,     5,     0,
       0,     9,     0,     0,     9,     0,     0,     9,     0,     0,
       6,     5,     8,    10,     1,     2,     3,     4,     1,     2,
       3,     1,     1,     1,     2,     2,     2,     2,     4,     1,
       3,     0,     4,     7,     7,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       6,     8,     6,     5,     6,     8,     8,     6,     1,     4,
       2,     0,     0,     7,     0,     0,     8,     3,     4,     3,
       5,     6,     5,     6,     8,     8,     6,     5,     6,     0,
       0,     5,     3,     4,     4,     5,     4,     3,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     2,     4,     3,     4,     5,     4,     4,     5,     4,
       3,     4,     3,     1,     1,     2,     4,     4,     0,     0,
       9,     1,     3,     3,     5,     0,     0,     8,     3,     3,
       0,     0,     8,     3,     3,     3,     0,     0,     8,     3,
       3,     0,     0,     8,     3,     4,     0,     0,     9,     4,
       3,     0,     0,     8,     3,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     3,     1,     4,     3,     3,     3,
       7,     8,     7,     4,     4,     4,     4,     4,     1,     6,
       7,     6,     6,     7,     6,     7,     6,     6,     6,     7,
       6,     6,     6,     5,     0,     1,     0,     1,     1,     0,
       1,     0,     1,     1,     0,     1,     5,     0,     2,     6,
       0,     4,     0,     9,     0,    11,     3,     3,     4,     1,
       1,     3,     3,     3,     1,     3,     1,     3,     0,     1,
       3,     3,     1,     3,     0,     1,     3,     1,     1,     1,
       2,     3,     3,     5,     1,     1,     1,     1,     1,     0,
       1,     1,     4,     3,     3,     6,     5,     1,     3,     0,
       2,     2,     4,     6,     5,     4,     6,     5,     0,     1,
       0,     1,     1,     0,     2,     0,     4,     6,     0,     6,
       1,     3,     1,     2,     0,     1,     3,     0,     1,     1,
       0,     5,     3,     3,     0,     6,     0,     1,     1,     1,
       0,     2,     0,     1,     1,     2,     0,     1,     0,     0,
       0,    13,     0,     2,     0,     1,     3,     1,     1,     2,
       2,     0,     1,     1,     1,     3,     0,     0,     0,     9,
       1,     4,     3,     3,     6,     5,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     4,     4,     1,     3,
       3,     0,     1,     3,     3,     5,     0,     2,     2,     2,
       2,     4,     0,     0,     7,     1,     1,     1,     3,     3,
       2,     4,     3,     1,     2,     0,     4,     1,     1,     1,
       1,     0,     0,     6,     4,     4,     3,     0,     6,     0,
       7,     4,     2,     2,     3,     2,     3,     2,     2,     3,
       3,     3,     2,     0,     0,     6,     2,     0,     0,     6,
       0,     0,     6,     0,     0,     6,     1,     0,     0,     6,
       0,     0,     7,     1,     0,     0,     6,     0,     0,     7,
       1,     0,     0,     6,     0,     0,     7,     0,     0,     6,
       0,     0,     6,     1,     3,     3,     0,     0,     0,     0,
      12,     0,     0,     0,     0,    12,     0,     0,     0,     0,
      13,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     5,     5,     6,     6,     8,
       8,     0,     1,     2,     3,     5,     1,     2,     1,     0,
       0,     1,     0,     0,    10,     0,     0,    10,     0,     0,
      10,     0,     0,    11,     0,     0,     7,     5,     0,     0,
      10,     3,     0,     0,    11,     0,     0,    11,     0,     0,
      10,     5,     0,     0,     9,     5,     0,     0,    10,     1,
       3,     0,     5,     5,     7,     9,     0,     3,     0,     1,
      11,    12,    13
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = DAS2_YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == DAS2_YYEMPTY)                                        \
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
   Use DAS2_YYerror or DAS2_YYUNDEF. */
#define YYERRCODE DAS2_YYUNDEF

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
#if DAS2_YYDEBUG

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

#  elif defined DAS2_YYLTYPE_IS_TRIVIAL && DAS2_YYLTYPE_IS_TRIVIAL

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
#else /* !DAS2_YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !DAS2_YYDEBUG */


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
            { /* gc_node; */ }
        break;

    case YYSYMBOL_string_builder: /* string_builder  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_reader: /* expr_reader  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_require_module_name: /* require_module_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_optional_require_guard: /* optional_require_guard  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_expression_label: /* expression_label  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_goto: /* expression_goto  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_else: /* expression_else  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_else_one_liner: /* expression_else_one_liner  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_if_one_liner: /* expression_if_one_liner  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_if_block: /* expression_if_block  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_else_block: /* expression_else_block  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_if_then_else: /* expression_if_then_else  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_if_then_else_oneliner: /* expression_if_then_else_oneliner  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_for_variable_name_with_pos_list: /* for_variable_name_with_pos_list  */
            { delete ((*yyvaluep).pNameWithPosList); }
        break;

    case YYSYMBOL_expression_for_loop: /* expression_for_loop  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_unsafe: /* expression_unsafe  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_while_loop: /* expression_while_loop  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_with: /* expression_with  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_with_alias: /* expression_with_alias  */
            { /* gc_node; */ }
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

    case YYSYMBOL_optional_for_annotations: /* optional_for_annotations  */
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

    case YYSYMBOL_optional_annotation_list_with_emit_semis: /* optional_annotation_list_with_emit_semis  */
            { delete ((*yyvaluep).faList); }
        break;

    case YYSYMBOL_optional_function_argument_list: /* optional_function_argument_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_optional_function_type: /* optional_function_type  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_function_name: /* function_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_das_type_name: /* das_type_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_function_declaration_header: /* function_declaration_header  */
            { ((*yyvaluep).pFuncDecl)->delRef(); }
        break;

    case YYSYMBOL_function_declaration: /* function_declaration  */
            { ((*yyvaluep).pFuncDecl)->delRef(); }
        break;

    case YYSYMBOL_expression_block_finally: /* expression_block_finally  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_block: /* expression_block  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_call_pipe_no_bracket: /* expr_call_pipe_no_bracket  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_any: /* expression_any  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expressions: /* expressions  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_optional_expr_list: /* optional_expr_list  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_optional_expr_map_tuple_list: /* optional_expr_map_tuple_list  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_type_declaration_no_options_list: /* type_declaration_no_options_list  */
            { deleteTypeDeclarationList(((*yyvaluep).pTypeDeclList)); }
        break;

    case YYSYMBOL_name_in_namespace: /* name_in_namespace  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_expression_delete: /* expression_delete  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_new_type_declaration: /* new_type_declaration  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_expr_new: /* expr_new  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_break: /* expression_break  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_continue: /* expression_continue  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_return: /* expression_return  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_yield: /* expression_yield  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_try_catch: /* expression_try_catch  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_tuple_expansion: /* tuple_expansion  */
            { delete ((*yyvaluep).pNameList); }
        break;

    case YYSYMBOL_tuple_expansion_variable_declaration: /* tuple_expansion_variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_expression_let: /* expression_let  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_cast: /* expr_cast  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_type_decl: /* expr_type_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_type_info: /* expr_type_info  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_list: /* expr_list  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_block_or_simple_block: /* block_or_simple_block  */
            { /* gc_node; */ }
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

    case YYSYMBOL_expr_full_block: /* expr_full_block  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_full_block_assumed_piped: /* expr_full_block_assumed_piped  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_numeric_const: /* expr_numeric_const  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_assign_no_bracket: /* expr_assign_no_bracket  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_named_call: /* expr_named_call  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_method_call_no_bracket: /* expr_method_call_no_bracket  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_func_addr_name: /* func_addr_name  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_func_addr_expr: /* func_addr_expr  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_field_no_bracket: /* expr_field_no_bracket  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_call: /* expr_call  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr: /* expr  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_no_bracket: /* expr_no_bracket  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_generator: /* expr_generator  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_mtag_no_bracket: /* expr_mtag_no_bracket  */
            { /* gc_node; */ }
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

    case YYSYMBOL_global_let_variable_name_with_pos_list: /* global_let_variable_name_with_pos_list  */
            { delete ((*yyvaluep).pNameWithPosList); }
        break;

    case YYSYMBOL_variable_declaration_list: /* variable_declaration_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_let_variable_declaration: /* let_variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_global_let_variable_declaration: /* global_let_variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_global_variable_declaration_list: /* global_variable_declaration_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
        break;

    case YYSYMBOL_enum_expression: /* enum_expression  */
            { delete ((*yyvaluep).pEnumPair); }
        break;

    case YYSYMBOL_enum_list: /* enum_list  */
            { /* gc owns Enumeration */ }
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
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_auto_type_declaration: /* auto_type_declaration  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_bitfield_bits: /* bitfield_bits  */
            { delete ((*yyvaluep).pNameList); }
        break;

    case YYSYMBOL_bitfield_alias_bits: /* bitfield_alias_bits  */
            { deleteNameExprList(((*yyvaluep).pNameExprList)); }
        break;

    case YYSYMBOL_bitfield_type_declaration: /* bitfield_type_declaration  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_table_type_pair: /* table_type_pair  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_dim_list: /* dim_list  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_type_declaration_no_options: /* type_declaration_no_options  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_optional_expr_list_in_braces: /* optional_expr_list_in_braces  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_type_declaration_no_options_no_dim: /* type_declaration_no_options_no_dim  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_type_declaration: /* type_declaration  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_make_decl: /* make_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_decl_no_bracket: /* make_decl_no_bracket  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_struct_fields: /* make_struct_fields  */
            { /* gc owns MakeStruct */ }
        break;

    case YYSYMBOL_make_variant_dim: /* make_variant_dim  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_struct_single: /* make_struct_single  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_struct_dim_list: /* make_struct_dim_list  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_struct_dim_decl: /* make_struct_dim_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_optional_make_struct_dim_decl: /* optional_make_struct_dim_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_struct_decl: /* make_struct_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_tuple_call: /* make_tuple_call  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_dim_decl: /* make_dim_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_map_tuple_list: /* expr_map_tuple_list  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_table_decl: /* make_table_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_table_call: /* make_table_call  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_array_comprehension_where: /* array_comprehension_where  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_table_comprehension: /* table_comprehension  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_array_comprehension: /* array_comprehension  */
            { /* gc_node; */ }
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
# if defined DAS2_YYLTYPE_IS_TRIVIAL && DAS2_YYLTYPE_IS_TRIVIAL
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

  yychar = DAS2_YYEMPTY; /* Cause a token to be read.  */

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
  if (yychar == DAS2_YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc, scanner);
    }

  if (yychar <= DAS2_YYEOF)
    {
      yychar = DAS2_YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == DAS2_YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = DAS2_YYUNDEF;
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
  yychar = DAS2_YYEMPTY;
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
                das2_yyerror(scanner,"module name has to be first declaration",tokAt(scanner,(yylsp[0])), CompilationError::invalid_module);
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

  case 21: /* top_level_reader_macro: expr_reader SEMICOLON  */
                                   {
        (void)(yyvsp[-1].pExpression); // gc_node — Expression, don't delete
    }
    break;

  case 22: /* optional_public_or_private_module: %empty  */
                        { (yyval.b) = yyextra->g_Program->policies.default_module_public; }
    break;

  case 23: /* optional_public_or_private_module: "public"  */
                        { (yyval.b) = true; }
    break;

  case 24: /* optional_public_or_private_module: "private"  */
                        { (yyval.b) = false; }
    break;

  case 25: /* module_name: '$'  */
                    { (yyval.s) = new string("$"); }
    break;

  case 26: /* module_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 27: /* optional_not_required: %empty  */
        { (yyval.b) = false; }
    break;

  case 28: /* optional_not_required: '!' "inscope"  */
                        { (yyval.b) = true; }
    break;

  case 29: /* module_declaration: "module" module_name optional_shared optional_public_or_private_module optional_not_required  */
                                                                                                                                    {
        yyextra->g_Program->thisModuleName = *(yyvsp[-3].s);
        yyextra->g_Program->thisModule->isPublic = (yyvsp[-1].b);
        yyextra->g_Program->thisModule->isModule = true;
        yyextra->g_Program->thisModule->visibleEverywhere = (yyvsp[0].b);
        if ( yyextra->g_Program->thisModule->name.empty() ) {
            yyextra->g_Program->library.renameModule(yyextra->g_Program->thisModule.get(),*(yyvsp[-3].s));
        } else if ( yyextra->g_Program->thisModule->name != *(yyvsp[-3].s) ){
            das2_yyerror(scanner,"this module already has a name " + yyextra->g_Program->thisModule->name,tokAt(scanner,(yylsp[-3])),
                CompilationError::already_declared_module_name);
        }
        if ( !yyextra->g_Program->policies.ignore_shared_modules ) {
            yyextra->g_Program->promoteToBuiltin = (yyvsp[-2].b);
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 30: /* character_sequence: STRING_CHARACTER  */
                                                                                  { (yyval.s) = new string(); *(yyval.s) += (yyvsp[0].ch); }
    break;

  case 31: /* character_sequence: STRING_CHARACTER_ESC  */
                                                                                  { (yyval.s) = new string(); *(yyval.s) += "\\\\"; }
    break;

  case 32: /* character_sequence: character_sequence STRING_CHARACTER  */
                                                                                  { (yyval.s) = (yyvsp[-1].s); *(yyvsp[-1].s) += (yyvsp[0].ch); }
    break;

  case 33: /* character_sequence: character_sequence STRING_CHARACTER_ESC  */
                                                                                  { (yyval.s) = (yyvsp[-1].s); *(yyvsp[-1].s) += "\\\\"; }
    break;

  case 34: /* string_constant: "start of the string" character_sequence "end of the string"  */
                                                           { (yyval.s) = (yyvsp[-1].s); }
    break;

  case 35: /* string_constant: "start of the string" "end of the string"  */
                                                           { (yyval.s) = new string(); }
    break;

  case 36: /* format_string: %empty  */
        { (yyval.s) = new string(); }
    break;

  case 37: /* format_string: format_string STRING_CHARACTER  */
                                                 { (yyval.s) = (yyvsp[-1].s); (yyvsp[-1].s)->push_back((yyvsp[0].ch)); }
    break;

  case 38: /* optional_format_string: %empty  */
        { (yyval.s) = new string(""); }
    break;

  case 39: /* $@1: %empty  */
            { das2_strfmt(scanner); }
    break;

  case 40: /* optional_format_string: ':' $@1 format_string  */
                                                         { (yyval.s) = (yyvsp[0].s); }
    break;

  case 41: /* string_builder_body: %empty  */
        {
        (yyval.pExpression) = new ExprStringBuilder();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 42: /* string_builder_body: string_builder_body character_sequence  */
                                                                                  {
        bool err;
        auto esconst = unescapeString(*(yyvsp[0].s),&err);
        if ( err ) das2_yyerror(scanner,"invalid escape sequence",tokAt(scanner,(yylsp[-1])), CompilationError::invalid_escape);
        auto sc = new ExprConstString(tokAt(scanner,(yylsp[0])),esconst);
        delete (yyvsp[0].s);
        static_cast<ExprStringBuilder *>((yyvsp[-1].pExpression))->elements.push_back(sc);
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 43: /* string_builder_body: string_builder_body "{" expr optional_format_string "}"  */
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

  case 44: /* string_builder: "start of the string" string_builder_body "end of the string"  */
                                                                   {
        auto strb = static_cast<ExprStringBuilder *>((yyvsp[-1].pExpression));
        if ( strb->elements.size()==0 ) {
            (yyval.pExpression) = new ExprConstString(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),"");
            // gc_node — don't delete $sb
        } else if ( strb->elements.size()==1 && strb->elements[0]->rtti_isStringConstant() ) {
            auto sconst = static_cast<ExprConstString*>(strb->elements[0]);
            (yyval.pExpression) = new ExprConstString(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),sconst->text);
            // gc_node — don't delete $sb
        } else {
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    }
    break;

  case 45: /* reader_character_sequence: STRING_CHARACTER  */
                               {
        if ( !yyextra->g_ReaderMacro->accept(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, (yyvsp[0].ch), tokAt(scanner,(yylsp[0]))) ) {
            das2_yyend_reader(scanner);
        }
    }
    break;

  case 46: /* reader_character_sequence: reader_character_sequence STRING_CHARACTER  */
                                                                {
        if ( !yyextra->g_ReaderMacro->accept(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, (yyvsp[0].ch), tokAt(scanner,(yylsp[0]))) ) {
            das2_yyend_reader(scanner);
        }
    }
    break;

  case 47: /* $@2: %empty  */
                                        {
        auto macros = yyextra->g_Program->getReaderMacro(*(yyvsp[0].s));
        if ( macros.size()==0 ) {
            das2_yyerror(scanner,"reader macro " + *(yyvsp[0].s) + " not found",tokAt(scanner,(yylsp[0])),
                CompilationError::lookup_macro);
            if ( yychar == '~' ) {
                yyextra->g_ReaderMacro = das_unknown_reader_macro();
                yyextra->g_ReaderExpr = new ExprReader(tokAt(scanner,(yylsp[-1])),yyextra->g_ReaderMacro);
                yyclearin ;
                das2_yybegin_reader(scanner);
            }
        } else if ( macros.size()>1 ) {
            string options;
            for ( auto & x : macros ) {
                options += "\t" + x->module->name + "::" + x->name + "\n";
            }
            das2_yyerror(scanner,"too many options for the reader macro " + *(yyvsp[0].s) +  "\n" + options, tokAt(scanner,(yylsp[0])),
                CompilationError::ambiguous_macro);
        } else if ( yychar != '~' ) {
            das2_yyerror(scanner,"expecting ~ after the reader macro", tokAt(scanner,(yylsp[0])),
                CompilationError::invalid_macro);
        } else {
            yyextra->g_ReaderMacro = macros.back();
            yyextra->g_ReaderExpr = new ExprReader(tokAt(scanner,(yylsp[-1])),yyextra->g_ReaderMacro);
            yyclearin ;
            das2_yybegin_reader(scanner);
        }
    }
    break;

  case 48: /* expr_reader: '%' name_in_namespace $@2 reader_character_sequence  */
                                     {
        yyextra->g_ReaderExpr->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[0]));
        (yyval.pExpression) = yyextra->g_ReaderExpr;
        int thisLine = 0;
        FileInfo * info = nullptr;
        if ( auto seqt = yyextra->g_ReaderMacro->suffix(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, thisLine, info, tokAt(scanner,(yylsp[0]))) ) {
            das2_accept_sequence(scanner,seqt,strlen(seqt),thisLine,info);
            yylloc.first_column = (yylsp[0]).first_column;
            yylloc.first_line = (yylsp[0]).first_line;
            yylloc.last_column = (yylsp[0]).last_column;
            yylloc.last_line = (yylsp[0]).last_line;
        }
        delete (yyvsp[-2].s);
        yyextra->g_ReaderMacro = nullptr;
        yyextra->g_ReaderExpr = nullptr;
    }
    break;

  case 49: /* options_declaration: "options" annotation_argument_list  */
                                                   {
        for ( auto & opt : *(yyvsp[0].aaList) ) {
            if ( yyextra->g_Access->isOptionAllowed(opt.name, yyextra->g_Program->thisModule->fileName) ) {
                if ( yyextra->g_Access->isOptionBlocked(opt.name, yyextra->g_Program->thisModule->fileName) ) {
                    // blocked: ok to write, silently ignored (not applied)
                } else {
                    yyextra->g_Program->options.push_back(opt);
                }
            } else {
                das2_yyerror(scanner,"option " + opt.name + " is not allowed here",
                    tokAt(scanner,(yylsp[0])), CompilationError::invalid_options);
            }
        }
        delete (yyvsp[0].aaList);
    }
    break;

  case 51: /* require_module_name: "name"  */
                   {
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 52: /* require_module_name: '%' require_module_name  */
                                     {
        *(yyvsp[0].s) = "%" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 53: /* require_module_name: '.' '/' require_module_name  */
                                         {
        *(yyvsp[0].s) = "./" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 54: /* require_module_name: ".." '/' require_module_name  */
                                            {
        *(yyvsp[0].s) = "../" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 55: /* require_module_name: '%' '/' require_module_name  */
                                         {
        *(yyvsp[0].s) = "%/" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 56: /* require_module_name: require_module_name '.' "name"  */
                                                {
        *(yyvsp[-2].s) += ".";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 57: /* require_module_name: require_module_name '/' "name"  */
                                                {
        *(yyvsp[-2].s) += "/";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 58: /* optional_require_guard: %empty  */
                                            { (yyval.s) = nullptr; }
    break;

  case 59: /* optional_require_guard: '?' require_module_name  */
                                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 60: /* require_module: optional_require_guard require_module_name is_public_module  */
                                                                                       {
        ast_requireModule(scanner,(yyvsp[-1].s),nullptr,(yyvsp[0].b),tokAt(scanner,(yylsp[-1])),(yyvsp[-2].s));
    }
    break;

  case 61: /* require_module: optional_require_guard require_module_name "as" "name" is_public_module  */
                                                                                                            {
        ast_requireModule(scanner,(yyvsp[-3].s),(yyvsp[-1].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-3])),(yyvsp[-4].s));
    }
    break;

  case 62: /* require_module: optional_require_guard '[' "name" ']' is_public_module  */
                                                                                {
        ast_requireModuleGroup(scanner,(yyvsp[-2].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-2])),(yyvsp[-4].s));
    }
    break;

  case 63: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 64: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 68: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 69: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 70: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 71: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 72: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 73: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 74: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 79: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 80: /* $@3: %empty  */
                                           {
    }
    break;

  case 81: /* expression_else: "else" optional_emit_semis $@3 expression_else_block  */
                                   {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 82: /* $@4: %empty  */
                                                                        {
    }
    break;

  case 83: /* expression_else: elif_or_static_elif '(' expr ')' optional_emit_semis $@4 expression_else_block expression_else  */
                                                         {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-7])),(yyvsp[-5].pExpression),(yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-7].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 84: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 85: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 86: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 87: /* expression_else_one_liner: "else" expression_if_one_liner  */
                                                      {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 88: /* expression_if_one_liner: expr_no_bracket  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 89: /* expression_if_one_liner: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 90: /* expression_if_one_liner: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 91: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 92: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 97: /* $@5: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 98: /* $@6: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 99: /* expression_if_block: '{' $@5 expressions $@6 '}' expression_block_finally  */
                                        {
        (yyval.pExpression) = (yyvsp[-3].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-5]),(yylsp[0]));
        if ( (yyvsp[0].pExpression) ) {
            auto pF = (ExprBlock *) (yyvsp[0].pExpression);
            auto pB = (ExprBlock *) (yyval.pExpression);
            swap ( pB->finalList, pF->list );
            // gc_node — don't delete Expression
        }
    }
    break;

  case 100: /* $@7: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 101: /* expression_if_block: $@7 expression_if_one_liner SEMICOLON  */
                                               {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 102: /* $@8: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 103: /* $@9: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 104: /* expression_else_block: '{' $@8 expressions $@9 '}' expression_block_finally  */
                                        {
        (yyval.pExpression) = (yyvsp[-3].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-5]),(yylsp[0]));
        if ( (yyvsp[0].pExpression) ) {
            auto pF = (ExprBlock *) (yyvsp[0].pExpression);
            auto pB = (ExprBlock *) (yyval.pExpression);
            swap ( pB->finalList, pF->list );
            // gc_node — don't delete Expression
        }
    }
    break;

  case 105: /* $@10: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 106: /* expression_else_block: $@10 expression_if_one_liner SEMICOLON  */
                                               {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 107: /* $@11: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 108: /* $@12: %empty  */
                                                                  {
    }
    break;

  case 109: /* expression_if_then_else: $@11 if_or_static_if '(' expr ')' optional_emit_semis $@12 expression_if_block expression_else  */
                                                       {
        yyextra->das_keyword = false;
        auto blk = (yyvsp[-1].pExpression)->rtti_isBlock() ? static_cast<ExprBlock *>((yyvsp[-1].pExpression)) : ast_wrapInBlock((yyvsp[-1].pExpression));
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-7])),(yyvsp[-5].pExpression),blk,(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-7].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 110: /* expression_if_then_else_oneliner: expression_if_one_liner "if" '(' expr ')' expression_else_one_liner SEMICOLON  */
                                                                                                                      {
        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ast_wrapInBlock((yyvsp[-6].pExpression)),(yyvsp[-1].pExpression) ? ast_wrapInBlock((yyvsp[-1].pExpression)) : nullptr);
    }
    break;

  case 111: /* for_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 112: /* for_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 113: /* for_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 114: /* for_variable_name_with_pos_list: '(' tuple_expansion ')'  */
                                       {
        auto pSL = new vector<VariableNameAndPosition>();
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        pSL->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 115: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 116: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "$i" '(' expr ')'  */
                                                                               {
        (yyvsp[-5].pNameWithPosList)->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = (yyvsp[-5].pNameWithPosList);
    }
    break;

  case 117: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 118: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' '(' tuple_expansion ')'  */
                                                                                 {
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
    }
    break;

  case 119: /* $@13: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 120: /* expression_for_loop: $@13 "for" optional_for_annotations '(' for_variable_name_with_pos_list "in" expr_list ')' optional_emit_semis expression_block  */
                                                                                                                                                                    {
        yyextra->das_keyword = false;
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-5].pNameWithPosList),(yyvsp[-3].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-8])),tokAt(scanner,(yylsp[0])),(yyvsp[-7].aaList));
    }
    break;

  case 121: /* expression_unsafe: "unsafe" optional_emit_semis expression_block  */
                                                                    {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-2])));
        pUnsafe->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 122: /* $@14: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 123: /* expression_while_loop: $@14 "while" optional_for_annotations '(' expr ')' optional_emit_semis expression_block  */
                                                                                                                        {
        yyextra->das_keyword = false;
        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-6])));
        pWhile->cond = (yyvsp[-3].pExpression);
        pWhile->body = (yyvsp[0].pExpression);
        if ( (yyvsp[-5].aaList) ) { pWhile->annotations = move(*(yyvsp[-5].aaList)); delete (yyvsp[-5].aaList); }
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 124: /* with_keyword_on: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 125: /* expression_with: with_keyword_on "with" '(' expr ')' optional_emit_semis expression_block  */
                                                                                                     {
        yyextra->das_keyword = false;
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-5])));
        pWith->with = (yyvsp[-3].pExpression);
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 126: /* expression_with: with_keyword_on "with" '(' "module" require_module_name ')' optional_emit_semis expression_block  */
                                                                                                                               {
        yyextra->das_keyword = false;
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-6])));
        { // single-name targets honor `require foo as bar` aliases
            auto ita = yyextra->das_module_alias.find(*(yyvsp[-3].s));
            if ( ita != yyextra->das_module_alias.end() ) *(yyvsp[-3].s) = ita->second;
        }
        pWith->moduleName = *(yyvsp[-3].s);
        delete (yyvsp[-3].s);
        if ( yyextra->g_Access ) { // .das_project may demand unsafe for this target module
            auto fi = pWith->at.fileInfo;
            pWith->moduleUnsafeByProject = yyextra->g_Access->isWithModuleUnsafe(pWith->moduleName, fi ? fi->name : "");
        }
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 127: /* expression_with_alias: "assume" "name" '=' expr  */
                                                      {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), ExpressionPtr((yyvsp[0].pExpression)));
        delete (yyvsp[-2].s);
    }
    break;

  case 128: /* expression_with_alias: "typedef" "name" '=' type_declaration  */
                                                                {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), TypeDeclPtr((yyvsp[0].pTypeDecl)));
        delete (yyvsp[-2].s);
    }
    break;

  case 129: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 130: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 131: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 132: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 133: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 134: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 135: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 136: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 137: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 138: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 139: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 140: /* annotation_argument_name: "default"  */
                    { (yyval.s) = new string("default"); }
    break;

  case 141: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 142: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 143: /* annotation_argument: annotation_argument_name '=' "@@" "name"  */
                                                                      { (yyval.aa) = new AnnotationArgument(*(yyvsp[-3].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-3]))); delete (yyvsp[0].s); delete (yyvsp[-3].s); }
    break;

  case 144: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 145: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 146: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 147: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 148: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 149: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                          {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[-4].s); }
    }
    break;

  case 150: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 151: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 152: /* metadata_argument_list: "@field" annotation_argument optional_emit_semis  */
                                                              {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[-1].aa));
    }
    break;

  case 153: /* metadata_argument_list: metadata_argument_list "@field" annotation_argument optional_emit_semis  */
                                                                                           {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-3].aaList),(yyvsp[-1].aa));
    }
    break;

  case 154: /* optional_for_annotations: %empty  */
                    {
        (yyval.aaList) = nullptr;
    }
    break;

  case 155: /* optional_for_annotations: '[' annotation_argument_list ']'  */
                                               {
        (yyval.aaList) = (yyvsp[-1].aaList);
    }
    break;

  case 156: /* optional_for_annotations: metadata_argument_list  */
                                     {
        (yyval.aaList) = (yyvsp[0].aaList);
    }
    break;

  case 157: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 158: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 159: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 160: /* annotation_declaration_name: "template"  */
                                    { (yyval.s) = new string("template"); }
    break;

  case 161: /* annotation_declaration_basic: annotation_declaration_name  */
                                          {
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner,(yylsp[0]));
        if ( yyextra->g_Access->isAnnotationAllowed(*(yyvsp[0].s), yyextra->g_Program->thisModuleName) ) {
            if ( auto ann = findAnnotation(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0]))) ) {
                (yyval.fa)->annotation = ann;
            } else {
                (yyval.fa)->annotation = new Annotation(*(yyvsp[0].s));
                das2_yyerror(scanner,"annotation " + *(yyvsp[0].s) + " is not found",
                            tokAt(scanner,(yylsp[0])), CompilationError::lookup_annotation);
            }
        } else {
            (yyval.fa)->annotation = new Annotation(*(yyvsp[0].s));
            das2_yyerror(scanner,"annotation " + *(yyvsp[0].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[0])), CompilationError::invalid_annotation);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 162: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
                                                                                 {
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner,(yylsp[-3]));
        if ( yyextra->g_Access->isAnnotationAllowed(*(yyvsp[-3].s), yyextra->g_Program->thisModuleName) ) {
            if ( auto ann = findAnnotation(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3]))) ) {
                (yyval.fa)->annotation = ann;
            } else {
                (yyval.fa)->annotation = new Annotation(*(yyvsp[-3].s));
                das2_yyerror(scanner,"annotation " + *(yyvsp[-3].s) + " is not found",
                            tokAt(scanner,(yylsp[-3])), CompilationError::lookup_annotation);
            }
        } else {
            (yyval.fa)->annotation = new Annotation(*(yyvsp[-3].s));
            das2_yyerror(scanner,"annotation " + *(yyvsp[-3].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[-3])), CompilationError::invalid_annotation);
        }
        swap ( (yyval.fa)->arguments, *(yyvsp[-1].aaList) );
        delete (yyvsp[-1].aaList);
        delete (yyvsp[-3].s);
    }
    break;

  case 163: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 164: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            (yyvsp[0].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 165: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            (yyvsp[-2].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            (yyvsp[0].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::And,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 166: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
                                                                            {
        if ( !(yyvsp[-2].fa)->annotation || !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            (yyvsp[-2].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        if ( !(yyvsp[0].fa)->annotation || !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            (yyvsp[0].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Or,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 167: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            (yyvsp[-2].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            (yyvsp[0].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Xor,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 168: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 169: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 170: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 171: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 172: /* optional_annotation_list: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 173: /* optional_annotation_list: '[' annotation_list ']'  */
                                       { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 174: /* optional_annotation_list_with_emit_semis: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 175: /* optional_annotation_list_with_emit_semis: '[' annotation_list ']' optional_emit_semis  */
                                                          { (yyval.faList) = (yyvsp[-2].faList); }
    break;

  case 176: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 177: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 178: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 179: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yyloc));
    }
    break;

  case 180: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 181: /* optional_function_type: "->" type_declaration  */
                                           {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 182: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 183: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 184: /* function_name: "operator" '='  */
                             { (yyval.s) = new string("="); }
    break;

  case 185: /* function_name: "operator" "<-"  */
                             { (yyval.s) = new string("<-"); }
    break;

  case 186: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 187: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 188: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 189: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 190: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 191: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 192: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 193: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 194: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 195: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 196: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 197: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 198: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 199: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 200: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 201: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 202: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 203: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 204: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 205: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 206: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 207: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 208: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 209: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 210: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 211: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 212: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 213: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 214: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 215: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 216: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 217: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 218: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 219: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 220: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 221: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 222: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 223: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 224: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 225: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 226: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 227: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 228: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 229: /* function_name: "operator" '[' ']' '='  */
                                 { (yyval.s) = new string("[]="); }
    break;

  case 230: /* function_name: "operator" '[' ']' "<-"  */
                                    { (yyval.s) = new string("[]<-"); }
    break;

  case 231: /* function_name: "operator" '[' ']' ":="  */
                                      { (yyval.s) = new string("[]:="); }
    break;

  case 232: /* function_name: "operator" '[' ']' "+="  */
                                     { (yyval.s) = new string("[]+="); }
    break;

  case 233: /* function_name: "operator" '[' ']' "-="  */
                                     { (yyval.s) = new string("[]-="); }
    break;

  case 234: /* function_name: "operator" '[' ']' "*="  */
                                     { (yyval.s) = new string("[]*="); }
    break;

  case 235: /* function_name: "operator" '[' ']' "/="  */
                                     { (yyval.s) = new string("[]/="); }
    break;

  case 236: /* function_name: "operator" '[' ']' "%="  */
                                     { (yyval.s) = new string("[]%="); }
    break;

  case 237: /* function_name: "operator" '[' ']' "&="  */
                                     { (yyval.s) = new string("[]&="); }
    break;

  case 238: /* function_name: "operator" '[' ']' "|="  */
                                     { (yyval.s) = new string("[]|="); }
    break;

  case 239: /* function_name: "operator" '[' ']' "^="  */
                                     { (yyval.s) = new string("[]^="); }
    break;

  case 240: /* function_name: "operator" '[' ']' "&&="  */
                                        { (yyval.s) = new string("[]&&="); }
    break;

  case 241: /* function_name: "operator" '[' ']' "||="  */
                                        { (yyval.s) = new string("[]||="); }
    break;

  case 242: /* function_name: "operator" '[' ']' "^^="  */
                                        { (yyval.s) = new string("[]^^="); }
    break;

  case 243: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 244: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 245: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 246: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 247: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 248: /* function_name: "operator" '.' "name" "+="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`+="); delete (yyvsp[-1].s); }
    break;

  case 249: /* function_name: "operator" '.' "name" "-="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`-="); delete (yyvsp[-1].s); }
    break;

  case 250: /* function_name: "operator" '.' "name" "*="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`*="); delete (yyvsp[-1].s); }
    break;

  case 251: /* function_name: "operator" '.' "name" "/="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`/="); delete (yyvsp[-1].s); }
    break;

  case 252: /* function_name: "operator" '.' "name" "%="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`%="); delete (yyvsp[-1].s); }
    break;

  case 253: /* function_name: "operator" '.' "name" "&="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&="); delete (yyvsp[-1].s); }
    break;

  case 254: /* function_name: "operator" '.' "name" "|="  */
                                          { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`|="); delete (yyvsp[-1].s); }
    break;

  case 255: /* function_name: "operator" '.' "name" "^="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^="); delete (yyvsp[-1].s); }
    break;

  case 256: /* function_name: "operator" '.' "name" "&&="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&&="); delete (yyvsp[-1].s); }
    break;

  case 257: /* function_name: "operator" '.' "name" "||="  */
                                            { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`||="); delete (yyvsp[-1].s); }
    break;

  case 258: /* function_name: "operator" '.' "name" "^^="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^^="); delete (yyvsp[-1].s); }
    break;

  case 259: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 260: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 261: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 262: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 263: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 264: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 265: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 266: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 267: /* function_name: "operator" "is" das_type_name  */
                                                { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 268: /* function_name: "operator" "as" das_type_name  */
                                                { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 269: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 270: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 271: /* function_name: "operator" '?' "as" das_type_name  */
                                                    { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 272: /* function_name: das_type_name  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 273: /* das_type_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 274: /* das_type_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 275: /* das_type_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 276: /* das_type_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 277: /* das_type_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 278: /* das_type_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 279: /* das_type_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 280: /* das_type_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 281: /* das_type_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 282: /* das_type_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 283: /* das_type_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 284: /* das_type_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 285: /* das_type_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 286: /* das_type_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 287: /* das_type_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 288: /* das_type_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 289: /* das_type_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 290: /* das_type_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 291: /* das_type_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 292: /* das_type_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 293: /* das_type_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 294: /* das_type_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 295: /* das_type_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 296: /* das_type_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 297: /* das_type_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 298: /* das_type_name: "float16"  */
                     { (yyval.s) = new string("float16"); }
    break;

  case 299: /* das_type_name: "half2"  */
                     { (yyval.s) = new string("half2"); }
    break;

  case 300: /* das_type_name: "half3"  */
                     { (yyval.s) = new string("half3"); }
    break;

  case 301: /* das_type_name: "half4"  */
                     { (yyval.s) = new string("half4"); }
    break;

  case 302: /* das_type_name: "half8"  */
                     { (yyval.s) = new string("half8"); }
    break;

  case 303: /* das_type_name: "short2"  */
                     { (yyval.s) = new string("short2"); }
    break;

  case 304: /* das_type_name: "short3"  */
                     { (yyval.s) = new string("short3"); }
    break;

  case 305: /* das_type_name: "short4"  */
                     { (yyval.s) = new string("short4"); }
    break;

  case 306: /* das_type_name: "short8"  */
                     { (yyval.s) = new string("short8"); }
    break;

  case 307: /* das_type_name: "ushort2"  */
                     { (yyval.s) = new string("ushort2"); }
    break;

  case 308: /* das_type_name: "ushort3"  */
                     { (yyval.s) = new string("ushort3"); }
    break;

  case 309: /* das_type_name: "ushort4"  */
                     { (yyval.s) = new string("ushort4"); }
    break;

  case 310: /* das_type_name: "ushort8"  */
                     { (yyval.s) = new string("ushort8"); }
    break;

  case 311: /* das_type_name: "byte2"  */
                     { (yyval.s) = new string("byte2"); }
    break;

  case 312: /* das_type_name: "byte3"  */
                     { (yyval.s) = new string("byte3"); }
    break;

  case 313: /* das_type_name: "byte4"  */
                     { (yyval.s) = new string("byte4"); }
    break;

  case 314: /* das_type_name: "byte8"  */
                     { (yyval.s) = new string("byte8"); }
    break;

  case 315: /* das_type_name: "byte16"  */
                     { (yyval.s) = new string("byte16"); }
    break;

  case 316: /* das_type_name: "ubyte2"  */
                     { (yyval.s) = new string("ubyte2"); }
    break;

  case 317: /* das_type_name: "ubyte3"  */
                     { (yyval.s) = new string("ubyte3"); }
    break;

  case 318: /* das_type_name: "ubyte4"  */
                     { (yyval.s) = new string("ubyte4"); }
    break;

  case 319: /* das_type_name: "ubyte8"  */
                     { (yyval.s) = new string("ubyte8"); }
    break;

  case 320: /* das_type_name: "ubyte16"  */
                     { (yyval.s) = new string("ubyte16"); }
    break;

  case 321: /* optional_template: %empty  */
                                        { (yyval.b) = false; }
    break;

  case 322: /* optional_template: "template"  */
                                        { (yyval.b) = true; }
    break;

  case 323: /* global_function_declaration: optional_annotation_list_with_emit_semis "def" optional_template function_declaration  */
                                                                                                                              {
        (yyvsp[0].pFuncDecl)->atDecl = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
        (yyvsp[0].pFuncDecl)->isTemplate = (yyvsp[-1].b);
        assignDefaultArguments((yyvsp[0].pFuncDecl));
        runFunctionAnnotations(scanner, yyextra, (yyvsp[0].pFuncDecl), (yyvsp[-3].faList), tokAt(scanner,(yylsp[-3])));
        if ( (yyvsp[0].pFuncDecl)->isGeneric() ) {
            implAddGenericFunction(scanner,(yyvsp[0].pFuncDecl));
        } else {
            if ( !yyextra->g_Program->addFunction((yyvsp[0].pFuncDecl)) ) {
                das2_yyerror(scanner,"function is already defined " +
                    (yyvsp[0].pFuncDecl)->getMangledName(),(yyvsp[0].pFuncDecl)->at,
                        CompilationError::already_declared_function);
            }
        }
        (yyvsp[0].pFuncDecl)->delRef();
    }
    break;

  case 324: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 325: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 326: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 327: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 328: /* $@15: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 329: /* function_declaration: optional_public_or_private_function $@15 function_declaration_header optional_emit_semis block_or_simple_block  */
                                                                                         {
        (yyvsp[-2].pFuncDecl)->body = (yyvsp[0].pExpression);
        (yyvsp[-2].pFuncDecl)->privateFunction = !(yyvsp[-4].b);
        (yyval.pFuncDecl) = (yyvsp[-2].pFuncDecl);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
        }
    }
    break;

  case 330: /* expression_block_finally: %empty  */
        {
        (yyval.pExpression) = nullptr;
    }
    break;

  case 331: /* $@16: %empty  */
                  {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 332: /* $@17: %empty  */
                             {
        yyextra->pop_nesteds();
    }
    break;

  case 333: /* expression_block_finally: "finally" $@16 '{' expressions $@17 '}'  */
          {
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 334: /* $@18: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 335: /* $@19: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 336: /* expression_block: '{' $@18 expressions $@19 '}' expression_block_finally  */
                                        {
        (yyval.pExpression) = (yyvsp[-3].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-5]),(yylsp[0]));
        if ( (yyvsp[0].pExpression) ) {
            auto pF = (ExprBlock *) (yyvsp[0].pExpression);
            auto pB = (ExprBlock *) (yyval.pExpression);
            swap ( pB->finalList, pF->list );
            // gc_node — don't delete Expression
        }
    }
    break;

  case 337: /* expr_call_pipe_no_bracket: expr_call expr_full_block_assumed_piped  */
                                                           {
        if ( (yyvsp[-1].pExpression)->rtti_isCallLikeExpr() ) {
            auto pCall = (ExprLooksLikeCall *) (yyvsp[-1].pExpression);
            pCall->arguments.push_back((yyvsp[0].pExpression));
            pCall->pipedCallArgument = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else if ( (yyvsp[-1].pExpression)->rtti_isNamedCall() ) {
            // piped block on a named call: pad-aware resolution lands it on the block param
            auto nc = (ExprNamedCall *) (yyvsp[-1].pExpression);
            nc->nonNamedArguments.push_back((yyvsp[0].pExpression));
            nc->pipedCallArgument = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else {
            das2_yyerror(scanner,"piped block requires a function call",tokAt(scanner,(yylsp[0])),
                CompilationError::cant_pipe);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    }
    break;

  case 338: /* expr_call_pipe_no_bracket: expr_method_call_no_bracket expr_full_block_assumed_piped  */
                                                                             {
        if ( (yyvsp[-1].pExpression)->rtti_isCallLikeExpr() ) {
            auto pCall = (ExprLooksLikeCall *) (yyvsp[-1].pExpression);
            pCall->arguments.push_back((yyvsp[0].pExpression));
            pCall->pipedCallArgument = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else if ( (yyvsp[-1].pExpression)->rtti_isNamedCall() ) {
            // piped block on a named call: pad-aware resolution lands it on the block param
            auto nc = (ExprNamedCall *) (yyvsp[-1].pExpression);
            nc->nonNamedArguments.push_back((yyvsp[0].pExpression));
            nc->pipedCallArgument = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else {
            das2_yyerror(scanner,"piped block requires a function call",tokAt(scanner,(yylsp[0])),
                CompilationError::cant_pipe);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    }
    break;

  case 339: /* expr_call_pipe_no_bracket: expr_field_no_bracket expr_full_block_assumed_piped  */
                                                                       {
        if ( (yyvsp[-1].pExpression)->rtti_isCallLikeExpr() ) {
            auto pCall = (ExprLooksLikeCall *) (yyvsp[-1].pExpression);
            pCall->arguments.push_back((yyvsp[0].pExpression));
            pCall->pipedCallArgument = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else if ( (yyvsp[-1].pExpression)->rtti_isNamedCall() ) {
            // piped block on a named call: pad-aware resolution lands it on the block param
            auto nc = (ExprNamedCall *) (yyvsp[-1].pExpression);
            nc->nonNamedArguments.push_back((yyvsp[0].pExpression));
            nc->pipedCallArgument = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else {
            das2_yyerror(scanner,"piped block requires a function call",tokAt(scanner,(yylsp[0])),
                CompilationError::cant_pipe);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    }
    break;

  case 340: /* expr_call_pipe_no_bracket: expr_named_call expr_full_block_assumed_piped  */
                                                                 {
        // free-function named call + piped block: pad-aware resolution lands it on the block param
        auto nc = (ExprNamedCall *) (yyvsp[-1].pExpression);
        nc->nonNamedArguments.push_back((yyvsp[0].pExpression));
        nc->pipedCallArgument = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 341: /* expression_any: SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 342: /* expression_any: expr_assign_no_bracket SEMICOLON  */
                                                    { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 343: /* expression_any: expression_delete SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 344: /* expression_any: expression_let  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 345: /* expression_any: expression_while_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 346: /* expression_any: expression_unsafe  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 347: /* expression_any: expression_with  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 348: /* expression_any: expression_with_alias SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 349: /* expression_any: expression_for_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 350: /* expression_any: expression_break SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 351: /* expression_any: expression_continue SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 352: /* expression_any: expression_return SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 353: /* expression_any: expression_yield SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 354: /* expression_any: expression_if_then_else  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 355: /* expression_any: expression_if_then_else_oneliner  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 356: /* expression_any: expression_try_catch  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 357: /* expression_any: expression_label SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 358: /* expression_any: expression_goto SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 359: /* expression_any: "pass" SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 360: /* $@20: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 361: /* $@21: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 362: /* expression_any: '{' $@20 expressions $@21 '}' expression_block_finally  */
                                        {
        (yyval.pExpression) = (yyvsp[-3].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-5]),(yylsp[0]));
        if ( (yyvsp[0].pExpression) ) {
            auto pF = (ExprBlock *) (yyvsp[0].pExpression);
            auto pB = (ExprBlock *) (yyval.pExpression);
            swap ( pB->finalList, pF->list );
            // gc_node — don't delete Expression
        }
    }
    break;

  case 363: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 364: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 365: /* expressions: expressions error  */
                                 {
        (void)(yyvsp[-1].pExpression); /* gc_node — don't delete Expression */ (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 366: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 367: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 368: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 369: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 370: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 371: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 372: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 373: /* name_in_namespace: "name" "::" "name"  */
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

  case 374: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 375: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 376: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 377: /* $@22: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 378: /* $@23: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 379: /* new_type_declaration: '<' $@22 type_declaration '>' $@23  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 380: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 381: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 382: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 383: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 384: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 385: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 386: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 387: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 388: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 389: /* expression_return: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 390: /* expression_return: "return" expr  */
                                      {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 391: /* expression_return: "return" "<-" expr  */
                                             {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 392: /* expression_yield: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 393: /* expression_yield: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 394: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 395: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 396: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 397: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 398: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 399: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 400: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 401: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 402: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 403: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 404: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 405: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                        {
        auto typeDecl = new TypeDecl(Type::autoinfer, tokAt(scanner,(yylsp[-5])));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameList),tokAt(scanner,(yylsp[-5])),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 406: /* expression_let: kwd_let optional_in_scope optional_field_annotation let_variable_declaration  */
                                                                                                {
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-1].aaList);
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-3].b),(yyvsp[-2].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 407: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 408: /* expression_let: kwd_let optional_in_scope '{' variable_declaration_list '}'  */
                                                                               {
        (yyval.pExpression) = ast_LetList(scanner,(yyvsp[-4].b),(yyvsp[-3].b),*(yyvsp[-1].pVarDeclList),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 409: /* $@24: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 410: /* $@25: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 411: /* expr_cast: "cast" '<' $@24 type_declaration_no_options '>' $@25 '(' expr ')'  */
                                                                                                                                                        {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-8])),(yyvsp[-1].pExpression),(yyvsp[-5].pTypeDecl));
    }
    break;

  case 412: /* $@26: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 413: /* $@27: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 414: /* expr_cast: "upcast" '<' $@26 type_declaration_no_options '>' $@27 '(' expr ')'  */
                                                                                                                                                          {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-8])),(yyvsp[-1].pExpression),(yyvsp[-5].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 415: /* $@28: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 416: /* $@29: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 417: /* expr_cast: "reinterpret" '<' $@28 type_declaration_no_options '>' $@29 '(' expr ')'  */
                                                                                                                                                               {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-8])),(yyvsp[-1].pExpression),(yyvsp[-5].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 418: /* $@30: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 419: /* $@31: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 420: /* expr_type_decl: "type" '<' $@30 type_declaration '>' $@31  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 421: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
                                                                          {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-3].s),ptd->typeexpr);
                // gc_node — don't delete Expression
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-3].s),(yyvsp[-1].pExpression));
            }
            delete (yyvsp[-3].s);
    }
    break;

  case 422: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
                                                                                                {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-6].s),ptd->typeexpr,*(yyvsp[-4].s));
                // gc_node — don't delete Expression
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-6].s),(yyvsp[-1].pExpression),*(yyvsp[-4].s));
            }
            delete (yyvsp[-6].s);
            delete (yyvsp[-4].s);
    }
    break;

  case 423: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" c_or_s "name" '>' '(' expr ')'  */
                                                                                                                        {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-8].s),ptd->typeexpr,*(yyvsp[-6].s),*(yyvsp[-4].s));
                // gc_node — don't delete Expression
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-8].s),(yyvsp[-1].pExpression),*(yyvsp[-6].s),*(yyvsp[-4].s));
            }
            delete (yyvsp[-8].s);
            delete (yyvsp[-6].s);
            delete (yyvsp[-4].s);
    }
    break;

  case 424: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 425: /* expr_list: "<-" expr  */
                             {
            (yyval.pExpression) = ast_makeMoveArgument(scanner, (yyvsp[0].pExpression), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 426: /* expr_list: expr_list ',' expr  */
                                        {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 427: /* expr_list: expr_list ',' "<-" expr  */
                                                   {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-3])),(yyvsp[-3].pExpression),ast_makeMoveArgument(scanner, (yyvsp[0].pExpression), tokAt(scanner,(yylsp[0]))));
    }
    break;

  case 428: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 429: /* block_or_simple_block: "=>" expr_no_bracket  */
                                                   {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->generated = true;
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 430: /* block_or_simple_block: "=>" "<-" expr_no_bracket  */
                                                          {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->generated = true;
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 431: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 432: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 433: /* block_or_lambda: "@@"  */
                  { (yyval.i) = 2;   /* local function */ }
    break;

  case 434: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 435: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 436: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 437: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 438: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 439: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 440: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 441: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 442: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 443: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis block_or_simple_block  */
                                                                                                                {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 444: /* expr_full_block_assumed_piped: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis block_or_simple_block  */
                                                                                                                {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 445: /* expr_full_block_assumed_piped: '{' expressions '}'  */
                                   {
        // block span is brace-to-brace (@$), not the statements' span (@block)
        (yyval.pExpression) = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc))),(yyvsp[-1].pExpression),tokAt(scanner,(yyloc)),tokAt(scanner,(yyloc)),LineInfo());
    }
    break;

  case 446: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = newConstLiteral<ExprConstInt>(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 447: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = newConstLiteral<ExprConstUInt>(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 448: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = newConstLiteral<ExprConstInt64>(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 449: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = newConstLiteral<ExprConstUInt64>(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 450: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = newConstLiteral<ExprConstUInt8>(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 451: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = newConstLiteral<ExprConstFloat>(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 452: /* expr_numeric_const: "float16 constant"  */
                                              { (yyval.pExpression) = newConstLiteral<ExprConstFloat16>(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 453: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = newConstLiteral<ExprConstDouble>(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 454: /* expr_assign_no_bracket: expr_no_bracket  */
                                                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 455: /* expr_assign_no_bracket: expr_no_bracket '=' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 456: /* expr_assign_no_bracket: expr_no_bracket "<-" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 457: /* expr_assign_no_bracket: expr_no_bracket "<-" make_table_decl  */
                                                                   { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 458: /* expr_assign_no_bracket: expr_no_bracket "<-" array_comprehension  */
                                                                     { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 459: /* expr_assign_no_bracket: expr_no_bracket ":=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 460: /* expr_assign_no_bracket: expr_no_bracket "!==" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression),true); }
    break;

  case 461: /* expr_assign_no_bracket: expr_no_bracket "!<-" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression),true); }
    break;

  case 462: /* expr_assign_no_bracket: expr_no_bracket "!<-" make_table_decl  */
                                                                      { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression),true); }
    break;

  case 463: /* expr_assign_no_bracket: expr_no_bracket "!<-" array_comprehension  */
                                                                        { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression),true); }
    break;

  case 464: /* expr_assign_no_bracket: expr_no_bracket "!:=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression),true); }
    break;

  case 465: /* expr_assign_no_bracket: expr_no_bracket "&=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 466: /* expr_assign_no_bracket: expr_no_bracket "|=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 467: /* expr_assign_no_bracket: expr_no_bracket "^=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 468: /* expr_assign_no_bracket: expr_no_bracket "&&=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 469: /* expr_assign_no_bracket: expr_no_bracket "||=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 470: /* expr_assign_no_bracket: expr_no_bracket "^^=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 471: /* expr_assign_no_bracket: expr_no_bracket "+=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 472: /* expr_assign_no_bracket: expr_no_bracket "-=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 473: /* expr_assign_no_bracket: expr_no_bracket "*=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 474: /* expr_assign_no_bracket: expr_no_bracket "/=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 475: /* expr_assign_no_bracket: expr_no_bracket "%=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 476: /* expr_assign_no_bracket: expr_no_bracket "<<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 477: /* expr_assign_no_bracket: expr_no_bracket ">>=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 478: /* expr_assign_no_bracket: expr_no_bracket "<<<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 479: /* expr_assign_no_bracket: expr_no_bracket ">>>=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 480: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 481: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 482: /* expr_named_call: name_in_namespace '(' expr_list ',' make_struct_fields ')'  */
                                                                                          {
        // bracket-less mixed named call: foo(pos..., name = value) -- named args are a strict suffix
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-3].pExpression));
        nc->arguments = (yyvsp[-1].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 483: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' ')'  */
                                                                    {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 484: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' expr_list ')'  */
                                                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-3])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 485: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                                     {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 486: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' expr_list ',' make_struct_fields ')'  */
                                                                                                                      {
        // bracket-less mixed named method call: a->m(pos..., name = value)
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-1].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        auto callArgs = sequenceToList((yyvsp[-3].pExpression));
        nc->nonNamedArguments.insert ( nc->nonNamedArguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 487: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' make_struct_fields ')'  */
                                                                                             {
        // bracket-less all-named method call, no positional: a->m(name = value)
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-1].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-5].pExpression));
        delete (yyvsp[-3].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 488: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 489: /* func_addr_name: "$i" '(' expr ')'  */
                                          {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 490: /* func_addr_expr: "@@" func_addr_name  */
                                            {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 491: /* $@32: %empty  */
                      { yyextra->das_arrow_depth ++; }
    break;

  case 492: /* $@33: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 493: /* func_addr_expr: "@@" '<' $@32 type_declaration_no_options '>' $@33 func_addr_name  */
                                                                                                                                                         {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 494: /* $@34: %empty  */
                      { yyextra->das_arrow_depth ++; }
    break;

  case 495: /* $@35: %empty  */
                                                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 496: /* func_addr_expr: "@@" '<' $@34 optional_function_argument_list optional_function_type '>' $@35 func_addr_name  */
                                                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value));
        expr->funcType = new TypeDecl(Type::tFunction, expr->at);
        expr->funcType->firstType = (yyvsp[-3].pTypeDecl);
        if ( (yyvsp[-4].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, expr->funcType, (yyvsp[-4].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-4].pVarDeclList));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 497: /* expr_field_no_bracket: expr_no_bracket '.' "name"  */
                                                         {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 498: /* expr_field_no_bracket: expr_no_bracket '.' '.' "name"  */
                                                             {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 499: /* expr_field_no_bracket: expr_no_bracket "!." "name"  */
                                                            {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 500: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' ')'  */
                                                                 {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 501: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' expr_list ')'  */
                                                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-3])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 502: /* expr_field_no_bracket: expr_no_bracket "!." "name" '(' ')'  */
                                                                    {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), *(yyvsp[-2].s), true);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 503: /* expr_field_no_bracket: expr_no_bracket "!." "name" '(' expr_list ')'  */
                                                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-3])), (yyvsp[-5].pExpression), *(yyvsp[-3].s), true);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 504: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 505: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' expr_list ',' make_struct_fields ')'  */
                                                                                                                   {
        // bracket-less mixed named method call: a.m(pos..., name = value)
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-1].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        auto callArgs = sequenceToList((yyvsp[-3].pExpression));
        nc->nonNamedArguments.insert ( nc->nonNamedArguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 506: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' make_struct_fields ')'  */
                                                                                          {
        // bracket-less all-named method call, no positional: a.m(name = value)
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-1].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-5].pExpression));
        delete (yyvsp[-3].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 507: /* expr_field_no_bracket: expr_no_bracket '.' basic_type_declaration '(' ')'  */
                                                                                   {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 508: /* expr_field_no_bracket: expr_no_bracket '.' basic_type_declaration '(' expr_list ')'  */
                                                                                                        {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-3])), (yyvsp[-5].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 509: /* $@36: %empty  */
                                          { yyextra->das_suppress_errors=true; }
    break;

  case 510: /* $@37: %empty  */
                                                                                       { yyextra->das_suppress_errors=false; }
    break;

  case 511: /* expr_field_no_bracket: expr_no_bracket '.' $@36 error $@37  */
                                                                                                                               {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), "");
        yyerrok;
    }
    break;

  case 512: /* expr_call: name_in_namespace '(' ')'  */
                                               {
            (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),*(yyvsp[-2].s));
            delete (yyvsp[-2].s);
    }
    break;

  case 513: /* expr_call: name_in_namespace '(' "uninitialized" ')'  */
                                                          {
            auto dd = new ExprMakeStruct(tokAt(scanner,(yylsp[-3])));
            dd->at = tokAt(scanner,(yylsp[-3]));
            dd->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            dd->useInitializer = false;
            dd->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = dd;
    }
    break;

  case 514: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
                                                               {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 515: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
                                                                                 {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-4])),*(yyvsp[-4].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-4].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 516: /* expr_call: name_in_namespace '(' expr_list ')'  */
                                                                    {
            (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),*(yyvsp[-3].s)),(yyvsp[-1].pExpression));
            delete (yyvsp[-3].s);
    }
    break;

  case 517: /* expr_call: basic_type_declaration '(' ')'  */
                                                    {
        (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-2].type)));
    }
    break;

  case 518: /* expr_call: basic_type_declaration '(' expr_list ')'  */
                                                                         {
        (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-3].type))),(yyvsp[-1].pExpression));
    }
    break;

  case 519: /* expr: expr_no_bracket  */
                                       { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 520: /* expr: make_table_decl  */
                                     { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 521: /* expr: array_comprehension  */
                                     { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 522: /* expr_no_bracket: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 523: /* expr_no_bracket: name_in_namespace  */
                                              { (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 524: /* expr_no_bracket: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 525: /* expr_no_bracket: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 526: /* expr_no_bracket: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 527: /* expr_no_bracket: make_decl_no_bracket  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 528: /* expr_no_bracket: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 529: /* expr_no_bracket: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 530: /* expr_no_bracket: expr_field_no_bracket  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 531: /* expr_no_bracket: expr_mtag_no_bracket  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 532: /* expr_no_bracket: '!' expr_no_bracket  */
                                                         { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",(yyvsp[0].pExpression)); }
    break;

  case 533: /* expr_no_bracket: '~' expr_no_bracket  */
                                                         { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",(yyvsp[0].pExpression)); }
    break;

  case 534: /* expr_no_bracket: '+' expr_no_bracket  */
                                                             { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",(yyvsp[0].pExpression)); }
    break;

  case 535: /* expr_no_bracket: '-' expr_no_bracket  */
                                                             { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",(yyvsp[0].pExpression)); }
    break;

  case 536: /* expr_no_bracket: expr_no_bracket "<<" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 537: /* expr_no_bracket: expr_no_bracket ">>" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 538: /* expr_no_bracket: expr_no_bracket "<<<" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 539: /* expr_no_bracket: expr_no_bracket ">>>" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 540: /* expr_no_bracket: expr_no_bracket '+' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 541: /* expr_no_bracket: expr_no_bracket '-' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 542: /* expr_no_bracket: expr_no_bracket '*' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 543: /* expr_no_bracket: expr_no_bracket '/' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 544: /* expr_no_bracket: expr_no_bracket '%' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 545: /* expr_no_bracket: expr_no_bracket '<' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 546: /* expr_no_bracket: expr_no_bracket '>' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 547: /* expr_no_bracket: expr_no_bracket "==" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 548: /* expr_no_bracket: expr_no_bracket "!=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 549: /* expr_no_bracket: expr_no_bracket "<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 550: /* expr_no_bracket: expr_no_bracket ">=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 551: /* expr_no_bracket: expr_no_bracket '&' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 552: /* expr_no_bracket: expr_no_bracket '|' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 553: /* expr_no_bracket: expr_no_bracket '^' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 554: /* expr_no_bracket: expr_no_bracket "&&" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 555: /* expr_no_bracket: expr_no_bracket "||" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 556: /* expr_no_bracket: expr_no_bracket "^^" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 557: /* expr_no_bracket: expr_no_bracket ".." expr_no_bracket  */
                                                                   {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 558: /* expr_no_bracket: "++" expr_no_bracket  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", (yyvsp[0].pExpression)); }
    break;

  case 559: /* expr_no_bracket: "--" expr_no_bracket  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", (yyvsp[0].pExpression)); }
    break;

  case 560: /* expr_no_bracket: expr_no_bracket "++"  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 561: /* expr_no_bracket: expr_no_bracket "--"  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 562: /* expr_no_bracket: '(' expr_list optional_comma ')'  */
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
                (yyval.pExpression) = (yyvsp[-2].pExpression);
            }
        }
    break;

  case 563: /* expr_no_bracket: '(' make_struct_single ')'  */
                                      {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        for ( auto & arg : *(((ExprMakeStruct *)(yyvsp[-1].pExpression))->structs.back()) ) {
            mkt->values.push_back(arg->value);
            mkt->recordNames.push_back(arg->name);
        }
        // gc_node — don't delete Expression
        (yyval.pExpression) = mkt;
    }
    break;

  case 564: /* expr_no_bracket: expr_no_bracket '[' expr ']'  */
                                                            { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 565: /* expr_no_bracket: expr_no_bracket '.' '[' expr ']'  */
                                                                { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 566: /* expr_no_bracket: expr_no_bracket "![" expr ']'  */
                                                               { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 567: /* expr_no_bracket: expr_no_bracket "?[" expr ']'  */
                                                            { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 568: /* expr_no_bracket: expr_no_bracket '.' "?[" expr ']'  */
                                                                { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 569: /* expr_no_bracket: expr_no_bracket "!?[" expr ']'  */
                                                               { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 570: /* expr_no_bracket: expr_no_bracket "?." "name"  */
                                                            { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 571: /* expr_no_bracket: expr_no_bracket '.' "?." "name"  */
                                                                { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 572: /* expr_no_bracket: expr_no_bracket "!?." "name"  */
                                                               { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 573: /* expr_no_bracket: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 574: /* expr_no_bracket: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 575: /* expr_no_bracket: '*' expr_no_bracket  */
                                                              { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 576: /* expr_no_bracket: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 577: /* expr_no_bracket: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 578: /* $@38: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 579: /* $@39: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 580: /* expr_no_bracket: "addr" '<' $@38 type_declaration_no_options '>' $@39 '(' expr ')'  */
                                                                                                                                                        {
        auto pRef2Ptr = new ExprRef2Ptr(tokAt(scanner,(yylsp[-8])),(yyvsp[-1].pExpression));
        pRef2Ptr->generated = true;
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-8])),pRef2Ptr,(yyvsp[-5].pTypeDecl));
        pCast->reinterpret = true;
        pCast->fromAddrSugar = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 581: /* expr_no_bracket: expr_generator  */
                                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 582: /* expr_no_bracket: expr_no_bracket "??" expr_no_bracket  */
                                                                         { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 583: /* expr_no_bracket: expr_no_bracket "!??" expr_no_bracket  */
                                                                            {
        auto nc = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        nc->no_promotion = true;
        (yyval.pExpression) = nc;
    }
    break;

  case 584: /* expr_no_bracket: expr_no_bracket '?' expr_no_bracket ':' expr_no_bracket  */
                                                                                           {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        }
    break;

  case 585: /* $@40: %empty  */
                                                          { yyextra->das_arrow_depth ++; }
    break;

  case 586: /* $@41: %empty  */
                                                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 587: /* expr_no_bracket: expr_no_bracket "is" "type" '<' $@40 type_declaration_no_options '>' $@41  */
                                                                                                                                                                  {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 588: /* expr_no_bracket: expr_no_bracket "is" basic_type_declaration  */
                                                                          {
        auto vdecl = new TypeDecl((yyvsp[0].type), tokAt(scanner,(yyloc)));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 589: /* expr_no_bracket: expr_no_bracket "is" "name"  */
                                                         {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 590: /* $@42: %empty  */
                                                         { yyextra->das_arrow_depth ++; }
    break;

  case 591: /* $@43: %empty  */
                                                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 592: /* expr_no_bracket: expr_no_bracket "!is" "type" '<' $@42 type_declaration_no_options '>' $@43  */
                                                                                                                                                                 {
        auto isx = new ExprIs(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
        isx->no_promotion = true;
        (yyval.pExpression) = isx;
    }
    break;

  case 593: /* expr_no_bracket: expr_no_bracket "!is" basic_type_declaration  */
                                                                         {
        auto vdecl = new TypeDecl((yyvsp[0].type), tokAt(scanner,(yyloc)));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        auto isx = new ExprIs(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),vdecl);
        isx->no_promotion = true;
        (yyval.pExpression) = isx;
    }
    break;

  case 594: /* expr_no_bracket: expr_no_bracket "!is" "name"  */
                                                        {
        auto isv = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        isv->no_promotion = true;
        (yyval.pExpression) = isv;
        delete (yyvsp[0].s);
    }
    break;

  case 595: /* expr_no_bracket: expr_no_bracket "as" "name"  */
                                                         {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 596: /* $@44: %empty  */
                                                          { yyextra->das_arrow_depth ++; }
    break;

  case 597: /* $@45: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 598: /* expr_no_bracket: expr_no_bracket "as" "type" '<' $@44 type_declaration '>' $@45  */
                                                                                                                                                       {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 599: /* expr_no_bracket: expr_no_bracket "as" basic_type_declaration  */
                                                                          {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 600: /* expr_no_bracket: expr_no_bracket "!as" "name"  */
                                                        {
        auto asv = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        asv->no_promotion = true;
        (yyval.pExpression) = asv;
        delete (yyvsp[0].s);
    }
    break;

  case 601: /* $@46: %empty  */
                                                         { yyextra->das_arrow_depth ++; }
    break;

  case 602: /* $@47: %empty  */
                                                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 603: /* expr_no_bracket: expr_no_bracket "!as" "type" '<' $@46 type_declaration '>' $@47  */
                                                                                                                                                      {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        auto asv = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        asv->no_promotion = true;
        (yyval.pExpression) = asv;
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 604: /* expr_no_bracket: expr_no_bracket "!as" basic_type_declaration  */
                                                                         {
        auto asv = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
        asv->no_promotion = true;
        (yyval.pExpression) = asv;
    }
    break;

  case 605: /* expr_no_bracket: expr_no_bracket '?' "as" "name"  */
                                                             {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 606: /* $@48: %empty  */
                                                              { yyextra->das_arrow_depth ++; }
    break;

  case 607: /* $@49: %empty  */
                                                                                                                          { yyextra->das_arrow_depth --; }
    break;

  case 608: /* expr_no_bracket: expr_no_bracket '?' "as" "type" '<' $@48 type_declaration '>' $@49  */
                                                                                                                                                           {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 609: /* expr_no_bracket: expr_no_bracket '?' "as" basic_type_declaration  */
                                                                              {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 610: /* expr_no_bracket: expr_no_bracket "!?as" "name"  */
                                                         {
        auto asv = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        asv->no_promotion = true;
        (yyval.pExpression) = asv;
        delete (yyvsp[0].s);
    }
    break;

  case 611: /* $@50: %empty  */
                                                          { yyextra->das_arrow_depth ++; }
    break;

  case 612: /* $@51: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 613: /* expr_no_bracket: expr_no_bracket "!?as" "type" '<' $@50 type_declaration '>' $@51  */
                                                                                                                                                       {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        auto asv = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        asv->no_promotion = true;
        (yyval.pExpression) = asv;
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 614: /* expr_no_bracket: expr_no_bracket "!?as" basic_type_declaration  */
                                                                          {
        auto asv = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
        asv->no_promotion = true;
        (yyval.pExpression) = asv;
    }
    break;

  case 615: /* expr_no_bracket: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 616: /* expr_no_bracket: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 617: /* expr_no_bracket: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 618: /* expr_no_bracket: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 619: /* expr_no_bracket: expr_method_call_no_bracket  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 620: /* expr_no_bracket: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 621: /* expr_no_bracket: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 622: /* expr_no_bracket: expr_no_bracket "<|" expr_no_bracket  */
                                                                      { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1])),true); }
    break;

  case 623: /* expr_no_bracket: expr_no_bracket "|>" expr_no_bracket  */
                                                                      { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 624: /* expr_no_bracket: expr_no_bracket "|>" basic_type_declaration  */
                                                                     {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 625: /* expr_no_bracket: expr_call_pipe_no_bracket  */
                                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 626: /* expr_no_bracket: "unsafe" '(' expr ')'  */
                                         {
            (yyvsp[-1].pExpression)->alwaysSafe = true;
            (yyvsp[-1].pExpression)->userSaidItsSafe = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    break;

  case 627: /* expr_no_bracket: expr_no_bracket "=>" expr_no_bracket  */
                                                               {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 628: /* expr_no_bracket: expr_no_bracket "=>" make_table_decl  */
                                                               {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 629: /* expr_no_bracket: expr_no_bracket "=>" array_comprehension  */
                                                                   {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 630: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 631: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 632: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list optional_emit_semis expression_block  */
                                                                                                                                                  {
        auto closure = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),(yyvsp[0].pExpression));
        ((ExprBlock *)(yyvsp[0].pExpression))->returnType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),closure,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 633: /* expr_mtag_no_bracket: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 634: /* expr_mtag_no_bracket: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 635: /* expr_mtag_no_bracket: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 636: /* expr_mtag_no_bracket: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 637: /* expr_mtag_no_bracket: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 638: /* expr_mtag_no_bracket: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 639: /* expr_mtag_no_bracket: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 640: /* expr_mtag_no_bracket: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 641: /* expr_mtag_no_bracket: expr_no_bracket '.' "$f" '(' expr ')'  */
                                                                           {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 642: /* expr_mtag_no_bracket: expr_no_bracket "?." "$f" '(' expr ')'  */
                                                                            {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 643: /* expr_mtag_no_bracket: expr_no_bracket '.' '.' "$f" '(' expr ')'  */
                                                                               {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 644: /* expr_mtag_no_bracket: expr_no_bracket "!." "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 645: /* expr_mtag_no_bracket: expr_no_bracket '.' "?." "$f" '(' expr ')'  */
                                                                                {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 646: /* expr_mtag_no_bracket: expr_no_bracket "!?." "$f" '(' expr ')'  */
                                                                               {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 647: /* expr_mtag_no_bracket: expr_no_bracket "as" "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 648: /* expr_mtag_no_bracket: expr_no_bracket "!as" "$f" '(' expr ')'  */
                                                                             {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        cfield->no_promotion = true;
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 649: /* expr_mtag_no_bracket: expr_no_bracket '?' "as" "$f" '(' expr ')'  */
                                                                                  {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 650: /* expr_mtag_no_bracket: expr_no_bracket "!?as" "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        cfield->no_promotion = true;
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 651: /* expr_mtag_no_bracket: expr_no_bracket "is" "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 652: /* expr_mtag_no_bracket: expr_no_bracket "!is" "$f" '(' expr ')'  */
                                                                             {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        cfield->no_promotion = true;
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 653: /* expr_mtag_no_bracket: "@@" "$c" '(' expr ')'  */
                                                           {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 654: /* optional_field_annotation: %empty  */
                                      { (yyval.aaList) = nullptr; }
    break;

  case 655: /* optional_field_annotation: metadata_argument_list  */
                                      { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 656: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 657: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 658: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 659: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 660: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 661: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 662: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 663: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 664: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 665: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 666: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 667: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 668: /* struct_variable_declaration_list: struct_variable_declaration_list "new line, semicolon"  */
                                                                 { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 669: /* struct_variable_declaration_list: struct_variable_declaration_list "typedef" "name" '=' type_declaration SEMICOLON  */
                                                                                                                {
        (yyval.pVarDeclList) = (yyvsp[-5].pVarDeclList);
        ast_structureAlias(scanner,(yyvsp[-3].s),(yyvsp[-1].pTypeDecl),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 670: /* $@52: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 671: /* struct_variable_declaration_list: struct_variable_declaration_list $@52 structure_variable_declaration SEMICOLON  */
                                                     {
        (yyval.pVarDeclList) = (yyvsp[-3].pVarDeclList);
        if ( (yyvsp[-1].pVarDecl) ) (yyvsp[-3].pVarDeclList)->push_back((yyvsp[-1].pVarDecl));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *((yyvsp[-1].pVarDecl)->pNameList) ) {
                    crd->afterStructureField(nl.name.c_str(), nl.at);
                }
            }
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterStructureFields(tak);
        }
    }
    break;

  case 672: /* $@53: %empty  */
                                                                                                                     {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 673: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable "abstract" optional_constant $@53 function_declaration_header SEMICOLON  */
                                                          {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 674: /* $@54: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 675: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@54 function_declaration_header optional_emit_semis block_or_simple_block  */
                                                                                                 {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-10].pVarDeclList),(yyvsp[-9].faList),(yyvsp[-6].b),(yyvsp[-7].b),(yyvsp[-5].i),(yyvsp[-4].b),(yyvsp[-2].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-8]),(yylsp[0])),tokAt(scanner,(yylsp[-9])));
            }
    break;

  case 676: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 677: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 678: /* function_argument_declaration_type: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none, tokAt(scanner,(yyloc))), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 679: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 680: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 681: /* function_argument_list: function_argument_declaration_no_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 682: /* function_argument_list: function_argument_declaration_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 683: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 684: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 685: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 686: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 687: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                       { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 688: /* tuple_alias_type_list: %empty  */
      {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 689: /* tuple_alias_type_list: tuple_type  */
                       {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 690: /* tuple_alias_type_list: tuple_alias_type_list semis tuple_type  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
        if ( !yyextra->g_CommentReaders.empty() ) {
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *((yyvsp[0].pVarDecl)->pNameList) ) {
                    crd->afterTupleEntry(nl.name.c_str(), nl.at);
                }
            }
        }
    }
    break;

  case 691: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 692: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 693: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 694: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 695: /* variant_alias_type_list: variant_type  */
                         {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 696: /* variant_alias_type_list: variant_alias_type_list semis variant_type  */
                                                               {
        (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
        if ( !yyextra->g_CommentReaders.empty() ) {
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *((yyvsp[0].pVarDecl)->pNameList) ) {
                    crd->afterVariantEntry(nl.name.c_str(), nl.at);
                }
            }
        }
    }
    break;

  case 697: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 698: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 699: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer, tokAt(scanner,(yylsp[0])));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 700: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer, tokAt(scanner,(yylsp[-1])));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 701: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer, tokAt(scanner,(yylsp[-2])));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 702: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 703: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 704: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 705: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 706: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 707: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 708: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 709: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 710: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 711: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 712: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 713: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 714: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 715: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "$i" '(' expr ')'  */
                                                                               {
        (yyvsp[-5].pNameWithPosList)->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = (yyvsp[-5].pNameWithPosList);
    }
    break;

  case 716: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 717: /* global_let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 718: /* global_let_variable_name_with_pos_list: global_let_variable_name_with_pos_list ',' "name"  */
                                                                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 719: /* variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 720: /* variable_declaration_list: variable_declaration_list SEMICOLON  */
                                                  {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 721: /* variable_declaration_list: variable_declaration_list let_variable_declaration  */
                                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
        (yyvsp[-1].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 722: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 723: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 724: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer, tokAt(scanner,(yylsp[-4])));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 725: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                         {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 726: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                               {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 727: /* global_let_variable_declaration: global_let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer, tokAt(scanner,(yylsp[-4])));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 728: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 729: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 730: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 731: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 732: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 733: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 734: /* global_variable_declaration_list: global_variable_declaration_list SEMICOLON  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 735: /* $@55: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 736: /* global_variable_declaration_list: global_variable_declaration_list $@55 optional_field_annotation let_variable_declaration  */
                                                                      {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders )
                for ( auto & nl : *((yyvsp[0].pVarDecl)->pNameList) )
                    crd->afterGlobalVariable(nl.name.c_str(),tak);
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterGlobalVariables(tak);
        }
        (yyval.pVarDeclList) = (yyvsp[-3].pVarDeclList);
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-1].aaList);
        (yyvsp[-3].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 737: /* global_let: kwd_let optional_shared optional_public_or_private_variable '{' global_variable_declaration_list '}'  */
                                                                                                                                       {
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 738: /* $@56: %empty  */
                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 739: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@56 optional_field_annotation global_let_variable_declaration  */
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

  case 740: /* enum_expression: "name"  */
                   {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        delete (yyvsp[0].s);
    }
    break;

  case 741: /* enum_expression: "name" '=' expr  */
                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[-2].s),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-2])));
        delete (yyvsp[-2].s);
    }
    break;

  case 744: /* enum_list: %empty  */
        {
        (yyval.pEnumList) = new Enumeration();
    }
    break;

  case 745: /* enum_list: enum_expression  */
                            {
        (yyval.pEnumList) = new Enumeration();
        if ( !(yyval.pEnumList)->add((yyvsp[0].pEnumPair)->name,(yyvsp[0].pEnumPair)->expr,(yyvsp[0].pEnumPair)->at) ) {
            das2_yyerror(scanner,"enumeration already declared " + (yyvsp[0].pEnumPair)->name, (yyvsp[0].pEnumPair)->at,
                CompilationError::already_declared_enumerator);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            for ( auto & crd : yyextra->g_CommentReaders ) {
                crd->afterEnumerationEntry((yyvsp[0].pEnumPair)->name.c_str(), (yyvsp[0].pEnumPair)->at);
            }
        }
        delete (yyvsp[0].pEnumPair);
    }
    break;

  case 746: /* enum_list: enum_list commas enum_expression  */
                                                 {
        if ( !(yyvsp[-2].pEnumList)->add((yyvsp[0].pEnumPair)->name,(yyvsp[0].pEnumPair)->expr,(yyvsp[0].pEnumPair)->at) ) {
            das2_yyerror(scanner,"enumeration already declared " + (yyvsp[0].pEnumPair)->name, (yyvsp[0].pEnumPair)->at,
                CompilationError::already_declared_enumerator);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            for ( auto & crd : yyextra->g_CommentReaders ) {
                crd->afterEnumerationEntry((yyvsp[0].pEnumPair)->name.c_str(), (yyvsp[0].pEnumPair)->at);
            }
        }
        delete (yyvsp[0].pEnumPair);
        (yyval.pEnumList) = (yyvsp[-2].pEnumList);
    }
    break;

  case 747: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 748: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 749: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 750: /* $@57: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 751: /* single_alias: optional_public_or_private_alias "name" $@57 '=' type_declaration  */
                                  {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        (yyvsp[0].pTypeDecl)->isPrivateAlias = !(yyvsp[-4].b);
        if ( (yyvsp[0].pTypeDecl)->baseType == Type::alias ) {
            das2_yyerror(scanner,"alias cannot be defined in terms of another alias "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::invalid_type_alias);
        }
        (yyvsp[0].pTypeDecl)->alias = *(yyvsp[-3].s);
        if ( !yyextra->g_Program->addAlias((yyvsp[0].pTypeDecl)) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::already_declared_type_alias);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterAlias((yyvsp[-3].s)->c_str(),pubename);
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 754: /* $@58: %empty  */
                                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 755: /* distinct_alias: optional_public_or_private_alias "name" "name" $@58 '=' type_declaration  */
                                  {
        if ( *(yyvsp[-4].s) != "distinct" ) {
            das2_yyerror(scanner,"expected 'distinct', got '"+*(yyvsp[-4].s)+"'",tokAt(scanner,(yylsp[-4])),
                CompilationError::invalid_distinct_type);
        } else {
            ast_distinctDeclaration(scanner,(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),!(yyvsp[-5].b),(yyvsp[0].pTypeDecl));
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterAlias((yyvsp[-3].s)->c_str(),pubename);
        }
        delete (yyvsp[-4].s);
        delete (yyvsp[-3].s);
    }
    break;

  case 756: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 757: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 758: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 759: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 760: /* optional_enum_basic_type_declaration: %empty  */
        {
        (yyval.type) = Type::tInt;
    }
    break;

  case 761: /* optional_enum_basic_type_declaration: ':' enum_basic_type_declaration  */
                                              {
        (yyval.type) = (yyvsp[0].type);
    }
    break;

  case 768: /* $@59: %empty  */
                                                                     {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 769: /* $@60: %empty  */
                                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 770: /* $@61: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 771: /* enum_declaration: optional_annotation_list_with_emit_semis "enum" $@59 optional_public_or_private_enum enum_name optional_enum_basic_type_declaration optional_emit_commas '{' $@60 enum_list optional_commas $@61 '}'  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-8].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-12].faList),tokAt(scanner,(yylsp[-12])),(yyvsp[-9].b),(yyvsp[-8].pEnum),(yyvsp[-3].pEnumList),(yyvsp[-7].type));
    }
    break;

  case 772: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 773: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 774: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 775: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 776: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 777: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 778: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 779: /* class_or_struct: "class" "template"  */
                                  { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 780: /* class_or_struct: "struct" "template"  */
                                  { (yyval.i) = CorS_StructTemplate; }
    break;

  case 781: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 782: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 783: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 784: /* optional_struct_variable_declaration_list: ';'  */
            {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 785: /* optional_struct_variable_declaration_list: '{' struct_variable_declaration_list '}'  */
                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 786: /* $@62: %empty  */
                                                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 787: /* $@63: %empty  */
                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 788: /* $@64: %empty  */
                                             {
        if ( (yyvsp[-1].pStructure) ) {
            (yyvsp[-1].pStructure)->isClass = (yyvsp[-4].i)==CorS_Class || (yyvsp[-4].i)==CorS_ClassTemplate;
            (yyvsp[-1].pStructure)->isTemplate = (yyvsp[-4].i)==CorS_ClassTemplate || (yyvsp[-4].i)==CorS_StructTemplate;
            (yyvsp[-1].pStructure)->privateStructure = !(yyvsp[-3].b);
        }
    }
    break;

  case 789: /* structure_declaration: optional_annotation_list_with_emit_semis $@62 class_or_struct optional_public_or_private_structure $@63 structure_name optional_emit_semis $@64 optional_struct_variable_declaration_list  */
                                                      {
        yyextra->pop_nesteds();
        if ( (yyvsp[-3].pStructure) ) {
            ast_structureDeclaration ( scanner, (yyvsp[-8].faList), tokAt(scanner,(yylsp[-6])), (yyvsp[-3].pStructure), tokAt(scanner,(yylsp[-3])), (yyvsp[0].pVarDeclList) );
            if ( !yyextra->g_CommentReaders.empty() ) {
                auto tak = tokAt(scanner,(yylsp[-6]));
                for ( auto & crd : yyextra->g_CommentReaders ) crd->afterStructure((yyvsp[-3].pStructure),tak);
            }
        } else {
            deleteVariableDeclarationList((yyvsp[0].pVarDeclList));
        }
    }
    break;

  case 790: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 791: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 792: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 793: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 794: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "$i" '(' expr ')'  */
                                                                           {
        (yyvsp[-5].pNameWithPosList)->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = (yyvsp[-5].pNameWithPosList);
    }
    break;

  case 795: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 796: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 797: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 798: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 799: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 800: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 801: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 802: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 803: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 804: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 805: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 806: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 807: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 808: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 809: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 810: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 811: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 812: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 813: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 814: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 815: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 816: /* basic_type_declaration: "float16"  */
                        { (yyval.type) = Type::tFloat16; }
    break;

  case 817: /* basic_type_declaration: "half2"  */
                        { (yyval.type) = Type::tHalf2; }
    break;

  case 818: /* basic_type_declaration: "half3"  */
                        { (yyval.type) = Type::tHalf3; }
    break;

  case 819: /* basic_type_declaration: "half4"  */
                        { (yyval.type) = Type::tHalf4; }
    break;

  case 820: /* basic_type_declaration: "half8"  */
                        { (yyval.type) = Type::tHalf8; }
    break;

  case 821: /* basic_type_declaration: "short2"  */
                        { (yyval.type) = Type::tShort2; }
    break;

  case 822: /* basic_type_declaration: "short3"  */
                        { (yyval.type) = Type::tShort3; }
    break;

  case 823: /* basic_type_declaration: "short4"  */
                        { (yyval.type) = Type::tShort4; }
    break;

  case 824: /* basic_type_declaration: "short8"  */
                        { (yyval.type) = Type::tShort8; }
    break;

  case 825: /* basic_type_declaration: "ushort2"  */
                        { (yyval.type) = Type::tUShort2; }
    break;

  case 826: /* basic_type_declaration: "ushort3"  */
                        { (yyval.type) = Type::tUShort3; }
    break;

  case 827: /* basic_type_declaration: "ushort4"  */
                        { (yyval.type) = Type::tUShort4; }
    break;

  case 828: /* basic_type_declaration: "ushort8"  */
                        { (yyval.type) = Type::tUShort8; }
    break;

  case 829: /* basic_type_declaration: "byte2"  */
                        { (yyval.type) = Type::tByte2; }
    break;

  case 830: /* basic_type_declaration: "byte3"  */
                        { (yyval.type) = Type::tByte3; }
    break;

  case 831: /* basic_type_declaration: "byte4"  */
                        { (yyval.type) = Type::tByte4; }
    break;

  case 832: /* basic_type_declaration: "byte8"  */
                        { (yyval.type) = Type::tByte8; }
    break;

  case 833: /* basic_type_declaration: "byte16"  */
                        { (yyval.type) = Type::tByte16; }
    break;

  case 834: /* basic_type_declaration: "ubyte2"  */
                        { (yyval.type) = Type::tUByte2; }
    break;

  case 835: /* basic_type_declaration: "ubyte3"  */
                        { (yyval.type) = Type::tUByte3; }
    break;

  case 836: /* basic_type_declaration: "ubyte4"  */
                        { (yyval.type) = Type::tUByte4; }
    break;

  case 837: /* basic_type_declaration: "ubyte8"  */
                        { (yyval.type) = Type::tUByte8; }
    break;

  case 838: /* basic_type_declaration: "ubyte16"  */
                        { (yyval.type) = Type::tUByte16; }
    break;

  case 839: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 840: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 841: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 842: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 843: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 844: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 845: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 846: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 847: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 848: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 849: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 850: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 851: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 852: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 853: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 854: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid, tokAt(scanner,(yyloc)));
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 855: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 856: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 857: /* auto_type_declaration: "$t" '(' expr ')'  */
                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::alias, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = "``MACRO``TAG``";
        (yyval.pTypeDecl)->isTag = true;
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner, (yylsp[-1]));
        (yyval.pTypeDecl)->firstType->typeMacroExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 858: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 859: /* bitfield_bits: bitfield_bits ';' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 860: /* bitfield_bits: bitfield_bits ',' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 861: /* bitfield_alias_bits: %empty  */
        {
        auto pSL = new vector<tuple<string,Expression *>>();
        (yyval.pNameExprList) = pSL;

    }
    break;

  case 862: /* bitfield_alias_bits: "name"  */
                   {
        (yyval.pNameExprList) = new vector<tuple<string,Expression *>>();
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.pNameExprList)->emplace_back(*(yyvsp[0].s),nullptr);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[0].s)->c_str(),atvname);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 863: /* bitfield_alias_bits: "name" '=' expr  */
                                   {
        (yyval.pNameExprList) = new vector<tuple<string,Expression *>>();
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        (yyval.pNameExprList)->emplace_back(*(yyvsp[-2].s),(yyvsp[0].pExpression));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[-2].s)->c_str(),atvname);
        }
        delete (yyvsp[-2].s);
    }
    break;

  case 864: /* bitfield_alias_bits: bitfield_alias_bits commas "name"  */
                                                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameExprList)->emplace_back(*(yyvsp[0].s),nullptr);
        (yyval.pNameExprList) = (yyvsp[-2].pNameExprList);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[0].s)->c_str(),atvname);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 865: /* bitfield_alias_bits: bitfield_alias_bits commas "name" '=' expr  */
                                                                    {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        (yyvsp[-4].pNameExprList)->emplace_back(*(yyvsp[-2].s),(yyvsp[0].pExpression));
        (yyval.pNameExprList) = (yyvsp[-4].pNameExprList);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[-2].s)->c_str(),atvname);
        }
        delete (yyvsp[-2].s);
    }
    break;

  case 866: /* bitfield_basic_type_declaration: %empty  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 867: /* bitfield_basic_type_declaration: ':' "uint8"  */
                             { (yyval.type) = Type::tBitfield8; }
    break;

  case 868: /* bitfield_basic_type_declaration: ':' "uint16"  */
                             { (yyval.type) = Type::tBitfield16; }
    break;

  case 869: /* bitfield_basic_type_declaration: ':' "uint"  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 870: /* bitfield_basic_type_declaration: ':' "uint64"  */
                             { (yyval.type) = Type::tBitfield64; }
    break;

  case 871: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' '>'  */
                                                                          {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-2].type), tokAt(scanner,(yyloc)));
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-2]));
    }
    break;

  case 872: /* $@65: %empty  */
                                                                     { yyextra->das_arrow_depth ++; }
    break;

  case 873: /* $@66: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 874: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' $@65 bitfield_bits '>' $@66  */
                                                                                                                                                             {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-5].type), tokAt(scanner,(yyloc)));
            (yyval.pTypeDecl)->argNames = *(yyvsp[-2].pNameList);
            auto maxBits = (yyval.pTypeDecl)->maxBitfieldBits();
            if ( (yyval.pTypeDecl)->argNames.size()>maxBits ) {
                das_yyerror(scanner,"only " + to_string(maxBits) + " different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-5])),
                    CompilationError::exceeds_bitfield);
            }
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
            delete (yyvsp[-2].pNameList);
    }
    break;

  case 877: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid, tokAt(scanner,(yyloc)));
        (yyval.aTypePair).secondType->at = (yyval.aTypePair).firstType->at;
    }
    break;

  case 878: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 879: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = appendDimExpr(nullptr, (yyvsp[-1].pExpression), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 880: /* dim_list: '[' ']'  */
                {
        (yyval.pTypeDecl) = appendDimExpr(nullptr, nullptr, tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 881: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = appendDimExpr((yyvsp[-3].pTypeDecl), (yyvsp[-1].pExpression), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 882: /* dim_list: dim_list '[' ']'  */
                              {
        (yyval.pTypeDecl) = appendDimExpr((yyvsp[-2].pTypeDecl), nullptr, tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 883: /* type_declaration_no_options: type_declaration_no_options_no_dim  */
                                                     {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 884: /* type_declaration_no_options: type_declaration_no_options_no_dim dim_list  */
                                                                       {
        if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeDecl ) {
            das2_yyerror(scanner,"type declaration can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_array_type);
        } else if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeMacro ) {
            das2_yyerror(scanner,"macro can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_array_type);
        }
        (yyval.pTypeDecl) = attachDimChain((yyvsp[0].pTypeDecl), (yyvsp[-1].pTypeDecl));
    }
    break;

  case 885: /* optional_expr_list_in_braces: %empty  */
            { (yyval.pExpression) = nullptr; }
    break;

  case 886: /* optional_expr_list_in_braces: '(' expr_list optional_comma ')'  */
                                                { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 887: /* type_declaration_no_options_no_dim: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type), tokAt(scanner,(yyloc))); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 888: /* type_declaration_no_options_no_dim: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 889: /* type_declaration_no_options_no_dim: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 890: /* type_declaration_no_options_no_dim: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 891: /* $@67: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 892: /* $@68: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 893: /* type_declaration_no_options_no_dim: "type" '<' $@67 type_declaration '>' $@68  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 894: /* type_declaration_no_options_no_dim: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->typeMacroExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 895: /* type_declaration_no_options_no_dim: name_in_namespace '(' optional_expr_list ')'  */
                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]), (yylsp[-1]));
        (yyval.pTypeDecl)->typeMacroExpr = sequenceToList((yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-3])), *(yyvsp[-3].s)));
        delete (yyvsp[-3].s);
    }
    break;

  case 896: /* type_declaration_no_options_no_dim: '$' name_in_namespace optional_expr_list_in_braces  */
                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-1]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = sequenceToList((yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-1])), *(yyvsp[-1].s)));
        delete (yyvsp[-1].s);
    }
    break;

  case 897: /* $@69: %empty  */
                                    { yyextra->das_arrow_depth ++; }
    break;

  case 898: /* type_declaration_no_options_no_dim: name_in_namespace '<' $@69 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 899: /* $@70: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 900: /* type_declaration_no_options_no_dim: '$' name_in_namespace '<' $@70 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 901: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 902: /* type_declaration_no_options_no_dim: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 903: /* type_declaration_no_options_no_dim: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 904: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 905: /* type_declaration_no_options_no_dim: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 906: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 907: /* type_declaration_no_options_no_dim: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 908: /* type_declaration_no_options_no_dim: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 909: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 910: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 911: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 912: /* type_declaration_no_options_no_dim: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 913: /* $@71: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 914: /* $@72: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 915: /* type_declaration_no_options_no_dim: "smart_ptr" '<' $@71 type_declaration '>' $@72  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 916: /* type_declaration_no_options_no_dim: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tPointer, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 917: /* $@73: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 918: /* $@74: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 919: /* type_declaration_no_options_no_dim: "array" '<' $@73 type_declaration '>' $@74  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 920: /* $@75: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 921: /* $@76: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 922: /* type_declaration_no_options_no_dim: "table" '<' $@75 table_type_pair '>' $@76  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 923: /* $@77: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 924: /* $@78: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 925: /* type_declaration_no_options_no_dim: "iterator" '<' $@77 type_declaration '>' $@78  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 926: /* type_declaration_no_options_no_dim: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->firstType->at = (yyval.pTypeDecl)->at;
    }
    break;

  case 927: /* $@79: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 928: /* $@80: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 929: /* type_declaration_no_options_no_dim: "block" '<' $@79 type_declaration '>' $@80  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 930: /* $@81: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 931: /* $@82: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 932: /* type_declaration_no_options_no_dim: "block" '<' $@81 optional_function_argument_list optional_function_type '>' $@82  */
                                                                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
        if ( (yyvsp[-3].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-3].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        }
    }
    break;

  case 933: /* type_declaration_no_options_no_dim: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->firstType->at = (yyval.pTypeDecl)->at;
    }
    break;

  case 934: /* $@83: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 935: /* $@84: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 936: /* type_declaration_no_options_no_dim: "function" '<' $@83 type_declaration '>' $@84  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 937: /* $@85: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 938: /* $@86: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 939: /* type_declaration_no_options_no_dim: "function" '<' $@85 optional_function_argument_list optional_function_type '>' $@86  */
                                                                                                                                                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
        if ( (yyvsp[-3].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-3].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        }
    }
    break;

  case 940: /* type_declaration_no_options_no_dim: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->firstType->at = (yyval.pTypeDecl)->at;
    }
    break;

  case 941: /* $@87: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 942: /* $@88: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 943: /* type_declaration_no_options_no_dim: "lambda" '<' $@87 type_declaration '>' $@88  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 944: /* $@89: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 945: /* $@90: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 946: /* type_declaration_no_options_no_dim: "lambda" '<' $@89 optional_function_argument_list optional_function_type '>' $@90  */
                                                                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
        if ( (yyvsp[-3].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-3].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        }
    }
    break;

  case 947: /* $@91: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 948: /* $@92: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 949: /* type_declaration_no_options_no_dim: "tuple" '<' $@91 tuple_type_list '>' $@92  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 950: /* $@93: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 951: /* $@94: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 952: /* type_declaration_no_options_no_dim: "variant" '<' $@93 variant_type_list '>' $@94  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant, tokAt(scanner,(yyloc)));
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 953: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 954: /* type_declaration: type_declaration '|' type_declaration_no_options  */
                                                                     {
        if ( (yyvsp[-2].pTypeDecl)->baseType==Type::option ) {
            (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
            (yyval.pTypeDecl)->argTypes.push_back((yyvsp[0].pTypeDecl));
        } else {
            (yyval.pTypeDecl) = new TypeDecl(Type::option, tokAt(scanner,(yyloc)));
            (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
            (yyval.pTypeDecl)->argTypes.push_back((yyvsp[-2].pTypeDecl));
            (yyval.pTypeDecl)->argTypes.push_back((yyvsp[0].pTypeDecl));
        }
    }
    break;

  case 955: /* type_declaration: type_declaration '|' '#'  */
                                             {
        if ( (yyvsp[-2].pTypeDecl)->baseType==Type::option ) {
            (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
            (yyval.pTypeDecl)->argTypes.push_back(new TypeDecl(*(yyvsp[-2].pTypeDecl)->argTypes.back()));
            (yyvsp[-2].pTypeDecl)->argTypes.back()->temporary ^= true;
        } else {
            (yyval.pTypeDecl) = new TypeDecl(Type::option, tokAt(scanner,(yyloc)));
            (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
            (yyval.pTypeDecl)->argTypes.push_back((yyvsp[-2].pTypeDecl));
            (yyval.pTypeDecl)->argTypes.push_back(new TypeDecl(*(yyvsp[-2].pTypeDecl)));
            (yyval.pTypeDecl)->argTypes.back()->temporary ^= true;
        }
    }
    break;

  case 956: /* $@95: %empty  */
                   {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 957: /* $@96: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 958: /* $@97: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 959: /* $@98: %empty  */
                                                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 960: /* tuple_alias_declaration: "tuple" $@95 optional_public_or_private_alias "name" optional_emit_semis $@96 '{' $@97 tuple_alias_type_list optional_semis $@98 '}'  */
          {
        auto vtype = new TypeDecl(Type::tTuple, tokAt(scanner,(yyloc)));
        vtype->alias = *(yyvsp[-8].s);
        vtype->at = tokAt(scanner,(yylsp[-8]));
        vtype->isPrivateAlias = !(yyvsp[-9].b);
        varDeclToTypeDecl(scanner, vtype, (yyvsp[-3].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-8].s),tokAt(scanner,(yylsp[-8])),
                CompilationError::already_declared_type_alias);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-8]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTuple((yyvsp[-8].s)->c_str(),atvname);
        }
        delete (yyvsp[-8].s);
    }
    break;

  case 961: /* $@99: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 962: /* $@100: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 963: /* $@101: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 964: /* $@102: %empty  */
                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 965: /* variant_alias_declaration: "variant" $@99 optional_public_or_private_alias "name" optional_emit_semis $@100 '{' $@101 variant_alias_type_list optional_semis $@102 '}'  */
          {
        auto vtype = new TypeDecl(Type::tVariant, tokAt(scanner,(yyloc)));
        vtype->alias = *(yyvsp[-8].s);
        vtype->at = tokAt(scanner,(yylsp[-8]));
        vtype->isPrivateAlias = !(yyvsp[-9].b);
        varDeclToTypeDecl(scanner, vtype, (yyvsp[-3].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-8].s),tokAt(scanner,(yylsp[-8])),
                CompilationError::already_declared_type_alias);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-8]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariant((yyvsp[-8].s)->c_str(),atvname);
        }
        delete (yyvsp[-8].s);
    }
    break;

  case 966: /* $@103: %empty  */
                      {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 967: /* $@104: %empty  */
                                                                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 968: /* $@105: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 969: /* $@106: %empty  */
                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-7]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 970: /* bitfield_alias_declaration: "bitfield" $@103 optional_public_or_private_alias "name" bitfield_basic_type_declaration optional_emit_commas $@104 '{' $@105 bitfield_alias_bits optional_commas $@106 '}'  */
          {
        auto btype = new TypeDecl((yyvsp[-8].type), tokAt(scanner,(yyloc)));
        btype->alias = *(yyvsp[-9].s);
        btype->at = tokAt(scanner,(yylsp[-9]));
        btype->isPrivateAlias = !(yyvsp[-10].b);
        for ( auto & p : *(yyvsp[-3].pNameExprList) ) {
            if ( !get<1>(p) ) {
                btype->argNames.push_back(get<0>(p));
            }
        }
        auto maxBits = btype->maxBitfieldBits();
        if ( btype->argNames.size()>maxBits ) {
            das_yyerror(scanner,"only " + to_string(maxBits) + " different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-9])),
                CompilationError::exceeds_bitfield);
        }
        for ( auto & p : *(yyvsp[-3].pNameExprList) ) {
            if ( get<1>(p) ) {
                ast_globalBitfieldConst ( scanner, btype, (yyvsp[-10].b), get<0>(p), get<1>(p) );
            }
        }
        if ( !yyextra->g_Program->addAlias(btype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-9].s),tokAt(scanner,(yylsp[-9])),
                CompilationError::already_declared_type_alias);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-9]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfield((yyvsp[-9].s)->c_str(),atvname);
        }
        delete (yyvsp[-9].s);
        delete (yyvsp[-3].pNameExprList);
    }
    break;

  case 971: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 972: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 973: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 974: /* make_decl: make_table_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 975: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 976: /* make_decl: table_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 977: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 978: /* make_decl_no_bracket: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 979: /* make_decl_no_bracket: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 980: /* make_decl_no_bracket: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 981: /* make_decl_no_bracket: table_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 982: /* make_decl_no_bracket: make_table_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 983: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 984: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 985: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 986: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 987: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 988: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 989: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 990: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 991: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 992: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 993: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 994: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 995: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 996: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 997: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 998: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 999: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 1000: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 1001: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 1002: /* $@107: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 1003: /* $@108: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 1004: /* make_struct_decl: "struct" '<' $@107 type_declaration_no_options '>' $@108 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                      {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 1005: /* $@109: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 1006: /* $@110: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 1007: /* make_struct_decl: "class" '<' $@109 type_declaration_no_options '>' $@110 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                     {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 1008: /* $@111: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 1009: /* $@112: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 1010: /* make_struct_decl: "variant" '<' $@111 variant_type_list '>' $@112 '(' use_initializer make_variant_dim ')'  */
                                                                                                                                                                                  {
        auto mkt = new TypeDecl(Type::tVariant, tokAt(scanner,(yylsp[-9])));
        varDeclToTypeDecl(scanner, mkt, (yyvsp[-6].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-6].pVarDeclList));
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = mkt;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 1011: /* $@113: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 1012: /* $@114: %empty  */
                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 1013: /* make_struct_decl: "variant" "type" '<' $@113 type_declaration_no_options '>' $@114 '(' use_initializer make_variant_dim ')'  */
                                                                                                                                                                                                    {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-10]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 1014: /* $@115: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 1015: /* $@116: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 1016: /* make_struct_decl: "default" '<' $@115 type_declaration_no_options '>' $@116 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 1017: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
        (yyval.pExpression) = mkt;
    }
    break;

  case 1018: /* $@117: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 1019: /* $@118: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 1020: /* make_tuple_call: "tuple" '<' $@117 tuple_type_list '>' $@118 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                 {
        auto mkt = new TypeDecl(Type::tTuple, tokAt(scanner,(yylsp[-9])));
        varDeclToTypeDecl(scanner, mkt, (yyvsp[-6].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-6].pVarDeclList));
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = mkt;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceTuple = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 1021: /* make_dim_decl: '[' optional_expr_list ']'  */
                                                  {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-2])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
            mka->gen2 = true;
            auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_array_move");
            tam->arguments.push_back(mka);
            (yyval.pExpression) = tam;
        } else {
            auto mks = new ExprMakeStruct();
            mks->at = tokAt(scanner,(yylsp[-2]));
            mks->makeType = new TypeDecl(Type::tArray, mks->at);
            mks->makeType->firstType = new TypeDecl(Type::autoinfer, mks->at);
            mks->useInitializer = true;
            mks->alwaysUseInitializer = true;
            (yyval.pExpression) = mks;
        }
    }
    break;

  case 1022: /* $@119: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 1023: /* $@120: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 1024: /* make_dim_decl: "array" "struct" '<' $@119 type_declaration_no_options '>' $@120 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 1025: /* $@121: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 1026: /* $@122: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 1027: /* make_dim_decl: "array" "tuple" '<' $@121 tuple_type_list '>' $@122 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                     {
        auto mkt = new TypeDecl(Type::tTuple, tokAt(scanner,(yylsp[-10])));
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

  case 1028: /* $@123: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 1029: /* $@124: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 1030: /* make_dim_decl: "array" "variant" '<' $@123 variant_type_list '>' $@124 '(' make_variant_dim ')'  */
                                                                                                                                                                      {
        auto mkt = new TypeDecl(Type::tVariant, tokAt(scanner,(yylsp[-9])));
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

  case 1031: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
                                                                   {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
        mka->gen2 = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_array_move");
        tam->arguments.push_back(mka);
        (yyval.pExpression) = tam;
    }
    break;

  case 1032: /* $@125: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 1033: /* $@126: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 1034: /* make_dim_decl: "array" '<' $@125 type_declaration_no_options '>' $@126 '(' optional_expr_list ')'  */
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
            msd->makeType = new TypeDecl(Type::tArray, msd->at);
            msd->makeType->firstType = (yyvsp[-5].pTypeDecl);
            msd->at = tokAt(scanner,(yylsp[-5]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 1035: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 1036: /* $@127: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 1037: /* $@128: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 1038: /* make_dim_decl: "fixed_array" '<' $@127 type_declaration_no_options '>' $@128 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 1039: /* expr_map_tuple_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 1040: /* expr_map_tuple_list: expr_map_tuple_list ',' expr  */
                                                      {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 1041: /* push_table_nesting: %empty  */
                    {
        yyextra->das_nested_parentheses ++;
    }
    break;

  case 1042: /* make_table_decl: '{' push_table_nesting optional_emit_semis optional_expr_map_tuple_list '}'  */
                                                                                                     {
        yyextra->das_nested_parentheses --;
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto mks = new ExprMakeStruct();
            mks->at = tokAt(scanner,(yylsp[-4]));
            mks->makeType = new TypeDecl(Type::tTable, tokAt(scanner,(yyloc)));
            mks->makeType->firstType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yylsp[-4])));
            mks->makeType->secondType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yylsp[0])));
            mks->useInitializer = true;
            mks->alwaysUseInitializer = true;
            (yyval.pExpression) = mks;
        }
    }
    break;

  case 1043: /* make_table_call: "table" '(' expr_map_tuple_list optional_comma ')'  */
                                                                             {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer, tokAt(scanner,(yyloc)));
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 1044: /* make_table_call: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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
            msd->makeType = new TypeDecl(Type::tTable, msd->at);
            msd->makeType->firstType = (yyvsp[-4].pTypeDecl);
            msd->makeType->secondType = new TypeDecl(Type::tVoid, tokAt(scanner,(yylsp[-6])));
            msd->at = tokAt(scanner,(yylsp[-6]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 1045: /* make_table_call: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
                                                                                                                                                             {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-8])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = new TypeDecl(Type::tTuple, tokAt(scanner,(yyloc)));
            mka->makeType->argTypes.push_back((yyvsp[-6].pTypeDecl));
            mka->makeType->argTypes.push_back((yyvsp[-4].pTypeDecl));
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-8])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto msd = new ExprMakeStruct();
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->makeType = new TypeDecl(Type::tTable, msd->at);
            msd->makeType->firstType = (yyvsp[-6].pTypeDecl);
            msd->makeType->secondType = (yyvsp[-4].pTypeDecl);
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 1046: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 1047: /* array_comprehension_where: ';' "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 1048: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 1049: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 1050: /* table_comprehension: '[' "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                               {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 1051: /* table_comprehension: '[' "iterator" "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 1052: /* array_comprehension: '{' push_table_nesting optional_emit_semis "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where '}'  */
                                                                                                                                                                                                      {
        yyextra->das_nested_parentheses --;
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
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
  yytoken = yychar == DAS2_YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
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

      if (yychar <= DAS2_YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == DAS2_YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, scanner);
          yychar = DAS2_YYEMPTY;
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
  if (yychar != DAS2_YYEMPTY)
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



void das2_yyfatalerror ( DAS2_YYLTYPE * lloc, yyscan_t scanner, const string & error, CompilationError cerr ) {
    yyextra->g_Program->error(error,"","",LineInfo(yyextra->g_FileAccessStack.back(),
        lloc->first_column,lloc->first_line,lloc->last_column,lloc->last_line),cerr);
}

void das2_yyerror ( DAS2_YYLTYPE * lloc, yyscan_t scanner, const string & error ) {
    if ( !yyextra->das_suppress_errors ) {
        yyextra->g_Program->error(error,"","",LineInfo(yyextra->g_FileAccessStack.back(),
            lloc->first_column,lloc->first_line,lloc->last_column,lloc->last_line),
                CompilationError::invalid_expression);
    }
}

LineInfo tokAt ( yyscan_t scanner, const struct DAS2_YYLTYPE & li ) {
    return LineInfo(yyextra->g_FileAccessStack.back(),
        li.first_column,li.first_line,
        li.last_column,li.last_line);
}

LineInfo tokRangeAt ( yyscan_t scanner, const struct DAS2_YYLTYPE & li, const struct DAS2_YYLTYPE & lie ) {
    return LineInfo(yyextra->g_FileAccessStack.back(),
        li.first_column,li.first_line,
        lie.last_column,lie.last_line);
}


