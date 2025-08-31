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
  YYSYMBOL_annotation_argument_value = 269, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 270, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 271, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 272,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 273, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 274,   /* metadata_argument_list  */
  YYSYMBOL_annotation_declaration_name = 275, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 276, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 277,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 278,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 279, /* optional_annotation_list  */
  YYSYMBOL_optional_function_argument_list = 280, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 281,   /* optional_function_type  */
  YYSYMBOL_function_name = 282,            /* function_name  */
  YYSYMBOL_global_function_declaration = 283, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 284, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 285, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 286,     /* function_declaration  */
  YYSYMBOL_287_8 = 287,                    /* $@8  */
  YYSYMBOL_open_block = 288,               /* open_block  */
  YYSYMBOL_close_block = 289,              /* close_block  */
  YYSYMBOL_expression_block = 290,         /* expression_block  */
  YYSYMBOL_expr_call_pipe = 291,           /* expr_call_pipe  */
  YYSYMBOL_expression_any = 292,           /* expression_any  */
  YYSYMBOL_expressions = 293,              /* expressions  */
  YYSYMBOL_expr_keyword = 294,             /* expr_keyword  */
  YYSYMBOL_optional_expr_list = 295,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_list_in_braces = 296, /* optional_expr_list_in_braces  */
  YYSYMBOL_optional_expr_map_tuple_list = 297, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 298, /* type_declaration_no_options_list  */
  YYSYMBOL_expression_keyword = 299,       /* expression_keyword  */
  YYSYMBOL_300_9 = 300,                    /* $@9  */
  YYSYMBOL_301_10 = 301,                   /* $@10  */
  YYSYMBOL_302_11 = 302,                   /* $@11  */
  YYSYMBOL_303_12 = 303,                   /* $@12  */
  YYSYMBOL_expr_pipe = 304,                /* expr_pipe  */
  YYSYMBOL_name_in_namespace = 305,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 306,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 307,     /* new_type_declaration  */
  YYSYMBOL_308_13 = 308,                   /* $@13  */
  YYSYMBOL_309_14 = 309,                   /* $@14  */
  YYSYMBOL_expr_new = 310,                 /* expr_new  */
  YYSYMBOL_expression_break = 311,         /* expression_break  */
  YYSYMBOL_expression_continue = 312,      /* expression_continue  */
  YYSYMBOL_expression_return_no_pipe = 313, /* expression_return_no_pipe  */
  YYSYMBOL_expression_return = 314,        /* expression_return  */
  YYSYMBOL_expression_yield_no_pipe = 315, /* expression_yield_no_pipe  */
  YYSYMBOL_expression_yield = 316,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 317,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 318,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 319,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 320,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 321,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 322, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 323,           /* expression_let  */
  YYSYMBOL_expr_cast = 324,                /* expr_cast  */
  YYSYMBOL_325_15 = 325,                   /* $@15  */
  YYSYMBOL_326_16 = 326,                   /* $@16  */
  YYSYMBOL_327_17 = 327,                   /* $@17  */
  YYSYMBOL_328_18 = 328,                   /* $@18  */
  YYSYMBOL_329_19 = 329,                   /* $@19  */
  YYSYMBOL_330_20 = 330,                   /* $@20  */
  YYSYMBOL_expr_type_decl = 331,           /* expr_type_decl  */
  YYSYMBOL_332_21 = 332,                   /* $@21  */
  YYSYMBOL_333_22 = 333,                   /* $@22  */
  YYSYMBOL_expr_type_info = 334,           /* expr_type_info  */
  YYSYMBOL_expr_list = 335,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 336,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 337,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 338,            /* capture_entry  */
  YYSYMBOL_capture_list = 339,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 340,    /* optional_capture_list  */
  YYSYMBOL_expr_block = 341,               /* expr_block  */
  YYSYMBOL_expr_full_block = 342,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 343, /* expr_full_block_assumed_piped  */
  YYSYMBOL_344_23 = 344,                   /* $@23  */
  YYSYMBOL_expr_numeric_const = 345,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign = 346,              /* expr_assign  */
  YYSYMBOL_expr_assign_pipe_right = 347,   /* expr_assign_pipe_right  */
  YYSYMBOL_expr_assign_pipe = 348,         /* expr_assign_pipe  */
  YYSYMBOL_expr_named_call = 349,          /* expr_named_call  */
  YYSYMBOL_expr_method_call = 350,         /* expr_method_call  */
  YYSYMBOL_func_addr_name = 351,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 352,           /* func_addr_expr  */
  YYSYMBOL_353_24 = 353,                   /* $@24  */
  YYSYMBOL_354_25 = 354,                   /* $@25  */
  YYSYMBOL_355_26 = 355,                   /* $@26  */
  YYSYMBOL_356_27 = 356,                   /* $@27  */
  YYSYMBOL_expr_field = 357,               /* expr_field  */
  YYSYMBOL_358_28 = 358,                   /* $@28  */
  YYSYMBOL_359_29 = 359,                   /* $@29  */
  YYSYMBOL_expr_call = 360,                /* expr_call  */
  YYSYMBOL_expr2 = 361,                    /* expr2  */
  YYSYMBOL_362_30 = 362,                   /* $@30  */
  YYSYMBOL_363_31 = 363,                   /* $@31  */
  YYSYMBOL_364_32 = 364,                   /* $@32  */
  YYSYMBOL_365_33 = 365,                   /* $@33  */
  YYSYMBOL_366_34 = 366,                   /* $@34  */
  YYSYMBOL_367_35 = 367,                   /* $@35  */
  YYSYMBOL_expr = 368,                     /* expr  */
  YYSYMBOL_expr_mtag = 369,                /* expr_mtag  */
  YYSYMBOL_optional_field_annotation = 370, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 371,        /* optional_override  */
  YYSYMBOL_optional_constant = 372,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 373, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 374, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 375, /* structure_variable_declaration  */
  YYSYMBOL_opt_sem = 376,                  /* opt_sem  */
  YYSYMBOL_struct_variable_declaration_list = 377, /* struct_variable_declaration_list  */
  YYSYMBOL_378_36 = 378,                   /* $@36  */
  YYSYMBOL_379_37 = 379,                   /* $@37  */
  YYSYMBOL_380_38 = 380,                   /* $@38  */
  YYSYMBOL_function_argument_declaration_no_type = 381, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 382, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 383,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 384,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 385,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 386,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 387,             /* variant_type  */
  YYSYMBOL_variant_type_list = 388,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 389,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 390,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 391, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 392, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 393,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 394,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 395,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 396, /* let_variable_name_with_pos_list  */
  YYSYMBOL_let_variable_declaration = 397, /* let_variable_declaration  */
  YYSYMBOL_global_variable_declaration_list = 398, /* global_variable_declaration_list  */
  YYSYMBOL_399_39 = 399,                   /* $@39  */
  YYSYMBOL_optional_shared = 400,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 401, /* optional_public_or_private_variable  */
  YYSYMBOL_global_let = 402,               /* global_let  */
  YYSYMBOL_403_40 = 403,                   /* $@40  */
  YYSYMBOL_enum_list = 404,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 405, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 406,             /* single_alias  */
  YYSYMBOL_407_41 = 407,                   /* $@41  */
  YYSYMBOL_alias_list = 408,               /* alias_list  */
  YYSYMBOL_alias_declaration = 409,        /* alias_declaration  */
  YYSYMBOL_410_42 = 410,                   /* $@42  */
  YYSYMBOL_optional_public_or_private_enum = 411, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 412,                /* enum_name  */
  YYSYMBOL_enum_declaration = 413,         /* enum_declaration  */
  YYSYMBOL_optional_structure_parent = 414, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 415,          /* optional_sealed  */
  YYSYMBOL_structure_name = 416,           /* structure_name  */
  YYSYMBOL_class_or_struct = 417,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 418, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 419, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 420,    /* structure_declaration  */
  YYSYMBOL_421_43 = 421,                   /* $@43  */
  YYSYMBOL_422_44 = 422,                   /* $@44  */
  YYSYMBOL_variable_name_with_pos_list = 423, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 424,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 425, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 426, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 427,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 428,            /* bitfield_bits  */
  YYSYMBOL_commas = 429,                   /* commas  */
  YYSYMBOL_bitfield_alias_bits = 430,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_type_declaration = 431, /* bitfield_type_declaration  */
  YYSYMBOL_432_45 = 432,                   /* $@45  */
  YYSYMBOL_433_46 = 433,                   /* $@46  */
  YYSYMBOL_c_or_s = 434,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 435,          /* table_type_pair  */
  YYSYMBOL_dim_list = 436,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 437, /* type_declaration_no_options  */
  YYSYMBOL_438_47 = 438,                   /* $@47  */
  YYSYMBOL_439_48 = 439,                   /* $@48  */
  YYSYMBOL_440_49 = 440,                   /* $@49  */
  YYSYMBOL_441_50 = 441,                   /* $@50  */
  YYSYMBOL_442_51 = 442,                   /* $@51  */
  YYSYMBOL_443_52 = 443,                   /* $@52  */
  YYSYMBOL_444_53 = 444,                   /* $@53  */
  YYSYMBOL_445_54 = 445,                   /* $@54  */
  YYSYMBOL_446_55 = 446,                   /* $@55  */
  YYSYMBOL_447_56 = 447,                   /* $@56  */
  YYSYMBOL_448_57 = 448,                   /* $@57  */
  YYSYMBOL_449_58 = 449,                   /* $@58  */
  YYSYMBOL_450_59 = 450,                   /* $@59  */
  YYSYMBOL_451_60 = 451,                   /* $@60  */
  YYSYMBOL_452_61 = 452,                   /* $@61  */
  YYSYMBOL_453_62 = 453,                   /* $@62  */
  YYSYMBOL_454_63 = 454,                   /* $@63  */
  YYSYMBOL_455_64 = 455,                   /* $@64  */
  YYSYMBOL_456_65 = 456,                   /* $@65  */
  YYSYMBOL_457_66 = 457,                   /* $@66  */
  YYSYMBOL_458_67 = 458,                   /* $@67  */
  YYSYMBOL_459_68 = 459,                   /* $@68  */
  YYSYMBOL_460_69 = 460,                   /* $@69  */
  YYSYMBOL_461_70 = 461,                   /* $@70  */
  YYSYMBOL_462_71 = 462,                   /* $@71  */
  YYSYMBOL_463_72 = 463,                   /* $@72  */
  YYSYMBOL_464_73 = 464,                   /* $@73  */
  YYSYMBOL_type_declaration = 465,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 466,  /* tuple_alias_declaration  */
  YYSYMBOL_467_74 = 467,                   /* $@74  */
  YYSYMBOL_468_75 = 468,                   /* $@75  */
  YYSYMBOL_469_76 = 469,                   /* $@76  */
  YYSYMBOL_470_77 = 470,                   /* $@77  */
  YYSYMBOL_variant_alias_declaration = 471, /* variant_alias_declaration  */
  YYSYMBOL_472_78 = 472,                   /* $@78  */
  YYSYMBOL_473_79 = 473,                   /* $@79  */
  YYSYMBOL_474_80 = 474,                   /* $@80  */
  YYSYMBOL_475_81 = 475,                   /* $@81  */
  YYSYMBOL_bitfield_alias_declaration = 476, /* bitfield_alias_declaration  */
  YYSYMBOL_477_82 = 477,                   /* $@82  */
  YYSYMBOL_478_83 = 478,                   /* $@83  */
  YYSYMBOL_make_decl = 479,                /* make_decl  */
  YYSYMBOL_make_struct_fields = 480,       /* make_struct_fields  */
  YYSYMBOL_make_variant_dim = 481,         /* make_variant_dim  */
  YYSYMBOL_make_struct_single = 482,       /* make_struct_single  */
  YYSYMBOL_make_struct_dim = 483,          /* make_struct_dim  */
  YYSYMBOL_make_struct_dim_list = 484,     /* make_struct_dim_list  */
  YYSYMBOL_make_struct_dim_decl = 485,     /* make_struct_dim_decl  */
  YYSYMBOL_optional_make_struct_dim_decl = 486, /* optional_make_struct_dim_decl  */
  YYSYMBOL_optional_block = 487,           /* optional_block  */
  YYSYMBOL_optional_trailing_semicolon_cur_cur = 488, /* optional_trailing_semicolon_cur_cur  */
  YYSYMBOL_optional_trailing_semicolon_cur_sqr = 489, /* optional_trailing_semicolon_cur_sqr  */
  YYSYMBOL_optional_trailing_semicolon_sqr_sqr = 490, /* optional_trailing_semicolon_sqr_sqr  */
  YYSYMBOL_optional_trailing_delim_sqr_sqr = 491, /* optional_trailing_delim_sqr_sqr  */
  YYSYMBOL_optional_trailing_delim_cur_sqr = 492, /* optional_trailing_delim_cur_sqr  */
  YYSYMBOL_use_initializer = 493,          /* use_initializer  */
  YYSYMBOL_make_struct_decl = 494,         /* make_struct_decl  */
  YYSYMBOL_495_84 = 495,                   /* $@84  */
  YYSYMBOL_496_85 = 496,                   /* $@85  */
  YYSYMBOL_497_86 = 497,                   /* $@86  */
  YYSYMBOL_498_87 = 498,                   /* $@87  */
  YYSYMBOL_499_88 = 499,                   /* $@88  */
  YYSYMBOL_500_89 = 500,                   /* $@89  */
  YYSYMBOL_501_90 = 501,                   /* $@90  */
  YYSYMBOL_502_91 = 502,                   /* $@91  */
  YYSYMBOL_make_tuple = 503,               /* make_tuple  */
  YYSYMBOL_make_map_tuple = 504,           /* make_map_tuple  */
  YYSYMBOL_make_tuple_call = 505,          /* make_tuple_call  */
  YYSYMBOL_506_92 = 506,                   /* $@92  */
  YYSYMBOL_507_93 = 507,                   /* $@93  */
  YYSYMBOL_make_dim = 508,                 /* make_dim  */
  YYSYMBOL_make_dim_decl = 509,            /* make_dim_decl  */
  YYSYMBOL_510_94 = 510,                   /* $@94  */
  YYSYMBOL_511_95 = 511,                   /* $@95  */
  YYSYMBOL_512_96 = 512,                   /* $@96  */
  YYSYMBOL_513_97 = 513,                   /* $@97  */
  YYSYMBOL_514_98 = 514,                   /* $@98  */
  YYSYMBOL_515_99 = 515,                   /* $@99  */
  YYSYMBOL_516_100 = 516,                  /* $@100  */
  YYSYMBOL_517_101 = 517,                  /* $@101  */
  YYSYMBOL_518_102 = 518,                  /* $@102  */
  YYSYMBOL_519_103 = 519,                  /* $@103  */
  YYSYMBOL_make_table = 520,               /* make_table  */
  YYSYMBOL_expr_map_tuple_list = 521,      /* expr_map_tuple_list  */
  YYSYMBOL_make_table_decl = 522,          /* make_table_decl  */
  YYSYMBOL_array_comprehension_where = 523, /* array_comprehension_where  */
  YYSYMBOL_optional_comma = 524,           /* optional_comma  */
  YYSYMBOL_array_comprehension = 525       /* array_comprehension  */
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
#define YYLAST   14214

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  224
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  302
/* YYNRULES -- Number of rules.  */
#define YYNRULES  934
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1778

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
       0,   579,   579,   580,   585,   586,   587,   588,   589,   590,
     591,   592,   593,   594,   595,   596,   597,   601,   604,   607,
     613,   614,   615,   619,   620,   624,   625,   629,   648,   649,
     650,   651,   655,   656,   660,   661,   665,   666,   666,   670,
     675,   684,   699,   715,   720,   728,   728,   769,   799,   803,
     804,   805,   809,   812,   816,   822,   831,   834,   840,   841,
     845,   849,   850,   854,   857,   863,   869,   872,   878,   879,
     883,   884,   885,   895,   908,   909,   913,   914,   914,   920,
     921,   922,   923,   924,   928,   938,   948,   948,   956,   956,
     960,   960,   969,   977,   989,   999,   999,  1006,  1007,  1008,
    1009,  1010,  1011,  1015,  1020,  1028,  1029,  1030,  1034,  1035,
    1036,  1037,  1038,  1039,  1040,  1041,  1047,  1050,  1056,  1059,
    1062,  1068,  1069,  1070,  1071,  1075,  1088,  1106,  1109,  1117,
    1128,  1139,  1150,  1153,  1160,  1164,  1171,  1172,  1176,  1177,
    1178,  1182,  1185,  1192,  1196,  1197,  1198,  1199,  1200,  1201,
    1202,  1203,  1204,  1205,  1206,  1207,  1208,  1209,  1210,  1211,
    1212,  1213,  1214,  1215,  1216,  1217,  1218,  1219,  1220,  1221,
    1222,  1223,  1224,  1225,  1226,  1227,  1228,  1229,  1230,  1231,
    1232,  1233,  1234,  1235,  1236,  1237,  1238,  1239,  1240,  1241,
    1242,  1243,  1244,  1245,  1246,  1247,  1248,  1249,  1250,  1251,
    1252,  1253,  1254,  1255,  1256,  1257,  1258,  1259,  1260,  1261,
    1262,  1263,  1264,  1265,  1266,  1267,  1268,  1269,  1270,  1271,
    1272,  1273,  1274,  1275,  1276,  1277,  1278,  1279,  1280,  1281,
    1282,  1283,  1284,  1285,  1286,  1287,  1288,  1289,  1290,  1295,
    1313,  1314,  1315,  1319,  1325,  1325,  1342,  1343,  1346,  1347,
    1350,  1357,  1381,  1399,  1408,  1414,  1415,  1416,  1417,  1418,
    1419,  1420,  1421,  1422,  1423,  1424,  1425,  1426,  1427,  1428,
    1429,  1430,  1431,  1432,  1433,  1434,  1435,  1439,  1444,  1450,
    1456,  1468,  1469,  1473,  1474,  1478,  1479,  1483,  1487,  1494,
    1494,  1494,  1500,  1500,  1500,  1509,  1543,  1551,  1558,  1565,
    1571,  1572,  1583,  1587,  1590,  1598,  1598,  1598,  1601,  1607,
    1610,  1614,  1618,  1625,  1632,  1638,  1642,  1646,  1649,  1652,
    1660,  1663,  1666,  1674,  1677,  1685,  1688,  1691,  1699,  1711,
    1712,  1713,  1717,  1718,  1722,  1723,  1727,  1732,  1740,  1751,
    1757,  1772,  1784,  1787,  1793,  1793,  1793,  1796,  1796,  1796,
    1801,  1801,  1801,  1809,  1809,  1809,  1815,  1829,  1845,  1863,
    1873,  1884,  1899,  1902,  1908,  1909,  1916,  1927,  1928,  1929,
    1933,  1934,  1935,  1936,  1937,  1941,  1946,  1954,  1955,  1968,
    1972,  1982,  1989,  1996,  1996,  2005,  2006,  2007,  2008,  2009,
    2010,  2011,  2015,  2016,  2017,  2018,  2019,  2020,  2021,  2022,
    2023,  2024,  2025,  2026,  2027,  2028,  2029,  2030,  2031,  2032,
    2033,  2037,  2047,  2056,  2065,  2070,  2071,  2072,  2073,  2074,
    2075,  2076,  2077,  2078,  2079,  2080,  2081,  2082,  2083,  2084,
    2085,  2086,  2091,  2098,  2110,  2115,  2125,  2129,  2136,  2139,
    2139,  2139,  2144,  2144,  2144,  2157,  2161,  2165,  2170,  2177,
    2186,  2191,  2198,  2198,  2198,  2205,  2209,  2219,  2228,  2237,
    2241,  2244,  2250,  2251,  2252,  2253,  2254,  2255,  2256,  2257,
    2258,  2259,  2260,  2261,  2262,  2263,  2264,  2265,  2266,  2267,
    2268,  2269,  2270,  2271,  2272,  2273,  2274,  2275,  2276,  2277,
    2278,  2285,  2286,  2287,  2288,  2289,  2290,  2291,  2292,  2293,
    2294,  2295,  2296,  2303,  2304,  2305,  2306,  2307,  2308,  2309,
    2310,  2324,  2325,  2326,  2327,  2328,  2331,  2334,  2335,  2335,
    2335,  2338,  2343,  2347,  2351,  2351,  2351,  2356,  2359,  2363,
    2363,  2363,  2368,  2371,  2372,  2373,  2374,  2375,  2376,  2377,
    2378,  2379,  2381,  2385,  2393,  2398,  2402,  2411,  2412,  2413,
    2414,  2415,  2416,  2417,  2421,  2425,  2429,  2433,  2437,  2441,
    2445,  2449,  2453,  2460,  2461,  2470,  2474,  2475,  2476,  2480,
    2481,  2485,  2486,  2487,  2491,  2492,  2496,  2507,  2508,  2509,
    2510,  2515,  2518,  2518,  2537,  2536,  2552,  2551,  2565,  2574,
    2586,  2595,  2605,  2606,  2607,  2608,  2609,  2613,  2616,  2625,
    2626,  2630,  2633,  2636,  2652,  2661,  2662,  2666,  2669,  2672,
    2686,  2687,  2691,  2697,  2703,  2712,  2715,  2722,  2725,  2731,
    2732,  2733,  2737,  2738,  2742,  2749,  2754,  2763,  2769,  2780,
    2783,  2788,  2793,  2801,  2811,  2822,  2825,  2825,  2845,  2846,
    2850,  2851,  2852,  2856,  2863,  2863,  2882,  2885,  2901,  2921,
    2922,  2923,  2928,  2928,  2958,  2961,  2968,  2978,  2978,  2982,
    2983,  2984,  2988,  2998,  3018,  3041,  3042,  3046,  3047,  3051,
    3057,  3058,  3059,  3060,  3064,  3065,  3066,  3070,  3073,  3084,
    3089,  3084,  3109,  3116,  3121,  3130,  3136,  3147,  3148,  3149,
    3150,  3151,  3152,  3153,  3154,  3155,  3156,  3157,  3158,  3159,
    3160,  3161,  3162,  3163,  3164,  3165,  3166,  3167,  3168,  3169,
    3170,  3171,  3172,  3173,  3177,  3178,  3179,  3180,  3181,  3182,
    3183,  3184,  3188,  3199,  3203,  3210,  3222,  3229,  3239,  3240,
    3245,  3250,  3265,  3275,  3288,  3288,  3288,  3301,  3302,  3306,
    3310,  3317,  3321,  3328,  3329,  3330,  3331,  3332,  3347,  3353,
    3353,  3353,  3357,  3362,  3369,  3369,  3376,  3380,  3384,  3389,
    3394,  3399,  3404,  3408,  3412,  3417,  3421,  3425,  3430,  3430,
    3430,  3436,  3443,  3443,  3443,  3448,  3448,  3448,  3454,  3454,
    3454,  3459,  3464,  3464,  3464,  3469,  3469,  3469,  3478,  3483,
    3483,  3483,  3488,  3488,  3488,  3497,  3502,  3502,  3502,  3507,
    3507,  3507,  3516,  3516,  3516,  3522,  3522,  3522,  3531,  3534,
    3545,  3561,  3561,  3566,  3575,  3561,  3604,  3604,  3609,  3619,
    3604,  3648,  3648,  3648,  3691,  3692,  3693,  3694,  3695,  3699,
    3706,  3713,  3719,  3725,  3732,  3739,  3745,  3754,  3757,  3763,
    3771,  3776,  3783,  3788,  3795,  3800,  3806,  3807,  3811,  3812,
    3817,  3818,  3822,  3823,  3827,  3828,  3832,  3833,  3834,  3838,
    3839,  3840,  3844,  3845,  3849,  3882,  3921,  3940,  3960,  3980,
    4001,  4001,  4001,  4009,  4009,  4009,  4016,  4016,  4016,  4027,
    4027,  4027,  4038,  4042,  4048,  4064,  4070,  4076,  4082,  4082,
    4082,  4096,  4101,  4108,  4128,  4156,  4180,  4180,  4180,  4190,
    4190,  4190,  4204,  4204,  4204,  4218,  4227,  4227,  4227,  4247,
    4254,  4254,  4254,  4264,  4269,  4276,  4279,  4285,  4305,  4324,
    4332,  4352,  4377,  4378,  4382,  4383,  4388,  4398,  4401,  4404,
    4407,  4415,  4424,  4436,  4446
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
  "expression_with_alias", "$@7", "annotation_argument_value",
  "annotation_argument_value_list", "annotation_argument_name",
  "annotation_argument", "annotation_argument_list",
  "metadata_argument_list", "annotation_declaration_name",
  "annotation_declaration_basic", "annotation_declaration",
  "annotation_list", "optional_annotation_list",
  "optional_function_argument_list", "optional_function_type",
  "function_name", "global_function_declaration",
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
  "$@29", "expr_call", "expr2", "$@30", "$@31", "$@32", "$@33", "$@34",
  "$@35", "expr", "expr_mtag", "optional_field_annotation",
  "optional_override", "optional_constant",
  "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "opt_sem", "struct_variable_declaration_list", "$@36", "$@37", "$@38",
  "function_argument_declaration_no_type",
  "function_argument_declaration_type", "function_argument_list",
  "tuple_type", "tuple_type_list", "tuple_alias_type_list", "variant_type",
  "variant_type_list", "variant_alias_type_list", "copy_or_move",
  "variable_declaration_no_type", "variable_declaration_type",
  "variable_declaration", "copy_or_move_or_clone", "optional_ref",
  "let_variable_name_with_pos_list", "let_variable_declaration",
  "global_variable_declaration_list", "$@39", "optional_shared",
  "optional_public_or_private_variable", "global_let", "$@40", "enum_list",
  "optional_public_or_private_alias", "single_alias", "$@41", "alias_list",
  "alias_declaration", "$@42", "optional_public_or_private_enum",
  "enum_name", "enum_declaration", "optional_structure_parent",
  "optional_sealed", "structure_name", "class_or_struct",
  "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@43", "$@44", "variable_name_with_pos_list", "basic_type_declaration",
  "enum_basic_type_declaration", "structure_type_declaration",
  "auto_type_declaration", "bitfield_bits", "commas",
  "bitfield_alias_bits", "bitfield_type_declaration", "$@45", "$@46",
  "c_or_s", "table_type_pair", "dim_list", "type_declaration_no_options",
  "$@47", "$@48", "$@49", "$@50", "$@51", "$@52", "$@53", "$@54", "$@55",
  "$@56", "$@57", "$@58", "$@59", "$@60", "$@61", "$@62", "$@63", "$@64",
  "$@65", "$@66", "$@67", "$@68", "$@69", "$@70", "$@71", "$@72", "$@73",
  "type_declaration", "tuple_alias_declaration", "$@74", "$@75", "$@76",
  "$@77", "variant_alias_declaration", "$@78", "$@79", "$@80", "$@81",
  "bitfield_alias_declaration", "$@82", "$@83", "make_decl",
  "make_struct_fields", "make_variant_dim", "make_struct_single",
  "make_struct_dim", "make_struct_dim_list", "make_struct_dim_decl",
  "optional_make_struct_dim_decl", "optional_block",
  "optional_trailing_semicolon_cur_cur",
  "optional_trailing_semicolon_cur_sqr",
  "optional_trailing_semicolon_sqr_sqr", "optional_trailing_delim_sqr_sqr",
  "optional_trailing_delim_cur_sqr", "use_initializer", "make_struct_decl",
  "$@84", "$@85", "$@86", "$@87", "$@88", "$@89", "$@90", "$@91",
  "make_tuple", "make_map_tuple", "make_tuple_call", "$@92", "$@93",
  "make_dim", "make_dim_decl", "$@94", "$@95", "$@96", "$@97", "$@98",
  "$@99", "$@100", "$@101", "$@102", "$@103", "make_table",
  "expr_map_tuple_list", "make_table_decl", "array_comprehension_where",
  "optional_comma", "array_comprehension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1561)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-800)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1561,    66, -1561, -1561,   107,   -64,   313,   -79, -1561,   -40,
     117,   117,   117, -1561, -1561,    96,    22, -1561, -1561, -1561,
     663, -1561, -1561, -1561,   335, -1561,   146, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561,    -7, -1561,   103,
     132,   136, -1561, -1561, -1561, -1561,   313, -1561,    21, -1561,
   -1561, -1561,   117,   117, -1561, -1561,   146, -1561, -1561, -1561,
   -1561, -1561,   186,   226, -1561, -1561, -1561, -1561,    22,    22,
      22,   164, -1561,   946,   374, -1561, -1561, -1561, -1561,   469,
     595,   299,   834, -1561,   848,    36,   107,   229,   -64,   264,
     325, -1561,   922,   922, -1561,   349,   679,   147,   663,   926,
     360,   403,   413, -1561,   432,   433, -1561, -1561,    -3,   107,
      22,    22,    22,    22, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561,   489, -1561, -1561, -1561, -1561, -1561, -1561, -1561,   -79,
   -1561, -1561, -1561, -1561, -1561,   928,   263, -1561, -1561, -1561,
   -1561,   644, -1561, -1561, -1561, -1561,   543, -1561, -1561, -1561,
   -1561,   679, -1561, -1561, -1561,   519, -1561, -1561, -1561, -1561,
   -1561,   581, -1561,   160, -1561,   871,   590,   946, 13939, -1561,
     328,   675, -1561,   -61, -1561, -1561, -1561,   931, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561,   301, -1561,   592, -1561, -1561,
     710, -1561,   -79,   -79,   -79, -1561, -1561, 13072, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561,   788,   844, -1561,   685,   -79,   952, -1561,
   -1561,   751, -1561,   689,   107,   107,   196,   -70, -1561, -1561,
   -1561,   263, -1561, 10669, -1561, -1561, -1561, -1561,   761,   796,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
     852,   824, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561,  1014, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561,   874,   831, -1561,
   -1561,   -36,   868, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561,   -79,   476,   872,   -79, -1561,   -61,   375,
   -1561,   107, -1561,   838,  1034,   759, -1561, -1561,   879,   887,
     888,   892,   912,   921, -1561, -1561, -1561,   903, -1561, -1561,
   -1561, -1561, -1561,   924, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561,   929, -1561, -1561, -1561,   932,
     938, -1561, -1561, -1561, -1561,   940,   944,   907,    96, -1561,
   -1561, -1561, -1561, -1561,   799,   934,   527, -1561, -1561, -1561,
   -1561, -1561, -1561,   972,   558, -1561,   927, -1561,   351,   962,
      -8,   933, 10669, -1561,  3064, -1561,   365, -1561,    96, -1561,
   -1561, -1561,   -70,   930, -1561, 10151,   976,   979, 10669, -1561,
      35, -1561, -1561, -1561, 10151, -1561, -1561,   981, -1561,   630,
     687,   712, -1561, -1561, 10151,   197, -1561, -1561, -1561,    25,
   -1561, -1561, -1561,    39,  6195, -1561,   939,  2905,   142, -1561,
   10518,   460, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, 10151, -1561, -1561,   215,   135,
     135,   135, -1561,   934, -1561,   955,   957, 10151, -1561, -1561,
     941, -1561, -1561,   125,   -68,   958,    45,  3899, -1561, -1561,
     -79,   513,  6403,   942, 10151,   989,   966,   968,   948, -1561,
     663,   969,  1006,  6611,   151,   568,   973, -1561,   639,   975,
     977,  4107, 10151, 10151,   189,   189,   189,   970,   971,   974,
     980,   982,   983, -1561,  1926,  2549,  6821, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561,  7029,   986, -1561,  1174, -1561,
   10151, 10151, 10151, 10151, 10151,  5779, 10151, -1561,   978, -1561,
   -1561,   663,   663, 10151,  1184, -1561, -1561, -1561, -1561, -1561,
   -1561,  7239,  1161, -1561, -1561, -1561,   771, -1561,   118,   663,
   -1561,   663,   663,   663, -1561,   663, -1561, -1561,  1137, -1561,
   -1561, -1561, -1561,   988, -1561, -1561,   224, -1561, -1561, -1561,
   -1561, -1561, -1561, 13262,  1070, -1561,   993, -1561, -1561, -1561,
   -1561, -1561, -1561,   476, 10151, -1561, -1561,    41,   679, -1561,
     994,  1008,  1009, -1561,  2648, -1561,  1172,   876, -1561, -1561,
   -1561,  4523, 10669, 10669, 10669,  2689, 10669, 10669,  1000,  1048,
   10669,   685, 10669,   685, 10669,   685, 10772,  1051, 10763, -1561,
   10151, -1561, -1561, -1561, -1561,  1007, -1561, -1561, 12553, 10151,
   -1561,   799, -1561, -1561,  1033,   -81, -1561, -1561,   314, -1561,
     934,   689,  1031,   314, -1561,   689, 10882,  1010,  1185, -1561,
   -1561,   200, -1561, -1561, -1561, -1561, -1561,   -79,   215,  1012,
   -1561,  1015, -1561, -1561,    96,   698, -1561,  1035,  1036,  1037,
   -1561, 10151,  4523, -1561,  1040,  1106,  2150, 13992,  1225, 10669,
   10151, 10151, -1561, 10151,   -79,  1046, -1561, -1561, 10151, -1561,
   -1561,  1045,  1076, -1561, 10151, -1561, -1561, 10151, -1561, -1561,
   10151, -1561, 10669,  4523, -1561, 13005,   659,   659,  1022, -1561,
     988, -1561, -1561, -1561, 10151, 10151, 10151, 10151, 10151, 10151,
     215,  3272,   215,  3481,   215,  1107, -1561,   801, -1561,   -79,
   -1561,   923,  1042,   659,   659,   187,   659,   659,   -66,  1237,
    1038,  1059, 13262,  1059,   400, -1561, -1561,   -79, -1561,   215,
     689, -1561,  1061,   -79, -1561, -1561, -1561,  4731, -1561, -1561,
   -1561, -1561, -1561, -1561,   344,    51,   189, -1561, 10551, 13747,
   10151, 10151, -1561, -1561, 10151, 10151, 10151, 10151,  1089, 10151,
     693, 10151, 10151, 10151, 10151, 10151, 10151, 10151, 10151, 10151,
    7447, 10151, 10151, 10151, 10151, 10151, 10151, 10151, 10151, 10151,
   10151, 13877, 10151,  4939,  4939,  4939,  4939,  4939,  4939,  4939,
    4939,  4939,  4939,  4939, 10151,  4939,  4939,  4939,  4939,  4939,
    4939, -1561,  7655, -1561,   679,    22,  1255, -1561,   -61, -1561,
   10669, -1561,  1093, -1561,  4523, -1561, 13005,  -109,   -26,  1068,
     462, -1561,   231,   775, -1561, -1561,   248,   794,   868,   813,
     868,   837,   868, -1561,   430, -1561,   436, -1561, 10669,  1050,
   -1561, -1561, 12664, -1561, 10669, -1561, -1561, 10669, -1561, -1561,
   -1561, 10151,  1098, -1561,  1099, -1561, 10669, -1561, 10151, 10669,
   10669, -1561,    27,   215, 10669,  5987,  7863,  1100, 10151, 10669,
   -1561, -1561, -1561, 10669,  1059, -1561,  1040, 10151, 10151, 10151,
   10151, 10151, 10151, 10151, 10151, 10151, 10151, 10151, 10151, 10151,
   10151, 10151, 10151, 10151, 10151,   -79,   137,  1054, -1561, 10917,
   -1561, -1561, 10669, 10669, 11028, 10669, -1561, -1561, 11068, 10669,
    1059, 10669, 10772,  1059,  1051,   354, -1561, 13005, -1561,    51,
   11179, 11214, 11325, 11365, 11476, 11511,    37,   189,  1056,    79,
    3690,  5149,  8071,  1134,  1085,    11,   381,  1086,   366,    47,
    8279,    11,   812,    54, 10151,  1097, -1561, 10151, -1561, 10669,
   -1561, 10669, -1561, 10151,   896,   215,   215,    56,    40, -1561,
   10151, -1561,  1062,  1065,  1066,   749, -1561, -1561,   111, 10151,
      57, -1561, 10151, -1561, -1561,   988,   -21,  5359, -1561,   346,
    1094,  1069,  1115,  1115, -1561, -1561,  1074,   218,   685, -1561,
    1095,  1075, -1561, -1561,  1096,  1079, -1561, -1561,   597,   597,
    1129,  1129,  1636,  1636,  1080,   620,  1081, -1561, 12699,   293,
     293,   993,   597,   597,  1925, 13522, 13559, 13373, 13846, 13225,
    2188, 13670,  2343,  1129,  1129,   557,   557,   620,   620,   620,
     694, 10151,  1084,  1087,   742, 10151,  1300,  1088, 12810,   189,
     189,   189, -1561, -1561,   771, -1561,   771, -1561,   771, -1561,
     771, -1561,   771, -1561,   771, -1561,   771, -1561,   771, -1561,
     771, -1561,   771, -1561,   771, -1561, -1561,   771, -1561,   771,
   -1561,   771, -1561,   771, -1561,   771, -1561,   771, -1561,   352,
   -1561,   501,   967,  1241,   663,   670, -1561, -1561, 13933, -1561,
   -1561, -1561, -1561, 10669, -1561, -1561, -1561,  1136, -1561,  1109,
   -1561,  1111, -1561,  1113, -1561, 10772, -1561,  1051,   559,   934,
   -1561, -1561,   934,   934, 11622, -1561,  1272,   -37, -1561,  1162,
    1244, 10151,    59,   847,   805,   388,  1101,  1102,  1144,  1103,
     260,  1116,   858, 10669, 10772,  1051,  1294,  1117, 13262, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561,  1110, 10669, -1561,
   -1561, 10151,  1396,  1444, -1561,  1546, -1561,  1667,  1118,  2300,
     582,  1119,   640,    51,   685, -1561, -1561, -1561, -1561, -1561,
    1121, 10151, -1561, 10151, 10151, 10151,  5569, 12553,   126, 10151,
     817,   805,   381, -1561, -1561,  1123, -1561, 10151, -1561,  1125,
   10151, -1561, 10151,   805,   818,  1130, -1561, -1561, 10151, -1561,
   -1561, -1561,   645,   677,  1124,    60,    67, 10151,   215,    74,
   -1561, 10151, 10151, 10669,   685,   -79, -1561, -1561,   995, 10151,
   -1561,  1307, 10151, -1561,  3064,    51,   183, -1561,  1131,   504,
   10359, -1561,   817, -1561, -1561, -1561,   578,   418,   218,  1153,
    1155,  1133,  1176,  1177, -1561,   611,   868, -1561, 10151, -1561,
   10151,  8487, 10151, -1561,  1160,  1143, -1561, -1561, 10151,  1146,
   -1561, 12850, 10151,  8695,  1150, -1561, 12961, -1561,  8903, -1561,
   -1561, -1561, -1561, -1561,   663, -1561, -1561,   814, -1561,    15,
     679,    51, -1561, -1561, -1561, -1561,   934, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561,  1151, 10669, -1561,  1180, 10151, -1561, -1561,   411, 10151,
   -1561,  1154, -1561, -1561, -1561,   717, -1561,  1156,  1203, -1561,
   -1561,  2308,   727,   764, -1561, -1561, 10151,  2362,   663, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,   477,   868,
    9111,   242, 11662, -1561, -1561,    11,   381, -1561,  1158,   236,
    1085, -1561, -1561, -1561, -1561,  1086,   284,    11,  1163, -1561,
   -1561, -1561, -1561,   461, -1561, -1561, -1561,  1197, 10151, 10151,
     631,    81, 10151, 11773, 11808,  2370,   868, -1561, -1561,   -79,
   -1561,   663,   729, -1561,   685, -1561,  1166,  5359,  1207,  1169,
     658,   457, -1561, -1561,  1216, -1561, -1561,   218,  1175,   222,
   10669, 11919, 10669, 11959, -1561,   458, 12070, -1561, 10151, 13411,
   10151, -1561, 12105,  5359, -1561,   472, 10151, -1561, -1561, -1561,
     544,   679,  1361,    15, -1561, -1561,   967, -1561,  1178, -1561,
   -1561, -1561, 10151,   934, -1561, -1561,  1179,  1181, -1561,   549,
   -1561, -1561, 10151,  1226, 10151,  1204, -1561, -1561, -1561, -1561,
    1187,  1188,  1190, -1561, 10151, 10151, 10151,  1191,  1334,  1192,
    1193,  9319, -1561,   222, -1561,   562, 10151,   386,   381, -1561,
   10151, 10151, 10151, 10151,   818, -1561, 10151, 10151,  1198, -1561,
   -1561,   564,   572, 10151, 10151,   753, -1561, -1561, -1561,  1206,
     116,  4315, -1561, 10151,   868, -1561,   625, -1561,   748, 10669,
      35, -1561,  1196, -1561, -1561,  9527, -1561, -1561,  2742, -1561,
     859, -1561, -1561, -1561, 10669, 12216, 12256, -1561,   629, -1561,
   12367, -1561, -1561, -1561, -1561,  1361,   215,  1199,  1334,  1334,
     -79,  1201,  1222,  1205,  1208,  1209,  1211,  1212, 10151, -1561,
   10151, -1561, -1561, -1561, 10151, -1561, -1561,  1334,  1334, -1561,
   12402, -1561, -1561, 13114, 10151, 10151, -1561, 12513, -1561, -1561,
   13114, -1561,   663, -1561, 10151, -1561,  1247,  1248,  1249, 13114,
     577, 10151,   491, -1561,   995, -1561,  9735,  9943, -1561, -1561,
   -1561, -1561, -1561, -1561,   663,   -79,  1217, 10669,    35,  1264,
   10151, -1561, 10151, -1561, -1561, -1561,   861, -1561, -1561,  1221,
   -1561, 13939, -1561, -1561, -1561, -1561, -1561,    63,    63, -1561,
   -1561, 10151, -1561, 10151,  1334,  1334,   805,  1224,  1227,  1059,
      63,   805, -1561,  1373,  1218, -1561, -1561,   391,  1265,   689,
    1257, -1561, 10151, 10151,  1233,  1268, 13114, -1561,   491, -1561,
   10151, 10151, -1561,   689, -1561, -1561,  1264, 10151, 10151,   663,
   -1561, -1561, -1561, -1561, -1561,   663, 13939,   805,  1085,  1261,
   -1561,  1240,  1242,  1245,  1250,    63,    63,  1085,  1251, -1561,
   -1561,  1252,  1253,  1254, 10151,  1246, 10151, 10151,  1260,   689,
   -1561,  1259,   663, 13114, -1561, 10151,  1262, -1561, -1561, -1561,
   10151,   663,   663, -1561, -1561,   679,   -79,   586,  1256, -1561,
   -1561, -1561, -1561, -1561,  1266,  1270, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561,  1284,  1267, 13114,
   -1561,   663, -1561, -1561, -1561,   679, -1561,   805, -1561, -1561,
   -1561, -1561,  1274, -1561, -1561,   600, -1561, -1561
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   136,     1,   332,     0,     0,     0,   657,   333,     0,
     649,   649,   649,    17,    18,     0,     0,    16,    15,     3,
       0,    10,     9,     8,     0,     7,   638,     6,    11,     5,
       4,    13,    12,    14,   106,   107,   105,   114,   116,    47,
      63,    60,    61,    49,    50,    51,     0,    52,    58,    48,
     247,   246,   649,   649,    24,    23,   638,   651,   650,   821,
     811,   816,     0,   300,    45,   122,   123,   124,     0,     0,
       0,   125,   127,   134,     0,   121,    19,   671,   670,   240,
     659,     0,   674,   639,   640,     0,     0,     0,     0,    53,
       0,    59,     0,     0,    56,     0,   580,   649,     0,    20,
       0,     0,     0,   302,     0,     0,   133,   128,     0,     0,
       0,     0,     0,     0,   137,   242,   241,   244,   239,   661,
     660,     0,   673,   672,   676,   675,   679,   642,   641,   644,
     112,   113,   110,   111,   109,     0,     0,   108,   117,    64,
      62,    58,    55,    54,   652,   577,   578,   654,   249,   248,
     656,   580,   658,    21,    22,    25,   822,   812,   817,   301,
      43,    46,   132,     0,   129,   130,   131,   135,     0,   662,
       0,   667,   635,   563,    28,    29,    33,     0,   101,   102,
      99,   100,    98,    97,   103,     0,    57,     0,   579,   655,
       0,    27,     0,     0,     0,    44,   126,     0,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,     0,     0,   143,   138,     0,     0,   646,
     668,     0,   680,   636,     0,     0,   565,     0,    30,    31,
      32,     0,   115,     0,    26,   730,   813,   818,   208,   209,
     206,   146,   147,   149,   148,   150,   151,   152,   153,   179,
     180,   177,   178,   170,   181,   182,   171,   168,   169,   207,
     190,     0,   205,   183,   184,   185,   186,   157,   158,   159,
     154,   155,   156,   167,     0,   173,   174,   172,   165,   166,
     161,   160,   162,   163,   164,   145,   144,   189,     0,   175,
     176,   563,   141,   277,   245,   714,   717,   720,   721,   715,
     718,   716,   719,     0,     0,   665,   677,   643,   563,     0,
     118,     0,   120,     0,   624,   622,   645,   104,     0,     0,
       0,     0,     0,     0,   687,   707,   688,   723,   689,   693,
     694,   695,   696,   713,   700,   701,   702,   703,   704,   705,
     706,   708,   709,   710,   711,   781,   692,   699,   712,   788,
     795,   690,   697,   691,   698,     0,     0,     0,     0,   722,
     743,   746,   744,   745,   808,   653,     0,   601,   607,   210,
     211,   204,   188,   212,   191,   187,     0,   139,   331,   592,
     593,     0,     0,   243,     0,   646,   580,   663,     0,   669,
     581,   681,     0,     0,   119,     0,     0,     0,     0,   623,
       0,   749,   772,   775,     0,   778,   768,     0,   734,   782,
     789,   796,   802,   805,     0,     0,   758,   763,   757,     0,
     771,   767,   760,     0,     0,   762,   747,     0,   733,   823,
     814,   819,   213,   193,   194,   196,   195,   197,   198,   199,
     200,   192,   201,   202,   203,     0,   329,   330,     0,   563,
     563,   563,   140,   142,   279,     0,     0,     0,    74,    75,
      88,   508,   509,     0,     0,     0,     0,   317,   503,   315,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   316,
       0,     0,     0,     0,     0,     0,     0,   713,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   552,     0,     0,     0,   255,   385,   387,
     386,   388,   389,   390,   391,     0,     0,    39,   246,   256,
       0,     0,     0,     0,     0,   281,     0,   367,   368,   506,
     505,     0,     0,     0,     0,   272,   267,   264,   263,   265,
     266,   285,   250,   299,   278,   258,   545,   257,   462,     0,
     536,    82,    83,    80,   270,    81,   271,   273,   335,   262,
     535,   534,   533,   136,   539,   504,     0,   259,   538,   537,
     511,   463,   512,   546,   392,   464,     0,   507,   824,   828,
     825,   826,   827,     0,     0,   647,   666,   582,   580,   564,
       0,     0,     0,   545,     0,   626,   627,     0,   620,   621,
     619,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   138,     0,   138,     0,   138,     0,     0,     0,   754,
     281,   765,   766,   759,   761,     0,   764,   748,     0,     0,
     810,   809,   731,   728,   732,   300,   737,   738,     0,   602,
     597,     0,     0,     0,   608,     0,     0,     0,   682,   589,
     590,   612,   594,   595,   596,   870,   873,     0,     0,     0,
     305,   309,   308,   314,     0,     0,   353,     0,     0,     0,
     906,     0,     0,   321,   318,     0,   546,   392,     0,     0,
     285,     0,   303,     0,     0,     0,   344,   347,     0,   276,
     350,     0,     0,    67,     0,    92,   910,     0,   879,   888,
       0,   876,     0,     0,   326,   323,   491,   492,   368,   380,
     136,   298,   296,   297,     0,     0,     0,     0,     0,     0,
       0,   848,     0,     0,     0,   886,   913,     0,   289,     0,
     292,     0,     0,   468,   467,   501,   466,   465,     0,     0,
       0,   924,   362,   924,   369,   274,   275,     0,    86,     0,
       0,   915,   924,     0,   383,   253,   543,     0,   261,   268,
     269,   320,   325,   334,     0,   377,     0,   260,     0,     0,
       0,     0,   493,   494,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   452,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   252,     0,   664,   580,     0,     0,   678,   563,   637,
       0,   625,     0,   629,     0,   634,   633,     0,     0,     0,
     739,   752,     0,     0,   724,   726,     0,     0,   141,     0,
     141,     0,   141,   599,     0,   605,     0,   725,     0,     0,
     756,   741,     0,   729,     0,   603,   815,     0,   609,   820,
     591,     0,     0,   611,     0,   610,     0,   613,     0,     0,
       0,    93,     0,     0,     0,   862,     0,     0,     0,     0,
     896,   899,   902,     0,   924,   322,   319,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   304,     0,
      94,    95,     0,     0,     0,     0,    65,    66,     0,     0,
     924,     0,     0,   924,     0,     0,   327,   324,   369,   377,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   300,
       0,     0,     0,   882,   840,   848,     0,   891,     0,     0,
       0,   848,     0,     0,     0,     0,   851,     0,   918,     0,
     280,     0,    42,     0,    40,     0,     0,     0,     0,   893,
     925,   282,     0,     0,     0,   439,   436,   438,    70,     0,
       0,   917,   925,   286,   277,   136,     0,   281,   455,     0,
     924,     0,     0,     0,   343,   342,     0,     0,   138,   295,
       0,     0,   522,   521,     0,     0,   523,   527,   469,   470,
     482,   483,   480,   481,     0,   517,     0,   499,     0,   540,
     541,   542,   471,   472,   487,   488,   489,   490,     0,     0,
     485,   486,   484,   478,   479,   474,   473,   475,   476,   477,
       0,     0,     0,   445,     0,     0,     0,     0,     0,     0,
       0,     0,   414,   423,   402,   424,   403,   426,   405,   425,
     404,   427,   406,   417,   396,   418,   397,   419,   398,   428,
     407,   429,   408,   416,   394,   395,   430,   409,   431,   410,
     420,   399,   421,   400,   422,   401,   415,   393,   460,     0,
     648,     0,   571,   574,     0,     0,   628,   631,   392,   632,
     750,   773,   776,     0,   779,   769,   735,     0,   783,     0,
     790,     0,   797,     0,   803,     0,   806,     0,     0,   287,
     753,   742,   598,   604,     0,   684,   685,   615,   614,     0,
       0,     0,     0,     0,   863,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   363,   402,
     403,   405,   404,   406,   396,   397,   398,   407,   408,   394,
     409,   410,   399,   400,   401,   393,   328,     0,     0,   919,
     513,     0,     0,     0,   514,     0,   544,     0,     0,     0,
       0,     0,     0,   377,   138,   547,   548,   549,   550,   551,
       0,     0,   849,     0,     0,     0,     0,   362,   848,     0,
       0,     0,     0,   857,   858,     0,   865,     0,   855,     0,
       0,   894,     0,     0,     0,     0,   853,   895,     0,   885,
     850,   914,     0,     0,    36,     0,     0,     0,     0,     0,
     510,     0,     0,     0,   138,     0,    68,    69,    70,     0,
      85,    76,     0,   916,     0,   377,     0,   456,     0,     0,
       0,   459,   925,   839,   457,   336,     0,     0,     0,     0,
       0,     0,     0,     0,   375,     0,   141,   518,     0,   524,
       0,     0,     0,   497,     0,     0,   528,   532,     0,     0,
     500,     0,     0,     0,     0,   446,     0,   453,     0,   495,
     413,   411,   412,   461,   137,   572,   573,   574,   575,   566,
     580,   377,   630,   751,   774,   777,   740,   780,   770,   736,
     727,   784,   786,   791,   793,   798,   800,   804,   600,   807,
     606,     0,     0,   683,     0,     0,   871,   874,     0,     0,
     306,     0,   311,   312,   310,     0,   356,     0,     0,   359,
     354,     0,     0,     0,   907,   905,   285,     0,     0,   345,
     348,   351,   911,   909,   880,   889,   887,   877,     0,   141,
       0,     0,     0,   830,   829,   848,     0,   883,     0,     0,
     841,   864,   856,   884,   854,   892,     0,   848,     0,   860,
     861,   868,   852,     0,   290,   293,    37,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   141,    71,    84,     0,
      77,     0,     0,   251,   138,   458,     0,   281,     0,     0,
     622,     0,   372,   373,     0,   371,   370,     0,     0,     0,
       0,     0,     0,     0,   434,     0,     0,   529,     0,   502,
       0,   498,     0,   281,   447,     0,     0,   496,   454,   450,
       0,   580,   569,   566,   567,   568,   571,   583,     0,   787,
     794,   801,   281,   288,   686,   616,     0,     0,    89,     0,
     307,   313,     0,     0,     0,     0,   355,   897,   900,   903,
       0,     0,     0,    96,     0,     0,     0,     0,   862,     0,
       0,     0,   254,     0,   553,     0,     0,     0,     0,   866,
       0,     0,     0,     0,     0,   859,     0,     0,   283,    34,
      41,     0,     0,     0,     0,     0,   437,   562,   440,     0,
      70,     0,    87,     0,   141,   432,     0,   337,   622,     0,
       0,   379,     0,   376,   378,     0,   364,   382,     0,   561,
       0,   559,   435,   556,     0,     0,     0,   555,     0,   448,
       0,   451,   588,   570,   584,   569,     0,     0,   862,   862,
       0,     0,     0,     0,     0,     0,     0,     0,   281,   920,
     285,   346,   349,   352,     0,   863,   881,   862,   862,   515,
       0,   381,   554,   922,     0,     0,   867,     0,   832,   831,
     922,   869,   922,   291,   281,   294,    38,     0,     0,   922,
       0,     0,     0,   443,    70,    72,   317,     0,    78,    82,
      83,    80,    81,    79,   922,     0,     0,     0,     0,     0,
       0,   374,     0,   365,   519,   525,     0,   560,   558,     0,
     557,     0,   586,   618,   617,   576,   755,   847,   847,    91,
     357,     0,   360,     0,   862,   862,   837,     0,     0,   924,
     847,   837,   516,     0,     0,   834,   833,     0,     0,     0,
     924,    35,     0,     0,     0,     0,   922,   441,     0,    73,
       0,     0,   323,     0,   384,   433,     0,     0,     0,     0,
     366,   520,   526,   530,   449,     0,     0,     0,   844,   924,
     846,     0,     0,     0,     0,   847,   847,   838,     0,   908,
     921,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     925,     0,   922,   922,   926,     0,     0,   444,   324,   933,
       0,     0,     0,   341,   531,   580,     0,     0,   925,   845,
     872,   875,   358,   361,     0,     0,   904,   912,   890,   878,
     923,   931,   836,   835,   932,   934,   284,     0,     0,   922,
     930,     0,   340,   339,   585,   580,   842,     0,   898,   901,
     929,   927,     0,   338,   587,     0,   928,   843
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1561, -1561,    -1, -1561, -1561, -1561, -1561, -1561,   728,  1399,
   -1561, -1561, -1561, -1561, -1561, -1561,  1493, -1561, -1561, -1561,
     990,  1452, -1561,  1356, -1561, -1561,  1413, -1561, -1561, -1561,
   -1224, -1561, -1561, -1561,   -29, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561,  1269, -1561, -1561,   -44,   -56,
   -1561, -1561, -1561,   478,   668,  -537,  -597,  -814, -1561, -1561,
   -1561, -1560, -1561, -1561,    -4,  -231,  -223,  -369, -1561,   503,
   -1561,  -622, -1561,  -684,    58,  -363, -1561, -1561, -1561, -1561,
    -438,     0, -1561, -1561, -1561, -1561, -1561,   -23,   -22,   -19,
   -1561,   -17, -1561, -1561, -1561,  1514, -1561,   505, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561,   244,    13,   456,    82,   243,  -914,  -473, -1561,  -544,
   -1561, -1561,  -384,  1082, -1561, -1561, -1561, -1523, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561,  1451, -1561, -1561,
   -1561, -1561, -1561, -1561,   889, -1561,  -159,    65,   -43,    75,
     206, -1561,  -150, -1561, -1561, -1561, -1561, -1561, -1561,   655,
    -417,  -901, -1561,  -420,  -899, -1561,  -644,   -35,   -33, -1561,
    -600, -1371, -1561,  -372, -1561, -1561,  1473, -1561, -1561, -1561,
    1148,  1108,   268, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,  -639,
     195, -1561,  1072, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
    -222, -1561, -1561,  -397, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561,  -238, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561,  1078,  -713,  -112,  -857,  -706,
   -1561, -1561, -1123,  -900, -1561, -1561, -1561, -1185,    42, -1354,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561,   330,
    -507, -1561, -1561, -1561,   825, -1561, -1561, -1561, -1561, -1561,
   -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1561, -1276,
    -740, -1561
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,   647,    18,   155,    56,   191,    19,   177,   183,
    1606,  1407,  1519,   741,   539,   161,   540,   105,    21,    22,
      47,    48,    49,    94,    23,    41,    42,   541,   542,  1259,
    1260,   543,  1421,  1531,   544,   545,   999,   546,   668,   669,
     547,   548,   549,   550,  1191,   184,   185,    37,    38,    39,
     236,    71,    72,    73,    74,    24,   302,   393,   226,    25,
     117,   227,   118,   168,   551,   150,   719,  1072,   554,   394,
     555,   750,  1605,   760,  1138,   603,   979,  1517,   981,  1518,
     557,   558,   559,   671,   894,  1480,   560,   561,   562,   563,
     564,   565,   566,   567,   458,   568,   774,  1276,  1014,   569,
     570,   932,  1494,   933,  1495,   935,  1496,   571,   899,  1486,
     572,   751,  1547,   573,  1284,  1285,  1018,   721,   574,   831,
    1005,   575,   685,  1073,   577,   578,   579,   997,   580,  1253,
    1612,  1254,  1678,   581,  1066,  1458,   582,   583,  1440,  1691,
    1442,  1692,  1554,  1734,   735,   585,   388,  1466,  1564,  1317,
    1319,  1114,   147,   597,   838,  1641,  1696,   389,   390,   391,
     863,   864,   440,   865,   866,   441,  1215,   659,   660,  1645,
     611,   410,   325,   326,   233,   318,    84,   129,    27,   173,
     314,    95,    96,   187,    97,    28,    53,   121,   170,    29,
     399,   231,   232,    82,   126,   401,    30,   171,   316,   661,
     586,   313,   371,   372,   856,   644,   376,   373,   619,  1329,
    1135,   849,   436,   374,   612,  1323,   868,   617,  1328,   613,
    1324,   614,  1325,   616,  1327,   620,  1331,   621,  1469,   622,
    1333,   623,  1470,   624,  1335,   625,  1471,   626,  1337,   627,
    1339,   650,    31,   101,   193,   377,   651,    32,   102,   194,
     378,   655,    33,   100,   192,   587,  1698,  1708,  1011,   965,
    1699,  1700,  1701,   966,   978,  1237,  1231,  1226,  1401,  1157,
     588,   889,  1476,   890,  1477,   944,  1500,   941,  1498,   967,
     761,   589,   942,  1499,   968,   590,  1163,  1575,  1164,  1576,
    1165,  1577,   903,  1490,   939,  1497,   737,   762,   591,  1664,
     991,   592
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      17,   189,   317,    52,   304,   375,   927,   844,   869,   736,
     576,   607,   765,   992,   237,    64,    75,   888,   964,    76,
     964,   653,  1003,   648,   858,   553,   860,   971,   862,   892,
     598,   556,   722,   723,  1418,  1204,   775,  1391,  1156,   683,
     641,  1200,   138,    90,  1129,  1202,  1131,  1151,  1133,  -136,
     677,   130,   131,   163,  1010,  1016,    65,  1211,   631,  1540,
     836,  1464,   104,   714,   957,  1222,     2,  1232,    75,    75,
      75,  1234,   633,     3,  1238,    62,  1247,  1262,    91,  1349,
    1408,  1695,   323,   397,    50,    66,   657,  1409,   437,  1677,
     883,   956,  1120,   969,  1412,   973,     4,   152,     5,   234,
       6,  1524,    40,   324,    51,    63,     7,   658,   553,   987,
      75,    75,    75,    75,   556,   874,     8,   731,   733,   386,
    1000,  1465,     9,  1255,   234,   172,    34,    35,  1255,    67,
     465,   466,   553,    54,  1256,  1257,  1736,   958,   556,  1256,
    1257,   110,   111,   112,  1586,   439,    10,   678,   679,  1268,
     476,   674,  1266,   986,   463,  1727,   481,   885,    68,   402,
     437,   235,   608,   552,  1167,    62,   229,  1628,    11,    12,
     426,   437,   609,   845,    57,  1121,    13,   460,   319,   957,
      58,    55,  1010,   949,   387,   461,   235,    85,   245,   246,
     247,   320,   657,   495,   496,    63,    14,   427,   428,  1267,
    1198,  1509,   132,  1201,    57,   148,   883,   133,    83,   134,
      58,  1017,   135,   658,  1647,  1648,  1214,   162,   649,   654,
     884,   958,   104,   303,   632,   149,    92,   498,   499,   610,
     884,    13,    69,  1660,  1661,   322,  1266,    93,   634,    62,
     884,    70,   553,   369,   905,   680,   595,   884,   556,   884,
     884,    14,   884,   884,  1152,   136,   635,   688,   835,  1248,
     884,   429,   636,  1362,   681,   430,  1363,   884,    62,    63,
    1273,   705,    15,   885,   884,   946,  1258,   404,   178,   179,
      36,  1614,  1697,    16,   958,   514,   515,   516,    50,  1378,
     386,   766,   926,  1010,  1269,   234,    86,  1351,    63,  1266,
    1705,  1706,    13,  1019,   122,   123,  1615,   642,   528,   395,
     883,   148,   400,   553,    50,   945,   790,   791,  1386,   556,
    1214,    98,    14,  1596,  1668,   670,  1669,   883,    87,    88,
     646,   149,   431,  1674,    51,   643,   432,   767,  1187,   433,
      77,    78,   535,    79,   553,  1279,  1245,  1246,  1683,  1249,
     556,  1424,    50,    86,   434,  1280,   776,   235,   456,   103,
     435,    13,   833,   883,  1545,   151,   837,   657,   425,   104,
     704,    80,    51,  1511,   847,   848,   850,   885,   852,   853,
     196,    14,   857,   109,   859,    50,   861,   426,   658,    13,
    1679,  1281,   369,   884,   885,   139,   886,   629,   596,   887,
    1726,   457,  1015,   811,   812,    51,  1117,  1468,   369,    14,
     537,   718,  1282,    13,   427,   428,   630,  1283,   321,   788,
     876,  1286,   790,   791,   879,    13,   875,  1506,   437,   180,
     885,   878,  1124,    14,   181,   907,   182,   369,   370,   135,
     369,  1010,    81,  1115,   891,    14,  1757,  1758,   839,  1126,
     556,   556,   556,   556,   556,   556,   556,   556,   556,   556,
     556,  1357,   556,   556,   556,   556,   556,   556,  1265,  1513,
    1241,   930,  1439,   369,   675,   553,   303,   907,   429,    13,
      93,   556,   430,  1772,  1212,  1508,    43,    44,    45,   699,
     303,    50,  1149,  1150,   241,  1263,   323,  1514,   141,    14,
     303,   303,   303,  1345,  1012,   964,  1166,   646,  1390,   811,
     812,    51,  1385,   883,   369,   369,   980,   324,   883,    46,
     964,   242,   144,  1594,   228,  1702,   115,  1397,  1716,  1001,
     145,    13,   116,   156,   998,  1192,  1193,  1712,  1195,  1270,
     755,   756,  1197,    62,  1199,   907,   106,   107,   108,   431,
     146,    14,   993,   432,  1228,  1203,   433,   994,   768,   594,
     769,   770,   771,  1013,   772,  1503,  1271,   113,    86,  1223,
    1224,   434,  1313,    63,    50,   777,   157,   435,   778,   779,
     885,   907,  1744,  1745,  1229,   885,   158,   370,   164,   165,
     166,   167,   114,   403,    51,    13,  1310,  1311,  1312,  1225,
     995,    13,  1529,   370,   907,   159,   843,  1379,  1352,  1411,
     160,  1428,   369,   369,   369,    14,   369,   369,   778,   779,
     369,    14,   369,   646,   369,    13,   369,    13,  1123,   646,
    1139,  1134,   370,   652,    62,   370,  1142,  1136,  1430,  1143,
     148,   778,   779,   993,  1137,    14,  1516,    14,  1147,   396,
    1437,   907,   119,   646,   907,   646,  1153,  1416,   120,   437,
     149,  1162,   169,   303,    63,   907,   443,   444,   445,   446,
     447,   448,   449,   450,   896,   782,   783,  1541,  1552,  1113,
     778,   779,  1491,   788,  1110,   789,   790,   791,   792,   369,
     303,   148,  1559,   793,   113,   451,  1501,  1220,   537,   718,
     438,    91,  1186,   426,  1188,   452,   453,   454,   188,   370,
     370,   149,   369,   689,  1536,   782,   783,  1340,  1338,  1314,
    1625,   684,  1426,   788,    13,   789,   790,   791,   792,   190,
     427,   428,   690,   793,   110,   303,   977,   907,   782,   783,
    1558,  1139,   907,  1139,    14,  1512,   788,    13,   789,   790,
     791,   792,   646,   303,   996,   907,   793,   907,   195,  1004,
    1341,   808,   809,   810,  1561,   907,  1361,    14,   706,  1570,
     907,  1428,   303,   811,   812,   646,   244,   782,   783,  1220,
     753,   230,  1592,  1375,  1607,   788,   243,   707,   790,   791,
     792,  1367,  1608,  1220,   429,   793,  1429,  1675,   430,   806,
     807,   808,   809,   810,  1437,    13,  1766,   370,   370,   370,
      13,   370,   370,   811,   812,   370,  1523,   370,  1220,   370,
    1777,   370,  1220,   299,   907,    14,  -785,  1534,    13,  1438,
      14,  -785,   426,   646,  1502,    75,   811,   812,   646,   709,
     369,  1377,    13,  1626,   145,  1119,  1404,  1639,    14,  -785,
    1567,  1036,  1299,   148,  1539,  1127,  1415,   409,   710,   427,
     428,  1462,    14,  1595,   146,   431,  1037,  1300,   369,   432,
     646,  1321,   433,   149,   369,   811,   812,   369,  1405,   300,
     576,  1318,    13,  -792,   370,  1326,   369,   434,  -792,   369,
     369,   124,    13,   435,   369,   553,  1658,   125,   897,   369,
    1304,   556,    14,   369,   301,   127,  -792,   370,  -799,   426,
     646,   128,    14,  -799,  1533,  1305,  1342,   898,  1482,  1711,
     646,   303,   907,   429,   315,   904,   906,   430,  1488,    13,
    1721,  -799,   369,   369,   379,   369,   427,   428,  1611,   369,
    1630,   369,   369,  1707,  1627,  -442,   907,   409,  1707,    14,
    -442,   940,   407,   303,   943,   408,  1657,   646,   409,  1739,
     720,   720,   720,   958,  1221,  1489,    13,  1230,  -442,   380,
    1221,  1230,   437,  1023,  1027,  1388,  1125,    13,  1266,   369,
    1137,   369,  1670,   153,  1737,   975,    14,   976,  1041,   154,
    1389,   437,   537,   718,   431,  1128,  1235,    14,   432,  1236,
     429,   433,  1398,   608,   430,  1399,  1067,  1255,  1400,  1602,
     437,  1009,   764,   609,  1130,   110,   434,   112,  1256,  1257,
    1342,  1342,   435,  1717,  1315,   381,  1624,   305,  1687,  1688,
    1316,   306,  1417,  1423,   437,   370,   383,  1242,  1132,  1243,
     764,    13,   382,  1548,   437,   307,   308,   384,  1350,   385,
     309,   310,   311,   312,  1775,   437,   437,   405,   437,  1360,
    1635,    14,  1693,   370,   392,   303,   303,   303,   398,   370,
     610,   431,   370,   238,   239,   432,  1109,   406,   433,   411,
     -79,   370,   142,   143,   370,   370,  1730,   412,   413,   370,
     110,   111,   112,   434,   370,    43,    44,    45,   370,   435,
     174,   175,   982,   983,  1473,   174,   175,   176,   238,   239,
     240,   414,   415,  1320,   662,   663,   664,  1322,    59,    60,
      61,   416,   417,   369,   418,  1478,   424,   370,   370,   419,
     370,   437,   420,  1483,   370,   369,   370,   370,   421,  1155,
     422,  1137,  1629,   764,   423,   442,   455,   459,   599,   605,
     778,   779,   606,   462,   618,   665,   639,   666,   676,  1358,
     -90,   693,   695,   369,   369,  1722,   696,   698,   697,   700,
    1467,   764,   701,   708,   370,   711,   370,   712,   813,   814,
     815,   816,   817,   818,   819,   820,   740,   742,   369,   724,
     725,   821,   822,   726,   758,   426,  1530,   823,   763,   727,
     754,   728,   729,   773,  1550,    16,   753,   824,   840,   738,
     825,   826,   832,   704,   753,   842,  1546,   827,   828,   829,
     854,   855,   427,   428,   652,   870,   873,   877,   882,   881,
    1686,   893,   720,   907,   895,   900,   901,   902,   776,   925,
     931,   936,   937,  1297,   948,   780,   781,   782,   783,   974,
     988,   303,   990,   369,  1002,   788,   989,   789,   790,   791,
     792,   985,  1034,  1112,   830,   793,  1116,   794,   795,  1122,
    1140,  1145,  1146,  1160,  1189,  1213,  1219,   426,  1220,  1227,
    1546,  1240,  1250,   584,  1251,  1252,   429,  1272,  1275,  1274,
     430,   537,   718,  1278,  1288,  1287,  1289,   426,  1290,  1291,
    1292,  1307,   764,  1302,   427,   428,  1303,  1308,  1318,  1330,
    1332,  1562,  1334,  1461,  1336,  1344,  1636,  1355,   370,  1420,
    1406,  1353,  1354,  1356,   427,   428,  1432,   426,  1433,  1366,
     370,   806,   807,   808,   809,   810,  1359,  1365,  1373,  1376,
    1380,  1392,   369,  1394,   303,   811,   812,  1649,  1402,  1435,
    1436,  1425,  1434,  1474,   427,   428,   667,   431,   370,   370,
    1447,   432,  1448,  1346,   433,  1450,   687,  1493,   429,  1456,
    1472,   692,   430,   694,  1481,  1484,  1485,  1510,  1520,   434,
    1537,  1515,   703,   370,  1221,   435,  1535,  1538,   429,  1542,
     715,   608,   430,  1544,  1563,  1348,  1221,  1501,  1568,  1572,
    1569,   609,  1684,   764,  1585,  1574,  1578,  1613,  1579,  1580,
    1584,  1587,  1588,   720,   739,   303,  1631,  1604,   429,  1646,
    1532,  1650,   430,  1651,  1671,  1652,  1714,  1653,  1654,   426,
    1655,  1656,   757,  1672,  1673,   303,  1715,  1685,  1719,   431,
     369,  1694,   369,   432,  1709,  1347,   433,  1710,   370,  1718,
    1720,  1724,  1729,  1725,  1738,  1381,   427,   428,   610,   431,
    1740,   434,  1741,   432,  1751,  1742,   433,   435,  1770,   984,
    1743,  1746,  1747,  1748,  1749,  1767,  1396,   426,  1754,  1756,
    1760,   434,  1403,   834,   137,  1771,  1768,   435,  1755,   431,
    1769,  1410,  1776,   432,    20,  1364,   433,   186,    89,   303,
     846,   140,  1618,  1111,   427,   428,  1422,  1264,  1619,  1620,
     327,   434,  1621,  1765,  1622,    26,  1591,   435,  1277,  1543,
     429,  1431,  1642,  1463,   430,   720,   720,   720,  1565,    99,
     764,  1643,   764,  1644,   764,  1445,   764,   370,   764,   369,
     764,  1566,   764,   593,   764,   672,   764,  1455,   764,  1713,
     764,   673,  1460,   764,   369,   764,  1601,   764,   972,   764,
    1395,   764,     0,   764,     0,     0,   303,     0,   429,     0,
       0,   687,   430,     0,   764,     0,     0,     0,     0,   426,
     928,     0,     0,     0,     0,  1764,     0,     0,     0,     0,
       0,   431,  1663,  1479,     0,   432,     0,  1369,   433,  1663,
       0,  1663,   947,     0,     0,     0,   427,   428,  1663,     0,
       0,     0,   996,   434,     0,  1774,     0,     0,     0,   435,
     963,   303,   963,  1663,  1505,     0,     0,   369,     0,     0,
       0,     0,     0,     0,     0,   370,     0,   370,     0,   431,
       0,     0,     0,   432,     0,  1370,   433,     0,     0,     0,
       0,     0,  1521,  1522,     0,     0,  1525,   778,   779,     0,
       0,   434,     0,     0,     0,     0,     0,   435,     0,     0,
     429,     0,     0,     0,   430,  1663,     0,     0,   996,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1733,     0,
       0,     0,     0,     0,  1735,     0,     0,     0,     0,     0,
     426,     0,  1074,  1076,  1078,  1080,  1082,  1084,  1086,  1088,
    1090,  1092,  1094,  1095,  1097,  1099,  1101,  1103,  1105,  1107,
       0,  1663,  1663,     0,     0,     0,     0,   427,   428,     0,
    1762,  1763,   303,  1118,   370,     0,     0,     0,     0,     0,
       0,   431,     0,     0,     0,   432,     0,  1371,   433,   370,
       0,     0,   780,   781,   782,   783,   784,     0,  1663,   785,
    1773,     0,   788,   434,   789,   790,   791,   792,  1610,   435,
       0,     0,   793,     0,   794,   795,     0,  1148,     0,     0,
       0,     0,     0,     0,     0,  1159,     0,  1161,     0,     0,
       0,   429,     0,     0,     0,   430,     0,  1169,  1170,  1171,
    1172,  1173,  1174,  1175,  1176,  1177,  1178,  1179,  1180,  1181,
    1182,  1183,  1184,  1185,     0,     0,     0,     0,     0,     0,
       0,     0,   370,     0,     0,     0,     0,     0,  1659,     0,
       0,     0,     0,     0,   764,     0,   804,   805,   806,   807,
     808,   809,   810,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   811,   812,     0,     0,   604,     0,     0,     0,
     684,     0,   431,  1239,     0,   615,   432,     0,  1372,   433,
       0,     0,  1244,     0,     0,   628,     0,     0,     0,     0,
       0,     0,     0,     0,   434,   638,     0,     0,  1261,     0,
     435,     0,     0,     0,     0,     0,  1075,  1077,  1079,  1081,
    1083,  1085,  1087,  1089,  1091,  1093,   656,  1096,  1098,  1100,
    1102,  1104,  1106,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   906,     0,     0,     0,   686,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   730,
       0,     0,     0,     0,     0,   328,   778,   779,     0,     0,
       0,   329,     0,   716,   717,     0,     0,   330,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   331,     0,     0,
       0,     0,     0,     0,     0,   332,     0,     0,     0,     0,
       0,   743,   744,   745,   746,   747,   752,   752,     0,     0,
     333,     0,     0,     0,     0,     0,     0,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,   366,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   780,   781,   782,   783,   784,     0,     0,   785,   786,
     787,   788,     0,   789,   790,   791,   792,     0,     0,     0,
       0,   793,     0,   794,   795,     0,     0,     0,     0,    62,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1368,   752,   367,     0,     0,     0,     0,     0,     0,     0,
     872,     0,     0,     0,     0,     0,     0,     0,     0,    63,
       0,     0,     0,  1383,  1384,     0,     0,     0,  1387,     0,
       0,     0,     0,     0,     0,     0,  1393,     0,     0,   963,
       0,     0,   801,   802,   803,   804,   805,   806,   807,   808,
     809,   810,   752,   686,     0,     0,     0,     0,     0,     0,
       0,   811,   812,     0,   929,     0,     0,   368,  1419,   934,
       0,     0,     0,   584,     0,   938,     0,     0,   752,     0,
    -362,   752,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   778,   779,     0,     0,   950,   951,   952,   953,   954,
     955,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   778,
     779,     0,     0,     0,     0,     0,     0,     0,   752,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1028,  1029,     0,  1475,  1030,  1031,  1032,  1033,     0,
    1035,     0,  1038,  1039,  1040,  1042,  1043,  1044,  1045,  1046,
    1047,  1049,  1050,  1051,  1052,  1053,  1054,  1055,  1056,  1057,
    1058,  1059,     0,  1068,     0,     0,   780,   781,   782,   783,
     784,     0,     0,   785,   786,   787,   788,     0,   789,   790,
     791,   792,     0,   752,     0,     0,   793,     0,   794,   795,
       0,     0,     0,     0,   796,   797,   798,     0,     0,     0,
     799,     0,     0,     0,   780,   781,   782,   783,   784,     0,
       0,   785,   786,   787,   788,  -362,   789,   790,   791,   792,
       0,     0,     0,     0,   793,     0,   794,   795,     0,     0,
       0,     0,  1144,   426,     0,  -362,     0,     0,     0,     0,
       0,   426,     0,  -362,     0,   800,   752,   801,   802,   803,
     804,   805,   806,   807,   808,   809,   810,     0,  1168,     0,
     427,   428,     0,     0,   778,   779,   811,   812,   427,   428,
       0,  1571,     0,  1573,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1581,  1582,  1583,   802,   803,   804,   805,
     806,   807,   808,   809,   810,   426,     0,     0,     0,     0,
    1598,  1599,     0,   426,   811,   812,  1603,     0,     0,     0,
       0,   743,  1217,   752,     0,     0,     0,     0,     0,     0,
    1623,   752,   427,   428,   429,     0,     0,     0,   430,     0,
     427,   428,   429,     0,  1633,     0,   430,     0,     0,     0,
       0,  1168,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   752,   780,
     781,   782,   783,   784,     0,     0,   785,   786,   787,   788,
       0,   789,   790,   791,   792,     0,     0,     0,     0,   793,
       0,   794,   795,  1665,  1666,     0,   429,     0,     0,     0,
     430,     0,     0,     0,   429,   431,     0,     0,   430,   432,
       0,  1374,   433,   431,     0,     0,  1682,   432,     0,  1487,
     433,     0,  1301,     0,     0,     0,  1306,   434,     0,  1689,
       0,  1690,     0,   435,     0,   434,     0,     0,     0,     0,
       0,   435,     0,     0,     0,     0,     0,     0,     0,     0,
    1703,     0,  1704,   804,   805,   806,   807,   808,   809,   810,
       0,     0,     0,     0,     0,     0,     0,   431,     0,   811,
     812,   432,   732,  1492,   433,   431,     0,     0,   328,   432,
    1728,  1528,   433,     0,   329,     0,  1731,  1732,     0,   434,
     330,     0,     0,     0,     0,   435,     0,   434,     0,     0,
     331,     0,     0,   435,     0,     0,     0,     0,   332,     0,
       0,     0,   752,  1750,     0,  1752,  1753,     0,     0,     0,
       0,     0,     0,   333,     0,     0,     0,     0,     0,  1761,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   752,     0,  1382,     0,     0,   752,     0,   778,
     779,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   752,     0,     0,     0,     0,     0,   752,
       0,     0,    62,     0,     0,     0,     0,     0,   752,     0,
       0,     0,  1413,  1414,     0,   367,     0,     0,     0,     0,
     778,   779,     0,   752,     0,     0,     0,     0,     0,     0,
       0,  1168,    63,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1441,
       0,  1443,   752,  1446,     0,     0,     0,     0,     0,  1449,
       0,     0,     0,  1452,   752,     0,     0,     0,     0,   752,
       0,     0,     0,     0,   780,   781,   782,   783,   784,     0,
     368,   785,   786,   787,   788,   426,   789,   790,   791,   792,
       0,     0,     0,     0,   793,     0,   794,   795,     0,     0,
       0,     0,   796,   797,   798,     0,     0,     0,   799,     0,
     752,     0,   427,   428,     0,   780,   781,   782,   783,   784,
       0,     0,   785,   786,   787,   788,     0,   789,   790,   791,
     792,     0,     0,     0,     0,   793,     0,   794,   795,     0,
       0,   752,     0,   796,   797,   798,     0,     0,     0,   799,
       0,     0,     0,   800,     0,   801,   802,   803,   804,   805,
     806,   807,   808,   809,   810,     0,     0,     0,     0,   752,
     752,     0,     0,   752,   811,   812,   429,     0,   841,     0,
     430,     0,     0,     0,     0,     0,     0,     0,   752,     0,
       0,     0,     0,     0,   800,     0,   801,   802,   803,   804,
     805,   806,   807,   808,   809,   810,     0,     0,     0,  1555,
       0,  1556,     0,     0,   752,   811,   812,  1560,     0,   851,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   752,   328,     0,     0,     0,     0,     0,
     329,     0,     0,     0,     0,     0,   330,   431,     0,     0,
       0,   432,     0,  1634,   433,     0,   331,     0,     0,     0,
       0,     0,  1590,     0,   332,     0,     0,  1593,     0,   434,
       0,  1597,     0,     0,  1600,   435,     0,     0,     0,   333,
       0,     0,     0,     0,  1609,   752,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   752,
       0,     0,     0,     0,     0,   752,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,   752,     0,     0,     0,     0,
       0,   367,  1676,     0,     0,   464,     0,   752,     0,   465,
     466,     3,     0,   467,   468,   469,     0,   470,    63,   471,
     472,   473,   474,   475,     0,     0,     0,     0,     0,   476,
     477,   478,   479,   480,     0,   481,     0,     0,     0,     0,
       0,     0,   482,   483,     0,     0,   484,     0,   485,   486,
       0,     0,   487,     0,     8,   488,   489,     0,   490,   491,
       0,     0,   492,   493,  1723,     0,   368,     0,   640,   494,
       0,   752,   495,   496,     0,   334,   335,   336,     0,   338,
     339,   340,   341,   342,   497,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,     0,   356,   357,   358,
       0,     0,   361,   362,   363,   364,   498,   499,   500,   501,
       0,     0,     0,     0,     0,     0,  1759,     0,     0,     0,
       0,     0,   502,   503,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   504,   505,   506,
       0,     0,     0,     0,     0,     0,     0,    62,     0,     0,
       0,     0,     0,     0,     0,   507,   508,   509,   510,   511,
       0,   512,     0,   513,   514,   515,   516,    50,   148,   517,
     518,   519,   520,   521,   522,   523,   524,    63,   525,   526,
     527,     0,     0,     0,     0,     0,     0,   528,   149,   529,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   530,   531,   532,     0,
      15,     0,     0,   533,   534,     0,     0,   465,   466,     0,
       0,   535,     0,   536,     0,   537,   538,   471,   472,   473,
     474,   475,     0,     0,     0,     0,     0,   476,     0,   478,
       0,     0,     0,   481,     0,   426,     0,     0,     0,     0,
       0,   483,     0,     0,     0,     0,     0,   486,     0,     0,
     487,     0,     0,   488,     0,   957,     0,   491,     0,     0,
       0,     0,   427,   428,     0,     0,     0,   600,     0,     0,
     495,   496,     0,   334,   335,   336,     0,   338,   339,   340,
     341,   342,   497,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,     0,   356,   357,   358,     0,     0,
     361,   362,   363,   364,   498,   499,   601,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     502,   503,     0,     0,     0,     0,   429,     0,     0,     0,
     430,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    62,     0,     0,     0,     0,
       0,     0,     0,   507,   508,   509,   510,   511,     0,   512,
     958,   513,   514,   515,   516,    50,     0,     0,   518,   519,
     520,   521,   522,   523,   524,   959,   602,   526,   527,     0,
       0,     0,     0,     0,     0,   528,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   431,     0,     0,
       0,   432,     0,     0,   960,   531,   532,     0,    15,     0,
       0,   533,   534,     0,     0,     0,   465,   466,     0,   961,
       0,   962,     0,   537,   538,   435,   471,   472,   473,   474,
     475,     0,     0,     0,     0,     0,   476,     0,   478,     0,
       0,     0,   481,     0,   426,     0,     0,     0,     0,     0,
     483,     0,     0,     0,     0,     0,   486,     0,     0,   487,
       0,     0,   488,     0,     0,     0,   491,     0,     0,     0,
       0,   427,   428,     0,     0,     0,   600,     0,     0,   495,
     496,     0,   334,   335,   336,     0,   338,   339,   340,   341,
     342,   497,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,     0,   356,   357,   358,     0,     0,   361,
     362,   363,   364,   498,   499,   601,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,   429,     0,     0,     0,   430,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   507,   508,   509,   510,   511,     0,   512,   958,
     513,   514,   515,   516,    50,     0,     0,   518,   519,   520,
     521,   522,   523,   524,   959,   602,   526,   527,     0,     0,
       0,     0,     0,     0,   528,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   431,     0,     0,     0,
     432,     0,     0,   960,   531,   532,     0,    15,     0,     0,
     533,   534,     0,     0,     0,   465,   466,     0,   961,     0,
     970,     0,   537,   538,   435,   471,   472,   473,   474,   475,
       0,     0,     0,     0,     0,   476,     0,   478,     0,     0,
       0,   481,     0,   633,     0,     0,     0,     0,     0,   483,
       0,     0,     0,     0,     0,   486,     0,     0,   487,     0,
       0,   488,     0,     0,     0,   491,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   600,     0,     0,   495,   496,
       0,   334,   335,   336,     0,   338,   339,   340,   341,   342,
     497,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,     0,   356,   357,   358,     0,     0,   361,   362,
     363,   364,   498,   499,   601,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   502,   503,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    62,     0,     0,     0,     0,     0,     0,
       0,   507,   508,   509,   510,   511,     0,   512,     0,   513,
     514,   515,   516,    50,     0,     0,   518,   519,   520,   521,
     522,   523,   524,    63,   602,   526,   527,     0,     0,     0,
       0,     0,     0,   528,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   634,
       0,     0,   530,   531,   532,     0,    15,     0,     0,   533,
     534,     0,     0,     0,   465,   466,     0,  1216,     0,   536,
       0,   537,   538,   636,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,     0,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   500,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,   682,     0,     0,     0,
       0,     0,   504,   505,   506,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,   465,   466,     0,     0,   535,     0,   536,     0,
     537,   538,   471,   472,   473,   474,   475,     0,     0,     0,
       0,     0,   476,     0,   478,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,   483,     0,     0,     0,
       0,     0,   486,     0,     0,   487,     0,     0,   488,     0,
       0,     0,   491,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   600,     0,     0,   495,   496,     0,   334,   335,
     336,     0,   338,   339,   340,   341,   342,   497,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,     0,
     356,   357,   358,     0,     0,   361,   362,   363,   364,   498,
     499,   500,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,   713,     0,     0,     0,     0,     0,
     504,   505,   506,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   507,   508,
     509,   510,   511,     0,   512,     0,   513,   514,   515,   516,
      50,     0,     0,   518,   519,   520,   521,   522,   523,   524,
      63,   602,   526,   527,     0,     0,     0,     0,     0,     0,
     528,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   530,
     531,   532,     0,    15,     0,     0,   533,   534,     0,     0,
     465,   466,     0,     0,   535,     0,   536,     0,   537,   538,
     471,   472,   473,   474,   475,     0,     0,     0,     0,     0,
     476,  1616,   478,   479,     0,     0,   481,     0,     0,     0,
       0,     0,     0,     0,   483,     0,     0,     0,     0,     0,
     486,     0,     0,   487,     0,     0,   488,   489,     0,     0,
     491,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     600,     0,     0,   495,   496,     0,   334,   335,   336,     0,
     338,   339,   340,   341,   342,   497,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,     0,   356,   357,
     358,     0,     0,   361,   362,   363,   364,   498,   499,   601,
    1617,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,   503,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   507,   508,   509,   510,
     511,     0,   512,     0,   513,   514,   515,   516,    50,     0,
       0,   518,   519,   520,   521,   522,   523,   524,    63,   602,
     526,   527,     0,     0,     0,     0,     0,     0,   528,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   530,   531,   532,
       0,    15,     0,     0,   533,   534,     0,     0,   465,   466,
       0,     0,   535,     0,   536,     0,   537,   538,   471,   472,
     473,   474,   475,     0,     0,     0,     0,     0,   476,     0,
     478,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,   483,     0,     0,     0,     0,     0,   486,     0,
       0,   487,     0,     0,   488,     0,     0,     0,   491,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   600,     0,
       0,   495,   496,     0,   334,   335,   336,     0,   338,   339,
     340,   341,   342,   497,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,     0,   356,   357,   358,     0,
       0,   361,   362,   363,   364,   498,   499,   500,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,   503,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   504,   505,   506,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   507,   508,   509,   510,   511,     0,
     512,     0,   513,   514,   515,   516,    50,     0,     0,   518,
     519,   520,   521,   522,   523,   524,    63,   602,   526,   527,
       0,     0,     0,     0,     0,     0,   528,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   530,   531,   532,     0,    15,
       0,     0,   533,   534,     0,     0,   465,   466,     0,     0,
     535,     0,   536,     0,   537,   538,   471,   472,   473,   474,
     475,     0,     0,     0,     0,     0,   476,     0,   478,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
     483,     0,     0,     0,     0,     0,   486,     0,     0,   487,
       0,     0,   488,     0,     0,     0,   491,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   600,     0,     0,   495,
     496,  1006,   334,   335,   336,     0,   338,   339,   340,   341,
     342,   497,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,     0,   356,   357,   358,     0,     0,   361,
     362,   363,   364,   498,   499,   601,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   507,   508,   509,   510,   511,     0,   512,   958,
     513,   514,   515,   516,    50,     0,     0,   518,   519,   520,
     521,   522,   523,   524,   959,   602,   526,   527,     0,     0,
       0,     0,     0,     0,   528,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   530,   531,   532,     0,    15,     0,     0,
     533,   534,     0,     0,   465,   466,     0,     0,  1007,     0,
     536,  1008,   537,   538,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,     0,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   500,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1069,  1070,  1071,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,     0,     0,   465,   466,   535,     0,   536,     0,
     537,   538,   748,     0,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,   749,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   601,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,     0,     0,   465,   466,   535,   637,   536,     0,
     537,   538,   748,     0,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,   749,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   601,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,   958,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,   959,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,     0,     0,   465,   466,   535,     0,   536,     0,
     537,   538,   748,     0,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,   749,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   601,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,     0,     0,   465,   466,   535,   870,   536,     0,
     537,   538,   748,     0,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,   749,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   601,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,   465,   466,     0,     0,   535,     0,   536,     0,
     537,   538,   471,   472,   473,   474,   475,     0,     0,     0,
       0,     0,   476,     0,   478,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,   483,     0,     0,     0,
       0,     0,   486,     0,     0,   487,     0,     0,   488,     0,
       0,     0,   491,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   600,     0,     0,   495,   496,  1154,   334,   335,
     336,     0,   338,   339,   340,   341,   342,   497,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,     0,
     356,   357,   358,     0,     0,   361,   362,   363,   364,   498,
     499,   601,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   507,   508,
     509,   510,   511,     0,   512,   958,   513,   514,   515,   516,
      50,     0,     0,   518,   519,   520,   521,   522,   523,   524,
     959,   602,   526,   527,     0,     0,     0,     0,     0,     0,
     528,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   530,
     531,   532,     0,    15,     0,     0,   533,   534,     0,     0,
     465,   466,     0,     0,   535,     0,   536,     0,   537,   538,
     471,   472,   473,   474,   475,     0,     0,     0,     0,     0,
     476,     0,   478,     0,     0,     0,   481,     0,     0,     0,
       0,     0,     0,     0,   483,     0,     0,     0,     0,     0,
     486,     0,     0,   487,     0,     0,   488,     0,     0,     0,
     491,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     600,     0,     0,   495,   496,     0,   334,   335,   336,     0,
     338,   339,   340,   341,   342,   497,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,     0,   356,   357,
     358,     0,     0,   361,   362,   363,   364,   498,   499,   601,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,   503,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   507,   508,   509,   510,
     511,     0,   512,     0,   513,   514,   515,   516,    50,     0,
       0,   518,   519,   520,   521,   522,   523,   524,    63,   602,
     526,   527,     0,     0,     0,     0,     0,     0,   528,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   530,   531,   532,
       0,    15,     0,     0,   533,   534,     0,     0,   465,   466,
       0,     0,   535,   637,   536,     0,   537,   538,   471,   472,
     473,   474,   475,     0,     0,     0,     0,     0,   476,     0,
     478,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,   483,     0,     0,     0,     0,     0,   486,     0,
       0,   487,     0,     0,   488,     0,     0,     0,   491,     0,
       0,     0,     0,     0,   691,     0,     0,     0,   600,     0,
       0,   495,   496,     0,   334,   335,   336,     0,   338,   339,
     340,   341,   342,   497,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,     0,   356,   357,   358,     0,
       0,   361,   362,   363,   364,   498,   499,   601,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,   503,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   507,   508,   509,   510,   511,     0,
     512,     0,   513,   514,   515,   516,    50,     0,     0,   518,
     519,   520,   521,   522,   523,   524,    63,   602,   526,   527,
       0,     0,     0,     0,     0,     0,   528,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   530,   531,   532,     0,    15,
       0,     0,   533,   534,     0,     0,   465,   466,     0,     0,
     535,     0,   536,     0,   537,   538,   471,   472,   473,   474,
     475,     0,     0,     0,     0,     0,   476,     0,   478,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
     483,     0,     0,     0,     0,     0,   486,     0,     0,   487,
       0,     0,   488,     0,     0,     0,   491,     0,     0,   702,
       0,     0,     0,     0,     0,     0,   600,     0,     0,   495,
     496,     0,   334,   335,   336,     0,   338,   339,   340,   341,
     342,   497,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,     0,   356,   357,   358,     0,     0,   361,
     362,   363,   364,   498,   499,   601,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   507,   508,   509,   510,   511,     0,   512,     0,
     513,   514,   515,   516,    50,     0,     0,   518,   519,   520,
     521,   522,   523,   524,    63,   602,   526,   527,     0,     0,
       0,     0,     0,     0,   528,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   530,   531,   532,     0,    15,     0,     0,
     533,   534,     0,     0,     0,     0,   465,   466,   535,     0,
     536,     0,   537,   538,   734,     0,   471,   472,   473,   474,
     475,     0,     0,     0,     0,     0,   476,     0,   478,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
     483,     0,     0,     0,     0,     0,   486,     0,     0,   487,
       0,     0,   488,     0,     0,     0,   491,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   600,     0,     0,   495,
     496,     0,   334,   335,   336,     0,   338,   339,   340,   341,
     342,   497,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,     0,   356,   357,   358,     0,     0,   361,
     362,   363,   364,   498,   499,   601,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   507,   508,   509,   510,   511,     0,   512,     0,
     513,   514,   515,   516,    50,     0,     0,   518,   519,   520,
     521,   522,   523,   524,    63,   602,   526,   527,     0,     0,
       0,     0,     0,     0,   528,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   530,   531,   532,     0,    15,     0,     0,
     533,   534,     0,     0,   465,   466,     0,     0,   535,     0,
     536,     0,   537,   538,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,     0,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   601,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   738,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,     0,     0,   465,   466,   535,     0,   536,     0,
     537,   538,   759,     0,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,     0,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   601,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,   465,   466,     0,     0,   535,     0,   536,     0,
     537,   538,   471,   472,   473,   474,   475,     0,     0,  1048,
       0,     0,   476,     0,   478,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,   483,     0,     0,     0,
       0,     0,   486,     0,     0,   487,     0,     0,   488,     0,
       0,     0,   491,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   600,     0,     0,   495,   496,     0,   334,   335,
     336,     0,   338,   339,   340,   341,   342,   497,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,     0,
     356,   357,   358,     0,     0,   361,   362,   363,   364,   498,
     499,   601,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   507,   508,
     509,   510,   511,     0,   512,     0,   513,   514,   515,   516,
      50,     0,     0,   518,   519,   520,   521,   522,   523,   524,
      63,   602,   526,   527,     0,     0,     0,     0,     0,     0,
     528,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   530,
     531,   532,     0,    15,     0,     0,   533,   534,     0,     0,
     465,   466,     0,     0,   535,     0,   536,     0,   537,   538,
     471,   472,   473,   474,   475,     0,     0,     0,     0,     0,
     476,     0,   478,     0,     0,     0,   481,     0,     0,     0,
       0,     0,     0,     0,   483,     0,     0,     0,     0,     0,
     486,     0,     0,   487,     0,     0,   488,     0,     0,     0,
     491,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     600,     0,     0,   495,   496,     0,   334,   335,   336,     0,
     338,   339,   340,   341,   342,   497,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,     0,   356,   357,
     358,     0,     0,   361,   362,   363,   364,   498,   499,   601,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,   503,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   507,   508,   509,   510,
     511,     0,   512,     0,   513,   514,   515,   516,    50,     0,
       0,   518,   519,   520,   521,   522,   523,   524,    63,   602,
     526,   527,     0,     0,     0,     0,     0,     0,   528,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   530,   531,   532,
       0,    15,     0,     0,   533,   534,     0,     0,   465,   466,
       0,     0,   535,     0,   536,  1108,   537,   538,   471,   472,
     473,   474,   475,     0,     0,     0,     0,     0,   476,     0,
     478,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,   483,     0,     0,     0,     0,     0,   486,     0,
       0,   487,     0,     0,   488,     0,     0,     0,   491,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   600,     0,
       0,   495,   496,     0,   334,   335,   336,     0,   338,   339,
     340,   341,   342,   497,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,     0,   356,   357,   358,     0,
       0,   361,   362,   363,   364,   498,   499,   601,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,   503,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   507,   508,   509,   510,   511,     0,
     512,     0,   513,   514,   515,   516,    50,     0,     0,   518,
     519,   520,   521,   522,   523,   524,    63,   602,   526,   527,
       0,     0,     0,     0,     0,     0,   528,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1158,     0,   530,   531,   532,     0,    15,
       0,     0,   533,   534,     0,     0,   465,   466,     0,     0,
     535,     0,   536,     0,   537,   538,   471,   472,   473,   474,
     475,     0,     0,     0,     0,     0,   476,     0,   478,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
     483,     0,     0,     0,     0,     0,   486,     0,     0,   487,
       0,     0,   488,     0,     0,     0,   491,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   600,     0,     0,   495,
     496,     0,   334,   335,   336,     0,   338,   339,   340,   341,
     342,   497,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,     0,   356,   357,   358,     0,     0,   361,
     362,   363,   364,   498,   499,   601,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   507,   508,   509,   510,   511,     0,   512,     0,
     513,   514,   515,   516,    50,     0,     0,   518,   519,   520,
     521,   522,   523,   524,    63,   602,   526,   527,     0,     0,
       0,     0,     0,     0,   528,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   530,   531,   532,     0,    15,     0,     0,
     533,   534,     0,     0,   465,   466,     0,     0,   535,     0,
     536,  1218,   537,   538,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,     0,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   601,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,   465,   466,     0,     0,   535,     0,   536,  1233,
     537,   538,   471,   472,   473,   474,   475,     0,     0,     0,
       0,     0,   476,     0,   478,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,   483,     0,     0,     0,
       0,     0,   486,     0,     0,   487,     0,     0,   488,     0,
       0,     0,   491,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   600,     0,     0,   495,   496,     0,   334,   335,
     336,     0,   338,   339,   340,   341,   342,   497,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,     0,
     356,   357,   358,     0,     0,   361,   362,   363,   364,   498,
     499,   601,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   507,   508,
     509,   510,   511,     0,   512,     0,   513,   514,   515,   516,
      50,     0,     0,   518,   519,   520,   521,   522,   523,   524,
      63,   602,   526,   527,     0,     0,     0,     0,     0,     0,
     528,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   530,
     531,   532,     0,    15,     0,     0,   533,   534,     0,     0,
     465,   466,     0,     0,   535,     0,   536,  1444,   537,   538,
     471,   472,   473,   474,   475,     0,     0,     0,     0,     0,
     476,     0,   478,     0,     0,     0,   481,     0,     0,     0,
       0,     0,     0,     0,   483,     0,     0,     0,     0,     0,
     486,     0,     0,   487,     0,     0,   488,     0,     0,     0,
     491,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     600,     0,     0,   495,   496,     0,   334,   335,   336,     0,
     338,   339,   340,   341,   342,   497,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,     0,   356,   357,
     358,     0,     0,   361,   362,   363,   364,   498,   499,   601,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,   503,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   507,   508,   509,   510,
     511,     0,   512,     0,   513,   514,   515,   516,    50,     0,
       0,   518,   519,   520,   521,   522,   523,   524,    63,   602,
     526,   527,     0,     0,     0,     0,     0,     0,   528,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   530,   531,   532,
       0,    15,     0,     0,   533,   534,     0,     0,   465,   466,
       0,     0,  1453,     0,   536,  1454,   537,   538,   471,   472,
     473,   474,   475,     0,     0,     0,     0,     0,   476,     0,
     478,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,   483,     0,     0,     0,     0,     0,   486,     0,
       0,   487,     0,     0,   488,     0,     0,     0,   491,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   600,     0,
       0,   495,   496,     0,   334,   335,   336,     0,   338,   339,
     340,   341,   342,   497,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,     0,   356,   357,   358,     0,
       0,   361,   362,   363,   364,   498,   499,   601,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,   503,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   507,   508,   509,   510,   511,     0,
     512,     0,   513,   514,   515,   516,    50,     0,     0,   518,
     519,   520,   521,   522,   523,   524,    63,   602,   526,   527,
       0,     0,     0,     0,     0,     0,   528,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   530,   531,   532,     0,    15,
       0,     0,   533,   534,     0,     0,   465,   466,     0,     0,
     535,     0,   536,  1459,   537,   538,   471,   472,   473,   474,
     475,     0,     0,     0,     0,     0,   476,     0,   478,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
     483,     0,     0,     0,     0,     0,   486,     0,     0,   487,
       0,     0,   488,     0,     0,     0,   491,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   600,     0,     0,   495,
     496,     0,   334,   335,   336,     0,   338,   339,   340,   341,
     342,   497,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,     0,   356,   357,   358,     0,     0,   361,
     362,   363,   364,   498,   499,   601,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   507,   508,   509,   510,   511,     0,   512,     0,
     513,   514,   515,   516,    50,     0,     0,   518,   519,   520,
     521,   522,   523,   524,    63,   602,   526,   527,     0,     0,
       0,     0,     0,     0,   528,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   530,   531,   532,     0,    15,     0,     0,
     533,   534,     0,     0,   465,   466,     0,     0,   535,     0,
     536,  1504,   537,   538,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,     0,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   601,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,     0,     0,     0,
       0,     0,   528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   530,   531,   532,     0,    15,     0,     0,   533,   534,
       0,     0,   465,   466,     0,     0,   535,     0,   536,  1589,
     537,   538,   471,   472,   473,   474,   475,     0,     0,     0,
       0,     0,   476,     0,   478,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,   483,     0,     0,     0,
       0,     0,   486,     0,     0,   487,     0,     0,   488,     0,
       0,     0,   491,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   600,     0,     0,   495,   496,     0,   334,   335,
     336,     0,   338,   339,   340,   341,   342,   497,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,     0,
     356,   357,   358,     0,     0,   361,   362,   363,   364,   498,
     499,   601,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   502,   503,     0,     0,     0,
       0,     0,     0,     0,  1632,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   507,   508,
     509,   510,   511,     0,   512,     0,   513,   514,   515,   516,
      50,     0,     0,   518,   519,   520,   521,   522,   523,   524,
      63,   602,   526,   527,     0,     0,     0,     0,     0,     0,
     528,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   530,
     531,   532,     0,    15,     0,     0,   533,   534,     0,     0,
     465,   466,     0,     0,   535,     0,   536,     0,   537,   538,
     471,   472,   473,   474,   475,     0,     0,     0,     0,     0,
     476,     0,   478,     0,     0,     0,   481,     0,     0,     0,
       0,     0,     0,     0,   483,     0,     0,     0,     0,     0,
     486,     0,     0,   487,     0,     0,   488,     0,     0,     0,
     491,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     600,     0,     0,   495,   496,     0,   334,   335,   336,     0,
     338,   339,   340,   341,   342,   497,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,     0,   356,   357,
     358,     0,     0,   361,   362,   363,   364,   498,   499,   601,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   502,   503,     0,     0,     0,     0,     0,
       0,     0,  1680,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   507,   508,   509,   510,
     511,     0,   512,     0,   513,   514,   515,   516,    50,     0,
       0,   518,   519,   520,   521,   522,   523,   524,    63,   602,
     526,   527,     0,     0,     0,     0,     0,     0,   528,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   530,   531,   532,
       0,    15,     0,     0,   533,   534,     0,     0,   465,   466,
       0,     0,   535,     0,   536,     0,   537,   538,   471,   472,
     473,   474,   475,     0,     0,     0,     0,     0,   476,     0,
     478,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,   483,     0,     0,     0,     0,     0,   486,     0,
       0,   487,     0,     0,   488,     0,     0,     0,   491,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   600,     0,
       0,   495,   496,     0,   334,   335,   336,     0,   338,   339,
     340,   341,   342,   497,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,     0,   356,   357,   358,     0,
       0,   361,   362,   363,   364,   498,   499,   601,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   502,   503,     0,     0,     0,     0,     0,     0,     0,
    1681,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   507,   508,   509,   510,   511,     0,
     512,     0,   513,   514,   515,   516,    50,     0,     0,   518,
     519,   520,   521,   522,   523,   524,    63,   602,   526,   527,
       0,     0,     0,     0,     0,     0,   528,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   530,   531,   532,     0,    15,
       0,     0,   533,   534,     0,     0,   465,   466,     0,     0,
     535,     0,   536,     0,   537,   538,   471,   472,   473,   474,
     475,     0,     0,     0,     0,     0,   476,     0,   478,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
     483,     0,     0,     0,     0,     0,   486,     0,     0,   487,
       0,     0,   488,     0,     0,     0,   491,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   600,     0,     0,   495,
     496,     0,   334,   335,   336,     0,   338,   339,   340,   341,
     342,   497,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,     0,   356,   357,   358,     0,     0,   361,
     362,   363,   364,   498,   499,   601,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   502,
     503,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   507,   508,   509,   510,   511,     0,   512,     0,
     513,   514,   515,   516,    50,     0,     0,   518,   519,   520,
     521,   522,   523,   524,    63,   602,   526,   527,     0,     0,
       0,     0,     0,     0,   528,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   530,   531,   532,     0,    15,     0,     0,
     533,   534,     0,     0,   465,   466,     0,     0,   535,     0,
     536,     0,   537,   538,   471,   472,   473,   474,   475,     0,
       0,     0,     0,     0,   476,     0,   478,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,   483,     0,
       0,     0,     0,     0,   486,     0,     0,   487,     0,     0,
     488,     0,     0,     0,   491,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   600,     0,     0,   495,   496,     0,
     334,   335,   336,     0,   338,   339,   340,   341,   342,   497,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,     0,   356,   357,   358,     0,     0,   361,   362,   363,
     364,   498,   499,   601,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   502,   503,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     507,   508,   509,   510,   511,     0,   512,     0,   513,   514,
     515,   516,    50,     0,     0,   518,   519,   520,   521,   522,
     523,   524,    63,   602,   526,   527,     0,   328,     0,     0,
       0,     0,   528,   329,     0,     0,     0,     0,     0,   330,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   331,
       0,   530,   531,   532,     0,    15,     0,   332,   533,   534,
    1020,     0,     0,     0,     0,     0,  1427,     0,   536,     0,
     537,   538,   333,     0,     0,     0,     0,     0,     0,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   334,   335,   336,     0,   338,   339,   340,   341,
     342,   497,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,     0,   356,   357,   358,     0,     0,   361,
     362,   363,   364,     0,     0,     0,     0,     0,     0,     0,
       0,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   367,     0,     0,     0,     0,     0,
       0,     0,     0,    13,     0,     0,     0,     0,   328,     0,
       0,   645,     0,     0,   329,     0,     0,     0,     0,     0,
     330,     0,     0,    14,     0,     0,     0,     0,     0,  1021,
     331,   646,     0,     0,     0,     0,     0,     0,   332,     0,
       0,     0,     0,     0,  1022,     0,     0,     0,     0,     0,
       0,     0,     0,   333,     0,     0,     0,     0,     0,   368,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   778,   779,     0,     0,     0,     0,
       0,   328,     0,     0,     0,     0,     0,   329,     0,     0,
       0,     0,     0,   330,     0,     0,     0,     0,     0,     0,
       0,     0,    62,   331,     0,     0,     0,     0,     0,     0,
       0,   332,     0,     0,     0,   367,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   333,     0,     0,     0,
       0,     0,    63,   334,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,     0,     0,     0,   780,
     781,   782,   783,   784,     0,     0,   785,   786,   787,   788,
     368,   789,   790,   791,   792,     0,     0,     0,     0,   793,
       0,   794,   795,   778,   779,     0,     0,   796,   797,   798,
       0,     0,     0,   799,     0,    62,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   367,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   778,   779,
       0,     0,     0,     0,     0,   645,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   800,     0,
     801,   802,   803,   804,   805,   806,   807,   808,   809,   810,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   811,
     812,     0,     0,   867,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   368,     0,     0,     0,     0,   780,   781,
     782,   783,   784,     0,     0,   785,   786,   787,   788,     0,
     789,   790,   791,   792,     0,     0,     0,     0,   793,     0,
     794,   795,     0,     0,     0,     0,   796,   797,   798,     0,
       0,     0,   799,   780,   781,   782,   783,   784,     0,     0,
     785,   786,   787,   788,     0,   789,   790,   791,   792,   778,
     779,     0,     0,   793,     0,   794,   795,     0,     0,     0,
       0,   796,   797,   798,     0,     0,     0,   799,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   800,     0,   801,
     802,   803,   804,   805,   806,   807,   808,   809,   810,   778,
     779,     0,     0,     0,     0,     0,     0,     0,   811,   812,
       0,     0,   880,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   800,     0,   801,   802,   803,   804,   805,   806,
     807,   808,   809,   810,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   811,   812,     0,     0,  1190,     0,     0,
       0,     0,     0,     0,   780,   781,   782,   783,   784,     0,
       0,   785,   786,   787,   788,     0,   789,   790,   791,   792,
       0,     0,     0,     0,   793,     0,   794,   795,     0,     0,
       0,     0,   796,   797,   798,     0,     0,     0,   799,     0,
       0,     0,     0,     0,   780,   781,   782,   783,   784,     0,
       0,   785,   786,   787,   788,     0,   789,   790,   791,   792,
     778,   779,     0,     0,   793,     0,   794,   795,     0,     0,
       0,     0,   796,   797,   798,     0,     0,     0,   799,     0,
       0,     0,     0,   800,     0,   801,   802,   803,   804,   805,
     806,   807,   808,   809,   810,   778,   779,     0,     0,     0,
       0,     0,     0,     0,   811,   812,     0,     0,  1194,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   800,     0,   801,   802,   803,   804,   805,
     806,   807,   808,   809,   810,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   811,   812,     0,     0,  1196,     0,
       0,     0,     0,     0,     0,   780,   781,   782,   783,   784,
       0,     0,   785,   786,   787,   788,     0,   789,   790,   791,
     792,     0,     0,     0,     0,   793,     0,   794,   795,     0,
       0,     0,     0,   796,   797,   798,     0,     0,     0,   799,
     780,   781,   782,   783,   784,     0,     0,   785,   786,   787,
     788,     0,   789,   790,   791,   792,   778,   779,     0,     0,
     793,     0,   794,   795,     0,     0,     0,     0,   796,   797,
     798,     0,     0,     0,   799,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   800,     0,   801,   802,   803,   804,
     805,   806,   807,   808,   809,   810,   778,   779,     0,     0,
       0,     0,     0,     0,     0,   811,   812,     0,     0,  1205,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   800,
       0,   801,   802,   803,   804,   805,   806,   807,   808,   809,
     810,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     811,   812,     0,     0,  1206,     0,     0,     0,     0,     0,
       0,   780,   781,   782,   783,   784,     0,     0,   785,   786,
     787,   788,     0,   789,   790,   791,   792,     0,     0,     0,
       0,   793,     0,   794,   795,     0,     0,     0,     0,   796,
     797,   798,     0,     0,     0,   799,     0,     0,     0,     0,
       0,   780,   781,   782,   783,   784,     0,     0,   785,   786,
     787,   788,     0,   789,   790,   791,   792,   778,   779,     0,
       0,   793,     0,   794,   795,     0,     0,     0,     0,   796,
     797,   798,     0,     0,     0,   799,     0,     0,     0,     0,
     800,     0,   801,   802,   803,   804,   805,   806,   807,   808,
     809,   810,   778,   779,     0,     0,     0,     0,     0,     0,
       0,   811,   812,     0,     0,  1207,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     800,     0,   801,   802,   803,   804,   805,   806,   807,   808,
     809,   810,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   811,   812,     0,     0,  1208,     0,     0,     0,     0,
       0,     0,   780,   781,   782,   783,   784,     0,     0,   785,
     786,   787,   788,     0,   789,   790,   791,   792,     0,     0,
       0,     0,   793,     0,   794,   795,     0,     0,     0,     0,
     796,   797,   798,     0,     0,     0,   799,   780,   781,   782,
     783,   784,     0,     0,   785,   786,   787,   788,     0,   789,
     790,   791,   792,   778,   779,     0,     0,   793,     0,   794,
     795,     0,     0,     0,     0,   796,   797,   798,     0,     0,
       0,   799,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   800,     0,   801,   802,   803,   804,   805,   806,   807,
     808,   809,   810,   778,   779,     0,     0,     0,     0,     0,
       0,     0,   811,   812,     0,     0,  1209,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   800,     0,   801,   802,
     803,   804,   805,   806,   807,   808,   809,   810,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   811,   812,     0,
       0,  1210,     0,     0,     0,     0,     0,     0,   780,   781,
     782,   783,   784,     0,     0,   785,   786,   787,   788,     0,
     789,   790,   791,   792,     0,     0,     0,     0,   793,     0,
     794,   795,     0,     0,     0,     0,   796,   797,   798,     0,
       0,     0,   799,     0,     0,     0,     0,     0,   780,   781,
     782,   783,   784,     0,     0,   785,   786,   787,   788,     0,
     789,   790,   791,   792,   778,   779,     0,     0,   793,     0,
     794,   795,     0,     0,     0,     0,   796,   797,   798,     0,
       0,     0,   799,     0,     0,     0,     0,   800,     0,   801,
     802,   803,   804,   805,   806,   807,   808,   809,   810,   778,
     779,     0,     0,     0,     0,     0,     0,     0,   811,   812,
       0,     0,  1343,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   800,     0,   801,
     802,   803,   804,   805,   806,   807,   808,   809,   810,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   811,   812,
       0,     0,  1507,     0,     0,     0,     0,     0,     0,   780,
     781,   782,   783,   784,     0,     0,   785,   786,   787,   788,
       0,   789,   790,   791,   792,     0,     0,     0,     0,   793,
       0,   794,   795,     0,     0,     0,     0,   796,   797,   798,
       0,     0,     0,   799,   780,   781,   782,   783,   784,     0,
       0,   785,   786,   787,   788,     0,   789,   790,   791,   792,
     778,   779,     0,     0,   793,     0,   794,   795,     0,     0,
       0,     0,   796,   797,   798,     0,     0,     0,   799,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   800,     0,
     801,   802,   803,   804,   805,   806,   807,   808,   809,   810,
     778,   779,     0,     0,     0,     0,     0,     0,     0,   811,
     812,     0,     0,  1526,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   800,     0,   801,   802,   803,   804,   805,
     806,   807,   808,   809,   810,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   811,   812,     0,     0,  1527,     0,
       0,     0,     0,     0,     0,   780,   781,   782,   783,   784,
       0,     0,   785,   786,   787,   788,     0,   789,   790,   791,
     792,     0,     0,     0,     0,   793,     0,   794,   795,     0,
       0,     0,     0,   796,   797,   798,     0,     0,     0,   799,
       0,     0,     0,     0,     0,   780,   781,   782,   783,   784,
       0,     0,   785,   786,   787,   788,     0,   789,   790,   791,
     792,   778,   779,     0,     0,   793,     0,   794,   795,     0,
       0,     0,     0,   796,   797,   798,     0,     0,     0,   799,
       0,     0,     0,     0,   800,     0,   801,   802,   803,   804,
     805,   806,   807,   808,   809,   810,   778,   779,     0,     0,
       0,     0,     0,     0,     0,   811,   812,     0,     0,  1549,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   800,     0,   801,   802,   803,   804,
     805,   806,   807,   808,   809,   810,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   811,   812,     0,     0,  1551,
       0,     0,     0,     0,     0,     0,   780,   781,   782,   783,
     784,     0,     0,   785,   786,   787,   788,     0,   789,   790,
     791,   792,     0,     0,     0,     0,   793,     0,   794,   795,
       0,     0,     0,     0,   796,   797,   798,     0,     0,     0,
     799,   780,   781,   782,   783,   784,     0,     0,   785,   786,
     787,   788,     0,   789,   790,   791,   792,   778,   779,     0,
       0,   793,     0,   794,   795,     0,     0,     0,     0,   796,
     797,   798,     0,     0,     0,   799,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   800,     0,   801,   802,   803,
     804,   805,   806,   807,   808,   809,   810,   778,   779,     0,
       0,     0,     0,     0,     0,     0,   811,   812,     0,     0,
    1553,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     800,     0,   801,   802,   803,   804,   805,   806,   807,   808,
     809,   810,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   811,   812,     0,     0,  1557,     0,     0,     0,     0,
       0,     0,   780,   781,   782,   783,   784,     0,     0,   785,
     786,   787,   788,     0,   789,   790,   791,   792,     0,     0,
       0,     0,   793,     0,   794,   795,     0,     0,     0,     0,
     796,   797,   798,     0,     0,     0,   799,     0,     0,     0,
       0,     0,   780,   781,   782,   783,   784,     0,     0,   785,
     786,   787,   788,     0,   789,   790,   791,   792,   778,   779,
       0,     0,   793,     0,   794,   795,     0,     0,     0,     0,
     796,   797,   798,     0,     0,     0,   799,     0,     0,     0,
       0,   800,     0,   801,   802,   803,   804,   805,   806,   807,
     808,   809,   810,   778,   779,     0,     0,     0,     0,     0,
       0,     0,   811,   812,     0,     0,  1637,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   800,     0,   801,   802,   803,   804,   805,   806,   807,
     808,   809,   810,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   811,   812,     0,     0,  1638,     0,     0,     0,
       0,     0,     0,   780,   781,   782,   783,   784,     0,     0,
     785,   786,   787,   788,     0,   789,   790,   791,   792,     0,
       0,     0,     0,   793,     0,   794,   795,     0,     0,     0,
       0,   796,   797,   798,     0,     0,     0,   799,   780,   781,
     782,   783,   784,     0,     0,   785,   786,   787,   788,     0,
     789,   790,   791,   792,   778,   779,     0,     0,   793,     0,
     794,   795,     0,     0,     0,     0,   796,   797,   798,     0,
       0,     0,   799,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   800,     0,   801,   802,   803,   804,   805,   806,
     807,   808,   809,   810,   778,   779,     0,     0,     0,     0,
       0,     0,     0,   811,   812,     0,     0,  1640,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   800,     0,   801,
     802,   803,   804,   805,   806,   807,   808,   809,   810,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   811,   812,
       0,     0,  1662,     0,     0,     0,     0,     0,     0,   780,
     781,   782,   783,   784,     0,     0,   785,   786,   787,   788,
       0,   789,   790,   791,   792,     0,     0,     0,     0,   793,
       0,   794,   795,     0,     0,     0,     0,   796,   797,   798,
       0,     0,     0,   799,     0,     0,     0,     0,     0,   780,
     781,   782,   783,   784,     0,     0,   785,   786,   787,   788,
       0,   789,   790,   791,   792,   778,   779,     0,     0,   793,
       0,   794,   795,     0,     0,     0,     0,   796,   797,   798,
       0,     0,     0,   799,     0,     0,     0,     0,   800,     0,
     801,   802,   803,   804,   805,   806,   807,   808,   809,   810,
     778,   779,     0,     0,     0,     0,     0,     0,     0,   811,
     812,     0,     0,  1667,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   800,     0,
     801,   802,   803,   804,   805,   806,   807,   808,   809,   810,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   811,
     812,   871,     0,     0,     0,     0,     0,     0,     0,     0,
     780,   781,   782,   783,   784,     0,     0,   785,   786,   787,
     788,     0,   789,   790,   791,   792,     0,     0,     0,     0,
     793,     0,   794,   795,     0,     0,     0,     0,   796,   797,
     798,     0,     0,     0,   799,   780,   781,   782,   783,   784,
       0,     0,   785,   786,   787,   788,     0,   789,   790,   791,
     792,   778,   779,     0,     0,   793,     0,   794,   795,     0,
       0,     0,     0,   796,   797,   798,     0,     0,     0,   799,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   800,
       0,   801,   802,   803,   804,   805,   806,   807,   808,   809,
     810,   778,   779,     0,     0,     0,     0,     0,     0,     0,
     811,   812,  1141,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   800,     0,   801,   802,   803,   804,
     805,   806,   807,   808,   809,   810,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   811,   812,  1293,     0,     0,
       0,     0,     0,     0,     0,     0,   780,   781,   782,   783,
     784,     0,     0,   785,   786,   787,   788,     0,   789,   790,
     791,   792,     0,     0,     0,     0,   793,     0,   794,   795,
       0,     0,     0,     0,   796,   797,   798,     0,     0,     0,
     799,     0,     0,     0,     0,     0,   780,   781,   782,   783,
     784,     0,     0,   785,   786,   787,   788,     0,   789,   790,
     791,   792,   778,   779,     0,     0,   793,     0,   794,   795,
       0,     0,     0,     0,   796,   797,   798,     0,     0,     0,
     799,     0,     0,     0,     0,   800,     0,   801,   802,   803,
     804,   805,   806,   807,   808,   809,   810,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   811,   812,  1309,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   800,     0,   801,   802,   803,
     804,   805,   806,   807,   808,   809,   810,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   811,   812,  1451,     0,
       0,     0,     0,     0,     0,     0,     0,   780,   781,   782,
     783,   784,     0,     0,   785,   786,   787,   788,     0,   789,
     790,   791,   792,   248,   249,     0,     0,   793,     0,   794,
     795,     0,     0,     0,     0,   796,   797,   798,     0,     0,
     250,   799,     0,   908,   909,   910,   911,   912,   913,   914,
     915,     0,     0,     0,     0,     0,   916,   917,     0,     0,
       0,     0,   918,     0,     0,   778,   779,  -392,     0,     0,
       0,     0,   824,     0,     0,   919,   920,     0,     0,     0,
       0,     0,   921,   922,   923,     0,   800,     0,   801,   802,
     803,   804,   805,   806,   807,   808,   809,   810,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   811,   812,  1457,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,     0,   924,
     269,   270,   271,     0,     0,     0,     0,     0,     0,   272,
     273,   274,   275,   276,     0,     0,   277,   278,   279,   280,
     281,   282,   283,     0,     0,     0,   537,   718,     0,     0,
     780,   781,   782,   783,   784,     0,     0,   785,   786,   787,
     788,     0,   789,   790,   791,   792,   778,   779,     0,     0,
     793,     0,   794,   795,     0,     0,     0,     0,   796,   797,
     798,     0,     0,     0,   799,     0,     0,   284,     0,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,    13,
       0,   295,   296,   778,   779,     0,     0,     0,   297,   298,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    14,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   800,
       0,   801,   802,   803,   804,   805,   806,   807,   808,   809,
     810,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     811,   812,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   780,   781,   782,   783,   784,     0,     0,   785,   786,
     787,   788,     0,   789,   790,   791,   792,     0,     0,     0,
       0,   793,     0,   794,   795,     0,     0,     0,     0,   796,
     797,   798,     0,     0,     0,   799,     0,     0,   780,   781,
     782,   783,   784,     0,     0,   785,   786,   787,   788,     0,
     789,   790,   791,   792,   778,   779,     0,     0,   793,     0,
     794,   795,     0,     0,     0,     0,   796,   797,   798,     0,
       0,     0,   799,     0,     0,     0,     0,     0,     0,     0,
     800,  1298,   801,   802,   803,   804,   805,   806,   807,   808,
     809,   810,   778,   779,     0,     0,     0,     0,     0,     0,
       0,   811,   812,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   800,     0,   801,
     802,   803,   804,   805,   806,   807,   808,   809,   810,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   811,   812,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   780,
     781,   782,   783,   784,     0,     0,   785,   786,   787,   788,
       0,   789,   790,   791,   792,     0,     0,     0,     0,   793,
       0,   794,   795,     0,     0,     0,     0,   796,   797,   798,
       0,     0,     0,  -800,     0,     0,     0,   780,   781,   782,
     783,   784,     0,     0,   785,   786,   787,   788,     0,   789,
     790,   791,   792,   778,   779,     0,     0,   793,     0,   794,
     795,     0,     0,     0,     0,   796,   797,   798,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   800,     0,
     801,   802,   803,   804,   805,   806,   807,   808,   809,   810,
     778,   779,     0,     0,     0,     0,     0,     0,     0,   811,
     812,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   800,     0,   801,   802,
     803,   804,   805,   806,   807,   808,   809,   810,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   811,   812,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   780,   781,
     782,   783,   784,     0,     0,   785,   786,   787,   788,     0,
     789,   790,   791,   792,     0,     0,     0,     0,   793,     0,
     794,   795,     0,     0,     0,     0,   796,     0,   798,     0,
       0,     0,     0,     0,     0,   780,   781,   782,   783,   784,
       0,     0,   785,   786,   787,   788,     0,   789,   790,   791,
     792,   778,   779,     0,     0,   793,     0,   794,   795,     0,
       0,     0,     0,   796,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   801,
     802,   803,   804,   805,   806,   807,   808,   809,   810,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   811,   812,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   801,   802,   803,   804,
     805,   806,   807,   808,   809,   810,  1024,     0,     0,     0,
       0,     0,     0,     0,     0,   811,   812,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   780,   781,   782,   783,
     784,     0,     0,   785,   786,   787,   788,     0,   789,   790,
     791,   792,     0,     0,     0,     0,   793,     0,   794,   795,
       0,     0,     0,     0,     0,     0,     0,     0,   334,   335,
     336,     0,   338,   339,   340,   341,   342,   497,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,     0,
     356,   357,   358,     0,     0,   361,   362,   363,   364,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1294,     0,     0,     0,   803,
     804,   805,   806,   807,   808,   809,   810,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   811,   812,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1025,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   334,   335,   336,
    1026,   338,   339,   340,   341,   342,   497,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,     0,   356,
     357,   358,     0,     0,   361,   362,   363,   364,   334,   335,
     336,     0,   338,   339,   340,   341,   342,   497,   344,   345,
     346,   347,   348,   349,   350,   351,   352,   353,   354,     0,
     356,   357,   358,     0,   197,   361,   362,   363,   364,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1295,     0,  1060,  1061,     0,     0,
     198,     0,   199,     0,   200,   201,   202,   203,   204,  1296,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,     0,   216,   217,   218,  1062,     0,   219,   220,   221,
     222,   908,   909,   910,   911,   912,   913,   914,   915,     0,
    1063,     0,     0,     0,   916,   917,     0,   223,   224,     0,
     918,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     824,     0,     0,   919,   920,     0,     0,     0,     0,     0,
     921,   922,   923,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1064,  1065,     0,     0,     0,    13,     0,
     908,   909,   910,   911,   912,   913,   914,   915,     0,     0,
       0,     0,   225,   916,   917,     0,     0,     0,    14,   918,
       0,     0,     0,     0,     0,     0,     0,   924,     0,   824,
       0,     0,   919,   920,     0,     0,     0,     0,     0,   921,
     922,   923,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   537,   718,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   924,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   537,   718
};

