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
  YYSYMBOL_LPIPE = 154,                    /* "<|"  */
  YYSYMBOL_RPIPE = 155,                    /* "|>"  */
  YYSYMBOL_CLONEEQU = 156,                 /* ":="  */
  YYSYMBOL_ROTL = 157,                     /* "<<<"  */
  YYSYMBOL_ROTR = 158,                     /* ">>>"  */
  YYSYMBOL_ROTLEQU = 159,                  /* "<<<="  */
  YYSYMBOL_ROTREQU = 160,                  /* ">>>="  */
  YYSYMBOL_MAPTO = 161,                    /* "=>"  */
  YYSYMBOL_DOUBLE_AT = 162,                /* "@@"  */
  YYSYMBOL_AT_FIELD = 163,                 /* "@field"  */
  YYSYMBOL_COLCOL = 164,                   /* "::"  */
  YYSYMBOL_ANDAND = 165,                   /* "&&"  */
  YYSYMBOL_OROR = 166,                     /* "||"  */
  YYSYMBOL_XORXOR = 167,                   /* "^^"  */
  YYSYMBOL_ANDANDEQU = 168,                /* "&&="  */
  YYSYMBOL_OROREQU = 169,                  /* "||="  */
  YYSYMBOL_XORXOREQU = 170,                /* "^^="  */
  YYSYMBOL_DOTDOT = 171,                   /* ".."  */
  YYSYMBOL_MTAG_E = 172,                   /* "$$"  */
  YYSYMBOL_MTAG_I = 173,                   /* "$i"  */
  YYSYMBOL_MTAG_V = 174,                   /* "$v"  */
  YYSYMBOL_MTAG_B = 175,                   /* "$b"  */
  YYSYMBOL_MTAG_A = 176,                   /* "$a"  */
  YYSYMBOL_MTAG_T = 177,                   /* "$t"  */
  YYSYMBOL_MTAG_C = 178,                   /* "$c"  */
  YYSYMBOL_MTAG_F = 179,                   /* "$f"  */
  YYSYMBOL_MTAG_DOTDOTDOT = 180,           /* "..."  */
  YYSYMBOL_INTEGER = 181,                  /* "integer constant"  */
  YYSYMBOL_LONG_INTEGER = 182,             /* "long integer constant"  */
  YYSYMBOL_UNSIGNED_INTEGER = 183,         /* "unsigned integer constant"  */
  YYSYMBOL_UNSIGNED_LONG_INTEGER = 184,    /* "unsigned long integer constant"  */
  YYSYMBOL_UNSIGNED_INT8 = 185,            /* "unsigned int8 constant"  */
  YYSYMBOL_DAS_FLOAT = 186,                /* "floating point constant"  */
  YYSYMBOL_DAS_FLOAT16_CONST = 187,        /* "float16 constant"  */
  YYSYMBOL_DOUBLE = 188,                   /* "double constant"  */
  YYSYMBOL_NAME = 189,                     /* "name"  */
  YYSYMBOL_DAS_EMIT_COMMA = 190,           /* "new line, comma"  */
  YYSYMBOL_DAS_EMIT_SEMICOLON = 191,       /* "new line, semicolon"  */
  YYSYMBOL_BEGIN_STRING = 192,             /* "start of the string"  */
  YYSYMBOL_STRING_CHARACTER = 193,         /* STRING_CHARACTER  */
  YYSYMBOL_STRING_CHARACTER_ESC = 194,     /* STRING_CHARACTER_ESC  */
  YYSYMBOL_END_STRING = 195,               /* "end of the string"  */
  YYSYMBOL_BEGIN_STRING_EXPR = 196,        /* "{"  */
  YYSYMBOL_END_STRING_EXPR = 197,          /* "}"  */
  YYSYMBOL_END_OF_READ = 198,              /* "end of failed eader macro"  */
  YYSYMBOL_199_ = 199,                     /* ','  */
  YYSYMBOL_200_ = 200,                     /* '='  */
  YYSYMBOL_201_ = 201,                     /* '?'  */
  YYSYMBOL_202_ = 202,                     /* ':'  */
  YYSYMBOL_203_ = 203,                     /* '|'  */
  YYSYMBOL_204_ = 204,                     /* '^'  */
  YYSYMBOL_205_ = 205,                     /* '&'  */
  YYSYMBOL_206_ = 206,                     /* '<'  */
  YYSYMBOL_207_ = 207,                     /* '>'  */
  YYSYMBOL_208_ = 208,                     /* '-'  */
  YYSYMBOL_209_ = 209,                     /* '+'  */
  YYSYMBOL_210_ = 210,                     /* '*'  */
  YYSYMBOL_211_ = 211,                     /* '/'  */
  YYSYMBOL_212_ = 212,                     /* '%'  */
  YYSYMBOL_UNARY_MINUS = 213,              /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 214,               /* UNARY_PLUS  */
  YYSYMBOL_215_ = 215,                     /* '~'  */
  YYSYMBOL_216_ = 216,                     /* '!'  */
  YYSYMBOL_PRE_INC = 217,                  /* PRE_INC  */
  YYSYMBOL_PRE_DEC = 218,                  /* PRE_DEC  */
  YYSYMBOL_LLPIPE = 219,                   /* LLPIPE  */
  YYSYMBOL_POST_INC = 220,                 /* POST_INC  */
  YYSYMBOL_POST_DEC = 221,                 /* POST_DEC  */
  YYSYMBOL_DEREF = 222,                    /* DEREF  */
  YYSYMBOL_223_ = 223,                     /* '.'  */
  YYSYMBOL_224_ = 224,                     /* '['  */
  YYSYMBOL_225_ = 225,                     /* ']'  */
  YYSYMBOL_226_ = 226,                     /* '('  */
  YYSYMBOL_227_ = 227,                     /* ')'  */
  YYSYMBOL_228_ = 228,                     /* '$'  */
  YYSYMBOL_229_ = 229,                     /* '@'  */
  YYSYMBOL_230_ = 230,                     /* ';'  */
  YYSYMBOL_231_ = 231,                     /* '{'  */
  YYSYMBOL_232_ = 232,                     /* '}'  */
  YYSYMBOL_233_ = 233,                     /* '#'  */
  YYSYMBOL_YYACCEPT = 234,                 /* $accept  */
  YYSYMBOL_program = 235,                  /* program  */
  YYSYMBOL_COMMA = 236,                    /* COMMA  */
  YYSYMBOL_SEMICOLON = 237,                /* SEMICOLON  */
  YYSYMBOL_top_level_reader_macro = 238,   /* top_level_reader_macro  */
  YYSYMBOL_optional_public_or_private_module = 239, /* optional_public_or_private_module  */
  YYSYMBOL_module_name = 240,              /* module_name  */
  YYSYMBOL_optional_not_required = 241,    /* optional_not_required  */
  YYSYMBOL_module_declaration = 242,       /* module_declaration  */
  YYSYMBOL_character_sequence = 243,       /* character_sequence  */
  YYSYMBOL_string_constant = 244,          /* string_constant  */
  YYSYMBOL_format_string = 245,            /* format_string  */
  YYSYMBOL_optional_format_string = 246,   /* optional_format_string  */
  YYSYMBOL_247_1 = 247,                    /* $@1  */
  YYSYMBOL_string_builder_body = 248,      /* string_builder_body  */
  YYSYMBOL_string_builder = 249,           /* string_builder  */
  YYSYMBOL_reader_character_sequence = 250, /* reader_character_sequence  */
  YYSYMBOL_expr_reader = 251,              /* expr_reader  */
  YYSYMBOL_252_2 = 252,                    /* $@2  */
  YYSYMBOL_options_declaration = 253,      /* options_declaration  */
  YYSYMBOL_require_declaration = 254,      /* require_declaration  */
  YYSYMBOL_require_module_name = 255,      /* require_module_name  */
  YYSYMBOL_optional_require_guard = 256,   /* optional_require_guard  */
  YYSYMBOL_require_module = 257,           /* require_module  */
  YYSYMBOL_is_public_module = 258,         /* is_public_module  */
  YYSYMBOL_expect_declaration = 259,       /* expect_declaration  */
  YYSYMBOL_expect_list = 260,              /* expect_list  */
  YYSYMBOL_expect_error = 261,             /* expect_error  */
  YYSYMBOL_expression_label = 262,         /* expression_label  */
  YYSYMBOL_expression_goto = 263,          /* expression_goto  */
  YYSYMBOL_elif_or_static_elif = 264,      /* elif_or_static_elif  */
  YYSYMBOL_emit_semis = 265,               /* emit_semis  */
  YYSYMBOL_optional_emit_semis = 266,      /* optional_emit_semis  */
  YYSYMBOL_expression_else = 267,          /* expression_else  */
  YYSYMBOL_268_3 = 268,                    /* $@3  */
  YYSYMBOL_269_4 = 269,                    /* $@4  */
  YYSYMBOL_if_or_static_if = 270,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 271, /* expression_else_one_liner  */
  YYSYMBOL_expression_if_one_liner = 272,  /* expression_if_one_liner  */
  YYSYMBOL_semis = 273,                    /* semis  */
  YYSYMBOL_optional_semis = 274,           /* optional_semis  */
  YYSYMBOL_expression_if_block = 275,      /* expression_if_block  */
  YYSYMBOL_276_5 = 276,                    /* $@5  */
  YYSYMBOL_277_6 = 277,                    /* $@6  */
  YYSYMBOL_278_7 = 278,                    /* $@7  */
  YYSYMBOL_expression_else_block = 279,    /* expression_else_block  */
  YYSYMBOL_280_8 = 280,                    /* $@8  */
  YYSYMBOL_281_9 = 281,                    /* $@9  */
  YYSYMBOL_282_10 = 282,                   /* $@10  */
  YYSYMBOL_expression_if_then_else = 283,  /* expression_if_then_else  */
  YYSYMBOL_284_11 = 284,                   /* $@11  */
  YYSYMBOL_285_12 = 285,                   /* $@12  */
  YYSYMBOL_expression_if_then_else_oneliner = 286, /* expression_if_then_else_oneliner  */
  YYSYMBOL_for_variable_name_with_pos_list = 287, /* for_variable_name_with_pos_list  */
  YYSYMBOL_expression_for_loop = 288,      /* expression_for_loop  */
  YYSYMBOL_289_13 = 289,                   /* $@13  */
  YYSYMBOL_expression_unsafe = 290,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 291,    /* expression_while_loop  */
  YYSYMBOL_292_14 = 292,                   /* $@14  */
  YYSYMBOL_with_keyword_on = 293,          /* with_keyword_on  */
  YYSYMBOL_expression_with = 294,          /* expression_with  */
  YYSYMBOL_expression_with_alias = 295,    /* expression_with_alias  */
  YYSYMBOL_annotation_argument_value = 296, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 297, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 298, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 299,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 300, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 301,   /* metadata_argument_list  */
  YYSYMBOL_optional_for_annotations = 302, /* optional_for_annotations  */
  YYSYMBOL_annotation_declaration_name = 303, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 304, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 305,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 306,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 307, /* optional_annotation_list  */
  YYSYMBOL_optional_annotation_list_with_emit_semis = 308, /* optional_annotation_list_with_emit_semis  */
  YYSYMBOL_optional_function_argument_list = 309, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 310,   /* optional_function_type  */
  YYSYMBOL_function_name = 311,            /* function_name  */
  YYSYMBOL_das_type_name = 312,            /* das_type_name  */
  YYSYMBOL_optional_template = 313,        /* optional_template  */
  YYSYMBOL_global_function_declaration = 314, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 315, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 316, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 317,     /* function_declaration  */
  YYSYMBOL_318_15 = 318,                   /* $@15  */
  YYSYMBOL_expression_block_finally = 319, /* expression_block_finally  */
  YYSYMBOL_320_16 = 320,                   /* $@16  */
  YYSYMBOL_321_17 = 321,                   /* $@17  */
  YYSYMBOL_expression_block = 322,         /* expression_block  */
  YYSYMBOL_323_18 = 323,                   /* $@18  */
  YYSYMBOL_324_19 = 324,                   /* $@19  */
  YYSYMBOL_expr_call_pipe_no_bracket = 325, /* expr_call_pipe_no_bracket  */
  YYSYMBOL_expression_any = 326,           /* expression_any  */
  YYSYMBOL_327_20 = 327,                   /* $@20  */
  YYSYMBOL_328_21 = 328,                   /* $@21  */
  YYSYMBOL_expressions = 329,              /* expressions  */
  YYSYMBOL_optional_expr_list = 330,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_map_tuple_list = 331, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 332, /* type_declaration_no_options_list  */
  YYSYMBOL_name_in_namespace = 333,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 334,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 335,     /* new_type_declaration  */
  YYSYMBOL_336_22 = 336,                   /* $@22  */
  YYSYMBOL_337_23 = 337,                   /* $@23  */
  YYSYMBOL_expr_new = 338,                 /* expr_new  */
  YYSYMBOL_expression_break = 339,         /* expression_break  */
  YYSYMBOL_expression_continue = 340,      /* expression_continue  */
  YYSYMBOL_expression_return = 341,        /* expression_return  */
  YYSYMBOL_expression_yield = 342,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 343,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 344,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 345,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 346,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 347,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 348, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 349,           /* expression_let  */
  YYSYMBOL_expr_cast = 350,                /* expr_cast  */
  YYSYMBOL_351_24 = 351,                   /* $@24  */
  YYSYMBOL_352_25 = 352,                   /* $@25  */
  YYSYMBOL_353_26 = 353,                   /* $@26  */
  YYSYMBOL_354_27 = 354,                   /* $@27  */
  YYSYMBOL_355_28 = 355,                   /* $@28  */
  YYSYMBOL_356_29 = 356,                   /* $@29  */
  YYSYMBOL_expr_type_decl = 357,           /* expr_type_decl  */
  YYSYMBOL_358_30 = 358,                   /* $@30  */
  YYSYMBOL_359_31 = 359,                   /* $@31  */
  YYSYMBOL_expr_type_info = 360,           /* expr_type_info  */
  YYSYMBOL_expr_list = 361,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 362,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 363,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 364,            /* capture_entry  */
  YYSYMBOL_capture_list = 365,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 366,    /* optional_capture_list  */
  YYSYMBOL_expr_full_block = 367,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 368, /* expr_full_block_assumed_piped  */
  YYSYMBOL_expr_numeric_const = 369,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign_no_bracket = 370,   /* expr_assign_no_bracket  */
  YYSYMBOL_expr_named_call = 371,          /* expr_named_call  */
  YYSYMBOL_expr_method_call_no_bracket = 372, /* expr_method_call_no_bracket  */
  YYSYMBOL_func_addr_name = 373,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 374,           /* func_addr_expr  */
  YYSYMBOL_375_32 = 375,                   /* $@32  */
  YYSYMBOL_376_33 = 376,                   /* $@33  */
  YYSYMBOL_377_34 = 377,                   /* $@34  */
  YYSYMBOL_378_35 = 378,                   /* $@35  */
  YYSYMBOL_expr_field_no_bracket = 379,    /* expr_field_no_bracket  */
  YYSYMBOL_380_36 = 380,                   /* $@36  */
  YYSYMBOL_381_37 = 381,                   /* $@37  */
  YYSYMBOL_expr_call = 382,                /* expr_call  */
  YYSYMBOL_expr = 383,                     /* expr  */
  YYSYMBOL_expr_no_bracket = 384,          /* expr_no_bracket  */
  YYSYMBOL_385_38 = 385,                   /* $@38  */
  YYSYMBOL_386_39 = 386,                   /* $@39  */
  YYSYMBOL_387_40 = 387,                   /* $@40  */
  YYSYMBOL_388_41 = 388,                   /* $@41  */
  YYSYMBOL_389_42 = 389,                   /* $@42  */
  YYSYMBOL_390_43 = 390,                   /* $@43  */
  YYSYMBOL_391_44 = 391,                   /* $@44  */
  YYSYMBOL_392_45 = 392,                   /* $@45  */
  YYSYMBOL_expr_generator = 393,           /* expr_generator  */
  YYSYMBOL_expr_mtag_no_bracket = 394,     /* expr_mtag_no_bracket  */
  YYSYMBOL_optional_field_annotation = 395, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 396,        /* optional_override  */
  YYSYMBOL_optional_constant = 397,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 398, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 399, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 400, /* structure_variable_declaration  */
  YYSYMBOL_struct_variable_declaration_list = 401, /* struct_variable_declaration_list  */
  YYSYMBOL_402_46 = 402,                   /* $@46  */
  YYSYMBOL_403_47 = 403,                   /* $@47  */
  YYSYMBOL_404_48 = 404,                   /* $@48  */
  YYSYMBOL_function_argument_declaration_no_type = 405, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 406, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 407,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 408,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 409,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 410,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 411,             /* variant_type  */
  YYSYMBOL_variant_type_list = 412,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 413,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 414,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 415, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 416, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 417,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 418,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 419,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 420, /* let_variable_name_with_pos_list  */
  YYSYMBOL_global_let_variable_name_with_pos_list = 421, /* global_let_variable_name_with_pos_list  */
  YYSYMBOL_variable_declaration_list = 422, /* variable_declaration_list  */
  YYSYMBOL_let_variable_declaration = 423, /* let_variable_declaration  */
  YYSYMBOL_global_let_variable_declaration = 424, /* global_let_variable_declaration  */
  YYSYMBOL_optional_shared = 425,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 426, /* optional_public_or_private_variable  */
  YYSYMBOL_global_variable_declaration_list = 427, /* global_variable_declaration_list  */
  YYSYMBOL_428_49 = 428,                   /* $@49  */
  YYSYMBOL_global_let = 429,               /* global_let  */
  YYSYMBOL_430_50 = 430,                   /* $@50  */
  YYSYMBOL_enum_expression = 431,          /* enum_expression  */
  YYSYMBOL_commas = 432,                   /* commas  */
  YYSYMBOL_enum_list = 433,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 434, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 435,             /* single_alias  */
  YYSYMBOL_436_51 = 436,                   /* $@51  */
  YYSYMBOL_alias_declaration = 437,        /* alias_declaration  */
  YYSYMBOL_distinct_alias = 438,           /* distinct_alias  */
  YYSYMBOL_optional_public_or_private_enum = 439, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 440,                /* enum_name  */
  YYSYMBOL_optional_enum_basic_type_declaration = 441, /* optional_enum_basic_type_declaration  */
  YYSYMBOL_optional_commas = 442,          /* optional_commas  */
  YYSYMBOL_emit_commas = 443,              /* emit_commas  */
  YYSYMBOL_optional_emit_commas = 444,     /* optional_emit_commas  */
  YYSYMBOL_enum_declaration = 445,         /* enum_declaration  */
  YYSYMBOL_446_52 = 446,                   /* $@52  */
  YYSYMBOL_447_53 = 447,                   /* $@53  */
  YYSYMBOL_448_54 = 448,                   /* $@54  */
  YYSYMBOL_optional_structure_parent = 449, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 450,          /* optional_sealed  */
  YYSYMBOL_structure_name = 451,           /* structure_name  */
  YYSYMBOL_class_or_struct = 452,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 453, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 454, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 455,    /* structure_declaration  */
  YYSYMBOL_456_55 = 456,                   /* $@55  */
  YYSYMBOL_457_56 = 457,                   /* $@56  */
  YYSYMBOL_458_57 = 458,                   /* $@57  */
  YYSYMBOL_variable_name_with_pos_list = 459, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 460,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 461, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 462, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 463,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 464,            /* bitfield_bits  */
  YYSYMBOL_bitfield_alias_bits = 465,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_basic_type_declaration = 466, /* bitfield_basic_type_declaration  */
  YYSYMBOL_bitfield_type_declaration = 467, /* bitfield_type_declaration  */
  YYSYMBOL_468_58 = 468,                   /* $@58  */
  YYSYMBOL_469_59 = 469,                   /* $@59  */
  YYSYMBOL_c_or_s = 470,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 471,          /* table_type_pair  */
  YYSYMBOL_dim_list = 472,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 473, /* type_declaration_no_options  */
  YYSYMBOL_optional_expr_list_in_braces = 474, /* optional_expr_list_in_braces  */
  YYSYMBOL_type_declaration_no_options_no_dim = 475, /* type_declaration_no_options_no_dim  */
  YYSYMBOL_476_60 = 476,                   /* $@60  */
  YYSYMBOL_477_61 = 477,                   /* $@61  */
  YYSYMBOL_478_62 = 478,                   /* $@62  */
  YYSYMBOL_479_63 = 479,                   /* $@63  */
  YYSYMBOL_480_64 = 480,                   /* $@64  */
  YYSYMBOL_481_65 = 481,                   /* $@65  */
  YYSYMBOL_482_66 = 482,                   /* $@66  */
  YYSYMBOL_483_67 = 483,                   /* $@67  */
  YYSYMBOL_484_68 = 484,                   /* $@68  */
  YYSYMBOL_485_69 = 485,                   /* $@69  */
  YYSYMBOL_486_70 = 486,                   /* $@70  */
  YYSYMBOL_487_71 = 487,                   /* $@71  */
  YYSYMBOL_488_72 = 488,                   /* $@72  */
  YYSYMBOL_489_73 = 489,                   /* $@73  */
  YYSYMBOL_490_74 = 490,                   /* $@74  */
  YYSYMBOL_491_75 = 491,                   /* $@75  */
  YYSYMBOL_492_76 = 492,                   /* $@76  */
  YYSYMBOL_493_77 = 493,                   /* $@77  */
  YYSYMBOL_494_78 = 494,                   /* $@78  */
  YYSYMBOL_495_79 = 495,                   /* $@79  */
  YYSYMBOL_496_80 = 496,                   /* $@80  */
  YYSYMBOL_497_81 = 497,                   /* $@81  */
  YYSYMBOL_498_82 = 498,                   /* $@82  */
  YYSYMBOL_499_83 = 499,                   /* $@83  */
  YYSYMBOL_500_84 = 500,                   /* $@84  */
  YYSYMBOL_501_85 = 501,                   /* $@85  */
  YYSYMBOL_502_86 = 502,                   /* $@86  */
  YYSYMBOL_503_87 = 503,                   /* $@87  */
  YYSYMBOL_type_declaration = 504,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 505,  /* tuple_alias_declaration  */
  YYSYMBOL_506_88 = 506,                   /* $@88  */
  YYSYMBOL_507_89 = 507,                   /* $@89  */
  YYSYMBOL_508_90 = 508,                   /* $@90  */
  YYSYMBOL_509_91 = 509,                   /* $@91  */
  YYSYMBOL_variant_alias_declaration = 510, /* variant_alias_declaration  */
  YYSYMBOL_511_92 = 511,                   /* $@92  */
  YYSYMBOL_512_93 = 512,                   /* $@93  */
  YYSYMBOL_513_94 = 513,                   /* $@94  */
  YYSYMBOL_514_95 = 514,                   /* $@95  */
  YYSYMBOL_bitfield_alias_declaration = 515, /* bitfield_alias_declaration  */
  YYSYMBOL_516_96 = 516,                   /* $@96  */
  YYSYMBOL_517_97 = 517,                   /* $@97  */
  YYSYMBOL_518_98 = 518,                   /* $@98  */
  YYSYMBOL_519_99 = 519,                   /* $@99  */
  YYSYMBOL_make_decl = 520,                /* make_decl  */
  YYSYMBOL_make_decl_no_bracket = 521,     /* make_decl_no_bracket  */
  YYSYMBOL_make_struct_fields = 522,       /* make_struct_fields  */
  YYSYMBOL_make_variant_dim = 523,         /* make_variant_dim  */
  YYSYMBOL_make_struct_single = 524,       /* make_struct_single  */
  YYSYMBOL_make_struct_dim_list = 525,     /* make_struct_dim_list  */
  YYSYMBOL_make_struct_dim_decl = 526,     /* make_struct_dim_decl  */
  YYSYMBOL_optional_make_struct_dim_decl = 527, /* optional_make_struct_dim_decl  */
  YYSYMBOL_use_initializer = 528,          /* use_initializer  */
  YYSYMBOL_make_struct_decl = 529,         /* make_struct_decl  */
  YYSYMBOL_530_100 = 530,                  /* $@100  */
  YYSYMBOL_531_101 = 531,                  /* $@101  */
  YYSYMBOL_532_102 = 532,                  /* $@102  */
  YYSYMBOL_533_103 = 533,                  /* $@103  */
  YYSYMBOL_534_104 = 534,                  /* $@104  */
  YYSYMBOL_535_105 = 535,                  /* $@105  */
  YYSYMBOL_536_106 = 536,                  /* $@106  */
  YYSYMBOL_537_107 = 537,                  /* $@107  */
  YYSYMBOL_538_108 = 538,                  /* $@108  */
  YYSYMBOL_539_109 = 539,                  /* $@109  */
  YYSYMBOL_make_tuple_call = 540,          /* make_tuple_call  */
  YYSYMBOL_541_110 = 541,                  /* $@110  */
  YYSYMBOL_542_111 = 542,                  /* $@111  */
  YYSYMBOL_make_dim_decl = 543,            /* make_dim_decl  */
  YYSYMBOL_544_112 = 544,                  /* $@112  */
  YYSYMBOL_545_113 = 545,                  /* $@113  */
  YYSYMBOL_546_114 = 546,                  /* $@114  */
  YYSYMBOL_547_115 = 547,                  /* $@115  */
  YYSYMBOL_548_116 = 548,                  /* $@116  */
  YYSYMBOL_549_117 = 549,                  /* $@117  */
  YYSYMBOL_550_118 = 550,                  /* $@118  */
  YYSYMBOL_551_119 = 551,                  /* $@119  */
  YYSYMBOL_552_120 = 552,                  /* $@120  */
  YYSYMBOL_553_121 = 553,                  /* $@121  */
  YYSYMBOL_expr_map_tuple_list = 554,      /* expr_map_tuple_list  */
  YYSYMBOL_push_table_nesting = 555,       /* push_table_nesting  */
  YYSYMBOL_make_table_decl = 556,          /* make_table_decl  */
  YYSYMBOL_make_table_call = 557,          /* make_table_call  */
  YYSYMBOL_array_comprehension_where = 558, /* array_comprehension_where  */
  YYSYMBOL_optional_comma = 559,           /* optional_comma  */
  YYSYMBOL_table_comprehension = 560,      /* table_comprehension  */
  YYSYMBOL_array_comprehension = 561       /* array_comprehension  */
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
#define YYLAST   11311

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  234
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  328
/* YYNRULES -- Number of rules.  */
#define YYNRULES  1016
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1799

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   461


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
       2,     2,     2,   216,     2,   233,   228,   212,   205,     2,
     226,   227,   210,   209,   199,   208,   223,   211,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   202,   230,
     206,   200,   207,   201,   229,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   224,     2,   225,   204,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   231,   203,   232,   215,     2,     2,     2,
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
     195,   196,   197,   198,   213,   214,   217,   218,   219,   220,
     221,   222
};

