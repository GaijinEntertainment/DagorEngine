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
  YYSYMBOL_DAS_TTUPLE = 102,               /* "tuple"  */
  YYSYMBOL_DAS_TVARIANT = 103,             /* "variant"  */
  YYSYMBOL_DAS_GENERATOR = 104,            /* "generator"  */
  YYSYMBOL_DAS_YIELD = 105,                /* "yield"  */
  YYSYMBOL_DAS_SEALED = 106,               /* "sealed"  */
  YYSYMBOL_ADDEQU = 107,                   /* "+="  */
  YYSYMBOL_SUBEQU = 108,                   /* "-="  */
  YYSYMBOL_DIVEQU = 109,                   /* "/="  */
  YYSYMBOL_MULEQU = 110,                   /* "*="  */
  YYSYMBOL_MODEQU = 111,                   /* "%="  */
  YYSYMBOL_ANDEQU = 112,                   /* "&="  */
  YYSYMBOL_OREQU = 113,                    /* "|="  */
  YYSYMBOL_XOREQU = 114,                   /* "^="  */
  YYSYMBOL_SHL = 115,                      /* "<<"  */
  YYSYMBOL_SHR = 116,                      /* ">>"  */
  YYSYMBOL_ADDADD = 117,                   /* "++"  */
  YYSYMBOL_SUBSUB = 118,                   /* "--"  */
  YYSYMBOL_LEEQU = 119,                    /* "<="  */
  YYSYMBOL_SHLEQU = 120,                   /* "<<="  */
  YYSYMBOL_SHREQU = 121,                   /* ">>="  */
  YYSYMBOL_GREQU = 122,                    /* ">="  */
  YYSYMBOL_EQUEQU = 123,                   /* "=="  */
  YYSYMBOL_NOTEQU = 124,                   /* "!="  */
  YYSYMBOL_RARROW = 125,                   /* "->"  */
  YYSYMBOL_LARROW = 126,                   /* "<-"  */
  YYSYMBOL_QQ = 127,                       /* "??"  */
  YYSYMBOL_QDOT = 128,                     /* "?."  */
  YYSYMBOL_QBRA = 129,                     /* "?["  */
  YYSYMBOL_LPIPE = 130,                    /* "<|"  */
  YYSYMBOL_LBPIPE = 131,                   /* " <|"  */
  YYSYMBOL_LLPIPE = 132,                   /* "$ <|"  */
  YYSYMBOL_LAPIPE = 133,                   /* "@ <|"  */
  YYSYMBOL_LFPIPE = 134,                   /* "@@ <|"  */
  YYSYMBOL_RPIPE = 135,                    /* "|>"  */
  YYSYMBOL_CLONEEQU = 136,                 /* ":="  */
  YYSYMBOL_ROTL = 137,                     /* "<<<"  */
  YYSYMBOL_ROTR = 138,                     /* ">>>"  */
  YYSYMBOL_ROTLEQU = 139,                  /* "<<<="  */
  YYSYMBOL_ROTREQU = 140,                  /* ">>>="  */
  YYSYMBOL_MAPTO = 141,                    /* "=>"  */
  YYSYMBOL_COLCOL = 142,                   /* "::"  */
  YYSYMBOL_ANDAND = 143,                   /* "&&"  */
  YYSYMBOL_OROR = 144,                     /* "||"  */
  YYSYMBOL_XORXOR = 145,                   /* "^^"  */
  YYSYMBOL_ANDANDEQU = 146,                /* "&&="  */
  YYSYMBOL_OROREQU = 147,                  /* "||="  */
  YYSYMBOL_XORXOREQU = 148,                /* "^^="  */
  YYSYMBOL_DOTDOT = 149,                   /* ".."  */
  YYSYMBOL_MTAG_E = 150,                   /* "$$"  */
  YYSYMBOL_MTAG_I = 151,                   /* "$i"  */
  YYSYMBOL_MTAG_V = 152,                   /* "$v"  */
  YYSYMBOL_MTAG_B = 153,                   /* "$b"  */
  YYSYMBOL_MTAG_A = 154,                   /* "$a"  */
  YYSYMBOL_MTAG_T = 155,                   /* "$t"  */
  YYSYMBOL_MTAG_C = 156,                   /* "$c"  */
  YYSYMBOL_MTAG_F = 157,                   /* "$f"  */
  YYSYMBOL_MTAG_DOTDOTDOT = 158,           /* "..."  */
  YYSYMBOL_BRABRAB = 159,                  /* "[["  */
  YYSYMBOL_BRACBRB = 160,                  /* "[{"  */
  YYSYMBOL_CBRCBRB = 161,                  /* "{{"  */
  YYSYMBOL_INTEGER = 162,                  /* "integer constant"  */
  YYSYMBOL_LONG_INTEGER = 163,             /* "long integer constant"  */
  YYSYMBOL_UNSIGNED_INTEGER = 164,         /* "unsigned integer constant"  */
  YYSYMBOL_UNSIGNED_LONG_INTEGER = 165,    /* "unsigned long integer constant"  */
  YYSYMBOL_UNSIGNED_INT8 = 166,            /* "unsigned int8 constant"  */
  YYSYMBOL_FLOAT = 167,                    /* "floating point constant"  */
  YYSYMBOL_DOUBLE = 168,                   /* "double constant"  */
  YYSYMBOL_NAME = 169,                     /* "name"  */
  YYSYMBOL_KEYWORD = 170,                  /* "keyword"  */
  YYSYMBOL_TYPE_FUNCTION = 171,            /* "type function"  */
  YYSYMBOL_BEGIN_STRING = 172,             /* "start of the string"  */
  YYSYMBOL_STRING_CHARACTER = 173,         /* STRING_CHARACTER  */
  YYSYMBOL_STRING_CHARACTER_ESC = 174,     /* STRING_CHARACTER_ESC  */
  YYSYMBOL_END_STRING = 175,               /* "end of the string"  */
  YYSYMBOL_BEGIN_STRING_EXPR = 176,        /* "{"  */
  YYSYMBOL_END_STRING_EXPR = 177,          /* "}"  */
  YYSYMBOL_END_OF_READ = 178,              /* "end of failed eader macro"  */
  YYSYMBOL_179_begin_of_code_block_ = 179, /* "begin of code block"  */
  YYSYMBOL_180_end_of_code_block_ = 180,   /* "end of code block"  */
  YYSYMBOL_181_end_of_expression_ = 181,   /* "end of expression"  */
  YYSYMBOL_SEMICOLON_CUR_CUR = 182,        /* ";}}"  */
  YYSYMBOL_SEMICOLON_CUR_SQR = 183,        /* ";}]"  */
  YYSYMBOL_SEMICOLON_SQR_SQR = 184,        /* ";]]"  */
  YYSYMBOL_COMMA_SQR_SQR = 185,            /* ",]]"  */
  YYSYMBOL_COMMA_CUR_SQR = 186,            /* ",}]"  */
  YYSYMBOL_187_ = 187,                     /* ','  */
  YYSYMBOL_188_ = 188,                     /* '='  */
  YYSYMBOL_189_ = 189,                     /* '?'  */
  YYSYMBOL_190_ = 190,                     /* ':'  */
  YYSYMBOL_191_ = 191,                     /* '|'  */
  YYSYMBOL_192_ = 192,                     /* '^'  */
  YYSYMBOL_193_ = 193,                     /* '&'  */
  YYSYMBOL_194_ = 194,                     /* '<'  */
  YYSYMBOL_195_ = 195,                     /* '>'  */
  YYSYMBOL_196_ = 196,                     /* '-'  */
  YYSYMBOL_197_ = 197,                     /* '+'  */
  YYSYMBOL_198_ = 198,                     /* '*'  */
  YYSYMBOL_199_ = 199,                     /* '/'  */
  YYSYMBOL_200_ = 200,                     /* '%'  */
  YYSYMBOL_UNARY_MINUS = 201,              /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 202,               /* UNARY_PLUS  */
  YYSYMBOL_203_ = 203,                     /* '~'  */
  YYSYMBOL_204_ = 204,                     /* '!'  */
  YYSYMBOL_PRE_INC = 205,                  /* PRE_INC  */
  YYSYMBOL_PRE_DEC = 206,                  /* PRE_DEC  */
  YYSYMBOL_POST_INC = 207,                 /* POST_INC  */
  YYSYMBOL_POST_DEC = 208,                 /* POST_DEC  */
  YYSYMBOL_DEREF = 209,                    /* DEREF  */
  YYSYMBOL_210_ = 210,                     /* '.'  */
  YYSYMBOL_211_ = 211,                     /* '['  */
  YYSYMBOL_212_ = 212,                     /* ']'  */
  YYSYMBOL_213_ = 213,                     /* '('  */
  YYSYMBOL_214_ = 214,                     /* ')'  */
  YYSYMBOL_215_ = 215,                     /* '$'  */
  YYSYMBOL_216_ = 216,                     /* '@'  */
  YYSYMBOL_217_ = 217,                     /* '#'  */
  YYSYMBOL_YYACCEPT = 218,                 /* $accept  */
  YYSYMBOL_program = 219,                  /* program  */
  YYSYMBOL_top_level_reader_macro = 220,   /* top_level_reader_macro  */
  YYSYMBOL_optional_public_or_private_module = 221, /* optional_public_or_private_module  */
  YYSYMBOL_module_name = 222,              /* module_name  */
  YYSYMBOL_module_declaration = 223,       /* module_declaration  */
  YYSYMBOL_character_sequence = 224,       /* character_sequence  */
  YYSYMBOL_string_constant = 225,          /* string_constant  */
  YYSYMBOL_string_builder_body = 226,      /* string_builder_body  */
  YYSYMBOL_string_builder = 227,           /* string_builder  */
  YYSYMBOL_reader_character_sequence = 228, /* reader_character_sequence  */
  YYSYMBOL_expr_reader = 229,              /* expr_reader  */
  YYSYMBOL_230_1 = 230,                    /* $@1  */
  YYSYMBOL_options_declaration = 231,      /* options_declaration  */
  YYSYMBOL_require_declaration = 232,      /* require_declaration  */
  YYSYMBOL_keyword_or_name = 233,          /* keyword_or_name  */
  YYSYMBOL_require_module_name = 234,      /* require_module_name  */
  YYSYMBOL_require_module = 235,           /* require_module  */
  YYSYMBOL_is_public_module = 236,         /* is_public_module  */
  YYSYMBOL_expect_declaration = 237,       /* expect_declaration  */
  YYSYMBOL_expect_list = 238,              /* expect_list  */
  YYSYMBOL_expect_error = 239,             /* expect_error  */
  YYSYMBOL_expression_label = 240,         /* expression_label  */
  YYSYMBOL_expression_goto = 241,          /* expression_goto  */
  YYSYMBOL_elif_or_static_elif = 242,      /* elif_or_static_elif  */
  YYSYMBOL_expression_else = 243,          /* expression_else  */
  YYSYMBOL_if_or_static_if = 244,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 245, /* expression_else_one_liner  */
  YYSYMBOL_246_2 = 246,                    /* $@2  */
  YYSYMBOL_expression_if_one_liner = 247,  /* expression_if_one_liner  */
  YYSYMBOL_expression_if_then_else = 248,  /* expression_if_then_else  */
  YYSYMBOL_249_3 = 249,                    /* $@3  */
  YYSYMBOL_expression_for_loop = 250,      /* expression_for_loop  */
  YYSYMBOL_251_4 = 251,                    /* $@4  */
  YYSYMBOL_expression_unsafe = 252,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 253,    /* expression_while_loop  */
  YYSYMBOL_expression_with = 254,          /* expression_with  */
  YYSYMBOL_expression_with_alias = 255,    /* expression_with_alias  */
  YYSYMBOL_256_5 = 256,                    /* $@5  */
  YYSYMBOL_annotation_argument_value = 257, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 258, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 259, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 260,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 261, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 262,   /* metadata_argument_list  */
  YYSYMBOL_annotation_declaration_name = 263, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 264, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 265,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 266,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 267, /* optional_annotation_list  */
  YYSYMBOL_optional_function_argument_list = 268, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 269,   /* optional_function_type  */
  YYSYMBOL_function_name = 270,            /* function_name  */
  YYSYMBOL_global_function_declaration = 271, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 272, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 273, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 274,     /* function_declaration  */
  YYSYMBOL_275_6 = 275,                    /* $@6  */
  YYSYMBOL_expression_block = 276,         /* expression_block  */
  YYSYMBOL_expr_call_pipe = 277,           /* expr_call_pipe  */
  YYSYMBOL_expression_any = 278,           /* expression_any  */
  YYSYMBOL_expressions = 279,              /* expressions  */
  YYSYMBOL_expr_keyword = 280,             /* expr_keyword  */
  YYSYMBOL_optional_expr_list = 281,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_list_in_braces = 282, /* optional_expr_list_in_braces  */
  YYSYMBOL_optional_expr_map_tuple_list = 283, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 284, /* type_declaration_no_options_list  */
  YYSYMBOL_expression_keyword = 285,       /* expression_keyword  */
  YYSYMBOL_286_7 = 286,                    /* $@7  */
  YYSYMBOL_287_8 = 287,                    /* $@8  */
  YYSYMBOL_288_9 = 288,                    /* $@9  */
  YYSYMBOL_289_10 = 289,                   /* $@10  */
  YYSYMBOL_expr_pipe = 290,                /* expr_pipe  */
  YYSYMBOL_name_in_namespace = 291,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 292,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 293,     /* new_type_declaration  */
  YYSYMBOL_294_11 = 294,                   /* $@11  */
  YYSYMBOL_295_12 = 295,                   /* $@12  */
  YYSYMBOL_expr_new = 296,                 /* expr_new  */
  YYSYMBOL_expression_break = 297,         /* expression_break  */
  YYSYMBOL_expression_continue = 298,      /* expression_continue  */
  YYSYMBOL_expression_return_no_pipe = 299, /* expression_return_no_pipe  */
  YYSYMBOL_expression_return = 300,        /* expression_return  */
  YYSYMBOL_expression_yield_no_pipe = 301, /* expression_yield_no_pipe  */
  YYSYMBOL_expression_yield = 302,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 303,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 304,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 305,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 306,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 307,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 308, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 309,           /* expression_let  */
  YYSYMBOL_expr_cast = 310,                /* expr_cast  */
  YYSYMBOL_311_13 = 311,                   /* $@13  */
  YYSYMBOL_312_14 = 312,                   /* $@14  */
  YYSYMBOL_313_15 = 313,                   /* $@15  */
  YYSYMBOL_314_16 = 314,                   /* $@16  */
  YYSYMBOL_315_17 = 315,                   /* $@17  */
  YYSYMBOL_316_18 = 316,                   /* $@18  */
  YYSYMBOL_expr_type_decl = 317,           /* expr_type_decl  */
  YYSYMBOL_318_19 = 318,                   /* $@19  */
  YYSYMBOL_319_20 = 319,                   /* $@20  */
  YYSYMBOL_expr_type_info = 320,           /* expr_type_info  */
  YYSYMBOL_expr_list = 321,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 322,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 323,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 324,            /* capture_entry  */
  YYSYMBOL_capture_list = 325,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 326,    /* optional_capture_list  */
  YYSYMBOL_expr_block = 327,               /* expr_block  */
  YYSYMBOL_expr_full_block = 328,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 329, /* expr_full_block_assumed_piped  */
  YYSYMBOL_330_21 = 330,                   /* $@21  */
  YYSYMBOL_expr_numeric_const = 331,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign = 332,              /* expr_assign  */
  YYSYMBOL_expr_assign_pipe_right = 333,   /* expr_assign_pipe_right  */
  YYSYMBOL_expr_assign_pipe = 334,         /* expr_assign_pipe  */
  YYSYMBOL_expr_named_call = 335,          /* expr_named_call  */
  YYSYMBOL_expr_method_call = 336,         /* expr_method_call  */
  YYSYMBOL_func_addr_name = 337,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 338,           /* func_addr_expr  */
  YYSYMBOL_339_22 = 339,                   /* $@22  */
  YYSYMBOL_340_23 = 340,                   /* $@23  */
  YYSYMBOL_341_24 = 341,                   /* $@24  */
  YYSYMBOL_342_25 = 342,                   /* $@25  */
  YYSYMBOL_expr_field = 343,               /* expr_field  */
  YYSYMBOL_344_26 = 344,                   /* $@26  */
  YYSYMBOL_345_27 = 345,                   /* $@27  */
  YYSYMBOL_expr_call = 346,                /* expr_call  */
  YYSYMBOL_expr = 347,                     /* expr  */
  YYSYMBOL_348_28 = 348,                   /* $@28  */
  YYSYMBOL_349_29 = 349,                   /* $@29  */
  YYSYMBOL_350_30 = 350,                   /* $@30  */
  YYSYMBOL_351_31 = 351,                   /* $@31  */
  YYSYMBOL_352_32 = 352,                   /* $@32  */
  YYSYMBOL_353_33 = 353,                   /* $@33  */
  YYSYMBOL_expr_mtag = 354,                /* expr_mtag  */
  YYSYMBOL_optional_field_annotation = 355, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 356,        /* optional_override  */
  YYSYMBOL_optional_constant = 357,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 358, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 359, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 360, /* structure_variable_declaration  */
  YYSYMBOL_struct_variable_declaration_list = 361, /* struct_variable_declaration_list  */
  YYSYMBOL_362_34 = 362,                   /* $@34  */
  YYSYMBOL_363_35 = 363,                   /* $@35  */
  YYSYMBOL_364_36 = 364,                   /* $@36  */
  YYSYMBOL_function_argument_declaration = 365, /* function_argument_declaration  */
  YYSYMBOL_function_argument_list = 366,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 367,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 368,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 369,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 370,             /* variant_type  */
  YYSYMBOL_variant_type_list = 371,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 372,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 373,             /* copy_or_move  */
  YYSYMBOL_variable_declaration = 374,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 375,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 376,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 377, /* let_variable_name_with_pos_list  */
  YYSYMBOL_let_variable_declaration = 378, /* let_variable_declaration  */
  YYSYMBOL_global_variable_declaration_list = 379, /* global_variable_declaration_list  */
  YYSYMBOL_380_37 = 380,                   /* $@37  */
  YYSYMBOL_optional_shared = 381,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 382, /* optional_public_or_private_variable  */
  YYSYMBOL_global_let = 383,               /* global_let  */
  YYSYMBOL_384_38 = 384,                   /* $@38  */
  YYSYMBOL_enum_list = 385,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 386, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 387,             /* single_alias  */
  YYSYMBOL_388_39 = 388,                   /* $@39  */
  YYSYMBOL_alias_list = 389,               /* alias_list  */
  YYSYMBOL_alias_declaration = 390,        /* alias_declaration  */
  YYSYMBOL_391_40 = 391,                   /* $@40  */
  YYSYMBOL_optional_public_or_private_enum = 392, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 393,                /* enum_name  */
  YYSYMBOL_enum_declaration = 394,         /* enum_declaration  */
  YYSYMBOL_395_41 = 395,                   /* $@41  */
  YYSYMBOL_396_42 = 396,                   /* $@42  */
  YYSYMBOL_397_43 = 397,                   /* $@43  */
  YYSYMBOL_398_44 = 398,                   /* $@44  */
  YYSYMBOL_optional_structure_parent = 399, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 400,          /* optional_sealed  */
  YYSYMBOL_structure_name = 401,           /* structure_name  */
  YYSYMBOL_class_or_struct = 402,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 403, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 404, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 405,    /* structure_declaration  */
  YYSYMBOL_406_45 = 406,                   /* $@45  */
  YYSYMBOL_407_46 = 407,                   /* $@46  */
  YYSYMBOL_variable_name_with_pos_list = 408, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 409,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 410, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 411, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 412,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 413,            /* bitfield_bits  */
  YYSYMBOL_bitfield_alias_bits = 414,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_type_declaration = 415, /* bitfield_type_declaration  */
  YYSYMBOL_416_47 = 416,                   /* $@47  */
  YYSYMBOL_417_48 = 417,                   /* $@48  */
  YYSYMBOL_c_or_s = 418,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 419,          /* table_type_pair  */
  YYSYMBOL_dim_list = 420,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 421, /* type_declaration_no_options  */
  YYSYMBOL_422_49 = 422,                   /* $@49  */
  YYSYMBOL_423_50 = 423,                   /* $@50  */
  YYSYMBOL_424_51 = 424,                   /* $@51  */
  YYSYMBOL_425_52 = 425,                   /* $@52  */
  YYSYMBOL_426_53 = 426,                   /* $@53  */
  YYSYMBOL_427_54 = 427,                   /* $@54  */
  YYSYMBOL_428_55 = 428,                   /* $@55  */
  YYSYMBOL_429_56 = 429,                   /* $@56  */
  YYSYMBOL_430_57 = 430,                   /* $@57  */
  YYSYMBOL_431_58 = 431,                   /* $@58  */
  YYSYMBOL_432_59 = 432,                   /* $@59  */
  YYSYMBOL_433_60 = 433,                   /* $@60  */
  YYSYMBOL_434_61 = 434,                   /* $@61  */
  YYSYMBOL_435_62 = 435,                   /* $@62  */
  YYSYMBOL_436_63 = 436,                   /* $@63  */
  YYSYMBOL_437_64 = 437,                   /* $@64  */
  YYSYMBOL_438_65 = 438,                   /* $@65  */
  YYSYMBOL_439_66 = 439,                   /* $@66  */
  YYSYMBOL_440_67 = 440,                   /* $@67  */
  YYSYMBOL_441_68 = 441,                   /* $@68  */
  YYSYMBOL_442_69 = 442,                   /* $@69  */
  YYSYMBOL_443_70 = 443,                   /* $@70  */
  YYSYMBOL_444_71 = 444,                   /* $@71  */
  YYSYMBOL_445_72 = 445,                   /* $@72  */
  YYSYMBOL_446_73 = 446,                   /* $@73  */
  YYSYMBOL_447_74 = 447,                   /* $@74  */
  YYSYMBOL_448_75 = 448,                   /* $@75  */
  YYSYMBOL_type_declaration = 449,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 450,  /* tuple_alias_declaration  */
  YYSYMBOL_451_76 = 451,                   /* $@76  */
  YYSYMBOL_452_77 = 452,                   /* $@77  */
  YYSYMBOL_453_78 = 453,                   /* $@78  */
  YYSYMBOL_454_79 = 454,                   /* $@79  */
  YYSYMBOL_variant_alias_declaration = 455, /* variant_alias_declaration  */
  YYSYMBOL_456_80 = 456,                   /* $@80  */
  YYSYMBOL_457_81 = 457,                   /* $@81  */
  YYSYMBOL_458_82 = 458,                   /* $@82  */
  YYSYMBOL_459_83 = 459,                   /* $@83  */
  YYSYMBOL_bitfield_alias_declaration = 460, /* bitfield_alias_declaration  */
  YYSYMBOL_461_84 = 461,                   /* $@84  */
  YYSYMBOL_462_85 = 462,                   /* $@85  */
  YYSYMBOL_463_86 = 463,                   /* $@86  */
  YYSYMBOL_464_87 = 464,                   /* $@87  */
  YYSYMBOL_make_decl = 465,                /* make_decl  */
  YYSYMBOL_make_struct_fields = 466,       /* make_struct_fields  */
  YYSYMBOL_make_variant_dim = 467,         /* make_variant_dim  */
  YYSYMBOL_make_struct_single = 468,       /* make_struct_single  */
  YYSYMBOL_make_struct_dim = 469,          /* make_struct_dim  */
  YYSYMBOL_make_struct_dim_list = 470,     /* make_struct_dim_list  */
  YYSYMBOL_make_struct_dim_decl = 471,     /* make_struct_dim_decl  */
  YYSYMBOL_optional_make_struct_dim_decl = 472, /* optional_make_struct_dim_decl  */
  YYSYMBOL_optional_block = 473,           /* optional_block  */
  YYSYMBOL_optional_trailing_semicolon_cur_cur = 474, /* optional_trailing_semicolon_cur_cur  */
  YYSYMBOL_optional_trailing_semicolon_cur_sqr = 475, /* optional_trailing_semicolon_cur_sqr  */
  YYSYMBOL_optional_trailing_semicolon_sqr_sqr = 476, /* optional_trailing_semicolon_sqr_sqr  */
  YYSYMBOL_optional_trailing_delim_sqr_sqr = 477, /* optional_trailing_delim_sqr_sqr  */
  YYSYMBOL_optional_trailing_delim_cur_sqr = 478, /* optional_trailing_delim_cur_sqr  */
  YYSYMBOL_use_initializer = 479,          /* use_initializer  */
  YYSYMBOL_make_struct_decl = 480,         /* make_struct_decl  */
  YYSYMBOL_481_88 = 481,                   /* $@88  */
  YYSYMBOL_482_89 = 482,                   /* $@89  */
  YYSYMBOL_483_90 = 483,                   /* $@90  */
  YYSYMBOL_484_91 = 484,                   /* $@91  */
  YYSYMBOL_485_92 = 485,                   /* $@92  */
  YYSYMBOL_486_93 = 486,                   /* $@93  */
  YYSYMBOL_487_94 = 487,                   /* $@94  */
  YYSYMBOL_488_95 = 488,                   /* $@95  */
  YYSYMBOL_make_tuple = 489,               /* make_tuple  */
  YYSYMBOL_make_map_tuple = 490,           /* make_map_tuple  */
  YYSYMBOL_make_tuple_call = 491,          /* make_tuple_call  */
  YYSYMBOL_492_96 = 492,                   /* $@96  */
  YYSYMBOL_493_97 = 493,                   /* $@97  */
  YYSYMBOL_make_dim = 494,                 /* make_dim  */
  YYSYMBOL_make_dim_decl = 495,            /* make_dim_decl  */
  YYSYMBOL_496_98 = 496,                   /* $@98  */
  YYSYMBOL_497_99 = 497,                   /* $@99  */
  YYSYMBOL_498_100 = 498,                  /* $@100  */
  YYSYMBOL_499_101 = 499,                  /* $@101  */
  YYSYMBOL_500_102 = 500,                  /* $@102  */
  YYSYMBOL_501_103 = 501,                  /* $@103  */
  YYSYMBOL_502_104 = 502,                  /* $@104  */
  YYSYMBOL_503_105 = 503,                  /* $@105  */
  YYSYMBOL_504_106 = 504,                  /* $@106  */
  YYSYMBOL_505_107 = 505,                  /* $@107  */
  YYSYMBOL_make_table = 506,               /* make_table  */
  YYSYMBOL_expr_map_tuple_list = 507,      /* expr_map_tuple_list  */
  YYSYMBOL_make_table_decl = 508,          /* make_table_decl  */
  YYSYMBOL_array_comprehension_where = 509, /* array_comprehension_where  */
  YYSYMBOL_optional_comma = 510,           /* optional_comma  */
  YYSYMBOL_array_comprehension = 511       /* array_comprehension  */
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
#define YYLAST   14980

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  218
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  294
/* YYNRULES -- Number of rules.  */
#define YYNRULES  890
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1690

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   445


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
       2,     2,     2,   204,     2,   217,   215,   200,   193,     2,
     213,   214,   198,   197,   187,   196,   210,   199,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   190,   181,
     194,   188,   195,   189,   216,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   211,     2,   212,   192,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   179,   191,   180,   203,     2,     2,     2,
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
     175,   176,   177,   178,   182,   183,   184,   185,   186,   201,
     202,   205,   206,   207,   208,   209
};

