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
  YYSYMBOL_266_7 = 266,                    /* $@7  */
  YYSYMBOL_annotation_argument_value = 267, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 268, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 269, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 270,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 271, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 272,   /* metadata_argument_list  */
  YYSYMBOL_annotation_declaration_name = 273, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 274, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 275,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 276,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 277, /* optional_annotation_list  */
  YYSYMBOL_optional_function_argument_list = 278, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 279,   /* optional_function_type  */
  YYSYMBOL_function_name = 280,            /* function_name  */
  YYSYMBOL_optional_template = 281,        /* optional_template  */
  YYSYMBOL_global_function_declaration = 282, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 283, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 284, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 285,     /* function_declaration  */
  YYSYMBOL_286_8 = 286,                    /* $@8  */
  YYSYMBOL_open_block = 287,               /* open_block  */
  YYSYMBOL_close_block = 288,              /* close_block  */
  YYSYMBOL_expression_block = 289,         /* expression_block  */
  YYSYMBOL_expr_call_pipe = 290,           /* expr_call_pipe  */
  YYSYMBOL_expression_any = 291,           /* expression_any  */
  YYSYMBOL_expressions = 292,              /* expressions  */
  YYSYMBOL_expr_keyword = 293,             /* expr_keyword  */
  YYSYMBOL_optional_expr_list = 294,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_list_in_braces = 295, /* optional_expr_list_in_braces  */
  YYSYMBOL_optional_expr_map_tuple_list = 296, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 297, /* type_declaration_no_options_list  */
  YYSYMBOL_expression_keyword = 298,       /* expression_keyword  */
  YYSYMBOL_299_9 = 299,                    /* $@9  */
  YYSYMBOL_300_10 = 300,                   /* $@10  */
  YYSYMBOL_301_11 = 301,                   /* $@11  */
  YYSYMBOL_302_12 = 302,                   /* $@12  */
  YYSYMBOL_expr_pipe = 303,                /* expr_pipe  */
  YYSYMBOL_name_in_namespace = 304,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 305,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 306,     /* new_type_declaration  */
  YYSYMBOL_307_13 = 307,                   /* $@13  */
  YYSYMBOL_308_14 = 308,                   /* $@14  */
  YYSYMBOL_expr_new = 309,                 /* expr_new  */
  YYSYMBOL_expression_break = 310,         /* expression_break  */
  YYSYMBOL_expression_continue = 311,      /* expression_continue  */
  YYSYMBOL_expression_return_no_pipe = 312, /* expression_return_no_pipe  */
  YYSYMBOL_expression_return = 313,        /* expression_return  */
  YYSYMBOL_expression_yield_no_pipe = 314, /* expression_yield_no_pipe  */
  YYSYMBOL_expression_yield = 315,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 316,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 317,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 318,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 319,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 320,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 321, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 322,           /* expression_let  */
  YYSYMBOL_expr_cast = 323,                /* expr_cast  */
  YYSYMBOL_324_15 = 324,                   /* $@15  */
  YYSYMBOL_325_16 = 325,                   /* $@16  */
  YYSYMBOL_326_17 = 326,                   /* $@17  */
  YYSYMBOL_327_18 = 327,                   /* $@18  */
  YYSYMBOL_328_19 = 328,                   /* $@19  */
  YYSYMBOL_329_20 = 329,                   /* $@20  */
  YYSYMBOL_expr_type_decl = 330,           /* expr_type_decl  */
  YYSYMBOL_331_21 = 331,                   /* $@21  */
  YYSYMBOL_332_22 = 332,                   /* $@22  */
  YYSYMBOL_expr_type_info = 333,           /* expr_type_info  */
  YYSYMBOL_expr_list = 334,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 335,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 336,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 337,            /* capture_entry  */
  YYSYMBOL_capture_list = 338,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 339,    /* optional_capture_list  */
  YYSYMBOL_expr_block = 340,               /* expr_block  */
  YYSYMBOL_expr_full_block = 341,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 342, /* expr_full_block_assumed_piped  */
  YYSYMBOL_343_23 = 343,                   /* $@23  */
  YYSYMBOL_expr_numeric_const = 344,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign = 345,              /* expr_assign  */
  YYSYMBOL_expr_assign_pipe_right = 346,   /* expr_assign_pipe_right  */
  YYSYMBOL_expr_assign_pipe = 347,         /* expr_assign_pipe  */
  YYSYMBOL_expr_named_call = 348,          /* expr_named_call  */
  YYSYMBOL_expr_method_call = 349,         /* expr_method_call  */
  YYSYMBOL_func_addr_name = 350,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 351,           /* func_addr_expr  */
  YYSYMBOL_352_24 = 352,                   /* $@24  */
  YYSYMBOL_353_25 = 353,                   /* $@25  */
  YYSYMBOL_354_26 = 354,                   /* $@26  */
  YYSYMBOL_355_27 = 355,                   /* $@27  */
  YYSYMBOL_expr_field = 356,               /* expr_field  */
  YYSYMBOL_357_28 = 357,                   /* $@28  */
  YYSYMBOL_358_29 = 358,                   /* $@29  */
  YYSYMBOL_expr_call = 359,                /* expr_call  */
  YYSYMBOL_expr = 360,                     /* expr  */
  YYSYMBOL_361_30 = 361,                   /* $@30  */
  YYSYMBOL_362_31 = 362,                   /* $@31  */
  YYSYMBOL_363_32 = 363,                   /* $@32  */
  YYSYMBOL_364_33 = 364,                   /* $@33  */
  YYSYMBOL_365_34 = 365,                   /* $@34  */
  YYSYMBOL_366_35 = 366,                   /* $@35  */
  YYSYMBOL_expr_mtag = 367,                /* expr_mtag  */
  YYSYMBOL_optional_field_annotation = 368, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 369,        /* optional_override  */
  YYSYMBOL_optional_constant = 370,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 371, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 372, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 373, /* structure_variable_declaration  */
  YYSYMBOL_struct_variable_declaration_list = 374, /* struct_variable_declaration_list  */
  YYSYMBOL_375_36 = 375,                   /* $@36  */
  YYSYMBOL_376_37 = 376,                   /* $@37  */
  YYSYMBOL_377_38 = 377,                   /* $@38  */
  YYSYMBOL_378_39 = 378,                   /* $@39  */
  YYSYMBOL_function_argument_declaration_no_type = 379, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 380, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 381,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 382,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 383,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 384,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 385,             /* variant_type  */
  YYSYMBOL_variant_type_list = 386,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 387,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 388,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 389, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 390, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 391,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 392,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 393,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 394, /* let_variable_name_with_pos_list  */
  YYSYMBOL_let_variable_declaration = 395, /* let_variable_declaration  */
  YYSYMBOL_global_variable_declaration_list = 396, /* global_variable_declaration_list  */
  YYSYMBOL_397_40 = 397,                   /* $@40  */
  YYSYMBOL_optional_shared = 398,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 399, /* optional_public_or_private_variable  */
  YYSYMBOL_global_let = 400,               /* global_let  */
  YYSYMBOL_401_41 = 401,                   /* $@41  */
  YYSYMBOL_enum_list = 402,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 403, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 404,             /* single_alias  */
  YYSYMBOL_405_42 = 405,                   /* $@42  */
  YYSYMBOL_alias_list = 406,               /* alias_list  */
  YYSYMBOL_alias_declaration = 407,        /* alias_declaration  */
  YYSYMBOL_408_43 = 408,                   /* $@43  */
  YYSYMBOL_optional_public_or_private_enum = 409, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 410,                /* enum_name  */
  YYSYMBOL_enum_declaration = 411,         /* enum_declaration  */
  YYSYMBOL_412_44 = 412,                   /* $@44  */
  YYSYMBOL_413_45 = 413,                   /* $@45  */
  YYSYMBOL_414_46 = 414,                   /* $@46  */
  YYSYMBOL_415_47 = 415,                   /* $@47  */
  YYSYMBOL_optional_structure_parent = 416, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 417,          /* optional_sealed  */
  YYSYMBOL_structure_name = 418,           /* structure_name  */
  YYSYMBOL_class_or_struct = 419,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 420, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 421, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 422,    /* structure_declaration  */
  YYSYMBOL_423_48 = 423,                   /* $@48  */
  YYSYMBOL_424_49 = 424,                   /* $@49  */
  YYSYMBOL_variable_name_with_pos_list = 425, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 426,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 427, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 428, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 429,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 430,            /* bitfield_bits  */
  YYSYMBOL_bitfield_alias_bits = 431,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_basic_type_declaration = 432, /* bitfield_basic_type_declaration  */
  YYSYMBOL_bitfield_type_declaration = 433, /* bitfield_type_declaration  */
  YYSYMBOL_434_50 = 434,                   /* $@50  */
  YYSYMBOL_435_51 = 435,                   /* $@51  */
  YYSYMBOL_c_or_s = 436,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 437,          /* table_type_pair  */
  YYSYMBOL_dim_list = 438,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 439, /* type_declaration_no_options  */
  YYSYMBOL_440_52 = 440,                   /* $@52  */
  YYSYMBOL_441_53 = 441,                   /* $@53  */
  YYSYMBOL_442_54 = 442,                   /* $@54  */
  YYSYMBOL_443_55 = 443,                   /* $@55  */
  YYSYMBOL_444_56 = 444,                   /* $@56  */
  YYSYMBOL_445_57 = 445,                   /* $@57  */
  YYSYMBOL_446_58 = 446,                   /* $@58  */
  YYSYMBOL_447_59 = 447,                   /* $@59  */
  YYSYMBOL_448_60 = 448,                   /* $@60  */
  YYSYMBOL_449_61 = 449,                   /* $@61  */
  YYSYMBOL_450_62 = 450,                   /* $@62  */
  YYSYMBOL_451_63 = 451,                   /* $@63  */
  YYSYMBOL_452_64 = 452,                   /* $@64  */
  YYSYMBOL_453_65 = 453,                   /* $@65  */
  YYSYMBOL_454_66 = 454,                   /* $@66  */
  YYSYMBOL_455_67 = 455,                   /* $@67  */
  YYSYMBOL_456_68 = 456,                   /* $@68  */
  YYSYMBOL_457_69 = 457,                   /* $@69  */
  YYSYMBOL_458_70 = 458,                   /* $@70  */
  YYSYMBOL_459_71 = 459,                   /* $@71  */
  YYSYMBOL_460_72 = 460,                   /* $@72  */
  YYSYMBOL_461_73 = 461,                   /* $@73  */
  YYSYMBOL_462_74 = 462,                   /* $@74  */
  YYSYMBOL_463_75 = 463,                   /* $@75  */
  YYSYMBOL_464_76 = 464,                   /* $@76  */
  YYSYMBOL_465_77 = 465,                   /* $@77  */
  YYSYMBOL_466_78 = 466,                   /* $@78  */
  YYSYMBOL_type_declaration = 467,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 468,  /* tuple_alias_declaration  */
  YYSYMBOL_469_79 = 469,                   /* $@79  */
  YYSYMBOL_470_80 = 470,                   /* $@80  */
  YYSYMBOL_471_81 = 471,                   /* $@81  */
  YYSYMBOL_472_82 = 472,                   /* $@82  */
  YYSYMBOL_variant_alias_declaration = 473, /* variant_alias_declaration  */
  YYSYMBOL_474_83 = 474,                   /* $@83  */
  YYSYMBOL_475_84 = 475,                   /* $@84  */
  YYSYMBOL_476_85 = 476,                   /* $@85  */
  YYSYMBOL_477_86 = 477,                   /* $@86  */
  YYSYMBOL_bitfield_alias_declaration = 478, /* bitfield_alias_declaration  */
  YYSYMBOL_479_87 = 479,                   /* $@87  */
  YYSYMBOL_480_88 = 480,                   /* $@88  */
  YYSYMBOL_481_89 = 481,                   /* $@89  */
  YYSYMBOL_482_90 = 482,                   /* $@90  */
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
  YYSYMBOL_499_91 = 499,                   /* $@91  */
  YYSYMBOL_500_92 = 500,                   /* $@92  */
  YYSYMBOL_501_93 = 501,                   /* $@93  */
  YYSYMBOL_502_94 = 502,                   /* $@94  */
  YYSYMBOL_503_95 = 503,                   /* $@95  */
  YYSYMBOL_504_96 = 504,                   /* $@96  */
  YYSYMBOL_505_97 = 505,                   /* $@97  */
  YYSYMBOL_506_98 = 506,                   /* $@98  */
  YYSYMBOL_make_tuple = 507,               /* make_tuple  */
  YYSYMBOL_make_map_tuple = 508,           /* make_map_tuple  */
  YYSYMBOL_make_tuple_call = 509,          /* make_tuple_call  */
  YYSYMBOL_510_99 = 510,                   /* $@99  */
  YYSYMBOL_511_100 = 511,                  /* $@100  */
  YYSYMBOL_make_dim = 512,                 /* make_dim  */
  YYSYMBOL_make_dim_decl = 513,            /* make_dim_decl  */
  YYSYMBOL_514_101 = 514,                  /* $@101  */
  YYSYMBOL_515_102 = 515,                  /* $@102  */
  YYSYMBOL_516_103 = 516,                  /* $@103  */
  YYSYMBOL_517_104 = 517,                  /* $@104  */
  YYSYMBOL_518_105 = 518,                  /* $@105  */
  YYSYMBOL_519_106 = 519,                  /* $@106  */
  YYSYMBOL_520_107 = 520,                  /* $@107  */
  YYSYMBOL_521_108 = 521,                  /* $@108  */
  YYSYMBOL_522_109 = 522,                  /* $@109  */
  YYSYMBOL_523_110 = 523,                  /* $@110  */
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
#define YYLAST   15045

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  222
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  308
/* YYNRULES -- Number of rules.  */
#define YYNRULES  964
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1789

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
       0,   569,   569,   570,   575,   576,   577,   578,   579,   580,
     581,   582,   583,   584,   585,   586,   587,   591,   597,   598,
     599,   603,   604,   608,   609,   613,   632,   633,   634,   635,
     639,   640,   644,   645,   649,   650,   650,   654,   659,   668,
     683,   699,   704,   712,   712,   751,   777,   781,   782,   783,
     787,   790,   794,   798,   802,   806,   812,   821,   824,   830,
     831,   835,   839,   840,   844,   847,   853,   859,   862,   868,
     869,   873,   874,   875,   884,   885,   889,   890,   894,   895,
     895,   901,   902,   903,   904,   905,   909,   915,   915,   921,
     921,   927,   935,   945,   954,   954,   958,   958,   964,   965,
     966,   967,   968,   969,   970,   974,   979,   987,   988,   989,
     993,   994,   995,   996,   997,   998,   999,  1000,  1001,  1007,
    1010,  1016,  1019,  1022,  1028,  1029,  1030,  1031,  1035,  1052,
    1074,  1077,  1087,  1102,  1117,  1132,  1135,  1142,  1146,  1153,
    1154,  1158,  1159,  1160,  1164,  1167,  1171,  1178,  1182,  1183,
    1184,  1185,  1186,  1187,  1188,  1189,  1190,  1191,  1192,  1193,
    1194,  1195,  1196,  1197,  1198,  1199,  1200,  1201,  1202,  1203,
    1204,  1205,  1206,  1207,  1208,  1209,  1210,  1211,  1212,  1213,
    1214,  1215,  1216,  1217,  1218,  1219,  1220,  1221,  1222,  1223,
    1224,  1225,  1226,  1227,  1228,  1229,  1230,  1231,  1232,  1233,
    1234,  1235,  1236,  1237,  1238,  1239,  1240,  1241,  1242,  1243,
    1244,  1245,  1246,  1247,  1248,  1249,  1250,  1251,  1252,  1253,
    1254,  1255,  1256,  1257,  1258,  1259,  1260,  1261,  1262,  1263,
    1264,  1265,  1266,  1267,  1268,  1269,  1270,  1271,  1272,  1273,
    1274,  1275,  1276,  1277,  1278,  1279,  1280,  1281,  1282,  1283,
    1284,  1285,  1286,  1287,  1288,  1289,  1293,  1294,  1298,  1317,
    1318,  1319,  1323,  1329,  1329,  1347,  1348,  1351,  1352,  1355,
    1359,  1370,  1379,  1388,  1394,  1395,  1396,  1397,  1398,  1399,
    1400,  1401,  1402,  1403,  1404,  1405,  1406,  1407,  1408,  1409,
    1410,  1411,  1412,  1413,  1414,  1418,  1423,  1429,  1435,  1446,
    1447,  1451,  1452,  1456,  1457,  1461,  1465,  1472,  1472,  1472,
    1478,  1478,  1478,  1487,  1521,  1524,  1527,  1530,  1536,  1537,
    1548,  1552,  1555,  1563,  1563,  1563,  1566,  1572,  1575,  1579,
    1583,  1590,  1597,  1603,  1607,  1611,  1614,  1617,  1625,  1628,
    1631,  1639,  1642,  1650,  1653,  1656,  1664,  1670,  1671,  1672,
    1676,  1677,  1681,  1682,  1686,  1691,  1699,  1705,  1711,  1717,
    1723,  1732,  1741,  1750,  1762,  1765,  1771,  1771,  1771,  1774,
    1774,  1774,  1779,  1779,  1779,  1787,  1787,  1787,  1793,  1803,
    1814,  1827,  1837,  1848,  1863,  1866,  1872,  1873,  1880,  1891,
    1892,  1893,  1897,  1898,  1899,  1900,  1901,  1905,  1910,  1918,
    1919,  1920,  1924,  1929,  1936,  1943,  1943,  1952,  1953,  1954,
    1955,  1956,  1957,  1958,  1962,  1963,  1964,  1965,  1966,  1967,
    1968,  1969,  1970,  1971,  1972,  1973,  1974,  1975,  1976,  1977,
    1978,  1979,  1980,  1984,  1985,  1986,  1987,  1992,  1993,  1994,
    1995,  1996,  1997,  1998,  1999,  2000,  2001,  2002,  2003,  2004,
    2005,  2006,  2007,  2008,  2013,  2019,  2030,  2036,  2047,  2051,
    2058,  2061,  2061,  2061,  2066,  2066,  2066,  2079,  2083,  2087,
    2093,  2101,  2109,  2115,  2123,  2123,  2123,  2130,  2134,  2143,
    2151,  2159,  2163,  2166,  2172,  2173,  2174,  2175,  2176,  2177,
    2178,  2179,  2180,  2181,  2182,  2183,  2184,  2185,  2186,  2187,
    2188,  2189,  2190,  2191,  2192,  2193,  2194,  2195,  2196,  2197,
    2198,  2199,  2200,  2201,  2202,  2203,  2204,  2205,  2206,  2207,
    2213,  2214,  2215,  2216,  2217,  2230,  2239,  2240,  2241,  2242,
    2243,  2244,  2245,  2246,  2247,  2248,  2249,  2250,  2253,  2256,
    2257,  2260,  2260,  2260,  2263,  2268,  2272,  2276,  2276,  2276,
    2281,  2284,  2288,  2288,  2288,  2293,  2296,  2297,  2298,  2299,
    2300,  2301,  2302,  2303,  2304,  2306,  2310,  2311,  2316,  2320,
    2321,  2322,  2323,  2324,  2325,  2326,  2330,  2334,  2338,  2342,
    2346,  2350,  2354,  2358,  2362,  2369,  2370,  2371,  2375,  2376,
    2377,  2381,  2382,  2386,  2387,  2388,  2392,  2393,  2397,  2408,
    2411,  2414,  2414,  2418,  2418,  2437,  2436,  2452,  2451,  2465,
    2474,  2486,  2495,  2505,  2506,  2507,  2508,  2509,  2513,  2516,
    2525,  2526,  2530,  2533,  2536,  2551,  2560,  2561,  2565,  2568,
    2571,  2584,  2585,  2589,  2595,  2601,  2610,  2613,  2620,  2623,
    2629,  2630,  2631,  2635,  2636,  2640,  2647,  2652,  2661,  2667,
    2678,  2681,  2686,  2691,  2699,  2710,  2713,  2716,  2716,  2736,
    2737,  2741,  2742,  2743,  2747,  2750,  2750,  2769,  2772,  2775,
    2790,  2809,  2810,  2811,  2816,  2816,  2842,  2843,  2847,  2848,
    2848,  2852,  2853,  2854,  2858,  2868,  2873,  2868,  2885,  2890,
    2885,  2905,  2906,  2910,  2911,  2915,  2921,  2922,  2923,  2924,
    2928,  2929,  2930,  2934,  2937,  2943,  2948,  2943,  2968,  2975,
    2980,  2989,  2995,  3006,  3007,  3008,  3009,  3010,  3011,  3012,
    3013,  3014,  3015,  3016,  3017,  3018,  3019,  3020,  3021,  3022,
    3023,  3024,  3025,  3026,  3027,  3028,  3029,  3030,  3031,  3032,
    3036,  3037,  3038,  3039,  3040,  3041,  3042,  3043,  3047,  3058,
    3062,  3069,  3081,  3088,  3094,  3103,  3108,  3111,  3121,  3134,
    3135,  3136,  3137,  3138,  3142,  3146,  3146,  3146,  3160,  3161,
    3165,  3169,  3176,  3180,  3187,  3188,  3189,  3190,  3191,  3205,
    3211,  3211,  3211,  3215,  3220,  3227,  3227,  3234,  3238,  3242,
    3247,  3252,  3257,  3262,  3266,  3270,  3275,  3279,  3283,  3288,
    3288,  3288,  3294,  3301,  3301,  3301,  3306,  3306,  3306,  3312,
    3312,  3312,  3317,  3322,  3322,  3322,  3327,  3327,  3327,  3336,
    3341,  3341,  3341,  3346,  3346,  3346,  3355,  3360,  3360,  3360,
    3365,  3365,  3365,  3374,  3374,  3374,  3380,  3380,  3380,  3389,
    3392,  3403,  3419,  3419,  3424,  3429,  3419,  3454,  3454,  3459,
    3465,  3454,  3490,  3490,  3495,  3500,  3490,  3540,  3541,  3542,
    3543,  3544,  3548,  3555,  3562,  3568,  3574,  3581,  3588,  3594,
    3603,  3606,  3612,  3620,  3625,  3632,  3637,  3644,  3649,  3655,
    3656,  3660,  3661,  3666,  3667,  3671,  3672,  3676,  3677,  3681,
    3682,  3683,  3687,  3688,  3689,  3693,  3694,  3698,  3704,  3711,
    3719,  3726,  3734,  3743,  3743,  3743,  3751,  3751,  3751,  3758,
    3758,  3758,  3769,  3769,  3769,  3780,  3783,  3789,  3803,  3809,
    3815,  3821,  3821,  3821,  3835,  3840,  3847,  3866,  3871,  3878,
    3878,  3878,  3888,  3888,  3888,  3902,  3902,  3902,  3916,  3925,
    3925,  3925,  3945,  3952,  3952,  3952,  3962,  3967,  3974,  3977,
    3983,  4002,  4011,  4019,  4039,  4064,  4065,  4069,  4070,  4075,
    4078,  4081,  4084,  4087,  4090
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

