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
  YYSYMBOL_SEMICOLON = 165,                /* "end of line"  */
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
  YYSYMBOL_191_ = 191,                     /* ','  */
  YYSYMBOL_192_ = 192,                     /* '='  */
  YYSYMBOL_193_ = 193,                     /* '?'  */
  YYSYMBOL_194_ = 194,                     /* ':'  */
  YYSYMBOL_195_ = 195,                     /* '|'  */
  YYSYMBOL_196_ = 196,                     /* '^'  */
  YYSYMBOL_197_ = 197,                     /* '&'  */
  YYSYMBOL_198_ = 198,                     /* '<'  */
  YYSYMBOL_199_ = 199,                     /* '>'  */
  YYSYMBOL_200_ = 200,                     /* '-'  */
  YYSYMBOL_201_ = 201,                     /* '+'  */
  YYSYMBOL_202_ = 202,                     /* '*'  */
  YYSYMBOL_203_ = 203,                     /* '/'  */
  YYSYMBOL_204_ = 204,                     /* '%'  */
  YYSYMBOL_UNARY_MINUS = 205,              /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 206,               /* UNARY_PLUS  */
  YYSYMBOL_207_ = 207,                     /* '~'  */
  YYSYMBOL_208_ = 208,                     /* '!'  */
  YYSYMBOL_PRE_INC = 209,                  /* PRE_INC  */
  YYSYMBOL_PRE_DEC = 210,                  /* PRE_DEC  */
  YYSYMBOL_POST_INC = 211,                 /* POST_INC  */
  YYSYMBOL_POST_DEC = 212,                 /* POST_DEC  */
  YYSYMBOL_DEREF = 213,                    /* DEREF  */
  YYSYMBOL_214_ = 214,                     /* '.'  */
  YYSYMBOL_215_ = 215,                     /* '['  */
  YYSYMBOL_216_ = 216,                     /* ']'  */
  YYSYMBOL_217_ = 217,                     /* '('  */
  YYSYMBOL_218_ = 218,                     /* ')'  */
  YYSYMBOL_219_ = 219,                     /* '$'  */
  YYSYMBOL_220_ = 220,                     /* '@'  */
  YYSYMBOL_221_ = 221,                     /* '#'  */
  YYSYMBOL_YYACCEPT = 222,                 /* $accept  */
  YYSYMBOL_program = 223,                  /* program  */
  YYSYMBOL_top_level_reader_macro = 224,   /* top_level_reader_macro  */
  YYSYMBOL_optional_public_or_private_module = 225, /* optional_public_or_private_module  */
  YYSYMBOL_module_name = 226,              /* module_name  */
  YYSYMBOL_optional_not_required = 227,    /* optional_not_required  */
  YYSYMBOL_module_declaration = 228,       /* module_declaration  */
  YYSYMBOL_character_sequence = 229,       /* character_sequence  */
  YYSYMBOL_string_constant = 230,          /* string_constant  */
  YYSYMBOL_format_string = 231,            /* format_string  */
  YYSYMBOL_optional_format_string = 232,   /* optional_format_string  */
  YYSYMBOL_233_1 = 233,                    /* $@1  */
  YYSYMBOL_string_builder_body = 234,      /* string_builder_body  */
  YYSYMBOL_string_builder = 235,           /* string_builder  */
  YYSYMBOL_reader_character_sequence = 236, /* reader_character_sequence  */
  YYSYMBOL_expr_reader = 237,              /* expr_reader  */
  YYSYMBOL_238_2 = 238,                    /* $@2  */
  YYSYMBOL_options_declaration = 239,      /* options_declaration  */
  YYSYMBOL_require_declaration = 240,      /* require_declaration  */
  YYSYMBOL_keyword_or_name = 241,          /* keyword_or_name  */
  YYSYMBOL_require_module_name = 242,      /* require_module_name  */
  YYSYMBOL_require_module = 243,           /* require_module  */
  YYSYMBOL_is_public_module = 244,         /* is_public_module  */
  YYSYMBOL_expect_declaration = 245,       /* expect_declaration  */
  YYSYMBOL_expect_list = 246,              /* expect_list  */
  YYSYMBOL_expect_error = 247,             /* expect_error  */
  YYSYMBOL_expression_label = 248,         /* expression_label  */
  YYSYMBOL_expression_goto = 249,          /* expression_goto  */
  YYSYMBOL_elif_or_static_elif = 250,      /* elif_or_static_elif  */
  YYSYMBOL_expression_else = 251,          /* expression_else  */
  YYSYMBOL_semicolon = 252,                /* semicolon  */
  YYSYMBOL_if_or_static_if = 253,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 254, /* expression_else_one_liner  */
  YYSYMBOL_255_3 = 255,                    /* $@3  */
  YYSYMBOL_expression_if_one_liner = 256,  /* expression_if_one_liner  */
  YYSYMBOL_expression_if_then_else = 257,  /* expression_if_then_else  */
  YYSYMBOL_258_4 = 258,                    /* $@4  */
  YYSYMBOL_expression_for_loop = 259,      /* expression_for_loop  */
  YYSYMBOL_260_5 = 260,                    /* $@5  */
  YYSYMBOL_expression_unsafe = 261,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 262,    /* expression_while_loop  */
  YYSYMBOL_expression_with = 263,          /* expression_with  */
  YYSYMBOL_expression_with_alias = 264,    /* expression_with_alias  */
  YYSYMBOL_265_6 = 265,                    /* $@6  */
  YYSYMBOL_annotation_argument_value = 266, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 267, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 268, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 269,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 270, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 271,   /* metadata_argument_list  */
  YYSYMBOL_annotation_declaration_name = 272, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 273, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 274,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 275,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 276, /* optional_annotation_list  */
  YYSYMBOL_optional_function_argument_list = 277, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 278,   /* optional_function_type  */
  YYSYMBOL_function_name = 279,            /* function_name  */
  YYSYMBOL_global_function_declaration = 280, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 281, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 282, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 283,     /* function_declaration  */
  YYSYMBOL_284_7 = 284,                    /* $@7  */
  YYSYMBOL_open_block = 285,               /* open_block  */
  YYSYMBOL_close_block = 286,              /* close_block  */
  YYSYMBOL_expression_block = 287,         /* expression_block  */
  YYSYMBOL_expr_call_pipe = 288,           /* expr_call_pipe  */
  YYSYMBOL_expression_any = 289,           /* expression_any  */
  YYSYMBOL_expressions = 290,              /* expressions  */
  YYSYMBOL_expr_keyword = 291,             /* expr_keyword  */
  YYSYMBOL_optional_expr_list = 292,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_list_in_braces = 293, /* optional_expr_list_in_braces  */
  YYSYMBOL_optional_expr_map_tuple_list = 294, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 295, /* type_declaration_no_options_list  */
  YYSYMBOL_expression_keyword = 296,       /* expression_keyword  */
  YYSYMBOL_297_8 = 297,                    /* $@8  */
  YYSYMBOL_298_9 = 298,                    /* $@9  */
  YYSYMBOL_299_10 = 299,                   /* $@10  */
  YYSYMBOL_300_11 = 300,                   /* $@11  */
  YYSYMBOL_expr_pipe = 301,                /* expr_pipe  */
  YYSYMBOL_name_in_namespace = 302,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 303,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 304,     /* new_type_declaration  */
  YYSYMBOL_305_12 = 305,                   /* $@12  */
  YYSYMBOL_306_13 = 306,                   /* $@13  */
  YYSYMBOL_expr_new = 307,                 /* expr_new  */
  YYSYMBOL_expression_break = 308,         /* expression_break  */
  YYSYMBOL_expression_continue = 309,      /* expression_continue  */
  YYSYMBOL_expression_return_no_pipe = 310, /* expression_return_no_pipe  */
  YYSYMBOL_expression_return = 311,        /* expression_return  */
  YYSYMBOL_expression_yield_no_pipe = 312, /* expression_yield_no_pipe  */
  YYSYMBOL_expression_yield = 313,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 314,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 315,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 316,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 317,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 318,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 319, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 320,           /* expression_let  */
  YYSYMBOL_expr_cast = 321,                /* expr_cast  */
  YYSYMBOL_322_14 = 322,                   /* $@14  */
  YYSYMBOL_323_15 = 323,                   /* $@15  */
  YYSYMBOL_324_16 = 324,                   /* $@16  */
  YYSYMBOL_325_17 = 325,                   /* $@17  */
  YYSYMBOL_326_18 = 326,                   /* $@18  */
  YYSYMBOL_327_19 = 327,                   /* $@19  */
  YYSYMBOL_expr_type_decl = 328,           /* expr_type_decl  */
  YYSYMBOL_329_20 = 329,                   /* $@20  */
  YYSYMBOL_330_21 = 330,                   /* $@21  */
  YYSYMBOL_expr_type_info = 331,           /* expr_type_info  */
  YYSYMBOL_expr_list = 332,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 333,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 334,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 335,            /* capture_entry  */
  YYSYMBOL_capture_list = 336,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 337,    /* optional_capture_list  */
  YYSYMBOL_expr_block = 338,               /* expr_block  */
  YYSYMBOL_expr_full_block = 339,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 340, /* expr_full_block_assumed_piped  */
  YYSYMBOL_341_22 = 341,                   /* $@22  */
  YYSYMBOL_expr_numeric_const = 342,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign = 343,              /* expr_assign  */
  YYSYMBOL_expr_assign_pipe_right = 344,   /* expr_assign_pipe_right  */
  YYSYMBOL_expr_assign_pipe = 345,         /* expr_assign_pipe  */
  YYSYMBOL_expr_named_call = 346,          /* expr_named_call  */
  YYSYMBOL_expr_method_call = 347,         /* expr_method_call  */
  YYSYMBOL_func_addr_name = 348,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 349,           /* func_addr_expr  */
  YYSYMBOL_350_23 = 350,                   /* $@23  */
  YYSYMBOL_351_24 = 351,                   /* $@24  */
  YYSYMBOL_352_25 = 352,                   /* $@25  */
  YYSYMBOL_353_26 = 353,                   /* $@26  */
  YYSYMBOL_expr_field = 354,               /* expr_field  */
  YYSYMBOL_355_27 = 355,                   /* $@27  */
  YYSYMBOL_356_28 = 356,                   /* $@28  */
  YYSYMBOL_expr_call = 357,                /* expr_call  */
  YYSYMBOL_expr = 358,                     /* expr  */
  YYSYMBOL_359_29 = 359,                   /* $@29  */
  YYSYMBOL_360_30 = 360,                   /* $@30  */
  YYSYMBOL_361_31 = 361,                   /* $@31  */
  YYSYMBOL_362_32 = 362,                   /* $@32  */
  YYSYMBOL_363_33 = 363,                   /* $@33  */
  YYSYMBOL_364_34 = 364,                   /* $@34  */
  YYSYMBOL_expr_mtag = 365,                /* expr_mtag  */
  YYSYMBOL_optional_field_annotation = 366, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 367,        /* optional_override  */
  YYSYMBOL_optional_constant = 368,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 369, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 370, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 371, /* structure_variable_declaration  */
  YYSYMBOL_struct_variable_declaration_list = 372, /* struct_variable_declaration_list  */
  YYSYMBOL_373_35 = 373,                   /* $@35  */
  YYSYMBOL_374_36 = 374,                   /* $@36  */
  YYSYMBOL_375_37 = 375,                   /* $@37  */
  YYSYMBOL_function_argument_declaration_no_type = 376, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 377, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 378,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 379,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 380,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 381,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 382,             /* variant_type  */
  YYSYMBOL_variant_type_list = 383,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 384,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 385,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 386, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 387, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 388,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 389,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 390,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 391, /* let_variable_name_with_pos_list  */
  YYSYMBOL_let_variable_declaration = 392, /* let_variable_declaration  */
  YYSYMBOL_global_variable_declaration_list = 393, /* global_variable_declaration_list  */
  YYSYMBOL_394_38 = 394,                   /* $@38  */
  YYSYMBOL_optional_shared = 395,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 396, /* optional_public_or_private_variable  */
  YYSYMBOL_global_let = 397,               /* global_let  */
  YYSYMBOL_398_39 = 398,                   /* $@39  */
  YYSYMBOL_enum_list = 399,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 400, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 401,             /* single_alias  */
  YYSYMBOL_402_40 = 402,                   /* $@40  */
  YYSYMBOL_alias_list = 403,               /* alias_list  */
  YYSYMBOL_alias_declaration = 404,        /* alias_declaration  */
  YYSYMBOL_405_41 = 405,                   /* $@41  */
  YYSYMBOL_optional_public_or_private_enum = 406, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 407,                /* enum_name  */
  YYSYMBOL_enum_declaration = 408,         /* enum_declaration  */
  YYSYMBOL_409_42 = 409,                   /* $@42  */
  YYSYMBOL_410_43 = 410,                   /* $@43  */
  YYSYMBOL_411_44 = 411,                   /* $@44  */
  YYSYMBOL_412_45 = 412,                   /* $@45  */
  YYSYMBOL_optional_structure_parent = 413, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 414,          /* optional_sealed  */
  YYSYMBOL_structure_name = 415,           /* structure_name  */
  YYSYMBOL_class_or_struct = 416,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 417, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 418, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 419,    /* structure_declaration  */
  YYSYMBOL_420_46 = 420,                   /* $@46  */
  YYSYMBOL_421_47 = 421,                   /* $@47  */
  YYSYMBOL_variable_name_with_pos_list = 422, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 423,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 424, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 425, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 426,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 427,            /* bitfield_bits  */
  YYSYMBOL_bitfield_alias_bits = 428,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_type_declaration = 429, /* bitfield_type_declaration  */
  YYSYMBOL_430_48 = 430,                   /* $@48  */
  YYSYMBOL_431_49 = 431,                   /* $@49  */
  YYSYMBOL_c_or_s = 432,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 433,          /* table_type_pair  */
  YYSYMBOL_dim_list = 434,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 435, /* type_declaration_no_options  */
  YYSYMBOL_436_50 = 436,                   /* $@50  */
  YYSYMBOL_437_51 = 437,                   /* $@51  */
  YYSYMBOL_438_52 = 438,                   /* $@52  */
  YYSYMBOL_439_53 = 439,                   /* $@53  */
  YYSYMBOL_440_54 = 440,                   /* $@54  */
  YYSYMBOL_441_55 = 441,                   /* $@55  */
  YYSYMBOL_442_56 = 442,                   /* $@56  */
  YYSYMBOL_443_57 = 443,                   /* $@57  */
  YYSYMBOL_444_58 = 444,                   /* $@58  */
  YYSYMBOL_445_59 = 445,                   /* $@59  */
  YYSYMBOL_446_60 = 446,                   /* $@60  */
  YYSYMBOL_447_61 = 447,                   /* $@61  */
  YYSYMBOL_448_62 = 448,                   /* $@62  */
  YYSYMBOL_449_63 = 449,                   /* $@63  */
  YYSYMBOL_450_64 = 450,                   /* $@64  */
  YYSYMBOL_451_65 = 451,                   /* $@65  */
  YYSYMBOL_452_66 = 452,                   /* $@66  */
  YYSYMBOL_453_67 = 453,                   /* $@67  */
  YYSYMBOL_454_68 = 454,                   /* $@68  */
  YYSYMBOL_455_69 = 455,                   /* $@69  */
  YYSYMBOL_456_70 = 456,                   /* $@70  */
  YYSYMBOL_457_71 = 457,                   /* $@71  */
  YYSYMBOL_458_72 = 458,                   /* $@72  */
  YYSYMBOL_459_73 = 459,                   /* $@73  */
  YYSYMBOL_460_74 = 460,                   /* $@74  */
  YYSYMBOL_461_75 = 461,                   /* $@75  */
  YYSYMBOL_462_76 = 462,                   /* $@76  */
  YYSYMBOL_type_declaration = 463,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 464,  /* tuple_alias_declaration  */
  YYSYMBOL_465_77 = 465,                   /* $@77  */
  YYSYMBOL_466_78 = 466,                   /* $@78  */
  YYSYMBOL_467_79 = 467,                   /* $@79  */
  YYSYMBOL_468_80 = 468,                   /* $@80  */
  YYSYMBOL_variant_alias_declaration = 469, /* variant_alias_declaration  */
  YYSYMBOL_470_81 = 470,                   /* $@81  */
  YYSYMBOL_471_82 = 471,                   /* $@82  */
  YYSYMBOL_472_83 = 472,                   /* $@83  */
  YYSYMBOL_473_84 = 473,                   /* $@84  */
  YYSYMBOL_bitfield_alias_declaration = 474, /* bitfield_alias_declaration  */
  YYSYMBOL_475_85 = 475,                   /* $@85  */
  YYSYMBOL_476_86 = 476,                   /* $@86  */
  YYSYMBOL_477_87 = 477,                   /* $@87  */
  YYSYMBOL_478_88 = 478,                   /* $@88  */
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
  YYSYMBOL_495_89 = 495,                   /* $@89  */
  YYSYMBOL_496_90 = 496,                   /* $@90  */
  YYSYMBOL_497_91 = 497,                   /* $@91  */
  YYSYMBOL_498_92 = 498,                   /* $@92  */
  YYSYMBOL_499_93 = 499,                   /* $@93  */
  YYSYMBOL_500_94 = 500,                   /* $@94  */
  YYSYMBOL_501_95 = 501,                   /* $@95  */
  YYSYMBOL_502_96 = 502,                   /* $@96  */
  YYSYMBOL_make_tuple = 503,               /* make_tuple  */
  YYSYMBOL_make_map_tuple = 504,           /* make_map_tuple  */
  YYSYMBOL_make_tuple_call = 505,          /* make_tuple_call  */
  YYSYMBOL_506_97 = 506,                   /* $@97  */
  YYSYMBOL_507_98 = 507,                   /* $@98  */
  YYSYMBOL_make_dim = 508,                 /* make_dim  */
  YYSYMBOL_make_dim_decl = 509,            /* make_dim_decl  */
  YYSYMBOL_510_99 = 510,                   /* $@99  */
  YYSYMBOL_511_100 = 511,                  /* $@100  */
  YYSYMBOL_512_101 = 512,                  /* $@101  */
  YYSYMBOL_513_102 = 513,                  /* $@102  */
  YYSYMBOL_514_103 = 514,                  /* $@103  */
  YYSYMBOL_515_104 = 515,                  /* $@104  */
  YYSYMBOL_516_105 = 516,                  /* $@105  */
  YYSYMBOL_517_106 = 517,                  /* $@106  */
  YYSYMBOL_518_107 = 518,                  /* $@107  */
  YYSYMBOL_519_108 = 519,                  /* $@108  */
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
#define YYLAST   14882

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  222
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  304
/* YYNRULES -- Number of rules.  */
#define YYNRULES  931
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1739

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   449


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
       2,     2,     2,   208,     2,   221,   219,   204,   197,     2,
     217,   218,   202,   201,   191,   200,   214,   203,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   194,   185,
     198,   192,   199,   193,   220,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   215,     2,   216,   196,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   183,   195,   184,   207,     2,     2,     2,
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
     188,   189,   190,   205,   206,   209,   210,   211,   212,   213
};