#if DAS_YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   543,   543,   544,   549,   550,   551,   552,   553,   554,
     555,   556,   557,   558,   559,   560,   561,   565,   571,   572,
     573,   577,   578,   582,   600,   601,   602,   603,   607,   608,
     612,   617,   626,   634,   650,   655,   663,   663,   702,   732,
     736,   737,   738,   742,   745,   749,   755,   764,   767,   773,
     774,   778,   782,   783,   787,   790,   796,   802,   805,   811,
     812,   816,   817,   818,   827,   828,   832,   833,   833,   839,
     840,   841,   842,   843,   847,   853,   853,   859,   859,   865,
     873,   883,   892,   892,   899,   900,   901,   902,   903,   904,
     908,   913,   921,   922,   923,   927,   928,   929,   930,   931,
     932,   933,   934,   940,   943,   949,   952,   955,   961,   962,
     963,   967,   980,   998,  1001,  1009,  1020,  1031,  1042,  1045,
    1052,  1056,  1063,  1064,  1068,  1069,  1070,  1074,  1077,  1084,
    1088,  1089,  1090,  1091,  1092,  1093,  1094,  1095,  1096,  1097,
    1098,  1099,  1100,  1101,  1102,  1103,  1104,  1105,  1106,  1107,
    1108,  1109,  1110,  1111,  1112,  1113,  1114,  1115,  1116,  1117,
    1118,  1119,  1120,  1121,  1122,  1123,  1124,  1125,  1126,  1127,
    1128,  1129,  1130,  1131,  1132,  1133,  1134,  1135,  1136,  1137,
    1138,  1139,  1140,  1141,  1142,  1143,  1144,  1145,  1146,  1147,
    1148,  1149,  1150,  1151,  1152,  1153,  1154,  1155,  1156,  1157,
    1158,  1159,  1160,  1161,  1162,  1163,  1164,  1165,  1166,  1167,
    1168,  1169,  1170,  1171,  1176,  1194,  1195,  1196,  1200,  1206,
    1206,  1223,  1227,  1238,  1247,  1256,  1262,  1263,  1264,  1265,
    1266,  1267,  1268,  1269,  1270,  1271,  1272,  1273,  1274,  1275,
    1276,  1277,  1278,  1279,  1280,  1281,  1282,  1286,  1291,  1297,
    1303,  1314,  1315,  1319,  1320,  1324,  1325,  1329,  1333,  1340,
    1340,  1340,  1346,  1346,  1346,  1355,  1389,  1392,  1395,  1398,
    1404,  1405,  1416,  1420,  1423,  1431,  1431,  1431,  1434,  1440,
    1443,  1447,  1451,  1458,  1465,  1471,  1475,  1479,  1482,  1485,
    1493,  1496,  1499,  1507,  1510,  1518,  1521,  1524,  1532,  1538,
    1539,  1540,  1544,  1545,  1549,  1550,  1554,  1559,  1567,  1573,
    1579,  1588,  1600,  1603,  1609,  1609,  1609,  1612,  1612,  1612,
    1617,  1617,  1617,  1625,  1625,  1625,  1631,  1641,  1652,  1665,
    1675,  1686,  1701,  1704,  1710,  1711,  1718,  1729,  1730,  1731,
    1735,  1736,  1737,  1738,  1739,  1743,  1748,  1756,  1757,  1758,
    1762,  1767,  1774,  1781,  1781,  1790,  1791,  1792,  1793,  1794,
    1795,  1796,  1800,  1801,  1802,  1803,  1804,  1805,  1806,  1807,
    1808,  1809,  1810,  1811,  1812,  1813,  1814,  1815,  1816,  1817,
    1818,  1822,  1823,  1824,  1825,  1830,  1831,  1832,  1833,  1834,
    1835,  1836,  1837,  1838,  1839,  1840,  1841,  1842,  1843,  1844,
    1845,  1846,  1851,  1858,  1870,  1875,  1885,  1889,  1896,  1899,
    1899,  1899,  1904,  1904,  1904,  1917,  1921,  1925,  1930,  1937,
    1942,  1949,  1949,  1949,  1956,  1960,  1970,  1979,  1988,  1992,
    1995,  2001,  2002,  2003,  2004,  2005,  2006,  2007,  2008,  2009,
    2010,  2011,  2012,  2013,  2014,  2015,  2016,  2017,  2018,  2019,
    2020,  2021,  2022,  2023,  2024,  2025,  2026,  2027,  2028,  2029,
    2030,  2031,  2032,  2033,  2034,  2035,  2036,  2042,  2043,  2044,
    2045,  2046,  2059,  2060,  2061,  2062,  2063,  2064,  2065,  2066,
    2067,  2068,  2069,  2070,  2073,  2076,  2077,  2080,  2080,  2080,
    2083,  2088,  2092,  2096,  2096,  2096,  2101,  2104,  2108,  2108,
    2108,  2113,  2116,  2117,  2118,  2119,  2120,  2121,  2122,  2123,
    2124,  2126,  2130,  2131,  2136,  2140,  2141,  2142,  2143,  2144,
    2145,  2146,  2150,  2154,  2158,  2162,  2166,  2170,  2174,  2178,
    2182,  2189,  2190,  2191,  2195,  2196,  2197,  2201,  2202,  2206,
    2207,  2208,  2212,  2213,  2217,  2228,  2231,  2231,  2250,  2249,
    2264,  2263,  2276,  2285,  2294,  2304,  2305,  2309,  2312,  2321,
    2322,  2326,  2329,  2332,  2348,  2357,  2358,  2362,  2365,  2368,
    2382,  2383,  2387,  2393,  2399,  2402,  2406,  2412,  2421,  2422,
    2423,  2427,  2428,  2432,  2439,  2444,  2453,  2459,  2470,  2473,
    2478,  2483,  2491,  2502,  2505,  2505,  2525,  2526,  2530,  2531,
    2532,  2536,  2539,  2539,  2558,  2561,  2564,  2579,  2598,  2599,
    2600,  2605,  2605,  2631,  2632,  2636,  2637,  2637,  2641,  2642,
    2643,  2647,  2657,  2662,  2657,  2674,  2679,  2674,  2694,  2695,
    2699,  2700,  2704,  2710,  2711,  2715,  2716,  2717,  2721,  2724,
    2730,  2735,  2730,  2749,  2756,  2761,  2770,  2776,  2787,  2788,
    2789,  2790,  2791,  2792,  2793,  2794,  2795,  2796,  2797,  2798,
    2799,  2800,  2801,  2802,  2803,  2804,  2805,  2806,  2807,  2808,
    2809,  2810,  2811,  2812,  2813,  2817,  2818,  2819,  2820,  2821,
    2822,  2823,  2824,  2828,  2839,  2843,  2850,  2862,  2869,  2878,
    2883,  2886,  2899,  2899,  2899,  2912,  2913,  2917,  2921,  2928,
    2932,  2939,  2940,  2941,  2942,  2943,  2958,  2964,  2964,  2964,
    2968,  2973,  2980,  2980,  2987,  2991,  2995,  3000,  3005,  3010,
    3015,  3019,  3023,  3028,  3032,  3036,  3041,  3041,  3041,  3047,
    3054,  3054,  3054,  3059,  3059,  3059,  3065,  3065,  3065,  3070,
    3074,  3074,  3074,  3079,  3079,  3079,  3088,  3092,  3092,  3092,
    3097,  3097,  3097,  3106,  3110,  3110,  3110,  3115,  3115,  3115,
    3124,  3124,  3124,  3130,  3130,  3130,  3139,  3142,  3153,  3169,
    3169,  3174,  3179,  3169,  3204,  3204,  3209,  3215,  3204,  3240,
    3240,  3245,  3250,  3240,  3280,  3281,  3282,  3283,  3284,  3288,
    3295,  3302,  3308,  3314,  3321,  3328,  3334,  3343,  3349,  3357,
    3362,  3369,  3374,  3381,  3386,  3392,  3393,  3397,  3398,  3403,
    3404,  3408,  3409,  3413,  3414,  3418,  3419,  3420,  3424,  3425,
    3426,  3430,  3431,  3435,  3441,  3448,  3456,  3463,  3471,  3480,
    3480,  3480,  3488,  3488,  3488,  3495,  3495,  3495,  3502,  3502,
    3502,  3513,  3516,  3522,  3536,  3542,  3548,  3554,  3554,  3554,
    3564,  3569,  3576,  3585,  3590,  3597,  3597,  3597,  3607,  3607,
    3607,  3617,  3617,  3617,  3627,  3636,  3636,  3636,  3656,  3663,
    3663,  3663,  3673,  3678,  3685,  3688,  3694,  3702,  3711,  3719,
    3739,  3764,  3765,  3769,  3770,  3775,  3778,  3781,  3784,  3787,
    3790
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
  "\"int8\"", "\"uint8\"", "\"int16\"", "\"uint16\"", "\"tuple\"",
  "\"variant\"", "\"generator\"", "\"yield\"", "\"sealed\"", "\"+=\"",
  "\"-=\"", "\"/=\"", "\"*=\"", "\"%=\"", "\"&=\"", "\"|=\"", "\"^=\"",
  "\"<<\"", "\">>\"", "\"++\"", "\"--\"", "\"<=\"", "\"<<=\"", "\">>=\"",
  "\">=\"", "\"==\"", "\"!=\"", "\"->\"", "\"<-\"", "\"??\"", "\"?.\"",
  "\"?[\"", "\"<|\"", "\" <|\"", "\"$ <|\"", "\"@ <|\"", "\"@@ <|\"",
  "\"|>\"", "\":=\"", "\"<<<\"", "\">>>\"", "\"<<<=\"", "\">>>=\"",
  "\"=>\"", "\"::\"", "\"&&\"", "\"||\"", "\"^^\"", "\"&&=\"", "\"||=\"",
  "\"^^=\"", "\"..\"", "\"$$\"", "\"$i\"", "\"$v\"", "\"$b\"", "\"$a\"",
  "\"$t\"", "\"$c\"", "\"$f\"", "\"...\"", "\"[[\"", "\"[{\"", "\"{{\"",
  "\"integer constant\"", "\"long integer constant\"",
  "\"unsigned integer constant\"", "\"unsigned long integer constant\"",
  "\"unsigned int8 constant\"", "\"floating point constant\"",
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
  "optional_public_or_private_module", "module_name", "module_declaration",
  "character_sequence", "string_constant", "string_builder_body",
  "string_builder", "reader_character_sequence", "expr_reader", "$@1",
  "options_declaration", "require_declaration", "keyword_or_name",
  "require_module_name", "require_module", "is_public_module",
  "expect_declaration", "expect_list", "expect_error", "expression_label",
  "expression_goto", "elif_or_static_elif", "expression_else",
  "if_or_static_if", "expression_else_one_liner", "$@2",
  "expression_if_one_liner", "expression_if_then_else", "$@3",
  "expression_for_loop", "$@4", "expression_unsafe",
  "expression_while_loop", "expression_with", "expression_with_alias",
  "$@5", "annotation_argument_value", "annotation_argument_value_list",
  "annotation_argument_name", "annotation_argument",
  "annotation_argument_list", "metadata_argument_list",
  "annotation_declaration_name", "annotation_declaration_basic",
  "annotation_declaration", "annotation_list", "optional_annotation_list",
  "optional_function_argument_list", "optional_function_type",
  "function_name", "global_function_declaration",
  "optional_public_or_private_function", "function_declaration_header",
  "function_declaration", "$@6", "expression_block", "expr_call_pipe",
  "expression_any", "expressions", "expr_keyword", "optional_expr_list",
  "optional_expr_list_in_braces", "optional_expr_map_tuple_list",
  "type_declaration_no_options_list", "expression_keyword", "$@7", "$@8",
  "$@9", "$@10", "expr_pipe", "name_in_namespace", "expression_delete",
  "new_type_declaration", "$@11", "$@12", "expr_new", "expression_break",
  "expression_continue", "expression_return_no_pipe", "expression_return",
  "expression_yield_no_pipe", "expression_yield", "expression_try_catch",
  "kwd_let_var_or_nothing", "kwd_let", "optional_in_scope",
  "tuple_expansion", "tuple_expansion_variable_declaration",
  "expression_let", "expr_cast", "$@13", "$@14", "$@15", "$@16", "$@17",
  "$@18", "expr_type_decl", "$@19", "$@20", "expr_type_info", "expr_list",
  "block_or_simple_block", "block_or_lambda", "capture_entry",
  "capture_list", "optional_capture_list", "expr_block", "expr_full_block",
  "expr_full_block_assumed_piped", "$@21", "expr_numeric_const",
  "expr_assign", "expr_assign_pipe_right", "expr_assign_pipe",
  "expr_named_call", "expr_method_call", "func_addr_name",
  "func_addr_expr", "$@22", "$@23", "$@24", "$@25", "expr_field", "$@26",
  "$@27", "expr_call", "expr", "$@28", "$@29", "$@30", "$@31", "$@32",
  "$@33", "expr_mtag", "optional_field_annotation", "optional_override",
  "optional_constant", "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "struct_variable_declaration_list", "$@34", "$@35", "$@36",
  "function_argument_declaration", "function_argument_list", "tuple_type",
  "tuple_type_list", "tuple_alias_type_list", "variant_type",
  "variant_type_list", "variant_alias_type_list", "copy_or_move",
  "variable_declaration", "copy_or_move_or_clone", "optional_ref",
  "let_variable_name_with_pos_list", "let_variable_declaration",
  "global_variable_declaration_list", "$@37", "optional_shared",
  "optional_public_or_private_variable", "global_let", "$@38", "enum_list",
  "optional_public_or_private_alias", "single_alias", "$@39", "alias_list",
  "alias_declaration", "$@40", "optional_public_or_private_enum",
  "enum_name", "enum_declaration", "$@41", "$@42", "$@43", "$@44",
  "optional_structure_parent", "optional_sealed", "structure_name",
  "class_or_struct", "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@45", "$@46", "variable_name_with_pos_list", "basic_type_declaration",
  "enum_basic_type_declaration", "structure_type_declaration",
  "auto_type_declaration", "bitfield_bits", "bitfield_alias_bits",
  "bitfield_type_declaration", "$@47", "$@48", "c_or_s", "table_type_pair",
  "dim_list", "type_declaration_no_options", "$@49", "$@50", "$@51",
  "$@52", "$@53", "$@54", "$@55", "$@56", "$@57", "$@58", "$@59", "$@60",
  "$@61", "$@62", "$@63", "$@64", "$@65", "$@66", "$@67", "$@68", "$@69",
  "$@70", "$@71", "$@72", "$@73", "$@74", "$@75", "type_declaration",
  "tuple_alias_declaration", "$@76", "$@77", "$@78", "$@79",
  "variant_alias_declaration", "$@80", "$@81", "$@82", "$@83",
  "bitfield_alias_declaration", "$@84", "$@85", "$@86", "$@87",
  "make_decl", "make_struct_fields", "make_variant_dim",
  "make_struct_single", "make_struct_dim", "make_struct_dim_list",
  "make_struct_dim_decl", "optional_make_struct_dim_decl",
  "optional_block", "optional_trailing_semicolon_cur_cur",
  "optional_trailing_semicolon_cur_sqr",
  "optional_trailing_semicolon_sqr_sqr", "optional_trailing_delim_sqr_sqr",
  "optional_trailing_delim_cur_sqr", "use_initializer", "make_struct_decl",
  "$@88", "$@89", "$@90", "$@91", "$@92", "$@93", "$@94", "$@95",
  "make_tuple", "make_map_tuple", "make_tuple_call", "$@96", "$@97",
  "make_dim", "make_dim_decl", "$@98", "$@99", "$@100", "$@101", "$@102",
  "$@103", "$@104", "$@105", "$@106", "$@107", "make_table",
  "expr_map_tuple_list", "make_table_decl", "array_comprehension_where",
  "optional_comma", "array_comprehension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1464)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-758)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1464,    46, -1464, -1464,    54,   -99,   160,   -52, -1464,   -17,
     636,   636,   636, -1464,   166,   291, -1464, -1464,   -12, -1464,
   -1464, -1464,   457, -1464,   133, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464,   152, -1464,   195,   179,   200,
   -1464, -1464, -1464, -1464,   160, -1464,    37, -1464,   636,   636,
   -1464, -1464,   133, -1464, -1464, -1464, -1464, -1464,   432,   316,
   -1464, -1464, -1464,   291,   291,   291,   455, -1464,   574,   134,
   -1464, -1464, -1464, -1464,   645,   681,   709, -1464,   733,    53,
      54,   520,   -99,   407,   522, -1464,   691,   691, -1464,   559,
     581,    60,   602,   734,   626,   631,   640, -1464,   648,   646,
   -1464, -1464,   -24,    54,   291,   291,   291,   291, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464,   661, -1464, -1464, -1464, -1464,
   -1464,   652, -1464, -1464, -1464, -1464, -1464,   741,   109, -1464,
   -1464, -1464, -1464,   784, -1464, -1464, -1464, -1464, -1464,   662,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,   678,
   -1464,    12, -1464,   206,   710,   574, 14811, -1464,   155,   766,
   -1464,   -71, -1464, -1464, -1464,   746, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464,   285, -1464,   687, -1464,   697,   708,   714,
   -1464, -1464, 13412, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,   849,   863,
   -1464,   696,   731, -1464,   680, -1464,   743, -1464,   737,    54,
      54,     3,   137, -1464, -1464, -1464,   109, -1464, 10066, -1464,
   -1464, -1464,   756,   763, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464,   767,   726, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,   918, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464,   772,   730, -1464, -1464,   182,   753, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,   765,   758,
     770, -1464,   -71,   141, -1464, -1464,    54,   738,   902,   621,
   -1464, -1464,   752,   759,   760,   739,   761,   768, -1464, -1464,
   -1464,   744, -1464, -1464, -1464, -1464, -1464,   769, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,   771,
   -1464, -1464, -1464,   773,   776, -1464, -1464, -1464, -1464,   777,
     778,   747,   166, -1464, -1464, -1464, -1464, -1464,   350,   782,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464,   781,   823, -1464,
     748, -1464,    92, -1464,    61, 10066, -1464,  2178,  -121, -1464,
     166, -1464, -1464, -1464,   137,   762, -1464,  9274,   795,   806,
   10066, -1464,   -25, -1464, -1464, -1464,  9274, -1464, -1464,   808,
   -1464,   405,   412,   429, -1464, -1464,  9274,   -79, -1464, -1464,
   -1464,    14, -1464, -1464, -1464,    42,  5432, -1464,   775,  9822,
     331,  9921,   507, -1464, -1464,  9274, -1464, -1464,   154,   -54,
   -1464,   782, -1464,   786,   787,  9274, -1464, -1464, -1464, -1464,
   -1464,   108,   -60,   789,    35,  3202, -1464, -1464,   731,   241,
    5634,   779,  9274,   809,   794,   796,   783, -1464,   810,   803,
     827,  5836,    94,   265,   804, -1464,   328,   805,   807,  3404,
    9274,  9274,   237,   237,   237,   790,   791,   793,   800,   801,
     802, -1464,  1594,  9637,  6040, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464,  6242,   814, -1464,  6446,   965, -1464,  9274,  9274,
    9274,  9274,  9274,  5028,  9274, -1464,   811, -1464, -1464,   828,
     836,  9274,  1008, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464,   264, -1464,    24,   838, -1464,   839,   841,   847,
   -1464,   851, -1464, -1464,   967, -1464, -1464, -1464, -1464,   824,
   -1464, -1464,   -50, -1464, -1464, -1464, -1464, -1464, -1464,  1721,
   -1464,   821, -1464, -1464, -1464, -1464, -1464, -1464,   404, -1464,
     856, -1464, -1464,    44, -1464, -1464,   829,   843,   846, -1464,
   10615, -1464,   998,  1819, -1464, -1464, -1464,  3808, 10066, 10066,
   10066, 10650, 10066, 10066,   831,   877, 10066,   696, 10066,   696,
   10066,   696, 10165,   878, 10760, -1464,  9274, -1464, -1464, -1464,
   -1464,   837, -1464, -1464, 12935,  9274, -1464,   350,   867, -1464,
     870,   -39, -1464, -1464,   456, -1464,   782,   871,   864,   456,
   -1464,   873, 10795,   844,  1013, -1464,   -47, -1464, -1464, -1464,
   13447,   154, -1464,   850, -1464, -1464,   166,   381, -1464,   872,
     874,   875, -1464,  9274,  3808, -1464,   883,   949, 10248,  1067,
   10066,  9274,  9274, 14188,  9274, 13447,   895, -1464, -1464,  9274,
   -1464, -1464,   896,   925, 14188,  9274, -1464, -1464,  9274, -1464,
   -1464,  9274, -1464, 10066,  3808, -1464, 10248,    50,    50,   876,
   -1464,   824, -1464, -1464, -1464,  9274,  9274,  9274,  9274,  9274,
    9274,   154,  2593,   154,  2796,   154, 13546, -1464,   742, -1464,
   13447, -1464,   732,   154, -1464,   901,   910,    50,    50,    80,
      50,    50,   154,  1077,   904, 14188,   904,   402, -1464, -1464,
   13447, -1464, -1464, -1464, -1464,  4010, -1464, -1464, -1464, -1464,
   -1464, -1464,   -41,    58,   237, -1464, 14590, 14685,  4212,  4212,
    4212,  4212,  4212,  4212,  4212,  4212,  9274,  9274, -1464, -1464,
    9274,  4212,  4212,  9274,  9274,  9274,   924,  4212,  9274,   555,
    9274,  9274,  9274,  9274,  9274,  9274,  4212,  4212,  9274,  9274,
    9274,  4212,  4212,  4212,  9274,  4212,  6648,  9274,  9274,  9274,
    9274,  9274,  9274,  9274,  9274,  9274,  9274, 14750,  9274, -1464,
    6850, -1464,  9274, -1464,  -121, -1464,   291,  1086,   -71, 10066,
   -1464,   926, -1464,  3808, -1464, 10360,    48,   119,   903,   454,
   -1464,   222,   482, -1464, -1464,   177,   672,   753,   673,   753,
     682,   753, -1464,   453, -1464,   511, -1464, 10066,   885,   904,
   -1464, -1464, 12970, -1464, -1464, 10066, -1464, -1464, 10066, -1464,
   -1464, -1464,  9274,   933, -1464,   934, -1464, 10066, -1464,  3808,
   10066, 10066, -1464,    25, 10066,  5230,  7052,   938,  9274, 10066,
   -1464, -1464, -1464, 10066,   904, -1464,   883,  9274,  9274,  9274,
    9274,  9274,  9274,  9274,  9274,  9274,  9274,  9274,  9274,  9274,
    9274,  9274,  9274,  9274,  9274,   731,   122,   899,   901, 14188,
   10905, -1464, -1464, 10066, 10066, 10940, 10066, -1464, -1464, 11050,
   10066,   904, 10066, 10066,   904, 10066,   131, -1464, 10248, -1464,
      58, 11085, 11195, 11230, 11340, 11375, 11485,    34,   237,   905,
     244,  2999,  4416,  7254, 13645,   927,     8,   286,   928,   347,
      47,  7456,     8,   410,    57,  9274,   936,  9274, -1464, -1464,
   10066, -1464, 10066, -1464,  9274,   364,    69,  9274,   937, -1464,
      77,   154,  9274,   907,   908,   913,   914,   477, -1464, -1464,
     677,  9274,   824,   190,  4620, -1464,   309,   927,   909,   952,
     952, -1464, -1464,   915,   186,   696, -1464,   935,   917, -1464,
   -1464,   940,   922, -1464, -1464,   237,   237,   237, -1464, -1464,
   10505, -1464, 10505, -1464, 10505, -1464, 10505, -1464, 10505, -1464,
   10505, -1464, 10505, -1464, 10505,   947,   947,   605, -1464, 10505,
   -1464, 10505,   605,  1587,  1587,   923, -1464, 10505,   486,   929,
   -1464, 13069,   167,   167,   821, 14188,   947,   947, -1464, 10505,
   -1464, 10505, 14432,  1954, 14403, -1464, 10505, -1464, 10505, -1464,
   10505, 14278, -1464, 10505, 14716, 13680,  1922, 14522,  1266,   605,
     605,   659,   659,   486,   486,   486,   572,  9274,   939,   941,
     580,  9274,  1130,   942, 13168, -1464,   318, 13770,   958,   219,
     755,  1065,   959,   388, -1464, -1464, 10470, -1464, -1464, -1464,
   -1464, 10066, -1464, -1464,   970, -1464, -1464,   946, -1464,   953,
   -1464,   954, -1464, 10165, -1464,   878,   569,   782, -1464, -1464,
   -1464,   782,   782, 11520, -1464,  1107,   -56, -1464, 10248,   978,
    1064,  9274,   683,   588,   326,   945,   948,   982, 11630,   452,
   11665,   690, 10066, 10066, 10066,  2098,   950, 14188, 14188, 14188,
   14188, 14188, 14188, 14188, 14188, 14188, 14188, 14188, 14188, 14188,
   14188, 14188, 14188, 14188, 14188, -1464,   943, 10066, -1464, -1464,
   -1464,  9274,  2513,  2715, -1464,  2918, -1464,  3121,   955,  3324,
    3526,   956,  3728,    58,   696, -1464, -1464, -1464, -1464, -1464,
     962,  9274, -1464,  9274,  9274,  9274,  4824, 12935,   147,  9274,
     606,   588,   286, -1464, -1464,   960, -1464,  9274,  9274, -1464,
     964, -1464,  9274,   588,   643,   966, -1464, -1464,  9274, 14188,
   -1464, -1464,   573,   611, 13805,  9274, -1464, -1464,  2391,  9274,
      78, -1464, -1464,  9274,  9274, 10066,   696,   731, -1464, -1464,
    9274, -1464,  9639,    58,   226, -1464,   963,   238,  9476, -1464,
   -1464, -1464,   254,   338,   186,   984,   991,   969,   992,   997,
   -1464,   269,   753, -1464,  9274, -1464,  9274, -1464, -1464, -1464,
    7658,  9274, -1464,   986,   971, -1464, -1464,  9274,   972, -1464,
   13203,  9274,  7860,   975, -1464, 13302, -1464,  8062, -1464, -1464,
   -1464, -1464,  1002, -1464, -1464,   215, -1464,    90, -1464,    58,
   -1464, -1464, -1464, -1464,   782, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,   977,
   10066, -1464,  1023,  9274, -1464, -1464,   355, -1464,   979, -1464,
   -1464, -1464,   612, -1464,  1025,   983, -1464, -1464,  3930,  4132,
    4209, -1464, -1464,  9274,  4336, 14188, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464,   686,   753,  8264,   647, 11775,
   14188, 14188,     8,   286, 14188,   985,   259,   927, -1464, -1464,
   14188,   928, -1464,   651,     8,   988, -1464, -1464, -1464, -1464,
     665, -1464, -1464, -1464,   667, -1464,   669,  9274, 11810, 11920,
    4413,   753, -1464, 13447, -1464,  1016,   696, -1464,   993,  4620,
    1033,   994,   546,   343, -1464, -1464,  1039, -1464, -1464,   186,
    1001,   158, 10066, 11955, 10066, 12065, -1464,   346, 12100, -1464,
    9274, 14313,  9274, -1464, 12210, -1464,   348,  9274, -1464, -1464,
   -1464,   349, -1464,  1176,    90, -1464, -1464,   755,  1003, -1464,
   -1464, -1464,  9274,   782, -1464, 14188,  1004,  1005, -1464, -1464,
   -1464,  9274,  1041,  1020,  9274, -1464, -1464, -1464, -1464,  1007,
    1009,  1011,  9274,  9274,  9274,  1015,  1151,  1017,  1018,  8466,
   -1464,   158, -1464,   383,  9274,   276,   286, -1464,  9274,  9274,
    9274,  9274,   643, -1464,  9274,  9274,  1019,  9274,  9274,   684,
   -1464, -1464, -1464,  1027,   677,  3606, -1464,   753, -1464,   289,
   -1464,   577, 10066,   -25, -1464,  1012, -1464, -1464,  8668, -1464,
   -1464,  4540, -1464,   699, -1464, -1464, -1464, 10066, 12245, 12355,
   -1464, -1464, 12390, -1464, -1464, -1464,  1176,   154,  1021,  1151,
    1151, 12500,  1034,  1024, 12535,  1026,  1029,  1030,  9274, -1464,
    9274,   605,   605,   605,  9274, -1464, -1464,  1151,   588, -1464,
   12645, -1464, -1464, 13895,  9274,  9274, -1464, 12680, 14188, 14188,
   13895, -1464,  1052,   605,  9274, -1464,  1052, 13895,  9274,   267,
   -1464, -1464,  8870,  9072, -1464, -1464, -1464, -1464, -1464, 14188,
     731,  1022, 10066,   -25,  1440,  9274, -1464,  9274, 14278, -1464,
   -1464,   705, -1464, -1464, -1464, 14811, -1464, -1464, -1464,   187,
     187, -1464,  9274,  9274, -1464,  1151,  1151,   588,  1037,  1042,
     904,   187,   927,  1044, -1464,  1185,  1028, 14188, 14188,   304,
    1075,  1081,  1057,  1082,  1051, 13895, -1464,   267,  9274,  9274,
   14188, -1464, -1464,  1440,  9274,  9274, 13934, 14313, -1464, -1464,
   -1464,  1083, 14811,   588,   927,  1078, -1464,  1053,  1054, 12790,
   12825,   187,   187,  1055, -1464, -1464,  1056,  1058, -1464,  9274,
    1059,  9274,  9274,  1061,  1094, -1464,  1062, -1464, -1464,  1066,
   -1464, 14188,  9274, 14024, 14059, -1464, -1464, -1464,   731,   392,
    1070, -1464, -1464, -1464, -1464, -1464,  1063,  1076, -1464, -1464,
   -1464, 14188, -1464, 14188, 14188, -1464, -1464, -1464, -1464, 14149,
   -1464, -1464, -1464, -1464,   588, -1464, -1464, -1464,   395, -1464
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   122,     1,   302,     0,     0,     0,   616,   303,     0,
     608,   608,   608,    16,     0,     0,    15,     3,     0,    10,
       9,     8,     0,     7,   596,     6,    11,     5,     4,    13,
      12,    14,    93,    94,    92,   101,   103,    38,    54,    51,
      52,    40,    41,    42,     0,    43,    49,    39,   608,   608,
      22,    21,   596,   610,   609,   779,   769,   774,     0,   270,
      36,   109,   110,     0,     0,     0,   111,   113,   120,     0,
     108,    17,   634,   633,   215,   618,   635,   597,   598,     0,
       0,     0,     0,    44,     0,    50,     0,     0,    47,     0,
       0,   608,     0,    18,     0,     0,     0,   272,     0,     0,
     119,   114,     0,     0,     0,     0,     0,     0,   123,   217,
     216,   219,   214,   620,   619,     0,   637,   636,   640,   600,
     599,   602,    99,   100,    97,    98,    96,     0,     0,    95,
     104,    55,    53,    49,    46,    45,   611,   613,   615,     0,
     617,    19,    20,    23,   780,   770,   775,   271,    34,    37,
     118,     0,   115,   116,   117,   121,     0,   621,     0,   630,
     593,   531,    24,    25,    29,     0,    88,    89,    86,    87,
      85,    84,    90,     0,    48,     0,   614,     0,     0,     0,
      35,   112,     0,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,     0,     0,
     129,   124,     0,   622,     0,   631,     0,   641,   594,     0,
       0,   533,     0,    26,    27,    28,     0,   102,     0,   781,
     771,   776,   183,   184,   181,   132,   133,   135,   134,   136,
     137,   138,   139,   165,   166,   163,   164,   156,   167,   168,
     157,   154,   155,   182,   176,     0,   180,   169,   170,   171,
     172,   143,   144,   145,   140,   141,   142,   153,     0,   159,
     160,   158,   151,   152,   147,   146,   148,   149,   150,   131,
     130,   175,     0,   161,   162,   531,   127,   247,   220,   604,
     675,   678,   681,   682,   676,   679,   677,   680,     0,   628,
     638,   601,   531,     0,   105,   107,     0,     0,   583,   581,
     603,    91,     0,     0,     0,     0,     0,     0,   648,   668,
     649,   684,   650,   654,   655,   656,   657,   674,   661,   662,
     663,   664,   665,   666,   667,   669,   670,   671,   672,   739,
     653,   660,   673,   746,   753,   651,   658,   652,   659,     0,
       0,     0,     0,   683,   701,   704,   702,   703,   766,   612,
     689,   561,   567,   185,   186,   179,   174,   187,   177,   173,
       0,   125,   301,   555,     0,     0,   218,     0,   623,   625,
       0,   632,   545,   642,     0,     0,   106,     0,     0,     0,
       0,   582,     0,   707,   730,   733,     0,   736,   726,     0,
     692,   740,   747,   754,   760,   763,     0,     0,   716,   721,
     715,     0,   729,   725,   718,     0,     0,   720,   705,     0,
     782,   772,   777,   188,   178,     0,   299,   300,     0,   531,
     126,   128,   249,     0,     0,     0,    64,    65,    77,   437,
     438,     0,     0,     0,     0,   287,   431,   285,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   286,     0,     0,
       0,     0,     0,     0,     0,   674,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   520,     0,     0,     0,   355,   357,   356,   358,   359,
     360,   361,     0,     0,    30,     0,   221,   226,     0,     0,
       0,     0,     0,     0,     0,   337,   338,   435,   434,     0,
       0,     0,     0,   242,   237,   234,   233,   235,   236,   269,
     248,   228,   514,   227,   432,     0,   505,    72,    73,    70,
     240,    71,   241,   243,   305,   232,   504,   503,   502,   122,
     508,   433,     0,   229,   507,   506,   478,   439,   479,   362,
     440,     0,   436,   784,   788,   785,   786,   787,     0,   605,
       0,   604,   629,   546,   595,   532,     0,     0,     0,   514,
       0,   585,   586,     0,   579,   580,   578,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   124,     0,   124,
       0,   124,     0,     0,     0,   712,   251,   723,   724,   717,
     719,     0,   722,   706,     0,     0,   768,   767,     0,   690,
       0,   270,   696,   695,     0,   562,   557,     0,     0,     0,
     568,     0,     0,     0,   643,   553,   572,   556,   829,   832,
       0,     0,   275,   279,   278,   284,     0,     0,   323,     0,
       0,     0,   865,     0,     0,   291,   288,     0,   332,     0,
       0,   255,     0,   273,     0,     0,     0,   314,   317,     0,
     246,   320,     0,     0,    58,     0,    79,   869,     0,   838,
     847,     0,   835,     0,     0,   296,   293,   467,   468,   338,
     350,   122,   268,   266,   267,     0,     0,     0,     0,     0,
       0,     0,   807,     0,     0,     0,   845,   872,     0,   259,
       0,   262,     0,     0,   874,   883,     0,   444,   443,   480,
     442,   441,     0,     0,   883,   332,   883,   339,   244,   245,
       0,    75,   353,   224,   512,     0,   231,   238,   239,   290,
     295,   304,     0,   347,     0,   230,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   469,   470,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   421,     0,   223,
       0,   606,     0,   624,   626,   639,     0,     0,   531,     0,
     584,     0,   588,     0,   592,   362,     0,     0,     0,   697,
     710,     0,     0,   685,   687,     0,     0,   127,     0,   127,
       0,   127,   559,     0,   565,     0,   686,     0,     0,   883,
     714,   699,     0,   691,   783,     0,   563,   773,     0,   569,
     778,   554,     0,     0,   571,     0,   570,     0,   573,     0,
       0,     0,    80,     0,     0,   821,     0,     0,     0,     0,
     855,   858,   861,     0,   883,   292,   289,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   883,   274,
       0,    81,    82,     0,     0,     0,     0,    56,    57,     0,
       0,   883,     0,     0,   883,     0,     0,   297,   294,   339,
     347,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     270,     0,     0,     0,   841,   799,   807,     0,   850,     0,
       0,     0,   807,     0,     0,     0,     0,     0,   810,   877,
       0,   250,     0,    33,     0,    31,     0,   884,     0,   247,
       0,     0,   884,     0,     0,     0,     0,   409,   406,   408,
      61,     0,   122,     0,     0,   424,     0,   798,     0,     0,
       0,   313,   312,     0,     0,   124,   265,     0,     0,   491,
     490,     0,     0,   492,   496,     0,     0,     0,   384,   393,
     372,   394,   373,   396,   375,   395,   374,   397,   376,   387,
     366,   388,   367,   389,   368,   445,   446,   458,   398,   377,
     399,   378,   459,   456,   457,     0,   386,   364,   485,     0,
     476,     0,   509,   510,   511,   365,   447,   448,   400,   379,
     401,   380,   463,   464,   465,   390,   369,   391,   370,   392,
     371,   466,   385,   363,     0,     0,   461,   462,   460,   454,
     455,   450,   449,   451,   452,   453,     0,     0,     0,   415,
       0,     0,     0,     0,     0,   429,     0,     0,     0,     0,
     539,   542,     0,     0,   587,   590,   362,   591,   708,   731,
     734,     0,   737,   727,     0,   693,   741,     0,   748,     0,
     755,     0,   761,     0,   764,     0,     0,   257,   711,   252,
     700,   558,   564,     0,   645,   646,   574,   577,   576,     0,
       0,     0,     0,   822,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   333,   372,   373,
     375,   374,   376,   366,   367,   368,   377,   378,   364,   379,
     380,   369,   370,   371,   363,   298,     0,     0,   878,   256,
     481,     0,     0,     0,   482,     0,   513,     0,     0,     0,
       0,     0,     0,   347,   124,   515,   516,   517,   518,   519,
       0,     0,   808,     0,     0,     0,     0,   332,   807,     0,
       0,     0,     0,   816,   817,     0,   824,     0,     0,   814,
       0,   853,     0,     0,     0,     0,   812,   854,     0,   844,
     809,   873,     0,     0,     0,     0,   875,   876,     0,     0,
       0,   852,   471,     0,     0,     0,   124,     0,    59,    60,
       0,    74,    66,   347,     0,   425,     0,     0,     0,   428,
     426,   306,     0,     0,     0,     0,     0,     0,     0,     0,
     345,     0,   127,   487,     0,   493,     0,   383,   381,   382,
       0,     0,   474,     0,     0,   497,   501,     0,     0,   477,
       0,     0,     0,     0,   416,     0,   422,     0,   472,   430,
     607,   627,   123,   540,   541,   542,   543,   534,   547,   347,
     589,   709,   732,   735,   698,   738,   728,   688,   694,   742,
     744,   749,   751,   756,   758,   762,   560,   765,   566,     0,
       0,   644,     0,     0,   830,   833,     0,   276,     0,   281,
     282,   280,     0,   326,     0,     0,   329,   324,     0,     0,
       0,   866,   864,   255,     0,    83,   315,   318,   321,   870,
     868,   839,   848,   846,   836,     0,   127,     0,     0,     0,
     790,   789,   807,     0,   842,     0,     0,   800,   823,   815,
     843,   851,   813,     0,   807,     0,   819,   820,   827,   811,
       0,   260,   263,    32,     0,   222,     0,     0,     0,     0,
       0,   127,    62,     0,    67,     0,   124,   427,     0,     0,
       0,     0,   581,     0,   342,   343,     0,   341,   340,     0,
       0,     0,     0,     0,     0,     0,   404,     0,     0,   498,
       0,   486,     0,   475,     0,   417,     0,     0,   473,   423,
     419,     0,   552,   537,   534,   535,   536,   539,     0,   745,
     752,   759,   251,   258,   647,   575,     0,     0,    78,   277,
     283,     0,     0,     0,     0,   325,   856,   859,   862,     0,
       0,     0,     0,     0,     0,     0,   821,     0,     0,     0,
     225,     0,   521,     0,     0,     0,     0,   825,     0,     0,
       0,     0,     0,   818,     0,     0,   253,     0,     0,     0,
     407,   530,   410,     0,    61,     0,    76,   127,   402,     0,
     307,   581,     0,     0,   349,     0,   346,   348,     0,   334,
     352,     0,   529,     0,   527,   405,   524,     0,     0,     0,
     523,   418,     0,   420,   538,   548,   537,     0,     0,   821,
     821,     0,     0,     0,     0,     0,     0,     0,   251,   879,
     255,   316,   319,   322,     0,   822,   840,   821,     0,   483,
       0,   351,   522,   881,     0,     0,   826,     0,   792,   791,
     881,   828,   881,   261,   251,   264,   881,   881,     0,     0,
     413,    63,   287,     0,    68,    72,    73,    70,    71,    69,
       0,     0,     0,     0,     0,     0,   344,     0,   335,   488,
     494,     0,   528,   526,   525,     0,   550,   544,   713,   806,
     806,   327,     0,     0,   330,   821,   821,     0,     0,     0,
     883,   806,   797,     0,   484,     0,     0,   794,   793,     0,
       0,     0,   883,     0,     0,   881,   411,     0,     0,     0,
     293,   354,   403,     0,     0,     0,     0,   336,   489,   495,
     499,     0,     0,     0,   803,   883,   805,     0,     0,     0,
       0,   806,   806,     0,   867,   880,     0,     0,   837,     0,
       0,     0,     0,     0,     0,   884,     0,   889,   885,     0,
     414,   294,     0,     0,     0,   311,   500,   549,     0,     0,
     884,   804,   831,   834,   328,   331,     0,     0,   863,   871,
     849,   882,   887,   796,   795,   888,   890,   254,   886,     0,
     310,   309,   551,   801,     0,   857,   860,   308,     0,   802
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1464, -1464, -1464, -1464, -1464, -1464,   564,  1200, -1464, -1464,
   -1464,  1279, -1464, -1464, -1464,   623,  1240, -1464,  1156, -1464,
   -1464,  1209, -1464, -1464, -1464,  -182, -1464, -1464, -1464,  -181,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,  1069,
   -1464, -1464,   -55,   -46, -1464, -1464, -1464,   524,   500,  -507,
    -556,  -791, -1464, -1464, -1464, -1463, -1464, -1464,  -210,  -361,
   -1464,   360, -1464, -1357, -1464, -1279,  -656,   -88, -1464, -1464,
   -1464, -1464,  -411,   -14, -1464, -1464, -1464, -1464, -1464,  -178,
    -159,  -156, -1464,  -128, -1464, -1464, -1464,  1348, -1464,   384,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464,  -440,  -100,   551,   -36,   121,  -891,  -436,
   -1464,  -510, -1464, -1464,  -366,  1133, -1464, -1464, -1464, -1405,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,   560,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464,  -132,   -57,  -148,
     -58,    85, -1464, -1464, -1464, -1464, -1464,   932, -1464,  -577,
   -1464, -1464,  -576, -1464, -1464,  -620,  -145,  -570, -1327, -1464,
    -348, -1464, -1464,  1312, -1464, -1464, -1464,   813,   919,    67,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
    -608,  -189, -1464,   930, -1464, -1464, -1464, -1464, -1464, -1464,
    -401, -1464, -1464,  -376, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464,  -215, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464,   931,  -684,  -222,
    -813,  -672, -1464, -1464, -1077,  -908, -1464, -1464, -1464, -1155,
     -96, -1380, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,
   -1464,   180,  -480, -1464, -1464, -1464,   675, -1464, -1464, -1464,
   -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464, -1464,   880,
   -1464, -1103,  -707, -1464
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    16,   143,    52,    17,   165,   171,   702,   507,
     149,   508,    99,    19,    20,    45,    46,    47,    88,    21,
      39,    40,   509,   510,  1220,  1221,   511,  1375,  1475,   512,
     513,   961,   514,   631,   515,   516,   517,   518,  1151,   172,
     173,    35,    36,    37,   221,    66,    67,    68,    69,    22,
     286,   376,   211,    23,   111,   212,   112,   156,   680,   988,
     520,   377,   521,   828,  1545,   887,  1096,   569,   940,  1465,
     942,  1466,   523,   524,   525,   633,   854,  1429,   526,   527,
     528,   529,   530,   531,   532,   533,   428,   534,   732,  1232,
     971,   535,   536,   893,  1442,   894,  1443,   896,  1444,   537,
     859,  1435,   538,   714,  1490,   539,  1240,  1241,   975,   682,
     540,   789,   962,   541,   647,   989,   543,   544,   545,   959,
     546,  1215,  1549,  1216,  1607,   547,  1062,  1409,   548,   715,
    1392,  1618,  1394,  1619,  1497,  1656,   550,   372,  1417,  1505,
    1275,  1277,  1072,   563,   798,  1575,  1622,   373,   374,   614,
     823,   421,   619,   825,   422,  1175,   625,   577,   392,   309,
     310,   218,   302,    78,   121,    25,   161,   378,    89,    90,
     175,    91,    26,    49,   115,   158,    27,   289,   560,   561,
    1068,   381,   216,   217,    76,   118,   383,    28,   159,   300,
     626,   551,   298,   355,   356,   815,   420,   357,   585,  1288,
    1300,   808,   418,   358,   578,  1281,   827,   583,  1286,   579,
    1282,   580,  1283,   582,  1285,   586,  1289,   587,  1419,   588,
    1291,   589,  1420,   590,  1293,   591,  1421,   592,  1295,   593,
    1297,   616,    29,    95,   178,   361,   617,    30,    96,   179,
     362,   621,    31,    94,   177,   360,   610,   552,  1624,  1593,
     968,   926,  1625,  1626,  1627,   927,   939,  1197,  1191,  1186,
    1358,  1116,   553,   850,  1426,   851,  1427,   905,  1448,   902,
    1446,   928,   704,   554,   903,  1447,   929,   555,  1122,  1515,
    1123,  1516,  1124,  1517,   863,  1439,   900,  1445,   698,   888,
     556,  1596,   948,   557
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      60,    70,   288,   803,   697,   646,   849,   953,   925,   954,
     925,   542,   723,   359,   573,   822,   519,   824,  1182,  1164,
     615,   620,   932,   853,  1194,   130,  1087,  1348,  1089,   222,
    1091,   817,   733,   819,   645,   821,   564,   683,   684,   354,
     639,   967,  1115,   607,  1440,  1111,     2,   597,   558,    70,
      70,    70,  -122,     3,  1171,  1483,   797,   151,   675,    84,
     559,   918,   973,    38,   716,  1508,  1526,  1192,   122,   123,
     844,   736,   737,    32,    33,   599,     4,  1198,     5,   844,
       6,   734,    58,   917,   519,   930,     7,   934,   219,  1205,
      70,    70,    70,    70,    85,   946,     8,  1209,  1367,   426,
     370,   574,     9,    98,   950,   219,   692,   694,   519,    59,
     307,   575,  1621,   433,   434,   595,    92,    53,   969,   104,
     105,   106,  1099,    54,   166,   167,    10,    48,   308,  1579,
    1580,   735,   846,   444,   596,   419,  1415,   640,   641,   449,
     845,   846,   427,   847,  1606,   220,   848,  1591,    11,    12,
    1226,   835,    50,   636,  1563,   408,   829,  1126,   139,  1658,
     431,  1588,   220,   576,   408,   304,   804,   748,   749,    71,
     384,   967,   970,   303,   910,   756,   463,   464,   759,   760,
     761,  1149,   409,   410,   305,   762,   354,  1602,  1457,  1181,
     150,   409,   410,   724,  1158,    77,  1416,  1161,    51,    80,
     918,   354,  1650,   864,   866,  1631,  1632,   598,   759,   760,
     466,   467,   845,   836,   353,   124,   519,   974,   839,   306,
     125,   845,   126,    34,   795,   127,   181,    13,   901,   642,
     354,   904,   354,   865,   845,   600,    86,   725,   649,   419,
     138,  1589,   429,  1078,   845,   411,    14,    87,   643,   412,
      58,   386,   666,   601,   411,   796,   845,    15,   412,   602,
     787,   788,  1413,   907,   845,   845,   128,   482,   483,   484,
    1343,   168,  1335,   287,   886,   430,   169,    59,   170,   967,
    1227,   127,  1276,   519,  1202,   966,  1203,   495,   307,   522,
     787,   788,   756,   354,   354,   759,   760,   906,   976,  1488,
    1308,  1536,   632,   612,   919,   623,   308,   665,    58,   613,
     419,   413,  1235,   519,  1079,   414,  1224,  1146,   415,   503,
     413,   107,  1236,   624,   414,    61,  1163,   415,    80,    41,
      42,    43,  1376,   416,   213,    59,   370,   287,   407,   417,
      79,   219,   416,  1210,   919,   214,   108,   919,   417,   104,
    1066,   106,   844,   385,    62,  1237,  1224,   522,  1084,  1224,
      44,   353,  1174,   806,   807,   809,   562,   811,   812,    81,
     844,   816,  1085,   818,  1238,   820,   353,   787,   788,  1239,
    1174,   522,    80,   408,   972,   844,    98,    82,  1418,   354,
     354,   354,  1075,   354,   354,  1459,   371,   354,   220,   354,
    1623,   354,   844,   354,  1225,   353,   107,   353,  1081,    58,
     409,   410,  1534,   419,   846,  1114,   287,  1082,   955,  1242,
     852,   408,  1093,  1073,  1095,  1180,    63,   353,   637,   967,
     844,  1272,   846,    58,  1456,   650,    59,  1600,  1107,  1601,
    1641,  1380,   519,  1603,  1604,   891,  1462,   846,   409,   410,
    1378,  1391,   505,   679,   651,  1223,  1389,  1201,    98,   667,
      59,   354,    72,    73,   846,    74,  1381,  1206,   353,   353,
    1183,  1184,   226,   411,  1109,  1110,  1180,   412,   668,   505,
     679,  1390,  1172,   716,   354,  1147,  1303,  1125,   519,   522,
     941,   716,   846,    75,   925,    64,  1228,  1347,  1185,   227,
     608,  1561,  1649,  1628,    65,   867,  1342,   736,   737,   925,
     960,   411,   609,   867,  1637,   412,  1296,  1152,  1153,  1298,
    1155,  1354,   670,  1229,  1157,  1380,  1159,  1160,  1188,  1162,
    1389,  1189,  1269,   867,   287,   867,   867,   223,   224,   413,
    1309,   671,   867,   414,    58,  1451,   415,   980,   984,  1247,
    1248,  1249,  1382,   955,  1666,  1667,   522,  1484,   956,  1190,
    1495,   416,  1501,  1503,   353,   353,   353,   417,   353,   353,
     867,    59,   353,  1024,   353,   857,   353,   413,   353,  1180,
    1473,   414,  1180,  1279,   415,   791,   522,   100,   101,   102,
    1195,  1188,   792,  1196,   858,  -743,   957,  1532,  1063,   416,
    -743,    97,  -750,   748,   749,   417,  1683,  -750,  1336,  1689,
     354,   756,  1097,   758,   759,   760,   761,    87,  -743,  -757,
    1101,   762,   856,  1102,  -757,  -750,   736,   737,   152,   153,
     154,   155,  1106,  1314,   612,   612,   353,   612,   354,  1112,
     613,   613,  -757,   613,  1121,   419,   354,  1315,  1092,   354,
     522,   522,   522,   522,   522,   522,   522,   522,   354,   353,
    1371,   354,   354,   522,   522,   354,  1071,  -412,   103,   522,
     354,  1306,  -412,   419,   354,  1145,   618,  1083,   522,   522,
     736,   737,   131,   522,   522,   522,  1560,   522,   612,  1217,
    -412,   133,   612,    53,   613,  1479,   787,   788,   613,    54,
    1218,  1219,   109,   958,   354,   354,  1094,   354,   110,   134,
     135,   354,  1019,   354,   354,   522,   354,   104,   105,   106,
     746,   747,   748,   749,  1020,  1097,  1460,  1097,   136,  1258,
     756,  1338,   758,   759,   760,   761,  1482,  1263,   113,   391,
     762,  1259,   764,   765,   114,   919,  1318,  1319,  1320,  1264,
     612,   354,  1353,   354,   612,   290,   613,  1224,  1360,   291,
     613,   522,   137,  1345,  1299,  1364,   116,  1562,  1361,  1366,
     391,  1324,   117,   292,   293,  1346,   748,   749,   294,   295,
     296,   297,    70,   140,   756,   353,   758,   759,   760,   761,
     119,   141,   612,   612,   762,   144,   120,   142,   613,   613,
     145,   782,   783,   784,   785,   786,  1362,  1431,   389,   146,
    1397,   390,  1273,   353,   391,   787,   788,   147,  1274,   148,
    1477,   353,  1406,  1355,   353,  1450,  1356,  1411,  1454,  1357,
     157,   160,  1461,   353,   867,  1535,   353,   353,   867,  1370,
     353,    85,   542,   176,  1592,   353,  1464,   519,  1467,   353,
    1468,   180,   867,   104,   867,  1256,   867,   784,   785,   786,
      41,    42,    43,   419,   419,  1548,  1284,  1086,  1088,   787,
     788,   867,   215,   419,   419,   228,   229,  1090,  1307,   353,
     353,   419,   353,  1636,   283,  1317,   353,   230,   353,   353,
     419,   353,   354,   231,  1570,  1646,   419,  1453,   284,  1449,
    1620,   505,   679,  1592,   354,   162,   163,   943,   944,   285,
     287,  1432,   299,  1565,   162,   163,   164,   301,  1661,   223,
     224,   225,   936,   937,   938,   363,   353,  1469,   353,    55,
      56,    57,   364,   354,   354,   354,   365,   549,   366,  1659,
     367,   368,   369,   375,   379,   388,   393,   570,   380,   382,
     423,   387,   396,   394,   395,   397,   581,   399,   354,   424,
     406,   425,   398,   400,   571,   401,   594,   402,   736,   737,
     403,   404,   405,   419,   565,   572,   604,   584,   656,  1642,
     628,   629,   829,   638,  1542,   622,   605,  1546,   657,   662,
     658,   660,   654,  1614,  1615,   630,   659,   661,   669,   672,
    1688,   673,   706,   685,   686,   648,   687,  1372,   701,   718,
     653,   408,   655,   688,   689,   690,  1491,   719,   721,   726,
     727,   664,   728,   681,   681,   681,   354,   717,   729,   676,
     677,   678,   730,   731,   790,    15,   793,   799,   409,   410,
     699,   801,   665,  1652,   696,   813,   814,   618,   833,   830,
     834,   837,   700,   840,   838,   696,   843,   842,   707,   708,
     709,   710,   711,   855,   748,   749,   860,   353,   861,   862,
     867,   720,   756,   722,   758,   759,   760,   761,   829,   353,
     734,   885,   762,   892,  1590,  1423,   897,   898,   947,   949,
     951,   952,   909,  1015,  1070,  1074,  1428,   408,  1080,  1098,
     722,   411,  1104,  1105,   829,   412,  1564,  1119,   353,   353,
     353,   354,   646,  1148,  1180,  1187,  1200,  1207,  1173,  1211,
     522,  1231,  1212,  1230,   409,   410,  1213,  1214,  1234,  1243,
    1244,  1266,  1276,   353,  1245,  1246,  1250,   805,  1271,  1287,
    1278,  1290,  1251,   782,   783,   784,   785,   786,  1292,  1294,
    1302,  1312,  1261,  1384,  1262,  1267,  1323,   787,   788,  1310,
    1385,  1387,  1311,  1474,  1322,   832,  1388,   413,   866,  1330,
    1333,   414,  1349,  1304,   415,  1337,  1352,  1377,  1359,  1493,
    1399,  1489,  1386,  1412,  1400,  1402,  1613,   411,  1407,   416,
    1422,   412,  1424,  1430,  1433,   417,  1434,  1476,  1458,   722,
    1463,   353,  1480,   354,   648,   354,  1481,  1478,  1485,  1504,
    1512,   696,   889,  1487,   890,  1513,  1449,  1509,  1510,   895,
    1518,  1525,  1550,  1519,  1520,   899,  1566,   722,  1524,  1582,
    1527,  1528,  1544,  1595,   908,  1578,  1612,  1583,  1639,  1585,
    1640,  1489,  1586,  1587,  1645,   911,   912,   913,   914,   915,
     916,  1634,   924,   413,   924,  1643,  1635,   414,  1638,  1305,
     415,  1644,  1647,  1648,  1657,  1660,   945,  1662,  1663,  1668,
    1669,  1672,  1670,  1675,  1676,   416,  1677,  1685,  1678,   129,
      18,   417,  1571,  1684,    83,   681,   353,   736,   737,   174,
    1686,   132,  1551,   354,  1554,   311,  1069,  1555,   990,   992,
     994,   996,   998,  1000,  1002,  1004,  1005,  1006,   354,  1208,
    1007,  1009,  1011,  1012,  1013,  1014,  1556,  1017,  1018,  1557,
    1021,  1022,  1023,  1025,  1026,  1027,  1029,  1031,  1032,  1033,
    1034,  1036,  1038,  1040,  1041,  1043,  1045,  1046,  1047,  1048,
    1049,  1050,  1051,  1052,  1053,  1054,  1055,  1558,  1064,    24,
    1611,  1531,  1067,  1486,  1233,  1383,   722,  1506,  1576,  1507,
    1414,   627,  1577,  1076,    93,  1633,  1541,     0,  1351,   933,
       0,   634,   635,   354,   794,   705,     0,     0,   353,     0,
     353,   746,   747,   748,   749,   750,     0,     0,   753,   754,
     755,   756,     0,   758,   759,   760,   761,     0,     0,     0,
       0,   762,  1103,   764,   765,     0,     0,     0,     0,  1108,
       0,     0,     0,     0,     0,     0,  1118,     0,  1120,     0,
       0,     0,     0,     0,     0,     0,     0,  1127,  1128,  1129,
    1130,  1131,  1132,  1133,  1134,  1135,  1136,  1137,  1138,  1139,
    1140,  1141,  1142,  1143,  1144,     0,     0,     0,  1682,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   722,
     780,   781,   782,   783,   784,   785,   786,     0,   353,   681,
       0,     0,     0,   408,     0,     0,   787,   788,     0,     0,
       0,   707,  1177,   353,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1199,     0,   696,     0,     0,
     409,   410,     0,     0,  1204,     0,     0,   696,     0,     0,
       0,     0,  1127,     0,     0,     0,     0,     0,     0,     0,
       0,  1222,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   958,   681,   681,   681,     0,
       0,   722,     0,   722,     0,   722,     0,   722,   353,   722,
       0,   722,     0,   722,     0,   722,     0,     0,     0,     0,
     722,     0,   722,   411,     0,     0,   574,   412,   722,     0,
       0,     0,     0,     0,     0,     0,   575,     0,     0,     0,
     722,     0,   722,     0,     0,     0,     0,   722,     0,   722,
       0,   722,     0,   958,   722,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   691,   736,   737,
       0,     0,     0,   312,     0,     0,     0,  1260,     0,   313,
       0,  1265,     0,     0,     0,   314,     0,   722,   576,   413,
       0,     0,     0,   414,     0,   315,   415,     0,     0,     0,
       0,     0,     0,   316,     0,     0,     0,     0,     0,     0,
       0,   416,     0,     0,     0,     0,     0,   417,   317,   722,
       0,     0,     0,     0,     0,   318,   319,   320,   321,   322,
     323,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,     0,     0,
       0,     0,   746,   747,   748,   749,   750,     0,     0,   753,
       0,  1325,   756,     0,   758,   759,   760,   761,     0,     0,
       0,     0,   762,     0,   764,   765,     0,     0,     0,     0,
       0,   -69,     0,  1339,  1340,  1341,    58,     0,     0,  1344,
       0,     0,   736,   737,     0,     0,     0,  1350,   924,   351,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    59,     0,     0,     0,     0,   549,     0,
       0,     0,     0,  1368,  1369,     0,     0,     0,     0,     0,
    1373,   780,   781,   782,   783,   784,   785,   786,  1127,     0,
       0,     0,     0,     0,     0,     0,     0,   787,   788,     0,
       0,     0,     0,     0,  1393,     0,  1395,     0,     0,   352,
       0,  1398,     0,     0,     0,     0,     0,  1401,     0,     0,
       0,  1404,     0,     0,     0,     0,     0,     0,   738,   739,
     740,   741,   742,   743,   744,   745,   746,   747,   748,   749,
     750,   751,   752,   753,   754,   755,   756,   757,   758,   759,
     760,   761,   408,     0,     0,     0,   762,   763,   764,   765,
     766,   767,     0,  1425,   768,   769,   770,   771,   772,   773,
     774,     0,   991,   993,   995,   997,   999,  1001,  1003,   409,
     410,     0,     0,   696,  1008,  1010,   722,     0,     0,     0,
    1016,     0,     0,     0,     0,     0,     0,     0,     0,  1028,
    1030,     0,     0,     0,  1035,  1037,  1039,     0,  1042,   775,
     776,     0,   777,   778,   779,   780,   781,   782,   783,   784,
     785,   786,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   787,   788,     0,     0,     0,   505,   679,     0,     0,
       0,     0,   411,   736,   737,   574,   412,     0,     0,     0,
       0,     0,     0,     0,     0,   575,     0,     0,     0,     0,
    1498,     0,  1499,     0,     0,     0,     0,  1502,     0,     0,
       0,     0,     0,     0,     0,   736,   737,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1511,     0,     0,  1514,     0,     0,     0,     0,     0,
     802,     0,  1521,  1522,  1523,     0,     0,   576,   413,  1530,
       0,     0,   414,     0,  1533,   415,     0,     0,  1537,  1538,
    1539,  1540,     0,     0,   696,  1543,     0,   696,  1547,     0,
     416,     0,     0,     0,     0,  1559,   417,   746,   747,   748,
     749,   750,     0,     0,   753,   754,   755,   756,  1568,   758,
     759,   760,   761,     0,     0,     0,     0,   762,     0,   764,
     765,     0,     0,     0,     0,     0,     0,     0,     0,   746,
     747,   748,   749,   750,     0,     0,   753,   754,   755,   756,
     696,   758,   759,   760,   761,     0,     0,     0,     0,   762,
       0,   764,   765,     0,  1597,  1598,     0,   768,     0,   770,
       0,     0,     0,     0,     0,     0,     0,     0,  1605,     0,
       0,     0,     0,  1610,   778,   779,   780,   781,   782,   783,
     784,   785,   786,     0,     0,  1616,     0,  1617,     0,     0,
       0,   408,   787,   788,     0,     0,     0,     0,     0,     0,
       0,     0,  1629,  1630,     0,   777,   778,   779,   780,   781,
     782,   783,   784,   785,   786,     0,     0,     0,   409,   410,
       0,     0,     0,     0,   787,   788,     0,     0,     0,  1651,
       0,     0,     0,     0,  1653,  1654,     0,     0,     0,   432,
       0,     0,     0,   433,   434,     3,     0,   435,   436,   437,
       0,   438,     0,   439,   440,   441,   442,   443,     0,  1671,
       0,  1673,  1674,   444,   445,   446,   447,   448,     0,   449,
       0,     0,  1679,     0,     0,     0,   450,   451,     0,     0,
     452,   411,   453,   454,     0,   412,   455,     0,     8,   456,
     457,     0,   458,   459,     0,     0,   460,   461,     0,     0,
       0,     0,     0,   462,     0,     0,   463,   464,     0,   318,
     319,   320,     0,   322,   323,   324,   325,   326,   465,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
       0,   340,   341,   342,     0,     0,   345,   346,   347,   348,
     466,   467,   468,   469,     0,     0,     0,   413,     0,     0,
       0,   414,     0,  1321,   415,   470,   471,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   416,
     472,   473,   474,     0,     0,   417,     0,     0,     0,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   475,   476,
     477,   478,   479,     0,   480,     0,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,    59,   492,   493,
     494,     0,     0,     0,     0,     0,     0,   495,   496,   497,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   498,   499,   500,     0,    14,     0,
       0,   501,   502,     0,     0,     0,     0,     0,     0,   503,
       0,   504,   432,   505,   506,     0,   433,   434,     3,     0,
     435,   436,   437,     0,   438,     0,   439,   440,   441,   442,
     443,     0,     0,     0,     0,     0,   444,   445,   446,   447,
     448,     0,   449,     0,     0,     0,     0,     0,     0,   450,
     451,     0,     0,   452,     0,   453,   454,     0,     0,   455,
       0,     8,   456,   457,     0,   458,   459,     0,     0,   460,
     461,     0,     0,     0,     0,     0,   462,     0,     0,   463,
     464,     0,   318,   319,   320,     0,   322,   323,   324,   325,
     326,   465,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,     0,   340,   341,   342,     0,     0,   345,
     346,   347,   348,   466,   467,   468,   469,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   470,   471,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   472,   473,   474,     0,     0,     0,     0,
       0,     0,     0,    58,     0,     0,     0,     0,     0,     0,
       0,   475,   476,   477,   478,   479,   408,   480,     0,   481,
     482,   483,   484,   485,   486,   487,   488,   489,   490,   491,
      59,   492,   493,   494,     0,     0,     0,     0,     0,     0,
     495,  1365,   497,   409,   410,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,   499,   500,
       0,    14,     0,     0,   501,   502,     0,     0,   433,   434,
       0,     0,   503,     0,   504,     0,   505,   506,   439,   440,
     441,   442,   443,     0,     0,     0,     0,     0,   444,     0,
     446,     0,     0,     0,   449,     0,   408,     0,     0,     0,
       0,     0,   451,     0,     0,     0,   411,     0,   454,     0,
     412,   455,     0,     0,   456,     0,   918,     0,   459,     0,
       0,     0,     0,   409,   410,     0,     0,     0,   566,     0,
       0,   463,   464,     0,   318,   319,   320,     0,   322,   323,
     324,   325,   326,   465,   328,   329,   330,   331,   332,   333,
     334,   335,   336,   337,   338,     0,   340,   341,   342,     0,
       0,   345,   346,   347,   348,   466,   467,   567,     0,     0,
       0,     0,   413,     0,     0,     0,   414,     0,  1326,   415,
     470,   471,     0,     0,     0,     0,   411,     0,     0,     0,
     412,     0,     0,     0,   416,     0,     0,     0,     0,     0,
     417,     0,     0,     0,     0,    58,     0,     0,     0,     0,
       0,     0,     0,   475,   476,   477,   478,   479,   408,   480,
     919,   481,   482,   483,   484,   485,   486,   487,   488,   489,
     490,   491,   920,   568,   493,   494,     0,     0,     0,     0,
       0,     0,   495,     0,     0,   409,   410,     0,     0,     0,
       0,     0,   413,     0,     0,     0,   414,     0,     0,   921,
     499,   500,     0,    14,     0,     0,   501,   502,     0,     0,
       0,   433,   434,     0,   922,     0,   923,     0,   505,   506,
     417,   439,   440,   441,   442,   443,     0,     0,     0,     0,
       0,   444,     0,   446,     0,     0,     0,   449,     0,   408,
       0,     0,     0,     0,     0,   451,     0,     0,   411,     0,
       0,   454,   412,     0,   455,     0,     0,   456,     0,     0,
       0,   459,     0,     0,     0,     0,   409,   410,     0,     0,
       0,   566,     0,     0,   463,   464,     0,   318,   319,   320,
       0,   322,   323,   324,   325,   326,   465,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,     0,   340,
     341,   342,     0,     0,   345,   346,   347,   348,   466,   467,
     567,     0,     0,     0,   413,     0,     0,     0,   414,     0,
    1327,   415,     0,   470,   471,     0,     0,     0,     0,   411,
       0,     0,     0,   412,     0,     0,   416,     0,     0,     0,
       0,     0,   417,     0,     0,     0,     0,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   475,   476,   477,   478,
     479,   408,   480,   919,   481,   482,   483,   484,   485,   486,
     487,   488,   489,   490,   491,   920,   568,   493,   494,     0,
       0,     0,     0,     0,     0,   495,     0,     0,   409,   410,
       0,     0,     0,     0,     0,   413,     0,     0,     0,   414,
       0,     0,   921,   499,   500,     0,    14,     0,     0,   501,
     502,     0,     0,     0,   433,   434,     0,   922,     0,   931,
       0,   505,   506,   417,   439,   440,   441,   442,   443,     0,
       0,     0,     0,     0,   444,     0,   446,     0,     0,     0,
     449,     0,   599,     0,     0,     0,     0,     0,   451,     0,
       0,   411,     0,     0,   454,   412,     0,   455,     0,     0,
     456,     0,     0,     0,   459,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   566,     0,     0,   463,   464,     0,
     318,   319,   320,     0,   322,   323,   324,   325,   326,   465,
     328,   329,   330,   331,   332,   333,   334,   335,   336,   337,
     338,     0,   340,   341,   342,     0,     0,   345,   346,   347,
     348,   466,   467,   567,     0,     0,     0,   413,     0,     0,
       0,   414,     0,  1328,   415,     0,   470,   471,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   416,
       0,     0,     0,     0,     0,   417,     0,     0,     0,     0,
       0,    58,     0,     0,     0,     0,     0,     0,     0,   475,
     476,   477,   478,   479,   408,   480,     0,   481,   482,   483,
     484,   485,   486,   487,   488,   489,   490,   491,    59,   568,
     493,   494,     0,     0,     0,     0,     0,     0,   495,     0,
       0,   409,   410,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   600,     0,     0,   498,   499,   500,     0,    14,
       0,     0,   501,   502,     0,     0,     0,   433,   434,     0,
    1176,     0,   504,     0,   505,   506,   602,   439,   440,   441,
     442,   443,     0,     0,     0,     0,     0,   444,     0,   446,
       0,     0,     0,   449,     0,     0,     0,     0,     0,     0,
       0,   451,     0,     0,   411,     0,     0,   454,   412,     0,
     455,     0,     0,   456,     0,     0,     0,   459,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   566,     0,     0,
     463,   464,     0,   318,   319,   320,     0,   322,   323,   324,
     325,   326,   465,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,     0,   340,   341,   342,     0,     0,
     345,   346,   347,   348,   466,   467,   468,     0,     0,     0,
     413,     0,     0,     0,   414,     0,  1329,   415,     0,   470,
     471,     0,     0,     0,     0,     0,     0,     0,   644,     0,
       0,     0,   416,     0,   472,   473,   474,     0,   417,     0,
       0,     0,     0,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   475,   476,   477,   478,   479,   408,   480,     0,
     481,   482,   483,   484,   485,   486,   487,   488,   489,   490,
     491,    59,   568,   493,   494,     0,     0,     0,     0,     0,
       0,   495,     0,     0,   409,   410,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   498,   499,
     500,     0,    14,     0,     0,   501,   502,     0,     0,   433,
     434,     0,     0,   503,     0,   504,     0,   505,   506,   439,
     440,   441,   442,   443,     0,     0,     0,     0,     0,   444,
       0,   446,     0,     0,     0,   449,     0,     0,     0,     0,
       0,     0,     0,   451,     0,     0,     0,   411,     0,   454,
       0,   412,   455,     0,     0,   456,     0,     0,     0,   459,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   566,
       0,     0,   463,   464,     0,   318,   319,   320,     0,   322,
     323,   324,   325,   326,   465,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,     0,   340,   341,   342,
       0,     0,   345,   346,   347,   348,   466,   467,   468,     0,
       0,     0,     0,   413,     0,     0,     0,   414,     0,  1331,
     415,   470,   471,     0,     0,     0,     0,     0,     0,     0,
     674,     0,     0,     0,     0,   416,   472,   473,   474,     0,
       0,   417,     0,     0,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,   475,   476,   477,   478,   479,   408,
     480,     0,   481,   482,   483,   484,   485,   486,   487,   488,
     489,   490,   491,    59,   568,   493,   494,     0,     0,     0,
       0,     0,     0,   495,     0,     0,   409,   410,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     498,   499,   500,     0,    14,     0,     0,   501,   502,     0,
       0,   433,   434,     0,     0,   503,     0,   504,     0,   505,
     506,   439,   440,   441,   442,   443,     0,     0,     0,     0,
       0,   444,  1552,   446,   447,     0,     0,   449,     0,     0,
       0,     0,     0,     0,     0,   451,     0,     0,     0,   411,
       0,   454,     0,   412,   455,     0,     0,   456,   457,     0,
       0,   459,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   566,     0,     0,   463,   464,     0,   318,   319,   320,
       0,   322,   323,   324,   325,   326,   465,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,     0,   340,
     341,   342,     0,     0,   345,   346,   347,   348,   466,   467,
     567,  1553,     0,     0,     0,   413,     0,     0,     0,   414,
       0,  1332,   415,   470,   471,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   416,     0,     0,
       0,     0,     0,   417,     0,     0,     0,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   475,   476,   477,   478,
     479,   408,   480,     0,   481,   482,   483,   484,   485,   486,
     487,   488,   489,   490,   491,    59,   568,   493,   494,     0,
       0,     0,     0,     0,     0,   495,     0,     0,   409,   410,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   498,   499,   500,     0,    14,     0,     0,   501,
     502,     0,     0,   433,   434,     0,     0,   503,     0,   504,
       0,   505,   506,   439,   440,   441,   442,   443,     0,     0,
       0,     0,     0,   444,     0,   446,     0,     0,     0,   449,
       0,     0,     0,     0,     0,     0,     0,   451,     0,     0,
       0,   411,     0,   454,     0,   412,   455,     0,     0,   456,
       0,     0,     0,   459,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   566,     0,     0,   463,   464,     0,   318,
     319,   320,     0,   322,   323,   324,   325,   326,   465,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
       0,   340,   341,   342,     0,     0,   345,   346,   347,   348,
     466,   467,   468,     0,     0,     0,     0,   413,     0,     0,
       0,   414,     0,  1334,   415,   470,   471,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   416,
     472,   473,   474,     0,     0,   417,     0,     0,     0,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   475,   476,
     477,   478,   479,   408,   480,     0,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,    59,   568,   493,
     494,     0,     0,     0,     0,     0,     0,   495,     0,     0,
     409,   410,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   498,   499,   500,     0,    14,     0,
       0,   501,   502,     0,     0,   433,   434,     0,     0,   503,
       0,   504,     0,   505,   506,   439,   440,   441,   442,   443,
       0,     0,     0,     0,     0,   444,     0,   446,     0,     0,
       0,   449,     0,     0,     0,     0,     0,     0,     0,   451,
       0,     0,     0,   411,     0,   454,     0,   412,   455,     0,
       0,   456,     0,     0,     0,   459,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   566,     0,     0,   463,   464,
     963,   318,   319,   320,     0,   322,   323,   324,   325,   326,
     465,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,     0,   340,   341,   342,     0,     0,   345,   346,
     347,   348,   466,   467,   567,     0,     0,     0,     0,   413,
       0,     0,     0,   414,     0,  1436,   415,   470,   471,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   416,     0,     0,     0,     0,     0,   417,     0,     0,
       0,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     475,   476,   477,   478,   479,   408,   480,   919,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   490,   491,   920,
     568,   493,   494,     0,     0,     0,     0,     0,     0,   495,
       0,     0,   409,   410,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   498,   499,   500,     0,
      14,     0,     0,   501,   502,     0,     0,   433,   434,     0,
       0,   964,     0,   504,   965,   505,   506,   439,   440,   441,
     442,   443,     0,     0,     0,     0,     0,   444,     0,   446,
       0,     0,   408,   449,     0,     0,     0,     0,     0,     0,
       0,   451,     0,     0,     0,   411,     0,   454,     0,   412,
     455,     0,     0,   456,     0,     0,     0,   459,     0,   409,
     410,     0,     0,     0,     0,     0,     0,   566,     0,     0,
     463,   464,     0,   318,   319,   320,     0,   322,   323,   324,
     325,   326,   465,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,     0,   340,   341,   342,     0,     0,
     345,   346,   347,   348,   466,   467,   468,     0,     0,     0,
       0,   413,     0,     0,     0,   414,     0,  1437,   415,   470,
     471,     0,   411,     0,     0,     0,   412,     0,     0,     0,
       0,     0,     0,   416,   985,   986,   987,     0,     0,   417,
       0,     0,     0,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   475,   476,   477,   478,   479,     0,   480,   408,
     481,   482,   483,   484,   485,   486,   487,   488,   489,   490,
     491,    59,   568,   493,   494,     0,     0,     0,     0,     0,
       0,   495,     0,     0,     0,     0,   409,   410,   413,     0,
       0,     0,   414,     0,  1438,   415,     0,     0,   498,   499,
     500,     0,    14,     0,     0,   501,   502,     0,     0,     0,
     416,   433,   434,   503,     0,   504,   417,   505,   506,   712,
       0,   439,   440,   441,   442,   443,     0,     0,     0,     0,
       0,   444,     0,   446,     0,     0,   408,   449,     0,     0,
       0,     0,     0,     0,     0,   451,     0,     0,     0,   411,
       0,   454,     0,   412,   455,   713,     0,   456,     0,     0,
       0,   459,     0,   409,   410,     0,     0,     0,     0,     0,
       0,   566,     0,     0,   463,   464,     0,   318,   319,   320,
       0,   322,   323,   324,   325,   326,   465,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,     0,   340,
     341,   342,     0,     0,   345,   346,   347,   348,   466,   467,
     567,     0,     0,     0,     0,   413,     0,     0,     0,   414,
       0,  1441,   415,   470,   471,     0,   411,     0,     0,     0,
     412,     0,     0,     0,     0,     0,     0,   416,     0,     0,
       0,     0,     0,   417,     0,     0,     0,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   475,   476,   477,   478,
     479,     0,   480,   408,   481,   482,   483,   484,   485,   486,
     487,   488,   489,   490,   491,    59,   568,   493,   494,     0,
       0,     0,     0,     0,     0,   495,     0,     0,     0,     0,
     409,   410,   413,     0,     0,     0,   414,     0,  1472,   415,
       0,     0,   498,   499,   500,     0,    14,     0,     0,   501,
     502,     0,     0,     0,   416,   433,   434,   503,   603,   504,
     417,   505,   506,   712,     0,   439,   440,   441,   442,   443,
       0,     0,     0,     0,     0,   444,     0,   446,     0,     0,
       0,   449,     0,     0,     0,     0,     0,     0,     0,   451,
       0,     0,     0,   411,     0,   454,     0,   412,   455,   713,
       0,   456,     0,     0,     0,   459,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   566,     0,     0,   463,   464,
       0,   318,   319,   320,     0,   322,   323,   324,   325,   326,
     465,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,     0,   340,   341,   342,     0,     0,   345,   346,
     347,   348,   466,   467,   567,     0,     0,     0,     0,   413,
       0,     0,     0,   414,     0,  1569,   415,   470,   471,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   416,     0,     0,     0,     0,     0,   417,     0,     0,
       0,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     475,   476,   477,   478,   479,     0,   480,   919,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   490,   491,   920,
     568,   493,   494,     0,     0,     0,     0,     0,     0,   495,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   498,   499,   500,     0,
      14,     0,     0,   501,   502,     0,     0,     0,     0,   433,
     434,   503,     0,   504,     0,   505,   506,   712,     0,   439,
     440,   441,   442,   443,     0,     0,     0,     0,     0,   444,
       0,   446,     0,     0,     0,   449,     0,     0,     0,     0,
       0,     0,     0,   451,     0,     0,     0,     0,     0,   454,
       0,     0,   455,   713,     0,   456,     0,     0,     0,   459,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   566,
       0,     0,   463,   464,     0,   318,   319,   320,     0,   322,
     323,   324,   325,   326,   465,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,     0,   340,   341,   342,
       0,     0,   345,   346,   347,   348,   466,   467,   567,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   470,   471,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,   475,   476,   477,   478,   479,     0,
     480,     0,   481,   482,   483,   484,   485,   486,   487,   488,
     489,   490,   491,    59,   568,   493,   494,     0,     0,     0,
       0,     0,     0,   495,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     498,   499,   500,     0,    14,     0,     0,   501,   502,     0,
       0,     0,     0,   433,   434,   503,   830,   504,     0,   505,
     506,   712,     0,   439,   440,   441,   442,   443,     0,     0,
       0,     0,     0,   444,     0,   446,     0,     0,     0,   449,
       0,     0,     0,     0,     0,     0,     0,   451,     0,     0,
       0,     0,     0,   454,     0,     0,   455,   713,     0,   456,
       0,     0,     0,   459,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   566,     0,     0,   463,   464,     0,   318,
     319,   320,     0,   322,   323,   324,   325,   326,   465,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
       0,   340,   341,   342,     0,     0,   345,   346,   347,   348,
     466,   467,   567,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   470,   471,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   475,   476,
     477,   478,   479,     0,   480,     0,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,    59,   568,   493,
     494,     0,     0,     0,     0,     0,     0,   495,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   498,   499,   500,     0,    14,     0,
       0,   501,   502,     0,     0,   433,   434,     0,     0,   503,
       0,   504,     0,   505,   506,   439,   440,   441,   442,   443,
       0,     0,     0,     0,     0,   444,     0,   446,     0,     0,
       0,   449,     0,     0,     0,     0,     0,     0,     0,   451,
       0,     0,     0,     0,     0,   454,     0,     0,   455,     0,
       0,   456,     0,     0,     0,   459,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   566,     0,     0,   463,   464,
    1113,   318,   319,   320,     0,   322,   323,   324,   325,   326,
     465,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,     0,   340,   341,   342,     0,     0,   345,   346,
     347,   348,   466,   467,   567,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   470,   471,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     475,   476,   477,   478,   479,     0,   480,   919,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   490,   491,   920,
     568,   493,   494,     0,     0,     0,     0,     0,     0,   495,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   498,   499,   500,     0,
      14,     0,     0,   501,   502,     0,     0,   433,   434,     0,
       0,   503,     0,   504,     0,   505,   506,   439,   440,   441,
     442,   443,     0,     0,     0,     0,     0,   444,     0,   446,
       0,     0,     0,   449,     0,     0,     0,     0,     0,     0,
       0,   451,     0,     0,     0,     0,     0,   454,     0,     0,
     455,     0,     0,   456,     0,     0,     0,   459,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   566,     0,     0,
     463,   464,     0,   318,   319,   320,     0,   322,   323,   324,
     325,   326,   465,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,     0,   340,   341,   342,     0,     0,
     345,   346,   347,   348,   466,   467,   567,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   470,
     471,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   475,   476,   477,   478,   479,     0,   480,     0,
     481,   482,   483,   484,   485,   486,   487,   488,   489,   490,
     491,    59,   568,   493,   494,     0,     0,     0,     0,     0,
       0,   495,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   498,   499,
     500,     0,    14,     0,     0,   501,   502,     0,     0,   433,
     434,     0,     0,   503,   603,   504,     0,   505,   506,   439,
     440,   441,   442,   443,     0,     0,     0,     0,     0,   444,
       0,   446,     0,     0,     0,   449,     0,     0,     0,     0,
       0,     0,     0,   451,     0,     0,     0,     0,     0,   454,
       0,     0,   455,     0,     0,   456,     0,     0,     0,   459,
       0,     0,     0,     0,     0,   652,     0,     0,     0,   566,
       0,     0,   463,   464,     0,   318,   319,   320,     0,   322,
     323,   324,   325,   326,   465,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,     0,   340,   341,   342,
       0,     0,   345,   346,   347,   348,   466,   467,   567,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   470,   471,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,   475,   476,   477,   478,   479,     0,
     480,     0,   481,   482,   483,   484,   485,   486,   487,   488,
     489,   490,   491,    59,   568,   493,   494,     0,     0,     0,
       0,     0,     0,   495,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     498,   499,   500,     0,    14,     0,     0,   501,   502,     0,
       0,   433,   434,     0,     0,   503,     0,   504,     0,   505,
     506,   439,   440,   441,   442,   443,     0,     0,     0,     0,
       0,   444,     0,   446,     0,     0,     0,   449,     0,     0,
       0,     0,     0,     0,     0,   451,     0,     0,     0,     0,
       0,   454,     0,     0,   455,     0,     0,   456,     0,     0,
       0,   459,     0,     0,   663,     0,     0,     0,     0,     0,
       0,   566,     0,     0,   463,   464,     0,   318,   319,   320,
       0,   322,   323,   324,   325,   326,   465,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,     0,   340,
     341,   342,     0,     0,   345,   346,   347,   348,   466,   467,
     567,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   470,   471,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   475,   476,   477,   478,
     479,     0,   480,     0,   481,   482,   483,   484,   485,   486,
     487,   488,   489,   490,   491,    59,   568,   493,   494,     0,
       0,     0,     0,     0,     0,   495,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   498,   499,   500,     0,    14,     0,     0,   501,
     502,     0,     0,     0,     0,   433,   434,   503,     0,   504,
       0,   505,   506,   695,     0,   439,   440,   441,   442,   443,
       0,     0,     0,     0,     0,   444,     0,   446,     0,     0,
       0,   449,     0,     0,     0,     0,     0,     0,     0,   451,
       0,     0,     0,     0,     0,   454,     0,     0,   455,     0,
       0,   456,     0,     0,     0,   459,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   566,     0,     0,   463,   464,
       0,   318,   319,   320,     0,   322,   323,   324,   325,   326,
     465,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,     0,   340,   341,   342,     0,     0,   345,   346,
     347,   348,   466,   467,   567,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   470,   471,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     475,   476,   477,   478,   479,     0,   480,     0,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   490,   491,    59,
     568,   493,   494,     0,     0,     0,     0,     0,     0,   495,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   498,   499,   500,     0,
      14,     0,     0,   501,   502,     0,     0,   433,   434,     0,
       0,   503,     0,   504,     0,   505,   506,   439,   440,   441,
     442,   443,     0,     0,     0,     0,     0,   444,     0,   446,
       0,     0,     0,   449,     0,     0,     0,     0,     0,     0,
       0,   451,     0,     0,     0,     0,     0,   454,     0,     0,
     455,     0,     0,   456,     0,     0,     0,   459,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   566,     0,     0,
     463,   464,     0,   318,   319,   320,     0,   322,   323,   324,
     325,   326,   465,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,     0,   340,   341,   342,     0,     0,
     345,   346,   347,   348,   466,   467,   567,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   470,
     471,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   475,   476,   477,   478,   479,     0,   480,     0,
     481,   482,   483,   484,   485,   486,   487,   488,   489,   490,
     491,    59,   568,   493,   494,     0,     0,     0,     0,     0,
       0,   495,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   699,     0,   498,   499,
     500,     0,    14,     0,     0,   501,   502,     0,     0,     0,
       0,   433,   434,   503,     0,   504,     0,   505,   506,   703,
       0,   439,   440,   441,   442,   443,     0,     0,     0,     0,
       0,   444,     0,   446,     0,     0,     0,   449,     0,     0,
       0,     0,     0,     0,     0,   451,     0,     0,     0,     0,
       0,   454,     0,     0,   455,     0,     0,   456,     0,     0,
       0,   459,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   566,     0,     0,   463,   464,     0,   318,   319,   320,
       0,   322,   323,   324,   325,   326,   465,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,     0,   340,
     341,   342,     0,     0,   345,   346,   347,   348,   466,   467,
     567,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   470,   471,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   475,   476,   477,   478,
     479,     0,   480,     0,   481,   482,   483,   484,   485,   486,
     487,   488,   489,   490,   491,    59,   568,   493,   494,     0,
       0,     0,     0,     0,     0,   495,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   498,   499,   500,     0,    14,     0,     0,   501,
     502,     0,     0,   433,   434,     0,     0,   503,     0,   504,
       0,   505,   506,   439,   440,   441,   442,   443,     0,     0,
    1044,     0,     0,   444,     0,   446,     0,     0,     0,   449,
       0,     0,     0,     0,     0,     0,     0,   451,     0,     0,
       0,     0,     0,   454,     0,     0,   455,     0,     0,   456,
       0,     0,     0,   459,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   566,     0,     0,   463,   464,     0,   318,
     319,   320,     0,   322,   323,   324,   325,   326,   465,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
       0,   340,   341,   342,     0,     0,   345,   346,   347,   348,
     466,   467,   567,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   470,   471,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   475,   476,
     477,   478,   479,     0,   480,     0,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,    59,   568,   493,
     494,     0,     0,     0,     0,     0,     0,   495,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   498,   499,   500,     0,    14,     0,
       0,   501,   502,     0,     0,   433,   434,     0,     0,   503,
       0,   504,     0,   505,   506,   439,   440,   441,   442,   443,
       0,     0,     0,     0,     0,   444,     0,   446,     0,     0,
       0,   449,     0,     0,     0,     0,     0,     0,     0,   451,
       0,     0,     0,     0,     0,   454,     0,     0,   455,     0,
       0,   456,     0,     0,     0,   459,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   566,     0,     0,   463,   464,
       0,   318,   319,   320,     0,   322,   323,   324,   325,   326,
     465,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,     0,   340,   341,   342,     0,     0,   345,   346,
     347,   348,   466,   467,   567,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   470,   471,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     475,   476,   477,   478,   479,     0,   480,     0,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   490,   491,    59,
     568,   493,   494,     0,     0,     0,     0,     0,     0,   495,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   498,   499,   500,     0,
      14,     0,     0,   501,   502,     0,     0,   433,   434,     0,
       0,   503,     0,   504,  1065,   505,   506,   439,   440,   441,
     442,   443,     0,     0,     0,     0,     0,   444,     0,   446,
       0,     0,     0,   449,     0,     0,     0,     0,     0,     0,
       0,   451,     0,     0,     0,     0,     0,   454,     0,     0,
     455,     0,     0,   456,     0,     0,     0,   459,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   566,     0,     0,
     463,   464,     0,   318,   319,   320,     0,   322,   323,   324,
     325,   326,   465,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,     0,   340,   341,   342,     0,     0,
     345,   346,   347,   348,   466,   467,   567,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   470,
     471,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   475,   476,   477,   478,   479,     0,   480,     0,
     481,   482,   483,   484,   485,   486,   487,   488,   489,   490,
     491,    59,   568,   493,   494,     0,     0,     0,     0,     0,
       0,   495,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1117,     0,   498,   499,
     500,     0,    14,     0,     0,   501,   502,     0,     0,   433,
     434,     0,     0,   503,     0,   504,     0,   505,   506,   439,
     440,   441,   442,   443,     0,     0,     0,     0,     0,   444,
       0,   446,     0,     0,     0,   449,     0,     0,     0,     0,
       0,     0,     0,   451,     0,     0,     0,     0,     0,   454,
       0,     0,   455,     0,     0,   456,     0,     0,     0,   459,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   566,
       0,     0,   463,   464,     0,   318,   319,   320,     0,   322,
     323,   324,   325,   326,   465,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,     0,   340,   341,   342,
       0,     0,   345,   346,   347,   348,   466,   467,   567,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   470,   471,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,   475,   476,   477,   478,   479,     0,
     480,     0,   481,   482,   483,   484,   485,   486,   487,   488,
     489,   490,   491,    59,   568,   493,   494,     0,     0,     0,
       0,     0,     0,   495,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     498,   499,   500,     0,    14,     0,     0,   501,   502,     0,
       0,   433,   434,     0,     0,   503,     0,   504,  1178,   505,
     506,   439,   440,   441,   442,   443,     0,     0,     0,     0,
       0,   444,     0,   446,     0,     0,     0,   449,     0,     0,
       0,     0,     0,     0,     0,   451,     0,     0,     0,     0,
       0,   454,     0,     0,   455,     0,     0,   456,     0,     0,
       0,   459,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   566,     0,     0,   463,   464,     0,   318,   319,   320,
       0,   322,   323,   324,   325,   326,   465,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,     0,   340,
     341,   342,     0,     0,   345,   346,   347,   348,   466,   467,
     567,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   470,   471,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   475,   476,   477,   478,
     479,     0,   480,     0,   481,   482,   483,   484,   485,   486,
     487,   488,   489,   490,   491,    59,   568,   493,   494,     0,
       0,     0,     0,     0,     0,   495,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   498,   499,   500,     0,    14,     0,     0,   501,
     502,     0,     0,   433,   434,     0,     0,   503,     0,   504,
    1193,   505,   506,   439,   440,   441,   442,   443,     0,     0,
       0,     0,     0,   444,     0,   446,     0,     0,     0,   449,
       0,     0,     0,     0,     0,     0,     0,   451,     0,     0,
       0,     0,     0,   454,     0,     0,   455,     0,     0,   456,
       0,     0,     0,   459,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   566,     0,     0,   463,   464,     0,   318,
     319,   320,     0,   322,   323,   324,   325,   326,   465,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
       0,   340,   341,   342,     0,     0,   345,   346,   347,   348,
     466,   467,   567,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   470,   471,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   475,   476,
     477,   478,   479,     0,   480,     0,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,    59,   568,   493,
     494,     0,     0,     0,     0,     0,     0,   495,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   498,   499,   500,     0,    14,     0,
       0,   501,   502,     0,     0,   433,   434,     0,     0,   503,
       0,   504,  1396,   505,   506,   439,   440,   441,   442,   443,
       0,     0,     0,     0,     0,   444,     0,   446,     0,     0,
       0,   449,     0,     0,     0,     0,     0,     0,     0,   451,
       0,     0,     0,     0,     0,   454,     0,     0,   455,     0,
       0,   456,     0,     0,     0,   459,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   566,     0,     0,   463,   464,
       0,   318,   319,   320,     0,   322,   323,   324,   325,   326,
     465,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,     0,   340,   341,   342,     0,     0,   345,   346,
     347,   348,   466,   467,   567,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   470,   471,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     475,   476,   477,   478,   479,     0,   480,     0,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   490,   491,    59,
     568,   493,   494,     0,     0,     0,     0,     0,     0,   495,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   498,   499,   500,     0,
      14,     0,     0,   501,   502,     0,     0,   433,   434,     0,
       0,   503,     0,   504,  1405,   505,   506,   439,   440,   441,
     442,   443,     0,     0,     0,     0,     0,   444,     0,   446,
       0,     0,     0,   449,     0,     0,     0,     0,     0,     0,
       0,   451,     0,     0,     0,     0,     0,   454,     0,     0,
     455,     0,     0,   456,     0,     0,     0,   459,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   566,     0,     0,
     463,   464,     0,   318,   319,   320,     0,   322,   323,   324,
     325,   326,   465,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,     0,   340,   341,   342,     0,     0,
     345,   346,   347,   348,   466,   467,   567,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   470,
     471,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   475,   476,   477,   478,   479,     0,   480,     0,
     481,   482,   483,   484,   485,   486,   487,   488,   489,   490,
     491,    59,   568,   493,   494,     0,     0,     0,     0,     0,
       0,   495,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   498,   499,
     500,     0,    14,     0,     0,   501,   502,     0,     0,   433,
     434,     0,     0,   503,     0,   504,  1410,   505,   506,   439,
     440,   441,   442,   443,     0,     0,     0,     0,     0,   444,
       0,   446,     0,     0,     0,   449,     0,     0,     0,     0,
       0,     0,     0,   451,     0,     0,     0,     0,     0,   454,
       0,     0,   455,     0,     0,   456,     0,     0,     0,   459,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   566,
       0,     0,   463,   464,     0,   318,   319,   320,     0,   322,
     323,   324,   325,   326,   465,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,     0,   340,   341,   342,
       0,     0,   345,   346,   347,   348,   466,   467,   567,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   470,   471,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,   475,   476,   477,   478,   479,     0,
     480,     0,   481,   482,   483,   484,   485,   486,   487,   488,
     489,   490,   491,    59,   568,   493,   494,     0,     0,     0,
       0,     0,     0,   495,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     498,   499,   500,     0,    14,     0,     0,   501,   502,     0,
       0,   433,   434,     0,     0,   503,     0,   504,  1452,   505,
     506,   439,   440,   441,   442,   443,     0,     0,     0,     0,
       0,   444,     0,   446,     0,     0,     0,   449,     0,     0,
       0,     0,     0,     0,     0,   451,     0,     0,     0,     0,
       0,   454,     0,     0,   455,     0,     0,   456,     0,     0,
       0,   459,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   566,     0,     0,   463,   464,     0,   318,   319,   320,
       0,   322,   323,   324,   325,   326,   465,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,     0,   340,
     341,   342,     0,     0,   345,   346,   347,   348,   466,   467,
     567,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   470,   471,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   475,   476,   477,   478,
     479,     0,   480,     0,   481,   482,   483,   484,   485,   486,
     487,   488,   489,   490,   491,    59,   568,   493,   494,     0,
       0,     0,     0,     0,     0,   495,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   498,   499,   500,     0,    14,     0,     0,   501,
     502,     0,     0,   433,   434,     0,     0,   503,     0,   504,
    1529,   505,   506,   439,   440,   441,   442,   443,     0,     0,
       0,     0,     0,   444,     0,   446,     0,     0,     0,   449,
       0,     0,     0,     0,     0,     0,     0,   451,     0,     0,
       0,     0,     0,   454,     0,     0,   455,     0,     0,   456,
       0,     0,     0,   459,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   566,     0,     0,   463,   464,     0,   318,
     319,   320,     0,   322,   323,   324,   325,   326,   465,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
       0,   340,   341,   342,     0,     0,   345,   346,   347,   348,
     466,   467,   567,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   470,   471,     0,     0,     0,
       0,     0,     0,     0,  1567,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      58,     0,     0,     0,     0,     0,     0,     0,   475,   476,
     477,   478,   479,     0,   480,     0,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,    59,   568,   493,
     494,     0,     0,     0,     0,     0,     0,   495,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   498,   499,   500,     0,    14,     0,
       0,   501,   502,     0,     0,   433,   434,     0,     0,   503,
       0,   504,     0,   505,   506,   439,   440,   441,   442,   443,
       0,     0,     0,     0,     0,   444,     0,   446,     0,     0,
       0,   449,     0,     0,     0,     0,     0,     0,     0,   451,
       0,     0,     0,     0,     0,   454,     0,     0,   455,     0,
       0,   456,     0,     0,     0,   459,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   566,     0,     0,   463,   464,
       0,   318,   319,   320,     0,   322,   323,   324,   325,   326,
     465,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,     0,   340,   341,   342,     0,     0,   345,   346,
     347,   348,   466,   467,   567,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   470,   471,     0,
       0,     0,     0,     0,     0,     0,  1608,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    58,     0,     0,     0,     0,     0,     0,     0,
     475,   476,   477,   478,   479,     0,   480,     0,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   490,   491,    59,
     568,   493,   494,     0,     0,     0,     0,     0,     0,   495,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   498,   499,   500,     0,
      14,     0,     0,   501,   502,     0,     0,   433,   434,     0,
       0,   503,     0,   504,     0,   505,   506,   439,   440,   441,
     442,   443,     0,     0,     0,     0,     0,   444,     0,   446,
       0,     0,     0,   449,     0,     0,     0,     0,     0,     0,
       0,   451,     0,     0,     0,     0,     0,   454,     0,     0,
     455,     0,     0,   456,     0,     0,     0,   459,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   566,     0,     0,
     463,   464,     0,   318,   319,   320,     0,   322,   323,   324,
     325,   326,   465,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,     0,   340,   341,   342,     0,     0,
     345,   346,   347,   348,   466,   467,   567,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   470,
     471,     0,     0,     0,     0,     0,     0,     0,  1609,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    58,     0,     0,     0,     0,     0,
       0,     0,   475,   476,   477,   478,   479,     0,   480,     0,
     481,   482,   483,   484,   485,   486,   487,   488,   489,   490,
     491,    59,   568,   493,   494,     0,     0,     0,     0,     0,
       0,   495,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   498,   499,
     500,     0,    14,     0,     0,   501,   502,     0,     0,   433,
     434,     0,     0,   503,     0,   504,     0,   505,   506,   439,
     440,   441,   442,   443,     0,     0,     0,     0,     0,   444,
       0,   446,     0,     0,     0,   449,     0,     0,     0,     0,
       0,     0,     0,   451,     0,     0,     0,     0,     0,   454,
       0,     0,   455,     0,     0,   456,     0,     0,     0,   459,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   566,
       0,     0,   463,   464,     0,   318,   319,   320,     0,   322,
     323,   324,   325,   326,   465,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,     0,   340,   341,   342,
       0,     0,   345,   346,   347,   348,   466,   467,   567,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   470,   471,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,   475,   476,   477,   478,   479,     0,
     480,     0,   481,   482,   483,   484,   485,   486,   487,   488,
     489,   490,   491,    59,   568,   493,   494,     0,     0,     0,
       0,     0,     0,   495,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     498,   499,   500,     0,    14,     0,     0,   501,   502,     0,
       0,   433,   434,     0,     0,   503,     0,   504,     0,   505,
     506,   439,   440,   441,   442,   443,     0,     0,     0,     0,
       0,   444,     0,   446,     0,     0,     0,   449,     0,     0,
       0,     0,     0,     0,     0,   451,     0,     0,     0,     0,
       0,   454,     0,     0,   455,     0,     0,   456,     0,     0,
       0,   459,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   566,     0,     0,   463,   464,     0,   318,   319,   320,
       0,   322,   323,   324,   325,   326,   465,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,     0,   340,
     341,   342,     0,     0,   345,   346,   347,   348,   466,   467,
     567,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   470,   471,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    58,     0,
       0,     0,     0,     0,     0,     0,   475,   476,   477,   478,
     479,     0,   480,     0,   481,   482,   483,   484,   485,   486,
     487,   488,   489,   490,   491,    59,   568,   493,   494,     0,
     693,  1374,     0,     0,     0,   495,   312,     0,     0,     0,
     736,   737,   313,     0,     0,     0,     0,     0,   314,     0,
       0,     0,   498,   499,   500,     0,    14,     0,   315,   501,
     502,     0,     0,     0,     0,     0,   316,  1379,     0,   504,
       0,   505,   506,     0,     0,     0,     0,     0,     0,     0,
       0,   317,     0,     0,     0,     0,     0,     0,   318,   319,
     320,   321,   322,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   746,   747,   748,   749,   750,     0,
       0,   753,   754,   755,   756,     0,   758,   759,   760,   761,
       0,     0,     0,     0,   762,     0,   764,   765,     0,    58,
       0,     0,   768,   769,   770,     0,     0,     0,   774,     0,
       0,     0,   351,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    59,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   776,     0,
     777,   778,   779,   780,   781,   782,   783,   784,   785,   786,
       0,   312,     0,     0,     0,     0,     0,   313,     0,   787,
     788,     0,   352,   314,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   315,     0,     0,     0,     0,     0,     0,
       0,   316,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   317,     0,     0,     0,
       0,     0,     0,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     312,     0,     0,     0,     0,     0,   313,     0,     0,     0,
       0,     0,   314,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   315,     0,    58,     0,     0,     0,     0,     0,
     316,     0,     0,     0,     0,     0,     0,   351,     0,     0,
       0,     0,     0,     0,     0,   317,     0,     0,     0,     0,
       0,    59,   318,   319,   320,   321,   322,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   352,     0,   606,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    58,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   351,     0,     0,     0,
       0,     0,     0,     0,     0,   312,     0,     0,     0,     0,
     611,   313,     0,     0,     0,     0,     0,   314,     0,     0,
       0,     0,   612,     0,     0,     0,     0,   315,   613,     0,
       0,     0,     0,     0,     0,   316,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     317,     0,     0,     0,     0,     0,   352,   318,   319,   320,
     321,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   312,     0,     0,     0,     0,     0,
     313,     0,     0,     0,     0,     0,   314,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   315,     0,    58,     0,
       0,     0,     0,     0,   316,     0,     0,     0,     0,     0,
       0,   351,     0,     0,     0,     0,     0,     0,     0,   317,
       0,     0,     0,     0,     0,    59,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   736,
     737,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   352,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    58,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     351,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   611,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   868,   869,   870,   871,   872,
     873,   874,   875,   746,   747,   748,   749,   750,   876,   877,
     753,   754,   755,   756,   878,   758,   759,   760,   761,  -362,
     352,   736,   737,   762,   763,   764,   765,   879,   880,     0,
       0,   768,   769,   770,   881,   882,   883,   774,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   884,   776,     0,   777,
     778,   779,   780,   781,   782,   783,   784,   785,   786,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   787,   788,
       0,     0,     0,   505,   679,     0,     0,   868,   869,   870,
     871,   872,   873,   874,   875,   746,   747,   748,   749,   750,
     876,   877,   753,   754,   755,   756,   878,   758,   759,   760,
     761,   736,   737,     0,     0,   762,   763,   764,   765,   879,
     880,     0,     0,   768,   769,   770,   881,   882,   883,   774,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   736,   737,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1077,     0,     0,     0,     0,     0,     0,   884,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,     0,   505,   679,   868,   869,   870,
     871,   872,   873,   874,   875,   746,   747,   748,   749,   750,
     876,   877,   753,   754,   755,   756,   878,   758,   759,   760,
     761,     0,     0,     0,     0,   762,   763,   764,   765,   879,
     880,     0,     0,   768,   769,   770,   881,   882,   883,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,   736,   737,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,  1280,     0,     0,   774,     0,     0,     0,   884,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   736,   737,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,     0,   505,   679,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,     0,
     505,   679,     0,     0,     0,     0,     0,     0,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,   736,   737,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   736,   737,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,   800,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,   810,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,   736,   737,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   736,   737,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,   826,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,   841,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,   736,   737,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   736,   737,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1150,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1154,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,   736,   737,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   736,   737,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1156,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1165,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,   736,   737,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   736,   737,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1166,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1167,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,   736,   737,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   736,   737,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1168,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1169,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,   736,   737,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   736,   737,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1170,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1301,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,   736,   737,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   736,   737,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1313,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1316,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,   736,   737,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   736,   737,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1455,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1470,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,   736,   737,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   736,   737,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1471,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1492,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,   736,   737,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   736,   737,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1494,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1496,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,   736,   737,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   736,   737,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1500,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1572,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,   736,   737,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   736,   737,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1573,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1574,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,   736,   737,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   736,   737,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1581,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1584,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,   736,   737,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   736,   737,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1594,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1599,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,   736,   737,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   736,   737,     0,     0,     0,     0,     0,     0,     0,
     787,   788,     0,     0,  1664,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,  1665,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   746,   747,   748,   749,   750,
     736,   737,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,     0,     0,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,   831,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,  1100,     0,   746,   747,   748,   749,   750,   736,
     737,   753,   754,   755,   756,     0,   758,   759,   760,   761,
       0,     0,     0,     0,   762,     0,   764,   765,     0,     0,
       0,     0,   768,   769,   770,     0,     0,     0,   774,     0,
       0,     0,     0,     0,   736,   737,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   776,     0,
     777,   778,   779,   780,   781,   782,   783,   784,   785,   786,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   787,
     788,  1252,     0,   746,   747,   748,   749,   750,     0,     0,
     753,   754,   755,   756,     0,   758,   759,   760,   761,     0,
       0,     0,     0,   762,     0,   764,   765,     0,     0,     0,
       0,   768,   769,   770,     0,     0,     0,   774,   746,   747,
     748,   749,   750,   736,   737,   753,   754,   755,   756,     0,
     758,   759,   760,   761,     0,     0,     0,     0,   762,     0,
     764,   765,     0,     0,     0,     0,   768,   769,   770,     0,
       0,     0,   774,     0,     0,     0,     0,   776,     0,   777,
     778,   779,   780,   781,   782,   783,   784,   785,   786,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   787,   788,
    1268,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   776,     0,   777,   778,   779,   780,   781,   782,
     783,   784,   785,   786,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   787,   788,  1403,     0,   746,   747,   748,
     749,   750,     0,     0,   753,   754,   755,   756,     0,   758,
     759,   760,   761,   232,   233,     0,     0,   762,     0,   764,
     765,     0,     0,     0,     0,   768,   769,   770,     0,     0,
     234,   774,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   736,   737,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   776,     0,   777,   778,   779,   780,   781,   782,   783,
     784,   785,   786,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   787,   788,  1408,     0,     0,     0,     0,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,   252,     0,     0,   253,
     254,   255,     0,     0,     0,     0,     0,     0,   256,   257,
     258,   259,   260,     0,     0,   261,   262,   263,   264,   265,
     266,   267,   746,   747,   748,   749,   750,   736,   737,   753,
     754,   755,   756,     0,   758,   759,   760,   761,     0,     0,
       0,     0,   762,     0,   764,   765,     0,     0,     0,     0,
     768,   769,   770,     0,     0,     0,   774,     0,     0,     0,
       0,   268,     0,   269,   270,   271,   272,   273,   274,   275,
     276,   277,   278,     0,     0,   279,   280,     0,     0,     0,
       0,     0,   281,   282,     0,     0,   287,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   776,     0,   777,   778,
     779,   780,   781,   782,   783,   784,   785,   786,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   787,   788,     0,
       0,   746,   747,   748,   749,   750,   736,   737,   753,   754,
     755,   756,     0,   758,   759,   760,   761,     0,     0,     0,
       0,   762,     0,   764,   765,     0,     0,   935,     0,   768,
     769,   770,     0,     0,     0,   774,     0,     0,     0,     0,
       0,   736,   737,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   776,     0,   777,   778,   779,
     780,   781,   782,   783,   784,   785,   786,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   787,   788,     0,     0,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,  1179,     0,   768,   769,
     770,   736,   737,     0,   774,   746,   747,   748,   749,   750,
       0,     0,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,   736,   737,     0,   774,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   776,
    1257,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,   746,   747,   748,   749,   750,
     787,   788,   753,   754,   755,   756,     0,   758,   759,   760,
     761,     0,     0,     0,     0,   762,     0,   764,   765,     0,
       0,     0,     0,   768,   769,   770,   736,   737,     0,   774,
     746,   747,   748,   749,   750,     0,     0,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,  1270,     0,     0,   774,   736,   737,     0,     0,   776,
       0,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,  1363,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
     746,   747,   748,   749,   750,   787,   788,   753,   754,   755,
     756,     0,   758,   759,   760,   761,     0,     0,     0,     0,
     762,     0,   764,   765,     0,     0,     0,     0,   768,   769,
     770,     0,     0,     0,   774,   736,   737,     0,     0,   746,
     747,   748,   749,   750,     0,     0,   753,   754,   755,   756,
       0,   758,   759,   760,   761,     0,     0,     0,     0,   762,
       0,   764,   765,     0,     0,     0,  1595,   768,   769,   770,
     736,   737,     0,   774,   776,     0,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   787,   788,     0,     0,     0,
       0,     0,     0,     0,     0,  1655,     0,     0,     0,     0,
       0,     0,     0,   776,     0,   777,   778,   779,   780,   781,
     782,   783,   784,   785,   786,     0,     0,     0,     0,   746,
     747,   748,   749,   750,   787,   788,   753,   754,   755,   756,
       0,   758,   759,   760,   761,     0,     0,     0,     0,   762,
       0,   764,   765,     0,     0,     0,     0,   768,   769,   770,
     736,   737,     0,   774,   746,   747,   748,   749,   750,     0,
       0,   753,   754,   755,   756,     0,   758,   759,   760,   761,
       0,     0,     0,     0,   762,     0,   764,   765,     0,     0,
       0,     0,   768,   769,   770,  1680,     0,     0,   774,   736,
     737,     0,     0,   776,     0,   777,   778,   779,   780,   781,
     782,   783,   784,   785,   786,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   787,   788,     0,     0,     0,     0,
    1681,     0,     0,     0,     0,     0,     0,     0,   776,     0,
     777,   778,   779,   780,   781,   782,   783,   784,   785,   786,
       0,     0,     0,     0,   746,   747,   748,   749,   750,   787,
     788,   753,   754,   755,   756,     0,   758,   759,   760,   761,
       0,     0,     0,     0,   762,     0,   764,   765,     0,     0,
       0,     0,   768,   769,   770,     0,     0,     0,   774,   736,
     737,     0,     0,   746,   747,   748,   749,   750,     0,     0,
     753,   754,   755,   756,     0,   758,   759,   760,   761,     0,
       0,     0,     0,   762,     0,   764,   765,     0,     0,     0,
    1687,   768,   769,   770,   736,   737,     0,   774,   776,     0,
     777,   778,   779,   780,   781,   782,   783,   784,   785,   786,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   787,
     788,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   776,     0,   777,
     778,   779,   780,   781,   782,   783,   784,   785,   786,     0,
       0,     0,     0,   746,   747,   748,   749,   750,   787,   788,
     753,   754,   755,   756,     0,   758,   759,   760,   761,     0,
       0,     0,     0,   762,     0,   764,   765,     0,     0,     0,
       0,   768,   769,   770,   736,   737,     0,  -758,   746,   747,
     748,   749,   750,     0,     0,   753,   754,   755,   756,     0,
     758,   759,   760,   761,     0,     0,     0,     0,   762,     0,
     764,   765,     0,   736,   737,     0,   768,   769,   770,     0,
       0,     0,     0,     0,     0,     0,     0,   776,     0,   777,
     778,   779,   780,   781,   782,   783,   784,   785,   786,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   787,   788,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   776,     0,   777,   778,   779,   780,   781,   782,
     783,   784,   785,   786,     0,     0,     0,     0,   746,   747,
     748,   749,   750,   787,   788,   753,   754,   755,   756,     0,
     758,   759,   760,   761,     0,     0,     0,     0,   762,     0,
     764,   765,     0,   736,   737,     0,   768,   746,   747,   748,
     749,   750,     0,     0,   753,   754,   755,   756,     0,   758,
     759,   760,   761,     0,     0,     0,     0,   762,     0,   764,
     765,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   777,   778,   779,   780,   781,   782,
     783,   784,   785,   786,     0,     0,     0,     0,     0,   977,
       0,     0,     0,   787,   788,     0,     0,     0,     0,     0,
       0,     0,     0,   777,   778,   779,   780,   781,   782,   783,
     784,   785,   786,     0,     0,     0,     0,   746,   747,   748,
     749,   750,   787,   788,   753,   754,   755,   756,     0,   758,
     759,   760,   761,     0,     0,     0,     0,   762,     0,   764,
     765,   318,   319,   320,     0,   322,   323,   324,   325,   326,
     465,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,     0,   340,   341,   342,     0,     0,   345,   346,
     347,   348,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   981,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   779,   780,   781,   782,   783,
     784,   785,   786,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   787,   788,     0,  1253,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   978,     0,     0,
       0,     0,     0,     0,     0,     0,   318,   319,   320,   979,
     322,   323,   324,   325,   326,   465,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,     0,   340,   341,
     342,     0,     0,   345,   346,   347,   348,   318,   319,   320,
       0,   322,   323,   324,   325,   326,   465,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   337,   338,     0,   340,
     341,   342,     0,     0,   345,   346,   347,   348,     0,     0,
       0,   318,   319,   320,     0,   322,   323,   324,   325,   326,
     465,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   982,   340,   341,   342,   182,     0,   345,   346,
     347,   348,     0,     0,   983,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1254,     0,     0,     0,     0,  1056,  1057,
       0,     0,   183,     0,   184,  1255,   185,   186,   187,   188,
     189,     0,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,     0,   201,   202,   203,  1058,     0,   204,
     205,   206,   207,     0,     0,     0,     0,     0,     0,  1059,
       0,     0,     0,     0,     0,     0,     0,     0,   208,   209,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1060,  1061,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     210
};