#define YYPACT_NINF (-1568)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-831)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1568,   611, -1568, -1568,    68,   -19,   230,   291, -1568,   -97,
     233,   233,   233, -1568, -1568,   257,   210, -1568, -1568,   429,
   -1568, -1568, -1568, -1568,   762, -1568,    96, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568,    62, -1568,   136,
     -29,   147, -1568,   -12, -1568, -1568, -1568,   599,   148, -1568,
      36, -1568, -1568, -1568,   233,   233, -1568, -1568,    96, -1568,
   -1568, -1568, -1568, -1568,   305,   363, -1568, -1568, -1568, -1568,
     210,   210,   210,   294, -1568,   899,  -122, -1568, -1568,   444,
     472,   496,   339,   701, -1568,   738,   206,    68,   447,   -19,
     230,   230,   436,   230,   512, -1568,   877,   877, -1568,   533,
     429,    16,   429,   776,   535,   545,   549, -1568,   557,   567,
   -1568, -1568,   -26,    68,   210,   210,   210,   210, -1568, -1568,
   -1568, -1568,   856, -1568, -1568,   562, -1568, -1568, -1568, -1568,
   -1568,   291, -1568, -1568, -1568, -1568, -1568,   884,   108,   460,
   -1568, -1568, -1568, -1568,   436,   436,   436,   696, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568,   429, -1568, -1568, -1568,   572,
   -1568, -1568, -1568, -1568, -1568,   589, -1568,   139, -1568,   266,
     649,   899, -1568, -1568, -1568, -1568, -1568,   252,   745, -1568,
     -83, -1568, -1568, -1568,   890, -1568, -1568, -1568, -1568, -1568,
     568, -1568, -1568,   168,   671, -1568,   675, -1568,   806, -1568,
     697,   291,   291, -1568, -1568, 14872,   805, -1568, -1568,   728,
   -1568,   698,    68,    68,   -35,   241, -1568, -1568, -1568,   735,
     108, -1568, -1568, 10566, -1568,   813,   291, -1568, -1568, 13703,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568,   885,   894, -1568,   721,   291,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,   291, -1568,
     771,   291, -1568, -1568,   -83,    -2, -1568,    68, -1568,   756,
     932,   772, -1568, -1568, -1568,   818,   828,   841,   760,   848,
     849, -1568, -1568, -1568,   786, -1568, -1568, -1568, -1568, -1568,
     -90, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568,   855, -1568, -1568, -1568,   861,   866, -1568, -1568,
   -1568, -1568,   867,   876,   794,   257, -1568, -1568, -1568, -1568,
   -1568,   442,   843, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
     902,   907, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568,   912,   870, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568,  1065, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,   917,
     875, -1568, -1568,   140,   -45, -1568, -1568, -1568,   684,   257,
   -1568, -1568, -1568,   241,   878, -1568,  9809,   920,   922, 10566,
   -1568,    41, -1568, -1568, -1568,  9809, -1568, -1568,   923,   900,
     -58,   -51,   -24, -1568, -1568,  9809,   470, -1568, -1568, -1568,
      30, -1568, -1568, -1568,     5,  6097, -1568,   882, 10314, -1568,
   10417,   655, -1568, -1568, -1568, -1568,   927,  1177,   873,   888,
   -1568,   153,   429,   625,   889, 10566, 10566, -1568,  2584, -1568,
     350, -1568,   453, -1568,    51, -1568, -1568,   906,   908, -1568,
   -1568,   126,   -33,   910,    42, -1568,   494,   892,   913,   919,
     904,   921,   905,   558,   925, -1568,   638,   926,   928,  9809,
    9809,   911,   914,   916,   918,   924,   930, -1568,  1911,  2184,
    6305, -1568, -1568, -1568, -1568, -1568, -1568, -1568,   929,   936,
   -1568,  6513,  9809,  9809,  9809,  9809,  9809,  5273,  6719, -1568,
     933, -1568, -1568, -1568,   -31, -1568, -1568, -1568, -1568,   942,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, 10899, -1568,   943,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568,  1082,  1181, -1568,
   -1568, -1568,  4029, 10566, 10566, 10566, 10937, 10566, 10566,   945,
     962, 10566,   721, 10566,   721, 10566,   721, 10669,   996, 11048,
   -1568,  9809, -1568, -1568, -1568, -1568, -1568,   959, -1568, -1568,
   13259,  9809, -1568,   442,   737,   119, -1568, -1568,   627, -1568,
     843,   453,   982,   627, -1568,   453, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568,  9809, -1568, -1568,   475,   122,   122,   122, -1568,
     843,   843, -1568,  9809, -1568, -1568, -1568,  3411, -1568,   291,
    6925, -1568,  9809,  1004, -1568,   429,  1013,  7131,   228,   983,
    3617,   205,   205,   205,  7337,   429,   429, -1568,  9809,  1170,
   -1568, -1568, -1568, -1568, -1568, -1568,  1145, -1568, -1568, -1568,
     246, -1568,   429,   429,   429,   429, -1568,   429, -1568, -1568,
    1124, -1568,   344, -1568, 10128,   684,  9809, -1568, -1568, -1568,
     210, -1568,  1183, -1568,   -83, -1568, -1568, -1568,   975, -1568,
   -1568,   257,   661, -1568,   995,   997,   999, -1568,  9809, 10566,
    9809,  9809, -1568, -1568,  9809, -1568,  9809, -1568,  9809, -1568,
   -1568,  9809, -1568, 10566,   610,   610,  9809,  9809,  9809,  9809,
    9809,  9809,   475,  2790,   475,  2997,   475, 13931, -1568,   887,
   -1568, -1568,   851,   475,  1010, -1568,  1009,   610,   610,    78,
     610,   610,   475,  1188,   986,  1012, 14228,   988,   -16,  1012,
    1015,   989,   375, -1568,  4235,    44, 10209, 14748,  9809,  9809,
   -1568, -1568,  9809,  9809,  9809,  9809,  1038,  9809,   759,  9809,
    9809,  9809,  9809,  9809,  9809,  9809,  9809,  9809,  7543,  9809,
    9809,  9809,  9809,  9809,  9809,  9809,  9809,  9809,  9809, 14810,
    9809, -1568,  7749,  1040, -1568,  4029, -1568,  1093,  1912,   269,
     309,  1027,   439, -1568,   351,   373, -1568, -1568,  1054,   624,
     -45,   665,   -45,   682,   -45, -1568,   345, -1568,   397, -1568,
   10566,  1039, -1568, -1568, 13297,   384, -1568,   453, 10566, -1568,
   -1568, 10566, -1568, -1568, 11083,  1014,  1186, -1568, -1568,   158,
   -1568, -1568, -1568, 13745,   475,  4029, -1568,  1041, 10753,  1219,
    9809, 14228,  1063, 13745,  1045, -1568,  1049,  1078, 14228, -1568,
   10566,  4029, -1568, 10753,  1028, -1568,   942, -1568, -1568, -1568,
   13745, -1568, -1568, 13745, -1568,   291, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568,   -27,   205, -1568,  4441,  4441,  4441,
    4441,  4441,  4441,  4441,  4441,  4441,  4441,  4441,  9809,  4441,
    4441,  4441,  4441,  4441,  4441, -1568,   453, 13838,  1074,    27,
     886,  1182,   429, 10566, 10566, 10566,  5479,  7955,  1080,  9809,
   10566, -1568, -1568, -1568, 10566,  1012,  1482,  1042, 11194, 10566,
   10566, 11232, 10566, 11343, 10566,  1012, 10566, 10669,  1012,   996,
     400, 11378, 11489, 11527, 11638, 11673, 11784,    40,   205,  3204,
    4649,  5685, 13968,  1066,     2,   261,  1067,   431,    54,  5891,
       2,   769,    58,  9809,  1077,  9809, -1568, -1568, 10566, 10566,
   -1568,  9809,   816,    60, -1568,  9809, -1568,    64,   475, -1568,
    9809, -1568,  9809, -1568,  9809, -1568,  9809,  1044,   775, -1568,
   -1568,  1046,  1048,   222, -1568, -1568,   160,  4857, -1568,   240,
    1050,  1055,    97,   721,  1058,  1059, -1568, -1568,  1068,  1060,
   -1568, -1568,   984,   984,   757,   757,  1641,  1641,  1061,    43,
    1076, -1568, 13408,   -17,   -17,   943,   984,   984, 14577, 14451,
   14484, 14321, 14779, 14061,  1020, 14603,  1320,   757,   757,  1036,
    1036,    43,    43,    43,   777,  9809,  1079,  1081,   778,  9809,
    1266,  1085, 13443, -1568,   410, -1568, -1568,  1912,  9809,  9809,
    9809,  9809,  9809,  9809,  9809,  9809,  9809,  9809,  9809,  9809,
    9809,  9809,  9809,  9809,  9809, -1568, -1568, -1568, -1568, 10566,
   -1568, -1568, -1568,   399, -1568,  1083, -1568,  1084, -1568,  1095,
   -1568, 10669, -1568,   996,   471,   843, -1568,  1053, -1568,  9809,
   -1568, -1568,   843,   843, -1568,  9809,  1102,  1111, 10566, -1568,
    9809, -1568,    65, -1568,  1041,  9809,   291, 14228,  1103, -1568,
   -1568, -1568, -1568,  1339, -1568, 10753, -1568,    44, -1568,   782,
    9809, -1568,   942,  1130,  1130, -1568, -1568, -1568,   205,   205,
     205, -1568, -1568, 10788, -1568, 10788, -1568, 10788, -1568, 10788,
   -1568, 10788, -1568, 10788, -1568, 10788, -1568, 10788, -1568, 10788,
   -1568, 10788, -1568, 10788, 14228, -1568, 10788, -1568, 10788, -1568,
   10788, -1568, 10788, -1568, 10788, -1568, 10788, -1568, -1568,  1114,
     429, -1568, -1568,   622, -1568,    50, -1568,  1665,  1750,   695,
     779,   411,  1089,  1092,  1138, 11822,   -99, 11933,   716, 10566,
   10669,   996,  2171,  1094,  1096, 10566, -1568, -1568,  2289,  2298,
   -1568,  4438, -1568,  4646,  1097,  4854,   552,  1098,   598,    44,
   -1568, -1568, -1568, -1568, -1568,  1104,  9809, -1568,  5065, 13259,
     110,  9809,   775,   779,   261, -1568, -1568,  1106, -1568,  9809,
    9809, -1568,  1115, -1568,  9809,   779,   780,  1117, -1568, -1568,
    9809, 14228, -1568, -1568,   600,   644, 14098,  9809, -1568,  9809,
      87, 14228, 11968, 14228, 14228, -1568,  1110,   175,  9809,  9809,
   10566,   721,   178, -1568,  1101,   251, 10015, -1568, -1568,    97,
    1147,  1161,  1119,  1164,  1165, -1568,   417,   -45, -1568,  9809,
   -1568,  9809,  8161,  9809, -1568,  1146,  1126, -1568, -1568,  9809,
    1128, -1568, 13554,  9809,  8367,  1131, -1568, 13592, -1568,  8573,
   -1568, -1568, -1568, 14228, 14228, 14228, 14228, 14228, 14228, 14228,
   14228, 14228, 14228, 14228, 14228, 14228, 14228, 14228, 14228, 14228,
   -1568, -1568, -1568,   843, -1568, -1568,  1174, -1568,  1176, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,  1139,
   10566, -1568, 13838, 12079, -1568,  1314,   -66, 14228,  9809, -1568,
   10566,  9809,    44,   721,   291, -1568, -1568,  9809, -1568,  2331,
    2584,    44, -1568,   529,   435, -1568, -1568, -1568, 10566, -1568,
    1325,    50, -1568, -1568,   886, -1568, -1568, -1568,  1141, -1568,
   -1568, -1568,   646, -1568,  1189,  1150, -1568, -1568,  5062,   657,
     667, -1568, -1568,  9809,  6094, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568,  1152,  8779,   783,     2,   261, 14228,
    1066, -1568, -1568, 14228,  1067, -1568,   807,     2,  1148, -1568,
   -1568, -1568, -1568,   810, -1568, -1568, -1568,  1190,   822,   834,
    9809,   184,  9809,  9809,  9809, 12117, 12228,  6302,   -45, -1568,
    1157,  4857,   461, -1568, -1568,  1203, -1568, -1568,    97,  1167,
     377, 10566, 12263, 10566, 12374, -1568,   492, 12412, -1568,  9809,
   14358,  9809, -1568, 12523,  4857, -1568,   497,  9809, -1568, -1568,
   -1568,   503, -1568, -1568, -1568, -1568, -1568, -1568, -1568,   843,
   -1568, -1568,  1207,  9809,   245,   843, 14228,   795,   -45, -1568,
   13745, -1568,   429, -1568,   721,  1211,  1169,   107,   149, -1568,
   -1568,  1325,   475,  1172,  1173, -1568, -1568,  9809,  1213,  1193,
    9809, -1568, -1568, -1568, -1568,  1178,  1175,  1180,  9809,  9809,
    9809,  1184,  1293,  1187,  1191,  8985, -1568,   506,  9809,   261,
   -1568,  9809,   780, -1568,  9809,  9809,  1139, -1568, -1568,  9809,
    9809,   842,  9809,  9809, 12558, 14228, 14228, -1568, -1568, -1568,
    1199, -1568,   543, -1568,  1185, -1568, -1568,  9191, -1568, -1568,
   10012, -1568,   746, -1568, -1568, -1568, 10566, 12669, 12707, -1568,
     566, -1568, 12818, -1568, -1568, 14228, -1568, -1568,   377,   782,
    3823, -1568,   -45, -1568,   840, 10566,    41, -1568, 14872, -1568,
   -1568, -1568, -1568,  1293,  1293, 12853,  1206,  1192, 12964,  1194,
    1195,  1197,  9809, -1568,  9809,   757,   757,   757,  9809, -1568,
   -1568,  1293,  1293, -1568, 13002, -1568, 14191, -1568, 14191, -1568,
    1202,   757, -1568,  1229,  1202, 14191,  9809, 14228, 14228,   227,
     523, -1568,  1198, -1568,  9809, 14321, -1568, -1568,   781, -1568,
   -1568,  1200, -1568, -1568, -1568,  9397,  9603, -1568, -1568, -1568,
   -1568, -1568, 14228,   291, 10566,    41,   192,  4029,   429, 14872,
      -6,    -6, -1568,  9809,  9809, -1568,  1293,  1293,   779,  1201,
    1204,  1012,    -6,   779, -1568,  1354,  1205,  1226,  1233, -1568,
    1239,  1208, 14191,  9809,  9809, -1568,   523, -1568, 14358, -1568,
   -1568, -1568, -1568,  9809,  9809, 14228, -1568,   192,  4029,  4029,
   -1568,  1912, -1568,   291,   779,  1066,  1234, -1568,  1209,  1210,
   13113, 13148,    -6,    -6,  1066,  1212, -1568, -1568,  1214,  1215,
    1216,  9809,  1225,  1231,  1242, -1568, -1568,  1236, 14228, 14228,
   -1568, -1568, 14228,  4029, -1568,  1912, -1568,  1912, -1568, -1568,
     507,  1218, -1568, -1568, -1568, -1568, -1568,  1224,  1235, -1568,
   -1568, -1568, -1568, 14228, -1568, -1568, -1568, -1568, -1568,  1912,
   -1568, -1568, -1568,   779, -1568, -1568, -1568,   508, -1568
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   139,     1,   350,     0,     0,     0,   679,   351,     0,
     671,   671,   671,    74,    75,     0,     0,    15,     3,     0,
      10,     9,     8,    16,     0,     7,   659,     6,    11,     5,
       4,    13,    12,    14,   108,   109,   107,   117,   119,    45,
      64,    61,    62,     0,    47,    48,    49,     0,     0,    50,
      59,    46,   266,   265,   671,   671,    22,    21,   659,   673,
     672,   852,   842,   847,     0,   318,    43,   125,   126,   127,
       0,     0,     0,   128,   130,   137,     0,   124,    17,   697,
     696,   256,   681,   700,   660,   661,     0,     0,     0,     0,
       0,     0,    51,     0,     0,    60,     0,     0,    57,     0,
       0,   671,     0,    18,     0,     0,     0,   320,     0,     0,
     136,   131,     0,     0,     0,     0,     0,     0,   140,   699,
     698,   257,   259,   683,   682,     0,   702,   701,   705,   663,
     662,   665,   115,   116,   113,   114,   111,     0,     0,     0,
     110,   120,    65,    63,    53,    54,    52,    59,    56,    55,
     674,   676,   268,   267,   678,     0,   680,    19,    20,    23,
     853,   843,   848,   319,    41,    44,   135,     0,   132,   133,
     134,   138,   261,   260,   263,   258,   684,     0,   693,   655,
     585,    26,    27,    31,     0,   103,   104,   101,   102,    99,
       0,    98,   105,     0,     0,    58,     0,   677,     0,    25,
     759,     0,     0,    42,   129,     0,     0,   685,   694,     0,
     706,   657,     0,     0,   587,     0,    28,    29,    30,     0,
       0,   118,   112,     0,    24,     0,     0,   844,   849,     0,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,     0,     0,   147,   141,     0,
     740,   743,   746,   747,   741,   744,   742,   745,     0,   667,
     691,   703,   656,   664,   585,     0,   121,     0,   123,     0,
     645,   643,   666,   100,   106,     0,     0,     0,     0,     0,
       0,   713,   733,   714,   749,   715,   719,   720,   721,   722,
     739,   726,   727,   728,   729,   730,   731,   732,   734,   735,
     736,   737,   812,   718,   725,   738,   819,   826,   716,   723,
     717,   724,     0,     0,     0,     0,   748,   774,   777,   775,
     776,   839,   675,   762,   763,   760,   761,   854,   622,   628,
     225,   226,   223,   150,   151,   153,   152,   154,   155,   156,
     157,   183,   184,   181,   182,   174,   185,   186,   175,   172,
     173,   224,   207,     0,   222,   187,   188,   189,   190,   161,
     162,   163,   158,   159,   160,   171,     0,   177,   178,   176,
     169,   170,   165,   164,   166,   167,   168,   149,   148,   206,
       0,   179,   180,   585,   144,   295,   264,   688,   686,     0,
     695,   599,   707,     0,     0,   122,     0,     0,     0,     0,
     644,     0,   780,   803,   806,     0,   809,   799,     0,     0,
     813,   820,   827,   833,   836,     0,   301,   789,   794,   788,
       0,   802,   798,   791,     0,     0,   793,   778,     0,   755,
     845,   850,   227,   228,   221,   205,   229,   208,   191,     0,
     142,   349,   613,   614,     0,     0,     0,   262,     0,   667,
       0,   668,     0,   692,   603,   658,   586,     0,     0,   490,
     491,     0,     0,     0,     0,   484,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   739,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   574,     0,     0,
       0,   407,   409,   408,   410,   411,   412,   413,     0,     0,
      37,   303,     0,     0,     0,     0,     0,   299,     0,   389,
     390,   488,   487,   568,   485,   559,   558,   557,   556,   139,
     562,   486,   561,   560,   532,   492,   533,     0,   493,     0,
     489,   857,   861,   858,   859,   860,   647,   648,     0,   641,
     642,   640,     0,     0,     0,     0,     0,     0,     0,     0,
     765,     0,   141,     0,   141,     0,   141,     0,     0,     0,
     785,   299,   784,   796,   797,   790,   792,     0,   795,   779,
       0,     0,   841,   840,   855,   318,   768,   769,     0,   623,
     618,     0,     0,     0,   629,     0,   230,   210,   211,   213,
     212,   214,   215,   216,   217,   209,   218,   219,   220,   194,
     195,   197,   196,   198,   199,   200,   201,   192,   193,   202,
     203,   204,     0,   347,   348,     0,   585,   585,   585,   143,
     146,   145,   297,     0,    76,    77,    89,   335,   333,     0,
       0,    96,     0,     0,   334,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   274,     0,     0,
     290,   285,   282,   281,   283,   284,   269,   317,   296,   276,
     568,   275,     0,    84,    85,    82,   288,    83,   289,   291,
     353,   280,     0,   277,   414,   689,     0,   669,   687,   601,
       0,   600,     0,   704,   585,   903,   906,   323,   327,   326,
     332,     0,     0,   375,     0,     0,     0,   939,     0,     0,
     303,     0,   366,   369,     0,   372,     0,   943,     0,   912,
     921,     0,   909,     0,   520,   521,     0,     0,     0,     0,
       0,     0,     0,   881,     0,     0,     0,   919,   946,     0,
     307,   310,     0,     0,     0,   948,   957,   497,   496,   534,
     495,   494,     0,     0,     0,   957,   384,     0,   318,   957,
     957,     0,   391,   566,     0,   399,     0,     0,     0,     0,
     522,   523,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   474,
       0,   646,     0,     0,   650,     0,   654,     0,   414,     0,
       0,     0,   770,   783,     0,     0,   750,   764,     0,     0,
     144,     0,   144,     0,   144,   620,     0,   626,     0,   751,
       0,   957,   787,   772,     0,     0,   756,     0,     0,   624,
     846,     0,   630,   851,     0,     0,   708,   610,   611,   633,
     615,   617,   616,     0,     0,     0,   339,   336,   384,     0,
       0,   321,     0,     0,     0,   294,     0,     0,    68,    91,
       0,     0,   344,   341,   390,   402,   139,   316,   314,   315,
       0,   292,   293,     0,    87,     0,   405,   272,   279,   286,
     287,   338,   343,   352,     0,     0,   278,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   271,     0,     0,     0,     0,
     593,   596,     0,     0,     0,     0,   895,     0,     0,     0,
       0,   929,   932,   935,     0,   957,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   957,     0,     0,   957,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   915,   873,   881,     0,   924,     0,     0,     0,
     881,     0,     0,     0,     0,     0,   884,   951,     0,     0,
      40,     0,    38,     0,   950,   958,   304,     0,     0,   926,
     958,   300,     0,   632,     0,   631,     0,     0,   958,   872,
     525,     0,     0,   461,   458,   460,     0,   299,   477,     0,
       0,     0,     0,   141,     0,     0,   545,   544,     0,     0,
     546,   550,   498,   499,   511,   512,   509,   510,     0,   539,
       0,   530,     0,   563,   564,   565,   500,   501,   516,   517,
     518,   519,     0,     0,   514,   515,   513,   507,   508,   503,
     502,   504,   505,   506,     0,     0,     0,   467,     0,     0,
       0,     0,     0,   482,     0,   649,   652,   414,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   653,   781,   804,   807,     0,
     810,   800,   752,     0,   814,     0,   821,     0,   828,     0,
     834,     0,   837,     0,     0,   305,   958,     0,   773,     0,
     757,   856,   619,   625,   612,     0,     0,     0,     0,   634,
       0,    92,     0,   340,   337,     0,     0,   322,     0,    93,
      94,    66,    67,     0,   345,   342,   391,   399,   298,    71,
       0,   295,   139,     0,     0,   365,   364,   313,     0,     0,
       0,   436,   445,   424,   446,   425,   448,   427,   447,   426,
     449,   428,   439,   418,   440,   419,   441,   420,   450,   429,
     451,   430,   438,   416,   417,   452,   431,   453,   432,   442,
     421,   443,   422,   444,   423,   437,   415,   690,   670,     0,
     140,   594,   595,   596,   597,   588,   604,     0,     0,     0,
     896,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   952,   535,     0,     0,
     536,     0,   567,     0,     0,     0,     0,     0,     0,   399,
     569,   570,   571,   572,   573,     0,     0,   882,     0,   384,
     881,     0,     0,     0,     0,   890,   891,     0,   898,     0,
       0,   888,     0,   927,     0,     0,     0,     0,   886,   928,
       0,   918,   883,   947,     0,     0,    34,     0,   949,     0,
       0,   385,     0,   863,   862,   524,     0,     0,     0,     0,
       0,   141,     0,   478,     0,     0,     0,   481,   479,     0,
       0,     0,     0,     0,     0,   397,     0,   144,   541,     0,
     547,     0,     0,     0,   528,     0,     0,   551,   555,     0,
       0,   531,     0,     0,     0,     0,   468,     0,   475,     0,
     526,   483,   651,   424,   425,   427,   426,   428,   418,   419,
     420,   429,   430,   416,   431,   432,   421,   422,   423,   415,
     782,   805,   808,   771,   811,   801,     0,   766,     0,   815,
     817,   822,   824,   829,   831,   835,   621,   838,   627,   301,
       0,   302,     0,     0,   710,   711,   636,   635,     0,   346,
       0,     0,   399,   141,     0,    69,    70,     0,    86,    78,
       0,   399,   354,     0,     0,   435,   433,   434,     0,   609,
     591,   588,   589,   590,   593,   904,   907,   324,     0,   329,
     330,   328,     0,   378,     0,     0,   381,   376,     0,     0,
       0,   940,   938,   303,     0,   367,   370,   373,   944,   942,
     913,   922,   920,   910,     0,     0,     0,   881,     0,   916,
     874,   897,   889,   917,   925,   887,     0,   881,     0,   893,
     894,   901,   885,     0,   308,   311,    35,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   144,   480,
       0,   299,     0,   394,   395,     0,   393,   392,     0,     0,
       0,     0,     0,     0,     0,   456,     0,     0,   552,     0,
     540,     0,   529,     0,   299,   469,     0,     0,   527,   476,
     472,     0,   754,   767,   753,   818,   825,   832,   786,   306,
     758,   709,     0,     0,     0,    97,    95,     0,   144,    72,
       0,    79,     0,   270,   141,     0,     0,   643,     0,   592,
     605,   591,     0,     0,     0,   325,   331,     0,     0,     0,
       0,   377,   930,   933,   936,     0,     0,     0,     0,     0,
       0,     0,   895,     0,     0,     0,   575,     0,     0,     0,
     899,     0,     0,   892,     0,     0,   301,    32,    39,     0,
       0,     0,     0,     0,     0,   865,   864,   459,   584,   462,
       0,   454,     0,   401,     0,   398,   400,     0,   386,   404,
       0,   583,     0,   581,   457,   578,     0,     0,     0,   577,
       0,   470,     0,   473,   712,   637,    90,   273,     0,    71,
       0,    88,   144,   355,   643,     0,     0,   602,     0,   607,
     639,   638,   598,   895,   895,     0,     0,     0,     0,     0,
       0,     0,   299,   953,   303,   368,   371,   374,     0,   896,
     914,   895,   895,   537,     0,   576,   955,   900,   955,   902,
     955,   309,   312,    36,   955,   955,     0,   867,   866,     0,
       0,   465,     0,   396,     0,   387,   542,   548,     0,   582,
     580,     0,   579,   403,    73,   335,     0,    80,    84,    85,
      82,    83,    81,     0,     0,     0,     0,     0,     0,     0,
     880,   880,   379,     0,     0,   382,   895,   895,   870,     0,
       0,   957,   880,   870,   538,     0,     0,     0,     0,    33,
       0,     0,   955,     0,     0,   463,     0,   455,   388,   543,
     549,   553,   471,     0,     0,   341,   406,     0,     0,     0,
     363,   414,   606,     0,     0,   877,   957,   879,     0,     0,
       0,     0,   880,   880,   871,     0,   941,   954,     0,     0,
       0,     0,     0,     0,     0,   963,   959,     0,   869,   868,
     466,   554,   342,     0,   361,   414,   359,   414,   362,   608,
       0,   958,   878,   905,   908,   380,   383,     0,     0,   937,
     945,   923,   911,   956,   961,   962,   964,   960,   357,   414,
     360,   358,   875,     0,   931,   934,   356,     0,   876
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1568, -1568, -1568, -1568, -1568, -1568, -1568,   687,  1368, -1568,
   -1568, -1568, -1568, -1568, -1568,  1454, -1568, -1568, -1568,   939,
     487, -1568,  1310, -1568, -1568,  1371, -1568, -1568, -1568,  -137,
      -1, -1568, -1568, -1568,  -136, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568,  1246, -1568, -1568,   -30,   -57,
   -1568, -1568, -1568,   588,   784,  -447,  -512,  -781, -1568, -1568,
   -1568, -1568, -1541, -1568, -1568,     4,  -197,  -258,  -449, -1568,
     327, -1568,  -568, -1314,  -697,    77,  -418, -1568, -1568, -1568,
   -1568,  -545,     0, -1568, -1568, -1568, -1568, -1568,  -131,  -130,
    -125, -1568,  -123, -1568, -1568, -1568,  1477, -1568,   336, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568,    63,  -115,  1008,    17,   195, -1101,  -563, -1568,
    -660, -1568, -1568,  -452,   678, -1568, -1568, -1568, -1567, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,   839, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568,  -168,    99,   -34,    98,
     298, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,   451,
    -415,  -919, -1568,  -421,  -914, -1568,  -845,   -28,   -23, -1568,
    -546, -1451, -1568,  -382, -1568, -1568,  1437, -1568, -1568, -1568,
    1037,  1072,   319, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568,  -690,  -192, -1568,  1029, -1568, -1568, -1568,
    1227, -1568, -1568, -1568,  -411, -1568, -1568,  -401, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568,  -218, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568,  1030,  -684,  -195,  -730,  -703, -1568, -1568, -1219,  -937,
   -1568, -1568, -1568, -1218,   -50, -1022, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568,   253,  -478, -1568, -1568, -1568,
     770, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568, -1568,
   -1568, -1568, -1568, -1568, -1568, -1197,  -736, -1568
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    17,   159,    58,   199,    18,   184,   191,  1643,
    1447,  1557,   742,   521,   165,   522,   109,    20,    21,    49,
      50,    51,    98,    22,    41,    42,   655,   656,  1377,  1378,
     587,   658,  1512,  1600,   659,   660,  1140,   661,   854,   662,
     663,   664,   665,  1371,   862,   192,   193,    37,    38,    39,
     214,    73,    74,    75,    76,    24,   394,   457,   258,   122,
      25,   174,   259,   175,   205,   395,   154,   875,  1151,   668,
     458,   669,   754,   572,   744,  1104,   523,   978,  1555,   979,
    1556,   671,   524,   672,   698,   925,  1525,   525,   673,   674,
     675,   676,   677,   678,   679,   625,   680,   894,  1383,  1145,
     681,   526,   939,  1538,   940,  1539,   942,  1540,   527,   930,
    1531,   528,   755,  1579,   529,  1295,  1296,  1013,   877,   530,
     915,  1142,   531,   807,  1152,   683,   532,   533,  1005,   534,
    1280,  1650,  1281,  1706,   535,  1060,  1489,   536,   756,  1471,
    1709,  1473,  1710,  1586,  1751,   538,   451,  1394,  1520,  1193,
    1195,   922,   464,   918,   694,  1608,  1679,   452,   453,   454,
     825,   826,   440,   827,   828,   441,   996,   847,   848,  1612,
     552,   411,   281,   282,   211,   274,    85,   131,    27,   180,
     398,    99,   100,   196,   101,    28,    55,   125,   177,    29,
     269,   462,   459,   916,   400,   209,   210,    83,   128,   402,
      30,   178,   271,   849,   539,   268,   328,   329,  1093,   584,
     226,   330,   818,  1493,  1101,   811,   437,   331,   553,  1340,
     830,   558,  1345,   554,  1341,   555,  1342,   557,  1344,   561,
    1349,   562,  1495,   563,  1351,   564,  1496,   565,  1353,   566,
    1497,   567,  1355,   568,  1357,   590,    31,   105,   201,   338,
     591,    32,   106,   202,   339,   595,    33,   104,   200,   439,
     837,   540,   760,  1735,   761,   964,  1726,  1727,  1728,   965,
     977,  1259,  1253,  1248,  1441,  1203,   541,   923,  1523,   924,
    1524,   949,  1544,   946,  1542,   966,   745,   542,   947,  1543,
     967,   543,  1209,  1619,  1210,  1620,  1211,  1621,   934,  1535,
     944,  1541,   739,   746,   544,  1696,   986,   545
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      23,   396,   805,   831,  1120,   332,   682,   806,   548,   667,
     887,    54,   215,   937,   273,    66,    77,   692,    78,   991,
     593,   465,   738,   997,   999,   588,  1431,  1244,  1226,   589,
     594,   327,   970,  1256,  1010,  1228,  1373,   583,   575,  1095,
     670,  1097,   957,  1099,   968,  1498,   972,   704,  1011,   963,
     820,   963,   822,   983,   824,   958,   167,   141,    94,  -139,
    1236,   993,   987,   573,   766,   767,  1606,  1678,  -816,   117,
      77,    77,    77,    59,  1254,  -823,    56,   212,  1260,    60,
    1267,   455,   765,  1705,  1269,  1368,  1404,    34,    35,   878,
     879,   689,   856,    95,   118,  1107,  1392,   733,   735,   151,
    1405,   156,  -830,   667,   225,   872,   403,  1450,  -759,   776,
      64,   993,   778,   779,    77,    77,    77,    77,   114,   115,
     116,   994,    57,   185,   186,   279,   995,   108,  1424,   438,
      13,   467,   468,  1143,   670,   179,  -816,   213,  1723,  1750,
      65,  -816,   763,  -823,   705,   706,   280,    40,  -823,   456,
      14,   474,   757,  1675,   197,   275,  1393,   476,    84,  -816,
     623,   770,   771,   958,  1122,    88,  -823,  1282,   549,   776,
    -830,   777,   778,   779,   780,  -830,   995,   839,   550,   781,
     152,   207,   842,   276,   701,   277,   764,  1243,   667,    87,
    1144,    90,   166,  -830,   483,   484,  1202,   799,   800,  1213,
     153,   667,   576,   624,  1012,   227,   228,   778,   779,  1224,
    1550,  1724,  1227,   278,   404,   152,    13,   327,   117,   670,
     577,   132,   133,   326,  1290,   427,   578,   574,   486,   487,
     337,  1117,   670,   551,  1291,   153,    14,   630,   631,    96,
     707,    36,  1642,  1190,    67,  1117,   327,   405,   327,  1117,
      97,  1117,   428,   429,    86,  1117,  1117,   799,   800,   708,
    1066,   666,   108,   327,   327,   688,   690,   693,   757,    64,
    1292,  1507,   397,    68,   187,   401,  1284,   449,  1117,   188,
    1514,   189,   212,  1282,   137,   993,   498,   499,   500,  1293,
      59,  1409,   799,   800,  1294,   449,    60,  1410,  1270,    65,
     212,  1605,   993,  1428,   410,   993,   327,   327,   936,   511,
    1123,   993,  1453,   838,    13,   994,   430,    69,   757,   549,
     431,  1562,   950,  1285,   697,   426,  1134,    87,   190,   550,
      87,  1637,  1147,  1282,    14,   809,   810,   812,    89,   814,
     815,   517,   213,   819,   438,   821,    70,   823,  -464,  1117,
     995,    93,  1118,    64,   993,  1119,   667,   204,   450,   220,
     213,   327,   327,   327,  1703,   327,   327,   995,    52,   327,
     995,   327,   134,   327,   102,   327,   995,   135,  1283,   136,
      43,   859,   137,    65,   551,   432,   221,   670,    53,   433,
     869,    52,   434,   279,   840,  1237,   123,   461,   843,   463,
      64,  1089,   124,    44,    45,    46,   667,   435,    52,   326,
     114,    53,   116,   436,   280,    52,  -464,  1103,    71,   995,
     155,  -464,   667,   138,   519,   874,   139,    72,    53,  1137,
      65,  1286,  1454,   427,    47,    53,  1125,   670,   326,  -464,
     326,  1697,  1242,  1698,    48,   716,   206,  1700,  1701,  1245,
    1246,   626,   628,   670,    52,   326,   326,   657,  1287,   687,
     428,   429,  1729,   691,   438,   519,   874,  1460,  1086,  1133,
    1398,   326,   702,  1739,    53,   427,   895,  1247,   107,   670,
     670,   670,   670,   670,   670,   670,   670,   670,   670,   670,
    1549,   670,   670,   670,   670,   670,   670,  1263,   326,   326,
    1552,  1297,   428,   429,   438,  1747,   108,  1268,  1087,    13,
      13,   113,  1146,  1767,  1768,    13,  1470,   327,    64,  1577,
    1630,  1503,  1197,  1198,   430,  1215,   921,  1001,   431,    14,
      14,   327,  1002,  1212,    92,    14,   586,  1427,  1218,  1219,
      52,  1221,   686,  1223,  1100,  1225,   438,   804,    65,    13,
    1090,   119,  1437,   326,   326,   326,   963,   326,   326,  1430,
      53,   326,    13,   326,    13,   326,   430,   326,   438,    14,
     431,   963,  1091,  1003,  1017,  1021,  1109,   144,   145,   120,
     146,   759,    14,   836,    14,  1385,  1386,  1387,   586,  1035,
    1346,  1680,  1681,   432,    13,  1121,  1102,   433,  1347,  1229,
     434,  1125,  1125,   121,    13,  1129,  1563,  1061,  1468,  1692,
    1693,     2,  1105,   142,    14,   435,  1250,   152,     3,  1251,
    1112,   436,  1138,  1113,    14,  1139,  1515,   845,  1321,  1399,
     586,   766,   767,  1469,   438,   432,    13,   153,   327,   433,
    1111,     4,   434,     5,   865,     6,   327,  1252,   846,   327,
      97,     7,  1468,  1517,   881,   882,    14,   435,   110,   111,
     112,     8,   586,   436,  1732,  1733,    64,     9,   570,  1390,
    1359,   888,   889,   890,   891,  1001,   892,  1570,   327,  1573,
     194,   896,  1358,  1125,   461,   147,  1356,   571,  1125,  1194,
      77,    10,   709,  1360,  1125,  1381,    65,  1125,  1242,  1242,
     857,   927,   168,   169,   170,   171,   150,  1199,   160,   326,
    1584,   710,  1208,    11,    12,  1591,  1536,    13,   161,  1187,
    1515,  1593,   162,   326,  1635,  1782,  1788,  1598,   770,   771,
     163,   327,   327,   327,  1242,   176,   776,    14,   327,   778,
     779,   780,   327,   586,   164,  1516,   781,   327,   327,    43,
     327,  1421,   327,    95,   327,   327,   717,  1242,   126,  1652,
    1105,  1105,  1004,    13,   127,    13,   203,    79,    80,  1458,
      81,   935,    44,    45,    46,   718,    13,  1572,   766,   767,
     198,   945,  1661,    14,   948,    14,   327,   327,   219,   586,
      13,   586,    13,   114,  1374,   129,    14,  1423,    82,  1444,
    1590,   130,    91,    47,  1704,  1375,  1376,  1085,  1408,    13,
      14,    13,    14,    48,  1414,    15,   627,  1103,   586,   438,
      13,  1673,    13,  1094,   799,   800,    16,  1009,   592,    14,
     326,    14,    13,   157,  1110,   586,   720,   586,   326,   158,
      14,   326,    14,  1445,   222,  1527,   586,  1597,   586,    13,
    1308,   208,    14,  1360,  1360,   721,  1533,   460,   586,   928,
     438,  1508,   152,   272,  1096,  1064,  1534,   223,  1369,    14,
     326,  1343,   224,   768,   769,   770,   771,   438,   929,  1457,
     260,  1098,   153,   776,   261,   777,   778,   779,   780,  1141,
     438,   225,   333,   781,  1397,   782,   783,   327,   262,   263,
    1366,   270,    13,   264,   265,   266,   267,   334,   283,   327,
     835,   438,   335,   172,   336,  1407,  1188,  1030,  1124,   173,
     391,  1196,    14,   326,   326,   326,   327,  1690,   682,   392,
     326,   667,  1031,  1276,   326,  1310,  1315,   757,   393,   326,
     326,   438,   326,  1191,   326,  1657,   326,   326,  1277,  1192,
    1311,  1316,  1282,  1257,  1250,  1738,  1258,   794,   795,   796,
     797,   798,   670,   408,  1438,   399,   409,  1439,  1548,   410,
    1440,   799,   800,   406,  1125,   407,   438,   415,   326,   326,
    1711,   609,   610,   611,   612,   613,   614,   615,   616,  1201,
    1762,  1528,  1551,   216,   217,  1554,  1725,  1725,  1125,  1103,
     617,  1125,  1602,   418,  1734,   766,   767,  1559,  1725,  1734,
     618,   425,  1545,  1125,   519,   874,   412,   327,   327,  1560,
     619,   620,   621,   327,   759,  1125,   413,  1646,   181,   182,
     980,   981,   759,  1125,  1674,   148,   149,   410,   438,   414,
    1760,   766,   767,   114,   115,   116,   416,   417,  1725,  1725,
      44,    45,    46,   420,  1689,  1264,  1265,   766,   767,   421,
    1677,   181,   182,   183,   422,   423,  1322,   216,   217,   218,
    1580,   974,   975,   976,   424,   442,  1640,   850,   851,   852,
     443,  1644,    61,    62,    63,   444,   445,   446,   327,   326,
     447,   448,  1348,   546,   466,   547,   559,   581,   560,  1787,
     596,   326,   770,   771,   695,   622,   696,   629,   703,   711,
     776,   712,   777,   778,   779,   780,  1509,   713,   326,   715,
     781,   714,   716,   719,   722,   803,   723,   740,   726,  1718,
    1719,   727,  1720,   728,   741,   729,   768,   769,   770,   771,
     772,   730,  1499,   773,   774,   775,   776,   731,   777,   778,
     779,   780,  1505,   762,   770,   771,   781,    16,   782,   783,
     802,   817,   776,   816,   777,   778,   779,   780,   327,   592,
    1518,  1753,   781,  1754,  1756,   832,   841,   864,   327,   866,
     884,   870,   885,  1513,   794,   795,   796,   797,   798,  1389,
     893,   920,   926,   931,   984,   932,   327,   933,   799,   800,
     985,   988,   989,   990,  1676,   992,   998,  1000,  1778,   326,
     326,  1028,  1578,  1065,   427,   326,   790,   791,   792,   793,
     794,   795,   796,   797,   798,   895,  1088,  1092,   667,  1116,
    1106,  1115,  1125,  1126,   799,   800,  1128,  1130,   796,   797,
     798,   428,   429,  1131,  1132,   537,  1596,  1189,  1136,  1194,
     799,   800,  1599,  1206,   556,  1582,  1298,  1242,  1249,   670,
    1216,  1262,  1275,  1278,   569,  1279,  1300,  1318,  1288,   667,
     667,  1361,  1289,  1717,   580,  1364,  1299,  1301,  1302,   327,
     326,   327,  1350,  1352,  1365,   597,   598,   599,   600,   601,
     602,   603,   604,  1303,  1354,  1370,  1313,   684,  1314,  1426,
     670,   670,  1319,  1382,   667,   430,  1388,  1400,   549,   431,
    1401,  1402,  1412,  1413,   605,  1419,  1422,  1436,   550,  1459,
    1463,  1425,  1432,  1443,   606,   607,   608,  1452,   724,   725,
    1448,  1435,  1449,  1442,  1464,   670,  1465,  1466,  1467,   737,
    1578,   766,   767,  1479,  1478,  1481,    13,  1492,  1487,  1494,
     737,   747,   748,   749,   750,   751,   571,  1502,  1519,  1526,
     326,  1500,  1529,  1629,  1553,  1476,    14,  1530,  1658,  1545,
     326,  1558,   427,   551,   432,  1571,  1574,  1486,   433,   657,
    1594,   434,  1491,  1576,  1603,  1604,  1616,  1695,   326,  1613,
    1614,   808,  1617,  1623,   327,  1622,   435,  1624,  1651,   428,
     429,  1628,   436,  1653,  1631,  1683,  1699,  1741,  1632,  1684,
    1743,  1686,  1687,   327,  1688,  1716,  1707,  1744,  1712,  1736,
     834,  1742,  1737,  1745,  1746,  1761,  1776,  1763,  1764,   982,
    1769,  1504,  1770,  1771,  1772,  1783,   768,   769,   770,   771,
     772,  1774,  1784,   773,   774,   775,   776,  1775,   777,   778,
     779,   780,  1777,  1785,   140,    19,   781,   195,   782,   783,
     143,   844,  1664,   430,  1667,  1759,   284,   431,  1380,  1668,
    1669,   326,   853,   326,   919,  1670,   858,  1671,    26,   861,
    1384,   863,   327,  1663,  1462,  1575,   868,  1609,  1547,   873,
    1521,  1391,  1522,   880,  1610,   103,   685,   883,  1740,  1611,
     699,   700,  1639,  1434,     0,   971,     0,     0,     0,     0,
       0,  1601,     0,  1561,     0,   427,     0,  1607,   792,   793,
     794,   795,   796,   797,   798,   917,     0,   419,     0,     0,
       0,     0,   432,     0,   799,   800,   433,     0,  1372,   434,
       0,     0,   428,   429,     0,     0,     0,     0,     0,   737,
     938,     0,     0,   941,   435,   943,     0,     0,     0,     0,
     436,     0,     0,     0,     0,   951,   952,   953,   954,   955,
     956,     0,   962,     0,   962,     0,  1154,  1156,  1158,  1160,
    1162,  1164,  1166,  1168,  1170,  1172,   326,  1175,  1177,  1179,
    1181,  1183,  1185,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   326,   430,  1022,  1023,     0,
     431,  1024,  1025,  1026,  1027,     0,  1029,     0,  1032,  1033,
    1034,  1036,  1037,  1038,  1039,  1040,  1041,  1043,  1044,  1045,
    1046,  1047,  1048,  1049,  1050,  1051,  1052,  1053,     0,  1062,
       0,     0,     0,     0,  1067,     0,     0,    13,     0,     0,
    1004,     0,     0,     0,     0,     0,     0,     0,     0,   876,
     876,   876,   766,   767,     0,     0,     0,    14,     0,     0,
       0,     0,     0,   586,   326,   432,     0,  1722,   886,   433,
       0,  1214,   434,     0,     0,     0,     0,     0,     0,     0,
       0,  1691,   886,     0,   858,     0,     0,   435,   427,  1127,
       0,     0,     0,   436,     0,     0,  1004,     0,     0,     0,
    1135,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1758,     0,     0,     0,     0,   428,   429,     0,   857,     0,
       0,     0,     0,     0,     0,     0,  1153,  1155,  1157,  1159,
    1161,  1163,  1165,  1167,  1169,  1171,  1173,  1174,  1176,  1178,
    1180,  1182,  1184,  1186,  1780,     0,  1781,   768,   769,   770,
     771,   772,     0,     0,   773,     0,  1205,   776,  1207,   777,
     778,   779,   780,     0,     0,     0,  1124,   781,  1786,   782,
     783,     0,     0,   427,     0,     0,     0,     0,     0,   430,
       0,     0,     0,   431,     0,     0,     0,     0,   747,  1239,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     428,   429,  1261,     0,   737,     0,   886,     0,     0,     0,
    1266,     0,     0,     0,   737,     0,     0,     0,     0,  1271,
       0,  1272,     0,  1273,     0,  1274,     0,     0,     0,   792,
     793,   794,   795,   796,   797,   798,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   799,   800,     0,   432,     0,
       0,     0,   433,     0,  1395,   434,   886,     0,     0,     0,
       0,     0,     0,     0,   430,     0,     0,     0,   431,     0,
     435,   886,     0,     0,     0,     0,   436,     0,     0,     0,
       0,     0,     0,     0,  1312,     0,     0,     0,  1317,     0,
       0,     0,     0,   876,     0,     0,     0,  1323,  1324,  1325,
    1326,  1327,  1328,  1329,  1330,  1331,  1332,  1333,  1334,  1335,
    1336,  1337,  1338,  1339,   732,     0,     0,     0,     0,     0,
     285,     0,     0,   766,   767,     0,   286,     0,     0,     0,
       0,     0,   287,   432,     0,     0,     0,   433,  1362,  1396,
     434,     0,   288,     0,  1363,     0,     0,     0,     0,  1367,
     289,     0,     0,     0,  1271,   435,   876,     0,     0,     0,
       0,   436,     0,     0,     0,   290,     0,     0,     0,  1379,
       0,     0,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,     0,     0,     0,     0,     0,
    1068,  1069,  1070,  1071,  1072,  1073,  1074,  1075,   768,   769,
     770,   771,   772,  1076,  1077,   773,   774,   775,   776,  1078,
     777,   778,   779,   780,     0,     0,     0,     0,   781,   908,
     782,   783,  1079,  1080,    64,     0,   784,   785,   786,  1081,
    1082,  1083,   787,     0,     0,     0,     0,   324,     0,     0,
       0,     0,     0,     0,     0,   886,     0,    13,     0,     0,
    1429,     0,     0,     0,    65,     0,     0,     0,  1433,   962,
       0,     0,     0,     0,     0,     0,     0,    14,     0,     0,
       0,     0,     0,     0,  1084,   788,     0,   789,   790,   791,
     792,   793,   794,   795,   796,   797,   798,  1455,  1456,     0,
       0,     0,     0,     0,     0,  1271,   799,   800,     0,     0,
     325,   519,   874,     0,     0,     0,     0,     0,  1472,     0,
    1474,     0,  1477,   886,     0,     0,     0,     0,  1480,     0,
       0,     0,  1483,     0,     0,     0,   876,   876,   876,     0,
       0,   886,     0,   886,     0,   886,     0,   886,     0,   886,
       0,   886,     0,   886,     0,   886,     0,   886,     0,   886,
       0,   886,     0,     0,   886,     0,   886,     0,   886,     0,
     886,     0,   886,     0,   886,     0,     0,   734,     0,     0,
       0,     0,     0,   285,   427,     0,     0,     0,     0,   286,
    1506,     0,     0,     0,     0,   287,  1510,     0,     0,   684,
       0,     0,     0,     0,     0,   288,     0,     0,     0,     0,
       0,   428,   429,   289,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   290,     0,
       0,     0,   737,     0,     0,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,     0,     0,
       0,  1564,  1565,  1566,     0,   430,     0,     0,     0,   431,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1587,     0,
    1588,     0,   427,     0,     0,     0,  1592,    64,     0,     0,
       0,   427,     0,     0,     0,     0,     0,     0,     0,     0,
     324,     0,  1595,  1511,     0,     0,     0,     0,     0,   428,
     429,     0,   766,   767,     0,     0,     0,    65,   428,   429,
       0,     0,     0,     0,   432,     0,  1615,     0,   433,  1618,
    1411,   434,     0,     0,     0,     0,     0,  1625,  1626,  1627,
       0,     0,     0,     0,  1634,     0,   435,  1636,     0,     0,
    1638,     0,   436,   737,  1641,     0,     0,     0,   737,  1645,
       0,  1647,  1648,   325,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   430,     0,     0,  1655,   431,     0,     0,
       0,     0,   430,     0,     0,     0,   431,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1672,
       0,     0,     0,     0,     0,     0,     0,   768,   769,   770,
     771,   772,     0,     0,   773,   774,   775,   776,     0,   777,
     778,   779,   780,   737,     0,     0,     0,   781,     0,   782,
     783,     0,     0,     0,     0,   784,   785,   786,     0,     0,
       0,   787,   432,     0,     0,  1702,   433,     0,  1415,   434,
       0,   432,     0,  1708,     0,   433,     0,  1416,   434,     0,
       0,     0,     0,     0,   435,  1715,     0,     0,     0,     0,
     436,     0,     0,   435,     0,   886,  1721,     0,     0,   436,
       0,     0,  1730,  1731,   788,     0,   789,   790,   791,   792,
     793,   794,   795,   796,   797,   798,     0,     0,     0,     0,
       0,     0,  1748,  1749,     0,   799,   800,     0,     0,     0,
       0,     0,     0,  1752,     0,     0,     0,  1755,  1757,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1773,     0,     0,     0,     0,   632,     0,     0,     0,   467,
     468,     3,  1779,   633,   634,   635,     0,   636,     0,   469,
     470,   471,   472,   473,     0,     0,     0,     0,     0,   474,
     637,   475,   638,   639,     0,   476,     0,     0,     0,     0,
       0,     0,   640,   477,   641,     0,   642,     0,   643,   478,
       0,     0,   479,     0,     8,   480,   644,     0,   645,   481,
       0,     0,   646,   647,     0,     0,     0,     0,     0,   648,
       0,     0,   483,   484,     0,   291,   292,   293,     0,   295,
     296,   297,   298,   299,   485,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,     0,   313,   314,   315,
       0,     0,   318,   319,   320,   321,   486,   487,   649,   650,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   489,   490,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   651,   652,   653,
       0,     0,     0,     0,     0,     0,     0,    64,     0,   886,
       0,     0,     0,     0,     0,   491,   492,   493,   494,   495,
       0,   496,     0,   497,   498,   499,   500,     0,   152,    13,
     501,   502,   503,   504,   505,   506,   507,    65,   654,   509,
     510,     0,     0,   886,     0,   886,     0,   511,   153,    14,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   512,   513,   514,   886,    15,     0,
       0,   515,   516,     0,     0,   467,   468,     0,     0,   517,
       0,   518,     0,   519,   520,   469,   470,   471,   472,   473,
       0,     0,     0,     0,     0,   474,     0,   475,     0,     0,
       0,   476,     0,   427,     0,     0,     0,     0,     0,   477,
       0,     0,     0,     0,     0,   478,     0,     0,   479,     0,
       0,   480,     0,   958,     0,   481,     0,     0,     0,     0,
     428,   429,     0,     0,     0,   482,     0,     0,   483,   484,
       0,   291,   292,   293,     0,   295,   296,   297,   298,   299,
     485,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,     0,   313,   314,   315,     0,     0,   318,   319,
     320,   321,   486,   487,   488,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   489,   490,
       0,     0,     0,     0,   430,     0,     0,     0,   431,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,     0,     0,     0,     0,     0,     0,
       0,   491,   492,   493,   494,   495,     0,   496,   757,   497,
     498,   499,   500,     0,     0,     0,   501,   502,   503,   504,
     505,   506,   507,   758,   508,   509,   510,     0,     0,     0,
       0,     0,     0,   511,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   432,     0,     0,     0,   433,     0,     0,
     959,   513,   514,     0,    15,     0,     0,   515,   516,     0,
       0,     0,   467,   468,     0,   960,     0,   961,     0,   519,
     520,   436,   469,   470,   471,   472,   473,     0,     0,     0,
       0,     0,   474,     0,   475,     0,     0,     0,   476,     0,
     427,     0,     0,     0,     0,     0,   477,     0,     0,     0,
       0,     0,   478,     0,     0,   479,     0,     0,   480,     0,
       0,     0,   481,     0,     0,     0,     0,   428,   429,     0,
       0,     0,   482,     0,     0,   483,   484,     0,   291,   292,
     293,     0,   295,   296,   297,   298,   299,   485,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,     0,
     313,   314,   315,     0,     0,   318,   319,   320,   321,   486,
     487,   488,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   489,   490,     0,     0,     0,
       0,   430,     0,     0,     0,   431,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   491,   492,
     493,   494,   495,     0,   496,   757,   497,   498,   499,   500,
       0,     0,     0,   501,   502,   503,   504,   505,   506,   507,
     758,   508,   509,   510,     0,     0,     0,     0,     0,     0,
     511,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     432,     0,     0,     0,   433,     0,     0,   959,   513,   514,
       0,    15,     0,     0,   515,   516,     0,     0,     0,   467,
     468,     0,   960,     0,   969,     0,   519,   520,   436,   469,
     470,   471,   472,   473,     0,     0,     0,     0,     0,   474,
       0,   475,     0,     0,     0,   476,     0,   575,     0,     0,
       0,     0,     0,   477,     0,     0,     0,     0,     0,   478,
       0,     0,   479,     0,     0,   480,     0,     0,     0,   481,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   482,
       0,     0,   483,   484,     0,   291,   292,   293,     0,   295,
     296,   297,   298,   299,   485,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,     0,   313,   314,   315,
       0,     0,   318,   319,   320,   321,   486,   487,   488,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   489,   490,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    64,     0,     0,
       0,     0,     0,     0,     0,   491,   492,   493,   494,   495,
       0,   496,     0,   497,   498,   499,   500,     0,     0,     0,
     501,   502,   503,   504,   505,   506,   507,    65,   508,   509,
     510,     0,     0,     0,     0,     0,     0,   511,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   576,     0,     0,   512,   513,   514,     0,    15,     0,
       0,   515,   516,     0,     0,     0,   467,   468,     0,  1238,
       0,   518,     0,   519,   520,   578,   469,   470,   471,   472,
     473,     0,     0,     0,     0,     0,   474,     0,   475,     0,
       0,     0,   476,     0,     0,     0,     0,     0,     0,     0,
     477,     0,     0,     0,     0,     0,   478,     0,     0,   479,
       0,     0,   480,     0,     0,     0,   481,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   482,     0,     0,   483,
     484,     0,   291,   292,   293,     0,   295,   296,   297,   298,
     299,   485,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,     0,   313,   314,   315,     0,     0,   318,
     319,   320,   321,   486,   487,   649,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   489,
     490,     0,     0,     0,     0,     0,     0,     0,   855,     0,
       0,     0,     0,     0,   651,   652,   653,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   491,   492,   493,   494,   495,     0,   496,     0,
     497,   498,   499,   500,     0,     0,     0,   501,   502,   503,
     504,   505,   506,   507,    65,   508,   509,   510,     0,     0,
       0,     0,     0,     0,   511,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,     0,    15,     0,     0,   515,   516,
       0,     0,   467,   468,     0,     0,   517,     0,   518,     0,
     519,   520,   469,   470,   471,   472,   473,     0,     0,     0,
       0,     0,   474,     0,   475,     0,     0,     0,   476,     0,
       0,     0,     0,     0,     0,     0,   477,     0,     0,     0,
       0,     0,   478,     0,     0,   479,     0,     0,   480,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   482,     0,     0,   483,   484,     0,   291,   292,
     293,     0,   295,   296,   297,   298,   299,   485,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,     0,
     313,   314,   315,     0,     0,   318,   319,   320,   321,   486,
     487,   649,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   489,   490,     0,     0,     0,
       0,     0,     0,     0,   871,     0,     0,     0,     0,     0,
     651,   652,   653,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   491,   492,
     493,   494,   495,     0,   496,     0,   497,   498,   499,   500,
       0,     0,     0,   501,   502,   503,   504,   505,   506,   507,
      65,   508,   509,   510,     0,     0,     0,     0,     0,     0,
     511,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   512,   513,   514,
       0,    15,     0,     0,   515,   516,     0,     0,   467,   468,
       0,     0,   517,     0,   518,     0,   519,   520,   469,   470,
     471,   472,   473,     0,     0,     0,     0,     0,   474,  1665,
     475,   638,     0,     0,   476,     0,     0,     0,     0,     0,
       0,     0,   477,     0,     0,     0,     0,     0,   478,     0,
       0,   479,     0,     0,   480,   644,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   482,     0,
       0,   483,   484,     0,   291,   292,   293,     0,   295,   296,
     297,   298,   299,   485,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,     0,   313,   314,   315,     0,
       0,   318,   319,   320,   321,   486,   487,   488,  1666,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   489,   490,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   491,   492,   493,   494,   495,     0,
     496,     0,   497,   498,   499,   500,     0,     0,     0,   501,
     502,   503,   504,   505,   506,   507,    65,   508,   509,   510,
       0,     0,     0,     0,     0,     0,   511,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,     0,    15,     0,     0,
     515,   516,     0,     0,   467,   468,     0,     0,   517,     0,
     518,     0,   519,   520,   469,   470,   471,   472,   473,     0,
       0,     0,     0,     0,   474,     0,   475,     0,     0,     0,
     476,     0,     0,     0,     0,     0,     0,     0,   477,     0,
       0,     0,     0,     0,   478,     0,     0,   479,     0,     0,
     480,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   482,     0,     0,   483,   484,     0,
     291,   292,   293,     0,   295,   296,   297,   298,   299,   485,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,     0,   313,   314,   315,     0,     0,   318,   319,   320,
     321,   486,   487,   649,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   489,   490,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   651,   652,   653,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     491,   492,   493,   494,   495,     0,   496,     0,   497,   498,
     499,   500,     0,     0,     0,   501,   502,   503,   504,   505,
     506,   507,    65,   508,   509,   510,     0,     0,     0,     0,
       0,     0,   511,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,     0,    15,     0,     0,   515,   516,     0,     0,
     467,   468,     0,     0,   517,     0,   518,     0,   519,   520,
     469,   470,   471,   472,   473,     0,     0,     0,     0,     0,
     474,     0,   475,     0,     0,     0,   476,     0,     0,     0,
       0,     0,     0,     0,   477,     0,     0,     0,     0,     0,
     478,     0,     0,   479,     0,     0,   480,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     482,     0,     0,   483,   484,  1006,   291,   292,   293,     0,
     295,   296,   297,   298,   299,   485,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,     0,   313,   314,
     315,     0,     0,   318,   319,   320,   321,   486,   487,   488,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   489,   490,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   491,   492,   493,   494,
     495,     0,   496,   757,   497,   498,   499,   500,     0,     0,
       0,   501,   502,   503,   504,   505,   506,   507,   758,   508,
     509,   510,     0,     0,     0,     0,     0,     0,   511,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,     0,    15,
       0,     0,   515,   516,     0,     0,   467,   468,     0,     0,
    1007,     0,   518,  1008,   519,   520,   469,   470,   471,   472,
     473,     0,     0,     0,     0,     0,   474,     0,   475,     0,
       0,   427,   476,     0,     0,     0,     0,     0,     0,     0,
     477,     0,     0,     0,     0,     0,   478,     0,     0,   479,
       0,     0,   480,     0,     0,     0,   481,     0,   428,   429,
       0,     0,     0,     0,     0,     0,   482,     0,     0,   483,
     484,     0,   291,   292,   293,     0,   295,   296,   297,   298,
     299,   485,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,     0,   313,   314,   315,     0,     0,   318,
     319,   320,   321,   486,   487,   649,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   489,
     490,     0,   430,     0,     0,     0,   431,     0,     0,     0,
       0,     0,     0,     0,  1148,  1149,  1150,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   491,   492,   493,   494,   495,     0,   496,     0,
     497,   498,   499,   500,     0,     0,     0,   501,   502,   503,
     504,   505,   506,   507,    65,   508,   509,   510,     0,     0,
       0,     0,     0,     0,   511,     0,     0,     0,     0,     0,
       0,   432,     0,     0,     0,   433,     0,  1417,   434,     0,
       0,   512,   513,   514,     0,    15,     0,     0,   515,   516,
       0,     0,     0,   435,   467,   468,   517,     0,   518,   436,
     519,   520,   752,     0,   469,   470,   471,   472,   473,     0,
       0,     0,     0,     0,   474,     0,   475,     0,     0,   427,
     476,     0,     0,     0,     0,     0,     0,     0,   477,     0,
       0,     0,     0,     0,   478,     0,     0,   479,   753,     0,
     480,     0,     0,     0,   481,     0,   428,   429,     0,     0,
       0,     0,     0,     0,   482,     0,     0,   483,   484,     0,
     291,   292,   293,     0,   295,   296,   297,   298,   299,   485,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,     0,   313,   314,   315,     0,     0,   318,   319,   320,
     321,   486,   487,   488,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   489,   490,     0,
     430,     0,     0,     0,   431,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     491,   492,   493,   494,   495,     0,   496,     0,   497,   498,
     499,   500,     0,     0,     0,   501,   502,   503,   504,   505,
     506,   507,    65,   508,   509,   510,     0,     0,     0,     0,
       0,     0,   511,     0,     0,     0,     0,     0,     0,   432,
       0,     0,     0,   433,     0,  1418,   434,     0,     0,   512,
     513,   514,     0,    15,     0,     0,   515,   516,     0,     0,
       0,   435,   467,   468,   517,   579,   518,   436,   519,   520,
     752,     0,   469,   470,   471,   472,   473,     0,     0,     0,
       0,     0,   474,     0,   475,     0,     0,   427,   476,     0,
       0,     0,     0,     0,     0,     0,   477,     0,     0,     0,
       0,     0,   478,     0,     0,   479,   753,     0,   480,     0,
       0,     0,   481,     0,   428,   429,     0,     0,     0,     0,
       0,     0,   482,     0,     0,   483,   484,     0,   291,   292,
     293,     0,   295,   296,   297,   298,   299,   485,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,     0,
     313,   314,   315,     0,     0,   318,   319,   320,   321,   486,
     487,   488,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   489,   490,     0,   430,     0,
       0,     0,   431,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   491,   492,
     493,   494,   495,     0,   496,   757,   497,   498,   499,   500,
       0,     0,     0,   501,   502,   503,   504,   505,   506,   507,
     758,   508,   509,   510,     0,     0,     0,     0,     0,     0,
     511,     0,     0,     0,     0,     0,     0,   432,     0,     0,
       0,   433,     0,  1420,   434,     0,     0,   512,   513,   514,
       0,    15,     0,     0,   515,   516,     0,     0,     0,   435,
     467,   468,   517,     0,   518,   436,   519,   520,   752,     0,
     469,   470,   471,   472,   473,     0,     0,     0,     0,     0,
     474,     0,   475,     0,     0,   427,   476,     0,     0,     0,
       0,     0,     0,     0,   477,     0,     0,     0,     0,     0,
     478,     0,     0,   479,   753,     0,   480,     0,     0,     0,
     481,     0,   428,   429,     0,     0,     0,     0,     0,     0,
     482,     0,     0,   483,   484,     0,   291,   292,   293,     0,
     295,   296,   297,   298,   299,   485,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,     0,   313,   314,
     315,     0,     0,   318,   319,   320,   321,   486,   487,   488,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   489,   490,     0,   430,     0,     0,     0,
     431,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   491,   492,   493,   494,
     495,     0,   496,     0,   497,   498,   499,   500,     0,     0,
       0,   501,   502,   503,   504,   505,   506,   507,    65,   508,
     509,   510,     0,     0,     0,     0,     0,     0,   511,     0,
       0,     0,     0,     0,     0,   432,     0,     0,     0,   433,
       0,  1532,   434,     0,     0,   512,   513,   514,     0,    15,
       0,     0,   515,   516,     0,     0,     0,   435,   467,   468,
     517,   832,   518,   436,   519,   520,   752,     0,   469,   470,
     471,   472,   473,     0,     0,     0,     0,     0,   474,     0,
     475,     0,     0,     0,   476,     0,     0,     0,     0,     0,
       0,     0,   477,     0,     0,     0,     0,     0,   478,     0,
       0,   479,   753,     0,   480,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   482,     0,
       0,   483,   484,     0,   291,   292,   293,     0,   295,   296,
     297,   298,   299,   485,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,     0,   313,   314,   315,     0,
       0,   318,   319,   320,   321,   486,   487,   488,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   489,   490,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   491,   492,   493,   494,   495,     0,
     496,     0,   497,   498,   499,   500,     0,     0,     0,   501,
     502,   503,   504,   505,   506,   507,    65,   508,   509,   510,
       0,     0,     0,     0,     0,     0,   511,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,     0,    15,     0,     0,
     515,   516,     0,     0,   467,   468,     0,     0,   517,     0,
     518,     0,   519,   520,   469,   470,   471,   472,   473,     0,
       0,     0,     0,     0,   474,     0,   475,     0,     0,     0,
     476,     0,     0,     0,     0,     0,     0,     0,   477,     0,
       0,     0,     0,     0,   478,     0,     0,   479,     0,     0,
     480,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   482,     0,     0,   483,   484,  1200,
     291,   292,   293,     0,   295,   296,   297,   298,   299,   485,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,     0,   313,   314,   315,     0,     0,   318,   319,   320,
     321,   486,   487,   488,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   489,   490,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     491,   492,   493,   494,   495,     0,   496,   757,   497,   498,
     499,   500,     0,     0,     0,   501,   502,   503,   504,   505,
     506,   507,   758,   508,   509,   510,     0,     0,     0,     0,
       0,     0,   511,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,     0,    15,     0,     0,   515,   516,     0,     0,
     467,   468,     0,     0,   517,     0,   518,     0,   519,   520,
     469,   470,   471,   472,   473,     0,     0,     0,     0,     0,
     474,     0,   475,     0,     0,     0,   476,     0,     0,     0,
       0,     0,     0,     0,   477,     0,     0,     0,     0,     0,
     478,     0,     0,   479,     0,     0,   480,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     482,     0,     0,   483,   484,     0,   291,   292,   293,     0,
     295,   296,   297,   298,   299,   485,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,     0,   313,   314,
     315,     0,     0,   318,   319,   320,   321,   486,   487,   488,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   489,   490,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   491,   492,   493,   494,
     495,     0,   496,   757,   497,   498,   499,   500,     0,     0,
       0,   501,   502,   503,   504,   505,   506,   507,   758,   508,
     509,   510,     0,     0,     0,     0,     0,     0,   511,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,     0,    15,
       0,     0,   515,   516,     0,     0,   467,   468,     0,     0,
     517,     0,   518,  1240,   519,   520,   469,   470,   471,   472,
     473,     0,     0,     0,     0,     0,   474,     0,   475,     0,
       0,     0,   476,     0,     0,     0,     0,     0,     0,     0,
     477,     0,     0,     0,     0,     0,   478,     0,     0,   479,
       0,     0,   480,     0,     0,     0,   481,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   482,     0,     0,   483,
     484,     0,   291,   292,   293,     0,   295,   296,   297,   298,
     299,   485,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,     0,   313,   314,   315,     0,     0,   318,
     319,   320,   321,   486,   487,   488,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   489,
     490,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   491,   492,   493,   494,   495,     0,   496,   757,
     497,   498,   499,   500,     0,     0,     0,   501,   502,   503,
     504,   505,   506,   507,   758,   508,   509,   510,     0,     0,
       0,     0,     0,     0,   511,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,     0,    15,     0,     0,   515,   516,
       0,     0,   467,   468,     0,     0,   517,     0,   518,  1255,
     519,   520,   469,   470,   471,   472,   473,     0,     0,     0,
       0,     0,   474,     0,   475,     0,     0,   427,   476,     0,
       0,     0,     0,     0,     0,     0,   477,     0,     0,     0,
       0,     0,   478,     0,     0,   479,     0,     0,   480,     0,
       0,     0,   481,     0,   428,   429,     0,     0,     0,     0,
       0,     0,   482,     0,     0,   483,   484,     0,   291,   292,
     293,     0,   295,   296,   297,   298,   299,   485,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,     0,
     313,   314,   315,     0,     0,   318,   319,   320,   321,   486,
     487,   488,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   489,   490,     0,   430,     0,
       0,     0,   431,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   491,   492,
     493,   494,   495,     0,   496,     0,   497,   498,   499,   500,
       0,     0,     0,   501,   502,   503,   504,   505,   506,   507,
      65,   508,   509,   510,     0,     0,     0,     0,     0,     0,
     511,     0,     0,     0,     0,     0,     0,   432,     0,     0,
       0,   433,     0,  1537,   434,     0,     0,   512,   513,   514,
       0,    15,     0,     0,   515,   516,     0,     0,     0,   435,
     467,   468,   517,   579,   518,   436,   519,   520,   736,     0,
     469,   470,   471,   472,   473,     0,     0,     0,     0,     0,
     474,     0,   475,     0,     0,   427,   476,     0,     0,     0,
       0,     0,     0,     0,   477,     0,     0,     0,     0,     0,
     478,     0,     0,   479,     0,     0,   480,     0,     0,     0,
     481,     0,   428,   429,     0,     0,     0,     0,     0,     0,
     482,     0,     0,   483,   484,     0,   291,   292,   293,     0,
     295,   296,   297,   298,   299,   485,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,     0,   313,   314,
     315,     0,     0,   318,   319,   320,   321,   486,   487,   488,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   489,   490,     0,   430,     0,     0,     0,
     431,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   491,   492,   493,   494,
     495,     0,   496,     0,   497,   498,   499,   500,     0,     0,
       0,   501,   502,   503,   504,   505,   506,   507,    65,   508,
     509,   510,     0,     0,     0,     0,     0,     0,   511,     0,
       0,     0,     0,     0,     0,   432,     0,     0,     0,   433,
       0,  1569,   434,     0,     0,   512,   513,   514,     0,    15,
       0,     0,   515,   516,     0,     0,     0,   435,   467,   468,
     517,     0,   518,   436,   519,   520,   743,     0,   469,   470,
     471,   472,   473,     0,     0,     0,     0,     0,   474,     0,
     475,     0,     0,     0,   476,     0,     0,     0,     0,     0,
       0,     0,   477,     0,     0,     0,     0,     0,   478,     0,
       0,   479,     0,     0,   480,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   482,     0,
       0,   483,   484,     0,   291,   292,   293,     0,   295,   296,
     297,   298,   299,   485,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,     0,   313,   314,   315,     0,
       0,   318,   319,   320,   321,   486,   487,   488,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   489,   490,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   491,   492,   493,   494,   495,     0,
     496,     0,   497,   498,   499,   500,     0,     0,     0,   501,
     502,   503,   504,   505,   506,   507,    65,   508,   509,   510,
       0,     0,     0,     0,     0,     0,   511,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,     0,    15,     0,     0,
     515,   516,     0,     0,   467,   468,     0,     0,   517,     0,
     518,     0,   519,   520,   469,   470,   471,   472,   473,     0,
       0,     0,     0,     0,   474,     0,   475,     0,     0,     0,
     476,     0,     0,     0,     0,     0,     0,     0,   477,     0,
       0,     0,     0,     0,   478,     0,     0,   479,     0,     0,
     480,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   482,     0,     0,   483,   484,     0,
     291,   292,   293,     0,   295,   296,   297,   298,   299,   485,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,     0,   313,   314,   315,     0,     0,   318,   319,   320,
     321,   486,   487,   488,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   489,   490,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     491,   492,   493,   494,   495,     0,   496,   757,   497,   498,
     499,   500,     0,     0,     0,   501,   502,   503,   504,   505,
     506,   507,   758,   508,   509,   510,     0,     0,     0,     0,
       0,     0,   511,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,     0,    15,     0,     0,   515,   516,     0,     0,
     467,   468,     0,     0,   517,     0,   518,     0,   519,   520,
     469,   470,   471,   472,   473,     0,     0,     0,     0,     0,
     474,     0,   475,     0,     0,     0,   476,     0,     0,     0,
       0,     0,     0,     0,   477,     0,     0,     0,     0,     0,
     478,     0,     0,   479,     0,     0,   480,     0,     0,     0,
     481,     0,     0,     0,     0,     0,   860,     0,     0,     0,
     482,     0,     0,   483,   484,     0,   291,   292,   293,     0,
     295,   296,   297,   298,   299,   485,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,     0,   313,   314,
     315,     0,     0,   318,   319,   320,   321,   486,   487,   488,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   489,   490,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   491,   492,   493,   494,
     495,     0,   496,     0,   497,   498,   499,   500,     0,     0,
       0,   501,   502,   503,   504,   505,   506,   507,    65,   508,
     509,   510,     0,     0,     0,     0,     0,     0,   511,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,     0,    15,
       0,     0,   515,   516,     0,     0,   467,   468,     0,     0,
     517,     0,   518,     0,   519,   520,   469,   470,   471,   472,
     473,     0,     0,     0,     0,     0,   474,     0,   475,     0,
       0,     0,   476,     0,     0,     0,     0,     0,     0,     0,
     477,     0,     0,     0,     0,     0,   478,     0,     0,   479,
       0,     0,   480,     0,     0,     0,   481,     0,     0,   867,
       0,     0,     0,     0,     0,     0,   482,     0,     0,   483,
     484,     0,   291,   292,   293,     0,   295,   296,   297,   298,
     299,   485,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,     0,   313,   314,   315,     0,     0,   318,
     319,   320,   321,   486,   487,   488,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   489,
     490,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   491,   492,   493,   494,   495,     0,   496,     0,
     497,   498,   499,   500,     0,     0,     0,   501,   502,   503,
     504,   505,   506,   507,    65,   508,   509,   510,     0,     0,
       0,     0,     0,     0,   511,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,     0,    15,     0,     0,   515,   516,
       0,     0,   467,   468,     0,     0,   517,     0,   518,     0,
     519,   520,   469,   470,   471,   472,   473,     0,     0,     0,
       0,     0,   474,     0,   475,     0,     0,     0,   476,     0,
       0,     0,     0,     0,     0,     0,   477,     0,     0,     0,
       0,     0,   478,     0,     0,   479,     0,     0,   480,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   482,     0,     0,   483,   484,     0,   291,   292,
     293,     0,   295,   296,   297,   298,   299,   485,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,     0,
     313,   314,   315,     0,     0,   318,   319,   320,   321,   486,
     487,   488,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   489,   490,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   491,   492,
     493,   494,   495,     0,   496,     0,   497,   498,   499,   500,
       0,     0,     0,   501,   502,   503,   504,   505,   506,   507,
      65,   508,   509,   510,     0,     0,     0,     0,     0,     0,
     511,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   740,     0,   512,   513,   514,
       0,    15,     0,     0,   515,   516,     0,     0,   467,   468,
       0,     0,   517,     0,   518,     0,   519,   520,   469,   470,
     471,   472,   473,     0,     0,  1042,     0,     0,   474,     0,
     475,     0,     0,     0,   476,     0,     0,     0,     0,     0,
       0,     0,   477,     0,     0,     0,     0,     0,   478,     0,
       0,   479,     0,     0,   480,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   482,     0,
       0,   483,   484,     0,   291,   292,   293,     0,   295,   296,
     297,   298,   299,   485,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,     0,   313,   314,   315,     0,
       0,   318,   319,   320,   321,   486,   487,   488,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   489,   490,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   491,   492,   493,   494,   495,     0,
     496,     0,   497,   498,   499,   500,     0,     0,     0,   501,
     502,   503,   504,   505,   506,   507,    65,   508,   509,   510,
       0,     0,     0,     0,     0,     0,   511,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,     0,    15,     0,     0,
     515,   516,     0,     0,   467,   468,     0,     0,   517,     0,
     518,     0,   519,   520,   469,   470,   471,   472,   473,     0,
       0,     0,     0,     0,   474,     0,   475,     0,     0,     0,
     476,     0,     0,     0,     0,     0,     0,     0,   477,     0,
       0,     0,     0,     0,   478,     0,     0,   479,     0,     0,
     480,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   482,     0,     0,   483,   484,     0,
     291,   292,   293,     0,   295,   296,   297,   298,   299,   485,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,     0,   313,   314,   315,     0,     0,   318,   319,   320,
     321,   486,   487,   488,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   489,   490,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     491,   492,   493,   494,   495,     0,   496,     0,   497,   498,
     499,   500,     0,     0,     0,   501,   502,   503,   504,   505,
     506,   507,    65,   508,   509,   510,     0,     0,     0,     0,
       0,     0,   511,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,     0,    15,     0,     0,   515,   516,     0,     0,
     467,   468,     0,     0,   517,     0,   518,  1063,   519,   520,
     469,   470,   471,   472,   473,     0,     0,     0,     0,     0,
     474,     0,   475,     0,     0,     0,   476,     0,     0,     0,
       0,     0,     0,     0,   477,     0,     0,     0,     0,     0,
     478,     0,     0,   479,     0,     0,   480,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     482,     0,     0,   483,   484,     0,   291,   292,   293,     0,
     295,   296,   297,   298,   299,   485,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,     0,   313,   314,
     315,     0,     0,   318,   319,   320,   321,   486,   487,   488,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   489,   490,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   491,   492,   493,   494,
     495,     0,   496,     0,   497,   498,   499,   500,     0,     0,
       0,   501,   502,   503,   504,   505,   506,   507,    65,   508,
     509,   510,     0,     0,     0,     0,     0,     0,   511,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1204,     0,   512,   513,   514,     0,    15,
       0,     0,   515,   516,     0,     0,   467,   468,     0,     0,
     517,     0,   518,     0,   519,   520,   469,   470,   471,   472,
     473,     0,     0,     0,     0,     0,   474,     0,   475,     0,
       0,     0,   476,     0,     0,     0,     0,     0,     0,     0,
     477,     0,     0,     0,     0,     0,   478,     0,     0,   479,
       0,     0,   480,     0,     0,     0,   481,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   482,     0,     0,   483,
     484,     0,   291,   292,   293,     0,   295,   296,   297,   298,
     299,   485,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,     0,   313,   314,   315,     0,     0,   318,
     319,   320,   321,   486,   487,   488,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   489,
     490,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   491,   492,   493,   494,   495,     0,   496,     0,
     497,   498,   499,   500,     0,     0,     0,   501,   502,   503,
     504,   505,   506,   507,    65,   508,   509,   510,     0,     0,
       0,     0,     0,     0,   511,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,     0,    15,     0,     0,   515,   516,
       0,     0,   467,   468,     0,     0,   517,     0,   518,  1475,
     519,   520,   469,   470,   471,   472,   473,     0,     0,     0,
       0,     0,   474,     0,   475,     0,     0,     0,   476,     0,
       0,     0,     0,     0,     0,     0,   477,     0,     0,     0,
       0,     0,   478,     0,     0,   479,     0,     0,   480,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   482,     0,     0,   483,   484,     0,   291,   292,
     293,     0,   295,   296,   297,   298,   299,   485,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,     0,
     313,   314,   315,     0,     0,   318,   319,   320,   321,   486,
     487,   488,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   489,   490,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   491,   492,
     493,   494,   495,     0,   496,     0,   497,   498,   499,   500,
       0,     0,     0,   501,   502,   503,   504,   505,   506,   507,
      65,   508,   509,   510,     0,     0,     0,     0,     0,     0,
     511,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   512,   513,   514,
       0,    15,     0,     0,   515,   516,     0,     0,   467,   468,
       0,     0,  1484,     0,   518,  1485,   519,   520,   469,   470,
     471,   472,   473,     0,     0,     0,     0,     0,   474,     0,
     475,     0,     0,     0,   476,     0,     0,     0,     0,     0,
       0,     0,   477,     0,     0,     0,     0,     0,   478,     0,
       0,   479,     0,     0,   480,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   482,     0,
       0,   483,   484,     0,   291,   292,   293,     0,   295,   296,
     297,   298,   299,   485,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,     0,   313,   314,   315,     0,
       0,   318,   319,   320,   321,   486,   487,   488,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   489,   490,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   491,   492,   493,   494,   495,     0,
     496,     0,   497,   498,   499,   500,     0,     0,     0,   501,
     502,   503,   504,   505,   506,   507,    65,   508,   509,   510,
       0,     0,     0,     0,     0,     0,   511,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,     0,    15,     0,     0,
     515,   516,     0,     0,   467,   468,     0,     0,   517,     0,
     518,  1490,   519,   520,   469,   470,   471,   472,   473,     0,
       0,     0,     0,     0,   474,     0,   475,     0,     0,     0,
     476,     0,     0,     0,     0,     0,     0,     0,   477,     0,
       0,     0,     0,     0,   478,     0,     0,   479,     0,     0,
     480,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   482,     0,     0,   483,   484,     0,
     291,   292,   293,     0,   295,   296,   297,   298,   299,   485,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,     0,   313,   314,   315,     0,     0,   318,   319,   320,
     321,   486,   487,   488,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   489,   490,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     491,   492,   493,   494,   495,     0,   496,     0,   497,   498,
     499,   500,     0,     0,     0,   501,   502,   503,   504,   505,
     506,   507,    65,   508,   509,   510,     0,     0,     0,     0,
       0,     0,   511,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,     0,    15,     0,     0,   515,   516,     0,     0,
     467,   468,     0,     0,   517,     0,   518,  1546,   519,   520,
     469,   470,   471,   472,   473,     0,     0,     0,     0,     0,
     474,     0,   475,     0,     0,     0,   476,     0,     0,     0,
       0,     0,     0,     0,   477,     0,     0,     0,     0,     0,
     478,     0,     0,   479,     0,     0,   480,     0,     0,     0,
     481,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     482,     0,     0,   483,   484,     0,   291,   292,   293,     0,
     295,   296,   297,   298,   299,   485,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,     0,   313,   314,
     315,     0,     0,   318,   319,   320,   321,   486,   487,   488,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   489,   490,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   491,   492,   493,   494,
     495,     0,   496,     0,   497,   498,   499,   500,     0,     0,
       0,   501,   502,   503,   504,   505,   506,   507,    65,   508,
     509,   510,     0,     0,     0,     0,     0,     0,   511,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   512,   513,   514,     0,    15,
       0,     0,   515,   516,     0,     0,   467,   468,     0,     0,
     517,     0,   518,  1633,   519,   520,   469,   470,   471,   472,
     473,     0,     0,     0,     0,     0,   474,     0,   475,     0,
       0,     0,   476,     0,     0,     0,     0,     0,     0,     0,
     477,     0,     0,     0,     0,     0,   478,     0,     0,   479,
       0,     0,   480,     0,     0,     0,   481,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   482,     0,     0,   483,
     484,     0,   291,   292,   293,     0,   295,   296,   297,   298,
     299,   485,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,     0,   313,   314,   315,     0,     0,   318,
     319,   320,   321,   486,   487,   488,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   489,
     490,     0,     0,     0,     0,     0,     0,     0,  1654,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,   491,   492,   493,   494,   495,     0,   496,     0,
     497,   498,   499,   500,     0,     0,     0,   501,   502,   503,
     504,   505,   506,   507,    65,   508,   509,   510,     0,     0,
       0,     0,     0,     0,   511,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   512,   513,   514,     0,    15,     0,     0,   515,   516,
       0,     0,   467,   468,     0,     0,   517,     0,   518,     0,
     519,   520,   469,   470,   471,   472,   473,     0,     0,     0,
       0,     0,   474,     0,   475,     0,     0,     0,   476,     0,
       0,     0,     0,     0,     0,     0,   477,     0,     0,     0,
       0,     0,   478,     0,     0,   479,     0,     0,   480,     0,
       0,     0,   481,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   482,     0,     0,   483,   484,     0,   291,   292,
     293,     0,   295,   296,   297,   298,   299,   485,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,     0,
     313,   314,   315,     0,     0,   318,   319,   320,   321,   486,
     487,   488,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   489,   490,     0,     0,     0,
       0,     0,     0,     0,  1713,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,   491,   492,
     493,   494,   495,     0,   496,     0,   497,   498,   499,   500,
       0,     0,     0,   501,   502,   503,   504,   505,   506,   507,
      65,   508,   509,   510,     0,     0,     0,     0,     0,     0,
     511,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   512,   513,   514,
       0,    15,     0,     0,   515,   516,     0,     0,   467,   468,
       0,     0,   517,     0,   518,     0,   519,   520,   469,   470,
     471,   472,   473,     0,     0,     0,     0,     0,   474,     0,
     475,     0,     0,     0,   476,     0,     0,     0,     0,     0,
       0,     0,   477,     0,     0,     0,     0,     0,   478,     0,
       0,   479,     0,     0,   480,     0,     0,     0,   481,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   482,     0,
       0,   483,   484,     0,   291,   292,   293,     0,   295,   296,
     297,   298,   299,   485,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,     0,   313,   314,   315,     0,
       0,   318,   319,   320,   321,   486,   487,   488,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   489,   490,     0,     0,     0,     0,     0,     0,     0,
    1714,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    64,     0,     0,     0,
       0,     0,     0,     0,   491,   492,   493,   494,   495,     0,
     496,     0,   497,   498,   499,   500,     0,     0,     0,   501,
     502,   503,   504,   505,   506,   507,    65,   508,   509,   510,
       0,     0,     0,     0,     0,     0,   511,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   512,   513,   514,     0,    15,     0,     0,
     515,   516,     0,     0,   467,   468,     0,     0,   517,     0,
     518,     0,   519,   520,   469,   470,   471,   472,   473,     0,
       0,     0,     0,     0,   474,     0,   475,     0,     0,     0,
     476,     0,     0,     0,     0,     0,     0,     0,   477,     0,
       0,     0,     0,     0,   478,     0,     0,   479,     0,     0,
     480,     0,     0,     0,   481,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   482,     0,     0,   483,   484,     0,
     291,   292,   293,     0,   295,   296,   297,   298,   299,   485,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,     0,   313,   314,   315,     0,     0,   318,   319,   320,
     321,   486,   487,   488,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   489,   490,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    64,     0,     0,     0,     0,     0,     0,     0,
     491,   492,   493,   494,   495,     0,   496,     0,   497,   498,
     499,   500,     0,     0,     0,   501,   502,   503,   504,   505,
     506,   507,    65,   508,   509,   510,     0,     0,     0,     0,
       0,     0,   511,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   512,
     513,   514,     0,    15,     0,     0,   515,   516,     0,     0,
     467,   468,     0,     0,   517,     0,   518,     0,   519,   520,
     469,   470,   471,   472,   473,     0,     0,     0,     0,     0,
     474,     0,   475,     0,     0,   427,   476,     0,     0,     0,
       0,     0,     0,     0,   477,     0,     0,     0,     0,     0,
     478,     0,     0,   479,     0,     0,   480,     0,     0,     0,
     481,     0,   428,   429,     0,     0,     0,     0,     0,     0,
     482,     0,     0,   483,   484,     0,   291,   292,   293,     0,
     295,   296,   297,   298,   299,   485,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,     0,   313,   314,
     315,     0,     0,   318,   319,   320,   321,   486,   487,   488,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   489,   490,     0,   430,     0,   -81,     0,
     431,     0,     0,     0,     0,     0,     0,     0,     0,   766,
     767,     0,     0,     0,     0,     0,     0,     0,    64,     0,
       0,     0,     0,     0,     0,     0,   491,   492,   493,   494,
     495,     0,   496,     0,   497,   498,   499,   500,     0,     0,
       0,   501,   502,   503,   504,   505,   506,   507,    65,   508,
     509,   510,     0,     0,     0,     0,     0,     0,   511,     0,
       0,     0,     0,     0,     0,   432,     0,     0,     0,   433,
       0,  1656,   434,     0,     0,   512,   513,   514,     0,    15,
       0,     0,   515,   516,     0,     0,     0,   435,  1014,     0,
    1461,     0,   518,   436,   519,   520,   897,   898,   899,   900,
     901,   902,   903,   904,   768,   769,   770,   771,   772,   905,
     906,   773,   774,   775,   776,   907,   777,   778,   779,   780,
       0,     0,     0,     0,   781,   908,   782,   783,   909,   910,
       0,     0,   784,   785,   786,   911,   912,   913,   787,     0,
     291,   292,   293,     0,   295,   296,   297,   298,   299,   485,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,     0,   313,   314,   315,     0,     0,   318,   319,   320,
     321,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     914,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,   285,     0,     0,     0,     0,     0,   286,
       0,     0,   799,   800,     0,   287,     0,   519,   874,     0,
       0,     0,     0,     0,     0,   288,     0,     0,     0,     0,
       0,     0,     0,   289,     0,     0,     0,  1015,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   290,     0,
       0,     0,  1016,     0,     0,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   285,     0,     0,     0,
       0,     0,   286,     0,     0,     0,     0,     0,   287,     0,
       0,     0,     0,     0,     0,     0,     0,    64,   288,     0,
       0,     0,     0,     0,     0,     0,   289,     0,     0,     0,
     324,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   290,     0,     0,     0,     0,     0,    65,   291,   292,
     293,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   320,   321,   322,
     323,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   325,     0,   582,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      64,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   324,     0,     0,     0,     0,     0,     0,
       0,     0,    13,     0,     0,   285,     0,     0,     0,     0,
     585,   286,     0,     0,     0,     0,     0,   287,     0,     0,
       0,     0,    14,     0,     0,     0,     0,   288,   586,     0,
       0,     0,     0,     0,     0,   289,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     290,     0,     0,     0,     0,     0,   325,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   306,   307,   308,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,   319,   320,   321,   322,   323,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   285,     0,
       0,     0,     0,     0,   286,     0,     0,     0,     0,     0,
     287,     0,     0,     0,     0,     0,     0,     0,     0,    64,
     288,     0,     0,     0,     0,     0,     0,     0,   289,     0,
       0,     0,   324,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   290,     0,     0,     0,     0,     0,    65,
     291,   292,   293,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,   317,   318,   319,   320,
     321,   322,   323,     0,   766,   767,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   325,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   766,
     767,     0,    64,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   324,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   585,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1068,  1069,  1070,  1071,  1072,  1073,  1074,  1075,   768,
     769,   770,   771,   772,  1076,  1077,   773,   774,   775,   776,
    1078,   777,   778,   779,   780,  -414,     0,     0,   325,   781,
     908,   782,   783,  1079,  1080,     0,     0,   784,   785,   786,
    1081,  1082,  1083,   787,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
     766,   767,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,     0,
       0,     0,     0,     0,     0,  1084,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,   766,   767,
       0,     0,     0,     0,     0,     0,     0,   799,   800,     0,
       0,     0,   519,   874,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   799,   800,     0,     0,     0,   519,   874,     0,
       0,     0,     0,     0,     0,   768,   769,   770,   771,   772,
       0,     0,   773,   774,   775,   776,     0,   777,   778,   779,
     780,     0,     0,     0,     0,   781,     0,   782,   783,     0,
       0,     0,     0,   784,   785,   786,     0,     0,     0,   787,
       0,     0,     0,   768,   769,   770,   771,   772,     0,     0,
     773,   774,   775,   776,     0,   777,   778,   779,   780,   766,
     767,     0,     0,   781,     0,   782,   783,     0,     0,     0,
       0,   784,   785,   786,     0,     0,     0,   787,     0,     0,
       0,     0,   788,     0,   789,   790,   791,   792,   793,   794,
     795,   796,   797,   798,   766,   767,     0,     0,     0,     0,
       0,     0,     0,   799,   800,     0,     0,   801,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     788,     0,   789,   790,   791,   792,   793,   794,   795,   796,
     797,   798,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   799,   800,     0,     0,   813,     0,     0,     0,     0,
       0,     0,     0,     0,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,   766,   767,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,   784,   785,   786,
       0,     0,     0,   787,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,   766,   767,     0,     0,     0,     0,     0,
       0,     0,   799,   800,     0,     0,   829,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   799,   800,     0,
       0,  1114,     0,     0,     0,     0,     0,     0,     0,     0,
     768,   769,   770,   771,   772,     0,     0,   773,   774,   775,
     776,     0,   777,   778,   779,   780,     0,     0,     0,     0,
     781,     0,   782,   783,     0,     0,     0,     0,   784,   785,
     786,     0,     0,     0,   787,     0,     0,     0,   768,   769,
     770,   771,   772,     0,     0,   773,   774,   775,   776,     0,
     777,   778,   779,   780,   766,   767,     0,     0,   781,     0,
     782,   783,     0,     0,     0,     0,   784,   785,   786,     0,
       0,     0,   787,     0,     0,     0,     0,   788,     0,   789,
     790,   791,   792,   793,   794,   795,   796,   797,   798,   766,
     767,     0,     0,     0,     0,     0,     0,     0,   799,   800,
       0,     0,  1217,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   788,     0,   789,   790,   791,
     792,   793,   794,   795,   796,   797,   798,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   799,   800,     0,     0,
    1220,     0,     0,     0,     0,     0,     0,     0,     0,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,     0,     0,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,   784,   785,   786,
       0,     0,     0,   787,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
     766,   767,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,   766,   767,
       0,     0,     0,     0,     0,     0,     0,   799,   800,     0,
       0,  1222,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   799,   800,     0,     0,  1230,     0,     0,     0,
       0,     0,     0,     0,     0,   768,   769,   770,   771,   772,
       0,     0,   773,   774,   775,   776,     0,   777,   778,   779,
     780,     0,     0,     0,     0,   781,     0,   782,   783,     0,
       0,     0,     0,   784,   785,   786,     0,     0,     0,   787,
       0,     0,     0,   768,   769,   770,   771,   772,     0,     0,
     773,   774,   775,   776,     0,   777,   778,   779,   780,   766,
     767,     0,     0,   781,     0,   782,   783,     0,     0,     0,
       0,   784,   785,   786,     0,     0,     0,   787,     0,     0,
       0,     0,   788,     0,   789,   790,   791,   792,   793,   794,
     795,   796,   797,   798,   766,   767,     0,     0,     0,     0,
       0,     0,     0,   799,   800,     0,     0,  1231,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     788,     0,   789,   790,   791,   792,   793,   794,   795,   796,
     797,   798,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   799,   800,     0,     0,  1232,     0,     0,     0,     0,
       0,     0,     0,     0,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,   766,   767,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,   784,   785,   786,
       0,     0,     0,   787,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,   766,   767,     0,     0,     0,     0,     0,
       0,     0,   799,   800,     0,     0,  1233,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   799,   800,     0,
       0,  1234,     0,     0,     0,     0,     0,     0,     0,     0,
     768,   769,   770,   771,   772,     0,     0,   773,   774,   775,
     776,     0,   777,   778,   779,   780,     0,     0,     0,     0,
     781,     0,   782,   783,     0,     0,     0,     0,   784,   785,
     786,     0,     0,     0,   787,     0,     0,     0,   768,   769,
     770,   771,   772,     0,     0,   773,   774,   775,   776,     0,
     777,   778,   779,   780,   766,   767,     0,     0,   781,     0,
     782,   783,     0,     0,     0,     0,   784,   785,   786,     0,
       0,     0,   787,     0,     0,     0,     0,   788,     0,   789,
     790,   791,   792,   793,   794,   795,   796,   797,   798,   766,
     767,     0,     0,     0,     0,     0,     0,     0,   799,   800,
       0,     0,  1235,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   788,     0,   789,   790,   791,
     792,   793,   794,   795,   796,   797,   798,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   799,   800,     0,     0,
    1403,     0,     0,     0,     0,     0,     0,     0,     0,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,     0,     0,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,   784,   785,   786,
       0,     0,     0,   787,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
     766,   767,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,   766,   767,
       0,     0,     0,     0,     0,     0,     0,   799,   800,     0,
       0,  1406,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   799,   800,     0,     0,  1451,     0,     0,     0,
       0,     0,     0,     0,     0,   768,   769,   770,   771,   772,
       0,     0,   773,   774,   775,   776,     0,   777,   778,   779,
     780,     0,     0,     0,     0,   781,     0,   782,   783,     0,
       0,     0,     0,   784,   785,   786,     0,     0,     0,   787,
       0,     0,     0,   768,   769,   770,   771,   772,     0,     0,
     773,   774,   775,   776,     0,   777,   778,   779,   780,   766,
     767,     0,     0,   781,     0,   782,   783,     0,     0,     0,
       0,   784,   785,   786,     0,     0,     0,   787,     0,     0,
       0,     0,   788,     0,   789,   790,   791,   792,   793,   794,
     795,   796,   797,   798,   766,   767,     0,     0,     0,     0,
       0,     0,     0,   799,   800,     0,     0,  1501,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     788,     0,   789,   790,   791,   792,   793,   794,   795,   796,
     797,   798,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   799,   800,     0,     0,  1567,     0,     0,     0,     0,
       0,     0,     0,     0,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,   766,   767,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,   784,   785,   786,
       0,     0,     0,   787,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,   766,   767,     0,     0,     0,     0,     0,
       0,     0,   799,   800,     0,     0,  1568,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   799,   800,     0,
       0,  1581,     0,     0,     0,     0,     0,     0,     0,     0,
     768,   769,   770,   771,   772,     0,     0,   773,   774,   775,
     776,     0,   777,   778,   779,   780,     0,     0,     0,     0,
     781,     0,   782,   783,     0,     0,     0,     0,   784,   785,
     786,     0,     0,     0,   787,     0,     0,     0,   768,   769,
     770,   771,   772,     0,     0,   773,   774,   775,   776,     0,
     777,   778,   779,   780,   766,   767,     0,     0,   781,     0,
     782,   783,     0,     0,     0,     0,   784,   785,   786,     0,
       0,     0,   787,     0,     0,     0,     0,   788,     0,   789,
     790,   791,   792,   793,   794,   795,   796,   797,   798,   766,
     767,     0,     0,     0,     0,     0,     0,     0,   799,   800,
       0,     0,  1583,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   788,     0,   789,   790,   791,
     792,   793,   794,   795,   796,   797,   798,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   799,   800,     0,     0,
    1585,     0,     0,     0,     0,     0,     0,     0,     0,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,     0,     0,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,   784,   785,   786,
       0,     0,     0,   787,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
     766,   767,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,   766,   767,
       0,     0,     0,     0,     0,     0,     0,   799,   800,     0,
       0,  1589,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   799,   800,     0,     0,  1649,     0,     0,     0,
       0,     0,     0,     0,     0,   768,   769,   770,   771,   772,
       0,     0,   773,   774,   775,   776,     0,   777,   778,   779,
     780,     0,     0,     0,     0,   781,     0,   782,   783,     0,
       0,     0,     0,   784,   785,   786,     0,     0,     0,   787,
       0,     0,     0,   768,   769,   770,   771,   772,     0,     0,
     773,   774,   775,   776,     0,   777,   778,   779,   780,   766,
     767,     0,     0,   781,     0,   782,   783,     0,     0,     0,
       0,   784,   785,   786,     0,     0,     0,   787,     0,     0,
       0,     0,   788,     0,   789,   790,   791,   792,   793,   794,
     795,   796,   797,   798,   766,   767,     0,     0,     0,     0,
       0,     0,     0,   799,   800,     0,     0,  1659,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     788,     0,   789,   790,   791,   792,   793,   794,   795,   796,
     797,   798,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   799,   800,     0,     0,  1660,     0,     0,     0,     0,
       0,     0,     0,     0,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,   766,   767,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,   784,   785,   786,
       0,     0,     0,   787,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,   766,   767,     0,     0,     0,     0,     0,
       0,     0,   799,   800,     0,     0,  1662,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   799,   800,     0,
       0,  1682,     0,     0,     0,     0,     0,     0,     0,     0,
     768,   769,   770,   771,   772,     0,     0,   773,   774,   775,
     776,     0,   777,   778,   779,   780,     0,     0,     0,     0,
     781,     0,   782,   783,     0,     0,     0,     0,   784,   785,
     786,     0,     0,     0,   787,     0,     0,     0,   768,   769,
     770,   771,   772,     0,     0,   773,   774,   775,   776,     0,
     777,   778,   779,   780,   766,   767,     0,     0,   781,     0,
     782,   783,     0,     0,     0,     0,   784,   785,   786,     0,
       0,     0,   787,     0,     0,     0,     0,   788,     0,   789,
     790,   791,   792,   793,   794,   795,   796,   797,   798,   766,
     767,     0,     0,     0,     0,     0,     0,     0,   799,   800,
       0,     0,  1685,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   788,     0,   789,   790,   791,
     792,   793,   794,   795,   796,   797,   798,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   799,   800,     0,     0,
    1694,     0,     0,     0,     0,     0,     0,     0,     0,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,     0,     0,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,   784,   785,   786,
       0,     0,     0,   787,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
     766,   767,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,   766,   767,
       0,     0,     0,     0,     0,     0,     0,   799,   800,     0,
       0,  1765,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   799,   800,     0,     0,  1766,     0,     0,     0,
       0,     0,     0,     0,     0,   768,   769,   770,   771,   772,
       0,     0,   773,   774,   775,   776,     0,   777,   778,   779,
     780,     0,     0,     0,     0,   781,     0,   782,   783,     0,
       0,     0,     0,   784,   785,   786,     0,     0,     0,   787,
       0,     0,     0,   768,   769,   770,   771,   772,     0,     0,
     773,   774,   775,   776,     0,   777,   778,   779,   780,   766,
     767,     0,     0,   781,     0,   782,   783,     0,     0,     0,
       0,   784,   785,   786,     0,     0,     0,   787,     0,     0,
       0,     0,   788,     0,   789,   790,   791,   792,   793,   794,
     795,   796,   797,   798,   766,   767,     0,     0,     0,     0,
       0,     0,     0,   799,   800,   833,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     788,     0,   789,   790,   791,   792,   793,   794,   795,   796,
     797,   798,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   799,   800,  1108,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,   766,   767,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,   784,   785,   786,
       0,     0,     0,   787,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,   766,   767,     0,     0,     0,     0,     0,
       0,     0,   799,   800,  1304,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   788,     0,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   799,   800,  1320,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     768,   769,   770,   771,   772,     0,     0,   773,   774,   775,
     776,     0,   777,   778,   779,   780,     0,     0,     0,     0,
     781,     0,   782,   783,     0,     0,     0,     0,   784,   785,
     786,     0,     0,     0,   787,     0,     0,     0,   768,   769,
     770,   771,   772,     0,     0,   773,   774,   775,   776,     0,
     777,   778,   779,   780,   340,   341,     0,     0,   781,     0,
     782,   783,     0,     0,     0,     0,   784,   785,   786,     0,
       0,   342,   787,     0,     0,     0,     0,   788,     0,   789,
     790,   791,   792,   793,   794,   795,   796,   797,   798,     0,
       0,     0,     0,     0,     0,     0,   766,   767,   799,   800,
    1482,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   788,     0,   789,   790,   791,
     792,   793,   794,   795,   796,   797,   798,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   799,   800,  1488,     0,
       0,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,     0,
       0,   361,   362,   363,     0,     0,     0,     0,     0,     0,
     364,   365,   366,   367,   368,     0,     0,   369,   370,   371,
     372,   373,   374,   375,     0,     0,     0,     0,     0,   766,
     767,   768,   769,   770,   771,   772,     0,     0,   773,   774,
     775,   776,     0,   777,   778,   779,   780,     0,     0,     0,
       0,   781,     0,   782,   783,     0,     0,     0,     0,   784,
     785,   786,     0,     0,     0,   787,   376,     0,   377,   378,
     379,   380,   381,   382,   383,   384,   385,   386,    52,     0,
     387,   388,     0,     0,     0,     0,     0,   389,   390,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    53,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   788,     0,
     789,   790,   791,   792,   793,   794,   795,   796,   797,   798,
       0,     0,   766,   767,   768,   769,   770,   771,   772,   799,
     800,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,   766,
     767,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    13,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    14,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,   768,   769,   770,
     771,   772,   799,   800,   773,   774,   775,   776,     0,   777,
     778,   779,   780,     0,     0,     0,     0,   781,     0,   782,
     783,     0,     0,   973,     0,   784,   785,   786,     0,     0,
       0,   787,   766,   767,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
    1241,     0,   784,   785,   786,     0,     0,     0,   787,   766,
     767,     0,     0,     0,   788,     0,   789,   790,   791,   792,
     793,   794,   795,   796,   797,   798,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   799,   800,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,   768,   769,   770,
     771,   772,   799,   800,   773,   774,   775,   776,     0,   777,
     778,   779,   780,     0,     0,     0,     0,   781,     0,   782,
     783,     0,     0,     0,     0,   784,   785,   786,     0,     0,
       0,   787,   766,   767,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,     0,     0,   787,   766,
     767,     0,     0,     0,   788,  1309,   789,   790,   791,   792,
     793,   794,   795,   796,   797,   798,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   799,   800,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   788,  1446,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,   768,   769,   770,
     771,   772,   799,   800,   773,   774,   775,   776,     0,   777,
     778,   779,   780,     0,     0,     0,     0,   781,     0,   782,
     783,     0,     0,     0,     0,   784,   785,   786,     0,     0,
       0,   787,   766,   767,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,     0,  1695,     0,   787,   766,
     767,     0,     0,     0,   788,     0,   789,   790,   791,   792,
     793,   794,   795,   796,   797,   798,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   799,   800,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,   768,   769,   770,
     771,   772,   799,   800,   773,   774,   775,   776,     0,   777,
     778,   779,   780,     0,     0,     0,     0,   781,     0,   782,
     783,     0,     0,     0,     0,   784,   785,   786,     0,     0,
       0,  -831,   766,   767,   768,   769,   770,   771,   772,     0,
       0,   773,   774,   775,   776,     0,   777,   778,   779,   780,
       0,     0,     0,     0,   781,     0,   782,   783,     0,     0,
       0,     0,   784,   785,   786,   766,   767,     0,     0,     0,
       0,     0,     0,     0,   788,     0,   789,   790,   791,   792,
     793,   794,   795,   796,   797,   798,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   799,   800,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   788,     0,   789,   790,   791,   792,   793,   794,   795,
     796,   797,   798,     0,     0,     0,     0,   768,   769,   770,
     771,   772,   799,   800,   773,   774,   775,   776,     0,   777,
     778,   779,   780,     0,     0,     0,     0,   781,     0,   782,
     783,     0,     0,     0,     0,   784,     0,   786,   766,   767,
     768,   769,   770,   771,   772,     0,     0,   773,   774,   775,
     776,     0,   777,   778,   779,   780,     0,     0,     0,     0,
     781,     0,   782,   783,   766,   767,     0,     0,   784,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   789,   790,   791,   792,
     793,   794,   795,   796,   797,   798,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   799,   800,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   789,
     790,   791,   792,   793,   794,   795,   796,   797,   798,     0,
       0,     0,     0,   768,   769,   770,   771,   772,   799,   800,
     773,   774,   775,   776,     0,   777,   778,   779,   780,     0,
       0,     0,     0,   781,     0,   782,   783,     0,     0,   768,
     769,   770,   771,   772,     0,     0,   773,   774,   775,   776,
       0,   777,   778,   779,   780,     0,     0,     0,     0,   781,
       0,   782,   783,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1018,     0,     0,
       0,     0,   789,   790,   791,   792,   793,   794,   795,   796,
     797,   798,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   799,   800,     0,     0,     0,     0,     0,  1305,     0,
     791,   792,   793,   794,   795,   796,   797,   798,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   799,   800,   291,
     292,   293,     0,   295,   296,   297,   298,   299,   485,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
       0,   313,   314,   315,     0,     0,   318,   319,   320,   321,
     291,   292,   293,     0,   295,   296,   297,   298,   299,   485,
     301,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,     0,   313,   314,   315,     0,     0,   318,   319,   320,
     321,   291,   292,   293,     0,   295,   296,   297,   298,   299,
     485,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,     0,   313,   314,   315,  1019,   229,   318,   319,
     320,   321,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1020,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1306,     0,  1054,
    1055,     0,     0,   230,     0,   231,     0,   232,   233,   234,
     235,   236,  1307,   237,   238,   239,   240,   241,   242,   243,
     244,   245,   246,   247,     0,   248,   249,   250,  1056,     0,
     251,   252,   253,   254,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1057,     0,     0,     0,     0,     0,     0,
     255,   256,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1058,  1059,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   257
};