#if DAS2_YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   594,   594,   595,   600,   601,   602,   603,   604,   605,
     606,   607,   608,   609,   610,   611,   612,   616,   617,   621,
     622,   626,   632,   633,   634,   638,   639,   643,   644,   648,
     667,   668,   669,   670,   674,   675,   679,   680,   684,   685,
     685,   689,   694,   703,   718,   734,   739,   747,   747,   786,
     804,   808,   811,   815,   819,   823,   827,   833,   842,   843,
     847,   850,   856,   857,   861,   865,   866,   870,   873,   879,
     885,   888,   894,   895,   899,   900,   904,   905,   909,   910,
     910,   914,   914,   923,   924,   928,   929,   935,   936,   937,
     938,   939,   943,   944,   948,   949,   953,   955,   953,   967,
     967,   975,   977,   975,   989,   989,   997,   999,   997,  1010,
    1017,  1024,  1029,  1038,  1046,  1052,  1056,  1064,  1074,  1074,
    1083,  1091,  1091,  1107,  1113,  1120,  1139,  1143,  1150,  1151,
    1152,  1153,  1154,  1155,  1159,  1164,  1172,  1173,  1174,  1175,
    1179,  1180,  1181,  1182,  1183,  1184,  1185,  1186,  1187,  1193,
    1196,  1202,  1205,  1211,  1214,  1217,  1223,  1224,  1225,  1226,
    1230,  1248,  1271,  1274,  1284,  1299,  1314,  1329,  1332,  1339,
    1343,  1350,  1351,  1355,  1356,  1360,  1361,  1362,  1366,  1369,
    1373,  1380,  1384,  1385,  1386,  1387,  1388,  1389,  1390,  1391,
    1392,  1393,  1394,  1395,  1396,  1397,  1398,  1399,  1400,  1401,
    1402,  1403,  1404,  1405,  1406,  1407,  1408,  1409,  1410,  1411,
    1412,  1413,  1414,  1415,  1416,  1417,  1418,  1419,  1420,  1421,
    1422,  1423,  1424,  1425,  1426,  1427,  1428,  1429,  1430,  1431,
    1432,  1433,  1434,  1435,  1436,  1437,  1438,  1439,  1440,  1441,
    1442,  1443,  1444,  1445,  1446,  1447,  1448,  1449,  1450,  1451,
    1452,  1453,  1454,  1455,  1456,  1457,  1458,  1459,  1460,  1461,
    1462,  1463,  1464,  1465,  1466,  1467,  1468,  1469,  1470,  1471,
    1475,  1476,  1477,  1478,  1479,  1480,  1481,  1482,  1483,  1484,
    1485,  1486,  1487,  1488,  1489,  1490,  1491,  1492,  1493,  1494,
    1495,  1496,  1497,  1498,  1499,  1500,  1501,  1502,  1503,  1504,
    1505,  1506,  1507,  1508,  1509,  1510,  1511,  1512,  1513,  1514,
    1515,  1516,  1517,  1518,  1519,  1520,  1521,  1522,  1526,  1527,
    1531,  1550,  1551,  1552,  1556,  1562,  1562,  1579,  1582,  1584,
    1582,  1596,  1598,  1596,  1613,  1631,  1649,  1667,  1678,  1679,
    1680,  1681,  1682,  1683,  1684,  1685,  1686,  1687,  1688,  1689,
    1690,  1691,  1692,  1693,  1694,  1695,  1696,  1697,  1699,  1697,
    1714,  1719,  1725,  1731,  1732,  1736,  1737,  1741,  1745,  1752,
    1753,  1764,  1768,  1771,  1779,  1779,  1779,  1782,  1788,  1791,
    1795,  1799,  1806,  1813,  1819,  1823,  1827,  1830,  1833,  1841,
    1844,  1852,  1858,  1859,  1860,  1864,  1865,  1869,  1870,  1874,
    1879,  1887,  1894,  1907,  1910,  1913,  1923,  1923,  1923,  1926,
    1926,  1926,  1931,  1931,  1931,  1939,  1939,  1939,  1945,  1955,
    1966,  1981,  1984,  1987,  1990,  1996,  1997,  2004,  2015,  2016,
    2017,  2021,  2022,  2023,  2024,  2025,  2029,  2034,  2042,  2043,
    2047,  2054,  2058,  2065,  2066,  2067,  2068,  2069,  2070,  2071,
    2072,  2076,  2077,  2078,  2079,  2080,  2081,  2082,  2083,  2084,
    2085,  2086,  2087,  2088,  2089,  2090,  2091,  2092,  2093,  2094,
    2095,  2096,  2100,  2106,  2113,  2125,  2131,  2139,  2147,  2158,
    2170,  2174,  2181,  2184,  2184,  2184,  2189,  2189,  2189,  2202,
    2206,  2210,  2216,  2224,  2232,  2243,  2252,  2258,  2266,  2266,
    2266,  2273,  2277,  2286,  2294,  2302,  2306,  2309,  2317,  2318,
    2319,  2326,  2327,  2328,  2329,  2330,  2331,  2332,  2333,  2334,
    2335,  2336,  2337,  2338,  2339,  2340,  2341,  2342,  2343,  2344,
    2345,  2346,  2347,  2348,  2349,  2350,  2351,  2352,  2353,  2354,
    2355,  2356,  2357,  2358,  2359,  2360,  2361,  2367,  2368,  2369,
    2370,  2371,  2386,  2395,  2396,  2397,  2398,  2399,  2400,  2401,
    2402,  2403,  2404,  2405,  2406,  2406,  2406,  2414,  2415,  2416,
    2419,  2419,  2419,  2422,  2427,  2431,  2435,  2435,  2435,  2440,
    2443,  2447,  2447,  2447,  2452,  2455,  2456,  2457,  2458,  2459,
    2460,  2461,  2462,  2463,  2465,  2469,  2470,  2475,  2481,  2487,
    2496,  2499,  2502,  2511,  2512,  2513,  2514,  2515,  2516,  2517,
    2521,  2525,  2529,  2533,  2537,  2541,  2545,  2549,  2553,  2560,
    2561,  2565,  2566,  2567,  2571,  2572,  2576,  2577,  2578,  2582,
    2583,  2587,  2599,  2602,  2603,  2607,  2607,  2626,  2625,  2640,
    2639,  2656,  2668,  2677,  2687,  2688,  2689,  2690,  2691,  2695,
    2698,  2707,  2708,  2712,  2715,  2719,  2732,  2741,  2742,  2746,
    2749,  2753,  2766,  2767,  2771,  2777,  2783,  2792,  2795,  2802,
    2805,  2811,  2812,  2813,  2817,  2818,  2822,  2829,  2834,  2843,
    2849,  2853,  2864,  2871,  2880,  2883,  2886,  2893,  2897,  2903,
    2915,  2918,  2923,  2935,  2936,  2940,  2941,  2942,  2946,  2949,
    2952,  2952,  2972,  2975,  2975,  2993,  2998,  3006,  3007,  3011,
    3014,  3027,  3044,  3045,  3046,  3051,  3051,  3077,  3078,  3085,
    3098,  3099,  3100,  3104,  3114,  3117,  3123,  3124,  3128,  3129,
    3133,  3134,  3138,  3140,  3145,  3138,  3161,  3162,  3166,  3167,
    3171,  3177,  3178,  3179,  3180,  3184,  3185,  3186,  3190,  3193,
    3199,  3201,  3206,  3199,  3227,  3234,  3239,  3248,  3254,  3258,
    3269,  3270,  3271,  3272,  3273,  3274,  3275,  3276,  3277,  3278,
    3279,  3280,  3281,  3282,  3283,  3284,  3285,  3286,  3287,  3288,
    3289,  3290,  3291,  3292,  3293,  3294,  3295,  3296,  3297,  3298,
    3299,  3300,  3301,  3302,  3303,  3304,  3305,  3306,  3307,  3308,
    3309,  3310,  3311,  3312,  3313,  3314,  3315,  3316,  3317,  3318,
    3322,  3323,  3324,  3325,  3326,  3327,  3328,  3329,  3333,  3344,
    3348,  3355,  3367,  3374,  3380,  3389,  3394,  3404,  3414,  3424,
    3437,  3438,  3439,  3440,  3441,  3445,  3449,  3449,  3449,  3463,
    3464,  3468,  3472,  3479,  3482,  3485,  3488,  3494,  3497,  3511,
    3512,  3516,  3517,  3518,  3519,  3520,  3520,  3520,  3524,  3529,
    3536,  3543,  3543,  3550,  3550,  3557,  3561,  3565,  3570,  3575,
    3580,  3585,  3589,  3593,  3598,  3602,  3606,  3611,  3611,  3611,
    3617,  3624,  3624,  3624,  3629,  3629,  3629,  3635,  3635,  3635,
    3640,  3645,  3645,  3645,  3650,  3650,  3650,  3659,  3664,  3664,
    3664,  3669,  3669,  3669,  3678,  3683,  3683,  3683,  3688,  3688,
    3688,  3697,  3697,  3697,  3703,  3703,  3703,  3712,  3715,  3726,
    3742,  3744,  3749,  3754,  3742,  3780,  3782,  3787,  3793,  3780,
    3819,  3821,  3826,  3831,  3819,  3872,  3873,  3874,  3875,  3876,
    3877,  3878,  3882,  3883,  3884,  3885,  3886,  3890,  3897,  3904,
    3910,  3916,  3923,  3930,  3936,  3945,  3948,  3954,  3962,  3967,
    3974,  3979,  3985,  3986,  3990,  3991,  3995,  3995,  3995,  4003,
    4003,  4003,  4010,  4010,  4010,  4021,  4021,  4021,  4028,  4028,
    4028,  4039,  4045,  4045,  4045,  4059,  4078,  4078,  4078,  4088,
    4088,  4088,  4102,  4102,  4102,  4116,  4125,  4125,  4125,  4145,
    4152,  4152,  4152,  4162,  4165,  4176,  4182,  4205,  4213,  4233,
    4258,  4259,  4263,  4264,  4269,  4272,  4282
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
  "\"|>\"", "\":=\"", "\"<<<\"", "\">>>\"", "\"<<<=\"", "\">>>=\"",
  "\"=>\"", "\"@@\"", "\"@field\"", "\"::\"", "\"&&\"", "\"||\"", "\"^^\"",
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
  "$@42", "$@43", "$@44", "$@45", "expr_generator", "expr_mtag_no_bracket",
  "optional_field_annotation", "optional_override", "optional_constant",
  "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "struct_variable_declaration_list", "$@46", "$@47", "$@48",
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
  "global_variable_declaration_list", "$@49", "global_let", "$@50",
  "enum_expression", "commas", "enum_list",
  "optional_public_or_private_alias", "single_alias", "$@51",
  "alias_declaration", "distinct_alias", "optional_public_or_private_enum",
  "enum_name", "optional_enum_basic_type_declaration", "optional_commas",
  "emit_commas", "optional_emit_commas", "enum_declaration", "$@52",
  "$@53", "$@54", "optional_structure_parent", "optional_sealed",
  "structure_name", "class_or_struct",
  "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@55", "$@56", "$@57", "variable_name_with_pos_list",
  "basic_type_declaration", "enum_basic_type_declaration",
  "structure_type_declaration", "auto_type_declaration", "bitfield_bits",
  "bitfield_alias_bits", "bitfield_basic_type_declaration",
  "bitfield_type_declaration", "$@58", "$@59", "c_or_s", "table_type_pair",
  "dim_list", "type_declaration_no_options",
  "optional_expr_list_in_braces", "type_declaration_no_options_no_dim",
  "$@60", "$@61", "$@62", "$@63", "$@64", "$@65", "$@66", "$@67", "$@68",
  "$@69", "$@70", "$@71", "$@72", "$@73", "$@74", "$@75", "$@76", "$@77",
  "$@78", "$@79", "$@80", "$@81", "$@82", "$@83", "$@84", "$@85", "$@86",
  "$@87", "type_declaration", "tuple_alias_declaration", "$@88", "$@89",
  "$@90", "$@91", "variant_alias_declaration", "$@92", "$@93", "$@94",
  "$@95", "bitfield_alias_declaration", "$@96", "$@97", "$@98", "$@99",
  "make_decl", "make_decl_no_bracket", "make_struct_fields",
  "make_variant_dim", "make_struct_single", "make_struct_dim_list",
  "make_struct_dim_decl", "optional_make_struct_dim_decl",
  "use_initializer", "make_struct_decl", "$@100", "$@101", "$@102",
  "$@103", "$@104", "$@105", "$@106", "$@107", "$@108", "$@109",
  "make_tuple_call", "$@110", "$@111", "make_dim_decl", "$@112", "$@113",
  "$@114", "$@115", "$@116", "$@117", "$@118", "$@119", "$@120", "$@121",
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