static const yytype_int16 yycheck[] =
{
      14,    15,   212,   573,   484,   445,   626,   714,   692,   716,
     694,   377,   522,   228,   390,   592,   377,   593,   926,   910,
     421,   422,   694,   631,   932,    80,   817,  1182,   819,   161,
     821,   587,   539,   589,   445,   591,   384,   473,   474,   228,
       5,   725,   855,   419,  1323,    20,     0,    33,   169,    63,
      64,    65,     8,     7,    20,  1382,   563,   103,   469,    22,
     181,    53,     4,   162,   504,  1422,  1446,    20,    15,    16,
     126,    21,    22,    19,    20,    33,    30,    20,    32,   126,
      34,   131,   142,   691,   445,   693,    40,   695,   159,    20,
     104,   105,   106,   107,    57,   703,    50,    20,    20,     7,
     154,   126,    56,   142,   712,   159,   482,   483,   469,   169,
     151,   136,  1575,     5,     6,   194,    49,    57,   159,   143,
     144,   145,   829,    63,    15,    16,    80,   179,   169,  1509,
    1510,   181,   188,    25,   213,   191,    46,   102,   103,    31,
     187,   188,    50,   190,  1549,   216,   193,  1527,   102,   103,
     963,   190,   169,   213,  1481,    33,   596,   864,    91,  1622,
     375,  1518,   216,   188,    33,   220,   577,   117,   118,   181,
     302,   855,   213,   219,   681,   125,    68,    69,   128,   129,
     130,   888,    60,    61,   181,   135,   375,  1544,  1343,   181,
     214,    60,    61,   169,   901,    62,   106,   904,   215,   187,
      53,   390,  1607,   643,   644,  1585,  1586,   193,   128,   129,
     102,   103,   187,   614,   228,   162,   577,   159,   619,   216,
     167,   187,   169,   169,   180,   172,   214,   181,   668,   194,
     419,   671,   421,   644,   187,   193,   199,   213,   448,   191,
     180,  1520,   181,   195,   187,   123,   200,   210,   213,   127,
     142,   306,   462,   211,   123,   211,   187,   211,   127,   217,
     210,   211,    47,   674,   187,   187,   213,   159,   160,   161,
    1178,   162,  1163,   179,   650,   214,   167,   169,   169,   963,
     964,   172,    67,   644,   940,   725,   942,   179,   151,   377,
     210,   211,   125,   482,   483,   128,   129,   673,   734,   141,
    1113,  1456,   194,   181,   157,   151,   169,   213,   142,   187,
     191,   189,   126,   674,   195,   193,   169,   195,   196,   211,
     189,   187,   136,   169,   193,    34,   195,   196,   187,   169,
     170,   171,  1223,   211,   179,   169,   154,   179,   352,   217,
     188,   159,   211,   951,   157,   190,   212,   157,   217,   143,
     790,   145,   126,   212,    63,   169,   169,   445,   181,   169,
     200,   375,   136,   578,   579,   580,   380,   582,   583,   190,
     126,   586,   195,   588,   188,   590,   390,   210,   211,   193,
     136,   469,   187,    33,   732,   126,   142,   187,  1279,   578,
     579,   580,   803,   582,   583,   136,   214,   586,   216,   588,
     213,   590,   126,   592,   214,   419,   187,   421,   809,   142,
      60,    61,   136,   191,   188,   855,   179,   195,   151,   975,
     630,    33,   823,   799,   825,   187,   135,   441,   442,  1113,
     126,   212,   188,   142,  1342,   194,   169,  1540,   849,  1542,
     136,   187,   803,  1546,  1547,   655,  1354,   188,    60,    61,
     212,  1242,   215,   216,   213,   962,   187,   937,   142,   194,
     169,   650,     5,     6,   188,     8,   212,   947,   482,   483,
     184,   185,   187,   123,   850,   851,   187,   127,   213,   215,
     216,   212,   918,   923,   673,   886,  1106,   863,   849,   577,
     700,   931,   188,    36,  1178,   204,   187,  1181,   212,   214,
     169,   212,  1605,  1580,   213,   187,  1178,    21,    22,  1193,
     720,   123,   181,   187,  1591,   127,  1093,   893,   894,  1095,
     896,  1193,   194,   214,   900,   187,   902,   903,   181,   905,
     187,   184,   214,   187,   179,   187,   187,   173,   174,   189,
     214,   213,   187,   193,   142,  1336,   196,   736,   737,   985,
     986,   987,   214,   151,  1631,  1632,   644,   214,   156,   212,
     214,   211,   214,   214,   578,   579,   580,   217,   582,   583,
     187,   169,   586,   762,   588,   194,   590,   189,   592,   187,
    1371,   193,   187,   195,   196,   181,   674,    63,    64,    65,
     180,   181,   188,   183,   213,   190,   194,   214,   787,   211,
     195,   169,   190,   117,   118,   217,   214,   195,  1164,   214,
     799,   125,   827,   127,   128,   129,   130,   210,   213,   190,
     835,   135,   636,   838,   195,   213,    21,    22,   104,   105,
     106,   107,   847,   181,   181,   181,   650,   181,   827,   854,
     187,   187,   213,   187,   859,   191,   835,   195,   195,   838,
     738,   739,   740,   741,   742,   743,   744,   745,   847,   673,
    1216,   850,   851,   751,   752,   854,   798,   190,   213,   757,
     859,  1111,   195,   191,   863,   885,   169,   195,   766,   767,
      21,    22,   162,   771,   772,   773,  1477,   775,   181,    12,
     213,   169,   181,    57,   187,  1379,   210,   211,   187,    63,
      23,    24,    57,   717,   893,   894,   195,   896,    63,    86,
      87,   900,   157,   902,   903,   803,   905,   143,   144,   145,
     115,   116,   117,   118,   169,   940,  1346,   942,   169,   157,
     125,  1171,   127,   128,   129,   130,   190,   157,    57,   193,
     135,   169,   137,   138,    63,   157,  1122,  1123,  1124,   169,
     181,   940,  1192,   942,   181,    75,   187,   169,  1198,    79,
     187,   849,   181,   157,   195,  1205,    57,   190,   195,  1209,
     193,  1147,    63,    93,    94,   169,   117,   118,    98,    99,
     100,   101,   796,   181,   125,   799,   127,   128,   129,   130,
      57,    57,   181,   181,   135,   169,    63,    63,   187,   187,
     169,   196,   197,   198,   199,   200,   195,   195,   187,   169,
    1250,   190,    57,   827,   193,   210,   211,   169,    63,   173,
    1376,   835,  1262,   180,   838,  1335,   183,  1267,   181,   186,
     169,   179,   181,   847,   187,  1455,   850,   851,   187,  1215,
     854,    57,  1208,   181,  1528,   859,   181,  1208,   181,   863,
     181,   173,   187,   143,   187,  1044,   187,   198,   199,   200,
     169,   170,   171,   191,   191,   181,  1081,   195,   195,   210,
     211,   187,   106,   191,   191,   188,   179,   195,   195,   893,
     894,   191,   896,  1590,    35,   195,   900,   179,   902,   903,
     191,   905,  1081,   179,   195,  1602,   191,  1337,    35,   213,
     195,   215,   216,  1587,  1093,   173,   174,   175,   176,   213,
     179,  1312,   169,  1483,   173,   174,   175,   180,  1625,   173,
     174,   175,   180,   181,   182,   169,   940,  1367,   942,    10,
      11,    12,   169,  1122,  1123,  1124,   169,   377,   212,  1623,
      22,   169,   212,   190,   179,    43,   194,   387,   190,   179,
     169,   213,   213,   194,   194,   194,   396,   213,  1147,   136,
     213,   213,   194,   194,   169,   194,   406,   194,    21,    22,
     194,   194,   194,   191,   212,   169,   416,   169,   169,  1599,
     194,   194,  1422,   194,  1464,   425,   211,  1467,   194,   162,
     194,   181,   213,  1563,  1564,   435,   213,   194,   194,   194,
    1684,   194,    37,   213,   213,   445,   213,  1217,   194,   181,
     450,    33,   452,   213,   213,   213,  1392,   181,    10,   181,
     181,   461,   181,   472,   473,   474,  1215,   216,   181,   469,
     470,   471,   181,    66,   213,   211,   180,   194,    60,    61,
     194,    43,   213,  1613,   484,   214,   169,   169,   181,   212,
     180,   180,   492,   180,   190,   495,    43,   213,   498,   499,
     500,   501,   502,   213,   117,   118,   194,  1081,   194,   194,
     187,   511,   125,   522,   127,   128,   129,   130,  1518,  1093,
     131,    14,   135,   188,  1524,  1300,   190,   162,   187,   179,
      13,   187,   216,   169,     8,   169,  1306,    33,   195,   214,
     549,   123,   169,   169,  1544,   127,  1482,   169,  1122,  1123,
    1124,  1300,  1552,   214,   187,   187,   180,   180,   213,   212,
    1208,   169,   214,   214,    60,    61,   213,   213,   213,   194,
     213,     1,    67,  1147,   194,   213,   213,   577,   180,   169,
     181,   195,   213,   196,   197,   198,   199,   200,   195,   195,
      43,   169,   213,   169,   213,   213,   213,   210,   211,   214,
     169,   169,   214,  1373,   214,   605,   169,   189,  1608,   214,
     214,   193,   212,   195,   196,   213,   212,   214,   212,  1394,
     194,  1391,   213,   181,   213,   213,  1562,   123,   213,   211,
     213,   127,   169,   214,   169,   217,   213,   181,   213,   648,
     212,  1215,   169,  1392,   644,  1394,   212,   214,   169,    33,
     169,   651,   652,   212,   654,   195,   213,   213,   213,   659,
     213,    70,   195,   214,   213,   665,   214,   676,   213,   195,
     213,   213,   213,   181,   674,   214,   214,   213,    53,   213,
     212,  1451,   213,   213,   187,   685,   686,   687,   688,   689,
     690,   214,   692,   189,   694,   180,   214,   193,   214,   195,
     196,   180,   180,   212,   181,   187,   702,   214,   214,   214,
     214,   212,   214,   212,   180,   211,   214,   214,   212,    79,
       1,   217,  1497,   213,    44,   734,  1300,    21,    22,   133,
     214,    82,  1474,  1482,  1475,   226,   796,  1475,   738,   739,
     740,   741,   742,   743,   744,   745,   746,   747,  1497,   949,
     750,   751,   752,   753,   754,   755,  1475,   757,   758,  1475,
     760,   761,   762,   763,   764,   765,   766,   767,   768,   769,
     770,   771,   772,   773,   774,   775,   776,   777,   778,   779,
     780,   781,   782,   783,   784,   785,   786,  1475,   788,     1,
    1560,  1451,   792,  1389,   970,  1234,   805,  1414,  1506,  1417,
    1275,   429,  1507,   803,    52,  1587,  1462,    -1,  1188,   694,
      -1,   441,   441,  1562,   561,   495,    -1,    -1,  1392,    -1,
    1394,   115,   116,   117,   118,   119,    -1,    -1,   122,   123,
     124,   125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,
      -1,   135,   842,   137,   138,    -1,    -1,    -1,    -1,   849,
      -1,    -1,    -1,    -1,    -1,    -1,   856,    -1,   858,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   867,   868,   869,
     870,   871,   872,   873,   874,   875,   876,   877,   878,   879,
     880,   881,   882,   883,   884,    -1,    -1,    -1,  1658,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   908,
     194,   195,   196,   197,   198,   199,   200,    -1,  1482,   918,
      -1,    -1,    -1,    33,    -1,    -1,   210,   211,    -1,    -1,
      -1,   921,   922,  1497,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   935,    -1,   937,    -1,    -1,
      60,    61,    -1,    -1,   944,    -1,    -1,   947,    -1,    -1,
      -1,    -1,   952,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   961,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1549,   985,   986,   987,    -1,
      -1,   990,    -1,   992,    -1,   994,    -1,   996,  1562,   998,
      -1,  1000,    -1,  1002,    -1,  1004,    -1,    -1,    -1,    -1,
    1009,    -1,  1011,   123,    -1,    -1,   126,   127,  1017,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   136,    -1,    -1,    -1,
    1029,    -1,  1031,    -1,    -1,    -1,    -1,  1036,    -1,  1038,
      -1,  1040,    -1,  1607,  1043,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    13,    21,    22,
      -1,    -1,    -1,    19,    -1,    -1,    -1,  1057,    -1,    25,
      -1,  1061,    -1,    -1,    -1,    31,    -1,  1076,   188,   189,
      -1,    -1,    -1,   193,    -1,    41,   196,    -1,    -1,    -1,
      -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   211,    -1,    -1,    -1,    -1,    -1,   217,    64,  1108,
      -1,    -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,    -1,    -1,
      -1,    -1,   115,   116,   117,   118,   119,    -1,    -1,   122,
      -1,  1151,   125,    -1,   127,   128,   129,   130,    -1,    -1,
      -1,    -1,   135,    -1,   137,   138,    -1,    -1,    -1,    -1,
      -1,    10,    -1,  1173,  1174,  1175,   142,    -1,    -1,  1179,
      -1,    -1,    21,    22,    -1,    -1,    -1,  1187,  1188,   155,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   169,    -1,    -1,    -1,    -1,  1208,    -1,
      -1,    -1,    -1,  1213,  1214,    -1,    -1,    -1,    -1,    -1,
    1220,   194,   195,   196,   197,   198,   199,   200,  1228,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   210,   211,    -1,
      -1,    -1,    -1,    -1,  1244,    -1,  1246,    -1,    -1,   215,
      -1,  1251,    -1,    -1,    -1,    -1,    -1,  1257,    -1,    -1,
      -1,  1261,    -1,    -1,    -1,    -1,    -1,    -1,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,    33,    -1,    -1,    -1,   135,   136,   137,   138,
     139,   140,    -1,  1303,   143,   144,   145,   146,   147,   148,
     149,    -1,   739,   740,   741,   742,   743,   744,   745,    60,
      61,    -1,    -1,  1323,   751,   752,  1335,    -1,    -1,    -1,
     757,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   766,
     767,    -1,    -1,    -1,   771,   772,   773,    -1,   775,   188,
     189,    -1,   191,   192,   193,   194,   195,   196,   197,   198,
     199,   200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   210,   211,    -1,    -1,    -1,   215,   216,    -1,    -1,
      -1,    -1,   123,    21,    22,   126,   127,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,    -1,    -1,    -1,
    1400,    -1,  1402,    -1,    -1,    -1,    -1,  1407,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1431,    -1,    -1,  1434,    -1,    -1,    -1,    -1,    -1,
     181,    -1,  1442,  1443,  1444,    -1,    -1,   188,   189,  1449,
      -1,    -1,   193,    -1,  1454,   196,    -1,    -1,  1458,  1459,
    1460,  1461,    -1,    -1,  1464,  1465,    -1,  1467,  1468,    -1,
     211,    -1,    -1,    -1,    -1,  1475,   217,   115,   116,   117,
     118,   119,    -1,    -1,   122,   123,   124,   125,  1488,   127,
     128,   129,   130,    -1,    -1,    -1,    -1,   135,    -1,   137,
     138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,
     116,   117,   118,   119,    -1,    -1,   122,   123,   124,   125,
    1520,   127,   128,   129,   130,    -1,    -1,    -1,    -1,   135,
      -1,   137,   138,    -1,  1534,  1535,    -1,   143,    -1,   145,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1548,    -1,
      -1,    -1,    -1,  1553,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,  1565,    -1,  1567,    -1,    -1,
      -1,    33,   210,   211,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1582,  1583,    -1,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    60,    61,
      -1,    -1,    -1,    -1,   210,   211,    -1,    -1,    -1,  1609,
      -1,    -1,    -1,    -1,  1614,  1615,    -1,    -1,    -1,     1,
      -1,    -1,    -1,     5,     6,     7,    -1,     9,    10,    11,
      -1,    13,    -1,    15,    16,    17,    18,    19,    -1,  1639,
      -1,  1641,  1642,    25,    26,    27,    28,    29,    -1,    31,
      -1,    -1,  1652,    -1,    -1,    -1,    38,    39,    -1,    -1,
      42,   123,    44,    45,    -1,   127,    48,    -1,    50,    51,
      52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,    -1,   189,    -1,    -1,
      -1,   193,    -1,   195,   196,   117,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   211,
     132,   133,   134,    -1,    -1,   217,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,   151,
     152,   153,   154,    -1,   156,    -1,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,   181,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   196,   197,   198,    -1,   200,    -1,
      -1,   203,   204,    -1,    -1,    -1,    -1,    -1,    -1,   211,
      -1,   213,     1,   215,   216,    -1,     5,     6,     7,    -1,
       9,    10,    11,    -1,    13,    -1,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      29,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      39,    -1,    -1,    42,    -1,    44,    45,    -1,    -1,    48,
      -1,    50,    51,    52,    -1,    54,    55,    -1,    -1,    58,
      59,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,   105,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   132,   133,   134,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   150,   151,   152,   153,   154,    33,   156,    -1,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,
     179,   180,   181,    60,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,
      -1,   200,    -1,    -1,   203,   204,    -1,    -1,     5,     6,
      -1,    -1,   211,    -1,   213,    -1,   215,   216,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    33,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,   123,    -1,    45,    -1,
     127,    48,    -1,    -1,    51,    -1,    53,    -1,    55,    -1,
      -1,    -1,    -1,    60,    61,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,   189,    -1,    -1,    -1,   193,    -1,   195,   196,
     117,   118,    -1,    -1,    -1,    -1,   123,    -1,    -1,    -1,
     127,    -1,    -1,    -1,   211,    -1,    -1,    -1,    -1,    -1,
     217,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   150,   151,   152,   153,   154,    33,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,   179,    -1,    -1,    60,    61,    -1,    -1,    -1,
      -1,    -1,   189,    -1,    -1,    -1,   193,    -1,    -1,   196,
     197,   198,    -1,   200,    -1,    -1,   203,   204,    -1,    -1,
      -1,     5,     6,    -1,   211,    -1,   213,    -1,   215,   216,
     217,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    33,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,   123,    -1,
      -1,    45,   127,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,   189,    -1,    -1,    -1,   193,    -1,
     195,   196,    -1,   117,   118,    -1,    -1,    -1,    -1,   123,
      -1,    -1,    -1,   127,    -1,    -1,   211,    -1,    -1,    -1,
      -1,    -1,   217,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
     154,    33,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    60,    61,
      -1,    -1,    -1,    -1,    -1,   189,    -1,    -1,    -1,   193,
      -1,    -1,   196,   197,   198,    -1,   200,    -1,    -1,   203,
     204,    -1,    -1,    -1,     5,     6,    -1,   211,    -1,   213,
      -1,   215,   216,   217,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,   123,    -1,    -1,    45,   127,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,   189,    -1,    -1,
      -1,   193,    -1,   195,   196,    -1,   117,   118,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   211,
      -1,    -1,    -1,    -1,    -1,   217,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,
     151,   152,   153,   154,    33,   156,    -1,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,    -1,
      -1,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   193,    -1,    -1,   196,   197,   198,    -1,   200,
      -1,    -1,   203,   204,    -1,    -1,    -1,     5,     6,    -1,
     211,    -1,   213,    -1,   215,   216,   217,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,   123,    -1,    -1,    45,   127,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
     189,    -1,    -1,    -1,   193,    -1,   195,   196,    -1,   117,
     118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,    -1,
      -1,    -1,   211,    -1,   132,   133,   134,    -1,   217,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,   151,   152,   153,   154,    33,   156,    -1,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,   179,    -1,    -1,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   196,   197,
     198,    -1,   200,    -1,    -1,   203,   204,    -1,    -1,     5,
       6,    -1,    -1,   211,    -1,   213,    -1,   215,   216,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,   123,    -1,    45,
      -1,   127,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,   189,    -1,    -1,    -1,   193,    -1,   195,
     196,   117,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     126,    -1,    -1,    -1,    -1,   211,   132,   133,   134,    -1,
      -1,   217,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   150,   151,   152,   153,   154,    33,
     156,    -1,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,    -1,    -1,    -1,
      -1,    -1,    -1,   179,    -1,    -1,    60,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     196,   197,   198,    -1,   200,    -1,    -1,   203,   204,    -1,
      -1,     5,     6,    -1,    -1,   211,    -1,   213,    -1,   215,
     216,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    26,    27,    28,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,   123,
      -1,    45,    -1,   127,    48,    -1,    -1,    51,    52,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,    -1,    -1,   189,    -1,    -1,    -1,   193,
      -1,   195,   196,   117,   118,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   211,    -1,    -1,
      -1,    -1,    -1,   217,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
     154,    33,   156,    -1,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    60,    61,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   196,   197,   198,    -1,   200,    -1,    -1,   203,
     204,    -1,    -1,     5,     6,    -1,    -1,   211,    -1,   213,
      -1,   215,   216,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,   123,    -1,    45,    -1,   127,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,   189,    -1,    -1,
      -1,   193,    -1,   195,   196,   117,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   211,
     132,   133,   134,    -1,    -1,   217,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,   151,
     152,   153,   154,    33,   156,    -1,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,
      60,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   196,   197,   198,    -1,   200,    -1,
      -1,   203,   204,    -1,    -1,     5,     6,    -1,    -1,   211,
      -1,   213,    -1,   215,   216,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,   123,    -1,    45,    -1,   127,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      70,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,   189,
      -1,    -1,    -1,   193,    -1,   195,   196,   117,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   211,    -1,    -1,    -1,    -1,    -1,   217,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,   154,    33,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,
      -1,    -1,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,    -1,
     200,    -1,    -1,   203,   204,    -1,    -1,     5,     6,    -1,
      -1,   211,    -1,   213,   214,   215,   216,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    33,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,   123,    -1,    45,    -1,   127,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    60,
      61,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,   189,    -1,    -1,    -1,   193,    -1,   195,   196,   117,
     118,    -1,   123,    -1,    -1,    -1,   127,    -1,    -1,    -1,
      -1,    -1,    -1,   211,   132,   133,   134,    -1,    -1,   217,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,   151,   152,   153,   154,    -1,   156,    33,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,   179,    -1,    -1,    -1,    -1,    60,    61,   189,    -1,
      -1,    -1,   193,    -1,   195,   196,    -1,    -1,   196,   197,
     198,    -1,   200,    -1,    -1,   203,   204,    -1,    -1,    -1,
     211,     5,     6,   211,    -1,   213,   217,   215,   216,    13,
      -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    33,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,   123,
      -1,    45,    -1,   127,    48,    49,    -1,    51,    -1,    -1,
      -1,    55,    -1,    60,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,   189,    -1,    -1,    -1,   193,
      -1,   195,   196,   117,   118,    -1,   123,    -1,    -1,    -1,
     127,    -1,    -1,    -1,    -1,    -1,    -1,   211,    -1,    -1,
      -1,    -1,    -1,   217,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
     154,    -1,   156,    33,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,
      60,    61,   189,    -1,    -1,    -1,   193,    -1,   195,   196,
      -1,    -1,   196,   197,   198,    -1,   200,    -1,    -1,   203,
     204,    -1,    -1,    -1,   211,     5,     6,   211,   212,   213,
     217,   215,   216,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,   123,    -1,    45,    -1,   127,    48,    49,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,   189,
      -1,    -1,    -1,   193,    -1,   195,   196,   117,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   211,    -1,    -1,    -1,    -1,    -1,   217,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,   154,    -1,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,    -1,
     200,    -1,    -1,   203,   204,    -1,    -1,    -1,    -1,     5,
       6,   211,    -1,   213,    -1,   215,   216,    13,    -1,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    49,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   117,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   150,   151,   152,   153,   154,    -1,
     156,    -1,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,    -1,    -1,    -1,
      -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     196,   197,   198,    -1,   200,    -1,    -1,   203,   204,    -1,
      -1,    -1,    -1,     5,     6,   211,   212,   213,    -1,   215,
     216,    13,    -1,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    49,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   117,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,   151,
     152,   153,   154,    -1,   156,    -1,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   196,   197,   198,    -1,   200,    -1,
      -1,   203,   204,    -1,    -1,     5,     6,    -1,    -1,   211,
      -1,   213,    -1,   215,   216,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      70,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,   154,    -1,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,    -1,
     200,    -1,    -1,   203,   204,    -1,    -1,     5,     6,    -1,
      -1,   211,    -1,   213,    -1,   215,   216,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
     118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,   151,   152,   153,   154,    -1,   156,    -1,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   196,   197,
     198,    -1,   200,    -1,    -1,   203,   204,    -1,    -1,     5,
       6,    -1,    -1,   211,   212,   213,    -1,   215,   216,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    61,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   117,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   150,   151,   152,   153,   154,    -1,
     156,    -1,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,    -1,    -1,    -1,
      -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     196,   197,   198,    -1,   200,    -1,    -1,   203,   204,    -1,
      -1,     5,     6,    -1,    -1,   211,    -1,   213,    -1,   215,
     216,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   117,   118,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
     154,    -1,   156,    -1,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   196,   197,   198,    -1,   200,    -1,    -1,   203,
     204,    -1,    -1,    -1,    -1,     5,     6,   211,    -1,   213,
      -1,   215,   216,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,   154,    -1,   156,    -1,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,    -1,
     200,    -1,    -1,   203,   204,    -1,    -1,     5,     6,    -1,
      -1,   211,    -1,   213,    -1,   215,   216,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
     118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,   151,   152,   153,   154,    -1,   156,    -1,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   194,    -1,   196,   197,
     198,    -1,   200,    -1,    -1,   203,   204,    -1,    -1,    -1,
      -1,     5,     6,   211,    -1,   213,    -1,   215,   216,    13,
      -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   117,   118,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
     154,    -1,   156,    -1,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   196,   197,   198,    -1,   200,    -1,    -1,   203,
     204,    -1,    -1,     5,     6,    -1,    -1,   211,    -1,   213,
      -1,   215,   216,    15,    16,    17,    18,    19,    -1,    -1,
      22,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   117,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,   151,
     152,   153,   154,    -1,   156,    -1,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   196,   197,   198,    -1,   200,    -1,
      -1,   203,   204,    -1,    -1,     5,     6,    -1,    -1,   211,
      -1,   213,    -1,   215,   216,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,   154,    -1,   156,    -1,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,    -1,
     200,    -1,    -1,   203,   204,    -1,    -1,     5,     6,    -1,
      -1,   211,    -1,   213,   214,   215,   216,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
     118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,   151,   152,   153,   154,    -1,   156,    -1,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   194,    -1,   196,   197,
     198,    -1,   200,    -1,    -1,   203,   204,    -1,    -1,     5,
       6,    -1,    -1,   211,    -1,   213,    -1,   215,   216,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   117,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   150,   151,   152,   153,   154,    -1,
     156,    -1,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,    -1,    -1,    -1,
      -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     196,   197,   198,    -1,   200,    -1,    -1,   203,   204,    -1,
      -1,     5,     6,    -1,    -1,   211,    -1,   213,   214,   215,
     216,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   117,   118,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
     154,    -1,   156,    -1,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   196,   197,   198,    -1,   200,    -1,    -1,   203,
     204,    -1,    -1,     5,     6,    -1,    -1,   211,    -1,   213,
     214,   215,   216,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   117,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,   151,
     152,   153,   154,    -1,   156,    -1,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   196,   197,   198,    -1,   200,    -1,
      -1,   203,   204,    -1,    -1,     5,     6,    -1,    -1,   211,
      -1,   213,   214,   215,   216,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,   154,    -1,   156,    -1,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,    -1,
     200,    -1,    -1,   203,   204,    -1,    -1,     5,     6,    -1,
      -1,   211,    -1,   213,   214,   215,   216,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
     118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,   151,   152,   153,   154,    -1,   156,    -1,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   196,   197,
     198,    -1,   200,    -1,    -1,   203,   204,    -1,    -1,     5,
       6,    -1,    -1,   211,    -1,   213,   214,   215,   216,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   117,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   150,   151,   152,   153,   154,    -1,
     156,    -1,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,    -1,    -1,    -1,
      -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     196,   197,   198,    -1,   200,    -1,    -1,   203,   204,    -1,
      -1,     5,     6,    -1,    -1,   211,    -1,   213,   214,   215,
     216,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   117,   118,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
     154,    -1,   156,    -1,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   196,   197,   198,    -1,   200,    -1,    -1,   203,
     204,    -1,    -1,     5,     6,    -1,    -1,   211,    -1,   213,
     214,   215,   216,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   117,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   126,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   150,   151,
     152,   153,   154,    -1,   156,    -1,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   196,   197,   198,    -1,   200,    -1,
      -1,   203,   204,    -1,    -1,     5,     6,    -1,    -1,   211,
      -1,   213,    -1,   215,   216,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   126,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,   154,    -1,   156,    -1,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,    -1,
     200,    -1,    -1,   203,   204,    -1,    -1,     5,     6,    -1,
      -1,   211,    -1,   213,    -1,   215,   216,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
     118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   126,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   150,   151,   152,   153,   154,    -1,   156,    -1,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   196,   197,
     198,    -1,   200,    -1,    -1,   203,   204,    -1,    -1,     5,
       6,    -1,    -1,   211,    -1,   213,    -1,   215,   216,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   117,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   150,   151,   152,   153,   154,    -1,
     156,    -1,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,    -1,    -1,    -1,
      -1,    -1,    -1,   179,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     196,   197,   198,    -1,   200,    -1,    -1,   203,   204,    -1,
      -1,     5,     6,    -1,    -1,   211,    -1,   213,    -1,   215,
     216,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   117,   118,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
     154,    -1,   156,    -1,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,    -1,
      13,    12,    -1,    -1,    -1,   179,    19,    -1,    -1,    -1,
      21,    22,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,
      -1,    -1,   196,   197,   198,    -1,   200,    -1,    41,   203,
     204,    -1,    -1,    -1,    -1,    -1,    49,   211,    -1,   213,
      -1,   215,   216,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,    -1,
      -1,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
      -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,   142,
      -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,    -1,
      -1,    -1,   155,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   169,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,    -1,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      -1,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,   210,
     211,    -1,   215,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,    -1,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,   155,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,   169,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   215,    -1,   217,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   155,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,
     169,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,   181,    -1,    -1,    -1,    -1,    41,   187,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      64,    -1,    -1,    -1,    -1,    -1,   215,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,   155,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,   169,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   215,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     155,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   169,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     215,    21,    22,   135,   136,   137,   138,   139,   140,    -1,
      -1,   143,   144,   145,   146,   147,   148,   149,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   188,   189,    -1,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   210,   211,
      -1,    -1,    -1,   215,   216,    -1,    -1,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,    21,    22,    -1,    -1,   135,   136,   137,   138,   139,
     140,    -1,    -1,   143,   144,   145,   146,   147,   148,   149,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   181,    -1,    -1,    -1,    -1,    -1,    -1,   188,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,    -1,   215,   216,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,   136,   137,   138,   139,
     140,    -1,    -1,   143,   144,   145,   146,   147,   148,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    21,    22,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,   181,    -1,    -1,   149,    -1,    -1,    -1,   188,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,    -1,   215,   216,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,    -1,
     215,   216,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    21,    22,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    21,    22,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    21,    22,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    21,    22,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    21,    22,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    21,    22,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    21,    22,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    21,    22,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    21,    22,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    21,    22,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    21,    22,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    21,    22,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    21,    22,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    21,    22,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    21,    22,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    21,    22,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,   214,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,   115,   116,   117,   118,   119,
      21,    22,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,   212,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,   212,    -1,   115,   116,   117,   118,   119,    21,
      22,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
      -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,    -1,
      -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,    -1,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   210,
     211,   212,    -1,   115,   116,   117,   118,   119,    -1,    -1,
     122,   123,   124,   125,    -1,   127,   128,   129,   130,    -1,
      -1,    -1,    -1,   135,    -1,   137,   138,    -1,    -1,    -1,
      -1,   143,   144,   145,    -1,    -1,    -1,   149,   115,   116,
     117,   118,   119,    21,    22,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,    -1,    -1,    -1,    -1,   135,    -1,
     137,   138,    -1,    -1,    -1,    -1,   143,   144,   145,    -1,
      -1,    -1,   149,    -1,    -1,    -1,    -1,   189,    -1,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   210,   211,
     212,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   189,    -1,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   210,   211,   212,    -1,   115,   116,   117,
     118,   119,    -1,    -1,   122,   123,   124,   125,    -1,   127,
     128,   129,   130,    21,    22,    -1,    -1,   135,    -1,   137,
     138,    -1,    -1,    -1,    -1,   143,   144,   145,    -1,    -1,
      38,   149,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   189,    -1,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   210,   211,   212,    -1,    -1,    -1,    -1,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    -1,   127,
     128,   129,    -1,    -1,    -1,    -1,    -1,    -1,   136,   137,
     138,   139,   140,    -1,    -1,   143,   144,   145,   146,   147,
     148,   149,   115,   116,   117,   118,   119,    21,    22,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,    -1,    -1,
      -1,    -1,   135,    -1,   137,   138,    -1,    -1,    -1,    -1,
     143,   144,   145,    -1,    -1,    -1,   149,    -1,    -1,    -1,
      -1,   189,    -1,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,   203,   204,    -1,    -1,    -1,
      -1,    -1,   210,   211,    -1,    -1,   179,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   189,    -1,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   200,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   210,   211,    -1,
      -1,   115,   116,   117,   118,   119,    21,    22,   122,   123,
     124,   125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,
      -1,   135,    -1,   137,   138,    -1,    -1,   141,    -1,   143,
     144,   145,    -1,    -1,    -1,   149,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,   141,    -1,   143,   144,
     145,    21,    22,    -1,   149,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    21,    22,    -1,   149,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
     210,   211,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,    -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,
      -1,    -1,    -1,   143,   144,   145,    21,    22,    -1,   149,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,   181,    -1,    -1,   149,    21,    22,    -1,    -1,   189,
      -1,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,   177,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,   210,   211,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,
     135,    -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,   149,    21,    22,    -1,    -1,   115,
     116,   117,   118,   119,    -1,    -1,   122,   123,   124,   125,
      -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,   135,
      -1,   137,   138,    -1,    -1,    -1,   181,   143,   144,   145,
      21,    22,    -1,   149,   189,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   210,   211,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   181,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   189,    -1,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,   115,
     116,   117,   118,   119,   210,   211,   122,   123,   124,   125,
      -1,   127,   128,   129,   130,    -1,    -1,    -1,    -1,   135,
      -1,   137,   138,    -1,    -1,    -1,    -1,   143,   144,   145,
      21,    22,    -1,   149,   115,   116,   117,   118,   119,    -1,
      -1,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
      -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,    -1,
      -1,    -1,   143,   144,   145,   181,    -1,    -1,   149,    21,
      22,    -1,    -1,   189,    -1,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   210,   211,    -1,    -1,    -1,    -1,
     181,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,    -1,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,   210,
     211,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
      -1,    -1,    -1,    -1,   135,    -1,   137,   138,    -1,    -1,
      -1,    -1,   143,   144,   145,    -1,    -1,    -1,   149,    21,
      22,    -1,    -1,   115,   116,   117,   118,   119,    -1,    -1,
     122,   123,   124,   125,    -1,   127,   128,   129,   130,    -1,
      -1,    -1,    -1,   135,    -1,   137,   138,    -1,    -1,    -1,
     181,   143,   144,   145,    21,    22,    -1,   149,   189,    -1,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   210,
     211,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,    -1,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,   115,   116,   117,   118,   119,   210,   211,
     122,   123,   124,   125,    -1,   127,   128,   129,   130,    -1,
      -1,    -1,    -1,   135,    -1,   137,   138,    -1,    -1,    -1,
      -1,   143,   144,   145,    21,    22,    -1,   149,   115,   116,
     117,   118,   119,    -1,    -1,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,    -1,    -1,    -1,    -1,   135,    -1,
     137,   138,    -1,    21,    22,    -1,   143,   144,   145,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   189,    -1,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   210,   211,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   189,    -1,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,    -1,    -1,    -1,    -1,   115,   116,
     117,   118,   119,   210,   211,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,    -1,    -1,    -1,    -1,   135,    -1,
     137,   138,    -1,    21,    22,    -1,   143,   115,   116,   117,
     118,   119,    -1,    -1,   122,   123,   124,   125,    -1,   127,
     128,   129,   130,    -1,    -1,    -1,    -1,   135,    -1,   137,
     138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,    -1,    -1,    -1,    -1,    -1,    19,
      -1,    -1,    -1,   210,   211,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,   115,   116,   117,
     118,   119,   210,   211,   122,   123,   124,   125,    -1,   127,
     128,   129,   130,    -1,    -1,    -1,    -1,   135,    -1,   137,
     138,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,   194,   195,   196,   197,
     198,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   210,   211,    -1,    19,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   157,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,   169,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,    -1,    -1,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,   157,    93,    94,    95,    35,    -1,    98,    99,
     100,   101,    -1,    -1,   169,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   157,    -1,    -1,    -1,    -1,   128,   129,
      -1,    -1,    71,    -1,    73,   169,    75,    76,    77,    78,
      79,    -1,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,   157,    -1,    98,
      99,   100,   101,    -1,    -1,    -1,    -1,    -1,    -1,   169,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     210,   211,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     169
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   219,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   102,   103,   181,   200,   211,   220,   223,   229,   231,
     232,   237,   267,   271,   305,   383,   390,   394,   405,   450,
     455,   460,    19,    20,   169,   259,   260,   261,   162,   238,
     239,   169,   170,   171,   200,   233,   234,   235,   179,   391,
     169,   215,   222,    57,    63,   386,   386,   386,   142,   169,
     291,    34,    63,   135,   204,   213,   263,   264,   265,   266,
     291,   181,     5,     6,     8,    36,   402,    62,   381,   188,
     187,   190,   187,   234,    22,    57,   199,   210,   236,   386,
     387,   389,   387,   381,   461,   451,   456,   169,   142,   230,
     265,   265,   265,   213,   143,   144,   145,   187,   212,    57,
      63,   272,   274,    57,    63,   392,    57,    63,   403,    57,
      63,   382,    15,    16,   162,   167,   169,   172,   213,   225,
     260,   162,   239,   169,   233,   233,   169,   181,   180,   387,
     181,    57,    63,   221,   169,   169,   169,   169,   173,   228,
     214,   261,   265,   265,   265,   265,   275,   169,   393,   406,
     179,   384,   173,   174,   175,   224,    15,    16,   162,   167,
     169,   225,   257,   258,   236,   388,   181,   462,   452,   457,
     173,   214,    35,    71,    73,    75,    76,    77,    78,    79,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    93,    94,    95,    98,    99,   100,   101,   117,   118,
     169,   270,   273,   179,   190,   106,   400,   401,   379,   159,
     216,   262,   355,   173,   174,   175,   187,   214,   188,   179,
     179,   179,    21,    22,    38,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   127,   128,   129,   136,   137,   138,   139,
     140,   143,   144,   145,   146,   147,   148,   149,   189,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   203,
     204,   210,   211,    35,    35,   213,   268,   179,   276,   395,
      75,    79,    93,    94,    98,    99,   100,   101,   410,   169,
     407,   180,   380,   261,   260,   181,   216,   151,   169,   377,
     378,   257,    19,    25,    31,    41,    49,    64,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   155,   215,   291,   409,   411,   412,   415,   421,   449,
     463,   453,   458,   169,   169,   169,   212,    22,   169,   212,
     154,   214,   355,   365,   366,   190,   269,   279,   385,   179,
     190,   399,   179,   404,   355,   212,   260,   213,    43,   187,
     190,   193,   376,   194,   194,   194,   213,   194,   194,   213,
     194,   194,   194,   194,   194,   194,   213,   291,    33,    60,
      61,   123,   127,   189,   193,   196,   211,   217,   420,   191,
     414,   369,   372,   169,   136,   213,     7,    50,   304,   181,
     214,   449,     1,     5,     6,     9,    10,    11,    13,    15,
      16,    17,    18,    19,    25,    26,    27,    28,    29,    31,
      38,    39,    42,    44,    45,    48,    51,    52,    54,    55,
      58,    59,    65,    68,    69,    80,   102,   103,   104,   105,
     117,   118,   132,   133,   134,   150,   151,   152,   153,   154,
     156,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   170,   171,   172,   179,   180,   181,   196,   197,
     198,   203,   204,   211,   213,   215,   216,   227,   229,   240,
     241,   244,   247,   248,   250,   252,   253,   254,   255,   277,
     278,   280,   285,   290,   291,   292,   296,   297,   298,   299,
     300,   301,   302,   303,   305,   309,   310,   317,   320,   323,
     328,   331,   332,   334,   335,   336,   338,   343,   346,   347,
     354,   409,   465,   480,   491,   495,   508,   511,   169,   181,
     396,   397,   291,   361,   378,   212,    65,   104,   170,   285,
     347,   169,   169,   421,   126,   136,   188,   375,   422,   427,
     429,   347,   431,   425,   169,   416,   433,   435,   437,   439,
     441,   443,   445,   447,   347,   194,   213,    33,   193,    33,
     193,   211,   217,   212,   347,   211,   217,   421,   169,   181,
     464,   169,   181,   187,   367,   418,   449,   454,   169,   370,
     418,   459,   347,   151,   169,   374,   408,   365,   194,   194,
     347,   251,   194,   293,   411,   465,   213,   291,   194,     5,
     102,   103,   194,   213,   126,   290,   321,   332,   347,   276,
     194,   213,    61,   347,   213,   347,   169,   194,   194,   213,
     181,   194,   162,    58,   347,   213,   276,   194,   213,   194,
     194,   213,   194,   194,   126,   290,   347,   347,   347,   216,
     276,   323,   327,   327,   327,   213,   213,   213,   213,   213,
     213,    13,   421,    13,   421,    13,   347,   490,   506,   194,
     347,   194,   226,    13,   490,   507,    37,   347,   347,   347,
     347,   347,    13,    49,   321,   347,   321,   216,   181,   181,
     347,    10,   323,   329,   169,   213,   181,   181,   181,   181,
     181,    66,   306,   267,   131,   181,    21,    22,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   135,   136,   137,   138,   139,   140,   143,   144,
     145,   146,   147,   148,   149,   188,   189,   191,   192,   193,
     194,   195,   196,   197,   198,   199,   200,   210,   211,   329,
     213,   181,   188,   180,   385,   180,   211,   267,   362,   194,
     214,    43,   181,   375,   290,   347,   449,   449,   419,   449,
     214,   449,   449,   214,   169,   413,   449,   268,   449,   268,
     449,   268,   367,   368,   370,   371,   214,   424,   281,   321,
     212,   212,   347,   181,   180,   190,   418,   180,   190,   418,
     180,   214,   213,    43,   126,   187,   188,   190,   193,   373,
     481,   483,   276,   408,   294,   213,   291,   194,   213,   318,
     194,   194,   194,   502,   321,   290,   321,   187,   107,   108,
     109,   110,   111,   112,   113,   114,   120,   121,   126,   139,
     140,   146,   147,   148,   188,    14,   421,   283,   507,   347,
     347,   276,   188,   311,   313,   347,   315,   190,   162,   347,
     504,   321,   487,   492,   321,   485,   421,   290,   347,   216,
     267,   347,   347,   347,   347,   347,   347,   408,    53,   157,
     169,   196,   211,   213,   347,   466,   469,   473,   489,   494,
     408,   213,   469,   494,   408,   141,   180,   181,   182,   474,
     286,   276,   288,   175,   176,   224,   408,   187,   510,   179,
     408,    13,   187,   510,   510,   151,   156,   194,   291,   337,
     276,   249,   330,    70,   211,   214,   321,   466,   468,   159,
     213,   308,   378,     4,   159,   326,   327,    19,   157,   169,
     409,    19,   157,   169,   409,   132,   133,   134,   277,   333,
     347,   333,   347,   333,   347,   333,   347,   333,   347,   333,
     347,   333,   347,   333,   347,   347,   347,   347,   333,   347,
     333,   347,   347,   347,   347,   169,   333,   347,   347,   157,
     169,   347,   347,   347,   409,   347,   347,   347,   333,   347,
     333,   347,   347,   347,   347,   333,   347,   333,   347,   333,
     347,   347,   333,   347,    22,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   128,   129,   157,   169,
     210,   211,   344,   409,   347,   214,   321,   347,   398,   266,
       8,   355,   360,   421,   169,   290,   347,   181,   195,   195,
     195,   418,   195,   195,   181,   195,   195,   269,   195,   269,
     195,   269,   195,   418,   195,   418,   284,   449,   214,   510,
     212,   449,   449,   347,   169,   169,   449,   290,   347,   421,
     421,    20,   449,    70,   321,   468,   479,   194,   347,   169,
     347,   449,   496,   498,   500,   421,   510,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   276,   195,   418,   214,   510,
     214,   256,   421,   421,   214,   421,   214,   421,   510,   421,
     421,   510,   421,   195,   326,   214,   214,   214,   214,   214,
     214,    20,   327,   213,   136,   373,   211,   347,   214,   141,
     187,   181,   473,   184,   185,   212,   477,   187,   181,   184,
     212,   476,    20,   214,   473,   180,   183,   475,    20,   347,
     180,   490,   284,   284,   347,    20,   490,   180,   279,    20,
     408,   212,   214,   213,   213,   339,   341,    12,    23,    24,
     242,   243,   347,   267,   169,   214,   468,   466,   187,   214,
     214,   169,   307,   307,   213,   126,   136,   169,   188,   193,
     324,   325,   268,   194,   213,   194,   213,   327,   327,   327,
     213,   213,   212,    19,   157,   169,   409,   190,   157,   169,
     347,   213,   213,   157,   169,   347,     1,   213,   212,   214,
     181,   180,   212,    57,    63,   358,    67,   359,   181,   195,
     181,   423,   428,   430,   449,   432,   426,   169,   417,   434,
     195,   438,   195,   442,   195,   446,   367,   448,   370,   195,
     418,   214,    43,   373,   195,   195,   321,   195,   468,   214,
     214,   214,   169,   214,   181,   195,   214,   195,   421,   421,
     421,   195,   214,   213,   421,   347,   195,   195,   195,   195,
     214,   195,   195,   214,   195,   326,   268,   213,   321,   347,
     347,   347,   469,   473,   347,   157,   169,   466,   477,   212,
     347,   489,   212,   321,   469,   180,   183,   186,   478,   212,
     321,   195,   195,   177,   321,   180,   321,    20,   347,   347,
     421,   268,   276,   347,    12,   245,   326,   214,   212,   211,
     187,   212,   214,   325,   169,   169,   213,   169,   169,   187,
     212,   269,   348,   347,   350,   347,   214,   321,   347,   194,
     213,   347,   213,   212,   347,   214,   321,   213,   212,   345,
     214,   321,   181,    47,   359,    46,   106,   356,   326,   436,
     440,   444,   213,   449,   169,   347,   482,   484,   276,   295,
     214,   195,   418,   169,   213,   319,   195,   195,   195,   503,
     283,   195,   312,   314,   316,   505,   488,   493,   486,   213,
     329,   269,   214,   321,   181,   214,   473,   477,   213,   136,
     373,   181,   473,   212,   181,   287,   289,   181,   181,   321,
     214,   214,   195,   269,   276,   246,   181,   268,   214,   466,
     169,   212,   190,   376,   214,   169,   324,   212,   141,   276,
     322,   421,   214,   449,   214,   214,   214,   352,   347,   347,
     214,   214,   347,   214,    33,   357,   356,   358,   281,   213,
     213,   347,   169,   195,   347,   497,   499,   501,   213,   214,
     213,   347,   347,   347,   213,    70,   479,   213,   213,   214,
     347,   322,   214,   347,   136,   373,   477,   347,   347,   347,
     347,   478,   490,   347,   213,   282,   490,   347,   181,   340,
     195,   243,    26,   105,   247,   297,   298,   299,   301,   347,
     269,   212,   190,   376,   421,   375,   214,   126,   347,   195,
     195,   449,   214,   214,   214,   363,   357,   374,   214,   479,
     479,   214,   195,   213,   214,   213,   213,   213,   281,   283,
     321,   479,   466,   467,   214,   181,   509,   347,   347,   214,
     509,   509,   281,   509,   509,   347,   337,   342,   126,   126,
     347,   276,   214,   421,   375,   375,   347,   347,   349,   351,
     195,   273,   364,   213,   466,   470,   471,   472,   472,   347,
     347,   479,   479,   467,   214,   214,   510,   472,   214,    53,
     212,   136,   373,   180,   180,   187,   510,   180,   212,   509,
     337,   347,   375,   347,   347,   181,   353,   181,   273,   466,
     187,   510,   214,   214,   214,   214,   472,   472,   214,   214,
     214,   347,   212,   347,   347,   212,   180,   214,   212,   347,
     181,   181,   276,   214,   213,   214,   214,   181,   466,   214
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   218,   219,   219,   219,   219,   219,   219,   219,   219,
     219,   219,   219,   219,   219,   219,   219,   220,   221,   221,
     221,   222,   222,   223,   224,   224,   224,   224,   225,   225,
     226,   226,   226,   227,   228,   228,   230,   229,   231,   232,
     233,   233,   233,   234,   234,   234,   234,   235,   235,   236,
     236,   237,   238,   238,   239,   239,   240,   241,   241,   242,
     242,   243,   243,   243,   244,   244,   245,   246,   245,   247,
     247,   247,   247,   247,   248,   249,   248,   251,   250,   252,
     253,   254,   256,   255,   257,   257,   257,   257,   257,   257,
     258,   258,   259,   259,   259,   260,   260,   260,   260,   260,
     260,   260,   260,   261,   261,   262,   262,   262,   263,   263,
     263,   264,   264,   265,   265,   265,   265,   265,   265,   265,
     266,   266,   267,   267,   268,   268,   268,   269,   269,   270,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   271,   272,   272,   272,   273,   275,
     274,   276,   276,   277,   277,   277,   278,   278,   278,   278,
     278,   278,   278,   278,   278,   278,   278,   278,   278,   278,
     278,   278,   278,   278,   278,   278,   278,   279,   279,   279,
     280,   281,   281,   282,   282,   283,   283,   284,   284,   286,
     287,   285,   288,   289,   285,   290,   290,   290,   290,   290,
     291,   291,   291,   292,   292,   294,   295,   293,   293,   296,
     296,   296,   296,   296,   296,   297,   298,   299,   299,   299,
     300,   300,   300,   301,   301,   302,   302,   302,   303,   304,
     304,   304,   305,   305,   306,   306,   307,   307,   308,   308,
     308,   308,   309,   309,   311,   312,   310,   313,   314,   310,
     315,   316,   310,   318,   319,   317,   320,   320,   320,   320,
     320,   320,   321,   321,   322,   322,   322,   323,   323,   323,
     324,   324,   324,   324,   324,   325,   325,   326,   326,   326,
     327,   327,   328,   330,   329,   331,   331,   331,   331,   331,
     331,   331,   332,   332,   332,   332,   332,   332,   332,   332,
     332,   332,   332,   332,   332,   332,   332,   332,   332,   332,
     332,   333,   333,   333,   333,   334,   334,   334,   334,   334,
     334,   334,   334,   334,   334,   334,   334,   334,   334,   334,
     334,   334,   335,   335,   336,   336,   337,   337,   338,   339,
     340,   338,   341,   342,   338,   343,   343,   343,   343,   343,
     343,   344,   345,   343,   346,   346,   346,   346,   346,   346,
     346,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   348,   349,   347,
     347,   347,   347,   350,   351,   347,   347,   347,   352,   353,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   354,   354,   354,   354,   354,
     354,   354,   354,   354,   354,   354,   354,   354,   354,   354,
     354,   355,   355,   355,   356,   356,   356,   357,   357,   358,
     358,   358,   359,   359,   360,   361,   362,   361,   363,   361,
     364,   361,   361,   365,   365,   366,   366,   367,   367,   368,
     368,   369,   369,   369,   370,   371,   371,   372,   372,   372,
     373,   373,   374,   374,   374,   374,   374,   374,   375,   375,
     375,   376,   376,   377,   377,   377,   377,   377,   378,   378,
     378,   378,   378,   379,   380,   379,   381,   381,   382,   382,
     382,   383,   384,   383,   385,   385,   385,   385,   386,   386,
     386,   388,   387,   389,   389,   390,   391,   390,   392,   392,
     392,   393,   395,   396,   394,   397,   398,   394,   399,   399,
     400,   400,   401,   402,   402,   403,   403,   403,   404,   404,
     406,   407,   405,   408,   408,   408,   408,   408,   409,   409,
     409,   409,   409,   409,   409,   409,   409,   409,   409,   409,
     409,   409,   409,   409,   409,   409,   409,   409,   409,   409,
     409,   409,   409,   409,   409,   410,   410,   410,   410,   410,
     410,   410,   410,   411,   412,   412,   412,   413,   413,   414,
     414,   414,   416,   417,   415,   418,   418,   419,   419,   420,
     420,   421,   421,   421,   421,   421,   421,   422,   423,   421,
     421,   421,   424,   421,   421,   421,   421,   421,   421,   421,
     421,   421,   421,   421,   421,   421,   425,   426,   421,   421,
     427,   428,   421,   429,   430,   421,   431,   432,   421,   421,
     433,   434,   421,   435,   436,   421,   421,   437,   438,   421,
     439,   440,   421,   421,   441,   442,   421,   443,   444,   421,
     445,   446,   421,   447,   448,   421,   449,   449,   449,   451,
     452,   453,   454,   450,   456,   457,   458,   459,   455,   461,
     462,   463,   464,   460,   465,   465,   465,   465,   465,   466,
     466,   466,   466,   466,   466,   466,   466,   467,   468,   469,
     469,   470,   470,   471,   471,   472,   472,   473,   473,   474,
     474,   475,   475,   476,   476,   477,   477,   477,   478,   478,
     478,   479,   479,   480,   480,   480,   480,   480,   480,   481,
     482,   480,   483,   484,   480,   485,   486,   480,   487,   488,
     480,   489,   489,   489,   490,   490,   491,   492,   493,   491,
     494,   494,   495,   495,   495,   496,   497,   495,   498,   499,
     495,   500,   501,   495,   495,   502,   503,   495,   495,   504,
     505,   495,   506,   506,   507,   507,   508,   508,   508,   508,
     508,   509,   509,   510,   510,   511,   511,   511,   511,   511,
     511
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     0,     1,
       1,     1,     1,     4,     1,     1,     2,     2,     3,     2,
       0,     2,     4,     3,     1,     2,     0,     4,     2,     2,
       1,     1,     1,     1,     2,     3,     3,     2,     4,     0,
       1,     2,     1,     3,     1,     3,     3,     3,     2,     1,
       1,     0,     2,     4,     1,     1,     0,     0,     3,     1,
       1,     1,     1,     1,     4,     0,     6,     0,     6,     2,
       3,     3,     0,     5,     1,     1,     1,     1,     1,     1,
       1,     3,     1,     1,     1,     3,     3,     3,     3,     3,
       3,     1,     5,     1,     3,     2,     3,     2,     1,     1,
       1,     1,     4,     1,     2,     3,     3,     3,     3,     2,
       1,     3,     0,     3,     0,     2,     3,     0,     2,     1,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     3,     3,     2,     2,     3,     4,     3,
       2,     2,     2,     2,     2,     3,     3,     3,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     0,     1,     1,     3,     0,
       4,     3,     7,     2,     2,     6,     1,     1,     1,     1,
       2,     2,     1,     1,     1,     1,     1,     1,     2,     2,
       1,     1,     1,     1,     2,     2,     2,     0,     2,     2,
       3,     0,     2,     0,     4,     0,     2,     1,     3,     0,
       0,     7,     0,     0,     7,     3,     2,     2,     2,     1,
       1,     3,     2,     2,     3,     0,     0,     5,     1,     2,
       5,     5,     5,     6,     2,     1,     1,     1,     2,     3,
       2,     2,     3,     2,     3,     2,     2,     3,     4,     1,
       1,     0,     1,     1,     1,     0,     1,     3,     9,     8,
       8,     7,     3,     3,     0,     0,     7,     0,     0,     7,
       0,     0,     7,     0,     0,     6,     5,     8,    10,     5,
       8,    10,     1,     3,     1,     2,     3,     1,     1,     2,
       2,     2,     2,     2,     4,     1,     3,     0,     4,     4,
       1,     6,     6,     0,     7,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     6,     8,     5,     6,     1,     4,     3,     0,
       0,     8,     0,     0,     9,     3,     4,     5,     6,     5,
       6,     0,     0,     5,     3,     4,     4,     5,     4,     3,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     4,     4,     5,     4,     5,     3,     4,     1,     1,
       2,     4,     4,     7,     8,     3,     5,     0,     0,     8,
       3,     3,     3,     0,     0,     8,     3,     4,     0,     0,
       9,     4,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     2,     4,     1,     4,     4,     4,     4,     4,
       1,     6,     7,     6,     6,     7,     7,     6,     7,     6,
       6,     0,     4,     1,     0,     1,     1,     0,     1,     0,
       1,     1,     0,     1,     5,     0,     0,     4,     0,     9,
       0,    10,     5,     3,     4,     1,     3,     1,     3,     1,
       3,     0,     2,     3,     3,     1,     3,     0,     2,     3,
       1,     1,     1,     2,     3,     5,     3,     3,     1,     1,
       1,     0,     1,     1,     4,     3,     3,     5,     4,     6,
       5,     5,     4,     0,     0,     4,     0,     1,     0,     1,
       1,     6,     0,     6,     0,     2,     3,     5,     0,     1,
       1,     0,     5,     2,     3,     4,     0,     4,     0,     1,
       1,     1,     0,     0,     9,     0,     0,    11,     0,     2,
       0,     1,     3,     1,     1,     0,     1,     1,     0,     3,
       0,     0,     7,     1,     4,     3,     3,     5,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     4,     4,     1,     3,     0,
       2,     3,     0,     0,     6,     1,     1,     1,     3,     3,
       4,     1,     1,     1,     1,     2,     3,     0,     0,     6,
       4,     5,     0,     9,     4,     2,     2,     3,     2,     3,
       2,     2,     3,     3,     3,     2,     0,     0,     6,     2,
       0,     0,     6,     0,     0,     6,     0,     0,     6,     1,
       0,     0,     6,     0,     0,     7,     1,     0,     0,     6,
       0,     0,     7,     1,     0,     0,     6,     0,     0,     7,
       0,     0,     6,     0,     0,     6,     1,     3,     3,     0,
       0,     0,     0,    10,     0,     0,     0,     0,    10,     0,
       0,     0,     0,    10,     1,     1,     1,     1,     1,     3,
       3,     5,     5,     6,     6,     8,     8,     1,     1,     1,
       3,     3,     5,     1,     2,     1,     0,     0,     2,     2,
       1,     2,     1,     2,     1,     2,     1,     1,     2,     1,
       1,     0,     1,     5,     4,     6,     7,     5,     7,     0,
       0,    10,     0,     0,    10,     0,     0,     9,     0,     0,
       7,     1,     3,     3,     3,     1,     5,     0,     0,    10,
       1,     3,     4,     4,     4,     0,     0,    11,     0,     0,
      11,     0,     0,    10,     5,     0,     0,     9,     5,     0,
       0,    10,     1,     3,     1,     3,     4,     3,     4,     7,
       9,     0,     3,     0,     1,     9,    10,    10,    10,     9,
      10
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
            { delete ((*yyvaluep).fa); }
        break;

    case YYSYMBOL_annotation_declaration: /* annotation_declaration  */
            { delete ((*yyvaluep).fa); }
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

    case YYSYMBOL_function_argument_declaration: /* function_argument_declaration  */
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
            { if ( ((*yyvaluep).pEnum)->use_count()==1 ) delete ((*yyvaluep).pEnum); }
        break;

    case YYSYMBOL_enum_name: /* enum_name  */
            { if ( ((*yyvaluep).pEnum)->use_count()==1 ) delete ((*yyvaluep).pEnum); }
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
            { delete ((*yyvaluep).pNameList); }
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
            { delete ((*yyvaluep).pMakeStruct); }
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

  case 17: /* top_level_reader_macro: expr_reader "end of expression"  */
                             {
        delete (yyvsp[-1].pExpression);    // we do nothing, we don't even attemp to 'visit'
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

  case 23: /* module_declaration: "module" module_name optional_shared optional_public_or_private_module  */
                                                                                                      {
        yyextra->g_Program->thisModuleName = *(yyvsp[-2].s);
        yyextra->g_Program->thisModule->isPublic = (yyvsp[0].b);
        yyextra->g_Program->thisModule->isModule = true;
        if ( yyextra->g_Program->thisModule->name.empty() ) {
            yyextra->g_Program->thisModule->name = *(yyvsp[-2].s);
        } else if ( yyextra->g_Program->thisModule->name != *(yyvsp[-2].s) ){
            das_yyerror(scanner,"this module already has a name " + yyextra->g_Program->thisModule->name,tokAt(scanner,(yylsp[-2])),
                CompilationError::module_already_has_a_name);
        }
        if ( !yyextra->g_Program->policies.ignore_shared_modules ) {
            yyextra->g_Program->promoteToBuiltin = (yyvsp[-1].b);
        }
        delete (yyvsp[-2].s);
    }
    break;

  case 24: /* character_sequence: STRING_CHARACTER  */
                                                                                  { (yyval.s) = new string(); *(yyval.s) += (yyvsp[0].ch); }
    break;

  case 25: /* character_sequence: STRING_CHARACTER_ESC  */
                                                                                  { (yyval.s) = new string(); *(yyval.s) += "\\\\"; }
    break;

  case 26: /* character_sequence: character_sequence STRING_CHARACTER  */
                                                                                  { (yyval.s) = (yyvsp[-1].s); *(yyvsp[-1].s) += (yyvsp[0].ch); }
    break;

  case 27: /* character_sequence: character_sequence STRING_CHARACTER_ESC  */
                                                                                  { (yyval.s) = (yyvsp[-1].s); *(yyvsp[-1].s) += "\\\\"; }
    break;

  case 28: /* string_constant: "start of the string" character_sequence "end of the string"  */
                                                           { (yyval.s) = (yyvsp[-1].s); }
    break;

  case 29: /* string_constant: "start of the string" "end of the string"  */
                                                           { (yyval.s) = new string(); }
    break;

  case 30: /* string_builder_body: %empty  */
        {
        (yyval.pExpression) = new ExprStringBuilder();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 31: /* string_builder_body: string_builder_body character_sequence  */
                                                                                  {
        bool err;
        auto esconst = unescapeString(*(yyvsp[0].s),&err);
        if ( err ) das_yyerror(scanner,"invalid escape sequence",tokAt(scanner,(yylsp[-1])), CompilationError::invalid_escape_sequence);
        auto sc = make_smart<ExprConstString>(tokAt(scanner,(yylsp[0])),esconst);
        delete (yyvsp[0].s);
        static_cast<ExprStringBuilder *>((yyvsp[-1].pExpression))->elements.push_back(sc);
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 32: /* string_builder_body: string_builder_body "{" expr "}"  */
                                                                                                        {
        auto se = ExpressionPtr((yyvsp[-1].pExpression));
        static_cast<ExprStringBuilder *>((yyvsp[-3].pExpression))->elements.push_back(se);
        (yyval.pExpression) = (yyvsp[-3].pExpression);
    }
    break;

  case 33: /* string_builder: "start of the string" string_builder_body "end of the string"  */
                                                                   {
        auto strb = static_cast<ExprStringBuilder *>((yyvsp[-1].pExpression));
        if ( strb->elements.size()==0 ) {
            (yyval.pExpression) = new ExprConstString(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),"");
            delete (yyvsp[-1].pExpression);
        } else if ( strb->elements.size()==1 && strb->elements[0]->rtti_isStringConstant() ) {
            auto sconst = static_pointer_cast<ExprConstString>(strb->elements[0]);
            (yyval.pExpression) = new ExprConstString(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),sconst->text);
            delete (yyvsp[-1].pExpression);
        } else {
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    }
    break;

  case 34: /* reader_character_sequence: STRING_CHARACTER  */
                               {
        if ( !yyextra->g_ReaderMacro->accept(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, (yyvsp[0].ch), tokAt(scanner,(yylsp[0]))) ) {
            das_yyend_reader(scanner);
        }
    }
    break;

  case 35: /* reader_character_sequence: reader_character_sequence STRING_CHARACTER  */
                                                                {
        if ( !yyextra->g_ReaderMacro->accept(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, (yyvsp[0].ch), tokAt(scanner,(yylsp[0]))) ) {
            das_yyend_reader(scanner);
        }
    }
    break;

  case 36: /* $@1: %empty  */
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
            yyextra->g_ReaderMacro = macros.back().get();
            yyextra->g_ReaderExpr = new ExprReader(tokAt(scanner,(yylsp[-1])),yyextra->g_ReaderMacro);
            yyclearin ;
            das_yybegin_reader(scanner);
        }
    }
    break;

  case 37: /* expr_reader: '%' name_in_namespace $@1 reader_character_sequence  */
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

  case 38: /* options_declaration: "options" annotation_argument_list  */
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
        if ( yyextra->g_Program->options.size() ) {
            for ( auto & opt : *(yyvsp[0].aaList) ) {
                if ( yyextra->g_Access->isOptionAllowed(opt.name, yyextra->g_Program->thisModuleName) ) {
                    yyextra->g_Program->options.push_back(opt);
                } else {
                    das_yyerror(scanner,"option " + opt.name + " is not allowed here",
                        tokAt(scanner,(yylsp[0])), CompilationError::invalid_option);
                }
            }
        } else {
            swap ( yyextra->g_Program->options, *(yyvsp[0].aaList) );
        }
        delete (yyvsp[0].aaList);
    }
    break;

  case 40: /* keyword_or_name: "name"  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 41: /* keyword_or_name: "keyword"  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 42: /* keyword_or_name: "type function"  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 43: /* require_module_name: keyword_or_name  */
                              {
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 44: /* require_module_name: '%' require_module_name  */
                                     {
        *(yyvsp[0].s) = "%" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 45: /* require_module_name: require_module_name '.' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += ".";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 46: /* require_module_name: require_module_name '/' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += "/";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 47: /* require_module: require_module_name is_public_module  */
                                                         {
        ast_requireModule(scanner,(yyvsp[-1].s),nullptr,(yyvsp[0].b),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 48: /* require_module: require_module_name "as" "name" is_public_module  */
                                                                              {
        ast_requireModule(scanner,(yyvsp[-3].s),(yyvsp[-1].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 49: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 50: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 54: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 55: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 56: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 57: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 58: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 59: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 60: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 61: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 62: /* expression_else: "else" expression_block  */
                                                           { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 63: /* expression_else: elif_or_static_elif expr expression_block expression_else  */
                                                                                          {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-3])),ExpressionPtr((yyvsp[-2].pExpression)),
            ExpressionPtr((yyvsp[-1].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
        eite->isStatic = (yyvsp[-3].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 64: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 65: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 66: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 67: /* $@2: %empty  */
                      { yyextra->das_need_oxford_comma = true; }
    break;

  case 68: /* expression_else_one_liner: "else" $@2 expression_if_one_liner  */
                                                                                                 {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 69: /* expression_if_one_liner: expr  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 70: /* expression_if_one_liner: expression_return_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 71: /* expression_if_one_liner: expression_yield_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 72: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 73: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 74: /* expression_if_then_else: if_or_static_if expr expression_block expression_else  */
                                                                                      {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-3])),ExpressionPtr((yyvsp[-2].pExpression)),
            ExpressionPtr((yyvsp[-1].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
        eite->isStatic = (yyvsp[-3].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 75: /* $@3: %empty  */
                                                     { yyextra->das_need_oxford_comma = true; }
    break;

  case 76: /* expression_if_then_else: expression_if_one_liner "if" $@3 expr expression_else_one_liner "end of expression"  */
                                                                                                                                                   {
        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-4])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr(ast_wrapInBlock((yyvsp[-5].pExpression))),(yyvsp[-1].pExpression) ? ExpressionPtr(ast_wrapInBlock((yyvsp[-1].pExpression))) : nullptr);
    }
    break;

  case 77: /* $@4: %empty  */
                     { yyextra->das_need_oxford_comma=false; }
    break;

  case 78: /* expression_for_loop: "for" $@4 variable_name_with_pos_list "in" expr_list expression_block  */
                                                                                                                                                 {
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-3].pNameWithPosList),(yyvsp[-1].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 79: /* expression_unsafe: "unsafe" expression_block  */
                                                 {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-1])));
        pUnsafe->body = ExpressionPtr((yyvsp[0].pExpression));
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 80: /* expression_while_loop: "while" expr expression_block  */
                                                               {
        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-2])));
        pWhile->cond = ExpressionPtr((yyvsp[-1].pExpression));
        pWhile->body = ExpressionPtr((yyvsp[0].pExpression));
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 81: /* expression_with: "with" expr expression_block  */
                                                         {
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-2])));
        pWith->with = ExpressionPtr((yyvsp[-1].pExpression));
        pWith->body = ExpressionPtr((yyvsp[0].pExpression));
        (yyval.pExpression) = pWith;
    }
    break;

  case 82: /* $@5: %empty  */
                                        { yyextra->das_need_oxford_comma=true; }
    break;

  case 83: /* expression_with_alias: "assume" "name" '=' $@5 expr  */
                                                                                               {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-4])), *(yyvsp[-3].s), (yyvsp[0].pExpression) );
        delete (yyvsp[-3].s);
    }
    break;

  case 84: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 85: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 86: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 87: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 88: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 89: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 90: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 91: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 92: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 93: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 94: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 95: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 96: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 97: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 98: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 99: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 100: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 101: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 102: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                          {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[-4].s); }
    }
    break;

  case 103: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 104: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 105: /* metadata_argument_list: '@' annotation_argument  */
                                      {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 106: /* metadata_argument_list: metadata_argument_list '@' annotation_argument  */
                                                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 107: /* metadata_argument_list: metadata_argument_list "end of expression"  */
                                         {
        (yyval.aaList) = (yyvsp[-1].aaList);
    }
    break;

  case 108: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 109: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 110: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 111: /* annotation_declaration_basic: annotation_declaration_name  */
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

  case 112: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
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

  case 113: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 114: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 115: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation.get()))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation); }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::And,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 116: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
                                                                            {
        if ( !(yyvsp[-2].fa)->annotation || !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation.get()))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation); }
        if ( !(yyvsp[0].fa)->annotation || !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Or,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 117: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation.get()))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation); }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Xor,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 118: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 119: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 120: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 121: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 122: /* optional_annotation_list: %empty  */
                                        { (yyval.faList) = nullptr; }
    break;

  case 123: /* optional_annotation_list: '[' annotation_list ']'  */
                                        { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 124: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 125: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 126: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 127: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 128: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 129: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 130: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 131: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 132: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 133: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 134: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 135: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 136: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 137: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 138: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 139: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 140: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 141: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 142: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 143: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 144: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 145: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 146: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 147: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 148: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 149: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 150: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 151: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 152: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 153: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 154: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 155: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 156: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 157: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 158: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 159: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 160: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 161: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 162: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 163: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 164: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 165: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 166: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 167: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 168: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 169: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 170: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 171: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 172: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 173: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 174: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 175: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 176: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 177: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 178: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 179: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 180: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 181: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 182: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 183: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 184: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 185: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 186: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 187: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 188: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 189: /* function_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 190: /* function_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 191: /* function_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 192: /* function_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 193: /* function_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 194: /* function_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 195: /* function_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 196: /* function_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 197: /* function_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 198: /* function_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 199: /* function_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 200: /* function_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 201: /* function_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 202: /* function_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 203: /* function_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 204: /* function_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 205: /* function_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 206: /* function_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 207: /* function_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 208: /* function_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 209: /* function_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 210: /* function_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 211: /* function_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 212: /* function_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 213: /* function_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 214: /* global_function_declaration: optional_annotation_list "def" function_declaration  */
                                                                                {
        (yyvsp[0].pFuncDecl)->atDecl = tokRangeAt(scanner,(yylsp[-1]),(yylsp[0]));
        assignDefaultArguments((yyvsp[0].pFuncDecl));
        runFunctionAnnotations(scanner, yyextra, (yyvsp[0].pFuncDecl), (yyvsp[-2].faList), tokAt(scanner,(yylsp[-2])));
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

  case 215: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 216: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 217: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 218: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 219: /* $@6: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 220: /* function_declaration: optional_public_or_private_function $@6 function_declaration_header expression_block  */
                                                                {
        (yyvsp[-1].pFuncDecl)->body = ExpressionPtr((yyvsp[0].pExpression));
        (yyvsp[-1].pFuncDecl)->privateFunction = !(yyvsp[-3].b);
        (yyval.pFuncDecl) = (yyvsp[-1].pFuncDecl);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
        }
    }
    break;

  case 221: /* expression_block: "begin of code block" expressions "end of code block"  */
                                                   {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
    }
    break;

  case 222: /* expression_block: "begin of code block" expressions "end of code block" "finally" "begin of code block" expressions "end of code block"  */
                                                                                          {
        auto pB = (ExprBlock *) (yyvsp[-5].pExpression);
        auto pF = (ExprBlock *) (yyvsp[-1].pExpression);
        swap ( pB->finalList, pF->list );
        (yyval.pExpression) = (yyvsp[-5].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-6]),(yylsp[0]));
        delete (yyvsp[-1].pExpression);
    }
    break;

  case 223: /* expr_call_pipe: expr expr_full_block_assumed_piped  */
                                                      {
        if ( (yyvsp[-1].pExpression)->rtti_isCallLikeExpr() ) {
            ((ExprLooksLikeCall *)(yyvsp[-1].pExpression))->arguments.push_back(ExpressionPtr((yyvsp[0].pExpression)));
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else {
            (yyval.pExpression) = (yyvsp[-1].pExpression);
            delete (yyvsp[0].pExpression);
        }
    }
    break;

  case 224: /* expr_call_pipe: expression_keyword expr_full_block_assumed_piped  */
                                                                    {
        if ( (yyvsp[-1].pExpression)->rtti_isCallLikeExpr() ) {
            ((ExprLooksLikeCall *)(yyvsp[-1].pExpression))->arguments.push_back(ExpressionPtr((yyvsp[0].pExpression)));
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        } else {
            (yyval.pExpression) = (yyvsp[-1].pExpression);
            delete (yyvsp[0].pExpression);
        }
    }
    break;

  case 225: /* expr_call_pipe: "generator" '<' type_declaration_no_options '>' optional_capture_list expr_full_block_assumed_piped  */
                                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-3].pTypeDecl),(yyvsp[-1].pCaptList),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])));
    }
    break;

  case 226: /* expression_any: "end of expression"  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 227: /* expression_any: expr_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 228: /* expression_any: expr_keyword  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 229: /* expression_any: expr_assign_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 230: /* expression_any: expr_assign "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 231: /* expression_any: expression_delete "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 232: /* expression_any: expression_let  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 233: /* expression_any: expression_while_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 234: /* expression_any: expression_unsafe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 235: /* expression_any: expression_with  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 236: /* expression_any: expression_with_alias  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 237: /* expression_any: expression_for_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 238: /* expression_any: expression_break "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 239: /* expression_any: expression_continue "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 240: /* expression_any: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 241: /* expression_any: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 242: /* expression_any: expression_if_then_else  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 243: /* expression_any: expression_try_catch  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 244: /* expression_any: expression_label "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 245: /* expression_any: expression_goto "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 246: /* expression_any: "pass" "end of expression"  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 247: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 248: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        }
    }
    break;

  case 249: /* expressions: expressions error  */
                                 {
        delete (yyvsp[-1].pExpression); (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 250: /* expr_keyword: "keyword" expr expression_block  */
                                                           {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s));
        pCall->arguments.push_back(ExpressionPtr((yyvsp[-1].pExpression)));
        auto resT = new TypeDecl(Type::autoinfer);
        auto blk = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,resT,(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])));
        pCall->arguments.push_back(ExpressionPtr(blk));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 251: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 252: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 253: /* optional_expr_list_in_braces: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 254: /* optional_expr_list_in_braces: '(' optional_expr_list optional_comma ')'  */
                                                             { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 255: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 256: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 257: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 258: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 259: /* $@7: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 260: /* $@8: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 261: /* expression_keyword: "keyword" '<' $@7 type_declaration_no_options_list '>' $@8 expr  */
                                                                                                                                                     {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 262: /* $@9: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 263: /* $@10: %empty  */
                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 264: /* expression_keyword: "type function" '<' $@9 type_declaration_no_options_list '>' $@10 optional_expr_list_in_braces  */
                                                                                                                                                                                   {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 265: /* expr_pipe: expr_assign " <|" expr_block  */
                                                        {
        Expression * pipeCall = (yyvsp[-2].pExpression)->tail();
        if ( pipeCall->rtti_isCallLikeExpr() ) {
            auto pCall = (ExprLooksLikeCall *) pipeCall;
            pCall->arguments.push_back(ExpressionPtr((yyvsp[0].pExpression)));
            (yyval.pExpression) = (yyvsp[-2].pExpression);
        } else if ( pipeCall->rtti_isVar() ) {
            // a += b <| c
            auto pVar = (ExprVar *) pipeCall;
            auto pCall = yyextra->g_Program->makeCall(pVar->at,pVar->name);
            pCall->arguments.push_back(ExpressionPtr((yyvsp[0].pExpression)));
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

  case 266: /* expr_pipe: "@ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 267: /* expr_pipe: "@@ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 268: /* expr_pipe: "$ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 269: /* expr_pipe: expr_call_pipe  */
                             {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 270: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 271: /* name_in_namespace: "name" "::" "name"  */
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

  case 272: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 273: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 274: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[0].pExpression)));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 275: /* $@11: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 276: /* $@12: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 277: /* new_type_declaration: '<' $@11 type_declaration '>' $@12  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 278: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 279: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),TypeDeclPtr((yyvsp[0].pTypeDecl)),false);
    }
    break;

  case 280: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),TypeDeclPtr((yyvsp[-3].pTypeDecl)),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 281: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),TypeDeclPtr((yyvsp[-3].pTypeDecl)),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 282: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),ExpressionPtr((yyvsp[-1].pExpression)));
    }
    break;

  case 283: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),ExpressionPtr((yyvsp[-1].pExpression)));
    }
    break;

  case 284: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 285: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 286: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 287: /* expression_return_no_pipe: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 288: /* expression_return_no_pipe: "return" expr_list  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),sequenceToTuple((yyvsp[0].pExpression)));
    }
    break;

  case 289: /* expression_return_no_pipe: "return" "<-" expr_list  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),sequenceToTuple((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 290: /* expression_return: expression_return_no_pipe "end of expression"  */
                                              {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 291: /* expression_return: "return" expr_pipe  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 292: /* expression_return: "return" "<-" expr_pipe  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),ExpressionPtr((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 293: /* expression_yield_no_pipe: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 294: /* expression_yield_no_pipe: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),ExpressionPtr((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 295: /* expression_yield: expression_yield_no_pipe "end of expression"  */
                                             {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 296: /* expression_yield: "yield" expr_pipe  */
                                          {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 297: /* expression_yield: "yield" "<-" expr_pipe  */
                                                 {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),ExpressionPtr((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 298: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 299: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 300: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 301: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 302: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 303: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 304: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 305: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 306: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 307: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 308: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr "end of expression"  */
                                                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-7].pNameList),tokAt(scanner,(yylsp[-7])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 309: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr "end of expression"  */
                                                                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 310: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr "end of expression"  */
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

  case 311: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr "end of expression"  */
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

  case 312: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 313: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 314: /* $@13: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 315: /* $@14: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 316: /* expr_cast: "cast" '<' $@13 type_declaration_no_options '>' $@14 expr  */
                                                                                                                                                {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[0].pExpression)),TypeDeclPtr((yyvsp[-3].pTypeDecl)));
    }
    break;

  case 317: /* $@15: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 318: /* $@16: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 319: /* expr_cast: "upcast" '<' $@15 type_declaration_no_options '>' $@16 expr  */
                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[0].pExpression)),TypeDeclPtr((yyvsp[-3].pTypeDecl)));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 320: /* $@17: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 321: /* $@18: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 322: /* expr_cast: "reinterpret" '<' $@17 type_declaration_no_options '>' $@18 expr  */
                                                                                                                                                       {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[0].pExpression)),TypeDeclPtr((yyvsp[-3].pTypeDecl)));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 323: /* $@19: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 324: /* $@20: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 325: /* expr_type_decl: "type" '<' $@19 type_declaration '>' $@20  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),TypeDeclPtr((yyvsp[-2].pTypeDecl)));
    }
    break;

  case 326: /* expr_type_info: "typeinfo" '(' name_in_namespace expr ')'  */
                                                                         {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-2].s),ptd->typeexpr);
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-2].s),ExpressionPtr((yyvsp[-1].pExpression)));
            }
            delete (yyvsp[-2].s);
    }
    break;

  case 327: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" '>' expr ')'  */
                                                                                                {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-5].s),ptd->typeexpr,*(yyvsp[-3].s));
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-5].s),ExpressionPtr((yyvsp[-1].pExpression)),*(yyvsp[-3].s));
            }
            delete (yyvsp[-5].s);
            delete (yyvsp[-3].s);
    }
    break;

  case 328: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" c_or_s "name" '>' expr ')'  */
                                                                                                                        {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-7].s),ptd->typeexpr,*(yyvsp[-5].s),*(yyvsp[-3].s));
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-7].s),ExpressionPtr((yyvsp[-1].pExpression)),*(yyvsp[-5].s),*(yyvsp[-3].s));
            }
            delete (yyvsp[-7].s);
            delete (yyvsp[-5].s);
            delete (yyvsp[-3].s);
    }
    break;

  case 329: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
                                                                          {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-3].s),ptd->typeexpr);
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-4])),*(yyvsp[-3].s),ExpressionPtr((yyvsp[-1].pExpression)));
            }
            delete (yyvsp[-3].s);
    }
    break;

  case 330: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
                                                                                                {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-6].s),ptd->typeexpr,*(yyvsp[-4].s));
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-7])),*(yyvsp[-6].s),ExpressionPtr((yyvsp[-1].pExpression)),*(yyvsp[-4].s));
            }
            delete (yyvsp[-6].s);
            delete (yyvsp[-4].s);
    }
    break;

  case 331: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" "end of expression" "name" '>' '(' expr ')'  */
                                                                                                                     {
            if ( (yyvsp[-1].pExpression)->rtti_isTypeDecl() ) {
                auto ptd = (ExprTypeDecl *)(yyvsp[-1].pExpression);
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-8].s),ptd->typeexpr,*(yyvsp[-6].s),*(yyvsp[-4].s));
                delete (yyvsp[-1].pExpression);
            } else {
                (yyval.pExpression) = new ExprTypeInfo(tokAt(scanner,(yylsp[-9])),*(yyvsp[-8].s),ExpressionPtr((yyvsp[-1].pExpression)),*(yyvsp[-6].s),*(yyvsp[-4].s));
            }
            delete (yyvsp[-8].s);
            delete (yyvsp[-6].s);
            delete (yyvsp[-4].s);
    }
    break;

  case 332: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 333: /* expr_list: expr_list ',' expr  */
                                            {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 334: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 335: /* block_or_simple_block: "=>" expr  */
                                        {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[0].pExpression)));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 336: /* block_or_simple_block: "=>" "<-" expr  */
                                               {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[0].pExpression)));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 337: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 338: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 339: /* block_or_lambda: '@' '@'  */
                { (yyval.i) = 2;   /* local function */ }
    break;

  case 340: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 341: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 342: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 343: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 344: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 345: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 346: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 347: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 348: /* optional_capture_list: "[[" capture_list ']' ']'  */
                                         { (yyval.pCaptList) = (yyvsp[-2].pCaptList); }
    break;

  case 349: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 350: /* expr_block: expression_block  */
                                            {
        ExprBlock * closure = (ExprBlock *) (yyvsp[0].pExpression);
        (yyval.pExpression) = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),ExpressionPtr((yyvsp[0].pExpression)));
        closure->returnType = make_smart<TypeDecl>(Type::autoinfer);
    }
    break;

  case 351: /* expr_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 352: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 353: /* $@21: %empty  */
                             {  yyextra->das_need_oxford_comma = false; }
    break;

  case 354: /* expr_full_block_assumed_piped: block_or_lambda $@21 optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type expression_block  */
                                                                                       {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 355: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 356: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 357: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 358: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 359: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 360: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 361: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 362: /* expr_assign: expr  */
                                             { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 363: /* expr_assign: expr '=' expr  */
                                             { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 364: /* expr_assign: expr "<-" expr  */
                                             { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 365: /* expr_assign: expr ":=" expr  */
                                             { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 366: /* expr_assign: expr "&=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 367: /* expr_assign: expr "|=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 368: /* expr_assign: expr "^=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 369: /* expr_assign: expr "&&=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 370: /* expr_assign: expr "||=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 371: /* expr_assign: expr "^^=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 372: /* expr_assign: expr "+=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 373: /* expr_assign: expr "-=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 374: /* expr_assign: expr "*=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 375: /* expr_assign: expr "/=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 376: /* expr_assign: expr "%=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 377: /* expr_assign: expr "<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 378: /* expr_assign: expr ">>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 379: /* expr_assign: expr "<<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 380: /* expr_assign: expr ">>>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 381: /* expr_assign_pipe_right: "@ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 382: /* expr_assign_pipe_right: "@@ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 383: /* expr_assign_pipe_right: "$ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 384: /* expr_assign_pipe_right: expr_call_pipe  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 385: /* expr_assign_pipe: expr '=' expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 386: /* expr_assign_pipe: expr "<-" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 387: /* expr_assign_pipe: expr "&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 388: /* expr_assign_pipe: expr "|=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 389: /* expr_assign_pipe: expr "^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 390: /* expr_assign_pipe: expr "&&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 391: /* expr_assign_pipe: expr "||=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 392: /* expr_assign_pipe: expr "^^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 393: /* expr_assign_pipe: expr "+=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 394: /* expr_assign_pipe: expr "-=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 395: /* expr_assign_pipe: expr "*=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 396: /* expr_assign_pipe: expr "/=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 397: /* expr_assign_pipe: expr "%=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 398: /* expr_assign_pipe: expr "<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 399: /* expr_assign_pipe: expr ">>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 400: /* expr_assign_pipe: expr "<<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 401: /* expr_assign_pipe: expr ">>>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 402: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 403: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 404: /* expr_method_call: expr "->" "name" '(' ')'  */
                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 405: /* expr_method_call: expr "->" "name" '(' expr_list ')'  */
                                                                              {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 406: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 407: /* func_addr_name: "$i" '(' expr ')'  */
                                          {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 408: /* func_addr_expr: '@' '@' func_addr_name  */
                                          {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 409: /* $@22: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 410: /* $@23: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 411: /* func_addr_expr: '@' '@' '<' $@22 type_declaration_no_options '>' $@23 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value.get()));
        expr->funcType = TypeDeclPtr((yyvsp[-3].pTypeDecl));
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 412: /* $@24: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 413: /* $@25: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 414: /* func_addr_expr: '@' '@' '<' $@24 optional_function_argument_list optional_function_type '>' $@25 func_addr_name  */
                                                                                                                                                                                     {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value.get()));
        expr->funcType = make_smart<TypeDecl>(Type::tFunction);
        expr->funcType->firstType = TypeDeclPtr((yyvsp[-3].pTypeDecl));
        if ( (yyvsp[-4].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, expr->funcType.get(), (yyvsp[-4].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-4].pVarDeclList));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 415: /* expr_field: expr '.' "name"  */
                                              {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), ExpressionPtr((yyvsp[-2].pExpression)), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 416: /* expr_field: expr '.' '.' "name"  */
                                                  {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), ExpressionPtr((yyvsp[-3].pExpression)), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 417: /* expr_field: expr '.' "name" '(' ')'  */
                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 418: /* expr_field: expr '.' "name" '(' expr_list ')'  */
                                                                           {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 419: /* expr_field: expr '.' basic_type_declaration '(' ')'  */
                                                                        {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 420: /* expr_field: expr '.' basic_type_declaration '(' expr_list ')'  */
                                                                                             {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 421: /* $@26: %empty  */
                               { yyextra->das_suppress_errors=true; }
    break;

  case 422: /* $@27: %empty  */
                                                                            { yyextra->das_suppress_errors=false; }
    break;

  case 423: /* expr_field: expr '.' $@26 error $@27  */
                                                                                                                    {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), ExpressionPtr((yyvsp[-4].pExpression)), "");
        yyerrok;
    }
    break;

  case 424: /* expr_call: name_in_namespace '(' ')'  */
                                               {
            (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),*(yyvsp[-2].s));
            delete (yyvsp[-2].s);
    }
    break;

  case 425: /* expr_call: name_in_namespace '(' "uninitialized" ')'  */
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

  case 426: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
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

  case 427: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
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

  case 428: /* expr_call: name_in_namespace '(' expr_list ')'  */
                                                                    {
            (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),*(yyvsp[-3].s)),(yyvsp[-1].pExpression));
            delete (yyvsp[-3].s);
    }
    break;

  case 429: /* expr_call: basic_type_declaration '(' ')'  */
                                                    {
        (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-2].type)));
    }
    break;

  case 430: /* expr_call: basic_type_declaration '(' expr_list ')'  */
                                                                         {
        (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-3].type))),(yyvsp[-1].pExpression));
    }
    break;

  case 431: /* expr: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 432: /* expr: name_in_namespace  */
                                              { (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 433: /* expr: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 434: /* expr: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 435: /* expr: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 436: /* expr: make_decl  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 437: /* expr: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 438: /* expr: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 439: /* expr: expr_field  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 440: /* expr: expr_mtag  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 441: /* expr: '!' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 442: /* expr: '~' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 443: /* expr: '+' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 444: /* expr: '-' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 445: /* expr: expr "<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 446: /* expr: expr ">>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 447: /* expr: expr "<<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 448: /* expr: expr ">>>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 449: /* expr: expr '+' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 450: /* expr: expr '-' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 451: /* expr: expr '*' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 452: /* expr: expr '/' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 453: /* expr: expr '%' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 454: /* expr: expr '<' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 455: /* expr: expr '>' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 456: /* expr: expr "==" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 457: /* expr: expr "!=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 458: /* expr: expr "<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 459: /* expr: expr ">=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 460: /* expr: expr '&' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 461: /* expr: expr '|' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 462: /* expr: expr '^' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 463: /* expr: expr "&&" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 464: /* expr: expr "||" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 465: /* expr: expr "^^" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 466: /* expr: expr ".." expr  */
                                             {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back(ExpressionPtr((yyvsp[-2].pExpression)));
        itv->arguments.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = itv;
    }
    break;

  case 467: /* expr: "++" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 468: /* expr: "--" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 469: /* expr: expr "++"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 470: /* expr: expr "--"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 471: /* expr: '(' expr_list optional_comma ')'  */
                                                         {
            if ( (yyvsp[-2].pExpression)->rtti_isSequence() ) {
                auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-2])));
                mkt->values = sequenceToList((yyvsp[-2].pExpression));
                (yyval.pExpression) = mkt;
            } else if ( (yyvsp[-1].b) ) {
                auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-2])));
                mkt->values.push_back((yyvsp[-2].pExpression));
                (yyval.pExpression) = mkt;
            } else {
                (yyval.pExpression) = (yyvsp[-2].pExpression);
            }
        }
    break;

  case 472: /* expr: expr '[' expr ']'  */
                                                 { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[-3].pExpression)), ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 473: /* expr: expr '.' '[' expr ']'  */
                                                     { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[-4].pExpression)), ExpressionPtr((yyvsp[-1].pExpression)), true); }
    break;

  case 474: /* expr: expr "?[" expr ']'  */
                                                 { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[-3].pExpression)), ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 475: /* expr: expr '.' "?[" expr ']'  */
                                                     { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[-4].pExpression)), ExpressionPtr((yyvsp[-1].pExpression)), true); }
    break;

  case 476: /* expr: expr "?." "name"  */
                                                 { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), ExpressionPtr((yyvsp[-2].pExpression)), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 477: /* expr: expr '.' "?." "name"  */
                                                     { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), ExpressionPtr((yyvsp[-3].pExpression)), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 478: /* expr: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 479: /* expr: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 480: /* expr: '*' expr  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 481: /* expr: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 482: /* expr: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 483: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])));
    }
    break;

  case 484: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])));
    }
    break;

  case 485: /* expr: expr "??" expr  */
                                                   { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 486: /* expr: expr '?' expr ':' expr  */
                                                          {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",ExpressionPtr((yyvsp[-4].pExpression)),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
        }
    break;

  case 487: /* $@28: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 488: /* $@29: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 489: /* expr: expr "is" "type" '<' $@28 type_declaration_no_options '>' $@29  */
                                                                                                                                                       {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[-7].pExpression)),TypeDeclPtr((yyvsp[-2].pTypeDecl)));
    }
    break;

  case 490: /* expr: expr "is" basic_type_declaration  */
                                                               {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),vdecl);
    }
    break;

  case 491: /* expr: expr "is" "name"  */
                                              {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 492: /* expr: expr "as" "name"  */
                                              {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 493: /* $@30: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 494: /* $@31: %empty  */
                                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 495: /* expr: expr "as" "type" '<' $@30 type_declaration '>' $@31  */
                                                                                                                                            {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[-7].pExpression)),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 496: /* expr: expr "as" basic_type_declaration  */
                                                               {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),das_to_string((yyvsp[0].type)));
    }
    break;

  case 497: /* expr: expr '?' "as" "name"  */
                                                  {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-3].pExpression)),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 498: /* $@32: %empty  */
                                                   { yyextra->das_arrow_depth ++; }
    break;

  case 499: /* $@33: %empty  */
                                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 500: /* expr: expr '?' "as" "type" '<' $@32 type_declaration '>' $@33  */
                                                                                                                                                {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[-8].pExpression)),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 501: /* expr: expr '?' "as" basic_type_declaration  */
                                                                   {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-3].pExpression)),das_to_string((yyvsp[0].type)));
    }
    break;

  case 502: /* expr: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 503: /* expr: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 504: /* expr: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 505: /* expr: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 506: /* expr: expr_method_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 507: /* expr: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 508: /* expr: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 509: /* expr: expr "<|" expr  */
                                                { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 510: /* expr: expr "|>" expr  */
                                                { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 511: /* expr: expr "|>" basic_type_declaration  */
                                                          {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 512: /* expr: name_in_namespace "name"  */
                                                { (yyval.pExpression) = ast_NameName(scanner,(yyvsp[-1].s),(yyvsp[0].s),tokAt(scanner,(yylsp[-1])),tokAt(scanner,(yylsp[0]))); }
    break;

  case 513: /* expr: "unsafe" '(' expr ')'  */
                                         {
        (yyvsp[-1].pExpression)->alwaysSafe = true;
        (yyvsp[-1].pExpression)->userSaidItsSafe = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 514: /* expr: expression_keyword  */
                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 515: /* expr_mtag: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 516: /* expr_mtag: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 517: /* expr_mtag: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 518: /* expr_mtag: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 519: /* expr_mtag: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 520: /* expr_mtag: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 521: /* expr_mtag: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 522: /* expr_mtag: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 523: /* expr_mtag: expr '.' "$f" '(' expr ')'  */
                                                                {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[-5].pExpression)), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 524: /* expr_mtag: expr "?." "$f" '(' expr ')'  */
                                                                 {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[-5].pExpression)), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 525: /* expr_mtag: expr '.' '.' "$f" '(' expr ')'  */
                                                                    {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[-6].pExpression)), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 526: /* expr_mtag: expr '.' "?." "$f" '(' expr ')'  */
                                                                     {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[-6].pExpression)), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 527: /* expr_mtag: expr "as" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),ExpressionPtr((yyvsp[-5].pExpression)),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 528: /* expr_mtag: expr '?' "as" "$f" '(' expr ')'  */
                                                                       {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),ExpressionPtr((yyvsp[-6].pExpression)),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 529: /* expr_mtag: expr "is" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),ExpressionPtr((yyvsp[-5].pExpression)),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 530: /* expr_mtag: '@' '@' "$c" '(' expr ')'  */
                                                         {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 531: /* optional_field_annotation: %empty  */
                                                        { (yyval.aaList) = nullptr; }
    break;

  case 532: /* optional_field_annotation: "[[" annotation_argument_list ']' ']'  */
                                                        { (yyval.aaList) = (yyvsp[-2].aaList); /*this one is gone when BRABRA is disabled*/ }
    break;

  case 533: /* optional_field_annotation: metadata_argument_list  */
                                                        { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 534: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 535: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 536: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 537: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 538: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 539: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 540: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 541: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 542: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 543: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 544: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 545: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 546: /* $@34: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 547: /* struct_variable_declaration_list: struct_variable_declaration_list $@34 structure_variable_declaration "end of expression"  */
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

  case 548: /* $@35: %empty  */
                                                                                                                     {
                yyextra->das_force_oxford_comma=true;
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 549: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable "abstract" optional_constant $@35 function_declaration_header "end of expression"  */
                                                    {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 550: /* $@36: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 551: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@36 function_declaration_header expression_block  */
                                                                        {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-9].pVarDeclList),(yyvsp[-8].faList),(yyvsp[-5].b),(yyvsp[-6].b),(yyvsp[-4].i),(yyvsp[-3].b),(yyvsp[-1].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-7]),(yylsp[0])),tokAt(scanner,(yylsp[-8])));
            }
    break;

  case 552: /* struct_variable_declaration_list: struct_variable_declaration_list '[' annotation_list ']' "end of expression"  */
                                                                                 {
        das_yyerror(scanner,"structure field or class method annotation expected to remain on the same line with the field or the class",
            tokAt(scanner,(yylsp[-2])), CompilationError::syntax_error);
        delete (yyvsp[-2].faList);
        (yyval.pVarDeclList) = (yyvsp[-4].pVarDeclList);
    }
    break;

  case 553: /* function_argument_declaration: optional_field_annotation kwd_let_var_or_nothing variable_declaration  */
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

  case 554: /* function_argument_declaration: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition{"``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))});
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 555: /* function_argument_list: function_argument_declaration  */
                                                                                 { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 556: /* function_argument_list: function_argument_list "end of expression" function_argument_declaration  */
                                                                                 { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 557: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 558: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition{*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))});
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 559: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 560: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                          { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 561: /* tuple_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 562: /* tuple_alias_type_list: tuple_alias_type_list c_or_s  */
                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 563: /* tuple_alias_type_list: tuple_alias_type_list tuple_type c_or_s  */
                                                            {
        (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[-1].pVarDecl));
        /*
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tokName = tokAt(scanner,@decl);
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *($decl->pNameList) ) {
                    crd->afterVariantEntry(nl.name.c_str(), nl.at);
                }
            }
        }
        */
    }
    break;

  case 564: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition{*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))});
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 565: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 566: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 567: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 568: /* variant_alias_type_list: variant_alias_type_list c_or_s  */
                                           {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 569: /* variant_alias_type_list: variant_alias_type_list variant_type c_or_s  */
                                                                {
        (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[-1].pVarDecl));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tokName = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *((yyvsp[-1].pVarDecl)->pNameList) ) {
                    crd->afterVariantEntry(nl.name.c_str(), nl.at);
                }
            }
        }
    }
    break;

  case 570: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 571: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 572: /* variable_declaration: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 573: /* variable_declaration: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 574: /* variable_declaration: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 575: /* variable_declaration: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 576: /* variable_declaration: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 577: /* variable_declaration: variable_name_with_pos_list copy_or_move expr_pipe  */
                                                                            {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 578: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 579: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 580: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 581: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 582: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 583: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 584: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{"``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-1].pExpression))});
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 585: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
                                         {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))});
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 586: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 587: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition{*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))});
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 588: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options "end of expression"  */
                                                                                            {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 589: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr "end of expression"  */
                                                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 590: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 591: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr "end of expression"  */
                                                                                                          {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 592: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr_pipe  */
                                                                                                           {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-3]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 593: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 594: /* $@37: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 595: /* global_variable_declaration_list: global_variable_declaration_list $@37 optional_field_annotation let_variable_declaration  */
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

  case 596: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 597: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 598: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 599: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 600: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 601: /* global_let: kwd_let optional_shared optional_public_or_private_variable "begin of code block" global_variable_declaration_list "end of code block"  */
                                                                                                                                       {
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 602: /* $@38: %empty  */
                                                                                        {
        yyextra->das_force_oxford_comma=true;
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 603: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@38 optional_field_annotation let_variable_declaration  */
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

  case 604: /* enum_list: %empty  */
        {
        (yyval.pEnum) = new Enumeration();
    }
    break;

  case 605: /* enum_list: enum_list "end of expression"  */
                          {
        (yyval.pEnum) = (yyvsp[-1].pEnum);
    }
    break;

  case 606: /* enum_list: enum_list "name" "end of expression"  */
                                     {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        if ( !(yyvsp[-2].pEnum)->add(*(yyvsp[-1].s),nullptr,tokAt(scanner,(yylsp[-1]))) ) {
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
        (yyval.pEnum) = (yyvsp[-2].pEnum);
    }
    break;

  case 607: /* enum_list: enum_list "name" '=' expr "end of expression"  */
                                                     {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        if ( !(yyvsp[-4].pEnum)->add(*(yyvsp[-3].s),ExpressionPtr((yyvsp[-1].pExpression)),tokAt(scanner,(yylsp[-3]))) ) {
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
        (yyval.pEnum) = (yyvsp[-4].pEnum);
    }
    break;

  case 608: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 609: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 610: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 611: /* $@39: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 612: /* single_alias: optional_public_or_private_alias "name" $@39 '=' type_declaration  */
                                  {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        (yyvsp[0].pTypeDecl)->isPrivateAlias = !(yyvsp[-4].b);
        if ( (yyvsp[0].pTypeDecl)->baseType == Type::alias ) {
            das_yyerror(scanner,"alias cannot be defined in terms of another alias "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::invalid_type);
        }
        (yyvsp[0].pTypeDecl)->alias = *(yyvsp[-3].s);
        if ( !yyextra->g_Program->addAlias(TypeDeclPtr((yyvsp[0].pTypeDecl))) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterAlias((yyvsp[-3].s)->c_str(),pubename);
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 616: /* $@40: %empty  */
                    { yyextra->das_force_oxford_comma=true;}
    break;

  case 618: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 619: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 620: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 621: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 622: /* $@41: %empty  */
                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 623: /* $@42: %empty  */
                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
    }
    break;

  case 624: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name "begin of code block" $@41 enum_list $@42 "end of code block"  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-5].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-8].faList),tokAt(scanner,(yylsp[-8])),(yyvsp[-6].b),(yyvsp[-5].pEnum),(yyvsp[-2].pEnum),Type::tInt);
    }
    break;

  case 625: /* $@43: %empty  */
                                                                                                                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 626: /* $@44: %empty  */
                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
    }
    break;

  case 627: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name ':' enum_basic_type_declaration "begin of code block" $@43 enum_list $@44 "end of code block"  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-7].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-10].faList),tokAt(scanner,(yylsp[-10])),(yyvsp[-8].b),(yyvsp[-7].pEnum),(yyvsp[-2].pEnum),(yyvsp[-5].type));
    }
    break;

  case 628: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 629: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 630: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 631: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 632: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 633: /* class_or_struct: "class"  */
                    { (yyval.b) = true; }
    break;

  case 634: /* class_or_struct: "struct"  */
                    { (yyval.b) = false; }
    break;

  case 635: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 636: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 637: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 638: /* optional_struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 639: /* optional_struct_variable_declaration_list: "begin of code block" struct_variable_declaration_list "end of code block"  */
                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 640: /* $@45: %empty  */
                                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 641: /* $@46: %empty  */
                         { if ( (yyvsp[0].pStructure) ) { (yyvsp[0].pStructure)->isClass = (yyvsp[-3].b); (yyvsp[0].pStructure)->privateStructure = !(yyvsp[-2].b); } }
    break;

  case 642: /* structure_declaration: optional_annotation_list class_or_struct optional_public_or_private_structure $@45 structure_name $@46 optional_struct_variable_declaration_list  */
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

  case 643: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 644: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{"``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)});
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 645: /* variable_name_with_pos_list: "name" "aka" "name"  */
                                         {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))});
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 646: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 647: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition{*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))});
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 648: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 649: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 650: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 651: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 652: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 653: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 654: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 655: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 656: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 657: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 658: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 659: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 660: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 661: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 662: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 663: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 664: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 665: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 666: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 667: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 668: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 669: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 670: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 671: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 672: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 673: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 674: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 675: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 676: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 677: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 678: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 679: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 680: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 681: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 682: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 683: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 684: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 685: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 686: /* auto_type_declaration: "$t" '(' expr ')'  */
                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::alias);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = "``MACRO``TAG``";
        (yyval.pTypeDecl)->isTag = true;
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner, (yylsp[-1]));
        (yyval.pTypeDecl)->firstType->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 687: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 688: /* bitfield_bits: bitfield_bits "end of expression" "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 689: /* bitfield_alias_bits: %empty  */
        {
        auto pSL = new vector<string>();
        (yyval.pNameList) = pSL;

    }
    break;

  case 690: /* bitfield_alias_bits: bitfield_alias_bits "end of expression"  */
                                      {
        (yyval.pNameList) = (yyvsp[-1].pNameList);
    }
    break;

  case 691: /* bitfield_alias_bits: bitfield_alias_bits "name" "end of expression"  */
                                                 {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[-1].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[-1].s)->c_str(),atvname);
        }
        delete (yyvsp[-1].s);
    }
    break;

  case 692: /* $@47: %empty  */
                                     { yyextra->das_arrow_depth ++; }
    break;

  case 693: /* $@48: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 694: /* bitfield_type_declaration: "bitfield" '<' $@47 bitfield_bits '>' $@48  */
                                                                                                                             {
            (yyval.pTypeDecl) = new TypeDecl(Type::tBitfield);
            (yyval.pTypeDecl)->argNames = *(yyvsp[-2].pNameList);
            if ( (yyval.pTypeDecl)->argNames.size()>32 ) {
                das_yyerror(scanner,"only 32 different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-5])),
                    CompilationError::invalid_type);
            }
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
            delete (yyvsp[-2].pNameList);
    }
    break;

  case 697: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 698: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 699: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 700: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 701: /* type_declaration_no_options: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 702: /* type_declaration_no_options: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 703: /* type_declaration_no_options: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 704: /* type_declaration_no_options: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 705: /* type_declaration_no_options: type_declaration_no_options dim_list  */
                                                                {
        if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeDecl ) {
            das_yyerror(scanner,"type declaration can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_type);
        } else if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeMacro ) {
            das_yyerror(scanner,"macro can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_type);
        }
        (yyvsp[-1].pTypeDecl)->dim.insert((yyvsp[-1].pTypeDecl)->dim.begin(), (yyvsp[0].pTypeDecl)->dim.begin(), (yyvsp[0].pTypeDecl)->dim.end());
        (yyvsp[-1].pTypeDecl)->dimExpr.insert((yyvsp[-1].pTypeDecl)->dimExpr.begin(), (yyvsp[0].pTypeDecl)->dimExpr.begin(), (yyvsp[0].pTypeDecl)->dimExpr.end());
        (yyvsp[-1].pTypeDecl)->removeDim = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyvsp[0].pTypeDecl)->dimExpr.clear();
        delete (yyvsp[0].pTypeDecl);
    }
    break;

  case 706: /* type_declaration_no_options: type_declaration_no_options '[' ']'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->dim.push_back(TypeDecl::dimAuto);
        (yyvsp[-2].pTypeDecl)->dimExpr.push_back(nullptr);
        (yyvsp[-2].pTypeDecl)->removeDim = false;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 707: /* $@49: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 708: /* $@50: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 709: /* type_declaration_no_options: "type" '<' $@49 type_declaration '>' $@50  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 710: /* type_declaration_no_options: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 711: /* type_declaration_no_options: '$' name_in_namespace '(' optional_expr_list ')'  */
                                                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-3])), *(yyvsp[-3].s)));
        delete (yyvsp[-3].s);
    }
    break;

  case 712: /* $@51: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 713: /* type_declaration_no_options: '$' name_in_namespace '<' $@51 type_declaration_no_options_list '>' '(' optional_expr_list ')'  */
                                                                                                                                                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-4].pTypeDeclList),(yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-7])), *(yyvsp[-7].s)));
        delete (yyvsp[-7].s);
    }
    break;

  case 714: /* type_declaration_no_options: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 715: /* type_declaration_no_options: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 716: /* type_declaration_no_options: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 717: /* type_declaration_no_options: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 718: /* type_declaration_no_options: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 719: /* type_declaration_no_options: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 720: /* type_declaration_no_options: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 721: /* type_declaration_no_options: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 722: /* type_declaration_no_options: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 723: /* type_declaration_no_options: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 724: /* type_declaration_no_options: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 725: /* type_declaration_no_options: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-1].pTypeDecl));
    }
    break;

  case 726: /* $@52: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 727: /* $@53: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 728: /* type_declaration_no_options: "smart_ptr" '<' $@52 type_declaration '>' $@53  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 729: /* type_declaration_no_options: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = TypeDeclPtr((yyvsp[-1].pTypeDecl));
    }
    break;

  case 730: /* $@54: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 731: /* $@55: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 732: /* type_declaration_no_options: "array" '<' $@54 type_declaration '>' $@55  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 733: /* $@56: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 734: /* $@57: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 735: /* type_declaration_no_options: "table" '<' $@56 table_type_pair '>' $@57  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].aTypePair).firstType);
        (yyval.pTypeDecl)->secondType = TypeDeclPtr((yyvsp[-2].aTypePair).secondType);
    }
    break;

  case 736: /* $@58: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 737: /* $@59: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 738: /* type_declaration_no_options: "iterator" '<' $@58 type_declaration '>' $@59  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 739: /* type_declaration_no_options: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 740: /* $@60: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 741: /* $@61: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 742: /* type_declaration_no_options: "block" '<' $@60 type_declaration '>' $@61  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 743: /* $@62: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 744: /* $@63: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 745: /* type_declaration_no_options: "block" '<' $@62 optional_function_argument_list optional_function_type '>' $@63  */
                                                                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
        if ( (yyvsp[-3].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-3].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        }
    }
    break;

  case 746: /* type_declaration_no_options: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 747: /* $@64: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 748: /* $@65: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 749: /* type_declaration_no_options: "function" '<' $@64 type_declaration '>' $@65  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 750: /* $@66: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 751: /* $@67: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 752: /* type_declaration_no_options: "function" '<' $@66 optional_function_argument_list optional_function_type '>' $@67  */
                                                                                                                                                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
        if ( (yyvsp[-3].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-3].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        }
    }
    break;

  case 753: /* type_declaration_no_options: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 754: /* $@68: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 755: /* $@69: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 756: /* type_declaration_no_options: "lambda" '<' $@68 type_declaration '>' $@69  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 757: /* $@70: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 758: /* $@71: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 759: /* type_declaration_no_options: "lambda" '<' $@70 optional_function_argument_list optional_function_type '>' $@71  */
                                                                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
        if ( (yyvsp[-3].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-3].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        }
    }
    break;

  case 760: /* $@72: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 761: /* $@73: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 762: /* type_declaration_no_options: "tuple" '<' $@72 tuple_type_list '>' $@73  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 763: /* $@74: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 764: /* $@75: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 765: /* type_declaration_no_options: "variant" '<' $@74 variant_type_list '>' $@75  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 766: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 767: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 768: /* type_declaration: type_declaration '|' '#'  */
                                             {
        if ( (yyvsp[-2].pTypeDecl)->baseType==Type::option ) {
            (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
            (yyval.pTypeDecl)->argTypes.push_back(make_smart<TypeDecl>(*(yyvsp[-2].pTypeDecl)->argTypes.back()));
            (yyvsp[-2].pTypeDecl)->argTypes.back()->temporary ^= true;
        } else {
            (yyval.pTypeDecl) = new TypeDecl(Type::option);
            (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
            (yyval.pTypeDecl)->argTypes.push_back((yyvsp[-2].pTypeDecl));
            (yyval.pTypeDecl)->argTypes.push_back(make_smart<TypeDecl>(*(yyvsp[-2].pTypeDecl)));
            (yyval.pTypeDecl)->argTypes.back()->temporary ^= true;
        }
    }
    break;

  case 769: /* $@76: %empty  */
                                                          { yyextra->das_need_oxford_comma=false; }
    break;

  case 770: /* $@77: %empty  */
                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 771: /* $@78: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 772: /* $@79: %empty  */
                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
    }
    break;

  case 773: /* tuple_alias_declaration: "tuple" optional_public_or_private_alias $@76 "name" $@77 "begin of code block" $@78 tuple_alias_type_list $@79 "end of code block"  */
          {
        auto vtype = make_smart<TypeDecl>(Type::tTuple);
        vtype->alias = *(yyvsp[-6].s);
        vtype->at = tokAt(scanner,(yylsp[-6]));
        vtype->isPrivateAlias = !(yyvsp[-8].b);
        varDeclToTypeDecl(scanner, vtype.get(), (yyvsp[-2].pVarDeclList), true);
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

  case 774: /* $@80: %empty  */
                                                            { yyextra->das_need_oxford_comma=false; }
    break;

  case 775: /* $@81: %empty  */
                                                                                                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 776: /* $@82: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 777: /* $@83: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
    }
    break;

  case 778: /* variant_alias_declaration: "variant" optional_public_or_private_alias $@80 "name" $@81 "begin of code block" $@82 variant_alias_type_list $@83 "end of code block"  */
          {
        auto vtype = make_smart<TypeDecl>(Type::tVariant);
        vtype->alias = *(yyvsp[-6].s);
        vtype->at = tokAt(scanner,(yylsp[-6]));
        vtype->isPrivateAlias = !(yyvsp[-8].b);
        varDeclToTypeDecl(scanner, vtype.get(), (yyvsp[-2].pVarDeclList), true);
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

  case 779: /* $@84: %empty  */
                                                             { yyextra->das_need_oxford_comma=false; }
    break;

  case 780: /* $@85: %empty  */
                                                                                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 781: /* $@86: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 782: /* $@87: %empty  */
                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
    }
    break;

  case 783: /* bitfield_alias_declaration: "bitfield" optional_public_or_private_alias $@84 "name" $@85 "begin of code block" $@86 bitfield_alias_bits $@87 "end of code block"  */
          {
        auto btype = make_smart<TypeDecl>(Type::tBitfield);
        btype->alias = *(yyvsp[-6].s);
        btype->at = tokAt(scanner,(yylsp[-6]));
        btype->argNames = *(yyvsp[-2].pNameList);
        btype->isPrivateAlias = !(yyvsp[-8].b);
        if ( btype->argNames.size()>32 ) {
            das_yyerror(scanner,"only 32 different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-6])),
                CompilationError::invalid_type);
        }
        if ( !yyextra->g_Program->addAlias(btype) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-6].s),tokAt(scanner,(yylsp[-6])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfield((yyvsp[-6].s)->c_str(),atvname);
        }
        delete (yyvsp[-6].s);
        delete (yyvsp[-2].pNameList);
    }
    break;

  case 784: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 785: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 786: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 787: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 788: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 789: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),ExpressionPtr((yyvsp[0].pExpression)),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 790: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),ExpressionPtr((yyvsp[0].pExpression)),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 791: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),ExpressionPtr((yyvsp[0].pExpression)),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 792: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),ExpressionPtr((yyvsp[0].pExpression)),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 793: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",ExpressionPtr((yyvsp[0].pExpression)),(yyvsp[-1].b),false);
        mfd->tag = ExpressionPtr((yyvsp[-3].pExpression));
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 794: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",ExpressionPtr((yyvsp[0].pExpression)),false,true);
        mfd->tag = ExpressionPtr((yyvsp[-3].pExpression));
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 795: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",ExpressionPtr((yyvsp[0].pExpression)),(yyvsp[-1].b),false);
        mfd->tag = ExpressionPtr((yyvsp[-3].pExpression));
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 796: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",ExpressionPtr((yyvsp[0].pExpression)),false,true);
        mfd->tag = ExpressionPtr((yyvsp[-3].pExpression));
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 797: /* make_variant_dim: make_struct_fields  */
                                {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 798: /* make_struct_single: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 799: /* make_struct_dim: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 800: /* make_struct_dim: make_struct_dim "end of expression" make_struct_fields  */
                                                         {
        ((ExprMakeStruct *) (yyvsp[-2].pExpression))->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 801: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 802: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 803: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 804: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 805: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 806: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 807: /* optional_block: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 808: /* optional_block: "where" expr_block  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 821: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 822: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 823: /* make_struct_decl: "[[" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = TypeDeclPtr((yyvsp[-3].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-4]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 824: /* make_struct_decl: "[[" type_declaration_no_options optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->makeType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
        msd->block = (yyvsp[-1].pExpression);
        msd->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = msd;
    }
    break;

  case 825: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                   {
        auto msd = new ExprMakeStruct();
        msd->makeType = TypeDeclPtr((yyvsp[-4].pTypeDecl));
        msd->useInitializer = true;
        msd->block = (yyvsp[-1].pExpression);
        msd->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pExpression) = msd;
    }
    break;

  case 826: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = TypeDeclPtr((yyvsp[-5].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 827: /* make_struct_decl: "[{" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
                                                                                                                                {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = TypeDeclPtr((yyvsp[-3].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-4]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_array_move");
        tam->arguments.push_back(ExpressionPtr((yyvsp[-2].pExpression)));
        (yyval.pExpression) = tam;
    }
    break;

  case 828: /* make_struct_decl: "[{" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
                                                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = TypeDeclPtr((yyvsp[-5].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-6]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),"to_array_move");
        tam->arguments.push_back(ExpressionPtr((yyvsp[-2].pExpression)));
        (yyval.pExpression) = tam;
    }
    break;

  case 829: /* $@88: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 830: /* $@89: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 831: /* make_struct_decl: "struct" '<' $@88 type_declaration_no_options '>' $@89 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                            {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 832: /* $@90: %empty  */
                            { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 833: /* $@91: %empty  */
                                                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 834: /* make_struct_decl: "class" '<' $@90 type_declaration_no_options '>' $@91 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                          {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 835: /* $@92: %empty  */
                               { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 836: /* $@93: %empty  */
                                                                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 837: /* make_struct_decl: "variant" '<' $@92 type_declaration_no_options '>' $@93 '(' make_variant_dim ')'  */
                                                                                                                                                                                                           {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-8]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-5].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 838: /* $@94: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 839: /* $@95: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 840: /* make_struct_decl: "default" '<' $@94 type_declaration_no_options '>' $@95 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = TypeDeclPtr((yyvsp[-3].pTypeDecl));
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 841: /* make_tuple: expr  */
                  {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 842: /* make_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back(ExpressionPtr((yyvsp[-2].pExpression)));
        mt->values.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = mt;
    }
    break;

  case 843: /* make_tuple: make_tuple ',' expr  */
                                      {
        ExprMakeTuple * mt;
        if ( (yyvsp[-2].pExpression)->rtti_isMakeTuple() ) {
            mt = static_cast<ExprMakeTuple *>((yyvsp[-2].pExpression));
        } else {
            mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-2])));
            mt->values.push_back(ExpressionPtr((yyvsp[-2].pExpression)));
        }
        mt->values.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = mt;
    }
    break;

  case 844: /* make_map_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back(ExpressionPtr((yyvsp[-2].pExpression)));
        mt->values.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = mt;
    }
    break;

  case 845: /* make_map_tuple: expr  */
                 {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 846: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = make_smart<TypeDecl>(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 847: /* $@96: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 848: /* $@97: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 849: /* make_tuple_call: "tuple" '<' $@96 type_declaration_no_options '>' $@97 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                            {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceTuple = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 850: /* make_dim: make_tuple  */
                        {
        auto mka = new ExprMakeArray();
        mka->values.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = mka;
    }
    break;

  case 851: /* make_dim: make_dim "end of expression" make_tuple  */
                                          {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 852: /* make_dim_decl: '[' expr_list optional_comma ']'  */
                                                          {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        mka->gen2 = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_array_move");
        tam->arguments.push_back(mka);
        (yyval.pExpression) = tam;
    }
    break;

  case 853: /* make_dim_decl: "[[" type_declaration_no_options make_dim optional_trailing_semicolon_sqr_sqr  */
                                                                                                         {
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 854: /* make_dim_decl: "[{" type_declaration_no_options make_dim optional_trailing_semicolon_cur_sqr  */
                                                                                                         {
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_array_move");
        tam->arguments.push_back(ExpressionPtr((yyvsp[-1].pExpression)));
        (yyval.pExpression) = tam;
    }
    break;

  case 855: /* $@98: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 856: /* $@99: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 857: /* make_dim_decl: "array" "struct" '<' $@98 type_declaration_no_options '>' $@99 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-10]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-10])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 858: /* $@100: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 859: /* $@101: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 860: /* make_dim_decl: "array" "tuple" '<' $@100 type_declaration_no_options '>' $@101 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-10]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceTuple = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-10])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 861: /* $@102: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 862: /* $@103: %empty  */
                                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 863: /* make_dim_decl: "array" "variant" '<' $@102 type_declaration_no_options '>' $@103 '(' make_variant_dim ')'  */
                                                                                                                                                                               {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-5].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-9])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 864: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
                                                                   {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        mka->gen2 = true;
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_array_move");
        tam->arguments.push_back(mka);
        (yyval.pExpression) = tam;
    }
    break;

  case 865: /* $@104: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 866: /* $@105: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 867: /* make_dim_decl: "array" '<' $@104 type_declaration_no_options '>' $@105 '(' optional_expr_list ')'  */
                                                                                                                                                                        {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-8])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = TypeDeclPtr((yyvsp[-5].pTypeDecl));
            mka->gen2 = true;
            auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-8])),"to_array_move");
            tam->arguments.push_back(mka);
            (yyval.pExpression) = tam;
        } else {
            auto msd = new ExprMakeStruct();
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->makeType = make_smart<TypeDecl>(Type::tArray);
            msd->makeType->firstType = TypeDeclPtr((yyvsp[-5].pTypeDecl));
            msd->at = tokAt(scanner,(yylsp[-5]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 868: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 869: /* $@106: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 870: /* $@107: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 871: /* make_dim_decl: "fixed_array" '<' $@106 type_declaration_no_options '>' $@107 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 872: /* make_table: make_map_tuple  */
                            {
        auto mka = new ExprMakeArray();
        mka->values.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = mka;
    }
    break;

  case 873: /* make_table: make_table "end of expression" make_map_tuple  */
                                                {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 874: /* expr_map_tuple_list: make_map_tuple  */
                                {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 875: /* expr_map_tuple_list: expr_map_tuple_list ',' make_map_tuple  */
                                                                {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 876: /* make_table_decl: "begin of code block" expr_map_tuple_list optional_comma "end of code block"  */
                                                                    {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 877: /* make_table_decl: "{{" make_table optional_trailing_semicolon_cur_cur  */
                                                                          {
        auto mkt = make_smart<TypeDecl>(Type::autoinfer);
        mkt->dim.push_back(TypeDecl::dimAuto);
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = mkt;
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-2]));
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_table_move");
        ttm->arguments.push_back(ExpressionPtr((yyvsp[-1].pExpression)));
        (yyval.pExpression) = ttm;
    }
    break;

  case 878: /* make_table_decl: "table" '(' optional_expr_map_tuple_list ')'  */
                                                                       {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-1].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 879: /* make_table_decl: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
                                                                                                                 {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-6])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = TypeDeclPtr((yyvsp[-4].pTypeDecl));
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto msd = new ExprMakeStruct();
            msd->at = tokAt(scanner,(yylsp[-6]));
            msd->makeType = make_smart<TypeDecl>(Type::tTable);
            msd->makeType->firstType = TypeDeclPtr((yyvsp[-4].pTypeDecl));
            msd->makeType->secondType = make_smart<TypeDecl>(Type::tVoid);
            msd->at = tokAt(scanner,(yylsp[-6]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 880: /* make_table_decl: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
                                                                                                                                                             {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-8])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = make_smart<TypeDecl>(Type::tTuple);
            mka->makeType->argTypes.push_back((yyvsp[-6].pTypeDecl));
            mka->makeType->argTypes.push_back((yyvsp[-4].pTypeDecl));
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-8])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto msd = new ExprMakeStruct();
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->makeType = make_smart<TypeDecl>(Type::tTable);
            msd->makeType->firstType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
            msd->makeType->secondType = TypeDeclPtr((yyvsp[-4].pTypeDecl));
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 881: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 882: /* array_comprehension_where: "end of expression" "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 883: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 884: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 885: /* array_comprehension: '[' "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                    {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 886: /* array_comprehension: '[' "iterator" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                                 {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 887: /* array_comprehension: "[[" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']' ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),true,false);
    }
    break;

  case 888: /* array_comprehension: "[{" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where "end of code block" ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),false,false);
    }
    break;

  case 889: /* array_comprehension: "begin of code block" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where "end of code block"  */
                                                                                                                                                              {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
    }
    break;

  case 890: /* array_comprehension: "{{" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where "end of code block" "end of code block"  */
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
                CompilationError::syntax_error);
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


