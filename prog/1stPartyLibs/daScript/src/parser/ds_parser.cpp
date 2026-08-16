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
    #include "lex.yy.h"

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
  YYSYMBOL_SEMICOLON = 188,                /* "end of line"  */
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
  YYSYMBOL_215_ = 215,                     /* ','  */
  YYSYMBOL_216_ = 216,                     /* '='  */
  YYSYMBOL_217_ = 217,                     /* '?'  */
  YYSYMBOL_218_ = 218,                     /* ':'  */
  YYSYMBOL_219_ = 219,                     /* '|'  */
  YYSYMBOL_220_ = 220,                     /* '^'  */
  YYSYMBOL_221_ = 221,                     /* '&'  */
  YYSYMBOL_222_ = 222,                     /* '<'  */
  YYSYMBOL_223_ = 223,                     /* '>'  */
  YYSYMBOL_224_ = 224,                     /* '-'  */
  YYSYMBOL_225_ = 225,                     /* '+'  */
  YYSYMBOL_226_ = 226,                     /* '*'  */
  YYSYMBOL_227_ = 227,                     /* '/'  */
  YYSYMBOL_228_ = 228,                     /* '%'  */
  YYSYMBOL_UNARY_MINUS = 229,              /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 230,               /* UNARY_PLUS  */
  YYSYMBOL_231_ = 231,                     /* '~'  */
  YYSYMBOL_232_ = 232,                     /* '!'  */
  YYSYMBOL_PRE_INC = 233,                  /* PRE_INC  */
  YYSYMBOL_PRE_DEC = 234,                  /* PRE_DEC  */
  YYSYMBOL_POST_INC = 235,                 /* POST_INC  */
  YYSYMBOL_POST_DEC = 236,                 /* POST_DEC  */
  YYSYMBOL_DEREF = 237,                    /* DEREF  */
  YYSYMBOL_238_ = 238,                     /* '.'  */
  YYSYMBOL_239_ = 239,                     /* '['  */
  YYSYMBOL_240_ = 240,                     /* ']'  */
  YYSYMBOL_241_ = 241,                     /* '('  */
  YYSYMBOL_242_ = 242,                     /* ')'  */
  YYSYMBOL_243_ = 243,                     /* '$'  */
  YYSYMBOL_244_ = 244,                     /* '@'  */
  YYSYMBOL_245_ = 245,                     /* '#'  */
  YYSYMBOL_YYACCEPT = 246,                 /* $accept  */
  YYSYMBOL_program = 247,                  /* program  */
  YYSYMBOL_top_level_reader_macro = 248,   /* top_level_reader_macro  */
  YYSYMBOL_optional_public_or_private_module = 249, /* optional_public_or_private_module  */
  YYSYMBOL_module_name = 250,              /* module_name  */
  YYSYMBOL_optional_not_required = 251,    /* optional_not_required  */
  YYSYMBOL_module_declaration = 252,       /* module_declaration  */
  YYSYMBOL_character_sequence = 253,       /* character_sequence  */
  YYSYMBOL_string_constant = 254,          /* string_constant  */
  YYSYMBOL_format_string = 255,            /* format_string  */
  YYSYMBOL_optional_format_string = 256,   /* optional_format_string  */
  YYSYMBOL_257_1 = 257,                    /* $@1  */
  YYSYMBOL_string_builder_body = 258,      /* string_builder_body  */
  YYSYMBOL_string_builder = 259,           /* string_builder  */
  YYSYMBOL_reader_character_sequence = 260, /* reader_character_sequence  */
  YYSYMBOL_expr_reader = 261,              /* expr_reader  */
  YYSYMBOL_262_2 = 262,                    /* $@2  */
  YYSYMBOL_options_declaration = 263,      /* options_declaration  */
  YYSYMBOL_require_declaration = 264,      /* require_declaration  */
  YYSYMBOL_keyword_or_name = 265,          /* keyword_or_name  */
  YYSYMBOL_require_module_name = 266,      /* require_module_name  */
  YYSYMBOL_require_module = 267,           /* require_module  */
  YYSYMBOL_is_public_module = 268,         /* is_public_module  */
  YYSYMBOL_expect_declaration = 269,       /* expect_declaration  */
  YYSYMBOL_expect_list = 270,              /* expect_list  */
  YYSYMBOL_expect_error = 271,             /* expect_error  */
  YYSYMBOL_expression_label = 272,         /* expression_label  */
  YYSYMBOL_expression_goto = 273,          /* expression_goto  */
  YYSYMBOL_elif_or_static_elif = 274,      /* elif_or_static_elif  */
  YYSYMBOL_expression_else = 275,          /* expression_else  */
  YYSYMBOL_semicolon = 276,                /* semicolon  */
  YYSYMBOL_if_or_static_if = 277,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 278, /* expression_else_one_liner  */
  YYSYMBOL_279_3 = 279,                    /* $@3  */
  YYSYMBOL_expression_if_one_liner = 280,  /* expression_if_one_liner  */
  YYSYMBOL_expression_if_then_else = 281,  /* expression_if_then_else  */
  YYSYMBOL_282_4 = 282,                    /* $@4  */
  YYSYMBOL_expression_for_loop = 283,      /* expression_for_loop  */
  YYSYMBOL_284_5 = 284,                    /* $@5  */
  YYSYMBOL_expression_unsafe = 285,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 286,    /* expression_while_loop  */
  YYSYMBOL_expression_with = 287,          /* expression_with  */
  YYSYMBOL_expression_with_alias = 288,    /* expression_with_alias  */
  YYSYMBOL_289_6 = 289,                    /* $@6  */
  YYSYMBOL_290_7 = 290,                    /* $@7  */
  YYSYMBOL_annotation_argument_value = 291, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 292, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 293, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 294,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 295, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 296,   /* metadata_argument_list  */
  YYSYMBOL_annotation_declaration_name = 297, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 298, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 299,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 300,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 301, /* optional_annotation_list  */
  YYSYMBOL_optional_function_argument_list = 302, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 303,   /* optional_function_type  */
  YYSYMBOL_function_name = 304,            /* function_name  */
  YYSYMBOL_optional_template = 305,        /* optional_template  */
  YYSYMBOL_global_function_declaration = 306, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 307, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 308, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 309,     /* function_declaration  */
  YYSYMBOL_310_8 = 310,                    /* $@8  */
  YYSYMBOL_open_block = 311,               /* open_block  */
  YYSYMBOL_close_block = 312,              /* close_block  */
  YYSYMBOL_expression_block = 313,         /* expression_block  */
  YYSYMBOL_expr_call_pipe = 314,           /* expr_call_pipe  */
  YYSYMBOL_expression_any = 315,           /* expression_any  */
  YYSYMBOL_expressions = 316,              /* expressions  */
  YYSYMBOL_expr_keyword = 317,             /* expr_keyword  */
  YYSYMBOL_optional_expr_list = 318,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_list_in_braces = 319, /* optional_expr_list_in_braces  */
  YYSYMBOL_optional_expr_map_tuple_list = 320, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 321, /* type_declaration_no_options_list  */
  YYSYMBOL_expression_keyword = 322,       /* expression_keyword  */
  YYSYMBOL_323_9 = 323,                    /* $@9  */
  YYSYMBOL_324_10 = 324,                   /* $@10  */
  YYSYMBOL_325_11 = 325,                   /* $@11  */
  YYSYMBOL_326_12 = 326,                   /* $@12  */
  YYSYMBOL_expr_pipe = 327,                /* expr_pipe  */
  YYSYMBOL_name_in_namespace = 328,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 329,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 330,     /* new_type_declaration  */
  YYSYMBOL_331_13 = 331,                   /* $@13  */
  YYSYMBOL_332_14 = 332,                   /* $@14  */
  YYSYMBOL_expr_new = 333,                 /* expr_new  */
  YYSYMBOL_expression_break = 334,         /* expression_break  */
  YYSYMBOL_expression_continue = 335,      /* expression_continue  */
  YYSYMBOL_expression_return_no_pipe = 336, /* expression_return_no_pipe  */
  YYSYMBOL_expression_return = 337,        /* expression_return  */
  YYSYMBOL_expression_yield_no_pipe = 338, /* expression_yield_no_pipe  */
  YYSYMBOL_expression_yield = 339,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 340,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 341,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 342,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 343,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 344,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 345, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 346,           /* expression_let  */
  YYSYMBOL_expr_cast = 347,                /* expr_cast  */
  YYSYMBOL_348_15 = 348,                   /* $@15  */
  YYSYMBOL_349_16 = 349,                   /* $@16  */
  YYSYMBOL_350_17 = 350,                   /* $@17  */
  YYSYMBOL_351_18 = 351,                   /* $@18  */
  YYSYMBOL_352_19 = 352,                   /* $@19  */
  YYSYMBOL_353_20 = 353,                   /* $@20  */
  YYSYMBOL_expr_type_decl = 354,           /* expr_type_decl  */
  YYSYMBOL_355_21 = 355,                   /* $@21  */
  YYSYMBOL_356_22 = 356,                   /* $@22  */
  YYSYMBOL_expr_type_info = 357,           /* expr_type_info  */
  YYSYMBOL_expr_list = 358,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 359,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 360,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 361,            /* capture_entry  */
  YYSYMBOL_capture_list = 362,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 363,    /* optional_capture_list  */
  YYSYMBOL_expr_block = 364,               /* expr_block  */
  YYSYMBOL_expr_full_block = 365,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 366, /* expr_full_block_assumed_piped  */
  YYSYMBOL_367_23 = 367,                   /* $@23  */
  YYSYMBOL_expr_numeric_const = 368,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign = 369,              /* expr_assign  */
  YYSYMBOL_expr_assign_pipe_right = 370,   /* expr_assign_pipe_right  */
  YYSYMBOL_expr_assign_pipe = 371,         /* expr_assign_pipe  */
  YYSYMBOL_expr_named_call = 372,          /* expr_named_call  */
  YYSYMBOL_expr_method_call = 373,         /* expr_method_call  */
  YYSYMBOL_func_addr_name = 374,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 375,           /* func_addr_expr  */
  YYSYMBOL_376_24 = 376,                   /* $@24  */
  YYSYMBOL_377_25 = 377,                   /* $@25  */
  YYSYMBOL_378_26 = 378,                   /* $@26  */
  YYSYMBOL_379_27 = 379,                   /* $@27  */
  YYSYMBOL_expr_field = 380,               /* expr_field  */
  YYSYMBOL_381_28 = 381,                   /* $@28  */
  YYSYMBOL_382_29 = 382,                   /* $@29  */
  YYSYMBOL_expr_call = 383,                /* expr_call  */
  YYSYMBOL_expr = 384,                     /* expr  */
  YYSYMBOL_385_30 = 385,                   /* $@30  */
  YYSYMBOL_386_31 = 386,                   /* $@31  */
  YYSYMBOL_387_32 = 387,                   /* $@32  */
  YYSYMBOL_388_33 = 388,                   /* $@33  */
  YYSYMBOL_389_34 = 389,                   /* $@34  */
  YYSYMBOL_390_35 = 390,                   /* $@35  */
  YYSYMBOL_expr_mtag = 391,                /* expr_mtag  */
  YYSYMBOL_optional_field_annotation = 392, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 393,        /* optional_override  */
  YYSYMBOL_optional_constant = 394,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 395, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 396, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 397, /* structure_variable_declaration  */
  YYSYMBOL_struct_variable_declaration_list = 398, /* struct_variable_declaration_list  */
  YYSYMBOL_399_36 = 399,                   /* $@36  */
  YYSYMBOL_400_37 = 400,                   /* $@37  */
  YYSYMBOL_401_38 = 401,                   /* $@38  */
  YYSYMBOL_402_39 = 402,                   /* $@39  */
  YYSYMBOL_function_argument_declaration_no_type = 403, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 404, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 405,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 406,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 407,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 408,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 409,             /* variant_type  */
  YYSYMBOL_variant_type_list = 410,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 411,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 412,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 413, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 414, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 415,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 416,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 417,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 418, /* let_variable_name_with_pos_list  */
  YYSYMBOL_let_variable_declaration = 419, /* let_variable_declaration  */
  YYSYMBOL_global_variable_declaration_list = 420, /* global_variable_declaration_list  */
  YYSYMBOL_421_40 = 421,                   /* $@40  */
  YYSYMBOL_optional_shared = 422,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 423, /* optional_public_or_private_variable  */
  YYSYMBOL_global_let = 424,               /* global_let  */
  YYSYMBOL_425_41 = 425,                   /* $@41  */
  YYSYMBOL_enum_list = 426,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 427, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 428,             /* single_alias  */
  YYSYMBOL_429_42 = 429,                   /* $@42  */
  YYSYMBOL_alias_list = 430,               /* alias_list  */
  YYSYMBOL_alias_declaration = 431,        /* alias_declaration  */
  YYSYMBOL_432_43 = 432,                   /* $@43  */
  YYSYMBOL_optional_public_or_private_enum = 433, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 434,                /* enum_name  */
  YYSYMBOL_enum_declaration = 435,         /* enum_declaration  */
  YYSYMBOL_436_44 = 436,                   /* $@44  */
  YYSYMBOL_437_45 = 437,                   /* $@45  */
  YYSYMBOL_438_46 = 438,                   /* $@46  */
  YYSYMBOL_439_47 = 439,                   /* $@47  */
  YYSYMBOL_optional_structure_parent = 440, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 441,          /* optional_sealed  */
  YYSYMBOL_structure_name = 442,           /* structure_name  */
  YYSYMBOL_class_or_struct = 443,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 444, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 445, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 446,    /* structure_declaration  */
  YYSYMBOL_447_48 = 447,                   /* $@48  */
  YYSYMBOL_448_49 = 448,                   /* $@49  */
  YYSYMBOL_variable_name_with_pos_list = 449, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 450,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 451, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 452, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 453,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 454,            /* bitfield_bits  */
  YYSYMBOL_bitfield_alias_bits = 455,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_basic_type_declaration = 456, /* bitfield_basic_type_declaration  */
  YYSYMBOL_bitfield_type_declaration = 457, /* bitfield_type_declaration  */
  YYSYMBOL_458_50 = 458,                   /* $@50  */
  YYSYMBOL_459_51 = 459,                   /* $@51  */
  YYSYMBOL_c_or_s = 460,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 461,          /* table_type_pair  */
  YYSYMBOL_dim_list = 462,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 463, /* type_declaration_no_options  */
  YYSYMBOL_464_52 = 464,                   /* $@52  */
  YYSYMBOL_465_53 = 465,                   /* $@53  */
  YYSYMBOL_466_54 = 466,                   /* $@54  */
  YYSYMBOL_467_55 = 467,                   /* $@55  */
  YYSYMBOL_468_56 = 468,                   /* $@56  */
  YYSYMBOL_469_57 = 469,                   /* $@57  */
  YYSYMBOL_470_58 = 470,                   /* $@58  */
  YYSYMBOL_471_59 = 471,                   /* $@59  */
  YYSYMBOL_472_60 = 472,                   /* $@60  */
  YYSYMBOL_473_61 = 473,                   /* $@61  */
  YYSYMBOL_474_62 = 474,                   /* $@62  */
  YYSYMBOL_475_63 = 475,                   /* $@63  */
  YYSYMBOL_476_64 = 476,                   /* $@64  */
  YYSYMBOL_477_65 = 477,                   /* $@65  */
  YYSYMBOL_478_66 = 478,                   /* $@66  */
  YYSYMBOL_479_67 = 479,                   /* $@67  */
  YYSYMBOL_480_68 = 480,                   /* $@68  */
  YYSYMBOL_481_69 = 481,                   /* $@69  */
  YYSYMBOL_482_70 = 482,                   /* $@70  */
  YYSYMBOL_483_71 = 483,                   /* $@71  */
  YYSYMBOL_484_72 = 484,                   /* $@72  */
  YYSYMBOL_485_73 = 485,                   /* $@73  */
  YYSYMBOL_486_74 = 486,                   /* $@74  */
  YYSYMBOL_487_75 = 487,                   /* $@75  */
  YYSYMBOL_488_76 = 488,                   /* $@76  */
  YYSYMBOL_489_77 = 489,                   /* $@77  */
  YYSYMBOL_490_78 = 490,                   /* $@78  */
  YYSYMBOL_type_declaration = 491,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 492,  /* tuple_alias_declaration  */
  YYSYMBOL_493_79 = 493,                   /* $@79  */
  YYSYMBOL_494_80 = 494,                   /* $@80  */
  YYSYMBOL_495_81 = 495,                   /* $@81  */
  YYSYMBOL_496_82 = 496,                   /* $@82  */
  YYSYMBOL_variant_alias_declaration = 497, /* variant_alias_declaration  */
  YYSYMBOL_498_83 = 498,                   /* $@83  */
  YYSYMBOL_499_84 = 499,                   /* $@84  */
  YYSYMBOL_500_85 = 500,                   /* $@85  */
  YYSYMBOL_501_86 = 501,                   /* $@86  */
  YYSYMBOL_bitfield_alias_declaration = 502, /* bitfield_alias_declaration  */
  YYSYMBOL_503_87 = 503,                   /* $@87  */
  YYSYMBOL_504_88 = 504,                   /* $@88  */
  YYSYMBOL_505_89 = 505,                   /* $@89  */
  YYSYMBOL_506_90 = 506,                   /* $@90  */
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
  YYSYMBOL_523_91 = 523,                   /* $@91  */
  YYSYMBOL_524_92 = 524,                   /* $@92  */
  YYSYMBOL_525_93 = 525,                   /* $@93  */
  YYSYMBOL_526_94 = 526,                   /* $@94  */
  YYSYMBOL_527_95 = 527,                   /* $@95  */
  YYSYMBOL_528_96 = 528,                   /* $@96  */
  YYSYMBOL_529_97 = 529,                   /* $@97  */
  YYSYMBOL_530_98 = 530,                   /* $@98  */
  YYSYMBOL_make_tuple = 531,               /* make_tuple  */
  YYSYMBOL_make_map_tuple = 532,           /* make_map_tuple  */
  YYSYMBOL_make_tuple_call = 533,          /* make_tuple_call  */
  YYSYMBOL_534_99 = 534,                   /* $@99  */
  YYSYMBOL_535_100 = 535,                  /* $@100  */
  YYSYMBOL_make_dim = 536,                 /* make_dim  */
  YYSYMBOL_make_dim_decl = 537,            /* make_dim_decl  */
  YYSYMBOL_538_101 = 538,                  /* $@101  */
  YYSYMBOL_539_102 = 539,                  /* $@102  */
  YYSYMBOL_540_103 = 540,                  /* $@103  */
  YYSYMBOL_541_104 = 541,                  /* $@104  */
  YYSYMBOL_542_105 = 542,                  /* $@105  */
  YYSYMBOL_543_106 = 543,                  /* $@106  */
  YYSYMBOL_544_107 = 544,                  /* $@107  */
  YYSYMBOL_545_108 = 545,                  /* $@108  */
  YYSYMBOL_546_109 = 546,                  /* $@109  */
  YYSYMBOL_547_110 = 547,                  /* $@110  */
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
#define YYLAST   16272

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  246
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  308
/* YYNRULES -- Number of rules.  */
#define YYNRULES  1013
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1844

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   473


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
       2,     2,     2,   232,     2,   245,   243,   228,   221,     2,
     241,   242,   226,   225,   215,   224,   238,   227,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   218,   209,
     222,   216,   223,   217,   244,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   239,     2,   240,   220,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   207,   219,   208,   231,     2,     2,     2,
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
     205,   206,   210,   211,   212,   213,   214,   229,   230,   233,
     234,   235,   236,   237
};

