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
  YYSYMBOL_DAS_TTUPLE = 102,               /* "tuple"  */
  YYSYMBOL_DAS_TVARIANT = 103,             /* "variant"  */
  YYSYMBOL_DAS_GENERATOR = 104,            /* "generator"  */
  YYSYMBOL_DAS_YIELD = 105,                /* "yield"  */
  YYSYMBOL_DAS_SEALED = 106,               /* "sealed"  */
  YYSYMBOL_DAS_TEMPLATE = 107,             /* "template"  */
  YYSYMBOL_ADDEQU = 108,                   /* "+="  */
  YYSYMBOL_SUBEQU = 109,                   /* "-="  */
  YYSYMBOL_DIVEQU = 110,                   /* "/="  */
  YYSYMBOL_MULEQU = 111,                   /* "*="  */
  YYSYMBOL_MODEQU = 112,                   /* "%="  */
  YYSYMBOL_ANDEQU = 113,                   /* "&="  */
  YYSYMBOL_OREQU = 114,                    /* "|="  */
  YYSYMBOL_XOREQU = 115,                   /* "^="  */
  YYSYMBOL_SHL = 116,                      /* "<<"  */
  YYSYMBOL_SHR = 117,                      /* ">>"  */
  YYSYMBOL_ADDADD = 118,                   /* "++"  */
  YYSYMBOL_SUBSUB = 119,                   /* "--"  */
  YYSYMBOL_LEEQU = 120,                    /* "<="  */
  YYSYMBOL_SHLEQU = 121,                   /* "<<="  */
  YYSYMBOL_SHREQU = 122,                   /* ">>="  */
  YYSYMBOL_GREQU = 123,                    /* ">="  */
  YYSYMBOL_EQUEQU = 124,                   /* "=="  */
  YYSYMBOL_NOTEQU = 125,                   /* "!="  */
  YYSYMBOL_RARROW = 126,                   /* "->"  */
  YYSYMBOL_LARROW = 127,                   /* "<-"  */
  YYSYMBOL_QQ = 128,                       /* "??"  */
  YYSYMBOL_QDOT = 129,                     /* "?."  */
  YYSYMBOL_QBRA = 130,                     /* "?["  */
  YYSYMBOL_LPIPE = 131,                    /* "<|"  */
  YYSYMBOL_LBPIPE = 132,                   /* " <|"  */
  YYSYMBOL_LLPIPE = 133,                   /* "$ <|"  */
  YYSYMBOL_LAPIPE = 134,                   /* "@ <|"  */
  YYSYMBOL_LFPIPE = 135,                   /* "@@ <|"  */
  YYSYMBOL_RPIPE = 136,                    /* "|>"  */
  YYSYMBOL_CLONEEQU = 137,                 /* ":="  */
  YYSYMBOL_ROTL = 138,                     /* "<<<"  */
  YYSYMBOL_ROTR = 139,                     /* ">>>"  */
  YYSYMBOL_ROTLEQU = 140,                  /* "<<<="  */
  YYSYMBOL_ROTREQU = 141,                  /* ">>>="  */
  YYSYMBOL_MAPTO = 142,                    /* "=>"  */
  YYSYMBOL_COLCOL = 143,                   /* "::"  */
  YYSYMBOL_ANDAND = 144,                   /* "&&"  */
  YYSYMBOL_OROR = 145,                     /* "||"  */
  YYSYMBOL_XORXOR = 146,                   /* "^^"  */
  YYSYMBOL_ANDANDEQU = 147,                /* "&&="  */
  YYSYMBOL_OROREQU = 148,                  /* "||="  */
  YYSYMBOL_XORXOREQU = 149,                /* "^^="  */
  YYSYMBOL_DOTDOT = 150,                   /* ".."  */
  YYSYMBOL_MTAG_E = 151,                   /* "$$"  */
  YYSYMBOL_MTAG_I = 152,                   /* "$i"  */
  YYSYMBOL_MTAG_V = 153,                   /* "$v"  */
  YYSYMBOL_MTAG_B = 154,                   /* "$b"  */
  YYSYMBOL_MTAG_A = 155,                   /* "$a"  */
  YYSYMBOL_MTAG_T = 156,                   /* "$t"  */
  YYSYMBOL_MTAG_C = 157,                   /* "$c"  */
  YYSYMBOL_MTAG_F = 158,                   /* "$f"  */
  YYSYMBOL_MTAG_DOTDOTDOT = 159,           /* "..."  */
  YYSYMBOL_BRABRAB = 160,                  /* "[["  */
  YYSYMBOL_BRACBRB = 161,                  /* "[{"  */
  YYSYMBOL_CBRCBRB = 162,                  /* "{{"  */
  YYSYMBOL_OPEN_BRACE = 163,               /* "new scope"  */
  YYSYMBOL_CLOSE_BRACE = 164,              /* "close scope"  */
  YYSYMBOL_SEMICOLON = 165,                /* SEMICOLON  */
  YYSYMBOL_INTEGER = 166,                  /* "integer constant"  */
  YYSYMBOL_LONG_INTEGER = 167,             /* "long integer constant"  */
  YYSYMBOL_UNSIGNED_INTEGER = 168,         /* "unsigned integer constant"  */
  YYSYMBOL_UNSIGNED_LONG_INTEGER = 169,    /* "unsigned long integer constant"  */
  YYSYMBOL_UNSIGNED_INT8 = 170,            /* "unsigned int8 constant"  */
  YYSYMBOL_DAS_FLOAT = 171,                /* "floating point constant"  */
  YYSYMBOL_DOUBLE = 172,                   /* "double constant"  */
  YYSYMBOL_NAME = 173,                     /* "name"  */
  YYSYMBOL_KEYWORD = 174,                  /* "keyword"  */
  YYSYMBOL_TYPE_FUNCTION = 175,            /* "type function"  */
  YYSYMBOL_BEGIN_STRING = 176,             /* "start of the string"  */
  YYSYMBOL_STRING_CHARACTER = 177,         /* STRING_CHARACTER  */
  YYSYMBOL_STRING_CHARACTER_ESC = 178,     /* STRING_CHARACTER_ESC  */
  YYSYMBOL_END_STRING = 179,               /* "end of the string"  */
  YYSYMBOL_BEGIN_STRING_EXPR = 180,        /* "{"  */
  YYSYMBOL_END_STRING_EXPR = 181,          /* "}"  */
  YYSYMBOL_END_OF_READ = 182,              /* "end of failed eader macro"  */
  YYSYMBOL_183_begin_of_code_block_ = 183, /* "begin of code block"  */
  YYSYMBOL_184_end_of_code_block_ = 184,   /* "end of code block"  */
  YYSYMBOL_185_end_of_expression_ = 185,   /* "end of expression"  */
  YYSYMBOL_SEMICOLON_CUR_CUR = 186,        /* ";}}"  */
  YYSYMBOL_SEMICOLON_CUR_SQR = 187,        /* ";}]"  */
  YYSYMBOL_SEMICOLON_SQR_SQR = 188,        /* ";]]"  */
  YYSYMBOL_COMMA_SQR_SQR = 189,            /* ",]]"  */
  YYSYMBOL_COMMA_CUR_SQR = 190,            /* ",}]"  */
  YYSYMBOL_END_OF_EXPR = 191,              /* END_OF_EXPR  */
  YYSYMBOL_EMPTY = 192,                    /* EMPTY  */
  YYSYMBOL_193_ = 193,                     /* ','  */
  YYSYMBOL_194_ = 194,                     /* '='  */
  YYSYMBOL_195_ = 195,                     /* '?'  */
  YYSYMBOL_196_ = 196,                     /* ':'  */
  YYSYMBOL_197_ = 197,                     /* '|'  */
  YYSYMBOL_198_ = 198,                     /* '^'  */
  YYSYMBOL_199_ = 199,                     /* '&'  */
  YYSYMBOL_200_ = 200,                     /* '<'  */
  YYSYMBOL_201_ = 201,                     /* '>'  */
  YYSYMBOL_202_ = 202,                     /* '-'  */
  YYSYMBOL_203_ = 203,                     /* '+'  */
  YYSYMBOL_204_ = 204,                     /* '*'  */
  YYSYMBOL_205_ = 205,                     /* '/'  */
  YYSYMBOL_206_ = 206,                     /* '%'  */
  YYSYMBOL_UNARY_MINUS = 207,              /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 208,               /* UNARY_PLUS  */
  YYSYMBOL_209_ = 209,                     /* '~'  */
  YYSYMBOL_210_ = 210,                     /* '!'  */
  YYSYMBOL_PRE_INC = 211,                  /* PRE_INC  */
  YYSYMBOL_PRE_DEC = 212,                  /* PRE_DEC  */
  YYSYMBOL_POST_INC = 213,                 /* POST_INC  */
  YYSYMBOL_POST_DEC = 214,                 /* POST_DEC  */
  YYSYMBOL_DEREF = 215,                    /* DEREF  */
  YYSYMBOL_216_ = 216,                     /* '.'  */
  YYSYMBOL_217_ = 217,                     /* '['  */
  YYSYMBOL_218_ = 218,                     /* ']'  */
  YYSYMBOL_219_ = 219,                     /* '('  */
  YYSYMBOL_220_ = 220,                     /* ')'  */
  YYSYMBOL_221_ = 221,                     /* '$'  */
  YYSYMBOL_222_ = 222,                     /* '@'  */
  YYSYMBOL_223_ = 223,                     /* '#'  */
  YYSYMBOL_YYACCEPT = 224,                 /* $accept  */
  YYSYMBOL_program = 225,                  /* program  */
  YYSYMBOL_semicolon = 226,                /* semicolon  */
  YYSYMBOL_top_level_reader_macro = 227,   /* top_level_reader_macro  */
  YYSYMBOL_optional_public_or_private_module = 228, /* optional_public_or_private_module  */
  YYSYMBOL_module_name = 229,              /* module_name  */
  YYSYMBOL_optional_not_required = 230,    /* optional_not_required  */
  YYSYMBOL_module_declaration = 231,       /* module_declaration  */
  YYSYMBOL_character_sequence = 232,       /* character_sequence  */
  YYSYMBOL_string_constant = 233,          /* string_constant  */
  YYSYMBOL_format_string = 234,            /* format_string  */
  YYSYMBOL_optional_format_string = 235,   /* optional_format_string  */
  YYSYMBOL_236_1 = 236,                    /* $@1  */
  YYSYMBOL_string_builder_body = 237,      /* string_builder_body  */
  YYSYMBOL_string_builder = 238,           /* string_builder  */
  YYSYMBOL_reader_character_sequence = 239, /* reader_character_sequence  */
  YYSYMBOL_expr_reader = 240,              /* expr_reader  */
  YYSYMBOL_241_2 = 241,                    /* $@2  */
  YYSYMBOL_options_declaration = 242,      /* options_declaration  */
  YYSYMBOL_require_declaration = 243,      /* require_declaration  */
  YYSYMBOL_keyword_or_name = 244,          /* keyword_or_name  */
  YYSYMBOL_require_module_name = 245,      /* require_module_name  */
  YYSYMBOL_require_module = 246,           /* require_module  */
  YYSYMBOL_is_public_module = 247,         /* is_public_module  */
  YYSYMBOL_expect_declaration = 248,       /* expect_declaration  */
  YYSYMBOL_expect_list = 249,              /* expect_list  */
  YYSYMBOL_expect_error = 250,             /* expect_error  */
  YYSYMBOL_expression_label = 251,         /* expression_label  */
  YYSYMBOL_expression_goto = 252,          /* expression_goto  */
  YYSYMBOL_elif_or_static_elif = 253,      /* elif_or_static_elif  */
  YYSYMBOL_expression_else = 254,          /* expression_else  */
  YYSYMBOL_if_or_static_if = 255,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 256, /* expression_else_one_liner  */
  YYSYMBOL_257_3 = 257,                    /* $@3  */
  YYSYMBOL_expression_if_one_liner = 258,  /* expression_if_one_liner  */
  YYSYMBOL_expression_if_then_else = 259,  /* expression_if_then_else  */
  YYSYMBOL_260_4 = 260,                    /* $@4  */
  YYSYMBOL_expression_for_loop = 261,      /* expression_for_loop  */
  YYSYMBOL_262_5 = 262,                    /* $@5  */
  YYSYMBOL_263_6 = 263,                    /* $@6  */
  YYSYMBOL_expression_unsafe = 264,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 265,    /* expression_while_loop  */
  YYSYMBOL_expression_with = 266,          /* expression_with  */
  YYSYMBOL_expression_with_alias = 267,    /* expression_with_alias  */
  YYSYMBOL_268_7 = 268,                    /* $@7  */
  YYSYMBOL_269_8 = 269,                    /* $@8  */
  YYSYMBOL_annotation_argument_value = 270, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 271, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 272, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 273,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 274, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 275,   /* metadata_argument_list  */
  YYSYMBOL_annotation_declaration_name = 276, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 277, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 278,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 279,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 280, /* optional_annotation_list  */
  YYSYMBOL_optional_function_argument_list = 281, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 282,   /* optional_function_type  */
  YYSYMBOL_function_name = 283,            /* function_name  */
  YYSYMBOL_optional_template = 284,        /* optional_template  */
  YYSYMBOL_global_function_declaration = 285, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 286, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 287, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 288,     /* function_declaration  */
  YYSYMBOL_289_9 = 289,                    /* $@9  */
  YYSYMBOL_open_block = 290,               /* open_block  */
  YYSYMBOL_close_block = 291,              /* close_block  */
  YYSYMBOL_expression_block = 292,         /* expression_block  */
  YYSYMBOL_expr_call_pipe = 293,           /* expr_call_pipe  */
  YYSYMBOL_expression_any = 294,           /* expression_any  */
  YYSYMBOL_expressions = 295,              /* expressions  */
  YYSYMBOL_expr_keyword = 296,             /* expr_keyword  */
  YYSYMBOL_optional_expr_list = 297,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_list_in_braces = 298, /* optional_expr_list_in_braces  */
  YYSYMBOL_optional_expr_map_tuple_list = 299, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 300, /* type_declaration_no_options_list  */
  YYSYMBOL_expression_keyword = 301,       /* expression_keyword  */
  YYSYMBOL_302_10 = 302,                   /* $@10  */
  YYSYMBOL_303_11 = 303,                   /* $@11  */
  YYSYMBOL_304_12 = 304,                   /* $@12  */
  YYSYMBOL_305_13 = 305,                   /* $@13  */
  YYSYMBOL_expr_pipe = 306,                /* expr_pipe  */
  YYSYMBOL_name_in_namespace = 307,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 308,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 309,     /* new_type_declaration  */
  YYSYMBOL_310_14 = 310,                   /* $@14  */
  YYSYMBOL_311_15 = 311,                   /* $@15  */
  YYSYMBOL_expr_new = 312,                 /* expr_new  */
  YYSYMBOL_expression_break = 313,         /* expression_break  */
  YYSYMBOL_expression_continue = 314,      /* expression_continue  */
  YYSYMBOL_expression_return_no_pipe = 315, /* expression_return_no_pipe  */
  YYSYMBOL_expression_return = 316,        /* expression_return  */
  YYSYMBOL_expression_yield_no_pipe = 317, /* expression_yield_no_pipe  */
  YYSYMBOL_expression_yield = 318,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 319,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 320,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 321,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 322,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 323,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 324, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 325,           /* expression_let  */
  YYSYMBOL_expr_cast = 326,                /* expr_cast  */
  YYSYMBOL_327_16 = 327,                   /* $@16  */
  YYSYMBOL_328_17 = 328,                   /* $@17  */
  YYSYMBOL_329_18 = 329,                   /* $@18  */
  YYSYMBOL_330_19 = 330,                   /* $@19  */
  YYSYMBOL_331_20 = 331,                   /* $@20  */
  YYSYMBOL_332_21 = 332,                   /* $@21  */
  YYSYMBOL_expr_type_decl = 333,           /* expr_type_decl  */
  YYSYMBOL_334_22 = 334,                   /* $@22  */
  YYSYMBOL_335_23 = 335,                   /* $@23  */
  YYSYMBOL_expr_type_info = 336,           /* expr_type_info  */
  YYSYMBOL_expr_list = 337,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 338,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 339,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 340,            /* capture_entry  */
  YYSYMBOL_capture_list = 341,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 342,    /* optional_capture_list  */
  YYSYMBOL_expr_block = 343,               /* expr_block  */
  YYSYMBOL_expr_full_block = 344,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 345, /* expr_full_block_assumed_piped  */
  YYSYMBOL_346_24 = 346,                   /* $@24  */
  YYSYMBOL_expr_numeric_const = 347,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign = 348,              /* expr_assign  */
  YYSYMBOL_expr_assign_pipe_right = 349,   /* expr_assign_pipe_right  */
  YYSYMBOL_expr_assign_pipe = 350,         /* expr_assign_pipe  */
  YYSYMBOL_expr_named_call = 351,          /* expr_named_call  */
  YYSYMBOL_expr_method_call = 352,         /* expr_method_call  */
  YYSYMBOL_func_addr_name = 353,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 354,           /* func_addr_expr  */
  YYSYMBOL_355_25 = 355,                   /* $@25  */
  YYSYMBOL_356_26 = 356,                   /* $@26  */
  YYSYMBOL_357_27 = 357,                   /* $@27  */
  YYSYMBOL_358_28 = 358,                   /* $@28  */
  YYSYMBOL_expr_field = 359,               /* expr_field  */
  YYSYMBOL_360_29 = 360,                   /* $@29  */
  YYSYMBOL_361_30 = 361,                   /* $@30  */
  YYSYMBOL_expr_call = 362,                /* expr_call  */
  YYSYMBOL_expr2 = 363,                    /* expr2  */
  YYSYMBOL_364_31 = 364,                   /* $@31  */
  YYSYMBOL_365_32 = 365,                   /* $@32  */
  YYSYMBOL_366_33 = 366,                   /* $@33  */
  YYSYMBOL_367_34 = 367,                   /* $@34  */
  YYSYMBOL_368_35 = 368,                   /* $@35  */
  YYSYMBOL_369_36 = 369,                   /* $@36  */
  YYSYMBOL_expr = 370,                     /* expr  */
  YYSYMBOL_expr_mtag = 371,                /* expr_mtag  */
  YYSYMBOL_optional_field_annotation = 372, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 373,        /* optional_override  */
  YYSYMBOL_optional_constant = 374,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 375, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 376, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 377, /* structure_variable_declaration  */
  YYSYMBOL_opt_sem = 378,                  /* opt_sem  */
  YYSYMBOL_struct_variable_declaration_list = 379, /* struct_variable_declaration_list  */
  YYSYMBOL_380_37 = 380,                   /* $@37  */
  YYSYMBOL_381_38 = 381,                   /* $@38  */
  YYSYMBOL_382_39 = 382,                   /* $@39  */
  YYSYMBOL_383_40 = 383,                   /* $@40  */
  YYSYMBOL_function_argument_declaration_no_type = 384, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 385, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 386,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 387,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 388,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 389,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 390,             /* variant_type  */
  YYSYMBOL_variant_type_list = 391,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 392,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 393,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 394, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 395, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 396,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 397,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 398,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 399, /* let_variable_name_with_pos_list  */
  YYSYMBOL_let_variable_declaration = 400, /* let_variable_declaration  */
  YYSYMBOL_global_variable_declaration_list = 401, /* global_variable_declaration_list  */
  YYSYMBOL_402_41 = 402,                   /* $@41  */
  YYSYMBOL_optional_shared = 403,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 404, /* optional_public_or_private_variable  */
  YYSYMBOL_global_let = 405,               /* global_let  */
  YYSYMBOL_406_42 = 406,                   /* $@42  */
  YYSYMBOL_enum_list = 407,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 408, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 409,             /* single_alias  */
  YYSYMBOL_410_43 = 410,                   /* $@43  */
  YYSYMBOL_alias_list = 411,               /* alias_list  */
  YYSYMBOL_alias_declaration = 412,        /* alias_declaration  */
  YYSYMBOL_413_44 = 413,                   /* $@44  */
  YYSYMBOL_optional_public_or_private_enum = 414, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 415,                /* enum_name  */
  YYSYMBOL_enum_declaration = 416,         /* enum_declaration  */
  YYSYMBOL_optional_structure_parent = 417, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 418,          /* optional_sealed  */
  YYSYMBOL_structure_name = 419,           /* structure_name  */
  YYSYMBOL_class_or_struct = 420,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 421, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 422, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 423,    /* structure_declaration  */
  YYSYMBOL_424_45 = 424,                   /* $@45  */
  YYSYMBOL_425_46 = 425,                   /* $@46  */
  YYSYMBOL_variable_name_with_pos_list = 426, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 427,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 428, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 429, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 430,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 431,            /* bitfield_bits  */
  YYSYMBOL_commas = 432,                   /* commas  */
  YYSYMBOL_bitfield_alias_bits = 433,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_basic_type_declaration = 434, /* bitfield_basic_type_declaration  */
  YYSYMBOL_bitfield_type_declaration = 435, /* bitfield_type_declaration  */
  YYSYMBOL_436_47 = 436,                   /* $@47  */
  YYSYMBOL_437_48 = 437,                   /* $@48  */
  YYSYMBOL_c_or_s = 438,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 439,          /* table_type_pair  */
  YYSYMBOL_dim_list = 440,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 441, /* type_declaration_no_options  */
  YYSYMBOL_442_49 = 442,                   /* $@49  */
  YYSYMBOL_443_50 = 443,                   /* $@50  */
  YYSYMBOL_444_51 = 444,                   /* $@51  */
  YYSYMBOL_445_52 = 445,                   /* $@52  */
  YYSYMBOL_446_53 = 446,                   /* $@53  */
  YYSYMBOL_447_54 = 447,                   /* $@54  */
  YYSYMBOL_448_55 = 448,                   /* $@55  */
  YYSYMBOL_449_56 = 449,                   /* $@56  */
  YYSYMBOL_450_57 = 450,                   /* $@57  */
  YYSYMBOL_451_58 = 451,                   /* $@58  */
  YYSYMBOL_452_59 = 452,                   /* $@59  */
  YYSYMBOL_453_60 = 453,                   /* $@60  */
  YYSYMBOL_454_61 = 454,                   /* $@61  */
  YYSYMBOL_455_62 = 455,                   /* $@62  */
  YYSYMBOL_456_63 = 456,                   /* $@63  */
  YYSYMBOL_457_64 = 457,                   /* $@64  */
  YYSYMBOL_458_65 = 458,                   /* $@65  */
  YYSYMBOL_459_66 = 459,                   /* $@66  */
  YYSYMBOL_460_67 = 460,                   /* $@67  */
  YYSYMBOL_461_68 = 461,                   /* $@68  */
  YYSYMBOL_462_69 = 462,                   /* $@69  */
  YYSYMBOL_463_70 = 463,                   /* $@70  */
  YYSYMBOL_464_71 = 464,                   /* $@71  */
  YYSYMBOL_465_72 = 465,                   /* $@72  */
  YYSYMBOL_466_73 = 466,                   /* $@73  */
  YYSYMBOL_467_74 = 467,                   /* $@74  */
  YYSYMBOL_468_75 = 468,                   /* $@75  */
  YYSYMBOL_type_declaration = 469,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 470,  /* tuple_alias_declaration  */
  YYSYMBOL_471_76 = 471,                   /* $@76  */
  YYSYMBOL_472_77 = 472,                   /* $@77  */
  YYSYMBOL_473_78 = 473,                   /* $@78  */
  YYSYMBOL_474_79 = 474,                   /* $@79  */
  YYSYMBOL_variant_alias_declaration = 475, /* variant_alias_declaration  */
  YYSYMBOL_476_80 = 476,                   /* $@80  */
  YYSYMBOL_477_81 = 477,                   /* $@81  */
  YYSYMBOL_478_82 = 478,                   /* $@82  */
  YYSYMBOL_479_83 = 479,                   /* $@83  */
  YYSYMBOL_bitfield_alias_declaration = 480, /* bitfield_alias_declaration  */
  YYSYMBOL_481_84 = 481,                   /* $@84  */
  YYSYMBOL_482_85 = 482,                   /* $@85  */
  YYSYMBOL_make_decl = 483,                /* make_decl  */
  YYSYMBOL_make_struct_fields = 484,       /* make_struct_fields  */
  YYSYMBOL_make_variant_dim = 485,         /* make_variant_dim  */
  YYSYMBOL_make_struct_single = 486,       /* make_struct_single  */
  YYSYMBOL_make_struct_dim = 487,          /* make_struct_dim  */
  YYSYMBOL_make_struct_dim_list = 488,     /* make_struct_dim_list  */
  YYSYMBOL_make_struct_dim_decl = 489,     /* make_struct_dim_decl  */
  YYSYMBOL_optional_make_struct_dim_decl = 490, /* optional_make_struct_dim_decl  */
  YYSYMBOL_optional_block = 491,           /* optional_block  */
  YYSYMBOL_optional_trailing_semicolon_cur_cur = 492, /* optional_trailing_semicolon_cur_cur  */
  YYSYMBOL_optional_trailing_semicolon_cur_sqr = 493, /* optional_trailing_semicolon_cur_sqr  */
  YYSYMBOL_optional_trailing_semicolon_sqr_sqr = 494, /* optional_trailing_semicolon_sqr_sqr  */
  YYSYMBOL_optional_trailing_delim_sqr_sqr = 495, /* optional_trailing_delim_sqr_sqr  */
  YYSYMBOL_optional_trailing_delim_cur_sqr = 496, /* optional_trailing_delim_cur_sqr  */
  YYSYMBOL_use_initializer = 497,          /* use_initializer  */
  YYSYMBOL_make_struct_decl = 498,         /* make_struct_decl  */
  YYSYMBOL_499_86 = 499,                   /* $@86  */
  YYSYMBOL_500_87 = 500,                   /* $@87  */
  YYSYMBOL_501_88 = 501,                   /* $@88  */
  YYSYMBOL_502_89 = 502,                   /* $@89  */
  YYSYMBOL_503_90 = 503,                   /* $@90  */
  YYSYMBOL_504_91 = 504,                   /* $@91  */
  YYSYMBOL_505_92 = 505,                   /* $@92  */
  YYSYMBOL_506_93 = 506,                   /* $@93  */
  YYSYMBOL_make_tuple = 507,               /* make_tuple  */
  YYSYMBOL_make_map_tuple = 508,           /* make_map_tuple  */
  YYSYMBOL_make_tuple_call = 509,          /* make_tuple_call  */
  YYSYMBOL_510_94 = 510,                   /* $@94  */
  YYSYMBOL_511_95 = 511,                   /* $@95  */
  YYSYMBOL_make_dim = 512,                 /* make_dim  */
  YYSYMBOL_make_dim_decl = 513,            /* make_dim_decl  */
  YYSYMBOL_514_96 = 514,                   /* $@96  */
  YYSYMBOL_515_97 = 515,                   /* $@97  */
  YYSYMBOL_516_98 = 516,                   /* $@98  */
  YYSYMBOL_517_99 = 517,                   /* $@99  */
  YYSYMBOL_518_100 = 518,                  /* $@100  */
  YYSYMBOL_519_101 = 519,                  /* $@101  */
  YYSYMBOL_520_102 = 520,                  /* $@102  */
  YYSYMBOL_521_103 = 521,                  /* $@103  */
  YYSYMBOL_522_104 = 522,                  /* $@104  */
  YYSYMBOL_523_105 = 523,                  /* $@105  */
  YYSYMBOL_make_table = 524,               /* make_table  */
  YYSYMBOL_expr_map_tuple_list = 525,      /* expr_map_tuple_list  */
  YYSYMBOL_make_table_decl = 526,          /* make_table_decl  */
  YYSYMBOL_array_comprehension_where = 527, /* array_comprehension_where  */
  YYSYMBOL_optional_comma = 528,           /* optional_comma  */
  YYSYMBOL_array_comprehension = 529       /* array_comprehension  */
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
#define YYLAST   14312

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  224
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  306
/* YYNRULES -- Number of rules.  */
#define YYNRULES  968
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1828

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   451


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
       2,     2,     2,   210,     2,   223,   221,   206,   199,     2,
     219,   220,   204,   203,   193,   202,   216,   205,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   196,   185,
     200,   194,   201,   195,   222,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   217,     2,   218,   198,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   183,   197,   184,   209,     2,     2,     2,
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
     175,   176,   177,   178,   179,   180,   181,   182,   186,   187,
     188,   189,   190,   191,   192,   207,   208,   211,   212,   213,
     214,   215
};