#if DAS_YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   562,   562,   563,   568,   569,   570,   571,   572,   573,
     574,   575,   576,   577,   578,   579,   580,   584,   590,   591,
     592,   596,   597,   601,   602,   606,   625,   626,   627,   628,
     632,   633,   637,   638,   642,   643,   643,   647,   652,   661,
     676,   692,   697,   705,   705,   744,   774,   778,   779,   780,
     784,   787,   791,   797,   806,   809,   815,   816,   820,   824,
     825,   829,   832,   838,   844,   847,   853,   854,   858,   859,
     860,   869,   870,   874,   875,   879,   880,   880,   886,   887,
     888,   889,   890,   894,   900,   900,   906,   906,   912,   920,
     930,   939,   939,   946,   947,   948,   949,   950,   951,   955,
     960,   968,   969,   970,   974,   975,   976,   977,   978,   979,
     980,   981,   987,   990,   996,   999,  1002,  1008,  1009,  1010,
    1011,  1015,  1028,  1046,  1049,  1057,  1068,  1079,  1090,  1093,
    1100,  1104,  1111,  1112,  1116,  1117,  1118,  1122,  1125,  1132,
    1136,  1137,  1138,  1139,  1140,  1141,  1142,  1143,  1144,  1145,
    1146,  1147,  1148,  1149,  1150,  1151,  1152,  1153,  1154,  1155,
    1156,  1157,  1158,  1159,  1160,  1161,  1162,  1163,  1164,  1165,
    1166,  1167,  1168,  1169,  1170,  1171,  1172,  1173,  1174,  1175,
    1176,  1177,  1178,  1179,  1180,  1181,  1182,  1183,  1184,  1185,
    1186,  1187,  1188,  1189,  1190,  1191,  1192,  1193,  1194,  1195,
    1196,  1197,  1198,  1199,  1200,  1201,  1202,  1203,  1204,  1205,
    1206,  1207,  1208,  1209,  1210,  1211,  1212,  1213,  1214,  1215,
    1216,  1217,  1218,  1219,  1220,  1221,  1222,  1223,  1224,  1225,
    1226,  1227,  1228,  1229,  1230,  1235,  1253,  1254,  1255,  1259,
    1265,  1265,  1283,  1284,  1287,  1288,  1291,  1295,  1306,  1315,
    1324,  1330,  1331,  1332,  1333,  1334,  1335,  1336,  1337,  1338,
    1339,  1340,  1341,  1342,  1343,  1344,  1345,  1346,  1347,  1348,
    1349,  1350,  1354,  1359,  1365,  1371,  1382,  1383,  1387,  1388,
    1392,  1393,  1397,  1401,  1408,  1408,  1408,  1414,  1414,  1414,
    1423,  1457,  1460,  1463,  1466,  1472,  1473,  1484,  1488,  1491,
    1499,  1499,  1499,  1502,  1508,  1511,  1515,  1519,  1526,  1533,
    1539,  1543,  1547,  1550,  1553,  1561,  1564,  1567,  1575,  1578,
    1586,  1589,  1592,  1600,  1606,  1607,  1608,  1612,  1613,  1617,
    1618,  1622,  1627,  1635,  1641,  1647,  1653,  1659,  1668,  1677,
    1686,  1698,  1701,  1707,  1707,  1707,  1710,  1710,  1710,  1715,
    1715,  1715,  1723,  1723,  1723,  1729,  1739,  1750,  1763,  1773,
    1784,  1799,  1802,  1808,  1809,  1816,  1827,  1828,  1829,  1833,
    1834,  1835,  1836,  1837,  1841,  1846,  1854,  1855,  1856,  1860,
    1865,  1872,  1879,  1879,  1888,  1889,  1890,  1891,  1892,  1893,
    1894,  1898,  1899,  1900,  1901,  1902,  1903,  1904,  1905,  1906,
    1907,  1908,  1909,  1910,  1911,  1912,  1913,  1914,  1915,  1916,
    1920,  1921,  1922,  1923,  1928,  1929,  1930,  1931,  1932,  1933,
    1934,  1935,  1936,  1937,  1938,  1939,  1940,  1941,  1942,  1943,
    1944,  1949,  1956,  1968,  1973,  1983,  1987,  1994,  1997,  1997,
    1997,  2002,  2002,  2002,  2015,  2019,  2023,  2028,  2035,  2044,
    2049,  2056,  2056,  2056,  2063,  2067,  2076,  2084,  2092,  2096,
    2099,  2105,  2106,  2107,  2108,  2109,  2110,  2111,  2112,  2113,
    2114,  2115,  2116,  2117,  2118,  2119,  2120,  2121,  2122,  2123,
    2124,  2125,  2126,  2127,  2128,  2129,  2130,  2131,  2132,  2133,
    2134,  2135,  2136,  2137,  2138,  2139,  2140,  2146,  2147,  2148,
    2149,  2150,  2163,  2172,  2173,  2174,  2175,  2176,  2177,  2178,
    2179,  2180,  2181,  2182,  2183,  2186,  2189,  2190,  2193,  2193,
    2193,  2196,  2201,  2205,  2209,  2209,  2209,  2214,  2217,  2221,
    2221,  2221,  2226,  2229,  2230,  2231,  2232,  2233,  2234,  2235,
    2236,  2237,  2239,  2243,  2244,  2249,  2253,  2254,  2255,  2256,
    2257,  2258,  2259,  2263,  2267,  2271,  2275,  2279,  2283,  2287,
    2291,  2295,  2302,  2303,  2304,  2308,  2309,  2310,  2314,  2315,
    2319,  2320,  2321,  2325,  2326,  2330,  2341,  2344,  2347,  2347,
    2366,  2365,  2381,  2380,  2394,  2403,  2415,  2424,  2434,  2435,
    2436,  2437,  2438,  2442,  2445,  2454,  2455,  2459,  2462,  2465,
    2481,  2490,  2491,  2495,  2498,  2501,  2515,  2516,  2520,  2526,
    2532,  2541,  2544,  2551,  2554,  2560,  2561,  2562,  2566,  2567,
    2571,  2578,  2583,  2592,  2598,  2609,  2612,  2617,  2622,  2630,
    2641,  2644,  2647,  2647,  2667,  2668,  2672,  2673,  2674,  2678,
    2681,  2681,  2700,  2703,  2706,  2721,  2740,  2741,  2742,  2747,
    2747,  2773,  2774,  2778,  2779,  2779,  2783,  2784,  2785,  2789,
    2799,  2804,  2799,  2816,  2821,  2816,  2836,  2837,  2841,  2842,
    2846,  2852,  2853,  2854,  2855,  2859,  2860,  2861,  2865,  2868,
    2874,  2879,  2874,  2899,  2906,  2911,  2920,  2926,  2937,  2938,
    2939,  2940,  2941,  2942,  2943,  2944,  2945,  2946,  2947,  2948,
    2949,  2950,  2951,  2952,  2953,  2954,  2955,  2956,  2957,  2958,
    2959,  2960,  2961,  2962,  2963,  2967,  2968,  2969,  2970,  2971,
    2972,  2973,  2974,  2978,  2989,  2993,  3000,  3012,  3019,  3028,
    3033,  3036,  3049,  3049,  3049,  3062,  3063,  3067,  3071,  3078,
    3082,  3089,  3090,  3091,  3092,  3093,  3108,  3114,  3114,  3114,
    3118,  3123,  3130,  3130,  3137,  3141,  3145,  3150,  3155,  3160,
    3165,  3169,  3173,  3178,  3182,  3186,  3191,  3191,  3191,  3197,
    3204,  3204,  3204,  3209,  3209,  3209,  3215,  3215,  3215,  3220,
    3225,  3225,  3225,  3230,  3230,  3230,  3239,  3244,  3244,  3244,
    3249,  3249,  3249,  3258,  3263,  3263,  3263,  3268,  3268,  3268,
    3277,  3277,  3277,  3283,  3283,  3283,  3292,  3295,  3306,  3322,
    3322,  3327,  3332,  3322,  3357,  3357,  3362,  3368,  3357,  3393,
    3393,  3398,  3403,  3393,  3433,  3434,  3435,  3436,  3437,  3441,
    3448,  3455,  3461,  3467,  3474,  3481,  3487,  3496,  3499,  3505,
    3513,  3518,  3525,  3530,  3537,  3542,  3548,  3549,  3553,  3554,
    3559,  3560,  3564,  3565,  3569,  3570,  3574,  3575,  3576,  3580,
    3581,  3582,  3586,  3587,  3591,  3597,  3604,  3612,  3619,  3627,
    3636,  3636,  3636,  3644,  3644,  3644,  3651,  3651,  3651,  3662,
    3662,  3662,  3673,  3676,  3682,  3696,  3702,  3708,  3714,  3714,
    3714,  3728,  3733,  3740,  3759,  3764,  3771,  3771,  3771,  3781,
    3781,  3781,  3795,  3795,  3795,  3809,  3818,  3818,  3818,  3838,
    3845,  3845,  3845,  3855,  3860,  3867,  3870,  3876,  3895,  3904,
    3912,  3932,  3957,  3958,  3962,  3963,  3968,  3971,  3974,  3977,
    3980,  3983
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
  "\"[{\"", "\"{{\"", "\"new scope\"", "\"close scope\"",
  "\"end of line\"", "\"integer constant\"", "\"long integer constant\"",
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
  "expression_with_alias", "$@6", "annotation_argument_value",
  "annotation_argument_value_list", "annotation_argument_name",
  "annotation_argument", "annotation_argument_list",
  "metadata_argument_list", "annotation_declaration_name",
  "annotation_declaration_basic", "annotation_declaration",
  "annotation_list", "optional_annotation_list",
  "optional_function_argument_list", "optional_function_type",
  "function_name", "global_function_declaration",
  "optional_public_or_private_function", "function_declaration_header",
  "function_declaration", "$@7", "open_block", "close_block",
  "expression_block", "expr_call_pipe", "expression_any", "expressions",
  "expr_keyword", "optional_expr_list", "optional_expr_list_in_braces",
  "optional_expr_map_tuple_list", "type_declaration_no_options_list",
  "expression_keyword", "$@8", "$@9", "$@10", "$@11", "expr_pipe",
  "name_in_namespace", "expression_delete", "new_type_declaration", "$@12",
  "$@13", "expr_new", "expression_break", "expression_continue",
  "expression_return_no_pipe", "expression_return",
  "expression_yield_no_pipe", "expression_yield", "expression_try_catch",
  "kwd_let_var_or_nothing", "kwd_let", "optional_in_scope",
  "tuple_expansion", "tuple_expansion_variable_declaration",
  "expression_let", "expr_cast", "$@14", "$@15", "$@16", "$@17", "$@18",
  "$@19", "expr_type_decl", "$@20", "$@21", "expr_type_info", "expr_list",
  "block_or_simple_block", "block_or_lambda", "capture_entry",
  "capture_list", "optional_capture_list", "expr_block", "expr_full_block",
  "expr_full_block_assumed_piped", "$@22", "expr_numeric_const",
  "expr_assign", "expr_assign_pipe_right", "expr_assign_pipe",
  "expr_named_call", "expr_method_call", "func_addr_name",
  "func_addr_expr", "$@23", "$@24", "$@25", "$@26", "expr_field", "$@27",
  "$@28", "expr_call", "expr", "$@29", "$@30", "$@31", "$@32", "$@33",
  "$@34", "expr_mtag", "optional_field_annotation", "optional_override",
  "optional_constant", "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "struct_variable_declaration_list", "$@35", "$@36", "$@37",
  "function_argument_declaration_no_type",
  "function_argument_declaration_type", "function_argument_list",
  "tuple_type", "tuple_type_list", "tuple_alias_type_list", "variant_type",
  "variant_type_list", "variant_alias_type_list", "copy_or_move",
  "variable_declaration_no_type", "variable_declaration_type",
  "variable_declaration", "copy_or_move_or_clone", "optional_ref",
  "let_variable_name_with_pos_list", "let_variable_declaration",
  "global_variable_declaration_list", "$@38", "optional_shared",
  "optional_public_or_private_variable", "global_let", "$@39", "enum_list",
  "optional_public_or_private_alias", "single_alias", "$@40", "alias_list",
  "alias_declaration", "$@41", "optional_public_or_private_enum",
  "enum_name", "enum_declaration", "$@42", "$@43", "$@44", "$@45",
  "optional_structure_parent", "optional_sealed", "structure_name",
  "class_or_struct", "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@46", "$@47", "variable_name_with_pos_list", "basic_type_declaration",
  "enum_basic_type_declaration", "structure_type_declaration",
  "auto_type_declaration", "bitfield_bits", "bitfield_alias_bits",
  "bitfield_type_declaration", "$@48", "$@49", "c_or_s", "table_type_pair",
  "dim_list", "type_declaration_no_options", "$@50", "$@51", "$@52",
  "$@53", "$@54", "$@55", "$@56", "$@57", "$@58", "$@59", "$@60", "$@61",
  "$@62", "$@63", "$@64", "$@65", "$@66", "$@67", "$@68", "$@69", "$@70",
  "$@71", "$@72", "$@73", "$@74", "$@75", "$@76", "type_declaration",
  "tuple_alias_declaration", "$@77", "$@78", "$@79", "$@80",
  "variant_alias_declaration", "$@81", "$@82", "$@83", "$@84",
  "bitfield_alias_declaration", "$@85", "$@86", "$@87", "$@88",
  "make_decl", "make_struct_fields", "make_variant_dim",
  "make_struct_single", "make_struct_dim", "make_struct_dim_list",
  "make_struct_dim_decl", "optional_make_struct_dim_decl",
  "optional_block", "optional_trailing_semicolon_cur_cur",
  "optional_trailing_semicolon_cur_sqr",
  "optional_trailing_semicolon_sqr_sqr", "optional_trailing_delim_sqr_sqr",
  "optional_trailing_delim_cur_sqr", "use_initializer", "make_struct_decl",
  "$@89", "$@90", "$@91", "$@92", "$@93", "$@94", "$@95", "$@96",
  "make_tuple", "make_map_tuple", "make_tuple_call", "$@97", "$@98",
  "make_dim", "make_dim_decl", "$@99", "$@100", "$@101", "$@102", "$@103",
  "$@104", "$@105", "$@106", "$@107", "$@108", "make_table",
  "expr_map_tuple_list", "make_table_decl", "array_comprehension_where",
  "optional_comma", "array_comprehension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1482)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-798)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1482,    52, -1482, -1482,    43,   -98,   314,   139, -1482,   -75,
     479,   479,   479, -1482, -1482,   119,    95, -1482, -1482,   216,
   -1482, -1482, -1482, -1482,   117, -1482,    78, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482,   -92, -1482,    94,
     -21,   135, -1482, -1482, -1482, -1482,   314, -1482,    20, -1482,
   -1482, -1482,   479,   479, -1482, -1482,    78, -1482, -1482, -1482,
   -1482, -1482,    54,   150, -1482, -1482, -1482, -1482,    95,    95,
      95,   131, -1482,   721,   213, -1482, -1482, -1482, -1482,   496,
     588,   175,   606, -1482,   767,    42,    43,   168,   -98,   182,
     268, -1482,   801,   801, -1482,   297,   216,    13,   216,   771,
     306,   404,   468, -1482,   510,   233, -1482, -1482,   -18,    43,
      95,    95,    95,    95, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482,   524, -1482, -1482, -1482, -1482, -1482, -1482, -1482,   139,
   -1482, -1482, -1482, -1482, -1482,   804,   115, -1482, -1482, -1482,
   -1482,   360, -1482, -1482, -1482, -1482, -1482, -1482, -1482,   216,
   -1482, -1482, -1482,   225, -1482, -1482, -1482, -1482, -1482,   430,
   -1482,   -12, -1482,   651,   586,   721, 14709, -1482,   209,   594,
   -1482,   -85, -1482, -1482, -1482,   809, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482,    87, -1482,   548, -1482,   694, -1482,   139,
     139,   139, -1482, -1482, 13567, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
     743,   748, -1482,   571,   139,   711, -1482, -1482,   629, -1482,
     552,    43,    43,   -29,   457, -1482, -1482, -1482,   115, -1482,
   10493, -1482, -1482, -1482, -1482,   682,   702, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482,   720,   605, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482,   811, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482,   728,   687, -1482, -1482,   -61,   724,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
     139, -1482,   770,   139, -1482, -1482,   -85,   308, -1482,    43,
   -1482,   751,   926,   623, -1482, -1482,   774,   779,   781,   772,
     792,   797, -1482, -1482, -1482,   782, -1482, -1482, -1482, -1482,
   -1482,   800, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482,   803, -1482, -1482, -1482,   806,   813, -1482,
   -1482, -1482, -1482,   814,   818,   790,   119, -1482, -1482, -1482,
   -1482, -1482,   601,   827, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482,   848,  1153, -1482,   807, -1482,   102,   216,   503,   810,
   10493, -1482,  2609, -1482,   526,   119, -1482, -1482, -1482,   457,
     815, -1482,  9627,   852,   854, 10493, -1482,   181, -1482, -1482,
   -1482,  9627, -1482, -1482,   857, -1482,   553,   556,   568, -1482,
   -1482,  9627,    -6, -1482, -1482, -1482,    17, -1482, -1482, -1482,
      15,  5915, -1482,   821, 10241,   542, 10344,   487, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482,  9627, -1482, -1482,   529,    38,    38,    38, -1482,   827,
   -1482,   835,   841,  9627, -1482, -1482, -1482, -1482, -1482,   891,
     -53,   847,    48,  3229, -1482, -1482,   139,    28,  6121,   829,
    9627,   882,   860,   861,   839, -1482,   216,   863,   897,  6327,
     113,   261,   872, -1482,   528,   873,   875,  3435,  9627,  9627,
     188,   188,   188,   859,   867,   874,   877,   878,   879, -1482,
    2187, 10138,  6535, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
    6741,   888, -1482,  6949,  9627,  9627,  9627,  9627,  9627,  5091,
    7155, -1482,   870, -1482, -1482,   216,   216, -1482,  9627,  1082,
   -1482, -1482, -1482, -1482, -1482, -1482,  1060, -1482, -1482, -1482,
     727, -1482,   -35,   216, -1482,   216,   216,   216, -1482,   216,
   -1482, -1482,  1034, -1482, -1482, -1482, -1482,   886, -1482, -1482,
       9, -1482, -1482, -1482, -1482, -1482, -1482,  9946, -1482,   885,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482,   336, -1482,   260,
   -1482,    99, -1482, -1482,   887,   905,   907, -1482,  1683, -1482,
    1064,  1792, -1482, -1482, -1482,  3847, 10493, 10493, 10493,  2368,
   10493, 10493,   892,   936, 10493,   571, 10493,   571, 10493,   571,
   10596,   938, 10836, -1482,  9627, -1482, -1482, -1482, -1482,   898,
   -1482, -1482, 13050,  9627, -1482,   601,   216, -1482,   260,   -55,
   -1482, -1482,   521, -1482,   827,   260,   919,   521, -1482,   260,
   10947,   900,  1072, -1482, -1482,   146, -1482, -1482, -1482, -1482,
   -1482, 13609,   529, -1482,   902, -1482, -1482,   119,   541, -1482,
     922,   923,   924, -1482,  9627,  3847, -1482,   932,   992, 10801,
    1113, 10493,  9627,  9627, 14092,  9627, 13609,   937, -1482, -1482,
    9627, -1482, -1482,   939,   964, 14092,  9627, -1482, -1482,  9627,
   -1482, -1482,  9627, -1482, 10493,  3847, -1482, 10801,   884,   884,
     911, -1482,   886, -1482, -1482, -1482,  9627,  9627,  9627,  9627,
    9627,  9627,   529,  1943,   529,  2815,   529, 13795, -1482,   858,
   -1482, 13609, -1482,   778,   529,   948, -1482,   943,   884,   884,
     -25,   884,   884,   529,  1123,   921,   950, 14092,   925,   -31,
     950,   954,   928,   222, -1482, -1482, 13609, -1482,   139, -1482,
   -1482, -1482,  4053, -1482, -1482, -1482, -1482, -1482, -1482,   -39,
      36,   188, -1482, 14486, 14517,  4259,  4259,  4259,  4259,  4259,
    4259,  4259,  4259,  9627,  9627, -1482, -1482,  9627,  4259,  4259,
    9627,  9627,  9627,   966,  4259,  9627,   555,  9627,  9627,  9627,
    9627,  9627,  9627,  4259,  4259,  9627,  9627,  9627,  4259,  4259,
    4259,  9627,  4259,  7361,  9627,  9627,  9627,  9627,  9627,  9627,
    9627,  9627,  9627,  9627, 14647,  9627, -1482,  7567,   526,  9627,
   -1482, -1482,    95, -1482,  1136, -1482,   -85, 10493, -1482,   981,
   -1482,  3847, -1482, 10680,   543,   636,   956,   331, -1482,   648,
     673, -1482, -1482,   132,   699,   724,   713,   724,   714,   724,
   -1482,   199, -1482,   237, -1482, 10493,   940, -1482, -1482, 13161,
   -1482, -1482, 10493, -1482, -1482, 10493, -1482, -1482, -1482,  9627,
     984, -1482,   986, -1482, 10493, -1482,  9627, 10493, 10493, -1482,
      14, 10493,  5297,  7773,   987,  9627, 10493, -1482, -1482, -1482,
   10493,   950, -1482,   932,  9627,  9627,  9627,  9627,  9627,  9627,
    9627,  9627,  9627,  9627,  9627,  9627,  9627,  9627,  9627,  9627,
    9627,  9627,   139,  2024,   944, 14092, 10985, -1482, -1482, 10493,
   10493, 11096, 10493, -1482, -1482, 11131, 10493,   950, 10493, 10596,
     950,   938,  1464, -1482, 10801, -1482,    36, 11242, 11280, 11391,
   11426, 11537, 11575,    34,   188,  3022,  4467,  5503, 13832,   970,
      -9,   439,   977,   386,    49,  5709,    -9,   739,    51,  9627,
     985,  9627, -1482, -1482, 10493, -1482, 10493, -1482,  9627,   753,
      58, -1482,  9627, -1482,    59,   529, -1482,  9627, -1482,  9627,
    9627,  9627,   952,   573, -1482, -1482,   955,   957,   575, -1482,
   -1482,   784,  9627, -1482,   886,    10,  4675, -1482,    91,   958,
     998,   998, -1482, -1482,   960,   183,   571, -1482,   975,   961,
   -1482, -1482,   983,   965, -1482, -1482,   188,   188,   188, -1482,
   -1482,  1495, -1482,  1495, -1482,  1495, -1482,  1495, -1482,  1495,
   -1482,  1495, -1482,  1495, -1482,  1495,   518,   518,  1330, -1482,
    1495, -1482,  1495,  1330,   949,   949,   967, -1482,  1495,   231,
     968, -1482, 13196,   140,   140,   885, 14092,   518,   518, -1482,
    1495, -1482,  1495, 14222, 10637,  9993, -1482,  1495, -1482,  1495,
   -1482,  1495, 14185, -1482,  1495, 14616, 13925, 14315, 14341,  1158,
    1330,  1330,   635,   635,   231,   231,   231,   614,  9627,   971,
     972,   618,  9627,  1174,   976, 13307, -1482,   244,   260, 13702,
     369,   783,  1116,   216,  2274, -1482, -1482, 10680, -1482, -1482,
   -1482, -1482, 10493, -1482, -1482, -1482,  1019, -1482,   995, -1482,
     996, -1482,   997, -1482, 10596, -1482,   938,   272,   827, -1482,
   -1482,   827,   827, 11686, -1482,  1154,   -46, 14092,  2326,  4256,
    9627,   715,   626,   326,   980,   988,  1026, 11721,   105, 11832,
     716, 10493, 10596,   938,  4464,   990, 14092, 14092, 14092, 14092,
   14092, 14092, 14092, 14092, 14092, 14092, 14092, 14092, 14092, 14092,
   14092, 14092, 14092, 14092, -1482,   999, 10493, -1482, -1482,  9627,
    4672,  4880, -1482,  6738, -1482,  9830,   994, 10326,   312,  1000,
     342,    36,   571, -1482, -1482, -1482, -1482, -1482,  1003,  9627,
   -1482,  4883, 13050,    12,  9627,   573,   626,   439, -1482, -1482,
    1007, -1482,  9627,  9627, -1482,  1012, -1482,  9627,   626,   639,
    1013, -1482, -1482,  9627, 14092, -1482, -1482,   366,   398, 13962,
    9627, -1482,  9627,    60, 11870, 14092, 14092, -1482,  1014,   184,
    9627,  9627, 10493,   571,   139, -1482, -1482,  9627, -1482,  1376,
    2609,    36,   196, -1482,  1015,   375,  9833, -1482, -1482, -1482,
     474,   403,   183,  1030,  1031,  1017,  1041,  1057, -1482,   482,
     724, -1482,  9627, -1482,  9627, -1482, -1482, -1482,  7979,  9627,
   -1482,  1040,  1022, -1482, -1482,  9627,  1023, -1482, 13345,  9627,
    8185,  1024, -1482, 13456, -1482,  8391, -1482, -1482, -1482, -1482,
     216, -1482, -1482,   476, -1482,   101, -1482,    36, -1482, -1482,
   -1482, -1482,   827, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482,  1025, 10493, -1482,
    1070,  9627, -1482, -1482,   412, -1482,  1027, -1482, -1482, -1482,
     402, -1482,  1074,  1032, -1482, -1482, 10339,   407,   434, -1482,
   -1482,  9627, 10420, 14092, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482,   732,   724,  8597,  -124,    -9,   439, 14092,
     970, -1482, -1482, 14092,   977, -1482,   660,    -9,  1036, -1482,
   -1482, -1482, -1482,   663, -1482, -1482, -1482,  1067,   671,   672,
    9627,   205,  9627,  9627,  9627, 11981, 12016, 10425,   724, -1482,
   13609, -1482,   216, -1482,   571, -1482,  1039,  4675,  1077,  1038,
     683,   425, -1482, -1482,  1087, -1482, -1482,   183,  1053,    46,
   10493, 12127, 10493, 12165, -1482,   432, 12276, -1482,  9627,  2180,
    9627, -1482, 12311,  4675, -1482,   435,  9627, -1482, -1482, -1482,
     440, -1482,  1237,   101, -1482, -1482,   783,  1063, -1482, -1482,
   -1482,  9627,   827, -1482, 14092,  1068,  1075, -1482, -1482, -1482,
    9627,  1120,  1096,  9627, -1482, -1482, -1482, -1482,  1081,  1085,
    1088,  9627,  9627,  9627,  1089,  1229,  1090,  1091,  8803, -1482,
      46, -1482,   441,  9627,   439, -1482,  9627,   639, -1482,  9627,
    9627,  1093, -1482, -1482,  9627,  9627,   679,  9627,  9627, 12422,
   14092, 14092, -1482, -1482, -1482,  1105,   784,  3641, -1482,   724,
   -1482,   488, -1482,   744, 10493,   181, -1482,  1094, -1482, -1482,
    9009, -1482, -1482, 11008, -1482,   722, -1482, -1482, -1482, 10493,
   12460, 12571, -1482,   493, -1482, 12606, -1482, -1482, -1482,  1237,
     529,  1097,  1229,  1229, 12717,  1112,  1100, 12755,  1101,  1102,
    1103,  9627, -1482,  9627,  1330,  1330,  1330,  9627, -1482, -1482,
    1229,  1229, -1482, 12866, -1482, -1482, 14055, -1482, 14055, -1482,
    1129,  1330,  9627, -1482,  1144,  1129, 14055,  9627, 14092, 14092,
     226,   465, -1482, -1482,  9215,  9421, -1482, -1482, -1482, -1482,
   -1482, 14092,   139,  1104, 10493,   181,   826,  3847, -1482,  9627,
   14185, -1482, -1482,   730, -1482, -1482,  1106, -1482, 14709, -1482,
   -1482, -1482, -1482, -1482,   122,   122, -1482,  9627,  9627, -1482,
    1229,  1229,   626,  1107,  1108,   950,   122,   626, -1482,  1270,
    1111,  1148,  1149,  1137, -1482,  1150,  1119, 14055,  9627,  9627,
   -1482,   465,  9627,  9627, 14092, -1482, -1482,   826,  3847,  3847,
   -1482, 10680,  2180, -1482, -1482, -1482, -1482,   216, 14709,   626,
     970,  1145, -1482,  1121,  1122, 12901, 13012,   122,   122,   970,
    1125, -1482, -1482,  1128,  1130,  1131,  9627,  1134,  1138,  1157,
   -1482,  1135, -1482, -1482,  1147, 14092, 14092, -1482, 14092,  3847,
   -1482, 10680, -1482, 10680, -1482, -1482, -1482,   139,   448,  1151,
   -1482, -1482, -1482, -1482, -1482,  1152,  1159, -1482, -1482, -1482,
   -1482, 14092, -1482, -1482, -1482, -1482, -1482, -1482, 10680, -1482,
   -1482, -1482, -1482,   626, -1482, -1482, -1482,   449, -1482
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   132,     1,   327,     0,     0,     0,   654,   328,     0,
     646,   646,   646,    71,    72,     0,     0,    15,     3,     0,
      10,     9,     8,    16,     0,     7,   634,     6,    11,     5,
       4,    13,    12,    14,   102,   103,   101,   110,   112,    45,
      61,    58,    59,    47,    48,    49,     0,    50,    56,    46,
     243,   242,   646,   646,    22,    21,   634,   648,   647,   819,
     809,   814,     0,   295,    43,   118,   119,   120,     0,     0,
       0,   121,   123,   130,     0,   117,    17,   672,   671,   236,
     656,     0,   675,   635,   636,     0,     0,     0,     0,    51,
       0,    57,     0,     0,    54,     0,     0,   646,     0,    18,
       0,     0,     0,   297,     0,     0,   129,   124,     0,     0,
       0,     0,     0,     0,   133,   238,   237,   240,   235,   658,
     657,     0,   674,   673,   677,   676,   680,   638,   637,   640,
     108,   109,   106,   107,   105,     0,     0,   104,   113,    62,
      60,    56,    53,    52,   649,   651,   245,   244,   653,     0,
     655,    19,    20,    23,   820,   810,   815,   296,    41,    44,
     128,     0,   125,   126,   127,   131,     0,   659,     0,   668,
     630,   562,    26,    27,    31,     0,    97,    98,    95,    96,
      94,    93,    99,     0,    55,     0,   652,     0,    25,     0,
       0,     0,    42,   122,     0,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
       0,     0,   139,   134,     0,     0,   660,   669,     0,   681,
     632,     0,     0,   564,     0,    28,    29,    30,     0,   111,
       0,    24,   821,   811,   816,   204,   205,   202,   142,   143,
     145,   144,   146,   147,   148,   149,   175,   176,   173,   174,
     166,   177,   178,   167,   164,   165,   203,   186,     0,   201,
     179,   180,   181,   182,   153,   154,   155,   150,   151,   152,
     163,     0,   169,   170,   168,   161,   162,   157,   156,   158,
     159,   160,   141,   140,   185,     0,   171,   172,   562,   137,
     272,   241,   715,   718,   721,   722,   716,   719,   717,   720,
       0,   642,   666,   678,   631,   639,   562,     0,   114,     0,
     116,     0,   620,   618,   641,   100,     0,     0,     0,     0,
       0,     0,   688,   708,   689,   724,   690,   694,   695,   696,
     697,   714,   701,   702,   703,   704,   705,   706,   707,   709,
     710,   711,   712,   779,   693,   700,   713,   786,   793,   691,
     698,   692,   699,     0,     0,     0,     0,   723,   741,   744,
     742,   743,   806,   650,   729,   597,   603,   206,   207,   200,
     184,   208,   187,   183,     0,   135,   326,   588,   589,     0,
       0,   239,     0,   663,   661,     0,   670,   576,   682,     0,
       0,   115,     0,     0,     0,     0,   619,     0,   747,   770,
     773,     0,   776,   766,     0,   732,   780,   787,   794,   800,
     803,     0,     0,   756,   761,   755,     0,   769,   765,   758,
       0,     0,   760,   745,     0,   822,   812,   817,   209,   189,
     190,   192,   191,   193,   194,   195,   196,   188,   197,   198,
     199,     0,   324,   325,     0,   562,   562,   562,   136,   138,
     274,     0,     0,     0,    73,    74,    86,   467,   468,     0,
       0,     0,     0,   312,   461,   310,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   311,     0,     0,     0,     0,
       0,     0,     0,   714,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   551,
       0,     0,     0,   384,   386,   385,   387,   388,   389,   390,
       0,     0,    37,   280,     0,     0,     0,     0,     0,   276,
       0,   366,   367,   465,   464,     0,     0,   251,     0,     0,
     267,   262,   259,   258,   260,   261,   246,   294,   273,   253,
     545,   252,   462,     0,   536,    81,    82,    79,   265,    80,
     266,   268,   330,   257,   535,   534,   533,   132,   539,   463,
       0,   254,   538,   537,   509,   469,   510,   391,   470,     0,
     466,   824,   828,   825,   826,   827,   642,     0,   643,     0,
     667,   578,   633,   563,     0,     0,     0,   545,     0,   622,
     623,     0,   616,   617,   615,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   134,     0,   134,     0,   134,
       0,     0,     0,   752,   276,   763,   764,   757,   759,     0,
     762,   746,     0,     0,   808,   807,     0,   730,     0,   295,
     735,   736,     0,   598,   593,     0,     0,     0,   604,     0,
       0,     0,   683,   585,   586,   608,   590,   592,   591,   870,
     873,     0,     0,   300,   304,   303,   309,     0,     0,   352,
       0,     0,     0,   906,     0,     0,   316,   313,     0,   361,
       0,     0,   280,     0,   298,     0,     0,     0,   343,   346,
       0,   271,   349,     0,     0,    65,     0,    88,   910,     0,
     879,   888,     0,   876,     0,     0,   321,   318,   497,   498,
     367,   379,   132,   293,   291,   292,     0,     0,     0,     0,
       0,     0,     0,   848,     0,     0,     0,   886,   913,     0,
     284,     0,   287,     0,     0,     0,   915,   924,   474,   473,
     511,   472,   471,     0,     0,     0,   924,   361,     0,   295,
     924,   924,     0,   368,   269,   270,     0,    84,     0,   382,
     249,   543,     0,   256,   263,   264,   315,   320,   329,     0,
     376,     0,   255,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   499,   500,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   451,     0,   248,     0,   664,     0,
     644,   662,     0,   577,     0,   679,   562,     0,   621,     0,
     625,     0,   629,   391,     0,     0,     0,   737,   750,     0,
       0,   725,   727,     0,     0,   137,     0,   137,     0,   137,
     595,     0,   601,     0,   726,     0,     0,   754,   739,     0,
     731,   823,     0,   599,   813,     0,   605,   818,   587,     0,
       0,   607,     0,   606,     0,   609,     0,     0,     0,    89,
       0,     0,   862,     0,     0,     0,     0,   896,   899,   902,
       0,   924,   317,   314,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   299,     0,    90,    91,     0,
       0,     0,     0,    63,    64,     0,     0,   924,     0,     0,
     924,     0,     0,   322,   319,   368,   376,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   882,   840,
     848,     0,   891,     0,     0,     0,   848,     0,     0,     0,
       0,     0,   851,   918,     0,   275,     0,    40,     0,    38,
       0,   917,   925,   281,     0,     0,   893,   925,   277,     0,
       0,     0,     0,   925,   839,   502,     0,     0,   438,   435,
     437,    68,     0,   272,   132,     0,   276,   454,     0,     0,
       0,     0,   342,   341,     0,     0,   134,   290,     0,     0,
     522,   521,     0,     0,   523,   527,     0,     0,     0,   413,
     422,   401,   423,   402,   425,   404,   424,   403,   426,   405,
     416,   395,   417,   396,   418,   397,   475,   476,   488,   427,
     406,   428,   407,   489,   486,   487,     0,   415,   393,   516,
       0,   507,     0,   540,   541,   542,   394,   477,   478,   429,
     408,   430,   409,   493,   494,   495,   419,   398,   420,   399,
     421,   400,   496,   414,   392,     0,     0,   491,   492,   490,
     484,   485,   480,   479,   481,   482,   483,     0,     0,     0,
     444,     0,     0,     0,     0,     0,   459,     0,     0,     0,
       0,   570,   573,     0,     0,   624,   627,   391,   628,   748,
     771,   774,     0,   777,   767,   733,     0,   781,     0,   788,
       0,   795,     0,   801,     0,   804,     0,     0,   282,   751,
     740,   594,   600,     0,   685,   686,   611,   610,     0,     0,
       0,     0,   863,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   362,   401,   402,   404,
     403,   405,   395,   396,   397,   406,   407,   393,   408,   409,
     398,   399,   400,   392,   323,     0,     0,   919,   512,     0,
       0,     0,   513,     0,   544,     0,     0,     0,     0,     0,
       0,   376,   134,   546,   547,   548,   549,   550,     0,     0,
     849,     0,   361,   848,     0,     0,     0,     0,   857,   858,
       0,   865,     0,     0,   855,     0,   894,     0,     0,     0,
       0,   853,   895,     0,   885,   850,   914,     0,     0,    34,
       0,   916,     0,     0,     0,   830,   829,   501,     0,     0,
       0,     0,     0,   134,     0,    66,    67,     0,    83,    75,
       0,   376,     0,   455,     0,     0,     0,   458,   456,   331,
       0,     0,     0,     0,     0,     0,     0,     0,   374,     0,
     137,   518,     0,   524,     0,   412,   410,   411,     0,     0,
     505,     0,     0,   528,   532,     0,     0,   508,     0,     0,
       0,     0,   445,     0,   452,     0,   503,   460,   665,   645,
     133,   571,   572,   573,   574,   565,   579,   376,   626,   749,
     772,   775,   738,   778,   768,   734,   728,   782,   784,   789,
     791,   796,   798,   802,   596,   805,   602,     0,     0,   684,
       0,     0,   871,   874,     0,   301,     0,   306,   307,   305,
       0,   355,     0,     0,   358,   353,     0,     0,     0,   907,
     905,   280,     0,    92,   344,   347,   350,   911,   909,   880,
     889,   887,   877,     0,   137,     0,     0,   848,     0,   883,
     841,   864,   856,   884,   892,   854,     0,   848,     0,   860,
     861,   868,   852,     0,   285,   288,    35,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   137,    69,
       0,    76,     0,   247,   134,   457,     0,   276,     0,     0,
     618,     0,   371,   372,     0,   370,   369,     0,     0,     0,
       0,     0,     0,     0,   433,     0,     0,   529,     0,   517,
       0,   506,     0,   276,   446,     0,     0,   504,   453,   449,
       0,   584,   568,   565,   566,   567,   570,     0,   785,   792,
     799,   276,   283,   687,   612,     0,     0,    87,   302,   308,
       0,     0,     0,     0,   354,   897,   900,   903,     0,     0,
       0,     0,     0,     0,     0,   862,     0,     0,     0,   250,
       0,   552,     0,     0,     0,   866,     0,     0,   859,     0,
       0,   278,    32,    39,     0,     0,     0,     0,     0,     0,
     832,   831,   436,   561,   439,     0,    68,     0,    85,   137,
     431,     0,   332,   618,     0,     0,   378,     0,   375,   377,
       0,   363,   381,     0,   560,     0,   558,   434,   555,     0,
       0,     0,   554,     0,   447,     0,   450,   569,   580,   568,
       0,     0,   862,   862,     0,     0,     0,     0,     0,     0,
       0,   276,   920,   280,   345,   348,   351,     0,   863,   881,
     862,   862,   514,     0,   380,   553,   922,   867,   922,   869,
     922,   286,   276,   289,    36,   922,   922,     0,   834,   833,
       0,     0,   442,    70,   312,     0,    77,    81,    82,    79,
      80,    78,     0,     0,     0,     0,     0,     0,   373,     0,
     364,   519,   525,     0,   559,   557,     0,   556,     0,   582,
     614,   613,   575,   753,   847,   847,   356,     0,     0,   359,
     862,   862,   837,     0,     0,   924,   847,   837,   515,     0,
       0,     0,     0,   924,    33,     0,     0,   922,     0,     0,
     440,     0,     0,     0,   318,   383,   432,     0,     0,     0,
     340,   391,   365,   520,   526,   530,   448,     0,     0,     0,
     844,   924,   846,     0,     0,     0,     0,   847,   847,   838,
       0,   908,   921,     0,     0,     0,     0,     0,     0,     0,
     925,     0,   930,   926,     0,   836,   835,   443,   319,     0,
     338,   391,   336,   391,   339,   531,   581,     0,     0,   925,
     845,   872,   875,   357,   360,     0,     0,   904,   912,   890,
     878,   923,   928,   929,   931,   279,   927,   334,   391,   337,
     335,   583,   842,     0,   898,   901,   333,     0,   843
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1482, -1482, -1482, -1482, -1482, -1482, -1482,   604,  1253, -1482,
   -1482, -1482, -1482, -1482, -1482,  1346, -1482, -1482, -1482,   869,
    1319, -1482,  1225, -1482, -1482,  1279, -1482, -1482, -1482,  -147,
      -1, -1482, -1482, -1482,  -142, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482,  1140, -1482, -1482,   -65,   -60, -1482,
   -1482, -1482,   436,   544,  -540,  -589,  -824, -1482, -1482, -1482,
   -1481, -1482, -1482,    31,  -206,  -219,  -386, -1482,   376, -1482,
    -617, -1482,  -674,   -91,  -328, -1482, -1482, -1482, -1482,  -472,
       4, -1482, -1482, -1482, -1482, -1482,  -137,  -136,  -135, -1482,
    -132, -1482, -1482, -1482,  1385, -1482,   378, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
    -427,  -103,   174,   -37,   120,  -903,  -446, -1482,  -541, -1482,
   -1482,  -389,  1560, -1482, -1482, -1482, -1416, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482,   953, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482,  -154,   -62,  -156,   -57,    81, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482,   593,  -420,  -907, -1482,
    -426,  -902, -1482,  -653,  -155,  -150, -1482,  -597, -1360, -1482,
    -387, -1482, -1482,  1340, -1482, -1482, -1482,   816,  1071,   190,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
    -633,  -195, -1482,   934, -1482, -1482, -1482, -1482, -1482, -1482,
    -318, -1482, -1482,  -368, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482,  -116, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482,   941,  -640,  -236,
    -726,  -703, -1482, -1482, -1247,  -919, -1482, -1482, -1482, -1194,
     -90, -1225, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482,   185,  -499, -1482, -1482, -1482,   680, -1482, -1482, -1482,
   -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482, -1482,
   -1482, -1279,  -736, -1482
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    17,   153,    56,   188,    18,   175,   181,  1584,
    1397,  1502,   733,   533,   159,   534,   105,    20,    21,    47,
      48,    49,    94,    22,    41,    42,   535,   536,  1257,  1258,
     641,   538,  1412,  1517,   539,   540,  1002,   541,   662,   542,
     543,   544,   545,  1189,   182,   183,    37,    38,    39,   233,
      71,    72,    73,    74,    24,   299,   391,   223,    25,   117,
     224,   118,   166,   300,   148,   711,  1029,   548,   392,   549,
     745,  1583,   735,  1137,   597,   974,  1500,   976,  1501,   551,
     552,   553,   664,   891,  1468,   554,   555,   556,   557,   558,
     559,   560,   561,   454,   562,   769,  1270,  1012,   563,   564,
     929,  1481,   930,  1482,   932,  1483,   565,   896,  1474,   566,
     746,  1532,   567,  1278,  1279,  1016,   713,   568,   826,  1004,
     569,   678,  1030,   571,   572,   573,  1000,   574,  1252,  1591,
    1253,  1651,   575,  1103,  1448,   576,   747,  1430,  1663,  1432,
    1664,  1539,  1705,   578,   386,  1456,  1548,  1313,  1315,  1113,
     591,   836,  1618,  1668,   387,   388,   389,   860,   861,   436,
     862,   863,   437,   991,   653,   654,  1622,   605,   407,   323,
     324,   230,   316,    84,   129,    27,   171,   394,    95,    96,
     185,    97,    28,    53,   121,   168,    29,   311,   589,   586,
    1108,   396,   228,   229,    82,   126,   398,    30,   169,   313,
     655,   579,   310,   369,   370,   853,   435,   371,   613,  1325,
    1134,   846,   433,   372,   606,  1319,   865,   611,  1324,   607,
    1320,   608,  1321,   610,  1323,   614,  1327,   615,  1458,   616,
    1329,   617,  1459,   618,  1331,   619,  1460,   620,  1333,   621,
    1335,   644,    31,   101,   190,   375,   645,    32,   102,   191,
     376,   649,    33,   100,   189,   374,   638,   580,   751,  1680,
     752,   960,  1671,  1672,  1673,   961,   973,  1232,  1226,  1221,
    1391,  1155,   581,   887,  1465,   888,  1466,   941,  1487,   938,
    1485,   962,   736,   582,   939,  1486,   963,   583,  1161,  1558,
    1162,  1559,  1163,  1560,   900,  1478,   936,  1484,   729,   737,
     584,  1640,   983,   585
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      23,   676,   886,   570,   841,   301,   547,   866,   924,   760,
     988,   647,   592,   728,   992,   994,   642,   234,    76,    64,
      75,   138,   966,  1381,   315,   706,   855,   770,   857,   890,
     859,  1128,  1198,  1130,  1150,  1132,  1009,   601,    52,  1200,
    1014,  1217,    90,  1202,   954,   368,   677,  1229,   627,   161,
     625,   834,     2,   670,  1209,   714,   715,   130,   131,     3,
    1525,  1493,    34,    35,   550,   954,   635,   904,    40,  1227,
      57,  1233,    75,    75,    75,   231,    58,    91,  1240,  1242,
    1400,   881,     4,   959,     5,   959,     6,   547,   104,   953,
      62,   964,     7,   968,   384,   145,   881,   150,    54,   231,
      85,   980,     8,   750,   796,   797,   990,  -132,     9,   452,
     984,   547,   104,   321,    75,    75,    75,    75,   643,   648,
      63,  1010,    77,    78,   373,    79,   110,   111,   112,    65,
     176,   177,    10,   842,   322,   232,    13,  1667,   761,   872,
      83,   771,   723,   725,    55,   550,   883,  1454,   186,   434,
     671,   672,   453,    80,    11,    12,    14,   385,    66,   232,
     170,   883,   399,  1605,   667,  1165,  1154,   318,   748,   550,
     748,   317,   946,    87,    13,  1650,  1216,   146,  1011,    86,
     122,   123,   762,  1262,  1495,  1262,   546,  1707,  1530,   824,
     825,   319,   623,   384,    14,   368,  1015,   147,   231,   226,
     160,  1196,    67,   902,  1199,   882,   193,  1455,   132,    50,
     368,   624,   628,   133,   626,   134,    36,    13,   135,   547,
     242,   243,   244,    92,    81,   882,   681,   103,  1263,    51,
     629,    68,   320,   943,    93,  1697,   630,    14,    62,   368,
     882,   368,   882,    98,   367,   682,   673,   901,   903,   882,
     882,   882,   773,   774,   401,  1357,    15,   680,   232,   136,
    1569,  1358,    62,   146,    13,   674,   793,    16,    63,   796,
     797,   697,   937,   881,   459,   940,    50,   550,   238,  1264,
     748,   178,  1266,   147,    14,    86,   179,   149,   180,   547,
    1352,   135,    63,   104,  1378,  1262,    51,    13,  1373,  1641,
    1577,  1642,    50,    69,  1353,   239,  1645,  1646,   602,  1267,
    1273,   881,    70,   923,   832,   368,   368,    14,   603,   547,
    1274,  1403,    51,   881,   873,  1017,    88,  1624,  1625,   876,
     696,  1125,   881,   990,   139,  1008,   942,   882,   883,  1669,
     884,   393,  1507,   885,   397,  1636,  1637,   550,   109,   785,
     786,    50,  1243,   881,   824,   825,  1275,   793,  1414,   795,
     796,   797,   798,  1648,    13,    62,  1265,   799,  1694,  1116,
     422,    51,    50,   604,   996,  1276,   883,   550,  1674,   997,
    1277,    13,  1013,   831,    14,   835,   455,   457,   883,  1684,
     640,   537,    51,   588,   367,    63,    93,   883,  1133,   590,
    1107,    14,    13,   225,   113,  1677,  1678,   531,   710,   367,
     158,   368,   368,   368,  1457,   368,   368,    91,   883,   368,
     998,   368,    14,   368,   146,   368,  1346,  1280,   640,   114,
    1715,  1716,   871,   187,   637,   904,  1135,    13,   367,   874,
     367,   141,   889,   877,   147,   824,   825,   550,   550,   550,
     550,   550,   550,   550,   550,   547,  1429,    14,  1494,   698,
     550,   550,  1307,   640,  1261,  1153,   550,   927,  1497,  1114,
     144,  1337,  1236,   367,   668,   550,   550,    13,   699,   154,
     550,   550,   550,  1241,   550,   691,   368,    43,    44,    45,
     844,   845,   847,  1341,   849,   850,    13,    14,   854,    86,
     856,    13,   858,   640,   106,   107,   108,    13,  1210,   368,
    1377,  1370,   975,   550,   367,   367,    14,   904,    46,  1148,
    1149,    14,   640,  1452,   400,  1387,   434,    14,   829,  1122,
     750,    13,  1164,   640,   754,   755,    57,  1001,   750,   773,
     774,  1372,    58,  1314,  1347,  1136,   162,   163,   164,   165,
    1490,    14,   763,   115,   764,   765,   766,   640,   767,   116,
     113,  1190,  1191,    13,  1193,  1394,  1215,    13,  1195,   772,
    1197,  1223,    13,   959,  1224,    50,  1380,   155,  1021,  1025,
    1285,  1286,  1287,    14,  1515,  1310,   830,    14,   959,   640,
     833,  1416,    14,   640,  1418,    51,  1404,  1395,   640,    13,
     840,  1470,  1225,   904,  1065,  1186,  1476,   192,    62,   321,
     367,   367,   367,  1374,   367,   367,  1427,   996,   367,    14,
     367,  1420,   367,   904,   367,   640,   904,  1218,  1219,  1104,
     322,   904,   904,  1477,   423,   870,   785,   786,    63,  1215,
    1215,   156,   368,  1526,   793,   119,   795,   796,   797,   798,
    1537,   120,    13,  1544,   799,  1220,   773,   774,  1546,  1575,
     646,   424,   425,   124,  1408,  1418,  1732,  1738,    13,   125,
     368,   893,    14,  1427,   712,   712,   712,   368,   640,  1215,
     368,   651,  1112,   157,  1215,   367,    13,  1479,    14,   368,
    1419,    13,   368,   368,   456,  1602,   368,   167,  1428,   587,
     227,   368,   652,  1184,  1603,   368,    14,    13,   367,  1616,
    1336,    14,   640,  1060,  1334,   636,   146,   314,   819,   820,
     821,   822,   823,  1344,   759,   426,   701,    14,  1061,   427,
     110,  1248,   824,   825,   368,   368,   147,   368,   434,   894,
     240,   368,  1119,   368,   368,   702,  1249,  -783,  1508,  1138,
    -790,   759,  -783,   785,   786,  -790,  1141,   999,   895,  1142,
     241,   793,  -797,   795,   796,   797,   798,  -797,  1146,  -441,
    -783,   799,  1296,  -790,  -441,  1151,  1301,  1521,   296,   368,
    1160,   368,  1376,   297,   748,  -797,   302,  1297,   298,  1003,
     303,  1302,  -441,  1356,   428,   110,  1254,   112,   429,  1262,
    1386,   430,   312,  1543,   304,   305,  1393,  1255,  1256,   306,
     307,   308,   309,  1398,   404,  1399,   431,   405,  1362,  1338,
     406,   380,   432,  1388,   127,  1519,  1389,   588,   151,  1390,
     128,   434,  1489,   381,   152,  1120,    75,   821,   822,   823,
    1311,   367,  1118,   434,  1551,  1496,  1312,  1123,  1499,   824,
     825,   904,  1126,   759,   904,   377,  1504,  1505,  1138,   423,
    1138,  1435,   904,   904,  1587,   110,   111,   112,   434,   367,
     904,   570,  1124,  1445,   547,   378,   367,  1524,  1450,   367,
     406,   759,  1136,  1237,  1407,  1238,   424,   425,   367,  1634,
    1294,   367,   367,   379,   434,   367,   461,   462,  1127,  1683,
     367,   382,  1308,   383,   367,   773,   774,  1691,   434,   434,
     434,   434,  1129,  1131,  1345,  1355,   472,   434,   390,  1338,
    1338,  1612,   477,  1230,  1223,   434,  1231,   368,  1607,  1665,
     235,   236,   550,   367,   367,  1710,   367,  1649,  1604,   368,
     367,   406,   367,   367,  1633,   712,   531,   710,  1492,  1488,
     426,   531,   710,   602,   427,   172,   173,   977,   978,   491,
     492,   142,   143,   603,   395,  1643,   368,   368,   402,   403,
     773,   774,   408,  1506,    43,    44,    45,   409,   367,   410,
     367,   172,   173,   174,  1670,  1670,   235,   236,   237,   411,
     412,   368,  1679,   494,   495,   413,  1670,  1679,   415,   414,
    1580,   416,   785,   786,   417,  1585,  1322,   421,  1658,  1659,
     793,   418,   419,   796,   797,   798,   420,   759,   604,   428,
     799,   438,   434,   429,   451,   599,   430,   600,   458,  1708,
     612,   593,  1471,   659,    62,  1409,   633,  1670,  1670,   660,
    1136,   431,   970,   971,   972,   669,   685,   432,   656,   657,
     658,   510,   511,   512,  1413,   687,   690,   368,   688,   689,
    1699,   692,  1533,   693,    63,   783,   784,   785,   786,   787,
     700,   703,   790,   704,   523,   793,   716,   795,   796,   797,
     798,    59,    60,    61,   717,   799,   732,   801,   802,   663,
     753,   718,   757,  1737,   719,   720,   721,   758,   824,   825,
     768,    16,   827,   837,   696,   730,   529,   839,  1309,   852,
     851,   646,  1316,   875,   867,   880,  1318,   879,   759,   892,
     897,   898,   899,   904,   771,  1467,   367,   922,   712,   928,
     934,   945,   981,   933,   982,  1660,   985,   986,   367,  1056,
    1635,   987,   989,   368,  1111,   993,   995,   817,   818,   819,
     820,   821,   822,   823,  1115,  1121,  1606,  1144,  1139,  1145,
    1158,  1215,  1187,   824,   825,   367,   367,   677,  1222,  1235,
    1247,  1269,  1250,  1281,  1251,  1304,  1268,  1272,  1282,   773,
     774,  1283,  1284,  1314,  1288,  1289,  1700,  1702,  1299,  1300,
     367,  1516,  1326,  1305,  1328,  1330,  1332,  1340,  1348,  1350,
     712,   712,   712,  1422,  1423,   759,  1349,   759,  1360,   759,
    1531,   759,  1368,   759,  1425,   759,  1361,   759,  1371,   759,
    1375,   547,  1462,  1382,   759,   903,   759,  1727,  1385,  1392,
    1426,  1402,   759,  1415,  1424,   368,  1657,   368,  1437,  1438,
    1440,  1446,  1461,  1463,   759,  1469,   759,  1472,  1503,  1473,
    1522,   759,  1498,   759,  1523,   759,   367,  1520,   759,   537,
    1527,   439,   440,   441,   442,   443,   444,   445,   446,  1529,
    1547,  1531,   547,   547,   783,   784,   785,   786,   787,   550,
    1488,   790,   791,   792,   793,  1552,   795,   796,   797,   798,
     447,   759,  1553,  1555,   799,  1556,   801,   802,  1561,  1568,
     448,   449,   450,  1562,  1592,  1563,  1567,  1570,  1571,  1451,
    1582,  1627,  1608,   547,  1639,  1623,  1535,  1628,  1630,  1631,
    1632,  1644,  1656,  1686,  1666,  1681,  1682,  1687,  1690,   368,
     550,   550,  1688,  1689,  1692,  1693,  1709,   979,   137,  1711,
    1712,  1724,   367,  1717,   368,   577,  1718,    19,  1719,  1720,
    1722,   773,   774,  1725,  1723,   598,   817,   818,   819,   820,
     821,   822,   823,  1726,   609,    89,   184,   140,  1733,  1593,
    1734,   550,   824,   825,   622,  1596,  1110,  1735,   325,  1260,
    1597,  1598,  1599,  1655,   632,  1600,    26,  1574,  1411,  1271,
    1528,  1549,  1421,  1619,  1453,  1620,    99,   773,   774,  1550,
    1621,  1685,   828,   665,   650,   967,     0,  1579,  1384,   368,
     666,  1518,     0,     0,     0,     0,   661,     0,     0,     0,
       0,     0,     0,  1613,     0,     0,   679,     0,     0,     0,
       0,   684,     0,   686,   367,     0,   367,     0,     0,     0,
       0,     0,   695,     0,     0,     0,   783,   784,   785,   786,
     707,   708,   709,     0,     0,     0,   793,     0,   795,   796,
     797,   798,     0,     0,     0,   727,   799,     0,   801,   802,
       0,     0,     0,   731,     0,     0,   727,   738,   739,   740,
     741,   742,     0,     0,     0,     0,     0,     0,  1731,     0,
       0,   756,   783,   784,   785,   786,   787,   423,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,     0,     0,
       0,     0,   799,     0,   801,   802,   773,   774,     0,     0,
     805,   806,   807,     0,   424,   425,   811,     0,   367,     0,
     819,   820,   821,   822,   823,     0,     0,     0,     0,     0,
       0,     0,     0,   367,   824,   825,     0,   759,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   843,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,     0,     0,     0,     0,     0,   869,     0,   426,     0,
     824,   825,   427,     0,     0,   999,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   367,     0,
       0,   783,   784,   785,   786,   787,     0,     0,   790,   791,
     792,   793,     0,   795,   796,   797,   798,     0,   679,     0,
       0,   799,     0,   801,   802,   727,   925,     0,   926,   805,
     806,   807,     0,   931,     0,   811,     0,     0,     0,   935,
       0,     0,     0,     0,     0,   999,     0,   428,   944,     0,
    1704,   429,     0,  1201,   430,     0,  1706,     0,     0,   947,
     948,   949,   950,   951,   952,     0,   958,     0,   958,   431,
       0,     0,     0,     0,     0,   432,     0,     0,   813,     0,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
    1729,     0,  1730,     0,   773,   774,     0,     0,     0,   824,
     825,     0,     0,     0,   531,   710,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1736,  1031,  1033,
    1035,  1037,  1039,  1041,  1043,  1045,  1046,  1047,     0,     0,
    1048,  1050,  1052,  1053,  1054,  1055,     0,  1058,  1059,     0,
    1062,  1063,  1064,  1066,  1067,  1068,  1070,  1072,  1073,  1074,
    1075,  1077,  1079,  1081,  1082,  1084,  1086,  1087,  1088,  1089,
    1090,  1091,  1092,  1093,  1094,  1095,  1096,     0,  1105,     0,
       0,     0,  1109,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1117,     0,     0,     0,     0,   783,
     784,   785,   786,   787,     0,     0,   790,   791,   792,   793,
       0,   795,   796,   797,   798,     0,     0,     0,     0,   799,
       0,   801,   802,     0,     0,   423,     0,   805,   806,   807,
       0,     0,  1143,   811,     0,   759,     0,     0,     0,  1147,
       0,     0,     0,     0,     0,     0,  1157,     0,  1159,     0,
       0,     0,   424,   425,     0,     0,     0,  1166,  1167,  1168,
    1169,  1170,  1171,  1172,  1173,  1174,  1175,  1176,  1177,  1178,
    1179,  1180,  1181,  1182,  1183,   759,   813,   759,   814,   815,
     816,   817,   818,   819,   820,   821,   822,   823,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   824,   825,     0,
       0,   838,   759,     0,     0,     0,     0,     0,   738,  1212,
       0,     0,     0,     0,     0,     0,   426,     0,     0,   602,
     427,     0,  1234,     0,   727,     0,     0,     0,     0,   603,
       0,  1239,     0,     0,     0,   727,     0,     0,     0,     0,
    1166,     0,  1244,  1245,  1246,     0,     0,     0,   461,   462,
       0,     0,     0,     0,     0,  1259,     0,    13,   467,   468,
     469,   470,   471,     0,     0,     0,     0,     0,   472,     0,
     474,     0,     0,     0,   477,     0,   423,    14,     0,     0,
       0,     0,   479,     0,   604,   428,     0,     0,   482,   429,
       0,   483,   430,     0,   484,     0,   954,     0,   487,     0,
       0,     0,     0,   424,   425,     0,     0,   431,   594,     0,
       0,   491,   492,   432,   332,   333,   334,     0,   336,   337,
     338,   339,   340,   493,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,     0,   354,   355,   356,     0,
       0,   359,   360,   361,   362,   494,   495,   595,     0,     0,
       0,  1298,     0,     0,     0,  1303,     0,   423,     0,     0,
       0,   498,   499,     0,     0,     0,     0,   426,     0,     0,
       0,   427,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   424,   425,    62,     0,     0,     0,
       0,     0,     0,     0,   503,   504,   505,   506,   507,     0,
     508,   748,   509,   510,   511,   512,     0,     0,     0,   513,
     514,   515,   516,   517,   518,   519,   749,   596,   521,   522,
       0,     0,     0,     0,     0,     0,   523,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   428,     0,     0,     0,
     429,     0,  1363,   955,   525,   526,     0,    15,   426,     0,
     527,   528,   427,     0,     0,     0,     0,     0,   956,     0,
     957,     0,   531,   532,   432,     0,     0,  1379,     0,     0,
       0,     0,     0,     0,     0,  1383,   958,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    13,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     722,   773,   774,  1405,  1406,     0,   326,     0,     0,    14,
    1410,     0,   327,   577,     0,   640,     0,   428,   328,  1166,
       0,   429,     0,  1185,   430,     0,     0,     0,   329,     0,
       0,     0,     0,     0,     0,  1431,   330,  1433,     0,   431,
       0,     0,  1436,     0,     0,   432,     0,     0,  1439,     0,
       0,   331,  1442,     0,     0,     0,     0,     0,   332,   333,
     334,   335,   336,   337,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,     0,     0,     0,  1464,     0,   783,   784,   785,   786,
     787,     0,     0,   790,   791,   792,   793,   423,   795,   796,
     797,   798,     0,     0,   727,     0,   799,     0,   801,   802,
       0,     0,     0,     0,   805,   806,   807,     0,     0,     0,
      62,     0,     0,     0,   424,   425,  1032,  1034,  1036,  1038,
    1040,  1042,  1044,   365,     0,     0,     0,     0,  1049,  1051,
       0,     0,     0,     0,  1057,  1509,  1510,  1511,     0,   423,
      63,     0,     0,  1069,  1071,     0,     0,     0,  1076,  1078,
    1080,     0,  1083,   813,     0,   814,   815,   816,   817,   818,
     819,   820,   821,   822,   823,     0,   424,   425,     0,   773,
     774,  1540,     0,  1541,   824,   825,     0,     0,   426,  1545,
       0,     0,   427,     0,     0,     0,   366,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1554,     0,     0,  1557,     0,     0,     0,
       0,     0,     0,     0,  1564,  1565,  1566,     0,     0,     0,
       0,  1573,     0,     0,     0,     0,  1576,     0,     0,  1578,
     426,     0,   727,  1581,   427,     0,     0,   727,  1586,     0,
    1588,  1589,     0,     0,     0,     0,     0,   428,     0,     0,
    1601,   429,     0,  1317,   430,     0,     0,     0,     0,     0,
       0,     0,     0,  1610,   783,   784,   785,   786,   787,   431,
       0,   790,   791,   792,   793,   432,   795,   796,   797,   798,
       0,     0,     0,     0,   799,     0,   801,   802,     0,     0,
       0,     0,   805,   806,   807,     0,   727,     0,   811,   428,
       0,     0,     0,   429,     0,  1342,   430,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1647,   431,     0,     0,     0,     0,     0,   432,  1654,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1661,   813,  1662,   814,   815,   816,   817,   818,   819,   820,
     821,   822,   823,     0,     0,     0,     0,     0,     0,     0,
    1675,  1676,   824,   825,     0,     0,   848,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1695,  1696,     0,     0,     0,  1698,     0,     0,     0,
     460,  1701,  1703,     0,   461,   462,     3,     0,   463,   464,
     465,     0,   466,     0,   467,   468,   469,   470,   471,     0,
       0,     0,     0,     0,   472,   473,   474,   475,   476,  1721,
     477,     0,     0,     0,     0,     0,     0,   478,   479,     0,
       0,   480,  1728,   481,   482,     0,     0,   483,     0,     8,
     484,   485,     0,   486,   487,     0,     0,   488,   489,     0,
       0,     0,     0,     0,   490,     0,     0,   491,   492,     0,
     332,   333,   334,     0,   336,   337,   338,   339,   340,   493,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,     0,   354,   355,   356,     0,     0,   359,   360,   361,
     362,   494,   495,   496,   497,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,   499,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   500,   501,   502,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     503,   504,   505,   506,   507,     0,   508,     0,   509,   510,
     511,   512,     0,   146,    13,   513,   514,   515,   516,   517,
     518,   519,    63,   520,   521,   522,     0,     0,     0,     0,
       0,     0,   523,   147,    14,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   524,
     525,   526,     0,    15,     0,     0,   527,   528,     0,     0,
     461,   462,     0,     0,   529,     0,   530,     0,   531,   532,
     467,   468,   469,   470,   471,     0,     0,     0,     0,     0,
     472,     0,   474,     0,     0,     0,   477,     0,   423,     0,
       0,     0,     0,     0,   479,     0,     0,     0,     0,     0,
     482,     0,     0,   483,     0,     0,   484,     0,     0,     0,
     487,     0,     0,     0,     0,   424,   425,     0,     0,     0,
     594,     0,     0,   491,   492,     0,   332,   333,   334,     0,
     336,   337,   338,   339,   340,   493,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,     0,   354,   355,
     356,     0,     0,   359,   360,   361,   362,   494,   495,   595,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   498,   499,     0,     0,     0,     0,   426,
       0,     0,     0,   427,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   503,   504,   505,   506,
     507,     0,   508,   748,   509,   510,   511,   512,     0,     0,
       0,   513,   514,   515,   516,   517,   518,   519,   749,   596,
     521,   522,     0,     0,     0,     0,     0,     0,   523,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   428,     0,
       0,     0,   429,     0,     0,   955,   525,   526,     0,    15,
       0,     0,   527,   528,     0,     0,     0,   461,   462,     0,
     956,     0,   965,     0,   531,   532,   432,   467,   468,   469,
     470,   471,     0,     0,     0,     0,     0,   472,     0,   474,
       0,     0,     0,   477,     0,   627,     0,     0,     0,     0,
       0,   479,     0,     0,     0,     0,     0,   482,     0,     0,
     483,     0,     0,   484,     0,     0,     0,   487,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   594,     0,     0,
     491,   492,     0,   332,   333,   334,     0,   336,   337,   338,
     339,   340,   493,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,     0,   354,   355,   356,     0,     0,
     359,   360,   361,   362,   494,   495,   595,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     498,   499,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    62,     0,     0,     0,     0,
       0,     0,     0,   503,   504,   505,   506,   507,     0,   508,
       0,   509,   510,   511,   512,     0,     0,     0,   513,   514,
     515,   516,   517,   518,   519,    63,   596,   521,   522,     0,
       0,     0,     0,     0,     0,   523,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   628,
       0,     0,   524,   525,   526,     0,    15,     0,     0,   527,
     528,     0,     0,     0,   461,   462,     0,  1211,     0,   530,
       0,   531,   532,   630,   467,   468,   469,   470,   471,     0,
       0,     0,     0,     0,   472,     0,   474,     0,     0,     0,
     477,     0,     0,     0,     0,     0,     0,     0,   479,     0,
       0,     0,     0,     0,   482,     0,     0,   483,     0,     0,
     484,     0,     0,     0,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   594,     0,     0,   491,   492,     0,
     332,   333,   334,     0,   336,   337,   338,   339,   340,   493,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,     0,   354,   355,   356,     0,     0,   359,   360,   361,
     362,   494,   495,   496,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,   499,     0,
       0,     0,     0,     0,     0,     0,   675,     0,     0,     0,
       0,     0,   500,   501,   502,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     503,   504,   505,   506,   507,     0,   508,     0,   509,   510,
     511,   512,     0,     0,     0,   513,   514,   515,   516,   517,
     518,   519,    63,   596,   521,   522,     0,     0,     0,     0,
       0,     0,   523,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   524,
     525,   526,     0,    15,     0,     0,   527,   528,     0,     0,
     461,   462,     0,     0,   529,     0,   530,     0,   531,   532,
     467,   468,   469,   470,   471,     0,     0,     0,     0,     0,
     472,     0,   474,     0,     0,     0,   477,     0,     0,     0,
       0,     0,     0,     0,   479,     0,     0,     0,     0,     0,
     482,     0,     0,   483,     0,     0,   484,     0,     0,     0,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     594,     0,     0,   491,   492,     0,   332,   333,   334,     0,
     336,   337,   338,   339,   340,   493,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,     0,   354,   355,
     356,     0,     0,   359,   360,   361,   362,   494,   495,   496,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   498,   499,     0,     0,     0,     0,     0,
       0,     0,   705,     0,     0,     0,     0,     0,   500,   501,
     502,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   503,   504,   505,   506,
     507,     0,   508,     0,   509,   510,   511,   512,     0,     0,
       0,   513,   514,   515,   516,   517,   518,   519,    63,   596,
     521,   522,     0,     0,     0,     0,     0,     0,   523,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   524,   525,   526,     0,    15,
       0,     0,   527,   528,     0,     0,   461,   462,     0,     0,
     529,     0,   530,     0,   531,   532,   467,   468,   469,   470,
     471,     0,     0,     0,     0,     0,   472,  1594,   474,   475,
       0,     0,   477,     0,     0,     0,     0,     0,     0,     0,
     479,     0,     0,     0,     0,     0,   482,     0,     0,   483,
       0,     0,   484,   485,     0,     0,   487,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   594,     0,     0,   491,
     492,     0,   332,   333,   334,     0,   336,   337,   338,   339,
     340,   493,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,     0,   354,   355,   356,     0,     0,   359,
     360,   361,   362,   494,   495,   595,  1595,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   498,
     499,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   503,   504,   505,   506,   507,     0,   508,     0,
     509,   510,   511,   512,     0,     0,     0,   513,   514,   515,
     516,   517,   518,   519,    63,   596,   521,   522,     0,     0,
       0,     0,     0,     0,   523,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   524,   525,   526,     0,    15,     0,     0,   527,   528,
       0,     0,   461,   462,     0,     0,   529,     0,   530,     0,
     531,   532,   467,   468,   469,   470,   471,     0,     0,     0,
       0,     0,   472,     0,   474,     0,     0,     0,   477,     0,
       0,     0,     0,     0,     0,     0,   479,     0,     0,     0,
       0,     0,   482,     0,     0,   483,     0,     0,   484,     0,
       0,     0,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   594,     0,     0,   491,   492,     0,   332,   333,
     334,     0,   336,   337,   338,   339,   340,   493,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,     0,
     354,   355,   356,     0,     0,   359,   360,   361,   362,   494,
     495,   496,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   498,   499,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     500,   501,   502,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   503,   504,
     505,   506,   507,     0,   508,     0,   509,   510,   511,   512,
       0,     0,     0,   513,   514,   515,   516,   517,   518,   519,
      63,   596,   521,   522,     0,     0,     0,     0,     0,     0,
     523,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   524,   525,   526,
       0,    15,     0,     0,   527,   528,     0,     0,   461,   462,
       0,     0,   529,     0,   530,     0,   531,   532,   467,   468,
     469,   470,   471,     0,     0,     0,     0,     0,   472,     0,
     474,     0,     0,     0,   477,     0,     0,     0,     0,     0,
       0,     0,   479,     0,     0,     0,     0,     0,   482,     0,
       0,   483,     0,     0,   484,     0,     0,     0,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   594,     0,
       0,   491,   492,  1005,   332,   333,   334,     0,   336,   337,
     338,   339,   340,   493,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,     0,   354,   355,   356,     0,
       0,   359,   360,   361,   362,   494,   495,   595,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   498,   499,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   503,   504,   505,   506,   507,     0,
     508,   748,   509,   510,   511,   512,     0,     0,     0,   513,
     514,   515,   516,   517,   518,   519,   749,   596,   521,   522,
       0,     0,     0,     0,     0,     0,   523,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   524,   525,   526,     0,    15,     0,     0,
     527,   528,     0,     0,   461,   462,     0,     0,  1006,     0,
     530,  1007,   531,   532,   467,   468,   469,   470,   471,     0,
       0,     0,     0,     0,   472,     0,   474,     0,     0,   423,
     477,     0,     0,     0,     0,     0,     0,     0,   479,     0,
       0,     0,     0,     0,   482,     0,     0,   483,     0,     0,
     484,     0,     0,     0,   487,     0,   424,   425,     0,     0,
       0,     0,     0,     0,   594,     0,     0,   491,   492,     0,
     332,   333,   334,     0,   336,   337,   338,   339,   340,   493,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,     0,   354,   355,   356,     0,     0,   359,   360,   361,
     362,   494,   495,   496,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,   499,     0,
     426,     0,     0,     0,   427,     0,     0,     0,     0,     0,
       0,     0,  1026,  1027,  1028,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     503,   504,   505,   506,   507,     0,   508,     0,   509,   510,
     511,   512,     0,     0,     0,   513,   514,   515,   516,   517,
     518,   519,    63,   596,   521,   522,     0,     0,     0,     0,
       0,     0,   523,     0,     0,     0,     0,     0,     0,   428,
       0,     0,     0,   429,     0,  1343,   430,     0,     0,   524,
     525,   526,     0,    15,     0,     0,   527,   528,     0,     0,
       0,   431,   461,   462,   529,     0,   530,   432,   531,   532,
     743,     0,   467,   468,   469,   470,   471,     0,     0,     0,
       0,     0,   472,     0,   474,     0,     0,   423,   477,     0,
       0,     0,     0,     0,     0,     0,   479,     0,     0,     0,
       0,     0,   482,     0,     0,   483,   744,     0,   484,     0,
       0,     0,   487,     0,   424,   425,     0,     0,     0,     0,
       0,     0,   594,     0,     0,   491,   492,     0,   332,   333,
     334,     0,   336,   337,   338,   339,   340,   493,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,     0,
     354,   355,   356,     0,     0,   359,   360,   361,   362,   494,
     495,   595,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   498,   499,     0,   426,     0,
       0,     0,   427,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   503,   504,
     505,   506,   507,     0,   508,     0,   509,   510,   511,   512,
       0,     0,     0,   513,   514,   515,   516,   517,   518,   519,
      63,   596,   521,   522,     0,     0,     0,     0,     0,     0,
     523,     0,     0,     0,     0,     0,     0,   428,     0,     0,
       0,   429,     0,  1359,   430,     0,     0,   524,   525,   526,
       0,    15,     0,     0,   527,   528,     0,     0,     0,   431,
     461,   462,   529,   631,   530,   432,   531,   532,   743,     0,
     467,   468,   469,   470,   471,     0,     0,     0,     0,     0,
     472,     0,   474,     0,     0,   423,   477,     0,     0,     0,
       0,     0,     0,     0,   479,     0,     0,     0,     0,     0,
     482,     0,     0,   483,   744,     0,   484,     0,     0,     0,
     487,     0,   424,   425,     0,     0,     0,     0,     0,     0,
     594,     0,     0,   491,   492,     0,   332,   333,   334,     0,
     336,   337,   338,   339,   340,   493,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,     0,   354,   355,
     356,     0,     0,   359,   360,   361,   362,   494,   495,   595,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   498,   499,     0,   426,     0,     0,     0,
     427,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   503,   504,   505,   506,
     507,     0,   508,   748,   509,   510,   511,   512,     0,     0,
       0,   513,   514,   515,   516,   517,   518,   519,   749,   596,
     521,   522,     0,     0,     0,     0,     0,     0,   523,     0,
       0,     0,     0,     0,     0,   428,     0,     0,     0,   429,
       0,  1364,   430,     0,     0,   524,   525,   526,     0,    15,
       0,     0,   527,   528,     0,     0,     0,   431,   461,   462,
     529,     0,   530,   432,   531,   532,   743,     0,   467,   468,
     469,   470,   471,     0,     0,     0,     0,     0,   472,     0,
     474,     0,     0,   423,   477,     0,     0,     0,     0,     0,
       0,     0,   479,     0,     0,     0,     0,     0,   482,     0,
       0,   483,   744,     0,   484,     0,     0,     0,   487,     0,
     424,   425,     0,     0,     0,     0,     0,     0,   594,     0,
       0,   491,   492,     0,   332,   333,   334,     0,   336,   337,
     338,   339,   340,   493,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,     0,   354,   355,   356,     0,
       0,   359,   360,   361,   362,   494,   495,   595,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   498,   499,     0,   426,     0,     0,     0,   427,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   503,   504,   505,   506,   507,     0,
     508,     0,   509,   510,   511,   512,     0,     0,     0,   513,
     514,   515,   516,   517,   518,   519,    63,   596,   521,   522,
       0,     0,     0,     0,     0,     0,   523,     0,     0,     0,
       0,     0,     0,   428,     0,     0,     0,   429,     0,  1365,
     430,     0,     0,   524,   525,   526,     0,    15,     0,     0,
     527,   528,     0,     0,     0,   431,   461,   462,   529,   867,
     530,   432,   531,   532,   743,     0,   467,   468,   469,   470,
     471,     0,     0,     0,     0,     0,   472,     0,   474,     0,
       0,     0,   477,     0,     0,     0,     0,     0,     0,     0,
     479,     0,     0,     0,     0,     0,   482,     0,     0,   483,
     744,     0,   484,     0,     0,     0,   487,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   594,     0,     0,   491,
     492,     0,   332,   333,   334,     0,   336,   337,   338,   339,
     340,   493,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,     0,   354,   355,   356,     0,     0,   359,
     360,   361,   362,   494,   495,   595,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   498,
     499,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   503,   504,   505,   506,   507,     0,   508,     0,
     509,   510,   511,   512,     0,     0,     0,   513,   514,   515,
     516,   517,   518,   519,    63,   596,   521,   522,     0,     0,
       0,     0,     0,     0,   523,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   524,   525,   526,     0,    15,     0,     0,   527,   528,
       0,     0,   461,   462,     0,     0,   529,     0,   530,     0,
     531,   532,   467,   468,   469,   470,   471,     0,     0,     0,
       0,     0,   472,     0,   474,     0,     0,     0,   477,     0,
       0,     0,     0,     0,     0,     0,   479,     0,     0,     0,
       0,     0,   482,     0,     0,   483,     0,     0,   484,     0,
       0,     0,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   594,     0,     0,   491,   492,  1152,   332,   333,
     334,     0,   336,   337,   338,   339,   340,   493,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,     0,
     354,   355,   356,     0,     0,   359,   360,   361,   362,   494,
     495,   595,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   498,   499,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   503,   504,
     505,   506,   507,     0,   508,   748,   509,   510,   511,   512,
       0,     0,     0,   513,   514,   515,   516,   517,   518,   519,
     749,   596,   521,   522,     0,     0,     0,     0,     0,     0,
     523,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   524,   525,   526,
       0,    15,     0,     0,   527,   528,     0,     0,   461,   462,
       0,     0,   529,     0,   530,     0,   531,   532,   467,   468,
     469,   470,   471,     0,     0,     0,     0,     0,   472,     0,
     474,     0,     0,     0,   477,     0,     0,     0,     0,     0,
       0,     0,   479,     0,     0,     0,     0,     0,   482,     0,
       0,   483,     0,     0,   484,     0,     0,     0,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   594,     0,
       0,   491,   492,     0,   332,   333,   334,     0,   336,   337,
     338,   339,   340,   493,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,     0,   354,   355,   356,     0,
       0,   359,   360,   361,   362,   494,   495,   595,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   498,   499,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   503,   504,   505,   506,   507,     0,
     508,   748,   509,   510,   511,   512,     0,     0,     0,   513,
     514,   515,   516,   517,   518,   519,   749,   596,   521,   522,
       0,     0,     0,     0,     0,     0,   523,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   524,   525,   526,     0,    15,     0,     0,
     527,   528,     0,     0,   461,   462,     0,     0,   529,     0,
     530,  1213,   531,   532,   467,   468,   469,   470,   471,     0,
       0,     0,     0,     0,   472,     0,   474,     0,     0,     0,
     477,     0,     0,     0,     0,     0,     0,     0,   479,     0,
       0,     0,     0,     0,   482,     0,     0,   483,     0,     0,
     484,     0,     0,     0,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   594,     0,     0,   491,   492,     0,
     332,   333,   334,     0,   336,   337,   338,   339,   340,   493,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,     0,   354,   355,   356,     0,     0,   359,   360,   361,
     362,   494,   495,   595,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,   499,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     503,   504,   505,   506,   507,     0,   508,   748,   509,   510,
     511,   512,     0,     0,     0,   513,   514,   515,   516,   517,
     518,   519,   749,   596,   521,   522,     0,     0,     0,     0,
       0,     0,   523,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   524,
     525,   526,     0,    15,     0,     0,   527,   528,     0,     0,
     461,   462,     0,     0,   529,     0,   530,  1228,   531,   532,
     467,   468,   469,   470,   471,     0,     0,     0,     0,     0,
     472,     0,   474,     0,     0,     0,   477,     0,     0,     0,
       0,     0,     0,     0,   479,     0,     0,     0,     0,     0,
     482,     0,     0,   483,     0,     0,   484,     0,     0,     0,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     594,     0,     0,   491,   492,     0,   332,   333,   334,     0,
     336,   337,   338,   339,   340,   493,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,     0,   354,   355,
     356,     0,     0,   359,   360,   361,   362,   494,   495,   595,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   498,   499,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   503,   504,   505,   506,
     507,     0,   508,     0,   509,   510,   511,   512,     0,     0,
       0,   513,   514,   515,   516,   517,   518,   519,    63,   596,
     521,   522,     0,     0,     0,     0,     0,     0,   523,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   524,   525,   526,     0,    15,
       0,     0,   527,   528,     0,     0,   461,   462,     0,     0,
     529,   631,   530,     0,   531,   532,   467,   468,   469,   470,
     471,     0,     0,     0,     0,     0,   472,     0,   474,     0,
       0,     0,   477,     0,     0,     0,     0,     0,     0,     0,
     479,     0,     0,     0,     0,     0,   482,     0,     0,   483,
       0,     0,   484,     0,     0,     0,   487,     0,     0,     0,
       0,     0,   683,     0,     0,     0,   594,     0,     0,   491,
     492,     0,   332,   333,   334,     0,   336,   337,   338,   339,
     340,   493,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,     0,   354,   355,   356,     0,     0,   359,
     360,   361,   362,   494,   495,   595,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   498,
     499,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   503,   504,   505,   506,   507,     0,   508,     0,
     509,   510,   511,   512,     0,     0,     0,   513,   514,   515,
     516,   517,   518,   519,    63,   596,   521,   522,     0,     0,
       0,     0,     0,     0,   523,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   524,   525,   526,     0,    15,     0,     0,   527,   528,
       0,     0,   461,   462,     0,     0,   529,     0,   530,     0,
     531,   532,   467,   468,   469,   470,   471,     0,     0,     0,
       0,     0,   472,     0,   474,     0,     0,     0,   477,     0,
       0,     0,     0,     0,     0,     0,   479,     0,     0,     0,
       0,     0,   482,     0,     0,   483,     0,     0,   484,     0,
       0,     0,   487,     0,     0,   694,     0,     0,     0,     0,
       0,     0,   594,     0,     0,   491,   492,     0,   332,   333,
     334,     0,   336,   337,   338,   339,   340,   493,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,     0,
     354,   355,   356,     0,     0,   359,   360,   361,   362,   494,
     495,   595,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   498,   499,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   503,   504,
     505,   506,   507,     0,   508,     0,   509,   510,   511,   512,
       0,     0,     0,   513,   514,   515,   516,   517,   518,   519,
      63,   596,   521,   522,     0,     0,     0,     0,     0,     0,
     523,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   524,   525,   526,
       0,    15,     0,     0,   527,   528,     0,     0,     0,     0,
     461,   462,   529,     0,   530,     0,   531,   532,   726,     0,
     467,   468,   469,   470,   471,     0,     0,     0,     0,     0,
     472,     0,   474,     0,     0,     0,   477,     0,     0,     0,
       0,     0,     0,     0,   479,     0,     0,     0,     0,     0,
     482,     0,     0,   483,     0,     0,   484,     0,     0,     0,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     594,     0,     0,   491,   492,     0,   332,   333,   334,     0,
     336,   337,   338,   339,   340,   493,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,     0,   354,   355,
     356,     0,     0,   359,   360,   361,   362,   494,   495,   595,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   498,   499,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   503,   504,   505,   506,
     507,     0,   508,     0,   509,   510,   511,   512,     0,     0,
       0,   513,   514,   515,   516,   517,   518,   519,    63,   596,
     521,   522,     0,     0,     0,     0,     0,     0,   523,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   524,   525,   526,     0,    15,
       0,     0,   527,   528,     0,     0,   461,   462,     0,     0,
     529,     0,   530,     0,   531,   532,   467,   468,   469,   470,
     471,     0,     0,     0,     0,     0,   472,     0,   474,     0,
       0,   423,   477,     0,     0,     0,     0,     0,     0,     0,
     479,     0,     0,     0,     0,     0,   482,     0,     0,   483,
       0,     0,   484,     0,     0,     0,   487,     0,   424,   425,
       0,     0,     0,     0,     0,     0,   594,     0,     0,   491,
     492,     0,   332,   333,   334,     0,   336,   337,   338,   339,
     340,   493,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,     0,   354,   355,   356,     0,     0,   359,
     360,   361,   362,   494,   495,   595,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   498,
     499,     0,   426,     0,     0,     0,   427,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   503,   504,   505,   506,   507,     0,   508,     0,
     509,   510,   511,   512,     0,     0,     0,   513,   514,   515,
     516,   517,   518,   519,    63,   596,   521,   522,     0,     0,
       0,     0,     0,     0,   523,     0,     0,     0,     0,     0,
       0,   428,     0,     0,     0,   429,     0,  1366,   430,   730,
       0,   524,   525,   526,     0,    15,     0,     0,   527,   528,
       0,     0,     0,   431,   461,   462,   529,     0,   530,   432,
     531,   532,   734,     0,   467,   468,   469,   470,   471,     0,
       0,     0,     0,     0,   472,     0,   474,     0,     0,     0,
     477,     0,     0,     0,     0,     0,     0,     0,   479,     0,
       0,     0,     0,     0,   482,     0,     0,   483,     0,     0,
     484,     0,     0,     0,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   594,     0,     0,   491,   492,     0,
     332,   333,   334,     0,   336,   337,   338,   339,   340,   493,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,     0,   354,   355,   356,     0,     0,   359,   360,   361,
     362,   494,   495,   595,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,   499,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     503,   504,   505,   506,   507,     0,   508,     0,   509,   510,
     511,   512,     0,     0,     0,   513,   514,   515,   516,   517,
     518,   519,    63,   596,   521,   522,     0,     0,     0,     0,
       0,     0,   523,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   524,
     525,   526,     0,    15,     0,     0,   527,   528,     0,     0,
     461,   462,     0,     0,   529,     0,   530,     0,   531,   532,
     467,   468,   469,   470,   471,     0,     0,     0,     0,     0,
     472,     0,   474,     0,     0,     0,   477,     0,     0,     0,
       0,     0,     0,     0,   479,     0,     0,     0,     0,     0,
     482,     0,     0,   483,     0,     0,   484,     0,     0,     0,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     594,     0,     0,   491,   492,     0,   332,   333,   334,     0,
     336,   337,   338,   339,   340,   493,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,     0,   354,   355,
     356,     0,     0,   359,   360,   361,   362,   494,   495,   595,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   498,   499,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   503,   504,   505,   506,
     507,     0,   508,   748,   509,   510,   511,   512,     0,     0,
       0,   513,   514,   515,   516,   517,   518,   519,   749,   596,
     521,   522,     0,     0,     0,     0,     0,     0,   523,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   524,   525,   526,     0,    15,
       0,     0,   527,   528,     0,     0,   461,   462,     0,     0,
     529,     0,   530,     0,   531,   532,   467,   468,   469,   470,
     471,     0,     0,  1085,     0,     0,   472,     0,   474,     0,
       0,     0,   477,     0,     0,     0,     0,     0,     0,     0,
     479,     0,     0,     0,     0,     0,   482,     0,     0,   483,
       0,     0,   484,     0,     0,     0,   487,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   594,     0,     0,   491,
     492,     0,   332,   333,   334,     0,   336,   337,   338,   339,
     340,   493,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,     0,   354,   355,   356,     0,     0,   359,
     360,   361,   362,   494,   495,   595,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   498,
     499,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   503,   504,   505,   506,   507,     0,   508,     0,
     509,   510,   511,   512,     0,     0,     0,   513,   514,   515,
     516,   517,   518,   519,    63,   596,   521,   522,     0,     0,
       0,     0,     0,     0,   523,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   524,   525,   526,     0,    15,     0,     0,   527,   528,
       0,     0,   461,   462,     0,     0,   529,     0,   530,     0,
     531,   532,   467,   468,   469,   470,   471,     0,     0,     0,
       0,     0,   472,     0,   474,     0,     0,     0,   477,     0,
       0,     0,     0,     0,     0,     0,   479,     0,     0,     0,
       0,     0,   482,     0,     0,   483,     0,     0,   484,     0,
       0,     0,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   594,     0,     0,   491,   492,     0,   332,   333,
     334,     0,   336,   337,   338,   339,   340,   493,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,     0,
     354,   355,   356,     0,     0,   359,   360,   361,   362,   494,
     495,   595,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   498,   499,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   503,   504,
     505,   506,   507,     0,   508,     0,   509,   510,   511,   512,
       0,     0,     0,   513,   514,   515,   516,   517,   518,   519,
      63,   596,   521,   522,     0,     0,     0,     0,     0,     0,
     523,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   524,   525,   526,
       0,    15,     0,     0,   527,   528,     0,     0,   461,   462,
       0,     0,   529,     0,   530,  1106,   531,   532,   467,   468,
     469,   470,   471,     0,     0,     0,     0,     0,   472,     0,
     474,     0,     0,     0,   477,     0,     0,     0,     0,     0,
       0,     0,   479,     0,     0,     0,     0,     0,   482,     0,
       0,   483,     0,     0,   484,     0,     0,     0,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   594,     0,
       0,   491,   492,     0,   332,   333,   334,     0,   336,   337,
     338,   339,   340,   493,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,     0,   354,   355,   356,     0,
       0,   359,   360,   361,   362,   494,   495,   595,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   498,   499,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   503,   504,   505,   506,   507,     0,
     508,     0,   509,   510,   511,   512,     0,     0,     0,   513,
     514,   515,   516,   517,   518,   519,    63,   596,   521,   522,
       0,     0,     0,     0,     0,     0,   523,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1156,     0,   524,   525,   526,     0,    15,     0,     0,
     527,   528,     0,     0,   461,   462,     0,     0,   529,     0,
     530,     0,   531,   532,   467,   468,   469,   470,   471,     0,
       0,     0,     0,     0,   472,     0,   474,     0,     0,     0,
     477,     0,     0,     0,     0,     0,     0,     0,   479,     0,
       0,     0,     0,     0,   482,     0,     0,   483,     0,     0,
     484,     0,     0,     0,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   594,     0,     0,   491,   492,     0,
     332,   333,   334,     0,   336,   337,   338,   339,   340,   493,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,     0,   354,   355,   356,     0,     0,   359,   360,   361,
     362,   494,   495,   595,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,   499,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     503,   504,   505,   506,   507,     0,   508,     0,   509,   510,
     511,   512,     0,     0,     0,   513,   514,   515,   516,   517,
     518,   519,    63,   596,   521,   522,     0,     0,     0,     0,
       0,     0,   523,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   524,
     525,   526,     0,    15,     0,     0,   527,   528,     0,     0,
     461,   462,     0,     0,   529,     0,   530,  1434,   531,   532,
     467,   468,   469,   470,   471,     0,     0,     0,     0,     0,
     472,     0,   474,     0,     0,     0,   477,     0,     0,     0,
       0,     0,     0,     0,   479,     0,     0,     0,     0,     0,
     482,     0,     0,   483,     0,     0,   484,     0,     0,     0,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     594,     0,     0,   491,   492,     0,   332,   333,   334,     0,
     336,   337,   338,   339,   340,   493,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,     0,   354,   355,
     356,     0,     0,   359,   360,   361,   362,   494,   495,   595,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   498,   499,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   503,   504,   505,   506,
     507,     0,   508,     0,   509,   510,   511,   512,     0,     0,
       0,   513,   514,   515,   516,   517,   518,   519,    63,   596,
     521,   522,     0,     0,     0,     0,     0,     0,   523,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   524,   525,   526,     0,    15,
       0,     0,   527,   528,     0,     0,   461,   462,     0,     0,
    1443,     0,   530,  1444,   531,   532,   467,   468,   469,   470,
     471,     0,     0,     0,     0,     0,   472,     0,   474,     0,
       0,     0,   477,     0,     0,     0,     0,     0,     0,     0,
     479,     0,     0,     0,     0,     0,   482,     0,     0,   483,
       0,     0,   484,     0,     0,     0,   487,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   594,     0,     0,   491,
     492,     0,   332,   333,   334,     0,   336,   337,   338,   339,
     340,   493,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,     0,   354,   355,   356,     0,     0,   359,
     360,   361,   362,   494,   495,   595,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   498,
     499,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   503,   504,   505,   506,   507,     0,   508,     0,
     509,   510,   511,   512,     0,     0,     0,   513,   514,   515,
     516,   517,   518,   519,    63,   596,   521,   522,     0,     0,
       0,     0,     0,     0,   523,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   524,   525,   526,     0,    15,     0,     0,   527,   528,
       0,     0,   461,   462,     0,     0,   529,     0,   530,  1449,
     531,   532,   467,   468,   469,   470,   471,     0,     0,     0,
       0,     0,   472,     0,   474,     0,     0,     0,   477,     0,
       0,     0,     0,     0,     0,     0,   479,     0,     0,     0,
       0,     0,   482,     0,     0,   483,     0,     0,   484,     0,
       0,     0,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   594,     0,     0,   491,   492,     0,   332,   333,
     334,     0,   336,   337,   338,   339,   340,   493,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,     0,
     354,   355,   356,     0,     0,   359,   360,   361,   362,   494,
     495,   595,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   498,   499,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   503,   504,
     505,   506,   507,     0,   508,     0,   509,   510,   511,   512,
       0,     0,     0,   513,   514,   515,   516,   517,   518,   519,
      63,   596,   521,   522,     0,     0,     0,     0,     0,     0,
     523,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   524,   525,   526,
       0,    15,     0,     0,   527,   528,     0,     0,   461,   462,
       0,     0,   529,     0,   530,  1491,   531,   532,   467,   468,
     469,   470,   471,     0,     0,     0,     0,     0,   472,     0,
     474,     0,     0,     0,   477,     0,     0,     0,     0,     0,
       0,     0,   479,     0,     0,     0,     0,     0,   482,     0,
       0,   483,     0,     0,   484,     0,     0,     0,   487,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   594,     0,
       0,   491,   492,     0,   332,   333,   334,     0,   336,   337,
     338,   339,   340,   493,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,     0,   354,   355,   356,     0,
       0,   359,   360,   361,   362,   494,   495,   595,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   498,   499,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   503,   504,   505,   506,   507,     0,
     508,     0,   509,   510,   511,   512,     0,     0,     0,   513,
     514,   515,   516,   517,   518,   519,    63,   596,   521,   522,
       0,     0,     0,     0,     0,     0,   523,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   524,   525,   526,     0,    15,     0,     0,
     527,   528,     0,     0,   461,   462,     0,     0,   529,     0,
     530,  1572,   531,   532,   467,   468,   469,   470,   471,     0,
       0,     0,     0,     0,   472,     0,   474,     0,     0,     0,
     477,     0,     0,     0,     0,     0,     0,     0,   479,     0,
       0,     0,     0,     0,   482,     0,     0,   483,     0,     0,
     484,     0,     0,     0,   487,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   594,     0,     0,   491,   492,     0,
     332,   333,   334,     0,   336,   337,   338,   339,   340,   493,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,     0,   354,   355,   356,     0,     0,   359,   360,   361,
     362,   494,   495,   595,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   498,   499,     0,
       0,     0,     0,     0,     0,     0,  1609,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    62,     0,     0,     0,     0,     0,     0,     0,
     503,   504,   505,   506,   507,     0,   508,     0,   509,   510,
     511,   512,     0,     0,     0,   513,   514,   515,   516,   517,
     518,   519,    63,   596,   521,   522,     0,     0,     0,     0,
       0,     0,   523,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   524,
     525,   526,     0,    15,     0,     0,   527,   528,     0,     0,
     461,   462,     0,     0,   529,     0,   530,     0,   531,   532,
     467,   468,   469,   470,   471,     0,     0,     0,     0,     0,
     472,     0,   474,     0,     0,     0,   477,     0,     0,     0,
       0,     0,     0,     0,   479,     0,     0,     0,     0,     0,
     482,     0,     0,   483,     0,     0,   484,     0,     0,     0,
     487,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     594,     0,     0,   491,   492,     0,   332,   333,   334,     0,
     336,   337,   338,   339,   340,   493,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   352,     0,   354,   355,
     356,     0,     0,   359,   360,   361,   362,   494,   495,   595,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   498,   499,     0,     0,     0,     0,     0,
       0,     0,  1652,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    62,     0,
       0,     0,     0,     0,     0,     0,   503,   504,   505,   506,
     507,     0,   508,     0,   509,   510,   511,   512,     0,     0,
       0,   513,   514,   515,   516,   517,   518,   519,    63,   596,
     521,   522,     0,     0,     0,     0,     0,     0,   523,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   524,   525,   526,     0,    15,
       0,     0,   527,   528,     0,     0,   461,   462,     0,     0,
     529,     0,   530,     0,   531,   532,   467,   468,   469,   470,
     471,     0,     0,     0,     0,     0,   472,     0,   474,     0,
       0,     0,   477,     0,     0,     0,     0,     0,     0,     0,
     479,     0,     0,     0,     0,     0,   482,     0,     0,   483,
       0,     0,   484,     0,     0,     0,   487,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   594,     0,     0,   491,
     492,     0,   332,   333,   334,     0,   336,   337,   338,   339,
     340,   493,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,     0,   354,   355,   356,     0,     0,   359,
     360,   361,   362,   494,   495,   595,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   498,
     499,     0,     0,     0,     0,     0,     0,     0,  1653,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    62,     0,     0,     0,     0,     0,
       0,     0,   503,   504,   505,   506,   507,     0,   508,     0,
     509,   510,   511,   512,     0,     0,     0,   513,   514,   515,
     516,   517,   518,   519,    63,   596,   521,   522,     0,     0,
       0,     0,     0,     0,   523,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   524,   525,   526,     0,    15,     0,     0,   527,   528,
       0,     0,   461,   462,     0,     0,   529,     0,   530,     0,
     531,   532,   467,   468,   469,   470,   471,     0,     0,     0,
       0,     0,   472,     0,   474,     0,     0,     0,   477,     0,
       0,     0,     0,     0,     0,     0,   479,     0,     0,     0,
       0,     0,   482,     0,     0,   483,     0,     0,   484,     0,
       0,     0,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   594,     0,     0,   491,   492,     0,   332,   333,
     334,     0,   336,   337,   338,   339,   340,   493,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,     0,
     354,   355,   356,     0,     0,   359,   360,   361,   362,   494,
     495,   595,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   498,   499,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      62,     0,     0,     0,     0,     0,     0,     0,   503,   504,
     505,   506,   507,     0,   508,     0,   509,   510,   511,   512,
       0,     0,     0,   513,   514,   515,   516,   517,   518,   519,
      63,   596,   521,   522,     0,     0,     0,     0,     0,     0,
     523,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   524,   525,   526,
       0,    15,     0,     0,   527,   528,     0,     0,   461,   462,
       0,     0,   529,     0,   530,     0,   531,   532,   467,   468,
     469,   470,   471,     0,     0,     0,     0,     0,   472,     0,
     474,     0,     0,   423,   477,     0,     0,     0,     0,     0,
       0,     0,   479,     0,     0,     0,     0,     0,   482,     0,
       0,   483,     0,     0,   484,     0,     0,     0,   487,     0,
     424,   425,     0,     0,     0,     0,     0,     0,   594,     0,
       0,   491,   492,     0,   332,   333,   334,     0,   336,   337,
     338,   339,   340,   493,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,     0,   354,   355,   356,     0,
       0,   359,   360,   361,   362,   494,   495,   595,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   498,   499,     0,   426,     0,   -78,     0,   427,     0,
       0,     0,     0,     0,     0,     0,     0,   773,   774,     0,
       0,     0,     0,     0,     0,     0,    62,     0,     0,     0,
       0,     0,     0,     0,   503,   504,   505,   506,   507,     0,
     508,     0,   509,   510,   511,   512,     0,     0,     0,   513,
     514,   515,   516,   517,   518,   519,    63,   596,   521,   522,
       0,     0,     0,     0,   773,   774,   523,     0,     0,     0,
       0,     0,     0,   428,     0,     0,     0,   429,     0,  1367,
     430,     0,     0,   524,   525,   526,     0,    15,     0,     0,
     527,   528,     0,     0,     0,   431,     0,     0,  1417,     0,
     530,   432,   531,   532,   775,   776,   777,   778,   779,   780,
     781,   782,   783,   784,   785,   786,   787,   788,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,     0,     0,
       0,     0,   799,   800,   801,   802,   803,   804,     0,     0,
     805,   806,   807,   808,   809,   810,   811,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   783,
     784,   785,   786,   787,     0,     0,   790,   791,   792,   793,
       0,   795,   796,   797,   798,     0,     0,     0,     0,   799,
       0,   801,   802,     0,     0,     0,     0,   805,   812,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,   724,     0,     0,     0,     0,     0,   326,     0,     0,
     824,   825,     0,   327,     0,   531,   710,     0,     0,   328,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   329,
       0,     0,     0,     0,     0,     0,     0,   330,   814,   815,
     816,   817,   818,   819,   820,   821,   822,   823,     0,     0,
       0,     0,   331,     0,     0,     0,     0,   824,   825,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     326,     0,     0,     0,     0,     0,   327,     0,     0,     0,
       0,     0,   328,     0,     0,     0,     0,     0,     0,     0,
       0,    62,   329,     0,     0,     0,     0,     0,     0,     0,
     330,     0,     0,     0,   365,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   331,     0,     0,     0,     0,
       0,    63,   332,   333,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   366,     0,   423,
       0,     0,     0,   326,     0,     0,     0,     0,     0,   327,
       0,     0,   423,     0,     0,   328,     0,     0,     0,     0,
       0,     0,     0,     0,    62,   329,   424,   425,     0,     0,
       0,     0,     0,   330,     0,     0,     0,   365,     0,   424,
     425,     0,     0,     0,     0,     0,     0,     0,   331,     0,
       0,     0,     0,     0,    63,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,     0,     0,
     426,     0,     0,   423,   427,     0,     0,     0,   423,     0,
     366,     0,   634,   426,     0,     0,     0,   427,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     424,   425,     0,     0,     0,   424,   425,    62,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     365,     0,     0,     0,     0,     0,     0,     0,     0,    13,
       0,     0,   326,     0,     0,     0,     0,   639,   327,   428,
       0,     0,     0,   429,   328,  1369,   430,     0,     0,    14,
       0,     0,   428,     0,   329,   640,   429,     0,  1475,   430,
       0,   431,   330,     0,   426,     0,     0,   432,   427,   426,
       0,     0,     0,   427,   431,     0,     0,   331,     0,     0,
     432,     0,     0,   366,   332,   333,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   428,     0,   326,     0,   429,   428,  1480,
     430,   327,   429,     0,  1514,   430,     0,   328,     0,     0,
       0,     0,     0,     0,     0,   431,    62,   329,     0,     0,
     431,   432,     0,     0,     0,   330,   432,     0,     0,   365,
       0,     0,     0,     0,     0,     0,     0,     0,   773,   774,
     331,     0,     0,     0,     0,     0,    63,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
       0,   773,   774,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   366,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    62,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   365,   783,   784,   785,   786,   787,     0,     0,
     790,   791,   792,   793,     0,   795,   796,   797,   798,   639,
       0,     0,     0,   799,     0,   801,   802,     0,     0,     0,
       0,   805,     0,   807,     0,     0,     0,     0,   905,   906,
     907,   908,   909,   910,   911,   912,   783,   784,   785,   786,
     787,   913,   914,   790,   791,   792,   793,   915,   795,   796,
     797,   798,     0,     0,     0,   366,   799,   800,   801,   802,
     916,   917,   773,   774,   805,   806,   807,   918,   919,   920,
     811,     0,   814,   815,   816,   817,   818,   819,   820,   821,
     822,   823,     0,     0,     0,    13,     0,     0,     0,     0,
       0,   824,   825,     0,     0,     0,     0,   773,   774,     0,
       0,     0,     0,     0,     0,    14,     0,     0,     0,     0,
       0,     0,   921,   813,     0,   814,   815,   816,   817,   818,
     819,   820,   821,   822,   823,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   824,   825,     0,     0,     0,   531,
     710,     0,     0,     0,     0,     0,     0,     0,     0,   905,
     906,   907,   908,   909,   910,   911,   912,   783,   784,   785,
     786,   787,   913,   914,   790,   791,   792,   793,   915,   795,
     796,   797,   798,  -391,     0,     0,     0,   799,   800,   801,
     802,   916,   917,     0,     0,   805,   806,   807,   918,   919,
     920,   811,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,   773,   774,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,     0,   811,     0,     0,     0,
       0,     0,     0,   921,   813,     0,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,   773,   774,     0,     0,
       0,     0,     0,     0,     0,   824,   825,     0,     0,     0,
     531,   710,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,   423,     0,     0,     0,     0,     0,     0,     0,     0,
     824,   825,     0,     0,   864,     0,     0,     0,     0,     0,
       0,     0,     0,   783,   784,   785,   786,   787,   424,   425,
     790,   791,   792,   793,     0,   795,   796,   797,   798,     0,
       0,     0,     0,   799,     0,   801,   802,     0,     0,     0,
       0,   805,   806,   807,     0,     0,     0,   811,     0,     0,
       0,   783,   784,   785,   786,   787,     0,     0,   790,   791,
     792,   793,     0,   795,   796,   797,   798,   773,   774,     0,
       0,   799,     0,   801,   802,     0,     0,     0,     0,   805,
     806,   807,   426,     0,     0,   811,   427,     0,     0,     0,
     813,     0,   814,   815,   816,   817,   818,   819,   820,   821,
     822,   823,   773,   774,     0,     0,     0,     0,     0,     0,
       0,   824,   825,     0,     0,   878,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   813,     0,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,   428,     0,  1188,     0,   429,     0,  1611,   430,     0,
       0,     0,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,   431,   795,   796,   797,   798,     0,   432,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,     0,   811,   783,   784,   785,
     786,   787,     0,     0,   790,   791,   792,   793,     0,   795,
     796,   797,   798,   773,   774,     0,     0,   799,     0,   801,
     802,     0,     0,     0,     0,   805,   806,   807,     0,     0,
       0,   811,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,   773,   774,     0,     0,     0,     0,     0,     0,     0,
     824,   825,     0,     0,  1192,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   813,     0,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   824,   825,     0,     0,  1194,
       0,     0,     0,     0,     0,     0,     0,     0,   783,   784,
     785,   786,   787,     0,     0,   790,   791,   792,   793,     0,
     795,   796,   797,   798,     0,     0,     0,     0,   799,     0,
     801,   802,     0,     0,     0,     0,   805,   806,   807,     0,
       0,     0,   811,     0,     0,     0,   783,   784,   785,   786,
     787,     0,     0,   790,   791,   792,   793,     0,   795,   796,
     797,   798,   773,   774,     0,     0,   799,     0,   801,   802,
       0,     0,     0,     0,   805,   806,   807,     0,     0,     0,
     811,     0,     0,     0,     0,   813,     0,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,   773,   774,     0,
       0,     0,     0,     0,     0,     0,   824,   825,     0,     0,
    1203,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   813,     0,   814,   815,   816,   817,   818,
     819,   820,   821,   822,   823,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   824,   825,     0,     0,  1204,     0,
       0,     0,     0,     0,     0,     0,     0,   783,   784,   785,
     786,   787,     0,     0,   790,   791,   792,   793,     0,   795,
     796,   797,   798,     0,     0,     0,     0,   799,     0,   801,
     802,     0,     0,     0,     0,   805,   806,   807,     0,     0,
       0,   811,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,   773,   774,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,     0,   811,     0,     0,     0,
       0,     0,     0,     0,   813,     0,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,   773,   774,     0,     0,
       0,     0,     0,     0,     0,   824,   825,     0,     0,  1205,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     824,   825,     0,     0,  1206,     0,     0,     0,     0,     0,
       0,     0,     0,   783,   784,   785,   786,   787,     0,     0,
     790,   791,   792,   793,     0,   795,   796,   797,   798,     0,
       0,     0,     0,   799,     0,   801,   802,     0,     0,     0,
       0,   805,   806,   807,     0,     0,     0,   811,     0,     0,
       0,   783,   784,   785,   786,   787,     0,     0,   790,   791,
     792,   793,     0,   795,   796,   797,   798,   773,   774,     0,
       0,   799,     0,   801,   802,     0,     0,     0,     0,   805,
     806,   807,     0,     0,     0,   811,     0,     0,     0,     0,
     813,     0,   814,   815,   816,   817,   818,   819,   820,   821,
     822,   823,   773,   774,     0,     0,     0,     0,     0,     0,
       0,   824,   825,     0,     0,  1207,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   813,     0,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,     0,     0,  1208,     0,     0,     0,     0,     0,     0,
       0,     0,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,     0,     0,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,     0,   811,   783,   784,   785,
     786,   787,     0,     0,   790,   791,   792,   793,     0,   795,
     796,   797,   798,   773,   774,     0,     0,   799,     0,   801,
     802,     0,     0,     0,     0,   805,   806,   807,     0,     0,
       0,   811,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,   773,   774,     0,     0,     0,     0,     0,     0,     0,
     824,   825,     0,     0,  1339,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   813,     0,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   824,   825,     0,     0,  1351,
       0,     0,     0,     0,     0,     0,     0,     0,   783,   784,
     785,   786,   787,     0,     0,   790,   791,   792,   793,     0,
     795,   796,   797,   798,     0,     0,     0,     0,   799,     0,
     801,   802,     0,     0,     0,     0,   805,   806,   807,     0,
       0,     0,   811,     0,     0,     0,   783,   784,   785,   786,
     787,     0,     0,   790,   791,   792,   793,     0,   795,   796,
     797,   798,   773,   774,     0,     0,   799,     0,   801,   802,
       0,     0,     0,     0,   805,   806,   807,     0,     0,     0,
     811,     0,     0,     0,     0,   813,     0,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,   773,   774,     0,
       0,     0,     0,     0,     0,     0,   824,   825,     0,     0,
    1354,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   813,     0,   814,   815,   816,   817,   818,
     819,   820,   821,   822,   823,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   824,   825,     0,     0,  1401,     0,
       0,     0,     0,     0,     0,     0,     0,   783,   784,   785,
     786,   787,     0,     0,   790,   791,   792,   793,     0,   795,
     796,   797,   798,     0,     0,     0,     0,   799,     0,   801,
     802,     0,     0,     0,     0,   805,   806,   807,     0,     0,
       0,   811,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,   773,   774,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,     0,   811,     0,     0,     0,
       0,     0,     0,     0,   813,     0,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,   773,   774,     0,     0,
       0,     0,     0,     0,     0,   824,   825,     0,     0,  1512,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     824,   825,     0,     0,  1513,     0,     0,     0,     0,     0,
       0,     0,     0,   783,   784,   785,   786,   787,     0,     0,
     790,   791,   792,   793,     0,   795,   796,   797,   798,     0,
       0,     0,     0,   799,     0,   801,   802,     0,     0,     0,
       0,   805,   806,   807,     0,     0,     0,   811,     0,     0,
       0,   783,   784,   785,   786,   787,     0,     0,   790,   791,
     792,   793,     0,   795,   796,   797,   798,   773,   774,     0,
       0,   799,     0,   801,   802,     0,     0,     0,     0,   805,
     806,   807,     0,     0,     0,   811,     0,     0,     0,     0,
     813,     0,   814,   815,   816,   817,   818,   819,   820,   821,
     822,   823,   773,   774,     0,     0,     0,     0,     0,     0,
       0,   824,   825,     0,     0,  1534,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   813,     0,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,     0,     0,  1536,     0,     0,     0,     0,     0,     0,
       0,     0,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,     0,     0,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,     0,   811,   783,   784,   785,
     786,   787,     0,     0,   790,   791,   792,   793,     0,   795,
     796,   797,   798,   773,   774,     0,     0,   799,     0,   801,
     802,     0,     0,     0,     0,   805,   806,   807,     0,     0,
       0,   811,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,   773,   774,     0,     0,     0,     0,     0,     0,     0,
     824,   825,     0,     0,  1538,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   813,     0,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   824,   825,     0,     0,  1542,
       0,     0,     0,     0,     0,     0,     0,     0,   783,   784,
     785,   786,   787,     0,     0,   790,   791,   792,   793,     0,
     795,   796,   797,   798,     0,     0,     0,     0,   799,     0,
     801,   802,     0,     0,     0,     0,   805,   806,   807,     0,
       0,     0,   811,     0,     0,     0,   783,   784,   785,   786,
     787,     0,     0,   790,   791,   792,   793,     0,   795,   796,
     797,   798,   773,   774,     0,     0,   799,     0,   801,   802,
       0,     0,     0,     0,   805,   806,   807,     0,     0,     0,
     811,     0,     0,     0,     0,   813,     0,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,   773,   774,     0,
       0,     0,     0,     0,     0,     0,   824,   825,     0,     0,
    1590,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   813,     0,   814,   815,   816,   817,   818,
     819,   820,   821,   822,   823,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   824,   825,     0,     0,  1614,     0,
       0,     0,     0,     0,     0,     0,     0,   783,   784,   785,
     786,   787,     0,     0,   790,   791,   792,   793,     0,   795,
     796,   797,   798,     0,     0,     0,     0,   799,     0,   801,
     802,     0,     0,     0,     0,   805,   806,   807,     0,     0,
       0,   811,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,   773,   774,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,     0,   811,     0,     0,     0,
       0,     0,     0,     0,   813,     0,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,   773,   774,     0,     0,
       0,     0,     0,     0,     0,   824,   825,     0,     0,  1615,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     824,   825,     0,     0,  1617,     0,     0,     0,     0,     0,
       0,     0,     0,   783,   784,   785,   786,   787,     0,     0,
     790,   791,   792,   793,     0,   795,   796,   797,   798,     0,
       0,     0,     0,   799,     0,   801,   802,     0,     0,     0,
       0,   805,   806,   807,     0,     0,     0,   811,     0,     0,
       0,   783,   784,   785,   786,   787,     0,     0,   790,   791,
     792,   793,     0,   795,   796,   797,   798,   773,   774,     0,
       0,   799,     0,   801,   802,     0,     0,     0,     0,   805,
     806,   807,     0,     0,     0,   811,     0,     0,     0,     0,
     813,     0,   814,   815,   816,   817,   818,   819,   820,   821,
     822,   823,   773,   774,     0,     0,     0,     0,     0,     0,
       0,   824,   825,     0,     0,  1626,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   813,     0,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,     0,     0,  1629,     0,     0,     0,     0,     0,     0,
       0,     0,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,     0,     0,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,     0,   811,   783,   784,   785,
     786,   787,     0,     0,   790,   791,   792,   793,     0,   795,
     796,   797,   798,   773,   774,     0,     0,   799,     0,   801,
     802,     0,     0,     0,     0,   805,   806,   807,     0,     0,
       0,   811,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,   773,   774,     0,     0,     0,     0,     0,     0,     0,
     824,   825,     0,     0,  1638,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   813,     0,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   824,   825,     0,     0,  1713,
       0,     0,     0,     0,     0,     0,     0,     0,   783,   784,
     785,   786,   787,     0,     0,   790,   791,   792,   793,     0,
     795,   796,   797,   798,     0,     0,     0,     0,   799,     0,
     801,   802,     0,     0,     0,     0,   805,   806,   807,     0,
       0,     0,   811,     0,     0,     0,   783,   784,   785,   786,
     787,     0,     0,   790,   791,   792,   793,     0,   795,   796,
     797,   798,   773,   774,     0,     0,   799,     0,   801,   802,
       0,     0,     0,     0,   805,   806,   807,     0,     0,     0,
     811,     0,     0,     0,     0,   813,     0,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,   773,   774,     0,
       0,     0,     0,     0,     0,     0,   824,   825,     0,     0,
    1714,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   813,     0,   814,   815,   816,   817,   818,
     819,   820,   821,   822,   823,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   824,   825,   868,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   783,   784,   785,
     786,   787,     0,     0,   790,   791,   792,   793,     0,   795,
     796,   797,   798,     0,     0,     0,     0,   799,     0,   801,
     802,     0,     0,     0,     0,   805,   806,   807,     0,     0,
       0,   811,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,   773,   774,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,     0,   811,     0,     0,     0,
       0,     0,     0,     0,   813,     0,   814,   815,   816,   817,
     818,   819,   820,   821,   822,   823,   773,   774,     0,     0,
       0,     0,     0,     0,     0,   824,   825,  1140,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     824,   825,  1290,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   783,   784,   785,   786,   787,     0,     0,
     790,   791,   792,   793,     0,   795,   796,   797,   798,     0,
       0,     0,     0,   799,     0,   801,   802,     0,     0,     0,
       0,   805,   806,   807,     0,     0,     0,   811,     0,     0,
       0,   783,   784,   785,   786,   787,     0,     0,   790,   791,
     792,   793,     0,   795,   796,   797,   798,   773,   774,     0,
       0,   799,     0,   801,   802,     0,     0,     0,     0,   805,
     806,   807,     0,     0,     0,   811,     0,     0,     0,     0,
     813,     0,   814,   815,   816,   817,   818,   819,   820,   821,
     822,   823,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   824,   825,  1306,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   813,     0,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,  1441,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   783,   784,   785,   786,   787,     0,     0,   790,
     791,   792,   793,     0,   795,   796,   797,   798,   245,   246,
       0,     0,   799,     0,   801,   802,     0,     0,     0,     0,
     805,   806,   807,     0,     0,   247,   811,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     773,   774,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   813,
       0,   814,   815,   816,   817,   818,   819,   820,   821,   822,
     823,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     824,   825,  1447,     0,     0,   248,   249,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,     0,     0,   266,   267,   268,     0,     0,
       0,     0,     0,     0,   269,   270,   271,   272,   273,     0,
       0,   274,   275,   276,   277,   278,   279,   280,     0,     0,
       0,     0,     0,   773,   774,   783,   784,   785,   786,   787,
       0,     0,   790,   791,   792,   793,     0,   795,   796,   797,
     798,     0,     0,     0,     0,   799,     0,   801,   802,     0,
       0,     0,     0,   805,   806,   807,     0,     0,     0,   811,
     281,     0,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,    50,     0,   292,   293,     0,     0,     0,     0,
       0,   294,   295,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    51,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   813,     0,   814,   815,   816,   817,   818,   819,
     820,   821,   822,   823,     0,     0,   773,   774,   783,   784,
     785,   786,   787,   824,   825,   790,   791,   792,   793,     0,
     795,   796,   797,   798,     0,     0,     0,     0,   799,     0,
     801,   802,     0,     0,     0,     0,   805,   806,   807,     0,
       0,     0,   811,   773,   774,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    13,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    14,     0,     0,
       0,     0,     0,     0,     0,   813,     0,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,     0,     0,     0,
       0,   783,   784,   785,   786,   787,   824,   825,   790,   791,
     792,   793,     0,   795,   796,   797,   798,     0,     0,     0,
       0,   799,     0,   801,   802,     0,     0,   969,     0,   805,
     806,   807,     0,     0,     0,   811,   773,   774,   783,   784,
     785,   786,   787,     0,     0,   790,   791,   792,   793,     0,
     795,   796,   797,   798,     0,     0,     0,     0,   799,     0,
     801,   802,     0,     0,  1214,     0,   805,   806,   807,     0,
       0,     0,   811,   773,   774,     0,     0,     0,   813,     0,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   813,     0,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,     0,     0,     0,
       0,   783,   784,   785,   786,   787,   824,   825,   790,   791,
     792,   793,     0,   795,   796,   797,   798,     0,     0,     0,
       0,   799,     0,   801,   802,     0,     0,     0,     0,   805,
     806,   807,     0,     0,     0,   811,   773,   774,   783,   784,
     785,   786,   787,     0,     0,   790,   791,   792,   793,     0,
     795,   796,   797,   798,     0,     0,     0,     0,   799,     0,
     801,   802,     0,     0,     0,     0,   805,   806,   807,     0,
       0,     0,   811,   773,   774,     0,     0,     0,   813,  1295,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   813,  1396,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,     0,     0,     0,
       0,   783,   784,   785,   786,   787,   824,   825,   790,   791,
     792,   793,     0,   795,   796,   797,   798,     0,     0,     0,
       0,   799,     0,   801,   802,     0,     0,     0,     0,   805,
     806,   807,     0,     0,     0,   811,   773,   774,   783,   784,
     785,   786,   787,     0,     0,   790,   791,   792,   793,     0,
     795,   796,   797,   798,     0,     0,     0,     0,   799,     0,
     801,   802,     0,     0,     0,     0,   805,   806,   807,     0,
    1639,     0,   811,   773,   774,     0,     0,     0,   813,     0,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   813,     0,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,     0,     0,     0,
       0,   783,   784,   785,   786,   787,   824,   825,   790,   791,
     792,   793,     0,   795,   796,   797,   798,     0,     0,     0,
       0,   799,     0,   801,   802,     0,     0,     0,     0,   805,
     806,   807,     0,     0,     0,  -798,   773,   774,   783,   784,
     785,   786,   787,     0,     0,   790,   791,   792,   793,     0,
     795,   796,   797,   798,     0,     0,     0,     0,   799,     0,
     801,   802,   773,   774,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   813,     0,
     814,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,     0,     0,     0,
       0,   783,   784,   785,   786,   787,   824,   825,   790,   791,
     792,   793,     0,   795,   796,   797,   798,     0,     0,     0,
       0,   799,     0,   801,   802,     0,     0,   783,   784,   785,
     786,   787,     0,     0,   790,   791,   792,   793,     0,   795,
     796,   797,   798,     0,     0,     0,     0,   799,     0,   801,
     802,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1018,     0,     0,     0,     0,
       0,   815,   816,   817,   818,   819,   820,   821,   822,   823,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   824,
     825,     0,     0,     0,     0,     0,  1022,     0,   816,   817,
     818,   819,   820,   821,   822,   823,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   824,   825,   332,   333,   334,
       0,   336,   337,   338,   339,   340,   493,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,     0,   354,
     355,   356,     0,     0,   359,   360,   361,   362,   332,   333,
     334,     0,   336,   337,   338,   339,   340,   493,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,     0,
     354,   355,   356,     0,     0,   359,   360,   361,   362,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1291,     0,     0,     0,     0,
       0,     0,     0,     0,  1019,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1020,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1023,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   332,   333,   334,
    1024,   336,   337,   338,   339,   340,   493,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,     0,   354,
     355,   356,     0,     0,   359,   360,   361,   362,   332,   333,
     334,     0,   336,   337,   338,   339,   340,   493,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,     0,
     354,   355,   356,     0,   194,   359,   360,   361,   362,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1292,     0,  1097,  1098,     0,     0,
     195,     0,   196,     0,   197,   198,   199,   200,   201,  1293,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,     0,   213,   214,   215,  1099,     0,   216,   217,   218,
     219,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1100,     0,     0,     0,     0,     0,     0,   220,   221,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1101,  1102,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   222
};