#if DAS_YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   593,   593,   594,   599,   600,   601,   602,   603,   604,
     605,   606,   607,   608,   609,   610,   611,   615,   621,   622,
     623,   627,   628,   632,   633,   637,   656,   657,   658,   659,
     663,   664,   668,   669,   673,   674,   674,   678,   683,   692,
     707,   723,   728,   736,   736,   775,   805,   809,   810,   811,
     815,   818,   822,   826,   830,   834,   840,   849,   852,   858,
     859,   863,   867,   868,   872,   875,   881,   887,   890,   896,
     897,   901,   902,   903,   912,   913,   917,   918,   922,   923,
     923,   929,   930,   931,   932,   933,   937,   943,   943,   949,
     949,   955,   963,   973,   982,   982,   986,   986,   992,   993,
     994,   995,   996,   997,   998,  1002,  1007,  1015,  1016,  1017,
    1021,  1022,  1023,  1024,  1025,  1026,  1027,  1028,  1029,  1035,
    1038,  1044,  1047,  1050,  1056,  1057,  1058,  1059,  1063,  1080,
    1102,  1105,  1115,  1130,  1145,  1160,  1163,  1170,  1174,  1181,
    1182,  1186,  1187,  1188,  1192,  1195,  1199,  1206,  1210,  1211,
    1212,  1213,  1214,  1215,  1216,  1217,  1218,  1219,  1220,  1221,
    1222,  1223,  1224,  1225,  1226,  1227,  1228,  1229,  1230,  1231,
    1232,  1233,  1234,  1235,  1236,  1237,  1238,  1239,  1240,  1241,
    1242,  1243,  1244,  1245,  1246,  1247,  1248,  1249,  1250,  1251,
    1252,  1253,  1254,  1255,  1256,  1257,  1258,  1259,  1260,  1261,
    1262,  1263,  1264,  1265,  1266,  1267,  1268,  1269,  1270,  1271,
    1272,  1273,  1274,  1275,  1276,  1277,  1278,  1279,  1280,  1281,
    1282,  1283,  1284,  1285,  1286,  1287,  1288,  1289,  1290,  1291,
    1292,  1293,  1294,  1295,  1296,  1297,  1298,  1299,  1300,  1301,
    1302,  1303,  1304,  1305,  1306,  1307,  1308,  1309,  1310,  1311,
    1312,  1313,  1314,  1315,  1316,  1317,  1318,  1319,  1320,  1321,
    1322,  1323,  1324,  1325,  1326,  1327,  1328,  1329,  1330,  1331,
    1332,  1333,  1334,  1335,  1336,  1337,  1338,  1339,  1340,  1344,
    1345,  1349,  1368,  1369,  1370,  1374,  1380,  1380,  1398,  1399,
    1402,  1403,  1406,  1410,  1421,  1430,  1439,  1445,  1446,  1447,
    1448,  1449,  1450,  1451,  1452,  1453,  1454,  1455,  1456,  1457,
    1458,  1459,  1460,  1461,  1462,  1463,  1464,  1465,  1469,  1474,
    1480,  1486,  1497,  1498,  1502,  1503,  1507,  1508,  1512,  1516,
    1523,  1523,  1523,  1529,  1529,  1529,  1538,  1572,  1575,  1578,
    1581,  1587,  1588,  1599,  1603,  1606,  1614,  1614,  1614,  1617,
    1623,  1626,  1630,  1634,  1641,  1648,  1654,  1658,  1662,  1665,
    1668,  1676,  1679,  1682,  1690,  1693,  1701,  1704,  1707,  1715,
    1721,  1722,  1723,  1727,  1728,  1732,  1733,  1737,  1742,  1750,
    1756,  1762,  1768,  1774,  1783,  1792,  1801,  1813,  1816,  1822,
    1822,  1822,  1825,  1825,  1825,  1830,  1830,  1830,  1838,  1838,
    1838,  1844,  1854,  1865,  1878,  1888,  1899,  1914,  1917,  1923,
    1924,  1931,  1942,  1943,  1944,  1948,  1949,  1950,  1951,  1952,
    1956,  1961,  1969,  1970,  1971,  1975,  1980,  1987,  1994,  1994,
    2003,  2004,  2005,  2006,  2007,  2008,  2009,  2010,  2014,  2015,
    2016,  2017,  2018,  2019,  2020,  2021,  2022,  2023,  2024,  2025,
    2026,  2027,  2028,  2029,  2030,  2031,  2032,  2036,  2037,  2038,
    2039,  2044,  2045,  2046,  2047,  2048,  2049,  2050,  2051,  2052,
    2053,  2054,  2055,  2056,  2057,  2058,  2059,  2060,  2065,  2071,
    2082,  2088,  2099,  2103,  2110,  2113,  2113,  2113,  2118,  2118,
    2118,  2131,  2135,  2139,  2145,  2153,  2161,  2167,  2175,  2175,
    2175,  2182,  2186,  2195,  2203,  2211,  2215,  2218,  2224,  2225,
    2226,  2227,  2228,  2229,  2230,  2231,  2232,  2233,  2234,  2235,
    2236,  2237,  2238,  2239,  2240,  2241,  2242,  2243,  2244,  2245,
    2246,  2247,  2248,  2249,  2250,  2251,  2252,  2253,  2254,  2255,
    2256,  2257,  2258,  2259,  2265,  2266,  2267,  2268,  2269,  2284,
    2293,  2294,  2295,  2296,  2297,  2298,  2299,  2300,  2301,  2302,
    2303,  2304,  2307,  2310,  2311,  2314,  2314,  2314,  2317,  2322,
    2326,  2330,  2330,  2330,  2335,  2338,  2342,  2342,  2342,  2347,
    2350,  2351,  2352,  2353,  2354,  2355,  2356,  2357,  2358,  2360,
    2364,  2365,  2370,  2374,  2375,  2376,  2377,  2378,  2379,  2380,
    2384,  2388,  2392,  2396,  2400,  2404,  2408,  2412,  2416,  2423,
    2424,  2425,  2429,  2430,  2431,  2435,  2436,  2440,  2441,  2442,
    2446,  2447,  2451,  2462,  2465,  2468,  2468,  2472,  2472,  2491,
    2490,  2506,  2505,  2519,  2528,  2540,  2549,  2559,  2560,  2561,
    2562,  2563,  2567,  2570,  2579,  2580,  2584,  2587,  2590,  2605,
    2614,  2615,  2619,  2622,  2625,  2638,  2639,  2643,  2649,  2655,
    2664,  2667,  2674,  2677,  2683,  2684,  2685,  2689,  2690,  2694,
    2701,  2706,  2715,  2721,  2725,  2736,  2740,  2746,  2752,  2761,
    2773,  2776,  2779,  2779,  2799,  2800,  2804,  2805,  2806,  2810,
    2813,  2813,  2832,  2835,  2838,  2853,  2872,  2873,  2874,  2879,
    2879,  2905,  2906,  2910,  2911,  2911,  2915,  2916,  2917,  2921,
    2931,  2936,  2931,  2948,  2953,  2948,  2968,  2969,  2973,  2974,
    2978,  2984,  2985,  2986,  2987,  2991,  2992,  2993,  2997,  3000,
    3006,  3011,  3006,  3031,  3038,  3043,  3052,  3058,  3062,  3073,
    3074,  3075,  3076,  3077,  3078,  3079,  3080,  3081,  3082,  3083,
    3084,  3085,  3086,  3087,  3088,  3089,  3090,  3091,  3092,  3093,
    3094,  3095,  3096,  3097,  3098,  3099,  3100,  3101,  3102,  3103,
    3104,  3105,  3106,  3107,  3108,  3109,  3110,  3111,  3112,  3113,
    3114,  3115,  3116,  3117,  3118,  3119,  3120,  3121,  3122,  3126,
    3127,  3128,  3129,  3130,  3131,  3132,  3133,  3137,  3148,  3152,
    3159,  3171,  3178,  3184,  3193,  3198,  3201,  3211,  3224,  3225,
    3226,  3227,  3228,  3232,  3236,  3236,  3236,  3250,  3251,  3255,
    3259,  3266,  3269,  3275,  3276,  3277,  3278,  3279,  3289,  3292,
    3292,  3292,  3296,  3301,  3308,  3308,  3315,  3319,  3323,  3328,
    3333,  3338,  3343,  3347,  3351,  3356,  3360,  3364,  3369,  3369,
    3369,  3375,  3382,  3382,  3382,  3387,  3387,  3387,  3393,  3393,
    3393,  3398,  3403,  3403,  3403,  3408,  3408,  3408,  3417,  3422,
    3422,  3422,  3427,  3427,  3427,  3436,  3441,  3441,  3441,  3446,
    3446,  3446,  3455,  3455,  3455,  3461,  3461,  3461,  3470,  3473,
    3484,  3500,  3500,  3505,  3510,  3500,  3535,  3535,  3540,  3546,
    3535,  3571,  3571,  3576,  3581,  3571,  3621,  3622,  3623,  3624,
    3625,  3629,  3636,  3643,  3649,  3655,  3662,  3669,  3675,  3684,
    3687,  3693,  3701,  3706,  3713,  3718,  3725,  3730,  3736,  3737,
    3741,  3742,  3747,  3748,  3752,  3753,  3757,  3758,  3762,  3763,
    3764,  3768,  3769,  3770,  3774,  3775,  3779,  3785,  3792,  3800,
    3807,  3815,  3824,  3824,  3824,  3832,  3832,  3832,  3839,  3839,
    3839,  3850,  3850,  3850,  3861,  3864,  3870,  3884,  3890,  3896,
    3902,  3902,  3902,  3916,  3921,  3928,  3947,  3952,  3959,  3959,
    3959,  3969,  3969,  3969,  3983,  3983,  3983,  3997,  4006,  4006,
    4006,  4026,  4033,  4033,  4033,  4043,  4048,  4055,  4058,  4064,
    4083,  4095,  4103,  4123,  4148,  4149,  4153,  4154,  4159,  4162,
    4165,  4168,  4171,  4174
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
  "\"close scope\"", "\"end of line\"", "\"integer constant\"",
  "\"long integer constant\"", "\"unsigned integer constant\"",
  "\"unsigned long integer constant\"", "\"unsigned int8 constant\"",
  "\"floating point constant\"", "\"float16 constant\"",
  "\"double constant\"", "\"name\"", "\"keyword\"", "\"type function\"",
  "\"start of the string\"", "STRING_CHARACTER", "STRING_CHARACTER_ESC",
  "\"end of the string\"", "\"{\"", "\"}\"",
  "\"end of failed eader macro\"", "\"begin of code block\"",
  "\"end of code block\"", "\"end of expression\"", "\";}}\"", "\";}]\"",
  "\";]]\"", "\",]]\"", "\",}]\"", "','", "'='", "'?'", "':'", "'|'",
  "'^'", "'&'", "'<'", "'>'", "'-'", "'+'", "'*'", "'/'", "'%'",
  "UNARY_MINUS", "UNARY_PLUS", "'~'", "'!'", "PRE_INC", "PRE_DEC",
  "POST_INC", "POST_DEC", "DEREF", "'.'", "'['", "']'", "'('", "')'",
  "'$'", "'@'", "'#'", "$accept", "program", "top_level_reader_macro",
  "optional_public_or_private_module", "module_name",
  "optional_not_required", "module_declaration", "character_sequence",
  "string_constant", "format_string", "optional_format_string", "$@1",
  "string_builder_body", "string_builder", "reader_character_sequence",
  "expr_reader", "$@2", "options_declaration", "require_declaration",
  "keyword_or_name", "require_module_name", "require_module",
  "is_public_module", "expect_declaration", "expect_list", "expect_error",
  "expression_label", "expression_goto", "elif_or_static_elif",
  "expression_else", "semicolon", "if_or_static_if",
  "expression_else_one_liner", "$@3", "expression_if_one_liner",
  "expression_if_then_else", "$@4", "expression_for_loop", "$@5",
  "expression_unsafe", "expression_while_loop", "expression_with",
  "expression_with_alias", "$@6", "$@7", "annotation_argument_value",
  "annotation_argument_value_list", "annotation_argument_name",
  "annotation_argument", "annotation_argument_list",
  "metadata_argument_list", "annotation_declaration_name",
  "annotation_declaration_basic", "annotation_declaration",
  "annotation_list", "optional_annotation_list",
  "optional_function_argument_list", "optional_function_type",
  "function_name", "optional_template", "global_function_declaration",
  "optional_public_or_private_function", "function_declaration_header",
  "function_declaration", "$@8", "open_block", "close_block",
  "expression_block", "expr_call_pipe", "expression_any", "expressions",
  "expr_keyword", "optional_expr_list", "optional_expr_list_in_braces",
  "optional_expr_map_tuple_list", "type_declaration_no_options_list",
  "expression_keyword", "$@9", "$@10", "$@11", "$@12", "expr_pipe",
  "name_in_namespace", "expression_delete", "new_type_declaration", "$@13",
  "$@14", "expr_new", "expression_break", "expression_continue",
  "expression_return_no_pipe", "expression_return",
  "expression_yield_no_pipe", "expression_yield", "expression_try_catch",
  "kwd_let_var_or_nothing", "kwd_let", "optional_in_scope",
  "tuple_expansion", "tuple_expansion_variable_declaration",
  "expression_let", "expr_cast", "$@15", "$@16", "$@17", "$@18", "$@19",
  "$@20", "expr_type_decl", "$@21", "$@22", "expr_type_info", "expr_list",
  "block_or_simple_block", "block_or_lambda", "capture_entry",
  "capture_list", "optional_capture_list", "expr_block", "expr_full_block",
  "expr_full_block_assumed_piped", "$@23", "expr_numeric_const",
  "expr_assign", "expr_assign_pipe_right", "expr_assign_pipe",
  "expr_named_call", "expr_method_call", "func_addr_name",
  "func_addr_expr", "$@24", "$@25", "$@26", "$@27", "expr_field", "$@28",
  "$@29", "expr_call", "expr", "$@30", "$@31", "$@32", "$@33", "$@34",
  "$@35", "expr_mtag", "optional_field_annotation", "optional_override",
  "optional_constant", "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "struct_variable_declaration_list", "$@36", "$@37", "$@38", "$@39",
  "function_argument_declaration_no_type",
  "function_argument_declaration_type", "function_argument_list",
  "tuple_type", "tuple_type_list", "tuple_alias_type_list", "variant_type",
  "variant_type_list", "variant_alias_type_list", "copy_or_move",
  "variable_declaration_no_type", "variable_declaration_type",
  "variable_declaration", "copy_or_move_or_clone", "optional_ref",
  "let_variable_name_with_pos_list", "let_variable_declaration",
  "global_variable_declaration_list", "$@40", "optional_shared",
  "optional_public_or_private_variable", "global_let", "$@41", "enum_list",
  "optional_public_or_private_alias", "single_alias", "$@42", "alias_list",
  "alias_declaration", "$@43", "optional_public_or_private_enum",
  "enum_name", "enum_declaration", "$@44", "$@45", "$@46", "$@47",
  "optional_structure_parent", "optional_sealed", "structure_name",
  "class_or_struct", "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@48", "$@49", "variable_name_with_pos_list", "basic_type_declaration",
  "enum_basic_type_declaration", "structure_type_declaration",
  "auto_type_declaration", "bitfield_bits", "bitfield_alias_bits",
  "bitfield_basic_type_declaration", "bitfield_type_declaration", "$@50",
  "$@51", "c_or_s", "table_type_pair", "dim_list",
  "type_declaration_no_options", "$@52", "$@53", "$@54", "$@55", "$@56",
  "$@57", "$@58", "$@59", "$@60", "$@61", "$@62", "$@63", "$@64", "$@65",
  "$@66", "$@67", "$@68", "$@69", "$@70", "$@71", "$@72", "$@73", "$@74",
  "$@75", "$@76", "$@77", "$@78", "type_declaration",
  "tuple_alias_declaration", "$@79", "$@80", "$@81", "$@82",
  "variant_alias_declaration", "$@83", "$@84", "$@85", "$@86",
  "bitfield_alias_declaration", "$@87", "$@88", "$@89", "$@90",
  "make_decl", "make_struct_fields", "make_variant_dim",
  "make_struct_single", "make_struct_dim", "make_struct_dim_list",
  "make_struct_dim_decl", "optional_make_struct_dim_decl",
  "optional_block", "optional_trailing_semicolon_cur_cur",
  "optional_trailing_semicolon_cur_sqr",
  "optional_trailing_semicolon_sqr_sqr", "optional_trailing_delim_sqr_sqr",
  "optional_trailing_delim_cur_sqr", "use_initializer", "make_struct_decl",
  "$@91", "$@92", "$@93", "$@94", "$@95", "$@96", "$@97", "$@98",
  "make_tuple", "make_map_tuple", "make_tuple_call", "$@99", "$@100",
  "make_dim", "make_dim_decl", "$@101", "$@102", "$@103", "$@104", "$@105",
  "$@106", "$@107", "$@108", "$@109", "$@110", "make_table",
  "expr_map_tuple_list", "make_table_decl", "array_comprehension_where",
  "optional_comma", "array_comprehension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1583)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-880)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1583,    55, -1583, -1583,    44,     1,   140,   308, -1583,  -115,
     288,   288,   288, -1583, -1583,   121,   291, -1583, -1583,   437,
   -1583, -1583, -1583, -1583,   631, -1583,    99, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583,    23, -1583,    35,
     171,    82, -1583,   217, -1583, -1583, -1583,   443,   234, -1583,
      61, -1583, -1583, -1583,   288,   288, -1583, -1583,    99, -1583,
   -1583, -1583, -1583, -1583,   279,   363, -1583, -1583, -1583, -1583,
     291,   291,   291,   265, -1583,   857,  -127, -1583, -1583,   394,
     448,   484,   732,   806, -1583,   831,    51,    44,   435,     1,
     140,   140,   391,   140,   459, -1583,   850,   850, -1583,   481,
     437,    18,   437,   841,   496,   514,   536, -1583,   558,   460,
   -1583, -1583,   -24,    44,   291,   291,   291,   291, -1583, -1583,
   -1583, -1583,   851, -1583, -1583,   563, -1583, -1583, -1583, -1583,
   -1583,   308, -1583, -1583, -1583, -1583, -1583,   881,   132,   541,
   -1583, -1583, -1583, -1583,   391,   391,   391,   723, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583,   437, -1583, -1583, -1583,   572,
   -1583, -1583, -1583, -1583, -1583,   616, -1583,  -118, -1583,   717,
     652,   857, -1583, -1583, -1583, -1583, -1583,   233,   721, -1583,
     -80, -1583, -1583, -1583,   889, -1583, -1583, -1583, -1583, -1583,
     596, -1583, -1583,   -22,   656, -1583,   646, -1583,   815, -1583,
     671,   308,   308, -1583, -1583, 16075,  1163, -1583, -1583,   705,
   -1583,   388,    44,    44,   186,    79, -1583, -1583, -1583,   767,
     132, -1583, -1583, 11624, -1583,   662,   308, -1583, -1583, 14801,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,   878,   931,
   -1583,   683,   308, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583,   308, -1583,   751,   308, -1583, -1583,   -80,   395, -1583,
      44, -1583,   735,   935,   522, -1583, -1583, -1583,   765,   770,
     780,   764,   796,   798, -1583, -1583, -1583,   788, -1583, -1583,
   -1583, -1583, -1583,   278, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583,   800, -1583, -1583, -1583,   810,
     823, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,   840,   844,
     812,   121, -1583, -1583, -1583, -1583, -1583,   599,   818, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583,   787,   860, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,   867,   814,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583,  1049, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583,   888,   838, -1583, -1583,   -44,
     -63, -1583, -1583, -1583,   375,   121, -1583, -1583, -1583,    79,
     854, -1583, 10684,   901,   212, 11624, -1583,     6, -1583, -1583,
   -1583, 10684, -1583, -1583,   905,   886,   -65,   -53,   -45, -1583,
   -1583, 10684,   523, -1583, -1583, -1583,    36, -1583, -1583, -1583,
      10,  6540, -1583,   864, 11276, -1583, 11451,   539, -1583, -1583,
   -1583, -1583,   913,   740,  1575,   874, -1583,   182,   437,   345,
     876, 11624, 11624, -1583,  2619, -1583,   240, -1583,   504, -1583,
      50, -1583, -1583,   898,   910, -1583, -1583,   824,   -66,   915,
      34, -1583,   525,   882,   919,   921,   903,   923,   907,   543,
     924, -1583,   559,   927,   928, 10684, 10684,   911,   912,   914,
     916,   917,   922, -1583,  2138, 11103,  6772, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583,   932,   934, -1583,  7004, 10684,
   10684, 10684, 10684, 10684,  5620,  7234, -1583,   920, -1583, -1583,
   -1583,   -55, -1583, -1583, -1583, -1583,   926, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583,  1900, -1583,   925, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583,   929,  1084,  1126, -1583, -1583, -1583,
    4232, 11624, 11624, 11624, 11967, 11624, 11624,   930,   937, 11624,
     683, 11624,   683, 11624,   683, 11797,   970, 12076, -1583, 10684,
   -1583, -1583, -1583, -1583, -1583,   933, -1583, -1583, 14348, 10684,
   -1583,   599,   589,   -72, -1583, -1583,   374, -1583,   818,   504,
     956,   374, -1583,   504, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   10684, -1583, -1583,   251,    64,    64,    64, -1583,   818,   818,
   -1583, 10684, -1583, -1583, -1583,  3542, -1583,   308,  7464, -1583,
   10684,   978, -1583,   437,   989,  7694,    60,   957,  3772,   158,
     158,   158,  7924,   437,   437, -1583, 10684,  1170, -1583, -1583,
   -1583, -1583, -1583, -1583,  1146, -1583, -1583, -1583,   711, -1583,
     437,   437,   437,   437, -1583,   437, -1583, -1583,  1122, -1583,
     244, -1583,  1764,   375, 10684, -1583, -1583, -1583,   291, -1583,
    1177, -1583,   -80, -1583, -1583, -1583,   950, -1583, -1583,   121,
     560, -1583,   971,   972,   973, -1583, 10684, 11624, 10684, 10684,
   -1583, -1583, 10684, -1583, 10684, -1583, 10684, -1583, -1583, 10684,
   -1583, 11624,   438,   438, 10684, 10684, 10684, 10684, 10684, 10684,
     251,  2849,   251,  3080,   251, 15032, -1583,   904, -1583, -1583,
     866,   251,   984, -1583,   981,   438,   438,   -76,   438,   438,
     251,  1184,   960,   987, 15394,   964,    17,   987,   992,   966,
     377, -1583,  4462,    52, 15738, 15793, 10684, 10684, -1583, -1583,
   10684, 10684, 10684, 10684,  1012, 10684,   597, 10684, 10684, 10684,
   10684, 10684, 10684, 10684, 10684, 10684,  8154, 10684, 10684, 10684,
   10684, 10684, 10684, 10684, 10684, 10684, 10684, 15974, 10684, -1583,
    8384, 10684,  1013, -1583,  4232, -1583,  1056, 11798,   -28,   347,
     990,   475, -1583,   609,   629, -1583, -1583,  1015,   733,   -63,
     752,   -63,   762,   -63, -1583,   299, -1583,   304, -1583, 11624,
     999, -1583, -1583, 14383,   342, -1583,   504, 11624, -1583, -1583,
   11624, -1583, -1583, 12111,   974,  1174, -1583, -1583,   146, -1583,
   -1583, -1583, 14844,   251,  4232, -1583,  1003, 11932,  1207, 10684,
   15394,  1025, 14844,  1007, -1583,  1006,  1036, 15394, -1583, 11624,
    4232, -1583, 11932,   983, -1583,   926, -1583, -1583, -1583, 14844,
   -1583, -1583, 14844, -1583,   308, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583,   152,   158, -1583,  4692,  4692,  4692,  4692,
    4692,  4692,  4692,  4692,  4692,  4692,  4692, 10684,  4692,  4692,
    4692,  4692,  4692,  4692, -1583,   504, 14938,  1031,   428,   865,
    1162,   437, 11624, 11624, 11624,  5850,  8614,  1033, 10684, 11624,
   -1583, -1583, -1583, 11624,   987,   872,   991, 12218, 11624, 11624,
   12253, 11624, 12360, 11624,   987, 11624, 11797,   987,   970,   588,
   12395, 12502, 12537, 12644, 12679, 12786,    15,   158,  3311,  4924,
    6080, 15072,  1017,    12,   102,  1019,  -102,    29,  6310,    12,
     832,    54, 10684,  1028, 10684, -1583, -1583, 11624, 11624, -1583,
   10684,   736,    59, -1583, 10684, -1583,    78,   251, -1583, 10684,
   -1583, 10684, -1583, 10684, -1583, 10684,   995,   627, -1583, -1583,
    1004,  1009,   -26, -1583, -1583,   189,  5156, -1583,    -6,   998,
    1014,   156,   683,  1022,  1026, -1583, -1583,  1032,  1027, -1583,
   -1583,  2244,  2244, 11443, 11443,  1719,  1719,  1029,   119,  1034,
   -1583, 14490,   -27,   -27,   925,  2244,  2244,  1495, 15528, 15568,
   15434, 15920, 15166, 15260, 11105, 11271, 11443, 11443,   564,   564,
     119,   119,   119,   637, 10684,  1038,  1039,   645, 10684,  1246,
    1040, 14525, -1583,    67, 12821, -1583, -1583, 11798, 10684, 10684,
   10684, 10684, 10684, 10684, 10684, 10684, 10684, 10684, 10684, 10684,
   10684, 10684, 10684, 10684, 10684, -1583, -1583, -1583, -1583, 11624,
   -1583, -1583, -1583,   319, -1583,  1030, -1583,  1035, -1583,  1043,
   -1583, 11797, -1583,   970,   389,   818, -1583,  1045, -1583, 10684,
   -1583, -1583,   818,   818, -1583, 10684,  1075,   267, 11624, -1583,
   10684, -1583,    88, -1583,  1003, 10684,   308, 15394,  1058, -1583,
   -1583, -1583, -1583,   622, -1583, 11932, -1583,    52, -1583,   842,
   10684, -1583,   926,  1094,  1094, -1583, -1583, -1583,   158,   158,
     158, -1583, -1583,  2368, -1583,  2368, -1583,  2368, -1583,  2368,
   -1583,  2368, -1583,  2368, -1583,  2368, -1583,  2368, -1583,  2368,
   -1583,  2368, -1583,  2368, 15394, -1583,  2368, -1583,  2368, -1583,
    2368, -1583,  2368, -1583,  2368, -1583,  2368, -1583, -1583,  1079,
     437, -1583, -1583,   663, -1583,    45, -1583,   835,   883,   778,
     650,   137,  1054,  1057,  1103, 12928,   229, 12963,   781, 11624,
   11797,   970,  1101,  1059,  1062, 11624, -1583, -1583,  1143,  1335,
   -1583,  1354, -1583,  1367,  1065,  3234,   429,  1066,   442,    52,
   -1583, -1583, -1583, -1583, -1583,  1068, 10684, -1583,  5388, 14348,
       4, 10684,   627,   650,   102, -1583, -1583,  1070, -1583, 10684,
   10684, -1583,  1071, -1583, 10684,   650,   712,  1072, -1583, -1583,
   10684, 15394, -1583, -1583,   486,   519, 15206, 10684, -1583, 10684,
      92, 15394, 13070, 15394, 15394, -1583,  1074,   180, 10684, 10684,
   11624,   683,   181, -1583,  1077,   447, 10914, -1583, -1583,   156,
    1116,  1123,  1080,  1131,  1132, -1583,   449,   -63, -1583, 10684,
   -1583, 10684,  8844, 10684, -1583,  1109,  1082, -1583, -1583, 10684,
    1091, -1583, 14632, 10684,  9074,  1092, -1583, 14667, -1583,  9304,
   -1583, -1583, -1583, -1583, 15394, 15394, 15394, 15394, 15394, 15394,
   15394, 15394, 15394, 15394, 15394, 15394, 15394, 15394, 15394, 15394,
   15394, -1583, -1583, -1583,   818, -1583, -1583,  1139, -1583,  1140,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
    1097, 11624, -1583, 14938, 13105, -1583,  1098,  1298,   -17, 15394,
   10684, -1583, 11624, 10684,    52,   683,   308, -1583, -1583, 10684,
   -1583,  2185,  2619,    52, -1583,   457,   141, -1583, -1583, -1583,
   11624, -1583,  1311,    45, -1583, -1583,   865, -1583, -1583, -1583,
    1106, -1583, -1583, -1583,   676, -1583,  1148,  1110, -1583, -1583,
    3465,   692,   694, -1583, -1583, 10684,  3695, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583,  1111,  9534,   716,    12,
     102, 15394,  1017, -1583, -1583, 15394,  1019, -1583,   720,    12,
    1113, -1583, -1583, -1583, -1583,   727, -1583, -1583, -1583,  1151,
     730,   731, 10684,   200, 10684, 10684, 10684, 13212, 13247,  3925,
     -63, -1583,  1115,  5156,   268, -1583, -1583,  1161, -1583, -1583,
     156,  1121,    22, 11624, 13354, 11624, 13389, -1583,   346, 13496,
   -1583, 10684,  2144, 10684, -1583, 13531,  5156, -1583,   358, 10684,
   -1583, -1583, -1583,   392, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583,   818, -1583, -1583, 10684,  1166, 10684,   340,   818, 15394,
     792,   -63, -1583, 14844, -1583,   437, -1583,   683,  1172,  1130,
     125,   215, -1583, -1583,  1311,   251,  1133,  1134, -1583, -1583,
   10684,  1175,  1153, 10684, -1583, -1583, -1583, -1583,  1137,  1142,
    1144, 10684, 10684, 10684,  1145,  1319,  1149,  1152,  9764, -1583,
     403, 10684,   102, -1583, 10684,   712, -1583, 10684, 10684,  1097,
   -1583, -1583, 10684, 10684,   744, 10684, 10684, 13638, 15394, 15394,
   -1583, -1583, -1583,  1169, -1583,   464, -1583,  1155, -1583, -1583,
    9994, -1583, -1583,  4155, -1583,   793, -1583, -1583, -1583, 11624,
   13673, 13780, -1583,   485, -1583, 13815, -1583, 13922, -1583, 15394,
   -1583, -1583,    22,   842,  4002, -1583,   -63, -1583,   575, 11624,
       6, -1583, 16075, -1583, -1583, -1583, -1583,  1319,  1319, 13957,
    1171,  1157, 14064,  1158,  1160,  1168, 10684, -1583, 10684, 11443,
   11443, 11443, 10684, -1583, -1583,  1319,  1319, -1583, 14099, -1583,
   15300, -1583, 15300, -1583,  1193, 11443, -1583,  1209,  1193, 15300,
   10684, 15394, 15394,   209,   261, -1583,  1178, -1583, 10684, 15434,
   -1583, -1583,   794, -1583, -1583,  1179, -1583, -1583, -1583, -1583,
   10224, 10454, -1583, -1583, -1583, -1583, -1583, 15394,   308, 11624,
       6,  1138,  4232,   437, 16075,   194,   194, -1583, 10684, 10684,
   -1583,  1319,  1319,   650,  1181,  1182,   987,   194,   650, -1583,
    1360,  1176,  1211,  1217, -1583,  1218,  1189, 15300, 10684, 10684,
   -1583,   261, -1583,  2144, -1583, -1583, -1583, -1583, 10684, 10684,
   15394, -1583,  1138,  4232,  4232, -1583, 11798, -1583,   308,   650,
    1017,  1202, -1583,  1188,  1190, 14206, 14241,   194,   194,  1017,
    1191, -1583, -1583,  1192,  1194,  1195, 10684,  1199,  1201,  1227,
   -1583, -1583,  1203, 15394, 15394, -1583, -1583, 15394,  4232, -1583,
   11798, -1583, 11798, -1583, -1583,   405,  1205, -1583, -1583, -1583,
   -1583, -1583,  1200,  1208, -1583, -1583, -1583, -1583, 15394, -1583,
   -1583, -1583, -1583, -1583, 11798, -1583, -1583, -1583,   650, -1583,
   -1583, -1583,   412, -1583
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   139,     1,   373,     0,     0,     0,   704,   374,     0,
     696,   696,   696,    74,    75,     0,     0,    15,     3,     0,
      10,     9,     8,    16,     0,     7,   684,     6,    11,     5,
       4,    13,    12,    14,   108,   109,   107,   117,   119,    45,
      64,    61,    62,     0,    47,    48,    49,     0,     0,    50,
      59,    46,   289,   288,   696,   696,    22,    21,   684,   698,
     697,   901,   891,   896,     0,   341,    43,   125,   126,   127,
       0,     0,     0,   128,   130,   137,     0,   124,    17,   722,
     721,   279,   706,   725,   685,   686,     0,     0,     0,     0,
       0,     0,    51,     0,     0,    60,     0,     0,    57,     0,
       0,   696,     0,    18,     0,     0,     0,   343,     0,     0,
     136,   131,     0,     0,     0,     0,     0,     0,   140,   724,
     723,   280,   282,   708,   707,     0,   727,   726,   730,   688,
     687,   690,   115,   116,   113,   114,   111,     0,     0,     0,
     110,   120,    65,    63,    53,    54,    52,    59,    56,    55,
     699,   701,   291,   290,   703,     0,   705,    19,    20,    23,
     902,   892,   897,   342,    41,    44,   135,     0,   132,   133,
     134,   138,   284,   283,   286,   281,   709,     0,   718,   680,
     609,    26,    27,    31,     0,   103,   104,   101,   102,    99,
       0,    98,   105,     0,     0,    58,     0,   702,     0,    25,
     808,     0,     0,    42,   129,     0,     0,   710,   719,     0,
     731,   682,     0,     0,   611,     0,    28,    29,    30,     0,
       0,   118,   112,     0,    24,     0,     0,   893,   898,     0,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,     0,     0,
     147,   141,     0,   789,   792,   795,   796,   790,   793,   791,
     794,     0,   692,   716,   728,   681,   689,   609,     0,   121,
       0,   123,     0,   669,   667,   691,   100,   106,     0,     0,
       0,     0,     0,     0,   739,   782,   740,   798,   741,   745,
     746,   747,   748,   788,   752,   753,   754,   755,   756,   757,
     758,   783,   784,   785,   786,   861,   744,   751,   787,   868,
     875,   742,   749,   743,   750,   759,   760,   761,   762,   763,
     764,   765,   766,   767,   768,   769,   770,   771,   772,   773,
     774,   775,   776,   777,   778,   779,   780,   781,     0,     0,
       0,     0,   797,   823,   826,   824,   825,   888,   700,   811,
     812,   809,   810,   903,   646,   652,   225,   226,   223,   150,
     151,   153,   152,   154,   155,   156,   157,   183,   184,   181,
     182,   174,   185,   186,   175,   172,   173,   224,   207,     0,
     222,   187,   188,   189,   190,   161,   162,   163,   158,   159,
     160,   171,     0,   177,   178,   176,   169,   170,   165,   164,
     166,   167,   168,   149,   148,   206,     0,   179,   180,   609,
     144,   318,   287,   713,   711,     0,   720,   623,   732,     0,
       0,   122,     0,     0,     0,     0,   668,     0,   829,   852,
     855,     0,   858,   848,     0,     0,   862,   869,   876,   882,
     885,     0,   324,   838,   843,   837,     0,   851,   847,   840,
       0,     0,   842,   827,     0,   804,   894,   899,   227,   228,
     221,   205,   229,   208,   191,     0,   142,   372,   637,   638,
       0,     0,     0,   285,     0,   692,     0,   693,     0,   717,
     627,   683,   610,     0,     0,   514,   515,     0,     0,     0,
       0,   508,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   788,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   598,     0,     0,     0,   430,   432,   431,
     433,   434,   435,   436,   437,     0,     0,    37,   326,     0,
       0,     0,     0,     0,   322,     0,   412,   413,   512,   511,
     592,   509,   583,   582,   581,   580,   139,   586,   510,   585,
     584,   556,   516,   557,     0,   517,     0,   513,   906,   910,
     907,   908,   909,   671,     0,   672,     0,   665,   666,   664,
       0,     0,     0,     0,     0,     0,     0,     0,   814,     0,
     141,     0,   141,     0,   141,     0,     0,     0,   834,   322,
     833,   845,   846,   839,   841,     0,   844,   828,     0,     0,
     890,   889,   904,   341,   817,   818,     0,   647,   642,     0,
       0,     0,   653,     0,   230,   210,   211,   213,   212,   214,
     215,   216,   217,   209,   218,   219,   220,   194,   195,   197,
     196,   198,   199,   200,   201,   192,   193,   202,   203,   204,
       0,   370,   371,     0,   609,   609,   609,   143,   146,   145,
     320,     0,    76,    77,    89,   358,   356,     0,     0,    96,
       0,     0,   357,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   297,     0,     0,   313,   308,
     305,   304,   306,   307,   292,   340,   319,   299,   592,   298,
       0,    84,    85,    82,   311,    83,   312,   314,   376,   303,
       0,   300,   438,   714,     0,   694,   712,   625,     0,   624,
       0,   729,   609,   952,   955,   346,   350,   349,   355,     0,
       0,   398,     0,     0,     0,   988,     0,     0,   326,     0,
     389,   392,     0,   395,     0,   992,     0,   961,   970,     0,
     958,     0,   544,   545,     0,     0,     0,     0,     0,     0,
       0,   930,     0,     0,     0,   968,   995,     0,   330,   333,
       0,     0,     0,   997,  1006,   521,   520,   558,   519,   518,
       0,     0,     0,  1006,   407,     0,   341,  1006,  1006,     0,
     414,   590,     0,   422,     0,     0,     0,     0,   546,   547,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,     0,   670,
       0,     0,     0,   675,     0,   679,     0,   438,     0,     0,
       0,   819,   832,     0,     0,   799,   813,     0,     0,   144,
       0,   144,     0,   144,   644,     0,   650,     0,   800,     0,
    1006,   836,   821,     0,     0,   805,     0,     0,   648,   895,
       0,   654,   900,     0,     0,   733,   634,   635,   657,   639,
     641,   640,     0,     0,     0,   362,   359,   407,     0,     0,
     344,     0,     0,     0,   317,     0,     0,    68,    91,     0,
       0,   367,   364,   413,   425,   139,   339,   337,   338,     0,
     315,   316,     0,    87,     0,   428,   295,   302,   309,   310,
     361,   366,   375,     0,     0,   301,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   294,     0,     0,     0,     0,   617,
     620,     0,     0,     0,     0,   944,     0,     0,     0,     0,
     978,   981,   984,     0,  1006,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1006,     0,     0,  1006,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   964,   922,   930,     0,   973,     0,     0,     0,   930,
       0,     0,     0,     0,     0,   933,  1000,     0,     0,    40,
       0,    38,     0,   999,  1007,   327,     0,     0,   975,  1007,
     323,     0,   656,     0,   655,     0,     0,  1007,   921,   549,
       0,     0,   485,   482,   484,     0,   322,   501,     0,     0,
       0,     0,   141,     0,     0,   569,   568,     0,     0,   570,
     574,   522,   523,   535,   536,   533,   534,     0,   563,     0,
     554,     0,   587,   588,   589,   524,   525,   540,   541,   542,
     543,     0,     0,   538,   539,   537,   531,   532,   527,   526,
     528,   529,   530,     0,     0,     0,   491,     0,     0,     0,
       0,     0,   506,     0,     0,   674,   677,   438,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   678,   830,   853,   856,     0,
     859,   849,   801,     0,   863,     0,   870,     0,   877,     0,
     883,     0,   886,     0,     0,   328,  1007,     0,   822,     0,
     806,   905,   643,   649,   636,     0,     0,     0,     0,   658,
       0,    92,     0,   363,   360,     0,     0,   345,     0,    93,
      94,    66,    67,     0,   368,   365,   414,   422,   321,    71,
       0,   318,   139,     0,     0,   388,   387,   336,     0,     0,
       0,   460,   469,   448,   470,   449,   472,   451,   471,   450,
     473,   452,   463,   442,   464,   443,   465,   444,   474,   453,
     475,   454,   462,   440,   441,   476,   455,   477,   456,   466,
     445,   467,   446,   468,   447,   461,   439,   715,   695,     0,
     140,   618,   619,   620,   621,   612,   628,     0,     0,     0,
     945,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1001,   559,     0,     0,
     560,     0,   591,     0,     0,     0,     0,     0,     0,   422,
     593,   594,   595,   596,   597,     0,     0,   931,     0,   407,
     930,     0,     0,     0,     0,   939,   940,     0,   947,     0,
       0,   937,     0,   976,     0,     0,     0,     0,   935,   977,
       0,   967,   932,   996,     0,     0,    34,     0,   998,     0,
       0,   408,     0,   912,   911,   548,     0,     0,     0,     0,
       0,   141,     0,   502,     0,     0,     0,   505,   503,     0,
       0,     0,     0,     0,     0,   420,     0,   144,   565,     0,
     571,     0,     0,     0,   552,     0,     0,   575,   579,     0,
       0,   555,     0,     0,     0,     0,   492,     0,   499,     0,
     550,   507,   673,   676,   448,   449,   451,   450,   452,   442,
     443,   444,   453,   454,   440,   455,   456,   445,   446,   447,
     439,   831,   854,   857,   820,   860,   850,     0,   815,     0,
     864,   866,   871,   873,   878,   880,   884,   645,   887,   651,
     324,     0,   325,     0,     0,   735,     0,   736,   660,   659,
       0,   369,     0,     0,   422,   141,     0,    69,    70,     0,
      86,    78,     0,   422,   377,     0,     0,   459,   457,   458,
       0,   633,   615,   612,   613,   614,   617,   953,   956,   347,
       0,   352,   353,   351,     0,   401,     0,     0,   404,   399,
       0,     0,     0,   989,   987,   326,     0,   390,   393,   396,
     993,   991,   962,   971,   969,   959,     0,     0,     0,   930,
       0,   965,   923,   946,   938,   966,   974,   936,     0,   930,
       0,   942,   943,   950,   934,     0,   331,   334,    35,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     144,   504,     0,   322,     0,   417,   418,     0,   416,   415,
       0,     0,     0,     0,     0,     0,     0,   480,     0,     0,
     576,     0,   564,     0,   553,     0,   322,   493,     0,     0,
     551,   500,   496,     0,   803,   816,   802,   867,   874,   881,
     835,   329,   807,   734,     0,     0,     0,     0,    97,    95,
       0,   144,    72,     0,    79,     0,   293,   141,     0,     0,
     667,     0,   616,   629,   615,     0,     0,     0,   348,   354,
       0,     0,     0,     0,   400,   979,   982,   985,     0,     0,
       0,     0,     0,     0,     0,   944,     0,     0,     0,   599,
       0,     0,     0,   948,     0,     0,   941,     0,     0,   324,
      32,    39,     0,     0,     0,     0,     0,     0,   914,   913,
     483,   608,   486,     0,   478,     0,   424,     0,   421,   423,
       0,   409,   427,     0,   607,     0,   605,   481,   602,     0,
       0,     0,   601,     0,   494,     0,   497,     0,   738,   661,
      90,   296,     0,    71,     0,    88,   144,   378,   667,     0,
       0,   626,     0,   631,   663,   662,   622,   944,   944,     0,
       0,     0,     0,     0,     0,     0,   322,  1002,   326,   391,
     394,   397,     0,   945,   963,   944,   944,   561,     0,   600,
    1004,   949,  1004,   951,  1004,   332,   335,    36,  1004,  1004,
       0,   916,   915,     0,     0,   489,     0,   419,     0,   410,
     566,   572,     0,   606,   604,     0,   603,   737,   426,    73,
     358,     0,    80,    84,    85,    82,    83,    81,     0,     0,
       0,     0,     0,     0,     0,   929,   929,   402,     0,     0,
     405,   944,   944,   919,     0,     0,  1006,   929,   919,   562,
       0,     0,     0,     0,    33,     0,     0,  1004,     0,     0,
     487,     0,   479,   411,   567,   573,   577,   495,     0,     0,
     364,   429,     0,     0,     0,   386,   438,   630,     0,     0,
     926,  1006,   928,     0,     0,     0,     0,   929,   929,   920,
       0,   990,  1003,     0,     0,     0,     0,     0,     0,     0,
    1012,  1008,     0,   918,   917,   490,   578,   365,     0,   384,
     438,   382,   438,   385,   632,     0,  1007,   927,   954,   957,
     403,   406,     0,     0,   986,   994,   972,   960,  1005,  1010,
    1011,  1013,  1009,   380,   438,   383,   381,   924,     0,   980,
     983,   379,     0,   925
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1583, -1583, -1583, -1583, -1583, -1583, -1583,   654,  1366, -1583,
   -1583, -1583, -1583, -1583, -1583,  1452, -1583, -1583, -1583,   918,
     400, -1583,  1307, -1583, -1583,  1368, -1583, -1583, -1583,  -198,
      -1, -1583, -1583, -1583,  -196, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583,  1236, -1583, -1583,   -34,   -40,
   -1583, -1583, -1583,   743,   725,  -484,  -600,  -835, -1583, -1583,
   -1583, -1583, -1582, -1583, -1583,    26,  -206,  -279,  -491, -1583,
     273, -1583,  -615, -1356,  -751,    48,  -481, -1583, -1583, -1583,
   -1583,  -592,     0, -1583, -1583, -1583, -1583, -1583,  -195,  -188,
    -186, -1583,  -184, -1583, -1583, -1583,  1471, -1583,   282, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583,  -231,  -174,   262,   -36,   148, -1147,  -659, -1583,
    -709, -1583, -1583,  -498,   887, -1583, -1583, -1583, -1577, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,   845, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583,  -160,    37,   -83,    47,
     252, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,   450,
    -465,  -969, -1583,  -468,  -961, -1583,  -881,   -78,   -75, -1583,
    -595, -1509, -1583,  -427, -1583, -1583,  1444, -1583, -1583, -1583,
    1001,  1119,    81, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583,  -732,  -221, -1583,   986, -1583, -1583, -1583,
    1186, -1583, -1583, -1583,  -357, -1583, -1583,  -424, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583,  -191, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583,   993,  -737,  -244,  -765,  -759, -1583, -1583, -1381,  -968,
   -1583, -1583, -1583, -1266,   -98, -1517, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583,   211,  -535, -1583, -1583, -1583,
     729, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583, -1583,
   -1583, -1583, -1583, -1583, -1583, -1294,  -778, -1583
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    17,   159,    58,   199,    18,   184,   191,  1697,
    1499,  1610,   790,   568,   165,   569,   109,    20,    21,    49,
      50,    51,    98,    22,    41,    42,   703,   704,  1429,  1430,
     635,   706,  1565,  1654,   707,   708,  1190,   709,   903,   710,
     711,   712,   713,  1423,   911,   192,   193,    37,    38,    39,
     214,    73,    74,    75,    76,    24,   440,   503,   281,   122,
      25,   174,   282,   175,   205,   441,   154,   924,  1201,   716,
     504,   717,   802,   620,   792,  1154,   570,  1027,  1608,  1028,
    1609,   719,   571,   720,   746,   974,  1578,   572,   721,   722,
     723,   724,   725,   726,   727,   673,   728,   943,  1435,  1195,
     729,   573,   988,  1591,   989,  1592,   991,  1593,   574,   979,
    1584,   575,   803,  1632,   576,  1345,  1346,  1062,   926,   577,
     964,  1192,   578,   856,  1202,   731,   579,   580,  1054,   581,
    1330,  1704,  1331,  1761,   582,  1109,  1541,   583,   804,  1523,
    1764,  1525,  1765,  1639,  1806,   585,   497,  1446,  1573,  1243,
    1245,   971,   510,   967,   742,  1662,  1734,   498,   499,   500,
     874,   875,   486,   876,   877,   487,  1045,   896,   897,  1666,
     600,   457,   304,   305,   211,   297,    85,   131,    27,   180,
     444,    99,   100,   196,   101,    28,    55,   125,   177,    29,
     292,   508,   505,   965,   446,   209,   210,    83,   128,   448,
      30,   178,   294,   898,   586,   291,   374,   375,  1143,   632,
     226,   376,   867,  1545,  1151,   860,   483,   377,   601,  1391,
     879,   606,  1396,   602,  1392,   603,  1393,   605,  1395,   609,
    1400,   610,  1547,   611,  1402,   612,  1548,   613,  1404,   614,
    1549,   615,  1406,   616,  1408,   638,    31,   105,   201,   384,
     639,    32,   106,   202,   385,   643,    33,   104,   200,   485,
     886,   587,   808,  1790,   809,  1013,  1781,  1782,  1783,  1014,
    1026,  1309,  1303,  1298,  1493,  1253,   588,   972,  1576,   973,
    1577,   998,  1597,   995,  1595,  1015,   793,   589,   996,  1596,
    1016,   590,  1259,  1673,  1260,  1674,  1261,  1675,   983,  1588,
     993,  1594,   787,   794,   591,  1751,  1035,   592
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      23,   854,   373,   442,   880,   296,   730,   986,   855,   936,
     869,   786,   871,   715,   873,    66,    77,  1170,    78,   641,
     215,   636,   511,   718,  1019,  1040,   740,  1276,  1483,  1046,
    1048,   596,   378,    54,  1145,  1286,  1147,  1278,  1149,   752,
    1425,   927,   928,   623,  1012,  1294,  1012,  1059,  1006,  1304,
    1017,  1306,  1021,   141,  1550,     2,  1060,  1007,  -139,  1032,
     631,  1660,     3,    34,    35,  1007,   132,   133,  1036,   621,
      77,    77,    77,   167,  1310,    59,   826,   827,  1684,  1317,
    1733,    60,    56,    94,  -865,     4,   501,     5,   117,     6,
     737,  1444,   813,   905,   108,     7,  -872,    87,  1319,   151,
      64,   156,  1157,   212,  -879,     8,   921,  1300,  1420,   715,
    1301,     9,  1502,   118,    77,    77,    77,    77,    95,   718,
     781,   783,   824,  -488,   204,   826,   827,  1760,    57,   637,
     642,    65,  1476,  1042,   495,    10,   102,   449,  1302,   212,
     814,   815,   811,   114,   115,   116,   887,   185,   186,  1730,
    1735,  1736,  1778,  -865,   197,   502,   597,   179,  -865,   753,
     754,    84,   847,   848,   213,  -872,   598,  1042,  1747,  1748,
    -872,  1172,   298,  -879,  1445,   749,  -865,  1043,  -879,   299,
      11,    12,   155,   108,  1805,   805,   812,  1630,  -872,   671,
      40,   484,  -488,   220,   715,  1136,  -879,  -488,   496,  1044,
     213,  1332,   484,   207,   718,   152,  1263,   715,    52,  1336,
    1252,   847,   848,   301,  1603,  -488,  1274,   718,   166,  1277,
     221,  1293,   599,   372,  1787,  1788,   153,   227,   228,    53,
    1167,   624,   672,  1044,   373,  1061,  1337,   152,    13,    86,
     134,    36,   495,    13,  1167,   135,    52,   212,   136,   625,
      87,   137,   383,  1696,   302,   626,   755,   622,   153,    14,
     818,   819,  1116,   373,    14,   373,   451,    53,   824,  1167,
     825,   826,   827,   828,  1167,   756,   303,  1560,   829,   888,
     373,   373,  1175,    15,   891,  1197,  1567,    64,    96,   738,
    1334,  1461,   138,  1167,    16,   139,  1042,    89,   714,    97,
    1462,   764,   736,  1167,   741,  1320,  1340,  1167,   213,  1371,
     678,   679,  1173,    43,  1295,  1296,  1341,   443,    65,  1335,
     447,   187,  1480,   373,   373,    67,   188,   302,  1184,   189,
    1042,  1042,   137,   985,   807,  1193,  1691,    44,    45,    46,
    1505,  1043,  1297,  1659,    52,    59,   456,   999,  1287,   303,
    1042,    60,  1175,  1342,    68,  1784,  1568,   847,   848,  1042,
    1615,  1167,  1044,   715,  1168,    53,  1794,  1169,    47,  1758,
     805,   472,  1343,   718,    13,   805,   190,  1344,    48,  1451,
     373,   373,   373,  1570,   373,   373,  1332,   594,   373,    88,
     373,  1332,   373,  1194,   373,    14,  1044,  1044,  1752,   944,
    1753,   566,   923,    13,  1755,  1756,  1822,  1823,   908,   595,
     858,   859,   861,   715,   863,   864,  1044,   918,   868,    52,
     870,    69,   872,   718,    14,  1044,   894,    64,    13,   715,
     300,  1333,    13,   889,   484,  1779,  1050,   892,  1456,   718,
      53,  1187,  1416,   507,    90,   509,  1506,    92,   895,    14,
      70,   206,  1457,    14,   906,   372,   734,    64,    65,   814,
     815,    93,  1347,  1802,  1417,   718,   718,   718,   718,   718,
     718,   718,   718,   718,   718,   718,   107,   718,   718,   718,
     718,   718,   718,  1520,   372,  1450,   372,    13,    65,  1313,
     144,   145,    13,   146,    52,  1183,   225,   674,   676,  1318,
    -808,   372,   372,   705,  1139,   735,   113,    13,    14,   739,
    1626,  1602,  1522,    14,   634,    53,  1196,   372,   750,   634,
    1153,  1605,  1150,    71,   119,   984,    52,  1152,    14,   108,
      13,  1479,    72,    13,  1397,   994,   373,  1556,   997,  1437,
    1438,  1439,  1398,    64,   372,   372,  1489,    53,  1247,  1248,
     373,    14,  1050,  1012,    14,  1175,  1482,  1051,  1159,  1262,
     675,  1175,    13,    13,  1268,  1269,   484,  1271,  1012,  1273,
    1137,  1275,   506,  1175,    65,   152,   295,    13,   120,   818,
     819,  1058,   970,    14,    14,   814,   815,   824,  1637,   634,
     826,   827,   828,  1066,  1070,   853,   153,   829,    14,  1052,
    1644,   372,   372,   372,   634,   372,   372,  1175,  1084,   372,
      87,   372,  1410,   372,   121,   372,    43,    13,  1175,  1113,
    1292,   473,  1616,  1171,   142,    13,  1110,  1292,  1265,    97,
      13,   885,   473,  1179,  1646,   450,    79,    80,    14,    81,
      44,    45,    46,   117,   634,  1689,    14,  1837,   474,   475,
    1188,    14,  1473,  1189,  1843,   473,   147,   634,   373,   474,
     475,   164,  1292,    13,  1520,  1475,   373,    82,  1240,   373,
      91,    47,  1568,  1174,    13,  1623,   847,   848,   150,  1292,
    1161,    48,   474,   475,    14,  1409,  1407,  1512,  1155,  1521,
     634,   152,   914,   160,   484,    14,  1162,  1569,   373,  1163,
    1292,   634,   930,   931,  1706,   818,   819,    13,  1433,  1496,
    1442,   161,   153,   824,  1589,   825,   826,   827,   828,   937,
     938,   939,   940,   829,   941,  1715,  1652,    13,    14,   945,
    1244,  1510,   507,   162,   634,   476,   640,   454,    77,   477,
     455,   379,  1497,   456,  1251,   618,   476,   757,    14,   976,
     477,   373,   373,   373,   634,   163,   380,   372,   373,  1237,
     176,   381,   373,   382,   619,   765,   758,   373,   373,   476,
     373,   372,   373,   477,   373,   373,  1625,    13,  1079,   807,
      95,   768,   977,  1249,   766,   194,   884,   807,  1258,   123,
     844,   845,   846,  1729,  1080,   124,   456,  1411,    14,  1643,
     769,   978,   847,   848,   198,   478,   373,   373,  1326,   479,
    1053,  1279,   480,   110,   111,   112,   478,   203,  1360,   114,
     479,  1728,  1759,   480,  1327,  1561,  1365,   481,   484,   513,
     514,   805,  1140,   482,  1361,  1460,  1155,  1155,   481,   478,
     219,  1466,  1366,   479,   482,  1424,   480,  1332,   484,   520,
     208,  1651,  1141,   222,  1426,   522,  1135,   168,   169,   170,
     171,   481,   223,   126,    13,  1427,  1428,   482,   473,   127,
    1358,   645,   646,   647,   648,   649,   650,   651,   652,   372,
      13,   224,    13,  1160,   114,    14,   116,   372,   129,   225,
     372,   634,   529,   530,   130,   474,   475,  1421,   157,  1580,
     653,    14,   293,    14,   158,   473,  1509,   634,   172,   634,
     654,   655,   656,   437,   173,  1586,   473,  1587,   373,   372,
    1490,  1153,  1241,  1491,   439,  1601,  1492,  1745,  1242,  1604,
     373,  1175,   474,   475,   730,  1175,  1607,   216,   217,  1612,
    1613,   715,  1175,   474,   475,  1175,  1175,   373,  1394,   532,
     533,   718,   484,  1700,   566,   923,  1144,  1411,  1411,  1175,
    1191,   925,   925,   925,   306,  1238,   438,  1656,  1793,   445,
    1246,   484,   372,   372,   372,  1146,   452,  1418,   453,   372,
     935,   484,   476,   372,   488,  1148,   477,   458,   372,   372,
      64,   372,   459,   372,   935,   372,   372,   484,  1780,  1780,
     484,  1449,   460,  1817,  1459,   461,  1789,   544,   545,   546,
    1780,  1789,   484,   484,   148,   149,  1711,  1766,   462,   476,
     463,    65,   466,   477,   114,   115,   116,   372,   372,   464,
     476,   558,   467,  1598,   477,   566,   923,   484,   373,   373,
    1307,  1300,  1815,  1308,   373,   468,   745,    44,    45,    46,
    1780,  1780,   478,   471,   491,  1478,   479,   489,  1447,   480,
      13,  1744,   469,   564,   490,  1732,   470,   181,   182,  1029,
    1030,   492,  1694,  1488,   481,  1314,  1315,  1698,   494,  1495,
     482,    14,   181,   182,   183,   493,  1500,   634,  1501,   478,
     216,   217,   218,   479,   512,  1264,   480,  1581,   593,  1633,
     478,  1842,   607,   629,   479,  1153,  1448,   480,   608,   373,
     644,   481,  1023,  1024,  1025,   670,  1373,   482,   677,   935,
     743,  1528,   481,   759,   899,   900,   901,   852,   482,    61,
      62,    63,   744,  1538,   473,  1773,  1774,   751,  1543,   372,
    1775,   760,  1399,   761,   762,   763,   767,  1562,   764,   770,
     771,   372,   774,   775,   788,   776,   789,   777,   778,   473,
     866,   474,   475,   779,   810,    16,   850,   640,   372,   935,
     851,   473,   865,   881,   890,   913,   473,  1808,   915,   919,
     933,  1809,  1811,   934,   935,   969,   474,   475,   942,  1557,
     373,   975,  1033,   980,   981,   982,  1034,  1037,   474,   475,
    1038,   373,  1039,   474,   475,  1041,   925,  1047,  1049,  1077,
    1115,   944,  1142,  1138,  1156,  1165,  1833,  1166,  1175,   373,
    1551,  1176,  1178,  1180,  1181,  1182,  1566,  1186,  1239,  1244,
    1256,  1558,  1292,  1266,  1299,  1731,  1312,  1325,   283,  1441,
    1338,   715,   284,  1631,  1348,  1328,  1600,  1368,   476,  1571,
    1329,   718,   477,  1401,  1350,  1339,   285,   286,  1403,   372,
     372,   287,   288,   289,   290,   372,  1405,  1349,  1351,   925,
    1352,  1614,  1415,   476,  1422,  1353,   597,   477,  1650,  1363,
    1364,  1369,   715,   715,  1653,   476,   598,  1412,   597,   477,
     476,  1434,   718,   718,   477,  1440,  1452,   584,   598,  1453,
    1454,  1464,   373,  1465,   373,  1772,   604,  1471,  1474,  1477,
    1484,  1487,  1494,  1515,    13,  1504,   617,   715,   478,  1511,
    1516,  1517,   479,  1531,  1463,   480,   628,   718,  1518,  1519,
     372,  1530,  1533,  1539,  1635,    14,  1544,  1546,   619,  1554,
     481,  1555,   599,   478,  1572,  1582,   482,   479,  1579,   732,
     480,  1583,  1598,  1606,   599,   478,  1611,  1624,  1627,   479,
     478,  1629,   480,  1648,   479,   481,  1467,   480,   473,  1657,
    1658,   482,  1670,  1631,  1667,  1668,  1671,   481,  1676,   935,
     772,   773,   481,   482,  1677,  1678,  1682,   473,   482,  1683,
    1685,   785,  1705,  1686,  1738,   474,   475,  1707,  1739,  1741,
     473,  1742,  1750,   785,   795,   796,   797,   798,   799,  1743,
    1754,   372,  1552,  1796,   474,   475,  1797,  1816,   373,  1798,
    1762,  1767,   372,  1791,  1792,  1799,  1800,   474,   475,  1801,
    1818,   705,  1819,  1824,  1825,  1831,  1826,  1827,   373,  1829,
     372,  1830,  1839,  1832,  1031,   857,  1838,   935,  1712,  1771,
    1840,  1746,   140,    19,   195,  1719,   307,   143,  1722,  1723,
     925,   925,   925,   968,  1432,   935,  1724,   935,  1725,   935,
    1726,   935,    26,   935,   883,   935,  1436,   935,  1718,   935,
    1574,   935,   476,   935,  1628,   935,   477,  1514,   935,   906,
     935,  1663,   935,  1575,   935,  1443,   935,  1664,   935,  1814,
    1665,   476,   103,   747,  1795,   477,   733,  1693,   373,   465,
     748,  1486,  1020,     0,   476,   893,   814,   815,   477,     0,
       0,     0,     0,   372,     0,   372,   902,     0,     0,     0,
     907,     0,     0,   910,     0,   912,     0,  1174,     0,     0,
     917,     0,     0,   922,     0,     0,     0,   929,     0,     0,
       0,   932,   478,     0,     0,     0,   479,     0,  1468,   480,
       0,     0,     0,     0,  1655,     0,     0,     0,     0,     0,
    1661,   478,     0,     0,   481,   479,     0,  1469,   480,   966,
     482,     0,     0,     0,   478,     0,     0,     0,   479,     0,
    1470,   480,     0,   481,     0,     0,     0,     0,     0,   482,
       0,     0,     0,   785,   987,     0,   481,   990,     0,   992,
       0,     0,   482,     0,     0,     0,     0,     0,     0,  1000,
    1001,  1002,  1003,  1004,  1005,     0,  1011,     0,  1011,     0,
       0,     0,     0,     0,   816,   817,   818,   819,   820,   372,
       0,   821,   822,   823,   824,     0,   825,   826,   827,   828,
       0,     0,     0,     0,   829,     0,   830,   831,     0,   372,
       0,  1071,  1072,     0,     0,  1073,  1074,  1075,  1076,     0,
    1078,     0,  1081,  1082,  1083,  1085,  1086,  1087,  1088,  1089,
    1090,  1092,  1093,  1094,  1095,  1096,  1097,  1098,  1099,  1100,
    1101,  1102,     0,  1111,     0,     0,  1114,     0,     0,  1117,
       0,     0,     0,     0,  1053,     0,   657,   658,   659,   660,
     661,   662,   663,   664,   837,   838,   839,   840,   841,   842,
     843,   844,   845,   846,     0,   665,     0,     0,     0,   372,
       0,     0,  1777,   847,   848,   666,     0,     0,     0,     0,
     814,   815,     0,     0,     0,   667,   668,   669,     0,   907,
       0,     0,     0,     0,  1177,     0,     0,     0,     0,     0,
       0,  1053,     0,     0,     0,  1185,     0,     0,     0,     0,
       0,     0,     0,     0,   -81,  1813,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   814,   815,     0,     0,     0,
       0,  1203,  1205,  1207,  1209,  1211,  1213,  1215,  1217,  1219,
    1221,  1223,  1224,  1226,  1228,  1230,  1232,  1234,  1236,  1835,
       0,  1836,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1255,   935,  1257,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1841,  1204,  1206,  1208,  1210,  1212,  1214,
    1216,  1218,  1220,  1222,     0,  1225,  1227,  1229,  1231,  1233,
    1235,     0,     0,   795,  1289,     0,     0,     0,   816,   817,
     818,   819,   820,     0,     0,   821,     0,  1311,   824,   785,
     825,   826,   827,   828,     0,  1316,     0,     0,   829,   785,
     830,   831,     0,     0,  1321,     0,  1322,     0,  1323,     0,
    1324,     0,     0,     0,     0,   946,   947,   948,   949,   950,
     951,   952,   953,   816,   817,   818,   819,   820,   954,   955,
     821,   822,   823,   824,   956,   825,   826,   827,   828,     0,
       0,   814,   815,   829,   957,   830,   831,   958,   959,     0,
       0,   832,   833,   834,   960,   961,   962,   835,     0,     0,
       0,   840,   841,   842,   843,   844,   845,   846,     0,  1362,
       0,     0,     0,  1367,     0,     0,     0,   847,   848,     0,
       0,     0,     0,  1374,  1375,  1376,  1377,  1378,  1379,  1380,
    1381,  1382,  1383,  1384,  1385,  1386,  1387,  1388,  1389,  1390,
     963,   836,     0,   837,   838,   839,   840,   841,   842,   843,
     844,   845,   846,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   847,   848,  1413,     0,     0,   566,   923,     0,
    1414,     0,     0,     0,     0,  1419,     0,     0,     0,     0,
    1321,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1431,     0,     0,   935,   816,
     817,   818,   819,   820,     0,     0,   821,   822,   823,   824,
       0,   825,   826,   827,   828,     0,     0,     0,     0,   829,
       0,   830,   831,     0,     0,     0,     0,   832,   833,   834,
       0,     0,   935,   835,   935,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   935,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   836,     0,   837,
     838,   839,   840,   841,   842,   843,   844,   845,   846,     0,
       0,     0,     0,     0,     0,     0,  1481,     0,   847,   848,
       0,     0,   849,     0,  1485,  1011,     0,     0,     0,     0,
       0,   780,     0,     0,     0,     0,     0,   308,     0,     0,
       0,     0,     0,   309,     0,   814,   815,     0,     0,   310,
       0,     0,     0,  1507,  1508,     0,     0,     0,     0,   311,
       0,  1321,     0,     0,     0,     0,     0,   312,     0,     0,
       0,     0,     0,     0,  1524,     0,  1526,  1564,  1529,     0,
       0,     0,   313,     0,  1532,     0,   814,   815,  1535,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   814,   815,     0,  1559,     0,
       0,     0,     0,     0,  1563,     0,     0,   732,     0,     0,
       0,     0,     0,   816,   817,   818,   819,   820,     0,     0,
     821,   822,   823,   824,     0,   825,   826,   827,   828,     0,
       0,     0,     0,   829,    64,   830,   831,     0,     0,     0,
     785,   832,   833,   834,     0,     0,     0,   370,     0,     0,
       0,     0,     0,     0,   816,   817,   818,   819,   820,     0,
       0,   821,   822,   823,   824,    65,   825,   826,   827,   828,
       0,     0,     0,     0,   829,     0,   830,   831,     0,  1617,
    1618,  1619,   832,   833,   834,     0,     0,     0,   835,     0,
       0,   836,     0,   837,   838,   839,   840,   841,   842,   843,
     844,   845,   846,     0,     0,     0,  1640,     0,  1641,     0,
       0,   371,   847,   848,  1645,   818,   819,     0,     0,   814,
     815,     0,     0,   824,     0,   825,   826,   827,   828,  1647,
       0,  1649,   836,   829,   837,   838,   839,   840,   841,   842,
     843,   844,   845,   846,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   847,   848,  1669,     0,     0,  1672,     0,
       0,     0,     0,     0,     0,     0,  1679,  1680,  1681,     0,
       0,     0,     0,  1688,     0,     0,  1690,     0,     0,  1692,
       0,     0,   785,  1695,     0,     0,     0,   785,  1699,     0,
    1701,  1702,     0,     0,     0,     0,     0,     0,   842,   843,
     844,   845,   846,     0,     0,  1709,     0,     0,     0,     0,
       0,     0,   847,   848,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1727,
       0,     0,     0,     0,     0,     0,     0,   816,   817,   818,
     819,   820,     0,     0,   821,   822,   823,   824,     0,   825,
     826,   827,   828,   785,     0,     0,     0,   829,     0,   830,
     831,     0,     0,     0,     0,   832,   833,   834,     0,     0,
       0,   835,     0,     0,     0,  1757,     0,     0,     0,     0,
       0,     0,     0,  1763,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1770,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1776,     0,     0,
       0,     0,     0,  1785,  1786,   836,     0,   837,   838,   839,
     840,   841,   842,   843,   844,   845,   846,     0,     0,     0,
       0,     0,     0,  1803,  1804,     0,   847,   848,     0,     0,
       0,   566,   923,     0,  1807,     0,     0,     0,  1810,  1812,
     680,     0,     0,     0,   513,   514,     3,     0,   681,   682,
     683,     0,   684,     0,   515,   516,   517,   518,   519,     0,
       0,  1828,     0,     0,   520,   685,   521,   686,   687,     0,
     522,     0,     0,  1834,     0,     0,     0,   688,   523,   689,
       0,   690,     0,   691,   524,     0,     0,   525,     0,     8,
     526,   692,     0,   693,   527,     0,     0,   694,   695,     0,
       0,     0,     0,     0,   696,     0,     0,   529,   530,     0,
     314,   315,   316,     0,   318,   319,   320,   321,   322,   531,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
     334,     0,   336,   337,   338,     0,     0,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   532,   533,   697,   698,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     535,   536,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   699,   700,   701,     0,     0,
       0,     0,     0,     0,     0,    64,     0,     0,     0,     0,
       0,     0,     0,   537,   538,   539,   540,   541,     0,   542,
       0,   543,   544,   545,   546,     0,   152,    13,   547,   548,
     549,   550,   551,   552,   553,   554,    65,   702,   556,   557,
       0,     0,     0,     0,     0,     0,   558,   153,    14,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   559,   560,   561,     0,    15,     0,     0,
     562,   563,     0,     0,   513,   514,     0,     0,   564,     0,
     565,     0,   566,   567,   515,   516,   517,   518,   519,     0,
       0,     0,     0,     0,   520,     0,   521,     0,     0,     0,
     522,     0,   473,     0,     0,     0,     0,     0,   523,     0,
       0,     0,     0,     0,   524,     0,     0,   525,     0,     0,
     526,     0,  1007,     0,   527,     0,     0,     0,     0,   474,
     475,     0,     0,     0,   528,     0,     0,   529,   530,     0,
     314,   315,   316,     0,   318,   319,   320,   321,   322,   531,
     324,   325,   326,   327,   328,   329,   330,   331,   332,   333,
     334,     0,   336,   337,   338,     0,     0,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   532,   533,   534,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     535,   536,     0,     0,     0,     0,   476,     0,     0,     0,
     477,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    64,     0,     0,     0,     0,
       0,     0,     0,   537,   538,   539,   540,   541,     0,   542,
     805,   543,   544,   545,   546,     0,     0,     0,   547,   548,
     549,   550,   551,   552,   553,   554,   806,   555,   556,   557,
       0,     0,     0,     0,     0,     0,   558,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   478,     0,     0,     0,
     479,     0,     0,  1008,   560,   561,     0,    15,     0,     0,
     562,   563,     0,     0,     0,   513,   514,     0,  1009,     0,
    1010,     0,   566,   567,   482,   515,   516,   517,   518,   519,
       0,     0,     0,     0,     0,   520,     0,   521,     0,     0,
       0,   522,     0,   473,     0,     0,     0,     0,     0,   523,
       0,     0,     0,     0,     0,   524,     0,     0,   525,     0,
       0,   526,     0,     0,     0,   527,     0,     0,     0,     0,
     474,   475,     0,     0,     0,   528,     0,     0,   529,   530,
       0,   314,   315,   316,     0,   318,   319,   320,   321,   322,
     531,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,     0,   336,   337,   338,     0,     0,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   532,   533,   534,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   535,   536,     0,     0,     0,     0,   476,     0,     0,
       0,   477,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   537,   538,   539,   540,   541,     0,
     542,   805,   543,   544,   545,   546,     0,   473,     0,   547,
     548,   549,   550,   551,   552,   553,   554,   806,   555,   556,
     557,     0,     0,     0,     0,     0,     0,   558,     0,     0,
       0,     0,     0,     0,   474,   475,     0,   478,     0,     0,
       0,   479,     0,     0,  1008,   560,   561,     0,    15,     0,
       0,   562,   563,     0,     0,     0,   513,   514,     0,  1009,
       0,  1018,     0,   566,   567,   482,   515,   516,   517,   518,
     519,     0,     0,     0,     0,     0,   520,     0,   521,     0,
       0,     0,   522,     0,   623,     0,     0,     0,     0,     0,
     523,     0,     0,     0,     0,     0,   524,     0,     0,   525,
       0,     0,   526,     0,     0,     0,   527,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   528,     0,     0,   529,
     530,   476,   314,   315,   316,   477,   318,   319,   320,   321,
     322,   531,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,     0,   336,   337,   338,     0,     0,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   532,   533,   534,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   478,   535,   536,     0,   479,     0,  1472,   480,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   481,     0,     0,     0,    64,     0,   482,
       0,     0,     0,     0,     0,   537,   538,   539,   540,   541,
       0,   542,     0,   543,   544,   545,   546,     0,   473,     0,
     547,   548,   549,   550,   551,   552,   553,   554,    65,   555,
     556,   557,     0,     0,     0,     0,     0,     0,   558,     0,
       0,     0,     0,     0,     0,   474,   475,     0,     0,     0,
       0,     0,   624,     0,     0,   559,   560,   561,     0,    15,
       0,     0,   562,   563,     0,     0,     0,   513,   514,     0,
    1288,     0,   565,     0,   566,   567,   626,   515,   516,   517,
     518,   519,     0,     0,     0,     0,     0,   520,     0,   521,
       0,     0,     0,   522,     0,     0,     0,     0,     0,     0,
       0,   523,     0,     0,     0,     0,     0,   524,     0,     0,
     525,     0,     0,   526,     0,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,     0,     0,
     529,   530,   476,   314,   315,   316,   477,   318,   319,   320,
     321,   322,   531,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,     0,   336,   337,   338,     0,     0,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   532,   533,   697,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   478,   535,   536,     0,   479,     0,  1585,   480,
       0,     0,   904,     0,     0,     0,     0,     0,   699,   700,
     701,     0,     0,     0,   481,     0,     0,     0,    64,     0,
     482,     0,     0,     0,     0,     0,   537,   538,   539,   540,
     541,     0,   542,     0,   543,   544,   545,   546,   473,     0,
       0,   547,   548,   549,   550,   551,   552,   553,   554,    65,
     555,   556,   557,     0,     0,     0,     0,     0,     0,   558,
       0,     0,     0,     0,     0,   474,   475,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   559,   560,   561,     0,
      15,     0,     0,   562,   563,     0,     0,   513,   514,     0,
       0,   564,     0,   565,     0,   566,   567,   515,   516,   517,
     518,   519,     0,     0,     0,     0,     0,   520,     0,   521,
       0,     0,     0,   522,     0,     0,     0,     0,     0,     0,
       0,   523,     0,     0,     0,     0,     0,   524,     0,     0,
     525,     0,     0,   526,     0,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,     0,     0,
     529,   530,   476,   314,   315,   316,   477,   318,   319,   320,
     321,   322,   531,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,     0,   336,   337,   338,     0,     0,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   532,   533,   697,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   478,   535,   536,     0,   479,     0,  1590,   480,
       0,     0,   920,     0,     0,     0,     0,     0,   699,   700,
     701,     0,     0,     0,   481,     0,     0,     0,    64,     0,
     482,     0,     0,     0,     0,     0,   537,   538,   539,   540,
     541,     0,   542,     0,   543,   544,   545,   546,   473,     0,
       0,   547,   548,   549,   550,   551,   552,   553,   554,    65,
     555,   556,   557,     0,     0,     0,     0,     0,     0,   558,
       0,     0,     0,     0,     0,   474,   475,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   559,   560,   561,     0,
      15,     0,     0,   562,   563,     0,     0,   513,   514,     0,
       0,   564,     0,   565,     0,   566,   567,   515,   516,   517,
     518,   519,     0,     0,     0,     0,     0,   520,  1720,   521,
     686,     0,     0,   522,     0,     0,     0,     0,     0,     0,
       0,   523,     0,     0,     0,     0,     0,   524,     0,     0,
     525,     0,     0,   526,   692,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,     0,     0,
     529,   530,   476,   314,   315,   316,   477,   318,   319,   320,
     321,   322,   531,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,     0,   336,   337,   338,     0,     0,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   532,   533,   534,
    1721,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   478,   535,   536,     0,   479,     0,  1622,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   481,     0,     0,     0,    64,     0,
     482,     0,     0,     0,     0,     0,   537,   538,   539,   540,
     541,     0,   542,     0,   543,   544,   545,   546,   473,     0,
       0,   547,   548,   549,   550,   551,   552,   553,   554,    65,
     555,   556,   557,     0,     0,     0,     0,     0,     0,   558,
       0,     0,     0,     0,     0,   474,   475,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   559,   560,   561,     0,
      15,     0,     0,   562,   563,     0,     0,   513,   514,     0,
       0,   564,     0,   565,     0,   566,   567,   515,   516,   517,
     518,   519,     0,     0,     0,     0,     0,   520,     0,   521,
       0,     0,     0,   522,     0,     0,     0,     0,     0,     0,
       0,   523,     0,     0,     0,     0,     0,   524,     0,     0,
     525,     0,     0,   526,     0,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,     0,     0,
     529,   530,   476,   314,   315,   316,   477,   318,   319,   320,
     321,   322,   531,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,     0,   336,   337,   338,     0,     0,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   532,   533,   697,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   478,   535,   536,     0,   479,     0,  1710,   480,
       0,     0,     0,     0,     0,     0,     0,     0,   699,   700,
     701,     0,     0,     0,   481,     0,     0,     0,    64,     0,
     482,     0,     0,     0,     0,     0,   537,   538,   539,   540,
     541,     0,   542,     0,   543,   544,   545,   546,     0,     0,
       0,   547,   548,   549,   550,   551,   552,   553,   554,    65,
     555,   556,   557,     0,     0,     0,     0,     0,     0,   558,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   559,   560,   561,     0,
      15,     0,     0,   562,   563,     0,     0,   513,   514,     0,
       0,   564,     0,   565,     0,   566,   567,   515,   516,   517,
     518,   519,     0,     0,     0,     0,     0,   520,     0,   521,
       0,     0,     0,   522,     0,     0,     0,     0,     0,     0,
       0,   523,     0,     0,     0,     0,     0,   524,     0,     0,
     525,     0,     0,   526,     0,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,     0,     0,
     529,   530,  1055,   314,   315,   316,     0,   318,   319,   320,
     321,   322,   531,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,     0,   336,   337,   338,     0,     0,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   532,   533,   534,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   535,   536,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   537,   538,   539,   540,
     541,     0,   542,   805,   543,   544,   545,   546,     0,     0,
       0,   547,   548,   549,   550,   551,   552,   553,   554,   806,
     555,   556,   557,     0,     0,     0,     0,     0,     0,   558,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   559,   560,   561,     0,
      15,     0,     0,   562,   563,     0,     0,   513,   514,     0,
       0,  1056,     0,   565,  1057,   566,   567,   515,   516,   517,
     518,   519,     0,     0,     0,     0,     0,   520,     0,   521,
       0,     0,     0,   522,     0,     0,     0,     0,     0,     0,
       0,   523,     0,     0,     0,     0,     0,   524,     0,     0,
     525,     0,     0,   526,     0,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,     0,     0,
     529,   530,     0,   314,   315,   316,     0,   318,   319,   320,
     321,   322,   531,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,     0,   336,   337,   338,     0,     0,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   532,   533,   697,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   535,   536,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1198,  1199,
    1200,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   537,   538,   539,   540,
     541,     0,   542,     0,   543,   544,   545,   546,     0,     0,
       0,   547,   548,   549,   550,   551,   552,   553,   554,    65,
     555,   556,   557,     0,     0,     0,     0,     0,     0,   558,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   559,   560,   561,     0,
      15,     0,     0,   562,   563,     0,     0,     0,     0,   513,
     514,   564,     0,   565,     0,   566,   567,   800,     0,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,   801,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,     0,
       0,   513,   514,   564,   627,   565,     0,   566,   567,   800,
       0,   515,   516,   517,   518,   519,     0,     0,     0,     0,
       0,   520,     0,   521,     0,     0,     0,   522,     0,     0,
       0,     0,     0,     0,     0,   523,     0,     0,     0,     0,
       0,   524,     0,     0,   525,   801,     0,   526,     0,     0,
       0,   527,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   528,     0,     0,   529,   530,     0,   314,   315,   316,
       0,   318,   319,   320,   321,   322,   531,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   333,   334,     0,   336,
     337,   338,     0,     0,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   532,   533,   534,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   535,   536,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     537,   538,   539,   540,   541,     0,   542,   805,   543,   544,
     545,   546,     0,     0,     0,   547,   548,   549,   550,   551,
     552,   553,   554,   806,   555,   556,   557,     0,     0,     0,
       0,     0,     0,   558,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     559,   560,   561,     0,    15,     0,     0,   562,   563,     0,
       0,     0,     0,   513,   514,   564,     0,   565,     0,   566,
     567,   800,     0,   515,   516,   517,   518,   519,     0,     0,
       0,     0,     0,   520,     0,   521,     0,     0,     0,   522,
       0,     0,     0,     0,     0,     0,     0,   523,     0,     0,
       0,     0,     0,   524,     0,     0,   525,   801,     0,   526,
       0,     0,     0,   527,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   528,     0,     0,   529,   530,     0,   314,
     315,   316,     0,   318,   319,   320,   321,   322,   531,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
       0,   336,   337,   338,     0,     0,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   532,   533,   534,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   535,
     536,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   537,   538,   539,   540,   541,     0,   542,     0,
     543,   544,   545,   546,     0,     0,     0,   547,   548,   549,
     550,   551,   552,   553,   554,    65,   555,   556,   557,     0,
       0,     0,     0,     0,     0,   558,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   559,   560,   561,     0,    15,     0,     0,   562,
     563,     0,     0,     0,     0,   513,   514,   564,   881,   565,
       0,   566,   567,   800,     0,   515,   516,   517,   518,   519,
       0,     0,     0,     0,     0,   520,     0,   521,     0,     0,
       0,   522,     0,     0,     0,     0,     0,     0,     0,   523,
       0,     0,     0,     0,     0,   524,     0,     0,   525,   801,
       0,   526,     0,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,     0,     0,   529,   530,
       0,   314,   315,   316,     0,   318,   319,   320,   321,   322,
     531,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,     0,   336,   337,   338,     0,     0,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   532,   533,   534,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   535,   536,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   537,   538,   539,   540,   541,     0,
     542,     0,   543,   544,   545,   546,     0,     0,     0,   547,
     548,   549,   550,   551,   552,   553,   554,    65,   555,   556,
     557,     0,     0,     0,     0,     0,     0,   558,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   559,   560,   561,     0,    15,     0,
       0,   562,   563,     0,     0,   513,   514,     0,     0,   564,
       0,   565,     0,   566,   567,   515,   516,   517,   518,   519,
       0,     0,     0,     0,     0,   520,     0,   521,     0,     0,
       0,   522,     0,     0,     0,     0,     0,     0,     0,   523,
       0,     0,     0,     0,     0,   524,     0,     0,   525,     0,
       0,   526,     0,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,     0,     0,   529,   530,
    1250,   314,   315,   316,     0,   318,   319,   320,   321,   322,
     531,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,     0,   336,   337,   338,     0,     0,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   532,   533,   534,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   535,   536,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   537,   538,   539,   540,   541,     0,
     542,   805,   543,   544,   545,   546,     0,     0,     0,   547,
     548,   549,   550,   551,   552,   553,   554,   806,   555,   556,
     557,     0,     0,     0,     0,     0,     0,   558,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   559,   560,   561,     0,    15,     0,
       0,   562,   563,     0,     0,   513,   514,     0,     0,   564,
       0,   565,     0,   566,   567,   515,   516,   517,   518,   519,
       0,     0,     0,     0,     0,   520,     0,   521,     0,     0,
       0,   522,     0,     0,     0,     0,     0,     0,     0,   523,
       0,     0,     0,     0,     0,   524,     0,     0,   525,     0,
       0,   526,     0,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,     0,     0,   529,   530,
       0,   314,   315,   316,     0,   318,   319,   320,   321,   322,
     531,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,     0,   336,   337,   338,     0,     0,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   532,   533,   534,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   535,   536,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   537,   538,   539,   540,   541,     0,
     542,   805,   543,   544,   545,   546,     0,     0,     0,   547,
     548,   549,   550,   551,   552,   553,   554,   806,   555,   556,
     557,     0,     0,     0,     0,     0,     0,   558,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   559,   560,   561,     0,    15,     0,
       0,   562,   563,     0,     0,   513,   514,     0,     0,   564,
       0,   565,  1290,   566,   567,   515,   516,   517,   518,   519,
       0,     0,     0,     0,     0,   520,     0,   521,     0,     0,
       0,   522,     0,     0,     0,     0,     0,     0,     0,   523,
       0,     0,     0,     0,     0,   524,     0,     0,   525,     0,
       0,   526,     0,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,     0,     0,   529,   530,
       0,   314,   315,   316,     0,   318,   319,   320,   321,   322,
     531,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,     0,   336,   337,   338,     0,     0,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   532,   533,   534,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   535,   536,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   537,   538,   539,   540,   541,     0,
     542,   805,   543,   544,   545,   546,     0,     0,     0,   547,
     548,   549,   550,   551,   552,   553,   554,   806,   555,   556,
     557,     0,     0,     0,     0,     0,     0,   558,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   559,   560,   561,     0,    15,     0,
       0,   562,   563,     0,     0,   513,   514,     0,     0,   564,
       0,   565,  1305,   566,   567,   515,   516,   517,   518,   519,
       0,     0,     0,     0,     0,   520,     0,   521,     0,     0,
       0,   522,     0,     0,     0,     0,     0,     0,     0,   523,
       0,     0,     0,     0,     0,   524,     0,     0,   525,     0,
       0,   526,     0,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   528,     0,     0,   529,   530,
       0,   314,   315,   316,     0,   318,   319,   320,   321,   322,
     531,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,     0,   336,   337,   338,     0,     0,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   532,   533,   534,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   535,   536,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   537,   538,   539,   540,   541,     0,
     542,     0,   543,   544,   545,   546,     0,     0,     0,   547,
     548,   549,   550,   551,   552,   553,   554,    65,   555,   556,
     557,     0,     0,     0,     0,     0,     0,   558,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   559,   560,   561,     0,    15,     0,
       0,   562,   563,     0,     0,     0,     0,   513,   514,   564,
     627,   565,     0,   566,   567,   784,     0,   515,   516,   517,
     518,   519,     0,     0,     0,     0,     0,   520,     0,   521,
       0,     0,     0,   522,     0,     0,     0,     0,     0,     0,
       0,   523,     0,     0,     0,     0,     0,   524,     0,     0,
     525,     0,     0,   526,     0,     0,     0,   527,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   528,     0,     0,
     529,   530,     0,   314,   315,   316,     0,   318,   319,   320,
     321,   322,   531,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,     0,   336,   337,   338,     0,     0,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   532,   533,   534,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   535,   536,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   537,   538,   539,   540,
     541,     0,   542,     0,   543,   544,   545,   546,     0,     0,
       0,   547,   548,   549,   550,   551,   552,   553,   554,    65,
     555,   556,   557,     0,     0,     0,     0,     0,     0,   558,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   559,   560,   561,     0,
      15,     0,     0,   562,   563,     0,     0,     0,     0,   513,
     514,   564,     0,   565,     0,   566,   567,   791,     0,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,   805,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,   806,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,   909,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,   916,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,  1091,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,  1112,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1254,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,  1527,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,  1536,     0,   565,  1537,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,  1542,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,  1599,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,  1687,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,  1708,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,  1768,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,  1769,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,     0,     0,     0,     0,
       0,   558,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   559,   560,
     561,     0,    15,     0,     0,   562,   563,     0,     0,   513,
     514,     0,     0,   564,     0,   565,     0,   566,   567,   515,
     516,   517,   518,   519,     0,     0,     0,     0,     0,   520,
       0,   521,     0,     0,     0,   522,     0,     0,     0,     0,
       0,     0,     0,   523,     0,     0,     0,     0,     0,   524,
       0,     0,   525,     0,     0,   526,     0,     0,     0,   527,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   528,
       0,     0,   529,   530,     0,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   532,
     533,   534,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   535,   536,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   537,   538,
     539,   540,   541,     0,   542,     0,   543,   544,   545,   546,
       0,     0,     0,   547,   548,   549,   550,   551,   552,   553,
     554,    65,   555,   556,   557,     0,   782,     0,     0,     0,
       0,   558,   308,     0,     0,     0,   814,   815,   309,     0,
       0,     0,     0,     0,   310,     0,     0,     0,   559,   560,
     561,     0,    15,     0,   311,   562,   563,     0,     0,     0,
       0,     0,   312,  1513,     0,   565,     0,   566,   567,     0,
       0,     0,     0,     0,     0,     0,     0,   313,     0,     0,
       0,     0,     0,     0,   314,   315,   316,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   816,   817,   818,   819,   820,     0,
       0,   821,   822,   823,   824,     0,   825,   826,   827,   828,
       0,     0,     0,     0,   829,     0,   830,   831,     0,    64,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   370,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   814,   815,     0,   308,     0,     0,     0,     0,
      65,   309,     0,     0,     0,     0,     0,   310,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   311,     0,     0,
       0,     0,     0,     0,     0,   312,   839,   840,   841,   842,
     843,   844,   845,   846,     0,     0,     0,     0,     0,     0,
     313,     0,     0,   847,   848,     0,   371,   314,   315,   316,
     317,   318,   319,   320,   321,   322,   323,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,     0,     0,     0,     0,     0,     0,     0,
     816,   817,   818,   819,   820,     0,     0,   821,   822,   823,
     824,     0,   825,   826,   827,   828,     0,     0,     0,     0,
     829,     0,   830,   831,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   370,     0,     0,     0,     0,
       0,     0,     0,     0,   814,   815,     0,     0,     0,     0,
     308,     0,     0,    65,     0,     0,   309,     0,     0,     0,
       0,     0,   310,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   311,   840,   841,   842,   843,   844,   845,   846,
     312,     0,     0,     0,     0,     0,     0,     0,     0,   847,
     848,     0,     0,     0,     0,   313,     0,     0,     0,   371,
       0,   630,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,     0,     0,
       0,     0,   816,   817,   818,   819,     0,     0,     0,     0,
       0,     0,   824,     0,   825,   826,   827,   828,     0,     0,
       0,     0,   829,     0,   830,   831,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     370,     0,     0,     0,     0,     0,     0,     0,     0,    13,
       0,     0,     0,   308,     0,     0,     0,     0,   633,   309,
       0,     0,     0,     0,     0,   310,     0,     0,     0,     0,
      14,     0,     0,     0,     0,   311,   634,   842,   843,   844,
     845,   846,     0,   312,     0,     0,     0,     0,     0,     0,
       0,   847,   848,     0,     0,     0,     0,     0,   313,     0,
       0,     0,     0,     0,   371,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   370,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   308,     0,     0,   814,
     815,    65,   309,     0,     0,     0,     0,     0,   310,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   311,     0,
       0,     0,     0,     0,     0,     0,   312,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   313,     0,     0,     0,     0,     0,   371,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   368,   369,     0,     0,     0,     0,     0,  1118,
    1119,  1120,  1121,  1122,  1123,  1124,  1125,   816,   817,   818,
     819,   820,  1126,  1127,   821,   822,   823,   824,  1128,   825,
     826,   827,   828,   814,   815,     0,     0,   829,   957,   830,
     831,  1129,  1130,    64,     0,   832,   833,   834,  1131,  1132,
    1133,   835,     0,     0,     0,     0,   370,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    13,     0,   814,   815,
       0,     0,     0,     0,   633,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    14,     0,     0,
       0,     0,     0,     0,  1134,   836,     0,   837,   838,   839,
     840,   841,   842,   843,   844,   845,   846,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   847,   848,     0,     0,
     371,   566,   923,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1118,  1119,  1120,  1121,  1122,  1123,  1124,
    1125,   816,   817,   818,   819,   820,  1126,  1127,   821,   822,
     823,   824,  1128,   825,   826,   827,   828,  -438,     0,     0,
       0,   829,   957,   830,   831,  1129,  1130,   814,   815,   832,
     833,   834,  1131,  1132,  1133,   835,   816,   817,   818,   819,
     820,     0,     0,   821,   822,   823,   824,     0,   825,   826,
     827,   828,     0,     0,     0,     0,   829,     0,   830,   831,
       0,     0,   814,   815,   832,   833,   834,     0,     0,     0,
     835,     0,     0,     0,     0,     0,     0,     0,  1134,   836,
       0,   837,   838,   839,   840,   841,   842,   843,   844,   845,
     846,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     847,   848,     0,     0,     0,   566,   923,     0,     0,     0,
       0,     0,     0,     0,   836,     0,   837,   838,   839,   840,
     841,   842,   843,   844,   845,   846,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   847,   848,     0,     0,   862,
       0,     0,     0,     0,     0,   816,   817,   818,   819,   820,
       0,     0,   821,   822,   823,   824,     0,   825,   826,   827,
     828,     0,     0,     0,     0,   829,     0,   830,   831,   814,
     815,     0,     0,   832,   833,   834,     0,     0,     0,   835,
     816,   817,   818,   819,   820,     0,     0,   821,   822,   823,
     824,     0,   825,   826,   827,   828,     0,     0,     0,     0,
     829,     0,   830,   831,   814,   815,     0,     0,   832,   833,
     834,     0,     0,     0,   835,     0,     0,     0,     0,     0,
       0,     0,     0,   836,     0,   837,   838,   839,   840,   841,
     842,   843,   844,   845,   846,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   847,   848,     0,     0,   878,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   836,     0,
     837,   838,   839,   840,   841,   842,   843,   844,   845,   846,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   847,
     848,     0,     0,  1164,     0,     0,     0,   816,   817,   818,
     819,   820,     0,     0,   821,   822,   823,   824,     0,   825,
     826,   827,   828,     0,     0,     0,     0,   829,     0,   830,
     831,   814,   815,     0,     0,   832,   833,   834,     0,     0,
       0,   835,   816,   817,   818,   819,   820,     0,     0,   821,
     822,   823,   824,     0,   825,   826,   827,   828,     0,     0,
       0,     0,   829,     0,   830,   831,   814,   815,     0,     0,
     832,   833,   834,     0,     0,     0,   835,     0,     0,     0,
       0,     0,     0,     0,     0,   836,     0,   837,   838,   839,
     840,   841,   842,   843,   844,   845,   846,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   847,   848,     0,     0,
    1267,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     836,     0,   837,   838,   839,   840,   841,   842,   843,   844,
     845,   846,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   847,   848,     0,     0,  1270,     0,     0,     0,   816,
     817,   818,   819,   820,     0,     0,   821,   822,   823,   824,
       0,   825,   826,   827,   828,     0,     0,     0,     0,   829,
       0,   830,   831,   814,   815,     0,     0,   832,   833,   834,
       0,     0,     0,   835,   816,   817,   818,   819,   820,     0,
       0,   821,   822,   823,   824,     0,   825,   826,   827,   828,
       0,     0,     0,     0,   829,     0,   830,   831,   814,   815,
       0,     0,   832,   833,   834,     0,     0,     0,   835,     0,
       0,     0,     0,     0,     0,     0,     0,   836,     0,   837,
     838,   839,   840,   841,   842,   843,   844,   845,   846,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   847,   848,
       0,     0,  1272,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   836,     0,   837,   838,   839,   840,   841,   842,
     843,   844,   845,   846,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   847,   848,     0,     0,  1280,     0,     0,
       0,   816,   817,   818,   819,   820,     0,     0,   821,   822,
     823,   824,     0,   825,   826,   827,   828,     0,     0,     0,
       0,   829,     0,   830,   831,   814,   815,     0,     0,   832,
     833,   834,     0,     0,     0,   835,   816,   817,   818,   819,
     820,     0,     0,   821,   822,   823,   824,     0,   825,   826,
     827,   828,     0,     0,     0,     0,   829,     0,   830,   831,
     814,   815,     0,     0,   832,   833,   834,     0,     0,     0,
     835,     0,     0,     0,     0,     0,     0,     0,     0,   836,
       0,   837,   838,   839,   840,   841,   842,   843,   844,   845,
     846,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     847,   848,     0,     0,  1281,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   836,     0,   837,   838,   839,   840,
     841,   842,   843,   844,   845,   846,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   847,   848,     0,     0,  1282,
       0,     0,     0,   816,   817,   818,   819,   820,     0,     0,
     821,   822,   823,   824,     0,   825,   826,   827,   828,     0,
       0,     0,     0,   829,     0,   830,   831,   814,   815,     0,
       0,   832,   833,   834,     0,     0,     0,   835,   816,   817,
     818,   819,   820,     0,     0,   821,   822,   823,   824,     0,
     825,   826,   827,   828,     0,     0,     0,     0,   829,     0,
     830,   831,   814,   815,     0,     0,   832,   833,   834,     0,
       0,     0,   835,     0,     0,     0,     0,     0,     0,     0,
       0,   836,     0,   837,   838,   839,   840,   841,   842,   843,
     844,   845,   846,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   847,   848,     0,     0,  1283,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   836,     0,   837,   838,
     839,   840,   841,   842,   843,   844,   845,   846,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   847,   848,     0,
       0,  1284,     0,     0,     0,   816,   817,   818,   819,   820,
       0,     0,   821,   822,   823,   824,     0,   825,   826,   827,
     828,     0,     0,     0,     0,   829,     0,   830,   831,   814,
     815,     0,     0,   832,   833,   834,     0,     0,     0,   835,
     816,   817,   818,   819,   820,     0,     0,   821,   822,   823,
     824,     0,   825,   826,   827,   828,     0,     0,     0,     0,
     829,     0,   830,   831,   814,   815,     0,     0,   832,   833,
     834,     0,     0,     0,   835,     0,     0,     0,     0,     0,
       0,     0,     0,   836,     0,   837,   838,   839,   840,   841,
     842,   843,   844,   845,   846,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   847,   848,     0,     0,  1285,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   836,     0,
     837,   838,   839,   840,   841,   842,   843,   844,   845,   846,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   847,
     848,     0,     0,  1372,     0,     0,     0,   816,   817,   818,
     819,   820,     0,     0,   821,   822,   823,   824,     0,   825,
     826,   827,   828,     0,     0,     0,     0,   829,     0,   830,
     831,   814,   815,     0,     0,   832,   833,   834,     0,     0,
       0,   835,   816,   817,   818,   819,   820,     0,     0,   821,
     822,   823,   824,     0,   825,   826,   827,   828,     0,     0,
       0,     0,   829,     0,   830,   831,   814,   815,     0,     0,
     832,   833,   834,     0,     0,     0,   835,     0,     0,     0,
       0,     0,     0,     0,     0,   836,     0,   837,   838,   839,
     840,   841,   842,   843,   844,   845,   846,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   847,   848,     0,     0,
    1455,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     836,     0,   837,   838,   839,   840,   841,   842,   843,   844,
     845,   846,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   847,   848,     0,     0,  1458,     0,     0,     0,   816,
     817,   818,   819,   820,     0,     0,   821,   822,   823,   824,
       0,   825,   826,   827,   828,     0,     0,     0,     0,   829,
       0,   830,   831,   814,   815,     0,     0,   832,   833,   834,
       0,     0,     0,   835,   816,   817,   818,   819,   820,     0,
       0,   821,   822,   823,   824,     0,   825,   826,   827,   828,
       0,     0,     0,     0,   829,     0,   830,   831,   814,   815,
       0,     0,   832,   833,   834,     0,     0,     0,   835,     0,
       0,     0,     0,     0,     0,     0,     0,   836,     0,   837,
     838,   839,   840,   841,   842,   843,   844,   845,   846,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   847,   848,
       0,     0,  1503,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   836,     0,   837,   838,   839,   840,   841,   842,
     843,   844,   845,   846,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   847,   848,     0,     0,  1553,     0,     0,
       0,   816,   817,   818,   819,   820,     0,     0,   821,   822,
     823,   824,     0,   825,   826,   827,   828,     0,     0,     0,
       0,   829,     0,   830,   831,   814,   815,     0,     0,   832,
     833,   834,     0,     0,     0,   835,   816,   817,   818,   819,
     820,     0,     0,   821,   822,   823,   824,     0,   825,   826,
     827,   828,     0,     0,     0,     0,   829,     0,   830,   831,
     814,   815,     0,     0,   832,   833,   834,     0,     0,     0,
     835,     0,     0,     0,     0,     0,     0,     0,     0,   836,
       0,   837,   838,   839,   840,   841,   842,   843,   844,   845,
     846,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     847,   848,     0,     0,  1620,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   836,     0,   837,   838,   839,   840,
     841,   842,   843,   844,   845,   846,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   847,   848,     0,     0,  1621,
       0,     0,     0,   816,   817,   818,   819,   820,     0,     0,
     821,   822,   823,   824,     0,   825,   826,   827,   828,     0,
       0,     0,     0,   829,     0,   830,   831,   814,   815,     0,
       0,   832,   833,   834,     0,     0,     0,   835,   816,   817,
     818,   819,   820,     0,     0,   821,   822,   823,   824,     0,
     825,   826,   827,   828,     0,     0,     0,     0,   829,     0,
     830,   831,   814,   815,     0,     0,   832,   833,   834,     0,
       0,     0,   835,     0,     0,     0,     0,     0,     0,     0,
       0,   836,     0,   837,   838,   839,   840,   841,   842,   843,
     844,   845,   846,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   847,   848,     0,     0,  1634,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   836,     0,   837,   838,
     839,   840,   841,   842,   843,   844,   845,   846,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   847,   848,     0,
       0,  1636,     0,     0,     0,   816,   817,   818,   819,   820,
       0,     0,   821,   822,   823,   824,     0,   825,   826,   827,
     828,     0,     0,     0,     0,   829,     0,   830,   831,   814,
     815,     0,     0,   832,   833,   834,     0,     0,     0,   835,
     816,   817,   818,   819,   820,     0,     0,   821,   822,   823,
     824,     0,   825,   826,   827,   828,     0,     0,     0,     0,
     829,     0,   830,   831,   814,   815,     0,     0,   832,   833,
     834,     0,     0,     0,   835,     0,     0,     0,     0,     0,
       0,     0,     0,   836,     0,   837,   838,   839,   840,   841,
     842,   843,   844,   845,   846,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   847,   848,     0,     0,  1638,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   836,     0,
     837,   838,   839,   840,   841,   842,   843,   844,   845,   846,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   847,
     848,     0,     0,  1642,     0,     0,     0,   816,   817,   818,
     819,   820,     0,     0,   821,   822,   823,   824,     0,   825,
     826,   827,   828,     0,     0,     0,     0,   829,     0,   830,
     831,   814,   815,     0,     0,   832,   833,   834,     0,     0,
       0,   835,   816,   817,   818,   819,   820,     0,     0,   821,
     822,   823,   824,     0,   825,   826,   827,   828,     0,     0,
       0,     0,   829,     0,   830,   831,   814,   815,     0,     0,
     832,   833,   834,     0,     0,     0,   835,     0,     0,     0,
       0,     0,     0,     0,     0,   836,     0,   837,   838,   839,
     840,   841,   842,   843,   844,   845,   846,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   847,   848,     0,     0,
    1703,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     836,     0,   837,   838,   839,   840,   841,   842,   843,   844,
     845,   846,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   847,   848,     0,     0,  1713,     0,     0,     0,   816,
     817,   818,   819,   820,     0,     0,   821,   822,   823,   824,
       0,   825,   826,   827,   828,     0,     0,     0,     0,   829,
       0,   830,   831,   814,   815,     0,     0,   832,   833,   834,
       0,     0,     0,   835,   816,   817,   818,   819,   820,     0,
       0,   821,   822,   823,   824,     0,   825,   826,   827,   828,
       0,     0,     0,     0,   829,     0,   830,   831,   814,   815,
       0,     0,   832,   833,   834,     0,     0,     0,   835,     0,
       0,     0,     0,     0,     0,     0,     0,   836,     0,   837,
     838,   839,   840,   841,   842,   843,   844,   845,   846,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   847,   848,
       0,     0,  1714,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   836,     0,   837,   838,   839,   840,   841,   842,
     843,   844,   845,   846,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   847,   848,     0,     0,  1716,     0,     0,
       0,   816,   817,   818,   819,   820,     0,     0,   821,   822,
     823,   824,     0,   825,   826,   827,   828,     0,     0,     0,
       0,   829,     0,   830,   831,   814,   815,     0,     0,   832,
     833,   834,     0,     0,     0,   835,   816,   817,   818,   819,
     820,     0,     0,   821,   822,   823,   824,     0,   825,   826,
     827,   828,     0,     0,     0,     0,   829,     0,   830,   831,
     814,   815,     0,     0,   832,   833,   834,     0,     0,     0,
     835,     0,     0,     0,     0,     0,     0,     0,     0,   836,
       0,   837,   838,   839,   840,   841,   842,   843,   844,   845,
     846,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     847,   848,     0,     0,  1717,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   836,     0,   837,   838,   839,   840,
     841,   842,   843,   844,   845,   846,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   847,   848,     0,     0,  1737,
       0,     0,     0,   816,   817,   818,   819,   820,     0,     0,
     821,   822,   823,   824,     0,   825,   826,   827,   828,     0,
       0,     0,     0,   829,     0,   830,   831,   814,   815,     0,
       0,   832,   833,   834,     0,     0,     0,   835,   816,   817,
     818,   819,   820,     0,     0,   821,   822,   823,   824,     0,
     825,   826,   827,   828,     0,     0,     0,     0,   829,     0,
     830,   831,   814,   815,     0,     0,   832,   833,   834,     0,
       0,     0,   835,     0,     0,     0,     0,     0,     0,     0,
       0,   836,     0,   837,   838,   839,   840,   841,   842,   843,
     844,   845,   846,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   847,   848,     0,     0,  1740,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   836,     0,   837,   838,
     839,   840,   841,   842,   843,   844,   845,   846,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   847,   848,     0,
       0,  1749,     0,     0,     0,   816,   817,   818,   819,   820,
       0,     0,   821,   822,   823,   824,     0,   825,   826,   827,
     828,     0,     0,     0,     0,   829,     0,   830,   831,   814,
     815,     0,     0,   832,   833,   834,     0,     0,     0,   835,
     816,   817,   818,   819,   820,     0,     0,   821,   822,   823,
     824,     0,   825,   826,   827,   828,     0,     0,     0,     0,
     829,     0,   830,   831,   814,   815,     0,     0,   832,   833,
     834,     0,     0,     0,   835,     0,     0,     0,     0,     0,
       0,     0,     0,   836,     0,   837,   838,   839,   840,   841,
     842,   843,   844,   845,   846,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   847,   848,     0,     0,  1820,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   836,     0,
     837,   838,   839,   840,   841,   842,   843,   844,   845,   846,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   847,
     848,     0,     0,  1821,     0,     0,     0,   816,   817,   818,
     819,   820,     0,     0,   821,   822,   823,   824,     0,   825,
     826,   827,   828,     0,     0,     0,     0,   829,     0,   830,
     831,   814,   815,     0,     0,   832,   833,   834,     0,     0,
       0,   835,   816,   817,   818,   819,   820,     0,     0,   821,
     822,   823,   824,     0,   825,   826,   827,   828,     0,     0,
       0,     0,   829,     0,   830,   831,   814,   815,     0,     0,
     832,   833,   834,     0,     0,     0,   835,     0,     0,     0,
       0,     0,     0,     0,     0,   836,     0,   837,   838,   839,
     840,   841,   842,   843,   844,   845,   846,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   847,   848,   882,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     836,     0,   837,   838,   839,   840,   841,   842,   843,   844,
     845,   846,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   847,   848,  1158,     0,     0,     0,     0,     0,   816,
     817,   818,   819,   820,     0,     0,   821,   822,   823,   824,
       0,   825,   826,   827,   828,     0,     0,     0,     0,   829,
       0,   830,   831,   814,   815,     0,     0,   832,   833,   834,
       0,     0,     0,   835,   816,   817,   818,   819,   820,     0,
       0,   821,   822,   823,   824,     0,   825,   826,   827,   828,
       0,     0,     0,     0,   829,     0,   830,   831,   814,   815,
       0,     0,   832,   833,   834,     0,     0,     0,   835,     0,
       0,     0,     0,     0,     0,     0,     0,   836,     0,   837,
     838,   839,   840,   841,   842,   843,   844,   845,   846,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   847,   848,
    1354,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   836,     0,   837,   838,   839,   840,   841,   842,
     843,   844,   845,   846,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   847,   848,  1370,     0,     0,     0,     0,
       0,   816,   817,   818,   819,   820,     0,     0,   821,   822,
     823,   824,     0,   825,   826,   827,   828,     0,     0,     0,
       0,   829,     0,   830,   831,     0,     0,     0,     0,   832,
     833,   834,     0,     0,     0,   835,   816,   817,   818,   819,
     820,     0,     0,   821,   822,   823,   824,     0,   825,   826,
     827,   828,   386,   387,     0,     0,   829,     0,   830,   831,
       0,     0,     0,     0,   832,   833,   834,     0,     0,   388,
     835,     0,     0,     0,     0,     0,     0,     0,     0,   836,
       0,   837,   838,   839,   840,   841,   842,   843,   844,   845,
     846,     0,     0,     0,     0,   814,   815,     0,     0,     0,
     847,   848,  1534,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   836,     0,   837,   838,   839,   840,
     841,   842,   843,   844,   845,   846,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   847,   848,  1540,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   389,   390,   391,   392,   393,   394,   395,   396,
     397,   398,   399,   400,   401,   402,   403,   404,   405,   406,
       0,     0,   407,   408,   409,     0,     0,     0,     0,   814,
     815,   410,   411,   412,   413,   414,     0,     0,   415,   416,
     417,   418,   419,   420,   421,     0,     0,     0,     0,     0,
       0,     0,     0,   816,   817,   818,   819,   820,     0,     0,
     821,   822,   823,   824,     0,   825,   826,   827,   828,     0,
       0,     0,     0,   829,     0,   830,   831,     0,     0,     0,
       0,   832,   833,   834,     0,     0,     0,   835,   422,     0,
     423,   424,   425,   426,   427,   428,   429,   430,   431,   432,
      52,     0,   433,   434,     0,     0,     0,     0,     0,   435,
     436,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    53,     0,   814,   815,     0,     0,     0,     0,     0,
       0,   836,     0,   837,   838,   839,   840,   841,   842,   843,
     844,   845,   846,     0,     0,     0,     0,   816,   817,   818,
     819,   820,   847,   848,   821,   822,   823,   824,     0,   825,
     826,   827,   828,   814,   815,     0,     0,   829,     0,   830,
     831,     0,     0,     0,     0,   832,   833,   834,     0,     0,
       0,   835,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    13,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    14,     0,     0,
       0,     0,     0,     0,     0,   836,     0,   837,   838,   839,
     840,   841,   842,   843,   844,   845,   846,     0,     0,     0,
       0,   816,   817,   818,   819,   820,   847,   848,   821,   822,
     823,   824,     0,   825,   826,   827,   828,   814,   815,     0,
       0,   829,     0,   830,   831,     0,     0,  1022,     0,   832,
     833,   834,     0,     0,     0,   835,     0,     0,     0,     0,
       0,   816,   817,   818,   819,   820,     0,     0,   821,   822,
     823,   824,     0,   825,   826,   827,   828,   814,   815,     0,
       0,   829,     0,   830,   831,     0,     0,  1291,     0,   832,
     833,   834,     0,     0,     0,   835,     0,     0,     0,   836,
       0,   837,   838,   839,   840,   841,   842,   843,   844,   845,
     846,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     847,   848,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   814,   815,     0,     0,     0,     0,     0,     0,   836,
       0,   837,   838,   839,   840,   841,   842,   843,   844,   845,
     846,     0,     0,     0,     0,   816,   817,   818,   819,   820,
     847,   848,   821,   822,   823,   824,     0,   825,   826,   827,
     828,   814,   815,     0,     0,   829,     0,   830,   831,     0,
       0,     0,     0,   832,   833,   834,     0,     0,     0,   835,
       0,     0,     0,     0,     0,   816,   817,   818,   819,   820,
       0,     0,   821,   822,   823,   824,     0,   825,   826,   827,
     828,     0,     0,     0,     0,   829,     0,   830,   831,     0,
       0,     0,     0,   832,   833,   834,     0,     0,     0,   835,
       0,     0,     0,   836,  1359,   837,   838,   839,   840,   841,
     842,   843,   844,   845,   846,     0,     0,     0,     0,   816,
     817,   818,   819,   820,   847,   848,   821,   822,   823,   824,
       0,   825,   826,   827,   828,   814,   815,     0,     0,   829,
       0,   830,   831,   836,  1498,   837,   838,   839,   840,   841,
     842,   843,   844,   845,   846,     0,     0,     0,     0,   816,
     817,   818,   819,   820,   847,   848,   821,   822,   823,   824,
       0,   825,   826,   827,   828,   814,   815,     0,     0,   829,
       0,   830,   831,     0,     0,     0,     0,   832,   833,   834,
       0,     0,     0,   835,     0,     0,     0,     0,     0,     0,
     838,   839,   840,   841,   842,   843,   844,   845,   846,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   847,   848,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1750,
       0,     0,     0,     0,     0,     0,     0,   836,     0,   837,
     838,   839,   840,   841,   842,   843,   844,   845,   846,     0,
       0,     0,     0,   816,   817,   818,   819,   820,   847,   848,
     821,   822,   823,   824,     0,   825,   826,   827,   828,   814,
     815,     0,     0,   829,     0,   830,   831,     0,     0,     0,
       0,   832,   833,   834,     0,     0,     0,   835,     0,     0,
       0,     0,     0,   816,   817,   818,   819,   820,     0,     0,
     821,   822,   823,   824,     0,   825,   826,   827,   828,   814,
     815,     0,     0,   829,     0,   830,   831,     0,     0,     0,
       0,   832,   833,   834,     0,     0,     0,  -880,     0,     0,
       0,   836,     0,   837,   838,   839,   840,   841,   842,   843,
     844,   845,   846,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   847,   848,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   836,     0,   837,   838,   839,   840,   841,   842,   843,
     844,   845,   846,     0,     0,     0,     0,   816,   817,   818,
     819,   820,   847,   848,   821,   822,   823,   824,     0,   825,
     826,   827,   828,     0,     0,     0,     0,   829,     0,   830,
     831,     0,     0,     0,     0,   832,     0,   834,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   816,   817,   818,
     819,   820,     0,     0,   821,   822,   823,   824,     0,   825,
     826,   827,   828,     0,     0,     0,     0,   829,     0,   830,
     831,     0,     0,     0,     0,   832,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   837,   838,   839,
     840,   841,   842,   843,   844,   845,   846,  1063,     0,     0,
       0,     0,     0,     0,     0,     0,   847,   848,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   837,   838,   839,
     840,   841,   842,   843,   844,   845,   846,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   847,   848,     0,   314,
     315,   316,  1067,   318,   319,   320,   321,   322,   531,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
       0,   336,   337,   338,     0,     0,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,     0,   314,   315,   316,     0,   318,   319,
     320,   321,   322,   531,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,   334,     0,   336,   337,   338,     0,
       0,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,     0,  1064,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1065,     0,     0,     0,  1355,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1068,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1069,   314,   315,   316,     0,   318,   319,   320,   321,   322,
     531,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,     0,   336,   337,   338,     0,     0,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   314,   315,   316,     0,   318,
     319,   320,   321,   322,   531,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,     0,   336,   337,   338,
       0,     0,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,     0,
       0,  1356,     0,     0,     0,     0,     0,     0,     0,     0,
     229,     0,     0,     0,     0,     0,     0,  1357,     0,     0,
       0,     0,     0,     0,     0,     0,  1103,  1104,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   230,     0,   231,     0,
     232,   233,   234,   235,   236,  1105,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,     0,   248,   249,
     250,  1106,     0,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1107,  1108,     0,     0,   278,   279,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   280
};