static const yytype_int16 yycheck[] =
{
       1,   259,   548,   571,   849,   223,   458,   552,   409,   458,
     670,     7,   180,   710,   211,    15,    16,   464,    19,   755,
     441,   403,   500,   759,   760,   440,  1244,   964,   947,   440,
     441,   223,   735,   970,   764,   949,  1137,   438,    33,   820,
     458,   822,   732,   824,   734,  1359,   736,     5,     4,   733,
     562,   735,   564,   743,   566,    53,   113,    87,    22,     8,
      20,   127,   752,    33,    21,    22,  1517,  1608,   126,   191,
      70,    71,    72,    57,    20,   126,   173,   160,    20,    63,
      20,   126,   529,  1650,    20,    20,   185,    19,    20,   652,
     653,    40,   637,    57,   216,   831,    46,   498,   499,   100,
     199,   102,   126,   552,   194,   650,   274,    20,   198,   126,
     143,   127,   129,   130,   114,   115,   116,   117,   144,   145,
     146,   137,   219,    15,    16,   152,   192,   143,  1229,   195,
     165,     5,     6,   160,   552,   131,   194,   220,  1679,  1706,
     173,   199,   173,   194,   102,   103,   173,   166,   199,   194,
     185,    25,   158,  1604,   155,   212,   106,    31,    62,   217,
       7,   118,   119,    53,   854,   194,   217,   173,   127,   126,
     194,   128,   129,   130,   131,   199,   192,   588,   137,   136,
     164,   177,   593,   213,   217,   220,   217,   185,   637,   191,
     217,   203,   218,   217,    68,    69,   926,   214,   215,   935,
     184,   650,   197,    50,   160,   201,   202,   129,   130,   945,
    1428,   217,   948,   214,   216,   164,   165,   409,   191,   637,
     215,    15,    16,   223,   127,    33,   221,   197,   102,   103,
     226,   191,   650,   192,   137,   184,   185,   455,   456,   203,
     198,   173,  1556,   216,    34,   191,   438,   277,   440,   191,
     214,   191,    60,    61,   192,   191,   191,   214,   215,   217,
     805,   458,   143,   455,   456,   462,   215,   464,   158,   143,
     173,  1372,   268,    63,   166,   271,  1006,   155,   191,   171,
    1381,   173,   160,   173,   176,   127,   160,   161,   162,   192,
      57,  1210,   214,   215,   197,   155,    63,  1211,   988,   173,
     160,   194,   127,  1240,   197,   127,   498,   499,   709,   183,
     855,   127,   137,   194,   165,   137,   124,   107,   158,   127,
     128,   137,   723,  1007,   198,   325,   871,   191,   220,   137,
     191,  1549,   895,   173,   185,   553,   554,   555,   191,   557,
     558,   215,   220,   561,   195,   563,   136,   565,   126,   191,
     192,   203,   194,   143,   127,   197,   805,   218,   218,   191,
     220,   553,   554,   555,   137,   557,   558,   192,   163,   561,
     192,   563,   166,   565,    55,   567,   192,   171,   218,   173,
     150,   639,   176,   173,   192,   193,   218,   805,   183,   197,
     648,   163,   200,   152,   591,   958,    57,   398,   595,   399,
     143,   812,    63,   173,   174,   175,   855,   215,   163,   409,
     144,   183,   146,   221,   173,   163,   194,   828,   208,   192,
     101,   199,   871,   217,   219,   220,   220,   217,   183,   876,
     173,   191,  1277,    33,   204,   183,   191,   855,   438,   217,
     440,  1638,   191,  1640,   214,   217,   194,  1644,  1645,   188,
     189,   452,   453,   871,   163,   455,   456,   458,   218,   460,
      60,    61,  1681,   464,   195,   219,   220,   216,   199,   870,
    1200,   471,   472,  1692,   183,    33,   132,   216,   173,   897,
     898,   899,   900,   901,   902,   903,   904,   905,   906,   907,
    1427,   909,   910,   911,   912,   913,   914,   975,   498,   499,
    1437,  1013,    60,    61,   195,  1702,   143,   985,   199,   165,
     165,   217,   894,  1732,  1733,   165,  1297,   709,   143,   142,
    1542,  1366,   923,   924,   124,   936,   694,   152,   128,   185,
     185,   723,   157,   934,    47,   185,   191,  1240,   939,   940,
     163,   942,   192,   944,   199,   946,   195,   548,   173,   165,
     199,   107,  1255,   553,   554,   555,  1240,   557,   558,  1243,
     183,   561,   165,   563,   165,   565,   124,   567,   195,   185,
     128,  1255,   199,   198,   766,   767,   192,    90,    91,   107,
      93,   518,   185,   584,   185,  1148,  1149,  1150,   191,   781,
     191,  1613,  1614,   193,   165,   853,   199,   197,   199,   199,
     200,   191,   191,   107,   165,   863,  1451,   799,   191,  1631,
    1632,     0,   830,   166,   185,   215,   185,   164,     7,   188,
     838,   221,   880,   841,   185,   883,   191,   152,   218,   218,
     191,    21,    22,   216,   195,   193,   165,   184,   830,   197,
     837,    30,   200,    32,   645,    34,   838,   216,   173,   841,
     214,    40,   191,   218,   655,   656,   185,   215,    70,    71,
      72,    50,   191,   221,  1686,  1687,   143,    56,   198,    47,
     199,   672,   673,   674,   675,   152,   677,  1458,   870,   218,
     220,   682,  1103,   191,   685,   173,  1101,   217,   191,    67,
     690,    80,   198,  1104,   191,  1142,   173,   191,   191,   191,
     637,   701,   114,   115,   116,   117,   173,   925,   173,   709,
     218,   217,   930,   102,   103,   218,  1413,   165,   173,   916,
     191,   218,   173,   723,   218,   218,   218,  1508,   118,   119,
     173,   923,   924,   925,   191,   173,   126,   185,   930,   129,
     130,   131,   934,   191,   177,   216,   136,   939,   940,   150,
     942,   199,   944,    57,   946,   947,   198,   191,    57,   216,
     978,   979,   762,   165,    63,   165,   177,     5,     6,  1281,
       8,   708,   173,   174,   175,   217,   165,  1461,    21,    22,
     208,   718,   216,   185,   721,   185,   978,   979,   220,   191,
     165,   191,   165,   144,    12,    57,   185,   199,    36,   199,
    1484,    63,   203,   204,  1649,    23,    24,   808,  1209,   165,
     185,   165,   185,   214,  1215,   204,   191,  1228,   191,   195,
     165,  1602,   165,   199,   214,   215,   215,   764,   173,   185,
     830,   185,   165,    57,   835,   191,   198,   191,   838,    63,
     185,   841,   185,   199,   173,   199,   191,  1507,   191,   165,
    1042,   106,   185,  1264,  1265,   217,   199,   173,   191,   198,
     195,  1373,   164,   165,   199,   802,   199,   192,  1126,   185,
     870,  1089,    66,   116,   117,   118,   119,   195,   217,  1280,
      75,   199,   184,   126,    79,   128,   129,   130,   131,   885,
     195,   194,    79,   136,   199,   138,   139,  1089,    93,    94,
    1118,   173,   165,    98,    99,   100,   101,    94,   173,  1101,
     173,   195,    99,    57,   101,   199,   917,   158,   855,    63,
      35,   922,   185,   923,   924,   925,  1118,  1624,  1380,    35,
     930,  1380,   173,   158,   934,   158,   158,   158,   217,   939,
     940,   195,   942,    57,   944,   199,   946,   947,   173,    63,
     173,   173,   173,   184,   185,  1691,   187,   200,   201,   202,
     203,   204,  1380,   191,   184,   194,   194,   187,   185,   197,
     190,   214,   215,   217,   191,    43,   195,   217,   978,   979,
     199,   108,   109,   110,   111,   112,   113,   114,   115,   926,
    1726,  1402,   185,   177,   178,   185,  1680,  1681,   191,  1410,
     127,   191,  1514,   217,  1688,    21,    22,   185,  1692,  1693,
     137,   217,   217,   191,   219,   220,   198,  1209,  1210,   185,
     147,   148,   149,  1215,   961,   191,   198,   185,   177,   178,
     179,   180,   969,   191,   194,    96,    97,   197,   195,   198,
    1724,    21,    22,   144,   145,   146,   198,   198,  1732,  1733,
     173,   174,   175,   198,  1622,   978,   979,    21,    22,   198,
    1606,   177,   178,   179,   198,   198,  1067,   177,   178,   179,
    1471,   184,   185,   186,   198,   173,  1554,   626,   627,   628,
     173,  1559,    10,    11,    12,   173,   216,    22,  1280,  1089,
     173,   216,  1093,   173,   216,   173,   173,   215,   198,  1783,
     173,  1101,   118,   119,   198,   217,   198,   218,   198,   217,
     126,   198,   128,   129,   130,   131,  1374,   198,  1118,   198,
     136,   217,   217,   198,   198,    43,   198,   198,   217,  1675,
    1676,   217,  1677,   217,   198,   217,   116,   117,   118,   119,
     120,   217,  1360,   123,   124,   125,   126,   217,   128,   129,
     130,   131,  1370,   220,   118,   119,   136,   215,   138,   139,
     217,   199,   126,   218,   128,   129,   130,   131,  1360,   173,
    1388,  1717,   136,  1718,  1719,   216,   194,   173,  1370,   166,
      10,   198,    37,  1380,   200,   201,   202,   203,   204,  1190,
      66,     8,   217,   198,   184,   198,  1388,   198,   214,   215,
     191,    13,   216,   191,  1605,   217,   191,   218,  1753,  1209,
    1210,   173,  1470,   173,    33,  1215,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   132,   199,   173,  1677,    43,
     191,   217,   191,    14,   214,   215,   173,   192,   202,   203,
     204,    60,    61,   194,   166,   406,  1504,   173,   220,    67,
     214,   215,  1510,   173,   415,  1473,   198,   191,   191,  1677,
     218,   184,   218,   217,   425,   217,   198,     1,   218,  1718,
    1719,   218,   217,  1674,   435,   173,   217,   217,   217,  1471,
    1280,  1473,   199,   199,   173,   108,   109,   110,   111,   112,
     113,   114,   115,   217,   199,   192,   217,   458,   217,  1236,
    1718,  1719,   217,   173,  1753,   124,   192,   218,   127,   128,
     218,   173,   218,   217,   137,   218,   218,  1254,   137,   218,
     173,   217,   216,  1260,   147,   148,   149,   217,   489,   490,
    1267,   216,  1269,   216,   173,  1753,   217,   173,   173,   500,
    1598,    21,    22,   217,   198,   217,   165,   173,   217,   173,
     511,   512,   513,   514,   515,   516,   217,    43,    33,   218,
    1360,  1362,   173,    70,   216,  1302,   185,   217,  1586,   217,
    1370,   181,    33,   192,   193,   218,   173,  1314,   197,  1380,
     173,   200,  1319,   216,   173,   216,   173,   185,  1388,   217,
     217,   552,   199,   218,  1586,   217,   215,   217,   199,    60,
      61,   217,   221,   218,   217,   199,   177,    53,   217,   217,
     184,   217,   217,  1605,   217,  1673,   218,   184,   218,   218,
     581,   216,   218,   184,   216,   191,   184,   218,   218,   742,
     218,  1368,   218,   218,   218,   217,   116,   117,   118,   119,
     120,   216,   218,   123,   124,   125,   126,   216,   128,   129,
     130,   131,   216,   218,    86,     1,   136,   147,   138,   139,
      89,   622,  1599,   124,  1600,  1723,   220,   128,  1141,  1600,
    1600,  1471,   633,  1473,   690,  1600,   637,  1600,     1,   640,
    1144,   642,  1674,  1598,  1289,  1468,   647,  1521,  1425,   650,
    1391,  1193,  1394,   654,  1522,    58,   459,   658,  1693,  1522,
     471,   471,  1552,  1250,    -1,   735,    -1,    -1,    -1,    -1,
      -1,  1512,    -1,  1450,    -1,    33,    -1,  1518,   198,   199,
     200,   201,   202,   203,   204,   686,    -1,   300,    -1,    -1,
      -1,    -1,   193,    -1,   214,   215,   197,    -1,   199,   200,
      -1,    -1,    60,    61,    -1,    -1,    -1,    -1,    -1,   710,
     711,    -1,    -1,   714,   215,   716,    -1,    -1,    -1,    -1,
     221,    -1,    -1,    -1,    -1,   726,   727,   728,   729,   730,
     731,    -1,   733,    -1,   735,    -1,   898,   899,   900,   901,
     902,   903,   904,   905,   906,   907,  1586,   909,   910,   911,
     912,   913,   914,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1605,   124,   768,   769,    -1,
     128,   772,   773,   774,   775,    -1,   777,    -1,   779,   780,
     781,   782,   783,   784,   785,   786,   787,   788,   789,   790,
     791,   792,   793,   794,   795,   796,   797,   798,    -1,   800,
      -1,    -1,    -1,    -1,   805,    -1,    -1,   165,    -1,    -1,
    1650,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   651,
     652,   653,    21,    22,    -1,    -1,    -1,   185,    -1,    -1,
      -1,    -1,    -1,   191,  1674,   193,    -1,  1678,   670,   197,
      -1,   199,   200,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1628,   684,    -1,   855,    -1,    -1,   215,    33,   860,
      -1,    -1,    -1,   221,    -1,    -1,  1706,    -1,    -1,    -1,
     871,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1721,    -1,    -1,    -1,    -1,    60,    61,    -1,  1665,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   897,   898,   899,   900,
     901,   902,   903,   904,   905,   906,   907,   908,   909,   910,
     911,   912,   913,   914,  1755,    -1,  1757,   116,   117,   118,
     119,   120,    -1,    -1,   123,    -1,   927,   126,   929,   128,
     129,   130,   131,    -1,    -1,    -1,  1713,   136,  1779,   138,
     139,    -1,    -1,    33,    -1,    -1,    -1,    -1,    -1,   124,
      -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,   959,   960,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      60,    61,   973,    -1,   975,    -1,   808,    -1,    -1,    -1,
     981,    -1,    -1,    -1,   985,    -1,    -1,    -1,    -1,   990,
      -1,   992,    -1,   994,    -1,   996,    -1,    -1,    -1,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,   193,    -1,
      -1,    -1,   197,    -1,   199,   200,   858,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   124,    -1,    -1,    -1,   128,    -1,
     215,   873,    -1,    -1,    -1,    -1,   221,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1055,    -1,    -1,    -1,  1059,    -1,
      -1,    -1,    -1,   895,    -1,    -1,    -1,  1068,  1069,  1070,
    1071,  1072,  1073,  1074,  1075,  1076,  1077,  1078,  1079,  1080,
    1081,  1082,  1083,  1084,    13,    -1,    -1,    -1,    -1,    -1,
      19,    -1,    -1,    21,    22,    -1,    25,    -1,    -1,    -1,
      -1,    -1,    31,   193,    -1,    -1,    -1,   197,  1109,   199,
     200,    -1,    41,    -1,  1115,    -1,    -1,    -1,    -1,  1120,
      49,    -1,    -1,    -1,  1125,   215,   958,    -1,    -1,    -1,
      -1,   221,    -1,    -1,    -1,    64,    -1,    -1,    -1,  1140,
      -1,    -1,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,    -1,    -1,    -1,    -1,    -1,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,   137,
     138,   139,   140,   141,   143,    -1,   144,   145,   146,   147,
     148,   149,   150,    -1,    -1,    -1,    -1,   156,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1067,    -1,   165,    -1,    -1,
    1241,    -1,    -1,    -1,   173,    -1,    -1,    -1,  1249,  1250,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   185,    -1,    -1,
      -1,    -1,    -1,    -1,   192,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,  1278,  1279,    -1,
      -1,    -1,    -1,    -1,    -1,  1286,   214,   215,    -1,    -1,
     219,   219,   220,    -1,    -1,    -1,    -1,    -1,  1299,    -1,
    1301,    -1,  1303,  1135,    -1,    -1,    -1,    -1,  1309,    -1,
      -1,    -1,  1313,    -1,    -1,    -1,  1148,  1149,  1150,    -1,
      -1,  1153,    -1,  1155,    -1,  1157,    -1,  1159,    -1,  1161,
      -1,  1163,    -1,  1165,    -1,  1167,    -1,  1169,    -1,  1171,
      -1,  1173,    -1,    -1,  1176,    -1,  1178,    -1,  1180,    -1,
    1182,    -1,  1184,    -1,  1186,    -1,    -1,    13,    -1,    -1,
      -1,    -1,    -1,    19,    33,    -1,    -1,    -1,    -1,    25,
    1371,    -1,    -1,    -1,    -1,    31,  1377,    -1,    -1,  1380,
      -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,
      -1,    60,    61,    49,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,  1413,    -1,    -1,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,    -1,    -1,
      -1,  1452,  1453,  1454,    -1,   124,    -1,    -1,    -1,   128,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1479,    -1,
    1481,    -1,    33,    -1,    -1,    -1,  1487,   143,    -1,    -1,
      -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     156,    -1,  1503,    12,    -1,    -1,    -1,    -1,    -1,    60,
      61,    -1,    21,    22,    -1,    -1,    -1,   173,    60,    61,
      -1,    -1,    -1,    -1,   193,    -1,  1527,    -1,   197,  1530,
     199,   200,    -1,    -1,    -1,    -1,    -1,  1538,  1539,  1540,
      -1,    -1,    -1,    -1,  1545,    -1,   215,  1548,    -1,    -1,
    1551,    -1,   221,  1554,  1555,    -1,    -1,    -1,  1559,  1560,
      -1,  1562,  1563,   219,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   124,    -1,    -1,  1577,   128,    -1,    -1,
      -1,    -1,   124,    -1,    -1,    -1,   128,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1600,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,  1624,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,   193,    -1,    -1,  1646,   197,    -1,   199,   200,
      -1,   193,    -1,  1654,    -1,   197,    -1,   199,   200,    -1,
      -1,    -1,    -1,    -1,   215,  1666,    -1,    -1,    -1,    -1,
     221,    -1,    -1,   215,    -1,  1507,  1677,    -1,    -1,   221,
      -1,    -1,  1683,  1684,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,  1703,  1704,    -1,   214,   215,    -1,    -1,    -1,
      -1,    -1,    -1,  1714,    -1,    -1,    -1,  1718,  1719,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1741,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,     5,
       6,     7,  1753,     9,    10,    11,    -1,    13,    -1,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    38,    39,    40,    -1,    42,    -1,    44,    45,
      -1,    -1,    48,    -1,    50,    51,    52,    -1,    54,    55,
      -1,    -1,    58,    59,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   133,   134,   135,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,  1721,
      -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,   155,
      -1,   157,    -1,   159,   160,   161,   162,    -1,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,  1755,    -1,  1757,    -1,   183,   184,   185,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   200,   201,   202,  1779,   204,    -1,
      -1,   207,   208,    -1,    -1,     5,     6,    -1,    -1,   215,
      -1,   217,    -1,   219,   220,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    53,    -1,    55,    -1,    -1,    -1,    -1,
      60,    61,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
      -1,    -1,    -1,    -1,   124,    -1,    -1,    -1,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,   152,   153,   154,   155,    -1,   157,   158,   159,
     160,   161,   162,    -1,    -1,    -1,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,    -1,    -1,    -1,
      -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   193,    -1,    -1,    -1,   197,    -1,    -1,
     200,   201,   202,    -1,   204,    -1,    -1,   207,   208,    -1,
      -1,    -1,     5,     6,    -1,   215,    -1,   217,    -1,   219,
     220,   221,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      33,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    60,    61,    -1,
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
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     193,    -1,    -1,    -1,   197,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,    -1,     5,
       6,    -1,   215,    -1,   217,    -1,   219,   220,   221,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    33,    -1,    -1,
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
      -1,   157,    -1,   159,   160,   161,   162,    -1,    -1,    -1,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   197,    -1,    -1,   200,   201,   202,    -1,   204,    -1,
      -1,   207,   208,    -1,    -1,    -1,     5,     6,    -1,   215,
      -1,   217,    -1,   219,   220,   221,    15,    16,    17,    18,
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
      -1,    -1,    -1,    -1,   133,   134,   135,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,
     133,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,   152,
     153,   154,   155,    -1,   157,    -1,   159,   160,   161,   162,
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,     5,     6,
      -1,    -1,   215,    -1,   217,    -1,   219,   220,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    52,    -1,    -1,    55,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,    -1,
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
      65,    -1,    -1,    68,    69,    70,    71,    72,    73,    -1,
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
     215,    -1,   217,   218,   219,   220,    15,    16,    17,    18,
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
      -1,    -1,    -1,    -1,   133,   134,   135,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,    -1,
     159,   160,   161,   162,    -1,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,    -1,    -1,   197,    -1,   199,   200,    -1,
      -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,   208,
      -1,    -1,    -1,   215,     5,     6,   215,    -1,   217,   221,
     219,   220,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    33,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    49,    -1,
      51,    -1,    -1,    -1,    55,    -1,    60,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
     124,    -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,   152,   153,   154,   155,    -1,   157,    -1,   159,   160,
     161,   162,    -1,    -1,    -1,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,    -1,    -1,    -1,    -1,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,    -1,    -1,   197,    -1,   199,   200,    -1,    -1,   200,
     201,   202,    -1,   204,    -1,    -1,   207,   208,    -1,    -1,
      -1,   215,     5,     6,   215,   216,   217,   221,   219,   220,
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
     153,   154,   155,    -1,   157,   158,   159,   160,   161,   162,
      -1,    -1,    -1,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,
     183,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,
      -1,   197,    -1,   199,   200,    -1,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,    -1,   215,
       5,     6,   215,    -1,   217,   221,   219,   220,    13,    -1,
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
     155,    -1,   157,    -1,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,    -1,   197,
      -1,   199,   200,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,    -1,   215,     5,     6,
     215,   216,   217,   221,   219,   220,    13,    -1,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    49,    -1,    51,    -1,    -1,    -1,    55,    -1,
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
     217,    -1,   219,   220,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    70,
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
      -1,    -1,   151,   152,   153,   154,   155,    -1,   157,   158,
     159,   160,   161,   162,    -1,    -1,    -1,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   200,   201,   202,    -1,   204,    -1,    -1,   207,   208,
      -1,    -1,     5,     6,    -1,    -1,   215,    -1,   217,   218,
     219,   220,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    33,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,
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
      45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,
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
     155,    -1,   157,    -1,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,    -1,   197,
      -1,   199,   200,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,    -1,   215,     5,     6,
     215,    -1,   217,   221,   219,   220,    13,    -1,    15,    16,
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
     151,   152,   153,   154,   155,    -1,   157,   158,   159,   160,
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
      55,    -1,    -1,    -1,    -1,    -1,    61,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,   198,    -1,   200,   201,   202,
      -1,   204,    -1,    -1,   207,   208,    -1,    -1,     5,     6,
      -1,    -1,   215,    -1,   217,    -1,   219,   220,    15,    16,
      17,    18,    19,    -1,    -1,    22,    -1,    -1,    25,    -1,
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
      -1,    -1,    -1,   198,    -1,   200,   201,   202,    -1,   204,
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
      -1,    -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,
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
     157,    -1,   159,   160,   161,   162,    -1,    -1,    -1,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
      -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   200,   201,   202,    -1,   204,    -1,    -1,
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
       5,     6,    -1,    -1,   215,    -1,   217,    -1,   219,   220,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    27,    -1,    -1,    33,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,
      55,    -1,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,   119,    -1,   124,    -1,    10,    -1,
     128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,   152,   153,   154,
     155,    -1,   157,    -1,   159,   160,   161,   162,    -1,    -1,
      -1,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,    -1,   197,
      -1,   199,   200,    -1,    -1,   200,   201,   202,    -1,   204,
      -1,    -1,   207,   208,    -1,    -1,    -1,   215,    19,    -1,
     215,    -1,   217,   221,   219,   220,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,   137,   138,   139,   140,   141,
      -1,    -1,   144,   145,   146,   147,   148,   149,   150,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     192,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    -1,   214,   215,    -1,    31,    -1,   219,   220,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    49,    -1,    -1,    -1,   158,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,   173,    -1,    -1,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,    41,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,
     156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,   173,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   219,    -1,   221,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   165,    -1,    -1,    19,    -1,    -1,    -1,    -1,
     173,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,   185,    -1,    -1,    -1,    -1,    41,   191,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      64,    -1,    -1,    -1,    -1,    -1,   219,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
      41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    -1,
      -1,    -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,   173,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   219,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    -1,   143,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   156,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,    -1,    -1,   219,   136,
     137,   138,   139,   140,   141,    -1,    -1,   144,   145,   146,
     147,   148,   149,   150,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,   192,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,    -1,   219,   220,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,    -1,    -1,    -1,   219,   220,    -1,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    21,
      22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,    -1,   193,    -1,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    21,    22,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,   193,    -1,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,
      -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,
     218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    21,
      22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,    -1,   193,    -1,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    21,    22,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,   193,    -1,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,
      -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,
     218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    21,
      22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,    -1,   193,    -1,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    21,    22,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,   193,    -1,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,
      -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,
     218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    21,
      22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,    -1,   193,    -1,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    21,    22,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    -1,   150,    -1,    -1,    -1,    -1,   193,    -1,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,
      -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,
     218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      21,    22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,    -1,    -1,   218,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,    -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,
      -1,    -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    21,
      22,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,    -1,    -1,    -1,   150,    -1,    -1,
      -1,    -1,   193,    -1,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   214,   215,   216,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     193,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,   216,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    21,    22,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,
      -1,    -1,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   214,   215,   216,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,   216,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,    -1,    -1,    -1,   150,    -1,    -1,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,    21,    22,    -1,    -1,   136,    -1,
     138,   139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,
      -1,    38,   150,    -1,    -1,    -1,    -1,   193,    -1,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,   214,   215,
     216,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   214,   215,   216,    -1,
      -1,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,    -1,
      -1,   128,   129,   130,    -1,    -1,    -1,    -1,    -1,    -1,
     137,   138,   139,   140,   141,    -1,    -1,   144,   145,   146,
     147,   148,   149,   150,    -1,    -1,    -1,    -1,    -1,    21,
      22,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,
      -1,   136,    -1,   138,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,    -1,    -1,    -1,   150,   193,    -1,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   163,    -1,
     207,   208,    -1,    -1,    -1,    -1,    -1,   214,   215,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
      -1,    -1,    21,    22,   116,   117,   118,   119,   120,   214,
     215,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   165,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   185,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,   214,   215,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,   142,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    21,    22,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
     142,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    21,
      22,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,   214,   215,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    21,    22,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,    -1,    -1,   150,    21,
      22,    -1,    -1,    -1,   193,   194,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,   214,   215,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    21,    22,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    -1,   185,    -1,   150,    21,
      22,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,   214,   215,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,    -1,    -1,
      -1,   150,    21,    22,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
      -1,    -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,
      -1,    -1,   144,   145,   146,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,   214,   215,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,    -1,    -1,    -1,    -1,   136,    -1,   138,
     139,    -1,    -1,    -1,    -1,   144,    -1,   146,    21,    22,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,
     136,    -1,   138,   139,    21,    22,    -1,    -1,   144,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,   214,   215,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,    -1,
      -1,    -1,    -1,   136,    -1,   138,   139,    -1,    -1,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,    -1,    -1,    -1,    -1,   136,
      -1,   138,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,
      -1,    -1,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,    -1,    -1,    -1,    -1,    -1,    19,    -1,
     197,   198,   199,   200,   201,   202,   203,   204,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,   158,    35,    98,    99,
     100,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   158,    -1,   129,
     130,    -1,    -1,    71,    -1,    73,    -1,    75,    76,    77,
      78,    79,   173,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,   158,    -1,
      98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   214,   215,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   173
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   223,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   102,   103,   165,   185,   204,   215,   224,   228,   237,
     239,   240,   245,   252,   277,   282,   318,   400,   407,   411,
     422,   468,   473,   478,    19,    20,   173,   269,   270,   271,
     166,   246,   247,   150,   173,   174,   175,   204,   214,   241,
     242,   243,   163,   183,   287,   408,   173,   219,   226,    57,
      63,   403,   403,   403,   143,   173,   304,    34,    63,   107,
     136,   208,   217,   273,   274,   275,   276,   304,   252,     5,
       6,     8,    36,   419,    62,   398,   192,   191,   194,   191,
     203,   203,   242,   203,    22,    57,   203,   214,   244,   403,
     404,   406,   404,   398,   479,   469,   474,   173,   143,   238,
     275,   275,   275,   217,   144,   145,   146,   191,   216,   107,
     107,   107,   281,    57,    63,   409,    57,    63,   420,    57,
      63,   399,    15,    16,   166,   171,   173,   176,   217,   220,
     230,   270,   166,   247,   242,   242,   242,   173,   241,   241,
     173,   252,   164,   184,   288,   404,   252,    57,    63,   225,
     173,   173,   173,   173,   177,   236,   218,   271,   275,   275,
     275,   275,    57,    63,   283,   285,   173,   410,   423,   287,
     401,   177,   178,   179,   229,    15,    16,   166,   171,   173,
     220,   230,   267,   268,   220,   244,   405,   252,   208,   227,
     480,   470,   475,   177,   218,   286,   194,   287,   106,   417,
     418,   396,   160,   220,   272,   368,   177,   178,   179,   220,
     191,   218,   173,   192,    66,   194,   432,   287,   287,    35,
      71,    73,    75,    76,    77,    78,    79,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    93,    94,
      95,    98,    99,   100,   101,   118,   119,   173,   280,   284,
      75,    79,    93,    94,    98,    99,   100,   101,   427,   412,
     173,   424,   165,   288,   397,   271,   270,   220,   252,   152,
     173,   394,   395,   173,   267,    19,    25,    31,    41,    49,
      64,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   156,   219,   304,   426,   428,   429,
     433,   439,   467,    79,    94,    99,   101,   287,   471,   476,
      21,    22,    38,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   128,   129,   130,   137,   138,   139,   140,   141,   144,
     145,   146,   147,   148,   149,   150,   193,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   207,   208,   214,
     215,    35,    35,   217,   278,   287,   289,   287,   402,   194,
     416,   287,   421,   368,   216,   270,   217,    43,   191,   194,
     197,   393,   198,   198,   198,   217,   198,   198,   217,   432,
     198,   198,   198,   198,   198,   217,   304,    33,    60,    61,
     124,   128,   193,   197,   200,   215,   221,   438,   195,   481,
     384,   387,   173,   173,   173,   216,    22,   173,   216,   155,
     218,   368,   379,   380,   381,   126,   194,   279,   292,   414,
     173,   252,   413,   304,   374,   395,   216,     5,     6,    15,
      16,    17,    18,    19,    25,    27,    31,    39,    45,    48,
      51,    55,    65,    68,    69,    80,   102,   103,   104,   118,
     119,   151,   152,   153,   154,   155,   157,   159,   160,   161,
     162,   166,   167,   168,   169,   170,   171,   172,   174,   175,
     176,   183,   200,   201,   202,   207,   208,   215,   217,   219,
     220,   235,   237,   298,   304,   309,   323,   330,   333,   336,
     341,   344,   348,   349,   351,   356,   359,   360,   367,   426,
     483,   498,   509,   513,   526,   529,   173,   173,   439,   127,
     137,   192,   392,   440,   445,   447,   360,   449,   443,   173,
     198,   451,   453,   455,   457,   459,   461,   463,   465,   360,
     198,   217,   295,    33,   197,    33,   197,   215,   221,   216,
     360,   215,   221,   439,   431,   173,   191,   252,   382,   436,
     467,   472,   173,   385,   436,   477,   173,   108,   109,   110,
     111,   112,   113,   114,   115,   137,   147,   148,   149,   108,
     109,   110,   111,   112,   113,   114,   115,   127,   137,   147,
     148,   149,   217,     7,    50,   317,   252,   191,   252,   218,
     467,   467,     1,     9,    10,    11,    13,    26,    28,    29,
      38,    40,    42,    44,    52,    54,    58,    59,    65,   104,
     105,   133,   134,   135,   174,   248,   249,   252,   253,   256,
     257,   259,   261,   262,   263,   264,   288,   290,   291,   293,
     298,   303,   305,   310,   311,   312,   313,   314,   315,   316,
     318,   322,   345,   347,   360,   402,   192,   252,   288,    40,
     215,   252,   277,   288,   376,   198,   198,   198,   306,   428,
     483,   217,   304,   198,     5,   102,   103,   198,   217,   198,
     217,   217,   198,   198,   217,   198,   217,   198,   217,   198,
     198,   217,   198,   198,   360,   360,   217,   217,   217,   217,
     217,   217,    13,   439,    13,   439,    13,   360,   508,   524,
     198,   198,   234,    13,   296,   508,   525,   360,   360,   360,
     360,   360,    13,    49,   294,   334,   360,   158,   173,   334,
     484,   486,   220,   173,   217,   277,    21,    22,   116,   117,
     118,   119,   120,   123,   124,   125,   126,   128,   129,   130,
     131,   136,   138,   139,   144,   145,   146,   150,   193,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,   214,
     215,   218,   217,    43,   252,   392,   303,   345,   360,   467,
     467,   437,   467,   218,   467,   467,   218,   199,   434,   467,
     278,   467,   278,   467,   278,   382,   383,   385,   386,   218,
     442,   294,   216,   216,   360,   173,   252,   482,   194,   436,
     288,   194,   436,   288,   360,   152,   173,   389,   390,   425,
     381,   381,   381,   360,   260,   127,   303,   334,   360,   289,
      61,   360,   266,   360,   173,   252,   166,    58,   360,   289,
     198,   127,   303,   360,   220,   289,   336,   340,   340,   340,
     360,   252,   252,   360,    10,    37,   336,   342,   252,   252,
     252,   252,   252,    66,   319,   132,   252,   108,   109,   110,
     111,   112,   113,   114,   115,   121,   122,   127,   137,   140,
     141,   147,   148,   149,   192,   342,   415,   360,   375,   276,
       8,   368,   373,   499,   501,   307,   217,   304,   198,   217,
     331,   198,   198,   198,   520,   334,   439,   296,   360,   324,
     326,   360,   328,   360,   522,   334,   505,   510,   334,   503,
     439,   360,   360,   360,   360,   360,   360,   425,    53,   200,
     215,   217,   360,   484,   487,   491,   507,   512,   425,   217,
     487,   512,   425,   142,   184,   185,   186,   492,   299,   301,
     179,   180,   229,   425,   184,   191,   528,   425,    13,   216,
     191,   528,   217,   127,   137,   192,   388,   528,   191,   528,
     218,   152,   157,   198,   304,   350,    70,   215,   218,   334,
     486,     4,   160,   339,    19,   158,   173,   426,    19,   158,
     173,   426,   360,   360,   360,   360,   360,   360,   173,   360,
     158,   173,   360,   360,   360,   426,   360,   360,   360,   360,
     360,   360,    22,   360,   360,   360,   360,   360,   360,   360,
     360,   360,   360,   360,   129,   130,   158,   173,   214,   215,
     357,   426,   360,   218,   334,   173,   303,   360,   108,   109,
     110,   111,   112,   113,   114,   115,   121,   122,   127,   140,
     141,   147,   148,   149,   192,   252,   199,   199,   199,   436,
     199,   199,   173,   430,   199,   279,   199,   279,   199,   279,
     199,   436,   199,   436,   297,   467,   191,   528,   216,   192,
     252,   288,   467,   467,   218,   217,    43,   191,   194,   197,
     388,   289,   425,   303,   334,   191,    14,   360,   173,   289,
     192,   194,   166,   439,   303,   360,   220,   277,   289,   289,
     258,   287,   343,   160,   217,   321,   395,   340,   133,   134,
     135,   290,   346,   360,   346,   360,   346,   360,   346,   360,
     346,   360,   346,   360,   346,   360,   346,   360,   346,   360,
     346,   360,   346,   360,   360,   346,   360,   346,   360,   346,
     360,   346,   360,   346,   360,   346,   360,   288,   252,   173,
     216,    57,    63,   371,    67,   372,   252,   439,   439,   467,
      70,   334,   486,   497,   198,   360,   173,   360,   467,   514,
     516,   518,   439,   528,   199,   436,   218,   218,   439,   439,
     218,   439,   218,   439,   528,   439,   383,   528,   386,   199,
     218,   218,   218,   218,   218,   218,    20,   340,   215,   360,
     218,   142,   191,   185,   491,   188,   189,   216,   495,   191,
     185,   188,   216,   494,    20,   218,   491,   184,   187,   493,
      20,   360,   184,   508,   297,   297,   360,    20,   508,    20,
     425,   360,   360,   360,   360,   218,   158,   173,   217,   217,
     352,   354,   173,   218,   486,   484,   191,   218,   218,   217,
     127,   137,   173,   192,   197,   337,   338,   278,   198,   217,
     198,   217,   217,   217,   216,    19,   158,   173,   426,   194,
     158,   173,   360,   217,   217,   158,   173,   360,     1,   217,
     216,   218,   252,   360,   360,   360,   360,   360,   360,   360,
     360,   360,   360,   360,   360,   360,   360,   360,   360,   360,
     441,   446,   448,   467,   450,   444,   191,   199,   252,   452,
     199,   456,   199,   460,   199,   464,   382,   466,   385,   199,
     436,   218,   360,   360,   173,   173,   467,   360,    20,   289,
     192,   265,   199,   339,    12,    23,    24,   250,   251,   360,
     292,   277,   173,   320,   320,   340,   340,   340,   192,   252,
      47,   372,    46,   106,   369,   199,   199,   199,   486,   218,
     218,   218,   173,   218,   185,   199,   218,   199,   439,   383,
     386,   199,   218,   217,   439,   199,   199,   199,   199,   218,
     199,   199,   218,   199,   339,   217,   334,   487,   491,   360,
     484,   495,   216,   360,   507,   216,   334,   487,   184,   187,
     190,   496,   216,   334,   199,   199,   194,   232,   334,   334,
      20,   218,   217,   137,   388,   360,   360,   439,   278,   218,
     216,   215,   338,   173,   173,   217,   173,   173,   191,   216,
     279,   361,   360,   363,   360,   218,   334,   360,   198,   217,
     360,   217,   216,   360,   215,   218,   334,   217,   216,   358,
     218,   334,   173,   435,   173,   454,   458,   462,   295,   467,
     252,   218,    43,   388,   334,   467,   360,   339,   278,   289,
     360,    12,   254,   288,   339,   191,   216,   218,   467,    33,
     370,   369,   371,   500,   502,   308,   218,   199,   436,   173,
     217,   332,   199,   199,   199,   521,   296,   199,   325,   327,
     329,   523,   506,   511,   504,   217,   218,   334,   185,   491,
     495,   185,   491,   216,   185,   300,   302,   233,   181,   185,
     185,   334,   137,   388,   360,   360,   360,   218,   218,   199,
     279,   218,   484,   218,   173,   337,   216,   142,   289,   335,
     439,   218,   467,   218,   218,   218,   365,   360,   360,   218,
     484,   218,   360,   218,   173,   360,   289,   342,   279,   289,
     255,   252,   278,   173,   216,   194,   393,   252,   377,   370,
     389,   390,   391,   217,   217,   360,   173,   199,   360,   515,
     517,   519,   217,   218,   217,   360,   360,   360,   217,    70,
     497,   217,   217,   218,   360,   218,   360,   495,   360,   496,
     508,   360,   295,   231,   508,   360,   185,   360,   360,   218,
     353,   199,   216,   218,   127,   360,   199,   199,   467,   218,
     218,   216,   218,   335,   251,    26,   105,   256,   310,   311,
     312,   314,   360,   279,   194,   393,   439,   392,   284,   378,
     497,   497,   218,   199,   217,   218,   217,   217,   217,   294,
     296,   334,   497,   497,   218,   185,   527,   527,   527,   177,
     527,   527,   360,   137,   388,   350,   355,   218,   360,   362,
     364,   199,   218,   127,   127,   360,   289,   439,   392,   392,
     303,   360,   252,   284,   217,   484,   488,   489,   490,   490,
     360,   360,   497,   497,   484,   485,   218,   218,   528,   490,
     485,    53,   216,   184,   184,   184,   216,   527,   360,   360,
     350,   366,   360,   392,   303,   360,   303,   360,   252,   289,
     484,   191,   528,   218,   218,   218,   218,   490,   490,   218,
     218,   218,   218,   360,   216,   216,   184,   216,   303,   360,
     252,   252,   218,   217,   218,   218,   252,   484,   218
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   222,   223,   223,   223,   223,   223,   223,   223,   223,
     223,   223,   223,   223,   223,   223,   223,   224,   225,   225,
     225,   226,   226,   227,   227,   228,   229,   229,   229,   229,
     230,   230,   231,   231,   232,   233,   232,   234,   234,   234,
     235,   236,   236,   238,   237,   239,   240,   241,   241,   241,
     242,   242,   242,   242,   242,   242,   242,   243,   243,   244,
     244,   245,   246,   246,   247,   247,   248,   249,   249,   250,
     250,   251,   251,   251,   252,   252,   253,   253,   254,   255,
     254,   256,   256,   256,   256,   256,   257,   258,   257,   260,
     259,   261,   262,   263,   265,   264,   266,   264,   267,   267,
     267,   267,   267,   267,   267,   268,   268,   269,   269,   269,
     270,   270,   270,   270,   270,   270,   270,   270,   270,   271,
     271,   272,   272,   272,   273,   273,   273,   273,   274,   274,
     275,   275,   275,   275,   275,   275,   275,   276,   276,   277,
     277,   278,   278,   278,   279,   279,   279,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   281,   281,   282,   283,
     283,   283,   284,   286,   285,   287,   287,   288,   288,   289,
     289,   290,   290,   290,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   292,   292,   292,   293,   294,
     294,   295,   295,   296,   296,   297,   297,   299,   300,   298,
     301,   302,   298,   303,   303,   303,   303,   303,   304,   304,
     304,   305,   305,   307,   308,   306,   306,   309,   309,   309,
     309,   309,   309,   310,   311,   312,   312,   312,   313,   313,
     313,   314,   314,   315,   315,   315,   316,   317,   317,   317,
     318,   318,   319,   319,   320,   320,   321,   321,   321,   321,
     321,   321,   321,   321,   322,   322,   324,   325,   323,   326,
     327,   323,   328,   329,   323,   331,   332,   330,   333,   333,
     333,   333,   333,   333,   334,   334,   335,   335,   335,   336,
     336,   336,   337,   337,   337,   337,   337,   338,   338,   339,
     339,   339,   340,   340,   341,   343,   342,   344,   344,   344,
     344,   344,   344,   344,   345,   345,   345,   345,   345,   345,
     345,   345,   345,   345,   345,   345,   345,   345,   345,   345,
     345,   345,   345,   346,   346,   346,   346,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   347,   347,   347,   347,
     347,   347,   347,   347,   348,   348,   349,   349,   350,   350,
     351,   352,   353,   351,   354,   355,   351,   356,   356,   356,
     356,   356,   356,   356,   357,   358,   356,   359,   359,   359,
     359,   359,   359,   359,   360,   360,   360,   360,   360,   360,
     360,   360,   360,   360,   360,   360,   360,   360,   360,   360,
     360,   360,   360,   360,   360,   360,   360,   360,   360,   360,
     360,   360,   360,   360,   360,   360,   360,   360,   360,   360,
     360,   360,   360,   360,   360,   360,   360,   360,   360,   360,
     360,   360,   360,   360,   360,   360,   360,   360,   360,   360,
     360,   361,   362,   360,   360,   360,   360,   363,   364,   360,
     360,   360,   365,   366,   360,   360,   360,   360,   360,   360,
     360,   360,   360,   360,   360,   360,   360,   360,   360,   367,
     367,   367,   367,   367,   367,   367,   367,   367,   367,   367,
     367,   367,   367,   367,   367,   368,   368,   368,   369,   369,
     369,   370,   370,   371,   371,   371,   372,   372,   373,   374,
     374,   375,   374,   376,   374,   377,   374,   378,   374,   374,
     379,   380,   380,   381,   381,   381,   381,   381,   382,   382,
     383,   383,   384,   384,   384,   385,   386,   386,   387,   387,
     387,   388,   388,   389,   389,   389,   390,   390,   391,   391,
     392,   392,   392,   393,   393,   394,   394,   394,   394,   394,
     395,   395,   395,   395,   395,   396,   396,   397,   396,   398,
     398,   399,   399,   399,   400,   401,   400,   402,   402,   402,
     402,   403,   403,   403,   405,   404,   406,   406,   407,   408,
     407,   409,   409,   409,   410,   412,   413,   411,   414,   415,
     411,   416,   416,   417,   417,   418,   419,   419,   419,   419,
     420,   420,   420,   421,   421,   423,   424,   422,   425,   425,
     425,   425,   425,   426,   426,   426,   426,   426,   426,   426,
     426,   426,   426,   426,   426,   426,   426,   426,   426,   426,
     426,   426,   426,   426,   426,   426,   426,   426,   426,   426,
     427,   427,   427,   427,   427,   427,   427,   427,   428,   429,
     429,   429,   430,   430,   430,   431,   431,   431,   431,   432,
     432,   432,   432,   432,   433,   434,   435,   433,   436,   436,
     437,   437,   438,   438,   439,   439,   439,   439,   439,   439,
     440,   441,   439,   439,   439,   442,   439,   439,   439,   439,
     439,   439,   439,   439,   439,   439,   439,   439,   439,   443,
     444,   439,   439,   445,   446,   439,   447,   448,   439,   449,
     450,   439,   439,   451,   452,   439,   453,   454,   439,   439,
     455,   456,   439,   457,   458,   439,   439,   459,   460,   439,
     461,   462,   439,   463,   464,   439,   465,   466,   439,   467,
     467,   467,   469,   470,   471,   472,   468,   474,   475,   476,
     477,   473,   479,   480,   481,   482,   478,   483,   483,   483,
     483,   483,   484,   484,   484,   484,   484,   484,   484,   484,
     485,   485,   486,   487,   487,   488,   488,   489,   489,   490,
     490,   491,   491,   492,   492,   493,   493,   494,   494,   495,
     495,   495,   496,   496,   496,   497,   497,   498,   498,   498,
     498,   498,   498,   499,   500,   498,   501,   502,   498,   503,
     504,   498,   505,   506,   498,   507,   507,   507,   508,   508,
     509,   510,   511,   509,   512,   512,   513,   513,   513,   514,
     515,   513,   516,   517,   513,   518,   519,   513,   513,   520,
     521,   513,   513,   522,   523,   513,   524,   524,   525,   525,
     526,   526,   526,   526,   526,   527,   527,   528,   528,   529,
     529,   529,   529,   529,   529
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
       1,     1,     1,     1,     1,     1,     0,     1,     4,     0,
       1,     1,     3,     0,     4,     1,     1,     1,     1,     3,
       7,     2,     2,     6,     1,     1,     1,     1,     2,     2,
       1,     1,     1,     1,     1,     1,     2,     2,     1,     1,
       1,     1,     2,     2,     2,     0,     2,     2,     3,     0,
       2,     0,     4,     0,     2,     1,     3,     0,     0,     7,
       0,     0,     7,     3,     2,     2,     2,     1,     1,     3,
       2,     2,     3,     0,     0,     5,     1,     2,     5,     5,
       5,     6,     2,     1,     1,     1,     2,     3,     2,     2,
       3,     2,     3,     2,     2,     3,     4,     1,     1,     0,
       1,     1,     1,     0,     1,     3,     9,     8,     8,     7,
       8,     7,     7,     6,     3,     3,     0,     0,     7,     0,
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
       5,     4,     3,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     2,     2,     2,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       2,     2,     2,     2,     4,     3,     4,     5,     4,     5,
       3,     4,     1,     1,     2,     4,     4,     7,     8,     3,
       5,     0,     0,     8,     3,     3,     3,     0,     0,     8,
       3,     4,     0,     0,     9,     4,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     2,     4,     1,     4,
       4,     4,     4,     4,     1,     6,     7,     6,     6,     7,
       7,     6,     7,     6,     6,     0,     4,     1,     0,     1,
       1,     0,     1,     0,     1,     1,     0,     1,     5,     0,
       2,     0,     7,     0,     4,     0,     9,     0,    10,     5,
       3,     3,     4,     1,     1,     3,     3,     3,     1,     3,
       1,     3,     0,     2,     3,     3,     1,     3,     0,     2,
       3,     1,     1,     1,     2,     3,     3,     5,     1,     1,
       1,     1,     1,     0,     1,     1,     4,     3,     3,     5,
       4,     6,     5,     5,     4,     0,     2,     0,     4,     0,
       1,     0,     1,     1,     6,     0,     6,     0,     2,     3,
       5,     0,     1,     1,     0,     5,     2,     3,     4,     0,
       4,     0,     1,     1,     1,     0,     0,     9,     0,     0,
      11,     0,     2,     0,     1,     3,     1,     1,     2,     2,
       0,     1,     1,     0,     3,     0,     0,     7,     1,     4,
       3,     3,     5,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       4,     4,     1,     3,     3,     0,     2,     3,     5,     0,
       2,     2,     2,     2,     4,     0,     0,     7,     1,     1,
       1,     3,     3,     4,     1,     1,     1,     1,     2,     3,
       0,     0,     6,     4,     3,     0,     7,     4,     2,     2,
       3,     2,     3,     2,     2,     3,     3,     3,     2,     0,
       0,     6,     2,     0,     0,     6,     0,     0,     6,     0,
       0,     6,     1,     0,     0,     6,     0,     0,     7,     1,
       0,     0,     6,     0,     0,     7,     1,     0,     0,     6,
       0,     0,     7,     0,     0,     6,     0,     0,     6,     1,
       3,     3,     0,     0,     0,     0,    10,     0,     0,     0,
       0,    10,     0,     0,     0,     0,    11,     1,     1,     1,
       1,     1,     3,     3,     5,     5,     6,     6,     8,     8,
       0,     1,     2,     1,     3,     3,     5,     1,     2,     1,
       0,     0,     2,     2,     1,     2,     1,     2,     1,     2,
       1,     1,     2,     1,     1,     0,     1,     5,     4,     6,
       7,     5,     7,     0,     0,    10,     0,     0,    10,     0,
       0,    10,     0,     0,     7,     1,     3,     3,     3,     1,
       5,     0,     0,    10,     1,     3,     3,     4,     4,     0,
       0,    11,     0,     0,    11,     0,     0,    10,     5,     0,
       0,     9,     5,     0,     0,    10,     1,     3,     1,     3,
       3,     3,     4,     7,     9,     0,     3,     0,     1,     9,
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
                yyextra->g_Program->options.push_back(opt);
            } else {
                das_yyerror(scanner,"option " + opt.name + " is not allowed here",
                    tokAt(scanner,(yylsp[0])), CompilationError::invalid_option);
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
                            tokAt(scanner,(yylsp[0])), CompilationError::invalid_annotation);
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
                            tokAt(scanner,(yylsp[-3])), CompilationError::invalid_annotation);
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
            delete (yyvsp[0].fa); (yyvsp[0].fa) = nullptr;
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
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
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
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        if ( !(yyvsp[0].fa)->annotation || !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
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
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
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

  case 256: /* optional_template: %empty  */
                                        { (yyval.b) = false; }
    break;

  case 257: /* optional_template: "template"  */
                                        { (yyval.b) = true; }
    break;

  case 258: /* global_function_declaration: optional_annotation_list "def" optional_template function_declaration  */
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

  case 259: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 260: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 261: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 262: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 263: /* $@8: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 264: /* function_declaration: optional_public_or_private_function $@8 function_declaration_header expression_block  */
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

  case 269: /* expression_block: open_block expressions close_block  */
                                                                  {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
    }
    break;

  case 270: /* expression_block: open_block expressions close_block "finally" open_block expressions close_block  */
                                                                                                                        {
        auto pB = (ExprBlock *) (yyvsp[-5].pExpression);
        auto pF = (ExprBlock *) (yyvsp[-1].pExpression);
        swap ( pB->finalList, pF->list );
        (yyval.pExpression) = (yyvsp[-5].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-6]),(yylsp[0]));
        delete (yyvsp[-1].pExpression);
    }
    break;

  case 271: /* expr_call_pipe: expr expr_full_block_assumed_piped  */
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

  case 272: /* expr_call_pipe: expression_keyword expr_full_block_assumed_piped  */
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

  case 273: /* expr_call_pipe: "generator" '<' type_declaration_no_options '>' optional_capture_list expr_full_block_assumed_piped  */
                                                                                                                                             {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-3].pTypeDecl),(yyvsp[-1].pCaptList),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 274: /* expression_any: semicolon  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 275: /* expression_any: expr_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 276: /* expression_any: expr_keyword  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 277: /* expression_any: expr_assign_pipe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 278: /* expression_any: expr_assign semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 279: /* expression_any: expression_delete semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 280: /* expression_any: expression_let  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 281: /* expression_any: expression_while_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 282: /* expression_any: expression_unsafe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 283: /* expression_any: expression_with  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 284: /* expression_any: expression_with_alias  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 285: /* expression_any: expression_for_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 286: /* expression_any: expression_break semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 287: /* expression_any: expression_continue semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 288: /* expression_any: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 289: /* expression_any: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 290: /* expression_any: expression_if_then_else  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 291: /* expression_any: expression_try_catch  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 292: /* expression_any: expression_label semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 293: /* expression_any: expression_goto semicolon  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 294: /* expression_any: "pass" semicolon  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 295: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 296: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 297: /* expressions: expressions error  */
                                 {
        (void)(yyvsp[-1].pExpression); /* gc_node — don't delete Expression */ (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 298: /* expr_keyword: "keyword" expr expression_block  */
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

  case 299: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 300: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 301: /* optional_expr_list_in_braces: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 302: /* optional_expr_list_in_braces: '(' optional_expr_list optional_comma ')'  */
                                                             { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 303: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 304: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 305: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 306: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 307: /* $@9: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 308: /* $@10: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 309: /* expression_keyword: "keyword" '<' $@9 type_declaration_no_options_list '>' $@10 expr  */
                                                                                                                                                     {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 310: /* $@11: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 311: /* $@12: %empty  */
                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 312: /* expression_keyword: "type function" '<' $@11 type_declaration_no_options_list '>' $@12 optional_expr_list_in_braces  */
                                                                                                                                                                                   {
        auto pCall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),*(yyvsp[-6].s));
        pCall->arguments = typesAndSequenceToList((yyvsp[-3].pTypeDeclList),(yyvsp[0].pExpression));
        delete (yyvsp[-6].s);
        (yyval.pExpression) = pCall;
    }
    break;

  case 313: /* expr_pipe: expr_assign " <|" expr_block  */
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

  case 314: /* expr_pipe: "@ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 315: /* expr_pipe: "@@ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 316: /* expr_pipe: "$ <|" expr_block  */
                               {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 317: /* expr_pipe: expr_call_pipe  */
                             {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 318: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 319: /* name_in_namespace: "name" "::" "name"  */
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

  case 320: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 321: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 322: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 323: /* $@13: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 324: /* $@14: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 325: /* new_type_declaration: '<' $@13 type_declaration '>' $@14  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 326: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 327: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 328: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 329: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 330: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 331: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 332: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 333: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 334: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 335: /* expression_return_no_pipe: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 336: /* expression_return_no_pipe: "return" expr_list  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),sequenceToTuple((yyvsp[0].pExpression)));
    }
    break;

  case 337: /* expression_return_no_pipe: "return" "<-" expr_list  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),sequenceToTuple((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 338: /* expression_return: expression_return_no_pipe semicolon  */
                                                    {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 339: /* expression_return: "return" expr_pipe  */
                                           {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 340: /* expression_return: "return" "<-" expr_pipe  */
                                                  {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 341: /* expression_yield_no_pipe: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 342: /* expression_yield_no_pipe: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 343: /* expression_yield: expression_yield_no_pipe semicolon  */
                                                   {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 344: /* expression_yield: "yield" expr_pipe  */
                                          {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 345: /* expression_yield: "yield" "<-" expr_pipe  */
                                                 {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 346: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 347: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 348: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 349: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 350: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 351: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 352: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 353: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 354: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 355: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 356: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-7].pNameList),tokAt(scanner,(yylsp[-7])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 357: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 358: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 359: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                           {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameList),tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 360: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 361: /* tuple_expansion_variable_declaration: "[[" tuple_expansion ']' ']' optional_ref copy_or_move_or_clone expr_pipe  */
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

  case 362: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr semicolon  */
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

  case 363: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr_pipe  */
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

  case 366: /* $@15: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 367: /* $@16: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 368: /* expr_cast: "cast" '<' $@15 type_declaration_no_options '>' $@16 expr  */
                                                                                                                                                {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
    }
    break;

  case 369: /* $@17: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 370: /* $@18: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 371: /* expr_cast: "upcast" '<' $@17 type_declaration_no_options '>' $@18 expr  */
                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 372: /* $@19: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 373: /* $@20: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 374: /* expr_cast: "reinterpret" '<' $@19 type_declaration_no_options '>' $@20 expr  */
                                                                                                                                                       {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 375: /* $@21: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 376: /* $@22: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 377: /* expr_type_decl: "type" '<' $@21 type_declaration '>' $@22  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 378: /* expr_type_info: "typeinfo" '(' name_in_namespace expr ')'  */
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

  case 379: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" '>' expr ')'  */
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

  case 380: /* expr_type_info: "typeinfo" '(' name_in_namespace '<' "name" c_or_s "name" '>' expr ')'  */
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

  case 381: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
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

  case 382: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
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

  case 383: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" "end of expression" "name" '>' '(' expr ')'  */
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

  case 384: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 385: /* expr_list: expr_list ',' expr  */
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
                                         { (yyval.pCaptList) = (yyvsp[-2].pCaptList); }
    break;

  case 401: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 402: /* expr_block: expression_block  */
                                            {
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

  case 405: /* $@23: %empty  */
                             {  yyextra->das_need_oxford_comma = false; }
    break;

  case 406: /* expr_full_block_assumed_piped: block_or_lambda $@23 optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type expression_block  */
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
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 434: /* expr_assign_pipe_right: "@@ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 435: /* expr_assign_pipe_right: "$ <|" expr_block  */
                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
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

  case 456: /* expr_method_call: expr "->" "name" '(' ')'  */
                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 457: /* expr_method_call: expr "->" "name" '(' expr_list ')'  */
                                                                              {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
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

  case 459: /* func_addr_name: "$i" '(' expr ')'  */
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

  case 461: /* $@24: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 462: /* $@25: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 463: /* func_addr_expr: '@' '@' '<' $@24 type_declaration_no_options '>' $@25 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 464: /* $@26: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 465: /* $@27: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 466: /* func_addr_expr: '@' '@' '<' $@26 optional_function_argument_list optional_function_type '>' $@27 func_addr_name  */
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

  case 467: /* expr_field: expr '.' "name"  */
                                              {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 468: /* expr_field: expr '.' '.' "name"  */
                                                  {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 469: /* expr_field: expr '.' "name" '(' ')'  */
                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 470: /* expr_field: expr '.' "name" '(' expr_list ')'  */
                                                                           {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 471: /* expr_field: expr '.' "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                       {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 472: /* expr_field: expr '.' basic_type_declaration '(' ')'  */
                                                                        {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 473: /* expr_field: expr '.' basic_type_declaration '(' expr_list ')'  */
                                                                                             {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 474: /* $@28: %empty  */
                               { yyextra->das_suppress_errors=true; }
    break;

  case 475: /* $@29: %empty  */
                                                                            { yyextra->das_suppress_errors=false; }
    break;

  case 476: /* expr_field: expr '.' $@28 error $@29  */
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
            dd->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            dd->useInitializer = false;
            dd->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = dd;
    }
    break;

  case 479: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
                                                               {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 480: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
                                                                                 {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-4])),*(yyvsp[-4].s));
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

  case 484: /* expr: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 485: /* expr: name_in_namespace  */
                                              { (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 486: /* expr: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 487: /* expr: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 488: /* expr: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 489: /* expr: make_decl  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 490: /* expr: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 491: /* expr: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 492: /* expr: expr_field  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 493: /* expr: expr_mtag  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 494: /* expr: '!' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",(yyvsp[0].pExpression)); }
    break;

  case 495: /* expr: '~' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",(yyvsp[0].pExpression)); }
    break;

  case 496: /* expr: '+' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",(yyvsp[0].pExpression)); }
    break;

  case 497: /* expr: '-' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",(yyvsp[0].pExpression)); }
    break;

  case 498: /* expr: expr "<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 499: /* expr: expr ">>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 500: /* expr: expr "<<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 501: /* expr: expr ">>>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 502: /* expr: expr '+' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 503: /* expr: expr '-' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 504: /* expr: expr '*' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 505: /* expr: expr '/' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 506: /* expr: expr '%' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 507: /* expr: expr '<' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 508: /* expr: expr '>' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 509: /* expr: expr "==" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 510: /* expr: expr "!=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 511: /* expr: expr "<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 512: /* expr: expr ">=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 513: /* expr: expr '&' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 514: /* expr: expr '|' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 515: /* expr: expr '^' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 516: /* expr: expr "&&" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 517: /* expr: expr "||" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 518: /* expr: expr "^^" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 519: /* expr: expr ".." expr  */
                                             {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 520: /* expr: "++" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", (yyvsp[0].pExpression)); }
    break;

  case 521: /* expr: "--" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", (yyvsp[0].pExpression)); }
    break;

  case 522: /* expr: expr "++"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 523: /* expr: expr "--"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 524: /* expr: '(' expr_list optional_comma ')'  */
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

  case 525: /* expr: '(' make_struct_single ')'  */
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

  case 526: /* expr: expr '[' expr ']'  */
                                                 { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 527: /* expr: expr '.' '[' expr ']'  */
                                                     { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 528: /* expr: expr "?[" expr ']'  */
                                                 { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 529: /* expr: expr '.' "?[" expr ']'  */
                                                     { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 530: /* expr: expr "?." "name"  */
                                                 { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 531: /* expr: expr '.' "?." "name"  */
                                                     { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 532: /* expr: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 533: /* expr: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 534: /* expr: '*' expr  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 535: /* expr: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 536: /* expr: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 537: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 538: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 539: /* expr: expr "??" expr  */
                                                   { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 540: /* expr: expr '?' expr ':' expr  */
                                                          {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        }
    break;

  case 541: /* $@30: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 542: /* $@31: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 543: /* expr: expr "is" "type" '<' $@30 type_declaration_no_options '>' $@31  */
                                                                                                                                                       {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 544: /* expr: expr "is" basic_type_declaration  */
                                                               {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 545: /* expr: expr "is" "name"  */
                                              {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 546: /* expr: expr "as" "name"  */
                                              {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 547: /* $@32: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 548: /* $@33: %empty  */
                                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 549: /* expr: expr "as" "type" '<' $@32 type_declaration '>' $@33  */
                                                                                                                                            {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 550: /* expr: expr "as" basic_type_declaration  */
                                                               {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 551: /* expr: expr '?' "as" "name"  */
                                                  {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 552: /* $@34: %empty  */
                                                   { yyextra->das_arrow_depth ++; }
    break;

  case 553: /* $@35: %empty  */
                                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 554: /* expr: expr '?' "as" "type" '<' $@34 type_declaration '>' $@35  */
                                                                                                                                                {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 555: /* expr: expr '?' "as" basic_type_declaration  */
                                                                   {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 556: /* expr: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 557: /* expr: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 558: /* expr: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 559: /* expr: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 560: /* expr: expr_method_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 561: /* expr: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 562: /* expr: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 563: /* expr: expr "<|" expr  */
                                                { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 564: /* expr: expr "|>" expr  */
                                                { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 565: /* expr: expr "|>" basic_type_declaration  */
                                                          {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 566: /* expr: name_in_namespace "name"  */
                                                { (yyval.pExpression) = ast_NameName(scanner,(yyvsp[-1].s),(yyvsp[0].s),tokAt(scanner,(yylsp[-1])),tokAt(scanner,(yylsp[0]))); }
    break;

  case 567: /* expr: "unsafe" '(' expr ')'  */
                                         {
        (yyvsp[-1].pExpression)->alwaysSafe = true;
        (yyvsp[-1].pExpression)->userSaidItsSafe = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 568: /* expr: expression_keyword  */
                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 569: /* expr_mtag: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 570: /* expr_mtag: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 571: /* expr_mtag: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 572: /* expr_mtag: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 573: /* expr_mtag: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 574: /* expr_mtag: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 575: /* expr_mtag: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 576: /* expr_mtag: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 577: /* expr_mtag: expr '.' "$f" '(' expr ')'  */
                                                                {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 578: /* expr_mtag: expr "?." "$f" '(' expr ')'  */
                                                                 {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 579: /* expr_mtag: expr '.' '.' "$f" '(' expr ')'  */
                                                                    {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 580: /* expr_mtag: expr '.' "?." "$f" '(' expr ')'  */
                                                                     {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 581: /* expr_mtag: expr "as" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 582: /* expr_mtag: expr '?' "as" "$f" '(' expr ')'  */
                                                                       {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 583: /* expr_mtag: expr "is" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 584: /* expr_mtag: '@' '@' "$c" '(' expr ')'  */
                                                         {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 585: /* optional_field_annotation: %empty  */
                                                        { (yyval.aaList) = nullptr; }
    break;

  case 586: /* optional_field_annotation: "[[" annotation_argument_list ']' ']'  */
                                                        { (yyval.aaList) = (yyvsp[-2].aaList); /*this one is gone when BRABRA is disabled*/ }
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

  case 599: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 600: /* struct_variable_declaration_list: struct_variable_declaration_list semicolon  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 601: /* $@36: %empty  */
                                                               { yyextra->das_force_oxford_comma=true;}
    break;

  case 602: /* struct_variable_declaration_list: struct_variable_declaration_list "typedef" $@36 "name" '=' type_declaration semicolon  */
                                                                                                                                                         {
        (yyval.pVarDeclList) = (yyvsp[-6].pVarDeclList);
        ast_structureAlias(scanner,(yyvsp[-3].s),(yyvsp[-1].pTypeDecl),tokAt(scanner,(yylsp[-5])));
    }
    break;

  case 603: /* $@37: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 604: /* struct_variable_declaration_list: struct_variable_declaration_list $@37 structure_variable_declaration semicolon  */
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

  case 605: /* $@38: %empty  */
                                                                                                                     {
                yyextra->das_force_oxford_comma=true;
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 606: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable "abstract" optional_constant $@38 function_declaration_header semicolon  */
                                                          {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 607: /* $@39: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 608: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@39 function_declaration_header expression_block  */
                                                                        {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-9].pVarDeclList),(yyvsp[-8].faList),(yyvsp[-5].b),(yyvsp[-6].b),(yyvsp[-4].i),(yyvsp[-3].b),(yyvsp[-1].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-7]),(yylsp[0])),tokAt(scanner,(yylsp[-8])));
            }
    break;

  case 609: /* struct_variable_declaration_list: struct_variable_declaration_list '[' annotation_list ']' semicolon  */
                                                                                       {
        das_yyerror(scanner,"structure field or class method annotation expected to remain on the same line with the field or the class",
            tokAt(scanner,(yylsp[-2])), CompilationError::syntax_error);
        delete (yyvsp[-2].faList);
        (yyval.pVarDeclList) = (yyvsp[-4].pVarDeclList);
    }
    break;

  case 610: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 611: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 612: /* function_argument_declaration_type: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 613: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 614: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 615: /* function_argument_list: function_argument_declaration_no_type semicolon function_argument_list  */
                                                                                            { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 616: /* function_argument_list: function_argument_declaration_type semicolon function_argument_list  */
                                                                                            { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 617: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 618: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 619: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 620: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 621: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                          { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 622: /* tuple_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 623: /* tuple_alias_type_list: tuple_alias_type_list c_or_s  */
                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 624: /* tuple_alias_type_list: tuple_alias_type_list tuple_type c_or_s  */
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

  case 625: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 626: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 627: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 628: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 629: /* variant_alias_type_list: variant_alias_type_list c_or_s  */
                                           {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 630: /* variant_alias_type_list: variant_alias_type_list variant_type c_or_s  */
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

  case 631: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 632: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 633: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 634: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 635: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 636: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 637: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 638: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 639: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 640: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 641: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 642: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 643: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 644: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 645: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 646: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 647: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 648: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 649: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 650: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options semicolon  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 651: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr semicolon  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 652: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr_pipe  */
                                                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 653: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr semicolon  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 654: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr_pipe  */
                                                                                                           {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-3]));
        typeDecl->ref = (yyvsp[-2].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-1].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-1].i) & CorM_CLONE) !=0;
    }
    break;

  case 655: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 656: /* global_variable_declaration_list: global_variable_declaration_list "end of line"  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 657: /* $@40: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 658: /* global_variable_declaration_list: global_variable_declaration_list $@40 optional_field_annotation let_variable_declaration  */
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

  case 659: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 660: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 661: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 662: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 663: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 664: /* global_let: kwd_let optional_shared optional_public_or_private_variable open_block global_variable_declaration_list close_block  */
                                                                                                                                                      {
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 665: /* $@41: %empty  */
                                                                                        {
        yyextra->das_force_oxford_comma=true;
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 666: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@41 optional_field_annotation let_variable_declaration  */
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

  case 667: /* enum_list: %empty  */
        {
        (yyval.pEnumList) = new Enumeration();
    }
    break;

  case 668: /* enum_list: enum_list semicolon  */
                                {
        (yyval.pEnumList) = (yyvsp[-1].pEnumList);
    }
    break;

  case 669: /* enum_list: enum_list "name" semicolon  */
                                           {
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

  case 670: /* enum_list: enum_list "name" '=' expr semicolon  */
                                                           {
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

  case 671: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 672: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 673: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 674: /* $@42: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 675: /* single_alias: optional_public_or_private_alias "name" $@42 '=' type_declaration  */
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

  case 679: /* $@43: %empty  */
                    { yyextra->das_force_oxford_comma=true;}
    break;

  case 681: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 682: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 683: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 684: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 685: /* $@44: %empty  */
                                                                                                                       {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 686: /* $@45: %empty  */
                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
    }
    break;

  case 687: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name open_block $@44 enum_list $@45 close_block  */
                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-5].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-8].faList),tokAt(scanner,(yylsp[-8])),(yyvsp[-6].b),(yyvsp[-5].pEnum),(yyvsp[-2].pEnumList),Type::tInt);
    }
    break;

  case 688: /* $@46: %empty  */
                                                                                                                                                            {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 689: /* $@47: %empty  */
                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
    }
    break;

  case 690: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name ':' enum_basic_type_declaration open_block $@46 enum_list $@47 close_block  */
                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-7].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-10].faList),tokAt(scanner,(yylsp[-10])),(yyvsp[-8].b),(yyvsp[-7].pEnum),(yyvsp[-2].pEnumList),(yyvsp[-5].type));
    }
    break;

  case 691: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 692: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 693: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 694: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 695: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 696: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 697: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 698: /* class_or_struct: "class" "template"  */
                                 { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 699: /* class_or_struct: "struct" "template"  */
                                 { (yyval.i) = CorS_StructTemplate; }
    break;

  case 700: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 701: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 702: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 703: /* optional_struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 704: /* optional_struct_variable_declaration_list: open_block struct_variable_declaration_list close_block  */
                                                                      {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 705: /* $@48: %empty  */
                                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 706: /* $@49: %empty  */
                         {
        if ( (yyvsp[0].pStructure) ) {
            (yyvsp[0].pStructure)->isClass = (yyvsp[-3].i)==CorS_Class || (yyvsp[-3].i)==CorS_ClassTemplate;
            (yyvsp[0].pStructure)->isTemplate = (yyvsp[-3].i)==CorS_ClassTemplate || (yyvsp[-3].i)==CorS_StructTemplate;
            (yyvsp[0].pStructure)->privateStructure = !(yyvsp[-2].b);
        }
    }
    break;

  case 707: /* structure_declaration: optional_annotation_list class_or_struct optional_public_or_private_structure $@48 structure_name $@49 optional_struct_variable_declaration_list  */
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

  case 708: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 709: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 710: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 711: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 712: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 713: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 714: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 715: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 716: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 717: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 718: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 719: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 720: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 721: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 722: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 723: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 724: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 725: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 726: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 727: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 728: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 729: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 730: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 731: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 732: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 733: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 734: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 735: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 736: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 737: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 738: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 739: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 740: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 741: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 742: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 743: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 744: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 745: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 746: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 747: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 748: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 749: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 750: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 751: /* auto_type_declaration: "$t" '(' expr ')'  */
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

  case 752: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 753: /* bitfield_bits: bitfield_bits semicolon "name"  */
                                                 {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 754: /* bitfield_bits: bitfield_bits ',' "name"  */
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

  case 756: /* bitfield_alias_bits: bitfield_alias_bits semicolon  */
                                            {
        (yyval.pNameExprList) = (yyvsp[-1].pNameExprList);
    }
    break;

  case 757: /* bitfield_alias_bits: bitfield_alias_bits "name" semicolon  */
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

  case 758: /* bitfield_alias_bits: bitfield_alias_bits "name" '=' expr semicolon  */
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

  case 759: /* bitfield_basic_type_declaration: %empty  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 760: /* bitfield_basic_type_declaration: ':' "uint8"  */
                             { (yyval.type) = Type::tBitfield8; }
    break;

  case 761: /* bitfield_basic_type_declaration: ':' "uint16"  */
                             { (yyval.type) = Type::tBitfield16; }
    break;

  case 762: /* bitfield_basic_type_declaration: ':' "uint"  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 763: /* bitfield_basic_type_declaration: ':' "uint64"  */
                             { (yyval.type) = Type::tBitfield64; }
    break;

  case 764: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' '>'  */
                                                                          {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-2].type));
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-2]));
    }
    break;

  case 765: /* $@50: %empty  */
                                                                     { yyextra->das_arrow_depth ++; }
    break;

  case 766: /* $@51: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 767: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' $@50 bitfield_bits '>' $@51  */
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

  case 770: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 771: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 772: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 773: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 774: /* type_declaration_no_options: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 775: /* type_declaration_no_options: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 776: /* type_declaration_no_options: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 777: /* type_declaration_no_options: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 778: /* type_declaration_no_options: type_declaration_no_options dim_list  */
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
    }
    break;

  case 779: /* type_declaration_no_options: type_declaration_no_options '[' ']'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->dim.push_back(TypeDecl::dimAuto);
        (yyvsp[-2].pTypeDecl)->dimExpr.push_back(nullptr);
        (yyvsp[-2].pTypeDecl)->removeDim = false;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 780: /* $@52: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 781: /* $@53: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 782: /* type_declaration_no_options: "type" '<' $@52 type_declaration '>' $@53  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 783: /* type_declaration_no_options: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 784: /* type_declaration_no_options: '$' name_in_namespace optional_expr_list_in_braces  */
                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-1]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-1])), *(yyvsp[-1].s)));
        delete (yyvsp[-1].s);
    }
    break;

  case 785: /* $@54: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 786: /* type_declaration_no_options: '$' name_in_namespace '<' $@54 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 787: /* type_declaration_no_options: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 788: /* type_declaration_no_options: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 789: /* type_declaration_no_options: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 790: /* type_declaration_no_options: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 791: /* type_declaration_no_options: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 792: /* type_declaration_no_options: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 793: /* type_declaration_no_options: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 794: /* type_declaration_no_options: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 795: /* type_declaration_no_options: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 796: /* type_declaration_no_options: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 797: /* type_declaration_no_options: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 798: /* type_declaration_no_options: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 799: /* $@55: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 800: /* $@56: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 801: /* type_declaration_no_options: "smart_ptr" '<' $@55 type_declaration '>' $@56  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 802: /* type_declaration_no_options: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 803: /* $@57: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 804: /* $@58: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 805: /* type_declaration_no_options: "array" '<' $@57 type_declaration '>' $@58  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 806: /* $@59: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 807: /* $@60: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 808: /* type_declaration_no_options: "table" '<' $@59 table_type_pair '>' $@60  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 809: /* $@61: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 810: /* $@62: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 811: /* type_declaration_no_options: "iterator" '<' $@61 type_declaration '>' $@62  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 812: /* type_declaration_no_options: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 813: /* $@63: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 814: /* $@64: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 815: /* type_declaration_no_options: "block" '<' $@63 type_declaration '>' $@64  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 816: /* $@65: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 817: /* $@66: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 818: /* type_declaration_no_options: "block" '<' $@65 optional_function_argument_list optional_function_type '>' $@66  */
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

  case 819: /* type_declaration_no_options: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 820: /* $@67: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 821: /* $@68: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 822: /* type_declaration_no_options: "function" '<' $@67 type_declaration '>' $@68  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 823: /* $@69: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 824: /* $@70: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 825: /* type_declaration_no_options: "function" '<' $@69 optional_function_argument_list optional_function_type '>' $@70  */
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

  case 826: /* type_declaration_no_options: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 827: /* $@71: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 828: /* $@72: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 829: /* type_declaration_no_options: "lambda" '<' $@71 type_declaration '>' $@72  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 830: /* $@73: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 831: /* $@74: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 832: /* type_declaration_no_options: "lambda" '<' $@73 optional_function_argument_list optional_function_type '>' $@74  */
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

  case 833: /* $@75: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 834: /* $@76: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 835: /* type_declaration_no_options: "tuple" '<' $@75 tuple_type_list '>' $@76  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 836: /* $@77: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 837: /* $@78: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 838: /* type_declaration_no_options: "variant" '<' $@77 variant_type_list '>' $@78  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 839: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 840: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 841: /* type_declaration: type_declaration '|' '#'  */
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

  case 842: /* $@79: %empty  */
                                                          { yyextra->das_need_oxford_comma=false; }
    break;

  case 843: /* $@80: %empty  */
                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 844: /* $@81: %empty  */
                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 845: /* $@82: %empty  */
                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
    }
    break;

  case 846: /* tuple_alias_declaration: "tuple" optional_public_or_private_alias $@79 "name" $@80 open_block $@81 tuple_alias_type_list $@82 close_block  */
                  {
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

  case 847: /* $@83: %empty  */
                                                            { yyextra->das_need_oxford_comma=false; }
    break;

  case 848: /* $@84: %empty  */
                                                                                                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 849: /* $@85: %empty  */
                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 850: /* $@86: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
    }
    break;

  case 851: /* variant_alias_declaration: "variant" optional_public_or_private_alias $@83 "name" $@84 open_block $@85 variant_alias_type_list $@86 close_block  */
                  {
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

  case 852: /* $@87: %empty  */
                                                             { yyextra->das_need_oxford_comma=false; }
    break;

  case 853: /* $@88: %empty  */
                                                                                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 854: /* $@89: %empty  */
                                                            {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 855: /* $@90: %empty  */
                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-5]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
    }
    break;

  case 856: /* bitfield_alias_declaration: "bitfield" optional_public_or_private_alias $@87 "name" $@88 bitfield_basic_type_declaration open_block $@89 bitfield_alias_bits $@90 close_block  */
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
                CompilationError::invalid_type);
        }
        for ( auto & p : *(yyvsp[-2].pNameExprList) ) {
            if ( get<1>(p) ) {
                ast_globalBitfieldConst ( scanner, btype, (yyvsp[-9].b), get<0>(p), get<1>(p) );
            }
        }
        if ( !yyextra->g_Program->addAlias(btype) ) {
            das_yyerror(scanner,"type alias is already defined "+*(yyvsp[-7].s),tokAt(scanner,(yylsp[-7])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-7]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfield((yyvsp[-7].s)->c_str(),atvname);
        }
        delete (yyvsp[-7].s);
        delete (yyvsp[-2].pNameExprList);
    }
    break;

  case 857: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 858: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 859: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 860: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 861: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 862: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 863: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 864: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 865: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 866: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 867: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 868: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 869: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 870: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 871: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 872: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 873: /* make_struct_dim: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 874: /* make_struct_dim: make_struct_dim "end of expression" make_struct_fields  */
                                                         {
        ((ExprMakeStruct *) (yyvsp[-2].pExpression))->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 875: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 876: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 877: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 878: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 879: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 880: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 881: /* optional_block: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 882: /* optional_block: "where" expr_block  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 895: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 896: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 897: /* make_struct_decl: "[[" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-4]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 898: /* make_struct_decl: "[[" type_declaration_no_options optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->makeType = (yyvsp[-2].pTypeDecl);
        msd->block = (yyvsp[-1].pExpression);
        msd->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = msd;
    }
    break;

  case 899: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                   {
        auto msd = new ExprMakeStruct();
        msd->makeType = (yyvsp[-4].pTypeDecl);
        msd->useInitializer = true;
        msd->block = (yyvsp[-1].pExpression);
        msd->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pExpression) = msd;
    }
    break;

  case 900: /* make_struct_decl: "[[" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_sqr_sqr  */
                                                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-5].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-6]));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 901: /* make_struct_decl: "[{" type_declaration_no_options make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
                                                                                                                                {
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-2].pExpression))->block = (yyvsp[-1].pExpression);
        (yyvsp[-2].pExpression)->at = tokAt(scanner,(yylsp[-4]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_array_move");
        tam->arguments.push_back((yyvsp[-2].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 902: /* make_struct_decl: "[{" type_declaration_no_options '(' ')' make_struct_dim optional_block optional_trailing_delim_cur_sqr  */
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

  case 903: /* $@91: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 904: /* $@92: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 905: /* make_struct_decl: "struct" '<' $@91 type_declaration_no_options '>' $@92 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                            {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 906: /* $@93: %empty  */
                            { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 907: /* $@94: %empty  */
                                                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 908: /* make_struct_decl: "class" '<' $@93 type_declaration_no_options '>' $@94 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                                                           {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 909: /* $@95: %empty  */
                               { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 910: /* $@96: %empty  */
                                                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 911: /* make_struct_decl: "variant" '<' $@95 variant_type_list '>' $@96 '(' use_initializer make_variant_dim ')'  */
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

  case 912: /* $@97: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 913: /* $@98: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 914: /* make_struct_decl: "default" '<' $@97 type_declaration_no_options '>' $@98 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 915: /* make_tuple: expr  */
                  {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 916: /* make_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 917: /* make_tuple: make_tuple ',' expr  */
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

  case 918: /* make_map_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 919: /* make_map_tuple: expr  */
                 {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 920: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = new TypeDecl(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 921: /* $@99: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 922: /* $@100: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 923: /* make_tuple_call: "tuple" '<' $@99 tuple_type_list '>' $@100 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 924: /* make_dim: make_tuple  */
                        {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 925: /* make_dim: make_dim "end of expression" make_tuple  */
                                          {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 926: /* make_dim_decl: '[' optional_expr_list ']'  */
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

  case 927: /* make_dim_decl: "[[" type_declaration_no_options make_dim optional_trailing_semicolon_sqr_sqr  */
                                                                                                         {
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-2].pTypeDecl);
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 928: /* make_dim_decl: "[{" type_declaration_no_options make_dim optional_trailing_semicolon_cur_sqr  */
                                                                                                         {
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-2].pTypeDecl);
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-3]));
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_array_move");
        tam->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = tam;
    }
    break;

  case 929: /* $@101: %empty  */
                                       { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 930: /* $@102: %empty  */
                                                                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 931: /* make_dim_decl: "array" "struct" '<' $@101 type_declaration_no_options '>' $@102 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 932: /* $@103: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 933: /* $@104: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 934: /* make_dim_decl: "array" "tuple" '<' $@103 tuple_type_list '>' $@104 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 935: /* $@105: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 936: /* $@106: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 937: /* make_dim_decl: "array" "variant" '<' $@105 variant_type_list '>' $@106 '(' make_variant_dim ')'  */
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

  case 938: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
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

  case 939: /* $@107: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 940: /* $@108: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 941: /* make_dim_decl: "array" '<' $@107 type_declaration_no_options '>' $@108 '(' optional_expr_list ')'  */
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

  case 942: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 943: /* $@109: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 944: /* $@110: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 945: /* make_dim_decl: "fixed_array" '<' $@109 type_declaration_no_options '>' $@110 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 946: /* make_table: make_map_tuple  */
                            {
        auto mka = new ExprMakeArray();
        mka->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mka;
    }
    break;

  case 947: /* make_table: make_table "end of expression" make_map_tuple  */
                                                {
        ((ExprMakeArray *) (yyvsp[-2].pExpression))->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 948: /* expr_map_tuple_list: make_map_tuple  */
                                {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 949: /* expr_map_tuple_list: expr_map_tuple_list ',' make_map_tuple  */
                                                                {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 950: /* make_table_decl: "begin of code block" optional_expr_map_tuple_list "end of code block"  */
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

  case 951: /* make_table_decl: "{{" make_table optional_trailing_semicolon_cur_cur  */
                                                                          {
        auto mkt = new TypeDecl(Type::autoinfer);
        mkt->dim.push_back(TypeDecl::dimAuto);
        ((ExprMakeArray *)(yyvsp[-1].pExpression))->makeType = mkt;
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-2]));
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),"to_table_move");
        ttm->arguments.push_back((yyvsp[-1].pExpression));
        (yyval.pExpression) = ttm;
    }
    break;

  case 952: /* make_table_decl: "table" '(' optional_expr_map_tuple_list ')'  */
                                                                       {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-1].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 953: /* make_table_decl: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 954: /* make_table_decl: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 955: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 956: /* array_comprehension_where: "end of expression" "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 957: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 958: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 959: /* array_comprehension: '[' "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                    {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 960: /* array_comprehension: '[' "iterator" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                                 {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 961: /* array_comprehension: "[[" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']' ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),true,false);
    }
    break;

  case 962: /* array_comprehension: "[{" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where "end of code block" ']'  */
                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-8])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-3].pExpression),(yyvsp[-2].pExpression),tokRangeAt(scanner,(yylsp[-3]),(yylsp[0])),false,false);
    }
    break;

  case 963: /* array_comprehension: "begin of code block" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where "end of code block"  */
                                                                                                                                                              {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
    }
    break;

  case 964: /* array_comprehension: "{{" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where "end of code block" "end of code block"  */
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