#define YYPACT_NINF (-1642)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-909)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1642,    70, -1642, -1642,    77,   -65,   -62,   293, -1642,   -86,
   -1642, -1642, -1642, -1642,   363,   599, -1642, -1642, -1642, -1642,
      12,    12,    12, -1642,   302, -1642,   157, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,    37, -1642,
      94,   152,   128, -1642,   265,   265, -1642, -1642, -1642,   188,
      12,    12, -1642, -1642,   157,   293,   293,   293,   228,   246,
   -1642, -1642, -1642, -1642,   599,   599,   599,   233, -1642,   810,
     114, -1642, -1642, -1642, -1642,   412, -1642,   503, -1642,   583,
      47,    77,   440,   -65,   407, -1642,   262,   446,   112,    23,
     487, -1642, -1642,   652,   492,   494,   527, -1642,   552,   475,
   -1642, -1642,   -79,    77,   599,   599,   599,   599,   533, -1642,
     686,   710,   644,   655,   724, -1642, -1642,   567, -1642, -1642,
     618, -1642, -1642, -1642,   789,   125, -1642, -1642, -1642, -1642,
     265,   265,   554,   265,   627,   634,   637, -1642, -1642,   628,
     633, -1642, -1642,   615,   640,   533,   533, -1642, -1642,   646,
   -1642,   113, -1642,   221,   679,   810, -1642,   656, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642,   660, -1642, -1642, -1642, -1642,
   -1642, -1642,   712, -1642, -1642, -1642, -1642,   815, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642,   170,   554,   554,   554, -1642,
   -1642,   826,  9993,  9993,   822, -1642,   588,   713, -1642, -1642,
   -1642, -1642, -1642, 10903, -1642,   704,   780,   181,    77,   751,
     736, -1642, -1642, -1642,   125, -1642, -1642,   733,   741,   748,
     691,   758,   760, -1642, -1642, -1642,   707, -1642, -1642, -1642,
   -1642, -1642,     2, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642,   784, -1642, -1642, -1642,   786,   788,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642,   797,   800,   785,
     363,   206, -1642, -1642, -1642, -1642,  1288,   703,   742,   742,
   -1642, -1642, -1642, -1642, -1642, -1642,   766, -1642,   783,   790,
    1530, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,   982,
     985, -1642,   801, -1642,   533,  1036,   713, -1642,   843,   533,
   -1642, -1642,   712,   533,    77, -1642,   591, -1642, -1642, -1642,
   -1642, -1642,  8487, -1642, -1642,   844,   829,   -34,    45,   135,
   -1642, -1642,  8487,   263, -1642,  6100, -1642, -1642, -1642,     9,
   -1642, -1642, -1642,    35, -1642,  6317,   812,  9521, -1642,   807,
   -1642, -1642, 10618, 10733, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642,   850,   819, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,  1031, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642,   865,   835, -1642, -1642,   -57,    81,   -80, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642,   832,   862, -1642,
     486, -1642,   533,   886,  9993, -1642,   168,  9993,  9993,  9993,
     870,   871, -1642, -1642,   376,   363,   872,    38, -1642,   272,
     853,   876,   877,   294,   883,   867,   305,   888, -1642,   437,
      29,   891,  9349,  9349,   326,   874,   879,   880,   881,   882,
     887, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642,  9349,  9349,  9349,  9349,  9349,  3930,  4798, -1642, -1642,
   -1642, -1642, -1642, -1642,   892, -1642, -1642, -1642, -1642,   878,
   -1642, -1642,   187,   187, -1642,   187,   187,   869,  1884, -1642,
   -1642,   893, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
    9993,  9993,   885,   909,  9993,   801,  9993,   801,  9993,   801,
   10152,   932,   904, -1642,  6100, -1642,  9993,  8487,   906,   940,
   -1642, -1642, -1642, -1642, -1642,   915, -1642, -1642,   917,  6534,
   -1642,  1288, -1642, 10152,   932, -1642, -1642, -1642, -1642, -1642,
   -1642, 11122,  2106,  1057,   918, -1642,   124,   913,    96,   919,
    9993,  9993, -1642,  8919, -1642, -1642, -1642, -1642,   363, -1642,
     564,   921,  1102,   641, -1642, -1642, -1642,   890, -1642, -1642,
   -1642,  8487,   514,   647,   942,   382, -1642, -1642, -1642, -1642,
     924, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
     479, -1642,   949,   950,   955, -1642,  6100,  9993,  8487,  8487,
   -1642, -1642, -1642,  8487, -1642,  8487, -1642,  6100, -1642, -1642,
    6100,   956, -1642,  9993,   738,   738,   938,   941,   150, -1642,
   -1642,  8487,  8487,  8487,  8487,  8487,  8487,   777,   738,   738,
     -69,   738,   738,   943,  1145,   946,   947,   270,   940,   967,
     945,   533,  3496,   599,  1164, -1642, -1642,   878, -1642, -1642,
   -1642, -1642, -1642, 10564, 10679,  9349,  9349, -1642, -1642,  9349,
    9349,  9349,  9349,   986,  9349,    97,  8487,  9349,  9349,  9349,
    9349,  8487,  9349,  9349,  9349,  9349,  9134,  9349,  9349,  9349,
    9349,  9349,  9349,  9349,  9349,  9349,  9349, 10995,  8487,  5015,
     669,   697, -1642, -1642,   988,   719,    81,   721,    81,   727,
      81,    27, -1642,   402,   742,   972, -1642,   424, -1642,  9993,
     940,   532,   742, -1642, -1642,  6751, -1642, -1642, -1642, -1642,
     953,   990, -1642,    12, -1642,    12, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642,  8487, -1642, -1642,   497,   -41,   -41,
     -41, -1642,   742,   742,  9349,  9522, -1642,   992, -1642, -1642,
   -1642, -1642,  8487,   993,   538,  9993,   168, -1642,  8487,    12,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642,  9993,  9993,  9993,
    9993,  4147,   995,  8487,  9993, -1642, -1642, -1642,  9993,   940,
     711, -1642,   987,   960,  9993,  9993,  9993,   969,  9993,   970,
    9993,   940,  9993, 10152,   940, -1642,   932,   242,  8487,  8487,
    9993,   801,   973,   974,   976,   978,   983,   984, -1642,  8487,
     701,   -64,   989, -1642,  8487, -1642,  8487, -1642,  8487,   991,
     375, -1642, -1642,  6968,   140,  3713, -1642,   200,   996,   167,
     994,   801,  2108,  1164,  1008,   998, -1642, -1642,  1010,  1002,
   -1642, -1642,   136,   136,   847,   847, 10402, 10402,  1003,   650,
    1004, -1642,   997,   -25,   -25,   893,   136,   136,  9522, -1642,
   -1642, 10291,  1221, 10264,  9522, 10794,  1794,  9678, 10370,  1701,
     847,   847,   496,   496,   650,   650,   650,   400,  8487,  1005,
    1006,   485,  8487,  1198,  1007,  1011, -1642,   226, -1642, -1642,
   -1642,   144, -1642,  1028, -1642,  1030, -1642,  1032,  9993, -1642,
   10152,  9993, -1642,   932,   535,  1013,  1012,  9993,  8487, -1642,
   -1642,  1041,    11, -1642,  9832, -1642,   -77, -1642,  1019,  1021,
    1174, -1642, -1642,   258, -1642, -1642, -1642, 10141,  2340,  1050,
   -1642,    11,    33,  1024, -1642,  1026,  1210,   890,  8487,    12,
   -1642, -1642, -1642, -1642,   742,   550,   952,   728,   539,   229,
    1027,  1029,   546,  1034,   739,  9993, 10152,   932,  1001,  1038,
    1033,  9993,  8487,  1040, -1642,  1188,  1255,  1376, -1642,  1381,
   -1642,  1416,  1043,  1573,   579,  1044,  9993,   706,  1164,  1045,
    1046,  1578,    81, -1642, -1642, -1642, -1642, -1642,  1037,  1056,
    1042,  1231,  1086,    44,   -64,  1049, -1642, -1642, -1642,  1054,
     178,  1055,  1078,   987,   281, -1642,  1059,   320,  5232, -1642,
   -1642, -1642,   214,    81, -1642,  7185, -1642,  1058,  7402,  1122,
    1123, -1642,    12,  1132,  7619,    90,  7836, -1642, -1642, -1642,
      12,    12,  1307, -1642,   948, -1642, -1642,  1305, -1642, -1642,
    1310,  1281, -1642,    12, -1642,    12,    12,    12,    12,    12,
   -1642,  1260, -1642,    12,  9638,   801, -1642,  8487, -1642,  8487,
    4364,  8487, -1642,  1121,  1103, -1642, -1642,  9349,  1104, -1642,
    1106,  8487,  4581,  1108, -1642,  1107, -1642,  5449, -1642,  6751,
   -1642, -1642, -1642,  1147, -1642,  1148, -1642, -1642, -1642, -1642,
   -1642, -1642,   742, -1642, -1642,   742, -1642, -1642,  1012, -1642,
   -1642,   742, -1642,  8487, -1642,   590, -1642, -1642, -1642,  1109,
   -1642,  1110, -1642,  8487,  1149,   577,  9993, -1642,  8487,  1113,
    8487,   631, -1642,  1151, -1642, -1642,  1338,   712, -1642,  8487,
    1158, -1642,  8487,    12, -1642, -1642, -1642, -1642,  1124, -1642,
   -1642, -1642,  1126,  1166, -1642, -1642,  1775,   708,   730, -1642,
   -1642,  8487,  1897, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642,  3927, -1642,   129, -1642, -1642, -1642,
    1146,  5666, -1642,  1159,  8487,  1168, -1642,   240,  6100,    24,
      54,   292,  8487,  8487,  8487,   -64, -1642, -1642, -1642,   375,
    1138,  3713,   256,  1170,  1177,  1154,  1182,  1192, -1642,   283,
     533,  8487, -1642,  1363,  8487, -1642,  1183,  1190, -1642,  1189,
    1211, -1642,  1058,  8487, -1642, -1642, -1642, -1642,  1171, -1642,
   -1642,  1172,  -103,  -103,  1175, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642,   -60, -1642,  9349,  9349,  9349,  9349,  9349,  9349,
    9349,  9349,  9349,  9349,  8487,  9349,  9349,  9349,  9349,  9349,
    9349,  9349,    81,  9993,  1167,  9993,  1173,  3713, -1642,   287,
     298,  1176, -1642,  8487, 10141,  8487, -1642,  1178,  3713, -1642,
     306,   314,  8487, -1642, -1642, -1642,   322, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642,  1204, -1642,  1179, -1642, -1642,
    1180, -1642,  1184,  1369,   122, -1642,  1378, -1642, -1642,  1181,
    1216,   755,  1350,    12,  1191, -1642,    12, -1642,  1193,  1194,
   -1642, -1642,  8487,  1215, -1642, -1642, -1642, -1642,  1197,  1213,
    1212,  1217,  1220,  1222,  1224,  1225,  1377,  1227, -1642,  1229,
    8053,  1058,   398, -1642, -1642,   340, -1642, -1642,  1230, -1642,
    1269, -1642,   347,  1233,  1422,  1086,  6100,  8487,  8487,  1239,
   -1642, -1642,    55, -1642,   361, -1642, -1642, -1642,  1278, -1642,
   -1642,   214, -1642,   -80, -1642,  1058, -1642,  9993,  8487, -1642,
   -1642, -1642, -1642,  2572,  8487,  8487,    77,   751,  1242,  1243,
    8270,  1086, -1642, -1642, -1642,  1884,  1884,  1884,  1884,  1884,
    1884,  1884,  1884,  1884,  1884,  1884, -1642, -1642,  1884,  1884,
    1884,  1884,  1884,  1884,  1884,   533,  4144, -1642,   745, -1642,
     404,  5883, -1642, -1642, -1642,  9993,  1244,  1246, -1642,   417,
    5883, -1642, -1642,  1247, -1642,  8487, -1642, -1642,  8487,  1289,
    8487, -1642, -1642, -1642,  9993, -1642, -1642,   665, -1642,    32,
   -1642, -1642, -1642,  1377,  1377,  1252,  1254,  1257,  1258,  1259,
    6100, -1642,  8487,  8487,  8487,  8487,  8487,  6100, -1642, -1642,
    1377,  1261,  1377, -1642,  1264, -1642, -1642,   398, -1642,  1293,
   -1642, -1642,  1251,  8487,  1303,   357,   369, -1642, -1642,   312,
    6100,  1267,  1270, -1642, -1642, -1642,   742, -1642,  1266,  1272,
    1273,   436,   -64,  8487,   265,  1275,   377,   -14,   -80, -1642,
   -1642,  1277,   379,   750, -1642, -1642,  1279,   409, -1642, -1642,
    1282, -1642, -1642,  1280,   -96,  1475,    32, -1642, -1642,   755,
      36,    36, -1642,  8487,  1377,  1377,   539,  1283,  1285,  1286,
    1287,  1291,  1292,   940,    36,  1377,   539, -1642, -1642, -1642,
    8487,  1297, -1642, -1642,  1290,  8487,  8487,   414, -1642, -1642,
    1378,  1503,   533, -1642,    59,  1299,   541,   533,   389, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642,  1475,   497,   539,  1317,  1330,
   -1642,  1304,  1306,  1308,    36,    36,  1317,  1309, -1642, -1642,
   -1642, -1642, -1642, -1642,  1311,  1312,   539,  1313,  1300, -1642,
    8487, -1642, -1642,  1314, -1642,  8704,    12, -1642,  6100,   533,
     533,  1058,  9993,   168, -1642,  2804, 10903, -1642, -1642, -1642,
   -1642,   426,  1315, -1642, -1642, -1642, -1642,  1316,  1318, -1642,
   -1642, -1642,  1319, -1642,  1484,  1325,  1300,  8487, -1642, -1642,
   -1642, -1642, -1642,  1884, -1642,  1322,   429,  1058,  1058, -1642,
     419,  8487,  1323,    12, 10903, -1642,   539, -1642, -1642, -1642,
    8487, -1642,  1329,  1300, -1642,   723,  8704,   533, -1642, -1642,
    8487,    12, -1642, -1642,   533,   431, -1642, -1642,  1324, -1642,
     533, -1642, -1642,  1331, -1642,    12,  1058,    12, -1642,   -80,
   -1642, -1642,  3036, -1642,  8487, -1642, -1642, -1642, -1642,  1326,
    1334,  1333,  1378, -1642, -1642,  8704,   533, -1642, -1642,    12,
   -1642,  3268, -1642,  1334,  1337,   723,  1378, -1642, -1642
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   173,     1,   395,     0,     0,    58,   712,   396,     0,
     930,   920,   925,    20,     0,     0,    19,    16,    15,     3,
       0,     0,     0,     8,   750,     7,   693,     6,    11,     5,
       4,    13,    12,    14,   137,   138,   139,   136,   147,   149,
      49,    67,    64,    65,     0,     0,    50,   714,   713,     0,
       0,     0,    26,    25,   693,   712,   712,   712,     0,   369,
      47,   157,   158,   159,     0,     0,     0,   160,   162,   169,
       0,   156,    21,    10,     9,   318,   732,     0,   694,   695,
       0,     0,     0,     0,     0,    51,     0,     0,    59,    62,
     715,   717,   718,    22,     0,     0,     0,   371,     0,     0,
     168,   163,     0,     0,     0,     0,     0,     0,    76,   319,
     321,   720,   742,   741,   745,   697,   696,   703,   145,   146,
       0,   143,   144,   141,     0,     0,   140,   150,    68,    66,
       0,     0,    52,     0,     0,     0,     0,    63,    60,     0,
       0,    23,    24,    27,   830,    76,    76,   370,    45,    48,
     167,     0,   164,   165,   166,   170,    74,    77,   174,   323,
     322,   325,   320,   722,   721,     0,   744,   743,   747,   746,
     751,   698,   619,   142,    30,    31,    35,     0,   132,   133,
     130,   131,   129,   128,   134,     0,    54,    55,    53,    57,
      56,    62,     0,     0,     0,    29,     0,   730,   921,   926,
      46,   161,    75,     0,   723,   724,   738,   700,     0,   620,
       0,    32,    33,    34,     0,   148,    61,     0,     0,     0,
       0,     0,     0,   760,   803,   761,   819,   762,   766,   767,
     768,   769,   809,   773,   774,   775,   776,   777,   778,   779,
     804,   805,   806,   807,   890,   765,   772,   808,   897,   904,
     763,   770,   764,   771,   780,   781,   782,   783,   784,   785,
     786,   787,   788,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,   799,   800,   801,   802,     0,     0,     0,
       0,   818,   851,   854,   852,   853,   917,   847,   719,   716,
      28,   833,   834,   831,   832,   728,   731,   931,     0,     0,
       0,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   317,     0,
       0,   181,   175,   269,    76,     0,   730,   739,     0,    76,
     702,   699,   619,    76,     0,   682,   674,   704,   135,   855,
     881,   884,     0,   887,   877,     0,     0,   891,   898,   905,
     911,   914,     0,   849,   861,   363,   867,   872,   866,     0,
     880,   876,   869,     0,   871,     0,   848,     0,   729,     0,
     922,   927,   260,   261,   258,   184,   185,   187,   186,   188,
     189,   190,   191,   217,   218,   215,   216,   208,   219,   220,
     209,   206,   207,   259,   242,     0,   257,   221,   222,   223,
     224,   195,   196,   197,   192,   193,   194,   205,     0,   211,
     212,   210,   203,   204,   199,   198,   200,   201,   202,   183,
     182,   241,     0,   213,   214,   619,   178,     0,   810,   813,
     816,   817,   811,   814,   812,   815,   725,     0,   736,   752,
       0,   151,    76,     0,     0,   675,     0,     0,     0,     0,
       0,     0,   517,   518,     0,     0,     0,     0,   511,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   809,     0,
       0,     0,     0,     0,   430,     0,     0,     0,     0,     0,
       0,   608,   443,   445,   444,   446,   447,   448,   449,   450,
      41,     0,     0,     0,     0,     0,   363,     0,   428,   429,
    1005,   515,   514,   595,   512,   588,   587,   586,   585,   171,
     591,   513,   590,   589,   559,   519,   560,     0,   508,   567,
     520,     0,   516,   942,   944,   943,   509,   946,   945,   510,
       0,     0,     0,   836,     0,   175,     0,   175,     0,   175,
       0,     0,     0,   863,     0,   860,     0,     0,     0,  1012,
     421,   874,   875,   868,   870,     0,   873,   844,     0,     0,
     919,   918,   932,   653,   659,   262,   264,   263,   265,   256,
     240,   266,   243,   225,     0,   176,   394,   644,   645,     0,
       0,     0,   324,     0,   331,   425,   326,   733,     0,   740,
       0,     0,   676,   674,   701,   152,   683,     0,   672,   673,
     671,     0,     0,     0,     0,   841,   966,   969,   374,   818,
     378,   377,   383,   935,   941,   936,   937,   938,   940,   939,
       0,   415,     0,     0,     0,   996,     0,     0,     0,     0,
     406,   409,   564,     0,   412,     0,  1000,     0,   978,   982,
       0,     0,   972,     0,   547,   548,     0,     0,   483,   480,
     482,     0,     0,     0,     0,     0,     0,     0,   524,   523,
     561,   522,   521,     0,     0,     0,     0,   369,  1012,  1012,
       0,    76,     0,     0,   438,   430,   360,   171,   337,   335,
     336,   334,   858,     0,     0,     0,     0,   549,   550,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,     0,     0,
       0,     0,   820,   835,     0,     0,   178,     0,   178,     0,
     178,   369,   651,     0,   649,     0,   657,     0,   821,     0,
    1012,     0,   367,   422,   859,  1013,   364,   865,   843,   846,
       0,   825,   654,    94,   660,    94,   267,   268,   245,   246,
     248,   247,   249,   250,   251,   252,   244,   253,   254,   255,
     229,   230,   232,   231,   233,   234,   235,   236,   227,   228,
     237,   238,   239,   226,     0,   392,   393,     0,   619,   619,
     619,   177,   180,   179,     0,   426,   360,   709,   737,   748,
     632,   753,     0,     0,     0,     0,     0,   690,     0,     0,
     856,   882,   885,    18,    17,   839,   840,     0,     0,     0,
       0,   964,     0,     0,     0,   986,   989,   992,     0,  1012,
       0,  1003,  1012,     0,     0,     0,     0,     0,     0,     0,
       0,  1012,     0,     0,  1012,   975,     0,     0,     0,     0,
       0,   175,     0,     0,     0,     0,     0,     0,    44,     0,
      42,     0,     0,   985,     0,   663,     0,   662,     0,     0,
    1013,   957,   552,   365,     0,   363,   501,     0,     0,     0,
       0,   175,     0,   438,     0,     0,   574,   573,     0,     0,
     575,   579,   525,   526,   538,   539,   536,   537,     0,   568,
       0,   557,     0,   592,   593,   594,   527,   528,   597,   598,
     599,   543,   544,   545,   546,     0,     0,   541,   542,   540,
     534,   535,   530,   529,   531,   532,   533,     0,     0,     0,
     489,     0,     0,     0,     0,     0,   506,     0,   888,   878,
     822,     0,   892,     0,   899,     0,   906,     0,     0,   912,
       0,     0,   915,     0,     0,     0,   849,     0,     0,   423,
     845,   826,   726,    92,    95,   923,    95,   928,     0,     0,
     754,   641,   642,   664,   646,   648,   647,   427,     0,   705,
     710,   726,   635,     0,   678,     0,   679,     0,     0,     0,
     692,   857,   883,   886,   842,     0,     0,     0,   965,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1013,     0,   562,     0,     0,     0,   563,     0,
     596,     0,     0,     0,     0,     0,     0,     0,   438,     0,
       0,     0,   178,   603,   604,   605,   606,   607,     0,    38,
       0,   110,     0,     0,     0,     0,   948,   947,   551,     0,
       0,     0,     0,  1012,     0,   502,     0,     0,     0,   505,
     503,   172,     0,   178,   362,   386,   384,     0,     0,     0,
       0,   385,     0,     0,     0,    76,     0,   357,   442,   338,
       0,     0,     0,   351,     0,   352,   346,     0,   343,   342,
       0,     0,   344,     0,   361,     0,    90,    91,    88,    89,
     353,   398,   341,     0,   451,   175,   570,     0,   576,     0,
       0,     0,   555,     0,     0,   580,   584,     0,     0,   558,
       0,     0,     0,     0,   490,     0,   499,     0,   553,     0,
     507,   889,   879,     0,   837,     0,   893,   895,   900,   902,
     907,   909,   650,   913,   652,   656,   916,   658,   849,   850,
     862,   368,   424,     0,   707,   727,   933,    93,   655,     0,
     661,     0,   643,     0,     0,     0,     0,   665,     0,     0,
       0,   727,   734,     0,   633,   749,     0,   619,   677,     0,
       0,   687,     0,     0,   691,   967,   970,   375,     0,   380,
     381,   379,     0,     0,   418,   416,     0,     0,     0,   997,
     995,   365,     0,  1004,  1007,   407,   410,   565,   413,  1001,
     999,   979,   983,   981,     0,   973,    76,   481,   618,   484,
       0,     0,    39,     0,     0,     0,   399,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1006,   366,   504,     0,
       0,   363,     0,     0,     0,     0,     0,     0,   436,     0,
      76,     0,   387,     0,     0,   372,     0,     0,   356,     0,
       0,    71,     0,     0,   389,   360,   354,   355,     0,    83,
      84,     0,   153,   153,     0,   345,   340,   347,   348,   349,
     350,   397,     0,   339,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   178,     0,     0,     0,     0,   363,   475,     0,
       0,     0,   581,     0,   569,     0,   556,     0,   363,   491,
       0,     0,     0,   554,   500,   496,     0,   824,   838,   823,
     896,   903,   910,   864,   827,   828,   708,     0,   924,   929,
       0,   756,     0,   757,   667,   666,   327,   706,   711,     0,
       0,   626,   629,     0,     0,   681,     0,   689,     0,     0,
     376,   382,     0,     0,   417,   987,   990,   993,     0,     0,
       0,     0,     0,     0,     0,     0,   964,     0,   976,     0,
       0,     0,     0,   487,   609,     0,    36,    43,     0,   112,
       0,   113,     0,     0,   114,     0,     0,     0,     0,     0,
     950,   949,     0,   472,     0,   474,   433,   434,     0,   432,
     431,     0,   439,     0,   388,     0,   373,     0,     0,    69,
      70,   120,   390,     0,     0,     0,     0,   155,     0,     0,
       0,     0,   684,   404,   403,   463,   464,   466,   465,   467,
     457,   458,   459,   468,   469,   453,   454,   455,   456,   470,
     471,   460,   461,   462,   452,    76,     0,   617,     0,   615,
       0,     0,   476,   479,   612,     0,     0,     0,   611,     0,
       0,   492,   495,     0,   497,     0,   934,   755,     0,     0,
       0,   328,   333,   735,     0,   627,   628,   629,   630,   621,
     636,   680,   688,   964,   964,     0,     0,     0,     0,     0,
     363,  1008,   365,     0,     0,     0,     0,     0,   965,   980,
     964,     0,   964,   600,     0,   602,   485,     0,   610,    40,
     111,   400,     0,     0,     0,     0,     0,   952,   951,     0,
       0,     0,     0,   437,   440,   391,   127,   126,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   571,
     577,     0,     0,     0,   616,   614,     0,     0,   613,   829,
       0,   759,   668,     0,     0,   624,   621,   622,   623,   626,
     963,   963,   419,     0,   964,   964,   955,     0,     0,     0,
       0,     0,     0,  1012,   963,   964,   955,   601,   488,    37,
       0,     0,   116,   117,     0,     0,     0,     0,   473,   435,
     327,    85,    76,   154,     0,     0,     0,    76,   674,   405,
     685,   686,   441,   572,   578,   477,   478,   582,   493,   494,
     758,   360,   634,   625,   637,   624,     0,     0,   960,  1012,
     962,     0,     0,     0,   963,   963,   956,     0,   998,  1009,
     408,   411,   566,   414,     0,     0,   955,     0,  1010,   115,
       0,   954,   953,     0,   359,     0,     0,   107,     0,    76,
      76,     0,     0,     0,   583,     0,     0,   639,   670,   669,
     631,     0,  1013,   961,   968,   971,   420,     0,     0,   994,
    1002,   984,     0,   974,     0,     0,  1010,     0,    86,    90,
      91,    88,    89,    87,   109,    99,     0,     0,     0,   124,
       0,     0,     0,     0,     0,   958,     0,   988,   991,   977,
       0,  1014,     0,  1010,    96,    78,     0,    76,   122,   125,
       0,     0,   330,   638,    76,     0,  1011,  1015,     0,   360,
      76,    72,    73,     0,   108,     0,     0,     0,   402,     0,
     959,  1016,     0,    79,     0,   100,   119,   401,   640,     0,
     104,     0,   327,   101,    80,     0,    76,    98,   360,     0,
      81,     0,   105,   104,     0,    78,   327,    82,   103
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1642, -1642,  -953,    -1, -1642, -1642, -1642, -1642, -1642,   875,
    1481, -1642, -1642, -1642, -1642, -1642, -1642,  1569, -1642, -1642,
   -1642,   -32, -1642, -1642,  1382, -1642, -1642,  1489, -1642, -1642,
   -1642, -1642,  -142,  -221, -1642, -1642, -1642, -1642, -1641,   794,
     795, -1642, -1642, -1642, -1642,  -208, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1043, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642,  1373, -1642, -1642,   -43,   -98,  -323,   288, -1642,
   -1642,   430,   889,   894,   578,  -540,  -730, -1642,  -356, -1642,
   -1642, -1642, -1455, -1642, -1642, -1522, -1642, -1642, -1036, -1642,
   -1642, -1642, -1642, -1642, -1642,  -819,  -379, -1196,   825,   -13,
   -1642, -1642, -1642, -1642, -1642, -1616, -1613, -1609, -1603, -1642,
   -1642,  1594, -1642, -1278, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642,  -518, -1411,   339,
     155, -1642,  -847, -1642,   318, -1642, -1642, -1642, -1642, -1345,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,   366,
     563, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642,  -162,     3,   -55,     4,    85, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642,   249,  -556,  -816, -1642,  -561,  -811, -1642,
    -969,   -58,   -52, -1642,  -613,  -608, -1642, -1642, -1642, -1272,
   -1642,  1553, -1642, -1642, -1642, -1642, -1642,   415,   597, -1642,
    1017, -1642, -1642, -1642, -1642, -1642, -1642, -1642,   602, -1642,
    1263, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642,  -170, -1642,  1131, -1642,
   -1642, -1642,  1390, -1642, -1642, -1642,  -611, -1642, -1642,  -342,
    -927, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
    -181, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,  -787, -1531,
    -666, -1642, -1642, -1478, -1335,  1141, -1642, -1642, -1642, -1642,
   -1642, -1642, -1642, -1642, -1642, -1642,  1142, -1642, -1642,  1143,
   -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642, -1642,
     977, -1642,  -476,  1152, -1626,  -681,  1153,  -468
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,   845,   846,    18,   143,    54,   195,    19,   177,
     183,  1549,  1253,  1416,   687,   531,   149,   532,    99,    21,
      22,    88,    45,    46,   138,    23,    42,    43,  1110,  1111,
    1763,   157,   158,  1764,  1780,  1793,  1301,  1686,  1112,   994,
     995,  1745,  1759,  1779,  1746,  1784,  1788,  1794,  1785,  1113,
    1114,  1725,  1115,  1073,  1116,  1117,  1118,  1119,  1120,  1121,
    1122,  1123,   184,   185,    38,    39,    40,   209,  1458,    67,
      68,    69,    70,   704,    24,   456,   612,   352,   353,   110,
      25,   161,   354,   162,   203,  1512,  1593,  1732,   615,   826,
    1199,   533,  1124,  1295,  1568,   912,   695,  1082,   771,   534,
    1125,   640,   850,  1390,   535,  1126,  1127,  1128,  1129,  1130,
     817,  1131,  1312,  1257,  1463,  1132,   536,   864,  1401,   865,
    1402,   868,  1404,   537,   854,  1394,   538,   579,   616,   539,
    1278,  1279,   911,   540,   708,   541,  1133,   542,   543,   680,
     544,   880,  1412,   881,  1547,   545,   963,  1354,   546,   580,
     548,   866,  1403,  1333,  1643,  1335,  1644,  1495,  1694,   549,
     550,   606,  1599,  1654,  1517,  1519,  1383,  1012,  1207,  1696,
    1734,   607,   608,   609,   762,   763,   783,   766,   767,   785,
     898,  1001,  1002,  1700,   631,   476,   623,   366,  1577,   624,
     367,    79,   117,   207,   362,    27,   172,  1010,  1185,  1011,
      49,    50,   140,    28,    51,   165,   205,   356,  1186,   296,
     297,    29,   111,   827,  1379,   619,   358,   359,   114,   170,
     831,    30,    77,   206,   620,  1003,   551,   466,   283,   284,
     971,   992,   197,   285,   754,  1358,   980,   634,   396,   286,
     575,   287,   477,  1021,   576,   769,   561,  1162,   478,  1022,
     479,  1023,   560,  1161,   564,  1166,   565,  1360,   566,  1168,
     567,  1361,   568,  1170,   569,  1362,   570,  1173,   571,  1176,
     764,    31,    56,   298,   593,  1189,    32,    57,   299,   594,
    1191,    33,    55,   399,   781,  1367,   642,   552,   699,  1667,
     700,  1659,  1660,  1661,  1031,   553,   848,  1388,   849,  1389,
     876,  1409,  1056,  1541,   872,  1406,   554,   873,  1407,   555,
    1035,  1527,  1036,  1528,  1037,  1529,   858,  1398,   870,  1405,
    1083,   701,   556,   557,  1715,   776,   558,   559
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      17,    60,    71,   198,   199,   151,   578,  1008,   646,   698,
     210,   288,   289,    89,   838,   836,   649,   899,   901,    72,
      73,    74,   282,   282,   847,   756,   973,   758,   975,   760,
     977,  1260,  1564,   784,  1198,  1399,   908,   782,   127,  1184,
    1464,  -173,   581,   652,  1718,   136,   596,   598,   671,    91,
      92,    71,    71,    71,   132,   591,   770,  1054,  1184,  1180,
     208,  1283,   118,   119,  1258,  1057,  1135,  1546,   583,  1719,
       2,  1539,  1720,  1203,  1426,  1560,  1721,     3,  1597,  1688,
     137,   613,  1722,   725,   726,  1677,   104,   105,   106,   985,
    1742,    71,    71,    71,    71,    13,    34,    35,   186,   187,
       4,   188,     5,    52,     6,  1765,   208,   397,  1684,  1070,
       7,  1264,   765,   621,    13,  -894,    41,  1758,  1087,   604,
       8,  1456,   208,  1662,   723,  1071,     9,   725,   726,   622,
    1719,   815,   627,  1720,    16,   604,  1675,  1721,   859,    44,
     178,   179,    53,  1722,  1789,  1712,    36,  1555,   150,   871,
      10,   614,   874,    16,   747,   748,   983,   713,   714,   621,
     987,  1598,  1072,   653,   654,   363,  1461,  1642,  -894,  1719,
     605,  1462,  1720,  -894,   816,   622,  1721,    13,  1039,   281,
     281,  1043,  1722,  1576,   907,  1030,  1707,  1708,  1600,  1601,
    1052,    98,  -894,  1055,  -901,    11,    12,  1423,   747,   748,
     470,   843,  1618,    13,   196,  1614,   361,  1616,  -830,   120,
     844,  1246,   457,  1424,   582,   696,    16,   469,  1639,    78,
    1227,   471,  1432,  1018,  1204,  1084,  1228,   282,   121,   978,
     610,   967,  1366,   122,   134,   672,   123,    80,  1086,   124,
     584,  1733,    16,  1259,   655,   787,   135,  -901,  1366,  1041,
    1425,  1363,  -901,  1259,  1259,   939,  1451,    15,  1259,   585,
    1787,    13,  1657,   940,   656,  1205,    37,   383,   586,  1664,
    1665,  -901,   895,   125,  1798,   386,   930,   717,   718,  1754,
    1676,   156,    14,   611,  -908,   723,   931,   724,   725,   726,
     727,   728,  1428,    81,    15,   819,   632,   633,   635,  -486,
      16,  1272,   387,   388,   282,  1641,   180,   282,   282,   282,
      75,   181,    81,   107,   182,   860,   665,   124,   628,   696,
     156,   472,   897,   134,   629,   397,   820,    83,   895,  1084,
     625,   877,  1250,  1029,  1263,   135,  1608,  -908,    76,   108,
     201,  1062,  -908,  1163,   742,   743,   744,   745,   746,   705,
      47,  1164,  -486,  1340,    82,  1410,    48,  -486,  1778,   747,
     748,  -908,  1218,  1280,  1273,  1351,   107,  1085,   630,   214,
    1274,  1093,    13,   987,  1165,  1545,  -486,    90,   897,   750,
     751,   480,   481,   755,   281,   757,   104,   759,   106,   389,
     282,   282,  1091,   390,   282,   772,   282,   215,   282,  1088,
     282,   487,  1267,  1275,  1212,  1510,   282,   489,   895,  1565,
      98,    16,   384,   360,  1276,   528,   529,    97,   706,  1277,
     895,  1223,  1177,   282,  1174,  1159,   896,  1089,  1159,   822,
     823,   895,   385,    84,    98,  1190,    84,   896,  1188,  1420,
     282,   282,   895,   391,   496,   497,   983,   392,  1427,  1058,
     393,    85,   386,  1160,    85,  1269,  1219,  1195,   897,   103,
    1196,   281,   895,  1197,   281,   281,   281,  1421,  1625,   573,
     897,   639,   650,   131,    86,   394,  1453,    86,   657,   387,
     388,   897,  1441,  1435,  1434,    87,  1491,   282,    87,   574,
      58,   679,   897,  1017,   100,   101,   102,  1269,   658,   676,
     662,   499,   500,   282,   677,  1500,  1025,  1026,   112,   113,
    1442,   666,   897,  1269,  1492,    59,  1038,   713,   714,  1269,
     663,  1159,  1045,  1046,  1047,  1493,  1049,    58,  1051,  1634,
    1053,   667,   678,  1501,   152,   153,   154,   155,  1061,  1159,
      58,  1502,   109,   917,   921,  1270,  1159,   281,   281,  1504,
    1490,   281,    59,   281,  1079,   281,  1420,   281,   935,   903,
    1269,  1499,    58,   281,  1080,    59,   389,  1548,  1159,   628,
     390,   676,   843,    13,  1552,   629,  1420,   964,  1269,  1148,
     281,   844,   638,   386,  1623,   397,  1561,    59,   772,  1149,
    1626,  1692,   843,    13,   475,  1332,  1624,   281,   281,   282,
     526,   844,  1485,  1269,  1638,   828,  1646,   530,  1269,   979,
     387,   388,    16,  1159,   843,    13,  1269,   983,   130,   630,
     391,   128,  1339,   844,   392,  1269,   837,   393,  1159,  1581,
    1269,   982,    16,    61,  1350,    81,  1649,   717,   718,  1356,
     115,  1683,  1586,   669,   281,   723,   116,   724,   725,   726,
     727,   728,   394,  1735,    16,  1729,  1747,   133,  1770,   621,
     281,  1633,    62,   670,  1153,   282,  1024,   291,   148,  1027,
     999,   713,   714,  1034,  1154,   622,   139,   282,   282,   282,
     282,   144,   292,   145,   282,   852,  1000,   293,   282,   294,
      71,  1748,  1749,  1226,   282,   282,   282,   389,   282,  1232,
     282,   390,   282,   282,  1582,   853,   744,   745,   746,   141,
     282,  1015,  1595,  1587,  1244,   142,   146,   397,   696,   747,
     748,   840,   843,    13,   156,   843,    13,  1016,  1084,    63,
    1776,   844,  1518,  1415,   844,  1760,   843,    13,   547,   986,
    1422,   147,  1178,   159,   386,   844,  1761,  1762,   572,   160,
    1372,   391,   134,  1222,    64,   392,   281,  1215,   393,   713,
     714,   588,    16,    58,   135,    16,  1373,   163,  1690,   843,
      13,   387,   388,   164,   166,  1146,    16,   135,   844,  1365,
     843,   168,   993,   394,   993,   167,  1242,   169,    59,   844,
     473,   717,   718,   474,   829,   830,   475,  1172,   171,   723,
    1175,   724,   725,   726,   727,   728,  1181,   173,   282,    16,
     282,   282,  1515,  1658,  1658,    65,   189,   282,  1516,  1666,
    1009,   843,   281,   190,   282,    66,   191,  1658,   192,  1666,
     844,   194,  1695,   193,   281,   281,   281,   281,  1020,   200,
     834,   281,   196,   835,   104,   281,   475,   202,  1476,   204,
     397,   281,   281,   281,   841,   281,  1477,   281,   389,   281,
     281,   709,   390,   710,   711,   282,   282,   281,   713,   714,
    1701,   282,   397,   747,   748,   208,   968,  1658,  1658,   717,
     718,   707,   707,   137,   707,   707,   282,   723,   290,  1666,
     725,   726,   727,   728,   211,   212,   843,    13,   843,    13,
     397,   843,    13,   295,   969,   844,   355,   844,  1556,   357,
     844,  1109,   391,  1245,   364,  1396,   392,   372,  1040,   393,
     843,    13,   397,   386,   397,   365,   972,   395,   974,   844,
     397,   397,  1674,   375,   976,  1217,    16,  1397,    16,   369,
    1772,    16,   397,   773,   394,   397,  1225,   370,   397,  1755,
     387,   388,  1580,   397,   371,   780,   398,  1647,  1299,  1300,
      16,   747,   748,  1292,   373,   281,   374,   281,   281,  1791,
     174,   175,   888,   889,   281,   104,   105,   106,  1703,  1457,
    1457,   281,   174,   175,   176,   386,   715,   716,   717,   718,
     377,  1486,   378,  1187,   379,  1187,   723,   839,   724,   725,
     726,   727,   728,   380,   729,   730,   381,  1109,   211,   212,
     213,   382,   387,   388,   400,  1374,  1211,   453,  1214,  1613,
     454,   401,   281,   281,   861,   863,   282,   455,   281,   867,
    1693,   869,   468,   562,   386,   563,   589,   389,   592,   599,
     628,   390,  1627,   281,   600,  1382,   629,   882,   883,   884,
     885,   886,   887,   601,   602,   742,   743,   744,   745,   746,
     603,   387,   388,   617,   618,   674,   675,  1004,  1005,  1006,
     747,   748,    94,    95,    96,   626,   636,   637,   651,   659,
    1731,    13,   660,   661,   688,   689,   690,   691,   692,   664,
     630,   391,   932,   665,   668,   392,   712,   673,   393,   389,
     681,  1288,   703,   390,  1411,   682,   683,   684,   685,  1296,
    1297,   458,   752,   686,   965,   459,   753,  1750,   702,   749,
      16,   765,  1305,   394,  1306,  1307,  1308,  1309,  1310,   460,
     461,   768,  1313,   774,   462,   463,   464,   465,  1443,   775,
     777,   989,   778,   818,   814,   833,   821,   832,   389,   842,
     851,  1607,   390,   391,  1488,   855,   856,   392,   892,  1216,
     393,   857,   875,   282,   878,   282,   900,   879,   910,   891,
    1726,   893,   902,   894,   981,   928,   825,   970,   990,   991,
     998,  1009,  1014,   281,  1032,   394,  1042,  1044,   800,   801,
     802,   803,   804,   805,   806,   807,  1048,  1050,  1013,  1156,
    1063,  1064,   391,  1065,  1019,  1066,   392,   808,  1229,   393,
    1067,  1068,  1387,   809,  1136,  1074,  1138,  1194,  1078,  1033,
    1092,   386,  1142,  1090,  1137,   810,   811,   812,  1139,  1140,
    1141,  1151,  1152,  1157,   394,  1167,  1158,  1169,   574,  1171,
    1179,  1183,   713,   714,  1059,  1060,  1192,  1193,   387,   388,
    1200,  1208,  1209,  1210,  1220,  1069,  1221,   813,  1252,  1231,
    1075,  1224,  1076,  1251,  1077,  1230,  1566,  1234,  1254,   861,
    1240,  1243,  1247,  1248,  1255,  1256,  1261,   282,   922,   923,
    1262,  1265,   924,   925,   926,   927,  1268,   929,   386,   614,
     933,   934,   936,   937,   938,   941,   942,   943,   944,   946,
     947,   948,   949,   950,   951,   952,   953,   954,   955,   956,
    1266,  1286,  1287,  1289,  1583,   387,   388,  1298,  1302,  1303,
     281,   386,   281,  1304,  1150,   282,  1311,  1342,  1155,  1343,
    1345,  1346,  1353,  1594,  1352,   389,  1357,  1359,  1371,   390,
    1380,  1368,  1369,  1578,   282,  1376,  1381,  1385,   387,   388,
    1730,  1391,  1392,  1413,  1182,  1393,  1417,  1419,  1571,  1436,
     715,   716,   717,   718,   719,  1433,  1437,   720,   721,   722,
     723,  1439,   724,   725,   726,   727,   728,  1445,   729,   730,
    1438,  1440,  1520,  1447,  1213,  1522,   732,  1007,   734,   391,
    1448,  1449,  1450,   392,  1487,  1235,   393,  1454,  1455,   679,
    1489,  1460,   389,  1494,  1505,  1498,   390,  1507,  1233,   386,
    1508,  1506,  1509,  1513,   386,  1511,  1514,  1518,  1521,  1523,
    1524,   394,  1526,  1530,   737,   738,   739,   740,   741,   742,
     743,   744,   745,   746,   281,   389,   387,   388,  1532,   390,
    1531,   387,   388,  1533,   747,   748,  1534,  1538,  1535,   386,
    1536,  1537,  1109,  1540,   989,  1542,   391,  1550,  1551,  1553,
     392,  1282,  1236,   393,  1285,  1554,  1559,  1562,  1572,  1573,
    1291,  1584,  1294,  1585,  1588,  1134,   387,   388,  1591,  1602,
    1603,  1620,   281,  1604,  1605,  1606,  1619,  1615,   394,   391,
    1687,  1617,  1622,   392,  1628,  1691,   393,  1629,  1630,  1631,
    1632,   281,  1637,  1334,  1645,  1336,  1648,  1341,  1653,  1650,
    1668,  1651,  1669,  1670,  1671,  1685,  1269,  1347,  1672,  1673,
    1680,   394,   282,   389,  1679,   989,  1689,   390,   389,  1702,
    1714,  1704,   390,  1705,   679,  1706,  1709,  1740,  1710,  1711,
    1713,  1736,  1636,  1737,  1717,  1738,  1739,  1727,  1728,  1364,
    1741,   402,   403,  1744,  1757,  1752,  1771,  1774,  1782,  1370,
    1786,   126,   890,   389,  1375,  1783,  1377,   390,   404,  1796,
      20,  1134,   129,   216,  1797,  1384,  1640,   391,  1386,   996,
     997,   392,   391,  1237,   393,  1795,   392,   368,  1238,   393,
    1206,  1459,   909,  1652,   984,    26,  1563,   861,  1698,  1655,
    1697,   913,  1596,  1656,  1699,  1766,   386,    93,  1201,   394,
       0,   386,  1769,  1202,   394,   641,  1378,   391,  1773,   467,
    1418,   392,   376,  1239,   393,   643,   644,   645,  1429,  1430,
    1431,     0,     0,   387,   388,   862,   647,   648,   387,   388,
       0,     0,     0,     0,  1790,     0,     0,  1444,     0,   394,
    1446,     0,     0,     0,     0,     0,     0,     0,     0,  1452,
       0,   405,   406,   407,   408,   409,   410,   411,   412,   413,
     414,   415,   416,   417,   418,   419,   420,   421,   422,   281,
       0,   423,   424,   425,     0,  1724,   426,   427,   428,   429,
     430,     0,     0,     0,  1109,   431,   432,   433,   434,   435,
     436,   437,     0,     0,     0,     0,     0,     0,     0,  1496,
    1344,  1497,     0,     0,     0,     0,     0,     0,  1503,     0,
     389,     0,   713,   714,   390,   389,     0,     0,     0,   390,
       0,   438,  1753,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,     0,     0,   449,   450,     0,     0,     0,
    1768,     0,     0,   451,   452,     0,     0,     0,  1525,     0,
       0,     0,     0,     0,  1775,     0,  1777,     0,     0,     0,
       0,  1109,     0,     0,   391,     0,  1544,     0,   392,   391,
    1241,   393,     0,   392,     0,  1249,   393,     0,  1792,     0,
    1109,     0,     0,  1557,  1558,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   394,     0,   386,     0,
       0,   394,     0,     0,  1567,   713,   714,     0,     0,     0,
    1569,  1570,     0,     0,     0,     0,  1575,     0,     0,     0,
       0,     0,     0,     0,     0,   387,   388,     0,     0,     0,
     715,   716,   717,   718,   719,     0,     0,   720,   721,   722,
     723,     0,   724,   725,   726,   727,   728,   989,   729,   730,
       0,     0,     0,     0,     0,     0,   989,     0,     0,     0,
       0,  1589,     0,     0,  1590,     0,  1592,  1465,  1466,  1467,
    1468,  1469,  1470,  1471,  1472,  1473,  1474,  1475,  1478,  1479,
    1480,  1481,  1482,  1483,  1484,     0,     0,     0,   861,  1609,
    1610,  1611,  1612,     0,     0,   713,   714,   740,   741,   742,
     743,   744,   745,   746,     0,     0,     0,     0,     0,  1621,
       0,     0,   389,     0,   747,   748,   390,     0,     0,     0,
     386,     0,     0,   715,   716,   717,   718,   719,     0,  1635,
     720,   721,   722,   723,     0,   724,   725,   726,   727,   728,
       0,   729,   730,     0,     0,   731,     0,   387,   388,   732,
     733,   734,     0,     0,     0,   735,     0,     0,     0,  1663,
       0,     0,     0,     0,     0,     0,   391,     0,     0,     0,
     392,     0,  1395,   393,     0,     0,  1678,     0,     0,     0,
       0,  1681,  1682,     0,     0,   736,  1147,   737,   738,   739,
     740,   741,   742,   743,   744,   745,   746,     0,   394,     0,
       0,     0,     0,     0,     0,     0,  1134,   747,   748,     0,
       0,     0,     0,   715,   716,   717,   718,   719,     0,     0,
     720,   721,   722,   723,     0,   724,   725,   726,   727,   728,
       0,   729,   730,     0,   389,   731,  1716,     0,   390,   732,
     733,   734,     0,     0,     0,   735,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1743,     0,   736,     0,   737,   738,   739,
     740,   741,   742,   743,   744,   745,   746,  1751,   391,     0,
       0,     0,   392,     0,  1400,   393,  1756,   747,   748,  1094,
       0,     0,     0,   480,   481,     3,  1767,  -121,  -106,  -106,
       0,  -118,     0,   482,   483,   484,   485,   486,     0,     0,
     394,     0,     0,   487,  1095,   488,  1096,  1097,     0,   489,
    1781,     0,     0,     0,     0,     0,  1098,   490,  1099,     0,
    -123,     0,  1100,   491,     0,     0,   492,     0,     8,   493,
    1101,     0,  1102,   494,     0,     0,  1103,  1104,     0,     0,
       0,     0,     0,  1105,     0,     0,   496,   497,     0,   223,
     224,   225,     0,   227,   228,   229,   230,   231,   498,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   243,
       0,   245,   246,   247,     0,     0,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   499,   500,   501,  1106,   788,   789,   790,
     791,   792,   793,   794,   795,     0,     0,     0,  1723,   502,
     503,     0,     0,     0,     0,     0,     0,     0,  1134,     0,
       0,     0,   796,     0,     0,     0,     0,     0,     0,     0,
     504,     0,    58,     0,   797,   798,   799,     0,     0,     0,
     505,   506,   507,   508,   509,     0,   510,     0,   511,   512,
     513,   514,   515,   516,   517,   518,   519,    59,     0,    13,
     520,     0,     0,     0,     0,     0,     0,     0,     0,  1723,
       0,     0,     0,     0,     0,     0,   521,   522,   523,     0,
      14,     0,     0,   524,   525,     0,     0,     0,     0,     0,
       0,     0,   526,     0,   527,  1134,   528,   529,    16,  1107,
    1108,  1094,     0,     0,     0,   480,   481,     3,  1723,  -121,
    -106,  -106,     0,  -118,  1134,   482,   483,   484,   485,   486,
       0,     0,     0,     0,     0,   487,  1095,   488,  1096,  1097,
       0,   489,     0,     0,     0,     0,     0,     0,  1098,   490,
    1099,     0,  -123,     0,  1100,   491,     0,     0,   492,     0,
       8,   493,  1101,     0,  1102,   494,     0,     0,  1103,  1104,
       0,     0,     0,     0,     0,  1105,     0,     0,   496,   497,
       0,   223,   224,   225,     0,   227,   228,   229,   230,   231,
     498,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,     0,   245,   246,   247,     0,     0,   250,   251,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   499,   500,   501,  1106,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,   503,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   504,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   505,   506,   507,   508,   509,     0,   510,     0,
     511,   512,   513,   514,   515,   516,   517,   518,   519,    59,
       0,    13,   520,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   521,   522,
     523,     0,    14,     0,     0,   524,   525,     0,     0,     0,
       0,     0,     0,     0,   526,     0,   527,     0,   528,   529,
      16,  1107,  -332,  1094,     0,     0,     0,   480,   481,     3,
       0,  -121,  -106,  -106,     0,  -118,     0,   482,   483,   484,
     485,   486,     0,     0,     0,     0,     0,   487,  1095,   488,
    1096,  1097,     0,   489,     0,     0,     0,     0,     0,     0,
    1098,   490,  1099,     0,  -123,     0,  1100,   491,     0,     0,
     492,     0,     8,   493,  1101,     0,  1102,   494,     0,     0,
    1103,  1104,     0,     0,     0,     0,     0,  1105,     0,     0,
     496,   497,     0,   223,   224,   225,     0,   227,   228,   229,
     230,   231,   498,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,     0,   245,   246,   247,     0,     0,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   499,   500,   501,
    1106,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,   503,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   504,     0,    58,     0,     0,     0,
       0,     0,     0,     0,   505,   506,   507,   508,   509,     0,
     510,     0,   511,   512,   513,   514,   515,   516,   517,   518,
     519,    59,     0,    13,   520,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     521,   522,   523,     0,    14,     0,     0,   524,   525,     0,
       0,     0,     0,     0,     0,     0,   526,     0,   527,     0,
     528,   529,    16,  1107,  -358,  1094,     0,     0,     0,   480,
     481,     3,     0,  -121,  -106,  -106,     0,  -118,     0,   482,
     483,   484,   485,   486,     0,     0,     0,     0,     0,   487,
    1095,   488,  1096,  1097,     0,   489,     0,     0,     0,     0,
       0,     0,  1098,   490,  1099,     0,  -123,     0,  1100,   491,
       0,     0,   492,     0,     8,   493,  1101,     0,  1102,   494,
       0,     0,  1103,  1104,     0,     0,     0,     0,     0,  1105,
       0,     0,   496,   497,     0,   223,   224,   225,     0,   227,
     228,   229,   230,   231,   498,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,     0,   245,   246,   247,
       0,     0,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   499,
     500,   501,  1106,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   504,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   505,   506,   507,   508,
     509,     0,   510,     0,   511,   512,   513,   514,   515,   516,
     517,   518,   519,    59,     0,    13,   520,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   521,   522,   523,     0,    14,     0,     0,   524,
     525,     0,     0,     0,     0,     0,     0,     0,   526,     0,
     527,     0,   528,   529,    16,  1107,  -329,  1094,     0,     0,
       0,   480,   481,     3,     0,  -121,  -106,  -106,     0,  -118,
       0,   482,   483,   484,   485,   486,     0,     0,     0,     0,
       0,   487,  1095,   488,  1096,  1097,     0,   489,     0,     0,
       0,     0,     0,     0,  1098,   490,  1099,     0,  -123,     0,
    1100,   491,     0,     0,   492,     0,     8,   493,  1101,     0,
    1102,   494,     0,     0,  1103,  1104,     0,     0,     0,     0,
       0,  1105,     0,     0,   496,   497,     0,   223,   224,   225,
       0,   227,   228,   229,   230,   231,   498,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,     0,   245,
     246,   247,     0,     0,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   499,   500,   501,  1106,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   504,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   505,   506,
     507,   508,   509,     0,   510,     0,   511,   512,   513,   514,
     515,   516,   517,   518,   519,    59,     0,    13,   520,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   521,   522,   523,     0,    14,     0,
       0,   524,   525,     0,     0,     0,     0,     0,     0,     0,
     526,     0,   527,     0,   528,   529,    16,  1107,   -97,  1094,
       0,     0,     0,   480,   481,     3,     0,  -121,  -106,  -106,
       0,  -118,     0,   482,   483,   484,   485,   486,     0,     0,
       0,     0,     0,   487,  1095,   488,  1096,  1097,     0,   489,
       0,     0,     0,     0,     0,     0,  1098,   490,  1099,     0,
    -123,     0,  1100,   491,     0,     0,   492,     0,     8,   493,
    1101,     0,  1102,   494,     0,     0,  1103,  1104,     0,     0,
       0,     0,     0,  1105,     0,     0,   496,   497,     0,   223,
     224,   225,     0,   227,   228,   229,   230,   231,   498,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   243,
       0,   245,   246,   247,     0,     0,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   499,   500,   501,  1106,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     504,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     505,   506,   507,   508,   509,     0,   510,     0,   511,   512,
     513,   514,   515,   516,   517,   518,   519,    59,     0,    13,
     520,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   521,   522,   523,     0,
      14,     0,     0,   524,   525,     0,     0,     0,     0,     0,
       0,     0,   526,     0,   527,     0,   528,   529,    16,  1107,
    -102,   480,   481,     0,     0,     0,     0,     0,     0,     0,
       0,   482,   483,   484,   485,   486,     0,     0,     0,     0,
       0,   487,     0,   488,     0,     0,     0,   489,     0,     0,
       0,     0,     0,     0,     0,   490,     0,     0,     0,     0,
       0,   491,     0,     0,   492,     0,     0,   493,     0,     0,
       0,   494,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   495,     0,     0,   496,   497,   904,   223,   224,   225,
       0,   227,   228,   229,   230,   231,   498,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,     0,   245,
     246,   247,     0,     0,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   499,   500,   501,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,   577,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   504,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   505,   506,
     507,   508,   509,     0,   510,   696,   511,   512,   513,   514,
     515,   516,   517,   518,   519,   697,     0,     0,   520,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   521,   522,   523,     0,    14,     0,
       0,   524,   525,     0,     0,     0,     0,     0,   480,   481,
     905,     0,   527,   906,   528,   529,   693,   530,   482,   483,
     484,   485,   486,     0,     0,     0,     0,     0,   487,     0,
     488,     0,     0,     0,   489,     0,     0,     0,     0,     0,
       0,     0,   490,     0,     0,     0,     0,     0,   491,     0,
       0,   492,   694,     0,   493,     0,     0,     0,   494,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   495,     0,
       0,   496,   497,     0,   223,   224,   225,     0,   227,   228,
     229,   230,   231,   498,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,   243,     0,   245,   246,   247,     0,
       0,   250,   251,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   499,   500,
     501,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   502,   503,     0,     0,     0,     0,
       0,     0,     0,   577,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   504,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   505,   506,   507,   508,   509,
       0,   510,   696,   511,   512,   513,   514,   515,   516,   517,
     518,   519,   697,     0,     0,   520,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   521,   522,   523,     0,    14,     0,     0,   524,   525,
       0,     0,     0,     0,     0,   480,   481,   526,     0,   527,
       0,   528,   529,   693,   530,   482,   483,   484,   485,   486,
       0,     0,     0,     0,     0,   487,     0,   488,     0,     0,
     386,   489,     0,     0,     0,     0,     0,     0,     0,   490,
       0,     0,     0,     0,     0,   491,     0,     0,   492,   694,
       0,   493,     0,     0,     0,   494,     0,   387,   388,     0,
       0,     0,     0,     0,     0,   495,     0,     0,   496,   497,
       0,   223,   224,   225,     0,   227,   228,   229,   230,   231,
     498,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,     0,   245,   246,   247,     0,     0,   250,   251,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   499,   500,   501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,   503,     0,   389,     0,     0,     0,   390,     0,
     577,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   504,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   505,   506,   507,   508,   509,     0,   510,     0,
     511,   512,   513,   514,   515,   516,   517,   518,   519,    59,
       0,     0,   520,     0,     0,     0,     0,     0,   391,     0,
       0,     0,   392,     0,  1408,   393,     0,     0,   521,   522,
     523,     0,    14,     0,     0,   524,   525,     0,     0,     0,
       0,     0,   480,   481,   526,     0,   527,     0,   528,   529,
     394,   530,   482,   483,   484,   485,   486,     0,     0,     0,
       0,     0,   487,     0,   488,     0,     0,   386,   489,     0,
       0,     0,     0,     0,     0,     0,   490,     0,     0,     0,
       0,     0,   491,     0,     0,   492,     0,     0,   493,     0,
       0,     0,   494,     0,   387,   388,     0,     0,     0,     0,
       0,     0,   495,     0,     0,   496,   497,  1028,   223,   224,
     225,     0,   227,   228,   229,   230,   231,   498,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,     0,
     245,   246,   247,     0,     0,   250,   251,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   499,   500,   501,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   502,   503,
       0,   389,     0,     0,     0,   390,     0,   577,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   504,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   505,
     506,   507,   508,   509,     0,   510,   696,   511,   512,   513,
     514,   515,   516,   517,   518,   519,   697,     0,     0,   520,
       0,     0,     0,     0,     0,   391,     0,     0,     0,   392,
       0,  1579,   393,     0,     0,   521,   522,   523,     0,    14,
       0,     0,   524,   525,     0,     0,     0,     0,     0,   480,
     481,   526,     0,   527,     0,   528,   529,   394,   530,   482,
     483,   484,   485,   486,     0,     0,     0,     0,     0,   487,
       0,   488,     0,     0,     0,   489,     0,     0,     0,     0,
       0,     0,     0,   490,     0,     0,     0,     0,     0,   491,
       0,     0,   492,     0,     0,   493,     0,     0,     0,   494,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   495,
       0,     0,   496,   497,     0,   223,   224,   225,     0,   227,
     228,   229,   230,   231,   498,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,     0,   245,   246,   247,
       0,     0,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   499,
     500,   501,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,   577,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   504,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   505,   506,   507,   508,
     509,     0,   510,   696,   511,   512,   513,   514,   515,   516,
     517,   518,   519,   697,     0,     0,   520,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   521,   522,   523,     0,    14,     0,     0,   524,
     525,     0,     0,     0,     0,     0,   480,   481,  1337,     0,
     527,  1338,   528,   529,     0,   530,   482,   483,   484,   485,
     486,     0,     0,     0,     0,     0,   487,     0,   488,     0,
       0,     0,   489,     0,     0,     0,     0,     0,     0,     0,
     490,     0,     0,     0,     0,     0,   491,     0,     0,   492,
       0,     0,   493,     0,     0,     0,   494,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   495,     0,     0,   496,
     497,     0,   223,   224,   225,     0,   227,   228,   229,   230,
     231,   498,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,   243,     0,   245,   246,   247,     0,     0,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   499,   500,   501,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   502,   503,     0,     0,     0,     0,     0,     0,
       0,   577,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   504,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   505,   506,   507,   508,   509,     0,   510,
     696,   511,   512,   513,   514,   515,   516,   517,   518,   519,
     697,     0,     0,   520,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   521,
     522,   523,     0,    14,     0,     0,   524,   525,     0,     0,
       0,     0,     0,   480,   481,  1348,     0,   527,  1349,   528,
     529,     0,   530,   482,   483,   484,   485,   486,     0,     0,
       0,     0,     0,   487,     0,   488,     0,     0,     0,   489,
       0,     0,     0,     0,     0,     0,     0,   490,     0,     0,
       0,     0,     0,   491,     0,     0,   492,     0,     0,   493,
       0,     0,     0,   494,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   495,     0,     0,   496,   497,     0,   223,
     224,   225,     0,   227,   228,   229,   230,   231,   498,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   243,
       0,   245,   246,   247,     0,     0,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   499,   500,   501,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,     0,     0,     0,   577,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     504,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     505,   506,   507,   508,   509,     0,   510,   696,   511,   512,
     513,   514,   515,   516,   517,   518,   519,   697,     0,     0,
     520,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   521,   522,   523,     0,
      14,     0,     0,   524,   525,     0,     0,     0,     0,     0,
     480,   481,   526,     0,   527,     0,   528,   529,     0,   530,
     482,   483,   484,   485,   486,     0,     0,     0,     0,     0,
     487,     0,   488,     0,     0,     0,   489,     0,     0,     0,
       0,     0,     0,     0,   490,     0,     0,     0,     0,     0,
     491,     0,     0,   492,     0,     0,   493,     0,     0,     0,
     494,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     495,     0,     0,   496,   497,     0,   223,   224,   225,     0,
     227,   228,   229,   230,   231,   498,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,     0,   245,   246,
     247,     0,     0,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     499,   500,   501,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   502,   503,     0,     0,
       0,     0,     0,     0,     0,   577,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   504,     0,    58,
       0,     0,     0,     0,     0,     0,     0,   505,   506,   507,
     508,   509,     0,   510,     0,   511,   512,   513,   514,   515,
     516,   517,   518,   519,    59,     0,     0,   520,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   521,   522,   523,     0,    14,     0,     0,
     524,   525,     0,     0,     0,     0,     0,   480,   481,   526,
       0,   527,   966,   528,   529,     0,   530,   482,   483,   484,
     485,   486,     0,     0,     0,     0,     0,   487,     0,   488,
       0,     0,     0,   489,     0,     0,     0,     0,     0,     0,
       0,   490,     0,     0,     0,     0,     0,   491,     0,     0,
     492,     0,     0,   493,     0,     0,     0,   494,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   495,     0,     0,
     496,   497,     0,   223,   224,   225,     0,   227,   228,   229,
     230,   231,   498,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,     0,   245,   246,   247,     0,     0,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   499,   500,   501,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,   503,     0,     0,     0,     0,     0,
       0,     0,   988,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   504,     0,    58,     0,     0,     0,
       0,     0,     0,     0,   505,   506,   507,   508,   509,     0,
     510,   696,   511,   512,   513,   514,   515,   516,   517,   518,
     519,   697,     0,     0,   520,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     521,   522,   523,     0,    14,     0,     0,   524,   525,     0,
       0,     0,     0,     0,   480,   481,  1271,     0,   527,     0,
     528,   529,     0,   530,   482,   483,   484,   485,   486,     0,
       0,     0,     0,     0,   487,     0,   488,     0,     0,     0,
     489,     0,     0,     0,     0,     0,     0,     0,   490,     0,
       0,     0,     0,     0,   491,     0,     0,   492,     0,     0,
     493,     0,     0,     0,   494,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   495,     0,     0,   496,   497,     0,
     223,   224,   225,     0,   227,   228,   229,   230,   231,   498,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,     0,   245,   246,   247,     0,     0,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   499,   500,   501,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     502,   503,     0,     0,     0,     0,     0,     0,     0,   577,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   504,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   505,   506,   507,   508,   509,     0,   510,     0,   511,
     512,   513,   514,   515,   516,   517,   518,   519,    59,     0,
       0,   520,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   521,   522,   523,
       0,    14,     0,     0,   524,   525,     0,     0,     0,     0,
       0,   480,   481,   526,     0,   527,  1355,   528,   529,     0,
     530,   482,   483,   484,   485,   486,     0,     0,     0,     0,
       0,   487,     0,   488,     0,     0,     0,   489,     0,     0,
       0,     0,     0,     0,     0,   490,     0,     0,     0,     0,
       0,   491,     0,     0,   492,     0,     0,   493,     0,     0,
       0,   494,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   495,     0,     0,   496,   497,     0,   223,   224,   225,
       0,   227,   228,   229,   230,   231,   498,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,     0,   245,
     246,   247,     0,     0,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   499,   500,   501,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,   577,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   504,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   505,   506,
     507,   508,   509,     0,   510,     0,   511,   512,   513,   514,
     515,   516,   517,   518,   519,    59,     0,     0,   520,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   521,   522,   523,     0,    14,     0,
       0,   524,   525,     0,     0,     0,     0,     0,   480,   481,
     526,     0,   527,  1414,   528,   529,     0,   530,   482,   483,
     484,   485,   486,     0,     0,     0,     0,     0,   487,     0,
     488,     0,     0,     0,   489,     0,     0,     0,     0,     0,
       0,     0,   490,     0,     0,     0,     0,     0,   491,     0,
       0,   492,     0,     0,   493,     0,     0,     0,   494,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   495,     0,
       0,   496,   497,     0,   223,   224,   225,     0,   227,   228,
     229,   230,   231,   498,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,   243,     0,   245,   246,   247,     0,
       0,   250,   251,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   499,   500,
     501,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   502,   503,     0,     0,     0,     0,
       0,     0,     0,   988,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   504,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   505,   506,   507,   508,   509,
       0,   510,   696,   511,   512,   513,   514,   515,   516,   517,
     518,   519,   697,     0,     0,   520,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   521,   522,   523,     0,    14,     0,     0,   524,   525,
       0,     0,     0,     0,     0,   480,   481,   526,     0,   527,
       0,   528,   529,     0,   530,   482,   483,   484,   485,   486,
       0,     0,     0,     0,     0,   487,     0,   488,     0,     0,
       0,   489,     0,     0,     0,     0,     0,     0,     0,   490,
       0,     0,     0,     0,     0,   491,     0,     0,   492,     0,
       0,   493,     0,     0,     0,   494,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   495,     0,     0,   496,   497,
       0,   223,   224,   225,     0,   227,   228,   229,   230,   231,
     498,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,     0,   245,   246,   247,     0,     0,   250,   251,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   499,   500,   501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,   503,     0,     0,     0,     0,     0,     0,     0,
     577,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   504,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   505,   506,   507,   508,   509,     0,   510,     0,
     511,   512,   513,   514,   515,   516,   517,   518,   519,    59,
       0,     0,   520,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   521,   522,
     523,     0,    14,     0,     0,   524,   525,     0,     0,     0,
       0,     0,   480,   481,   526,     0,   527,     0,   528,   529,
       0,   530,   482,   483,   484,   485,   486,     0,     0,     0,
       0,     0,   487,     0,   488,     0,     0,     0,   489,     0,
       0,     0,     0,     0,     0,     0,   490,     0,     0,     0,
       0,     0,   491,     0,     0,   492,     0,     0,   493,     0,
       0,     0,   494,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   495,     0,     0,   496,   497,     0,   223,   224,
     225,     0,   227,   228,   229,   230,   231,   498,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,     0,
     245,   246,   247,     0,     0,   250,   251,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   499,   500,   501,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   502,   503,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   504,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   505,
     506,   507,   508,   509,     0,   510,     0,   511,   512,   513,
     514,   515,   516,   517,   518,   519,    59,     0,     0,   520,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   521,   522,   523,     0,    14,
       0,     0,   524,   525,     0,     0,     0,     0,     0,   480,
     481,   526,   587,   527,     0,   528,   529,     0,   530,   482,
     483,   484,   485,   486,     0,     0,     0,     0,     0,   487,
       0,   488,     0,     0,     0,   489,     0,     0,     0,     0,
       0,     0,     0,   490,     0,     0,     0,     0,     0,   491,
       0,     0,   492,     0,     0,   493,     0,     0,     0,   494,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   495,
       0,     0,   496,   497,     0,   223,   224,   225,     0,   227,
     228,   229,   230,   231,   498,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,     0,   245,   246,   247,
       0,     0,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   499,
     500,   501,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   504,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   505,   506,   507,   508,
     509,     0,   510,     0,   511,   512,   513,   514,   515,   516,
     517,   518,   519,    59,     0,     0,   520,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   521,   522,   523,     0,    14,     0,     0,   524,
     525,     0,     0,     0,     0,     0,   480,   481,   526,   779,
     527,     0,   528,   529,     0,   530,   482,   483,   484,   485,
     486,     0,     0,     0,     0,     0,   487,     0,   488,     0,
       0,     0,   489,     0,     0,     0,     0,     0,     0,     0,
     490,     0,     0,     0,     0,     0,   491,     0,     0,   492,
       0,     0,   493,     0,     0,     0,   494,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   495,     0,     0,   496,
     497,     0,   223,   224,   225,     0,   227,   228,   229,   230,
     231,   498,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,   243,     0,   245,   246,   247,     0,     0,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   499,   500,   501,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   502,   503,     0,     0,     0,     0,     0,     0,
       0,   988,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   504,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   505,   506,   507,   508,   509,     0,   510,
       0,   511,   512,   513,   514,   515,   516,   517,   518,   519,
      59,     0,     0,   520,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   521,
     522,   523,     0,    14,     0,     0,   524,   525,     0,     0,
       0,     0,     0,   480,   481,   526,     0,   527,     0,   528,
     529,  1081,   530,   482,   483,   484,   485,   486,     0,     0,
       0,     0,     0,   487,     0,   488,     0,     0,     0,   489,
       0,     0,     0,     0,     0,     0,     0,   490,     0,     0,
       0,     0,     0,   491,     0,     0,   492,     0,     0,   493,
       0,     0,     0,   494,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   495,     0,     0,   496,   497,     0,   223,
     224,   225,     0,   227,   228,   229,   230,   231,   498,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   243,
       0,   245,   246,   247,     0,     0,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   499,   500,   501,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     504,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     505,   506,   507,   508,   509,     0,   510,     0,   511,   512,
     513,   514,   515,   516,   517,   518,   519,    59,     0,     0,
     520,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   521,   522,   523,     0,
      14,     0,     0,   524,   525,     0,     0,     0,     0,     0,
     480,   481,   526,     0,   527,     0,   528,   529,     0,   530,
     482,   483,   484,   485,   486,     0,     0,     0,     0,     0,
     487,     0,   488,     0,     0,     0,   489,     0,     0,     0,
       0,     0,     0,     0,   490,     0,     0,     0,     0,     0,
     491,     0,     0,   492,     0,     0,   493,     0,     0,     0,
     494,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     495,     0,     0,   496,   497,     0,   223,   224,   225,     0,
     227,   228,   229,   230,   231,   498,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,     0,   245,   246,
     247,     0,     0,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     499,   500,   501,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   502,   503,     0,     0,
       0,     0,     0,     0,     0,  1281,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   504,     0,    58,
       0,     0,     0,     0,     0,     0,     0,   505,   506,   507,
     508,   509,     0,   510,     0,   511,   512,   513,   514,   515,
     516,   517,   518,   519,    59,     0,     0,   520,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   521,   522,   523,     0,    14,     0,     0,
     524,   525,     0,     0,     0,     0,     0,   480,   481,   526,
       0,   527,     0,   528,   529,     0,   530,   482,   483,   484,
     485,   486,     0,     0,     0,     0,     0,   487,     0,   488,
       0,     0,     0,   489,     0,     0,     0,     0,     0,     0,
       0,   490,     0,     0,     0,     0,     0,   491,     0,     0,
     492,     0,     0,   493,     0,     0,     0,   494,     0,     0,
       0,     0,     0,  1284,     0,     0,     0,   495,     0,     0,
     496,   497,     0,   223,   224,   225,     0,   227,   228,   229,
     230,   231,   498,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,     0,   245,   246,   247,     0,     0,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   499,   500,   501,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,   503,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   504,     0,    58,     0,     0,     0,
       0,     0,     0,     0,   505,   506,   507,   508,   509,     0,
     510,     0,   511,   512,   513,   514,   515,   516,   517,   518,
     519,    59,     0,     0,   520,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     521,   522,   523,     0,    14,     0,     0,   524,   525,     0,
       0,     0,     0,     0,   480,   481,   526,     0,   527,     0,
     528,   529,     0,   530,   482,   483,   484,   485,   486,     0,
       0,     0,     0,     0,   487,     0,   488,     0,     0,     0,
     489,     0,     0,     0,     0,     0,     0,     0,   490,     0,
       0,     0,     0,     0,   491,     0,     0,   492,     0,     0,
     493,     0,     0,     0,   494,     0,     0,  1290,     0,     0,
       0,     0,     0,     0,   495,     0,     0,   496,   497,     0,
     223,   224,   225,     0,   227,   228,   229,   230,   231,   498,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,     0,   245,   246,   247,     0,     0,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   499,   500,   501,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     502,   503,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   504,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   505,   506,   507,   508,   509,     0,   510,     0,   511,
     512,   513,   514,   515,   516,   517,   518,   519,    59,     0,
       0,   520,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   521,   522,   523,
       0,    14,     0,     0,   524,   525,     0,     0,     0,     0,
       0,   480,   481,   526,     0,   527,     0,   528,   529,     0,
     530,   482,   483,   484,   485,   486,     0,     0,     0,     0,
       0,   487,     0,   488,     0,     0,     0,   489,     0,     0,
       0,     0,     0,     0,     0,   490,     0,     0,     0,     0,
       0,   491,     0,     0,   492,     0,     0,   493,     0,     0,
       0,   494,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   495,     0,     0,   496,   497,     0,   223,   224,   225,
       0,   227,   228,   229,   230,   231,   498,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,     0,   245,
     246,   247,     0,     0,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   499,   500,   501,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,  1293,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   504,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   505,   506,
     507,   508,   509,     0,   510,     0,   511,   512,   513,   514,
     515,   516,   517,   518,   519,    59,     0,     0,   520,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   521,   522,   523,     0,    14,     0,
       0,   524,   525,     0,     0,     0,     0,     0,   480,   481,
     526,     0,   527,     0,   528,   529,     0,   530,   482,   483,
     484,   485,   486,     0,     0,     0,     0,     0,   487,     0,
     488,     0,     0,     0,   489,     0,     0,     0,     0,     0,
       0,     0,   490,     0,     0,     0,     0,     0,   491,     0,
       0,   492,     0,     0,   493,     0,     0,     0,   494,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   495,     0,
       0,   496,   497,     0,   223,   224,   225,     0,   227,   228,
     229,   230,   231,   498,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,   243,     0,   245,   246,   247,     0,
       0,   250,   251,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   499,   500,
     501,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   502,   503,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   504,     0,    58,     0,     0,
       0,     0,     0,     0,     0,   505,   506,   507,   508,   509,
       0,   510,     0,   511,   512,   513,   514,   515,   516,   517,
     518,   519,    59,     0,     0,   520,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   521,   522,   523,     0,    14,     0,     0,   524,   525,
       0,     0,     0,     0,     0,   480,   481,   526,     0,   527,
    1543,   528,   529,     0,   530,   482,   483,   484,   485,   486,
       0,     0,     0,     0,     0,   487,     0,   488,     0,     0,
       0,   489,     0,     0,     0,     0,     0,     0,     0,   490,
       0,     0,     0,     0,     0,   491,     0,     0,   492,     0,
       0,   493,     0,     0,     0,   494,  1574,     0,     0,     0,
       0,     0,     0,     0,     0,   495,     0,     0,   496,   497,
       0,   223,   224,   225,     0,   227,   228,   229,   230,   231,
     498,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,     0,   245,   246,   247,     0,     0,   250,   251,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   499,   500,   501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,   503,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   504,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   505,   506,   507,   508,   509,     0,   510,     0,
     511,   512,   513,   514,   515,   516,   517,   518,   519,    59,
       0,     0,   520,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   521,   522,
     523,     0,    14,     0,     0,   524,   525,     0,     0,     0,
       0,     0,   480,   481,   526,     0,   527,     0,   528,   529,
       0,   530,   482,   483,   484,   485,   486,     0,     0,     0,
       0,     0,   487,     0,   488,     0,     0,     0,   489,     0,
       0,     0,     0,     0,     0,     0,   490,     0,     0,     0,
       0,     0,   491,     0,     0,   492,     0,     0,   493,     0,
       0,     0,   494,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   495,     0,     0,   496,   497,     0,   223,   224,
     225,     0,   227,   228,   229,   230,   231,   498,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,     0,
     245,   246,   247,     0,     0,   250,   251,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   499,   500,   501,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   502,   503,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   504,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   505,
     506,   507,   508,   509,     0,   510,     0,   511,   512,   513,
     514,   515,   516,   517,   518,   519,    59,     0,     0,   520,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   521,   522,   523,     0,    14,
       0,     0,   524,   525,     0,     0,     0,     0,     0,   480,
     481,   526,     0,   527,     0,   528,   529,     0,   530,   482,
     483,   484,   485,   486,     0,     0,     0,     0,     0,   487,
    1095,   488,  1096,     0,     0,   489,     0,     0,     0,     0,
       0,     0,     0,   490,     0,     0,     0,     0,     0,   491,
       0,     0,   492,     0,     0,   493,  1101,     0,     0,   494,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   495,
       0,     0,   496,   497,     0,   223,   224,   225,     0,   227,
     228,   229,   230,   231,   498,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,     0,   245,   246,   247,
       0,     0,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   499,
     500,   501,  1106,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   504,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   505,   506,   507,   508,
     509,     0,   510,     0,   511,   512,   513,   514,   515,   516,
     517,   518,   519,    59,     0,     0,   520,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   521,   522,   523,     0,    14,     0,     0,   524,
     525,     0,     0,     0,   480,   481,     0,     0,   526,     0,
     527,     0,   528,   529,   482,   483,   484,   485,   486,     0,
       0,     0,     0,     0,   487,     0,   488,     0,     0,     0,
     489,     0,     0,     0,     0,     0,     0,     0,   490,     0,
       0,     0,     0,     0,   491,     0,     0,   492,     0,     0,
     493,     0,     0,     0,   494,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   495,     0,     0,   496,   497,     0,
     223,   224,   225,     0,   227,   228,   229,   230,   231,   498,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,     0,   245,   246,   247,     0,     0,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   499,   500,   501,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     502,   503,     0,     0,     0,     0,     0,     0,     0,   824,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   504,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   505,   506,   507,   508,   509,     0,   510,     0,   511,
     512,   513,   514,   515,   516,   517,   518,   519,    59,     0,
       0,   520,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   521,   522,   523,
       0,    14,     0,     0,   524,   525,     0,     0,     0,   480,
     481,     0,     0,   526,     0,   527,     0,   528,   529,   482,
     483,   484,   485,   486,     0,     0,   945,     0,     0,   487,
       0,   488,     0,     0,     0,   489,     0,     0,     0,     0,
       0,     0,     0,   490,     0,     0,     0,     0,     0,   491,
       0,     0,   492,     0,     0,   493,     0,     0,     0,   494,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   495,
       0,     0,   496,   497,     0,   223,   224,   225,     0,   227,
     228,   229,   230,   231,   498,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,     0,   245,   246,   247,
       0,     0,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   499,
     500,   501,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   504,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   505,   506,   507,   508,
     509,     0,   510,     0,   511,   512,   513,   514,   515,   516,
     517,   518,   519,    59,     0,     0,   520,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   521,   522,   523,     0,    14,     0,     0,   524,
     525,     0,     0,     0,   480,   481,     0,     0,   526,     0,
     527,     0,   528,   529,   482,   483,   484,   485,   486,     0,
       0,     0,     0,     0,   487,     0,   488,     0,     0,     0,
     489,     0,     0,     0,     0,     0,     0,     0,   490,     0,
       0,     0,     0,     0,   491,     0,     0,   492,     0,     0,
     493,     0,     0,     0,   494,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   495,     0,     0,   496,   497,     0,
     223,   224,   225,     0,   227,   228,   229,   230,   231,   498,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,     0,   245,   246,   247,     0,     0,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   499,   500,   501,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     502,   503,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   504,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   505,   506,   507,   508,   509,     0,   510,     0,   511,
     512,   513,   514,   515,   516,   517,   518,   519,    59,     0,
     217,   520,     0,   713,   714,     0,   218,     0,     0,     0,
       0,     0,   219,     0,     0,     0,     0,   521,   522,   523,
       0,    14,   220,     0,   524,   525,     0,     0,     0,     0,
     221,     0,     0,   526,     0,   527,     0,   528,   529,     0,
       0,     0,     0,     0,     0,   222,     0,     0,     0,     0,
       0,     0,   223,   224,   225,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   -87,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   713,
     714,   715,   716,   717,   718,   719,     0,     0,   720,   721,
     722,   723,     0,   724,   725,   726,   727,   728,     0,   729,
     730,     0,     0,  -909,     0,    58,     0,   732,   733,   734,
       0,     0,     0,  -909,     0,     0,     0,     0,   279,   713,
     714,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      59,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   736,     0,   737,   738,   739,   740,   741,
     742,   743,   744,   745,   746,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   747,   748,     0,     0,   280,
       0,     0,     0,     0,   590,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1314,
    1315,  1316,  1317,  1318,  1319,  1320,  1321,   715,   716,   717,
     718,   719,  1322,  1323,   720,   721,   722,   723,  1324,   724,
     725,   726,   727,   728,  1325,   729,   730,  1326,  1327,   731,
       0,     0,     0,   732,   733,   734,  1328,  1329,  1330,   735,
       0,     0,     0,     0,     0,     0,     0,   715,   716,   717,
     718,   719,     0,     0,   720,   721,   722,   723,     0,   724,
     725,   726,   727,   728,     0,   729,   730,     0,  1331,   736,
       0,   737,   738,   739,   740,   741,   742,   743,   744,   745,
     746,   217,     0,     0,     0,     0,     0,   218,     0,     0,
       0,   747,   748,   219,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   220,     0,     0,     0,     0,     0,     0,
       0,   221,   738,   739,   740,   741,   742,   743,   744,   745,
     746,     0,     0,     0,     0,     0,   222,     0,     0,     0,
       0,   747,   748,   223,   224,   225,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   279,
       0,     0,   217,     0,     0,     0,     0,     0,   218,     0,
       0,   761,     0,    13,   219,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   220,     0,     0,     0,     0,     0,
       0,     0,   221,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   222,     0,     0,
     280,     0,    16,     0,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,   251,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    58,     0,     0,
       0,     0,   713,   714,     0,     0,     0,     0,     0,     0,
     279,   217,     0,     0,     0,     0,     0,   218,     0,     0,
       0,     0,    59,   219,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   220,     0,     0,     0,     0,     0,     0,
       0,   221,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   222,     0,     0,     0,
       0,   280,     0,   223,   224,   225,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,     0,
     715,   716,   717,   718,   719,   713,   714,   720,   721,   722,
     723,     0,   724,   725,   726,   727,   728,     0,   729,   730,
       0,     0,     0,     0,     0,     0,   732,   733,   734,     0,
       0,     0,   713,   714,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   279,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   761,   736,     0,   737,   738,   739,   740,   741,   742,
     743,   744,   745,   746,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   747,   748,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     280,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   713,   714,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   715,   716,   717,   718,   719,     0,     0,
     720,   721,   722,   723,     0,   724,   725,   726,   727,   728,
       0,   729,   730,   713,   714,     0,     0,     0,     0,   732,
     715,   716,   717,   718,   719,     0,     0,   720,   721,   722,
     723,     0,   724,   725,   726,   727,   728,     0,   729,   730,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   737,   738,   739,
     740,   741,   742,   743,   744,   745,   746,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   747,   748,     0,
       0,     0,     0,     0,   737,   738,   739,   740,   741,   742,
     743,   744,   745,   746,     0,     0,     0,     0,     0,   715,
     716,   717,   718,   719,   747,   748,   720,   721,   722,   723,
       0,   724,   725,   726,   727,   728,     0,   729,   730,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   715,   716,   717,   718,   719,     0,     0,   720,     0,
       0,   723,     0,   724,   725,   726,   727,   728,     0,   729,
     730,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   739,   740,   741,   742,   743,
     744,   745,   746,   914,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   747,   748,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   740,   741,
     742,   743,   744,   745,   746,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   747,   748,     0,     0,     0,
       0,     0,     0,     0,     0,   223,   224,   225,     0,   227,
     228,   229,   230,   231,   498,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,     0,   245,   246,   247,
       0,     0,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   301,
       0,   302,     0,   303,   304,   305,   306,   307,   918,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
       0,   319,   320,   321,     0,     0,   322,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   915,     0,     0,     0,     0,     0,     0,
     223,   224,   225,   916,   227,   228,   229,   230,   231,   498,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,     0,   245,   246,   247,     0,     0,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   276,   301,     0,   302,   595,   303,   304,
     305,   306,   307,  1143,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,     0,   319,   320,   321,     0,
       0,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   919,     0,
       0,     0,     0,     0,     0,   223,   224,   225,   920,   227,
     228,   229,   230,   231,   498,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,     0,   245,   246,   247,
       0,     0,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,     0,
       0,     0,   597,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   300,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1144,   301,     0,   302,     0,   303,   304,
     305,   306,   307,  1145,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,     0,   319,   320,   321,     0,
       0,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   349,   350,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   223,   224,   225,     0,
     227,   228,   229,   230,   231,   498,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,     0,   245,   246,
     247,     0,   351,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   957,   958,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   959,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   960,     0,     0,     0,     0,     0,
       0,     0,     0,   301,     0,   302,     0,   303,   304,   305,
     306,   307,     0,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,     0,   319,   320,   321,   961,   962,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   786
};