static const yytype_int16 yycheck[] =
{
       1,   151,   233,     7,   227,   243,   690,   607,   630,   516,
     394,   408,   556,   753,   173,    15,    16,   661,   731,    20,
     733,   441,   762,   440,   621,   394,   623,   733,   625,   668,
     402,   394,   505,   506,  1258,   949,   573,  1222,   895,   477,
     437,   942,    86,    22,   858,   944,   860,    20,   862,     8,
       5,    15,    16,   109,   767,     4,    34,    20,    33,  1430,
     597,    46,   143,   501,    53,   965,     0,    20,    68,    69,
      70,   971,    33,     7,    20,   143,    20,    20,    57,    20,
      20,  1641,   152,   314,   163,    63,   152,    20,   197,  1612,
     127,   730,   201,   732,    20,   734,    30,    98,    32,   160,
      34,    20,   166,   173,   183,   173,    40,   173,   477,   748,
     110,   111,   112,   113,   477,   196,    50,   514,   515,   155,
     759,   106,    56,    12,   160,   129,    19,    20,    12,   107,
       5,     6,   501,   173,    23,    24,  1696,   158,   501,    23,
      24,   144,   145,   146,  1498,   376,    80,   102,   103,  1006,
      25,   219,   173,   219,   392,  1678,    31,   194,   136,   318,
     197,   222,   127,   394,   904,   143,   170,  1538,   102,   103,
      33,   197,   137,   611,    57,   201,   165,   185,   234,    53,
      63,   221,   895,   720,   220,   193,   222,   194,   192,   193,
     194,   235,   152,    68,    69,   173,   185,    60,    61,   220,
     940,  1386,   166,   943,    57,   164,   127,   171,    62,   173,
      63,   160,   176,   173,  1568,  1569,   137,   220,   440,   441,
     193,   158,   143,   227,   199,   184,   205,   102,   103,   194,
     193,   165,   210,  1587,  1588,   236,   173,   216,   199,   143,
     193,   219,   611,   243,   682,   200,   396,   193,   611,   193,
     193,   185,   193,   193,   893,   219,   217,   480,   217,   219,
     193,   124,   223,  1164,   219,   128,  1165,   193,   143,   173,
    1010,   494,   206,   194,   193,   713,   165,   321,    15,    16,
     173,   165,   219,   217,   158,   160,   161,   162,   163,  1203,
     155,   173,   689,  1006,  1007,   160,   193,  1154,   173,   173,
    1654,  1655,   165,   776,     5,     6,  1530,   165,   183,   313,
     127,   164,   316,   682,   163,   712,   129,   130,  1218,   682,
     137,    53,   185,  1508,  1600,   200,  1602,   127,   196,   193,
     193,   184,   195,  1609,   183,   193,   199,   219,   201,   202,
       5,     6,   217,     8,   713,   127,   985,   986,  1624,   988,
     713,  1265,   163,   193,   217,   137,   132,   222,     7,   173,
     223,   165,   593,   127,   142,    97,   597,   152,   368,   143,
     219,    36,   183,   137,   612,   613,   614,   194,   616,   617,
     220,   185,   620,   219,   622,   163,   624,    33,   173,   165,
    1614,   173,   392,   193,   194,   166,   196,   200,   398,   199,
    1676,    50,   774,   216,   217,   183,   844,  1321,   408,   185,
     221,   222,   194,   165,    60,    61,   219,   199,   222,   126,
     651,  1018,   129,   130,   655,   165,   648,   185,   197,   166,
     194,   653,   201,   185,   171,   193,   173,   437,   243,   176,
     440,  1154,   107,   840,   667,   185,  1722,  1723,   598,   201,
     813,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,   201,   825,   826,   827,   828,   829,   830,  1005,   185,
     977,   694,  1286,   473,   474,   844,   480,   193,   124,   165,
     216,   844,   128,  1759,   957,  1385,   173,   174,   175,   490,
     494,   163,   889,   890,   193,  1002,   152,  1397,   173,   185,
     504,   505,   506,  1147,   160,  1218,   903,   193,  1221,   216,
     217,   183,  1218,   127,   514,   515,   739,   173,   127,   206,
    1233,   220,   173,   137,   196,  1648,    57,  1233,   137,   760,
     165,   165,    63,   173,   757,   932,   933,  1660,   935,   193,
     541,   542,   939,   143,   941,   193,    68,    69,    70,   195,
     185,   185,   152,   199,   188,   201,   202,   157,   559,   194,
     561,   562,   563,   219,   565,  1379,   220,   193,   193,   188,
     189,   217,   220,   173,   163,   576,   173,   223,    21,    22,
     194,   193,  1705,  1706,   218,   194,   173,   392,   110,   111,
     112,   113,   218,   218,   183,   165,  1069,  1070,  1071,   218,
     200,   165,  1416,   408,   193,   173,   607,  1204,   220,  1248,
     177,   193,   612,   613,   614,   185,   616,   617,    21,    22,
     620,   185,   622,   193,   624,   165,   626,   165,   850,   193,
     868,   201,   437,   173,   143,   440,   874,   201,   220,   877,
     164,    21,    22,   152,   866,   185,   185,   185,   886,   173,
     193,   193,    57,   193,   193,   193,   894,  1254,    63,   197,
     184,   899,   173,   667,   173,   193,   108,   109,   110,   111,
     112,   113,   114,   115,   674,   118,   119,   220,   220,   838,
      21,    22,  1366,   126,   834,   128,   129,   130,   131,   689,
     694,   164,   220,   136,   193,   137,   219,   193,   221,   222,
     173,    57,   925,    33,   926,   147,   148,   149,   165,   514,
     515,   184,   712,   200,  1427,   118,   119,  1137,  1135,   218,
    1534,   477,   218,   126,   165,   128,   129,   130,   131,   210,
      60,    61,   219,   136,   144,   739,   737,   193,   118,   119,
    1453,   979,   193,   981,   185,  1389,   126,   165,   128,   129,
     130,   131,   193,   757,   754,   193,   136,   193,   177,   763,
     201,   204,   205,   206,   220,   193,  1163,   185,   200,   220,
     193,   193,   776,   216,   217,   193,    66,   118,   119,   193,
     536,   106,   220,   201,   220,   126,   194,   219,   129,   130,
     131,  1188,   220,   193,   124,   136,   218,   220,   128,   202,
     203,   204,   205,   206,   193,   165,   220,   612,   613,   614,
     165,   616,   617,   216,   217,   620,   185,   622,   193,   624,
     220,   626,   193,    35,   193,   185,   196,  1424,   165,   218,
     185,   201,    33,   193,  1378,   835,   216,   217,   193,   200,
     840,   201,   165,   218,   165,   846,   201,   218,   185,   219,
    1472,   158,   158,   164,   196,   856,  1253,   199,   219,    60,
      61,    47,   185,  1507,   185,   195,   173,   173,   868,   199,
     193,   201,   202,   184,   874,   216,   217,   877,   201,    35,
    1264,    67,   165,   196,   689,  1123,   886,   217,   201,   889,
     890,    57,   165,   223,   894,  1264,  1580,    63,   200,   899,
     158,  1264,   185,   903,   219,    57,   219,   712,   196,    33,
     193,    63,   185,   201,   185,   173,  1138,   219,   201,  1659,
     193,   925,   193,   124,   173,   681,   682,   128,   201,   165,
    1670,   219,   932,   933,   173,   935,    60,    61,   185,   939,
    1540,   941,   942,  1656,   196,   196,   193,   199,  1661,   185,
     201,   707,   193,   957,   710,   196,  1578,   193,   199,  1699,
     504,   505,   506,   158,   965,   201,   165,   968,   219,   173,
     971,   972,   197,   778,   779,   158,   201,   165,   173,   979,
    1202,   981,  1604,    57,  1697,   184,   185,   186,   793,    63,
     173,   197,   221,   222,   195,   201,   184,   185,   199,   187,
     124,   202,   184,   127,   128,   187,   811,    12,   190,  1516,
     197,   767,   556,   137,   201,   144,   217,   146,    23,    24,
    1242,  1243,   223,  1667,    57,   173,  1533,    75,  1628,  1629,
      63,    79,  1255,  1264,   197,   840,    22,   979,   201,   981,
     584,   165,   218,  1440,   197,    93,    94,   173,   201,   218,
      98,    99,   100,   101,  1767,   197,   197,   219,   197,   201,
     201,   185,   201,   868,   196,  1069,  1070,  1071,   196,   874,
     194,   195,   877,   177,   178,   199,   832,    43,   202,   200,
      10,   886,    92,    93,   889,   890,  1686,   200,   200,   894,
     144,   145,   146,   217,   899,   173,   174,   175,   903,   223,
     177,   178,   179,   180,  1342,   177,   178,   179,   177,   178,
     179,   219,   200,  1114,   459,   460,   461,  1118,    10,    11,
      12,   200,   219,  1123,   200,  1348,   219,   932,   933,   200,
     935,   197,   200,  1355,   939,  1135,   941,   942,   200,   895,
     200,  1363,  1539,   687,   200,   173,   219,   185,   218,   173,
      21,    22,   173,   220,   173,   200,   217,   200,   200,  1160,
     219,   219,   173,  1163,  1164,  1672,   200,   219,   200,   200,
    1320,   715,   166,   200,   979,   200,   981,   200,   108,   109,
     110,   111,   112,   113,   114,   115,   200,    13,  1188,   219,
     219,   121,   122,   219,    10,    33,  1419,   127,    37,   219,
     222,   219,   219,    66,  1442,   217,   962,   137,   200,   200,
     140,   141,   219,   219,   970,    43,  1439,   147,   148,   149,
     220,   173,    60,    61,   173,   218,   193,   196,    43,   219,
    1627,   219,   776,   193,   219,   200,   200,   200,   132,    14,
     194,   196,   166,  1048,   222,   116,   117,   118,   119,   142,
      13,  1255,   193,  1253,   193,   126,   218,   128,   129,   130,
     131,   219,   173,     8,   194,   136,   173,   138,   139,   201,
     220,   173,   173,   173,   220,   219,   142,    33,   193,   193,
    1503,   184,   220,   394,   219,   219,   124,   193,   173,   220,
     128,   221,   222,   219,   219,   200,   200,    33,   219,   219,
     219,     1,   846,   219,    60,    61,   219,   219,    67,   173,
     201,  1461,   201,  1314,   201,    43,  1554,   173,  1123,    12,
     196,   220,   220,   220,    60,    61,   173,    33,   173,   219,
    1135,   202,   203,   204,   205,   206,   220,   220,   220,   220,
     219,   218,  1342,   218,  1348,   216,   217,  1570,   218,   173,
     173,   220,   219,   173,    60,    61,   467,   195,  1163,  1164,
     200,   199,   219,   201,   202,   219,   477,  1368,   124,   219,
     219,   482,   128,   484,   220,   219,   173,   219,   181,   217,
     173,   218,   493,  1188,  1385,   223,   220,   218,   124,   173,
     501,   127,   128,   218,    33,  1151,  1397,   219,   219,   173,
     219,   137,  1625,   947,    70,   201,   219,   201,   220,   219,
     219,   219,   219,   957,   525,  1419,   220,   219,   124,   220,
    1421,   220,   128,   201,   177,   220,    53,   219,   219,    33,
     219,   219,   543,   185,   185,  1439,   218,   220,  1669,   195,
    1440,   220,  1442,   199,   220,   201,   202,   220,  1253,   184,
     193,   218,  1683,   185,   193,  1211,    60,    61,   194,   195,
     220,   217,   220,   199,   218,   220,   202,   223,   184,   741,
     220,   220,   220,   220,   220,   219,  1232,    33,   218,   220,
     218,   217,  1238,   594,    85,   218,   220,   223,  1719,   195,
     220,  1247,   218,   199,     1,   201,   202,   141,    46,  1503,
     611,    88,  1531,   835,    60,    61,  1262,  1004,  1531,  1531,
     241,   217,  1531,  1736,  1531,     1,  1503,   223,  1013,  1437,
     124,  1278,  1565,  1317,   128,  1069,  1070,  1071,  1463,    56,
    1074,  1566,  1076,  1566,  1078,  1291,  1080,  1342,  1082,  1539,
    1084,  1466,  1086,   395,  1088,   473,  1090,  1303,  1092,  1661,
    1094,   473,  1308,  1097,  1554,  1099,  1514,  1101,   733,  1103,
    1230,  1105,    -1,  1107,    -1,    -1,  1570,    -1,   124,    -1,
      -1,   682,   128,    -1,  1118,    -1,    -1,    -1,    -1,    33,
     691,    -1,    -1,    -1,    -1,  1735,    -1,    -1,    -1,    -1,
      -1,   195,  1593,  1349,    -1,   199,    -1,   201,   202,  1600,
      -1,  1602,   713,    -1,    -1,    -1,    60,    61,  1609,    -1,
      -1,    -1,  1612,   217,    -1,  1765,    -1,    -1,    -1,   223,
     731,  1625,   733,  1624,  1380,    -1,    -1,  1627,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1440,    -1,  1442,    -1,   195,
      -1,    -1,    -1,   199,    -1,   201,   202,    -1,    -1,    -1,
      -1,    -1,  1408,  1409,    -1,    -1,  1412,    21,    22,    -1,
      -1,   217,    -1,    -1,    -1,    -1,    -1,   223,    -1,    -1,
     124,    -1,    -1,    -1,   128,  1676,    -1,    -1,  1678,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1689,    -1,
      -1,    -1,    -1,    -1,  1695,    -1,    -1,    -1,    -1,    -1,
      33,    -1,   813,   814,   815,   816,   817,   818,   819,   820,
     821,   822,   823,   824,   825,   826,   827,   828,   829,   830,
      -1,  1722,  1723,    -1,    -1,    -1,    -1,    60,    61,    -1,
    1731,  1732,  1736,   844,  1539,    -1,    -1,    -1,    -1,    -1,
      -1,   195,    -1,    -1,    -1,   199,    -1,   201,   202,  1554,
      -1,    -1,   116,   117,   118,   119,   120,    -1,  1759,   123,
    1761,    -1,   126,   217,   128,   129,   130,   131,  1524,   223,
      -1,    -1,   136,    -1,   138,   139,    -1,   888,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   896,    -1,   898,    -1,    -1,
      -1,   124,    -1,    -1,    -1,   128,    -1,   908,   909,   910,
     911,   912,   913,   914,   915,   916,   917,   918,   919,   920,
     921,   922,   923,   924,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1627,    -1,    -1,    -1,    -1,    -1,  1584,    -1,
      -1,    -1,    -1,    -1,  1378,    -1,   200,   201,   202,   203,
     204,   205,   206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   216,   217,    -1,    -1,   405,    -1,    -1,    -1,
    1616,    -1,   195,   974,    -1,   414,   199,    -1,   201,   202,
      -1,    -1,   983,    -1,    -1,   424,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,   434,    -1,    -1,   999,    -1,
     223,    -1,    -1,    -1,    -1,    -1,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,   455,   825,   826,   827,
     828,   829,   830,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1680,    -1,    -1,    -1,   477,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    13,
      -1,    -1,    -1,    -1,    -1,    19,    21,    22,    -1,    -1,
      -1,    25,    -1,   502,   503,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,
      -1,   530,   531,   532,   533,   534,   535,   536,    -1,    -1,
      64,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1191,   630,   156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     639,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,
      -1,    -1,    -1,  1214,  1215,    -1,    -1,    -1,  1219,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1227,    -1,    -1,  1230,
      -1,    -1,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   681,   682,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   216,   217,    -1,   693,    -1,    -1,   221,  1259,   698,
      -1,    -1,    -1,  1264,    -1,   704,    -1,    -1,   707,    -1,
      10,   710,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,   724,   725,   726,   727,   728,
     729,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   767,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   780,   781,    -1,  1345,   784,   785,   786,   787,    -1,
     789,    -1,   791,   792,   793,   794,   795,   796,   797,   798,
     799,   800,   801,   802,   803,   804,   805,   806,   807,   808,
     809,   810,    -1,   812,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    -1,   832,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,   165,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   881,    33,    -1,   185,    -1,    -1,    -1,    -1,
      -1,    33,    -1,   193,    -1,   195,   895,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,    -1,   907,    -1,
      60,    61,    -1,    -1,    21,    22,   216,   217,    60,    61,
      -1,  1482,    -1,  1484,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1494,  1495,  1496,   198,   199,   200,   201,
     202,   203,   204,   205,   206,    33,    -1,    -1,    -1,    -1,
    1511,  1512,    -1,    33,   216,   217,  1517,    -1,    -1,    -1,
      -1,   960,   961,   962,    -1,    -1,    -1,    -1,    -1,    -1,
    1531,   970,    60,    61,   124,    -1,    -1,    -1,   128,    -1,
      60,    61,   124,    -1,  1545,    -1,   128,    -1,    -1,    -1,
      -1,   990,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1007,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,  1594,  1595,    -1,   124,    -1,    -1,    -1,
     128,    -1,    -1,    -1,   124,   195,    -1,    -1,   128,   199,
      -1,   201,   202,   195,    -1,    -1,  1617,   199,    -1,   201,
     202,    -1,  1061,    -1,    -1,    -1,  1065,   217,    -1,  1630,
      -1,  1632,    -1,   223,    -1,   217,    -1,    -1,    -1,    -1,
      -1,   223,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1651,    -1,  1653,   200,   201,   202,   203,   204,   205,   206,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   216,
     217,   199,    13,   201,   202,   195,    -1,    -1,    19,   199,
    1681,   201,   202,    -1,    25,    -1,  1687,  1688,    -1,   217,
      31,    -1,    -1,    -1,    -1,   223,    -1,   217,    -1,    -1,
      41,    -1,    -1,   223,    -1,    -1,    -1,    -1,    49,    -1,
      -1,    -1,  1151,  1714,    -1,  1716,  1717,    -1,    -1,    -1,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,  1730,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1211,    -1,  1213,    -1,    -1,  1216,    -1,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1232,    -1,    -1,    -1,    -1,    -1,  1238,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,  1247,    -1,
      -1,    -1,  1251,  1252,    -1,   156,    -1,    -1,    -1,    -1,
      21,    22,    -1,  1262,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1270,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1288,
      -1,  1290,  1291,  1292,    -1,    -1,    -1,    -1,    -1,  1298,
      -1,    -1,    -1,  1302,  1303,    -1,    -1,    -1,    -1,  1308,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
     221,   123,   124,   125,   126,    33,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
    1349,    -1,    60,    61,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,  1380,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
      -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,    -1,    -1,    -1,    -1,  1408,
    1409,    -1,    -1,  1412,   216,   217,   124,    -1,   220,    -1,
     128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1427,    -1,
      -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    -1,    -1,    -1,  1448,
      -1,  1450,    -1,    -1,  1453,   216,   217,  1456,    -1,   220,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1472,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    -1,    -1,    -1,    -1,    31,   195,    -1,    -1,
      -1,   199,    -1,   201,   202,    -1,    41,    -1,    -1,    -1,
      -1,    -1,  1501,    -1,    49,    -1,    -1,  1506,    -1,   217,
      -1,  1510,    -1,    -1,  1513,   223,    -1,    -1,    -1,    64,
      -1,    -1,    -1,    -1,  1523,  1524,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1578,
      -1,    -1,    -1,    -1,    -1,  1584,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,    -1,    -1,  1604,    -1,    -1,    -1,    -1,
      -1,   156,  1611,    -1,    -1,     1,    -1,  1616,    -1,     5,
       6,     7,    -1,     9,    10,    11,    -1,    13,   173,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    38,    39,    -1,    -1,    42,    -1,    44,    45,
      -1,    -1,    48,    -1,    50,    51,    52,    -1,    54,    55,
      -1,    -1,    58,    59,  1673,    -1,   221,    -1,   223,    65,
      -1,  1680,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,    -1,    -1,    -1,    -1,    -1,  1725,    -1,    -1,    -1,
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   133,   134,   135,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,   184,   185,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,
      -1,   217,    -1,   219,    -1,   221,   222,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    33,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    53,    -1,    55,    -1,    -1,
      -1,    -1,    60,    61,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,   124,    -1,    -1,    -1,
     128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,    -1,
      -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,    -1,
      -1,   199,    -1,    -1,   202,   203,   204,    -1,   206,    -1,
      -1,   209,   210,    -1,    -1,    -1,     5,     6,    -1,   217,
      -1,   219,    -1,   221,   222,   223,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    60,    61,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,   124,    -1,    -1,    -1,   128,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,   158,
     159,   160,   161,   162,   163,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,    -1,    -1,
     199,    -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,
     209,   210,    -1,    -1,    -1,     5,     6,    -1,   217,    -1,
     219,    -1,   221,   222,   223,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    39,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   199,
      -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,
     210,    -1,    -1,    -1,     5,     6,    -1,   217,    -1,   219,
      -1,   221,   222,   223,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,    -1,
      -1,    -1,   133,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,    -1,
     221,   222,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,
     133,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,
     153,   154,   155,    -1,   157,    -1,   159,   160,   161,   162,
     163,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,
       5,     6,    -1,    -1,   217,    -1,   219,    -1,   221,   222,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    26,    27,    28,    -1,    -1,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    48,    -1,    -1,    51,    52,    -1,    -1,
      55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,
     155,    -1,   157,    -1,   159,   160,   161,   162,   163,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,
      -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,
      -1,    -1,   217,    -1,   219,    -1,   221,   222,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   133,   134,   135,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,
     157,    -1,   159,   160,   161,   162,   163,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,   206,
      -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,
     217,    -1,   219,    -1,   221,   222,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    70,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,   158,
     159,   160,   161,   162,   163,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,
     209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,
     219,   220,   221,   222,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   133,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,    -1,    -1,     5,     6,   217,    -1,   219,    -1,
     221,   222,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    49,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,    -1,    -1,     5,     6,   217,   218,   219,    -1,
     221,   222,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    49,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,   158,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,    -1,    -1,     5,     6,   217,    -1,   219,    -1,
     221,   222,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    49,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,    -1,    -1,     5,     6,   217,   218,   219,    -1,
     221,   222,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    49,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,    -1,
     221,   222,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    70,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,
     153,   154,   155,    -1,   157,   158,   159,   160,   161,   162,
     163,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,
       5,     6,    -1,    -1,   217,    -1,   219,    -1,   221,   222,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,
      -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,
      -1,    -1,   217,   218,   219,    -1,   221,   222,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    61,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,
     157,    -1,   159,   160,   161,   162,   163,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,   206,
      -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,
     217,    -1,   219,    -1,   221,   222,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    58,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,   163,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,
     209,   210,    -1,    -1,    -1,    -1,     5,     6,   217,    -1,
     219,    -1,   221,   222,    13,    -1,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,   163,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,
     209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,
     219,    -1,   221,   222,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,    -1,    -1,     5,     6,   217,    -1,   219,    -1,
     221,   222,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,    -1,
     221,   222,    15,    16,    17,    18,    19,    -1,    -1,    22,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,
     153,   154,   155,    -1,   157,    -1,   159,   160,   161,   162,
     163,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,
       5,     6,    -1,    -1,   217,    -1,   219,    -1,   221,   222,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,
      -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,
      -1,    -1,   217,    -1,   219,   220,   221,   222,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,
     157,    -1,   159,   160,   161,   162,   163,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   200,    -1,   202,   203,   204,    -1,   206,
      -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,
     217,    -1,   219,    -1,   221,   222,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,   163,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,
     209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,
     219,   220,   221,   222,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,   220,
     221,   222,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,
     153,   154,   155,    -1,   157,    -1,   159,   160,   161,   162,
     163,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,
       5,     6,    -1,    -1,   217,    -1,   219,   220,   221,   222,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,
      -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,
      -1,    -1,   217,    -1,   219,   220,   221,   222,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,
     157,    -1,   159,   160,   161,   162,   163,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,   206,
      -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,
     217,    -1,   219,   220,   221,   222,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,   163,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,
     209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,
     219,   220,   221,   222,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,   206,    -1,    -1,   209,   210,
      -1,    -1,     5,     6,    -1,    -1,   217,    -1,   219,   220,
     221,   222,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,
     153,   154,   155,    -1,   157,    -1,   159,   160,   161,   162,
     163,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,   204,    -1,   206,    -1,    -1,   209,   210,    -1,    -1,
       5,     6,    -1,    -1,   217,    -1,   219,    -1,   221,   222,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,
      55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,
     155,    -1,   157,    -1,   159,   160,   161,   162,   163,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,
      -1,   206,    -1,    -1,   209,   210,    -1,    -1,     5,     6,
      -1,    -1,   217,    -1,   219,    -1,   221,   222,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,
     157,    -1,   159,   160,   161,   162,   163,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,   206,
      -1,    -1,   209,   210,    -1,    -1,     5,     6,    -1,    -1,
     217,    -1,   219,    -1,   221,   222,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,   163,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,   204,    -1,   206,    -1,    -1,
     209,   210,    -1,    -1,     5,     6,    -1,    -1,   217,    -1,
     219,    -1,   221,   222,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,   163,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    19,    -1,    -1,
      -1,    -1,   183,    25,    -1,    -1,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,
      -1,   202,   203,   204,    -1,   206,    -1,    49,   209,   210,
      19,    -1,    -1,    -1,    -1,    -1,   217,    -1,   219,    -1,
     221,   222,    64,    -1,    -1,    -1,    -1,    -1,    -1,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   156,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   165,    -1,    -1,    -1,    -1,    19,    -1,
      -1,   173,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,
      31,    -1,    -1,   185,    -1,    -1,    -1,    -1,    -1,   158,
      41,   193,    -1,    -1,    -1,    -1,    -1,    -1,    49,    -1,
      -1,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,   221,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    41,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    -1,    -1,    -1,   156,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,   173,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
     221,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    21,    22,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,   143,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   156,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   221,    -1,    -1,    -1,    -1,   116,   117,
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
      -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   195,    -1,   197,   198,   199,   200,   201,   202,
     203,   204,   205,   206,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   216,   217,    -1,    -1,   220,    -1,    -1,
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
      -1,    -1,    -1,    -1,   216,   217,    -1,    -1,   220,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   216,   217,    -1,    -1,   220,    -1,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    21,    22,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,   220,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,
      -1,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     216,   217,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    21,    22,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
     195,    -1,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   216,   217,    -1,    -1,   220,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     195,    -1,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   216,   217,    -1,    -1,   220,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    21,    22,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   195,    -1,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   216,   217,    -1,    -1,   220,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,
      -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,   195,    -1,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
      -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
      -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   216,   217,    -1,    -1,   220,    -1,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
      -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,   220,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,   220,
      -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
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
     217,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
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
     216,   217,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   216,   217,   218,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    21,    22,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   216,   217,   218,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   216,   217,   218,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    21,    22,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      38,   150,    -1,   108,   109,   110,   111,   112,   113,   114,
     115,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,    -1,
      -1,    -1,   127,    -1,    -1,    21,    22,   132,    -1,    -1,
      -1,    -1,   137,    -1,    -1,   140,   141,    -1,    -1,    -1,
      -1,    -1,   147,   148,   149,    -1,   195,    -1,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,   218,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,    -1,   194,
     128,   129,   130,    -1,    -1,    -1,    -1,    -1,    -1,   137,
     138,   139,   140,   141,    -1,    -1,   144,   145,   146,   147,
     148,   149,   150,    -1,    -1,    -1,   221,   222,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    21,    22,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,    -1,    -1,   195,    -1,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,   165,
      -1,   209,   210,    21,    22,    -1,    -1,    -1,   216,   217,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   185,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,
      -1,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     216,   217,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   216,   217,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    21,    22,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   195,    -1,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,    -1,   146,    -1,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,   199,
     200,   201,   202,   203,   204,   205,   206,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   216,   217,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   158,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,
     173,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    35,    98,    99,   100,   101,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   158,    -1,   129,   130,    -1,    -1,
      71,    -1,    73,    -1,    75,    76,    77,    78,    79,   173,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,   158,    -1,    98,    99,   100,
     101,   108,   109,   110,   111,   112,   113,   114,   115,    -1,
     173,    -1,    -1,    -1,   121,   122,    -1,   118,   119,    -1,
     127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     137,    -1,    -1,   140,   141,    -1,    -1,    -1,    -1,    -1,
     147,   148,   149,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   216,   217,    -1,    -1,    -1,   165,    -1,
     108,   109,   110,   111,   112,   113,   114,   115,    -1,    -1,
      -1,    -1,   173,   121,   122,    -1,    -1,    -1,   185,   127,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   194,    -1,   137,
      -1,    -1,   140,   141,    -1,    -1,    -1,    -1,    -1,   147,
     148,   149,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   221,   222,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   194,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   221,   222
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   225,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   102,   103,   165,   185,   206,   217,   226,   227,   231,
     240,   242,   243,   248,   279,   283,   319,   402,   409,   413,
     420,   466,   471,   476,    19,    20,   173,   271,   272,   273,
     166,   249,   250,   173,   174,   175,   206,   244,   245,   246,
     163,   183,   288,   410,   173,   221,   229,    57,    63,   405,
     405,   405,   143,   173,   305,    34,    63,   107,   136,   210,
     219,   275,   276,   277,   278,   305,   226,     5,     6,     8,
      36,   107,   417,    62,   400,   194,   193,   196,   193,   245,
      22,    57,   205,   216,   247,   405,   406,   408,   406,   400,
     477,   467,   472,   173,   143,   241,   277,   277,   277,   219,
     144,   145,   146,   193,   218,    57,    63,   284,   286,    57,
      63,   411,     5,     6,    57,    63,   418,    57,    63,   401,
      15,    16,   166,   171,   173,   176,   219,   233,   272,   166,
     250,   173,   244,   244,   173,   165,   185,   376,   164,   184,
     289,   406,   226,    57,    63,   228,   173,   173,   173,   173,
     177,   239,   220,   273,   277,   277,   277,   277,   287,   173,
     412,   421,   288,   403,   177,   178,   179,   232,    15,    16,
     166,   171,   173,   233,   269,   270,   247,   407,   165,   376,
     210,   230,   478,   468,   473,   177,   220,    35,    71,    73,
      75,    76,    77,    78,    79,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    93,    94,    95,    98,
      99,   100,   101,   118,   119,   173,   282,   285,   196,   288,
     106,   415,   416,   398,   160,   222,   274,   370,   177,   178,
     179,   193,   220,   194,    66,   288,   288,   288,    21,    22,
      38,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   128,
     129,   130,   137,   138,   139,   140,   141,   144,   145,   146,
     147,   148,   149,   150,   195,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   209,   210,   216,   217,    35,
      35,   219,   280,   288,   290,    75,    79,    93,    94,    98,
      99,   100,   101,   425,   404,   173,   422,   289,   399,   273,
     272,   222,   226,   152,   173,   396,   397,   269,    19,    25,
      31,    41,    49,    64,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   156,   221,   305,
     424,   426,   427,   431,   437,   465,   430,   469,   474,   173,
     173,   173,   218,    22,   173,   218,   155,   220,   370,   381,
     382,   383,   196,   281,   293,   288,   173,   289,   196,   414,
     288,   419,   370,   218,   272,   219,    43,   193,   196,   199,
     395,   200,   200,   200,   219,   200,   200,   219,   200,   200,
     200,   200,   200,   200,   219,   305,    33,    60,    61,   124,
     128,   195,   199,   202,   217,   223,   436,   197,   173,   289,
     386,   389,   173,   108,   109,   110,   111,   112,   113,   114,
     115,   137,   147,   148,   149,   219,     7,    50,   318,   185,
     185,   193,   220,   465,     1,     5,     6,     9,    10,    11,
      13,    15,    16,    17,    18,    19,    25,    26,    27,    28,
      29,    31,    38,    39,    42,    44,    45,    48,    51,    52,
      54,    55,    58,    59,    65,    68,    69,    80,   102,   103,
     104,   105,   118,   119,   133,   134,   135,   151,   152,   153,
     154,   155,   157,   159,   160,   161,   162,   165,   166,   167,
     168,   169,   170,   171,   172,   174,   175,   176,   183,   185,
     202,   203,   204,   209,   210,   217,   219,   221,   222,   238,
     240,   251,   252,   255,   258,   259,   261,   264,   265,   266,
     267,   288,   289,   291,   292,   294,   299,   304,   305,   306,
     310,   311,   312,   313,   314,   315,   316,   317,   319,   323,
     324,   331,   334,   337,   342,   345,   346,   348,   349,   350,
     352,   357,   360,   361,   368,   369,   424,   479,   494,   505,
     509,   522,   525,   404,   194,   376,   305,   377,   397,   218,
      65,   104,   174,   299,   361,   173,   173,   437,   127,   137,
     194,   394,   438,   443,   445,   361,   447,   441,   173,   432,
     449,   451,   453,   455,   457,   459,   461,   463,   361,   200,
     219,    33,   199,    33,   199,   217,   223,   218,   361,   217,
     223,   437,   165,   193,   429,   173,   193,   226,   384,   434,
     465,   470,   173,   387,   434,   475,   361,   152,   173,   391,
     392,   423,   383,   383,   383,   200,   200,   368,   262,   263,
     200,   307,   426,   479,   219,   305,   200,     5,   102,   103,
     200,   219,   127,   304,   335,   346,   361,   368,   290,   200,
     219,    61,   368,   219,   368,   173,   200,   200,   219,   226,
     200,   166,    58,   368,   219,   290,   200,   219,   200,   200,
     219,   200,   200,   127,   304,   368,   361,   361,   222,   290,
     337,   341,   341,   341,   219,   219,   219,   219,   219,   219,
      13,   437,    13,   437,    13,   368,   504,   520,   200,   368,
     200,   237,    13,   361,   361,   361,   361,   361,    13,    49,
     295,   335,   361,   335,   222,   226,   226,   368,    10,    13,
     297,   504,   521,    37,   337,   343,   173,   219,   226,   226,
     226,   226,   226,    66,   320,   279,   132,   226,    21,    22,
     116,   117,   118,   119,   120,   123,   124,   125,   126,   128,
     129,   130,   131,   136,   138,   139,   144,   145,   146,   150,
     195,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,   216,   217,   108,   109,   110,   111,   112,   113,   114,
     115,   121,   122,   127,   137,   140,   141,   147,   148,   149,
     194,   343,   219,   289,   368,   217,   279,   289,   378,   376,
     200,   220,    43,   226,   394,   304,   368,   465,   465,   435,
     465,   220,   465,   465,   220,   173,   428,   465,   280,   465,
     280,   465,   280,   384,   385,   387,   388,   220,   440,   295,
     218,   218,   361,   193,   196,   434,   289,   196,   434,   289,
     220,   219,    43,   127,   193,   194,   196,   199,   390,   495,
     497,   290,   423,   219,   308,   219,   305,   200,   219,   332,
     200,   200,   200,   516,   335,   304,   335,   193,   108,   109,
     110,   111,   112,   113,   114,   115,   121,   122,   127,   140,
     141,   147,   148,   149,   194,    14,   437,   297,   368,   361,
     290,   194,   325,   327,   361,   329,   196,   166,   361,   518,
     335,   501,   506,   335,   499,   437,   304,   368,   222,   279,
     361,   361,   361,   361,   361,   361,   423,    53,   158,   173,
     202,   217,   219,   368,   480,   483,   487,   503,   508,   423,
     219,   483,   508,   423,   142,   184,   186,   226,   488,   300,
     290,   302,   179,   180,   232,   219,   219,   423,    13,   218,
     193,   524,   524,   152,   157,   200,   305,   351,   290,   260,
     423,   289,   193,   524,   288,   344,    70,   217,   220,   335,
     480,   482,   160,   219,   322,   397,     4,   160,   340,   341,
      19,   158,   173,   424,    19,   158,   173,   424,   361,   361,
     361,   361,   361,   361,   173,   361,   158,   173,   361,   361,
     361,   424,   361,   361,   361,   361,   361,   361,    22,   361,
     361,   361,   361,   361,   361,   361,   361,   361,   361,   361,
     129,   130,   158,   173,   216,   217,   358,   424,   361,   133,
     134,   135,   291,   347,   368,   347,   368,   347,   368,   347,
     368,   347,   368,   347,   368,   347,   368,   347,   368,   347,
     368,   347,   368,   347,   368,   368,   347,   368,   347,   368,
     347,   368,   347,   368,   347,   368,   347,   368,   220,   335,
     376,   278,     8,   370,   375,   437,   173,   304,   368,   226,
     201,   201,   201,   434,   201,   201,   201,   226,   201,   281,
     201,   281,   201,   281,   201,   434,   201,   434,   298,   465,
     220,   218,   465,   465,   361,   173,   173,   465,   368,   437,
     437,    20,   423,   465,    70,   335,   482,   493,   200,   368,
     173,   368,   465,   510,   512,   514,   437,   524,   361,   368,
     368,   368,   368,   368,   368,   368,   368,   368,   368,   368,
     368,   368,   368,   368,   368,   368,   290,   201,   434,   220,
     220,   268,   437,   437,   220,   437,   220,   437,   524,   437,
     385,   524,   388,   201,   340,   220,   220,   220,   220,   220,
     220,    20,   341,   219,   137,   390,   217,   361,   220,   142,
     193,   226,   487,   188,   189,   218,   491,   193,   188,   218,
     226,   490,    20,   220,   487,   184,   187,   489,    20,   368,
     184,   504,   298,   298,   368,   423,   423,    20,   219,   423,
     220,   219,   219,   353,   355,    12,    23,    24,   165,   253,
     254,   368,    20,   504,   293,   279,   173,   220,   482,   480,
     193,   220,   193,   524,   220,   173,   321,   321,   219,   127,
     137,   173,   194,   199,   338,   339,   280,   200,   219,   200,
     219,   219,   219,   218,    19,   158,   173,   424,   196,   158,
     173,   361,   219,   219,   158,   173,   361,     1,   219,   218,
     341,   341,   341,   220,   218,    57,    63,   373,    67,   374,
     226,   201,   226,   439,   444,   446,   465,   448,   442,   433,
     173,   450,   201,   454,   201,   458,   201,   462,   384,   464,
     387,   201,   434,   220,    43,   390,   201,   201,   335,    20,
     201,   482,   220,   220,   220,   173,   220,   201,   226,   220,
     201,   437,   385,   388,   201,   220,   219,   437,   368,   201,
     201,   201,   201,   220,   201,   201,   220,   201,   340,   280,
     219,   335,   361,   368,   368,   483,   487,   368,   158,   173,
     480,   491,   218,   368,   218,   503,   335,   483,   184,   187,
     190,   492,   218,   335,   201,   201,   196,   235,    20,    20,
     335,   423,    20,   361,   361,   437,   280,   290,   254,   368,
      12,   256,   335,   289,   340,   220,   218,   217,   193,   218,
     220,   339,   173,   173,   219,   173,   173,   193,   218,   281,
     362,   361,   364,   361,   220,   335,   361,   200,   219,   361,
     219,   218,   361,   217,   220,   335,   219,   218,   359,   220,
     335,   226,    47,   374,    46,   106,   371,   376,   340,   452,
     456,   460,   219,   465,   173,   368,   496,   498,   290,   335,
     309,   220,   201,   434,   219,   173,   333,   201,   201,   201,
     517,   297,   201,   226,   326,   328,   330,   519,   502,   507,
     500,   219,   343,   281,   220,   335,   185,   220,   487,   491,
     219,   137,   390,   185,   487,   218,   185,   301,   303,   236,
     181,   335,   335,   185,    20,   335,   220,   220,   201,   281,
     290,   257,   226,   185,   280,   220,   480,   173,   218,   196,
     395,   220,   173,   338,   218,   142,   290,   336,   437,   220,
     465,   220,   220,   220,   366,   361,   361,   220,   480,   220,
     361,   220,   376,    33,   372,   371,   373,   295,   219,   219,
     220,   368,   173,   368,   201,   511,   513,   515,   219,   220,
     219,   368,   368,   368,   219,    70,   493,   219,   219,   220,
     361,   336,   220,   361,   137,   390,   491,   361,   368,   368,
     361,   492,   504,   368,   219,   296,   234,   220,   220,   361,
     335,   185,   354,   201,   165,   254,    26,   105,   258,   311,
     312,   313,   315,   368,   504,   281,   218,   196,   395,   437,
     394,   220,   127,   368,   201,   201,   465,   220,   220,   218,
     220,   379,   372,   391,   392,   393,   220,   493,   493,   290,
     220,   201,   220,   219,   219,   219,   219,   295,   297,   335,
     493,   493,   220,   226,   523,   368,   368,   220,   523,   523,
     295,   177,   185,   185,   523,   220,   361,   351,   356,   254,
     127,   127,   368,   523,   290,   220,   437,   394,   394,   368,
     368,   363,   365,   201,   220,   285,   380,   219,   480,   484,
     485,   486,   486,   368,   368,   493,   493,   480,   481,   220,
     220,   524,   486,   481,    53,   218,   137,   390,   184,   289,
     193,   524,   504,   361,   218,   185,   523,   351,   368,   289,
     394,   368,   368,   226,   367,   226,   285,   480,   193,   524,
     220,   220,   220,   220,   486,   486,   220,   220,   220,   220,
     368,   218,   368,   368,   218,   289,   220,   523,   523,   361,
     218,   368,   226,   226,   376,   290,   220,   219,   220,   220,
     184,   218,   523,   226,   376,   480,   218,   220
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   224,   225,   225,   225,   225,   225,   225,   225,   225,
     225,   225,   225,   225,   225,   225,   225,   226,   226,   227,
     228,   228,   228,   229,   229,   230,   230,   231,   232,   232,
     232,   232,   233,   233,   234,   234,   235,   236,   235,   237,
     237,   237,   238,   239,   239,   241,   240,   242,   243,   244,
     244,   244,   245,   245,   245,   245,   246,   246,   247,   247,
     248,   249,   249,   250,   250,   251,   252,   252,   253,   253,
     254,   254,   254,   254,   255,   255,   256,   257,   256,   258,
     258,   258,   258,   258,   259,   259,   260,   259,   262,   261,
     263,   261,   264,   265,   266,   268,   267,   269,   269,   269,
     269,   269,   269,   270,   270,   271,   271,   271,   272,   272,
     272,   272,   272,   272,   272,   272,   273,   273,   274,   274,
     274,   275,   275,   275,   275,   276,   276,   277,   277,   277,
     277,   277,   277,   277,   278,   278,   279,   279,   280,   280,
     280,   281,   281,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   283,
     284,   284,   284,   285,   287,   286,   288,   288,   289,   289,
     290,   290,   291,   291,   291,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   293,   293,   293,
     294,   295,   295,   296,   296,   297,   297,   298,   298,   300,
     301,   299,   302,   303,   299,   304,   304,   304,   304,   304,
     305,   305,   305,   306,   306,   308,   309,   307,   307,   310,
     310,   310,   310,   310,   310,   311,   312,   313,   313,   313,
     314,   314,   314,   315,   315,   316,   316,   316,   317,   318,
     318,   318,   319,   319,   320,   320,   321,   321,   322,   322,
     322,   322,   323,   323,   325,   326,   324,   327,   328,   324,
     329,   330,   324,   332,   333,   331,   334,   334,   334,   334,
     334,   334,   335,   335,   336,   336,   336,   337,   337,   337,
     338,   338,   338,   338,   338,   339,   339,   340,   340,   340,
     341,   341,   342,   344,   343,   345,   345,   345,   345,   345,
     345,   345,   346,   346,   346,   346,   346,   346,   346,   346,
     346,   346,   346,   346,   346,   346,   346,   346,   346,   346,
     346,   347,   347,   347,   347,   348,   348,   348,   348,   348,
     348,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     348,   348,   349,   349,   350,   350,   351,   351,   352,   353,
     354,   352,   355,   356,   352,   357,   357,   357,   357,   357,
     357,   357,   358,   359,   357,   360,   360,   360,   360,   360,
     360,   360,   361,   361,   361,   361,   361,   361,   361,   361,
     361,   361,   361,   361,   361,   361,   361,   361,   361,   361,
     361,   361,   361,   361,   361,   361,   361,   361,   361,   361,
     361,   361,   361,   361,   361,   361,   361,   361,   361,   361,
     361,   361,   361,   361,   361,   361,   361,   361,   361,   361,
     361,   361,   361,   361,   361,   361,   361,   361,   362,   363,
     361,   361,   361,   361,   364,   365,   361,   361,   361,   366,
     367,   361,   361,   361,   361,   361,   361,   361,   361,   361,
     361,   361,   361,   361,   361,   361,   368,   369,   369,   369,
     369,   369,   369,   369,   369,   369,   369,   369,   369,   369,
     369,   369,   369,   370,   370,   370,   371,   371,   371,   372,
     372,   373,   373,   373,   374,   374,   375,   376,   376,   376,
     376,   377,   378,   377,   379,   377,   380,   377,   377,   381,
     382,   382,   383,   383,   383,   383,   383,   384,   384,   385,
     385,   386,   386,   386,   387,   388,   388,   389,   389,   389,
     390,   390,   391,   391,   391,   392,   392,   393,   393,   394,
     394,   394,   395,   395,   396,   396,   396,   396,   396,   397,
     397,   397,   397,   397,   397,   398,   399,   398,   400,   400,
     401,   401,   401,   402,   403,   402,   404,   404,   404,   405,
     405,   405,   407,   406,   408,   408,   409,   410,   409,   411,
     411,   411,   412,   413,   413,   414,   414,   415,   415,   416,
     417,   417,   417,   417,   418,   418,   418,   419,   419,   421,
     422,   420,   423,   423,   423,   423,   423,   424,   424,   424,
     424,   424,   424,   424,   424,   424,   424,   424,   424,   424,
     424,   424,   424,   424,   424,   424,   424,   424,   424,   424,
     424,   424,   424,   424,   425,   425,   425,   425,   425,   425,
     425,   425,   426,   427,   427,   427,   428,   428,   429,   429,
     430,   430,   430,   430,   432,   433,   431,   434,   434,   435,
     435,   436,   436,   437,   437,   437,   437,   437,   437,   438,
     439,   437,   437,   437,   440,   437,   437,   437,   437,   437,
     437,   437,   437,   437,   437,   437,   437,   437,   441,   442,
     437,   437,   443,   444,   437,   445,   446,   437,   447,   448,
     437,   437,   449,   450,   437,   451,   452,   437,   437,   453,
     454,   437,   455,   456,   437,   437,   457,   458,   437,   459,
     460,   437,   461,   462,   437,   463,   464,   437,   465,   465,
     465,   467,   468,   469,   470,   466,   472,   473,   474,   475,
     471,   477,   478,   476,   479,   479,   479,   479,   479,   480,
     480,   480,   480,   480,   480,   480,   480,   481,   481,   482,
     483,   483,   484,   484,   485,   485,   486,   486,   487,   487,
     488,   488,   489,   489,   490,   490,   491,   491,   491,   492,
     492,   492,   493,   493,   494,   494,   494,   494,   494,   494,
     495,   496,   494,   497,   498,   494,   499,   500,   494,   501,
     502,   494,   503,   503,   503,   504,   504,   505,   506,   507,
     505,   508,   508,   509,   509,   509,   510,   511,   509,   512,
     513,   509,   514,   515,   509,   509,   516,   517,   509,   509,
     518,   519,   509,   520,   520,   521,   521,   522,   522,   522,
     522,   522,   523,   523,   524,   524,   525,   525,   525,   525,
     525,   525,   525,   525,   525
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     1,     1,     2,
       0,     1,     1,     1,     1,     0,     2,     5,     1,     1,
       2,     2,     3,     2,     0,     2,     0,     0,     3,     0,
       2,     5,     3,     1,     2,     0,     4,     2,     2,     1,
       1,     1,     1,     2,     3,     3,     2,     4,     0,     1,
       2,     1,     3,     1,     3,     3,     3,     2,     1,     1,
       0,     2,     4,     5,     1,     1,     0,     0,     3,     1,
       1,     1,     1,     1,     5,     4,     0,     6,     0,     6,
       0,     8,     2,     3,     3,     0,     6,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     1,     5,     1,     3,     2,     3,
       2,     1,     1,     1,     1,     1,     4,     1,     2,     3,
       3,     3,     3,     2,     1,     3,     0,     3,     0,     2,
       3,     0,     2,     1,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     3,     3,     2,
       2,     3,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     3,     2,     2,     2,     2,     2,
       3,     3,     3,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       0,     1,     1,     3,     0,     4,     1,     1,     1,     1,
       3,     7,     2,     2,     6,     1,     1,     1,     1,     1,
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
       0,     8,     0,     0,     9,     3,     4,     5,     6,     8,
       5,     6,     0,     0,     5,     3,     4,     4,     5,     4,
       3,     4,     1,     1,     1,     2,     2,     2,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     4,     5,     4,     5,     3,
       4,     2,     5,     1,     1,     1,     1,     1,     1,     1,
       4,     1,     1,     4,     4,     7,     8,     3,     0,     0,
       8,     3,     3,     3,     0,     0,     8,     3,     4,     0,
       0,     9,     4,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     3,     2,     4,     1,     1,     4,     4,     4,
       4,     4,     1,     6,     7,     6,     6,     7,     7,     6,
       7,     6,     6,     0,     4,     1,     0,     1,     1,     0,
       1,     0,     1,     1,     0,     1,     5,     1,     1,     2,
       0,     0,     0,     5,     0,    10,     0,    11,     6,     3,
       3,     4,     1,     1,     3,     3,     3,     1,     3,     1,
       3,     0,     2,     3,     3,     1,     3,     0,     2,     3,
       1,     1,     1,     2,     3,     3,     5,     1,     1,     1,
       1,     1,     0,     1,     1,     4,     3,     3,     5,     4,
       6,     5,     5,     4,     4,     0,     0,     5,     0,     1,
       0,     1,     1,     6,     0,     6,     0,     3,     5,     0,
       1,     1,     0,     5,     2,     3,     4,     0,     4,     0,
       1,     1,     1,     7,     9,     0,     2,     0,     1,     3,
       1,     1,     2,     2,     0,     1,     1,     0,     3,     0,
       0,     7,     1,     4,     3,     3,     5,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     4,     1,     3,     1,     2,
       0,     3,     3,     2,     0,     0,     6,     1,     1,     1,
       3,     3,     4,     1,     1,     1,     1,     2,     3,     0,
       0,     6,     4,     5,     0,     9,     4,     2,     2,     3,
       2,     3,     2,     2,     3,     3,     3,     2,     0,     0,
       6,     2,     0,     0,     6,     0,     0,     6,     0,     0,
       6,     1,     0,     0,     6,     0,     0,     7,     1,     0,
       0,     6,     0,     0,     7,     1,     0,     0,     6,     0,
       0,     7,     0,     0,     6,     0,     0,     6,     1,     3,
       3,     0,     0,     0,     0,    10,     0,     0,     0,     0,
      10,     0,     0,     8,     1,     1,     1,     1,     1,     3,
       3,     5,     5,     6,     6,     8,     8,     0,     1,     2,
       1,     3,     3,     5,     1,     2,     1,     0,     0,     2,
       2,     1,     2,     1,     2,     1,     2,     1,     1,     2,
       1,     1,     0,     1,     5,     4,     6,     7,     5,     7,
       0,     0,    10,     0,     0,    10,     0,     0,    10,     0,
       0,     7,     1,     3,     3,     3,     1,     5,     0,     0,
      10,     1,     3,     3,     4,     4,     0,     0,    11,     0,
       0,    11,     0,     0,    10,     5,     0,     0,     9,     5,
       0,     0,    10,     1,     3,     1,     3,     3,     3,     4,
       7,     9,     0,     3,     0,     1,     9,    11,    12,    11,
      10,    10,    10,     9,    10
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
            { if ( ((*yyvaluep).pEnum)->use_count()==1 ) delete ((*yyvaluep).pEnum); }
        break;

    case YYSYMBOL_alias_list: /* alias_list  */
            { delete ((*yyvaluep).positions); }
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
        auto sc = make_smart<ExprConstString>(tokAt(scanner,(yylsp[0])),esconst);
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
            call_fmt->arguments.push_back(make_smart<ExprConstString>(tokAt(scanner,(yylsp[-1])),":" + *(yyvsp[-1].s)));
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
            auto sconst = static_pointer_cast<ExprConstString>(strb->elements[0]);
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
            yyextra->g_ReaderMacro = macros.back().get();
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

  case 54: /* require_module_name: require_module_name '.' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += ".";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 55: /* require_module_name: require_module_name '/' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += "/";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 56: /* require_module: require_module_name is_public_module  */
                                                         {
        ast_requireModule(scanner,(yyvsp[-1].s),nullptr,(yyvsp[0].b),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 57: /* require_module: require_module_name "as" "name" is_public_module  */
                                                                              {
        ast_requireModule(scanner,(yyvsp[-3].s),(yyvsp[-1].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 58: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 59: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 63: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 64: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 65: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 66: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 67: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 68: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 69: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 70: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 71: /* expression_else: "else" expression_block  */
                                                           { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 72: /* expression_else: elif_or_static_elif expr expression_block expression_else  */
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

  case 73: /* expression_else: elif_or_static_elif expr expression_block SEMICOLON expression_else  */
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

  case 74: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 75: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 76: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 77: /* $@3: %empty  */
                      { yyextra->das_need_oxford_comma = true; }
    break;

  case 78: /* expression_else_one_liner: "else" $@3 expression_if_one_liner  */
                                                                                                 {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 79: /* expression_if_one_liner: expr  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 80: /* expression_if_one_liner: expression_return_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 81: /* expression_if_one_liner: expression_yield_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 82: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 83: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 84: /* expression_if_then_else: if_or_static_if expr expression_block SEMICOLON expression_else  */
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

  case 85: /* expression_if_then_else: if_or_static_if expr expression_block expression_else  */
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

  case 86: /* $@4: %empty  */
                                                     { yyextra->das_need_oxford_comma = true; }
    break;

  case 87: /* expression_if_then_else: expression_if_one_liner "if" $@4 expr expression_else_one_liner semicolon  */
                                                                                                                                                         {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-2])), (yyvsp[-2].pExpression)->at);

        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-4])),(yyvsp[-2].pExpression),ast_wrapInBlock((yyvsp[-5].pExpression)),(yyvsp[-1].pExpression) ? ast_wrapInBlock((yyvsp[-1].pExpression)) : nullptr);
    }
    break;

  case 88: /* $@5: %empty  */
                     { yyextra->das_need_oxford_comma=false; }
    break;

  case 89: /* expression_for_loop: "for" $@5 variable_name_with_pos_list "in" expr_list expression_block  */
                                                                                                                                                 {
        format::wrap_par_expr(tokRangeAt(scanner, (yylsp[-3]), (yylsp[-1])), tokRangeAt(scanner, (yylsp[-3]), (yylsp[-1])));
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-3].pNameWithPosList),(yyvsp[-1].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 90: /* $@6: %empty  */
                     { // Had to add to successfully convert to v2 syntax, just copied from ds2_parser
                             yyextra->das_keyword = true;
     }
    break;

  case 91: /* expression_for_loop: "for" $@6 '(' variable_name_with_pos_list "in" expr_list ')' expression_block  */
                                                                                                  {
        yyextra->das_keyword = false;
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-4].pNameWithPosList),(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 92: /* expression_unsafe: "unsafe" expression_block  */
                                                 {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-1])));
        pUnsafe->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 93: /* expression_while_loop: "while" expr expression_block  */
                                                               {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-1])), (yyvsp[-1].pExpression)->at);

        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-2])));
        pWhile->cond = (yyvsp[-1].pExpression);
        pWhile->body = (yyvsp[0].pExpression);
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 94: /* expression_with: "with" expr expression_block  */
                                                         {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-1])), (yyvsp[-1].pExpression)->at);
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-2])));
        pWith->with = (yyvsp[-1].pExpression);
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 95: /* $@7: %empty  */
                                        { yyextra->das_need_oxford_comma=true; }
    break;

  case 96: /* expression_with_alias: "assume" "name" '=' $@7 expr semicolon  */
                                                                                                         {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-5])), *(yyvsp[-4].s), (yyvsp[-1].pExpression) );
        delete (yyvsp[-4].s);
    }
    break;

  case 97: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 98: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 99: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 100: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 101: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 102: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 103: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 104: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 105: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 106: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 107: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 108: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 109: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 110: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[-2].s); }
    break;

  case 111: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[-2].s); }
    break;

  case 112: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[-2].s); }
    break;

  case 113: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); delete (yyvsp[-2].s); }
    break;

  case 114: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 115: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                               {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokRangeAt(scanner,(yylsp[-4]),(yylsp[0]))); delete (yyvsp[-4].s); }
    }
    break;

  case 116: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 117: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 118: /* metadata_argument_list: '@' annotation_argument  */
                                      {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 119: /* metadata_argument_list: metadata_argument_list '@' annotation_argument  */
                                                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 120: /* metadata_argument_list: metadata_argument_list semicolon  */
                                               {
        (yyval.aaList) = (yyvsp[-1].aaList);
    }
    break;

  case 121: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 122: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 123: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 124: /* annotation_declaration_name: "template"  */
                                    { (yyval.s) = new string("template"); }
    break;

  case 125: /* annotation_declaration_basic: annotation_declaration_name  */
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

  case 126: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
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

  case 127: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 128: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 129: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
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

  case 130: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
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

  case 131: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
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

  case 132: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 133: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 134: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 135: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 136: /* optional_annotation_list: %empty  */
                                        { (yyval.faList) = nullptr; }
    break;

  case 137: /* optional_annotation_list: '[' annotation_list ']'  */
                                        { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 138: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 139: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 140: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 141: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 142: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 143: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 144: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 145: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 146: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 147: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 148: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 149: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 150: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 151: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 152: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 153: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 154: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 155: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 156: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 157: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 158: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 159: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 160: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 161: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 162: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 163: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 164: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 165: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 166: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 167: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 168: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 169: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 170: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 171: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 172: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 173: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 174: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 175: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 176: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 177: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 178: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 179: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 180: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 181: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 182: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 183: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 184: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 185: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 186: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 187: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 188: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 189: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 190: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 191: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 192: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 193: /* function_name: "operator" '.' "name" "+="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`+="); delete (yyvsp[-1].s); }
    break;

  case 194: /* function_name: "operator" '.' "name" "-="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`-="); delete (yyvsp[-1].s); }
    break;

  case 195: /* function_name: "operator" '.' "name" "*="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`*="); delete (yyvsp[-1].s); }
    break;

  case 196: /* function_name: "operator" '.' "name" "/="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`/="); delete (yyvsp[-1].s); }
    break;

  case 197: /* function_name: "operator" '.' "name" "%="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`%="); delete (yyvsp[-1].s); }
    break;

  case 198: /* function_name: "operator" '.' "name" "&="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&="); delete (yyvsp[-1].s); }
    break;

  case 199: /* function_name: "operator" '.' "name" "|="  */
                                          { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`|="); delete (yyvsp[-1].s); }
    break;

  case 200: /* function_name: "operator" '.' "name" "^="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^="); delete (yyvsp[-1].s); }
    break;

  case 201: /* function_name: "operator" '.' "name" "&&="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&&="); delete (yyvsp[-1].s); }
    break;

  case 202: /* function_name: "operator" '.' "name" "||="  */
                                            { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`||="); delete (yyvsp[-1].s); }
    break;

  case 203: /* function_name: "operator" '.' "name" "^^="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^^="); delete (yyvsp[-1].s); }
    break;

  case 204: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 205: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 206: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 207: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 208: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 209: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 210: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 211: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 212: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 213: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 214: /* function_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 215: /* function_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 216: /* function_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 217: /* function_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 218: /* function_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 219: /* function_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 220: /* function_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 221: /* function_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 222: /* function_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 223: /* function_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 224: /* function_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 225: /* function_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 226: /* function_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 227: /* function_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 228: /* function_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 229: /* function_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 230: /* function_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 231: /* function_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 232: /* function_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 233: /* function_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 234: /* function_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 235: /* function_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 236: /* function_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 237: /* function_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 238: /* function_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 239: /* global_function_declaration: optional_annotation_list "def" function_declaration  */
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

  case 240: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 241: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 242: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 243: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 244: /* $@8: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 245: /* function_declaration: optional_public_or_private_function $@8 function_declaration_header expression_block  */
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

  case 246: /* open_block: "begin of code block"  */
          { (yyval.ui) = 0xdeadbeef; }
    break;

  case 247: /* open_block: "new scope"  */
                        { (yyval.ui) = (yyvsp[0].i); }
    break;

  case 248: /* close_block: "end of code block"  */
          { (yyval.ui) = 0xdeadbeef; }
    break;

  case 249: /* close_block: "close scope"  */
                         { (yyval.ui) = (yyvsp[0].i); }
    break;

  case 250: /* expression_block: open_block expressions close_block  */
                                                                  {
        auto prev_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(prev_loc, (yyvsp[-2].ui), format::get_substring(prev_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size, format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
    }
    break;

  case 251: /* expression_block: open_block expressions close_block "finally" open_block expressions close_block  */
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

  case 252: /* expr_call_pipe: expr expr_full_block_assumed_piped  */
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

  case 253: /* expr_call_pipe: expression_keyword expr_full_block_assumed_piped  */
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

  case 254: /* expr_call_pipe: "generator" '<' type_declaration_no_options '>' optional_capture_list expr_full_block_assumed_piped  */
                                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-3].pTypeDecl),(yyvsp[-1].pCaptList),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])));
    }
    break;

  case 255: /* expression_any: SEMICOLON  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 256: /* expression_any: "end of expression"  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 257: /* expression_any: expr_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 258: /* expression_any: expr_keyword  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 259: /* expression_any: expr_assign_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 260: /* expression_any: expr_assign semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 261: /* expression_any: expression_delete semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 262: /* expression_any: expression_let  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 263: /* expression_any: expression_while_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 264: /* expression_any: expression_unsafe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 265: /* expression_any: expression_with  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 266: /* expression_any: expression_with_alias  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 267: /* expression_any: expression_for_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 268: /* expression_any: expression_break semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 269: /* expression_any: expression_continue semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 270: /* expression_any: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 271: /* expression_any: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 272: /* expression_any: expression_if_then_else  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 273: /* expression_any: expression_try_catch  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 274: /* expression_any: expression_label semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 275: /* expression_any: expression_goto semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 276: /* expression_any: "pass" semicolon  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 277: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 278: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 279: /* expressions: expressions error  */
                                 {
        delete (yyvsp[-1].pExpression); (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 280: /* expr_keyword: "keyword" expr expression_block  */
                                                           {
        format::wrap_par_expr(tokAt(scanner,(yylsp[-1])), (yyvsp[-1].pExpression)->at); // wrap match (expr)
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s));
        pCall->arguments.push_back((yyvsp[-1].pExpression));
        auto resT = new TypeDecl(Type::autoinfer);
        auto blk = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,resT,(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])));
        pCall->arguments.push_back(blk);
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 281: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 282: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 283: /* optional_expr_list_in_braces: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 284: /* optional_expr_list_in_braces: '(' optional_expr_list optional_comma ')'  */
                                                             { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 285: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 286: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 287: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 288: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 289: /* $@9: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 290: /* $@10: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 291: /* expression_keyword: "keyword" '<' $@9 type_declaration_no_options_list '>' $@10 expr  */
                                                                                                                                                     {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 292: /* $@11: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 293: /* $@12: %empty  */
                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 294: /* expression_keyword: "type function" '<' $@11 type_declaration_no_options_list '>' $@12 optional_expr_list_in_braces  */
                                                                                                                                                                                   {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 295: /* expr_pipe: expr_assign " <|" expr_block  */
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

  case 296: /* expr_pipe: "@ <|" expr_block  */
                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
            format::get_writer() << "@";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[-1]))));
        }

        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 297: /* expr_pipe: "@@ <|" expr_block  */
                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
            format::get_writer() << "@@";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[-1]))));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 298: /* expr_pipe: "$ <|" expr_block  */
                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[-1]))))) {
            format::get_writer() << "$";
            format::finish_rule(format::Pos::from_last(tokAt(scanner, (yylsp[-1]))));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 299: /* expr_pipe: expr_call_pipe  */
                             {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 300: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 301: /* name_in_namespace: "name" "::" "name"  */
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

  case 302: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 303: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 304: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 305: /* $@13: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 306: /* $@14: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 307: /* new_type_declaration: '<' $@13 type_declaration '>' $@14  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 308: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 309: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 310: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 311: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 312: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 313: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 314: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 315: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 316: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 317: /* expression_return_no_pipe: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 318: /* expression_return_no_pipe: "return" expr_list  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),sequenceToTuple((yyvsp[0].pExpression)));
    }
    break;

  case 319: /* expression_return_no_pipe: "return" "<-" expr_list  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),sequenceToTuple((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 320: /* expression_return: expression_return_no_pipe semicolon  */
                                                    {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 321: /* expression_return: "return" expr_pipe  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 322: /* expression_return: "return" "<-" expr_pipe  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 323: /* expression_yield_no_pipe: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 324: /* expression_yield_no_pipe: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 325: /* expression_yield: expression_yield_no_pipe semicolon  */
                                                   {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 326: /* expression_yield: "yield" expr_pipe  */
                                          {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 327: /* expression_yield: "yield" "<-" expr_pipe  */
                                                 {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 328: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                              {
        const auto end_block = format::Pos::from_last(tokAt(scanner, (yylsp[-2])));
        const auto start = format::Pos::from(tokAt(scanner, (yylsp[-3])));
        if (format::is_replace_braces()) {
            format::skip_spaces_or_print(tokAt(scanner, (yylsp[-3])), tokAt(scanner, (yylsp[-2])), tokAt(scanner, (yylsp[-1])), yyextra->das_tab_size);
        }

        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 329: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 330: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 331: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 332: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 333: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 334: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 335: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 336: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 337: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 338: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
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

  case 339: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 340: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 341: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 342: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 343: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 344: /* $@15: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 345: /* $@16: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 346: /* expr_cast: "cast" '<' $@15 type_declaration_no_options '>' $@16 expr  */
                                                                                                                                                {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
    }
    break;

  case 347: /* $@17: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 348: /* $@18: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 349: /* expr_cast: "upcast" '<' $@17 type_declaration_no_options '>' $@18 expr  */
                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 350: /* $@19: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 351: /* $@20: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 352: /* expr_cast: "reinterpret" '<' $@19 type_declaration_no_options '>' $@20 expr  */
                                                                                                                                                       {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 353: /* $@21: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 354: /* $@22: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 355: /* expr_type_decl: "type" '<' $@21 type_declaration '>' $@22  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 356: /* expr_type_info: "typeinfo" '(' name_in_namespace expr ')'  */
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

  case 357: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" '>' expr ')'  */
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

  case 358: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" c_or_s "name" '>' expr ')'  */
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

  case 359: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
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

  case 360: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
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

  case 361: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" semicolon "name" '>' '(' expr ')'  */
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

  case 362: /* expr_list: expr2  */
                       {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 363: /* expr_list: expr_list ',' expr2  */
                                             {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 364: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 365: /* block_or_simple_block: "=>" expr  */
                                        {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 366: /* block_or_simple_block: "=>" "<-" expr  */
                                               {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 367: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 368: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 369: /* block_or_lambda: '@' '@'  */
                { (yyval.i) = 2;   /* local function */ }
    break;

  case 370: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 371: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 372: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 373: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 374: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 375: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 376: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 377: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 378: /* optional_capture_list: "[[" capture_list ']' ']'  */
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

  case 379: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 380: /* expr_block: expression_block  */
                                            {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))))) {
            format::get_writer() << "$() ";
            format::finish_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))));
        }

        ExprBlock * closure = (ExprBlock *) (yyvsp[0].pExpression);
        (yyval.pExpression) = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),(yyvsp[0].pExpression));
        closure->returnType = make_smart<TypeDecl>(Type::autoinfer);
    }
    break;

  case 381: /* expr_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 382: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 383: /* $@23: %empty  */
                             {  yyextra->das_need_oxford_comma = false; }
    break;

  case 384: /* expr_full_block_assumed_piped: block_or_lambda $@23 optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type expression_block  */
                                                                                       {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 385: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 386: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 387: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 388: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 389: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 390: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 391: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 392: /* expr_assign: expr  */
                                             { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 393: /* expr_assign: expr '=' expr  */
                                             { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 394: /* expr_assign: expr "<-" expr  */
                                             { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 395: /* expr_assign: expr ":=" expr  */
                                             { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 396: /* expr_assign: expr "&=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 397: /* expr_assign: expr "|=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 398: /* expr_assign: expr "^=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 399: /* expr_assign: expr "&&=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 400: /* expr_assign: expr "||=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 401: /* expr_assign: expr "^^=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 402: /* expr_assign: expr "+=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 403: /* expr_assign: expr "-=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 404: /* expr_assign: expr "*=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 405: /* expr_assign: expr "/=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 406: /* expr_assign: expr "%=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 407: /* expr_assign: expr "<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 408: /* expr_assign: expr ">>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 409: /* expr_assign: expr "<<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 410: /* expr_assign: expr ">>>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 411: /* expr_assign_pipe_right: "@ <|" expr_block  */
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

  case 412: /* expr_assign_pipe_right: "@@ <|" expr_block  */
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

  case 413: /* expr_assign_pipe_right: "$ <|" expr_block  */
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

  case 414: /* expr_assign_pipe_right: expr_call_pipe  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 415: /* expr_assign_pipe: expr '=' expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 416: /* expr_assign_pipe: expr "<-" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 417: /* expr_assign_pipe: expr "&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 418: /* expr_assign_pipe: expr "|=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 419: /* expr_assign_pipe: expr "^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 420: /* expr_assign_pipe: expr "&&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 421: /* expr_assign_pipe: expr "||=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 422: /* expr_assign_pipe: expr "^^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 423: /* expr_assign_pipe: expr "+=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 424: /* expr_assign_pipe: expr "-=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 425: /* expr_assign_pipe: expr "*=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 426: /* expr_assign_pipe: expr "/=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 427: /* expr_assign_pipe: expr "%=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 428: /* expr_assign_pipe: expr "<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 429: /* expr_assign_pipe: expr ">>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 430: /* expr_assign_pipe: expr "<<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 431: /* expr_assign_pipe: expr ">>>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 432: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 433: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 434: /* expr_method_call: expr2 "->" "name" '(' ')'  */
                                                          {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 435: /* expr_method_call: expr2 "->" "name" '(' expr_list ')'  */
                                                                               {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 436: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 437: /* func_addr_name: "$i" '(' expr2 ')'  */
                                           {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 438: /* func_addr_expr: '@' '@' func_addr_name  */
                                          {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 439: /* $@24: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 440: /* $@25: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 441: /* func_addr_expr: '@' '@' '<' $@24 type_declaration_no_options '>' $@25 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value.get()));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 442: /* $@26: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 443: /* $@27: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 444: /* func_addr_expr: '@' '@' '<' $@26 optional_function_argument_list optional_function_type '>' $@27 func_addr_name  */
                                                                                                                                                                                     {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value.get()));
        expr->funcType = make_smart<TypeDecl>(Type::tFunction);
        expr->funcType->firstType = (yyvsp[-3].pTypeDecl);
        if ( (yyvsp[-4].pVarDeclList) ) {
            varDeclToTypeDecl(scanner, expr->funcType.get(), (yyvsp[-4].pVarDeclList));
            deleteVariableDeclarationList((yyvsp[-4].pVarDeclList));
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 445: /* expr_field: expr2 '.' "name"  */
                                               {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 446: /* expr_field: expr2 '.' '.' "name"  */
                                                   {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 447: /* expr_field: expr2 '.' "name" '(' ')'  */
                                                       {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 448: /* expr_field: expr2 '.' "name" '(' expr_list ')'  */
                                                                            {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 449: /* expr_field: expr2 '.' "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                        {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 450: /* expr_field: expr2 '.' basic_type_declaration '(' ')'  */
                                                                         {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 451: /* expr_field: expr2 '.' basic_type_declaration '(' expr_list ')'  */
                                                                                              {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 452: /* $@28: %empty  */
                                { yyextra->das_suppress_errors=true; }
    break;

  case 453: /* $@29: %empty  */
                                                                             { yyextra->das_suppress_errors=false; }
    break;

  case 454: /* expr_field: expr2 '.' $@28 error $@29  */
                                                                                                                     {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), "");
        yyerrok;
    }
    break;

  case 455: /* expr_call: name_in_namespace '(' ')'  */
                                               {
            (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),*(yyvsp[-2].s));
            delete (yyvsp[-2].s);
    }
    break;

  case 456: /* expr_call: name_in_namespace '(' "uninitialized" ')'  */
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

  case 457: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
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

  case 458: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
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

  case 459: /* expr_call: name_in_namespace '(' expr_list ')'  */
                                                                    {
            (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),*(yyvsp[-3].s)),(yyvsp[-1].pExpression));
            delete (yyvsp[-3].s);
    }
    break;

  case 460: /* expr_call: basic_type_declaration '(' ')'  */
                                                    {
        (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-2].type)));
    }
    break;

  case 461: /* expr_call: basic_type_declaration '(' expr_list ')'  */
                                                                         {
        (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-3].type))),(yyvsp[-1].pExpression));
    }
    break;

  case 462: /* expr2: name_in_namespace  */
                                              { need_wrap_current_expr = true; (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 463: /* expr2: expr_field  */
                                              { need_wrap_current_expr = true; (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0])); (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 464: /* expr2: expr_mtag  */
                                              { need_wrap_current_expr = true; (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0])); (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 465: /* expr2: '!' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"!",(yyvsp[0].pExpression)); }
    break;

  case 466: /* expr2: '~' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"~",(yyvsp[0].pExpression)); }
    break;

  case 467: /* expr2: '+' expr2  */
                                                   { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"+",(yyvsp[0].pExpression)); }
    break;

  case 468: /* expr2: '-' expr2  */
                                                   { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"-",(yyvsp[0].pExpression)); }
    break;

  case 469: /* expr2: expr2 "<<" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 470: /* expr2: expr2 ">>" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 471: /* expr2: expr2 "<<<" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 472: /* expr2: expr2 ">>>" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 473: /* expr2: expr2 '+' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 474: /* expr2: expr2 '-' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 475: /* expr2: expr2 '*' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 476: /* expr2: expr2 '/' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 477: /* expr2: expr2 '%' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 478: /* expr2: expr2 '<' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 479: /* expr2: expr2 '>' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 480: /* expr2: expr2 "==" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 481: /* expr2: expr2 "!=" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 482: /* expr2: expr2 "<=" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 483: /* expr2: expr2 ">=" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 484: /* expr2: expr2 '&' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 485: /* expr2: expr2 '|' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 486: /* expr2: expr2 '^' expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 487: /* expr2: expr2 "&&" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 488: /* expr2: expr2 "||" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 489: /* expr2: expr2 "^^" expr2  */
                                               { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp2(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 490: /* expr2: expr2 ".." expr2  */
                                               {
        need_wrap_current_expr = true;
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 491: /* expr2: "++" expr2  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"++", (yyvsp[0].pExpression)); }
    break;

  case 492: /* expr2: "--" expr2  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"--", (yyvsp[0].pExpression)); }
    break;

  case 493: /* expr2: expr2 "++"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 494: /* expr2: expr2 "--"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp1(tokRangeAt(scanner,(yylsp[-1]), (yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 495: /* expr2: expr2 '[' expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprAt(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 496: /* expr2: expr2 '.' '[' expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprAt(tokRangeAt(scanner,(yylsp[-4]), (yylsp[0])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 497: /* expr2: expr2 "?[" expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeAt(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 498: /* expr2: expr2 '.' "?[" expr2 ']'  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeAt(tokRangeAt(scanner,(yylsp[-4]), (yylsp[0])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 499: /* expr2: expr2 "?." "name"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeField(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 500: /* expr2: expr2 '.' "?." "name"  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprSafeField(tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 501: /* expr2: '*' expr2  */
                                                            { need_wrap_current_expr = true; (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 502: /* expr2: expr2 '?' expr2 ':' expr2  */
                                                             {
        need_wrap_current_expr = true; (yyval.pExpression) = new ExprOp3(tokRangeAt(scanner,(yylsp[-4]), (yylsp[0])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 503: /* expr2: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 504: /* expr2: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 505: /* expr2: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 506: /* expr2: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 507: /* expr2: make_decl  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 508: /* expr2: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 509: /* expr2: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 510: /* expr2: '(' expr_list optional_comma ')'  */
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

  case 511: /* expr2: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 512: /* expr2: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 513: /* expr2: "deref" '(' expr2 ')'  */
                                                    { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 514: /* expr2: "addr" '(' expr2 ')'  */
                                                    { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 515: /* expr2: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])));
    }
    break;

  case 516: /* expr2: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr2 ')'  */
                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])));
    }
    break;

  case 517: /* expr2: expr2 "??" expr2  */
                                                     { (yyval.pExpression) = new ExprNullCoalescing(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 518: /* $@30: %empty  */
                                                { yyextra->das_arrow_depth ++; }
    break;

  case 519: /* $@31: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 520: /* expr2: expr2 "is" "type" '<' $@30 type_declaration_no_options '>' $@31  */
                                                                                                                                                             {
        (yyval.pExpression) = new ExprIs(tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 521: /* expr2: expr2 "is" basic_type_declaration  */
                                                                {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 522: /* expr2: expr2 "is" "name"  */
                                               {
        (yyval.pExpression) = new ExprIsVariant(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 523: /* expr2: expr2 "as" "name"  */
                                               {
        (yyval.pExpression) = new ExprAsVariant(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 524: /* $@32: %empty  */
                                                { yyextra->das_arrow_depth ++; }
    break;

  case 525: /* $@33: %empty  */
                                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 526: /* expr2: expr2 "as" "type" '<' $@32 type_declaration '>' $@33  */
                                                                                                                                                  {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 527: /* expr2: expr2 "as" basic_type_declaration  */
                                                                {
        (yyval.pExpression) = new ExprAsVariant(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 528: /* expr2: expr2 '?' "as" "name"  */
                                                   {
        (yyval.pExpression) = new ExprSafeAsVariant(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 529: /* $@34: %empty  */
                                                    { yyextra->das_arrow_depth ++; }
    break;

  case 530: /* $@35: %empty  */
                                                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 531: /* expr2: expr2 '?' "as" "type" '<' $@34 type_declaration '>' $@35  */
                                                                                                                                                      {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokRangeAt(scanner,(yylsp[-8]), (yylsp[-1])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 532: /* expr2: expr2 '?' "as" basic_type_declaration  */
                                                                    {
        (yyval.pExpression) = new ExprSafeAsVariant(tokRangeAt(scanner,(yylsp[-3]), (yylsp[0])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 533: /* expr2: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 534: /* expr2: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 535: /* expr2: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 536: /* expr2: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 537: /* expr2: expr_method_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); (yyval.pExpression)->at = tokAt(scanner, (yylsp[0])); }
    break;

  case 538: /* expr2: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 539: /* expr2: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 540: /* expr2: expr2 "<|" expr2  */
                                                  { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]))); }
    break;

  case 541: /* expr2: expr2 "|>" expr2  */
                                                  { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-2]), (yylsp[0]))); (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])); }
    break;

  case 542: /* expr2: expr2 "|>" basic_type_declaration  */
                                                           {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])));
    }
    break;

  case 543: /* expr2: name_in_namespace "name"  */
                                                  {
        if (format::prepare_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-1]))))) {
            format::get_writer() << "." << format::get_substring(tokAt(scanner,(yylsp[0])));
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        }

        (yyval.pExpression) = ast_NameName(scanner,(yyvsp[-1].s),(yyvsp[0].s),tokRangeAt(scanner,(yylsp[-1]),(yylsp[0])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 544: /* expr2: "unsafe" '(' expr2 ')'  */
                                          {
        (yyvsp[-1].pExpression)->alwaysSafe = true;
        (yyvsp[-1].pExpression)->userSaidItsSafe = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 545: /* expr2: expression_keyword  */
                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 546: /* expr: expr2  */
                                       {
        if (need_wrap_current_expr) {
            format::wrap_par_expr_newline(tokAt(scanner,(yylsp[0])), (yyvsp[0].pExpression)->at);
            need_wrap_current_expr = false;
        }
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 547: /* expr_mtag: "$$" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 548: /* expr_mtag: "$i" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 549: /* expr_mtag: "$v" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 550: /* expr_mtag: "$b" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 551: /* expr_mtag: "$a" '(' expr2 ')'  */
                                                      { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 552: /* expr_mtag: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 553: /* expr_mtag: "$c" '(' expr2 ')' '(' ')'  */
                                                             {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 554: /* expr_mtag: "$c" '(' expr2 ')' '(' expr_list ')'  */
                                                                                 {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 555: /* expr_mtag: expr2 '.' "$f" '(' expr2 ')'  */
                                                                  {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 556: /* expr_mtag: expr2 "?." "$f" '(' expr2 ')'  */
                                                                   {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 557: /* expr_mtag: expr2 '.' '.' "$f" '(' expr2 ')'  */
                                                                      {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 558: /* expr_mtag: expr2 '.' "?." "$f" '(' expr2 ')'  */
                                                                       {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 559: /* expr_mtag: expr2 "as" "$f" '(' expr2 ')'  */
                                                                     {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 560: /* expr_mtag: expr2 '?' "as" "$f" '(' expr2 ')'  */
                                                                         {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 561: /* expr_mtag: expr2 "is" "$f" '(' expr2 ')'  */
                                                                     {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 562: /* expr_mtag: '@' '@' "$c" '(' expr2 ')'  */
                                                          {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 563: /* optional_field_annotation: %empty  */
                                                        { (yyval.aaList) = nullptr; }
    break;

  case 564: /* optional_field_annotation: "[[" annotation_argument_list ']' ']'  */
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

  case 565: /* optional_field_annotation: metadata_argument_list  */
                                                        { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 566: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 567: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 568: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 569: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 570: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 571: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 572: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 573: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 574: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 575: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 576: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 577: /* opt_sem: SEMICOLON  */
                { (yyval.b) = false; }
    break;

  case 578: /* opt_sem: "end of expression"  */
          { (yyval.b) = true; }
    break;

  case 579: /* opt_sem: "end of expression" SEMICOLON  */
                    { (yyval.b) = true; }
    break;

  case 580: /* opt_sem: %empty  */
                  {(yyval.b) = false; }
    break;

  case 581: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 582: /* $@36: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 583: /* struct_variable_declaration_list: struct_variable_declaration_list $@36 structure_variable_declaration semicolon opt_sem  */
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

  case 584: /* $@37: %empty  */
                                                                                                                     {
                yyextra->das_force_oxford_comma=true;
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 585: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable "abstract" optional_constant $@37 function_declaration_header semicolon opt_sem  */
                                                                  {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure->isTemplate;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-9].pVarDeclList),(yyvsp[-8].faList),(yyvsp[-6].b),(yyvsp[-4].b), (yyvsp[-2].pFuncDecl));
            }
    break;

  case 586: /* $@38: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 587: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@38 function_declaration_header expression_block opt_sem  */
                                                                                {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure->isTemplate;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-10].pVarDeclList),(yyvsp[-9].faList),(yyvsp[-6].b),(yyvsp[-7].b),(yyvsp[-5].i),(yyvsp[-4].b),(yyvsp[-2].pFuncDecl),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-8]),(yylsp[-1])),tokAt(scanner,(yylsp[-9])));
            }
    break;

  case 588: /* struct_variable_declaration_list: struct_variable_declaration_list '[' annotation_list ']' semicolon opt_sem  */
                                                                                               {
        das_yyerror(scanner,"structure field or class method annotation expected to remain on the same line with the field or the class",
            tokAt(scanner,(yylsp[-3])), CompilationError::syntax_error);
        delete (yyvsp[-3].faList);
        (yyval.pVarDeclList) = (yyvsp[-5].pVarDeclList);
    }
    break;

  case 589: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 590: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 591: /* function_argument_declaration_type: "$a" '(' expr2 ')'  */
                                      {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 592: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 593: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 594: /* function_argument_list: function_argument_declaration_no_type "end of expression" function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 595: /* function_argument_list: function_argument_declaration_type "end of expression" function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 596: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 597: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 598: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 599: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 600: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                          { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 601: /* tuple_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 602: /* tuple_alias_type_list: tuple_alias_type_list c_or_s  */
                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 603: /* tuple_alias_type_list: tuple_alias_type_list tuple_type c_or_s  */
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

  case 604: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 605: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 606: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 607: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 608: /* variant_alias_type_list: variant_alias_type_list c_or_s  */
                                           {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 609: /* variant_alias_type_list: variant_alias_type_list variant_type c_or_s  */
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

  case 610: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 611: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 612: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 613: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 614: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 615: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 616: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 617: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 618: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 619: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 620: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 621: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 622: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 623: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 624: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 625: /* let_variable_name_with_pos_list: "$i" '(' expr2 ')'  */
                                      {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 626: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 627: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 628: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 629: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options semicolon  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 630: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 631: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 632: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr semicolon  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 633: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr  */
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

  case 634: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr_pipe  */
                                                                                                           {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-3]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 635: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 636: /* $@39: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 637: /* global_variable_declaration_list: global_variable_declaration_list $@39 optional_field_annotation let_variable_declaration opt_sem  */
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

  case 638: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 639: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 640: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 641: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 642: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 643: /* global_let: kwd_let optional_shared optional_public_or_private_variable open_block global_variable_declaration_list close_block  */
                                                                                                                                                                   {
        handle_brace(format::Pos::from(tokAt(scanner, (yylsp[-2]))), (yyvsp[-2].ui),
                     format::get_substring(tokRangeAt(scanner, (yylsp[-2]), (yylsp[0]))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[0]))));
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 644: /* $@40: %empty  */
                                                                                        {
        yyextra->das_force_oxford_comma=true;
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 645: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@40 optional_field_annotation let_variable_declaration  */
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

  case 646: /* enum_list: %empty  */
        {
        (yyval.pEnum) = new Enumeration();
    }
    break;

  case 647: /* enum_list: enum_list "name" opt_sem  */
                                              {
        format::skip_token(true, false, tokAt(scanner,(yylsp[0])));
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

  case 648: /* enum_list: enum_list "name" '=' expr opt_sem  */
                                                              {
        format::skip_token(true, false, tokAt(scanner,(yylsp[0])));
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        if ( !(yyvsp[-4].pEnum)->add(*(yyvsp[-3].s),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-3]))) ) {
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

  case 649: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 650: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 651: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 652: /* $@41: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 653: /* single_alias: optional_public_or_private_alias "name" $@41 '=' type_declaration  */
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

  case 654: /* alias_list: single_alias opt_sem  */
                                    {
        (yyval.positions) = new vector<LineInfo>(1, tokAt(scanner, (yylsp[-1])));
    }
    break;

  case 655: /* alias_list: alias_list single_alias opt_sem  */
                                                       {
        (yyvsp[-2].positions)->emplace_back(tokAt(scanner, (yylsp[-1])));
        (yyval.positions) = (yyvsp[-2].positions);
    }
    break;

  case 656: /* alias_declaration: "typedef" open_block alias_list close_block  */
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

  case 657: /* $@42: %empty  */
                    { yyextra->das_force_oxford_comma=true;}
    break;

  case 659: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 660: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 661: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 662: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 663: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name open_block enum_list close_block  */
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
        ast_enumDeclaration(scanner,(yyvsp[-6].faList),tokAt(scanner,(yylsp[-6])),(yyvsp[-4].b),(yyvsp[-3].pEnum),(yyvsp[-1].pEnum),Type::tInt);
    }
    break;

  case 664: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name ':' enum_basic_type_declaration open_block enum_list close_block  */
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
        ast_enumDeclaration(scanner,(yyvsp[-8].faList),tokAt(scanner,(yylsp[-8])),(yyvsp[-6].b),(yyvsp[-5].pEnum),(yyvsp[-1].pEnum),(yyvsp[-3].type));
    }
    break;

  case 665: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 666: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 667: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 668: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 669: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 670: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 671: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 672: /* class_or_struct: "template" "class"  */
                                 { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 673: /* class_or_struct: "template" "struct"  */
                                 { (yyval.i) = CorS_StructTemplate; }
    break;

  case 674: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 675: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 676: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 677: /* optional_struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 678: /* optional_struct_variable_declaration_list: open_block struct_variable_declaration_list close_block  */
                                                                                      {
        const auto prev_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(prev_loc, (yyvsp[-2].ui),
                     format::get_substring(prev_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 679: /* $@43: %empty  */
                                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 680: /* $@44: %empty  */
                         {
        if ( (yyvsp[0].pStructure) ) {
            (yyvsp[0].pStructure)->isClass = (yyvsp[-3].i)==CorS_Class || (yyvsp[-3].i)==CorS_ClassTemplate;
            (yyvsp[0].pStructure)->isTemplate = (yyvsp[-3].i)==CorS_ClassTemplate || (yyvsp[-3].i)==CorS_StructTemplate;
            (yyvsp[0].pStructure)->privateStructure = !(yyvsp[-2].b);
        }
    }
    break;

  case 681: /* structure_declaration: optional_annotation_list class_or_struct optional_public_or_private_structure $@43 structure_name $@44 optional_struct_variable_declaration_list  */
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

  case 682: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 683: /* variable_name_with_pos_list: "$i" '(' expr2 ')'  */
                                      {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 684: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 685: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 686: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 687: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 688: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 689: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 690: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 691: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 692: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 693: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 694: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 695: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 696: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 697: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 698: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 699: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 700: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 701: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 702: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 703: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 704: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 705: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 706: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 707: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 708: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 709: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 710: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 711: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 712: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 713: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 714: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 715: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 716: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 717: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 718: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 719: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 720: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 721: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 722: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 723: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 724: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 725: /* auto_type_declaration: "$t" '(' expr2 ')'  */
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

  case 726: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 727: /* bitfield_bits: bitfield_bits semicolon "name"  */
                                                 {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 730: /* bitfield_alias_bits: %empty  */
              {
        auto pSL = new vector<string>();
        (yyval.pNameList) = pSL;

    }
    break;

  case 731: /* bitfield_alias_bits: bitfield_alias_bits "name" SEMICOLON  */
                                                       {
        if (format::enum_bitfield_with_comma() && format::is_replace_braces() && format::prepare_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-1]))))) {
            format::get_writer() << ",";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        }

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

  case 732: /* bitfield_alias_bits: bitfield_alias_bits "name" commas  */
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

  case 733: /* bitfield_alias_bits: bitfield_alias_bits "name"  */
                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-1].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-1].pNameList);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[0].s)->c_str(),atvname);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 734: /* $@45: %empty  */
                                     { yyextra->das_arrow_depth ++; }
    break;

  case 735: /* $@46: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 736: /* bitfield_type_declaration: "bitfield" '<' $@45 bitfield_bits '>' $@46  */
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

  case 739: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 740: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 741: /* dim_list: '[' expr2 ']'  */
                              {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 742: /* dim_list: dim_list '[' expr2 ']'  */
                                             {
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 743: /* type_declaration_no_options: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 744: /* type_declaration_no_options: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 745: /* type_declaration_no_options: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 746: /* type_declaration_no_options: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 747: /* type_declaration_no_options: type_declaration_no_options dim_list  */
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

  case 748: /* type_declaration_no_options: type_declaration_no_options '[' ']'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->dim.push_back(TypeDecl::dimAuto);
        (yyvsp[-2].pTypeDecl)->dimExpr.push_back(nullptr);
        (yyvsp[-2].pTypeDecl)->removeDim = false;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 749: /* $@47: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 750: /* $@48: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 751: /* type_declaration_no_options: "type" '<' $@47 type_declaration '>' $@48  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 752: /* type_declaration_no_options: "typedecl" '(' expr2 ')'  */
                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 753: /* type_declaration_no_options: '$' name_in_namespace '(' optional_expr_list ')'  */
                                                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-3])), *(yyvsp[-3].s)));
        delete (yyvsp[-3].s);
    }
    break;

  case 754: /* $@49: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 755: /* type_declaration_no_options: '$' name_in_namespace '<' $@49 type_declaration_no_options_list '>' '(' optional_expr_list ')'  */
                                                                                                                                                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-4].pTypeDeclList),(yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-7])), *(yyvsp[-7].s)));
        delete (yyvsp[-7].s);
    }
    break;

  case 756: /* type_declaration_no_options: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 757: /* type_declaration_no_options: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 758: /* type_declaration_no_options: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 759: /* type_declaration_no_options: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 760: /* type_declaration_no_options: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 761: /* type_declaration_no_options: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 762: /* type_declaration_no_options: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 763: /* type_declaration_no_options: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 764: /* type_declaration_no_options: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 765: /* type_declaration_no_options: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 766: /* type_declaration_no_options: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 767: /* type_declaration_no_options: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 768: /* $@50: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 769: /* $@51: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 770: /* type_declaration_no_options: "smart_ptr" '<' $@50 type_declaration '>' $@51  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 771: /* type_declaration_no_options: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 772: /* $@52: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 773: /* $@53: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 774: /* type_declaration_no_options: "array" '<' $@52 type_declaration '>' $@53  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 775: /* $@54: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 776: /* $@55: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 777: /* type_declaration_no_options: "table" '<' $@54 table_type_pair '>' $@55  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 778: /* $@56: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 779: /* $@57: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 780: /* type_declaration_no_options: "iterator" '<' $@56 type_declaration '>' $@57  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 781: /* type_declaration_no_options: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 782: /* $@58: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 783: /* $@59: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 784: /* type_declaration_no_options: "block" '<' $@58 type_declaration '>' $@59  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 785: /* $@60: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 786: /* $@61: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 787: /* type_declaration_no_options: "block" '<' $@60 optional_function_argument_list optional_function_type '>' $@61  */
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

  case 788: /* type_declaration_no_options: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 789: /* $@62: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 790: /* $@63: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 791: /* type_declaration_no_options: "function" '<' $@62 type_declaration '>' $@63  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 792: /* $@64: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 793: /* $@65: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 794: /* type_declaration_no_options: "function" '<' $@64 optional_function_argument_list optional_function_type '>' $@65  */
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

  case 795: /* type_declaration_no_options: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 796: /* $@66: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 797: /* $@67: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 798: /* type_declaration_no_options: "lambda" '<' $@66 type_declaration '>' $@67  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 799: /* $@68: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 800: /* $@69: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 801: /* type_declaration_no_options: "lambda" '<' $@68 optional_function_argument_list optional_function_type '>' $@69  */
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

  case 802: /* $@70: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 803: /* $@71: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 804: /* type_declaration_no_options: "tuple" '<' $@70 tuple_type_list '>' $@71  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 805: /* $@72: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 806: /* $@73: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 807: /* type_declaration_no_options: "variant" '<' $@72 variant_type_list '>' $@73  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 808: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 809: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 810: /* type_declaration: type_declaration '|' '#'  */
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

  case 811: /* $@74: %empty  */
                                                          { yyextra->das_need_oxford_comma=false; }
    break;

  case 812: /* $@75: %empty  */
                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 813: /* $@76: %empty  */
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

  case 814: /* $@77: %empty  */
                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
    }
    break;

  case 815: /* tuple_alias_declaration: "tuple" optional_public_or_private_alias $@74 "name" $@75 open_block $@76 tuple_alias_type_list $@77 close_block  */
                         {
        if (format::is_replace_braces() && (yyvsp[-4].ui) != 0xdeadbeef && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))))) {
            format::get_writer() << "\n" << string((yyvsp[0].ui) * yyextra->das_tab_size, ' ') + "}";
            format::finish_rule(format::Pos::from(tokAt(scanner,(yylsp[0]))));
        }
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

  case 816: /* $@78: %empty  */
                                                            { yyextra->das_need_oxford_comma=false; }
    break;

  case 817: /* $@79: %empty  */
                                                                                                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 818: /* $@80: %empty  */
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

  case 819: /* $@81: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
    }
    break;

  case 820: /* variant_alias_declaration: "variant" optional_public_or_private_alias $@78 "name" $@79 open_block $@80 variant_alias_type_list $@81 close_block  */
                         {
        if (format::is_replace_braces() && (yyvsp[0].ui) != 0xdeadbeef && format::prepare_rule(format::Pos::from(tokAt(scanner, (yylsp[0]))))) {
            format::get_writer() << "\n" << string((yyvsp[0].ui) * yyextra->das_tab_size, ' ') + "}";
            format::finish_rule(format::Pos::from(tokAt(scanner,(yylsp[0]))));
        }
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

  case 821: /* $@82: %empty  */
                                                             { yyextra->das_need_oxford_comma=false; }
    break;

  case 822: /* $@83: %empty  */
                                                                                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 823: /* bitfield_alias_declaration: "bitfield" optional_public_or_private_alias $@82 "name" $@83 open_block bitfield_alias_bits close_block  */
                                                                   {
        const auto prev_loc = format::Pos::from(tokAt(scanner,(yylsp[-2])));
        handle_brace(prev_loc, (yyvsp[-2].ui),
                     format::get_substring(prev_loc, format::Pos::from_last(tokAt(scanner,(yylsp[-1])))),
                     yyextra->das_tab_size,
                     format::Pos::from_last(tokAt(scanner,(yylsp[-1]))));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
        auto btype = make_smart<TypeDecl>(Type::tBitfield);
        btype->alias = *(yyvsp[-4].s);
        btype->at = tokAt(scanner,(yylsp[-4]));
        btype->argNames = *(yyvsp[-1].pNameList);
        btype->isPrivateAlias = !(yyvsp[-6].b);
        if ( btype->argNames.size()>32 ) {
            das_yyerror(scanner,"only 32 different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-4])),
                CompilationError::invalid_type);
        }
        if ( !yyextra->g_Program->addAlias(btype) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-4].s),tokAt(scanner,(yylsp[-4])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfield((yyvsp[-4].s)->c_str(),atvname);
        }
        delete (yyvsp[-4].s);
        delete (yyvsp[-1].pNameList);
    }
    break;

  case 824: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 825: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 826: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 827: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 828: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 829: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 830: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 831: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = make_smart<MakeFieldDecl>(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 832: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = make_smart<MakeFieldDecl>(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 833: /* make_struct_fields: "$f" '(' expr2 ')' copy_or_move expr  */
                                                                            {
        auto mfd = make_smart<MakeFieldDecl>(tokRangeAt(scanner,(yylsp[-5]), (yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 834: /* make_struct_fields: "$f" '(' expr2 ')' ":=" expr  */
                                                                   {
        auto mfd = make_smart<MakeFieldDecl>(tokRangeAt(scanner, (yylsp[-5]), (yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 835: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr2 ')' copy_or_move expr  */
                                                                                                        {
        auto mfd = make_smart<MakeFieldDecl>(tokRangeAt(scanner,(yylsp[-5]),(yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 836: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr2 ')' ":=" expr  */
                                                                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokRangeAt(scanner,(yylsp[-5]), (yylsp[0])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 837: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 838: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 839: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 840: /* make_struct_dim: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 841: /* make_struct_dim: make_struct_dim semicolon make_struct_fields  */
                                                               {
        ((ExprMakeStruct *) (yyvsp[-2].pExpression))->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 842: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 843: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 844: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 845: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 846: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 847: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 848: /* optional_block: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 849: /* optional_block: "where" expr_block  */
                                  { (yyvsp[0].pExpression)->at = tokAt(scanner, (yylsp[0])); (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 862: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 863: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 864: /* make_struct_decl: "[[" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
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

  case 865: /* make_struct_decl: "[[" type_declaration_no_options optional_block optional_trailing_delim_sqr_sqr  */
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

  case 866: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' optional_block optional_trailing_delim_sqr_sqr  */
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

  case 867: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
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

  case 868: /* make_struct_decl: "[{" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
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

  case 869: /* make_struct_decl: "[{" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
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

  case 870: /* $@84: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 871: /* $@85: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 872: /* make_struct_decl: "struct" '<' $@84 type_declaration_no_options '>' $@85 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                            {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 873: /* $@86: %empty  */
                            { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 874: /* $@87: %empty  */
                                                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 875: /* make_struct_decl: "class" '<' $@86 type_declaration_no_options '>' $@87 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                          {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 876: /* $@88: %empty  */
                               { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 877: /* $@89: %empty  */
                                                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 878: /* make_struct_decl: "variant" '<' $@88 variant_type_list '>' $@89 '(' use_initializer make_variant_dim ')'  */
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

  case 879: /* $@90: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 880: /* $@91: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 881: /* make_struct_decl: "default" '<' $@90 type_declaration_no_options '>' $@91 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 882: /* make_tuple: expr  */
                  {
        (yyvsp[0].pExpression)->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 883: /* make_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokRangeAt(scanner,(yylsp[-2]), (yylsp[0])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 884: /* make_tuple: make_tuple ',' expr  */
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

  case 885: /* make_map_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 886: /* make_map_tuple: expr  */
                 {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 887: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = make_smart<TypeDecl>(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 888: /* $@92: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 889: /* $@93: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 890: /* make_tuple_call: "tuple" '<' $@92 tuple_type_list '>' $@93 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 891: /* make_dim: make_tuple  */
                        {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 892: /* make_dim: make_dim semicolon make_tuple  */
                                                {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 893: /* make_dim_decl: '[' optional_expr_list ']'  */
                                                    {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-2])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
            mka->gen2 = true;
            auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_array_move");
            tam->arguments.push_back(mka);
            (yyval.pExpression) = tam;
        } else {
            auto mks = new ExprMakeStruct();
            mks->at = tokAt(scanner,(yylsp[-2]));
            mks->makeType = make_smart<TypeDecl>(Type::tArray);
            mks->makeType->firstType = make_smart<TypeDecl>(Type::autoinfer);
            mks->useInitializer = true;
            mks->alwaysUseInitializer = true;
            (yyval.pExpression) = mks;
        }

    }
    break;

  case 894: /* make_dim_decl: "[[" type_declaration_no_options make_dim optional_trailing_semicolon_sqr_sqr  */
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

  case 895: /* make_dim_decl: "[{" type_declaration_no_options make_dim optional_trailing_semicolon_cur_sqr  */
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

  case 896: /* $@94: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 897: /* $@95: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 898: /* make_dim_decl: "array" "struct" '<' $@94 type_declaration_no_options '>' $@95 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 899: /* $@96: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 900: /* $@97: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 901: /* make_dim_decl: "array" "tuple" '<' $@96 tuple_type_list '>' $@97 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 902: /* $@98: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 903: /* $@99: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 904: /* make_dim_decl: "array" "variant" '<' $@98 variant_type_list '>' $@99 '(' make_variant_dim ')'  */
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

  case 905: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
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

  case 906: /* $@100: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 907: /* $@101: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 908: /* make_dim_decl: "array" '<' $@100 type_declaration_no_options '>' $@101 '(' optional_expr_list ')'  */
                                                                                                                                                                        {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-8])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = (yyvsp[-5].pTypeDecl);
            mka->gen2 = true;
            auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-8])),"to_array_move");
            tam->arguments.push_back(mka);
            (yyval.pExpression) = tam;
        } else {
            auto msd = new ExprMakeStruct();
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->makeType = make_smart<TypeDecl>(Type::tArray);
            msd->makeType->firstType = (yyvsp[-5].pTypeDecl);
            msd->at = tokAt(scanner,(yylsp[-5]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 909: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 910: /* $@102: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 911: /* $@103: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 912: /* make_dim_decl: "fixed_array" '<' $@102 type_declaration_no_options '>' $@103 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 913: /* make_table: make_map_tuple  */
                            {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 914: /* make_table: make_table semicolon make_map_tuple  */
                                                      {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 915: /* expr_map_tuple_list: make_map_tuple  */
                                {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 916: /* expr_map_tuple_list: expr_map_tuple_list ',' make_map_tuple  */
                                                                {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 917: /* make_table_decl: open_block optional_expr_map_tuple_list close_block  */
                                                                              {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-2])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto mks = new ExprMakeStruct();
            mks->at = tokAt(scanner,(yylsp[-2]));
            mks->makeType = make_smart<TypeDecl>(Type::tTable);
            mks->makeType->firstType = make_smart<TypeDecl>(Type::autoinfer);
            mks->makeType->secondType = make_smart<TypeDecl>(Type::autoinfer);
            mks->useInitializer = true;
            mks->alwaysUseInitializer = true;
            (yyval.pExpression) = mks;
        }

    }
    break;

  case 918: /* make_table_decl: "{{" make_table optional_trailing_semicolon_cur_cur  */
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

        auto mkt = make_smart<TypeDecl>(Type::autoinfer);
        mkt->dim.push_back(TypeDecl::dimAuto);
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = mkt;
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-2]));
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_table_move");
        ttm->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = ttm;
    }
    break;

  case 919: /* make_table_decl: "table" '(' optional_expr_map_tuple_list ')'  */
                                                                       {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-1].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 920: /* make_table_decl: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
                                                                                                                 {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-6])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = (yyvsp[-4].pTypeDecl);
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto msd = new ExprMakeStruct();
            msd->at = tokAt(scanner,(yylsp[-6]));
            msd->makeType = make_smart<TypeDecl>(Type::tTable);
            msd->makeType->firstType = (yyvsp[-4].pTypeDecl);
            msd->makeType->secondType = make_smart<TypeDecl>(Type::tVoid);
            msd->at = tokAt(scanner,(yylsp[-6]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 921: /* make_table_decl: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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
            msd->makeType->firstType = (yyvsp[-6].pTypeDecl);
            msd->makeType->secondType = (yyvsp[-4].pTypeDecl);
            msd->at = tokAt(scanner,(yylsp[-8]));
            msd->useInitializer = true;
            msd->alwaysUseInitializer = true;
            (yyval.pExpression) = msd;
        }
    }
    break;

  case 922: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 923: /* array_comprehension_where: semicolon "where" expr  */
                                          { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 924: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 925: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 926: /* array_comprehension: '[' "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                     {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-6]))))) {
            format::get_writer() << "(" << format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-6]))),
                                                                 format::Pos::from_last(tokAt(scanner,(yylsp[-4])))) << ")";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-4]))));
        }

        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 927: /* array_comprehension: '[' "for" '(' variable_name_with_pos_list "in" expr_list ')' "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 928: /* array_comprehension: '[' "iterator" "for" '(' variable_name_with_pos_list "in" expr_list ')' "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                                         {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 929: /* array_comprehension: "begin of code block" "for" '(' variable_name_with_pos_list "in" expr_list ')' "end of expression" make_map_tuple array_comprehension_where "end of code block"  */
                                                                                                                                                                     {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
    }
    break;

  case 930: /* array_comprehension: '[' "iterator" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where ']'  */
                                                                                                                                                                  {
        if (format::is_replace_braces() && format::prepare_rule(format::Pos::from(tokAt(scanner,(yylsp[-6]))))) {
            format::get_writer() << "(" << format::get_substring(format::Pos::from(tokAt(scanner,(yylsp[-6]))),
                                                                 format::Pos::from_last(tokAt(scanner,(yylsp[-4])))) << ")";
            format::finish_rule(format::Pos::from_last(tokAt(scanner,(yylsp[-4]))));
        }
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 931: /* array_comprehension: "[[" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where ']' ']'  */
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

  case 932: /* array_comprehension: "[{" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr2 array_comprehension_where "end of code block" ']'  */
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

  case 933: /* array_comprehension: open_block "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where close_block  */
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

  case 934: /* array_comprehension: "{{" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where close_block close_block  */
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