#if DAS_YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   586,   586,   587,   592,   593,   594,   595,   596,   597,
     598,   599,   600,   601,   602,   603,   604,   608,   611,   614,
     620,   621,   622,   626,   627,   631,   632,   636,   655,   656,
     657,   658,   662,   663,   667,   668,   672,   673,   673,   677,
     682,   691,   706,   722,   727,   735,   735,   776,   802,   806,
     807,   808,   812,   815,   819,   823,   827,   831,   837,   846,
     849,   855,   856,   860,   864,   865,   869,   872,   878,   884,
     887,   893,   894,   898,   899,   900,   910,   923,   924,   928,
     929,   929,   935,   936,   937,   938,   939,   943,   953,   963,
     963,   971,   971,   975,   975,   984,   992,  1004,  1014,  1014,
    1018,  1018,  1024,  1025,  1026,  1027,  1028,  1029,  1033,  1038,
    1046,  1047,  1048,  1052,  1053,  1054,  1055,  1056,  1057,  1058,
    1059,  1060,  1066,  1069,  1075,  1078,  1081,  1087,  1088,  1089,
    1090,  1094,  1107,  1125,  1128,  1136,  1147,  1158,  1169,  1172,
    1179,  1183,  1190,  1191,  1195,  1196,  1197,  1201,  1204,  1208,
    1215,  1219,  1220,  1221,  1222,  1223,  1224,  1225,  1226,  1227,
    1228,  1229,  1230,  1231,  1232,  1233,  1234,  1235,  1236,  1237,
    1238,  1239,  1240,  1241,  1242,  1243,  1244,  1245,  1246,  1247,
    1248,  1249,  1250,  1251,  1252,  1253,  1254,  1255,  1256,  1257,
    1258,  1259,  1260,  1261,  1262,  1263,  1264,  1265,  1266,  1267,
    1268,  1269,  1270,  1271,  1272,  1273,  1274,  1275,  1276,  1277,
    1278,  1279,  1280,  1281,  1282,  1283,  1284,  1285,  1286,  1287,
    1288,  1289,  1290,  1291,  1292,  1293,  1294,  1295,  1296,  1297,
    1298,  1299,  1300,  1301,  1302,  1303,  1304,  1305,  1306,  1307,
    1308,  1309,  1310,  1311,  1312,  1313,  1314,  1315,  1316,  1317,
    1318,  1319,  1320,  1321,  1322,  1323,  1324,  1325,  1326,  1330,
    1331,  1335,  1354,  1355,  1356,  1360,  1366,  1366,  1383,  1384,
    1387,  1388,  1391,  1398,  1422,  1440,  1449,  1455,  1456,  1457,
    1458,  1459,  1460,  1461,  1462,  1463,  1464,  1465,  1466,  1467,
    1468,  1469,  1470,  1471,  1472,  1473,  1474,  1475,  1476,  1480,
    1485,  1491,  1497,  1509,  1510,  1514,  1515,  1519,  1520,  1524,
    1528,  1535,  1535,  1535,  1541,  1541,  1541,  1550,  1584,  1592,
    1599,  1606,  1612,  1613,  1624,  1628,  1631,  1639,  1639,  1639,
    1642,  1648,  1651,  1655,  1659,  1666,  1673,  1679,  1683,  1687,
    1690,  1693,  1701,  1704,  1707,  1715,  1718,  1726,  1729,  1732,
    1740,  1752,  1753,  1754,  1758,  1759,  1763,  1764,  1768,  1773,
    1781,  1792,  1798,  1813,  1825,  1828,  1834,  1834,  1834,  1837,
    1837,  1837,  1842,  1842,  1842,  1850,  1850,  1850,  1856,  1870,
    1886,  1904,  1914,  1925,  1940,  1943,  1949,  1950,  1957,  1968,
    1969,  1970,  1974,  1975,  1976,  1977,  1978,  1982,  1987,  1995,
    1996,  2009,  2013,  2023,  2030,  2037,  2037,  2046,  2047,  2048,
    2049,  2050,  2051,  2052,  2056,  2057,  2058,  2059,  2060,  2061,
    2062,  2063,  2064,  2065,  2066,  2067,  2068,  2069,  2070,  2071,
    2072,  2073,  2074,  2078,  2088,  2097,  2106,  2111,  2112,  2113,
    2114,  2115,  2116,  2117,  2118,  2119,  2120,  2121,  2122,  2123,
    2124,  2125,  2126,  2127,  2132,  2138,  2149,  2154,  2164,  2168,
    2175,  2178,  2178,  2178,  2183,  2183,  2183,  2196,  2200,  2204,
    2209,  2216,  2224,  2229,  2236,  2236,  2236,  2243,  2247,  2257,
    2266,  2275,  2279,  2282,  2288,  2289,  2290,  2291,  2292,  2293,
    2294,  2295,  2296,  2297,  2298,  2299,  2300,  2301,  2302,  2303,
    2304,  2305,  2306,  2307,  2308,  2309,  2310,  2311,  2312,  2313,
    2314,  2315,  2316,  2323,  2324,  2325,  2326,  2327,  2328,  2329,
    2330,  2331,  2332,  2333,  2334,  2341,  2342,  2343,  2344,  2345,
    2346,  2347,  2348,  2362,  2363,  2364,  2365,  2366,  2369,  2372,
    2373,  2373,  2373,  2376,  2381,  2385,  2389,  2389,  2389,  2394,
    2397,  2401,  2401,  2401,  2406,  2409,  2410,  2411,  2412,  2413,
    2414,  2415,  2416,  2417,  2419,  2423,  2431,  2436,  2440,  2449,
    2450,  2451,  2452,  2453,  2454,  2455,  2459,  2463,  2467,  2471,
    2475,  2479,  2483,  2487,  2491,  2498,  2499,  2508,  2512,  2513,
    2514,  2518,  2519,  2523,  2524,  2525,  2529,  2530,  2534,  2545,
    2546,  2547,  2548,  2553,  2556,  2556,  2560,  2560,  2579,  2578,
    2594,  2593,  2607,  2616,  2628,  2637,  2647,  2648,  2649,  2650,
    2651,  2655,  2658,  2667,  2668,  2672,  2675,  2678,  2693,  2702,
    2703,  2707,  2710,  2713,  2726,  2727,  2731,  2737,  2743,  2752,
    2755,  2762,  2765,  2771,  2772,  2773,  2777,  2778,  2782,  2789,
    2794,  2803,  2809,  2820,  2823,  2828,  2833,  2841,  2851,  2862,
    2865,  2865,  2885,  2886,  2890,  2891,  2892,  2896,  2903,  2903,
    2922,  2925,  2941,  2961,  2962,  2963,  2968,  2968,  2998,  3001,
    3008,  3018,  3018,  3022,  3023,  3024,  3028,  3038,  3058,  3081,
    3082,  3086,  3087,  3091,  3097,  3098,  3099,  3100,  3104,  3105,
    3106,  3110,  3113,  3124,  3129,  3124,  3149,  3156,  3161,  3170,
    3176,  3187,  3188,  3189,  3190,  3191,  3192,  3193,  3194,  3195,
    3196,  3197,  3198,  3199,  3200,  3201,  3202,  3203,  3204,  3205,
    3206,  3207,  3208,  3209,  3210,  3211,  3212,  3213,  3217,  3218,
    3219,  3220,  3221,  3222,  3223,  3224,  3228,  3239,  3243,  3250,
    3262,  3269,  3275,  3285,  3286,  3291,  3296,  3310,  3320,  3330,
    3340,  3350,  3363,  3364,  3365,  3366,  3367,  3371,  3375,  3375,
    3375,  3389,  3390,  3394,  3398,  3405,  3409,  3416,  3417,  3418,
    3419,  3420,  3435,  3441,  3441,  3441,  3445,  3450,  3457,  3457,
    3464,  3468,  3472,  3477,  3482,  3487,  3492,  3496,  3500,  3505,
    3509,  3513,  3518,  3518,  3518,  3524,  3531,  3531,  3531,  3536,
    3536,  3536,  3542,  3542,  3542,  3547,  3552,  3552,  3552,  3557,
    3557,  3557,  3566,  3571,  3571,  3571,  3576,  3576,  3576,  3585,
    3590,  3590,  3590,  3595,  3595,  3595,  3604,  3604,  3604,  3610,
    3610,  3610,  3619,  3622,  3633,  3649,  3649,  3654,  3663,  3649,
    3692,  3692,  3697,  3707,  3692,  3736,  3736,  3736,  3789,  3790,
    3791,  3792,  3793,  3797,  3804,  3811,  3817,  3823,  3830,  3837,
    3843,  3852,  3855,  3861,  3869,  3874,  3881,  3886,  3893,  3898,
    3904,  3905,  3909,  3910,  3915,  3916,  3920,  3921,  3925,  3926,
    3930,  3931,  3932,  3936,  3937,  3938,  3942,  3943,  3947,  3980,
    4019,  4038,  4058,  4078,  4099,  4099,  4099,  4107,  4107,  4107,
    4114,  4114,  4114,  4125,  4125,  4125,  4136,  4140,  4146,  4162,
    4168,  4174,  4180,  4180,  4180,  4194,  4199,  4206,  4226,  4254,
    4278,  4278,  4278,  4288,  4288,  4288,  4302,  4302,  4302,  4316,
    4325,  4325,  4325,  4345,  4352,  4352,  4352,  4362,  4367,  4374,
    4377,  4383,  4403,  4422,  4430,  4450,  4475,  4476,  4480,  4481,
    4486,  4496,  4499,  4502,  4505,  4513,  4522,  4534,  4544
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
  "\"variant\"", "\"generator\"", "\"yield\"", "\"sealed\"",
  "\"template\"", "\"+=\"", "\"-=\"", "\"/=\"", "\"*=\"", "\"%=\"",
  "\"&=\"", "\"|=\"", "\"^=\"", "\"<<\"", "\">>\"", "\"++\"", "\"--\"",
  "\"<=\"", "\"<<=\"", "\">>=\"", "\">=\"", "\"==\"", "\"!=\"", "\"->\"",
  "\"<-\"", "\"??\"", "\"?.\"", "\"?[\"", "\"<|\"", "\" <|\"", "\"$ <|\"",
  "\"@ <|\"", "\"@@ <|\"", "\"|>\"", "\":=\"", "\"<<<\"", "\">>>\"",
  "\"<<<=\"", "\">>>=\"", "\"=>\"", "\"::\"", "\"&&\"", "\"||\"", "\"^^\"",
  "\"&&=\"", "\"||=\"", "\"^^=\"", "\"..\"", "\"$$\"", "\"$i\"", "\"$v\"",
  "\"$b\"", "\"$a\"", "\"$t\"", "\"$c\"", "\"$f\"", "\"...\"", "\"[[\"",
  "\"[{\"", "\"{{\"", "\"new scope\"", "\"close scope\"", "SEMICOLON",
  "\"integer constant\"", "\"long integer constant\"",
  "\"unsigned integer constant\"", "\"unsigned long integer constant\"",
  "\"unsigned int8 constant\"", "\"floating point constant\"",
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