static const yytype_int16 yycheck[] =
{
       1,    14,    15,   145,   146,   103,   385,   826,   484,   527,
     172,   192,   193,    45,   627,   623,   484,   698,   699,    20,
      21,    22,   192,   193,   635,   565,   756,   567,   758,   569,
     760,  1074,  1443,   594,  1003,  1231,   702,   593,    81,   992,
    1312,     8,    33,     5,  1685,    22,   402,   403,    19,    50,
      51,    64,    65,    66,    86,   397,   574,   873,  1011,   986,
     163,  1097,    15,    16,    20,   876,   913,  1412,    33,  1685,
       0,  1406,  1685,    40,    20,    20,  1685,     7,    46,    20,
      57,   161,  1685,   152,   153,  1616,   165,   166,   167,   770,
    1716,   104,   105,   106,   107,   191,    19,    20,   130,   131,
      30,   133,    32,   189,    34,  1746,   163,   203,  1630,   173,
      40,  1080,   189,   173,   191,   149,   181,  1743,   905,   176,
      50,   224,   163,  1601,   149,   189,    56,   152,   153,   189,
    1746,     7,   474,  1746,   230,   176,  1614,  1746,   656,   201,
      15,    16,   228,  1746,  1785,  1676,    69,  1425,   227,   667,
      80,   231,   670,   230,   223,   224,   767,    21,    22,   173,
     771,   129,   226,   125,   126,   208,   226,  1578,   202,  1785,
     227,   231,  1785,   207,    50,   189,  1785,   191,   859,   192,
     193,   862,  1785,  1461,   702,   851,  1664,  1665,  1523,  1524,
     871,   164,   226,   874,   149,   125,   126,   173,   223,   224,
     362,   190,  1547,   191,   202,  1540,   207,  1542,   206,   162,
     199,  1058,   354,   189,   205,   179,   230,   359,   232,    62,
    1036,   363,  1265,   836,   191,   189,  1037,   397,   181,   202,
     149,   749,  1185,   186,   211,   206,   189,   200,   904,   192,
     205,  1696,   230,   199,   206,   601,   223,   202,  1201,   860,
     226,  1178,   207,   199,   199,   731,  1292,   224,   199,   224,
    1782,   191,   226,   731,   226,   232,   189,   280,   233,  1604,
    1605,   226,   150,   226,  1796,    33,   179,   141,   142,  1734,
    1615,   191,   212,   202,   149,   149,   189,   151,   152,   153,
     154,   155,  1261,   199,   224,   199,   477,   478,   479,   149,
     230,  1088,    60,    61,   474,  1577,   181,   477,   478,   479,
       8,   186,   199,   199,   189,   657,   226,   192,   150,   179,
     191,   364,   200,   211,   156,   203,   230,   199,   150,   189,
     472,   673,  1062,   851,   156,   223,  1532,   202,    36,   225,
     227,   881,   207,   199,   208,   209,   210,   211,   212,   162,
      57,   207,   202,  1140,   202,   226,    63,   207,  1769,   223,
     224,   226,  1028,  1093,   150,  1152,   199,   227,   200,   199,
     156,   911,   191,   984,   230,  1411,   226,   189,   200,   560,
     561,     5,     6,   564,   397,   566,   165,   568,   167,   147,
     560,   561,   225,   151,   564,   576,   566,   227,   568,   199,
     570,    25,  1083,   189,  1017,  1374,   576,    31,   150,  1445,
     164,   230,   206,   232,   200,   228,   229,   189,   231,   205,
     150,  1032,   983,   593,   980,   199,   156,   227,   199,   610,
     611,   150,   226,   171,   164,   996,   171,   156,   994,   199,
     610,   611,   150,   201,    68,    69,  1057,   205,   156,   207,
     208,   189,    33,   227,   189,   199,   227,   199,   200,   226,
     202,   474,   150,   205,   477,   478,   479,   227,   156,   206,
     200,   484,   485,   211,   212,   233,  1295,   212,   206,    60,
      61,   200,   199,   227,  1271,   223,   199,   657,   223,   226,
     164,   504,   200,   835,    64,    65,    66,   199,   226,   173,
     206,   125,   126,   673,   178,   199,   848,   849,     5,     6,
     227,   206,   200,   199,   227,   189,   858,    21,    22,   199,
     226,   199,   864,   865,   866,   227,   868,   164,   870,  1572,
     872,   226,   206,   227,   104,   105,   106,   107,   880,   199,
     164,   227,   130,   713,   714,   225,   199,   560,   561,   227,
    1337,   564,   189,   566,   179,   568,   199,   570,   728,   701,
     199,  1348,   164,   576,   189,   189,   147,   227,   199,   150,
     151,   173,   190,   191,   227,   156,   199,   747,   199,   179,
     593,   199,   206,    33,   227,   203,   225,   189,   769,   189,
    1559,   202,   190,   191,   205,  1135,   227,   610,   611,   769,
     224,   199,  1332,   199,   227,   618,   227,   231,   199,   207,
      60,    61,   230,   199,   190,   191,   199,  1228,   211,   200,
     201,   181,  1140,   199,   205,   199,   627,   208,   199,   225,
     199,   207,   230,    34,  1152,   199,   227,   141,   142,  1157,
      57,   227,   225,   206,   657,   149,    63,   151,   152,   153,
     154,   155,   233,   227,   230,  1691,   227,   211,   227,   173,
     673,   225,    63,   226,   179,   835,   847,    79,   193,   850,
     173,    21,    22,   854,   189,   189,   189,   847,   848,   849,
     850,   189,    94,   189,   854,   206,   189,    99,   858,   101,
     703,  1727,  1728,  1035,   864,   865,   866,   147,   868,  1041,
     870,   151,   872,   873,  1491,   226,   210,   211,   212,    57,
     880,   173,    47,  1500,  1056,    63,   189,   203,   179,   223,
     224,   207,   190,   191,   191,   190,   191,   189,   189,   130,
    1766,   199,    67,  1251,   199,    12,   190,   191,   372,   207,
    1258,   189,   207,    57,    33,   199,    23,    24,   382,    63,
     173,   201,   211,   207,   155,   205,   769,   207,   208,    21,
      22,   395,   230,   164,   223,   230,   189,    57,   227,   190,
     191,    60,    61,    63,   130,   945,   230,   223,   199,   189,
     190,    57,   783,   233,   785,   130,   207,    63,   189,   199,
     199,   141,   142,   202,   230,   231,   205,   978,   231,   149,
     981,   151,   152,   153,   154,   155,   987,   189,   978,   230,
     980,   981,    57,  1600,  1601,   216,   189,   987,    63,  1606,
     189,   190,   835,   189,   994,   226,   189,  1614,   200,  1616,
     199,   216,  1651,   200,   847,   848,   849,   850,   839,   193,
     199,   854,   202,   202,   165,   858,   205,   191,  1324,   189,
     203,   864,   865,   866,   207,   868,  1324,   870,   147,   872,
     873,   543,   151,   545,   546,  1035,  1036,   880,    21,    22,
    1657,  1041,   203,   223,   224,   163,   207,  1664,  1665,   141,
     142,   542,   543,    57,   545,   546,  1056,   149,    66,  1676,
     152,   153,   154,   155,   193,   194,   190,   191,   190,   191,
     203,   190,   191,   190,   207,   199,   202,   199,  1426,   129,
     199,   912,   201,   207,   163,   207,   205,   226,   207,   208,
     190,   191,   203,    33,   203,   189,   207,   224,   207,   199,
     203,   203,  1613,   226,   207,   207,   230,   207,   230,   206,
    1759,   230,   203,   577,   233,   203,   207,   206,   203,  1736,
      60,    61,   207,   203,   206,   589,   190,   207,    10,    11,
     230,   223,   224,  1105,   206,   978,   206,   980,   981,  1788,
     193,   194,   195,   196,   987,   165,   166,   167,  1659,  1302,
    1303,   994,   193,   194,   195,    33,   139,   140,   141,   142,
     206,  1333,   206,   994,   206,   996,   149,   631,   151,   152,
     153,   154,   155,   206,   157,   158,   206,  1008,   193,   194,
     195,   226,    60,    61,   231,  1196,  1017,    35,  1019,  1537,
      35,   231,  1035,  1036,   658,   659,  1196,   226,  1041,   663,
    1638,   665,   189,   189,    33,   206,   224,   147,   231,   189,
     150,   151,  1560,  1056,   225,  1207,   156,   681,   682,   683,
     684,   685,   686,    22,   189,   208,   209,   210,   211,   212,
     225,    60,    61,   231,   202,   502,   503,   818,   819,   820,
     223,   224,    55,    56,    57,   189,   206,   206,   206,   226,
    1693,   191,   206,   206,   521,   522,   523,   524,   525,   206,
     200,   201,   726,   226,   206,   205,   227,   206,   208,   147,
     226,  1102,   224,   151,  1246,   226,   226,   226,   226,  1110,
    1111,    75,   227,   226,   748,    79,   207,  1730,   226,   226,
     230,   189,  1123,   233,  1125,  1126,  1127,  1128,  1129,    93,
      94,   227,  1133,   227,    98,    99,   100,   101,  1280,   199,
     225,   775,   225,   230,   226,    43,   227,   226,   147,   207,
     226,  1530,   151,   201,  1335,   206,   206,   205,    13,   207,
     208,   206,   206,  1333,   226,  1335,   199,   226,     4,   226,
    1688,   225,   227,   226,   202,   189,   613,   189,   225,   189,
     814,   189,   189,  1196,   189,   233,   199,   227,   131,   132,
     133,   134,   135,   136,   137,   138,   227,   227,   832,     1,
     227,   227,   201,   227,   838,   227,   205,   150,   207,   208,
     227,   227,  1213,   156,   206,   226,   206,    43,   227,   853,
     226,    33,   225,   227,   226,   168,   169,   170,   226,   226,
     226,   226,   226,   226,   233,   207,   225,   207,   226,   207,
     227,   200,    21,    22,   878,   879,   227,   226,    60,    61,
     200,   227,   226,    43,   227,   889,   227,   200,   202,   226,
     894,   227,   896,   226,   898,   227,  1447,   227,   226,   903,
     227,   227,   227,   227,    43,   189,   227,  1447,   715,   716,
     226,   226,   719,   720,   721,   722,   227,   724,    33,   231,
     727,   728,   729,   730,   731,   732,   733,   734,   735,   736,
     737,   738,   739,   740,   741,   742,   743,   744,   745,   746,
     232,   189,   189,   181,  1495,    60,    61,    10,    13,     9,
    1333,    33,  1335,    42,   958,  1495,    66,   206,   962,   226,
     226,   225,   225,  1514,   226,   147,   189,   189,   189,   151,
     189,   232,   232,  1485,  1514,   232,     8,   189,    60,    61,
    1692,   227,   226,   207,   988,   189,   197,   189,  1456,   189,
     139,   140,   141,   142,   143,   227,   189,   146,   147,   148,
     149,   189,   151,   152,   153,   154,   155,    14,   157,   158,
     226,   189,  1383,   200,  1018,  1386,   165,   824,   167,   201,
     200,   202,   181,   205,   227,   207,   208,   226,   226,  1412,
     227,   226,   147,   227,   200,   227,   151,   227,  1042,    33,
     226,   232,    43,   232,    33,    37,   200,    67,   227,   226,
     226,   233,   207,   226,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,  1447,   147,    60,    61,   226,   151,
     227,    60,    61,   226,   223,   224,   226,    70,   226,    33,
     226,   226,  1453,   226,  1088,   226,   201,   227,   189,   226,
     205,  1095,   207,   208,  1098,    43,   227,   189,   226,   226,
    1104,   227,  1106,   227,   227,   912,    60,    61,   189,   227,
     226,   230,  1495,   226,   226,   226,   193,   226,   233,   201,
    1632,   227,   189,   205,   227,  1637,   208,   227,   232,   227,
     227,  1514,   227,  1137,   227,  1139,   227,  1141,    33,   227,
     227,   231,   227,   227,   227,    12,   199,  1151,   227,   227,
     230,   233,  1692,   147,   227,  1159,   227,   151,   147,   199,
     230,   227,   151,   227,  1547,   227,   227,    53,   227,   227,
     227,   226,  1574,   227,   230,   227,   227,  1689,  1690,  1183,
     225,    21,    22,   231,   225,   232,   232,   226,   232,  1193,
     227,    80,   687,   147,  1198,   231,  1200,   151,    38,   232,
       1,  1008,    83,   191,  1795,  1209,  1577,   201,  1212,   785,
     785,   205,   201,   207,   208,  1793,   205,   214,   207,   208,
    1012,  1303,   703,  1594,   769,     1,  1441,  1231,  1656,  1596,
    1655,   707,  1517,  1599,  1656,  1747,    33,    54,  1011,   233,
      -1,    33,  1754,  1011,   233,   484,  1201,   201,  1760,   356,
    1254,   205,   232,   207,   208,   484,   484,   484,  1262,  1263,
    1264,    -1,    -1,    60,    61,   658,   484,   484,    60,    61,
      -1,    -1,    -1,    -1,  1786,    -1,    -1,  1281,    -1,   233,
    1284,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1293,
      -1,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,  1692,
      -1,   151,   152,   153,    -1,  1686,   156,   157,   158,   159,
     160,    -1,    -1,    -1,  1695,   165,   166,   167,   168,   169,
     170,   171,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1343,
    1147,  1345,    -1,    -1,    -1,    -1,    -1,    -1,  1352,    -1,
     147,    -1,    21,    22,   151,   147,    -1,    -1,    -1,   151,
      -1,   201,  1733,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,    -1,    -1,   215,   216,    -1,    -1,    -1,
    1751,    -1,    -1,   223,   224,    -1,    -1,    -1,  1392,    -1,
      -1,    -1,    -1,    -1,  1765,    -1,  1767,    -1,    -1,    -1,
      -1,  1772,    -1,    -1,   201,    -1,  1410,    -1,   205,   201,
     207,   208,    -1,   205,    -1,   207,   208,    -1,  1789,    -1,
    1791,    -1,    -1,  1427,  1428,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   233,    -1,    33,    -1,
      -1,   233,    -1,    -1,  1448,    21,    22,    -1,    -1,    -1,
    1454,  1455,    -1,    -1,    -1,    -1,  1460,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,   155,  1491,   157,   158,
      -1,    -1,    -1,    -1,    -1,    -1,  1500,    -1,    -1,    -1,
      -1,  1505,    -1,    -1,  1508,    -1,  1510,  1314,  1315,  1316,
    1317,  1318,  1319,  1320,  1321,  1322,  1323,  1324,  1325,  1326,
    1327,  1328,  1329,  1330,  1331,    -1,    -1,    -1,  1532,  1533,
    1534,  1535,  1536,    -1,    -1,    21,    22,   206,   207,   208,
     209,   210,   211,   212,    -1,    -1,    -1,    -1,    -1,  1553,
      -1,    -1,   147,    -1,   223,   224,   151,    -1,    -1,    -1,
      33,    -1,    -1,   139,   140,   141,   142,   143,    -1,  1573,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,   155,
      -1,   157,   158,    -1,    -1,   161,    -1,    60,    61,   165,
     166,   167,    -1,    -1,    -1,   171,    -1,    -1,    -1,  1603,
      -1,    -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,    -1,
     205,    -1,   207,   208,    -1,    -1,  1620,    -1,    -1,    -1,
      -1,  1625,  1626,    -1,    -1,   201,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,    -1,   233,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1453,   223,   224,    -1,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,   155,
      -1,   157,   158,    -1,   147,   161,  1680,    -1,   151,   165,
     166,   167,    -1,    -1,    -1,   171,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1717,    -1,   201,    -1,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,  1731,   201,    -1,
      -1,    -1,   205,    -1,   207,   208,  1740,   223,   224,     1,
      -1,    -1,    -1,     5,     6,     7,  1750,     9,    10,    11,
      -1,    13,    -1,    15,    16,    17,    18,    19,    -1,    -1,
     233,    -1,    -1,    25,    26,    27,    28,    29,    -1,    31,
    1774,    -1,    -1,    -1,    -1,    -1,    38,    39,    40,    -1,
      42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,    51,
      52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   131,   132,   133,
     134,   135,   136,   137,   138,    -1,    -1,    -1,  1685,   141,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1695,    -1,
      -1,    -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     162,    -1,   164,    -1,   168,   169,   170,    -1,    -1,    -1,
     172,   173,   174,   175,   176,    -1,   178,    -1,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,    -1,   191,
     192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1746,
      -1,    -1,    -1,    -1,    -1,    -1,   208,   209,   210,    -1,
     212,    -1,    -1,   215,   216,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   224,    -1,   226,  1772,   228,   229,   230,   231,
     232,     1,    -1,    -1,    -1,     5,     6,     7,  1785,     9,
      10,    11,    -1,    13,  1791,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   162,    -1,   164,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   172,   173,   174,   175,   176,    -1,   178,    -1,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
      -1,   191,   192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   208,   209,
     210,    -1,   212,    -1,    -1,   215,   216,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   224,    -1,   226,    -1,   228,   229,
     230,   231,   232,     1,    -1,    -1,    -1,     5,     6,     7,
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
      -1,    -1,    -1,    -1,   162,    -1,   164,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   172,   173,   174,   175,   176,    -1,
     178,    -1,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,    -1,   191,   192,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     208,   209,   210,    -1,   212,    -1,    -1,   215,   216,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   224,    -1,   226,    -1,
     228,   229,   230,   231,   232,     1,    -1,    -1,    -1,     5,
       6,     7,    -1,     9,    10,    11,    -1,    13,    -1,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    38,    39,    40,    -1,    42,    -1,    44,    45,
      -1,    -1,    48,    -1,    50,    51,    52,    -1,    54,    55,
      -1,    -1,    58,    59,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,   164,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   172,   173,   174,   175,
     176,    -1,   178,    -1,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,    -1,   191,   192,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   208,   209,   210,    -1,   212,    -1,    -1,   215,
     216,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,    -1,
     226,    -1,   228,   229,   230,   231,   232,     1,    -1,    -1,
      -1,     5,     6,     7,    -1,     9,    10,    11,    -1,    13,
      -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    26,    27,    28,    29,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    38,    39,    40,    -1,    42,    -1,
      44,    45,    -1,    -1,    48,    -1,    50,    51,    52,    -1,
      54,    55,    -1,    -1,    58,    59,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,
     164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,   173,
     174,   175,   176,    -1,   178,    -1,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,    -1,   191,   192,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   208,   209,   210,    -1,   212,    -1,
      -1,   215,   216,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     224,    -1,   226,    -1,   228,   229,   230,   231,   232,     1,
      -1,    -1,    -1,     5,     6,     7,    -1,     9,    10,    11,
      -1,    13,    -1,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    40,    -1,
      42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,    51,
      52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     162,    -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     172,   173,   174,   175,   176,    -1,   178,    -1,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,    -1,   191,
     192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   208,   209,   210,    -1,
     212,    -1,    -1,   215,   216,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   224,    -1,   226,    -1,   228,   229,   230,   231,
     232,     5,     6,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    70,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,
     164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,   173,
     174,   175,   176,    -1,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,    -1,    -1,   192,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   208,   209,   210,    -1,   212,    -1,
      -1,   215,   216,    -1,    -1,    -1,    -1,    -1,     5,     6,
     224,    -1,   226,   227,   228,   229,    13,   231,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    49,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   162,    -1,   164,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   172,   173,   174,   175,   176,
      -1,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,    -1,    -1,   192,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   208,   209,   210,    -1,   212,    -1,    -1,   215,   216,
      -1,    -1,    -1,    -1,    -1,     5,     6,   224,    -1,   226,
      -1,   228,   229,    13,   231,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      33,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    49,
      -1,    51,    -1,    -1,    -1,    55,    -1,    60,    61,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,   147,    -1,    -1,    -1,   151,    -1,
     150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   162,    -1,   164,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   172,   173,   174,   175,   176,    -1,   178,    -1,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
      -1,    -1,   192,    -1,    -1,    -1,    -1,    -1,   201,    -1,
      -1,    -1,   205,    -1,   207,   208,    -1,    -1,   208,   209,
     210,    -1,   212,    -1,    -1,   215,   216,    -1,    -1,    -1,
      -1,    -1,     5,     6,   224,    -1,   226,    -1,   228,   229,
     233,   231,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    33,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    70,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,
      -1,   147,    -1,    -1,    -1,   151,    -1,   150,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,
      -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,
     173,   174,   175,   176,    -1,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,    -1,    -1,   192,
      -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,    -1,   205,
      -1,   207,   208,    -1,    -1,   208,   209,   210,    -1,   212,
      -1,    -1,   215,   216,    -1,    -1,    -1,    -1,    -1,     5,
       6,   224,    -1,   226,    -1,   228,   229,   233,   231,    15,
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
      -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,   164,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   172,   173,   174,   175,
     176,    -1,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,    -1,    -1,   192,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   208,   209,   210,    -1,   212,    -1,    -1,   215,
     216,    -1,    -1,    -1,    -1,    -1,     5,     6,   224,    -1,
     226,   227,   228,   229,    -1,   231,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   162,    -1,   164,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   172,   173,   174,   175,   176,    -1,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,    -1,    -1,   192,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   208,
     209,   210,    -1,   212,    -1,    -1,   215,   216,    -1,    -1,
      -1,    -1,    -1,     5,     6,   224,    -1,   226,   227,   228,
     229,    -1,   231,    15,    16,    17,    18,    19,    -1,    -1,
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
     162,    -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     172,   173,   174,   175,   176,    -1,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,    -1,    -1,
     192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   208,   209,   210,    -1,
     212,    -1,    -1,   215,   216,    -1,    -1,    -1,    -1,    -1,
       5,     6,   224,    -1,   226,    -1,   228,   229,    -1,   231,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,
      55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,   164,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,   173,   174,
     175,   176,    -1,   178,    -1,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,    -1,    -1,   192,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   208,   209,   210,    -1,   212,    -1,    -1,
     215,   216,    -1,    -1,    -1,    -1,    -1,     5,     6,   224,
      -1,   226,   227,   228,   229,    -1,   231,    15,    16,    17,
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
      -1,    -1,    -1,    -1,   162,    -1,   164,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   172,   173,   174,   175,   176,    -1,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,    -1,    -1,   192,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     208,   209,   210,    -1,   212,    -1,    -1,   215,   216,    -1,
      -1,    -1,    -1,    -1,     5,     6,   224,    -1,   226,    -1,
     228,   229,    -1,   231,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
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
     141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   162,    -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   172,   173,   174,   175,   176,    -1,   178,    -1,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,    -1,
      -1,   192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   208,   209,   210,
      -1,   212,    -1,    -1,   215,   216,    -1,    -1,    -1,    -1,
      -1,     5,     6,   224,    -1,   226,   227,   228,   229,    -1,
     231,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,
     164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,   173,
     174,   175,   176,    -1,   178,    -1,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,    -1,    -1,   192,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   208,   209,   210,    -1,   212,    -1,
      -1,   215,   216,    -1,    -1,    -1,    -1,    -1,     5,     6,
     224,    -1,   226,   227,   228,   229,    -1,   231,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   162,    -1,   164,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   172,   173,   174,   175,   176,
      -1,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,    -1,    -1,   192,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   208,   209,   210,    -1,   212,    -1,    -1,   215,   216,
      -1,    -1,    -1,    -1,    -1,     5,     6,   224,    -1,   226,
      -1,   228,   229,    -1,   231,    15,    16,    17,    18,    19,
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
      -1,    -1,   162,    -1,   164,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   172,   173,   174,   175,   176,    -1,   178,    -1,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
      -1,    -1,   192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   208,   209,
     210,    -1,   212,    -1,    -1,   215,   216,    -1,    -1,    -1,
      -1,    -1,     5,     6,   224,    -1,   226,    -1,   228,   229,
      -1,   231,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,
      -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,
     173,   174,   175,   176,    -1,   178,    -1,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,    -1,    -1,   192,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   208,   209,   210,    -1,   212,
      -1,    -1,   215,   216,    -1,    -1,    -1,    -1,    -1,     5,
       6,   224,   225,   226,    -1,   228,   229,    -1,   231,    15,
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
      -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,   164,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   172,   173,   174,   175,
     176,    -1,   178,    -1,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,    -1,    -1,   192,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   208,   209,   210,    -1,   212,    -1,    -1,   215,
     216,    -1,    -1,    -1,    -1,    -1,     5,     6,   224,   225,
     226,    -1,   228,   229,    -1,   231,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   141,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   162,    -1,   164,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   172,   173,   174,   175,   176,    -1,   178,
      -1,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,    -1,    -1,   192,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   208,
     209,   210,    -1,   212,    -1,    -1,   215,   216,    -1,    -1,
      -1,    -1,    -1,     5,     6,   224,    -1,   226,    -1,   228,
     229,    13,   231,    15,    16,    17,    18,    19,    -1,    -1,
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
     162,    -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     172,   173,   174,   175,   176,    -1,   178,    -1,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,    -1,    -1,
     192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   208,   209,   210,    -1,
     212,    -1,    -1,   215,   216,    -1,    -1,    -1,    -1,    -1,
       5,     6,   224,    -1,   226,    -1,   228,   229,    -1,   231,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,
      55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   141,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,   164,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,   173,   174,
     175,   176,    -1,   178,    -1,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,    -1,    -1,   192,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   208,   209,   210,    -1,   212,    -1,    -1,
     215,   216,    -1,    -1,    -1,    -1,    -1,     5,     6,   224,
      -1,   226,    -1,   228,   229,    -1,   231,    15,    16,    17,
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
      -1,    -1,    -1,    -1,   162,    -1,   164,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   172,   173,   174,   175,   176,    -1,
     178,    -1,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,    -1,    -1,   192,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     208,   209,   210,    -1,   212,    -1,    -1,   215,   216,    -1,
      -1,    -1,    -1,    -1,     5,     6,   224,    -1,   226,    -1,
     228,   229,    -1,   231,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    58,    -1,    -1,
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
      -1,   162,    -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   172,   173,   174,   175,   176,    -1,   178,    -1,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,    -1,
      -1,   192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   208,   209,   210,
      -1,   212,    -1,    -1,   215,   216,    -1,    -1,    -1,    -1,
      -1,     5,     6,   224,    -1,   226,    -1,   228,   229,    -1,
     231,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,
     164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,   173,
     174,   175,   176,    -1,   178,    -1,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,    -1,    -1,   192,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   208,   209,   210,    -1,   212,    -1,
      -1,   215,   216,    -1,    -1,    -1,    -1,    -1,     5,     6,
     224,    -1,   226,    -1,   228,   229,    -1,   231,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   162,    -1,   164,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   172,   173,   174,   175,   176,
      -1,   178,    -1,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,    -1,    -1,   192,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   208,   209,   210,    -1,   212,    -1,    -1,   215,   216,
      -1,    -1,    -1,    -1,    -1,     5,     6,   224,    -1,   226,
     227,   228,   229,    -1,   231,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    56,    -1,    -1,    -1,
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
      -1,    -1,   162,    -1,   164,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   172,   173,   174,   175,   176,    -1,   178,    -1,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
      -1,    -1,   192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   208,   209,
     210,    -1,   212,    -1,    -1,   215,   216,    -1,    -1,    -1,
      -1,    -1,     5,     6,   224,    -1,   226,    -1,   228,   229,
      -1,   231,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   141,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,
      -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,
     173,   174,   175,   176,    -1,   178,    -1,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,    -1,    -1,   192,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   208,   209,   210,    -1,   212,
      -1,    -1,   215,   216,    -1,    -1,    -1,    -1,    -1,     5,
       6,   224,    -1,   226,    -1,   228,   229,    -1,   231,    15,
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
      -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,   164,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   172,   173,   174,   175,
     176,    -1,   178,    -1,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,    -1,    -1,   192,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   208,   209,   210,    -1,   212,    -1,    -1,   215,
     216,    -1,    -1,    -1,     5,     6,    -1,    -1,   224,    -1,
     226,    -1,   228,   229,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
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
     141,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   162,    -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   172,   173,   174,   175,   176,    -1,   178,    -1,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,    -1,
      -1,   192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   208,   209,   210,
      -1,   212,    -1,    -1,   215,   216,    -1,    -1,    -1,     5,
       6,    -1,    -1,   224,    -1,   226,    -1,   228,   229,    15,
      16,    17,    18,    19,    -1,    -1,    22,    -1,    -1,    25,
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
      -1,    -1,    -1,    -1,    -1,    -1,   162,    -1,   164,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   172,   173,   174,   175,
     176,    -1,   178,    -1,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,    -1,    -1,   192,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   208,   209,   210,    -1,   212,    -1,    -1,   215,
     216,    -1,    -1,    -1,     5,     6,    -1,    -1,   224,    -1,
     226,    -1,   228,   229,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
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
      -1,   162,    -1,   164,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   172,   173,   174,   175,   176,    -1,   178,    -1,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,    -1,
      19,   192,    -1,    21,    22,    -1,    25,    -1,    -1,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,   208,   209,   210,
      -1,   212,    41,    -1,   215,   216,    -1,    -1,    -1,    -1,
      49,    -1,    -1,   224,    -1,   226,    -1,   228,   229,    -1,
      -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    -1,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,    10,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,   139,   140,   141,   142,   143,    -1,    -1,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,   155,    -1,   157,
     158,    -1,    -1,   161,    -1,   164,    -1,   165,   166,   167,
      -1,    -1,    -1,   171,    -1,    -1,    -1,    -1,   177,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     189,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   201,    -1,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   223,   224,    -1,    -1,   228,
      -1,    -1,    -1,    -1,   233,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
      -1,    -1,    -1,   165,   166,   167,   168,   169,   170,   171,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,   155,    -1,   157,   158,    -1,   200,   201,
      -1,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,
      -1,   223,   224,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,   204,   205,   206,   207,   208,   209,   210,   211,
     212,    -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,   223,   224,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   164,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   177,
      -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      -1,   189,    -1,   191,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,
     228,    -1,   230,    -1,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   164,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
     177,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,
      -1,    -1,   189,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,   228,    -1,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,    -1,
     139,   140,   141,   142,   143,    21,    22,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,   155,    -1,   157,   158,
      -1,    -1,    -1,    -1,    -1,    -1,   165,   166,   167,    -1,
      -1,    -1,    21,    22,    -1,    -1,   164,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   177,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   189,   201,    -1,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   223,   224,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,   155,
      -1,   157,   158,    21,    22,    -1,    -1,    -1,    -1,   165,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,   155,    -1,   157,   158,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   223,   224,    -1,
      -1,    -1,    -1,    -1,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,    -1,    -1,    -1,    -1,    -1,   139,
     140,   141,   142,   143,   223,   224,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,   155,    -1,   157,   158,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   139,   140,   141,   142,   143,    -1,    -1,   146,    -1,
      -1,   149,    -1,   151,   152,   153,   154,   155,    -1,   157,
     158,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   205,   206,   207,   208,   209,
     210,   211,   212,    19,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   223,   224,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   206,   207,
     208,   209,   210,   211,   212,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   223,   224,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    71,
      -1,    73,    -1,    75,    76,    77,    78,    79,    19,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   179,    -1,    -1,    -1,    -1,    -1,    -1,
      71,    72,    73,   189,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    71,    -1,    73,   189,    75,    76,
      77,    78,    79,    19,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   179,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    72,    73,   189,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,    -1,   189,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   179,    71,    -1,    73,    -1,    75,    76,
      77,    78,    79,   189,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   141,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,   189,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   152,   153,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    71,    -1,    73,    -1,    75,    76,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,   223,   224,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   189
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   235,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   125,   126,   191,   212,   224,   230,   237,   238,   242,
     251,   253,   254,   259,   308,   314,   345,   429,   437,   445,
     455,   505,   510,   515,    19,    20,    69,   189,   298,   299,
     300,   181,   260,   261,   201,   256,   257,    57,    63,   434,
     435,   438,   189,   228,   240,   516,   506,   511,   164,   189,
     333,    34,    63,   130,   155,   216,   226,   303,   304,   305,
     306,   333,   237,   237,   237,     8,    36,   456,    62,   425,
     200,   199,   202,   199,   171,   189,   212,   223,   255,   255,
     189,   237,   237,   425,   434,   434,   434,   189,   164,   252,
     305,   305,   305,   226,   165,   166,   167,   199,   225,   130,
     313,   446,     5,     6,   452,    57,    63,   426,    15,    16,
     162,   181,   186,   189,   192,   226,   244,   299,   181,   261,
     211,   211,   255,   211,   211,   223,    22,    57,   258,   189,
     436,    57,    63,   239,   189,   189,   189,   189,   193,   250,
     227,   300,   305,   305,   305,   305,   191,   265,   266,    57,
      63,   315,   317,    57,    63,   439,   130,   130,    57,    63,
     453,   231,   430,   189,   193,   194,   195,   243,    15,    16,
     181,   186,   189,   244,   296,   297,   255,   255,   255,   189,
     189,   189,   200,   200,   216,   241,   202,   466,   266,   266,
     193,   227,   191,   318,   189,   440,   457,   427,   163,   301,
     395,   193,   194,   195,   199,   227,   258,    19,    25,    31,
      41,    49,    64,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   177,
     228,   333,   460,   462,   463,   467,   473,   475,   504,   504,
      66,    79,    94,    99,   101,   190,   443,   444,   507,   512,
      35,    71,    73,    75,    76,    77,    78,    79,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    93,
      94,    95,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   141,
     142,   189,   311,   312,   316,   202,   441,   129,   450,   451,
     232,   237,   428,   299,   163,   189,   421,   424,   296,   206,
     206,   206,   226,   206,   206,   226,   466,   206,   206,   206,
     206,   206,   226,   333,   206,   226,    33,    60,    61,   147,
     151,   201,   205,   208,   233,   224,   472,   203,   190,   517,
     231,   231,    21,    22,    38,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   151,   152,   153,   156,   157,   158,   159,
     160,   165,   166,   167,   168,   169,   170,   171,   201,   203,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   215,
     216,   223,   224,    35,    35,   226,   309,   266,    75,    79,
      93,    94,    98,    99,   100,   101,   461,   444,   189,   266,
     395,   266,   299,   199,   202,   205,   419,   476,   482,   484,
       5,     6,    15,    16,    17,    18,    19,    25,    27,    31,
      39,    45,    48,    51,    55,    65,    68,    69,    80,   125,
     126,   127,   141,   142,   162,   172,   173,   174,   175,   176,
     178,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     192,   208,   209,   210,   215,   216,   224,   226,   228,   229,
     231,   249,   251,   325,   333,   338,   350,   357,   360,   363,
     367,   369,   371,   372,   374,   379,   382,   383,   384,   393,
     394,   460,   521,   529,   540,   543,   556,   557,   560,   561,
     486,   480,   189,   206,   488,   490,   492,   494,   496,   498,
     500,   502,   383,   206,   226,   474,   478,   150,   330,   361,
     383,    33,   205,    33,   205,   224,   233,   225,   383,   224,
     233,   473,   231,   508,   513,   189,   312,   189,   312,   189,
     225,    22,   189,   225,   176,   227,   395,   405,   406,   407,
     149,   202,   310,   161,   231,   322,   362,   231,   202,   449,
     458,   173,   189,   420,   423,   266,   189,   473,   150,   156,
     200,   418,   504,   504,   471,   504,   206,   206,   206,   333,
     335,   462,   520,   529,   540,   543,   556,   557,   560,   561,
     333,   206,     5,   125,   126,   206,   226,   206,   226,   226,
     206,   206,   206,   226,   206,   226,   206,   226,   206,   206,
     226,    19,   206,   206,   384,   384,   173,   178,   206,   333,
     373,   226,   226,   226,   226,   226,   226,   248,   384,   384,
     384,   384,   384,    13,    49,   330,   179,   189,   361,   522,
     524,   555,   226,   224,   307,   162,   231,   363,   368,   368,
     368,   368,   227,    21,    22,   139,   140,   141,   142,   143,
     146,   147,   148,   149,   151,   152,   153,   154,   155,   157,
     158,   161,   165,   166,   167,   171,   201,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   223,   224,   226,
     504,   504,   227,   207,   468,   504,   309,   504,   309,   504,
     309,   189,   408,   409,   504,   189,   411,   412,   227,   479,
     361,   332,   504,   383,   227,   199,   559,   225,   225,   225,
     383,   518,   408,   410,   411,   413,   189,   312,   131,   132,
     133,   134,   135,   136,   137,   138,   156,   168,   169,   170,
     131,   132,   133,   134,   135,   136,   137,   138,   150,   156,
     168,   169,   170,   200,   226,     7,    50,   344,   230,   199,
     230,   227,   504,   504,   150,   384,   323,   447,   333,   230,
     231,   454,   226,    43,   199,   202,   419,   237,   418,   383,
     207,   207,   207,   190,   199,   236,   237,   470,   530,   532,
     336,   226,   206,   226,   358,   206,   206,   206,   550,   361,
     473,   383,   554,   383,   351,   353,   385,   383,   355,   383,
     552,   361,   538,   541,   361,   206,   534,   473,   226,   226,
     375,   377,   383,   383,   383,   383,   383,   383,   195,   196,
     243,   226,    13,   225,   226,   150,   156,   200,   414,   559,
     199,   559,   227,   266,    70,   224,   227,   361,   524,   306,
       4,   366,   329,   307,    19,   179,   189,   460,    19,   179,
     189,   460,   384,   384,   384,   384,   384,   384,   189,   384,
     179,   189,   383,   384,   384,   460,   384,   384,   384,   556,
     561,   384,   384,   384,   384,    22,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   152,   153,   179,
     189,   223,   224,   380,   460,   383,   227,   361,   207,   207,
     189,   464,   207,   310,   207,   310,   207,   310,   202,   207,
     470,   202,   207,   470,   332,   559,   207,   470,   150,   383,
     225,   189,   465,   237,   273,   274,   273,   274,   383,   173,
     189,   415,   416,   459,   407,   407,   407,   384,   329,   189,
     431,   433,   401,   383,   189,   173,   189,   473,   418,   383,
     237,   477,   483,   485,   504,   473,   473,   504,    70,   361,
     524,   528,   189,   383,   504,   544,   546,   548,   473,   559,
     207,   470,   199,   559,   227,   473,   473,   473,   227,   473,
     227,   473,   559,   473,   409,   559,   536,   412,   207,   383,
     383,   473,   309,   227,   227,   227,   227,   227,   227,   383,
     173,   189,   226,   287,   226,   383,   383,   383,   227,   179,
     189,    13,   331,   554,   189,   227,   524,   522,   199,   227,
     227,   225,   226,   309,     1,    26,    28,    29,    38,    40,
      44,    52,    54,    58,    59,    65,   128,   231,   232,   237,
     262,   263,   272,   283,   284,   286,   288,   289,   290,   291,
     292,   293,   294,   295,   326,   334,   339,   340,   341,   342,
     343,   345,   349,   370,   384,   366,   206,   226,   206,   226,
     226,   226,   225,    19,   179,   189,   460,   202,   179,   189,
     383,   226,   226,   179,   189,   383,     1,   226,   225,   199,
     227,   487,   481,   199,   207,   230,   489,   207,   493,   207,
     497,   207,   504,   501,   408,   504,   503,   411,   207,   227,
     474,   504,   383,   200,   236,   432,   442,   237,   408,   509,
     411,   514,   227,   226,    43,   199,   202,   205,   414,   324,
     200,   432,   442,    40,   191,   232,   308,   402,   227,   226,
      43,   237,   418,   383,   237,   207,   207,   207,   524,   227,
     227,   227,   207,   470,   227,   207,   473,   409,   412,   207,
     227,   226,   473,   383,   227,   207,   207,   207,   207,   207,
     227,   207,   207,   227,   473,   207,   366,   227,   227,   207,
     310,   226,   202,   246,   226,    43,   189,   347,    20,   199,
     287,   227,   226,   156,   414,   226,   232,   559,   227,   199,
     225,   224,   522,   150,   156,   189,   200,   205,   364,   365,
     310,   150,   383,   322,    61,   383,   189,   189,   237,   181,
      58,   383,   266,   150,   383,   327,   237,   237,    10,    10,
      11,   270,    13,     9,    42,   237,   237,   237,   237,   237,
     237,    66,   346,   237,   131,   132,   133,   134,   135,   136,
     137,   138,   144,   145,   150,   156,   159,   160,   168,   169,
     170,   200,   309,   387,   383,   389,   383,   224,   227,   361,
     522,   383,   206,   226,   384,   226,   225,   383,   224,   227,
     361,   522,   226,   225,   381,   227,   361,   189,   469,   189,
     491,   495,   499,   474,   383,   189,   236,   519,   232,   232,
     383,   189,   173,   189,   504,   383,   232,   383,   431,   448,
     189,     8,   395,   400,   383,   189,   383,   237,   531,   533,
     337,   227,   226,   189,   359,   207,   207,   207,   551,   331,
     207,   352,   354,   386,   356,   553,   539,   542,   207,   535,
     226,   266,   376,   207,   227,   361,   247,   197,   383,   189,
     199,   227,   361,   173,   189,   226,    20,   156,   414,   383,
     383,   383,   287,   227,   522,   227,   189,   189,   226,   189,
     189,   199,   227,   266,   383,    14,   383,   200,   200,   202,
     181,   322,   383,   329,   226,   226,   224,   301,   302,   302,
     226,   226,   231,   348,   423,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   556,   561,   384,   384,
     384,   384,   384,   384,   384,   310,   473,   227,   504,   227,
     522,   199,   227,   227,   227,   391,   383,   383,   227,   522,
     199,   227,   227,   383,   227,   200,   232,   227,   226,    43,
     414,    37,   319,   232,   200,    57,    63,   398,    67,   399,
     237,   227,   237,   226,   226,   383,   207,   545,   547,   549,
     226,   227,   226,   226,   226,   226,   226,   226,    70,   528,
     226,   537,   226,   227,   383,   322,   373,   378,   227,   245,
     227,   189,   227,   226,    43,   347,   361,   383,   383,   227,
      20,   225,   189,   364,   362,   322,   504,   383,   328,   383,
     383,   300,   226,   226,    56,   383,   347,   422,   266,   207,
     207,   225,   522,   504,   227,   227,   225,   522,   227,   383,
     383,   189,   383,   320,   504,    47,   399,    46,   129,   396,
     528,   528,   227,   226,   226,   226,   226,   330,   331,   383,
     383,   383,   383,   361,   528,   226,   528,   227,   373,   193,
     230,   383,   189,   227,   227,   156,   414,   361,   227,   227,
     232,   227,   227,   225,   287,   383,   255,   227,   227,   232,
     237,   423,   362,   388,   390,   227,   227,   207,   227,   227,
     227,   231,   237,    33,   397,   396,   398,   226,   522,   525,
     526,   527,   527,   383,   528,   528,   522,   523,   227,   227,
     227,   227,   227,   227,   559,   527,   528,   523,   383,   227,
     230,   383,   383,   227,   319,    12,   271,   266,    20,   227,
     227,   266,   202,   419,   392,   329,   403,   397,   415,   416,
     417,   522,   199,   559,   227,   227,   227,   527,   527,   227,
     227,   227,   523,   227,   230,   558,   383,   230,   272,   339,
     340,   341,   342,   384,   237,   285,   361,   266,   266,   322,
     473,   418,   321,   316,   404,   227,   226,   227,   227,   227,
      53,   225,   558,   383,   231,   275,   278,   227,   322,   322,
     418,   383,   232,   237,   316,   522,   383,   225,   558,   276,
      12,    23,    24,   264,   267,   272,   266,   383,   237,   266,
     227,   232,   329,   266,   226,   237,   322,   237,   362,   277,
     268,   383,   232,   231,   279,   282,   227,   319,   280,   272,
     266,   329,   237,   269,   281,   279,   232,   267,   319
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   234,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   235,   235,   235,   235,   235,   236,   236,   237,
     237,   238,   239,   239,   239,   240,   240,   241,   241,   242,
     243,   243,   243,   243,   244,   244,   245,   245,   246,   247,
     246,   248,   248,   248,   249,   250,   250,   252,   251,   253,
     254,   255,   255,   255,   255,   255,   255,   255,   256,   256,
     257,   257,   258,   258,   259,   260,   260,   261,   261,   262,
     263,   263,   264,   264,   265,   265,   266,   266,   267,   268,
     267,   269,   267,   270,   270,   271,   271,   272,   272,   272,
     272,   272,   273,   273,   274,   274,   276,   277,   275,   278,
     275,   280,   281,   279,   282,   279,   284,   285,   283,   286,
     287,   287,   287,   287,   287,   287,   287,   287,   289,   288,
     290,   292,   291,   293,   294,   294,   295,   295,   296,   296,
     296,   296,   296,   296,   297,   297,   298,   298,   298,   298,
     299,   299,   299,   299,   299,   299,   299,   299,   299,   300,
     300,   301,   301,   302,   302,   302,   303,   303,   303,   303,
     304,   304,   305,   305,   305,   305,   305,   305,   305,   306,
     306,   307,   307,   308,   308,   309,   309,   309,   310,   310,
     310,   311,   311,   311,   311,   311,   311,   311,   311,   311,
     311,   311,   311,   311,   311,   311,   311,   311,   311,   311,
     311,   311,   311,   311,   311,   311,   311,   311,   311,   311,
     311,   311,   311,   311,   311,   311,   311,   311,   311,   311,
     311,   311,   311,   311,   311,   311,   311,   311,   311,   311,
     311,   311,   311,   311,   311,   311,   311,   311,   311,   311,
     311,   311,   311,   311,   311,   311,   311,   311,   311,   311,
     311,   311,   311,   311,   311,   311,   311,   311,   311,   311,
     311,   311,   311,   311,   311,   311,   311,   311,   311,   311,
     312,   312,   312,   312,   312,   312,   312,   312,   312,   312,
     312,   312,   312,   312,   312,   312,   312,   312,   312,   312,
     312,   312,   312,   312,   312,   312,   312,   312,   312,   312,
     312,   312,   312,   312,   312,   312,   312,   312,   312,   312,
     312,   312,   312,   312,   312,   312,   312,   312,   313,   313,
     314,   315,   315,   315,   316,   318,   317,   319,   320,   321,
     319,   323,   324,   322,   325,   325,   325,   325,   326,   326,
     326,   326,   326,   326,   326,   326,   326,   326,   326,   326,
     326,   326,   326,   326,   326,   326,   326,   327,   328,   326,
     329,   329,   329,   330,   330,   331,   331,   332,   332,   333,
     333,   333,   334,   334,   336,   337,   335,   335,   338,   338,
     338,   338,   338,   338,   339,   340,   341,   341,   341,   342,
     342,   343,   344,   344,   344,   345,   345,   346,   346,   347,
     347,   348,   348,   349,   349,   349,   351,   352,   350,   353,
     354,   350,   355,   356,   350,   358,   359,   357,   360,   360,
     360,   361,   361,   361,   361,   362,   362,   362,   363,   363,
     363,   364,   364,   364,   364,   364,   365,   365,   366,   366,
     367,   368,   368,   369,   369,   369,   369,   369,   369,   369,
     369,   370,   370,   370,   370,   370,   370,   370,   370,   370,
     370,   370,   370,   370,   370,   370,   370,   370,   370,   370,
     370,   370,   371,   371,   371,   372,   372,   372,   372,   372,
     373,   373,   374,   375,   376,   374,   377,   378,   374,   379,
     379,   379,   379,   379,   379,   379,   379,   379,   380,   381,
     379,   382,   382,   382,   382,   382,   382,   382,   383,   383,
     383,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   385,   386,   384,   384,   384,   384,
     387,   388,   384,   384,   384,   384,   389,   390,   384,   384,
     384,   391,   392,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     393,   393,   393,   394,   394,   394,   394,   394,   394,   394,
     394,   394,   394,   394,   394,   394,   394,   394,   394,   395,
     395,   396,   396,   396,   397,   397,   398,   398,   398,   399,
     399,   400,   401,   401,   401,   402,   401,   403,   401,   404,
     401,   405,   406,   406,   407,   407,   407,   407,   407,   408,
     408,   409,   409,   410,   410,   410,   411,   412,   412,   413,
     413,   413,   414,   414,   415,   415,   415,   416,   416,   417,
     417,   418,   418,   418,   419,   419,   420,   420,   420,   420,
     420,   420,   421,   421,   422,   422,   422,   423,   423,   423,
     424,   424,   424,   425,   425,   426,   426,   426,   427,   427,
     428,   427,   429,   430,   429,   431,   431,   432,   432,   433,
     433,   433,   434,   434,   434,   436,   435,   437,   437,   438,
     439,   439,   439,   440,   441,   441,   442,   442,   443,   443,
     444,   444,   446,   447,   448,   445,   449,   449,   450,   450,
     451,   452,   452,   452,   452,   453,   453,   453,   454,   454,
     456,   457,   458,   455,   459,   459,   459,   459,   459,   459,
     460,   460,   460,   460,   460,   460,   460,   460,   460,   460,
     460,   460,   460,   460,   460,   460,   460,   460,   460,   460,
     460,   460,   460,   460,   460,   460,   460,   460,   460,   460,
     460,   460,   460,   460,   460,   460,   460,   460,   460,   460,
     460,   460,   460,   460,   460,   460,   460,   460,   460,   460,
     461,   461,   461,   461,   461,   461,   461,   461,   462,   463,
     463,   463,   464,   464,   464,   465,   465,   465,   465,   465,
     466,   466,   466,   466,   466,   467,   468,   469,   467,   470,
     470,   471,   471,   472,   472,   472,   472,   473,   473,   474,
     474,   475,   475,   475,   475,   476,   477,   475,   475,   475,
     475,   478,   475,   479,   475,   475,   475,   475,   475,   475,
     475,   475,   475,   475,   475,   475,   475,   480,   481,   475,
     475,   482,   483,   475,   484,   485,   475,   486,   487,   475,
     475,   488,   489,   475,   490,   491,   475,   475,   492,   493,
     475,   494,   495,   475,   475,   496,   497,   475,   498,   499,
     475,   500,   501,   475,   502,   503,   475,   504,   504,   504,
     506,   507,   508,   509,   505,   511,   512,   513,   514,   510,
     516,   517,   518,   519,   515,   520,   520,   520,   520,   520,
     520,   520,   521,   521,   521,   521,   521,   522,   522,   522,
     522,   522,   522,   522,   522,   523,   523,   524,   525,   525,
     526,   526,   527,   527,   528,   528,   530,   531,   529,   532,
     533,   529,   534,   535,   529,   536,   537,   529,   538,   539,
     529,   540,   541,   542,   540,   543,   544,   545,   543,   546,
     547,   543,   548,   549,   543,   543,   550,   551,   543,   543,
     552,   553,   543,   554,   554,   555,   556,   557,   557,   557,
     558,   558,   559,   559,   560,   560,   561
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
       3,     5,     0,     1,     2,     1,     3,     1,     3,     3,
       3,     2,     1,     1,     1,     2,     0,     1,     0,     0,
       4,     0,     8,     1,     1,     0,     2,     1,     1,     1,
       1,     1,     1,     2,     0,     1,     0,     0,     6,     0,
       3,     0,     0,     6,     0,     3,     0,     0,     9,     7,
       1,     4,     3,     3,     3,     6,     5,     5,     0,    10,
       3,     0,     8,     0,     7,     8,     4,     4,     1,     1,
       1,     1,     1,     1,     1,     3,     1,     1,     1,     1,
       3,     3,     4,     3,     3,     3,     3,     1,     5,     1,
       3,     3,     4,     0,     3,     1,     1,     1,     1,     1,
       1,     4,     1,     2,     3,     3,     3,     3,     2,     1,
       3,     0,     3,     0,     4,     0,     2,     3,     0,     2,
       2,     1,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     3,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       3,     2,     2,     3,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     3,     2,     2,     2,
       2,     2,     3,     3,     3,     3,     3,     4,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       4,     0,     1,     1,     3,     0,     5,     0,     0,     0,
       6,     0,     0,     6,     2,     2,     2,     2,     1,     2,
       2,     1,     1,     1,     1,     2,     1,     2,     2,     2,
       2,     1,     1,     1,     2,     2,     2,     0,     0,     6,
       0,     2,     2,     0,     2,     0,     2,     1,     3,     1,
       3,     2,     2,     3,     0,     0,     5,     1,     2,     5,
       5,     5,     6,     2,     1,     1,     1,     2,     3,     2,
       3,     4,     1,     1,     0,     1,     1,     1,     0,     1,
       3,     8,     7,     3,     3,     5,     0,     0,     9,     0,
       0,     9,     0,     0,     9,     0,     0,     6,     5,     8,
      10,     1,     2,     3,     4,     1,     2,     3,     1,     1,
       1,     2,     2,     2,     2,     4,     1,     3,     0,     4,
       7,     7,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     6,     8,     6,     5,     6,     8,     8,     6,
       1,     4,     2,     0,     0,     7,     0,     0,     8,     3,
       4,     5,     6,     8,     8,     6,     5,     6,     0,     0,
       5,     3,     4,     4,     5,     4,     3,     4,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     4,     3,     4,     5,     4,     5,     3,     4,     1,
       1,     2,     4,     4,     0,     0,     9,     1,     3,     5,
       0,     0,     8,     3,     3,     3,     0,     0,     8,     3,
       4,     0,     0,     9,     4,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     3,     1,     4,     3,     3,     3,
       7,     8,     7,     4,     4,     4,     4,     4,     1,     6,
       7,     6,     6,     7,     7,     6,     7,     6,     5,     0,
       1,     0,     1,     1,     0,     1,     0,     1,     1,     0,
       1,     5,     0,     2,     6,     0,     4,     0,     9,     0,
      11,     3,     3,     4,     1,     1,     3,     3,     3,     1,
       3,     1,     3,     0,     1,     3,     3,     1,     3,     0,
       1,     3,     1,     1,     1,     2,     3,     3,     5,     1,
       1,     1,     1,     1,     0,     1,     1,     4,     3,     3,
       6,     5,     1,     3,     0,     2,     2,     4,     6,     5,
       4,     6,     5,     0,     1,     0,     1,     1,     0,     2,
       0,     4,     6,     0,     6,     1,     3,     1,     2,     0,
       1,     3,     0,     1,     1,     0,     5,     3,     3,     5,
       0,     1,     1,     1,     0,     2,     0,     1,     1,     2,
       0,     1,     0,     0,     0,    13,     0,     2,     0,     1,
       3,     1,     1,     2,     2,     0,     1,     1,     1,     3,
       0,     0,     0,     9,     1,     4,     3,     3,     6,     5,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       4,     4,     1,     3,     3,     0,     1,     3,     3,     5,
       0,     2,     2,     2,     2,     4,     0,     0,     7,     1,
       1,     1,     3,     3,     2,     4,     3,     1,     2,     0,
       4,     1,     1,     1,     1,     0,     0,     6,     4,     4,
       3,     0,     6,     0,     7,     4,     2,     2,     3,     2,
       3,     2,     2,     3,     3,     3,     2,     0,     0,     6,
       2,     0,     0,     6,     0,     0,     6,     0,     0,     6,
       1,     0,     0,     6,     0,     0,     7,     1,     0,     0,
       6,     0,     0,     7,     1,     0,     0,     6,     0,     0,
       7,     0,     0,     6,     0,     0,     6,     1,     3,     3,
       0,     0,     0,     0,    12,     0,     0,     0,     0,    12,
       0,     0,     0,     0,    13,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     5,
       5,     6,     6,     8,     8,     0,     1,     2,     3,     5,
       1,     2,     1,     0,     0,     1,     0,     0,    10,     0,
       0,    10,     0,     0,    10,     0,     0,    11,     0,     0,
       7,     5,     0,     0,    10,     3,     0,     0,    11,     0,
       0,    11,     0,     0,    10,     5,     0,     0,     9,     5,
       0,     0,    10,     1,     3,     0,     5,     5,     7,     9,
       0,     3,     0,     1,    11,    12,    13
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

  case 62: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 63: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 67: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 68: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 69: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 70: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 71: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 72: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 73: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 78: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 79: /* $@3: %empty  */
                                           {
    }
    break;

  case 80: /* expression_else: "else" optional_emit_semis $@3 expression_else_block  */
                                   {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 81: /* $@4: %empty  */
                                                                        {
    }
    break;

  case 82: /* expression_else: elif_or_static_elif '(' expr ')' optional_emit_semis $@4 expression_else_block expression_else  */
                                                         {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-7])),(yyvsp[-5].pExpression),(yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-7].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 83: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 84: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 85: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 86: /* expression_else_one_liner: "else" expression_if_one_liner  */
                                                      {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 87: /* expression_if_one_liner: expr_no_bracket  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 88: /* expression_if_one_liner: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 89: /* expression_if_one_liner: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 90: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 91: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 96: /* $@5: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 97: /* $@6: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 98: /* expression_if_block: '{' $@5 expressions $@6 '}' expression_block_finally  */
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

  case 99: /* $@7: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 100: /* expression_if_block: $@7 expression_if_one_liner SEMICOLON  */
                                               {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 101: /* $@8: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 102: /* $@9: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 103: /* expression_else_block: '{' $@8 expressions $@9 '}' expression_block_finally  */
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

  case 104: /* $@10: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 105: /* expression_else_block: $@10 expression_if_one_liner SEMICOLON  */
                                               {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 106: /* $@11: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 107: /* $@12: %empty  */
                                                                  {
    }
    break;

  case 108: /* expression_if_then_else: $@11 if_or_static_if '(' expr ')' optional_emit_semis $@12 expression_if_block expression_else  */
                                                       {
        yyextra->das_keyword = false;
        auto blk = (yyvsp[-1].pExpression)->rtti_isBlock() ? static_cast<ExprBlock *>((yyvsp[-1].pExpression)) : ast_wrapInBlock((yyvsp[-1].pExpression));
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-7])),(yyvsp[-5].pExpression),blk,(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-7].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 109: /* expression_if_then_else_oneliner: expression_if_one_liner "if" '(' expr ')' expression_else_one_liner SEMICOLON  */
                                                                                                                      {
        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ast_wrapInBlock((yyvsp[-6].pExpression)),(yyvsp[-1].pExpression) ? ast_wrapInBlock((yyvsp[-1].pExpression)) : nullptr);
    }
    break;

  case 110: /* for_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 111: /* for_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 112: /* for_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 113: /* for_variable_name_with_pos_list: '(' tuple_expansion ')'  */
                                       {
        auto pSL = new vector<VariableNameAndPosition>();
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        pSL->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 114: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 115: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "$i" '(' expr ')'  */
                                                                               {
        (yyvsp[-5].pNameWithPosList)->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = (yyvsp[-5].pNameWithPosList);
    }
    break;

  case 116: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 117: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' '(' tuple_expansion ')'  */
                                                                                 {
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
    }
    break;

  case 118: /* $@13: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 119: /* expression_for_loop: $@13 "for" optional_for_annotations '(' for_variable_name_with_pos_list "in" expr_list ')' optional_emit_semis expression_block  */
                                                                                                                                                                    {
        yyextra->das_keyword = false;
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-5].pNameWithPosList),(yyvsp[-3].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-8])),tokAt(scanner,(yylsp[0])),(yyvsp[-7].aaList));
    }
    break;

  case 120: /* expression_unsafe: "unsafe" optional_emit_semis expression_block  */
                                                                    {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-2])));
        pUnsafe->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 121: /* $@14: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 122: /* expression_while_loop: $@14 "while" optional_for_annotations '(' expr ')' optional_emit_semis expression_block  */
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

  case 123: /* with_keyword_on: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 124: /* expression_with: with_keyword_on "with" '(' expr ')' optional_emit_semis expression_block  */
                                                                                                     {
        yyextra->das_keyword = false;
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-5])));
        pWith->with = (yyvsp[-3].pExpression);
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 125: /* expression_with: with_keyword_on "with" '(' "module" require_module_name ')' optional_emit_semis expression_block  */
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

  case 126: /* expression_with_alias: "assume" "name" '=' expr  */
                                                      {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), ExpressionPtr((yyvsp[0].pExpression)));
        delete (yyvsp[-2].s);
    }
    break;

  case 127: /* expression_with_alias: "typedef" "name" '=' type_declaration  */
                                                                {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), TypeDeclPtr((yyvsp[0].pTypeDecl)));
        delete (yyvsp[-2].s);
    }
    break;

  case 128: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 129: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 130: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 131: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 132: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 133: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 134: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 135: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 136: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 137: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 138: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 139: /* annotation_argument_name: "default"  */
                    { (yyval.s) = new string("default"); }
    break;

  case 140: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 141: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 142: /* annotation_argument: annotation_argument_name '=' "@@" "name"  */
                                                                      { (yyval.aa) = new AnnotationArgument(*(yyvsp[-3].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-3]))); delete (yyvsp[0].s); delete (yyvsp[-3].s); }
    break;

  case 143: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 144: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 145: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 146: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 147: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 148: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                          {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[-4].s); }
    }
    break;

  case 149: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 150: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 151: /* metadata_argument_list: "@field" annotation_argument optional_emit_semis  */
                                                              {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[-1].aa));
    }
    break;

  case 152: /* metadata_argument_list: metadata_argument_list "@field" annotation_argument optional_emit_semis  */
                                                                                           {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-3].aaList),(yyvsp[-1].aa));
    }
    break;

  case 153: /* optional_for_annotations: %empty  */
                    {
        (yyval.aaList) = nullptr;
    }
    break;

  case 154: /* optional_for_annotations: '[' annotation_argument_list ']'  */
                                               {
        (yyval.aaList) = (yyvsp[-1].aaList);
    }
    break;

  case 155: /* optional_for_annotations: metadata_argument_list  */
                                     {
        (yyval.aaList) = (yyvsp[0].aaList);
    }
    break;

  case 156: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 157: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 158: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 159: /* annotation_declaration_name: "template"  */
                                    { (yyval.s) = new string("template"); }
    break;

  case 160: /* annotation_declaration_basic: annotation_declaration_name  */
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

  case 161: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
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

  case 162: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 163: /* annotation_declaration: '!' annotation_declaration  */
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

  case 164: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
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

  case 165: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
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

  case 166: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
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

  case 167: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 168: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 169: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 170: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 171: /* optional_annotation_list: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 172: /* optional_annotation_list: '[' annotation_list ']'  */
                                       { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 173: /* optional_annotation_list_with_emit_semis: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 174: /* optional_annotation_list_with_emit_semis: '[' annotation_list ']' optional_emit_semis  */
                                                          { (yyval.faList) = (yyvsp[-2].faList); }
    break;

  case 175: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 176: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 177: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 178: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 179: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 180: /* optional_function_type: "->" type_declaration  */
                                           {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 181: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 182: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 183: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 184: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 185: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 186: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 187: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 188: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 189: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 190: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 191: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 192: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 193: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 194: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 195: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 196: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 197: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 198: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 199: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 200: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 201: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 202: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 203: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 204: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 205: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 206: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 207: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 208: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 209: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 210: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 211: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 212: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 213: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 214: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 215: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 216: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 217: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 218: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 219: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 220: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 221: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 222: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 223: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 224: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 225: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 226: /* function_name: "operator" '[' ']' '='  */
                                 { (yyval.s) = new string("[]="); }
    break;

  case 227: /* function_name: "operator" '[' ']' "<-"  */
                                    { (yyval.s) = new string("[]<-"); }
    break;

  case 228: /* function_name: "operator" '[' ']' ":="  */
                                      { (yyval.s) = new string("[]:="); }
    break;

  case 229: /* function_name: "operator" '[' ']' "+="  */
                                     { (yyval.s) = new string("[]+="); }
    break;

  case 230: /* function_name: "operator" '[' ']' "-="  */
                                     { (yyval.s) = new string("[]-="); }
    break;

  case 231: /* function_name: "operator" '[' ']' "*="  */
                                     { (yyval.s) = new string("[]*="); }
    break;

  case 232: /* function_name: "operator" '[' ']' "/="  */
                                     { (yyval.s) = new string("[]/="); }
    break;

  case 233: /* function_name: "operator" '[' ']' "%="  */
                                     { (yyval.s) = new string("[]%="); }
    break;

  case 234: /* function_name: "operator" '[' ']' "&="  */
                                     { (yyval.s) = new string("[]&="); }
    break;

  case 235: /* function_name: "operator" '[' ']' "|="  */
                                     { (yyval.s) = new string("[]|="); }
    break;

  case 236: /* function_name: "operator" '[' ']' "^="  */
                                     { (yyval.s) = new string("[]^="); }
    break;

  case 237: /* function_name: "operator" '[' ']' "&&="  */
                                        { (yyval.s) = new string("[]&&="); }
    break;

  case 238: /* function_name: "operator" '[' ']' "||="  */
                                        { (yyval.s) = new string("[]||="); }
    break;

  case 239: /* function_name: "operator" '[' ']' "^^="  */
                                        { (yyval.s) = new string("[]^^="); }
    break;

  case 240: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 241: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 242: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 243: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 244: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 245: /* function_name: "operator" '.' "name" "+="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`+="); delete (yyvsp[-1].s); }
    break;

  case 246: /* function_name: "operator" '.' "name" "-="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`-="); delete (yyvsp[-1].s); }
    break;

  case 247: /* function_name: "operator" '.' "name" "*="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`*="); delete (yyvsp[-1].s); }
    break;

  case 248: /* function_name: "operator" '.' "name" "/="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`/="); delete (yyvsp[-1].s); }
    break;

  case 249: /* function_name: "operator" '.' "name" "%="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`%="); delete (yyvsp[-1].s); }
    break;

  case 250: /* function_name: "operator" '.' "name" "&="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&="); delete (yyvsp[-1].s); }
    break;

  case 251: /* function_name: "operator" '.' "name" "|="  */
                                          { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`|="); delete (yyvsp[-1].s); }
    break;

  case 252: /* function_name: "operator" '.' "name" "^="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^="); delete (yyvsp[-1].s); }
    break;

  case 253: /* function_name: "operator" '.' "name" "&&="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&&="); delete (yyvsp[-1].s); }
    break;

  case 254: /* function_name: "operator" '.' "name" "||="  */
                                            { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`||="); delete (yyvsp[-1].s); }
    break;

  case 255: /* function_name: "operator" '.' "name" "^^="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^^="); delete (yyvsp[-1].s); }
    break;

  case 256: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 257: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 258: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 259: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 260: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 261: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 262: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 263: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 264: /* function_name: "operator" "is" das_type_name  */
                                                { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 265: /* function_name: "operator" "as" das_type_name  */
                                                { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 266: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 267: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 268: /* function_name: "operator" '?' "as" das_type_name  */
                                                    { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 269: /* function_name: das_type_name  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 270: /* das_type_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 271: /* das_type_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 272: /* das_type_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 273: /* das_type_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 274: /* das_type_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 275: /* das_type_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 276: /* das_type_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 277: /* das_type_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 278: /* das_type_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 279: /* das_type_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 280: /* das_type_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 281: /* das_type_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 282: /* das_type_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 283: /* das_type_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 284: /* das_type_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 285: /* das_type_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 286: /* das_type_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 287: /* das_type_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 288: /* das_type_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 289: /* das_type_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 290: /* das_type_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 291: /* das_type_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 292: /* das_type_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 293: /* das_type_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 294: /* das_type_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 295: /* das_type_name: "float16"  */
                     { (yyval.s) = new string("float16"); }
    break;

  case 296: /* das_type_name: "half2"  */
                     { (yyval.s) = new string("half2"); }
    break;

  case 297: /* das_type_name: "half3"  */
                     { (yyval.s) = new string("half3"); }
    break;

  case 298: /* das_type_name: "half4"  */
                     { (yyval.s) = new string("half4"); }
    break;

  case 299: /* das_type_name: "half8"  */
                     { (yyval.s) = new string("half8"); }
    break;

  case 300: /* das_type_name: "short2"  */
                     { (yyval.s) = new string("short2"); }
    break;

  case 301: /* das_type_name: "short3"  */
                     { (yyval.s) = new string("short3"); }
    break;

  case 302: /* das_type_name: "short4"  */
                     { (yyval.s) = new string("short4"); }
    break;

  case 303: /* das_type_name: "short8"  */
                     { (yyval.s) = new string("short8"); }
    break;

  case 304: /* das_type_name: "ushort2"  */
                     { (yyval.s) = new string("ushort2"); }
    break;

  case 305: /* das_type_name: "ushort3"  */
                     { (yyval.s) = new string("ushort3"); }
    break;

  case 306: /* das_type_name: "ushort4"  */
                     { (yyval.s) = new string("ushort4"); }
    break;

  case 307: /* das_type_name: "ushort8"  */
                     { (yyval.s) = new string("ushort8"); }
    break;

  case 308: /* das_type_name: "byte2"  */
                     { (yyval.s) = new string("byte2"); }
    break;

  case 309: /* das_type_name: "byte3"  */
                     { (yyval.s) = new string("byte3"); }
    break;

  case 310: /* das_type_name: "byte4"  */
                     { (yyval.s) = new string("byte4"); }
    break;

  case 311: /* das_type_name: "byte8"  */
                     { (yyval.s) = new string("byte8"); }
    break;

  case 312: /* das_type_name: "byte16"  */
                     { (yyval.s) = new string("byte16"); }
    break;

  case 313: /* das_type_name: "ubyte2"  */
                     { (yyval.s) = new string("ubyte2"); }
    break;

  case 314: /* das_type_name: "ubyte3"  */
                     { (yyval.s) = new string("ubyte3"); }
    break;

  case 315: /* das_type_name: "ubyte4"  */
                     { (yyval.s) = new string("ubyte4"); }
    break;

  case 316: /* das_type_name: "ubyte8"  */
                     { (yyval.s) = new string("ubyte8"); }
    break;

  case 317: /* das_type_name: "ubyte16"  */
                     { (yyval.s) = new string("ubyte16"); }
    break;

  case 318: /* optional_template: %empty  */
                                        { (yyval.b) = false; }
    break;

  case 319: /* optional_template: "template"  */
                                        { (yyval.b) = true; }
    break;

  case 320: /* global_function_declaration: optional_annotation_list_with_emit_semis "def" optional_template function_declaration  */
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

  case 321: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 322: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 323: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 324: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 325: /* $@15: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 326: /* function_declaration: optional_public_or_private_function $@15 function_declaration_header optional_emit_semis block_or_simple_block  */
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

  case 327: /* expression_block_finally: %empty  */
        {
        (yyval.pExpression) = nullptr;
    }
    break;

  case 328: /* $@16: %empty  */
                  {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 329: /* $@17: %empty  */
                             {
        yyextra->pop_nesteds();
    }
    break;

  case 330: /* expression_block_finally: "finally" $@16 '{' expressions $@17 '}'  */
          {
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 331: /* $@18: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 332: /* $@19: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 333: /* expression_block: '{' $@18 expressions $@19 '}' expression_block_finally  */
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

  case 334: /* expr_call_pipe_no_bracket: expr_call expr_full_block_assumed_piped  */
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

  case 335: /* expr_call_pipe_no_bracket: expr_method_call_no_bracket expr_full_block_assumed_piped  */
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

  case 336: /* expr_call_pipe_no_bracket: expr_field_no_bracket expr_full_block_assumed_piped  */
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

  case 337: /* expr_call_pipe_no_bracket: expr_named_call expr_full_block_assumed_piped  */
                                                                 {
        // free-function named call + piped block: pad-aware resolution lands it on the block param
        auto nc = (ExprNamedCall *) (yyvsp[-1].pExpression);
        nc->nonNamedArguments.push_back((yyvsp[0].pExpression));
        nc->pipedCallArgument = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 338: /* expression_any: SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 339: /* expression_any: expr_assign_no_bracket SEMICOLON  */
                                                    { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 340: /* expression_any: expression_delete SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 341: /* expression_any: expression_let  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 342: /* expression_any: expression_while_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 343: /* expression_any: expression_unsafe  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 344: /* expression_any: expression_with  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 345: /* expression_any: expression_with_alias SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 346: /* expression_any: expression_for_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 347: /* expression_any: expression_break SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 348: /* expression_any: expression_continue SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 349: /* expression_any: expression_return SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 350: /* expression_any: expression_yield SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 351: /* expression_any: expression_if_then_else  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 352: /* expression_any: expression_if_then_else_oneliner  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 353: /* expression_any: expression_try_catch  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 354: /* expression_any: expression_label SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 355: /* expression_any: expression_goto SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 356: /* expression_any: "pass" SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 357: /* $@20: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 358: /* $@21: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 359: /* expression_any: '{' $@20 expressions $@21 '}' expression_block_finally  */
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

  case 360: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 361: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 362: /* expressions: expressions error  */
                                 {
        (void)(yyvsp[-1].pExpression); /* gc_node — don't delete Expression */ (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 363: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 364: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 365: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 366: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 367: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 368: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 369: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 370: /* name_in_namespace: "name" "::" "name"  */
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

  case 371: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 372: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 373: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 374: /* $@22: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 375: /* $@23: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 376: /* new_type_declaration: '<' $@22 type_declaration '>' $@23  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 377: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 378: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 379: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 380: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 381: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 382: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 383: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 384: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 385: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 386: /* expression_return: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 387: /* expression_return: "return" expr  */
                                      {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 388: /* expression_return: "return" "<-" expr  */
                                             {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 389: /* expression_yield: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 390: /* expression_yield: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 391: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 392: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 393: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 394: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 395: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 396: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 397: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 398: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 399: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 400: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 401: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 402: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                        {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-5]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameList),tokAt(scanner,(yylsp[-5])),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 403: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 404: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 405: /* expression_let: kwd_let optional_in_scope '{' variable_declaration_list '}'  */
                                                                               {
        (yyval.pExpression) = ast_LetList(scanner,(yyvsp[-4].b),(yyvsp[-3].b),*(yyvsp[-1].pVarDeclList),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 406: /* $@24: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 407: /* $@25: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 408: /* expr_cast: "cast" '<' $@24 type_declaration_no_options '>' $@25 '(' expr ')'  */
                                                                                                                                                        {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-8])),(yyvsp[-1].pExpression),(yyvsp[-5].pTypeDecl));
    }
    break;

  case 409: /* $@26: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 410: /* $@27: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 411: /* expr_cast: "upcast" '<' $@26 type_declaration_no_options '>' $@27 '(' expr ')'  */
                                                                                                                                                          {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-8])),(yyvsp[-1].pExpression),(yyvsp[-5].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 412: /* $@28: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 413: /* $@29: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 414: /* expr_cast: "reinterpret" '<' $@28 type_declaration_no_options '>' $@29 '(' expr ')'  */
                                                                                                                                                               {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-8])),(yyvsp[-1].pExpression),(yyvsp[-5].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 415: /* $@30: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 416: /* $@31: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 417: /* expr_type_decl: "type" '<' $@30 type_declaration '>' $@31  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 418: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
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

  case 419: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
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

  case 420: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" c_or_s "name" '>' '(' expr ')'  */
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

  case 421: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 422: /* expr_list: "<-" expr  */
                             {
            (yyval.pExpression) = ast_makeMoveArgument(scanner, (yyvsp[0].pExpression), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 423: /* expr_list: expr_list ',' expr  */
                                        {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 424: /* expr_list: expr_list ',' "<-" expr  */
                                                   {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-3])),(yyvsp[-3].pExpression),ast_makeMoveArgument(scanner, (yyvsp[0].pExpression), tokAt(scanner,(yylsp[0]))));
    }
    break;

  case 425: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 426: /* block_or_simple_block: "=>" expr_no_bracket  */
                                                   {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 427: /* block_or_simple_block: "=>" "<-" expr_no_bracket  */
                                                          {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 428: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 429: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 430: /* block_or_lambda: "@@"  */
                  { (yyval.i) = 2;   /* local function */ }
    break;

  case 431: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 432: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 433: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 434: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 435: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 436: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 437: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 438: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 439: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 440: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis block_or_simple_block  */
                                                                                                                {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 441: /* expr_full_block_assumed_piped: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis block_or_simple_block  */
                                                                                                                {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 442: /* expr_full_block_assumed_piped: '{' expressions '}'  */
                                   {
        // block span is brace-to-brace (@$), not the statements' span (@block)
        (yyval.pExpression) = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,new TypeDecl(Type::autoinfer),(yyvsp[-1].pExpression),tokAt(scanner,(yyloc)),tokAt(scanner,(yyloc)),LineInfo());
    }
    break;

  case 443: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 444: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 445: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 446: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 447: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 448: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 449: /* expr_numeric_const: "float16 constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat16(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 450: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 451: /* expr_assign_no_bracket: expr_no_bracket  */
                                                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 452: /* expr_assign_no_bracket: expr_no_bracket '=' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 453: /* expr_assign_no_bracket: expr_no_bracket "<-" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 454: /* expr_assign_no_bracket: expr_no_bracket "<-" make_table_decl  */
                                                                   { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 455: /* expr_assign_no_bracket: expr_no_bracket "<-" array_comprehension  */
                                                                     { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 456: /* expr_assign_no_bracket: expr_no_bracket ":=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 457: /* expr_assign_no_bracket: expr_no_bracket "&=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 458: /* expr_assign_no_bracket: expr_no_bracket "|=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 459: /* expr_assign_no_bracket: expr_no_bracket "^=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 460: /* expr_assign_no_bracket: expr_no_bracket "&&=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 461: /* expr_assign_no_bracket: expr_no_bracket "||=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 462: /* expr_assign_no_bracket: expr_no_bracket "^^=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 463: /* expr_assign_no_bracket: expr_no_bracket "+=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 464: /* expr_assign_no_bracket: expr_no_bracket "-=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 465: /* expr_assign_no_bracket: expr_no_bracket "*=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 466: /* expr_assign_no_bracket: expr_no_bracket "/=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 467: /* expr_assign_no_bracket: expr_no_bracket "%=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 468: /* expr_assign_no_bracket: expr_no_bracket "<<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 469: /* expr_assign_no_bracket: expr_no_bracket ">>=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 470: /* expr_assign_no_bracket: expr_no_bracket "<<<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 471: /* expr_assign_no_bracket: expr_no_bracket ">>>=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 472: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 473: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 474: /* expr_named_call: name_in_namespace '(' expr_list ',' make_struct_fields ')'  */
                                                                                          {
        // bracket-less mixed named call: foo(pos..., name = value) -- named args are a strict suffix
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-3].pExpression));
        nc->arguments = (yyvsp[-1].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 475: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' ')'  */
                                                                    {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 476: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' expr_list ')'  */
                                                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 477: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                                     {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 478: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' expr_list ',' make_struct_fields ')'  */
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

  case 479: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' make_struct_fields ')'  */
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

  case 480: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 481: /* func_addr_name: "$i" '(' expr ')'  */
                                          {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 482: /* func_addr_expr: "@@" func_addr_name  */
                                            {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 483: /* $@32: %empty  */
                      { yyextra->das_arrow_depth ++; }
    break;

  case 484: /* $@33: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 485: /* func_addr_expr: "@@" '<' $@32 type_declaration_no_options '>' $@33 func_addr_name  */
                                                                                                                                                         {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 486: /* $@34: %empty  */
                      { yyextra->das_arrow_depth ++; }
    break;

  case 487: /* $@35: %empty  */
                                                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 488: /* func_addr_expr: "@@" '<' $@34 optional_function_argument_list optional_function_type '>' $@35 func_addr_name  */
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

  case 489: /* expr_field_no_bracket: expr_no_bracket '.' "name"  */
                                                         {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 490: /* expr_field_no_bracket: expr_no_bracket '.' '.' "name"  */
                                                             {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 491: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' ')'  */
                                                                 {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 492: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' expr_list ')'  */
                                                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 493: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 494: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' expr_list ',' make_struct_fields ')'  */
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

  case 495: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' make_struct_fields ')'  */
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

  case 496: /* expr_field_no_bracket: expr_no_bracket '.' basic_type_declaration '(' ')'  */
                                                                                   {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 497: /* expr_field_no_bracket: expr_no_bracket '.' basic_type_declaration '(' expr_list ')'  */
                                                                                                        {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 498: /* $@36: %empty  */
                                          { yyextra->das_suppress_errors=true; }
    break;

  case 499: /* $@37: %empty  */
                                                                                       { yyextra->das_suppress_errors=false; }
    break;

  case 500: /* expr_field_no_bracket: expr_no_bracket '.' $@36 error $@37  */
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
            dd->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            dd->useInitializer = false;
            dd->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = dd;
    }
    break;

  case 503: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
                                                               {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 504: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
                                                                                 {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-4])),*(yyvsp[-4].s));
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

  case 508: /* expr: expr_no_bracket  */
                                       { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 509: /* expr: make_table_decl  */
                                     { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 510: /* expr: array_comprehension  */
                                     { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 511: /* expr_no_bracket: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 512: /* expr_no_bracket: name_in_namespace  */
                                              { (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 513: /* expr_no_bracket: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 514: /* expr_no_bracket: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 515: /* expr_no_bracket: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 516: /* expr_no_bracket: make_decl_no_bracket  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 517: /* expr_no_bracket: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 518: /* expr_no_bracket: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 519: /* expr_no_bracket: expr_field_no_bracket  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 520: /* expr_no_bracket: expr_mtag_no_bracket  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 521: /* expr_no_bracket: '!' expr_no_bracket  */
                                                         { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",(yyvsp[0].pExpression)); }
    break;

  case 522: /* expr_no_bracket: '~' expr_no_bracket  */
                                                         { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",(yyvsp[0].pExpression)); }
    break;

  case 523: /* expr_no_bracket: '+' expr_no_bracket  */
                                                             { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",(yyvsp[0].pExpression)); }
    break;

  case 524: /* expr_no_bracket: '-' expr_no_bracket  */
                                                             { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",(yyvsp[0].pExpression)); }
    break;

  case 525: /* expr_no_bracket: expr_no_bracket "<<" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 526: /* expr_no_bracket: expr_no_bracket ">>" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 527: /* expr_no_bracket: expr_no_bracket "<<<" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 528: /* expr_no_bracket: expr_no_bracket ">>>" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 529: /* expr_no_bracket: expr_no_bracket '+' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 530: /* expr_no_bracket: expr_no_bracket '-' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 531: /* expr_no_bracket: expr_no_bracket '*' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 532: /* expr_no_bracket: expr_no_bracket '/' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 533: /* expr_no_bracket: expr_no_bracket '%' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 534: /* expr_no_bracket: expr_no_bracket '<' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 535: /* expr_no_bracket: expr_no_bracket '>' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 536: /* expr_no_bracket: expr_no_bracket "==" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 537: /* expr_no_bracket: expr_no_bracket "!=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 538: /* expr_no_bracket: expr_no_bracket "<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 539: /* expr_no_bracket: expr_no_bracket ">=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 540: /* expr_no_bracket: expr_no_bracket '&' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 541: /* expr_no_bracket: expr_no_bracket '|' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 542: /* expr_no_bracket: expr_no_bracket '^' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 543: /* expr_no_bracket: expr_no_bracket "&&" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 544: /* expr_no_bracket: expr_no_bracket "||" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 545: /* expr_no_bracket: expr_no_bracket "^^" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 546: /* expr_no_bracket: expr_no_bracket ".." expr_no_bracket  */
                                                                   {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 547: /* expr_no_bracket: "++" expr_no_bracket  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", (yyvsp[0].pExpression)); }
    break;

  case 548: /* expr_no_bracket: "--" expr_no_bracket  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", (yyvsp[0].pExpression)); }
    break;

  case 549: /* expr_no_bracket: expr_no_bracket "++"  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 550: /* expr_no_bracket: expr_no_bracket "--"  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 551: /* expr_no_bracket: '(' expr_list optional_comma ')'  */
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

  case 552: /* expr_no_bracket: '(' make_struct_single ')'  */
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

  case 553: /* expr_no_bracket: expr_no_bracket '[' expr ']'  */
                                                            { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 554: /* expr_no_bracket: expr_no_bracket '.' '[' expr ']'  */
                                                                { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 555: /* expr_no_bracket: expr_no_bracket "?[" expr ']'  */
                                                            { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 556: /* expr_no_bracket: expr_no_bracket '.' "?[" expr ']'  */
                                                                { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 557: /* expr_no_bracket: expr_no_bracket "?." "name"  */
                                                            { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 558: /* expr_no_bracket: expr_no_bracket '.' "?." "name"  */
                                                                { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 559: /* expr_no_bracket: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 560: /* expr_no_bracket: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 561: /* expr_no_bracket: '*' expr_no_bracket  */
                                                              { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 562: /* expr_no_bracket: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 563: /* expr_no_bracket: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 564: /* $@38: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 565: /* $@39: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 566: /* expr_no_bracket: "addr" '<' $@38 type_declaration_no_options '>' $@39 '(' expr ')'  */
                                                                                                                                                        {
        auto pRef2Ptr = new ExprRef2Ptr(tokAt(scanner,(yylsp[-8])),(yyvsp[-1].pExpression));
        pRef2Ptr->generated = true;
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-8])),pRef2Ptr,(yyvsp[-5].pTypeDecl));
        pCast->reinterpret = true;
        pCast->fromAddrSugar = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 567: /* expr_no_bracket: expr_generator  */
                                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 568: /* expr_no_bracket: expr_no_bracket "??" expr_no_bracket  */
                                                                         { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 569: /* expr_no_bracket: expr_no_bracket '?' expr_no_bracket ':' expr_no_bracket  */
                                                                                           {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        }
    break;

  case 570: /* $@40: %empty  */
                                                          { yyextra->das_arrow_depth ++; }
    break;

  case 571: /* $@41: %empty  */
                                                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 572: /* expr_no_bracket: expr_no_bracket "is" "type" '<' $@40 type_declaration_no_options '>' $@41  */
                                                                                                                                                                  {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 573: /* expr_no_bracket: expr_no_bracket "is" basic_type_declaration  */
                                                                          {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 574: /* expr_no_bracket: expr_no_bracket "is" "name"  */
                                                         {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 575: /* expr_no_bracket: expr_no_bracket "as" "name"  */
                                                         {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 576: /* $@42: %empty  */
                                                          { yyextra->das_arrow_depth ++; }
    break;

  case 577: /* $@43: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 578: /* expr_no_bracket: expr_no_bracket "as" "type" '<' $@42 type_declaration '>' $@43  */
                                                                                                                                                       {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 579: /* expr_no_bracket: expr_no_bracket "as" basic_type_declaration  */
                                                                          {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 580: /* expr_no_bracket: expr_no_bracket '?' "as" "name"  */
                                                             {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 581: /* $@44: %empty  */
                                                              { yyextra->das_arrow_depth ++; }
    break;

  case 582: /* $@45: %empty  */
                                                                                                                          { yyextra->das_arrow_depth --; }
    break;

  case 583: /* expr_no_bracket: expr_no_bracket '?' "as" "type" '<' $@44 type_declaration '>' $@45  */
                                                                                                                                                           {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 584: /* expr_no_bracket: expr_no_bracket '?' "as" basic_type_declaration  */
                                                                              {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 585: /* expr_no_bracket: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 586: /* expr_no_bracket: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 587: /* expr_no_bracket: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 588: /* expr_no_bracket: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 589: /* expr_no_bracket: expr_method_call_no_bracket  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 590: /* expr_no_bracket: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 591: /* expr_no_bracket: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 592: /* expr_no_bracket: expr_no_bracket "<|" expr_no_bracket  */
                                                                      { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1])),true); }
    break;

  case 593: /* expr_no_bracket: expr_no_bracket "|>" expr_no_bracket  */
                                                                      { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 594: /* expr_no_bracket: expr_no_bracket "|>" basic_type_declaration  */
                                                                     {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 595: /* expr_no_bracket: expr_call_pipe_no_bracket  */
                                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 596: /* expr_no_bracket: "unsafe" '(' expr ')'  */
                                         {
            (yyvsp[-1].pExpression)->alwaysSafe = true;
            (yyvsp[-1].pExpression)->userSaidItsSafe = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    break;

  case 597: /* expr_no_bracket: expr_no_bracket "=>" expr_no_bracket  */
                                                               {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 598: /* expr_no_bracket: expr_no_bracket "=>" make_table_decl  */
                                                               {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 599: /* expr_no_bracket: expr_no_bracket "=>" array_comprehension  */
                                                                   {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 600: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 601: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 602: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list optional_emit_semis expression_block  */
                                                                                                                                                  {
        auto closure = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),(yyvsp[0].pExpression));
        ((ExprBlock *)(yyvsp[0].pExpression))->returnType = new TypeDecl(Type::autoinfer);
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),closure,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 603: /* expr_mtag_no_bracket: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 604: /* expr_mtag_no_bracket: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 605: /* expr_mtag_no_bracket: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 606: /* expr_mtag_no_bracket: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 607: /* expr_mtag_no_bracket: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 608: /* expr_mtag_no_bracket: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 609: /* expr_mtag_no_bracket: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 610: /* expr_mtag_no_bracket: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 611: /* expr_mtag_no_bracket: expr_no_bracket '.' "$f" '(' expr ')'  */
                                                                           {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 612: /* expr_mtag_no_bracket: expr_no_bracket "?." "$f" '(' expr ')'  */
                                                                            {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 613: /* expr_mtag_no_bracket: expr_no_bracket '.' '.' "$f" '(' expr ')'  */
                                                                               {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 614: /* expr_mtag_no_bracket: expr_no_bracket '.' "?." "$f" '(' expr ')'  */
                                                                                {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 615: /* expr_mtag_no_bracket: expr_no_bracket "as" "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 616: /* expr_mtag_no_bracket: expr_no_bracket '?' "as" "$f" '(' expr ')'  */
                                                                                  {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 617: /* expr_mtag_no_bracket: expr_no_bracket "is" "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 618: /* expr_mtag_no_bracket: "@@" "$c" '(' expr ')'  */
                                                           {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 619: /* optional_field_annotation: %empty  */
                                      { (yyval.aaList) = nullptr; }
    break;

  case 620: /* optional_field_annotation: metadata_argument_list  */
                                      { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 621: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 622: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 623: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 624: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 625: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 626: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 627: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 628: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 629: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 630: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 631: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 632: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 633: /* struct_variable_declaration_list: struct_variable_declaration_list "new line, semicolon"  */
                                                                 { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 634: /* struct_variable_declaration_list: struct_variable_declaration_list "typedef" "name" '=' type_declaration SEMICOLON  */
                                                                                                                {
        (yyval.pVarDeclList) = (yyvsp[-5].pVarDeclList);
        ast_structureAlias(scanner,(yyvsp[-3].s),(yyvsp[-1].pTypeDecl),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 635: /* $@46: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 636: /* struct_variable_declaration_list: struct_variable_declaration_list $@46 structure_variable_declaration SEMICOLON  */
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

  case 637: /* $@47: %empty  */
                                                                                                                     {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 638: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable "abstract" optional_constant $@47 function_declaration_header SEMICOLON  */
                                                          {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 639: /* $@48: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 640: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@48 function_declaration_header optional_emit_semis block_or_simple_block  */
                                                                                                 {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-10].pVarDeclList),(yyvsp[-9].faList),(yyvsp[-6].b),(yyvsp[-7].b),(yyvsp[-5].i),(yyvsp[-4].b),(yyvsp[-2].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-8]),(yylsp[0])),tokAt(scanner,(yylsp[-9])));
            }
    break;

  case 641: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 642: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 643: /* function_argument_declaration_type: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 644: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 645: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 646: /* function_argument_list: function_argument_declaration_no_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 647: /* function_argument_list: function_argument_declaration_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 648: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 649: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 650: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 651: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 652: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                       { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 653: /* tuple_alias_type_list: %empty  */
      {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 654: /* tuple_alias_type_list: tuple_type  */
                       {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 655: /* tuple_alias_type_list: tuple_alias_type_list semis tuple_type  */
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

  case 656: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 657: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 658: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 659: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 660: /* variant_alias_type_list: variant_type  */
                         {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 661: /* variant_alias_type_list: variant_alias_type_list semis variant_type  */
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

  case 662: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 663: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 664: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 665: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 666: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 667: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 668: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 669: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 670: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 671: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 672: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 673: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 674: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 675: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 676: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 677: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 678: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 679: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 680: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "$i" '(' expr ')'  */
                                                                               {
        (yyvsp[-5].pNameWithPosList)->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = (yyvsp[-5].pNameWithPosList);
    }
    break;

  case 681: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 682: /* global_let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 683: /* global_let_variable_name_with_pos_list: global_let_variable_name_with_pos_list ',' "name"  */
                                                                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 684: /* variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 685: /* variable_declaration_list: variable_declaration_list SEMICOLON  */
                                                  {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 686: /* variable_declaration_list: variable_declaration_list let_variable_declaration  */
                                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
        (yyvsp[-1].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 687: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 688: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 689: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 690: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                         {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 691: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                               {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 692: /* global_let_variable_declaration: global_let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 693: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 694: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 695: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 696: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 697: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 698: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 699: /* global_variable_declaration_list: global_variable_declaration_list SEMICOLON  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 700: /* $@49: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 701: /* global_variable_declaration_list: global_variable_declaration_list $@49 optional_field_annotation let_variable_declaration  */
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

  case 702: /* global_let: kwd_let optional_shared optional_public_or_private_variable '{' global_variable_declaration_list '}'  */
                                                                                                                                       {
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 703: /* $@50: %empty  */
                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 704: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@50 optional_field_annotation global_let_variable_declaration  */
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

  case 705: /* enum_expression: "name"  */
                   {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        delete (yyvsp[0].s);
    }
    break;

  case 706: /* enum_expression: "name" '=' expr  */
                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[-2].s),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-2])));
        delete (yyvsp[-2].s);
    }
    break;

  case 709: /* enum_list: %empty  */
        {
        (yyval.pEnumList) = new Enumeration();
    }
    break;

  case 710: /* enum_list: enum_expression  */
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

  case 711: /* enum_list: enum_list commas enum_expression  */
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

  case 712: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 713: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 714: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 715: /* $@51: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 716: /* single_alias: optional_public_or_private_alias "name" $@51 '=' type_declaration  */
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

  case 719: /* distinct_alias: optional_public_or_private_alias "name" "name" '=' type_declaration  */
                                                                                               {
        if ( *(yyvsp[-3].s) != "distinct" ) {
            das2_yyerror(scanner,"expected 'distinct', got '"+*(yyvsp[-3].s)+"'",tokAt(scanner,(yylsp[-3])),
                CompilationError::invalid_distinct_type);
        } else {
            ast_distinctDeclaration(scanner,(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])),!(yyvsp[-4].b),(yyvsp[0].pTypeDecl));
        }
        delete (yyvsp[-3].s);
        delete (yyvsp[-2].s);
    }
    break;

  case 720: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 721: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 722: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 723: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 724: /* optional_enum_basic_type_declaration: %empty  */
        {
        (yyval.type) = Type::tInt;
    }
    break;

  case 725: /* optional_enum_basic_type_declaration: ':' enum_basic_type_declaration  */
                                              {
        (yyval.type) = (yyvsp[0].type);
    }
    break;

  case 732: /* $@52: %empty  */
                                                                     {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 733: /* $@53: %empty  */
                                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 734: /* $@54: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 735: /* enum_declaration: optional_annotation_list_with_emit_semis "enum" $@52 optional_public_or_private_enum enum_name optional_enum_basic_type_declaration optional_emit_commas '{' $@53 enum_list optional_commas $@54 '}'  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-8].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-12].faList),tokAt(scanner,(yylsp[-12])),(yyvsp[-9].b),(yyvsp[-8].pEnum),(yyvsp[-3].pEnumList),(yyvsp[-7].type));
    }
    break;

  case 736: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 737: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 738: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 739: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 740: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 741: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 742: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 743: /* class_or_struct: "class" "template"  */
                                  { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 744: /* class_or_struct: "struct" "template"  */
                                  { (yyval.i) = CorS_StructTemplate; }
    break;

  case 745: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 746: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 747: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 748: /* optional_struct_variable_declaration_list: ';'  */
            {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 749: /* optional_struct_variable_declaration_list: '{' struct_variable_declaration_list '}'  */
                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 750: /* $@55: %empty  */
                                                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 751: /* $@56: %empty  */
                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 752: /* $@57: %empty  */
                                             {
        if ( (yyvsp[-1].pStructure) ) {
            (yyvsp[-1].pStructure)->isClass = (yyvsp[-4].i)==CorS_Class || (yyvsp[-4].i)==CorS_ClassTemplate;
            (yyvsp[-1].pStructure)->isTemplate = (yyvsp[-4].i)==CorS_ClassTemplate || (yyvsp[-4].i)==CorS_StructTemplate;
            (yyvsp[-1].pStructure)->privateStructure = !(yyvsp[-3].b);
        }
    }
    break;

  case 753: /* structure_declaration: optional_annotation_list_with_emit_semis $@55 class_or_struct optional_public_or_private_structure $@56 structure_name optional_emit_semis $@57 optional_struct_variable_declaration_list  */
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

  case 754: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 755: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 756: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 757: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 758: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "$i" '(' expr ')'  */
                                                                           {
        (yyvsp[-5].pNameWithPosList)->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = (yyvsp[-5].pNameWithPosList);
    }
    break;

  case 759: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 760: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 761: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 762: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 763: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 764: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 765: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 766: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 767: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 768: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 769: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 770: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 771: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 772: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 773: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 774: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 775: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 776: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 777: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 778: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 779: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 780: /* basic_type_declaration: "float16"  */
                        { (yyval.type) = Type::tFloat16; }
    break;

  case 781: /* basic_type_declaration: "half2"  */
                        { (yyval.type) = Type::tHalf2; }
    break;

  case 782: /* basic_type_declaration: "half3"  */
                        { (yyval.type) = Type::tHalf3; }
    break;

  case 783: /* basic_type_declaration: "half4"  */
                        { (yyval.type) = Type::tHalf4; }
    break;

  case 784: /* basic_type_declaration: "half8"  */
                        { (yyval.type) = Type::tHalf8; }
    break;

  case 785: /* basic_type_declaration: "short2"  */
                        { (yyval.type) = Type::tShort2; }
    break;

  case 786: /* basic_type_declaration: "short3"  */
                        { (yyval.type) = Type::tShort3; }
    break;

  case 787: /* basic_type_declaration: "short4"  */
                        { (yyval.type) = Type::tShort4; }
    break;

  case 788: /* basic_type_declaration: "short8"  */
                        { (yyval.type) = Type::tShort8; }
    break;

  case 789: /* basic_type_declaration: "ushort2"  */
                        { (yyval.type) = Type::tUShort2; }
    break;

  case 790: /* basic_type_declaration: "ushort3"  */
                        { (yyval.type) = Type::tUShort3; }
    break;

  case 791: /* basic_type_declaration: "ushort4"  */
                        { (yyval.type) = Type::tUShort4; }
    break;

  case 792: /* basic_type_declaration: "ushort8"  */
                        { (yyval.type) = Type::tUShort8; }
    break;

  case 793: /* basic_type_declaration: "byte2"  */
                        { (yyval.type) = Type::tByte2; }
    break;

  case 794: /* basic_type_declaration: "byte3"  */
                        { (yyval.type) = Type::tByte3; }
    break;

  case 795: /* basic_type_declaration: "byte4"  */
                        { (yyval.type) = Type::tByte4; }
    break;

  case 796: /* basic_type_declaration: "byte8"  */
                        { (yyval.type) = Type::tByte8; }
    break;

  case 797: /* basic_type_declaration: "byte16"  */
                        { (yyval.type) = Type::tByte16; }
    break;

  case 798: /* basic_type_declaration: "ubyte2"  */
                        { (yyval.type) = Type::tUByte2; }
    break;

  case 799: /* basic_type_declaration: "ubyte3"  */
                        { (yyval.type) = Type::tUByte3; }
    break;

  case 800: /* basic_type_declaration: "ubyte4"  */
                        { (yyval.type) = Type::tUByte4; }
    break;

  case 801: /* basic_type_declaration: "ubyte8"  */
                        { (yyval.type) = Type::tUByte8; }
    break;

  case 802: /* basic_type_declaration: "ubyte16"  */
                        { (yyval.type) = Type::tUByte16; }
    break;

  case 803: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 804: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 805: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 806: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 807: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 808: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 809: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 810: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 811: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 812: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 813: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 814: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 815: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 816: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 817: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 818: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 819: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 820: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 821: /* auto_type_declaration: "$t" '(' expr ')'  */
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

  case 822: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 823: /* bitfield_bits: bitfield_bits ';' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 824: /* bitfield_bits: bitfield_bits ',' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 825: /* bitfield_alias_bits: %empty  */
        {
        auto pSL = new vector<tuple<string,Expression *>>();
        (yyval.pNameExprList) = pSL;

    }
    break;

  case 826: /* bitfield_alias_bits: "name"  */
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

  case 827: /* bitfield_alias_bits: "name" '=' expr  */
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

  case 828: /* bitfield_alias_bits: bitfield_alias_bits commas "name"  */
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

  case 829: /* bitfield_alias_bits: bitfield_alias_bits commas "name" '=' expr  */
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

  case 830: /* bitfield_basic_type_declaration: %empty  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 831: /* bitfield_basic_type_declaration: ':' "uint8"  */
                             { (yyval.type) = Type::tBitfield8; }
    break;

  case 832: /* bitfield_basic_type_declaration: ':' "uint16"  */
                             { (yyval.type) = Type::tBitfield16; }
    break;

  case 833: /* bitfield_basic_type_declaration: ':' "uint"  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 834: /* bitfield_basic_type_declaration: ':' "uint64"  */
                             { (yyval.type) = Type::tBitfield64; }
    break;

  case 835: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' '>'  */
                                                                          {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-2].type));
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-2]));
    }
    break;

  case 836: /* $@58: %empty  */
                                                                     { yyextra->das_arrow_depth ++; }
    break;

  case 837: /* $@59: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 838: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' $@58 bitfield_bits '>' $@59  */
                                                                                                                                                             {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-5].type));
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

  case 841: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 842: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 843: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = appendDimExpr(nullptr, (yyvsp[-1].pExpression), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 844: /* dim_list: '[' ']'  */
                {
        (yyval.pTypeDecl) = appendDimExpr(nullptr, nullptr, tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 845: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = appendDimExpr((yyvsp[-3].pTypeDecl), (yyvsp[-1].pExpression), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 846: /* dim_list: dim_list '[' ']'  */
                              {
        (yyval.pTypeDecl) = appendDimExpr((yyvsp[-2].pTypeDecl), nullptr, tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 847: /* type_declaration_no_options: type_declaration_no_options_no_dim  */
                                                     {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 848: /* type_declaration_no_options: type_declaration_no_options_no_dim dim_list  */
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

  case 849: /* optional_expr_list_in_braces: %empty  */
            { (yyval.pExpression) = nullptr; }
    break;

  case 850: /* optional_expr_list_in_braces: '(' expr_list optional_comma ')'  */
                                                { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 851: /* type_declaration_no_options_no_dim: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 852: /* type_declaration_no_options_no_dim: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 853: /* type_declaration_no_options_no_dim: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 854: /* type_declaration_no_options_no_dim: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 855: /* $@60: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 856: /* $@61: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 857: /* type_declaration_no_options_no_dim: "type" '<' $@60 type_declaration '>' $@61  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 858: /* type_declaration_no_options_no_dim: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->typeMacroExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 859: /* type_declaration_no_options_no_dim: name_in_namespace '(' optional_expr_list ')'  */
                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]), (yylsp[-1]));
        (yyval.pTypeDecl)->typeMacroExpr = sequenceToList((yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-3])), *(yyvsp[-3].s)));
        delete (yyvsp[-3].s);
    }
    break;

  case 860: /* type_declaration_no_options_no_dim: '$' name_in_namespace optional_expr_list_in_braces  */
                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-1]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = sequenceToList((yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-1])), *(yyvsp[-1].s)));
        delete (yyvsp[-1].s);
    }
    break;

  case 861: /* $@62: %empty  */
                                    { yyextra->das_arrow_depth ++; }
    break;

  case 862: /* type_declaration_no_options_no_dim: name_in_namespace '<' $@62 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 863: /* $@63: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 864: /* type_declaration_no_options_no_dim: '$' name_in_namespace '<' $@63 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 865: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 866: /* type_declaration_no_options_no_dim: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 867: /* type_declaration_no_options_no_dim: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 868: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 869: /* type_declaration_no_options_no_dim: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 870: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 871: /* type_declaration_no_options_no_dim: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 872: /* type_declaration_no_options_no_dim: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 873: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 874: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 875: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 876: /* type_declaration_no_options_no_dim: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 877: /* $@64: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 878: /* $@65: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 879: /* type_declaration_no_options_no_dim: "smart_ptr" '<' $@64 type_declaration '>' $@65  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 880: /* type_declaration_no_options_no_dim: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 881: /* $@66: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 882: /* $@67: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 883: /* type_declaration_no_options_no_dim: "array" '<' $@66 type_declaration '>' $@67  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 884: /* $@68: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 885: /* $@69: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 886: /* type_declaration_no_options_no_dim: "table" '<' $@68 table_type_pair '>' $@69  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 887: /* $@70: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 888: /* $@71: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 889: /* type_declaration_no_options_no_dim: "iterator" '<' $@70 type_declaration '>' $@71  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 890: /* type_declaration_no_options_no_dim: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 891: /* $@72: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 892: /* $@73: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 893: /* type_declaration_no_options_no_dim: "block" '<' $@72 type_declaration '>' $@73  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 894: /* $@74: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 895: /* $@75: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 896: /* type_declaration_no_options_no_dim: "block" '<' $@74 optional_function_argument_list optional_function_type '>' $@75  */
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

  case 897: /* type_declaration_no_options_no_dim: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 898: /* $@76: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 899: /* $@77: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 900: /* type_declaration_no_options_no_dim: "function" '<' $@76 type_declaration '>' $@77  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 901: /* $@78: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 902: /* $@79: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 903: /* type_declaration_no_options_no_dim: "function" '<' $@78 optional_function_argument_list optional_function_type '>' $@79  */
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

  case 904: /* type_declaration_no_options_no_dim: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 905: /* $@80: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 906: /* $@81: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 907: /* type_declaration_no_options_no_dim: "lambda" '<' $@80 type_declaration '>' $@81  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 908: /* $@82: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 909: /* $@83: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 910: /* type_declaration_no_options_no_dim: "lambda" '<' $@82 optional_function_argument_list optional_function_type '>' $@83  */
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

  case 911: /* $@84: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 912: /* $@85: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 913: /* type_declaration_no_options_no_dim: "tuple" '<' $@84 tuple_type_list '>' $@85  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 914: /* $@86: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 915: /* $@87: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 916: /* type_declaration_no_options_no_dim: "variant" '<' $@86 variant_type_list '>' $@87  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 917: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 918: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 919: /* type_declaration: type_declaration '|' '#'  */
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

  case 920: /* $@88: %empty  */
                   {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 921: /* $@89: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 922: /* $@90: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 923: /* $@91: %empty  */
                                                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 924: /* tuple_alias_declaration: "tuple" $@88 optional_public_or_private_alias "name" optional_emit_semis $@89 '{' $@90 tuple_alias_type_list optional_semis $@91 '}'  */
          {
        auto vtype = new TypeDecl(Type::tTuple);
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

  case 925: /* $@92: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 926: /* $@93: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 927: /* $@94: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 928: /* $@95: %empty  */
                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 929: /* variant_alias_declaration: "variant" $@92 optional_public_or_private_alias "name" optional_emit_semis $@93 '{' $@94 variant_alias_type_list optional_semis $@95 '}'  */
          {
        auto vtype = new TypeDecl(Type::tVariant);
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

  case 930: /* $@96: %empty  */
                      {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 931: /* $@97: %empty  */
                                                                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 932: /* $@98: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 933: /* $@99: %empty  */
                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-7]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 934: /* bitfield_alias_declaration: "bitfield" $@96 optional_public_or_private_alias "name" bitfield_basic_type_declaration optional_emit_commas $@97 '{' $@98 bitfield_alias_bits optional_commas $@99 '}'  */
          {
        auto btype = new TypeDecl((yyvsp[-8].type));
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

  case 935: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 936: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 937: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 938: /* make_decl: make_table_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 939: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 940: /* make_decl: table_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 941: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 942: /* make_decl_no_bracket: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 943: /* make_decl_no_bracket: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 944: /* make_decl_no_bracket: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 945: /* make_decl_no_bracket: table_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 946: /* make_decl_no_bracket: make_table_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 947: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 948: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 949: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 950: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 951: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 952: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 953: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 954: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 955: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 956: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 957: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 958: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 959: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 960: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 961: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 962: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 963: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 964: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 965: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 966: /* $@100: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 967: /* $@101: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 968: /* make_struct_decl: "struct" '<' $@100 type_declaration_no_options '>' $@101 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                      {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 969: /* $@102: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 970: /* $@103: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 971: /* make_struct_decl: "class" '<' $@102 type_declaration_no_options '>' $@103 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                     {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 972: /* $@104: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 973: /* $@105: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 974: /* make_struct_decl: "variant" '<' $@104 variant_type_list '>' $@105 '(' use_initializer make_variant_dim ')'  */
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

  case 975: /* $@106: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 976: /* $@107: %empty  */
                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 977: /* make_struct_decl: "variant" "type" '<' $@106 type_declaration_no_options '>' $@107 '(' use_initializer make_variant_dim ')'  */
                                                                                                                                                                                                    {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-10]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 978: /* $@108: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 979: /* $@109: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 980: /* make_struct_decl: "default" '<' $@108 type_declaration_no_options '>' $@109 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 981: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = new TypeDecl(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 982: /* $@110: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 983: /* $@111: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 984: /* make_tuple_call: "tuple" '<' $@110 tuple_type_list '>' $@111 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 985: /* make_dim_decl: '[' optional_expr_list ']'  */
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

  case 986: /* $@112: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 987: /* $@113: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 988: /* make_dim_decl: "array" "struct" '<' $@112 type_declaration_no_options '>' $@113 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 989: /* $@114: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 990: /* $@115: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 991: /* make_dim_decl: "array" "tuple" '<' $@114 tuple_type_list '>' $@115 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 992: /* $@116: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 993: /* $@117: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 994: /* make_dim_decl: "array" "variant" '<' $@116 variant_type_list '>' $@117 '(' make_variant_dim ')'  */
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

  case 995: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
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

  case 996: /* $@118: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 997: /* $@119: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 998: /* make_dim_decl: "array" '<' $@118 type_declaration_no_options '>' $@119 '(' optional_expr_list ')'  */
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

  case 999: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 1000: /* $@120: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 1001: /* $@121: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 1002: /* make_dim_decl: "fixed_array" '<' $@120 type_declaration_no_options '>' $@121 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 1003: /* expr_map_tuple_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 1004: /* expr_map_tuple_list: expr_map_tuple_list ',' expr  */
                                                      {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 1005: /* push_table_nesting: %empty  */
                    {
        yyextra->das_nested_parentheses ++;
    }
    break;

  case 1006: /* make_table_decl: '{' push_table_nesting optional_emit_semis optional_expr_map_tuple_list '}'  */
                                                                                                     {
        yyextra->das_nested_parentheses --;
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = new TypeDecl(Type::autoinfer);
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto mks = new ExprMakeStruct();
            mks->at = tokAt(scanner,(yylsp[-4]));
            mks->makeType = new TypeDecl(Type::tTable);
            mks->makeType->firstType = new TypeDecl(Type::autoinfer);
            mks->makeType->secondType = new TypeDecl(Type::autoinfer);
            mks->useInitializer = true;
            mks->alwaysUseInitializer = true;
            (yyval.pExpression) = mks;
        }
    }
    break;

  case 1007: /* make_table_call: "table" '(' expr_map_tuple_list optional_comma ')'  */
                                                                             {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 1008: /* make_table_call: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 1009: /* make_table_call: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 1010: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 1011: /* array_comprehension_where: ';' "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 1012: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 1013: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 1014: /* table_comprehension: '[' "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                               {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 1015: /* table_comprehension: '[' "iterator" "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 1016: /* array_comprehension: '{' push_table_nesting optional_emit_semis "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where '}'  */
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