static const yytype_int16 yycheck[] =
{
       1,   473,   655,   392,   601,   224,   392,   624,   682,   550,
     746,   437,   399,   512,   750,   751,   436,   171,    19,    15,
      16,    86,   725,  1217,   230,   497,   615,   567,   617,   662,
     619,   855,   939,   857,    20,   859,   762,   405,     7,   941,
       4,   960,    22,   946,    53,   240,   473,   966,    33,   109,
      33,   591,     0,     5,    20,   501,   502,    15,    16,     7,
    1420,   185,    19,    20,   392,    53,   434,   191,   166,    20,
      57,    20,    68,    69,    70,   160,    63,    57,    20,    20,
      20,   127,    30,   723,    32,   725,    34,   473,   143,   722,
     143,   724,    40,   726,   155,    96,   127,    98,   173,   160,
     192,   734,    50,   530,   129,   130,   137,     8,    56,     7,
     743,   497,   143,   152,   110,   111,   112,   113,   436,   437,
     173,   160,     5,     6,   240,     8,   144,   145,   146,    34,
      15,    16,    80,   605,   173,   220,   165,  1618,   173,   194,
      62,   132,   510,   511,   219,   473,   192,    46,   149,   195,
     102,   103,    50,    36,   102,   103,   185,   218,    63,   220,
     129,   192,   316,  1523,   217,   901,   892,   232,   158,   497,
     158,   231,   712,   194,   165,  1591,   185,   164,   217,   191,
       5,     6,   217,   173,  1378,   173,   392,  1668,   142,   214,
     215,   220,   198,   155,   185,   390,   160,   184,   160,   168,
     218,   937,   107,   675,   940,   191,   218,   106,   166,   163,
     405,   217,   197,   171,   197,   173,   173,   165,   176,   605,
     189,   190,   191,   203,   107,   191,   198,   173,   218,   183,
     215,   136,   233,   705,   214,  1651,   221,   185,   143,   434,
     191,   436,   191,    53,   240,   217,   198,   674,   675,   191,
     191,   191,    21,    22,   319,  1162,   204,   476,   220,   217,
    1485,  1163,   143,   164,   165,   217,   126,   215,   173,   129,
     130,   490,   699,   127,   390,   702,   163,   605,   191,  1005,
     158,   166,   191,   184,   185,   191,   171,    97,   173,   675,
     185,   176,   173,   143,  1213,   173,   183,   165,  1201,  1578,
    1494,  1580,   163,   208,   199,   218,  1585,  1586,   127,   218,
     127,   127,   217,   681,   215,   510,   511,   185,   137,   705,
     137,   137,   183,   127,   642,   771,   191,  1552,  1553,   647,
     217,   199,   127,   137,   166,   762,   704,   191,   192,   217,
     194,   310,   137,   197,   313,  1570,  1571,   675,   217,   118,
     119,   163,   985,   127,   214,   215,   173,   126,  1261,   128,
     129,   130,   131,   137,   165,   143,  1006,   136,  1647,   841,
     366,   183,   163,   192,   152,   192,   192,   705,  1625,   157,
     197,   165,   769,   589,   185,   591,   387,   388,   192,  1636,
     191,   392,   183,   394,   390,   173,   214,   192,   199,   395,
     827,   185,   165,   194,   191,  1630,  1631,   219,   220,   405,
     177,   606,   607,   608,  1317,   610,   611,    57,   192,   614,
     198,   616,   185,   618,   164,   620,  1152,  1016,   191,   216,
    1677,  1678,   638,   208,   435,   191,   199,   165,   434,   645,
     436,   173,   661,   649,   184,   214,   215,   775,   776,   777,
     778,   779,   780,   781,   782,   841,  1280,   185,  1377,   198,
     788,   789,   218,   191,  1004,   892,   794,   686,  1387,   837,
     173,   199,   971,   469,   470,   803,   804,   165,   217,   173,
     808,   809,   810,   982,   812,   486,   681,   173,   174,   175,
     606,   607,   608,  1146,   610,   611,   165,   185,   614,   191,
     616,   165,   618,   191,    68,    69,    70,   165,   954,   704,
    1213,   199,   731,   841,   510,   511,   185,   191,   204,   887,
     888,   185,   191,    47,   216,  1228,   195,   185,   192,   847,
     957,   165,   900,   191,   535,   536,    57,   756,   965,    21,
      22,   199,    63,    67,   218,   863,   110,   111,   112,   113,
    1374,   185,   553,    57,   555,   556,   557,   191,   559,    63,
     191,   929,   930,   165,   932,   199,   191,   165,   936,   570,
     938,   185,   165,  1213,   188,   163,  1216,   173,   773,   774,
    1026,  1027,  1028,   185,  1408,   216,   587,   185,  1228,   191,
     591,   216,   185,   191,   191,   183,  1249,   199,   191,   165,
     601,   199,   216,   191,   799,   923,   199,   177,   143,   152,
     606,   607,   608,  1202,   610,   611,   191,   152,   614,   185,
     616,   218,   618,   191,   620,   191,   191,   188,   189,   824,
     173,   191,   191,   199,    33,   636,   118,   119,   173,   191,
     191,   173,   837,   218,   126,    57,   128,   129,   130,   131,
     218,    63,   165,   218,   136,   216,    21,    22,   218,   218,
     173,    60,    61,    57,  1253,   191,   218,   218,   165,    63,
     865,   667,   185,   191,   500,   501,   502,   872,   191,   191,
     875,   152,   836,   173,   191,   681,   165,  1361,   185,   884,
     216,   165,   887,   888,   191,  1519,   891,   173,   216,   173,
     106,   896,   173,   922,   216,   900,   185,   165,   704,   216,
    1136,   185,   191,   158,  1134,   173,   164,   165,   200,   201,
     202,   203,   204,  1150,   550,   124,   198,   185,   173,   128,
     144,   158,   214,   215,   929,   930,   184,   932,   195,   198,
     192,   936,   199,   938,   939,   217,   173,   194,  1401,   865,
     194,   577,   199,   118,   119,   199,   872,   753,   217,   875,
      66,   126,   194,   128,   129,   130,   131,   199,   884,   194,
     217,   136,   158,   217,   199,   891,   158,  1417,    35,   974,
     896,   976,  1209,    35,   158,   217,    75,   173,   217,   758,
      79,   173,   217,  1161,   193,   144,    12,   146,   197,   173,
    1227,   200,   173,  1443,    93,    94,  1233,    23,    24,    98,
      99,   100,   101,  1240,   191,  1242,   215,   194,  1186,  1137,
     197,   216,   221,   184,    57,  1414,   187,   828,    57,   190,
      63,   195,  1373,    22,    63,   199,   832,   202,   203,   204,
      57,   837,   843,   195,  1461,   185,    63,   199,   185,   214,
     215,   191,   853,   679,   191,   173,   185,   185,   974,    33,
     976,  1288,   191,   191,   185,   144,   145,   146,   195,   865,
     191,  1260,   199,  1300,  1260,   173,   872,   194,  1305,   875,
     197,   707,  1200,   974,  1252,   976,    60,    61,   884,  1563,
    1085,   887,   888,   173,   195,   891,     5,     6,   199,  1635,
     896,   173,  1108,   216,   900,    21,    22,  1643,   195,   195,
     195,   195,   199,   199,   199,   199,    25,   195,   194,  1237,
    1238,   199,    31,   184,   185,   195,   187,  1122,  1525,   199,
     177,   178,  1260,   929,   930,  1671,   932,  1590,   194,  1134,
     936,   197,   938,   939,  1561,   771,   219,   220,  1375,   217,
     124,   219,   220,   127,   128,   177,   178,   179,   180,    68,
      69,    92,    93,   137,   194,  1582,  1161,  1162,   217,    43,
      21,    22,   198,  1400,   173,   174,   175,   198,   974,   198,
     976,   177,   178,   179,  1624,  1625,   177,   178,   179,   217,
     198,  1186,  1632,   102,   103,   198,  1636,  1637,   198,   217,
    1499,   198,   118,   119,   198,  1504,  1122,   217,  1605,  1606,
     126,   198,   198,   129,   130,   131,   198,   843,   192,   193,
     136,   173,   195,   197,   217,   173,   200,   173,   218,  1669,
     173,   216,  1350,   198,   143,  1254,   215,  1677,  1678,   198,
    1358,   215,   184,   185,   186,   198,   217,   221,   455,   456,
     457,   160,   161,   162,  1260,   173,   217,  1252,   198,   198,
    1657,   198,  1430,   166,   173,   116,   117,   118,   119,   120,
     198,   198,   123,   198,   183,   126,   217,   128,   129,   130,
     131,    10,    11,    12,   217,   136,   198,   138,   139,   198,
     220,   217,    10,  1733,   217,   217,   217,    37,   214,   215,
      66,   215,   217,   198,   217,   198,   215,    43,  1109,   173,
     218,   173,  1113,   194,   216,    43,  1117,   217,   944,   217,
     198,   198,   198,   191,   132,  1344,  1122,    14,   954,   192,
     166,   220,   184,   194,   191,  1607,    13,   216,  1134,   173,
    1567,   191,   217,  1338,     8,   191,   218,   198,   199,   200,
     201,   202,   203,   204,   173,   199,  1524,   173,   218,   173,
     173,   191,   218,   214,   215,  1161,  1162,  1594,   191,   184,
     218,   173,   217,   198,   217,     1,   218,   217,   217,    21,
      22,   198,   217,    67,   217,   217,  1658,  1659,   217,   217,
    1186,  1410,   173,   217,   199,   199,   199,    43,   218,   173,
    1026,  1027,  1028,   173,   173,  1031,   218,  1033,   218,  1035,
    1429,  1037,   218,  1039,   173,  1041,   217,  1043,   218,  1045,
     217,  1607,  1338,   216,  1050,  1652,  1052,  1699,   216,   216,
     173,   217,  1058,   218,   217,  1430,  1604,  1432,   198,   217,
     217,   217,   217,   173,  1070,   218,  1072,   173,   181,   217,
     173,  1077,   216,  1079,   216,  1081,  1252,   218,  1084,  1260,
     173,   108,   109,   110,   111,   112,   113,   114,   115,   216,
      33,  1490,  1658,  1659,   116,   117,   118,   119,   120,  1607,
     217,   123,   124,   125,   126,   217,   128,   129,   130,   131,
     137,  1117,   217,   173,   136,   199,   138,   139,   217,    70,
     147,   148,   149,   218,   199,   217,   217,   217,   217,  1310,
     217,   199,   218,  1699,   185,   218,  1432,   217,   217,   217,
     217,   177,   218,    53,   218,   218,   218,   216,   191,  1524,
    1658,  1659,   184,   184,   184,   216,   191,   733,    85,   218,
     218,   184,  1338,   218,  1539,   392,   218,     1,   218,   218,
     216,    21,    22,   218,   216,   402,   198,   199,   200,   201,
     202,   203,   204,   216,   411,    46,   141,    88,   217,  1516,
     218,  1699,   214,   215,   421,  1517,   832,   218,   238,  1003,
    1517,  1517,  1517,  1602,   431,  1517,     1,  1490,    12,  1011,
    1427,  1453,  1272,  1549,  1313,  1550,    56,    21,    22,  1456,
    1550,  1637,   586,   469,   451,   725,    -1,  1497,  1223,  1604,
     469,  1412,    -1,    -1,    -1,    -1,   463,    -1,    -1,    -1,
      -1,    -1,    -1,  1539,    -1,    -1,   473,    -1,    -1,    -1,
      -1,   478,    -1,   480,  1430,    -1,  1432,    -1,    -1,    -1,
      -1,    -1,   489,    -1,    -1,    -1,   116,   117,   118,   119,
     497,   498,   499,    -1,    -1,    -1,   126,    -1,   128,   129,
     130,   131,    -1,    -1,    -1,   512,   136,    -1,   138,   139,
      -1,    -1,    -1,   520,    -1,    -1,   523,   524,   525,   526,
     527,   528,    -1,    -1,    -1,    -1,    -1,    -1,  1707,    -1,
      -1,   538,   116,   117,   118,   119,   120,    33,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,    -1,   136,    -1,   138,   139,    21,    22,    -1,    -1,
     144,   145,   146,    -1,    60,    61,   150,    -1,  1524,    -1,
     200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1539,   214,   215,    -1,  1373,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   605,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    -1,    -1,    -1,    -1,    -1,   633,    -1,   124,    -1,
     214,   215,   128,    -1,    -1,  1591,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1604,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,   675,    -1,
      -1,   136,    -1,   138,   139,   682,   683,    -1,   685,   144,
     145,   146,    -1,   690,    -1,   150,    -1,    -1,    -1,   696,
      -1,    -1,    -1,    -1,    -1,  1651,    -1,   193,   705,    -1,
    1661,   197,    -1,   199,   200,    -1,  1667,    -1,    -1,   716,
     717,   718,   719,   720,   721,    -1,   723,    -1,   725,   215,
      -1,    -1,    -1,    -1,    -1,   221,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
    1701,    -1,  1703,    -1,    21,    22,    -1,    -1,    -1,   214,
     215,    -1,    -1,    -1,   219,   220,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1728,   775,   776,
     777,   778,   779,   780,   781,   782,   783,   784,    -1,    -1,
     787,   788,   789,   790,   791,   792,    -1,   794,   795,    -1,
     797,   798,   799,   800,   801,   802,   803,   804,   805,   806,
     807,   808,   809,   810,   811,   812,   813,   814,   815,   816,
     817,   818,   819,   820,   821,   822,   823,    -1,   825,    -1,
      -1,    -1,   829,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   841,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    33,    -1,   144,   145,   146,
      -1,    -1,   879,   150,    -1,  1661,    -1,    -1,    -1,   886,
      -1,    -1,    -1,    -1,    -1,    -1,   893,    -1,   895,    -1,
      -1,    -1,    60,    61,    -1,    -1,    -1,   904,   905,   906,
     907,   908,   909,   910,   911,   912,   913,   914,   915,   916,
     917,   918,   919,   920,   921,  1701,   193,  1703,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,   218,  1728,    -1,    -1,    -1,    -1,    -1,   955,   956,
      -1,    -1,    -1,    -1,    -1,    -1,   124,    -1,    -1,   127,
     128,    -1,   969,    -1,   971,    -1,    -1,    -1,    -1,   137,
      -1,   978,    -1,    -1,    -1,   982,    -1,    -1,    -1,    -1,
     987,    -1,   989,   990,   991,    -1,    -1,    -1,     5,     6,
      -1,    -1,    -1,    -1,    -1,  1002,    -1,   165,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    33,   185,    -1,    -1,
      -1,    -1,    39,    -1,   192,   193,    -1,    -1,    45,   197,
      -1,    48,   200,    -1,    51,    -1,    53,    -1,    55,    -1,
      -1,    -1,    -1,    60,    61,    -1,    -1,   215,    65,    -1,
      -1,    68,    69,   221,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,  1098,    -1,    -1,    -1,  1102,    -1,    33,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,   124,    -1,    -1,
      -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    60,    61,   143,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,
     157,   158,   159,   160,   161,   162,    -1,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,    -1,
     197,    -1,  1189,   200,   201,   202,    -1,   204,   124,    -1,
     207,   208,   128,    -1,    -1,    -1,    -1,    -1,   215,    -1,
     217,    -1,   219,   220,   221,    -1,    -1,  1214,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1222,  1223,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   165,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      13,    21,    22,  1250,  1251,    -1,    19,    -1,    -1,   185,
    1257,    -1,    25,  1260,    -1,   191,    -1,   193,    31,  1266,
      -1,   197,    -1,   199,   200,    -1,    -1,    -1,    41,    -1,
      -1,    -1,    -1,    -1,    -1,  1282,    49,  1284,    -1,   215,
      -1,    -1,  1289,    -1,    -1,   221,    -1,    -1,  1295,    -1,
      -1,    64,  1299,    -1,    -1,    -1,    -1,    -1,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,    -1,    -1,    -1,  1341,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    33,   128,   129,
     130,   131,    -1,    -1,  1361,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    60,    61,   776,   777,   778,   779,
     780,   781,   782,   156,    -1,    -1,    -1,    -1,   788,   789,
      -1,    -1,    -1,    -1,   794,  1402,  1403,  1404,    -1,    33,
     173,    -1,    -1,   803,   804,    -1,    -1,    -1,   808,   809,
     810,    -1,   812,   193,    -1,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,    -1,    60,    61,    -1,    21,
      22,  1438,    -1,  1440,   214,   215,    -1,    -1,   124,  1446,
      -1,    -1,   128,    -1,    -1,    -1,   219,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1470,    -1,    -1,  1473,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1481,  1482,  1483,    -1,    -1,    -1,
      -1,  1488,    -1,    -1,    -1,    -1,  1493,    -1,    -1,  1496,
     124,    -1,  1499,  1500,   128,    -1,    -1,  1504,  1505,    -1,
    1507,  1508,    -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,
    1517,   197,    -1,   199,   200,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1530,   116,   117,   118,   119,   120,   215,
      -1,   123,   124,   125,   126,   221,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,  1563,    -1,   150,   193,
      -1,    -1,    -1,   197,    -1,   199,   200,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1587,   215,    -1,    -1,    -1,    -1,    -1,   221,  1595,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1607,   193,  1609,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1627,  1628,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1648,  1649,    -1,    -1,    -1,  1653,    -1,    -1,    -1,
       1,  1658,  1659,    -1,     5,     6,     7,    -1,     9,    10,
      11,    -1,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,  1686,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    -1,
      -1,    42,  1699,    44,    45,    -1,    -1,    48,    -1,    50,
      51,    52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   133,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,    -1,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,   184,   185,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
     201,   202,    -1,   204,    -1,    -1,   207,   208,    -1,    -1,
       5,     6,    -1,    -1,   215,    -1,   217,    -1,   219,   220,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,    31,    -1,    33,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,
      55,    -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,   124,
      -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,
     155,    -1,   157,   158,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,
      -1,    -1,   197,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,    -1,     5,     6,    -1,
     215,    -1,   217,    -1,   219,   220,   221,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    33,    -1,    -1,    -1,    -1,
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
      -1,   159,   160,   161,   162,    -1,    -1,    -1,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,    -1,
      -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   197,
      -1,    -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,
     208,    -1,    -1,    -1,     5,     6,    -1,   215,    -1,   217,
      -1,   219,   220,   221,    15,    16,    17,    18,    19,    -1,
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
     161,   162,    -1,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
     201,   202,    -1,   204,    -1,    -1,   207,   208,    -1,    -1,
       5,     6,    -1,    -1,   215,    -1,   217,    -1,   219,   220,
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
      -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,   133,   134,
     135,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,
     155,    -1,   157,    -1,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,     5,     6,    -1,    -1,
     215,    -1,   217,    -1,   219,   220,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    52,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,   105,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,    -1,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,   208,
      -1,    -1,     5,     6,    -1,    -1,   215,    -1,   217,    -1,
     219,   220,    15,    16,    17,    18,    19,    -1,    -1,    -1,
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
     133,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,
     153,   154,   155,    -1,   157,    -1,   159,   160,   161,   162,
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,     5,     6,
      -1,    -1,   215,    -1,   217,    -1,   219,   220,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    70,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,
     157,   158,   159,   160,   161,   162,    -1,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   200,   201,   202,    -1,   204,    -1,    -1,
     207,   208,    -1,    -1,     5,     6,    -1,    -1,   215,    -1,
     217,   218,   219,   220,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    33,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    60,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
     124,    -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   133,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,    -1,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,    -1,    -1,   197,    -1,   199,   200,    -1,    -1,   200,
     201,   202,    -1,   204,    -1,    -1,   207,   208,    -1,    -1,
      -1,   215,     5,     6,   215,    -1,   217,   221,   219,   220,
      13,    -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    33,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    49,    -1,    51,    -1,
      -1,    -1,    55,    -1,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,    -1,   124,    -1,
      -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,
     153,   154,   155,    -1,   157,    -1,   159,   160,   161,   162,
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,
      -1,   197,    -1,   199,   200,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,    -1,   215,
       5,     6,   215,   216,   217,   221,   219,   220,    13,    -1,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    33,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    48,    49,    -1,    51,    -1,    -1,    -1,
      55,    -1,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,   119,    -1,   124,    -1,    -1,    -1,
     128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,
     155,    -1,   157,   158,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,    -1,   197,
      -1,   199,   200,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,    -1,   215,     5,     6,
     215,    -1,   217,   221,   219,   220,    13,    -1,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    33,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    49,    -1,    51,    -1,    -1,    -1,    55,    -1,
      60,    61,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,   124,    -1,    -1,    -1,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,
     157,    -1,   159,   160,   161,   162,    -1,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,   193,    -1,    -1,    -1,   197,    -1,   199,
     200,    -1,    -1,   200,   201,   202,    -1,   204,    -1,    -1,
     207,   208,    -1,    -1,    -1,   215,     5,     6,   215,   216,
     217,   221,   219,   220,    13,    -1,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      49,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
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
     159,   160,   161,   162,    -1,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,   208,
      -1,    -1,     5,     6,    -1,    -1,   215,    -1,   217,    -1,
     219,   220,    15,    16,    17,    18,    19,    -1,    -1,    -1,
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
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,     5,     6,
      -1,    -1,   215,    -1,   217,    -1,   219,   220,    15,    16,
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
     157,   158,   159,   160,   161,   162,    -1,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   200,   201,   202,    -1,   204,    -1,    -1,
     207,   208,    -1,    -1,     5,     6,    -1,    -1,   215,    -1,
     217,   218,   219,   220,    15,    16,    17,    18,    19,    -1,
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
     151,   152,   153,   154,   155,    -1,   157,   158,   159,   160,
     161,   162,    -1,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
     201,   202,    -1,   204,    -1,    -1,   207,   208,    -1,    -1,
       5,     6,    -1,    -1,   215,    -1,   217,   218,   219,   220,
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
     155,    -1,   157,    -1,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,     5,     6,    -1,    -1,
     215,   216,   217,    -1,   219,   220,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,
      -1,    -1,    61,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,    -1,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,   208,
      -1,    -1,     5,     6,    -1,    -1,   215,    -1,   217,    -1,
     219,   220,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    58,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,    -1,    -1,
       5,     6,   215,    -1,   217,    -1,   219,   220,    13,    -1,
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
     155,    -1,   157,    -1,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,     5,     6,    -1,    -1,
     215,    -1,   217,    -1,   219,   220,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    33,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    60,    61,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,   124,    -1,    -1,    -1,   128,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,    -1,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,    -1,    -1,   197,    -1,   199,   200,   198,
      -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,   208,
      -1,    -1,    -1,   215,     5,     6,   215,    -1,   217,   221,
     219,   220,    13,    -1,    15,    16,    17,    18,    19,    -1,
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
     161,   162,    -1,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
     201,   202,    -1,   204,    -1,    -1,   207,   208,    -1,    -1,
       5,     6,    -1,    -1,   215,    -1,   217,    -1,   219,   220,
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
     155,    -1,   157,   158,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,     5,     6,    -1,    -1,
     215,    -1,   217,    -1,   219,   220,    15,    16,    17,    18,
      19,    -1,    -1,    22,    -1,    -1,    25,    -1,    27,    -1,
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
     159,   160,   161,   162,    -1,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,   208,
      -1,    -1,     5,     6,    -1,    -1,   215,    -1,   217,    -1,
     219,   220,    15,    16,    17,    18,    19,    -1,    -1,    -1,
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
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,     5,     6,
      -1,    -1,   215,    -1,   217,   218,   219,   220,    15,    16,
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
     157,    -1,   159,   160,   161,   162,    -1,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,    -1,   200,   201,   202,    -1,   204,    -1,    -1,
     207,   208,    -1,    -1,     5,     6,    -1,    -1,   215,    -1,
     217,    -1,   219,   220,    15,    16,    17,    18,    19,    -1,
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
     161,   162,    -1,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
     201,   202,    -1,   204,    -1,    -1,   207,   208,    -1,    -1,
       5,     6,    -1,    -1,   215,    -1,   217,   218,   219,   220,
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
     155,    -1,   157,    -1,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,     5,     6,    -1,    -1,
     215,    -1,   217,   218,   219,   220,    15,    16,    17,    18,
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
     159,   160,   161,   162,    -1,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,   208,
      -1,    -1,     5,     6,    -1,    -1,   215,    -1,   217,   218,
     219,   220,    15,    16,    17,    18,    19,    -1,    -1,    -1,
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
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,     5,     6,
      -1,    -1,   215,    -1,   217,   218,   219,   220,    15,    16,
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
     157,    -1,   159,   160,   161,   162,    -1,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   200,   201,   202,    -1,   204,    -1,    -1,
     207,   208,    -1,    -1,     5,     6,    -1,    -1,   215,    -1,
     217,   218,   219,   220,    15,    16,    17,    18,    19,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,    -1,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,
     201,   202,    -1,   204,    -1,    -1,   207,   208,    -1,    -1,
       5,     6,    -1,    -1,   215,    -1,   217,    -1,   219,   220,
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
     155,    -1,   157,    -1,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,     5,     6,    -1,    -1,
     215,    -1,   217,    -1,   219,   220,    15,    16,    17,    18,
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
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   127,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,    -1,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,   208,
      -1,    -1,     5,     6,    -1,    -1,   215,    -1,   217,    -1,
     219,   220,    15,    16,    17,    18,    19,    -1,    -1,    -1,
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
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,     5,     6,
      -1,    -1,   215,    -1,   217,    -1,   219,   220,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    33,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      60,    61,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,   124,    -1,    10,    -1,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,    -1,
     157,    -1,   159,   160,   161,   162,    -1,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    21,    22,   183,    -1,    -1,    -1,
      -1,    -1,    -1,   193,    -1,    -1,    -1,   197,    -1,   199,
     200,    -1,    -1,   200,   201,   202,    -1,   204,    -1,    -1,
     207,   208,    -1,    -1,    -1,   215,    -1,    -1,   215,    -1,
     217,   221,   219,   220,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,    -1,    -1,
      -1,    -1,   136,   137,   138,   139,   140,   141,    -1,    -1,
     144,   145,   146,   147,   148,   149,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   192,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    13,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,
     214,   215,    -1,    25,    -1,   219,   220,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    -1,    -1,
      -1,    -1,    64,    -1,    -1,    -1,    -1,   214,   215,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    -1,   156,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,   173,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   219,    -1,    33,
      -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    -1,    33,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    41,    60,    61,    -1,    -1,
      -1,    -1,    -1,    49,    -1,    -1,    -1,   156,    -1,    60,
      61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,   173,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,    -1,    -1,
     124,    -1,    -1,    33,   128,    -1,    -1,    -1,    33,    -1,
     219,    -1,   221,   124,    -1,    -1,    -1,   128,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      60,    61,    -1,    -1,    -1,    60,    61,   143,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   165,
      -1,    -1,    19,    -1,    -1,    -1,    -1,   173,    25,   193,
      -1,    -1,    -1,   197,    31,   199,   200,    -1,    -1,   185,
      -1,    -1,   193,    -1,    41,   191,   197,    -1,   199,   200,
      -1,   215,    49,    -1,   124,    -1,    -1,   221,   128,   124,
      -1,    -1,    -1,   128,   215,    -1,    -1,    64,    -1,    -1,
     221,    -1,    -1,   219,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   193,    -1,    19,    -1,   197,   193,   199,
     200,    25,   197,    -1,   199,   200,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   215,   143,    41,    -1,    -1,
     215,   221,    -1,    -1,    -1,    49,   221,    -1,    -1,   156,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      64,    -1,    -1,    -1,    -1,    -1,   173,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   219,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   156,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   173,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,    -1,   146,    -1,    -1,    -1,    -1,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,   131,    -1,    -1,    -1,   219,   136,   137,   138,   139,
     140,   141,    21,    22,   144,   145,   146,   147,   148,   149,
     150,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    -1,    -1,    -1,   165,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,    -1,    -1,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,   185,    -1,    -1,    -1,    -1,
      -1,    -1,   192,   193,    -1,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   214,   215,    -1,    -1,    -1,   219,
     220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,    -1,    -1,    -1,   136,   137,   138,
     139,   140,   141,    -1,    -1,   144,   145,   146,   147,   148,
     149,   150,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    21,    22,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,   192,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,    -1,
     219,   220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    60,    61,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    21,    22,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,   124,    -1,    -1,   150,   128,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,   193,    -1,   218,    -1,   197,    -1,   199,   200,    -1,
      -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,   215,   128,   129,   130,   131,    -1,   221,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    21,    22,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    21,    22,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,
     218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    21,    22,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    21,    22,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    21,    22,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    21,    22,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,
     218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    21,    22,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    21,    22,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    21,    22,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    21,    22,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,
     218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    21,    22,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    21,    22,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    -1,    -1,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    21,    22,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,   218,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,    21,    22,    -1,    -1,   136,    -1,   138,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,
     150,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,
     218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   214,   215,   216,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    21,    22,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,   216,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,   216,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    21,    22,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,   216,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,   216,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,    21,    22,
      -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,    -1,    -1,    38,   150,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,   215,   216,    -1,    -1,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,    -1,    -1,   128,   129,   130,    -1,    -1,
      -1,    -1,    -1,    -1,   137,   138,   139,   140,   141,    -1,
      -1,   144,   145,   146,   147,   148,   149,   150,    -1,    -1,
      -1,    -1,    -1,    21,    22,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,   163,    -1,   207,   208,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   193,    -1,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,    -1,    -1,    21,    22,   116,   117,
     118,   119,   120,   214,   215,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   165,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   185,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,   214,   215,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,   142,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    21,    22,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,   142,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    21,    22,    -1,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,   214,   215,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    21,    22,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    21,    22,    -1,    -1,    -1,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,   214,   215,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    21,    22,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
     185,    -1,   150,    21,    22,    -1,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,   214,   215,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,    21,    22,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,
     138,   139,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,   214,   215,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,
      -1,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,    -1,    -1,    -1,    -1,    -1,    19,    -1,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   158,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,
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
     101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     173,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   173
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   223,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   102,   103,   165,   185,   204,   215,   224,   228,   237,
     239,   240,   245,   252,   276,   280,   316,   397,   404,   408,
     419,   464,   469,   474,    19,    20,   173,   268,   269,   270,
     166,   246,   247,   173,   174,   175,   204,   241,   242,   243,
     163,   183,   285,   405,   173,   219,   226,    57,    63,   400,
     400,   400,   143,   173,   302,    34,    63,   107,   136,   208,
     217,   272,   273,   274,   275,   302,   252,     5,     6,     8,
      36,   107,   416,    62,   395,   192,   191,   194,   191,   242,
      22,    57,   203,   214,   244,   400,   401,   403,   401,   395,
     475,   465,   470,   173,   143,   238,   274,   274,   274,   217,
     144,   145,   146,   191,   216,    57,    63,   281,   283,    57,
      63,   406,     5,     6,    57,    63,   417,    57,    63,   396,
      15,    16,   166,   171,   173,   176,   217,   230,   269,   166,
     247,   173,   241,   241,   173,   252,   164,   184,   286,   401,
     252,    57,    63,   225,   173,   173,   173,   173,   177,   236,
     218,   270,   274,   274,   274,   274,   284,   173,   407,   420,
     285,   398,   177,   178,   179,   229,    15,    16,   166,   171,
     173,   230,   266,   267,   244,   402,   252,   208,   227,   476,
     466,   471,   177,   218,    35,    71,    73,    75,    76,    77,
      78,    79,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    93,    94,    95,    98,    99,   100,   101,
     118,   119,   173,   279,   282,   194,   285,   106,   414,   415,
     393,   160,   220,   271,   366,   177,   178,   179,   191,   218,
     192,    66,   285,   285,   285,    21,    22,    38,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   128,   129,   130,   137,
     138,   139,   140,   141,   144,   145,   146,   147,   148,   149,
     150,   193,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,   207,   208,   214,   215,    35,    35,   217,   277,
     285,   287,    75,    79,    93,    94,    98,    99,   100,   101,
     424,   409,   173,   421,   165,   286,   394,   270,   269,   220,
     252,   152,   173,   391,   392,   266,    19,    25,    31,    41,
      49,    64,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   156,   219,   302,   423,   425,
     426,   429,   435,   463,   477,   467,   472,   173,   173,   173,
     216,    22,   173,   216,   155,   218,   366,   376,   377,   378,
     194,   278,   290,   285,   399,   194,   413,   285,   418,   366,
     216,   269,   217,    43,   191,   194,   197,   390,   198,   198,
     198,   217,   198,   198,   217,   198,   198,   198,   198,   198,
     198,   217,   302,    33,    60,    61,   124,   128,   193,   197,
     200,   215,   221,   434,   195,   428,   381,   384,   173,   108,
     109,   110,   111,   112,   113,   114,   115,   137,   147,   148,
     149,   217,     7,    50,   315,   252,   191,   252,   218,   463,
       1,     5,     6,     9,    10,    11,    13,    15,    16,    17,
      18,    19,    25,    26,    27,    28,    29,    31,    38,    39,
      42,    44,    45,    48,    51,    52,    54,    55,    58,    59,
      65,    68,    69,    80,   102,   103,   104,   105,   118,   119,
     133,   134,   135,   151,   152,   153,   154,   155,   157,   159,
     160,   161,   162,   166,   167,   168,   169,   170,   171,   172,
     174,   175,   176,   183,   200,   201,   202,   207,   208,   215,
     217,   219,   220,   235,   237,   248,   249,   252,   253,   256,
     257,   259,   261,   262,   263,   264,   286,   288,   289,   291,
     296,   301,   302,   303,   307,   308,   309,   310,   311,   312,
     313,   314,   316,   320,   321,   328,   331,   334,   339,   342,
     343,   345,   346,   347,   349,   354,   357,   358,   365,   423,
     479,   494,   505,   509,   522,   525,   411,   173,   252,   410,
     302,   372,   392,   216,    65,   104,   174,   296,   358,   173,
     173,   435,   127,   137,   192,   389,   436,   441,   443,   358,
     445,   439,   173,   430,   447,   449,   451,   453,   455,   457,
     459,   461,   358,   198,   217,    33,   197,    33,   197,   215,
     221,   216,   358,   215,   221,   435,   173,   252,   478,   173,
     191,   252,   379,   432,   463,   468,   173,   382,   432,   473,
     358,   152,   173,   386,   387,   422,   378,   378,   378,   198,
     198,   358,   260,   198,   304,   425,   479,   217,   302,   198,
       5,   102,   103,   198,   217,   127,   301,   332,   343,   358,
     287,   198,   217,    61,   358,   217,   358,   173,   198,   198,
     217,   252,   198,   166,    58,   358,   217,   287,   198,   217,
     198,   198,   217,   198,   198,   127,   301,   358,   358,   358,
     220,   287,   334,   338,   338,   338,   217,   217,   217,   217,
     217,   217,    13,   435,    13,   435,    13,   358,   504,   520,
     198,   358,   198,   234,    13,   294,   504,   521,   358,   358,
     358,   358,   358,    13,    49,   292,   332,   358,   158,   173,
     332,   480,   482,   220,   252,   252,   358,    10,    37,   334,
     340,   173,   217,   252,   252,   252,   252,   252,    66,   317,
     276,   132,   252,    21,    22,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   136,
     137,   138,   139,   140,   141,   144,   145,   146,   147,   148,
     149,   150,   192,   193,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   214,   215,   340,   217,   399,   192,
     252,   286,   215,   252,   276,   286,   373,   198,   218,    43,
     252,   389,   301,   358,   463,   463,   433,   463,   218,   463,
     463,   218,   173,   427,   463,   277,   463,   277,   463,   277,
     379,   380,   382,   383,   218,   438,   292,   216,   216,   358,
     252,   286,   194,   432,   286,   194,   432,   286,   218,   217,
      43,   127,   191,   192,   194,   197,   385,   495,   497,   287,
     422,   305,   217,   302,   198,   217,   329,   198,   198,   198,
     516,   332,   301,   332,   191,   108,   109,   110,   111,   112,
     113,   114,   115,   121,   122,   127,   140,   141,   147,   148,
     149,   192,    14,   435,   294,   358,   358,   287,   192,   322,
     324,   358,   326,   194,   166,   358,   518,   332,   501,   506,
     332,   499,   435,   301,   358,   220,   276,   358,   358,   358,
     358,   358,   358,   422,    53,   200,   215,   217,   358,   480,
     483,   487,   503,   508,   422,   217,   483,   508,   422,   142,
     184,   185,   186,   488,   297,   287,   299,   179,   180,   229,
     422,   184,   191,   524,   422,    13,   216,   191,   524,   217,
     137,   385,   524,   191,   524,   218,   152,   157,   198,   302,
     348,   287,   258,   285,   341,    70,   215,   218,   332,   482,
     160,   217,   319,   392,     4,   160,   337,   338,    19,   158,
     173,   423,    19,   158,   173,   423,   133,   134,   135,   288,
     344,   358,   344,   358,   344,   358,   344,   358,   344,   358,
     344,   358,   344,   358,   344,   358,   358,   358,   358,   344,
     358,   344,   358,   358,   358,   358,   173,   344,   358,   358,
     158,   173,   358,   358,   358,   423,   358,   358,   358,   344,
     358,   344,   358,   358,   358,   358,   344,   358,   344,   358,
     344,   358,   358,   344,   358,    22,   358,   358,   358,   358,
     358,   358,   358,   358,   358,   358,   358,   129,   130,   158,
     173,   214,   215,   355,   423,   358,   218,   332,   412,   358,
     275,     8,   366,   371,   435,   173,   301,   358,   252,   199,
     199,   199,   432,   199,   199,   199,   252,   199,   278,   199,
     278,   199,   278,   199,   432,   199,   432,   295,   463,   218,
     216,   463,   463,   358,   173,   173,   463,   358,   435,   435,
      20,   463,    70,   332,   482,   493,   198,   358,   173,   358,
     463,   510,   512,   514,   435,   524,   358,   358,   358,   358,
     358,   358,   358,   358,   358,   358,   358,   358,   358,   358,
     358,   358,   358,   358,   287,   199,   432,   218,   218,   265,
     435,   435,   218,   435,   218,   435,   524,   435,   380,   524,
     383,   199,   337,   218,   218,   218,   218,   218,   218,    20,
     338,   215,   358,   218,   142,   191,   185,   487,   188,   189,
     216,   491,   191,   185,   188,   216,   490,    20,   218,   487,
     184,   187,   489,    20,   358,   184,   504,   295,   295,   358,
      20,   504,    20,   422,   358,   358,   358,   218,   158,   173,
     217,   217,   350,   352,    12,    23,    24,   250,   251,   358,
     290,   276,   173,   218,   482,   480,   191,   218,   218,   173,
     318,   318,   217,   127,   137,   173,   192,   197,   335,   336,
     277,   198,   217,   198,   217,   338,   338,   338,   217,   217,
     216,    19,   158,   173,   423,   194,   158,   173,   358,   217,
     217,   158,   173,   358,     1,   217,   216,   218,   286,   252,
     216,    57,    63,   369,    67,   370,   252,   199,   252,   437,
     442,   444,   463,   446,   440,   431,   173,   448,   199,   452,
     199,   456,   199,   460,   379,   462,   382,   199,   432,   218,
      43,   385,   199,   199,   332,   199,   482,   218,   218,   218,
     173,   218,   185,   199,   218,   199,   435,   380,   383,   199,
     218,   217,   435,   358,   199,   199,   199,   199,   218,   199,
     199,   218,   199,   337,   277,   217,   332,   483,   487,   358,
     480,   491,   216,   358,   503,   216,   332,   483,   184,   187,
     190,   492,   216,   332,   199,   199,   194,   232,   332,   332,
      20,   218,   217,   137,   385,   358,   358,   435,   277,   287,
     358,    12,   254,   286,   337,   218,   216,   215,   191,   216,
     218,   336,   173,   173,   217,   173,   173,   191,   216,   278,
     359,   358,   361,   358,   218,   332,   358,   198,   217,   358,
     217,   216,   358,   215,   218,   332,   217,   216,   356,   218,
     332,   252,    47,   370,    46,   106,   367,   337,   450,   454,
     458,   217,   463,   173,   358,   496,   498,   287,   306,   218,
     199,   432,   173,   217,   330,   199,   199,   199,   517,   294,
     199,   323,   325,   327,   519,   502,   507,   500,   217,   340,
     278,   218,   332,   185,   487,   491,   185,   487,   216,   185,
     298,   300,   233,   181,   185,   185,   332,   137,   385,   358,
     358,   358,   218,   218,   199,   278,   287,   255,   252,   277,
     218,   480,   173,   216,   194,   390,   218,   173,   335,   216,
     142,   287,   333,   435,   218,   463,   218,   218,   218,   363,
     358,   358,   218,   480,   218,   358,   218,    33,   368,   367,
     369,   292,   217,   217,   358,   173,   199,   358,   511,   513,
     515,   217,   218,   217,   358,   358,   358,   217,    70,   493,
     217,   217,   218,   358,   333,   218,   358,   491,   358,   492,
     504,   358,   217,   293,   231,   504,   358,   185,   358,   358,
     218,   351,   199,   251,    26,   105,   256,   308,   309,   310,
     312,   358,   278,   216,   194,   390,   435,   389,   218,   127,
     358,   199,   199,   463,   218,   218,   216,   218,   374,   368,
     386,   387,   388,   218,   493,   493,   218,   199,   217,   218,
     217,   217,   217,   292,   294,   332,   493,   493,   218,   185,
     523,   523,   523,   292,   177,   523,   523,   358,   137,   385,
     348,   353,   127,   127,   358,   287,   218,   435,   389,   389,
     301,   358,   358,   360,   362,   199,   218,   282,   375,   217,
     480,   484,   485,   486,   486,   358,   358,   493,   493,   480,
     481,   218,   218,   524,   486,   481,    53,   216,   184,   184,
     191,   524,   184,   216,   523,   358,   358,   348,   358,   389,
     301,   358,   301,   358,   252,   364,   252,   282,   480,   191,
     524,   218,   218,   218,   218,   486,   486,   218,   218,   218,
     218,   358,   216,   216,   184,   218,   216,   301,   358,   252,
     252,   287,   218,   217,   218,   218,   252,   480,   218
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   222,   223,   223,   223,   223,   223,   223,   223,   223,
     223,   223,   223,   223,   223,   223,   223,   224,   225,   225,
     225,   226,   226,   227,   227,   228,   229,   229,   229,   229,
     230,   230,   231,   231,   232,   233,   232,   234,   234,   234,
     235,   236,   236,   238,   237,   239,   240,   241,   241,   241,
     242,   242,   242,   242,   243,   243,   244,   244,   245,   246,
     246,   247,   247,   248,   249,   249,   250,   250,   251,   251,
     251,   252,   252,   253,   253,   254,   255,   254,   256,   256,
     256,   256,   256,   257,   258,   257,   260,   259,   261,   262,
     263,   265,   264,   266,   266,   266,   266,   266,   266,   267,
     267,   268,   268,   268,   269,   269,   269,   269,   269,   269,
     269,   269,   270,   270,   271,   271,   271,   272,   272,   272,
     272,   273,   273,   274,   274,   274,   274,   274,   274,   274,
     275,   275,   276,   276,   277,   277,   277,   278,   278,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   280,   281,   281,   281,   282,
     284,   283,   285,   285,   286,   286,   287,   287,   288,   288,
     288,   289,   289,   289,   289,   289,   289,   289,   289,   289,
     289,   289,   289,   289,   289,   289,   289,   289,   289,   289,
     289,   289,   290,   290,   290,   291,   292,   292,   293,   293,
     294,   294,   295,   295,   297,   298,   296,   299,   300,   296,
     301,   301,   301,   301,   301,   302,   302,   302,   303,   303,
     305,   306,   304,   304,   307,   307,   307,   307,   307,   307,
     308,   309,   310,   310,   310,   311,   311,   311,   312,   312,
     313,   313,   313,   314,   315,   315,   315,   316,   316,   317,
     317,   318,   318,   319,   319,   319,   319,   319,   319,   319,
     319,   320,   320,   322,   323,   321,   324,   325,   321,   326,
     327,   321,   329,   330,   328,   331,   331,   331,   331,   331,
     331,   332,   332,   333,   333,   333,   334,   334,   334,   335,
     335,   335,   335,   335,   336,   336,   337,   337,   337,   338,
     338,   339,   341,   340,   342,   342,   342,   342,   342,   342,
     342,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     344,   344,   344,   344,   345,   345,   345,   345,   345,   345,
     345,   345,   345,   345,   345,   345,   345,   345,   345,   345,
     345,   346,   346,   347,   347,   348,   348,   349,   350,   351,
     349,   352,   353,   349,   354,   354,   354,   354,   354,   354,
     354,   355,   356,   354,   357,   357,   357,   357,   357,   357,
     357,   358,   358,   358,   358,   358,   358,   358,   358,   358,
     358,   358,   358,   358,   358,   358,   358,   358,   358,   358,
     358,   358,   358,   358,   358,   358,   358,   358,   358,   358,
     358,   358,   358,   358,   358,   358,   358,   358,   358,   358,
     358,   358,   358,   358,   358,   358,   358,   358,   358,   358,
     358,   358,   358,   358,   358,   358,   358,   358,   359,   360,
     358,   358,   358,   358,   361,   362,   358,   358,   358,   363,
     364,   358,   358,   358,   358,   358,   358,   358,   358,   358,
     358,   358,   358,   358,   358,   358,   365,   365,   365,   365,
     365,   365,   365,   365,   365,   365,   365,   365,   365,   365,
     365,   365,   366,   366,   366,   367,   367,   367,   368,   368,
     369,   369,   369,   370,   370,   371,   372,   372,   373,   372,
     374,   372,   375,   372,   372,   376,   377,   377,   378,   378,
     378,   378,   378,   379,   379,   380,   380,   381,   381,   381,
     382,   383,   383,   384,   384,   384,   385,   385,   386,   386,
     386,   387,   387,   388,   388,   389,   389,   389,   390,   390,
     391,   391,   391,   391,   391,   392,   392,   392,   392,   392,
     393,   393,   394,   393,   395,   395,   396,   396,   396,   397,
     398,   397,   399,   399,   399,   399,   400,   400,   400,   402,
     401,   403,   403,   404,   405,   404,   406,   406,   406,   407,
     409,   410,   408,   411,   412,   408,   413,   413,   414,   414,
     415,   416,   416,   416,   416,   417,   417,   417,   418,   418,
     420,   421,   419,   422,   422,   422,   422,   422,   423,   423,
     423,   423,   423,   423,   423,   423,   423,   423,   423,   423,
     423,   423,   423,   423,   423,   423,   423,   423,   423,   423,
     423,   423,   423,   423,   423,   424,   424,   424,   424,   424,
     424,   424,   424,   425,   426,   426,   426,   427,   427,   428,
     428,   428,   430,   431,   429,   432,   432,   433,   433,   434,
     434,   435,   435,   435,   435,   435,   435,   436,   437,   435,
     435,   435,   438,   435,   435,   435,   435,   435,   435,   435,
     435,   435,   435,   435,   435,   435,   439,   440,   435,   435,
     441,   442,   435,   443,   444,   435,   445,   446,   435,   435,
     447,   448,   435,   449,   450,   435,   435,   451,   452,   435,
     453,   454,   435,   435,   455,   456,   435,   457,   458,   435,
     459,   460,   435,   461,   462,   435,   463,   463,   463,   465,
     466,   467,   468,   464,   470,   471,   472,   473,   469,   475,
     476,   477,   478,   474,   479,   479,   479,   479,   479,   480,
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
     525,   525
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     0,     1,
       1,     1,     1,     0,     2,     5,     1,     1,     2,     2,
       3,     2,     0,     2,     0,     0,     3,     0,     2,     5,
       3,     1,     2,     0,     4,     2,     2,     1,     1,     1,
       1,     2,     3,     3,     2,     4,     0,     1,     2,     1,
       3,     1,     3,     3,     3,     2,     1,     1,     0,     2,
       4,     1,     1,     1,     1,     0,     0,     3,     1,     1,
       1,     1,     1,     4,     0,     6,     0,     6,     2,     3,
       3,     0,     5,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       1,     5,     1,     3,     2,     3,     2,     1,     1,     1,
       1,     1,     4,     1,     2,     3,     3,     3,     3,     2,
       1,     3,     0,     3,     0,     2,     3,     0,     2,     1,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     3,     3,     2,     2,     3,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       3,     2,     2,     2,     2,     2,     3,     3,     3,     4,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     0,     1,     1,     3,
       0,     4,     1,     1,     1,     1,     3,     7,     2,     2,
       6,     1,     1,     1,     1,     2,     2,     1,     1,     1,
       1,     1,     1,     2,     2,     1,     1,     1,     1,     2,
       2,     2,     0,     2,     2,     3,     0,     2,     0,     4,
       0,     2,     1,     3,     0,     0,     7,     0,     0,     7,
       3,     2,     2,     2,     1,     1,     3,     2,     2,     3,
       0,     0,     5,     1,     2,     5,     5,     5,     6,     2,
       1,     1,     1,     2,     3,     2,     2,     3,     2,     3,
       2,     2,     3,     4,     1,     1,     0,     1,     1,     1,
       0,     1,     3,     9,     8,     8,     7,     8,     7,     7,
       6,     3,     3,     0,     0,     7,     0,     0,     7,     0,
       0,     7,     0,     0,     6,     5,     8,    10,     5,     8,
      10,     1,     3,     1,     2,     3,     1,     1,     2,     2,
       2,     2,     2,     4,     1,     3,     0,     4,     4,     1,
       6,     6,     0,     7,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       2,     2,     2,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     6,     8,     5,     6,     1,     4,     3,     0,     0,
       8,     0,     0,     9,     3,     4,     5,     6,     8,     5,
       6,     0,     0,     5,     3,     4,     4,     5,     4,     3,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     4,     3,     4,     5,     4,     5,     3,     4,     1,
       1,     2,     4,     4,     7,     8,     3,     5,     0,     0,
       8,     3,     3,     3,     0,     0,     8,     3,     4,     0,
       0,     9,     4,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     3,     2,     4,     1,     4,     4,     4,     4,
       4,     1,     6,     7,     6,     6,     7,     7,     6,     7,
       6,     6,     0,     4,     1,     0,     1,     1,     0,     1,
       0,     1,     1,     0,     1,     5,     0,     2,     0,     4,
       0,     9,     0,    10,     5,     3,     3,     4,     1,     1,
       3,     3,     3,     1,     3,     1,     3,     0,     2,     3,
       3,     1,     3,     0,     2,     3,     1,     1,     1,     2,
       3,     3,     5,     1,     1,     1,     1,     1,     0,     1,
       1,     4,     3,     3,     5,     4,     6,     5,     5,     4,
       0,     2,     0,     4,     0,     1,     0,     1,     1,     6,
       0,     6,     0,     2,     3,     5,     0,     1,     1,     0,
       5,     2,     3,     4,     0,     4,     0,     1,     1,     1,
       0,     0,     9,     0,     0,    11,     0,     2,     0,     1,
       3,     1,     1,     2,     2,     0,     1,     1,     0,     3,
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
       3,     5,     5,     6,     6,     8,     8,     0,     1,     2,
       1,     3,     3,     5,     1,     2,     1,     0,     0,     2,
       2,     1,     2,     1,     2,     1,     2,     1,     1,     2,
       1,     1,     0,     1,     5,     4,     6,     7,     5,     7,
       0,     0,    10,     0,     0,    10,     0,     0,    10,     0,
       0,     7,     1,     3,     3,     3,     1,     5,     0,     0,
      10,     1,     3,     3,     4,     4,     0,     0,    11,     0,
       0,    11,     0,     0,    10,     5,     0,     0,     9,     5,
       0,     0,    10,     1,     3,     1,     3,     3,     3,     4,
       7,     9,     0,     3,     0,     1,     9,    10,    10,    10,
       9,    10
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

  case 17: /* top_level_reader_macro: expr_reader semicolon  */
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
                CompilationError::module_already_has_a_name);
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
        if ( err ) das_yyerror(scanner,"invalid escape sequence",tokAt(scanner,(yylsp[-1])), CompilationError::invalid_escape_sequence);
        auto sc = make_smart<ExprConstString>(tokAt(scanner,(yylsp[0])),esconst);
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
            call_fmt->arguments.push_back(make_smart<ExprConstString>(tokAt(scanner,(yylsp[-1])),":" + *(yyvsp[-1].s)));
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

  case 52: /* require_module_name: require_module_name '.' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += ".";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 53: /* require_module_name: require_module_name '/' keyword_or_name  */
                                                           {
        *(yyvsp[-2].s) += "/";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 54: /* require_module: require_module_name is_public_module  */
                                                         {
        ast_requireModule(scanner,(yyvsp[-1].s),nullptr,(yyvsp[0].b),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 55: /* require_module: require_module_name "as" "name" is_public_module  */
                                                                              {
        ast_requireModule(scanner,(yyvsp[-3].s),(yyvsp[-1].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 56: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 57: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 61: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 62: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 63: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 64: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 65: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 66: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 67: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 68: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 69: /* expression_else: "else" expression_block  */
                                                           { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 70: /* expression_else: elif_or_static_elif expr expression_block expression_else  */
                                                                                          {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),
            (yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-3].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 71: /* semicolon: "end of line"  */
                       {}
    break;

  case 72: /* semicolon: "end of expression"  */
          {}
    break;

  case 73: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 74: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 75: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 76: /* $@3: %empty  */
                      { yyextra->das_need_oxford_comma = true; }
    break;

  case 77: /* expression_else_one_liner: "else" $@3 expression_if_one_liner  */
                                                                                                 {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 78: /* expression_if_one_liner: expr  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 79: /* expression_if_one_liner: expression_return_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 80: /* expression_if_one_liner: expression_yield_no_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 81: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 82: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 83: /* expression_if_then_else: if_or_static_if expr expression_block expression_else  */
                                                                                      {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),
            (yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-3].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 84: /* $@4: %empty  */
                                                     { yyextra->das_need_oxford_comma = true; }
    break;

  case 85: /* expression_if_then_else: expression_if_one_liner "if" $@4 expr expression_else_one_liner semicolon  */
                                                                                                                                                         {
        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-4])),(yyvsp[-2].pExpression),ast_wrapInBlock((yyvsp[-5].pExpression)),(yyvsp[-1].pExpression) ? ast_wrapInBlock((yyvsp[-1].pExpression)) : nullptr);
    }
    break;

  case 86: /* $@5: %empty  */
                     { yyextra->das_need_oxford_comma=false; }
    break;

  case 87: /* expression_for_loop: "for" $@5 variable_name_with_pos_list "in" expr_list expression_block  */
                                                                                                                                                 {
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-3].pNameWithPosList),(yyvsp[-1].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 88: /* expression_unsafe: "unsafe" expression_block  */
                                                 {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-1])));
        pUnsafe->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 89: /* expression_while_loop: "while" expr expression_block  */
                                                               {
        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-2])));
        pWhile->cond = (yyvsp[-1].pExpression);
        pWhile->body = (yyvsp[0].pExpression);
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 90: /* expression_with: "with" expr expression_block  */
                                                         {
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-2])));
        pWith->with = (yyvsp[-1].pExpression);
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 91: /* $@6: %empty  */
                                        { yyextra->das_need_oxford_comma=true; }
    break;

  case 92: /* expression_with_alias: "assume" "name" '=' $@6 expr  */
                                                                                               {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-4])), *(yyvsp[-3].s), (yyvsp[0].pExpression) );
        delete (yyvsp[-3].s);
    }
    break;

  case 93: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 94: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 95: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 96: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 97: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 98: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 99: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 100: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 101: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 102: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 103: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 104: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 105: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 106: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 107: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 108: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 109: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 110: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 111: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                          {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[-4].s); }
    }
    break;

  case 112: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 113: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 114: /* metadata_argument_list: '@' annotation_argument  */
                                      {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 115: /* metadata_argument_list: metadata_argument_list '@' annotation_argument  */
                                                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 116: /* metadata_argument_list: metadata_argument_list semicolon  */
                                               {
        (yyval.aaList) = (yyvsp[-1].aaList);
    }
    break;

  case 117: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 118: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 119: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 120: /* annotation_declaration_name: "template"  */
                                    { (yyval.s) = new string("template"); }
    break;

  case 121: /* annotation_declaration_basic: annotation_declaration_name  */
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

  case 122: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
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

  case 123: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 124: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 125: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
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

  case 126: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
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

  case 127: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
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

  case 128: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 129: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 130: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 131: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 132: /* optional_annotation_list: %empty  */
                                        { (yyval.faList) = nullptr; }
    break;

  case 133: /* optional_annotation_list: '[' annotation_list ']'  */
                                        { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 134: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 135: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 136: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 137: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 138: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 139: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 140: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 141: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 142: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 143: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 144: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 145: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 146: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 147: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 148: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 149: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 150: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 151: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 152: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 153: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 154: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 155: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 156: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 157: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 158: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 159: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 160: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 161: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 162: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 163: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 164: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 165: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 166: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 167: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 168: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 169: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 170: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 171: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 172: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 173: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 174: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 175: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 176: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 177: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 178: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 179: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 180: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 181: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 182: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 183: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 184: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 185: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 186: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 187: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 188: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 189: /* function_name: "operator" '.' "name" "+="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`+="); delete (yyvsp[-1].s); }
    break;

  case 190: /* function_name: "operator" '.' "name" "-="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`-="); delete (yyvsp[-1].s); }
    break;

  case 191: /* function_name: "operator" '.' "name" "*="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`*="); delete (yyvsp[-1].s); }
    break;

  case 192: /* function_name: "operator" '.' "name" "/="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`/="); delete (yyvsp[-1].s); }
    break;

  case 193: /* function_name: "operator" '.' "name" "%="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`%="); delete (yyvsp[-1].s); }
    break;

  case 194: /* function_name: "operator" '.' "name" "&="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&="); delete (yyvsp[-1].s); }
    break;

  case 195: /* function_name: "operator" '.' "name" "|="  */
                                          { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`|="); delete (yyvsp[-1].s); }
    break;

  case 196: /* function_name: "operator" '.' "name" "^="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^="); delete (yyvsp[-1].s); }
    break;

  case 197: /* function_name: "operator" '.' "name" "&&="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&&="); delete (yyvsp[-1].s); }
    break;

  case 198: /* function_name: "operator" '.' "name" "||="  */
                                            { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`||="); delete (yyvsp[-1].s); }
    break;

  case 199: /* function_name: "operator" '.' "name" "^^="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^^="); delete (yyvsp[-1].s); }
    break;

  case 200: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 201: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 202: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 203: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 204: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 205: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 206: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 207: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 208: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 209: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 210: /* function_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 211: /* function_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 212: /* function_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 213: /* function_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 214: /* function_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 215: /* function_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 216: /* function_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 217: /* function_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 218: /* function_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 219: /* function_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 220: /* function_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 221: /* function_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 222: /* function_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 223: /* function_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 224: /* function_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 225: /* function_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 226: /* function_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 227: /* function_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 228: /* function_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 229: /* function_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 230: /* function_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 231: /* function_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 232: /* function_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 233: /* function_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 234: /* function_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 235: /* global_function_declaration: optional_annotation_list "def" function_declaration  */
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

  case 236: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 237: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 238: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 239: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 240: /* $@7: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 241: /* function_declaration: optional_public_or_private_function $@7 function_declaration_header expression_block  */
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

  case 246: /* expression_block: open_block expressions close_block  */
                                                                  {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
    }
    break;

  case 247: /* expression_block: open_block expressions close_block "finally" open_block expressions close_block  */
                                                                                                                        {
        auto pB = (ExprBlock *) (yyvsp[-5].pExpression);
        auto pF = (ExprBlock *) (yyvsp[-1].pExpression);
        swap ( pB->finalList, pF->list );
        (yyval.pExpression) = (yyvsp[-5].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-6]),(yylsp[0]));
        delete (yyvsp[-1].pExpression);
    }
    break;

  case 248: /* expr_call_pipe: expr expr_full_block_assumed_piped  */
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

  case 249: /* expr_call_pipe: expression_keyword expr_full_block_assumed_piped  */
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

  case 250: /* expr_call_pipe: "generator" '<' type_declaration_no_options '>' optional_capture_list expr_full_block_assumed_piped  */
                                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-3].pTypeDecl),(yyvsp[-1].pCaptList),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])));
    }
    break;

  case 251: /* expression_any: semicolon  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 252: /* expression_any: expr_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 253: /* expression_any: expr_keyword  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 254: /* expression_any: expr_assign_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 255: /* expression_any: expr_assign semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 256: /* expression_any: expression_delete semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 257: /* expression_any: expression_let  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 258: /* expression_any: expression_while_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 259: /* expression_any: expression_unsafe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 260: /* expression_any: expression_with  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 261: /* expression_any: expression_with_alias  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 262: /* expression_any: expression_for_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 263: /* expression_any: expression_break semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 264: /* expression_any: expression_continue semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 265: /* expression_any: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 266: /* expression_any: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 267: /* expression_any: expression_if_then_else  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 268: /* expression_any: expression_try_catch  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 269: /* expression_any: expression_label semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 270: /* expression_any: expression_goto semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 271: /* expression_any: "pass" semicolon  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 272: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 273: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 274: /* expressions: expressions error  */
                                 {
        delete (yyvsp[-1].pExpression); (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 275: /* expr_keyword: "keyword" expr expression_block  */
                                                           {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s));
        pCall->arguments.push_back((yyvsp[-1].pExpression));
        auto resT = new TypeDecl(Type::autoinfer);
        auto blk = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,resT,(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])));
        pCall->arguments.push_back(blk);
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 276: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 277: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 278: /* optional_expr_list_in_braces: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 279: /* optional_expr_list_in_braces: '(' optional_expr_list optional_comma ')'  */
                                                             { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 280: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 281: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 282: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 283: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 284: /* $@8: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 285: /* $@9: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 286: /* expression_keyword: "keyword" '<' $@8 type_declaration_no_options_list '>' $@9 expr  */
                                                                                                                                                     {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 287: /* $@10: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 288: /* $@11: %empty  */
                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 289: /* expression_keyword: "type function" '<' $@10 type_declaration_no_options_list '>' $@11 optional_expr_list_in_braces  */
                                                                                                                                                                                   {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 290: /* expr_pipe: expr_assign " <|" expr_block  */
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

  case 291: /* expr_pipe: "@ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 292: /* expr_pipe: "@@ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 293: /* expr_pipe: "$ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 294: /* expr_pipe: expr_call_pipe  */
                             {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 295: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 296: /* name_in_namespace: "name" "::" "name"  */
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

  case 297: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 298: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 299: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 300: /* $@12: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 301: /* $@13: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 302: /* new_type_declaration: '<' $@12 type_declaration '>' $@13  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 303: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 304: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 305: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 306: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 307: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 308: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 309: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 310: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 311: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 312: /* expression_return_no_pipe: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 313: /* expression_return_no_pipe: "return" expr_list  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),sequenceToTuple((yyvsp[0].pExpression)));
    }
    break;

  case 314: /* expression_return_no_pipe: "return" "<-" expr_list  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),sequenceToTuple((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 315: /* expression_return: expression_return_no_pipe semicolon  */
                                                    {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 316: /* expression_return: "return" expr_pipe  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 317: /* expression_return: "return" "<-" expr_pipe  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 318: /* expression_yield_no_pipe: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 319: /* expression_yield_no_pipe: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 320: /* expression_yield: expression_yield_no_pipe semicolon  */
                                                   {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 321: /* expression_yield: "yield" expr_pipe  */
                                          {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 322: /* expression_yield: "yield" "<-" expr_pipe  */
                                                 {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 323: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 324: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 325: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 326: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 327: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 328: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 329: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 330: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 331: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 332: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 333: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-7].pNameList),tokAt(scanner,(yylsp[-7])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 334: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 335: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 336: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                           {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameList),tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 337: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 338: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr_pipe  */
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

  case 339: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 340: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr_pipe  */
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

  case 341: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 342: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 343: /* $@14: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 344: /* $@15: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 345: /* expr_cast: "cast" '<' $@14 type_declaration_no_options '>' $@15 expr  */
                                                                                                                                                {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
    }
    break;

  case 346: /* $@16: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 347: /* $@17: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 348: /* expr_cast: "upcast" '<' $@16 type_declaration_no_options '>' $@17 expr  */
                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 349: /* $@18: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 350: /* $@19: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 351: /* expr_cast: "reinterpret" '<' $@18 type_declaration_no_options '>' $@19 expr  */
                                                                                                                                                       {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 352: /* $@20: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 353: /* $@21: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 354: /* expr_type_decl: "type" '<' $@20 type_declaration '>' $@21  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 355: /* expr_type_info: "typeinfo" '(' name_in_namespace expr ')'  */
                                                                         {
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

  case 356: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" '>' expr ')'  */
                                                                                                {
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

  case 357: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" c_or_s "name" '>' expr ')'  */
                                                                                                                        {
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

  case 358: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
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

  case 359: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
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

  case 360: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" "end of expression" "name" '>' '(' expr ')'  */
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

  case 361: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 362: /* expr_list: expr_list ',' expr  */
                                            {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 363: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 364: /* block_or_simple_block: "=>" expr  */
                                        {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 365: /* block_or_simple_block: "=>" "<-" expr  */
                                               {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 366: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 367: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 368: /* block_or_lambda: '@' '@'  */
                { (yyval.i) = 2;   /* local function */ }
    break;

  case 369: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 370: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 371: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 372: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 373: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 374: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 375: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 376: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 377: /* optional_capture_list: "[[" capture_list ']' ']'  */
                                         { (yyval.pCaptList) = (yyvsp[-2].pCaptList); }
    break;

  case 378: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 379: /* expr_block: expression_block  */
                                            {
        ExprBlock * closure = (ExprBlock *) (yyvsp[0].pExpression);
        (yyval.pExpression) = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),(yyvsp[0].pExpression));
        closure->returnType = make_smart<TypeDecl>(Type::autoinfer);
    }
    break;

  case 380: /* expr_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 381: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 382: /* $@22: %empty  */
                             {  yyextra->das_need_oxford_comma = false; }
    break;

  case 383: /* expr_full_block_assumed_piped: block_or_lambda $@22 optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type expression_block  */
                                                                                       {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 384: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 385: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 386: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 387: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 388: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 389: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 390: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 391: /* expr_assign: expr  */
                                             { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 392: /* expr_assign: expr '=' expr  */
                                             { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 393: /* expr_assign: expr "<-" expr  */
                                             { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 394: /* expr_assign: expr ":=" expr  */
                                             { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 395: /* expr_assign: expr "&=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 396: /* expr_assign: expr "|=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 397: /* expr_assign: expr "^=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 398: /* expr_assign: expr "&&=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 399: /* expr_assign: expr "||=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 400: /* expr_assign: expr "^^=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 401: /* expr_assign: expr "+=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 402: /* expr_assign: expr "-=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 403: /* expr_assign: expr "*=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 404: /* expr_assign: expr "/=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 405: /* expr_assign: expr "%=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 406: /* expr_assign: expr "<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 407: /* expr_assign: expr ">>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 408: /* expr_assign: expr "<<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 409: /* expr_assign: expr ">>>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 410: /* expr_assign_pipe_right: "@ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 411: /* expr_assign_pipe_right: "@@ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 412: /* expr_assign_pipe_right: "$ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 413: /* expr_assign_pipe_right: expr_call_pipe  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 414: /* expr_assign_pipe: expr '=' expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 415: /* expr_assign_pipe: expr "<-" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 416: /* expr_assign_pipe: expr "&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 417: /* expr_assign_pipe: expr "|=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 418: /* expr_assign_pipe: expr "^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 419: /* expr_assign_pipe: expr "&&=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 420: /* expr_assign_pipe: expr "||=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 421: /* expr_assign_pipe: expr "^^=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 422: /* expr_assign_pipe: expr "+=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 423: /* expr_assign_pipe: expr "-=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 424: /* expr_assign_pipe: expr "*=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 425: /* expr_assign_pipe: expr "/=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 426: /* expr_assign_pipe: expr "%=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 427: /* expr_assign_pipe: expr "<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 428: /* expr_assign_pipe: expr ">>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 429: /* expr_assign_pipe: expr "<<<=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 430: /* expr_assign_pipe: expr ">>>=" expr_assign_pipe_right  */
                                                                  { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 431: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 432: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 433: /* expr_method_call: expr "->" "name" '(' ')'  */
                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 434: /* expr_method_call: expr "->" "name" '(' expr_list ')'  */
                                                                              {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 435: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 436: /* func_addr_name: "$i" '(' expr ')'  */
                                          {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 437: /* func_addr_expr: '@' '@' func_addr_name  */
                                          {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 438: /* $@23: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 439: /* $@24: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 440: /* func_addr_expr: '@' '@' '<' $@23 type_declaration_no_options '>' $@24 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value.get()));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 441: /* $@25: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 442: /* $@26: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 443: /* func_addr_expr: '@' '@' '<' $@25 optional_function_argument_list optional_function_type '>' $@26 func_addr_name  */
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

  case 444: /* expr_field: expr '.' "name"  */
                                              {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 445: /* expr_field: expr '.' '.' "name"  */
                                                  {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 446: /* expr_field: expr '.' "name" '(' ')'  */
                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 447: /* expr_field: expr '.' "name" '(' expr_list ')'  */
                                                                           {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 448: /* expr_field: expr '.' "name" '(' '[' make_struct_fields ']' ')'  */
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

  case 449: /* expr_field: expr '.' basic_type_declaration '(' ')'  */
                                                                        {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 450: /* expr_field: expr '.' basic_type_declaration '(' expr_list ')'  */
                                                                                             {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 451: /* $@27: %empty  */
                               { yyextra->das_suppress_errors=true; }
    break;

  case 452: /* $@28: %empty  */
                                                                            { yyextra->das_suppress_errors=false; }
    break;

  case 453: /* expr_field: expr '.' $@27 error $@28  */
                                                                                                                    {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), "");
        yyerrok;
    }
    break;

  case 454: /* expr_call: name_in_namespace '(' ')'  */
                                               {
            (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),*(yyvsp[-2].s));
            delete (yyvsp[-2].s);
    }
    break;

  case 455: /* expr_call: name_in_namespace '(' "uninitialized" ')'  */
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

  case 456: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
                                                               {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 457: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
                                                                                 {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-4])),*(yyvsp[-4].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-4].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 458: /* expr_call: name_in_namespace '(' expr_list ')'  */
                                                                    {
            (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),*(yyvsp[-3].s)),(yyvsp[-1].pExpression));
            delete (yyvsp[-3].s);
    }
    break;

  case 459: /* expr_call: basic_type_declaration '(' ')'  */
                                                    {
        (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-2].type)));
    }
    break;

  case 460: /* expr_call: basic_type_declaration '(' expr_list ')'  */
                                                                         {
        (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-3].type))),(yyvsp[-1].pExpression));
    }
    break;

  case 461: /* expr: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 462: /* expr: name_in_namespace  */
                                              { (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 463: /* expr: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 464: /* expr: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 465: /* expr: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 466: /* expr: make_decl  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 467: /* expr: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 468: /* expr: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 469: /* expr: expr_field  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 470: /* expr: expr_mtag  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 471: /* expr: '!' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",(yyvsp[0].pExpression)); }
    break;

  case 472: /* expr: '~' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",(yyvsp[0].pExpression)); }
    break;

  case 473: /* expr: '+' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",(yyvsp[0].pExpression)); }
    break;

  case 474: /* expr: '-' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",(yyvsp[0].pExpression)); }
    break;

  case 475: /* expr: expr "<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 476: /* expr: expr ">>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 477: /* expr: expr "<<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 478: /* expr: expr ">>>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 479: /* expr: expr '+' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 480: /* expr: expr '-' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 481: /* expr: expr '*' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 482: /* expr: expr '/' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 483: /* expr: expr '%' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 484: /* expr: expr '<' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 485: /* expr: expr '>' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 486: /* expr: expr "==" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 487: /* expr: expr "!=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 488: /* expr: expr "<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 489: /* expr: expr ">=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 490: /* expr: expr '&' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 491: /* expr: expr '|' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 492: /* expr: expr '^' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 493: /* expr: expr "&&" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 494: /* expr: expr "||" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 495: /* expr: expr "^^" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 496: /* expr: expr ".." expr  */
                                             {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 497: /* expr: "++" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", (yyvsp[0].pExpression)); }
    break;

  case 498: /* expr: "--" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", (yyvsp[0].pExpression)); }
    break;

  case 499: /* expr: expr "++"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 500: /* expr: expr "--"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 501: /* expr: '(' expr_list optional_comma ')'  */
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

  case 502: /* expr: '(' make_struct_single ')'  */
                                      {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        for ( auto & arg : *(((ExprMakeStruct *)(yyvsp[-1].pExpression))->structs.back()) ) {
            mkt->values.push_back(arg->value);
            mkt->recordNames.push_back(arg->name);
        }
        delete (yyvsp[-1].pExpression);
        (yyval.pExpression) = mkt;
    }
    break;

  case 503: /* expr: expr '[' expr ']'  */
                                                 { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 504: /* expr: expr '.' '[' expr ']'  */
                                                     { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 505: /* expr: expr "?[" expr ']'  */
                                                 { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 506: /* expr: expr '.' "?[" expr ']'  */
                                                     { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 507: /* expr: expr "?." "name"  */
                                                 { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 508: /* expr: expr '.' "?." "name"  */
                                                     { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 509: /* expr: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 510: /* expr: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 511: /* expr: '*' expr  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 512: /* expr: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 513: /* expr: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 514: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])));
    }
    break;

  case 515: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])));
    }
    break;

  case 516: /* expr: expr "??" expr  */
                                                   { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 517: /* expr: expr '?' expr ':' expr  */
                                                          {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        }
    break;

  case 518: /* $@29: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 519: /* $@30: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 520: /* expr: expr "is" "type" '<' $@29 type_declaration_no_options '>' $@30  */
                                                                                                                                                       {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 521: /* expr: expr "is" basic_type_declaration  */
                                                               {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 522: /* expr: expr "is" "name"  */
                                              {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 523: /* expr: expr "as" "name"  */
                                              {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 524: /* $@31: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 525: /* $@32: %empty  */
                                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 526: /* expr: expr "as" "type" '<' $@31 type_declaration '>' $@32  */
                                                                                                                                            {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 527: /* expr: expr "as" basic_type_declaration  */
                                                               {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 528: /* expr: expr '?' "as" "name"  */
                                                  {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 529: /* $@33: %empty  */
                                                   { yyextra->das_arrow_depth ++; }
    break;

  case 530: /* $@34: %empty  */
                                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 531: /* expr: expr '?' "as" "type" '<' $@33 type_declaration '>' $@34  */
                                                                                                                                                {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 532: /* expr: expr '?' "as" basic_type_declaration  */
                                                                   {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 533: /* expr: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 534: /* expr: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 535: /* expr: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 536: /* expr: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 537: /* expr: expr_method_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 538: /* expr: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 539: /* expr: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 540: /* expr: expr "<|" expr  */
                                                { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 541: /* expr: expr "|>" expr  */
                                                { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 542: /* expr: expr "|>" basic_type_declaration  */
                                                          {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 543: /* expr: name_in_namespace "name"  */
                                                { (yyval.pExpression) = ast_NameName(scanner,(yyvsp[-1].s),(yyvsp[0].s),tokAt(scanner,(yylsp[-1])),tokAt(scanner,(yylsp[0]))); }
    break;

  case 544: /* expr: "unsafe" '(' expr ')'  */
                                         {
        (yyvsp[-1].pExpression)->alwaysSafe = true;
        (yyvsp[-1].pExpression)->userSaidItsSafe = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 545: /* expr: expression_keyword  */
                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 546: /* expr_mtag: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 547: /* expr_mtag: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 548: /* expr_mtag: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 549: /* expr_mtag: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 550: /* expr_mtag: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 551: /* expr_mtag: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 552: /* expr_mtag: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 553: /* expr_mtag: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 554: /* expr_mtag: expr '.' "$f" '(' expr ')'  */
                                                                {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 555: /* expr_mtag: expr "?." "$f" '(' expr ')'  */
                                                                 {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 556: /* expr_mtag: expr '.' '.' "$f" '(' expr ')'  */
                                                                    {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 557: /* expr_mtag: expr '.' "?." "$f" '(' expr ')'  */
                                                                     {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 558: /* expr_mtag: expr "as" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 559: /* expr_mtag: expr '?' "as" "$f" '(' expr ')'  */
                                                                       {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 560: /* expr_mtag: expr "is" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 561: /* expr_mtag: '@' '@' "$c" '(' expr ')'  */
                                                         {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 562: /* optional_field_annotation: %empty  */
                                                        { (yyval.aaList) = nullptr; }
    break;

  case 563: /* optional_field_annotation: "[[" annotation_argument_list ']' ']'  */
                                                        { (yyval.aaList) = (yyvsp[-2].aaList); /*this one is gone when BRABRA is disabled*/ }
    break;

  case 564: /* optional_field_annotation: metadata_argument_list  */
                                                        { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 565: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 566: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 567: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 568: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 569: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 570: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 571: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 572: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 573: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 574: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 575: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 576: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 577: /* struct_variable_declaration_list: struct_variable_declaration_list semicolon  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 578: /* $@35: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 579: /* struct_variable_declaration_list: struct_variable_declaration_list $@35 structure_variable_declaration semicolon  */
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

  case 580: /* $@36: %empty  */
                                                                                                                     {
                yyextra->das_force_oxford_comma=true;
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 581: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable "abstract" optional_constant $@36 function_declaration_header semicolon  */
                                                          {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 582: /* $@37: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 583: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@37 function_declaration_header expression_block  */
                                                                        {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-9].pVarDeclList),(yyvsp[-8].faList),(yyvsp[-5].b),(yyvsp[-6].b),(yyvsp[-4].i),(yyvsp[-3].b),(yyvsp[-1].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-7]),(yylsp[0])),tokAt(scanner,(yylsp[-8])));
            }
    break;

  case 584: /* struct_variable_declaration_list: struct_variable_declaration_list '[' annotation_list ']' semicolon  */
                                                                                       {
        das_yyerror(scanner,"structure field or class method annotation expected to remain on the same line with the field or the class",
            tokAt(scanner,(yylsp[-2])), CompilationError::syntax_error);
        delete (yyvsp[-2].faList);
        (yyval.pVarDeclList) = (yyvsp[-4].pVarDeclList);
    }
    break;

  case 585: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 586: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 587: /* function_argument_declaration_type: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 588: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 589: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 590: /* function_argument_list: function_argument_declaration_no_type semicolon function_argument_list  */
                                                                                            { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 591: /* function_argument_list: function_argument_declaration_type semicolon function_argument_list  */
                                                                                            { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 592: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 593: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 594: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 595: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 596: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                          { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 597: /* tuple_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 598: /* tuple_alias_type_list: tuple_alias_type_list c_or_s  */
                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 599: /* tuple_alias_type_list: tuple_alias_type_list tuple_type c_or_s  */
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

  case 600: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 601: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 602: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 603: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 604: /* variant_alias_type_list: variant_alias_type_list c_or_s  */
                                           {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 605: /* variant_alias_type_list: variant_alias_type_list variant_type c_or_s  */
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

  case 606: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 607: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 608: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 609: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 610: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 611: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 612: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 613: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 614: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 615: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 616: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 617: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 618: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 619: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 620: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 621: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 622: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 623: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 624: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 625: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options semicolon  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 626: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 627: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 628: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr semicolon  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 629: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr_pipe  */
                                                                                                           {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-3]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 630: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 631: /* global_variable_declaration_list: global_variable_declaration_list "end of line"  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 632: /* $@38: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 633: /* global_variable_declaration_list: global_variable_declaration_list $@38 optional_field_annotation let_variable_declaration  */
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

  case 634: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 635: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 636: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 637: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 638: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 639: /* global_let: kwd_let optional_shared optional_public_or_private_variable open_block global_variable_declaration_list close_block  */
                                                                                                                                                      {
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 640: /* $@39: %empty  */
                                                                                        {
        yyextra->das_force_oxford_comma=true;
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 641: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@39 optional_field_annotation let_variable_declaration  */
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

  case 642: /* enum_list: %empty  */
        {
        (yyval.pEnum) = new Enumeration();
    }
    break;

  case 643: /* enum_list: enum_list semicolon  */
                                {
        (yyval.pEnum) = (yyvsp[-1].pEnum);
    }
    break;

  case 644: /* enum_list: enum_list "name" semicolon  */
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

  case 645: /* enum_list: enum_list "name" '=' expr semicolon  */
                                                           {
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

  case 646: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 647: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 648: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 649: /* $@40: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 650: /* single_alias: optional_public_or_private_alias "name" $@40 '=' type_declaration  */
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
        delete (yyvsp[-3].s);
    }
    break;

  case 654: /* $@41: %empty  */
                    { yyextra->das_force_oxford_comma=true;}
    break;

  case 656: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 657: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 658: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 659: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 660: /* $@42: %empty  */
                                                                                                                       {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 661: /* $@43: %empty  */
                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
    }
    break;

  case 662: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name open_block $@42 enum_list $@43 close_block  */
                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-5].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-8].faList),tokAt(scanner,(yylsp[-8])),(yyvsp[-6].b),(yyvsp[-5].pEnum),(yyvsp[-2].pEnum),Type::tInt);
    }
    break;

  case 663: /* $@44: %empty  */
                                                                                                                                                            {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 664: /* $@45: %empty  */
                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
    }
    break;

  case 665: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name ':' enum_basic_type_declaration open_block $@44 enum_list $@45 close_block  */
                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-7].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-10].faList),tokAt(scanner,(yylsp[-10])),(yyvsp[-8].b),(yyvsp[-7].pEnum),(yyvsp[-2].pEnum),(yyvsp[-5].type));
    }
    break;

  case 666: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 667: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 668: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 669: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 670: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 671: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 672: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 673: /* class_or_struct: "template" "class"  */
                                 { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 674: /* class_or_struct: "template" "struct"  */
                                 { (yyval.i) = CorS_StructTemplate; }
    break;

  case 675: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 676: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 677: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 678: /* optional_struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 679: /* optional_struct_variable_declaration_list: open_block struct_variable_declaration_list close_block  */
                                                                      {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 680: /* $@46: %empty  */
                                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 681: /* $@47: %empty  */
                         {
        if ( (yyvsp[0].pStructure) ) {
            (yyvsp[0].pStructure)->isClass = (yyvsp[-3].i)==CorS_Class || (yyvsp[-3].i)==CorS_ClassTemplate;
            (yyvsp[0].pStructure)->isTemplate = (yyvsp[-3].i)==CorS_ClassTemplate || (yyvsp[-3].i)==CorS_StructTemplate;
            (yyvsp[0].pStructure)->privateStructure = !(yyvsp[-2].b);
        }
    }
    break;

  case 682: /* structure_declaration: optional_annotation_list class_or_struct optional_public_or_private_structure $@46 structure_name $@47 optional_struct_variable_declaration_list  */
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

  case 683: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 684: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 685: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 686: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 687: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 688: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 689: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 690: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 691: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 692: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 693: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 694: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 695: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 696: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 697: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 698: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 699: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 700: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 701: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 702: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 703: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 704: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 705: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 706: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 707: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 708: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 709: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 710: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 711: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 712: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 713: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 714: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 715: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 716: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 717: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 718: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 719: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 720: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 721: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 722: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 723: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 724: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 725: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 726: /* auto_type_declaration: "$t" '(' expr ')'  */
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

  case 727: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 728: /* bitfield_bits: bitfield_bits semicolon "name"  */
                                                 {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 729: /* bitfield_alias_bits: %empty  */
        {
        auto pSL = new vector<string>();
        (yyval.pNameList) = pSL;

    }
    break;

  case 730: /* bitfield_alias_bits: bitfield_alias_bits semicolon  */
                                            {
        (yyval.pNameList) = (yyvsp[-1].pNameList);
    }
    break;

  case 731: /* bitfield_alias_bits: bitfield_alias_bits "name" semicolon  */
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

  case 732: /* $@48: %empty  */
                                     { yyextra->das_arrow_depth ++; }
    break;

  case 733: /* $@49: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 734: /* bitfield_type_declaration: "bitfield" '<' $@48 bitfield_bits '>' $@49  */
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

  case 737: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 738: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 739: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 740: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 741: /* type_declaration_no_options: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 742: /* type_declaration_no_options: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 743: /* type_declaration_no_options: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 744: /* type_declaration_no_options: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 745: /* type_declaration_no_options: type_declaration_no_options dim_list  */
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

  case 746: /* type_declaration_no_options: type_declaration_no_options '[' ']'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->dim.push_back(TypeDecl::dimAuto);
        (yyvsp[-2].pTypeDecl)->dimExpr.push_back(nullptr);
        (yyvsp[-2].pTypeDecl)->removeDim = false;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 747: /* $@50: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 748: /* $@51: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 749: /* type_declaration_no_options: "type" '<' $@50 type_declaration '>' $@51  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 750: /* type_declaration_no_options: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 751: /* type_declaration_no_options: '$' name_in_namespace '(' optional_expr_list ')'  */
                                                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-3])), *(yyvsp[-3].s)));
        delete (yyvsp[-3].s);
    }
    break;

  case 752: /* $@52: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 753: /* type_declaration_no_options: '$' name_in_namespace '<' $@52 type_declaration_no_options_list '>' '(' optional_expr_list ')'  */
                                                                                                                                                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-4].pTypeDeclList),(yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-7])), *(yyvsp[-7].s)));
        delete (yyvsp[-7].s);
    }
    break;

  case 754: /* type_declaration_no_options: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 755: /* type_declaration_no_options: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 756: /* type_declaration_no_options: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 757: /* type_declaration_no_options: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 758: /* type_declaration_no_options: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 759: /* type_declaration_no_options: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 760: /* type_declaration_no_options: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 761: /* type_declaration_no_options: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 762: /* type_declaration_no_options: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 763: /* type_declaration_no_options: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 764: /* type_declaration_no_options: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 765: /* type_declaration_no_options: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 766: /* $@53: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 767: /* $@54: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 768: /* type_declaration_no_options: "smart_ptr" '<' $@53 type_declaration '>' $@54  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 769: /* type_declaration_no_options: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 770: /* $@55: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 771: /* $@56: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 772: /* type_declaration_no_options: "array" '<' $@55 type_declaration '>' $@56  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 773: /* $@57: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 774: /* $@58: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 775: /* type_declaration_no_options: "table" '<' $@57 table_type_pair '>' $@58  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 776: /* $@59: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 777: /* $@60: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 778: /* type_declaration_no_options: "iterator" '<' $@59 type_declaration '>' $@60  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 779: /* type_declaration_no_options: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 780: /* $@61: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 781: /* $@62: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 782: /* type_declaration_no_options: "block" '<' $@61 type_declaration '>' $@62  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 783: /* $@63: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 784: /* $@64: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 785: /* type_declaration_no_options: "block" '<' $@63 optional_function_argument_list optional_function_type '>' $@64  */
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

  case 786: /* type_declaration_no_options: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 787: /* $@65: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 788: /* $@66: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 789: /* type_declaration_no_options: "function" '<' $@65 type_declaration '>' $@66  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 790: /* $@67: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 791: /* $@68: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 792: /* type_declaration_no_options: "function" '<' $@67 optional_function_argument_list optional_function_type '>' $@68  */
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

  case 793: /* type_declaration_no_options: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 794: /* $@69: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 795: /* $@70: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 796: /* type_declaration_no_options: "lambda" '<' $@69 type_declaration '>' $@70  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 797: /* $@71: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 798: /* $@72: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 799: /* type_declaration_no_options: "lambda" '<' $@71 optional_function_argument_list optional_function_type '>' $@72  */
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

  case 800: /* $@73: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 801: /* $@74: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 802: /* type_declaration_no_options: "tuple" '<' $@73 tuple_type_list '>' $@74  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 803: /* $@75: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 804: /* $@76: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 805: /* type_declaration_no_options: "variant" '<' $@75 variant_type_list '>' $@76  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 806: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 807: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 808: /* type_declaration: type_declaration '|' '#'  */
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

  case 809: /* $@77: %empty  */
                                                          { yyextra->das_need_oxford_comma=false; }
    break;

  case 810: /* $@78: %empty  */
                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 811: /* $@79: %empty  */
                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 812: /* $@80: %empty  */
                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
    }
    break;

  case 813: /* tuple_alias_declaration: "tuple" optional_public_or_private_alias $@77 "name" $@78 open_block $@79 tuple_alias_type_list $@80 close_block  */
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

  case 814: /* $@81: %empty  */
                                                            { yyextra->das_need_oxford_comma=false; }
    break;

  case 815: /* $@82: %empty  */
                                                                                                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 816: /* $@83: %empty  */
                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 817: /* $@84: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
    }
    break;

  case 818: /* variant_alias_declaration: "variant" optional_public_or_private_alias $@81 "name" $@82 open_block $@83 variant_alias_type_list $@84 close_block  */
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

  case 819: /* $@85: %empty  */
                                                             { yyextra->das_need_oxford_comma=false; }
    break;

  case 820: /* $@86: %empty  */
                                                                                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 821: /* $@87: %empty  */
                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 822: /* $@88: %empty  */
                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
    }
    break;

  case 823: /* bitfield_alias_declaration: "bitfield" optional_public_or_private_alias $@85 "name" $@86 open_block $@87 bitfield_alias_bits $@88 close_block  */
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
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 830: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 831: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 832: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 833: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 834: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 835: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 836: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
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

  case 841: /* make_struct_dim: make_struct_dim "end of expression" make_struct_fields  */
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
                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 862: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 863: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 864: /* make_struct_decl: "[[" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-4]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 865: /* make_struct_decl: "[[" type_declaration_no_options optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->makeType = (yyvsp[-2].pTypeDecl);
        msd->block = (yyvsp[-1].pExpression);
        msd->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = msd;
    }
    break;

  case 866: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                   {
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
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-5].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 868: /* make_struct_decl: "[{" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
                                                                                                                                {
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
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-5].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-6]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),"to_array_move");
        tam->arguments.push_back((yyvsp[-2].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 870: /* $@89: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 871: /* $@90: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 872: /* make_struct_decl: "struct" '<' $@89 type_declaration_no_options '>' $@90 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                            {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 873: /* $@91: %empty  */
                            { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 874: /* $@92: %empty  */
                                                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 875: /* make_struct_decl: "class" '<' $@91 type_declaration_no_options '>' $@92 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                           {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 876: /* $@93: %empty  */
                               { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 877: /* $@94: %empty  */
                                                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 878: /* make_struct_decl: "variant" '<' $@93 variant_type_list '>' $@94 '(' use_initializer make_variant_dim ')'  */
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

  case 879: /* $@95: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 880: /* $@96: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 881: /* make_struct_decl: "default" '<' $@95 type_declaration_no_options '>' $@96 use_initializer  */
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
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 883: /* make_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 884: /* make_tuple: make_tuple ',' expr  */
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

  case 885: /* make_map_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
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

  case 888: /* $@97: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 889: /* $@98: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 890: /* make_tuple_call: "tuple" '<' $@97 tuple_type_list '>' $@98 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 892: /* make_dim: make_dim "end of expression" make_tuple  */
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
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-2].pTypeDecl);
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 895: /* make_dim_decl: "[{" type_declaration_no_options make_dim optional_trailing_semicolon_cur_sqr  */
                                                                                                         {
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-2].pTypeDecl);
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 896: /* $@99: %empty  */
                                       { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 897: /* $@100: %empty  */
                                                                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 898: /* make_dim_decl: "array" "struct" '<' $@99 type_declaration_no_options '>' $@100 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 899: /* $@101: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 900: /* $@102: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 901: /* make_dim_decl: "array" "tuple" '<' $@101 tuple_type_list '>' $@102 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 902: /* $@103: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 903: /* $@104: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 904: /* make_dim_decl: "array" "variant" '<' $@103 variant_type_list '>' $@104 '(' make_variant_dim ')'  */
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

  case 906: /* $@105: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 907: /* $@106: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 908: /* make_dim_decl: "array" '<' $@105 type_declaration_no_options '>' $@106 '(' optional_expr_list ')'  */
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

  case 910: /* $@107: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 911: /* $@108: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 912: /* make_dim_decl: "fixed_array" '<' $@107 type_declaration_no_options '>' $@108 '(' expr_list optional_comma ')'  */
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

  case 914: /* make_table: make_table "end of expression" make_map_tuple  */
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

  case 917: /* make_table_decl: "begin of code block" optional_expr_map_tuple_list "end of code block"  */
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

  case 923: /* array_comprehension_where: "end of expression" "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 924: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 925: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 926: /* array_comprehension: '[' "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                    {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 927: /* array_comprehension: '[' "iterator" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                                 {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 928: /* array_comprehension: "[[" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']' ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),true,false);
    }
    break;

  case 929: /* array_comprehension: "[{" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where "end of code block" ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),false,false);
    }
    break;

  case 930: /* array_comprehension: "begin of code block" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where "end of code block"  */
                                                                                                                                                              {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
    }
    break;

  case 931: /* array_comprehension: "{{" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where "end of code block" "end of code block"  */
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