#define YYPACT_NINF (-1602)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-834)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1602,   476, -1602, -1602,    40,   -93,   444,   596, -1602,   127,
     266,   266,   266, -1602, -1602,   -18,   356, -1602, -1602, -1602,
     606, -1602, -1602, -1602,   415, -1602,    37, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602,   -71, -1602,   -82,
      61,    98, -1602,   166, -1602, -1602, -1602,   418,   171, -1602,
      35, -1602, -1602, -1602,   266,   266, -1602, -1602,    37, -1602,
   -1602, -1602, -1602, -1602,   162,   238, -1602, -1602, -1602, -1602,
     356,   356,   356,   225, -1602,   914,   -89, -1602, -1602,   339,
     355,   405,   514,   640, -1602,   920,    60,    40,   382,   -93,
     444,   444,   343,   444,   433, -1602,   930,   930, -1602,   473,
     639,    64,   606,   932,   526,   533,   535, -1602,   591,   601,
   -1602, -1602,   -11,    40,   356,   356,   356,   356, -1602, -1602,
   -1602, -1602,   935, -1602, -1602,   644, -1602, -1602, -1602, -1602,
   -1602,   596, -1602, -1602, -1602, -1602, -1602,   947,   121,   593,
   -1602, -1602, -1602, -1602,   343,   343,   343,   772, -1602, -1602,
   -1602, -1602,   686, -1602, -1602, -1602, -1602,   639, -1602, -1602,
   -1602,   664, -1602, -1602, -1602, -1602, -1602,   738, -1602,   -16,
   -1602,   548,   773,   914, -1602, -1602, -1602, -1602, -1602,   505,
     816, -1602,   -83, -1602, -1602, -1602,   952, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602,    -2,   758, -1602,   742, -1602, -1602,
     882, -1602,   754,   596,   596, -1602, -1602, 14053,  1034, -1602,
   -1602,   791, -1602,   672,    40,    40,    -3,   649, -1602, -1602,
   -1602,   121, -1602, -1602, 10557, -1602,   807,   596, -1602, -1602,
   12995, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602,   931,   940, -1602,   759,
     596, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,   596,
     699,   803,   596, -1602,   -83,   243, -1602,    40, -1602,   777,
     983,   786, -1602, -1602,   831,   838,   862,   864,   888,   892,
   -1602, -1602, -1602,   881, -1602, -1602, -1602, -1602, -1602,   817,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602,   919, -1602, -1602, -1602,   922,   941, -1602, -1602, -1602,
   -1602,   945,   946,   921,   -18, -1602, -1602, -1602, -1602, -1602,
    2256,   853, -1602, -1602, -1602, -1602, -1602, -1602, -1602,   979,
     980, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602,   981,   942, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602,  1133, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,   985,   943,
   -1602, -1602,   192,   -33, -1602, -1602, -1602,   416, -1602,   -18,
   -1602, -1602, -1602,   649,   944, -1602,  9936,   986,   995, 10557,
   -1602,   212, -1602, -1602, -1602,  9936, -1602, -1602,   996,   970,
       4,   176,   177, -1602, -1602,  9936,   163, -1602, -1602, -1602,
      21, -1602, -1602, -1602,    22,  5980, -1602,   954, 10303,   743,
   10406,   595, -1602, -1602, -1602, -1602,  1003,  1189,  1468,   959,
   -1602,   309,   994,     9,   960, 10557, 10557, -1602,  2849,   699,
    9936, -1602, -1602,    48,   639, -1602,   982,   984, -1602, -1602,
    1004,   -30,   988,    44, -1602,   364,   962,   989,   990,   964,
     991,   966,   575,   992, -1602,   620,   993,   997,  9936,  9936,
     976,   977,   999,  1000,  1005,  1006, -1602,  2147,  2325,  6190,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602,   998,  1001, -1602,
    1186,  9936,  9936,  9936,  9936,  9936,  5564,  9936, -1602,   987,
   -1602, -1602,  6400, -1602,   -51, -1602, -1602, -1602, -1602,  1009,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602,  2473, -1602,  1008,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602,  1160,   553, -1602,
   -1602, -1602,  4308, 10557, 10557, 10557, 10651, 10557, 10557,  1002,
    1007, 10557,   759, 10557,   759, 10557,   759, 10660,  1037, 10770,
   -1602,  9936, -1602, -1602, -1602, -1602, -1602,  1010, -1602, -1602,
   12552,  9936, -1602,  2256,   320, -1602,     7, -1602, -1602,   477,
   -1602,   853,   672,  1015,   477, -1602,   672, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602,  9936, -1602, -1602,   658,   195,   195,   195,
   -1602,   853,   853, -1602,  9936, -1602, -1602,  1011,  3684, -1602,
     596,  6608, -1602,  9936,  1040, -1602,   606,  1048,  6816,   286,
    1016,  3892,   259,   259,   259, -1602,  7024, -1602,   606,   606,
    9936,  1219, -1602, -1602, -1602, -1602, -1602, -1602,  1194, -1602,
   -1602, -1602,   273, -1602,   606,   606,   606,   606, -1602,   606,
   -1602, -1602,  1167, -1602,   157, -1602, 13185,   893, -1602,   639,
   -1602,   356,  1226, -1602,   -83, -1602, -1602, -1602, -1602,  1018,
   -1602, -1602,   -18,   694, -1602,  1038,  1045,  1046, -1602,  9936,
   10557,  9936,  9936, -1602, -1602,  9936, -1602,  9936, -1602,  9936,
   -1602, -1602,  9936, -1602, 10557,    57,    57,  9936,  9936,  9936,
    9936,  9936,  9936,   658,  3057,   658,  3266,   658,  1105, -1602,
     648, -1602, -1602,   887,  1029,    57,    57,   -44,    57,    57,
      -8,  1236,  1032,  1058, 13185,  1058,   300,   658,   672, -1602,
    1059, -1602,  4516,    46, 13830, 13929,  9936,  9936, -1602, -1602,
    9936,  9936,  9936,  9936,  1080,  9936,   255,  9936,  9936,  9936,
    9936,  9936,  9936,  9936,  9936,  9936,  7232,  9936,  9936,  9936,
    9936,  9936,  9936,  9936,  9936,  9936,  9936, 13991,  9936, -1602,
    7440,  1081, -1602,  4308, -1602,  1123, 14047,   641,   766,  1056,
     419, -1602,   821,   824, -1602, -1602,  1085,   835,   -33,   842,
     -33,   847,   -33, -1602,   520, -1602,   522, -1602, 10557,  1067,
   -1602, -1602, 12587, -1602, -1602,  9936,  1068, 10557, -1602, -1602,
   10557, -1602, -1602, 10805,  1043,  1222, -1602, -1602,   211, -1602,
   -1602, -1602,   596,   658,  1047,  4308, -1602,  1078,  1894, 14090,
    1259,  9936, -1602,  1102,   596,  1084, -1602,  1087,  1120, -1602,
   -1602, 10557,  4308, -1602, 14047,  1066, -1602,  1009, -1602, -1602,
   -1602,   596, -1602, -1602,   596, -1602,   596, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602,    19,   259, -1602,  4724,  4724,
    4724,  4724,  4724,  4724,  4724,  4724,  4724,  4724,  4724,  9936,
    4724,  4724,  4724,  4724,  4724,  4724, -1602, -1602,  1117,   381,
     937,  1224,   606, 10557, 10557, 10557,  5772,  7648,  1119,  9936,
   10557, -1602, -1602, -1602, 10557,  1058,   665,  1088, 10916, 10557,
   10557, 10956, 10557, 11067, 10557,  1058, 10557, 10660,  1058,  1037,
     697, 11102, 11213, 11253, 11364, 11399, 11510,    23,   259,  1075,
     -37,  3475,  4934,  7856,  1163,  1114,     5,   227,  1116,   420,
      42,  8064,     5,   767,    63,  9936,  1130, -1602,  9936, -1602,
   10557, 10557, -1602,  9936,   834,   658,   658,    67,   215, -1602,
    9936, -1602,  1113,  1115,  1121,   179, -1602, -1602,    77, -1602,
    9936, -1602,   245,  5144, -1602,   284,  1142,  1122,  1124,   115,
     759,  1141,  1125, -1602, -1602,  1145,  1127, -1602, -1602,  1757,
    1757,  1422,  1422, 13753, 13753,  1128,    30,  1129, -1602, 12698,
     -21,   -21,  1008,  1757,  1757, 13593, 13445, 13482, 13296, 13960,
   13148, 13617, 13642,  2051,  1422,  1422,  1151,  1151,    30,    30,
      30,   338,  9936,  1131,  1135,   481,  9936,  1338,  1143, 12738,
   -1602,   308, -1602, -1602,  2623,  9936,  9936,  9936,  9936,  9936,
    9936,  9936,  9936,  9936,  9936,  9936,  9936,  9936,  9936,  9936,
    9936,  9936, -1602, -1602, -1602, -1602, 10557, -1602, -1602, -1602,
     549, -1602,  1148, -1602,  1164, -1602,  1168, -1602, 10660, -1602,
    1037,   552,   853, -1602,  1132, -1602,    15, -1602,   853,   853,
   -1602,  9936,  1188, -1602,  1191, -1602, 10557, -1602,  9936, -1602,
      78,   658, -1602,  1078,  9936,   596, -1602,  1176, -1602, -1602,
   -1602, -1602,   895, -1602, 14047, -1602,    46, -1602,   128,  9936,
   -1602,  1009,  1200,  1200, -1602, -1602, -1602,   259,   259,   259,
   -1602, -1602,   273, -1602,   273, -1602,   273, -1602,   273, -1602,
     273, -1602,   273, -1602,   273, -1602,   273, -1602,   273, -1602,
     273, -1602,   273, -1602, -1602,   273, -1602,   273, -1602,   273,
   -1602,   273, -1602,   273, -1602,   273,  1180,   606, -1602, -1602,
     801, -1602,    72,   639,   900,  1182,   848,   727,   352,  1155,
    1158,  1206,  1162,   337,  1165,   850, 10557, 10660,  1037,  1235,
    1166,  1169, 10557, -1602, -1602,  1252,  1472, -1602,  1486, -1602,
    2075,  1170,  2236,   581,  1172,   584,    46, -1602, -1602, -1602,
   -1602, -1602,  1175,  9936, -1602,  9936,  9936,  9936,  5354, 12552,
      11,  9936,   739,   727,   227, -1602, -1602,  1171, -1602,  9936,
   -1602,  1177,  9936, -1602,  9936,   727,   800,  1178, -1602, -1602,
    9936, -1602, -1602, -1602,   607,   642,  1201,    82,    83,  9936,
     658,    87, 13185, -1602,  9936,  9936, 10557,   759,  9936, -1602,
     214, -1602,  1183,   502, 10144, -1602,   739, -1602, -1602,   115,
    1214,  1225,  1181,  1229,  1231, -1602,   568,   -33, -1602,  9936,
   -1602,  9936,  8272,  9936, -1602,  1216,  1204, -1602, -1602,  9936,
    1205, -1602, 12849,  9936,  8480,  1207, -1602, 12884, -1602,  8688,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602,   853, -1602, -1602,  1254, -1602,  1256, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,  1212,
   10557, -1602, -1602,  1068, 11550, -1602,  1382,   203, -1602,  9936,
      90, -1602, 10557,  9936,    46,   759,   596, -1602, -1602,   950,
    9936, -1602,  1420,  2849,    46, -1602,   569,   353, -1602, -1602,
   -1602, 10557,   639,  1402,    72, -1602, -1602,   937, -1602, -1602,
   -1602, -1602,  1218, -1602, -1602, -1602,   661, -1602,  1220,  1268,
   -1602, -1602,  2321,   676,   744, -1602, -1602,  9936,  2436, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,  1223,  8896,
      32, 11661, -1602, -1602,     5,   227, -1602,  1230,   217,  1114,
   -1602, -1602, -1602, -1602,  1116,    76,     5,  1232, -1602, -1602,
   -1602, -1602,   147, -1602, -1602, -1602,  1267,  9936,  9936,   470,
      92,  9936, 11696, 11807,  2496,   -33,   725, -1602,  1237,  5144,
     369, -1602, -1602,  1282, -1602, -1602,   115,  1238,   148, 10557,
   11847, 10557, 11958, -1602,   395, 11993, -1602,  9936, 13334,  9936,
   -1602, 12104,  5144, -1602,   407,  9936, -1602, -1602, -1602,   410,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602,   853, -1602,  1286,
    9936,   397,  9936,   289,   606,   889,   -33, -1602, -1602,   596,
   -1602,   606, -1602,   759,  1287,  1244,   457,   289, -1602, -1602,
   -1602,  1402,   658,  1245,  1246, -1602, -1602,  9936,  1290,  9936,
    1266, -1602, -1602, -1602, -1602,  1249,  1251,  1253,  9936,  9936,
    9936,  1255,  1406,  1258,  1263,  9104, -1602,   432,  9936,   247,
     227, -1602,  9936,  9936,  9936,  9936,   800, -1602,  9936,  9936,
    1212, -1602, -1602,   466,   491,  9936,  9936,   764, -1602, -1602,
   -1602,  1277,  9936, -1602,   572, -1602,  1264, -1602, -1602,  9312,
   -1602, -1602,  2762, -1602,   855, -1602, -1602, -1602, 10557, 12144,
   12255, -1602,   580, -1602, 12290, -1602, -1602, -1602, -1602,   496,
   -1602, -1602, -1602,   148,   130,  4100, -1602,   -33, -1602,   691,
   10557,   212,   639, 14053, -1602, -1602, -1602, -1602,  1406,  1406,
    1265,  1285,  1270,  1269,  1272,  1274,  1275,  9936, -1602,  9936,
   -1602, -1602, -1602,  9936, -1602, -1602,  1406,  1406, -1602, 12401,
   -1602, 13037,  9936,  9936, -1602, 12441, -1602, -1602, 13037, -1602,
     606, -1602, -1602,  1310,  1312,  1313, 13037,   511,  9936,   455,
   -1602,   606,  1280, -1602,  9936, -1602, -1602, -1602,   860, -1602,
   -1602,  1281, -1602,   596, -1602,   950, -1602,  9520,  9728, -1602,
   -1602, -1602, -1602, -1602, -1602,   596, 10557,   212,   265,  9936,
   -1602,   606, 14053,   146,   146, -1602,  9936, -1602,  9936,  1406,
    1406,   727,  1283,  1284,  1058,   146,   727, -1602,  1442,  1289,
   -1602, -1602,   264,  1318,   672, -1602,  9936,  9936,  1292,  1323,
   13037, -1602,   455,   672, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602,  9936,  9936, -1602, -1602,   265,  9936,  9936,   606,
     639,   596,   727,  1114,  1324, -1602,  1291,  1296,  1298,  1301,
     146,   146,  1114,  1302, -1602, -1602,  1303,  1305,  1307,  9936,
    1311,  9936,  9936,  1316,   672,   606, 13037, -1602,  9936,  1325,
   -1602, -1602, -1602, -1602,  9936,   606,   606, -1602, -1602,   639,
     543,  1309, -1602, -1602, -1602, -1602, -1602,  1322,  1329, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,  1346,
    1326, 13037, -1602,   606, -1602, -1602, -1602, -1602,   727, -1602,
   -1602, -1602, -1602,  1336, -1602,   561, -1602, -1602
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   142,     1,   354,     0,     0,     0,   681,   355,     0,
     673,   673,   673,    17,    18,     0,     0,    16,    15,     3,
       0,    10,     9,     8,     0,     7,   662,     6,    11,     5,
       4,    13,    12,    14,   111,   112,   110,   120,   122,    47,
      66,    63,    64,     0,    49,    50,    51,     0,     0,    52,
      61,    48,   269,   268,   673,   673,    24,    23,   662,   675,
     674,   855,   845,   850,     0,   322,    45,   128,   129,   130,
       0,     0,     0,   131,   133,   140,     0,   127,    19,   695,
     694,   259,   683,   698,   663,   664,     0,     0,     0,     0,
       0,     0,    53,     0,     0,    62,     0,     0,    59,     0,
     602,   673,     0,    20,     0,     0,     0,   324,     0,     0,
     139,   134,     0,     0,     0,     0,     0,     0,   143,   697,
     696,   260,   262,   685,   684,     0,   700,   699,   703,   666,
     665,   668,   118,   119,   116,   117,   114,     0,     0,     0,
     113,   123,    67,    65,    55,    56,    54,    61,    58,    57,
     676,   599,   600,   678,   271,   270,   680,   602,   682,    21,
      22,    25,   856,   846,   851,   323,    43,    46,   138,     0,
     135,   136,   137,   141,   264,   263,   266,   261,   686,     0,
     691,   659,   585,    28,    29,    33,     0,   106,   107,   104,
     105,   103,   102,   108,     0,     0,    60,     0,   601,   679,
       0,    27,   762,     0,     0,    44,   132,     0,     0,   670,
     692,     0,   704,   660,     0,     0,   587,     0,    30,    31,
      32,     0,   121,   115,     0,    26,     0,     0,   847,   852,
       0,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,   257,   258,     0,     0,   150,   144,
       0,   738,   741,   744,   745,   739,   742,   740,   743,     0,
       0,   689,   701,   667,   585,     0,   124,     0,   126,     0,
     648,   646,   669,   109,     0,     0,     0,     0,     0,     0,
     711,   731,   712,   747,   713,   717,   718,   719,   720,   737,
     724,   725,   726,   727,   728,   729,   730,   732,   733,   734,
     735,   815,   716,   723,   736,   822,   829,   714,   721,   715,
     722,     0,     0,     0,     0,   746,   777,   780,   778,   779,
     842,   677,   765,   766,   763,   764,   755,   625,   631,   228,
     229,   226,   153,   154,   156,   155,   157,   158,   159,   160,
     186,   187,   184,   185,   177,   188,   189,   178,   175,   176,
     227,   210,     0,   225,   190,   191,   192,   193,   164,   165,
     166,   161,   162,   163,   174,     0,   180,   181,   179,   172,
     173,   168,   167,   169,   170,   171,   152,   151,   209,     0,
     182,   183,   585,   147,   299,   267,   670,   602,   687,     0,
     693,   603,   705,     0,     0,   125,     0,     0,     0,     0,
     647,     0,   783,   806,   809,     0,   812,   802,     0,     0,
     816,   823,   830,   836,   839,     0,   305,   792,   797,   791,
       0,   805,   801,   794,     0,     0,   796,   781,     0,     0,
     848,   853,   230,   231,   224,   208,   232,   211,   194,     0,
     145,   353,   616,   617,     0,     0,     0,   265,     0,     0,
       0,   671,   690,   606,   602,   586,     0,     0,   530,   531,
       0,     0,     0,     0,   525,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   737,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   574,     0,     0,     0,
     407,   409,   408,   410,   411,   412,   413,     0,     0,    39,
     268,     0,     0,     0,     0,     0,   303,     0,   389,   390,
     528,   527,   307,   567,   484,   558,   557,   556,   555,   142,
     561,   526,   560,   559,   533,   485,   534,     0,   486,     0,
     529,   858,   862,   859,   860,   861,   650,   651,     0,   644,
     645,   643,     0,     0,     0,     0,     0,     0,     0,     0,
     768,     0,   144,     0,   144,     0,   144,     0,     0,     0,
     788,   303,   787,   799,   800,   793,   795,     0,   798,   782,
       0,     0,   844,   843,   758,   857,   322,   771,   772,     0,
     626,   621,     0,     0,     0,   632,     0,   233,   213,   214,
     216,   215,   217,   218,   219,   220,   212,   221,   222,   223,
     197,   198,   200,   199,   201,   202,   203,   204,   195,   196,
     205,   206,   207,     0,   351,   352,     0,   585,   585,   585,
     146,   149,   148,   301,     0,    77,    78,    91,   339,   337,
       0,     0,   100,     0,     0,   338,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   277,     0,   278,     0,     0,
       0,     0,   294,   289,   286,   285,   287,   288,   272,   321,
     300,   280,   567,   279,     0,    85,    86,    83,   292,    84,
     293,   295,   357,   284,     0,   281,   568,   414,   688,   602,
     604,     0,     0,   702,   585,   661,   904,   907,   327,   331,
     330,   336,     0,     0,   375,     0,     0,     0,   940,     0,
       0,   307,     0,   366,   369,     0,   372,     0,   944,     0,
     913,   922,     0,   910,     0,   513,   514,     0,     0,     0,
       0,     0,     0,     0,   882,     0,     0,     0,   920,   947,
       0,   311,   314,     0,     0,   490,   489,   523,   488,   487,
       0,     0,     0,   958,   384,   958,   391,     0,     0,   949,
     958,   565,     0,   399,     0,     0,     0,     0,   515,   516,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   474,     0,   649,
       0,     0,   653,     0,   658,     0,   657,     0,     0,     0,
     773,   786,     0,     0,   748,   767,     0,     0,   147,     0,
     147,     0,   147,   623,     0,   629,     0,   749,     0,   958,
     790,   775,     0,   756,   753,     0,   757,     0,   627,   849,
       0,   633,   854,     0,     0,   706,   613,   614,   636,   618,
     619,   620,     0,     0,     0,     0,   343,   340,   568,   414,
       0,     0,   325,     0,     0,     0,   298,     0,     0,    70,
      95,     0,     0,   348,   345,   390,   402,   142,   320,   318,
     319,     0,   296,   297,     0,    89,     0,   405,   275,   283,
     290,   291,   342,   347,   356,     0,     0,   282,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   274,   672,     0,     0,
     593,   596,     0,     0,     0,     0,   896,     0,     0,     0,
       0,   930,   933,   936,     0,   958,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   958,     0,     0,   958,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     322,     0,     0,     0,   916,   874,   882,     0,   925,     0,
       0,     0,   882,     0,     0,     0,     0,   885,     0,   952,
       0,     0,    42,     0,    40,     0,     0,     0,     0,   927,
     959,   304,     0,     0,     0,   461,   458,   460,     0,   951,
     959,   308,     0,   303,   477,     0,   958,     0,     0,     0,
     144,     0,     0,   544,   543,     0,     0,   545,   549,   491,
     492,   504,   505,   502,   503,     0,   539,     0,   521,     0,
     562,   563,   564,   493,   494,   509,   510,   511,   512,     0,
       0,   507,   508,   506,   500,   501,   496,   495,   497,   498,
     499,     0,     0,     0,   467,     0,     0,     0,     0,     0,
     482,     0,   652,   655,   414,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   656,   784,   807,   810,     0,   813,   803,   750,
       0,   817,     0,   824,     0,   831,     0,   837,     0,   840,
       0,     0,   309,   959,     0,   776,   761,   754,   622,   628,
     615,     0,     0,   635,     0,   634,     0,   637,     0,    96,
       0,     0,   344,   341,     0,     0,   326,     0,    97,    98,
      68,    69,     0,   349,   346,   391,   399,   302,    73,     0,
     299,   142,     0,     0,   365,   364,   317,     0,     0,     0,
     436,   445,   424,   446,   425,   448,   427,   447,   426,   449,
     428,   439,   418,   440,   419,   441,   420,   450,   429,   451,
     430,   438,   416,   417,   452,   431,   453,   432,   442,   421,
     443,   422,   444,   423,   437,   415,     0,   143,   594,   595,
     596,   597,   588,   602,     0,     0,     0,   897,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   953,   535,     0,     0,   536,     0,   566,
       0,     0,     0,     0,     0,     0,   399,   569,   570,   571,
     572,   573,     0,     0,   883,     0,     0,     0,     0,   384,
     882,     0,     0,     0,     0,   891,   892,     0,   899,     0,
     889,     0,     0,   928,     0,     0,     0,     0,   887,   929,
       0,   919,   884,   948,     0,     0,    36,     0,     0,     0,
       0,     0,   385,   532,     0,     0,     0,   144,     0,   950,
       0,   478,     0,     0,     0,   481,   959,   873,   479,     0,
       0,     0,     0,     0,     0,   397,     0,   147,   540,     0,
     546,     0,     0,     0,   519,     0,     0,   550,   554,     0,
       0,   522,     0,     0,     0,     0,   468,     0,   475,     0,
     517,   483,   654,   424,   425,   427,   426,   428,   418,   419,
     420,   429,   430,   416,   431,   432,   421,   422,   423,   415,
     785,   808,   811,   774,   814,   804,     0,   769,     0,   818,
     820,   825,   827,   832,   834,   838,   624,   841,   630,   305,
       0,   306,   759,   760,     0,   708,   709,   639,   638,     0,
       0,   350,     0,     0,   399,   144,     0,    71,    72,    73,
       0,    88,    79,     0,   399,   358,     0,     0,   435,   433,
     434,     0,   602,   591,   588,   589,   590,   593,   607,   905,
     908,   328,     0,   333,   334,   332,     0,   378,     0,     0,
     381,   376,     0,     0,     0,   941,   939,   307,     0,   367,
     370,   373,   945,   943,   914,   923,   921,   911,     0,     0,
       0,     0,   864,   863,   882,     0,   917,     0,     0,   875,
     898,   890,   918,   888,   926,     0,   882,     0,   894,   895,
     902,   886,     0,   312,   315,    37,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   147,     0,   480,     0,   303,
       0,   394,   395,     0,   393,   392,     0,     0,     0,     0,
       0,     0,     0,   456,     0,     0,   551,     0,   524,     0,
     520,     0,   303,   469,     0,     0,   518,   476,   472,     0,
     752,   770,   751,   821,   828,   835,   789,   310,   707,     0,
       0,     0,     0,     0,     0,     0,   147,    74,    87,     0,
      80,     0,   273,   144,     0,     0,   646,     0,   612,   592,
     608,   591,     0,     0,     0,   329,   335,     0,     0,     0,
       0,   377,   931,   934,   937,     0,     0,     0,     0,     0,
       0,     0,   896,     0,     0,     0,   575,     0,     0,     0,
       0,   900,     0,     0,     0,     0,     0,   893,     0,     0,
     305,    34,    41,     0,     0,     0,     0,     0,   459,   584,
     462,     0,     0,   454,     0,   401,     0,   398,   400,     0,
     386,   404,     0,   583,     0,   581,   457,   578,     0,     0,
       0,   577,     0,   470,     0,   473,   710,   640,    92,     0,
     101,    99,   276,     0,    73,     0,    90,   147,   359,   646,
       0,     0,   602,     0,   610,   642,   641,   598,   896,   896,
       0,     0,     0,     0,     0,     0,     0,   303,   954,   307,
     368,   371,   374,     0,   897,   915,   896,   896,   537,     0,
     576,   956,     0,     0,   901,     0,   866,   865,   956,   903,
     956,   313,   316,    38,     0,     0,   956,     0,     0,     0,
     465,   956,     0,   396,     0,   387,   541,   547,     0,   582,
     580,     0,   579,     0,   403,    73,    75,   339,     0,    81,
      85,    86,    83,    84,    82,     0,     0,     0,     0,     0,
     605,     0,     0,   881,   881,   379,     0,   382,     0,   896,
     896,   871,     0,     0,   958,   881,   871,   538,     0,     0,
     868,   867,     0,     0,     0,    35,     0,     0,     0,     0,
     956,   463,     0,     0,   455,   388,   542,   548,   552,   471,
      94,    76,     0,     0,   345,   406,     0,     0,     0,     0,
     602,     0,     0,   878,   958,   880,     0,     0,     0,     0,
     881,   881,   872,     0,   942,   955,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   956,   956,   960,     0,     0,
     466,   967,   553,   346,     0,     0,     0,   363,   609,   602,
       0,   959,   879,   906,   909,   380,   383,     0,     0,   938,
     946,   924,   912,   957,   965,   870,   869,   966,   968,     0,
       0,   956,   964,     0,   362,   361,   611,   876,     0,   932,
     935,   963,   961,     0,   360,     0,   962,   877
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1602, -1602,    -1, -1602, -1602, -1602, -1602, -1602,   788,  1450,
   -1602, -1602, -1602, -1602, -1602, -1602,  1554, -1602, -1602, -1602,
     974,   400, -1602,  1409, -1602, -1602,  1470, -1602, -1602, -1602,
   -1348, -1602, -1602, -1602,   -58, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602,  1343, -1602, -1602,   -34,
     -50, -1602, -1602, -1602,   550,   875,  -445,  -526,  -794, -1602,
   -1602, -1602, -1602, -1557, -1602, -1602,    -5,  -209,  -255,  -424,
   -1602,   427, -1602,  -568, -1317,  -698,    94,   417, -1602, -1602,
   -1602, -1602,  -519,     0, -1602, -1602, -1602, -1602, -1602,   -46,
     -45,   -43, -1602,   -42, -1602, -1602, -1602,  1570, -1602,   431,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602,  -376,   -38,   587,   108,   303, -1088,  -624,
   -1602,  -660, -1602, -1602,  -448,   507, -1602, -1602, -1602, -1601,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,  1074,
   -1602, -1602, -1602, -1602, -1602, -1602,  1077, -1602,  -150,   199,
      66,   202,   411, -1602,  -156, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602,   510,  -426,  -906, -1602,  -421,  -902, -1602,  -827,
      70,    71, -1602,  -541, -1452, -1602,  -378, -1602, -1602,  1546,
   -1602, -1602, -1602,  1210,  1139,   244, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602,  -668,  -128, -1602,  1137, -1602, -1602,   503, -1602,
    1314, -1602, -1602, -1602,  -360, -1602, -1602,  -403, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602,  -197, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,  1149,
    -725,  -105,  -882,  -713, -1602, -1602, -1226,  -927, -1602, -1602,
   -1602, -1198,    52, -1077, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602,   360,  -491, -1602, -1602, -1602,   885, -1602,
   -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602, -1602,
   -1602, -1602, -1602,  -931,  -738, -1602
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,   588,    18,   161,    58,   201,    19,   186,   192,
    1663,  1456,  1571,   743,   520,   167,   521,   109,    21,    22,
      49,    50,    51,    98,    23,    41,    42,   658,   659,  1380,
    1381,   660,  1521,  1615,   661,   662,  1139,   663,   853,   854,
     664,   665,   666,   667,  1373,   863,   193,   194,    37,    38,
      39,   216,    73,    74,    75,    76,    24,   393,   457,   259,
     122,    25,   176,   260,   177,   207,   522,   156,   876,  1150,
     670,   458,   671,   752,   572,   758,  1101,   523,   980,  1569,
     981,  1570,   673,   524,   674,   699,   925,  1535,   525,   675,
     676,   677,   678,   679,   680,   681,   626,   682,   895,  1386,
    1144,   683,   526,   939,  1548,   940,  1549,   942,  1550,   527,
     930,  1541,   528,   753,  1591,   529,  1295,  1296,  1010,   878,
     530,   916,  1141,   531,   805,  1151,   685,   532,   533,   997,
     534,  1276,  1669,  1277,  1732,   535,  1057,  1497,   536,   686,
    1479,  1736,  1481,  1737,  1598,  1782,   738,   538,   451,  1397,
    1530,  1190,  1192,   922,   153,   463,   918,   694,  1623,  1702,
     452,   453,   454,   823,   824,   440,   825,   826,   441,  1237,
     846,   847,  1627,   552,   411,   281,   282,   213,   274,    85,
     131,    27,   182,   270,    99,   100,   197,   101,    28,    55,
     125,   179,    29,   400,   211,   212,    83,   128,   402,    30,
     180,   272,   848,   539,   269,   327,   328,  1090,   836,   439,
     227,   329,   816,  1501,  1098,   809,   437,   330,   553,  1340,
     828,   558,  1345,   554,  1341,   555,  1342,   557,  1344,   561,
    1349,   562,  1503,   563,  1351,   564,  1504,   565,  1353,   566,
    1505,   567,  1355,   568,  1357,   591,    31,   105,   203,   337,
     592,    32,   106,   204,   338,   596,    33,   104,   202,   540,
    1753,  1763,  1007,   966,  1754,  1755,  1756,   967,   979,  1259,
    1253,  1248,  1450,  1200,   541,   923,  1533,   924,  1534,   949,
    1554,   946,  1552,   968,   759,   542,   947,  1553,   969,   543,
    1206,  1634,  1207,  1635,  1208,  1636,   934,  1545,   944,  1551,
     740,   760,   544,  1719,   991,   545
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      17,   199,    54,   829,   273,   395,   548,   803,   739,   965,
     684,   965,   888,   937,   589,    66,    77,   992,   692,    78,
     594,  1118,  1001,   972,  1092,   464,  1094,   331,  1096,   879,
     880,  1518,   217,   804,   669,   583,   818,  1006,   820,  1244,
     822,  1223,  1506,  1233,  1199,  1256,  1440,  1225,  1375,   705,
    1008,   764,   765,   141,   573,   575,  -142,    94,   958,    34,
      35,   398,  1254,   169,   958,   957,  1701,   970,  1731,   974,
      77,    77,    77,    40,  1621,   132,   133,   214,   764,   765,
     590,   595,   987,  1260,   763,   776,   777,  1269,   690,   998,
    1113,  1104,    95,   455,   734,   736,   326,  1278,  1369,    84,
    1236,   158,  1457,  1458,   117,   774,   108,  1461,   776,   777,
    1512,    87,  1576,    64,    77,    77,    77,    77,  1395,   856,
    1282,    59,   761,    86,   403,    64,   181,    60,   669,   118,
    -819,  1780,   873,   114,   115,   116,   187,   188,  1428,   215,
    1376,   755,  1376,    65,   844,  1751,   706,   707,   768,   769,
     108,  1377,  1378,  1377,  1378,    65,   774,  1115,   775,   776,
     777,   778,    13,   456,   275,   845,   779,  1697,   762,   959,
      13,   279,   797,   798,   209,   768,   769,    87,  1396,  1142,
    1362,   276,    14,   774,  1280,  1120,   776,   777,   778,   702,
      14,   221,   280,   779,   628,   797,   798,  1210,   228,   229,
    -819,  1006,   629,   837,   206,  -819,  1009,  1221,   834,   168,
    1224,   986,   154,    36,   669,   278,  1114,  1558,   222,   277,
     574,   576,   336,  -819,   325,  1124,   134,   669,   154,   838,
     585,   135,   155,   136,   841,  1114,   137,  1561,  1143,   577,
      96,   461,  1290,   405,   708,   578,   797,   798,   155,   668,
     688,    97,  1291,  1662,   693,   394,  1114,    88,   631,   632,
    1114,  1565,   857,   709,   396,   691,  1686,   401,  1287,  1124,
    1114,  1114,  1146,   797,   798,  1114,  1114,  1006,  1283,   138,
    1114,   326,   139,  1114,  1063,  1114,  1515,   189,  1292,   896,
    1589,    89,   190,  1379,   191,  1685,  1523,   137,   427,   102,
      56,  1413,  -826,  -833,   959,  -464,  1414,   936,   695,  1293,
     326,    52,   326,  1435,  1294,  1402,   624,  1267,  1268,  1280,
    1271,   950,    13,    59,   426,   428,   429,   326,   326,    60,
    1113,    53,  1568,   935,  1234,   107,  1122,  1741,  1113,   549,
    1124,  1113,    14,   945,  1113,   157,   948,   449,    57,   550,
     449,  1236,   214,  1133,  1563,   214,   807,   808,   810,   625,
     812,   813,  1654,   570,   817,  1752,   819,   844,   821,   326,
     326,    90,  -826,  -833,  1113,  -464,    93,  -826,  -833,   669,
    -464,   108,   571,   839,  1652,   860,  1005,   842,   845,   430,
      67,  1113,   549,   431,   870,  -826,  -833,  1115,  -464,   462,
     438,  1771,   550,   959,  1114,  1115,   551,  1116,  1115,   325,
    1117,  1115,   450,  1027,   215,  1245,  1246,   215,  1280,    68,
      79,    80,    52,    81,  1061,   326,   326,   326,  1028,   326,
     326,   669,  1136,   326,  1270,   326,    87,   326,   325,   326,
     325,  1115,    53,    64,   113,  1247,   119,    92,   669,    52,
    1086,    82,   993,  1370,    13,   325,   325,   994,  1115,   551,
     432,   404,   120,    69,   433,  1281,  1100,   434,  1132,    53,
     325,   703,  1006,    65,    14,  1645,     2,  1284,  1757,  1123,
     518,   875,   435,     3,  1297,   833,   438,  1263,   436,  1767,
     144,   145,    70,   146,   518,   875,  1310,   325,   325,    64,
     995,  1124,    13,  1478,  1285,   717,     4,  1560,     5,  1279,
       6,  1311,   121,   834,   835,   965,     7,  1145,  1439,  1566,
    1194,  1195,    14,  1388,  1389,  1390,     8,  1434,  1321,    65,
     965,  1209,     9,   917,  1797,  1798,  1215,  1216,  1408,  1218,
    1510,  1220,  1446,  1222,   921,  1124,  1524,   802,   142,   999,
    1198,  1703,  1704,   325,   325,   325,    10,   325,   325,    97,
      52,   325,  1476,   325,   710,   325,    71,   325,    43,  1715,
    1716,   123,  1403,  1526,   117,    72,  1212,   124,    11,    12,
      53,   151,   326,   711,    13,    13,   427,   755,  1124,  1585,
    1124,    44,    45,    46,    43,   755,   326,  1119,    64,  1187,
    1124,   152,  1460,  1124,    14,    14,   147,   993,  1250,  1128,
     460,  1564,   587,   428,   429,  1596,   438,    44,    45,    46,
     110,   111,   112,    91,    47,  1124,  1137,  1603,    65,  1138,
    1605,  1102,  1760,  1761,    48,   394,  1014,  1018,  1251,  1315,
    1108,    13,    13,  1109,   394,   866,   150,   394,   394,   394,
      47,  1032,  1650,  1620,  1316,  1575,   410,   882,   883,  1124,
      48,    14,    14,  1124,   170,   171,   172,   173,    52,  1058,
     587,  1581,  1356,   889,   890,   891,   892,   430,   893,  1358,
     549,   431,    15,   897,  1124,    13,  1664,    13,    53,  1124,
     550,    77,   114,    16,   116,  1242,  1384,   126,   427,   162,
     326,   208,   927,   127,  1124,    14,   163,    14,   164,   326,
     325,  1665,   326,   587,    13,   587,  1683,    13,    13,  1546,
    1468,  1097,  1613,  1099,   325,   428,   429,  1723,  1196,  1724,
     427,  1729,  1653,  1205,    14,  1728,  1242,    14,    14,   978,
    1733,  1360,  1346,   326,  1584,   587,    13,   551,   432,    13,
    1347,  1465,   433,  1359,  1242,   434,   996,   428,   429,    52,
      13,  1476,  1524,  1817,   165,  1242,    14,  1602,   593,    14,
     435,    13,    13,  1242,   587,   718,   436,   587,   166,    53,
      14,  1827,  1425,  1102,  1102,  1427,  1477,  1525,   587,   430,
    1672,    14,    14,   431,   719,   326,   326,   326,  1681,  1779,
     587,   279,   326,  1412,   151,  1082,   326,    13,  1453,  1418,
     844,   326,   326,    13,   326,   195,   326,   178,   326,   326,
     721,   430,   280,  1695,   152,   431,    13,    14,   325,    95,
      13,   845,   976,    14,   977,   587,   154,   325,   438,   722,
     325,    13,  1083,  1454,  1809,  1810,    14,   394,  1393,  1516,
      14,   198,   326,   326,   587,  1612,   155,  1430,   587,   394,
     432,    14,  1537,   154,   433,  1100,  1211,   434,  1191,   587,
    1371,   325,   397,  1464,   200,   672,   394,  1543,  1445,   394,
    1823,  1140,   435,   155,  1452,   959,   332,  1696,   436,  1343,
     410,   394,   432,  1459,   928,  1772,   433,  1437,  1226,   434,
    1280,   333,  1466,   -82,  1360,  1360,   334,   154,   335,    13,
    1582,  1308,  1438,   929,   435,   205,   584,   114,  1124,  1367,
     436,  1193,   210,   325,   325,   325,  1484,   155,   427,    14,
     325,   223,    13,   427,   325,   684,   224,   587,  1494,   325,
     325,  1713,   325,  1499,   325,  1544,   325,   325,   225,  1668,
     226,  1257,    14,   394,  1258,   428,   429,  1124,   326,   669,
     428,   429,  1376,   438,   271,  1243,   390,  1084,  1252,   672,
     326,  1243,  1252,  1377,  1378,   391,  1766,   129,   392,   408,
     325,   325,   409,   130,  1447,   410,  1762,  1448,   326,   159,
    1449,  1762,   174,  1511,  1188,   160,   406,  1617,   175,   399,
    1189,   898,   899,   900,   901,   902,   903,   904,   905,   466,
     467,   218,   219,   226,   906,   907,  1792,  -762,   438,   430,
     908,   438,  1087,   431,   430,  1088,   407,  1790,   431,   473,
     909,   412,   438,   910,   911,   475,  1091,  1398,   413,   438,
     912,   913,   914,  1093,   438,   438,  1538,   438,  1095,  1401,
     438,  1411,   438,  1557,  1100,   672,  1677,   438,   114,   115,
     116,  1738,   414,  1322,   183,   184,   982,   983,   672,  1712,
     148,   149,   482,   483,  1264,  1265,  1592,  1660,   326,   326,
    1699,  1573,  1574,   415,   326,  1577,   325,   915,   416,  1348,
     432,  1671,   417,  1825,   433,   432,  1374,   434,   325,   433,
     418,  1399,   434,    44,    45,    46,   485,   486,  1555,   261,
     518,   875,   435,   262,   518,   875,   325,   435,   436,   420,
     394,  1517,   421,   436,   183,   184,   185,   263,   264,   218,
     219,   220,   265,   266,   267,   268,  1609,   849,   850,   851,
     425,   422,   394,   394,   394,   423,   424,    64,   326,    61,
      62,    63,   442,   443,   444,   446,  1747,  1748,   447,   546,
     445,   448,   465,  1507,   497,   498,   499,    52,   547,   559,
     560,   581,   764,   765,  1522,  1513,   597,    65,   623,   627,
     630,   712,   696,   715,   697,   717,  1392,   510,   704,   713,
     714,   716,   720,   723,  1527,   727,   728,   724,   741,   744,
    1667,   742,  1409,   801,   698,  1784,   325,   325,   815,   756,
     593,   840,   325,   865,   867,   427,   871,  1698,   729,   730,
     672,   516,   814,  1590,   731,   732,    16,   800,   830,   885,
     -93,   886,   326,   894,   920,  1775,  1528,   926,   931,   877,
     877,   877,   428,   429,   326,   932,   933,   975,   985,   988,
     989,   990,  1000,  1025,  1062,   896,  1608,  1085,  1089,   887,
    1103,  1107,  1111,   326,  1614,  1112,  1121,  1714,   427,   768,
     769,  1124,   672,  1125,   887,  1127,   325,   774,  1129,   775,
     776,   777,   778,  1130,  1594,   427,  1131,   779,  1135,   672,
    1186,  1191,  1203,  1746,  1235,   428,   429,   598,   599,   600,
     601,   602,   603,   604,   605,  1241,   430,  1242,  1213,  1249,
     431,   857,   428,   429,  1262,   672,   672,   672,   672,   672,
     672,   672,   672,   672,   672,   672,   606,   672,   672,   672,
     672,   672,   672,  1273,  1274,  1286,   607,   608,   609,  1318,
    1275,  1298,  1288,  1289,  1299,  1300,  1301,  1302,  1303,  1350,
    1313,   326,  1361,   326,  1314,   794,   795,   796,  1590,   430,
     325,  1365,  1319,   431,  1366,  1352,  1123,   797,   798,  1354,
    1372,   394,   325,  1385,  1391,  1404,   430,   432,  1405,  1406,
     431,   433,  1407,  1400,   434,  1410,  1416,  1471,  1417,  1441,
    1423,   325,  1426,   887,  1429,  1443,  1451,  1455,  1472,   435,
    1473,  1678,  1474,  1467,  1475,   436,  1153,  1155,  1157,  1159,
    1161,  1163,  1165,  1167,  1169,  1171,  1486,  1174,  1176,  1178,
    1180,  1182,  1184,  1487,  1489,  1509,  1495,  1500,  1740,  1502,
     432,   571,  1520,  1243,   433,  1529,  1415,   434,  1536,  1539,
    1745,  1540,  1555,   764,   765,  1243,   887,   432,  1572,  1562,
    1567,   433,   435,  1419,   434,  1586,  1588,  1583,   436,  1606,
    1618,   887,  1619,  1631,  1628,  1629,  1700,  1633,  1637,   435,
     326,  1638,  1639,   394,  1643,   436,  1644,  1646,  1670,   325,
     537,   325,  1647,   877,  1673,  1705,  1706,  1725,  1708,   556,
    1707,  1709,   326,  1710,  1711,  1769,  1789,  1726,  1727,   569,
    1734,  1739,  1773,  1764,  1765,   427,   394,  1770,  1778,   580,
    1777,  1793,  1610,  1611,   394,  1774,  1794,  1791,  1795,   427,
    1616,  1796,  1799,  1800,  1781,  1801,  1622,  1802,  1818,  1804,
    1821,   984,   428,   429,  1807,   687,   140,   689,   766,   767,
     768,   769,  1819,  1812,  1822,   877,   428,   429,   774,  1820,
     775,   776,   777,   778,  1826,    20,   196,  1689,   779,   143,
     780,   781,   725,   726,   283,  1808,   919,  1383,   326,  1690,
    1691,    26,  1692,  1693,  1387,  1684,   610,   611,   612,   613,
     614,   615,   616,   617,  1587,   745,   746,   747,   748,   749,
     754,   754,  1470,  1531,  1788,   618,   430,  1624,   325,  1532,
     431,  1394,  1625,  1626,   103,   619,   459,   700,   394,  1363,
     430,  1768,  1444,   419,   431,   620,   621,   622,  1659,   701,
     325,   973,     0,     0,   792,   793,   794,   795,   796,   806,
       0,     0,     0,  1816,     0,     0,     0,     0,   797,   798,
       0,     0,     0,     0,     0,   754,     0,     0,     0,     0,
    1718,   887,     0,     0,     0,   832,     0,  1718,     0,  1718,
       0,     0,     0,     0,     0,  1718,     0,   432,     0,   996,
    1718,   433,     0,  1420,   434,     0,     0,     0,   394,     0,
       0,   432,     0,     0,     0,   433,     0,  1421,   434,   435,
     394,     0,     0,     0,     0,   436,   325,   843,     0,     0,
    1750,     0,     0,   435,     0,     0,     0,     0,     0,   436,
       0,   852,   858,     0,     0,   859,     0,     0,   862,     0,
     864,   887,     0,     0,     0,   869,     0,     0,   874,  1718,
       0,     0,   996,   881,   877,   877,   877,   884,     0,   887,
       0,   887,     0,   887,     0,   887,   394,   887,  1787,   887,
       0,   887,     0,   887,     0,   887,     0,   887,     0,   887,
       0,     0,   887,     0,   887,     0,   887,     0,   887,     0,
     887,     0,   887,     0,  1718,  1718,     0,     0,   764,   765,
       0,     0,     0,   754,  1814,  1815,   938,     0,     0,   941,
       0,   943,     0,   754,     0,     0,   754,     0,     0,     0,
     672,   951,   952,   953,   954,   955,   956,     0,     0,     0,
    1718,   964,  1824,   964,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   754,     0,     0,     0,
    1019,  1020,     0,     0,  1021,  1022,  1023,  1024,     0,  1026,
       0,  1029,  1030,  1031,  1033,  1034,  1035,  1036,  1037,  1038,
    1040,  1041,  1042,  1043,  1044,  1045,  1046,  1047,  1048,  1049,
    1050,     0,  1059,     0,   754,   768,   769,     0,     0,     0,
    1064,     0,     0,   774,     0,   775,   776,   777,   778,     0,
       0,     0,     0,   779,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  -384,     0,     0,     0,     0,     0,
       0,     0,  1106,     0,     0,   764,   765,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   858,
       0,     0,   859,     0,     0,     0,     0,     0,  1126,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1134,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   792,
     793,   794,   795,   796,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   797,   798,  1152,  1154,  1156,  1158,  1160,
    1162,  1164,  1166,  1168,  1170,  1172,  1173,  1175,  1177,  1179,
    1181,  1183,  1185,     0,     0,     0,     0,     0,     0,     0,
     754,     0,     0,     0,  1202,     0,  1204,     0,     0,     0,
     766,   767,   768,   769,   770,     0,     0,   771,   772,   773,
     774,     0,   775,   776,   777,   778,     0,     0,     0,     0,
     779,     0,   780,   781,     0,   745,  1239,   754,   782,   783,
     784,     0,     0,     0,   785,   754,     0,     0,     0,     0,
       0,     0,  1261,     0,     0,     0,     0,     0,     0,  -384,
    1266,     0,     0,     0,  1272,     0,     0,     0,     0,     0,
       0,     0,   764,   765,     0,     0,     0,   754,     0,  -384,
       0,     0,     0,     0,     0,     0,     0,  -384,     0,   786,
       0,   787,   788,   789,   790,   791,   792,   793,   794,   795,
     796,     0,   887,     0,     0,     0,     0,     0,   427,     0,
     797,   798,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1312,     0,     0,     0,
    1317,     0,     0,     0,     0,   428,   429,     0,     0,     0,
       0,     0,  1323,  1324,  1325,  1326,  1327,  1328,  1329,  1330,
    1331,  1332,  1333,  1334,  1335,  1336,  1337,  1338,  1339,     0,
     733,     0,     0,     0,     0,     0,   284,   766,   767,   768,
     769,   770,   285,     0,   771,   772,   773,   774,   286,   775,
     776,   777,   778,     0,     0,  1364,     0,   779,   287,   780,
     781,     0,     0,     0,     0,  1368,   288,     0,  1272,   430,
       0,     0,     0,   431,     0,     0,     0,     0,     0,     0,
       0,   289,     0,     0,     0,     0,  1382,     0,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   790,   791,   792,   793,   794,   795,   796,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   797,   798,   427,
     432,     0,     0,     0,   433,     0,  1422,   434,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   427,
      64,     0,   435,     0,     0,     0,   428,   429,   436,     0,
       0,     0,     0,   323,     0,     0,     0,   754,     0,  1431,
       0,     0,   754,  1432,  1433,     0,   428,   429,  1436,     0,
      65,     0,     0,     0,     0,     0,  1442,     0,   754,   964,
       0,     0,     0,     0,   754,     0,     0,     0,   735,     0,
       0,     0,     0,   754,   284,     0,     0,     0,  1462,  1463,
     285,     0,   754,     0,   427,     0,   286,     0,  1272,     0,
     430,     0,     0,     0,   431,     0,   287,     0,   324,     0,
       0,     0,     0,  1480,   288,  1482,   754,  1485,     0,     0,
     430,   428,   429,  1488,   431,     0,     0,  1491,   754,   289,
       0,     0,     0,   754,     0,     0,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,   319,   320,   321,   322,     0,
       0,   432,     0,     0,     0,   433,     0,  1424,   434,     0,
       0,     0,     0,   754,     0,   430,     0,     0,     0,   431,
    1514,   432,     0,   435,     0,   433,     0,  1519,   434,   436,
     687,     0,     0,     0,     0,     0,     0,     0,    64,   427,
       0,     0,     0,   435,     0,     0,     0,     0,     0,   436,
       0,   323,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   764,   765,   428,   429,    65,     0,
       0,     0,     0,   754,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   432,     0,     0,     0,
     433,     0,  1542,   434,     0,     0,     0,     0,     0,   427,
       0,   754,   754,     0,     0,   754,     0,     0,   435,     0,
       0,     0,     0,   754,   436,     0,   324,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   428,   429,     0,     0,
     430,  1599,     0,  1600,   431,     0,   754,     0,     0,  1604,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   754,  1607,     0,   766,
     767,   768,   769,   770,     0,     0,   771,   772,   773,   774,
       0,   775,   776,   777,   778,     0,     0,     0,     0,   779,
       0,   780,   781,     0,  1630,     0,  1632,   782,   783,   784,
     430,     0,     0,   785,   431,  1640,  1641,  1642,     0,  1649,
       0,   432,  1651,     0,     0,   433,  1655,  1547,   434,  1658,
    1656,  1657,     0,     0,     0,     0,  1661,     0,     0,  1666,
     754,     0,     0,   435,     0,     0,     0,     0,     0,   436,
       0,     0,     0,     0,     0,     0,  1675,     0,   786,     0,
     787,   788,   789,   790,   791,   792,   793,   794,   795,   796,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   797,
     798,   432,  1694,   799,     0,   433,     0,  1580,   434,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   754,     0,   435,     0,     0,     0,   754,     0,   436,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1720,
    1721,  1065,  1066,  1067,  1068,  1069,  1070,  1071,  1072,     0,
       0,     0,  1730,     0,  1073,  1074,     0,     0,     0,     0,
    1075,  1735,     0,     0,     0,     0,     0,     0,     0,     0,
     909,   754,     0,  1076,  1077,  1744,     0,     0,     0,     0,
    1078,  1079,  1080,     0,     0,     0,  1749,     0,     0,     0,
       0,     0,     0,  1758,     0,  1759,     0,     0,    13,     0,
       0,     0,     0,     0,     0,   427,     0,     0,     0,     0,
       0,  1776,     0,     0,     0,     0,     0,     0,    14,     0,
       0,     0,     0,     0,     0,     0,   754,  1081,     0,     0,
    1783,     0,   428,   429,  1785,  1786,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   518,   875,  1803,     0,  1805,  1806,
     633,     0,  1811,     0,   466,   467,     3,     0,   634,   635,
     636,  1813,   637,     0,   468,   469,   470,   471,   472,     0,
       0,     0,     0,     0,   473,   638,   474,   639,   640,     0,
     475,     0,     0,     0,     0,     0,   430,   641,   476,   642,
     431,   643,     0,   644,   477,     0,     0,   478,     0,     8,
     479,   645,     0,   646,   480,     0,     0,   647,   648,     0,
       0,     0,     0,     0,   649,     0,     0,   482,   483,     0,
     290,   291,   292,     0,   294,   295,   296,   297,   298,   484,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,     0,   312,   313,   314,     0,     0,   317,   318,   319,
     320,   485,   486,   650,   651,     0,     0,   432,     0,     0,
       0,   433,     0,  1676,   434,     0,     0,   488,   489,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   435,
       0,     0,   652,   653,   654,   436,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     490,   491,   492,   493,   494,     0,   495,     0,   496,   497,
     498,   499,    52,   154,   655,   500,   501,   502,   503,   504,
     505,   506,    65,   656,   508,   509,     0,     0,     0,     0,
       0,     0,   510,   155,   657,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   511,   512,   513,     0,    15,     0,     0,   514,   515,
       0,     0,   466,   467,     0,     0,   516,     0,   517,     0,
     518,   519,   468,   469,   470,   471,   472,     0,     0,     0,
       0,     0,   473,     0,   474,     0,     0,     0,   475,     0,
     427,     0,     0,     0,     0,     0,   476,     0,     0,     0,
       0,     0,   477,     0,     0,   478,     0,     0,   479,     0,
     958,     0,   480,     0,     0,     0,     0,   428,   429,     0,
       0,     0,   481,     0,     0,   482,   483,     0,   290,   291,
     292,     0,   294,   295,   296,   297,   298,   484,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,     0,
     312,   313,   314,     0,     0,   317,   318,   319,   320,   485,
     486,   487,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   488,   489,     0,     0,     0,
       0,   430,     0,     0,     0,   431,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   490,   491,
     492,   493,   494,     0,   495,   959,   496,   497,   498,   499,
      52,     0,     0,   500,   501,   502,   503,   504,   505,   506,
     960,   507,   508,   509,     0,     0,     0,     0,     0,     0,
     510,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   432,     0,     0,     0,   433,     0,     0,   961,
     512,   513,     0,    15,     0,     0,   514,   515,     0,     0,
       0,   466,   467,     0,   962,     0,   963,     0,   518,   519,
     436,   468,   469,   470,   471,   472,     0,     0,     0,     0,
       0,   473,     0,   474,     0,     0,     0,   475,     0,   427,
       0,     0,     0,     0,     0,   476,     0,     0,     0,     0,
       0,   477,     0,     0,   478,     0,     0,   479,     0,     0,
       0,   480,     0,     0,     0,     0,   428,   429,     0,     0,
       0,   481,     0,     0,   482,   483,     0,   290,   291,   292,
       0,   294,   295,   296,   297,   298,   484,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,     0,   312,
     313,   314,     0,     0,   317,   318,   319,   320,   485,   486,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   488,   489,     0,     0,     0,     0,
     430,     0,     0,     0,   431,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
       0,     0,     0,     0,     0,     0,     0,   490,   491,   492,
     493,   494,     0,   495,   959,   496,   497,   498,   499,    52,
       0,     0,   500,   501,   502,   503,   504,   505,   506,   960,
     507,   508,   509,     0,     0,     0,     0,     0,     0,   510,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   432,     0,     0,     0,   433,     0,     0,   961,   512,
     513,     0,    15,     0,     0,   514,   515,     0,     0,     0,
     466,   467,     0,   962,     0,   971,     0,   518,   519,   436,
     468,   469,   470,   471,   472,     0,     0,     0,     0,     0,
     473,     0,   474,     0,     0,     0,   475,     0,   575,     0,
       0,     0,     0,     0,   476,     0,     0,     0,     0,     0,
     477,     0,     0,   478,     0,     0,   479,     0,     0,     0,
     480,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     481,     0,     0,   482,   483,     0,   290,   291,   292,     0,
     294,   295,   296,   297,   298,   484,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,     0,   312,   313,
     314,     0,     0,   317,   318,   319,   320,   485,   486,   487,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   488,   489,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   490,   491,   492,   493,
     494,     0,   495,     0,   496,   497,   498,   499,    52,     0,
       0,   500,   501,   502,   503,   504,   505,   506,    65,   507,
     508,   509,     0,     0,     0,     0,     0,     0,   510,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   576,     0,     0,   511,   512,   513,
       0,    15,     0,     0,   514,   515,     0,     0,     0,   466,
     467,     0,  1238,     0,   517,     0,   518,   519,   578,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,     0,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   650,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,   855,     0,     0,     0,     0,     0,   652,   653,   654,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,     0,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,    65,   507,   508,
     509,     0,     0,     0,     0,     0,     0,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   511,   512,   513,     0,
      15,     0,     0,   514,   515,     0,     0,   466,   467,     0,
       0,   516,     0,   517,     0,   518,   519,   468,   469,   470,
     471,   472,     0,     0,     0,     0,     0,   473,     0,   474,
       0,     0,     0,   475,     0,     0,     0,     0,     0,     0,
       0,   476,     0,     0,     0,     0,     0,   477,     0,     0,
     478,     0,     0,   479,     0,     0,     0,   480,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   481,     0,     0,
     482,   483,     0,   290,   291,   292,     0,   294,   295,   296,
     297,   298,   484,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,     0,   312,   313,   314,     0,     0,
     317,   318,   319,   320,   485,   486,   650,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     488,   489,     0,     0,     0,     0,     0,     0,     0,   872,
       0,     0,     0,     0,     0,   652,   653,   654,     0,     0,
       0,     0,     0,     0,     0,    64,     0,     0,     0,     0,
       0,     0,     0,   490,   491,   492,   493,   494,     0,   495,
       0,   496,   497,   498,   499,    52,     0,     0,   500,   501,
     502,   503,   504,   505,   506,    65,   507,   508,   509,     0,
       0,     0,     0,     0,     0,   510,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   511,   512,   513,     0,    15,     0,
       0,   514,   515,     0,     0,   466,   467,     0,     0,   516,
       0,   517,     0,   518,   519,   468,   469,   470,   471,   472,
       0,     0,     0,     0,     0,   473,  1687,   474,   639,     0,
       0,   475,     0,     0,     0,     0,     0,     0,     0,   476,
       0,     0,     0,     0,     0,   477,     0,     0,   478,     0,
       0,   479,   645,     0,     0,   480,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   481,     0,     0,   482,   483,
       0,   290,   291,   292,     0,   294,   295,   296,   297,   298,
     484,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,     0,   312,   313,   314,     0,     0,   317,   318,
     319,   320,   485,   486,   487,  1688,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   488,   489,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,     0,     0,     0,     0,     0,     0,
       0,   490,   491,   492,   493,   494,     0,   495,     0,   496,
     497,   498,   499,    52,     0,     0,   500,   501,   502,   503,
     504,   505,   506,    65,   507,   508,   509,     0,     0,     0,
       0,     0,     0,   510,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   511,   512,   513,     0,    15,     0,     0,   514,
     515,     0,     0,   466,   467,     0,     0,   516,     0,   517,
       0,   518,   519,   468,   469,   470,   471,   472,     0,     0,
       0,     0,     0,   473,     0,   474,     0,     0,     0,   475,
       0,     0,     0,     0,     0,     0,     0,   476,     0,     0,
       0,     0,     0,   477,     0,     0,   478,     0,     0,   479,
       0,     0,     0,   480,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   481,     0,     0,   482,   483,     0,   290,
     291,   292,     0,   294,   295,   296,   297,   298,   484,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
       0,   312,   313,   314,     0,     0,   317,   318,   319,   320,
     485,   486,   650,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   488,   489,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   652,   653,   654,     0,     0,     0,     0,     0,     0,
       0,    64,     0,     0,     0,     0,     0,     0,     0,   490,
     491,   492,   493,   494,     0,   495,     0,   496,   497,   498,
     499,    52,     0,     0,   500,   501,   502,   503,   504,   505,
     506,    65,   507,   508,   509,     0,     0,     0,     0,     0,
       0,   510,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     511,   512,   513,     0,    15,     0,     0,   514,   515,     0,
       0,   466,   467,     0,     0,   516,     0,   517,     0,   518,
     519,   468,   469,   470,   471,   472,     0,     0,     0,     0,
       0,   473,     0,   474,     0,     0,     0,   475,     0,     0,
       0,     0,     0,     0,     0,   476,     0,     0,     0,     0,
       0,   477,     0,     0,   478,     0,     0,   479,     0,     0,
       0,   480,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   481,     0,     0,   482,   483,  1002,   290,   291,   292,
       0,   294,   295,   296,   297,   298,   484,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,     0,   312,
     313,   314,     0,     0,   317,   318,   319,   320,   485,   486,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   488,   489,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
       0,     0,     0,     0,     0,     0,     0,   490,   491,   492,
     493,   494,     0,   495,   959,   496,   497,   498,   499,    52,
       0,     0,   500,   501,   502,   503,   504,   505,   506,   960,
     507,   508,   509,     0,     0,     0,     0,     0,     0,   510,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   511,   512,
     513,     0,    15,     0,     0,   514,   515,     0,     0,   466,
     467,     0,     0,  1003,     0,   517,  1004,   518,   519,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,     0,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   650,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1147,  1148,  1149,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,     0,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,    65,   507,   508,
     509,     0,     0,     0,     0,     0,     0,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   511,   512,   513,     0,
      15,     0,     0,   514,   515,     0,     0,     0,     0,   466,
     467,   516,     0,   517,     0,   518,   519,   750,     0,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,   751,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,     0,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,    65,   507,   508,
     509,     0,     0,     0,     0,     0,     0,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   511,   512,   513,     0,
      15,     0,     0,   514,   515,     0,     0,     0,     0,   466,
     467,   516,   579,   517,     0,   518,   519,   750,     0,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,   751,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,   959,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,   960,   507,   508,
     509,     0,     0,     0,     0,     0,     0,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   511,   512,   513,     0,
      15,     0,     0,   514,   515,     0,     0,     0,     0,   466,
     467,   516,     0,   517,     0,   518,   519,   750,     0,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,   751,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,     0,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,    65,   507,   508,
     509,     0,     0,     0,     0,     0,     0,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   511,   512,   513,     0,
      15,     0,     0,   514,   515,     0,     0,     0,     0,   466,
     467,   516,   830,   517,     0,   518,   519,   750,     0,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,   751,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,     0,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,    65,   507,   508,
     509,     0,     0,     0,     0,     0,     0,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   511,   512,   513,     0,
      15,     0,     0,   514,   515,     0,     0,   466,   467,     0,
       0,   516,     0,   517,     0,   518,   519,   468,   469,   470,
     471,   472,     0,     0,     0,     0,     0,   473,     0,   474,
       0,     0,     0,   475,     0,     0,     0,     0,     0,     0,
       0,   476,     0,     0,     0,     0,     0,   477,     0,     0,
     478,     0,     0,   479,     0,     0,     0,   480,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   481,     0,     0,
     482,   483,  1197,   290,   291,   292,     0,   294,   295,   296,
     297,   298,   484,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,     0,   312,   313,   314,     0,     0,
     317,   318,   319,   320,   485,   486,   487,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     488,   489,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    64,     0,     0,     0,     0,
       0,     0,     0,   490,   491,   492,   493,   494,     0,   495,
     959,   496,   497,   498,   499,    52,     0,     0,   500,   501,
     502,   503,   504,   505,   506,   960,   507,   508,   509,     0,
       0,     0,     0,     0,     0,   510,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   511,   512,   513,     0,    15,     0,
       0,   514,   515,     0,     0,   466,   467,     0,     0,   516,
       0,   517,     0,   518,   519,   468,   469,   470,   471,   472,
       0,     0,     0,     0,     0,   473,     0,   474,     0,     0,
       0,   475,     0,     0,     0,     0,     0,     0,     0,   476,
       0,     0,     0,     0,     0,   477,     0,     0,   478,     0,
       0,   479,     0,     0,     0,   480,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   481,     0,     0,   482,   483,
       0,   290,   291,   292,     0,   294,   295,   296,   297,   298,
     484,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,     0,   312,   313,   314,     0,     0,   317,   318,
     319,   320,   485,   486,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   488,   489,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,     0,     0,     0,     0,     0,     0,
       0,   490,   491,   492,   493,   494,     0,   495,     0,   496,
     497,   498,   499,    52,     0,     0,   500,   501,   502,   503,
     504,   505,   506,    65,   507,   508,   509,     0,     0,     0,
       0,     0,     0,   510,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   511,   512,   513,     0,    15,     0,     0,   514,
     515,     0,     0,     0,     0,   466,   467,   516,   579,   517,
       0,   518,   519,   737,     0,   468,   469,   470,   471,   472,
       0,     0,     0,     0,     0,   473,     0,   474,     0,     0,
       0,   475,     0,     0,     0,     0,     0,     0,     0,   476,
       0,     0,     0,     0,     0,   477,     0,     0,   478,     0,
       0,   479,     0,     0,     0,   480,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   481,     0,     0,   482,   483,
       0,   290,   291,   292,     0,   294,   295,   296,   297,   298,
     484,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,     0,   312,   313,   314,     0,     0,   317,   318,
     319,   320,   485,   486,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   488,   489,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,     0,     0,     0,     0,     0,     0,
       0,   490,   491,   492,   493,   494,     0,   495,     0,   496,
     497,   498,   499,    52,     0,     0,   500,   501,   502,   503,
     504,   505,   506,    65,   507,   508,   509,     0,     0,     0,
       0,     0,     0,   510,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   511,   512,   513,     0,    15,     0,     0,   514,
     515,     0,     0,     0,     0,   466,   467,   516,     0,   517,
       0,   518,   519,   757,     0,   468,   469,   470,   471,   472,
       0,     0,     0,     0,     0,   473,     0,   474,     0,     0,
       0,   475,     0,     0,     0,     0,     0,     0,     0,   476,
       0,     0,     0,     0,     0,   477,     0,     0,   478,     0,
       0,   479,     0,     0,     0,   480,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   481,     0,     0,   482,   483,
       0,   290,   291,   292,     0,   294,   295,   296,   297,   298,
     484,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,     0,   312,   313,   314,     0,     0,   317,   318,
     319,   320,   485,   486,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   488,   489,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,     0,     0,     0,     0,     0,     0,
       0,   490,   491,   492,   493,   494,     0,   495,     0,   496,
     497,   498,   499,    52,     0,     0,   500,   501,   502,   503,
     504,   505,   506,    65,   507,   508,   509,     0,     0,     0,
       0,     0,     0,   510,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   511,   512,   513,     0,    15,     0,     0,   514,
     515,     0,     0,   466,   467,     0,     0,   516,     0,   517,
       0,   518,   519,   468,   469,   470,   471,   472,     0,     0,
       0,     0,     0,   473,     0,   474,     0,     0,     0,   475,
       0,     0,     0,     0,     0,     0,     0,   476,     0,     0,
       0,     0,     0,   477,     0,     0,   478,     0,     0,   479,
       0,     0,     0,   480,     0,     0,     0,     0,     0,   861,
       0,     0,     0,   481,     0,     0,   482,   483,     0,   290,
     291,   292,     0,   294,   295,   296,   297,   298,   484,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
       0,   312,   313,   314,     0,     0,   317,   318,   319,   320,
     485,   486,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   488,   489,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    64,     0,     0,     0,     0,     0,     0,     0,   490,
     491,   492,   493,   494,     0,   495,     0,   496,   497,   498,
     499,    52,     0,     0,   500,   501,   502,   503,   504,   505,
     506,    65,   507,   508,   509,     0,     0,     0,     0,     0,
       0,   510,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     511,   512,   513,     0,    15,     0,     0,   514,   515,     0,
       0,   466,   467,     0,     0,   516,     0,   517,     0,   518,
     519,   468,   469,   470,   471,   472,     0,     0,     0,     0,
       0,   473,     0,   474,     0,     0,     0,   475,     0,     0,
       0,     0,     0,     0,     0,   476,     0,     0,     0,     0,
       0,   477,     0,     0,   478,     0,     0,   479,     0,     0,
       0,   480,     0,     0,   868,     0,     0,     0,     0,     0,
       0,   481,     0,     0,   482,   483,     0,   290,   291,   292,
       0,   294,   295,   296,   297,   298,   484,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,     0,   312,
     313,   314,     0,     0,   317,   318,   319,   320,   485,   486,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   488,   489,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
       0,     0,     0,     0,     0,     0,     0,   490,   491,   492,
     493,   494,     0,   495,     0,   496,   497,   498,   499,    52,
       0,     0,   500,   501,   502,   503,   504,   505,   506,    65,
     507,   508,   509,     0,     0,     0,     0,     0,     0,   510,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   511,   512,
     513,     0,    15,     0,     0,   514,   515,     0,     0,   466,
     467,     0,     0,   516,     0,   517,     0,   518,   519,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,     0,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,     0,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,    65,   507,   508,
     509,     0,     0,     0,     0,     0,     0,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   741,     0,   511,   512,   513,     0,
      15,     0,     0,   514,   515,     0,     0,   466,   467,     0,
       0,   516,     0,   517,     0,   518,   519,   468,   469,   470,
     471,   472,     0,     0,  1039,     0,     0,   473,     0,   474,
       0,     0,     0,   475,     0,     0,     0,     0,     0,     0,
       0,   476,     0,     0,     0,     0,     0,   477,     0,     0,
     478,     0,     0,   479,     0,     0,     0,   480,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   481,     0,     0,
     482,   483,     0,   290,   291,   292,     0,   294,   295,   296,
     297,   298,   484,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,     0,   312,   313,   314,     0,     0,
     317,   318,   319,   320,   485,   486,   487,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     488,   489,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    64,     0,     0,     0,     0,
       0,     0,     0,   490,   491,   492,   493,   494,     0,   495,
       0,   496,   497,   498,   499,    52,     0,     0,   500,   501,
     502,   503,   504,   505,   506,    65,   507,   508,   509,     0,
       0,     0,     0,     0,     0,   510,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   511,   512,   513,     0,    15,     0,
       0,   514,   515,     0,     0,   466,   467,     0,     0,   516,
       0,   517,     0,   518,   519,   468,   469,   470,   471,   472,
       0,     0,     0,     0,     0,   473,     0,   474,     0,     0,
       0,   475,     0,     0,     0,     0,     0,     0,     0,   476,
       0,     0,     0,     0,     0,   477,     0,     0,   478,     0,
       0,   479,     0,     0,     0,   480,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   481,     0,     0,   482,   483,
       0,   290,   291,   292,     0,   294,   295,   296,   297,   298,
     484,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,     0,   312,   313,   314,     0,     0,   317,   318,
     319,   320,   485,   486,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   488,   489,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,     0,     0,     0,     0,     0,     0,
       0,   490,   491,   492,   493,   494,     0,   495,     0,   496,
     497,   498,   499,    52,     0,     0,   500,   501,   502,   503,
     504,   505,   506,    65,   507,   508,   509,     0,     0,     0,
       0,     0,     0,   510,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   511,   512,   513,     0,    15,     0,     0,   514,
     515,     0,     0,   466,   467,     0,     0,   516,     0,   517,
    1060,   518,   519,   468,   469,   470,   471,   472,     0,     0,
       0,     0,     0,   473,     0,   474,     0,     0,     0,   475,
       0,     0,     0,     0,     0,     0,     0,   476,     0,     0,
       0,     0,     0,   477,     0,     0,   478,     0,     0,   479,
       0,     0,     0,   480,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   481,     0,     0,   482,   483,     0,   290,
     291,   292,     0,   294,   295,   296,   297,   298,   484,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
       0,   312,   313,   314,     0,     0,   317,   318,   319,   320,
     485,   486,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   488,   489,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    64,     0,     0,     0,     0,     0,     0,     0,   490,
     491,   492,   493,   494,     0,   495,     0,   496,   497,   498,
     499,    52,     0,     0,   500,   501,   502,   503,   504,   505,
     506,    65,   507,   508,   509,     0,     0,     0,     0,     0,
       0,   510,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1201,     0,
     511,   512,   513,     0,    15,     0,     0,   514,   515,     0,
       0,   466,   467,     0,     0,   516,     0,   517,     0,   518,
     519,   468,   469,   470,   471,   472,     0,     0,     0,     0,
       0,   473,     0,   474,     0,     0,     0,   475,     0,     0,
       0,     0,     0,     0,     0,   476,     0,     0,     0,     0,
       0,   477,     0,     0,   478,     0,     0,   479,     0,     0,
       0,   480,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   481,     0,     0,   482,   483,     0,   290,   291,   292,
       0,   294,   295,   296,   297,   298,   484,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,     0,   312,
     313,   314,     0,     0,   317,   318,   319,   320,   485,   486,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   488,   489,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
       0,     0,     0,     0,     0,     0,     0,   490,   491,   492,
     493,   494,     0,   495,     0,   496,   497,   498,   499,    52,
       0,     0,   500,   501,   502,   503,   504,   505,   506,    65,
     507,   508,   509,     0,     0,     0,     0,     0,     0,   510,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   511,   512,
     513,     0,    15,     0,     0,   514,   515,     0,     0,   466,
     467,     0,     0,   516,     0,   517,  1240,   518,   519,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,     0,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,     0,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,    65,   507,   508,
     509,     0,     0,     0,     0,     0,     0,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   511,   512,   513,     0,
      15,     0,     0,   514,   515,     0,     0,   466,   467,     0,
       0,   516,     0,   517,  1255,   518,   519,   468,   469,   470,
     471,   472,     0,     0,     0,     0,     0,   473,     0,   474,
       0,     0,     0,   475,     0,     0,     0,     0,     0,     0,
       0,   476,     0,     0,     0,     0,     0,   477,     0,     0,
     478,     0,     0,   479,     0,     0,     0,   480,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   481,     0,     0,
     482,   483,     0,   290,   291,   292,     0,   294,   295,   296,
     297,   298,   484,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,     0,   312,   313,   314,     0,     0,
     317,   318,   319,   320,   485,   486,   487,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     488,   489,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    64,     0,     0,     0,     0,
       0,     0,     0,   490,   491,   492,   493,   494,     0,   495,
       0,   496,   497,   498,   499,    52,     0,     0,   500,   501,
     502,   503,   504,   505,   506,    65,   507,   508,   509,     0,
       0,     0,     0,     0,     0,   510,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   511,   512,   513,     0,    15,     0,
       0,   514,   515,     0,     0,   466,   467,     0,     0,   516,
       0,   517,  1483,   518,   519,   468,   469,   470,   471,   472,
       0,     0,     0,     0,     0,   473,     0,   474,     0,     0,
       0,   475,     0,     0,     0,     0,     0,     0,     0,   476,
       0,     0,     0,     0,     0,   477,     0,     0,   478,     0,
       0,   479,     0,     0,     0,   480,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   481,     0,     0,   482,   483,
       0,   290,   291,   292,     0,   294,   295,   296,   297,   298,
     484,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,     0,   312,   313,   314,     0,     0,   317,   318,
     319,   320,   485,   486,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   488,   489,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,     0,     0,     0,     0,     0,     0,
       0,   490,   491,   492,   493,   494,     0,   495,     0,   496,
     497,   498,   499,    52,     0,     0,   500,   501,   502,   503,
     504,   505,   506,    65,   507,   508,   509,     0,     0,     0,
       0,     0,     0,   510,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   511,   512,   513,     0,    15,     0,     0,   514,
     515,     0,     0,   466,   467,     0,     0,  1492,     0,   517,
    1493,   518,   519,   468,   469,   470,   471,   472,     0,     0,
       0,     0,     0,   473,     0,   474,     0,     0,     0,   475,
       0,     0,     0,     0,     0,     0,     0,   476,     0,     0,
       0,     0,     0,   477,     0,     0,   478,     0,     0,   479,
       0,     0,     0,   480,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   481,     0,     0,   482,   483,     0,   290,
     291,   292,     0,   294,   295,   296,   297,   298,   484,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
       0,   312,   313,   314,     0,     0,   317,   318,   319,   320,
     485,   486,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   488,   489,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    64,     0,     0,     0,     0,     0,     0,     0,   490,
     491,   492,   493,   494,     0,   495,     0,   496,   497,   498,
     499,    52,     0,     0,   500,   501,   502,   503,   504,   505,
     506,    65,   507,   508,   509,     0,     0,     0,     0,     0,
       0,   510,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     511,   512,   513,     0,    15,     0,     0,   514,   515,     0,
       0,   466,   467,     0,     0,   516,     0,   517,  1498,   518,
     519,   468,   469,   470,   471,   472,     0,     0,     0,     0,
       0,   473,     0,   474,     0,     0,     0,   475,     0,     0,
       0,     0,     0,     0,     0,   476,     0,     0,     0,     0,
       0,   477,     0,     0,   478,     0,     0,   479,     0,     0,
       0,   480,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   481,     0,     0,   482,   483,     0,   290,   291,   292,
       0,   294,   295,   296,   297,   298,   484,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,     0,   312,
     313,   314,     0,     0,   317,   318,   319,   320,   485,   486,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   488,   489,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
       0,     0,     0,     0,     0,     0,     0,   490,   491,   492,
     493,   494,     0,   495,     0,   496,   497,   498,   499,    52,
       0,     0,   500,   501,   502,   503,   504,   505,   506,    65,
     507,   508,   509,     0,     0,     0,     0,     0,     0,   510,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   511,   512,
     513,     0,    15,     0,     0,   514,   515,     0,     0,   466,
     467,     0,     0,   516,     0,   517,  1556,   518,   519,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,     0,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,     0,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,    65,   507,   508,
     509,     0,     0,     0,     0,     0,     0,   510,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   511,   512,   513,     0,
      15,     0,     0,   514,   515,     0,     0,   466,   467,     0,
       0,   516,     0,   517,  1648,   518,   519,   468,   469,   470,
     471,   472,     0,     0,     0,     0,     0,   473,     0,   474,
       0,     0,     0,   475,     0,     0,     0,     0,     0,     0,
       0,   476,     0,     0,     0,     0,     0,   477,     0,     0,
     478,     0,     0,   479,     0,     0,     0,   480,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   481,     0,     0,
     482,   483,     0,   290,   291,   292,     0,   294,   295,   296,
     297,   298,   484,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,     0,   312,   313,   314,     0,     0,
     317,   318,   319,   320,   485,   486,   487,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     488,   489,     0,     0,     0,     0,     0,     0,     0,  1674,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    64,     0,     0,     0,     0,
       0,     0,     0,   490,   491,   492,   493,   494,     0,   495,
       0,   496,   497,   498,   499,    52,     0,     0,   500,   501,
     502,   503,   504,   505,   506,    65,   507,   508,   509,     0,
       0,     0,     0,     0,     0,   510,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   511,   512,   513,     0,    15,     0,
       0,   514,   515,     0,     0,   466,   467,     0,     0,   516,
       0,   517,     0,   518,   519,   468,   469,   470,   471,   472,
       0,     0,     0,     0,     0,   473,     0,   474,     0,     0,
       0,   475,     0,     0,     0,     0,     0,     0,     0,   476,
       0,     0,     0,     0,     0,   477,     0,     0,   478,     0,
       0,   479,     0,     0,     0,   480,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   481,     0,     0,   482,   483,
       0,   290,   291,   292,     0,   294,   295,   296,   297,   298,
     484,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,     0,   312,   313,   314,     0,     0,   317,   318,
     319,   320,   485,   486,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   488,   489,
       0,     0,     0,     0,     0,     0,     0,  1742,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,     0,     0,     0,     0,     0,     0,
       0,   490,   491,   492,   493,   494,     0,   495,     0,   496,
     497,   498,   499,    52,     0,     0,   500,   501,   502,   503,
     504,   505,   506,    65,   507,   508,   509,     0,     0,     0,
       0,     0,     0,   510,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   511,   512,   513,     0,    15,     0,     0,   514,
     515,     0,     0,   466,   467,     0,     0,   516,     0,   517,
       0,   518,   519,   468,   469,   470,   471,   472,     0,     0,
       0,     0,     0,   473,     0,   474,     0,     0,     0,   475,
       0,     0,     0,     0,     0,     0,     0,   476,     0,     0,
       0,     0,     0,   477,     0,     0,   478,     0,     0,   479,
       0,     0,     0,   480,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   481,     0,     0,   482,   483,     0,   290,
     291,   292,     0,   294,   295,   296,   297,   298,   484,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
       0,   312,   313,   314,     0,     0,   317,   318,   319,   320,
     485,   486,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   488,   489,     0,     0,
       0,     0,     0,     0,     0,  1743,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    64,     0,     0,     0,     0,     0,     0,     0,   490,
     491,   492,   493,   494,     0,   495,     0,   496,   497,   498,
     499,    52,     0,     0,   500,   501,   502,   503,   504,   505,
     506,    65,   507,   508,   509,     0,     0,     0,     0,     0,
       0,   510,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     511,   512,   513,     0,    15,     0,     0,   514,   515,     0,
       0,   466,   467,     0,     0,   516,     0,   517,     0,   518,
     519,   468,   469,   470,   471,   472,     0,     0,     0,     0,
       0,   473,     0,   474,     0,     0,     0,   475,     0,     0,
       0,     0,     0,     0,     0,   476,     0,     0,     0,     0,
       0,   477,     0,     0,   478,     0,     0,   479,     0,     0,
       0,   480,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   481,     0,     0,   482,   483,     0,   290,   291,   292,
       0,   294,   295,   296,   297,   298,   484,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,     0,   312,
     313,   314,     0,     0,   317,   318,   319,   320,   485,   486,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   488,   489,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
       0,     0,     0,     0,     0,     0,     0,   490,   491,   492,
     493,   494,     0,   495,     0,   496,   497,   498,   499,    52,
       0,     0,   500,   501,   502,   503,   504,   505,   506,    65,
     507,   508,   509,     0,     0,     0,     0,     0,     0,   510,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   511,   512,
     513,     0,    15,     0,     0,   514,   515,     0,     0,   466,
     467,     0,     0,   516,     0,   517,     0,   518,   519,   468,
     469,   470,   471,   472,     0,     0,     0,     0,     0,   473,
       0,   474,     0,     0,     0,   475,     0,     0,     0,     0,
       0,     0,     0,   476,     0,     0,     0,     0,     0,   477,
       0,     0,   478,     0,     0,   479,     0,     0,     0,   480,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   481,
       0,     0,   482,   483,     0,   290,   291,   292,     0,   294,
     295,   296,   297,   298,   484,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,     0,   312,   313,   314,
       0,     0,   317,   318,   319,   320,   485,   486,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   488,   489,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   490,   491,   492,   493,   494,
       0,   495,     0,   496,   497,   498,   499,    52,     0,     0,
     500,   501,   502,   503,   504,   505,   506,    65,   507,   508,
     509,     0,   284,     0,     0,     0,     0,   510,   285,     0,
       0,     0,     0,     0,   286,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   287,     0,   511,   512,   513,     0,
      15,     0,   288,   514,   515,     0,     0,     0,     0,     0,
       0,  1469,     0,   517,     0,   518,   519,   289,     0,     0,
       0,     0,     0,     0,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   284,     0,     0,     0,     0,
       0,   285,     0,     0,     0,     0,     0,   286,     0,     0,
       0,     0,     0,     0,     0,     0,    64,   287,     0,     0,
       0,     0,     0,     0,     0,   288,     0,     0,     0,   323,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     289,     0,     0,     0,     0,     0,    65,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   320,   321,   322,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   324,     0,   582,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   323,     0,     0,     0,     0,     0,     0,     0,
       0,    13,     0,     0,     0,     0,   284,     0,     0,   586,
       0,     0,   285,     0,     0,     0,     0,     0,   286,     0,
       0,    14,     0,     0,     0,     0,     0,     0,   287,   587,
       0,     0,     0,     0,     0,     0,   288,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   289,     0,     0,     0,     0,     0,   324,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,   319,   320,   321,
     322,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   764,   765,     0,     0,     0,     0,     0,   284,
       0,     0,     0,     0,     0,   285,     0,     0,     0,     0,
       0,   286,     0,     0,     0,     0,     0,     0,     0,     0,
      64,   287,     0,     0,     0,     0,     0,     0,     0,   288,
       0,     0,     0,   323,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   289,     0,     0,     0,     0,     0,
      65,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   322,     0,     0,     0,   766,   767,   768,
     769,   770,     0,     0,   771,   772,   773,   774,   324,   775,
     776,   777,   778,     0,     0,     0,     0,   779,     0,   780,
     781,   764,   765,     0,     0,   782,   783,   784,     0,     0,
       0,   785,     0,    64,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   323,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   764,   765,     0,     0,
       0,     0,     0,   586,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   786,     0,   787,   788,
     789,   790,   791,   792,   793,   794,   795,   796,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   797,   798,     0,
       0,   811,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   324,     0,     0,     0,     0,   766,   767,   768,   769,
     770,     0,     0,   771,   772,   773,   774,     0,   775,   776,
     777,   778,     0,     0,     0,     0,   779,     0,   780,   781,
       0,     0,     0,     0,   782,   783,   784,     0,     0,     0,
     785,   766,   767,   768,   769,   770,     0,     0,   771,   772,
     773,   774,     0,   775,   776,   777,   778,   764,   765,     0,
       0,   779,     0,   780,   781,     0,     0,     0,     0,   782,
     783,   784,     0,     0,     0,   785,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   786,     0,   787,   788,   789,
     790,   791,   792,   793,   794,   795,   796,   764,   765,     0,
       0,     0,     0,     0,     0,     0,   797,   798,     0,     0,
     827,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     786,     0,   787,   788,   789,   790,   791,   792,   793,   794,
     795,   796,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   797,   798,     0,     0,  1110,     0,     0,     0,     0,
       0,     0,   766,   767,   768,   769,   770,     0,     0,   771,
     772,   773,   774,     0,   775,   776,   777,   778,     0,     0,
       0,     0,   779,     0,   780,   781,     0,     0,     0,     0,
     782,   783,   784,     0,     0,     0,   785,     0,     0,     0,
       0,     0,   766,   767,   768,   769,   770,     0,     0,   771,
     772,   773,   774,     0,   775,   776,   777,   778,   764,   765,
       0,     0,   779,     0,   780,   781,     0,     0,     0,     0,
     782,   783,   784,     0,     0,     0,   785,     0,     0,     0,
       0,   786,     0,   787,   788,   789,   790,   791,   792,   793,
     794,   795,   796,   764,   765,     0,     0,     0,     0,     0,
       0,     0,   797,   798,     0,     0,  1214,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   786,     0,   787,   788,   789,   790,   791,   792,   793,
     794,   795,   796,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   797,   798,     0,     0,  1217,     0,     0,     0,
       0,     0,     0,   766,   767,   768,   769,   770,     0,     0,
     771,   772,   773,   774,     0,   775,   776,   777,   778,     0,
       0,     0,     0,   779,     0,   780,   781,     0,     0,     0,
       0,   782,   783,   784,     0,     0,     0,   785,   766,   767,
     768,   769,   770,     0,     0,   771,   772,   773,   774,     0,
     775,   776,   777,   778,   764,   765,     0,     0,   779,     0,
     780,   781,     0,     0,     0,     0,   782,   783,   784,     0,
       0,     0,   785,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   786,     0,   787,   788,   789,   790,   791,   792,
     793,   794,   795,   796,   764,   765,     0,     0,     0,     0,
       0,     0,     0,   797,   798,     0,     0,  1219,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   786,     0,   787,
     788,   789,   790,   791,   792,   793,   794,   795,   796,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   797,   798,
       0,     0,  1227,     0,     0,     0,     0,     0,     0,   766,
     767,   768,   769,   770,     0,     0,   771,   772,   773,   774,
       0,   775,   776,   777,   778,     0,     0,     0,     0,   779,
       0,   780,   781,     0,     0,     0,     0,   782,   783,   784,
       0,     0,     0,   785,     0,     0,     0,     0,     0,   766,
     767,   768,   769,   770,     0,     0,   771,   772,   773,   774,
       0,   775,   776,   777,   778,   764,   765,     0,     0,   779,
       0,   780,   781,     0,     0,     0,     0,   782,   783,   784,
       0,     0,     0,   785,     0,     0,     0,     0,   786,     0,
     787,   788,   789,   790,   791,   792,   793,   794,   795,   796,
     764,   765,     0,     0,     0,     0,     0,     0,     0,   797,
     798,     0,     0,  1228,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   786,     0,
     787,   788,   789,   790,   791,   792,   793,   794,   795,   796,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   797,
     798,     0,     0,  1229,     0,     0,     0,     0,     0,     0,
     766,   767,   768,   769,   770,     0,     0,   771,   772,   773,
     774,     0,   775,   776,   777,   778,     0,     0,     0,     0,
     779,     0,   780,   781,     0,     0,     0,     0,   782,   783,
     784,     0,     0,     0,   785,   766,   767,   768,   769,   770,
       0,     0,   771,   772,   773,   774,     0,   775,   776,   777,
     778,   764,   765,     0,     0,   779,     0,   780,   781,     0,
       0,     0,     0,   782,   783,   784,     0,     0,     0,   785,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   786,
       0,   787,   788,   789,   790,   791,   792,   793,   794,   795,
     796,   764,   765,     0,     0,     0,     0,     0,     0,     0,
     797,   798,     0,     0,  1230,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   786,     0,   787,   788,   789,   790,
     791,   792,   793,   794,   795,   796,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   797,   798,     0,     0,  1231,
       0,     0,     0,     0,     0,     0,   766,   767,   768,   769,
     770,     0,     0,   771,   772,   773,   774,     0,   775,   776,
     777,   778,     0,     0,     0,     0,   779,     0,   780,   781,
       0,     0,     0,     0,   782,   783,   784,     0,     0,     0,
     785,     0,     0,     0,     0,     0,   766,   767,   768,   769,
     770,     0,     0,   771,   772,   773,   774,     0,   775,   776,
     777,   778,   764,   765,     0,     0,   779,     0,   780,   781,
       0,     0,     0,     0,   782,   783,   784,     0,     0,     0,
     785,     0,     0,     0,     0,   786,     0,   787,   788,   789,
     790,   791,   792,   793,   794,   795,   796,   764,   765,     0,
       0,     0,     0,     0,     0,     0,   797,   798,     0,     0,
    1232,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   786,     0,   787,   788,   789,
     790,   791,   792,   793,   794,   795,   796,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   797,   798,     0,     0,
    1508,     0,     0,     0,     0,     0,     0,   766,   767,   768,
     769,   770,     0,     0,   771,   772,   773,   774,     0,   775,
     776,   777,   778,     0,     0,     0,     0,   779,     0,   780,
     781,     0,     0,     0,     0,   782,   783,   784,     0,     0,
       0,   785,   766,   767,   768,   769,   770,     0,     0,   771,
     772,   773,   774,     0,   775,   776,   777,   778,   764,   765,
       0,     0,   779,     0,   780,   781,     0,     0,     0,     0,
     782,   783,   784,     0,     0,     0,   785,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   786,     0,   787,   788,
     789,   790,   791,   792,   793,   794,   795,   796,   764,   765,
       0,     0,     0,     0,     0,     0,     0,   797,   798,     0,
       0,  1559,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   786,     0,   787,   788,   789,   790,   791,   792,   793,
     794,   795,   796,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   797,   798,     0,     0,  1578,     0,     0,     0,
       0,     0,     0,   766,   767,   768,   769,   770,     0,     0,
     771,   772,   773,   774,     0,   775,   776,   777,   778,     0,
       0,     0,     0,   779,     0,   780,   781,     0,     0,     0,
       0,   782,   783,   784,     0,     0,     0,   785,     0,     0,
       0,     0,     0,   766,   767,   768,   769,   770,     0,     0,
     771,   772,   773,   774,     0,   775,   776,   777,   778,   764,
     765,     0,     0,   779,     0,   780,   781,     0,     0,     0,
       0,   782,   783,   784,     0,     0,     0,   785,     0,     0,
       0,     0,   786,     0,   787,   788,   789,   790,   791,   792,
     793,   794,   795,   796,   764,   765,     0,     0,     0,     0,
       0,     0,     0,   797,   798,     0,     0,  1579,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   786,     0,   787,   788,   789,   790,   791,   792,
     793,   794,   795,   796,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   797,   798,     0,     0,  1593,     0,     0,
       0,     0,     0,     0,   766,   767,   768,   769,   770,     0,
       0,   771,   772,   773,   774,     0,   775,   776,   777,   778,
       0,     0,     0,     0,   779,     0,   780,   781,     0,     0,
       0,     0,   782,   783,   784,     0,     0,     0,   785,   766,
     767,   768,   769,   770,     0,     0,   771,   772,   773,   774,
       0,   775,   776,   777,   778,   764,   765,     0,     0,   779,
       0,   780,   781,     0,     0,     0,     0,   782,   783,   784,
       0,     0,     0,   785,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   786,     0,   787,   788,   789,   790,   791,
     792,   793,   794,   795,   796,   764,   765,     0,     0,     0,
       0,     0,     0,     0,   797,   798,     0,     0,  1595,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   786,     0,
     787,   788,   789,   790,   791,   792,   793,   794,   795,   796,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   797,
     798,     0,     0,  1597,     0,     0,     0,     0,     0,     0,
     766,   767,   768,   769,   770,     0,     0,   771,   772,   773,
     774,     0,   775,   776,   777,   778,     0,     0,     0,     0,
     779,     0,   780,   781,     0,     0,     0,     0,   782,   783,
     784,     0,     0,     0,   785,     0,     0,     0,     0,     0,
     766,   767,   768,   769,   770,     0,     0,   771,   772,   773,
     774,     0,   775,   776,   777,   778,   764,   765,     0,     0,
     779,     0,   780,   781,     0,     0,     0,     0,   782,   783,
     784,     0,     0,     0,   785,     0,     0,     0,     0,   786,
       0,   787,   788,   789,   790,   791,   792,   793,   794,   795,
     796,   764,   765,     0,     0,     0,     0,     0,     0,     0,
     797,   798,     0,     0,  1601,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   786,
       0,   787,   788,   789,   790,   791,   792,   793,   794,   795,
     796,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     797,   798,     0,     0,  1679,     0,     0,     0,     0,     0,
       0,   766,   767,   768,   769,   770,     0,     0,   771,   772,
     773,   774,     0,   775,   776,   777,   778,     0,     0,     0,
       0,   779,     0,   780,   781,     0,     0,     0,     0,   782,
     783,   784,     0,     0,     0,   785,   766,   767,   768,   769,
     770,     0,     0,   771,   772,   773,   774,     0,   775,   776,
     777,   778,   764,   765,     0,     0,   779,     0,   780,   781,
       0,     0,     0,     0,   782,   783,   784,     0,     0,     0,
     785,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     786,     0,   787,   788,   789,   790,   791,   792,   793,   794,
     795,   796,   764,   765,     0,     0,     0,     0,     0,     0,
       0,   797,   798,     0,     0,  1680,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   786,     0,   787,   788,   789,
     790,   791,   792,   793,   794,   795,   796,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   797,   798,     0,     0,
    1682,     0,     0,     0,     0,     0,     0,   766,   767,   768,
     769,   770,     0,     0,   771,   772,   773,   774,     0,   775,
     776,   777,   778,     0,     0,     0,     0,   779,     0,   780,
     781,     0,     0,     0,     0,   782,   783,   784,     0,     0,
       0,   785,     0,     0,     0,     0,     0,   766,   767,   768,
     769,   770,     0,     0,   771,   772,   773,   774,     0,   775,
     776,   777,   778,   764,   765,     0,     0,   779,     0,   780,
     781,     0,     0,     0,     0,   782,   783,   784,     0,     0,
       0,   785,     0,     0,     0,     0,   786,     0,   787,   788,
     789,   790,   791,   792,   793,   794,   795,   796,   764,   765,
       0,     0,     0,     0,     0,     0,     0,   797,   798,     0,
       0,  1717,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   786,     0,   787,   788,
     789,   790,   791,   792,   793,   794,   795,   796,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   797,   798,     0,
       0,  1722,     0,     0,     0,     0,     0,     0,   766,   767,
     768,   769,   770,     0,     0,   771,   772,   773,   774,     0,
     775,   776,   777,   778,     0,     0,     0,     0,   779,     0,
     780,   781,     0,     0,     0,     0,   782,   783,   784,     0,
       0,     0,   785,   766,   767,   768,   769,   770,     0,     0,
     771,   772,   773,   774,     0,   775,   776,   777,   778,   764,
     765,     0,     0,   779,     0,   780,   781,     0,     0,     0,
       0,   782,   783,   784,     0,     0,     0,   785,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   786,     0,   787,
     788,   789,   790,   791,   792,   793,   794,   795,   796,   764,
     765,     0,     0,     0,     0,     0,     0,     0,   797,   798,
     831,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   786,     0,   787,   788,   789,   790,   791,   792,
     793,   794,   795,   796,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   797,   798,  1105,     0,     0,     0,     0,
       0,     0,     0,     0,   766,   767,   768,   769,   770,     0,
       0,   771,   772,   773,   774,     0,   775,   776,   777,   778,
       0,     0,     0,     0,   779,     0,   780,   781,     0,     0,
       0,     0,   782,   783,   784,     0,     0,     0,   785,     0,
       0,     0,     0,     0,   766,   767,   768,   769,   770,     0,
       0,   771,   772,   773,   774,     0,   775,   776,   777,   778,
     764,   765,     0,     0,   779,     0,   780,   781,     0,     0,
       0,     0,   782,   783,   784,     0,     0,     0,   785,     0,
       0,     0,     0,   786,     0,   787,   788,   789,   790,   791,
     792,   793,   794,   795,   796,   764,   765,     0,     0,     0,
       0,     0,     0,     0,   797,   798,  1304,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   786,     0,   787,   788,   789,   790,   791,
     792,   793,   794,   795,   796,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   797,   798,  1320,     0,     0,     0,
       0,     0,     0,     0,     0,   766,   767,   768,   769,   770,
       0,     0,   771,   772,   773,   774,     0,   775,   776,   777,
     778,     0,     0,     0,     0,   779,     0,   780,   781,     0,
       0,     0,     0,   782,   783,   784,     0,     0,     0,   785,
     766,   767,   768,   769,   770,     0,     0,   771,   772,   773,
     774,     0,   775,   776,   777,   778,   339,   340,     0,     0,
     779,     0,   780,   781,     0,     0,     0,     0,   782,   783,
     784,     0,     0,   341,   785,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   786,     0,   787,   788,   789,   790,
     791,   792,   793,   794,   795,   796,     0,     0,   764,   765,
       0,     0,     0,     0,     0,   797,   798,  1490,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   786,
       0,   787,   788,   789,   790,   791,   792,   793,   794,   795,
     796,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     797,   798,  1496,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,     0,     0,   360,   361,   362,     0,     0,     0,     0,
       0,     0,   363,   364,   365,   366,   367,     0,     0,   368,
     369,   370,   371,   372,   373,   374,     0,     0,     0,     0,
       0,     0,     0,   766,   767,   768,   769,   770,     0,     0,
     771,   772,   773,   774,     0,   775,   776,   777,   778,   764,
     765,     0,     0,   779,     0,   780,   781,     0,     0,     0,
       0,   782,   783,   784,     0,     0,     0,   785,     0,     0,
     375,     0,   376,   377,   378,   379,   380,   381,   382,   383,
     384,   385,    13,     0,   386,   387,   764,   765,     0,     0,
       0,   388,   389,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    14,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   786,     0,   787,   788,   789,   790,   791,   792,
     793,   794,   795,   796,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   797,   798,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   766,   767,   768,   769,   770,     0,
       0,   771,   772,   773,   774,     0,   775,   776,   777,   778,
       0,     0,     0,     0,   779,     0,   780,   781,     0,     0,
       0,     0,   782,   783,   784,     0,     0,     0,   785,     0,
       0,   766,   767,   768,   769,   770,     0,     0,   771,   772,
     773,   774,     0,   775,   776,   777,   778,   764,   765,     0,
       0,   779,     0,   780,   781,     0,     0,     0,     0,   782,
     783,   784,     0,     0,     0,   785,     0,     0,     0,     0,
       0,     0,     0,   786,  1309,   787,   788,   789,   790,   791,
     792,   793,   794,   795,   796,   764,   765,     0,     0,     0,
       0,     0,     0,     0,   797,   798,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     786,     0,   787,   788,   789,   790,   791,   792,   793,   794,
     795,   796,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   797,   798,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   766,   767,   768,   769,   770,     0,     0,   771,
     772,   773,   774,     0,   775,   776,   777,   778,     0,     0,
       0,     0,   779,     0,   780,   781,     0,     0,     0,     0,
     782,   783,   784,     0,     0,     0,  -834,     0,     0,     0,
     766,   767,   768,   769,   770,     0,     0,   771,   772,   773,
     774,     0,   775,   776,   777,   778,   764,   765,     0,     0,
     779,     0,   780,   781,     0,     0,     0,     0,   782,   783,
     784,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   786,     0,   787,   788,   789,   790,   791,   792,   793,
     794,   795,   796,   764,   765,     0,     0,     0,     0,     0,
       0,     0,   797,   798,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   786,
       0,   787,   788,   789,   790,   791,   792,   793,   794,   795,
     796,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     797,   798,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   766,   767,   768,   769,   770,     0,     0,   771,   772,
     773,   774,     0,   775,   776,   777,   778,     0,     0,     0,
       0,   779,     0,   780,   781,     0,     0,     0,     0,   782,
       0,   784,     0,     0,     0,     0,     0,     0,   766,   767,
     768,   769,   770,     0,     0,   771,   772,   773,   774,     0,
     775,   776,   777,   778,   764,   765,     0,     0,   779,     0,
     780,   781,     0,     0,     0,     0,   782,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   764,   765,
       0,     0,   787,   788,   789,   790,   791,   792,   793,   794,
     795,   796,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   797,   798,   764,   765,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   787,
     788,   789,   790,   791,   792,   793,   794,   795,   796,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   797,   798,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   766,
     767,   768,   769,   770,     0,     0,   771,   772,   773,   774,
       0,   775,   776,   777,   778,     0,     0,     0,     0,   779,
       0,   780,   781,   766,   767,   768,   769,   770,     0,     0,
     771,   772,   773,   774,     0,   775,   776,   777,   778,     0,
       0,     0,     0,   779,     0,   780,   781,     0,   766,   767,
     768,   769,   770,     0,     0,   771,   772,   773,   774,     0,
     775,   776,   777,   778,   764,   765,     0,     0,   779,     0,
     780,   781,     0,     0,     0,     0,     0,     0,     0,     0,
     787,   788,   789,   790,   791,   792,   793,   794,   795,   796,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   797,
     798,     0,     0,     0,     0,   788,   789,   790,   791,   792,
     793,   794,   795,   796,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   797,   798,     0,     0,     0,     0,     0,
       0,   789,   790,   791,   792,   793,   794,   795,   796,  1011,
       0,     0,     0,     0,     0,     0,     0,     0,   797,   798,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   766,
     767,   768,   769,   770,     0,     0,   771,     0,     0,   774,
       0,   775,   776,   777,   778,     0,     0,     0,     0,   779,
       0,   780,   781,     0,     0,     0,     0,     0,     0,     0,
       0,   290,   291,   292,     0,   294,   295,   296,   297,   298,
     484,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,     0,   312,   313,   314,     0,     0,   317,   318,
     319,   320,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1015,     0,
       0,     0,     0,   790,   791,   792,   793,   794,   795,   796,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   797,
     798,     0,     0,     0,     0,     0,     0,     0,     0,  1305,
       0,     0,     0,     0,     0,     0,     0,     0,  1012,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     290,   291,   292,  1013,   294,   295,   296,   297,   298,   484,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,     0,   312,   313,   314,     0,     0,   317,   318,   319,
     320,   290,   291,   292,     0,   294,   295,   296,   297,   298,
     484,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,     0,   312,   313,   314,     0,     0,   317,   318,
     319,   320,   290,   291,   292,     0,   294,   295,   296,   297,
     298,   484,   300,   301,   302,   303,   304,   305,   306,   307,
     308,   309,   310,     0,   312,   313,   314,  1016,   230,   317,
     318,   319,   320,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1017,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1306,     0,
    1051,  1052,     0,     0,   231,     0,   232,     0,   233,   234,
     235,   236,   237,  1307,   238,   239,   240,   241,   242,   243,
     244,   245,   246,   247,   248,     0,   249,   250,   251,  1053,
       0,   252,   253,   254,   255,  1065,  1066,  1067,  1068,  1069,
    1070,  1071,  1072,     0,  1054,     0,     0,     0,  1073,  1074,
       0,   256,   257,     0,  1075,     0,     0,     0,     0,  -414,
       0,     0,     0,     0,   909,     0,     0,  1076,  1077,     0,
       0,     0,     0,     0,  1078,  1079,  1080,     0,  1065,  1066,
    1067,  1068,  1069,  1070,  1071,  1072,     0,  1055,  1056,     0,
       0,  1073,  1074,     0,     0,     0,     0,  1075,     0,     0,
       0,     0,     0,     0,     0,     0,   258,   909,     0,     0,
    1076,  1077,     0,     0,     0,     0,     0,  1078,  1079,  1080,
       0,  1081,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   518,   875,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1081,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   518,   875
};