static const yytype_int16 yycheck[] =
{
       1,   596,   223,   282,   619,   211,   504,   758,   600,   718,
     610,   546,   612,   504,   614,    15,    16,   898,    19,   487,
     180,   486,   449,   504,   783,   803,   510,   996,  1294,   807,
     808,   455,   223,     7,   869,    20,   871,   998,   873,     5,
    1187,   700,   701,    33,   781,  1013,   783,   812,   780,    20,
     782,  1019,   784,    87,  1410,     0,     4,    53,     8,   791,
     484,  1570,     7,    19,    20,    53,    15,    16,   800,    33,
      70,    71,    72,   113,    20,    57,   152,   153,  1595,    20,
    1662,    63,   197,    22,   149,    30,   149,    32,   215,    34,
      40,    46,   576,   685,   166,    40,   149,   215,    20,   100,
     166,   102,   880,   183,   149,    50,   698,   209,    20,   600,
     212,    56,    20,   240,   114,   115,   116,   117,    57,   600,
     544,   545,   149,   149,   242,   152,   153,  1704,   243,   486,
     487,   197,  1279,   150,   178,    80,    55,   297,   240,   183,
      21,    22,   197,   167,   168,   169,   218,    15,    16,  1658,
    1667,  1668,  1734,   218,   155,   218,   150,   131,   223,   125,
     126,    62,   238,   239,   244,   218,   160,   150,  1685,  1686,
     223,   903,   212,   218,   129,   241,   241,   160,   223,   213,
     125,   126,   101,   166,  1761,   181,   241,   165,   241,     7,
     189,   219,   218,   215,   685,   223,   241,   223,   242,   216,
     244,   197,   219,   177,   685,   187,   984,   698,   186,   215,
     975,   238,   239,   214,  1480,   241,   994,   698,   242,   997,
     242,   209,   216,   223,  1741,  1742,   208,   201,   202,   207,
     215,   221,    50,   216,   455,   183,   242,   187,   188,   216,
     189,   197,   178,   188,   215,   194,   186,   183,   197,   239,
     215,   200,   226,  1609,   175,   245,   222,   221,   208,   209,
     141,   142,   854,   484,   209,   486,   300,   207,   149,   215,
     151,   152,   153,   154,   215,   241,   197,  1424,   159,   636,
     501,   502,   215,   228,   641,   944,  1433,   166,   227,   239,
    1055,  1260,   241,   215,   239,   244,   150,   215,   504,   238,
    1261,   241,   508,   215,   510,  1037,   150,   215,   244,   242,
     501,   502,   904,   173,   212,   213,   160,   291,   197,  1056,
     294,   189,  1290,   544,   545,    34,   194,   175,   920,   197,
     150,   150,   200,   757,   565,   183,  1602,   197,   198,   199,
     160,   160,   240,   218,   186,    57,   221,   771,  1007,   197,
     150,    63,   215,   197,    63,  1736,   215,   238,   239,   150,
     160,   215,   216,   854,   218,   207,  1747,   221,   228,   160,
     181,   371,   216,   854,   188,   181,   244,   221,   238,   242,
     601,   602,   603,   242,   605,   606,   197,   175,   609,   218,
     611,   197,   613,   241,   615,   209,   216,   216,  1692,   155,
    1694,   243,   244,   188,  1698,  1699,  1787,  1788,   687,   197,
     601,   602,   603,   904,   605,   606,   216,   696,   609,   186,
     611,   130,   613,   904,   209,   216,   175,   166,   188,   920,
     244,   242,   188,   639,   219,   241,   175,   643,   209,   920,
     207,   925,   175,   444,   227,   445,  1327,    47,   197,   209,
     159,   218,   223,   209,   685,   455,   216,   166,   197,    21,
      22,   227,  1062,  1757,   197,   946,   947,   948,   949,   950,
     951,   952,   953,   954,   955,   956,   197,   958,   959,   960,
     961,   962,   963,   215,   484,  1250,   486,   188,   197,  1024,
      90,    91,   188,    93,   186,   919,   218,   498,   499,  1034,
     222,   501,   502,   504,   861,   506,   241,   188,   209,   510,
     242,  1479,  1347,   209,   215,   207,   943,   517,   518,   215,
     877,  1489,   223,   232,   130,   756,   186,   223,   209,   166,
     188,  1290,   241,   188,   215,   766,   757,  1418,   769,  1198,
    1199,  1200,   223,   166,   544,   545,  1305,   207,   972,   973,
     771,   209,   175,  1290,   209,   215,  1293,   180,   216,   983,
     215,   215,   188,   188,   988,   989,   219,   991,  1305,   993,
     223,   995,   197,   215,   197,   187,   188,   188,   130,   141,
     142,   812,   742,   209,   209,    21,    22,   149,   242,   215,
     152,   153,   154,   814,   815,   596,   208,   159,   209,   222,
     242,   601,   602,   603,   215,   605,   606,   215,   829,   609,
     215,   611,   223,   613,   130,   615,   173,   188,   215,   850,
     215,    33,  1503,   902,   189,   188,   847,   215,   985,   238,
     188,   632,    33,   912,   242,   240,     5,     6,   209,     8,
     197,   198,   199,   215,   215,   242,   209,   242,    60,    61,
     929,   209,   223,   932,   242,    33,   197,   215,   879,    60,
      61,   201,   215,   188,   215,   223,   887,    36,   240,   890,
     227,   228,   215,   904,   188,  1510,   238,   239,   197,   215,
     886,   238,    60,    61,   209,  1153,  1151,   240,   879,   240,
     215,   187,   693,   197,   219,   209,   887,   240,   919,   890,
     215,   215,   703,   704,   240,   141,   142,   188,  1192,   223,
      47,   197,   208,   149,  1465,   151,   152,   153,   154,   720,
     721,   722,   723,   159,   725,   240,  1561,   188,   209,   730,
      67,  1331,   733,   197,   215,   147,   197,   215,   738,   151,
     218,    79,   223,   221,   975,   222,   147,   222,   209,   749,
     151,   972,   973,   974,   215,   197,    94,   757,   979,   965,
     197,    99,   983,   101,   241,   222,   241,   988,   989,   147,
     991,   771,   993,   151,   995,   996,  1513,   188,   181,  1010,
      57,   222,   222,   974,   241,   244,   197,  1018,   979,    57,
     226,   227,   228,   218,   197,    63,   221,  1154,   209,  1536,
     241,   241,   238,   239,   232,   217,  1027,  1028,   181,   221,
     810,   223,   224,    70,    71,    72,   217,   201,   181,   167,
     221,  1656,  1703,   224,   197,  1425,   181,   239,   219,     5,
       6,   181,   223,   245,   197,  1259,  1027,  1028,   239,   217,
     244,  1265,   197,   221,   245,   223,   224,   197,   219,    25,
     129,  1560,   223,   197,    12,    31,   857,   114,   115,   116,
     117,   239,   216,    57,   188,    23,    24,   245,    33,    63,
    1091,   131,   132,   133,   134,   135,   136,   137,   138,   879,
     188,    66,   188,   884,   167,   209,   169,   887,    57,   218,
     890,   215,    68,    69,    63,    60,    61,  1176,    57,   223,
     160,   209,   197,   209,    63,    33,  1330,   215,    57,   215,
     170,   171,   172,    35,    63,   223,    33,   223,  1139,   919,
     208,  1278,    57,   211,   241,   209,   214,  1678,    63,   209,
    1151,   215,    60,    61,  1432,   215,   209,   201,   202,   209,
     209,  1432,   215,    60,    61,   215,   215,  1168,  1139,   125,
     126,  1432,   219,   209,   243,   244,   223,  1314,  1315,   215,
     934,   699,   700,   701,   197,   966,    35,  1567,  1746,   218,
     971,   219,   972,   973,   974,   223,   241,  1168,    43,   979,
     718,   219,   147,   983,   197,   223,   151,   222,   988,   989,
     166,   991,   222,   993,   732,   995,   996,   219,  1735,  1736,
     219,   223,   222,  1781,   223,   241,  1743,   183,   184,   185,
    1747,  1748,   219,   219,    96,    97,   223,   223,   222,   147,
     222,   197,   222,   151,   167,   168,   169,  1027,  1028,   241,
     147,   207,   222,   241,   151,   243,   244,   219,  1259,  1260,
     208,   209,  1779,   211,  1265,   222,   222,   197,   198,   199,
    1787,  1788,   217,   241,   240,  1286,   221,   197,   223,   224,
     188,  1676,   222,   239,   197,  1660,   222,   201,   202,   203,
     204,    22,  1607,  1304,   239,  1027,  1028,  1612,   240,  1310,
     245,   209,   201,   202,   203,   197,  1317,   215,  1319,   217,
     201,   202,   203,   221,   240,   223,   224,  1454,   197,  1523,
     217,  1838,   197,   239,   221,  1462,   223,   224,   222,  1330,
     197,   239,   208,   209,   210,   241,  1117,   245,   242,   857,
     222,  1352,   239,   241,   674,   675,   676,    43,   245,    10,
      11,    12,   222,  1364,    33,  1730,  1731,   222,  1369,  1139,
    1732,   222,  1143,   222,   241,   222,   222,  1426,   241,   222,
     222,  1151,   241,   241,   222,   241,   222,   241,   241,    33,
     223,    60,    61,   241,   244,   239,   241,   197,  1168,   907,
     241,    33,   242,   240,   218,   197,    33,  1772,   189,   222,
      10,  1773,  1774,    37,   922,     8,    60,    61,    66,  1420,
    1411,   241,   208,   222,   222,   222,   215,    13,    60,    61,
     240,  1422,   215,    60,    61,   241,   944,   215,   242,   197,
     197,   155,   197,   223,   215,   241,  1808,    43,   215,  1440,
    1411,    14,   197,   216,   218,   189,  1432,   244,   197,    67,
     197,  1422,   215,   242,   215,  1659,   208,   242,    75,  1240,
     242,  1732,    79,  1522,   222,   241,  1477,     1,   147,  1440,
     241,  1732,   151,   223,   222,   241,    93,    94,   223,  1259,
    1260,    98,    99,   100,   101,  1265,   223,   241,   241,  1007,
     241,  1502,   197,   147,   216,   241,   150,   151,  1557,   241,
     241,   241,  1773,  1774,  1563,   147,   160,   242,   150,   151,
     147,   197,  1773,  1774,   151,   216,   242,   452,   160,   242,
     197,   242,  1523,   241,  1525,  1729,   461,   242,   242,   241,
     240,   240,   240,   197,   188,   241,   471,  1808,   217,   242,
     197,   241,   221,   241,   223,   224,   481,  1808,   197,   197,
    1330,   222,   241,   241,  1525,   209,   197,   197,   241,   241,
     239,    43,   216,   217,    33,   197,   245,   221,   242,   504,
     224,   241,   241,   240,   216,   217,   205,   242,   197,   221,
     217,   240,   224,   197,   221,   239,   223,   224,    33,   197,
     240,   245,   197,  1652,   241,   241,   223,   239,   241,  1117,
     535,   536,   239,   245,   242,   241,   241,    33,   245,    70,
     241,   546,   223,   241,   223,    60,    61,   242,   241,   241,
      33,   241,   209,   558,   559,   560,   561,   562,   563,   241,
     201,  1411,  1413,    53,    60,    61,   240,   215,  1639,   208,
     242,   242,  1422,   242,   242,   208,   208,    60,    61,   240,
     242,  1432,   242,   242,   242,   208,   242,   242,  1659,   240,
    1440,   240,   242,   240,   790,   600,   241,  1185,  1639,  1728,
     242,  1682,    86,     1,   147,  1653,   220,    89,  1654,  1654,
    1198,  1199,  1200,   738,  1191,  1203,  1654,  1205,  1654,  1207,
    1654,  1209,     1,  1211,   629,  1213,  1194,  1215,  1652,  1217,
    1443,  1219,   147,  1221,  1520,  1223,   151,  1339,  1226,  1720,
    1228,  1574,  1230,  1446,  1232,  1243,  1234,  1575,  1236,  1778,
    1575,   147,    58,   517,  1748,   151,   505,  1605,  1729,   323,
     517,  1300,   783,    -1,   147,   670,    21,    22,   151,    -1,
      -1,    -1,    -1,  1523,    -1,  1525,   681,    -1,    -1,    -1,
     685,    -1,    -1,   688,    -1,   690,    -1,  1768,    -1,    -1,
     695,    -1,    -1,   698,    -1,    -1,    -1,   702,    -1,    -1,
      -1,   706,   217,    -1,    -1,    -1,   221,    -1,   223,   224,
      -1,    -1,    -1,    -1,  1565,    -1,    -1,    -1,    -1,    -1,
    1571,   217,    -1,    -1,   239,   221,    -1,   223,   224,   734,
     245,    -1,    -1,    -1,   217,    -1,    -1,    -1,   221,    -1,
     223,   224,    -1,   239,    -1,    -1,    -1,    -1,    -1,   245,
      -1,    -1,    -1,   758,   759,    -1,   239,   762,    -1,   764,
      -1,    -1,   245,    -1,    -1,    -1,    -1,    -1,    -1,   774,
     775,   776,   777,   778,   779,    -1,   781,    -1,   783,    -1,
      -1,    -1,    -1,    -1,   139,   140,   141,   142,   143,  1639,
      -1,   146,   147,   148,   149,    -1,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    -1,  1659,
      -1,   816,   817,    -1,    -1,   820,   821,   822,   823,    -1,
     825,    -1,   827,   828,   829,   830,   831,   832,   833,   834,
     835,   836,   837,   838,   839,   840,   841,   842,   843,   844,
     845,   846,    -1,   848,    -1,    -1,   851,    -1,    -1,   854,
      -1,    -1,    -1,    -1,  1704,    -1,   131,   132,   133,   134,
     135,   136,   137,   138,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,    -1,   150,    -1,    -1,    -1,  1729,
      -1,    -1,  1733,   238,   239,   160,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,    -1,   170,   171,   172,    -1,   904,
      -1,    -1,    -1,    -1,   909,    -1,    -1,    -1,    -1,    -1,
      -1,  1761,    -1,    -1,    -1,   920,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    10,  1776,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,
      -1,   946,   947,   948,   949,   950,   951,   952,   953,   954,
     955,   956,   957,   958,   959,   960,   961,   962,   963,  1810,
      -1,  1812,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   976,  1560,   978,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1834,   947,   948,   949,   950,   951,   952,
     953,   954,   955,   956,    -1,   958,   959,   960,   961,   962,
     963,    -1,    -1,  1008,  1009,    -1,    -1,    -1,   139,   140,
     141,   142,   143,    -1,    -1,   146,    -1,  1022,   149,  1024,
     151,   152,   153,   154,    -1,  1030,    -1,    -1,   159,  1034,
     161,   162,    -1,    -1,  1039,    -1,  1041,    -1,  1043,    -1,
    1045,    -1,    -1,    -1,    -1,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,    -1,
      -1,    21,    22,   159,   160,   161,   162,   163,   164,    -1,
      -1,   167,   168,   169,   170,   171,   172,   173,    -1,    -1,
      -1,   222,   223,   224,   225,   226,   227,   228,    -1,  1104,
      -1,    -1,    -1,  1108,    -1,    -1,    -1,   238,   239,    -1,
      -1,    -1,    -1,  1118,  1119,  1120,  1121,  1122,  1123,  1124,
    1125,  1126,  1127,  1128,  1129,  1130,  1131,  1132,  1133,  1134,
     216,   217,    -1,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   238,   239,  1159,    -1,    -1,   243,   244,    -1,
    1165,    -1,    -1,    -1,    -1,  1170,    -1,    -1,    -1,    -1,
    1175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1190,    -1,    -1,  1776,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,
      -1,   161,   162,    -1,    -1,    -1,    -1,   167,   168,   169,
      -1,    -1,  1810,   173,  1812,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1834,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1291,    -1,   238,   239,
      -1,    -1,   242,    -1,  1299,  1300,    -1,    -1,    -1,    -1,
      -1,    13,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    21,    22,    -1,    -1,    31,
      -1,    -1,    -1,  1328,  1329,    -1,    -1,    -1,    -1,    41,
      -1,  1336,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,  1349,    -1,  1351,    12,  1353,    -1,
      -1,    -1,    64,    -1,  1359,    -1,    21,    22,  1363,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,    21,    22,    -1,  1423,    -1,
      -1,    -1,    -1,    -1,  1429,    -1,    -1,  1432,    -1,    -1,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    -1,
      -1,    -1,    -1,   159,   166,   161,   162,    -1,    -1,    -1,
    1465,   167,   168,   169,    -1,    -1,    -1,   179,    -1,    -1,
      -1,    -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,
      -1,   146,   147,   148,   149,   197,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    -1,  1504,
    1505,  1506,   167,   168,   169,    -1,    -1,    -1,   173,    -1,
      -1,   217,    -1,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,    -1,    -1,    -1,  1531,    -1,  1533,    -1,
      -1,   243,   238,   239,  1539,   141,   142,    -1,    -1,    21,
      22,    -1,    -1,   149,    -1,   151,   152,   153,   154,  1554,
      -1,  1556,   217,   159,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   238,   239,  1580,    -1,    -1,  1583,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1591,  1592,  1593,    -1,
      -1,    -1,    -1,  1598,    -1,    -1,  1601,    -1,    -1,  1604,
      -1,    -1,  1607,  1608,    -1,    -1,    -1,  1612,  1613,    -1,
    1615,  1616,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,   227,   228,    -1,    -1,  1630,    -1,    -1,    -1,    -1,
      -1,    -1,   238,   239,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1654,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,  1678,    -1,    -1,    -1,   159,    -1,   161,
     162,    -1,    -1,    -1,    -1,   167,   168,   169,    -1,    -1,
      -1,   173,    -1,    -1,    -1,  1700,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1708,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1721,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1732,    -1,    -1,
      -1,    -1,    -1,  1738,  1739,   217,    -1,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    -1,    -1,    -1,
      -1,    -1,    -1,  1758,  1759,    -1,   238,   239,    -1,    -1,
      -1,   243,   244,    -1,  1769,    -1,    -1,    -1,  1773,  1774,
       1,    -1,    -1,    -1,     5,     6,     7,    -1,     9,    10,
      11,    -1,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,  1796,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      31,    -1,    -1,  1808,    -1,    -1,    -1,    38,    39,    40,
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
      -1,    -1,    -1,    -1,    -1,   156,   157,   158,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,   180,
      -1,   182,   183,   184,   185,    -1,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      -1,    -1,    -1,    -1,    -1,    -1,   207,   208,   209,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   224,   225,   226,    -1,   228,    -1,    -1,
     231,   232,    -1,    -1,     5,     6,    -1,    -1,   239,    -1,
     241,    -1,   243,   244,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    53,    -1,    55,    -1,    -1,    -1,    -1,    60,
      61,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     141,   142,    -1,    -1,    -1,    -1,   147,    -1,    -1,    -1,
     151,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,   180,
     181,   182,   183,   184,   185,    -1,    -1,    -1,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,    -1,    -1,
     221,    -1,    -1,   224,   225,   226,    -1,   228,    -1,    -1,
     231,   232,    -1,    -1,    -1,     5,     6,    -1,   239,    -1,
     241,    -1,   243,   244,   245,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      60,    61,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   141,   142,    -1,    -1,    -1,    -1,   147,    -1,    -1,
      -1,   151,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,    -1,
     180,   181,   182,   183,   184,   185,    -1,    33,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    60,    61,    -1,   217,    -1,    -1,
      -1,   221,    -1,    -1,   224,   225,   226,    -1,   228,    -1,
      -1,   231,   232,    -1,    -1,    -1,     5,     6,    -1,   239,
      -1,   241,    -1,   243,   244,   245,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,   147,    71,    72,    73,   151,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   217,   141,   142,    -1,   221,    -1,   223,   224,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   239,    -1,    -1,    -1,   166,    -1,   245,
      -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,   178,
      -1,   180,    -1,   182,   183,   184,   185,    -1,    33,    -1,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,
      -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,
      -1,    -1,   221,    -1,    -1,   224,   225,   226,    -1,   228,
      -1,    -1,   231,   232,    -1,    -1,    -1,     5,     6,    -1,
     239,    -1,   241,    -1,   243,   244,   245,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,   147,    71,    72,    73,   151,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,   141,   142,    -1,   221,    -1,   223,   224,
      -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,   156,   157,
     158,    -1,    -1,    -1,   239,    -1,    -1,    -1,   166,    -1,
     245,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,    33,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   224,   225,   226,    -1,
     228,    -1,    -1,   231,   232,    -1,    -1,     5,     6,    -1,
      -1,   239,    -1,   241,    -1,   243,   244,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,   147,    71,    72,    73,   151,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,   141,   142,    -1,   221,    -1,   223,   224,
      -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,   156,   157,
     158,    -1,    -1,    -1,   239,    -1,    -1,    -1,   166,    -1,
     245,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,    33,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   224,   225,   226,    -1,
     228,    -1,    -1,   231,   232,    -1,    -1,     5,     6,    -1,
      -1,   239,    -1,   241,    -1,   243,   244,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    52,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,   147,    71,    72,    73,   151,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,   141,   142,    -1,   221,    -1,   223,   224,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   239,    -1,    -1,    -1,   166,    -1,
     245,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,    33,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   224,   225,   226,    -1,
     228,    -1,    -1,   231,   232,    -1,    -1,     5,     6,    -1,
      -1,   239,    -1,   241,    -1,   243,   244,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,   147,    71,    72,    73,   151,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,   141,   142,    -1,   221,    -1,   223,   224,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   156,   157,
     158,    -1,    -1,    -1,   239,    -1,    -1,    -1,   166,    -1,
     245,    -1,    -1,    -1,    -1,    -1,   174,   175,   176,   177,
     178,    -1,   180,    -1,   182,   183,   184,   185,    -1,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   224,   225,   226,    -1,
     228,    -1,    -1,   231,   232,    -1,    -1,     5,     6,    -1,
      -1,   239,    -1,   241,    -1,   243,   244,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    70,    71,    72,    73,    -1,    75,    76,    77,
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
     178,    -1,   180,   181,   182,   183,   184,   185,    -1,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   224,   225,   226,    -1,
     228,    -1,    -1,   231,   232,    -1,    -1,     5,     6,    -1,
      -1,   239,    -1,   241,   242,   243,   244,    15,    16,    17,
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
     178,    -1,   180,    -1,   182,   183,   184,   185,    -1,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   224,   225,   226,    -1,
     228,    -1,    -1,   231,   232,    -1,    -1,    -1,    -1,     5,
       6,   239,    -1,   241,    -1,   243,   244,    13,    -1,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,    -1,
      -1,     5,     6,   239,   240,   241,    -1,   243,   244,    13,
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
     174,   175,   176,   177,   178,    -1,   180,   181,   182,   183,
     184,   185,    -1,    -1,    -1,   189,   190,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     224,   225,   226,    -1,   228,    -1,    -1,   231,   232,    -1,
      -1,    -1,    -1,     5,     6,   239,    -1,   241,    -1,   243,
     244,    13,    -1,    15,    16,    17,    18,    19,    -1,    -1,
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
     182,   183,   184,   185,    -1,    -1,    -1,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   224,   225,   226,    -1,   228,    -1,    -1,   231,
     232,    -1,    -1,    -1,    -1,     5,     6,   239,   240,   241,
      -1,   243,   244,    13,    -1,    15,    16,    17,    18,    19,
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
     180,    -1,   182,   183,   184,   185,    -1,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   224,   225,   226,    -1,   228,    -1,
      -1,   231,   232,    -1,    -1,     5,     6,    -1,    -1,   239,
      -1,   241,    -1,   243,   244,    15,    16,    17,    18,    19,
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
     180,   181,   182,   183,   184,   185,    -1,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   224,   225,   226,    -1,   228,    -1,
      -1,   231,   232,    -1,    -1,     5,     6,    -1,    -1,   239,
      -1,   241,    -1,   243,   244,    15,    16,    17,    18,    19,
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
     180,   181,   182,   183,   184,   185,    -1,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   224,   225,   226,    -1,   228,    -1,
      -1,   231,   232,    -1,    -1,     5,     6,    -1,    -1,   239,
      -1,   241,   242,   243,   244,    15,    16,    17,    18,    19,
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
     180,   181,   182,   183,   184,   185,    -1,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   224,   225,   226,    -1,   228,    -1,
      -1,   231,   232,    -1,    -1,     5,     6,    -1,    -1,   239,
      -1,   241,   242,   243,   244,    15,    16,    17,    18,    19,
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
     180,    -1,   182,   183,   184,   185,    -1,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   224,   225,   226,    -1,   228,    -1,
      -1,   231,   232,    -1,    -1,    -1,    -1,     5,     6,   239,
     240,   241,    -1,   243,   244,    13,    -1,    15,    16,    17,
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
     178,    -1,   180,    -1,   182,   183,   184,   185,    -1,    -1,
      -1,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   224,   225,   226,    -1,
     228,    -1,    -1,   231,   232,    -1,    -1,    -1,    -1,     5,
       6,   239,    -1,   241,    -1,   243,   244,    13,    -1,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
     176,   177,   178,    -1,   180,   181,   182,   183,   184,   185,
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    61,    -1,    -1,    -1,    65,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   222,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,   242,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   222,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,   242,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,   242,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,   242,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,   242,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,   242,   243,   244,    15,
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
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,
     176,   177,   178,    -1,   180,    -1,   182,   183,   184,   185,
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    -1,   231,   232,    -1,    -1,     5,
       6,    -1,    -1,   239,    -1,   241,    -1,   243,   244,    15,
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
      -1,    -1,    -1,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    13,    -1,    -1,    -1,
      -1,   207,    19,    -1,    -1,    -1,    21,    22,    25,    -1,
      -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,   224,   225,
     226,    -1,   228,    -1,    41,   231,   232,    -1,    -1,    -1,
      -1,    -1,    49,   239,    -1,   241,    -1,   243,   244,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,
      -1,   146,   147,   148,   149,    -1,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    -1,   166,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    19,    -1,    -1,    -1,    -1,
     197,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,   221,   222,   223,   224,
     225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,
      64,    -1,    -1,   238,   239,    -1,   243,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,
     159,    -1,   161,   162,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    -1,
      19,    -1,    -1,   197,    -1,    -1,    25,    -1,    -1,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,   222,   223,   224,   225,   226,   227,   228,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,
     239,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,   243,
      -1,   245,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,    -1,    -1,
      -1,    -1,   139,   140,   141,   142,    -1,    -1,    -1,    -1,
      -1,    -1,   149,    -1,   151,   152,   153,   154,    -1,    -1,
      -1,    -1,   159,    -1,   161,   162,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   166,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   188,
      -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,   197,    25,
      -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
     209,    -1,    -1,    -1,    -1,    41,   215,   224,   225,   226,
     227,   228,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   238,   239,    -1,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,   243,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    21,
      22,   197,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,   243,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,    -1,    -1,    -1,    -1,    -1,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,   154,    21,    22,    -1,    -1,   159,   160,   161,
     162,   163,   164,   166,    -1,   167,   168,   169,   170,   171,
     172,   173,    -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   188,    -1,    21,    22,
      -1,    -1,    -1,    -1,   197,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   209,    -1,    -1,
      -1,    -1,    -1,    -1,   216,   217,    -1,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,
     243,   243,   244,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,    -1,    -1,
      -1,   159,   160,   161,   162,   163,   164,    21,    22,   167,
     168,   169,   170,   171,   172,   173,   139,   140,   141,   142,
     143,    -1,    -1,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,
      -1,    -1,    21,    22,   167,   168,   169,    -1,    -1,    -1,
     173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
      -1,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     238,   239,    -1,    -1,    -1,   243,   244,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,   242,
      -1,    -1,    -1,    -1,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,
      22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,
     159,    -1,   161,   162,    21,    22,    -1,    -1,   167,   168,
     169,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   217,    -1,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   238,   239,    -1,    -1,   242,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,
     239,    -1,    -1,   242,    -1,    -1,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,
     162,    21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,
      -1,   173,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,    -1,    -1,
      -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,
     167,   168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,
     242,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     217,    -1,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   238,   239,    -1,    -1,   242,    -1,    -1,    -1,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,
      -1,   161,   162,    21,    22,    -1,    -1,   167,   168,   169,
      -1,    -1,    -1,   173,   139,   140,   141,   142,   143,    -1,
      -1,   146,   147,   148,   149,    -1,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,
      -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,   239,
      -1,    -1,   242,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,    -1,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   238,   239,    -1,    -1,   242,    -1,    -1,
      -1,   139,   140,   141,   142,   143,    -1,    -1,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,
      -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,   139,   140,   141,   142,
     143,    -1,    -1,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,
      21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,
     173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,
      -1,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     238,   239,    -1,    -1,   242,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,   242,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    -1,
      -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,
      -1,   167,   168,   169,    -1,    -1,    -1,   173,   139,   140,
     141,   142,   143,    -1,    -1,   146,   147,   148,   149,    -1,
     151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,
     161,   162,    21,    22,    -1,    -1,   167,   168,   169,    -1,
      -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   217,    -1,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   238,   239,    -1,    -1,   242,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,
      -1,   242,    -1,    -1,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,
      22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,
     159,    -1,   161,   162,    21,    22,    -1,    -1,   167,   168,
     169,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   217,    -1,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   238,   239,    -1,    -1,   242,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,
     239,    -1,    -1,   242,    -1,    -1,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,
     162,    21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,
      -1,   173,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,    -1,    -1,
      -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,
     167,   168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,
     242,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     217,    -1,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   238,   239,    -1,    -1,   242,    -1,    -1,    -1,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,
      -1,   161,   162,    21,    22,    -1,    -1,   167,   168,   169,
      -1,    -1,    -1,   173,   139,   140,   141,   142,   143,    -1,
      -1,   146,   147,   148,   149,    -1,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,
      -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,   239,
      -1,    -1,   242,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,    -1,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   238,   239,    -1,    -1,   242,    -1,    -1,
      -1,   139,   140,   141,   142,   143,    -1,    -1,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,
      -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,   139,   140,   141,   142,
     143,    -1,    -1,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,
      21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,
     173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,
      -1,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     238,   239,    -1,    -1,   242,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,   242,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    -1,
      -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,
      -1,   167,   168,   169,    -1,    -1,    -1,   173,   139,   140,
     141,   142,   143,    -1,    -1,   146,   147,   148,   149,    -1,
     151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,
     161,   162,    21,    22,    -1,    -1,   167,   168,   169,    -1,
      -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   217,    -1,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   238,   239,    -1,    -1,   242,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,
      -1,   242,    -1,    -1,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,
      22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,
     159,    -1,   161,   162,    21,    22,    -1,    -1,   167,   168,
     169,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   217,    -1,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   238,   239,    -1,    -1,   242,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,
     239,    -1,    -1,   242,    -1,    -1,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,
     162,    21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,
      -1,   173,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,    -1,    -1,
      -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,
     167,   168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,
     242,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     217,    -1,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   238,   239,    -1,    -1,   242,    -1,    -1,    -1,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,
      -1,   161,   162,    21,    22,    -1,    -1,   167,   168,   169,
      -1,    -1,    -1,   173,   139,   140,   141,   142,   143,    -1,
      -1,   146,   147,   148,   149,    -1,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,
      -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,   239,
      -1,    -1,   242,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,    -1,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   238,   239,    -1,    -1,   242,    -1,    -1,
      -1,   139,   140,   141,   142,   143,    -1,    -1,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,
      -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,   139,   140,   141,   142,
     143,    -1,    -1,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,
      21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,
     173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,
      -1,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     238,   239,    -1,    -1,   242,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,   242,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    -1,
      -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,
      -1,   167,   168,   169,    -1,    -1,    -1,   173,   139,   140,
     141,   142,   143,    -1,    -1,   146,   147,   148,   149,    -1,
     151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,
     161,   162,    21,    22,    -1,    -1,   167,   168,   169,    -1,
      -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   217,    -1,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   238,   239,    -1,    -1,   242,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,
      -1,   242,    -1,    -1,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,
      22,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
     139,   140,   141,   142,   143,    -1,    -1,   146,   147,   148,
     149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,
     159,    -1,   161,   162,    21,    22,    -1,    -1,   167,   168,
     169,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   217,    -1,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   238,   239,    -1,    -1,   242,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,
     239,    -1,    -1,   242,    -1,    -1,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,
     162,    21,    22,    -1,    -1,   167,   168,   169,    -1,    -1,
      -1,   173,   139,   140,   141,   142,   143,    -1,    -1,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,    -1,    -1,
      -1,    -1,   159,    -1,   161,   162,    21,    22,    -1,    -1,
     167,   168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   238,   239,   240,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     217,    -1,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   238,   239,   240,    -1,    -1,    -1,    -1,    -1,   139,
     140,   141,   142,   143,    -1,    -1,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    -1,    -1,    -1,    -1,   159,
      -1,   161,   162,    21,    22,    -1,    -1,   167,   168,   169,
      -1,    -1,    -1,   173,   139,   140,   141,   142,   143,    -1,
      -1,   146,   147,   148,   149,    -1,   151,   152,   153,   154,
      -1,    -1,    -1,    -1,   159,    -1,   161,   162,    21,    22,
      -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,   239,
     240,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,    -1,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   238,   239,   240,    -1,    -1,    -1,    -1,
      -1,   139,   140,   141,   142,   143,    -1,    -1,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,    -1,    -1,    -1,
      -1,   159,    -1,   161,   162,    -1,    -1,    -1,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,   139,   140,   141,   142,
     143,    -1,    -1,   146,   147,   148,   149,    -1,   151,   152,
     153,   154,    21,    22,    -1,    -1,   159,    -1,   161,   162,
      -1,    -1,    -1,    -1,   167,   168,   169,    -1,    -1,    38,
     173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,
      -1,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,
     238,   239,   240,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   238,   239,   240,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
      -1,    -1,   151,   152,   153,    -1,    -1,    -1,    -1,    21,
      22,   160,   161,   162,   163,   164,    -1,    -1,   167,   168,
     169,   170,   171,   172,   173,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    -1,
      -1,    -1,    -1,   159,    -1,   161,   162,    -1,    -1,    -1,
      -1,   167,   168,   169,    -1,    -1,    -1,   173,   217,    -1,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
     186,    -1,   231,   232,    -1,    -1,    -1,    -1,    -1,   238,
     239,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,   217,    -1,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,    -1,    -1,    -1,    -1,   139,   140,   141,
     142,   143,   238,   239,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    21,    22,    -1,    -1,   159,    -1,   161,
     162,    -1,    -1,    -1,    -1,   167,   168,   169,    -1,    -1,
      -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   188,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   209,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   217,    -1,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    -1,    -1,    -1,
      -1,   139,   140,   141,   142,   143,   238,   239,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,    21,    22,    -1,
      -1,   159,    -1,   161,   162,    -1,    -1,   165,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,
      -1,   139,   140,   141,   142,   143,    -1,    -1,   146,   147,
     148,   149,    -1,   151,   152,   153,   154,    21,    22,    -1,
      -1,   159,    -1,   161,   162,    -1,    -1,   165,    -1,   167,
     168,   169,    -1,    -1,    -1,   173,    -1,    -1,    -1,   217,
      -1,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     238,   239,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,   217,
      -1,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,    -1,    -1,    -1,    -1,   139,   140,   141,   142,   143,
     238,   239,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    21,    22,    -1,    -1,   159,    -1,   161,   162,    -1,
      -1,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
      -1,    -1,    -1,    -1,    -1,   139,   140,   141,   142,   143,
      -1,    -1,   146,   147,   148,   149,    -1,   151,   152,   153,
     154,    -1,    -1,    -1,    -1,   159,    -1,   161,   162,    -1,
      -1,    -1,    -1,   167,   168,   169,    -1,    -1,    -1,   173,
      -1,    -1,    -1,   217,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,   139,
     140,   141,   142,   143,   238,   239,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    21,    22,    -1,    -1,   159,
      -1,   161,   162,   217,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,    -1,    -1,    -1,    -1,   139,
     140,   141,   142,   143,   238,   239,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,    21,    22,    -1,    -1,   159,
      -1,   161,   162,    -1,    -1,    -1,    -1,   167,   168,   169,
      -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,
     220,   221,   222,   223,   224,   225,   226,   227,   228,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   238,   239,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   209,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,    -1,
      -1,    -1,    -1,   139,   140,   141,   142,   143,   238,   239,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    21,
      22,    -1,    -1,   159,    -1,   161,   162,    -1,    -1,    -1,
      -1,   167,   168,   169,    -1,    -1,    -1,   173,    -1,    -1,
      -1,    -1,    -1,   139,   140,   141,   142,   143,    -1,    -1,
     146,   147,   148,   149,    -1,   151,   152,   153,   154,    21,
      22,    -1,    -1,   159,    -1,   161,   162,    -1,    -1,    -1,
      -1,   167,   168,   169,    -1,    -1,    -1,   173,    -1,    -1,
      -1,   217,    -1,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   238,   239,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   217,    -1,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,    -1,    -1,    -1,    -1,   139,   140,   141,
     142,   143,   238,   239,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,
     162,    -1,    -1,    -1,    -1,   167,    -1,   169,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,   140,   141,
     142,   143,    -1,    -1,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,    -1,    -1,    -1,    -1,   159,    -1,   161,
     162,    -1,    -1,    -1,    -1,   167,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    19,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    71,
      72,    73,    19,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,   181,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   197,    -1,    -1,    -1,    19,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   181,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     197,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,   181,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      35,    -1,    -1,    -1,    -1,    -1,    -1,   197,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   152,   153,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    73,    -1,
      75,    76,    77,    78,    79,   181,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,   197,    -1,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   238,   239,    -1,    -1,   141,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   197
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   247,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   125,   126,   188,   209,   228,   239,   248,   252,   261,
     263,   264,   269,   276,   301,   306,   342,   424,   431,   435,
     446,   492,   497,   502,    19,    20,   197,   293,   294,   295,
     189,   270,   271,   173,   197,   198,   199,   228,   238,   265,
     266,   267,   186,   207,   311,   432,   197,   243,   250,    57,
      63,   427,   427,   427,   166,   197,   328,    34,    63,   130,
     159,   232,   241,   297,   298,   299,   300,   328,   276,     5,
       6,     8,    36,   443,    62,   422,   216,   215,   218,   215,
     227,   227,   266,   227,    22,    57,   227,   238,   268,   427,
     428,   430,   428,   422,   503,   493,   498,   197,   166,   262,
     299,   299,   299,   241,   167,   168,   169,   215,   240,   130,
     130,   130,   305,    57,    63,   433,    57,    63,   444,    57,
      63,   423,    15,    16,   189,   194,   197,   200,   241,   244,
     254,   294,   189,   271,   266,   266,   266,   197,   265,   265,
     197,   276,   187,   208,   312,   428,   276,    57,    63,   249,
     197,   197,   197,   197,   201,   260,   242,   295,   299,   299,
     299,   299,    57,    63,   307,   309,   197,   434,   447,   311,
     425,   201,   202,   203,   253,    15,    16,   189,   194,   197,
     244,   254,   291,   292,   244,   268,   429,   276,   232,   251,
     504,   494,   499,   201,   242,   310,   218,   311,   129,   441,
     442,   420,   183,   244,   296,   392,   201,   202,   203,   244,
     215,   242,   197,   216,    66,   218,   456,   311,   311,    35,
      71,    73,    75,    76,    77,    78,    79,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    93,    94,
      95,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   141,   142,
     197,   304,   308,    75,    79,    93,    94,    98,    99,   100,
     101,   451,   436,   197,   448,   188,   312,   421,   295,   294,
     244,   276,   175,   197,   418,   419,   197,   291,    19,    25,
      31,    41,    49,    64,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     179,   243,   328,   450,   452,   453,   457,   463,   491,    79,
      94,    99,   101,   311,   495,   500,    21,    22,    38,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   151,   152,   153,
     160,   161,   162,   163,   164,   167,   168,   169,   170,   171,
     172,   173,   217,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   231,   232,   238,   239,    35,    35,   241,
     302,   311,   313,   311,   426,   218,   440,   311,   445,   392,
     240,   294,   241,    43,   215,   218,   221,   417,   222,   222,
     222,   241,   222,   222,   241,   456,   222,   222,   222,   222,
     222,   241,   328,    33,    60,    61,   147,   151,   217,   221,
     224,   239,   245,   462,   219,   505,   408,   411,   197,   197,
     197,   240,    22,   197,   240,   178,   242,   392,   403,   404,
     405,   149,   218,   303,   316,   438,   197,   276,   437,   328,
     398,   419,   240,     5,     6,    15,    16,    17,    18,    19,
      25,    27,    31,    39,    45,    48,    51,    55,    65,    68,
      69,    80,   125,   126,   127,   141,   142,   174,   175,   176,
     177,   178,   180,   182,   183,   184,   185,   189,   190,   191,
     192,   193,   194,   195,   196,   198,   199,   200,   207,   224,
     225,   226,   231,   232,   239,   241,   243,   244,   259,   261,
     322,   328,   333,   347,   354,   357,   360,   365,   368,   372,
     373,   375,   380,   383,   384,   391,   450,   507,   522,   533,
     537,   550,   553,   197,   175,   197,   463,   150,   160,   216,
     416,   464,   469,   471,   384,   473,   467,   197,   222,   475,
     477,   479,   481,   483,   485,   487,   489,   384,   222,   241,
     319,    33,   221,    33,   221,   239,   245,   240,   384,   239,
     245,   463,   455,   197,   215,   276,   406,   460,   491,   496,
     197,   409,   460,   501,   197,   131,   132,   133,   134,   135,
     136,   137,   138,   160,   170,   171,   172,   131,   132,   133,
     134,   135,   136,   137,   138,   150,   160,   170,   171,   172,
     241,     7,    50,   341,   276,   215,   276,   242,   491,   491,
       1,     9,    10,    11,    13,    26,    28,    29,    38,    40,
      42,    44,    52,    54,    58,    59,    65,   127,   128,   156,
     157,   158,   198,   272,   273,   276,   277,   280,   281,   283,
     285,   286,   287,   288,   312,   314,   315,   317,   322,   327,
     329,   334,   335,   336,   337,   338,   339,   340,   342,   346,
     369,   371,   384,   426,   216,   276,   312,    40,   239,   276,
     301,   312,   400,   222,   222,   222,   330,   452,   507,   241,
     328,   222,     5,   125,   126,   222,   241,   222,   241,   241,
     222,   222,   241,   222,   241,   222,   241,   222,   222,   241,
     222,   222,   384,   384,   241,   241,   241,   241,   241,   241,
      13,   463,    13,   463,    13,   384,   532,   548,   222,   222,
     258,    13,   320,   532,   549,   384,   384,   384,   384,   384,
      13,    49,   318,   358,   384,   181,   197,   358,   508,   510,
     244,   197,   241,   301,    21,    22,   139,   140,   141,   142,
     143,   146,   147,   148,   149,   151,   152,   153,   154,   159,
     161,   162,   167,   168,   169,   173,   217,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,   238,   239,   242,
     241,   241,    43,   276,   416,   327,   369,   384,   491,   491,
     461,   491,   242,   491,   491,   242,   223,   458,   491,   302,
     491,   302,   491,   302,   406,   407,   409,   410,   242,   466,
     318,   240,   240,   384,   197,   276,   506,   218,   460,   312,
     218,   460,   312,   384,   175,   197,   413,   414,   449,   405,
     405,   405,   384,   284,   150,   327,   358,   384,   313,    61,
     384,   290,   384,   197,   276,   189,    58,   384,   313,   222,
     150,   327,   384,   244,   313,   360,   364,   364,   364,   384,
     276,   276,   384,    10,    37,   360,   366,   276,   276,   276,
     276,   276,    66,   343,   155,   276,   131,   132,   133,   134,
     135,   136,   137,   138,   144,   145,   150,   160,   163,   164,
     170,   171,   172,   216,   366,   439,   384,   399,   300,     8,
     392,   397,   523,   525,   331,   241,   328,   222,   241,   355,
     222,   222,   222,   544,   358,   463,   320,   384,   348,   350,
     384,   352,   384,   546,   358,   529,   534,   358,   527,   463,
     384,   384,   384,   384,   384,   384,   449,    53,   224,   239,
     241,   384,   508,   511,   515,   531,   536,   449,   241,   511,
     536,   449,   165,   208,   209,   210,   516,   323,   325,   203,
     204,   253,   449,   208,   215,   552,   449,    13,   240,   215,
     552,   241,   150,   160,   216,   412,   552,   215,   552,   242,
     175,   180,   222,   328,   374,    70,   239,   242,   358,   510,
       4,   183,   363,    19,   181,   197,   450,    19,   181,   197,
     450,   384,   384,   384,   384,   384,   384,   197,   384,   181,
     197,   384,   384,   384,   450,   384,   384,   384,   384,   384,
     384,    22,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   152,   153,   181,   197,   238,   239,   381,
     450,   384,   242,   358,   384,   197,   327,   384,   131,   132,
     133,   134,   135,   136,   137,   138,   144,   145,   150,   163,
     164,   170,   171,   172,   216,   276,   223,   223,   223,   460,
     223,   223,   197,   454,   223,   303,   223,   303,   223,   303,
     223,   460,   223,   460,   321,   491,   215,   552,   240,   216,
     276,   312,   491,   491,   242,   241,    43,   215,   218,   221,
     412,   313,   449,   327,   358,   215,    14,   384,   197,   313,
     216,   218,   189,   463,   327,   384,   244,   301,   313,   313,
     282,   311,   367,   183,   241,   345,   419,   364,   156,   157,
     158,   314,   370,   384,   370,   384,   370,   384,   370,   384,
     370,   384,   370,   384,   370,   384,   370,   384,   370,   384,
     370,   384,   370,   384,   384,   370,   384,   370,   384,   370,
     384,   370,   384,   370,   384,   370,   384,   312,   276,   197,
     240,    57,    63,   395,    67,   396,   276,   463,   463,   491,
      70,   358,   510,   521,   222,   384,   197,   384,   491,   538,
     540,   542,   463,   552,   223,   460,   242,   242,   463,   463,
     242,   463,   242,   463,   552,   463,   407,   552,   410,   223,
     242,   242,   242,   242,   242,   242,    20,   364,   239,   384,
     242,   165,   215,   209,   515,   212,   213,   240,   519,   215,
     209,   212,   240,   518,    20,   242,   515,   208,   211,   517,
      20,   384,   208,   532,   321,   321,   384,    20,   532,    20,
     449,   384,   384,   384,   384,   242,   181,   197,   241,   241,
     376,   378,   197,   242,   510,   508,   215,   242,   242,   241,
     150,   160,   197,   216,   221,   361,   362,   302,   222,   241,
     222,   241,   241,   241,   240,    19,   181,   197,   450,   218,
     181,   197,   384,   241,   241,   181,   197,   384,     1,   241,
     240,   242,   242,   276,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   465,   470,   472,   491,   474,   468,   215,   223,   276,
     476,   223,   480,   223,   484,   223,   488,   406,   490,   409,
     223,   460,   242,   384,   384,   197,   175,   197,   491,   384,
      20,   313,   216,   289,   223,   363,    12,    23,    24,   274,
     275,   384,   316,   301,   197,   344,   344,   364,   364,   364,
     216,   276,    47,   396,    46,   129,   393,   223,   223,   223,
     510,   242,   242,   242,   197,   242,   209,   223,   242,   223,
     463,   407,   410,   223,   242,   241,   463,   223,   223,   223,
     223,   242,   223,   223,   242,   223,   363,   241,   358,   511,
     515,   384,   508,   519,   240,   384,   531,   240,   358,   511,
     208,   211,   214,   520,   240,   358,   223,   223,   218,   256,
     358,   358,    20,   242,   241,   160,   412,   384,   384,   463,
     302,   242,   240,   239,   362,   197,   197,   241,   197,   197,
     215,   240,   303,   385,   384,   387,   384,   242,   358,   384,
     222,   241,   384,   241,   240,   384,   239,   242,   358,   241,
     240,   382,   242,   358,   197,   459,   197,   478,   482,   486,
     319,   491,   276,   242,   241,    43,   412,   358,   491,   384,
     363,   302,   313,   384,    12,   278,   312,   363,   215,   240,
     242,   491,    33,   394,   393,   395,   524,   526,   332,   242,
     223,   460,   197,   241,   356,   223,   223,   223,   545,   320,
     223,   349,   351,   353,   547,   530,   535,   528,   241,   242,
     358,   209,   515,   519,   209,   515,   240,   209,   324,   326,
     257,   205,   209,   209,   358,   160,   412,   384,   384,   384,
     242,   242,   223,   303,   242,   508,   242,   197,   361,   240,
     165,   313,   359,   463,   242,   491,   242,   242,   242,   389,
     384,   384,   242,   508,   242,   384,   242,   384,   197,   384,
     313,   366,   303,   313,   279,   276,   302,   197,   240,   218,
     417,   276,   401,   394,   413,   414,   415,   241,   241,   384,
     197,   223,   384,   539,   541,   543,   241,   242,   241,   384,
     384,   384,   241,    70,   521,   241,   241,   242,   384,   242,
     384,   519,   384,   520,   532,   384,   319,   255,   532,   384,
     209,   384,   384,   242,   377,   223,   240,   242,   150,   384,
     223,   223,   491,   242,   242,   240,   242,   242,   359,   275,
      26,   128,   280,   334,   335,   336,   338,   384,   303,   218,
     417,   463,   416,   308,   402,   521,   521,   242,   223,   241,
     242,   241,   241,   241,   318,   320,   358,   521,   521,   242,
     209,   551,   551,   551,   201,   551,   551,   384,   160,   412,
     374,   379,   242,   384,   386,   388,   223,   242,   150,   150,
     384,   313,   463,   416,   416,   327,   384,   276,   308,   241,
     508,   512,   513,   514,   514,   384,   384,   521,   521,   508,
     509,   242,   242,   552,   514,   509,    53,   240,   208,   208,
     208,   240,   551,   384,   384,   374,   390,   384,   416,   327,
     384,   327,   384,   276,   313,   508,   215,   552,   242,   242,
     242,   242,   514,   514,   242,   242,   242,   242,   384,   240,
     240,   208,   240,   327,   384,   276,   276,   242,   241,   242,
     242,   276,   508,   242
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   246,   247,   247,   247,   247,   247,   247,   247,   247,
     247,   247,   247,   247,   247,   247,   247,   248,   249,   249,
     249,   250,   250,   251,   251,   252,   253,   253,   253,   253,
     254,   254,   255,   255,   256,   257,   256,   258,   258,   258,
     259,   260,   260,   262,   261,   263,   264,   265,   265,   265,
     266,   266,   266,   266,   266,   266,   266,   267,   267,   268,
     268,   269,   270,   270,   271,   271,   272,   273,   273,   274,
     274,   275,   275,   275,   276,   276,   277,   277,   278,   279,
     278,   280,   280,   280,   280,   280,   281,   282,   281,   284,
     283,   285,   286,   287,   289,   288,   290,   288,   291,   291,
     291,   291,   291,   291,   291,   292,   292,   293,   293,   293,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   295,
     295,   296,   296,   296,   297,   297,   297,   297,   298,   298,
     299,   299,   299,   299,   299,   299,   299,   300,   300,   301,
     301,   302,   302,   302,   303,   303,   303,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   304,   304,   304,   304,   304,   305,
     305,   306,   307,   307,   307,   308,   310,   309,   311,   311,
     312,   312,   313,   313,   314,   314,   314,   315,   315,   315,
     315,   315,   315,   315,   315,   315,   315,   315,   315,   315,
     315,   315,   315,   315,   315,   315,   315,   315,   316,   316,
     316,   317,   318,   318,   319,   319,   320,   320,   321,   321,
     323,   324,   322,   325,   326,   322,   327,   327,   327,   327,
     327,   328,   328,   328,   329,   329,   331,   332,   330,   330,
     333,   333,   333,   333,   333,   333,   334,   335,   336,   336,
     336,   337,   337,   337,   338,   338,   339,   339,   339,   340,
     341,   341,   341,   342,   342,   343,   343,   344,   344,   345,
     345,   345,   345,   345,   345,   345,   345,   346,   346,   348,
     349,   347,   350,   351,   347,   352,   353,   347,   355,   356,
     354,   357,   357,   357,   357,   357,   357,   358,   358,   359,
     359,   359,   360,   360,   360,   361,   361,   361,   361,   361,
     362,   362,   363,   363,   363,   364,   364,   365,   367,   366,
     368,   368,   368,   368,   368,   368,   368,   368,   369,   369,
     369,   369,   369,   369,   369,   369,   369,   369,   369,   369,
     369,   369,   369,   369,   369,   369,   369,   370,   370,   370,
     370,   371,   371,   371,   371,   371,   371,   371,   371,   371,
     371,   371,   371,   371,   371,   371,   371,   371,   372,   372,
     373,   373,   374,   374,   375,   376,   377,   375,   378,   379,
     375,   380,   380,   380,   380,   380,   380,   380,   381,   382,
     380,   383,   383,   383,   383,   383,   383,   383,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   384,   385,   386,   384,   384,   384,
     384,   387,   388,   384,   384,   384,   389,   390,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   391,   391,   391,   391,   391,   391,   391,
     391,   391,   391,   391,   391,   391,   391,   391,   391,   392,
     392,   392,   393,   393,   393,   394,   394,   395,   395,   395,
     396,   396,   397,   398,   398,   399,   398,   400,   398,   401,
     398,   402,   398,   398,   403,   404,   404,   405,   405,   405,
     405,   405,   406,   406,   407,   407,   408,   408,   408,   409,
     410,   410,   411,   411,   411,   412,   412,   413,   413,   413,
     414,   414,   415,   415,   416,   416,   416,   417,   417,   418,
     418,   418,   418,   418,   418,   419,   419,   419,   419,   419,
     420,   420,   421,   420,   422,   422,   423,   423,   423,   424,
     425,   424,   426,   426,   426,   426,   427,   427,   427,   429,
     428,   430,   430,   431,   432,   431,   433,   433,   433,   434,
     436,   437,   435,   438,   439,   435,   440,   440,   441,   441,
     442,   443,   443,   443,   443,   444,   444,   444,   445,   445,
     447,   448,   446,   449,   449,   449,   449,   449,   449,   450,
     450,   450,   450,   450,   450,   450,   450,   450,   450,   450,
     450,   450,   450,   450,   450,   450,   450,   450,   450,   450,
     450,   450,   450,   450,   450,   450,   450,   450,   450,   450,
     450,   450,   450,   450,   450,   450,   450,   450,   450,   450,
     450,   450,   450,   450,   450,   450,   450,   450,   450,   451,
     451,   451,   451,   451,   451,   451,   451,   452,   453,   453,
     453,   454,   454,   454,   455,   455,   455,   455,   456,   456,
     456,   456,   456,   457,   458,   459,   457,   460,   460,   461,
     461,   462,   462,   463,   463,   463,   463,   463,   463,   464,
     465,   463,   463,   463,   466,   463,   463,   463,   463,   463,
     463,   463,   463,   463,   463,   463,   463,   463,   467,   468,
     463,   463,   469,   470,   463,   471,   472,   463,   473,   474,
     463,   463,   475,   476,   463,   477,   478,   463,   463,   479,
     480,   463,   481,   482,   463,   463,   483,   484,   463,   485,
     486,   463,   487,   488,   463,   489,   490,   463,   491,   491,
     491,   493,   494,   495,   496,   492,   498,   499,   500,   501,
     497,   503,   504,   505,   506,   502,   507,   507,   507,   507,
     507,   508,   508,   508,   508,   508,   508,   508,   508,   509,
     509,   510,   511,   511,   512,   512,   513,   513,   514,   514,
     515,   515,   516,   516,   517,   517,   518,   518,   519,   519,
     519,   520,   520,   520,   521,   521,   522,   522,   522,   522,
     522,   522,   523,   524,   522,   525,   526,   522,   527,   528,
     522,   529,   530,   522,   531,   531,   531,   532,   532,   533,
     534,   535,   533,   536,   536,   537,   537,   537,   538,   539,
     537,   540,   541,   537,   542,   543,   537,   537,   544,   545,
     537,   537,   546,   547,   537,   548,   548,   549,   549,   550,
     550,   550,   550,   550,   551,   551,   552,   552,   553,   553,
     553,   553,   553,   553
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     0,     1,
       1,     1,     1,     0,     2,     5,     1,     1,     2,     2,
       3,     2,     0,     2,     0,     0,     3,     0,     2,     5,
       3,     1,     2,     0,     4,     2,     2,     1,     1,     1,
       1,     2,     3,     3,     3,     3,     3,     2,     4,     0,
       1,     2,     1,     3,     1,     3,     3,     3,     2,     1,
       1,     0,     2,     4,     1,     1,     1,     1,     0,     0,
       3,     1,     1,     1,     1,     1,     4,     0,     6,     0,
       6,     2,     3,     3,     0,     5,     0,     5,     1,     1,
       3,     1,     1,     1,     1,     1,     3,     1,     1,     1,
       3,     3,     5,     3,     3,     3,     3,     1,     5,     1,
       3,     2,     3,     2,     1,     1,     1,     1,     1,     4,
       1,     2,     3,     3,     3,     3,     2,     1,     3,     0,
       3,     0,     2,     3,     0,     2,     2,     1,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     3,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     3,     2,     2,     3,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     3,     2,     2,     2,     2,     2,     3,     3,     3,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     4,     0,     1,     1,     3,     0,     4,     1,     1,
       1,     1,     3,     7,     2,     2,     6,     1,     1,     1,
       1,     2,     2,     1,     1,     1,     1,     1,     1,     2,
       2,     1,     1,     1,     1,     2,     2,     2,     0,     2,
       2,     3,     0,     2,     0,     4,     0,     2,     1,     3,
       0,     0,     7,     0,     0,     7,     3,     2,     2,     2,
       1,     1,     3,     2,     2,     3,     0,     0,     5,     1,
       2,     5,     5,     5,     6,     2,     1,     1,     1,     2,
       3,     2,     2,     3,     2,     3,     2,     2,     3,     4,
       1,     1,     0,     1,     1,     1,     0,     1,     3,     9,
       8,     8,     7,     8,     7,     7,     6,     3,     3,     0,
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
       1,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       2,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     2,     2,     2,     4,     3,
       4,     5,     4,     5,     3,     4,     1,     1,     2,     4,
       4,     7,     8,     3,     5,     0,     0,     8,     3,     3,
       3,     0,     0,     8,     3,     4,     0,     0,     9,     4,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     3,
       2,     4,     1,     4,     4,     4,     4,     4,     1,     6,
       7,     6,     6,     7,     7,     6,     7,     6,     6,     0,
       4,     1,     0,     1,     1,     0,     1,     0,     1,     1,
       0,     1,     5,     0,     2,     0,     7,     0,     4,     0,
       9,     0,    10,     5,     3,     3,     4,     1,     1,     3,
       3,     3,     1,     3,     1,     3,     0,     2,     3,     3,
       1,     3,     0,     2,     3,     1,     1,     1,     2,     3,
       3,     5,     1,     1,     1,     1,     1,     0,     1,     1,
       4,     3,     3,     6,     5,     4,     6,     5,     5,     4,
       0,     2,     0,     4,     0,     1,     0,     1,     1,     6,
       0,     6,     0,     2,     3,     5,     0,     1,     1,     0,
       5,     2,     3,     4,     0,     4,     0,     1,     1,     1,
       0,     0,     9,     0,     0,    11,     0,     2,     0,     1,
       3,     1,     1,     2,     2,     0,     1,     1,     0,     3,
       0,     0,     7,     1,     4,     3,     3,     6,     5,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     4,
       4,     1,     3,     3,     0,     2,     3,     5,     0,     2,
       2,     2,     2,     4,     0,     0,     7,     1,     1,     1,
       3,     3,     4,     1,     1,     1,     1,     2,     3,     0,
       0,     6,     4,     3,     0,     7,     4,     2,     2,     3,
       2,     3,     2,     2,     3,     3,     3,     2,     0,     0,
       6,     2,     0,     0,     6,     0,     0,     6,     0,     0,
       6,     1,     0,     0,     6,     0,     0,     7,     1,     0,
       0,     6,     0,     0,     7,     1,     0,     0,     6,     0,
       0,     7,     0,     0,     6,     0,     0,     6,     1,     3,
       3,     0,     0,     0,     0,    10,     0,     0,     0,     0,
      10,     0,     0,     0,     0,    11,     1,     1,     1,     1,
       1,     3,     3,     5,     5,     6,     6,     8,     8,     0,
       1,     2,     1,     3,     3,     5,     1,     2,     1,     0,
       0,     2,     2,     1,     2,     1,     2,     1,     2,     1,
       1,     2,     1,     1,     0,     1,     5,     4,     6,     7,
       5,     7,     0,     0,    10,     0,     0,    10,     0,     0,
      10,     0,     0,     7,     1,     3,     3,     3,     1,     5,
       0,     0,    10,     1,     3,     3,     4,     4,     0,     0,
      11,     0,     0,    11,     0,     0,    10,     5,     0,     0,
       9,     5,     0,     0,    10,     1,     3,     1,     3,     3,
       3,     4,     7,     9,     0,     3,     0,     1,     9,    10,
      10,    10,     9,    10
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
            { /* gc_node; */ }
        break;

    case YYSYMBOL_string_builder: /* string_builder  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_reader: /* expr_reader  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_keyword_or_name: /* keyword_or_name  */
            { delete ((*yyvaluep).s); }
        break;

    case YYSYMBOL_require_module_name: /* require_module_name  */
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

    case YYSYMBOL_expression_if_then_else: /* expression_if_then_else  */
            { /* gc_node; */ }
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
            { /* gc owns TypeDecl */ }
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
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_call_pipe: /* expr_call_pipe  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_any: /* expression_any  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expressions: /* expressions  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_keyword: /* expr_keyword  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_optional_expr_list: /* optional_expr_list  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_optional_expr_list_in_braces: /* optional_expr_list_in_braces  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_optional_expr_map_tuple_list: /* optional_expr_map_tuple_list  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_type_declaration_no_options_list: /* type_declaration_no_options_list  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_expression_keyword: /* expression_keyword  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_pipe: /* expr_pipe  */
            { /* gc_node; */ }
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

    case YYSYMBOL_expression_return_no_pipe: /* expression_return_no_pipe  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_return: /* expression_return  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expression_yield_no_pipe: /* expression_yield_no_pipe  */
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

    case YYSYMBOL_expr_block: /* expr_block  */
            { /* gc_node; */ }
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

    case YYSYMBOL_expr_assign: /* expr_assign  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_assign_pipe_right: /* expr_assign_pipe_right  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_assign_pipe: /* expr_assign_pipe  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_named_call: /* expr_named_call  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_method_call: /* expr_method_call  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_func_addr_name: /* func_addr_name  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_func_addr_expr: /* func_addr_expr  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_field: /* expr_field  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_call: /* expr_call  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr: /* expr  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_mtag: /* expr_mtag  */
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

    case YYSYMBOL_let_variable_declaration: /* let_variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_global_variable_declaration_list: /* global_variable_declaration_list  */
            { deleteVariableDeclarationList(((*yyvaluep).pVarDeclList)); }
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

    case YYSYMBOL_type_declaration: /* type_declaration  */
            { /* gc owns TypeDecl */ }
        break;

    case YYSYMBOL_make_decl: /* make_decl  */
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

    case YYSYMBOL_make_struct_dim: /* make_struct_dim  */
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

    case YYSYMBOL_optional_block: /* optional_block  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_struct_decl: /* make_struct_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_tuple: /* make_tuple  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_map_tuple: /* make_map_tuple  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_tuple_call: /* make_tuple_call  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_dim: /* make_dim  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_dim_decl: /* make_dim_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_table: /* make_table  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_expr_map_tuple_list: /* expr_map_tuple_list  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_make_table_decl: /* make_table_decl  */
            { /* gc_node; */ }
        break;

    case YYSYMBOL_array_comprehension_where: /* array_comprehension_where  */
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
                das_yyerror(scanner,"module name has to be first declaration",tokAt(scanner,(yylsp[0])), CompilationError::invalid_module);
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

  case 17: /* top_level_reader_macro: expr_reader semicolon  */
                                   {
        (void)(yyvsp[-1].pExpression); // gc_node — Expression, don't delete
    }
    break;

  case 18: /* optional_public_or_private_module: %empty  */
                        { (yyval.b) = yyextra->g_Program->policies.default_module_public; }
    break;

  case 19: /* optional_public_or_private_module: "public"  */
                        { (yyval.b) = true; }
    break;

  case 20: /* optional_public_or_private_module: "private"  */
                        { (yyval.b) = false; }
    break;

  case 21: /* module_name: '$'  */
                    { (yyval.s) = new string("$"); }
    break;

  case 22: /* module_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 23: /* optional_not_required: %empty  */
        { (yyval.b) = false; }
    break;

  case 24: /* optional_not_required: '!' "inscope"  */
                        { (yyval.b) = true; }
    break;

  case 25: /* module_declaration: "module" module_name optional_shared optional_public_or_private_module optional_not_required  */
                                                                                                                                    {
        yyextra->g_Program->thisModuleName = *(yyvsp[-3].s);
        yyextra->g_Program->thisModule->isPublic = (yyvsp[-1].b);
        yyextra->g_Program->thisModule->isModule = true;
        yyextra->g_Program->thisModule->visibleEverywhere = (yyvsp[0].b);
        if ( yyextra->g_Program->thisModule->name.empty() ) {
            yyextra->g_Program->library.renameModule(yyextra->g_Program->thisModule.get(),*(yyvsp[-3].s));
        } else if ( yyextra->g_Program->thisModule->name != *(yyvsp[-3].s) ){
            das_yyerror(scanner,"this module already has a name " + yyextra->g_Program->thisModule->name,tokAt(scanner,(yylsp[-3])),
                CompilationError::already_declared_module_name);
        }
        if ( !yyextra->g_Program->policies.ignore_shared_modules ) {
            yyextra->g_Program->promoteToBuiltin = (yyvsp[-2].b);
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 26: /* character_sequence: STRING_CHARACTER  */
                                                                                  { (yyval.s) = new string(); *(yyval.s) += (yyvsp[0].ch); }
    break;

  case 27: /* character_sequence: STRING_CHARACTER_ESC  */
                                                                                  { (yyval.s) = new string(); *(yyval.s) += "\\\\"; }
    break;

  case 28: /* character_sequence: character_sequence STRING_CHARACTER  */
                                                                                  { (yyval.s) = (yyvsp[-1].s); *(yyvsp[-1].s) += (yyvsp[0].ch); }
    break;

  case 29: /* character_sequence: character_sequence STRING_CHARACTER_ESC  */
                                                                                  { (yyval.s) = (yyvsp[-1].s); *(yyvsp[-1].s) += "\\\\"; }
    break;

  case 30: /* string_constant: "start of the string" character_sequence "end of the string"  */
                                                           { (yyval.s) = (yyvsp[-1].s); }
    break;

  case 31: /* string_constant: "start of the string" "end of the string"  */
                                                           { (yyval.s) = new string(); }
    break;

  case 32: /* format_string: %empty  */
        { (yyval.s) = new string(); }
    break;

  case 33: /* format_string: format_string STRING_CHARACTER  */
                                                 { (yyval.s) = (yyvsp[-1].s); (yyvsp[-1].s)->push_back((yyvsp[0].ch)); }
    break;

  case 34: /* optional_format_string: %empty  */
        { (yyval.s) = new string(""); }
    break;

  case 35: /* $@1: %empty  */
            { das_strfmt(scanner); }
    break;

  case 36: /* optional_format_string: ':' $@1 format_string  */
                                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 37: /* string_builder_body: %empty  */
        {
        (yyval.pExpression) = new ExprStringBuilder();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 38: /* string_builder_body: string_builder_body character_sequence  */
                                                                                  {
        bool err;
        auto esconst = unescapeString(*(yyvsp[0].s),&err);
        if ( err ) das_yyerror(scanner,"invalid escape sequence",tokAt(scanner,(yylsp[-1])), CompilationError::invalid_escape);
        auto sc = new ExprConstString(tokAt(scanner,(yylsp[0])),esconst);
        delete (yyvsp[0].s);
        static_cast<ExprStringBuilder *>((yyvsp[-1].pExpression))->elements.push_back(sc);
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 39: /* string_builder_body: string_builder_body "{" expr optional_format_string "}"  */
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

  case 40: /* string_builder: "start of the string" string_builder_body "end of the string"  */
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

  case 41: /* reader_character_sequence: STRING_CHARACTER  */
                               {
        if ( !yyextra->g_ReaderMacro->accept(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, (yyvsp[0].ch), tokAt(scanner,(yylsp[0]))) ) {
            das_yyend_reader(scanner);
        }
    }
    break;

  case 42: /* reader_character_sequence: reader_character_sequence STRING_CHARACTER  */
                                                                {
        if ( !yyextra->g_ReaderMacro->accept(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, (yyvsp[0].ch), tokAt(scanner,(yylsp[0]))) ) {
            das_yyend_reader(scanner);
        }
    }
    break;

  case 43: /* $@2: %empty  */
                                        {
        auto macros = yyextra->g_Program->getReaderMacro(*(yyvsp[0].s));
        if ( macros.size()==0 ) {
            das_yyerror(scanner,"reader macro " + *(yyvsp[0].s) + " not found",tokAt(scanner,(yylsp[0])),
                CompilationError::lookup_macro);
        } else if ( macros.size()>1 ) {
            string options;
            for ( auto & x : macros ) {
                options += "\t" + x->module->name + "::" + x->name + "\n";
            }
            das_yyerror(scanner,"too many options for the reader macro " + *(yyvsp[0].s) +  "\n" + options, tokAt(scanner,(yylsp[0])),
                CompilationError::ambiguous_macro);
        } else if ( yychar != '~' ) {
            das_yyerror(scanner,"expecting ~ after the reader macro", tokAt(scanner,(yylsp[0])),
                CompilationError::invalid_macro);
        } else {
            yyextra->g_ReaderMacro = macros.back();
            yyextra->g_ReaderExpr = new ExprReader(tokAt(scanner,(yylsp[-1])),yyextra->g_ReaderMacro);
            yyclearin ;
            das_yybegin_reader(scanner);
        }
    }
    break;

  case 44: /* expr_reader: '%' name_in_namespace $@2 reader_character_sequence  */
                                     {
        yyextra->g_ReaderExpr->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[0]));
        (yyval.pExpression) = yyextra->g_ReaderExpr;
        int thisLine = 0;
        FileInfo * info = nullptr;
        if ( auto seqt = yyextra->g_ReaderMacro->suffix(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, thisLine, info, tokAt(scanner,(yylsp[0]))) ) {
            das_accept_sequence(scanner,seqt,strlen(seqt),thisLine,info);
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

  case 45: /* options_declaration: "options" annotation_argument_list  */
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
                    tokAt(scanner,(yylsp[0])), CompilationError::invalid_options);
            }
        }
        delete (yyvsp[0].aaList);
    }
    break;

  case 47: /* keyword_or_name: "name"  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 48: /* keyword_or_name: "keyword"  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 49: /* keyword_or_name: "type function"  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 50: /* require_module_name: keyword_or_name  */
                              {
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 51: /* require_module_name: '%' require_module_name  */
                                     {
        *(yyvsp[0].s) = "%" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 52: /* require_module_name: '.' '/' require_module_name  */
                                         {
        *(yyvsp[0].s) = "./" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 53: /* require_module_name: ".." '/' require_module_name  */
                                            {
        *(yyvsp[0].s) = "../" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 54: /* require_module_name: '%' '/' require_module_name  */
                                         {
        *(yyvsp[0].s) = "%/" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 55: /* require_module_name: require_module_name '.' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += ".";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 56: /* require_module_name: require_module_name '/' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += "/";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 57: /* require_module: require_module_name is_public_module  */
                                                         {
        ast_requireModule(scanner,(yyvsp[-1].s),nullptr,(yyvsp[0].b),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 58: /* require_module: require_module_name "as" "name" is_public_module  */
                                                                              {
        ast_requireModule(scanner,(yyvsp[-3].s),(yyvsp[-1].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 59: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 60: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 64: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 65: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 66: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 67: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 68: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 69: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 70: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 71: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 72: /* expression_else: "else" expression_block  */
                                                           { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 73: /* expression_else: elif_or_static_elif expr expression_block expression_else  */
                                                                                          {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),
            (yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-3].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 74: /* semicolon: "end of line"  */
                       {}
    break;

  case 75: /* semicolon: "end of expression"  */
          {}
    break;

  case 76: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 77: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 78: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 79: /* $@3: %empty  */
                      { yyextra->das_need_oxford_comma = true; }
    break;

  case 80: /* expression_else_one_liner: "else" $@3 expression_if_one_liner  */
                                                                                                 {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 81: /* expression_if_one_liner: expr  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 82: /* expression_if_one_liner: expression_return_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 83: /* expression_if_one_liner: expression_yield_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 84: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 85: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 86: /* expression_if_then_else: if_or_static_if expr expression_block expression_else  */
                                                                                      {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),
            (yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-3].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 87: /* $@4: %empty  */
                                                     { yyextra->das_need_oxford_comma = true; }
    break;

  case 88: /* expression_if_then_else: expression_if_one_liner "if" $@4 expr expression_else_one_liner semicolon  */
                                                                                                                                                         {
        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-4])),(yyvsp[-2].pExpression),ast_wrapInBlock((yyvsp[-5].pExpression)),(yyvsp[-1].pExpression) ? ast_wrapInBlock((yyvsp[-1].pExpression)) : nullptr);
    }
    break;

  case 89: /* $@5: %empty  */
                     { yyextra->das_need_oxford_comma=false; }
    break;

  case 90: /* expression_for_loop: "for" $@5 variable_name_with_pos_list "in" expr_list expression_block  */
                                                                                                                                                 {
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-3].pNameWithPosList),(yyvsp[-1].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 91: /* expression_unsafe: "unsafe" expression_block  */
                                                 {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-1])));
        pUnsafe->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 92: /* expression_while_loop: "while" expr expression_block  */
                                                               {
        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-2])));
        pWhile->cond = (yyvsp[-1].pExpression);
        pWhile->body = (yyvsp[0].pExpression);
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 93: /* expression_with: "with" expr expression_block  */
                                                         {
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-2])));
        pWith->with = (yyvsp[-1].pExpression);
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 94: /* $@6: %empty  */
                                        { yyextra->das_need_oxford_comma=true; }
    break;

  case 95: /* expression_with_alias: "assume" "name" '=' $@6 expr  */
                                                                                               {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-4])), *(yyvsp[-3].s), ExpressionPtr((yyvsp[0].pExpression)));
        delete (yyvsp[-3].s);
    }
    break;

  case 96: /* $@7: %empty  */
                         { yyextra->das_force_oxford_comma=true;}
    break;

  case 97: /* expression_with_alias: "typedef" $@7 "name" '=' type_declaration  */
                                                                                                         {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-4])), *(yyvsp[-2].s), TypeDeclPtr((yyvsp[0].pTypeDecl)));
    }
    break;

  case 98: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 99: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 100: /* annotation_argument_value: '@' '@' "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 101: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 102: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 103: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 104: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 105: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 106: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 107: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 108: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 109: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 110: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 111: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 112: /* annotation_argument: annotation_argument_name '=' '@' '@' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[0].s); delete (yyvsp[-4].s); }
    break;

  case 113: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 114: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 115: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 116: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 117: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 118: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                          {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[-4].s); }
    }
    break;

  case 119: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 120: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 121: /* metadata_argument_list: '@' annotation_argument  */
                                      {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 122: /* metadata_argument_list: metadata_argument_list '@' annotation_argument  */
                                                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 123: /* metadata_argument_list: metadata_argument_list semicolon  */
                                               {
        (yyval.aaList) = (yyvsp[-1].aaList);
    }
    break;

  case 124: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 125: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 126: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 127: /* annotation_declaration_name: "template"  */
                                    { (yyval.s) = new string("template"); }
    break;

  case 128: /* annotation_declaration_basic: annotation_declaration_name  */
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
            das_yyerror(scanner,"annotation " + *(yyvsp[0].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[0])), CompilationError::invalid_annotation);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 129: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
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
            das_yyerror(scanner,"annotation " + *(yyvsp[-3].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[-3])), CompilationError::invalid_annotation);
        }
        swap ( (yyval.fa)->arguments, *(yyvsp[-1].aaList) );
        delete (yyvsp[-1].aaList);
        delete (yyvsp[-3].s);
    }
    break;

  case 130: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 131: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            (yyvsp[0].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 132: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            (yyvsp[-2].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            (yyvsp[0].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::And,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 133: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
                                                                            {
        if ( !(yyvsp[-2].fa)->annotation || !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            (yyvsp[-2].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        if ( !(yyvsp[0].fa)->annotation || !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            (yyvsp[0].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Or,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 134: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            (yyvsp[-2].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            (yyvsp[0].fa) = nullptr; // gc_node — don't delete AnnotationDeclaration
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Xor,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 135: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 136: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 137: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 138: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 139: /* optional_annotation_list: %empty  */
                                        { (yyval.faList) = nullptr; }
    break;

  case 140: /* optional_annotation_list: '[' annotation_list ']'  */
                                        { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 141: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 142: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 143: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 144: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 145: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 146: /* optional_function_type: "->" type_declaration  */
                                           {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 147: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 148: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 149: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 150: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 151: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 152: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 153: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 154: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 155: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 156: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 157: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 158: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 159: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 160: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 161: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 162: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 163: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 164: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 165: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 166: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 167: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 168: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 169: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 170: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 171: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 172: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 173: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 174: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 175: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 176: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 177: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 178: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 179: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 180: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 181: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 182: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 183: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 184: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 185: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 186: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 187: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 188: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 189: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 190: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 191: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 192: /* function_name: "operator" '[' ']' "<-"  */
                                    { (yyval.s) = new string("[]<-"); }
    break;

  case 193: /* function_name: "operator" '[' ']' ":="  */
                                      { (yyval.s) = new string("[]:="); }
    break;

  case 194: /* function_name: "operator" '[' ']' "+="  */
                                     { (yyval.s) = new string("[]+="); }
    break;

  case 195: /* function_name: "operator" '[' ']' "-="  */
                                     { (yyval.s) = new string("[]-="); }
    break;

  case 196: /* function_name: "operator" '[' ']' "*="  */
                                     { (yyval.s) = new string("[]*="); }
    break;

  case 197: /* function_name: "operator" '[' ']' "/="  */
                                     { (yyval.s) = new string("[]/="); }
    break;

  case 198: /* function_name: "operator" '[' ']' "%="  */
                                     { (yyval.s) = new string("[]%="); }
    break;

  case 199: /* function_name: "operator" '[' ']' "&="  */
                                     { (yyval.s) = new string("[]&="); }
    break;

  case 200: /* function_name: "operator" '[' ']' "|="  */
                                     { (yyval.s) = new string("[]|="); }
    break;

  case 201: /* function_name: "operator" '[' ']' "^="  */
                                     { (yyval.s) = new string("[]^="); }
    break;

  case 202: /* function_name: "operator" '[' ']' "&&="  */
                                        { (yyval.s) = new string("[]&&="); }
    break;

  case 203: /* function_name: "operator" '[' ']' "||="  */
                                        { (yyval.s) = new string("[]||="); }
    break;

  case 204: /* function_name: "operator" '[' ']' "^^="  */
                                        { (yyval.s) = new string("[]^^="); }
    break;

  case 205: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 206: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 207: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 208: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 209: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 210: /* function_name: "operator" '.' "name" "+="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`+="); delete (yyvsp[-1].s); }
    break;

  case 211: /* function_name: "operator" '.' "name" "-="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`-="); delete (yyvsp[-1].s); }
    break;

  case 212: /* function_name: "operator" '.' "name" "*="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`*="); delete (yyvsp[-1].s); }
    break;

  case 213: /* function_name: "operator" '.' "name" "/="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`/="); delete (yyvsp[-1].s); }
    break;

  case 214: /* function_name: "operator" '.' "name" "%="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`%="); delete (yyvsp[-1].s); }
    break;

  case 215: /* function_name: "operator" '.' "name" "&="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&="); delete (yyvsp[-1].s); }
    break;

  case 216: /* function_name: "operator" '.' "name" "|="  */
                                          { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`|="); delete (yyvsp[-1].s); }
    break;

  case 217: /* function_name: "operator" '.' "name" "^="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^="); delete (yyvsp[-1].s); }
    break;

  case 218: /* function_name: "operator" '.' "name" "&&="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&&="); delete (yyvsp[-1].s); }
    break;

  case 219: /* function_name: "operator" '.' "name" "||="  */
                                            { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`||="); delete (yyvsp[-1].s); }
    break;

  case 220: /* function_name: "operator" '.' "name" "^^="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^^="); delete (yyvsp[-1].s); }
    break;

  case 221: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 222: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 223: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 224: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 225: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 226: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 227: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 228: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 229: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 230: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 231: /* function_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 232: /* function_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 233: /* function_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 234: /* function_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 235: /* function_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 236: /* function_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 237: /* function_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 238: /* function_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 239: /* function_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 240: /* function_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 241: /* function_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 242: /* function_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 243: /* function_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 244: /* function_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 245: /* function_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 246: /* function_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 247: /* function_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 248: /* function_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 249: /* function_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 250: /* function_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 251: /* function_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 252: /* function_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 253: /* function_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 254: /* function_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 255: /* function_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 256: /* function_name: "float16"  */
                     { (yyval.s) = new string("float16"); }
    break;

  case 257: /* function_name: "half2"  */
                     { (yyval.s) = new string("half2"); }
    break;

  case 258: /* function_name: "half3"  */
                     { (yyval.s) = new string("half3"); }
    break;

  case 259: /* function_name: "half4"  */
                     { (yyval.s) = new string("half4"); }
    break;

  case 260: /* function_name: "half8"  */
                     { (yyval.s) = new string("half8"); }
    break;

  case 261: /* function_name: "short2"  */
                     { (yyval.s) = new string("short2"); }
    break;

  case 262: /* function_name: "short3"  */
                     { (yyval.s) = new string("short3"); }
    break;

  case 263: /* function_name: "short4"  */
                     { (yyval.s) = new string("short4"); }
    break;

  case 264: /* function_name: "short8"  */
                     { (yyval.s) = new string("short8"); }
    break;

  case 265: /* function_name: "ushort2"  */
                     { (yyval.s) = new string("ushort2"); }
    break;

  case 266: /* function_name: "ushort3"  */
                     { (yyval.s) = new string("ushort3"); }
    break;

  case 267: /* function_name: "ushort4"  */
                     { (yyval.s) = new string("ushort4"); }
    break;

  case 268: /* function_name: "ushort8"  */
                     { (yyval.s) = new string("ushort8"); }
    break;

  case 269: /* function_name: "byte2"  */
                     { (yyval.s) = new string("byte2"); }
    break;

  case 270: /* function_name: "byte3"  */
                     { (yyval.s) = new string("byte3"); }
    break;

  case 271: /* function_name: "byte4"  */
                     { (yyval.s) = new string("byte4"); }
    break;

  case 272: /* function_name: "byte8"  */
                     { (yyval.s) = new string("byte8"); }
    break;

  case 273: /* function_name: "byte16"  */
                     { (yyval.s) = new string("byte16"); }
    break;

  case 274: /* function_name: "ubyte2"  */
                     { (yyval.s) = new string("ubyte2"); }
    break;

  case 275: /* function_name: "ubyte3"  */
                     { (yyval.s) = new string("ubyte3"); }
    break;

  case 276: /* function_name: "ubyte4"  */
                     { (yyval.s) = new string("ubyte4"); }
    break;

  case 277: /* function_name: "ubyte8"  */
                     { (yyval.s) = new string("ubyte8"); }
    break;

  case 278: /* function_name: "ubyte16"  */
                     { (yyval.s) = new string("ubyte16"); }
    break;

  case 279: /* optional_template: %empty  */
                                        { (yyval.b) = false; }
    break;

  case 280: /* optional_template: "template"  */
                                        { (yyval.b) = true; }
    break;

  case 281: /* global_function_declaration: optional_annotation_list "def" optional_template function_declaration  */
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
                        CompilationError::already_declared_function);
            }
        }
        (yyvsp[0].pFuncDecl)->delRef();
    }
    break;

  case 282: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 283: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 284: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 285: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 286: /* $@8: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 287: /* function_declaration: optional_public_or_private_function $@8 function_declaration_header expression_block  */
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

  case 292: /* expression_block: open_block expressions close_block  */
                                                                  {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
    }
    break;

  case 293: /* expression_block: open_block expressions close_block "finally" open_block expressions close_block  */
                                                                                                                        {
        auto pB = (ExprBlock *) (yyvsp[-5].pExpression);
        auto pF = (ExprBlock *) (yyvsp[-1].pExpression);
        swap ( pB->finalList, pF->list );
        (yyval.pExpression) = (yyvsp[-5].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-6]),(yylsp[0]));
        // gc_node — don't delete Expression
    }
    break;

  case 294: /* expr_call_pipe: expr expr_full_block_assumed_piped  */
                                                      {
        if ( (yyvsp[-1].pExpression)->rtti_isCallLikeExpr() ) {
            ((ExprLooksLikeCall *)(yyvsp[-1].pExpression))->arguments.push_back((yyvsp[0].pExpression));
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else {
            (yyval.pExpression) = (yyvsp[-1].pExpression);
            // gc_node — don't delete Expression
        }
    }
    break;

  case 295: /* expr_call_pipe: expression_keyword expr_full_block_assumed_piped  */
                                                                    {
        if ( (yyvsp[-1].pExpression)->rtti_isCallLikeExpr() ) {
            ((ExprLooksLikeCall *)(yyvsp[-1].pExpression))->arguments.push_back((yyvsp[0].pExpression));
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else {
            (yyval.pExpression) = (yyvsp[-1].pExpression);
            // gc_node — don't delete Expression
        }
    }
    break;

  case 296: /* expr_call_pipe: "generator" '<' type_declaration_no_options '>' optional_capture_list expr_full_block_assumed_piped  */
                                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-3].pTypeDecl),(yyvsp[-1].pCaptList),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 297: /* expression_any: semicolon  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 298: /* expression_any: expr_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 299: /* expression_any: expr_keyword  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 300: /* expression_any: expr_assign_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 301: /* expression_any: expr_assign semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 302: /* expression_any: expression_delete semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 303: /* expression_any: expression_let  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 304: /* expression_any: expression_while_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 305: /* expression_any: expression_unsafe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 306: /* expression_any: expression_with  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 307: /* expression_any: expression_with_alias  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 308: /* expression_any: expression_for_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 309: /* expression_any: expression_break semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 310: /* expression_any: expression_continue semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 311: /* expression_any: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 312: /* expression_any: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 313: /* expression_any: expression_if_then_else  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 314: /* expression_any: expression_try_catch  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 315: /* expression_any: expression_label semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 316: /* expression_any: expression_goto semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 317: /* expression_any: "pass" semicolon  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 318: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 319: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 320: /* expressions: expressions error  */
                                 {
        (void)(yyvsp[-1].pExpression); /* gc_node — don't delete Expression */ (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 321: /* expr_keyword: "keyword" expr expression_block  */
                                                           {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s));
        pCall->arguments.push_back((yyvsp[-1].pExpression));
        auto resT = new TypeDecl(Type::autoinfer);
        auto blk = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,resT,(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),LineInfo());
        pCall->arguments.push_back(blk);
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 322: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 323: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 324: /* optional_expr_list_in_braces: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 325: /* optional_expr_list_in_braces: '(' optional_expr_list optional_comma ')'  */
                                                             { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 326: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 327: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 328: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 329: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 330: /* $@9: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 331: /* $@10: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 332: /* expression_keyword: "keyword" '<' $@9 type_declaration_no_options_list '>' $@10 expr  */
                                                                                                                                                     {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 333: /* $@11: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 334: /* $@12: %empty  */
                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 335: /* expression_keyword: "type function" '<' $@11 type_declaration_no_options_list '>' $@12 optional_expr_list_in_braces  */
                                                                                                                                                                                   {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 336: /* expr_pipe: expr_assign " <|" expr_block  */
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
                    tokAt(scanner,(yylsp[-1])),CompilationError::cant_expression);
                delete (yyvsp[0].pExpression);
            } else {
                pMS->block = (yyvsp[0].pExpression);
            }
            (yyval.pExpression) = (yyvsp[-2].pExpression);
        } else {
            das_yyerror(scanner,"can only pipe into function call or [[ make structure ]]",
                tokAt(scanner,(yylsp[-1])),CompilationError::cant_expression);
            delete (yyvsp[0].pExpression);
            (yyval.pExpression) = (yyvsp[-2].pExpression);
        }
    }
    break;

  case 337: /* expr_pipe: "@ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 338: /* expr_pipe: "@@ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 339: /* expr_pipe: "$ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 340: /* expr_pipe: expr_call_pipe  */
                             {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 341: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 342: /* name_in_namespace: "name" "::" "name"  */
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

  case 343: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 344: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 345: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 346: /* $@13: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 347: /* $@14: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 348: /* new_type_declaration: '<' $@13 type_declaration '>' $@14  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 349: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 350: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 351: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 352: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 353: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 354: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 355: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 356: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 357: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 358: /* expression_return_no_pipe: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 359: /* expression_return_no_pipe: "return" expr_list  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),sequenceToTuple((yyvsp[0].pExpression)));
    }
    break;

  case 360: /* expression_return_no_pipe: "return" "<-" expr_list  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),sequenceToTuple((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 361: /* expression_return: expression_return_no_pipe semicolon  */
                                                    {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 362: /* expression_return: "return" expr_pipe  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 363: /* expression_return: "return" "<-" expr_pipe  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 364: /* expression_yield_no_pipe: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 365: /* expression_yield_no_pipe: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 366: /* expression_yield: expression_yield_no_pipe semicolon  */
                                                   {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 367: /* expression_yield: "yield" expr_pipe  */
                                          {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 368: /* expression_yield: "yield" "<-" expr_pipe  */
                                                 {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 369: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 370: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 371: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 372: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 373: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 374: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 375: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 376: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 377: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 378: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 379: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-7].pNameList),tokAt(scanner,(yylsp[-7])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 380: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 381: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 382: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                           {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameList),tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 383: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr semicolon  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-6]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 384: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr_pipe  */
                                                                                                           {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-5]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameList),tokAt(scanner,(yylsp[-5])),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 385: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 386: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr_pipe  */
                                                                                                   {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameList),tokAt(scanner,(yylsp[-4])),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
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

  case 389: /* $@15: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 390: /* $@16: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 391: /* expr_cast: "cast" '<' $@15 type_declaration_no_options '>' $@16 expr  */
                                                                                                                                                {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
    }
    break;

  case 392: /* $@17: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 393: /* $@18: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 394: /* expr_cast: "upcast" '<' $@17 type_declaration_no_options '>' $@18 expr  */
                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 395: /* $@19: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 396: /* $@20: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 397: /* expr_cast: "reinterpret" '<' $@19 type_declaration_no_options '>' $@20 expr  */
                                                                                                                                                       {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 398: /* $@21: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 399: /* $@22: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 400: /* expr_type_decl: "type" '<' $@21 type_declaration '>' $@22  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 401: /* expr_type_info: "typeinfo" '(' name_in_namespace expr ')'  */
                                                                         {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-2].s),ptd->typeexpr);
                // gc_node — don't delete Expression
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-2].s),(yyvsp[-1].pExpression));
            }
            delete (yyvsp[-2].s);
    }
    break;

  case 402: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" '>' expr ')'  */
                                                                                                {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-5].s),ptd->typeexpr,*(yyvsp[-3].s));
                // gc_node — don't delete Expression
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-5].s),(yyvsp[-1].pExpression),*(yyvsp[-3].s));
            }
            delete (yyvsp[-5].s);
            delete (yyvsp[-3].s);
    }
    break;

  case 403: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" c_or_s "name" '>' expr ')'  */
                                                                                                                        {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-7].s),ptd->typeexpr,*(yyvsp[-5].s),*(yyvsp[-3].s));
                // gc_node — don't delete Expression
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
                // gc_node — don't delete Expression
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
                // gc_node — don't delete Expression
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-6].s),(yyvsp[-1].pExpression),*(yyvsp[-4].s));
            }
            delete (yyvsp[-6].s);
            delete (yyvsp[-4].s);
    }
    break;

  case 406: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" "end of expression" "name" '>' '(' expr ')'  */
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

  case 407: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 408: /* expr_list: expr_list ',' expr  */
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
                                         { (yyval.pCaptList) = (yyvsp[-2].pCaptList); }
    break;

  case 424: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 425: /* expr_block: expression_block  */
                                            {
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

  case 428: /* $@23: %empty  */
                             {  yyextra->das_need_oxford_comma = false; }
    break;

  case 429: /* expr_full_block_assumed_piped: block_or_lambda $@23 optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type expression_block  */
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
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 458: /* expr_assign_pipe_right: "@@ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 459: /* expr_assign_pipe_right: "$ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
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

  case 480: /* expr_method_call: expr "->" "name" '(' ')'  */
                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 481: /* expr_method_call: expr "->" "name" '(' expr_list ')'  */
                                                                              {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
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

  case 483: /* func_addr_name: "$i" '(' expr ')'  */
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

  case 485: /* $@24: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 486: /* $@25: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 487: /* func_addr_expr: '@' '@' '<' $@24 type_declaration_no_options '>' $@25 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 488: /* $@26: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 489: /* $@27: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 490: /* func_addr_expr: '@' '@' '<' $@26 optional_function_argument_list optional_function_type '>' $@27 func_addr_name  */
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

  case 491: /* expr_field: expr '.' "name"  */
                                              {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 492: /* expr_field: expr '.' '.' "name"  */
                                                  {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 493: /* expr_field: expr '.' "name" '(' ')'  */
                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 494: /* expr_field: expr '.' "name" '(' expr_list ')'  */
                                                                           {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 495: /* expr_field: expr '.' "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                       {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 496: /* expr_field: expr '.' basic_type_declaration '(' ')'  */
                                                                        {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 497: /* expr_field: expr '.' basic_type_declaration '(' expr_list ')'  */
                                                                                             {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 498: /* $@28: %empty  */
                               { yyextra->das_suppress_errors=true; }
    break;

  case 499: /* $@29: %empty  */
                                                                            { yyextra->das_suppress_errors=false; }
    break;

  case 500: /* expr_field: expr '.' $@28 error $@29  */
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

  case 508: /* expr: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 509: /* expr: name_in_namespace  */
                                              { (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 510: /* expr: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 511: /* expr: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 512: /* expr: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 513: /* expr: make_decl  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 514: /* expr: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 515: /* expr: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 516: /* expr: expr_field  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 517: /* expr: expr_mtag  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 518: /* expr: '!' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",(yyvsp[0].pExpression)); }
    break;

  case 519: /* expr: '~' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",(yyvsp[0].pExpression)); }
    break;

  case 520: /* expr: '+' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",(yyvsp[0].pExpression)); }
    break;

  case 521: /* expr: '-' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",(yyvsp[0].pExpression)); }
    break;

  case 522: /* expr: expr "<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 523: /* expr: expr ">>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 524: /* expr: expr "<<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 525: /* expr: expr ">>>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 526: /* expr: expr '+' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 527: /* expr: expr '-' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 528: /* expr: expr '*' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 529: /* expr: expr '/' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 530: /* expr: expr '%' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 531: /* expr: expr '<' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 532: /* expr: expr '>' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 533: /* expr: expr "==" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 534: /* expr: expr "!=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 535: /* expr: expr "<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 536: /* expr: expr ">=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 537: /* expr: expr '&' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 538: /* expr: expr '|' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 539: /* expr: expr '^' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 540: /* expr: expr "&&" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 541: /* expr: expr "||" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 542: /* expr: expr "^^" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 543: /* expr: expr ".." expr  */
                                             {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 544: /* expr: "++" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", (yyvsp[0].pExpression)); }
    break;

  case 545: /* expr: "--" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", (yyvsp[0].pExpression)); }
    break;

  case 546: /* expr: expr "++"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 547: /* expr: expr "--"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 548: /* expr: '(' expr_list optional_comma ')'  */
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

  case 549: /* expr: '(' make_struct_single ')'  */
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

  case 550: /* expr: expr '[' expr ']'  */
                                                 { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 551: /* expr: expr '.' '[' expr ']'  */
                                                     { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 552: /* expr: expr "?[" expr ']'  */
                                                 { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 553: /* expr: expr '.' "?[" expr ']'  */
                                                     { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 554: /* expr: expr "?." "name"  */
                                                 { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 555: /* expr: expr '.' "?." "name"  */
                                                     { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 556: /* expr: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 557: /* expr: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 558: /* expr: '*' expr  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 559: /* expr: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 560: /* expr: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 561: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 562: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 563: /* expr: expr "??" expr  */
                                                   { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 564: /* expr: expr '?' expr ':' expr  */
                                                          {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        }
    break;

  case 565: /* $@30: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 566: /* $@31: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 567: /* expr: expr "is" "type" '<' $@30 type_declaration_no_options '>' $@31  */
                                                                                                                                                       {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 568: /* expr: expr "is" basic_type_declaration  */
                                                               {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 569: /* expr: expr "is" "name"  */
                                              {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 570: /* expr: expr "as" "name"  */
                                              {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 571: /* $@32: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 572: /* $@33: %empty  */
                                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 573: /* expr: expr "as" "type" '<' $@32 type_declaration '>' $@33  */
                                                                                                                                            {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 574: /* expr: expr "as" basic_type_declaration  */
                                                               {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 575: /* expr: expr '?' "as" "name"  */
                                                  {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 576: /* $@34: %empty  */
                                                   { yyextra->das_arrow_depth ++; }
    break;

  case 577: /* $@35: %empty  */
                                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 578: /* expr: expr '?' "as" "type" '<' $@34 type_declaration '>' $@35  */
                                                                                                                                                {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 579: /* expr: expr '?' "as" basic_type_declaration  */
                                                                   {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 580: /* expr: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 581: /* expr: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 582: /* expr: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 583: /* expr: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 584: /* expr: expr_method_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 585: /* expr: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 586: /* expr: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 587: /* expr: expr "<|" expr  */
                                                { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 588: /* expr: expr "|>" expr  */
                                                { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 589: /* expr: expr "|>" basic_type_declaration  */
                                                          {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 590: /* expr: name_in_namespace "name"  */
                                                { (yyval.pExpression) = ast_NameName(scanner,(yyvsp[-1].s),(yyvsp[0].s),tokAt(scanner,(yylsp[-1])),tokAt(scanner,(yylsp[0]))); }
    break;

  case 591: /* expr: "unsafe" '(' expr ')'  */
                                         {
        (yyvsp[-1].pExpression)->alwaysSafe = true;
        (yyvsp[-1].pExpression)->userSaidItsSafe = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 592: /* expr: expression_keyword  */
                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 593: /* expr_mtag: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 594: /* expr_mtag: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 595: /* expr_mtag: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 596: /* expr_mtag: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 597: /* expr_mtag: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 598: /* expr_mtag: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 599: /* expr_mtag: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 600: /* expr_mtag: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 601: /* expr_mtag: expr '.' "$f" '(' expr ')'  */
                                                                {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 602: /* expr_mtag: expr "?." "$f" '(' expr ')'  */
                                                                 {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 603: /* expr_mtag: expr '.' '.' "$f" '(' expr ')'  */
                                                                    {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 604: /* expr_mtag: expr '.' "?." "$f" '(' expr ')'  */
                                                                     {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 605: /* expr_mtag: expr "as" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 606: /* expr_mtag: expr '?' "as" "$f" '(' expr ')'  */
                                                                       {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 607: /* expr_mtag: expr "is" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 608: /* expr_mtag: '@' '@' "$c" '(' expr ')'  */
                                                         {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 609: /* optional_field_annotation: %empty  */
                                                        { (yyval.aaList) = nullptr; }
    break;

  case 610: /* optional_field_annotation: "[[" annotation_argument_list ']' ']'  */
                                                        { (yyval.aaList) = (yyvsp[-2].aaList); /*this one is gone when BRABRA is disabled*/ }
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

  case 623: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 624: /* struct_variable_declaration_list: struct_variable_declaration_list semicolon  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 625: /* $@36: %empty  */
                                                               { yyextra->das_force_oxford_comma=true;}
    break;

  case 626: /* struct_variable_declaration_list: struct_variable_declaration_list "typedef" $@36 "name" '=' type_declaration semicolon  */
                                                                                                                                                         {
        (yyval.pVarDeclList) = (yyvsp[-6].pVarDeclList);
        ast_structureAlias(scanner,(yyvsp[-3].s),(yyvsp[-1].pTypeDecl),tokAt(scanner,(yylsp[-5])));
    }
    break;

  case 627: /* $@37: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 628: /* struct_variable_declaration_list: struct_variable_declaration_list $@37 structure_variable_declaration semicolon  */
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

  case 629: /* $@38: %empty  */
                                                                                                                     {
                yyextra->das_force_oxford_comma=true;
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 630: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable "abstract" optional_constant $@38 function_declaration_header semicolon  */
                                                          {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 631: /* $@39: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 632: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@39 function_declaration_header expression_block  */
                                                                        {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-9].pVarDeclList),(yyvsp[-8].faList),(yyvsp[-5].b),(yyvsp[-6].b),(yyvsp[-4].i),(yyvsp[-3].b),(yyvsp[-1].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-7]),(yylsp[0])),tokAt(scanner,(yylsp[-8])));
            }
    break;

  case 633: /* struct_variable_declaration_list: struct_variable_declaration_list '[' annotation_list ']' semicolon  */
                                                                                       {
        das_yyerror(scanner,"structure field or class method annotation expected to remain on the same line with the field or the class",
            tokAt(scanner,(yylsp[-2])), CompilationError::invalid_annotation);
        delete (yyvsp[-2].faList);
        (yyval.pVarDeclList) = (yyvsp[-4].pVarDeclList);
    }
    break;

  case 634: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 635: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 636: /* function_argument_declaration_type: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 637: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 638: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 639: /* function_argument_list: function_argument_declaration_no_type semicolon function_argument_list  */
                                                                                            { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 640: /* function_argument_list: function_argument_declaration_type semicolon function_argument_list  */
                                                                                            { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 641: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 642: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 643: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 644: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 645: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                          { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 646: /* tuple_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 647: /* tuple_alias_type_list: tuple_alias_type_list c_or_s  */
                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 648: /* tuple_alias_type_list: tuple_alias_type_list tuple_type c_or_s  */
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

  case 649: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 650: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 651: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 652: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 653: /* variant_alias_type_list: variant_alias_type_list c_or_s  */
                                           {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 654: /* variant_alias_type_list: variant_alias_type_list variant_type c_or_s  */
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

  case 655: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 656: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 657: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 658: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 659: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 660: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 661: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 662: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 663: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 664: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 665: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 666: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 667: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 668: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 669: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 670: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 671: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 672: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 673: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "$i" '(' expr ')'  */
                                                                               {
        (yyvsp[-5].pNameWithPosList)->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = (yyvsp[-5].pNameWithPosList);
    }
    break;

  case 674: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 675: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options semicolon  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 676: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[-1]));
    }
    break;

  case 677: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 678: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 679: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr_pipe  */
                                                                                                           {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-3]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->atEnd = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 680: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 681: /* global_variable_declaration_list: global_variable_declaration_list "end of line"  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 682: /* $@40: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 683: /* global_variable_declaration_list: global_variable_declaration_list $@40 optional_field_annotation let_variable_declaration  */
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

  case 684: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 685: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 686: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 687: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 688: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 689: /* global_let: kwd_let optional_shared optional_public_or_private_variable open_block global_variable_declaration_list close_block  */
                                                                                                                                                      {
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 690: /* $@41: %empty  */
                                                                                        {
        yyextra->das_force_oxford_comma=true;
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 691: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@41 optional_field_annotation let_variable_declaration  */
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

  case 692: /* enum_list: %empty  */
        {
        (yyval.pEnumList) = new Enumeration();
    }
    break;

  case 693: /* enum_list: enum_list semicolon  */
                                {
        (yyval.pEnumList) = (yyvsp[-1].pEnumList);
    }
    break;

  case 694: /* enum_list: enum_list "name" semicolon  */
                                           {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        if ( !(yyvsp[-2].pEnumList)->add(*(yyvsp[-1].s),nullptr,tokAt(scanner,(yylsp[-1]))) ) {
            das_yyerror(scanner,"enumeration already declared " + *(yyvsp[-1].s), tokAt(scanner,(yylsp[-1])),
                CompilationError::already_declared_enumerator);
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

  case 695: /* enum_list: enum_list "name" '=' expr semicolon  */
                                                           {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        if ( !(yyvsp[-4].pEnumList)->add(*(yyvsp[-3].s),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-3]))) ) {
            das_yyerror(scanner,"enumeration value already declared " + *(yyvsp[-3].s), tokAt(scanner,(yylsp[-3])),
                CompilationError::already_declared_enumerator);
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

  case 696: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 697: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 698: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 699: /* $@42: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 700: /* single_alias: optional_public_or_private_alias "name" $@42 '=' type_declaration  */
                                  {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        (yyvsp[0].pTypeDecl)->isPrivateAlias = !(yyvsp[-4].b);
        if ( (yyvsp[0].pTypeDecl)->baseType == Type::alias ) {
            das_yyerror(scanner,"alias cannot be defined in terms of another alias "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::invalid_type_alias);
        }
        (yyvsp[0].pTypeDecl)->alias = *(yyvsp[-3].s);
        if ( !yyextra->g_Program->addAlias((yyvsp[0].pTypeDecl)) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::already_declared_type_alias);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterAlias((yyvsp[-3].s)->c_str(),pubename);
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 704: /* $@43: %empty  */
                    { yyextra->das_force_oxford_comma=true;}
    break;

  case 706: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 707: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 708: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 709: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 710: /* $@44: %empty  */
                                                                                                                       {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 711: /* $@45: %empty  */
                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
    }
    break;

  case 712: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name open_block $@44 enum_list $@45 close_block  */
                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-5].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-8].faList),tokAt(scanner,(yylsp[-8])),(yyvsp[-6].b),(yyvsp[-5].pEnum),(yyvsp[-2].pEnumList),Type::tInt);
    }
    break;

  case 713: /* $@46: %empty  */
                                                                                                                                                            {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 714: /* $@47: %empty  */
                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
    }
    break;

  case 715: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name ':' enum_basic_type_declaration open_block $@46 enum_list $@47 close_block  */
                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-7].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-10].faList),tokAt(scanner,(yylsp[-10])),(yyvsp[-8].b),(yyvsp[-7].pEnum),(yyvsp[-2].pEnumList),(yyvsp[-5].type));
    }
    break;

  case 716: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 717: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 718: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 719: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 720: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 721: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 722: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 723: /* class_or_struct: "class" "template"  */
                                 { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 724: /* class_or_struct: "struct" "template"  */
                                 { (yyval.i) = CorS_StructTemplate; }
    break;

  case 725: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 726: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 727: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 728: /* optional_struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 729: /* optional_struct_variable_declaration_list: open_block struct_variable_declaration_list close_block  */
                                                                      {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 730: /* $@48: %empty  */
                                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 731: /* $@49: %empty  */
                         {
        if ( (yyvsp[0].pStructure) ) {
            (yyvsp[0].pStructure)->isClass = (yyvsp[-3].i)==CorS_Class || (yyvsp[-3].i)==CorS_ClassTemplate;
            (yyvsp[0].pStructure)->isTemplate = (yyvsp[-3].i)==CorS_ClassTemplate || (yyvsp[-3].i)==CorS_StructTemplate;
            (yyvsp[0].pStructure)->privateStructure = !(yyvsp[-2].b);
        }
    }
    break;

  case 732: /* structure_declaration: optional_annotation_list class_or_struct optional_public_or_private_structure $@48 structure_name $@49 optional_struct_variable_declaration_list  */
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

  case 733: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 734: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 735: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 736: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 737: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "$i" '(' expr ')'  */
                                                                           {
        (yyvsp[-5].pNameWithPosList)->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = (yyvsp[-5].pNameWithPosList);
    }
    break;

  case 738: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 739: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 740: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 741: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 742: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 743: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 744: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 745: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 746: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 747: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 748: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 749: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 750: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 751: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 752: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 753: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 754: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 755: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 756: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 757: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 758: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 759: /* basic_type_declaration: "float16"  */
                        { (yyval.type) = Type::tFloat16; }
    break;

  case 760: /* basic_type_declaration: "half2"  */
                        { (yyval.type) = Type::tHalf2; }
    break;

  case 761: /* basic_type_declaration: "half3"  */
                        { (yyval.type) = Type::tHalf3; }
    break;

  case 762: /* basic_type_declaration: "half4"  */
                        { (yyval.type) = Type::tHalf4; }
    break;

  case 763: /* basic_type_declaration: "half8"  */
                        { (yyval.type) = Type::tHalf8; }
    break;

  case 764: /* basic_type_declaration: "short2"  */
                        { (yyval.type) = Type::tShort2; }
    break;

  case 765: /* basic_type_declaration: "short3"  */
                        { (yyval.type) = Type::tShort3; }
    break;

  case 766: /* basic_type_declaration: "short4"  */
                        { (yyval.type) = Type::tShort4; }
    break;

  case 767: /* basic_type_declaration: "short8"  */
                        { (yyval.type) = Type::tShort8; }
    break;

  case 768: /* basic_type_declaration: "ushort2"  */
                        { (yyval.type) = Type::tUShort2; }
    break;

  case 769: /* basic_type_declaration: "ushort3"  */
                        { (yyval.type) = Type::tUShort3; }
    break;

  case 770: /* basic_type_declaration: "ushort4"  */
                        { (yyval.type) = Type::tUShort4; }
    break;

  case 771: /* basic_type_declaration: "ushort8"  */
                        { (yyval.type) = Type::tUShort8; }
    break;

  case 772: /* basic_type_declaration: "byte2"  */
                        { (yyval.type) = Type::tByte2; }
    break;

  case 773: /* basic_type_declaration: "byte3"  */
                        { (yyval.type) = Type::tByte3; }
    break;

  case 774: /* basic_type_declaration: "byte4"  */
                        { (yyval.type) = Type::tByte4; }
    break;

  case 775: /* basic_type_declaration: "byte8"  */
                        { (yyval.type) = Type::tByte8; }
    break;

  case 776: /* basic_type_declaration: "byte16"  */
                        { (yyval.type) = Type::tByte16; }
    break;

  case 777: /* basic_type_declaration: "ubyte2"  */
                        { (yyval.type) = Type::tUByte2; }
    break;

  case 778: /* basic_type_declaration: "ubyte3"  */
                        { (yyval.type) = Type::tUByte3; }
    break;

  case 779: /* basic_type_declaration: "ubyte4"  */
                        { (yyval.type) = Type::tUByte4; }
    break;

  case 780: /* basic_type_declaration: "ubyte8"  */
                        { (yyval.type) = Type::tUByte8; }
    break;

  case 781: /* basic_type_declaration: "ubyte16"  */
                        { (yyval.type) = Type::tUByte16; }
    break;

  case 782: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 783: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 784: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 785: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 786: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 787: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 788: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 789: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 790: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 791: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 792: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 793: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 794: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 795: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 796: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 797: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 798: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 799: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 800: /* auto_type_declaration: "$t" '(' expr ')'  */
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

  case 801: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 802: /* bitfield_bits: bitfield_bits semicolon "name"  */
                                                 {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 803: /* bitfield_bits: bitfield_bits ',' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 804: /* bitfield_alias_bits: %empty  */
        {
        auto pSL = new vector<tuple<string,Expression *>>();
        (yyval.pNameExprList) = pSL;

    }
    break;

  case 805: /* bitfield_alias_bits: bitfield_alias_bits semicolon  */
                                            {
        (yyval.pNameExprList) = (yyvsp[-1].pNameExprList);
    }
    break;

  case 806: /* bitfield_alias_bits: bitfield_alias_bits "name" semicolon  */
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

  case 807: /* bitfield_alias_bits: bitfield_alias_bits "name" '=' expr semicolon  */
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

  case 808: /* bitfield_basic_type_declaration: %empty  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 809: /* bitfield_basic_type_declaration: ':' "uint8"  */
                             { (yyval.type) = Type::tBitfield8; }
    break;

  case 810: /* bitfield_basic_type_declaration: ':' "uint16"  */
                             { (yyval.type) = Type::tBitfield16; }
    break;

  case 811: /* bitfield_basic_type_declaration: ':' "uint"  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 812: /* bitfield_basic_type_declaration: ':' "uint64"  */
                             { (yyval.type) = Type::tBitfield64; }
    break;

  case 813: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' '>'  */
                                                                          {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-2].type));
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-2]));
    }
    break;

  case 814: /* $@50: %empty  */
                                                                     { yyextra->das_arrow_depth ++; }
    break;

  case 815: /* $@51: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 816: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' $@50 bitfield_bits '>' $@51  */
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

  case 819: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 820: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 821: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = appendDimExpr(nullptr, (yyvsp[-1].pExpression), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 822: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = appendDimExpr((yyvsp[-3].pTypeDecl), (yyvsp[-1].pExpression), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 823: /* type_declaration_no_options: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 824: /* type_declaration_no_options: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 825: /* type_declaration_no_options: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 826: /* type_declaration_no_options: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 827: /* type_declaration_no_options: type_declaration_no_options dim_list  */
                                                                {
        if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeDecl ) {
            das_yyerror(scanner,"type declaration can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_array_type);
        } else if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeMacro ) {
            das_yyerror(scanner,"macro can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_array_type);
        }
        (yyval.pTypeDecl) = attachDimChain((yyvsp[0].pTypeDecl), (yyvsp[-1].pTypeDecl));
    }
    break;

  case 828: /* type_declaration_no_options: type_declaration_no_options '[' ']'  */
                                                      {
        (yyval.pTypeDecl) = appendAutoDim((yyvsp[-2].pTypeDecl), tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 829: /* $@52: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 830: /* $@53: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 831: /* type_declaration_no_options: "type" '<' $@52 type_declaration '>' $@53  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 832: /* type_declaration_no_options: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->typeMacroExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 833: /* type_declaration_no_options: '$' name_in_namespace optional_expr_list_in_braces  */
                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-1]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = sequenceToList((yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-1])), *(yyvsp[-1].s)));
        delete (yyvsp[-1].s);
    }
    break;

  case 834: /* $@54: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 835: /* type_declaration_no_options: '$' name_in_namespace '<' $@54 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->typeMacroExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->typeMacroExpr.insert((yyval.pTypeDecl)->typeMacroExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 836: /* type_declaration_no_options: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 837: /* type_declaration_no_options: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 838: /* type_declaration_no_options: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 839: /* type_declaration_no_options: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 840: /* type_declaration_no_options: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 841: /* type_declaration_no_options: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 842: /* type_declaration_no_options: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 843: /* type_declaration_no_options: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 844: /* type_declaration_no_options: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 845: /* type_declaration_no_options: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 846: /* type_declaration_no_options: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 847: /* type_declaration_no_options: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 848: /* $@55: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 849: /* $@56: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 850: /* type_declaration_no_options: "smart_ptr" '<' $@55 type_declaration '>' $@56  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 851: /* type_declaration_no_options: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 852: /* $@57: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 853: /* $@58: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 854: /* type_declaration_no_options: "array" '<' $@57 type_declaration '>' $@58  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 855: /* $@59: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 856: /* $@60: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 857: /* type_declaration_no_options: "table" '<' $@59 table_type_pair '>' $@60  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 858: /* $@61: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 859: /* $@62: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 860: /* type_declaration_no_options: "iterator" '<' $@61 type_declaration '>' $@62  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 861: /* type_declaration_no_options: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 862: /* $@63: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 863: /* $@64: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 864: /* type_declaration_no_options: "block" '<' $@63 type_declaration '>' $@64  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 865: /* $@65: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 866: /* $@66: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 867: /* type_declaration_no_options: "block" '<' $@65 optional_function_argument_list optional_function_type '>' $@66  */
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

  case 868: /* type_declaration_no_options: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 869: /* $@67: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 870: /* $@68: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 871: /* type_declaration_no_options: "function" '<' $@67 type_declaration '>' $@68  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 872: /* $@69: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 873: /* $@70: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 874: /* type_declaration_no_options: "function" '<' $@69 optional_function_argument_list optional_function_type '>' $@70  */
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

  case 875: /* type_declaration_no_options: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 876: /* $@71: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 877: /* $@72: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 878: /* type_declaration_no_options: "lambda" '<' $@71 type_declaration '>' $@72  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 879: /* $@73: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 880: /* $@74: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 881: /* type_declaration_no_options: "lambda" '<' $@73 optional_function_argument_list optional_function_type '>' $@74  */
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

  case 882: /* $@75: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 883: /* $@76: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 884: /* type_declaration_no_options: "tuple" '<' $@75 tuple_type_list '>' $@76  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 885: /* $@77: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 886: /* $@78: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 887: /* type_declaration_no_options: "variant" '<' $@77 variant_type_list '>' $@78  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 888: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 889: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 890: /* type_declaration: type_declaration '|' '#'  */
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

  case 891: /* $@79: %empty  */
                                                          { yyextra->das_need_oxford_comma=false; }
    break;

  case 892: /* $@80: %empty  */
                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 893: /* $@81: %empty  */
                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 894: /* $@82: %empty  */
                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
    }
    break;

  case 895: /* tuple_alias_declaration: "tuple" optional_public_or_private_alias $@79 "name" $@80 open_block $@81 tuple_alias_type_list $@82 close_block  */
                  {
        auto vtype = new TypeDecl(Type::tTuple);
        vtype->alias = *(yyvsp[-6].s);
        vtype->at = tokAt(scanner,(yylsp[-6]));
        vtype->isPrivateAlias = !(yyvsp[-8].b);
        varDeclToTypeDecl(scanner, vtype, (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-6].s),tokAt(scanner,(yylsp[-6])),
                CompilationError::already_declared_type_alias);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTuple((yyvsp[-6].s)->c_str(),atvname);
        }
        delete (yyvsp[-6].s);
    }
    break;

  case 896: /* $@83: %empty  */
                                                            { yyextra->das_need_oxford_comma=false; }
    break;

  case 897: /* $@84: %empty  */
                                                                                                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 898: /* $@85: %empty  */
                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 899: /* $@86: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
    }
    break;

  case 900: /* variant_alias_declaration: "variant" optional_public_or_private_alias $@83 "name" $@84 open_block $@85 variant_alias_type_list $@86 close_block  */
                  {
        auto vtype = new TypeDecl(Type::tVariant);
        vtype->alias = *(yyvsp[-6].s);
        vtype->at = tokAt(scanner,(yylsp[-6]));
        vtype->isPrivateAlias = !(yyvsp[-8].b);
        varDeclToTypeDecl(scanner, vtype, (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-6].s),tokAt(scanner,(yylsp[-6])),
                CompilationError::already_declared_type_alias);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariant((yyvsp[-6].s)->c_str(),atvname);
        }
        delete (yyvsp[-6].s);
    }
    break;

  case 901: /* $@87: %empty  */
                                                             { yyextra->das_need_oxford_comma=false; }
    break;

  case 902: /* $@88: %empty  */
                                                                                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 903: /* $@89: %empty  */
                                                            {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 904: /* $@90: %empty  */
                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-5]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
    }
    break;

  case 905: /* bitfield_alias_declaration: "bitfield" optional_public_or_private_alias $@87 "name" $@88 bitfield_basic_type_declaration open_block $@89 bitfield_alias_bits $@90 close_block  */
                  {
        auto btype = new TypeDecl((yyvsp[-5].type));
        btype->alias = *(yyvsp[-7].s);
        btype->at = tokAt(scanner,(yylsp[-7]));
        btype->isPrivateAlias = !(yyvsp[-9].b);
        for ( auto & p : *(yyvsp[-2].pNameExprList) ) {
            if ( !get<1>(p) ) {
                btype->argNames.push_back(get<0>(p));
            }
        }
        auto maxBits = btype->maxBitfieldBits();
        if ( btype->argNames.size()>maxBits ) {
            das_yyerror(scanner,"only " + to_string(maxBits) + " different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-7])),
                CompilationError::exceeds_bitfield);
        }
        for ( auto & p : *(yyvsp[-2].pNameExprList) ) {
            if ( get<1>(p) ) {
                ast_globalBitfieldConst ( scanner, btype, (yyvsp[-9].b), get<0>(p), get<1>(p) );
            }
        }
        if ( !yyextra->g_Program->addAlias(btype) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-7].s),tokAt(scanner,(yylsp[-7])),
                CompilationError::already_declared_type_alias);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-7]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfield((yyvsp[-7].s)->c_str(),atvname);
        }
        delete (yyvsp[-7].s);
        delete (yyvsp[-2].pNameExprList);
    }
    break;

  case 906: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 907: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 908: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 909: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 910: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 911: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 912: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 913: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 914: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 915: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 916: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 917: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 918: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 919: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 920: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 921: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 922: /* make_struct_dim: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 923: /* make_struct_dim: make_struct_dim "end of expression" make_struct_fields  */
                                                         {
        ((ExprMakeStruct *) (yyvsp[-2].pExpression))->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 924: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 925: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 926: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 927: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 928: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 929: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 930: /* optional_block: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 931: /* optional_block: "where" expr_block  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 944: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 945: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 946: /* make_struct_decl: "[[" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-4]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 947: /* make_struct_decl: "[[" type_declaration_no_options optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->makeType = (yyvsp[-2].pTypeDecl);
        msd->block = (yyvsp[-1].pExpression);
        msd->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = msd;
    }
    break;

  case 948: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                   {
        auto msd = new ExprMakeStruct();
        msd->makeType = (yyvsp[-4].pTypeDecl);
        msd->useInitializer = true;
        msd->block = (yyvsp[-1].pExpression);
        msd->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pExpression) = msd;
    }
    break;

  case 949: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-5].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 950: /* make_struct_decl: "[{" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
                                                                                                                                {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-4]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_array_move");
        tam->arguments.push_back((yyvsp[-2].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 951: /* make_struct_decl: "[{" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
                                                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-5].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-6]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),"to_array_move");
        tam->arguments.push_back((yyvsp[-2].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 952: /* $@91: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 953: /* $@92: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 954: /* make_struct_decl: "struct" '<' $@91 type_declaration_no_options '>' $@92 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                            {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 955: /* $@93: %empty  */
                            { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 956: /* $@94: %empty  */
                                                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 957: /* make_struct_decl: "class" '<' $@93 type_declaration_no_options '>' $@94 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                           {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 958: /* $@95: %empty  */
                               { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 959: /* $@96: %empty  */
                                                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 960: /* make_struct_decl: "variant" '<' $@95 variant_type_list '>' $@96 '(' use_initializer make_variant_dim ')'  */
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

  case 961: /* $@97: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 962: /* $@98: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 963: /* make_struct_decl: "default" '<' $@97 type_declaration_no_options '>' $@98 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 964: /* make_tuple: expr  */
                  {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 965: /* make_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 966: /* make_tuple: make_tuple ',' expr  */
                                      {
        ExprMakeTuple * mt;
        if ( (yyvsp[-2].pExpression)->rtti_isMakeTuple() ) {
            mt = static_cast<ExprMakeTuple *>((yyvsp[-2].pExpression));
        } else {
            mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-2])));
            mt->values.push_back((yyvsp[-2].pExpression));
        }
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 967: /* make_map_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 968: /* make_map_tuple: expr  */
                 {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 969: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = new TypeDecl(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 970: /* $@99: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 971: /* $@100: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 972: /* make_tuple_call: "tuple" '<' $@99 tuple_type_list '>' $@100 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 973: /* make_dim: make_tuple  */
                        {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 974: /* make_dim: make_dim "end of expression" make_tuple  */
                                          {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 975: /* make_dim_decl: '[' optional_expr_list ']'  */
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

  case 976: /* make_dim_decl: "[[" type_declaration_no_options make_dim optional_trailing_semicolon_sqr_sqr  */
                                                                                                         {
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-2].pTypeDecl);
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 977: /* make_dim_decl: "[{" type_declaration_no_options make_dim optional_trailing_semicolon_cur_sqr  */
                                                                                                         {
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-2].pTypeDecl);
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 978: /* $@101: %empty  */
                                       { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 979: /* $@102: %empty  */
                                                                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 980: /* make_dim_decl: "array" "struct" '<' $@101 type_declaration_no_options '>' $@102 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 981: /* $@103: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 982: /* $@104: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 983: /* make_dim_decl: "array" "tuple" '<' $@103 tuple_type_list '>' $@104 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 984: /* $@105: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 985: /* $@106: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 986: /* make_dim_decl: "array" "variant" '<' $@105 variant_type_list '>' $@106 '(' make_variant_dim ')'  */
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

  case 987: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
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

  case 988: /* $@107: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 989: /* $@108: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 990: /* make_dim_decl: "array" '<' $@107 type_declaration_no_options '>' $@108 '(' optional_expr_list ')'  */
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

  case 991: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 992: /* $@109: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 993: /* $@110: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 994: /* make_dim_decl: "fixed_array" '<' $@109 type_declaration_no_options '>' $@110 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 995: /* make_table: make_map_tuple  */
                            {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 996: /* make_table: make_table "end of expression" make_map_tuple  */
                                                {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 997: /* expr_map_tuple_list: make_map_tuple  */
                                {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 998: /* expr_map_tuple_list: expr_map_tuple_list ',' make_map_tuple  */
                                                                {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 999: /* make_table_decl: "begin of code block" optional_expr_map_tuple_list "end of code block"  */
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

  case 1000: /* make_table_decl: "{{" make_table optional_trailing_semicolon_cur_cur  */
                                                                          {
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

  case 1001: /* make_table_decl: "table" '(' optional_expr_map_tuple_list ')'  */
                                                                       {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-1].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 1002: /* make_table_decl: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 1003: /* make_table_decl: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 1004: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 1005: /* array_comprehension_where: "end of expression" "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 1006: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 1007: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 1008: /* array_comprehension: '[' "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                    {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 1009: /* array_comprehension: '[' "iterator" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                                 {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 1010: /* array_comprehension: "[[" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']' ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),true,false);
    }
    break;

  case 1011: /* array_comprehension: "[{" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where "end of code block" ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),false,false);
    }
    break;

  case 1012: /* array_comprehension: "begin of code block" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where "end of code block"  */
                                                                                                                                                              {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
    }
    break;

  case 1013: /* array_comprehension: "{{" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where "end of code block" "end of code block"  */
                                                                                                                                                                    {
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



void das_yyfatalerror ( DAS_YYLTYPE * lloc, yyscan_t scanner, const string & error, CompilationError cerr ) {
    yyextra->g_Program->error(error,"","",LineInfo(yyextra->g_FileAccessStack.back(),
        lloc->first_column,lloc->first_line,lloc->last_column,lloc->last_line),cerr);
}

void das_yyerror ( DAS_YYLTYPE * lloc, yyscan_t scanner, const string & error ) {
    if ( !yyextra->das_suppress_errors ) {
        yyextra->g_Program->error(error,"","",LineInfo(yyextra->g_FileAccessStack.back(),
            lloc->first_column,lloc->first_line,lloc->last_column,lloc->last_line),
                CompilationError::invalid_expression);
    }
}

LineInfo tokAt ( yyscan_t scanner, const struct DAS_YYLTYPE & li ) {
    return LineInfo(yyextra->g_FileAccessStack.back(),
        li.first_column,li.first_line,
        li.last_column,li.last_line);
}

LineInfo tokRangeAt ( yyscan_t scanner, const struct DAS_YYLTYPE & li, const struct DAS_YYLTYPE & lie ) {
    return LineInfo(yyextra->g_FileAccessStack.back(),
        li.first_column,li.first_line,
        lie.last_column,lie.last_line);
}