static const yytype_int16 yycheck[] =
{
       1,   157,     7,   571,   213,   260,   409,   548,   499,   734,
     458,   736,   672,   711,   440,    15,    16,   755,   463,    20,
     441,   848,   760,   736,   818,   403,   820,   224,   822,   653,
     654,  1379,   182,   552,   458,   438,   562,   762,   564,   966,
     566,   947,  1359,    20,   926,   972,  1244,   949,  1136,     5,
       4,    21,    22,    87,    33,    33,     8,    22,    53,    19,
      20,   270,    20,   113,    53,   733,  1623,   735,  1669,   737,
      70,    71,    72,   166,  1526,    15,    16,   160,    21,    22,
     440,   441,   750,    20,   529,   129,   130,    20,    40,   757,
     127,   829,    57,   126,   497,   498,   224,    20,    20,    62,
     137,   102,    20,    20,   193,   126,   143,    20,   129,   130,
      20,   193,    20,   143,   114,   115,   116,   117,    46,   638,
    1002,    57,   173,   194,   274,   143,   131,    63,   552,   218,
     126,  1732,   651,   144,   145,   146,    15,    16,  1226,   222,
      12,   517,    12,   173,   152,  1702,   102,   103,   118,   119,
     143,    23,    24,    23,    24,   173,   126,   194,   128,   129,
     130,   131,   165,   196,   214,   173,   136,  1619,   219,   158,
     165,   152,   216,   217,   179,   118,   119,   193,   106,   160,
     165,   215,   185,   126,   173,   853,   129,   130,   131,   219,
     185,   193,   173,   136,   185,   216,   217,   935,   203,   204,
     196,   926,   193,   196,   220,   201,   160,   945,   193,   220,
     948,   219,   164,   173,   638,   216,   193,   185,   220,   222,
     199,   199,   227,   219,   224,   193,   166,   651,   164,   589,
     439,   171,   184,   173,   594,   193,   176,  1435,   219,   217,
     205,   397,   127,   277,   200,   223,   216,   217,   184,   458,
     459,   216,   137,  1570,   463,   260,   193,   196,   455,   456,
     193,   185,   638,   219,   269,   217,  1614,   272,  1006,   193,
     193,   193,   896,   216,   217,   193,   193,  1002,  1003,   219,
     193,   409,   222,   193,   803,   193,  1374,   166,   173,   132,
     142,   193,   171,   165,   173,   165,  1384,   176,    33,    55,
     173,  1207,   126,   126,   158,   126,  1208,   710,   464,   194,
     438,   163,   440,  1240,   199,  1197,     7,   985,   986,   173,
     988,   724,   165,    57,   324,    60,    61,   455,   456,    63,
     127,   183,   185,   709,   958,   173,   855,  1685,   127,   127,
     193,   127,   185,   719,   127,   101,   722,   155,   221,   137,
     155,   137,   160,   872,   137,   160,   553,   554,   555,    50,
     557,   558,  1560,   200,   561,   219,   563,   152,   565,   497,
     498,   205,   196,   196,   127,   196,   205,   201,   201,   803,
     201,   143,   219,   592,   137,   640,   762,   596,   173,   124,
      34,   127,   127,   128,   649,   219,   219,   194,   219,   399,
     197,   137,   137,   158,   193,   194,   194,   196,   194,   409,
     199,   194,   220,   158,   222,   188,   189,   222,   173,    63,
       5,     6,   163,     8,   800,   553,   554,   555,   173,   557,
     558,   855,   877,   561,   219,   563,   193,   565,   438,   567,
     440,   194,   183,   143,   219,   218,   107,    47,   872,   163,
     810,    36,   152,  1121,   165,   455,   456,   157,   194,   194,
     195,   218,   107,   107,   199,   220,   826,   202,   871,   183,
     470,   471,  1197,   173,   185,  1552,     0,   193,  1704,   855,
     221,   222,   217,     7,  1010,   165,   197,   978,   223,  1715,
      90,    91,   136,    93,   221,   222,   158,   497,   498,   143,
     200,   193,   165,  1297,   220,   219,    30,  1434,    32,  1000,
      34,   173,   107,   193,   194,  1240,    40,   895,  1243,  1446,
     923,   924,   185,  1147,  1148,  1149,    50,  1240,   220,   173,
    1255,   934,    56,   689,  1760,  1761,   939,   940,   201,   942,
    1367,   944,  1255,   946,   694,   193,   193,   548,   166,   758,
     926,  1628,  1629,   553,   554,   555,    80,   557,   558,   216,
     163,   561,   193,   563,   200,   565,   210,   567,   150,  1646,
    1647,    57,   220,   220,   193,   219,   936,    63,   102,   103,
     183,   165,   710,   219,   165,   165,    33,   963,   193,   220,
     193,   173,   174,   175,   150,   971,   724,   852,   143,   218,
     193,   185,  1270,   193,   185,   185,   173,   152,   188,   864,
     194,  1438,   193,    60,    61,   220,   197,   173,   174,   175,
      70,    71,    72,   205,   206,   193,   881,   220,   173,   884,
     220,   828,  1709,  1710,   216,   640,   764,   765,   218,   158,
     837,   165,   165,   840,   649,   646,   173,   652,   653,   654,
     206,   779,   220,   196,   173,   185,   199,   658,   659,   193,
     216,   185,   185,   193,   114,   115,   116,   117,   163,   797,
     193,  1465,  1098,   674,   675,   676,   677,   124,   679,  1100,
     127,   128,   206,   684,   193,   165,   220,   165,   183,   193,
     137,   691,   144,   217,   146,   193,  1141,    57,    33,   173,
     828,   196,   702,    63,   193,   185,   173,   185,   173,   837,
     710,   220,   840,   193,   165,   193,   220,   165,   165,  1417,
     218,   201,  1516,   201,   724,    60,    61,  1658,   925,  1660,
      33,   220,  1559,   930,   185,  1666,   193,   185,   185,   740,
    1671,  1101,   193,   871,  1469,   193,   165,   194,   195,   165,
     201,  1277,   199,   201,   193,   202,   756,    60,    61,   163,
     165,   193,   193,   220,   173,   193,   185,  1492,   173,   185,
     217,   165,   165,   193,   193,   200,   223,   193,   177,   183,
     185,   220,   201,   980,   981,   201,   218,   218,   193,   124,
     218,   185,   185,   128,   219,   923,   924,   925,   218,  1730,
     193,   152,   930,  1206,   165,   806,   934,   165,   201,  1212,
     152,   939,   940,   165,   942,   222,   944,   173,   946,   947,
     200,   124,   173,  1617,   185,   128,   165,   185,   828,    57,
     165,   173,   184,   185,   186,   193,   164,   837,   197,   219,
     840,   165,   201,   201,  1775,  1776,   185,   852,    47,  1375,
     185,   165,   980,   981,   193,  1515,   184,  1233,   193,   864,
     195,   185,   201,   164,   199,  1225,   201,   202,    67,   193,
    1125,   871,   173,  1276,   210,   458,   881,   201,  1254,   884,
    1811,   886,   217,   184,  1260,   158,    79,   196,   223,  1086,
     199,   896,   195,  1269,   200,  1722,   199,   158,   201,   202,
     173,    94,  1278,    10,  1264,  1265,    99,   164,   101,   165,
     185,  1039,   173,   219,   217,   177,   173,   144,   193,  1116,
     223,   922,   106,   923,   924,   925,  1302,   184,    33,   185,
     930,   173,   165,    33,   934,  1383,   194,   193,  1314,   939,
     940,  1639,   942,  1319,   944,   201,   946,   947,    66,   185,
     196,   184,   185,   958,   187,    60,    61,   193,  1086,  1383,
      60,    61,    12,   197,   173,   966,    35,   201,   969,   552,
    1098,   972,   973,    23,    24,    35,  1714,    57,   219,   193,
     980,   981,   196,    63,   184,   199,  1711,   187,  1116,    57,
     190,  1716,    57,  1369,    57,    63,   219,  1523,    63,   196,
      63,   108,   109,   110,   111,   112,   113,   114,   115,     5,
       6,   177,   178,   196,   121,   122,  1754,   200,   197,   124,
     127,   197,   201,   128,   124,   201,    43,  1752,   128,    25,
     137,   200,   197,   140,   141,    31,   201,  1193,   200,   197,
     147,   148,   149,   201,   197,   197,  1406,   197,   201,   201,
     197,   201,   197,  1429,  1414,   638,   201,   197,   144,   145,
     146,   201,   200,  1064,   177,   178,   179,   180,   651,  1637,
      96,    97,    68,    69,   980,   981,  1479,  1568,  1206,  1207,
    1621,  1457,  1458,   219,  1212,  1461,  1086,   194,   200,  1090,
     195,  1582,   200,  1818,   199,   195,   201,   202,  1098,   199,
     219,   201,   202,   173,   174,   175,   102,   103,   219,    75,
     221,   222,   217,    79,   221,   222,  1116,   217,   223,   200,
    1125,  1376,   200,   223,   177,   178,   179,    93,    94,   177,
     178,   179,    98,    99,   100,   101,  1512,   627,   628,   629,
     219,   200,  1147,  1148,  1149,   200,   200,   143,  1276,    10,
      11,    12,   173,   173,   173,    22,  1697,  1698,   173,   173,
     218,   218,   218,  1360,   160,   161,   162,   163,   173,   173,
     200,   217,    21,    22,  1383,  1372,   173,   173,   219,   185,
     220,   219,   200,   219,   200,   219,  1187,   183,   200,   200,
     200,   200,   200,   200,  1391,   219,   219,   200,   200,    13,
    1576,   200,  1203,    43,   200,  1746,  1206,  1207,   201,   222,
     173,   196,  1212,   173,   166,    33,   200,  1620,   219,   219,
     803,   217,   220,  1478,   219,   219,   217,   219,   218,    10,
     219,    37,  1360,    66,     8,  1726,  1392,   219,   200,   652,
     653,   654,    60,    61,  1372,   200,   200,   142,   219,    13,
     218,   193,   193,   173,   173,   132,  1511,   201,   173,   672,
     193,   193,   219,  1391,  1519,    43,   219,  1643,    33,   118,
     119,   193,   855,    14,   687,   173,  1276,   126,   194,   128,
     129,   130,   131,   196,  1481,    33,   166,   136,   222,   872,
     173,    67,   173,  1696,   219,    60,    61,   108,   109,   110,
     111,   112,   113,   114,   115,   142,   124,   193,   220,   193,
     128,  1687,    60,    61,   184,   898,   899,   900,   901,   902,
     903,   904,   905,   906,   907,   908,   137,   910,   911,   912,
     913,   914,   915,   220,   219,   193,   147,   148,   149,     1,
     219,   200,   220,   219,   219,   200,   219,   219,   219,   201,
     219,  1479,   220,  1481,   219,   204,   205,   206,  1613,   124,
    1360,   173,   219,   128,   173,   201,  1742,   216,   217,   201,
     194,  1376,  1372,   173,   194,   220,   124,   195,   220,   173,
     128,   199,   220,   201,   202,   220,   220,   173,   219,   218,
     220,  1391,   220,   806,   219,   218,   218,   196,   173,   217,
     219,  1598,   173,   220,   173,   223,   899,   900,   901,   902,
     903,   904,   905,   906,   907,   908,   200,   910,   911,   912,
     913,   914,   915,   219,   219,    43,   219,   173,  1683,   173,
     195,   219,    12,  1434,   199,    33,   201,   202,   220,   219,
    1695,   173,   219,    21,    22,  1446,   859,   195,   181,   219,
     218,   199,   217,   201,   202,   173,   218,   220,   223,   173,
     173,   874,   218,   173,   219,   219,  1622,   201,   219,   217,
    1598,   220,   219,  1478,   219,   223,    70,   219,   201,  1479,
     406,  1481,   219,   896,   220,   220,   201,   177,   219,   415,
     220,   219,  1620,   219,   219,    53,  1751,   185,   185,   425,
     220,   220,   184,   220,   220,    33,  1511,   218,   185,   435,
     218,   220,  1513,  1514,  1519,  1724,   220,   193,   220,    33,
    1521,   220,   220,   220,  1733,   220,  1527,   220,   219,   218,
     184,   743,    60,    61,   218,   458,    86,   460,   116,   117,
     118,   119,   220,   218,   218,   958,    60,    61,   126,   220,
     128,   129,   130,   131,   218,     1,   147,  1615,   136,    89,
     138,   139,   488,   489,   221,  1774,   691,  1140,  1696,  1615,
    1615,     1,  1615,  1615,  1143,  1613,   108,   109,   110,   111,
     112,   113,   114,   115,  1476,   511,   512,   513,   514,   515,
     516,   517,  1289,  1394,  1750,   127,   124,  1531,  1598,  1397,
     128,  1190,  1532,  1532,    58,   137,   396,   470,  1613,  1106,
     124,  1716,  1252,   299,   128,   147,   148,   149,  1566,   470,
    1620,   736,    -1,    -1,   202,   203,   204,   205,   206,   552,
      -1,    -1,    -1,  1789,    -1,    -1,    -1,    -1,   216,   217,
      -1,    -1,    -1,    -1,    -1,   571,    -1,    -1,    -1,    -1,
    1651,  1064,    -1,    -1,    -1,   581,    -1,  1658,    -1,  1660,
      -1,    -1,    -1,    -1,    -1,  1666,    -1,   195,    -1,  1669,
    1671,   199,    -1,   201,   202,    -1,    -1,    -1,  1683,    -1,
      -1,   195,    -1,    -1,    -1,   199,    -1,   201,   202,   217,
    1695,    -1,    -1,    -1,    -1,   223,  1696,   623,    -1,    -1,
    1701,    -1,    -1,   217,    -1,    -1,    -1,    -1,    -1,   223,
      -1,   634,   638,    -1,    -1,   638,    -1,    -1,   641,    -1,
     643,  1134,    -1,    -1,    -1,   648,    -1,    -1,   651,  1730,
      -1,    -1,  1732,   656,  1147,  1148,  1149,   660,    -1,  1152,
      -1,  1154,    -1,  1156,    -1,  1158,  1751,  1160,  1749,  1162,
      -1,  1164,    -1,  1166,    -1,  1168,    -1,  1170,    -1,  1172,
      -1,    -1,  1175,    -1,  1177,    -1,  1179,    -1,  1181,    -1,
    1183,    -1,  1185,    -1,  1775,  1776,    -1,    -1,    21,    22,
      -1,    -1,    -1,   709,  1785,  1786,   712,    -1,    -1,   715,
      -1,   717,    -1,   719,    -1,    -1,   722,    -1,    -1,    -1,
    1383,   727,   728,   729,   730,   731,   732,    -1,    -1,    -1,
    1811,   734,  1813,   736,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   762,    -1,    -1,    -1,
     766,   767,    -1,    -1,   770,   771,   772,   773,    -1,   775,
      -1,   777,   778,   779,   780,   781,   782,   783,   784,   785,
     786,   787,   788,   789,   790,   791,   792,   793,   794,   795,
     796,    -1,   798,    -1,   800,   118,   119,    -1,    -1,    -1,
     803,    -1,    -1,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   835,    -1,    -1,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   855,
      -1,    -1,   855,    -1,    -1,    -1,    -1,    -1,   861,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   872,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,   204,   205,   206,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   216,   217,   898,   899,   900,   901,   902,
     903,   904,   905,   906,   907,   908,   909,   910,   911,   912,
     913,   914,   915,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     926,    -1,    -1,    -1,   927,    -1,   929,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,    -1,   961,   962,   963,   144,   145,
     146,    -1,    -1,    -1,   150,   971,    -1,    -1,    -1,    -1,
      -1,    -1,   975,    -1,    -1,    -1,    -1,    -1,    -1,   165,
     983,    -1,    -1,    -1,   990,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,    -1,  1003,    -1,   185,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,
      -1,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    -1,  1515,    -1,    -1,    -1,    -1,    -1,    33,    -1,
     216,   217,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1052,    -1,    -1,    -1,
    1056,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,
      -1,    -1,  1065,  1066,  1067,  1068,  1069,  1070,  1071,  1072,
    1073,  1074,  1075,  1076,  1077,  1078,  1079,  1080,  1081,    -1,
      13,    -1,    -1,    -1,    -1,    -1,    19,   116,   117,   118,
     119,   120,    25,    -1,   123,   124,   125,   126,    31,   128,
     129,   130,   131,    -1,    -1,  1111,    -1,   136,    41,   138,
     139,    -1,    -1,    -1,    -1,  1118,    49,    -1,  1124,   124,
      -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,  1139,    -1,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   200,   201,   202,   203,   204,   205,   206,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    33,
     195,    -1,    -1,    -1,   199,    -1,   201,   202,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    33,
     143,    -1,   217,    -1,    -1,    -1,    60,    61,   223,    -1,
      -1,    -1,    -1,   156,    -1,    -1,    -1,  1233,    -1,  1235,
      -1,    -1,  1238,  1236,  1237,    -1,    60,    61,  1241,    -1,
     173,    -1,    -1,    -1,    -1,    -1,  1249,    -1,  1254,  1252,
      -1,    -1,    -1,    -1,  1260,    -1,    -1,    -1,    13,    -1,
      -1,    -1,    -1,  1269,    19,    -1,    -1,    -1,  1274,  1275,
      25,    -1,  1278,    -1,    33,    -1,    31,    -1,  1284,    -1,
     124,    -1,    -1,    -1,   128,    -1,    41,    -1,   221,    -1,
      -1,    -1,    -1,  1299,    49,  1301,  1302,  1303,    -1,    -1,
     124,    60,    61,  1309,   128,    -1,    -1,  1313,  1314,    64,
      -1,    -1,    -1,  1319,    -1,    -1,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,    -1,
      -1,   195,    -1,    -1,    -1,   199,    -1,   201,   202,    -1,
      -1,    -1,    -1,  1369,    -1,   124,    -1,    -1,    -1,   128,
    1373,   195,    -1,   217,    -1,   199,    -1,  1380,   202,   223,
    1383,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    33,
      -1,    -1,    -1,   217,    -1,    -1,    -1,    -1,    -1,   223,
      -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    60,    61,   173,    -1,
      -1,    -1,    -1,  1429,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,    -1,    -1,
     199,    -1,   201,   202,    -1,    -1,    -1,    -1,    -1,    33,
      -1,  1457,  1458,    -1,    -1,  1461,    -1,    -1,   217,    -1,
      -1,    -1,    -1,  1469,   223,    -1,   221,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,
     124,  1487,    -1,  1489,   128,    -1,  1492,    -1,    -1,  1495,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1512,  1510,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,  1537,    -1,  1539,   144,   145,   146,
     124,    -1,    -1,   150,   128,  1548,  1549,  1550,    -1,  1555,
      -1,   195,  1558,    -1,    -1,   199,  1562,   201,   202,  1565,
    1563,  1564,    -1,    -1,    -1,    -1,  1569,    -1,    -1,  1575,
    1576,    -1,    -1,   217,    -1,    -1,    -1,    -1,    -1,   223,
      -1,    -1,    -1,    -1,    -1,    -1,  1589,    -1,   195,    -1,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,   195,  1615,   220,    -1,   199,    -1,   201,   202,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1637,    -1,   217,    -1,    -1,    -1,  1643,    -1,   223,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1652,
    1653,   108,   109,   110,   111,   112,   113,   114,   115,    -1,
      -1,    -1,  1668,    -1,   121,   122,    -1,    -1,    -1,    -1,
     127,  1674,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     137,  1687,    -1,   140,   141,  1688,    -1,    -1,    -1,    -1,
     147,   148,   149,    -1,    -1,    -1,  1699,    -1,    -1,    -1,
      -1,    -1,    -1,  1706,    -1,  1708,    -1,    -1,   165,    -1,
      -1,    -1,    -1,    -1,    -1,    33,    -1,    -1,    -1,    -1,
      -1,  1727,    -1,    -1,    -1,    -1,    -1,    -1,   185,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1742,   194,    -1,    -1,
    1743,    -1,    60,    61,  1747,  1748,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   221,   222,  1769,    -1,  1771,  1772,
       1,    -1,  1778,    -1,     5,     6,     7,    -1,     9,    10,
      11,  1784,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      31,    -1,    -1,    -1,    -1,    -1,   124,    38,    39,    40,
     128,    42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,
      51,    52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,    -1,    -1,   195,    -1,    -1,
      -1,   199,    -1,   201,   202,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,
      -1,    -1,   133,   134,   135,   223,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,   184,   185,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,    -1,
     221,   222,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      33,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      53,    -1,    55,    -1,    -1,    -1,    -1,    60,    61,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,
      -1,   124,    -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,
     153,   154,   155,    -1,   157,   158,   159,   160,   161,   162,
     163,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   195,    -1,    -1,    -1,   199,    -1,    -1,   202,
     203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,
      -1,     5,     6,    -1,   217,    -1,   219,    -1,   221,   222,
     223,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    33,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
     124,    -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,
     154,   155,    -1,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   195,    -1,    -1,    -1,   199,    -1,    -1,   202,   203,
     204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,    -1,
       5,     6,    -1,   217,    -1,   219,    -1,   221,   222,   223,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    31,    -1,    33,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,
      55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,
     155,    -1,   157,    -1,   159,   160,   161,   162,   163,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   199,    -1,    -1,   202,   203,   204,
      -1,   206,    -1,    -1,   209,   210,    -1,    -1,    -1,     5,
       6,    -1,   217,    -1,   219,    -1,   221,   222,   223,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   127,    -1,    -1,    -1,    -1,    -1,   133,   134,   135,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,
      -1,   217,    -1,   219,    -1,   221,   222,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,
      -1,    -1,    -1,    -1,    -1,   133,   134,   135,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,   157,
      -1,   159,   160,   161,   162,   163,    -1,    -1,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,    -1,
      -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,
      -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,
      -1,   219,    -1,   221,   222,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    52,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,   152,   153,   154,   155,    -1,   157,    -1,   159,
     160,   161,   162,   163,    -1,    -1,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,    -1,    -1,    -1,
      -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,
     210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,
      -1,   221,   222,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   133,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,
     152,   153,   154,   155,    -1,   157,    -1,   159,   160,   161,
     162,   163,    -1,    -1,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,
      -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,
      -1,     5,     6,    -1,    -1,   217,    -1,   219,    -1,   221,
     222,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    70,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,
     154,   155,    -1,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
     204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,
       6,    -1,    -1,   217,    -1,   219,   220,   221,   222,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   133,   134,   135,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,    -1,    -1,     5,
       6,   217,    -1,   219,    -1,   221,   222,    13,    -1,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,    -1,    -1,     5,
       6,   217,   218,   219,    -1,   221,   222,    13,    -1,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,   158,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,    -1,    -1,     5,
       6,   217,    -1,   219,    -1,   221,   222,    13,    -1,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,    -1,    -1,     5,
       6,   217,   218,   219,    -1,   221,   222,    13,    -1,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,
      -1,   217,    -1,   219,    -1,   221,   222,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    70,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,    -1,
      -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,
      -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,
      -1,   219,    -1,   221,   222,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,   152,   153,   154,   155,    -1,   157,    -1,   159,
     160,   161,   162,   163,    -1,    -1,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,    -1,    -1,    -1,
      -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,
     210,    -1,    -1,    -1,    -1,     5,     6,   217,   218,   219,
      -1,   221,   222,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,   152,   153,   154,   155,    -1,   157,    -1,   159,
     160,   161,   162,   163,    -1,    -1,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,    -1,    -1,    -1,
      -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,
     210,    -1,    -1,    -1,    -1,     5,     6,   217,    -1,   219,
      -1,   221,   222,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,   152,   153,   154,   155,    -1,   157,    -1,   159,
     160,   161,   162,   163,    -1,    -1,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,    -1,    -1,    -1,
      -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,
     210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,
      -1,   221,   222,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    61,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,
     152,   153,   154,   155,    -1,   157,    -1,   159,   160,   161,
     162,   163,    -1,    -1,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,
      -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,
      -1,     5,     6,    -1,    -1,   217,    -1,   219,    -1,   221,
     222,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,
     154,   155,    -1,   157,    -1,   159,   160,   161,   162,   163,
      -1,    -1,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
     204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,
       6,    -1,    -1,   217,    -1,   219,    -1,   221,   222,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   200,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,
      -1,   217,    -1,   219,    -1,   221,   222,    15,    16,    17,
      18,    19,    -1,    -1,    22,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,   157,
      -1,   159,   160,   161,   162,   163,    -1,    -1,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,    -1,
      -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,
      -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,
      -1,   219,    -1,   221,   222,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,   152,   153,   154,   155,    -1,   157,    -1,   159,
     160,   161,   162,   163,    -1,    -1,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,    -1,    -1,    -1,
      -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,
     210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,
     220,   221,   222,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,
     152,   153,   154,   155,    -1,   157,    -1,   159,   160,   161,
     162,   163,    -1,    -1,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,
      -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,    -1,
     202,   203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,
      -1,     5,     6,    -1,    -1,   217,    -1,   219,    -1,   221,
     222,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,
     154,   155,    -1,   157,    -1,   159,   160,   161,   162,   163,
      -1,    -1,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
     204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,
       6,    -1,    -1,   217,    -1,   219,   220,   221,   222,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,
      -1,   217,    -1,   219,   220,   221,   222,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,   157,
      -1,   159,   160,   161,   162,   163,    -1,    -1,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,    -1,
      -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,
      -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,
      -1,   219,   220,   221,   222,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,   152,   153,   154,   155,    -1,   157,    -1,   159,
     160,   161,   162,   163,    -1,    -1,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,    -1,    -1,    -1,
      -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,
     210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,
     220,   221,   222,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,
     152,   153,   154,   155,    -1,   157,    -1,   159,   160,   161,
     162,   163,    -1,    -1,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,
      -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,
      -1,     5,     6,    -1,    -1,   217,    -1,   219,   220,   221,
     222,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,
     154,   155,    -1,   157,    -1,   159,   160,   161,   162,   163,
      -1,    -1,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
     204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,
       6,    -1,    -1,   217,    -1,   219,   220,   221,   222,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,
      -1,   217,    -1,   219,   220,   221,   222,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,   157,
      -1,   159,   160,   161,   162,   163,    -1,    -1,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,    -1,
      -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,
      -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,
      -1,   219,    -1,   221,   222,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,   152,   153,   154,   155,    -1,   157,    -1,   159,
     160,   161,   162,   163,    -1,    -1,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,    -1,    -1,    -1,
      -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,
     210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,
      -1,   221,   222,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,
     152,   153,   154,   155,    -1,   157,    -1,   159,   160,   161,
     162,   163,    -1,    -1,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,
      -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,
      -1,     5,     6,    -1,    -1,   217,    -1,   219,    -1,   221,
     222,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,
     154,   155,    -1,   157,    -1,   159,   160,   161,   162,   163,
      -1,    -1,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
     204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,
       6,    -1,    -1,   217,    -1,   219,    -1,   221,   222,    15,
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
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    19,    -1,    -1,    -1,    -1,   183,    25,    -1,
      -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    -1,   202,   203,   204,    -1,
     206,    -1,    49,   209,   210,    -1,    -1,    -1,    -1,    -1,
      -1,   217,    -1,   219,    -1,   221,   222,    64,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,    41,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,   156,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      64,    -1,    -1,    -1,    -1,    -1,   173,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   221,    -1,   223,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   165,    -1,    -1,    -1,    -1,    19,    -1,    -1,   173,
      -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,
      -1,   185,    -1,    -1,    -1,    -1,    -1,    -1,    41,   193,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,   221,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      -1,    -1,    -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
     173,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,   221,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    21,    22,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   156,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
      -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,
      -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   221,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    21,    22,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,
     220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     195,    -1,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   216,   217,    -1,    -1,   220,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    21,    22,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,   195,    -1,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   216,   217,    -1,    -1,   220,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   195,    -1,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   216,   217,    -1,    -1,   220,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   195,    -1,   197,   198,   199,   200,   201,   202,
     203,   204,   205,   206,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   216,   217,    -1,    -1,   220,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
      -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    21,    22,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,   195,    -1,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,
      -1,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     216,   217,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,   220,
      -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    21,    22,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,
     220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,
     220,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    21,    22,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,
      -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   195,    -1,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   216,   217,    -1,    -1,   220,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    21,
      22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,    -1,   195,    -1,   197,   198,   199,   200,   201,   202,
     203,   204,   205,   206,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   216,   217,    -1,    -1,   220,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   195,    -1,   197,   198,   199,   200,   201,   202,
     203,   204,   205,   206,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   216,   217,    -1,    -1,   220,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    21,    22,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   216,   217,    -1,    -1,   220,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    21,    22,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,   195,
      -1,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     216,   217,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,
      -1,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     216,   217,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    21,    22,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     195,    -1,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   216,   217,    -1,    -1,   220,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,
     220,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    21,    22,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,
      -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,
      -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    21,
      22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
     218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   195,    -1,   197,   198,   199,   200,   201,   202,
     203,   204,   205,   206,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   216,   217,   218,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   216,   217,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   216,   217,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    21,    22,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    38,   150,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    -1,    -1,    21,    22,
      -1,    -1,    -1,    -1,    -1,   216,   217,   218,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,
      -1,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     216,   217,   218,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,    -1,    -1,   128,   129,   130,    -1,    -1,    -1,    -1,
      -1,    -1,   137,   138,   139,   140,   141,    -1,    -1,   144,
     145,   146,   147,   148,   149,   150,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    21,
      22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
     195,    -1,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   165,    -1,   209,   210,    21,    22,    -1,    -1,
      -1,   216,   217,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   185,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   195,    -1,   197,   198,   199,   200,   201,   202,
     203,   204,   205,   206,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   216,   217,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    21,    22,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   216,   217,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     195,    -1,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   216,   217,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    21,    22,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   195,    -1,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   216,   217,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,
      -1,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     216,   217,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
      -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   216,   217,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,    -1,    -1,    -1,    -1,   198,   199,   200,   201,   202,
     203,   204,   205,   206,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   216,   217,    -1,    -1,    -1,    -1,    -1,
      -1,   199,   200,   201,   202,   203,   204,   205,   206,    19,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,    -1,    -1,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,
      -1,    -1,    -1,   200,   201,   202,   203,   204,   205,   206,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   158,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      71,    72,    73,   173,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,   158,    35,    98,
      99,   100,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   158,    -1,
     129,   130,    -1,    -1,    71,    -1,    73,    -1,    75,    76,
      77,    78,    79,   173,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,   158,
      -1,    98,    99,   100,   101,   108,   109,   110,   111,   112,
     113,   114,   115,    -1,   173,    -1,    -1,    -1,   121,   122,
      -1,   118,   119,    -1,   127,    -1,    -1,    -1,    -1,   132,
      -1,    -1,    -1,    -1,   137,    -1,    -1,   140,   141,    -1,
      -1,    -1,    -1,    -1,   147,   148,   149,    -1,   108,   109,
     110,   111,   112,   113,   114,   115,    -1,   216,   217,    -1,
      -1,   121,   122,    -1,    -1,    -1,    -1,   127,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   173,   137,    -1,    -1,
     140,   141,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,
      -1,   194,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   221,   222,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   194,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   221,   222
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   225,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   102,   103,   165,   185,   206,   217,   226,   227,   231,
     240,   242,   243,   248,   280,   285,   321,   405,   412,   416,
     423,   470,   475,   480,    19,    20,   173,   272,   273,   274,
     166,   249,   250,   150,   173,   174,   175,   206,   216,   244,
     245,   246,   163,   183,   290,   413,   173,   221,   229,    57,
      63,   408,   408,   408,   143,   173,   307,    34,    63,   107,
     136,   210,   219,   276,   277,   278,   279,   307,   226,     5,
       6,     8,    36,   420,    62,   403,   194,   193,   196,   193,
     205,   205,   245,   205,    22,    57,   205,   216,   247,   408,
     409,   411,   409,   403,   481,   471,   476,   173,   143,   241,
     278,   278,   278,   219,   144,   145,   146,   193,   218,   107,
     107,   107,   284,    57,    63,   414,    57,    63,   421,    57,
      63,   404,    15,    16,   166,   171,   173,   176,   219,   222,
     233,   273,   166,   250,   245,   245,   245,   173,   244,   244,
     173,   165,   185,   378,   164,   184,   291,   409,   226,    57,
      63,   228,   173,   173,   173,   173,   177,   239,   220,   274,
     278,   278,   278,   278,    57,    63,   286,   288,   173,   415,
     424,   290,   406,   177,   178,   179,   232,    15,    16,   166,
     171,   173,   233,   270,   271,   222,   247,   410,   165,   378,
     210,   230,   482,   472,   477,   177,   220,   289,   196,   290,
     106,   418,   419,   401,   160,   222,   275,   372,   177,   178,
     179,   193,   220,   173,   194,    66,   196,   434,   290,   290,
      35,    71,    73,    75,    76,    77,    78,    79,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    93,
      94,    95,    98,    99,   100,   101,   118,   119,   173,   283,
     287,    75,    79,    93,    94,    98,    99,   100,   101,   428,
     407,   173,   425,   291,   402,   274,   273,   222,   226,   152,
     173,   399,   400,   270,    19,    25,    31,    41,    49,    64,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   156,   221,   307,   427,   429,   430,   435,
     441,   469,    79,    94,    99,   101,   290,   473,   478,    21,
      22,    38,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     128,   129,   130,   137,   138,   139,   140,   141,   144,   145,
     146,   147,   148,   149,   150,   195,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   209,   210,   216,   217,
      35,    35,   219,   281,   290,   292,   290,   173,   291,   196,
     417,   290,   422,   372,   218,   273,   219,    43,   193,   196,
     199,   398,   200,   200,   200,   219,   200,   200,   219,   434,
     200,   200,   200,   200,   200,   219,   307,    33,    60,    61,
     124,   128,   195,   199,   202,   217,   223,   440,   197,   433,
     389,   392,   173,   173,   173,   218,    22,   173,   218,   155,
     220,   372,   384,   385,   386,   126,   196,   282,   295,   407,
     194,   378,   307,   379,   400,   218,     5,     6,    15,    16,
      17,    18,    19,    25,    27,    31,    39,    45,    48,    51,
      55,    65,    68,    69,    80,   102,   103,   104,   118,   119,
     151,   152,   153,   154,   155,   157,   159,   160,   161,   162,
     166,   167,   168,   169,   170,   171,   172,   174,   175,   176,
     183,   202,   203,   204,   209,   210,   217,   219,   221,   222,
     238,   240,   290,   301,   307,   312,   326,   333,   336,   339,
     344,   347,   351,   352,   354,   359,   362,   363,   371,   427,
     483,   498,   509,   513,   526,   529,   173,   173,   441,   127,
     137,   194,   397,   442,   447,   449,   363,   451,   445,   173,
     200,   453,   455,   457,   459,   461,   463,   465,   467,   363,
     200,   219,   298,    33,   199,    33,   199,   217,   223,   218,
     363,   217,   223,   441,   173,   291,   173,   193,   226,   387,
     438,   469,   474,   173,   390,   438,   479,   173,   108,   109,
     110,   111,   112,   113,   114,   115,   137,   147,   148,   149,
     108,   109,   110,   111,   112,   113,   114,   115,   127,   137,
     147,   148,   149,   219,     7,    50,   320,   185,   185,   193,
     220,   469,   469,     1,     9,    10,    11,    13,    26,    28,
      29,    38,    40,    42,    44,    52,    54,    58,    59,    65,
     104,   105,   133,   134,   135,   165,   174,   185,   251,   252,
     255,   258,   259,   261,   264,   265,   266,   267,   291,   293,
     294,   296,   301,   306,   308,   313,   314,   315,   316,   317,
     318,   319,   321,   325,   348,   350,   363,   370,   291,   370,
      40,   217,   280,   291,   381,   378,   200,   200,   200,   309,
     429,   483,   219,   307,   200,     5,   102,   103,   200,   219,
     200,   219,   219,   200,   200,   219,   200,   219,   200,   219,
     200,   200,   219,   200,   200,   363,   363,   219,   219,   219,
     219,   219,   219,    13,   441,    13,   441,    13,   370,   508,
     524,   200,   200,   237,    13,   363,   363,   363,   363,   363,
      13,    49,   297,   337,   363,   337,   222,    13,   299,   508,
     525,   173,   219,   280,    21,    22,   116,   117,   118,   119,
     120,   123,   124,   125,   126,   128,   129,   130,   131,   136,
     138,   139,   144,   145,   146,   150,   195,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   216,   217,   220,
     219,    43,   226,   397,   306,   348,   370,   469,   469,   439,
     469,   220,   469,   469,   220,   201,   436,   469,   281,   469,
     281,   469,   281,   387,   388,   390,   391,   220,   444,   297,
     218,   218,   363,   165,   193,   194,   432,   196,   438,   291,
     196,   438,   291,   363,   152,   173,   394,   395,   426,   386,
     386,   386,   370,   262,   263,   127,   306,   337,   363,   370,
     292,    61,   370,   269,   370,   173,   226,   166,    58,   370,
     292,   200,   127,   306,   370,   222,   292,   339,   343,   343,
     343,   370,   226,   226,   370,    10,    37,   339,   345,   226,
     226,   226,   226,   226,    66,   322,   132,   226,   108,   109,
     110,   111,   112,   113,   114,   115,   121,   122,   127,   137,
     140,   141,   147,   148,   149,   194,   345,   378,   380,   279,
       8,   372,   377,   499,   501,   310,   219,   307,   200,   219,
     334,   200,   200,   200,   520,   337,   441,   299,   363,   327,
     329,   363,   331,   363,   522,   337,   505,   510,   337,   503,
     441,   363,   363,   363,   363,   363,   363,   426,    53,   158,
     173,   202,   217,   219,   370,   484,   487,   491,   507,   512,
     426,   219,   487,   512,   426,   142,   184,   186,   226,   492,
     302,   304,   179,   180,   232,   219,   219,   426,    13,   218,
     193,   528,   528,   152,   157,   200,   307,   353,   426,   291,
     193,   528,    70,   217,   220,   337,   484,   486,     4,   160,
     342,    19,   158,   173,   427,    19,   158,   173,   427,   363,
     363,   363,   363,   363,   363,   173,   363,   158,   173,   363,
     363,   363,   427,   363,   363,   363,   363,   363,   363,    22,
     363,   363,   363,   363,   363,   363,   363,   363,   363,   363,
     363,   129,   130,   158,   173,   216,   217,   360,   427,   363,
     220,   337,   173,   306,   370,   108,   109,   110,   111,   112,
     113,   114,   115,   121,   122,   127,   140,   141,   147,   148,
     149,   194,   226,   201,   201,   201,   438,   201,   201,   173,
     431,   201,   282,   201,   282,   201,   282,   201,   438,   201,
     438,   300,   469,   193,   528,   218,   370,   193,   469,   469,
     220,   219,    43,   127,   193,   194,   196,   199,   393,   292,
     426,   219,   306,   337,   193,    14,   370,   173,   292,   194,
     196,   166,   441,   306,   370,   222,   280,   292,   292,   260,
     290,   346,   160,   219,   324,   400,   343,   133,   134,   135,
     293,   349,   370,   349,   370,   349,   370,   349,   370,   349,
     370,   349,   370,   349,   370,   349,   370,   349,   370,   349,
     370,   349,   370,   370,   349,   370,   349,   370,   349,   370,
     349,   370,   349,   370,   349,   370,   173,   218,    57,    63,
     375,    67,   376,   226,   441,   441,   469,    70,   337,   486,
     497,   200,   370,   173,   370,   469,   514,   516,   518,   441,
     528,   201,   438,   220,   220,   441,   441,   220,   441,   220,
     441,   528,   441,   388,   528,   391,   201,   220,   220,   220,
     220,   220,   220,    20,   343,   219,   137,   393,   217,   363,
     220,   142,   193,   226,   491,   188,   189,   218,   495,   193,
     188,   218,   226,   494,    20,   220,   491,   184,   187,   493,
      20,   370,   184,   508,   300,   300,   370,   426,   426,    20,
     219,   426,   363,   220,   219,   219,   355,   357,    20,   508,
     173,   220,   486,   484,   193,   220,   193,   528,   220,   219,
     127,   137,   173,   194,   199,   340,   341,   281,   200,   219,
     200,   219,   219,   219,   218,    19,   158,   173,   427,   196,
     158,   173,   363,   219,   219,   158,   173,   363,     1,   219,
     218,   220,   226,   370,   370,   370,   370,   370,   370,   370,
     370,   370,   370,   370,   370,   370,   370,   370,   370,   370,
     443,   448,   450,   469,   452,   446,   193,   201,   226,   454,
     201,   458,   201,   462,   201,   466,   387,   468,   390,   201,
     438,   220,   165,   432,   363,   173,   173,   469,   370,    20,
     426,   292,   194,   268,   201,   342,    12,    23,    24,   165,
     253,   254,   370,   295,   280,   173,   323,   323,   343,   343,
     343,   194,   226,    47,   376,    46,   106,   373,   378,   201,
     201,   201,   486,   220,   220,   220,   173,   220,   201,   226,
     220,   201,   441,   388,   391,   201,   220,   219,   441,   201,
     201,   201,   201,   220,   201,   201,   220,   201,   342,   219,
     337,   363,   370,   370,   487,   491,   370,   158,   173,   484,
     495,   218,   370,   218,   507,   337,   487,   184,   187,   190,
     496,   218,   337,   201,   201,   196,   235,    20,    20,   337,
     426,    20,   363,   363,   441,   281,   337,   220,   218,   217,
     341,   173,   173,   219,   173,   173,   193,   218,   282,   364,
     363,   366,   363,   220,   337,   363,   200,   219,   363,   219,
     218,   363,   217,   220,   337,   219,   218,   361,   220,   337,
     173,   437,   173,   456,   460,   464,   298,   469,   220,    43,
     393,   337,    20,   469,   370,   342,   281,   292,   254,   370,
      12,   256,   291,   342,   193,   218,   220,   469,   378,    33,
     374,   373,   375,   500,   502,   311,   220,   201,   438,   219,
     173,   335,   201,   201,   201,   521,   299,   201,   328,   330,
     332,   523,   506,   511,   504,   219,   220,   337,   185,   220,
     491,   495,   219,   137,   393,   185,   491,   218,   185,   303,
     305,   236,   181,   337,   337,   185,    20,   337,   220,   220,
     201,   282,   185,   220,   484,   220,   173,   340,   218,   142,
     292,   338,   441,   220,   469,   220,   220,   220,   368,   363,
     363,   220,   484,   220,   363,   220,   173,   370,   292,   337,
     226,   226,   345,   282,   292,   257,   226,   281,   173,   218,
     196,   398,   226,   382,   374,   394,   395,   396,   219,   219,
     370,   173,   370,   201,   515,   517,   519,   219,   220,   219,
     370,   370,   370,   219,    70,   497,   219,   219,   220,   363,
     220,   363,   137,   393,   495,   363,   370,   370,   363,   496,
     508,   370,   298,   234,   220,   220,   363,   337,   185,   356,
     201,   508,   218,   220,   127,   370,   201,   201,   469,   220,
     220,   218,   220,   220,   338,   165,   254,    26,   105,   258,
     313,   314,   315,   317,   370,   282,   196,   398,   441,   397,
     378,   287,   383,   497,   497,   220,   201,   220,   219,   219,
     219,   219,   297,   299,   337,   497,   497,   220,   226,   527,
     370,   370,   220,   527,   527,   177,   185,   185,   527,   220,
     363,   353,   358,   527,   220,   370,   365,   367,   201,   220,
     292,   254,   127,   127,   370,   292,   441,   397,   397,   370,
     226,   287,   219,   484,   488,   489,   490,   490,   370,   370,
     497,   497,   484,   485,   220,   220,   528,   490,   485,    53,
     218,   137,   393,   184,   291,   508,   363,   218,   185,   527,
     353,   291,   369,   370,   397,   370,   370,   226,   378,   292,
     484,   193,   528,   220,   220,   220,   220,   490,   490,   220,
     220,   220,   220,   370,   218,   370,   370,   218,   291,   527,
     527,   363,   218,   370,   226,   226,   378,   220,   219,   220,
     220,   184,   218,   527,   226,   484,   218,   220
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   224,   225,   225,   225,   225,   225,   225,   225,   225,
     225,   225,   225,   225,   225,   225,   225,   226,   226,   227,
     228,   228,   228,   229,   229,   230,   230,   231,   232,   232,
     232,   232,   233,   233,   234,   234,   235,   236,   235,   237,
     237,   237,   238,   239,   239,   241,   240,   242,   243,   244,
     244,   244,   245,   245,   245,   245,   245,   245,   245,   246,
     246,   247,   247,   248,   249,   249,   250,   250,   251,   252,
     252,   253,   253,   254,   254,   254,   254,   255,   255,   256,
     257,   256,   258,   258,   258,   258,   258,   259,   259,   260,
     259,   262,   261,   263,   261,   264,   265,   266,   268,   267,
     269,   267,   270,   270,   270,   270,   270,   270,   271,   271,
     272,   272,   272,   273,   273,   273,   273,   273,   273,   273,
     273,   273,   274,   274,   275,   275,   275,   276,   276,   276,
     276,   277,   277,   278,   278,   278,   278,   278,   278,   278,
     279,   279,   280,   280,   281,   281,   281,   282,   282,   282,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   284,
     284,   285,   286,   286,   286,   287,   289,   288,   290,   290,
     291,   291,   292,   292,   293,   293,   293,   294,   294,   294,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   294,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   295,
     295,   295,   296,   297,   297,   298,   298,   299,   299,   300,
     300,   302,   303,   301,   304,   305,   301,   306,   306,   306,
     306,   306,   307,   307,   307,   308,   308,   310,   311,   309,
     309,   312,   312,   312,   312,   312,   312,   313,   314,   315,
     315,   315,   316,   316,   316,   317,   317,   318,   318,   318,
     319,   320,   320,   320,   321,   321,   322,   322,   323,   323,
     324,   324,   324,   324,   325,   325,   327,   328,   326,   329,
     330,   326,   331,   332,   326,   334,   335,   333,   336,   336,
     336,   336,   336,   336,   337,   337,   338,   338,   338,   339,
     339,   339,   340,   340,   340,   340,   340,   341,   341,   342,
     342,   342,   343,   343,   344,   346,   345,   347,   347,   347,
     347,   347,   347,   347,   348,   348,   348,   348,   348,   348,
     348,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     348,   348,   348,   349,   349,   349,   349,   350,   350,   350,
     350,   350,   350,   350,   350,   350,   350,   350,   350,   350,
     350,   350,   350,   350,   351,   351,   352,   352,   353,   353,
     354,   355,   356,   354,   357,   358,   354,   359,   359,   359,
     359,   359,   359,   359,   360,   361,   359,   362,   362,   362,
     362,   362,   362,   362,   363,   363,   363,   363,   363,   363,
     363,   363,   363,   363,   363,   363,   363,   363,   363,   363,
     363,   363,   363,   363,   363,   363,   363,   363,   363,   363,
     363,   363,   363,   363,   363,   363,   363,   363,   363,   363,
     363,   363,   363,   363,   363,   363,   363,   363,   363,   363,
     363,   363,   363,   363,   363,   363,   363,   363,   363,   363,
     364,   365,   363,   363,   363,   363,   366,   367,   363,   363,
     363,   368,   369,   363,   363,   363,   363,   363,   363,   363,
     363,   363,   363,   363,   363,   363,   363,   363,   370,   371,
     371,   371,   371,   371,   371,   371,   371,   371,   371,   371,
     371,   371,   371,   371,   371,   372,   372,   372,   373,   373,
     373,   374,   374,   375,   375,   375,   376,   376,   377,   378,
     378,   378,   378,   379,   380,   379,   381,   379,   382,   379,
     383,   379,   379,   384,   385,   385,   386,   386,   386,   386,
     386,   387,   387,   388,   388,   389,   389,   389,   390,   391,
     391,   392,   392,   392,   393,   393,   394,   394,   394,   395,
     395,   396,   396,   397,   397,   397,   398,   398,   399,   399,
     399,   399,   399,   400,   400,   400,   400,   400,   400,   401,
     402,   401,   403,   403,   404,   404,   404,   405,   406,   405,
     407,   407,   407,   408,   408,   408,   410,   409,   411,   411,
     412,   413,   412,   414,   414,   414,   415,   416,   416,   417,
     417,   418,   418,   419,   420,   420,   420,   420,   421,   421,
     421,   422,   422,   424,   425,   423,   426,   426,   426,   426,
     426,   427,   427,   427,   427,   427,   427,   427,   427,   427,
     427,   427,   427,   427,   427,   427,   427,   427,   427,   427,
     427,   427,   427,   427,   427,   427,   427,   427,   428,   428,
     428,   428,   428,   428,   428,   428,   429,   430,   430,   430,
     431,   431,   431,   432,   432,   433,   433,   433,   433,   433,
     433,   433,   434,   434,   434,   434,   434,   435,   436,   437,
     435,   438,   438,   439,   439,   440,   440,   441,   441,   441,
     441,   441,   441,   442,   443,   441,   441,   441,   444,   441,
     441,   441,   441,   441,   441,   441,   441,   441,   441,   441,
     441,   441,   445,   446,   441,   441,   447,   448,   441,   449,
     450,   441,   451,   452,   441,   441,   453,   454,   441,   455,
     456,   441,   441,   457,   458,   441,   459,   460,   441,   441,
     461,   462,   441,   463,   464,   441,   465,   466,   441,   467,
     468,   441,   469,   469,   469,   471,   472,   473,   474,   470,
     476,   477,   478,   479,   475,   481,   482,   480,   483,   483,
     483,   483,   483,   484,   484,   484,   484,   484,   484,   484,
     484,   485,   485,   486,   487,   487,   488,   488,   489,   489,
     490,   490,   491,   491,   492,   492,   493,   493,   494,   494,
     495,   495,   495,   496,   496,   496,   497,   497,   498,   498,
     498,   498,   498,   498,   499,   500,   498,   501,   502,   498,
     503,   504,   498,   505,   506,   498,   507,   507,   507,   508,
     508,   509,   510,   511,   509,   512,   512,   513,   513,   513,
     514,   515,   513,   516,   517,   513,   518,   519,   513,   513,
     520,   521,   513,   513,   522,   523,   513,   524,   524,   525,
     525,   526,   526,   526,   526,   526,   527,   527,   528,   528,
     529,   529,   529,   529,   529,   529,   529,   529,   529
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
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     4,     0,     1,     1,     3,     0,     4,     1,     1,
       1,     1,     3,     7,     2,     2,     6,     1,     1,     1,
       1,     1,     2,     2,     1,     1,     1,     1,     1,     1,
       2,     2,     1,     1,     1,     1,     2,     2,     2,     0,
       2,     2,     3,     0,     2,     0,     4,     0,     2,     1,
       3,     0,     0,     7,     0,     0,     7,     3,     2,     2,
       2,     1,     1,     3,     2,     2,     3,     0,     0,     5,
       1,     2,     5,     5,     5,     6,     2,     1,     1,     1,
       2,     3,     2,     2,     3,     2,     3,     2,     2,     3,
       4,     1,     1,     0,     1,     1,     1,     0,     1,     3,
       9,     8,     8,     7,     3,     3,     0,     0,     7,     0,
       0,     7,     0,     0,     7,     0,     0,     6,     5,     8,
      10,     5,     8,    10,     1,     3,     1,     2,     3,     1,
       1,     2,     2,     2,     2,     2,     4,     1,     3,     0,
       4,     4,     1,     6,     6,     0,     7,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     2,     2,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     6,     8,     5,     6,     1,     4,
       3,     0,     0,     8,     0,     0,     9,     3,     4,     5,
       6,     8,     5,     6,     0,     0,     5,     3,     4,     4,
       5,     4,     3,     4,     1,     1,     1,     2,     2,     2,
       2,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     2,     2,     2,     4,     5,     4,
       5,     3,     4,     2,     5,     1,     1,     1,     1,     1,
       1,     1,     4,     1,     1,     4,     4,     7,     8,     3,
       0,     0,     8,     3,     3,     3,     0,     0,     8,     3,
       4,     0,     0,     9,     4,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     3,     2,     4,     1,     1,     4,
       4,     4,     4,     4,     1,     6,     7,     6,     6,     7,
       7,     6,     7,     6,     6,     0,     4,     1,     0,     1,
       1,     0,     1,     0,     1,     1,     0,     1,     5,     1,
       1,     2,     0,     0,     0,     8,     0,     5,     0,    10,
       0,    11,     6,     3,     3,     4,     1,     1,     3,     3,
       3,     1,     3,     1,     3,     0,     2,     3,     3,     1,
       3,     0,     2,     3,     1,     1,     1,     2,     3,     3,
       5,     1,     1,     1,     1,     1,     0,     1,     1,     4,
       3,     3,     5,     4,     6,     5,     5,     4,     4,     0,
       0,     5,     0,     1,     0,     1,     1,     6,     0,     6,
       0,     3,     5,     0,     1,     1,     0,     5,     2,     3,
       4,     0,     4,     0,     1,     1,     1,     7,     9,     0,
       2,     0,     1,     3,     1,     1,     2,     2,     0,     1,
       1,     0,     3,     0,     0,     7,     1,     4,     3,     3,
       5,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     4,
       1,     3,     3,     1,     2,     0,     3,     3,     2,     5,
       5,     4,     0,     2,     2,     2,     2,     4,     0,     0,
       7,     1,     1,     1,     3,     3,     4,     1,     1,     1,
       1,     2,     3,     0,     0,     6,     4,     3,     0,     7,
       4,     2,     2,     3,     2,     3,     2,     2,     3,     3,
       3,     2,     0,     0,     6,     2,     0,     0,     6,     0,
       0,     6,     0,     0,     6,     1,     0,     0,     6,     0,
       0,     7,     1,     0,     0,     6,     0,     0,     7,     1,
       0,     0,     6,     0,     0,     7,     0,     0,     6,     0,
       0,     6,     1,     3,     3,     0,     0,     0,     0,    10,
       0,     0,     0,     0,    10,     0,     0,     9,     1,     1,
       1,     1,     1,     3,     3,     5,     5,     6,     6,     8,
       8,     0,     1,     2,     1,     3,     3,     5,     1,     2,
       1,     0,     0,     2,     2,     1,     2,     1,     2,     1,
       2,     1,     1,     2,     1,     1,     0,     1,     5,     4,
       6,     7,     5,     7,     0,     0,    10,     0,     0,    10,
       0,     0,    10,     0,     0,     7,     1,     3,     3,     3,
       1,     5,     0,     0,    10,     1,     3,     3,     4,     4,
       0,     0,    11,     0,     0,    11,     0,     0,    10,     5,
       0,     0,     9,     5,     0,     0,    10,     1,     3,     1,
       3,     3,     3,     4,     7,     9,     0,     3,     0,     1,
       9,    11,    12,    11,    10,    10,    10,     9,    10
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
                yyextra->g_Program->options.push_back(opt);
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

  case 259: /* optional_template: %empty  */
                                        { (yyval.b) = false; }
    break;

  case 260: /* optional_template: "template"  */
                                        { (yyval.b) = true; }
    break;

  case 261: /* global_function_declaration: optional_annotation_list "def" optional_template function_declaration  */
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

  case 262: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 263: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 264: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 265: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 266: /* $@9: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 267: /* function_declaration: optional_public_or_private_function $@9 function_declaration_header expression_block  */
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

  case 268: /* open_block: "begin of code block"  */
          { (yyval.ui) = 0xdeadbeef; }
    break;

  case 269: /* open_block: "new scope"  */
                        { (yyval.ui) = (yyvsp[0].i); }
    break;

  case 270: /* close_block: "end of code block"  */
          { (yyval.ui) = 0xdeadbeef; }
    break;

  case 271: /* close_block: "close scope"  */
                         { (yyval.ui) = (yyvsp[0].i); }
    break;

  case 272: /* expression_block: open_block expressions close_block  */
                                                                  {
        auto prev_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(prev_loc, (yyvsp[-2].ui), format::get_substring(prev_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size, format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
    }
    break;

  case 273: /* expression_block: open_block expressions close_block "finally" open_block expressions close_block  */
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

  case 274: /* expr_call_pipe: expr expr_full_block_assumed_piped  */
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

  case 275: /* expr_call_pipe: expression_keyword expr_full_block_assumed_piped  */
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

  case 276: /* expr_call_pipe: "generator" '<' type_declaration_no_options '>' optional_capture_list expr_full_block_assumed_piped  */
                                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-3].pTypeDecl),(yyvsp[-1].pCaptList),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 277: /* expression_any: SEMICOLON  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 278: /* expression_any: "end of expression"  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 279: /* expression_any: expr_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 280: /* expression_any: expr_keyword  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 281: /* expression_any: expr_assign_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 282: /* expression_any: expr_assign semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 283: /* expression_any: expression_delete semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 284: /* expression_any: expression_let  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 285: /* expression_any: expression_while_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 286: /* expression_any: expression_unsafe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 287: /* expression_any: expression_with  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 288: /* expression_any: expression_with_alias  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 289: /* expression_any: expression_for_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 290: /* expression_any: expression_break semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 291: /* expression_any: expression_continue semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 292: /* expression_any: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 293: /* expression_any: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 294: /* expression_any: expression_if_then_else  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 295: /* expression_any: expression_try_catch  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 296: /* expression_any: expression_label semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 297: /* expression_any: expression_goto semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 298: /* expression_any: "pass" semicolon  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 299: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 300: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 301: /* expressions: expressions error  */
                                 {
        delete (yyvsp[-1].pExpression); (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 302: /* expr_keyword: "keyword" expr expression_block  */
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

  case 303: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 304: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 305: /* optional_expr_list_in_braces: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 306: /* optional_expr_list_in_braces: '(' optional_expr_list optional_comma ')'  */
                                                             { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 307: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 308: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 309: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 310: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 311: /* $@10: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 312: /* $@11: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 313: /* expression_keyword: "keyword" '<' $@10 type_declaration_no_options_list '>' $@11 expr  */
                                                                                                                                                     {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 314: /* $@12: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 315: /* $@13: %empty  */
                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 316: /* expression_keyword: "type function" '<' $@12 type_declaration_no_options_list '>' $@13 optional_expr_list_in_braces  */
                                                                                                                                                                                   {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 317: /* expr_pipe: expr_assign " <|" expr_block  */
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

  case 318: /* expr_pipe: "@ <|" expr_block  */
                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
            format::get_writer() << "@";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[-1]))));
        }

        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 319: /* expr_pipe: "@@ <|" expr_block  */
                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
            format::get_writer() << "@@";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[-1]))));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 320: /* expr_pipe: "$ <|" expr_block  */
                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
            format::get_writer() << "$";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[-1]))));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 321: /* expr_pipe: expr_call_pipe  */
                             {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 322: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 323: /* name_in_namespace: "name" "::" "name"  */
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

  case 324: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 325: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 326: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 327: /* $@14: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 328: /* $@15: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 329: /* new_type_declaration: '<' $@14 type_declaration '>' $@15  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 330: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 331: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 332: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 333: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 334: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 335: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 336: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 337: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 338: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 339: /* expression_return_no_pipe: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 340: /* expression_return_no_pipe: "return" expr_list  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),sequenceToTuple((yyvsp[0].pExpression)));
    }
    break;

  case 341: /* expression_return_no_pipe: "return" "<-" expr_list  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),sequenceToTuple((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 342: /* expression_return: expression_return_no_pipe semicolon  */
                                                    {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 343: /* expression_return: "return" expr_pipe  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 344: /* expression_return: "return" "<-" expr_pipe  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 345: /* expression_yield_no_pipe: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 346: /* expression_yield_no_pipe: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 347: /* expression_yield: expression_yield_no_pipe semicolon  */
                                                   {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 348: /* expression_yield: "yield" expr_pipe  */
                                          {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 349: /* expression_yield: "yield" "<-" expr_pipe  */
                                                 {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 350: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                              {
        const auto end_block = format::Pos::from_last(tokAt(scanner, (yylsp[-2])));
        const auto start = format::Pos::from(tokAt(scanner, (yylsp[-3])));
        if (format::is_replace_braces()) {
            format::skip_spaces_or_print(tokAt(scanner, (yylsp[-3])), tokAt(scanner, (yylsp[-2])), tokAt(scanner, (yylsp[-1])), yyextra->das_tab_size);
        }

        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 351: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 352: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 353: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 354: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 355: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 356: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 357: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 358: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 359: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 360: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
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

  case 361: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 362: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 363: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 364: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 365: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 366: /* $@16: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 367: /* $@17: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 368: /* expr_cast: "cast" '<' $@16 type_declaration_no_options '>' $@17 expr  */
                                                                                                                                                {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
    }
    break;

  case 369: /* $@18: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 370: /* $@19: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 371: /* expr_cast: "upcast" '<' $@18 type_declaration_no_options '>' $@19 expr  */
                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 372: /* $@20: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 373: /* $@21: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 374: /* expr_cast: "reinterpret" '<' $@20 type_declaration_no_options '>' $@21 expr  */
                                                                                                                                                       {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 375: /* $@22: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 376: /* $@23: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 377: /* expr_type_decl: "type" '<' $@22 type_declaration '>' $@23  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 378: /* expr_type_info: "typeinfo" '(' name_in_namespace expr ')'  */
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

  case 379: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" '>' expr ')'  */
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

  case 380: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" c_or_s "name" '>' expr ')'  */
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

  case 381: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
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

  case 382: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
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

  case 383: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" semicolon "name" '>' '(' expr ')'  */
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

  case 384: /* expr_list: expr2  */
                       {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 385: /* expr_list: expr_list ',' expr2  */
                                             {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 386: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 387: /* block_or_simple_block: "=>" expr  */
                                        {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 388: /* block_or_simple_block: "=>" "<-" expr  */
                                               {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 389: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 390: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 391: /* block_or_lambda: '@' '@'  */
                { (yyval.i) = 2;   /* local function */ }
    break;

  case 392: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 393: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 394: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 395: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 396: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 397: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 398: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 399: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 400: /* optional_capture_list: "[[" capture_list ']' ']'  */
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

  case 401: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 402: /* expr_block: expression_block  */
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

  case 403: /* expr_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 404: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 405: /* $@24: %empty  */
                             {  yyextra->das_need_oxford_comma = false; }
    break;

  case 406: /* expr_full_block_assumed_piped: block_or_lambda $@24 optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type expression_block  */
                                                                                       {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 407: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 408: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 409: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 410: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 411: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 412: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 413: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 414: /* expr_assign: expr  */
                                             { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 415: /* expr_assign: expr '=' expr  */
                                             { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 416: /* expr_assign: expr "<-" expr  */
                                             { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 417: /* expr_assign: expr ":=" expr  */
                                             { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 418: /* expr_assign: expr "&=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 419: /* expr_assign: expr "|=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 420: /* expr_assign: expr "^=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 421: /* expr_assign: expr "&&=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 422: /* expr_assign: expr "||=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 423: /* expr_assign: expr "^^=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 424: /* expr_assign: expr "+=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 425: /* expr_assign: expr "-=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 426: /* expr_assign: expr "*=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 427: /* expr_assign: expr "/=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 428: /* expr_assign: expr "%=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 429: /* expr_assign: expr "<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 430: /* expr_assign: expr ">>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 431: /* expr_assign: expr "<<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 432: /* expr_assign: expr ">>>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 433: /* expr_assign_pipe_right: "@ <|" expr_block  */
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

  case 434: /* expr_assign_pipe_right: "@@ <|" expr_block  */
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

  case 435: /* expr_assign_pipe_right: "$ <|" expr_block  */
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

  case 436: /* expr_assign_pipe_right: expr_call_pipe  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 437: /* expr_assign_pipe: expr '=' expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 438: /* expr_assign_pipe: expr "<-" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 439: /* expr_assign_pipe: expr "&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 440: /* expr_assign_pipe: expr "|=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 441: /* expr_assign_pipe: expr "^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 442: /* expr_assign_pipe: expr "&&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 443: /* expr_assign_pipe: expr "||=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 444: /* expr_assign_pipe: expr "^^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 445: /* expr_assign_pipe: expr "+=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 446: /* expr_assign_pipe: expr "-=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 447: /* expr_assign_pipe: expr "*=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 448: /* expr_assign_pipe: expr "/=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 449: /* expr_assign_pipe: expr "%=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 450: /* expr_assign_pipe: expr "<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 451: /* expr_assign_pipe: expr ">>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 452: /* expr_assign_pipe: expr "<<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 453: /* expr_assign_pipe: expr ">>>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 454: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 455: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 456: /* expr_method_call: expr2 "->" "name" '(' ')'  */
                                                          {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 457: /* expr_method_call: expr2 "->" "name" '(' expr_list ')'  */
                                                                               {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 458: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 459: /* func_addr_name: "$i" '(' expr2 ')'  */
                                           {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 460: /* func_addr_expr: '@' '@' func_addr_name  */
                                          {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 461: /* $@25: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 462: /* $@26: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 463: /* func_addr_expr: '@' '@' '<' $@25 type_declaration_no_options '>' $@26 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 464: /* $@27: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 465: /* $@28: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 466: /* func_addr_expr: '@' '@' '<' $@27 optional_function_argument_list optional_function_type '>' $@28 func_addr_name  */
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

  case 467: /* expr_field: expr2 '.' "name"  */
                                               {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 468: /* expr_field: expr2 '.' '.' "name"  */
                                                   {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 469: /* expr_field: expr2 '.' "name" '(' ')'  */
                                                       {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 470: /* expr_field: expr2 '.' "name" '(' expr_list ')'  */
                                                                            {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 471: /* expr_field: expr2 '.' "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                        {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 472: /* expr_field: expr2 '.' basic_type_declaration '(' ')'  */
                                                                         {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 473: /* expr_field: expr2 '.' basic_type_declaration '(' expr_list ')'  */
                                                                                              {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 474: /* $@29: %empty  */
                                { yyextra->das_suppress_errors=true; }
    break;

  case 475: /* $@30: %empty  */
                                                                             { yyextra->das_suppress_errors=false; }
    break;

  case 476: /* expr_field: expr2 '.' $@29 error $@30  */
                                                                                                                     {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), "");
        yyerrok;
    }
    break;

  case 477: /* expr_call: name_in_namespace '(' ')'  */
                                               {
            (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),*(yyvsp[-2].s));
            delete (yyvsp[-2].s);
    }
    break;

  case 478: /* expr_call: name_in_namespace '(' "uninitialized" ')'  */
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

  case 479: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
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

  case 480: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
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

  case 481: /* expr_call: name_in_namespace '(' expr_list ')'  */
                                                                    {
            (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),*(yyvsp[-3].s)),(yyvsp[-1].pExpression));
            delete (yyvsp[-3].s);
    }
    break;

  case 482: /* expr_call: basic_type_declaration '(' ')'  */
                                                    {
        (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-2].type)));
    }
    break;

  case 483: /* expr_call: basic_type_declaration '(' expr_list ')'  */
                                                                         {
        (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-3].type))),(yyvsp[-1].pExpression));
    }
    break;

  case 484: /* expr2: name_in_namespace  */
                                              { need_wrap_current_expr = true; (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 485: /* expr2: expr_field  */
                                              { need_wrap_current_expr = true; (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0])); (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 486: /* expr2: expr_mtag  */
                                              { need_wrap_current_expr = true; (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0])); (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 487: /* expr2: '!' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"!",(yyvsp[0].pExpression)); }
    break;

  case 488: /* expr2: '~' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"~",(yyvsp[0].pExpression)); }
    break;

  case 489: /* expr2: '+' expr2  */
                                                   { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"+",(yyvsp[0].pExpression)); }
    break;

  case 490: /* expr2: '-' expr2  */
                                                   { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"-",(yyvsp[0].pExpression)); }
    break;

  case 491: /* expr2: expr2 "<<" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 492: /* expr2: expr2 ">>" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 493: /* expr2: expr2 "<<<" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 494: /* expr2: expr2 ">>>" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 495: /* expr2: expr2 '+' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 496: /* expr2: expr2 '-' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 497: /* expr2: expr2 '*' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 498: /* expr2: expr2 '/' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 499: /* expr2: expr2 '%' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 500: /* expr2: expr2 '<' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 501: /* expr2: expr2 '>' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 502: /* expr2: expr2 "==" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 503: /* expr2: expr2 "!=" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 504: /* expr2: expr2 "<=" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 505: /* expr2: expr2 ">=" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 506: /* expr2: expr2 '&' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 507: /* expr2: expr2 '|' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 508: /* expr2: expr2 '^' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 509: /* expr2: expr2 "&&" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 510: /* expr2: expr2 "||" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 511: /* expr2: expr2 "^^" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 512: /* expr2: expr2 ".." expr2  */
                                               {
        need_wrap_current_expr = true;
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 513: /* expr2: "++" expr2  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"++", (yyvsp[0].pExpression)); }
    break;

  case 514: /* expr2: "--" expr2  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"--", (yyvsp[0].pExpression)); }
    break;

  case 515: /* expr2: expr2 "++"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 516: /* expr2: expr2 "--"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 517: /* expr2: expr2 '[' expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprAt(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 518: /* expr2: expr2 '.' '[' expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprAt(tokRangeAt(scanner,(yylsp[-4]), (yylsp[0])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 519: /* expr2: expr2 "?[" expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeAt(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 520: /* expr2: expr2 '.' "?[" expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeAt(tokRangeAt(scanner,(yylsp[-4]), (yylsp[0])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 521: /* expr2: expr2 "?." "name"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeField(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 522: /* expr2: expr2 '.' "?." "name"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeField(tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 523: /* expr2: '*' expr2  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 524: /* expr2: expr2 '?' expr2 ':' expr2  */
                                                             {
        need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp3(tokRangeAt(scanner,(yylsp[-4]), (yylsp[0])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 525: /* expr2: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 526: /* expr2: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 527: /* expr2: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 528: /* expr2: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 529: /* expr2: make_decl  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 530: /* expr2: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 531: /* expr2: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 532: /* expr2: '(' expr_list optional_comma ')'  */
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
                (yyvsp[-2].pExpression)->at = tokAt(scanner, (yylsp[-2]));
                (yyval.pExpression) = (yyvsp[-2].pExpression);
            }
        }
    break;

  case 533: /* expr2: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 534: /* expr2: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 535: /* expr2: "deref" '(' expr2 ')'  */
                                                    { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 536: /* expr2: "addr" '(' expr2 ')'  */
                                                    { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 537: /* expr2: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 538: /* expr2: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr2 ')'  */
                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 539: /* expr2: expr2 "??" expr2  */
                                                     { (yyval.pExpression) = new ExprNullCoalescing(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 540: /* $@31: %empty  */
                                                { yyextra->das_arrow_depth ++; }
    break;

  case 541: /* $@32: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 542: /* expr2: expr2 "is" "type" '<' $@31 type_declaration_no_options '>' $@32  */
                                                                                                                                                             {
        (yyval.pExpression) = new ExprIs(tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 543: /* expr2: expr2 "is" basic_type_declaration  */
                                                                {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 544: /* expr2: expr2 "is" "name"  */
                                               {
        (yyval.pExpression) = new ExprIsVariant(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 545: /* expr2: expr2 "as" "name"  */
                                               {
        (yyval.pExpression) = new ExprAsVariant(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 546: /* $@33: %empty  */
                                                { yyextra->das_arrow_depth ++; }
    break;

  case 547: /* $@34: %empty  */
                                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 548: /* expr2: expr2 "as" "type" '<' $@33 type_declaration '>' $@34  */
                                                                                                                                                  {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 549: /* expr2: expr2 "as" basic_type_declaration  */
                                                                {
        (yyval.pExpression) = new ExprAsVariant(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 550: /* expr2: expr2 '?' "as" "name"  */
                                                   {
        (yyval.pExpression) = new ExprSafeAsVariant(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 551: /* $@35: %empty  */
                                                    { yyextra->das_arrow_depth ++; }
    break;

  case 552: /* $@36: %empty  */
                                                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 553: /* expr2: expr2 '?' "as" "type" '<' $@35 type_declaration '>' $@36  */
                                                                                                                                                      {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokRangeAt(scanner,(yylsp[-8]), (yylsp[-1])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 554: /* expr2: expr2 '?' "as" basic_type_declaration  */
                                                                    {
        (yyval.pExpression) = new ExprSafeAsVariant(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 555: /* expr2: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 556: /* expr2: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 557: /* expr2: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 558: /* expr2: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 559: /* expr2: expr_method_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); (yyval.pExpression)->at = tokAt(scanner, (yylsp[0])); }
    break;

  case 560: /* expr2: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 561: /* expr2: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 562: /* expr2: expr2 "<|" expr2  */
                                                  { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); }
    break;

  case 563: /* expr2: expr2 "|>" expr2  */
                                                  { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-2]), (yylsp[0]))); (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])); }
    break;

  case 564: /* expr2: expr2 "|>" basic_type_declaration  */
                                                           {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])));
    }
    break;

  case 565: /* expr2: name_in_namespace "name"  */
                                                  {
        if (format::prepare_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-1]))))) {
            format::get_writer() << "." << format::get_substring(tokAt(scanner,(yylsp[0])));
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        (yyval.pExpression) = ast_NameName(scanner,(yyvsp[-1].s),(yyvsp[0].s),tokRangeAt(scanner,(yylsp[-1]),(yylsp[0])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 566: /* expr2: "unsafe" '(' expr2 ')'  */
                                          {
        (yyvsp[-1].pExpression)->alwaysSafe = true;
        (yyvsp[-1].pExpression)->userSaidItsSafe = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 567: /* expr2: expression_keyword  */
                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 568: /* expr: expr2  */
                                       {
        if (need_wrap_current_expr) {
            format::wrap_par_expr_newline(tokAt(scanner,(yylsp[0])), (yyvsp[0].pExpression)->at);
            need_wrap_current_expr = false;
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 569: /* expr_mtag: "$$" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 570: /* expr_mtag: "$i" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 571: /* expr_mtag: "$v" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 572: /* expr_mtag: "$b" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 573: /* expr_mtag: "$a" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 574: /* expr_mtag: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 575: /* expr_mtag: "$c" '(' expr2 ')' '(' ')'  */
                                                             {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 576: /* expr_mtag: "$c" '(' expr2 ')' '(' expr_list ')'  */
                                                                                 {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 577: /* expr_mtag: expr2 '.' "$f" '(' expr2 ')'  */
                                                                  {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 578: /* expr_mtag: expr2 "?." "$f" '(' expr2 ')'  */
                                                                   {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 579: /* expr_mtag: expr2 '.' '.' "$f" '(' expr2 ')'  */
                                                                      {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 580: /* expr_mtag: expr2 '.' "?." "$f" '(' expr2 ')'  */
                                                                       {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 581: /* expr_mtag: expr2 "as" "$f" '(' expr2 ')'  */
                                                                     {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 582: /* expr_mtag: expr2 '?' "as" "$f" '(' expr2 ')'  */
                                                                         {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 583: /* expr_mtag: expr2 "is" "$f" '(' expr2 ')'  */
                                                                     {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 584: /* expr_mtag: '@' '@' "$c" '(' expr2 ')'  */
                                                          {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 585: /* optional_field_annotation: %empty  */
                                                        { (yyval.aaList) = nullptr; }
    break;

  case 586: /* optional_field_annotation: "[[" annotation_argument_list ']' ']'  */
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

  case 587: /* optional_field_annotation: metadata_argument_list  */
                                                        { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 588: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 589: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 590: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 591: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 592: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 593: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 594: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 595: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 596: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 597: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 598: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 599: /* opt_sem: SEMICOLON  */
                { (yyval.b) = false; }
    break;

  case 600: /* opt_sem: "end of expression"  */
          { (yyval.b) = true; }
    break;

  case 601: /* opt_sem: "end of expression" SEMICOLON  */
                    { (yyval.b) = true; }
    break;

  case 602: /* opt_sem: %empty  */
                  {(yyval.b) = false; }
    break;

  case 603: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 604: /* $@37: %empty  */
                                                               { yyextra->das_force_oxford_comma=true;}
    break;

  case 605: /* struct_variable_declaration_list: struct_variable_declaration_list "typedef" $@37 "name" '=' type_declaration semicolon opt_sem  */
                                                                                                                                                                 {
        (yyval.pVarDeclList) = (yyvsp[-7].pVarDeclList);
        ast_structureAlias(scanner,(yyvsp[-4].s),(yyvsp[-2].pTypeDecl),tokAt(scanner,(yylsp[-6])));
    }
    break;

  case 606: /* $@38: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 607: /* struct_variable_declaration_list: struct_variable_declaration_list $@38 structure_variable_declaration semicolon opt_sem  */
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

  case 608: /* $@39: %empty  */
                                                                                                                     {
                yyextra->das_force_oxford_comma=true;
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 609: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable "abstract" optional_constant $@39 function_declaration_header semicolon opt_sem  */
                                                                  {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure->isTemplate;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-9].pVarDeclList),(yyvsp[-8].faList),(yyvsp[-6].b),(yyvsp[-4].b), (yyvsp[-2].pFuncDecl));
            }
    break;

  case 610: /* $@40: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 611: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@40 function_declaration_header expression_block opt_sem  */
                                                                                {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure->isTemplate;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-10].pVarDeclList),(yyvsp[-9].faList),(yyvsp[-6].b),(yyvsp[-7].b),(yyvsp[-5].i),(yyvsp[-4].b),(yyvsp[-2].pFuncDecl),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-8]),(yylsp[-1])),tokAt(scanner,(yylsp[-9])));
            }
    break;

  case 612: /* struct_variable_declaration_list: struct_variable_declaration_list '[' annotation_list ']' semicolon opt_sem  */
                                                                                               {
        das_yyerror(scanner,"structure field or class method annotation expected to remain on the same line with the field or the class",
            tokAt(scanner,(yylsp[-3])), CompilationError::syntax_error);
        delete (yyvsp[-3].faList);
        (yyval.pVarDeclList) = (yyvsp[-5].pVarDeclList);
    }
    break;

  case 613: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 614: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 615: /* function_argument_declaration_type: "$a" '(' expr2 ')'  */
                                      {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 616: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 617: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 618: /* function_argument_list: function_argument_declaration_no_type "end of expression" function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 619: /* function_argument_list: function_argument_declaration_type "end of expression" function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 620: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 621: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 622: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 623: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 624: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                          { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 625: /* tuple_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 626: /* tuple_alias_type_list: tuple_alias_type_list c_or_s  */
                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 627: /* tuple_alias_type_list: tuple_alias_type_list tuple_type c_or_s  */
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

  case 628: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 629: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 630: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 631: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 632: /* variant_alias_type_list: variant_alias_type_list c_or_s  */
                                           {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 633: /* variant_alias_type_list: variant_alias_type_list variant_type c_or_s  */
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

  case 634: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 635: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 636: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 637: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 638: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 639: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 640: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 641: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 642: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 643: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 644: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 645: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 646: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 647: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 648: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 649: /* let_variable_name_with_pos_list: "$i" '(' expr2 ')'  */
                                      {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 650: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 651: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 652: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 653: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options semicolon  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 654: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 655: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 656: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr semicolon  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 657: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr  */
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

  case 658: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr_pipe  */
                                                                                                           {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-3]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 659: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 660: /* $@41: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 661: /* global_variable_declaration_list: global_variable_declaration_list $@41 optional_field_annotation let_variable_declaration opt_sem  */
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

  case 662: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 663: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 664: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 665: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 666: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 667: /* global_let: kwd_let optional_shared optional_public_or_private_variable open_block global_variable_declaration_list close_block  */
                                                                                                                                                                   {
        handle_brace(format::Pos::from(tokAt(scanner, (yylsp[-2]))), (yyvsp[-2].ui),
                     format::get_substring(tokRangeAt(scanner, (yylsp[-2]), (yylsp[0]))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 668: /* $@42: %empty  */
                                                                                        {
        yyextra->das_force_oxford_comma=true;
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 669: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@42 optional_field_annotation let_variable_declaration  */
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

  case 670: /* enum_list: %empty  */
        {
        (yyval.pEnumList) = new Enumeration();
    }
    break;

  case 671: /* enum_list: enum_list "name" opt_sem  */
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

  case 672: /* enum_list: enum_list "name" '=' expr opt_sem  */
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

  case 673: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 674: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 675: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 676: /* $@43: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 677: /* single_alias: optional_public_or_private_alias "name" $@43 '=' type_declaration  */
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

  case 678: /* alias_list: single_alias opt_sem  */
                                    {
        (yyval.positions) = new vector<LineInfo>(1, tokAt(scanner, (yylsp[-1])));
    }
    break;

  case 679: /* alias_list: alias_list single_alias opt_sem  */
                                                       {
        (yyvsp[-2].positions)->emplace_back(tokAt(scanner, (yylsp[-1])));
        (yyval.positions) = (yyvsp[-2].positions);
    }
    break;

  case 680: /* alias_declaration: "typedef" open_block alias_list close_block  */
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

  case 681: /* $@44: %empty  */
                    { yyextra->das_force_oxford_comma=true;}
    break;

  case 683: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 684: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 685: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 686: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 687: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name open_block enum_list close_block  */
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

  case 688: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name ':' enum_basic_type_declaration open_block enum_list close_block  */
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

  case 689: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 690: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 691: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 692: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 693: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 694: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 695: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 696: /* class_or_struct: "class" "template"  */
                                  { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 697: /* class_or_struct: "struct" "template"  */
                                  { (yyval.i) = CorS_StructTemplate; }
    break;

  case 698: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 699: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 700: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 701: /* optional_struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 702: /* optional_struct_variable_declaration_list: open_block struct_variable_declaration_list close_block  */
                                                                                      {
        const auto prev_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(prev_loc, (yyvsp[-2].ui),
                     format::get_substring(prev_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 703: /* $@45: %empty  */
                                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 704: /* $@46: %empty  */
                         {
        if ( (yyvsp[0].pStructure) ) {
            (yyvsp[0].pStructure)->isClass = (yyvsp[-3].i)==CorS_Class || (yyvsp[-3].i)==CorS_ClassTemplate;
            (yyvsp[0].pStructure)->isTemplate = (yyvsp[-3].i)==CorS_ClassTemplate || (yyvsp[-3].i)==CorS_StructTemplate;
            (yyvsp[0].pStructure)->privateStructure = !(yyvsp[-2].b);
        }
    }
    break;

  case 705: /* structure_declaration: optional_annotation_list class_or_struct optional_public_or_private_structure $@45 structure_name $@46 optional_struct_variable_declaration_list  */
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

  case 706: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 707: /* variable_name_with_pos_list: "$i" '(' expr2 ')'  */
                                      {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 708: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 709: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 710: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 711: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 712: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 713: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 714: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 715: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 716: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 717: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 718: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 719: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 720: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 721: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 722: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 723: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 724: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 725: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 726: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 727: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 728: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 729: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 730: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 731: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 732: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 733: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 734: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 735: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 736: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 737: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 738: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 739: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 740: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 741: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 742: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 743: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 744: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 745: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 746: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 747: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 748: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 749: /* auto_type_declaration: "$t" '(' expr2 ')'  */
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

  case 750: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 751: /* bitfield_bits: bitfield_bits semicolon "name"  */
                                                 {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 752: /* bitfield_bits: bitfield_bits ',' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 755: /* bitfield_alias_bits: %empty  */
       {
        auto pSL = new vector<tuple<string,Expression *>>();
        (yyval.pNameExprList) = pSL;

    }
    break;

  case 756: /* bitfield_alias_bits: bitfield_alias_bits "name" SEMICOLON  */
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

  case 757: /* bitfield_alias_bits: bitfield_alias_bits "name" commas  */
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

  case 758: /* bitfield_alias_bits: bitfield_alias_bits "name"  */
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

  case 759: /* bitfield_alias_bits: bitfield_alias_bits "name" '=' expr SEMICOLON  */
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

  case 760: /* bitfield_alias_bits: bitfield_alias_bits "name" '=' expr commas  */
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

  case 761: /* bitfield_alias_bits: bitfield_alias_bits "name" '=' expr  */
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

  case 762: /* bitfield_basic_type_declaration: %empty  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 763: /* bitfield_basic_type_declaration: ':' "uint8"  */
                             { (yyval.type) = Type::tBitfield8; }
    break;

  case 764: /* bitfield_basic_type_declaration: ':' "uint16"  */
                             { (yyval.type) = Type::tBitfield16; }
    break;

  case 765: /* bitfield_basic_type_declaration: ':' "uint"  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 766: /* bitfield_basic_type_declaration: ':' "uint64"  */
                             { (yyval.type) = Type::tBitfield64; }
    break;

  case 767: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' '>'  */
                                                                          {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-2].type));
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-2]));
    }
    break;

  case 768: /* $@47: %empty  */
                                                                     { yyextra->das_arrow_depth ++; }
    break;

  case 769: /* $@48: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 770: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' $@47 bitfield_bits '>' $@48  */
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

  case 773: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 774: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 775: /* dim_list: '[' expr2 ']'  */
                              {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 776: /* dim_list: dim_list '[' expr2 ']'  */
                                             {
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 777: /* type_declaration_no_options: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 778: /* type_declaration_no_options: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 779: /* type_declaration_no_options: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 780: /* type_declaration_no_options: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 781: /* type_declaration_no_options: type_declaration_no_options dim_list  */
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

  case 782: /* type_declaration_no_options: type_declaration_no_options '[' ']'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->dim.push_back(TypeDecl::dimAuto);
        (yyvsp[-2].pTypeDecl)->dimExpr.push_back(nullptr);
        (yyvsp[-2].pTypeDecl)->removeDim = false;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 783: /* $@49: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 784: /* $@50: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 785: /* type_declaration_no_options: "type" '<' $@49 type_declaration '>' $@50  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 786: /* type_declaration_no_options: "typedecl" '(' expr2 ')'  */
                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 787: /* type_declaration_no_options: '$' name_in_namespace optional_expr_list_in_braces  */
                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-1]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-1])), *(yyvsp[-1].s)));
        delete (yyvsp[-1].s);
    }
    break;

  case 788: /* $@51: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 789: /* type_declaration_no_options: '$' name_in_namespace '<' $@51 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 790: /* type_declaration_no_options: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 791: /* type_declaration_no_options: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 792: /* type_declaration_no_options: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 793: /* type_declaration_no_options: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 794: /* type_declaration_no_options: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 795: /* type_declaration_no_options: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 796: /* type_declaration_no_options: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 797: /* type_declaration_no_options: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 798: /* type_declaration_no_options: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 799: /* type_declaration_no_options: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 800: /* type_declaration_no_options: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 801: /* type_declaration_no_options: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 802: /* $@52: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 803: /* $@53: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 804: /* type_declaration_no_options: "smart_ptr" '<' $@52 type_declaration '>' $@53  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 805: /* type_declaration_no_options: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 806: /* $@54: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 807: /* $@55: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 808: /* type_declaration_no_options: "array" '<' $@54 type_declaration '>' $@55  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 809: /* $@56: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 810: /* $@57: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 811: /* type_declaration_no_options: "table" '<' $@56 table_type_pair '>' $@57  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 812: /* $@58: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 813: /* $@59: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 814: /* type_declaration_no_options: "iterator" '<' $@58 type_declaration '>' $@59  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 815: /* type_declaration_no_options: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 816: /* $@60: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 817: /* $@61: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 818: /* type_declaration_no_options: "block" '<' $@60 type_declaration '>' $@61  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 819: /* $@62: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 820: /* $@63: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 821: /* type_declaration_no_options: "block" '<' $@62 optional_function_argument_list optional_function_type '>' $@63  */
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

  case 822: /* type_declaration_no_options: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 823: /* $@64: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 824: /* $@65: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 825: /* type_declaration_no_options: "function" '<' $@64 type_declaration '>' $@65  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 826: /* $@66: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 827: /* $@67: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 828: /* type_declaration_no_options: "function" '<' $@66 optional_function_argument_list optional_function_type '>' $@67  */
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

  case 829: /* type_declaration_no_options: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 830: /* $@68: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 831: /* $@69: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 832: /* type_declaration_no_options: "lambda" '<' $@68 type_declaration '>' $@69  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 833: /* $@70: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 834: /* $@71: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 835: /* type_declaration_no_options: "lambda" '<' $@70 optional_function_argument_list optional_function_type '>' $@71  */
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

  case 836: /* $@72: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 837: /* $@73: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 838: /* type_declaration_no_options: "tuple" '<' $@72 tuple_type_list '>' $@73  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 839: /* $@74: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 840: /* $@75: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 841: /* type_declaration_no_options: "variant" '<' $@74 variant_type_list '>' $@75  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 842: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 843: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 844: /* type_declaration: type_declaration '|' '#'  */
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

  case 845: /* $@76: %empty  */
                                                          { yyextra->das_need_oxford_comma=false; }
    break;

  case 846: /* $@77: %empty  */
                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 847: /* $@78: %empty  */
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

  case 848: /* $@79: %empty  */
                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
    }
    break;

  case 849: /* tuple_alias_declaration: "tuple" optional_public_or_private_alias $@76 "name" $@77 open_block $@78 tuple_alias_type_list $@79 close_block  */
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

  case 850: /* $@80: %empty  */
                                                            { yyextra->das_need_oxford_comma=false; }
    break;

  case 851: /* $@81: %empty  */
                                                                                                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 852: /* $@82: %empty  */
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

  case 853: /* $@83: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
    }
    break;

  case 854: /* variant_alias_declaration: "variant" optional_public_or_private_alias $@80 "name" $@81 open_block $@82 variant_alias_type_list $@83 close_block  */
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

  case 855: /* $@84: %empty  */
                                                             { yyextra->das_need_oxford_comma=false; }
    break;

  case 856: /* $@85: %empty  */
                                                                                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 857: /* bitfield_alias_declaration: "bitfield" optional_public_or_private_alias $@84 "name" $@85 bitfield_basic_type_declaration open_block bitfield_alias_bits close_block  */
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

  case 858: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 859: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 860: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 861: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 862: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 863: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 864: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 865: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 866: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 867: /* make_struct_fields: "$f" '(' expr2 ')' copy_or_move expr  */
                                                                            {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-5]), (yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 868: /* make_struct_fields: "$f" '(' expr2 ')' ":=" expr  */
                                                                   {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner, (yylsp[-5]), (yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 869: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr2 ')' copy_or_move expr  */
                                                                                                        {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-5]),(yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 870: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr2 ')' ":=" expr  */
                                                                                               {
        auto mfd = new MakeFieldDecl(tokRangeAt(scanner,(yylsp[-5]), (yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 871: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 872: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 873: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 874: /* make_struct_dim: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 875: /* make_struct_dim: make_struct_dim semicolon make_struct_fields  */
                                                               {
        ((ExprMakeStruct *) (yyvsp[-2].pExpression))->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 876: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 877: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 878: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 879: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 880: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 881: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 882: /* optional_block: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 883: /* optional_block: "where" expr_block  */
                                  { (yyvsp[0].pExpression)->at = tokAt(scanner, (yylsp[0])); (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 896: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 897: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 898: /* make_struct_decl: "[[" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
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

  case 899: /* make_struct_decl: "[[" type_declaration_no_options optional_block optional_trailing_delim_sqr_sqr  */
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

  case 900: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' optional_block optional_trailing_delim_sqr_sqr  */
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

  case 901: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
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

  case 902: /* make_struct_decl: "[{" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
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

  case 903: /* make_struct_decl: "[{" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
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

  case 904: /* $@86: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 905: /* $@87: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 906: /* make_struct_decl: "struct" '<' $@86 type_declaration_no_options '>' $@87 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                            {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 907: /* $@88: %empty  */
                            { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 908: /* $@89: %empty  */
                                                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 909: /* make_struct_decl: "class" '<' $@88 type_declaration_no_options '>' $@89 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                          {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 910: /* $@90: %empty  */
                               { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 911: /* $@91: %empty  */
                                                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 912: /* make_struct_decl: "variant" '<' $@90 variant_type_list '>' $@91 '(' use_initializer make_variant_dim ')'  */
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

  case 913: /* $@92: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 914: /* $@93: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 915: /* make_struct_decl: "default" '<' $@92 type_declaration_no_options '>' $@93 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 916: /* make_tuple: expr  */
                  {
        (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 917: /* make_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 918: /* make_tuple: make_tuple ',' expr  */
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

  case 919: /* make_map_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 920: /* make_map_tuple: expr  */
                 {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 921: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = new TypeDecl(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 922: /* $@94: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 923: /* $@95: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 924: /* make_tuple_call: "tuple" '<' $@94 tuple_type_list '>' $@95 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 925: /* make_dim: make_tuple  */
                        {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 926: /* make_dim: make_dim semicolon make_tuple  */
                                                {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 927: /* make_dim_decl: '[' optional_expr_list ']'  */
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

  case 928: /* make_dim_decl: "[[" type_declaration_no_options make_dim optional_trailing_semicolon_sqr_sqr  */
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

  case 929: /* make_dim_decl: "[{" type_declaration_no_options make_dim optional_trailing_semicolon_cur_sqr  */
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

  case 930: /* $@96: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 931: /* $@97: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 932: /* make_dim_decl: "array" "struct" '<' $@96 type_declaration_no_options '>' $@97 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 933: /* $@98: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 934: /* $@99: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 935: /* make_dim_decl: "array" "tuple" '<' $@98 tuple_type_list '>' $@99 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 936: /* $@100: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 937: /* $@101: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 938: /* make_dim_decl: "array" "variant" '<' $@100 variant_type_list '>' $@101 '(' make_variant_dim ')'  */
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

  case 939: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
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

  case 940: /* $@102: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 941: /* $@103: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 942: /* make_dim_decl: "array" '<' $@102 type_declaration_no_options '>' $@103 '(' optional_expr_list ')'  */
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

  case 943: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 944: /* $@104: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 945: /* $@105: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 946: /* make_dim_decl: "fixed_array" '<' $@104 type_declaration_no_options '>' $@105 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 947: /* make_table: make_map_tuple  */
                            {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 948: /* make_table: make_table semicolon make_map_tuple  */
                                                      {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 949: /* expr_map_tuple_list: make_map_tuple  */
                                {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 950: /* expr_map_tuple_list: expr_map_tuple_list ',' make_map_tuple  */
                                                                {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 951: /* make_table_decl: open_block optional_expr_map_tuple_list close_block  */
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

  case 952: /* make_table_decl: "{{" make_table optional_trailing_semicolon_cur_cur  */
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

        auto mkt = new TypeDecl(Type::autoinfer);
        mkt->dim.push_back(TypeDecl::dimAuto);
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = mkt;
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-2]));
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_table_move");
        ttm->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = ttm;
    }
    break;

  case 953: /* make_table_decl: "table" '(' optional_expr_map_tuple_list ')'  */
                                                                       {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-1].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 954: /* make_table_decl: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 955: /* make_table_decl: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 956: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 957: /* array_comprehension_where: semicolon "where" expr  */
                                          { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 958: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 959: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 960: /* array_comprehension: '[' "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-6]))))) {
            format::get_writer() << "(" << format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-6]))),
                                                                 format::Pos::from_last(tokAt(scanner,(yylsp[-4])))) << ")";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-4]))));
        }

        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 961: /* array_comprehension: '[' "for" '(' variable_name_with_pos_list "in" expr_list ')' "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 962: /* array_comprehension: '[' "iterator" "for" '(' variable_name_with_pos_list "in" expr_list ')' "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                                         {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 963: /* array_comprehension: "begin of code block" "for" '(' variable_name_with_pos_list "in" expr_list ')' "end of expression" make_map_tuple array_comprehension_where "end of code block"  */
                                                                                                                                                                     {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
    }
    break;

  case 964: /* array_comprehension: '[' "iterator" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                                  {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-6]))))) {
            format::get_writer() << "(" << format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-6]))),
                                                                 format::Pos::from_last(tokAt(scanner,(yylsp[-4])))) << ")";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-4]))));
        }
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 965: /* array_comprehension: "[[" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where ']' ']'  */
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

  case 966: /* array_comprehension: "[{" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where "end of code block" ']'  */
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

  case 967: /* array_comprehension: open_block "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where close_block  */
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

  case 968: /* array_comprehension: "{{" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where close_block close_block  */
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


