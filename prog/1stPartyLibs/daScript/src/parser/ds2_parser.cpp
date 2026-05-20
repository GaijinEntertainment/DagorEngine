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
  YYSYMBOL_RPIPE = 132,                    /* "|>"  */
  YYSYMBOL_CLONEEQU = 133,                 /* ":="  */
  YYSYMBOL_ROTL = 134,                     /* "<<<"  */
  YYSYMBOL_ROTR = 135,                     /* ">>>"  */
  YYSYMBOL_ROTLEQU = 136,                  /* "<<<="  */
  YYSYMBOL_ROTREQU = 137,                  /* ">>>="  */
  YYSYMBOL_MAPTO = 138,                    /* "=>"  */
  YYSYMBOL_COLCOL = 139,                   /* "::"  */
  YYSYMBOL_ANDAND = 140,                   /* "&&"  */
  YYSYMBOL_OROR = 141,                     /* "||"  */
  YYSYMBOL_XORXOR = 142,                   /* "^^"  */
  YYSYMBOL_ANDANDEQU = 143,                /* "&&="  */
  YYSYMBOL_OROREQU = 144,                  /* "||="  */
  YYSYMBOL_XORXOREQU = 145,                /* "^^="  */
  YYSYMBOL_DOTDOT = 146,                   /* ".."  */
  YYSYMBOL_MTAG_E = 147,                   /* "$$"  */
  YYSYMBOL_MTAG_I = 148,                   /* "$i"  */
  YYSYMBOL_MTAG_V = 149,                   /* "$v"  */
  YYSYMBOL_MTAG_B = 150,                   /* "$b"  */
  YYSYMBOL_MTAG_A = 151,                   /* "$a"  */
  YYSYMBOL_MTAG_T = 152,                   /* "$t"  */
  YYSYMBOL_MTAG_C = 153,                   /* "$c"  */
  YYSYMBOL_MTAG_F = 154,                   /* "$f"  */
  YYSYMBOL_MTAG_DOTDOTDOT = 155,           /* "..."  */
  YYSYMBOL_INTEGER = 156,                  /* "integer constant"  */
  YYSYMBOL_LONG_INTEGER = 157,             /* "long integer constant"  */
  YYSYMBOL_UNSIGNED_INTEGER = 158,         /* "unsigned integer constant"  */
  YYSYMBOL_UNSIGNED_LONG_INTEGER = 159,    /* "unsigned long integer constant"  */
  YYSYMBOL_UNSIGNED_INT8 = 160,            /* "unsigned int8 constant"  */
  YYSYMBOL_DAS_FLOAT = 161,                /* "floating point constant"  */
  YYSYMBOL_DOUBLE = 162,                   /* "double constant"  */
  YYSYMBOL_NAME = 163,                     /* "name"  */
  YYSYMBOL_DAS_EMIT_COMMA = 164,           /* "new line, comma"  */
  YYSYMBOL_DAS_EMIT_SEMICOLON = 165,       /* "new line, semicolon"  */
  YYSYMBOL_BEGIN_STRING = 166,             /* "start of the string"  */
  YYSYMBOL_STRING_CHARACTER = 167,         /* STRING_CHARACTER  */
  YYSYMBOL_STRING_CHARACTER_ESC = 168,     /* STRING_CHARACTER_ESC  */
  YYSYMBOL_END_STRING = 169,               /* "end of the string"  */
  YYSYMBOL_BEGIN_STRING_EXPR = 170,        /* "{"  */
  YYSYMBOL_END_STRING_EXPR = 171,          /* "}"  */
  YYSYMBOL_END_OF_READ = 172,              /* "end of failed eader macro"  */
  YYSYMBOL_173_ = 173,                     /* ','  */
  YYSYMBOL_174_ = 174,                     /* '='  */
  YYSYMBOL_175_ = 175,                     /* '?'  */
  YYSYMBOL_176_ = 176,                     /* ':'  */
  YYSYMBOL_177_ = 177,                     /* '|'  */
  YYSYMBOL_178_ = 178,                     /* '^'  */
  YYSYMBOL_179_ = 179,                     /* '&'  */
  YYSYMBOL_180_ = 180,                     /* '<'  */
  YYSYMBOL_181_ = 181,                     /* '>'  */
  YYSYMBOL_182_ = 182,                     /* '-'  */
  YYSYMBOL_183_ = 183,                     /* '+'  */
  YYSYMBOL_184_ = 184,                     /* '*'  */
  YYSYMBOL_185_ = 185,                     /* '/'  */
  YYSYMBOL_186_ = 186,                     /* '%'  */
  YYSYMBOL_UNARY_MINUS = 187,              /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 188,               /* UNARY_PLUS  */
  YYSYMBOL_189_ = 189,                     /* '~'  */
  YYSYMBOL_190_ = 190,                     /* '!'  */
  YYSYMBOL_PRE_INC = 191,                  /* PRE_INC  */
  YYSYMBOL_PRE_DEC = 192,                  /* PRE_DEC  */
  YYSYMBOL_LLPIPE = 193,                   /* LLPIPE  */
  YYSYMBOL_POST_INC = 194,                 /* POST_INC  */
  YYSYMBOL_POST_DEC = 195,                 /* POST_DEC  */
  YYSYMBOL_DEREF = 196,                    /* DEREF  */
  YYSYMBOL_197_ = 197,                     /* '.'  */
  YYSYMBOL_198_ = 198,                     /* '['  */
  YYSYMBOL_199_ = 199,                     /* ']'  */
  YYSYMBOL_200_ = 200,                     /* '('  */
  YYSYMBOL_201_ = 201,                     /* ')'  */
  YYSYMBOL_202_ = 202,                     /* '$'  */
  YYSYMBOL_203_ = 203,                     /* '@'  */
  YYSYMBOL_204_ = 204,                     /* ';'  */
  YYSYMBOL_205_ = 205,                     /* '{'  */
  YYSYMBOL_206_ = 206,                     /* '}'  */
  YYSYMBOL_207_ = 207,                     /* '#'  */
  YYSYMBOL_YYACCEPT = 208,                 /* $accept  */
  YYSYMBOL_program = 209,                  /* program  */
  YYSYMBOL_COMMA = 210,                    /* COMMA  */
  YYSYMBOL_SEMICOLON = 211,                /* SEMICOLON  */
  YYSYMBOL_top_level_reader_macro = 212,   /* top_level_reader_macro  */
  YYSYMBOL_optional_public_or_private_module = 213, /* optional_public_or_private_module  */
  YYSYMBOL_module_name = 214,              /* module_name  */
  YYSYMBOL_optional_not_required = 215,    /* optional_not_required  */
  YYSYMBOL_module_declaration = 216,       /* module_declaration  */
  YYSYMBOL_character_sequence = 217,       /* character_sequence  */
  YYSYMBOL_string_constant = 218,          /* string_constant  */
  YYSYMBOL_format_string = 219,            /* format_string  */
  YYSYMBOL_optional_format_string = 220,   /* optional_format_string  */
  YYSYMBOL_221_1 = 221,                    /* $@1  */
  YYSYMBOL_string_builder_body = 222,      /* string_builder_body  */
  YYSYMBOL_string_builder = 223,           /* string_builder  */
  YYSYMBOL_reader_character_sequence = 224, /* reader_character_sequence  */
  YYSYMBOL_expr_reader = 225,              /* expr_reader  */
  YYSYMBOL_226_2 = 226,                    /* $@2  */
  YYSYMBOL_options_declaration = 227,      /* options_declaration  */
  YYSYMBOL_require_declaration = 228,      /* require_declaration  */
  YYSYMBOL_require_module_name = 229,      /* require_module_name  */
  YYSYMBOL_require_module = 230,           /* require_module  */
  YYSYMBOL_is_public_module = 231,         /* is_public_module  */
  YYSYMBOL_expect_declaration = 232,       /* expect_declaration  */
  YYSYMBOL_expect_list = 233,              /* expect_list  */
  YYSYMBOL_expect_error = 234,             /* expect_error  */
  YYSYMBOL_expression_label = 235,         /* expression_label  */
  YYSYMBOL_expression_goto = 236,          /* expression_goto  */
  YYSYMBOL_elif_or_static_elif = 237,      /* elif_or_static_elif  */
  YYSYMBOL_emit_semis = 238,               /* emit_semis  */
  YYSYMBOL_optional_emit_semis = 239,      /* optional_emit_semis  */
  YYSYMBOL_expression_else = 240,          /* expression_else  */
  YYSYMBOL_241_3 = 241,                    /* $@3  */
  YYSYMBOL_242_4 = 242,                    /* $@4  */
  YYSYMBOL_if_or_static_if = 243,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 244, /* expression_else_one_liner  */
  YYSYMBOL_expression_if_one_liner = 245,  /* expression_if_one_liner  */
  YYSYMBOL_semis = 246,                    /* semis  */
  YYSYMBOL_optional_semis = 247,           /* optional_semis  */
  YYSYMBOL_expression_if_block = 248,      /* expression_if_block  */
  YYSYMBOL_249_5 = 249,                    /* $@5  */
  YYSYMBOL_250_6 = 250,                    /* $@6  */
  YYSYMBOL_251_7 = 251,                    /* $@7  */
  YYSYMBOL_expression_else_block = 252,    /* expression_else_block  */
  YYSYMBOL_253_8 = 253,                    /* $@8  */
  YYSYMBOL_254_9 = 254,                    /* $@9  */
  YYSYMBOL_255_10 = 255,                   /* $@10  */
  YYSYMBOL_expression_if_then_else = 256,  /* expression_if_then_else  */
  YYSYMBOL_257_11 = 257,                   /* $@11  */
  YYSYMBOL_258_12 = 258,                   /* $@12  */
  YYSYMBOL_expression_if_then_else_oneliner = 259, /* expression_if_then_else_oneliner  */
  YYSYMBOL_for_variable_name_with_pos_list = 260, /* for_variable_name_with_pos_list  */
  YYSYMBOL_expression_for_loop = 261,      /* expression_for_loop  */
  YYSYMBOL_262_13 = 262,                   /* $@13  */
  YYSYMBOL_expression_unsafe = 263,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 264,    /* expression_while_loop  */
  YYSYMBOL_265_14 = 265,                   /* $@14  */
  YYSYMBOL_expression_with = 266,          /* expression_with  */
  YYSYMBOL_267_15 = 267,                   /* $@15  */
  YYSYMBOL_expression_with_alias = 268,    /* expression_with_alias  */
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
  YYSYMBOL_optional_annotation_list_with_emit_semis = 280, /* optional_annotation_list_with_emit_semis  */
  YYSYMBOL_optional_function_argument_list = 281, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 282,   /* optional_function_type  */
  YYSYMBOL_function_name = 283,            /* function_name  */
  YYSYMBOL_das_type_name = 284,            /* das_type_name  */
  YYSYMBOL_optional_template = 285,        /* optional_template  */
  YYSYMBOL_global_function_declaration = 286, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 287, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 288, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 289,     /* function_declaration  */
  YYSYMBOL_290_16 = 290,                   /* $@16  */
  YYSYMBOL_expression_block_finally = 291, /* expression_block_finally  */
  YYSYMBOL_292_17 = 292,                   /* $@17  */
  YYSYMBOL_293_18 = 293,                   /* $@18  */
  YYSYMBOL_expression_block = 294,         /* expression_block  */
  YYSYMBOL_295_19 = 295,                   /* $@19  */
  YYSYMBOL_296_20 = 296,                   /* $@20  */
  YYSYMBOL_expr_call_pipe_no_bracket = 297, /* expr_call_pipe_no_bracket  */
  YYSYMBOL_expression_any = 298,           /* expression_any  */
  YYSYMBOL_299_21 = 299,                   /* $@21  */
  YYSYMBOL_300_22 = 300,                   /* $@22  */
  YYSYMBOL_expressions = 301,              /* expressions  */
  YYSYMBOL_optional_expr_list = 302,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_map_tuple_list = 303, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 304, /* type_declaration_no_options_list  */
  YYSYMBOL_name_in_namespace = 305,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 306,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 307,     /* new_type_declaration  */
  YYSYMBOL_308_23 = 308,                   /* $@23  */
  YYSYMBOL_309_24 = 309,                   /* $@24  */
  YYSYMBOL_expr_new = 310,                 /* expr_new  */
  YYSYMBOL_expression_break = 311,         /* expression_break  */
  YYSYMBOL_expression_continue = 312,      /* expression_continue  */
  YYSYMBOL_expression_return = 313,        /* expression_return  */
  YYSYMBOL_expression_yield = 314,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 315,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 316,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 317,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 318,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 319,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 320, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 321,           /* expression_let  */
  YYSYMBOL_expr_cast = 322,                /* expr_cast  */
  YYSYMBOL_323_25 = 323,                   /* $@25  */
  YYSYMBOL_324_26 = 324,                   /* $@26  */
  YYSYMBOL_325_27 = 325,                   /* $@27  */
  YYSYMBOL_326_28 = 326,                   /* $@28  */
  YYSYMBOL_327_29 = 327,                   /* $@29  */
  YYSYMBOL_328_30 = 328,                   /* $@30  */
  YYSYMBOL_expr_type_decl = 329,           /* expr_type_decl  */
  YYSYMBOL_330_31 = 330,                   /* $@31  */
  YYSYMBOL_331_32 = 331,                   /* $@32  */
  YYSYMBOL_expr_type_info = 332,           /* expr_type_info  */
  YYSYMBOL_expr_list = 333,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 334,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 335,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 336,            /* capture_entry  */
  YYSYMBOL_capture_list = 337,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 338,    /* optional_capture_list  */
  YYSYMBOL_expr_full_block = 339,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 340, /* expr_full_block_assumed_piped  */
  YYSYMBOL_expr_numeric_const = 341,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign_no_bracket = 342,   /* expr_assign_no_bracket  */
  YYSYMBOL_expr_named_call = 343,          /* expr_named_call  */
  YYSYMBOL_expr_method_call_no_bracket = 344, /* expr_method_call_no_bracket  */
  YYSYMBOL_func_addr_name = 345,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 346,           /* func_addr_expr  */
  YYSYMBOL_347_33 = 347,                   /* $@33  */
  YYSYMBOL_348_34 = 348,                   /* $@34  */
  YYSYMBOL_349_35 = 349,                   /* $@35  */
  YYSYMBOL_350_36 = 350,                   /* $@36  */
  YYSYMBOL_expr_field_no_bracket = 351,    /* expr_field_no_bracket  */
  YYSYMBOL_352_37 = 352,                   /* $@37  */
  YYSYMBOL_353_38 = 353,                   /* $@38  */
  YYSYMBOL_expr_call = 354,                /* expr_call  */
  YYSYMBOL_expr = 355,                     /* expr  */
  YYSYMBOL_expr_no_bracket = 356,          /* expr_no_bracket  */
  YYSYMBOL_357_39 = 357,                   /* $@39  */
  YYSYMBOL_358_40 = 358,                   /* $@40  */
  YYSYMBOL_359_41 = 359,                   /* $@41  */
  YYSYMBOL_360_42 = 360,                   /* $@42  */
  YYSYMBOL_361_43 = 361,                   /* $@43  */
  YYSYMBOL_362_44 = 362,                   /* $@44  */
  YYSYMBOL_expr_generator = 363,           /* expr_generator  */
  YYSYMBOL_expr_mtag_no_bracket = 364,     /* expr_mtag_no_bracket  */
  YYSYMBOL_optional_field_annotation = 365, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 366,        /* optional_override  */
  YYSYMBOL_optional_constant = 367,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 368, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 369, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 370, /* structure_variable_declaration  */
  YYSYMBOL_struct_variable_declaration_list = 371, /* struct_variable_declaration_list  */
  YYSYMBOL_372_45 = 372,                   /* $@45  */
  YYSYMBOL_373_46 = 373,                   /* $@46  */
  YYSYMBOL_374_47 = 374,                   /* $@47  */
  YYSYMBOL_function_argument_declaration_no_type = 375, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 376, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 377,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 378,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 379,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 380,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 381,             /* variant_type  */
  YYSYMBOL_variant_type_list = 382,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 383,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 384,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 385, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 386, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 387,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 388,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 389,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 390, /* let_variable_name_with_pos_list  */
  YYSYMBOL_global_let_variable_name_with_pos_list = 391, /* global_let_variable_name_with_pos_list  */
  YYSYMBOL_variable_declaration_list = 392, /* variable_declaration_list  */
  YYSYMBOL_let_variable_declaration = 393, /* let_variable_declaration  */
  YYSYMBOL_global_let_variable_declaration = 394, /* global_let_variable_declaration  */
  YYSYMBOL_optional_shared = 395,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 396, /* optional_public_or_private_variable  */
  YYSYMBOL_global_variable_declaration_list = 397, /* global_variable_declaration_list  */
  YYSYMBOL_398_48 = 398,                   /* $@48  */
  YYSYMBOL_global_let = 399,               /* global_let  */
  YYSYMBOL_400_49 = 400,                   /* $@49  */
  YYSYMBOL_enum_expression = 401,          /* enum_expression  */
  YYSYMBOL_commas = 402,                   /* commas  */
  YYSYMBOL_enum_list = 403,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 404, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 405,             /* single_alias  */
  YYSYMBOL_406_50 = 406,                   /* $@50  */
  YYSYMBOL_alias_declaration = 407,        /* alias_declaration  */
  YYSYMBOL_optional_public_or_private_enum = 408, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 409,                /* enum_name  */
  YYSYMBOL_optional_enum_basic_type_declaration = 410, /* optional_enum_basic_type_declaration  */
  YYSYMBOL_optional_commas = 411,          /* optional_commas  */
  YYSYMBOL_emit_commas = 412,              /* emit_commas  */
  YYSYMBOL_optional_emit_commas = 413,     /* optional_emit_commas  */
  YYSYMBOL_enum_declaration = 414,         /* enum_declaration  */
  YYSYMBOL_415_51 = 415,                   /* $@51  */
  YYSYMBOL_416_52 = 416,                   /* $@52  */
  YYSYMBOL_417_53 = 417,                   /* $@53  */
  YYSYMBOL_optional_structure_parent = 418, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 419,          /* optional_sealed  */
  YYSYMBOL_structure_name = 420,           /* structure_name  */
  YYSYMBOL_class_or_struct = 421,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 422, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 423, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 424,    /* structure_declaration  */
  YYSYMBOL_425_54 = 425,                   /* $@54  */
  YYSYMBOL_426_55 = 426,                   /* $@55  */
  YYSYMBOL_427_56 = 427,                   /* $@56  */
  YYSYMBOL_variable_name_with_pos_list = 428, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 429,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 430, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 431, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 432,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 433,            /* bitfield_bits  */
  YYSYMBOL_bitfield_alias_bits = 434,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_basic_type_declaration = 435, /* bitfield_basic_type_declaration  */
  YYSYMBOL_bitfield_type_declaration = 436, /* bitfield_type_declaration  */
  YYSYMBOL_437_57 = 437,                   /* $@57  */
  YYSYMBOL_438_58 = 438,                   /* $@58  */
  YYSYMBOL_c_or_s = 439,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 440,          /* table_type_pair  */
  YYSYMBOL_dim_list = 441,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 442, /* type_declaration_no_options  */
  YYSYMBOL_optional_expr_list_in_braces = 443, /* optional_expr_list_in_braces  */
  YYSYMBOL_type_declaration_no_options_no_dim = 444, /* type_declaration_no_options_no_dim  */
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
  YYSYMBOL_463_77 = 463,                   /* $@77  */
  YYSYMBOL_464_78 = 464,                   /* $@78  */
  YYSYMBOL_465_79 = 465,                   /* $@79  */
  YYSYMBOL_466_80 = 466,                   /* $@80  */
  YYSYMBOL_467_81 = 467,                   /* $@81  */
  YYSYMBOL_468_82 = 468,                   /* $@82  */
  YYSYMBOL_469_83 = 469,                   /* $@83  */
  YYSYMBOL_470_84 = 470,                   /* $@84  */
  YYSYMBOL_471_85 = 471,                   /* $@85  */
  YYSYMBOL_472_86 = 472,                   /* $@86  */
  YYSYMBOL_type_declaration = 473,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 474,  /* tuple_alias_declaration  */
  YYSYMBOL_475_87 = 475,                   /* $@87  */
  YYSYMBOL_476_88 = 476,                   /* $@88  */
  YYSYMBOL_477_89 = 477,                   /* $@89  */
  YYSYMBOL_478_90 = 478,                   /* $@90  */
  YYSYMBOL_variant_alias_declaration = 479, /* variant_alias_declaration  */
  YYSYMBOL_480_91 = 480,                   /* $@91  */
  YYSYMBOL_481_92 = 481,                   /* $@92  */
  YYSYMBOL_482_93 = 482,                   /* $@93  */
  YYSYMBOL_483_94 = 483,                   /* $@94  */
  YYSYMBOL_bitfield_alias_declaration = 484, /* bitfield_alias_declaration  */
  YYSYMBOL_485_95 = 485,                   /* $@95  */
  YYSYMBOL_486_96 = 486,                   /* $@96  */
  YYSYMBOL_487_97 = 487,                   /* $@97  */
  YYSYMBOL_488_98 = 488,                   /* $@98  */
  YYSYMBOL_make_decl = 489,                /* make_decl  */
  YYSYMBOL_make_decl_no_bracket = 490,     /* make_decl_no_bracket  */
  YYSYMBOL_make_struct_fields = 491,       /* make_struct_fields  */
  YYSYMBOL_make_variant_dim = 492,         /* make_variant_dim  */
  YYSYMBOL_make_struct_single = 493,       /* make_struct_single  */
  YYSYMBOL_make_struct_dim_list = 494,     /* make_struct_dim_list  */
  YYSYMBOL_make_struct_dim_decl = 495,     /* make_struct_dim_decl  */
  YYSYMBOL_optional_make_struct_dim_decl = 496, /* optional_make_struct_dim_decl  */
  YYSYMBOL_use_initializer = 497,          /* use_initializer  */
  YYSYMBOL_make_struct_decl = 498,         /* make_struct_decl  */
  YYSYMBOL_499_99 = 499,                   /* $@99  */
  YYSYMBOL_500_100 = 500,                  /* $@100  */
  YYSYMBOL_501_101 = 501,                  /* $@101  */
  YYSYMBOL_502_102 = 502,                  /* $@102  */
  YYSYMBOL_503_103 = 503,                  /* $@103  */
  YYSYMBOL_504_104 = 504,                  /* $@104  */
  YYSYMBOL_505_105 = 505,                  /* $@105  */
  YYSYMBOL_506_106 = 506,                  /* $@106  */
  YYSYMBOL_507_107 = 507,                  /* $@107  */
  YYSYMBOL_508_108 = 508,                  /* $@108  */
  YYSYMBOL_make_tuple_call = 509,          /* make_tuple_call  */
  YYSYMBOL_510_109 = 510,                  /* $@109  */
  YYSYMBOL_511_110 = 511,                  /* $@110  */
  YYSYMBOL_make_dim_decl = 512,            /* make_dim_decl  */
  YYSYMBOL_513_111 = 513,                  /* $@111  */
  YYSYMBOL_514_112 = 514,                  /* $@112  */
  YYSYMBOL_515_113 = 515,                  /* $@113  */
  YYSYMBOL_516_114 = 516,                  /* $@114  */
  YYSYMBOL_517_115 = 517,                  /* $@115  */
  YYSYMBOL_518_116 = 518,                  /* $@116  */
  YYSYMBOL_519_117 = 519,                  /* $@117  */
  YYSYMBOL_520_118 = 520,                  /* $@118  */
  YYSYMBOL_521_119 = 521,                  /* $@119  */
  YYSYMBOL_522_120 = 522,                  /* $@120  */
  YYSYMBOL_expr_map_tuple_list = 523,      /* expr_map_tuple_list  */
  YYSYMBOL_push_table_nesting = 524,       /* push_table_nesting  */
  YYSYMBOL_make_table_decl = 525,          /* make_table_decl  */
  YYSYMBOL_make_table_call = 526,          /* make_table_call  */
  YYSYMBOL_array_comprehension_where = 527, /* array_comprehension_where  */
  YYSYMBOL_optional_comma = 528,           /* optional_comma  */
  YYSYMBOL_table_comprehension = 529,      /* table_comprehension  */
  YYSYMBOL_array_comprehension = 530       /* array_comprehension  */
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
#define YYLAST   9456

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  208
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  323
/* YYNRULES -- Number of rules.  */
#define YYNRULES  947
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1691

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   435


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
       2,     2,     2,   190,     2,   207,   202,   186,   179,     2,
     200,   201,   184,   183,   173,   182,   197,   185,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   176,   204,
     180,   174,   181,   175,   203,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   198,     2,   199,   178,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   205,   177,   206,   189,     2,     2,     2,
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
     165,   166,   167,   168,   169,   170,   171,   172,   187,   188,
     191,   192,   193,   194,   195,   196
};

#if DAS2_YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   566,   566,   567,   572,   573,   574,   575,   576,   577,
     578,   579,   580,   581,   582,   583,   584,   588,   589,   593,
     594,   598,   604,   605,   606,   610,   611,   615,   616,   620,
     639,   640,   641,   642,   646,   647,   651,   652,   656,   657,
     657,   661,   666,   675,   690,   706,   711,   719,   719,   758,
     772,   776,   779,   783,   787,   791,   795,   801,   810,   813,
     819,   820,   824,   828,   829,   833,   836,   842,   848,   851,
     857,   858,   862,   863,   867,   868,   872,   873,   873,   877,
     877,   886,   887,   891,   892,   898,   899,   900,   901,   902,
     906,   907,   911,   912,   916,   918,   916,   930,   930,   938,
     940,   938,   952,   952,   960,   962,   960,   973,   980,   987,
     992,  1001,  1009,  1015,  1023,  1033,  1033,  1042,  1050,  1050,
    1063,  1063,  1075,  1079,  1086,  1087,  1088,  1089,  1090,  1091,
    1095,  1100,  1108,  1109,  1110,  1114,  1115,  1116,  1117,  1118,
    1119,  1120,  1121,  1122,  1128,  1131,  1137,  1140,  1146,  1147,
    1148,  1149,  1153,  1171,  1194,  1197,  1207,  1222,  1237,  1252,
    1255,  1262,  1266,  1273,  1274,  1278,  1279,  1283,  1284,  1285,
    1289,  1292,  1296,  1303,  1307,  1308,  1309,  1310,  1311,  1312,
    1313,  1314,  1315,  1316,  1317,  1318,  1319,  1320,  1321,  1322,
    1323,  1324,  1325,  1326,  1327,  1328,  1329,  1330,  1331,  1332,
    1333,  1334,  1335,  1336,  1337,  1338,  1339,  1340,  1341,  1342,
    1343,  1344,  1345,  1346,  1347,  1348,  1349,  1350,  1351,  1352,
    1353,  1354,  1355,  1356,  1357,  1358,  1359,  1360,  1361,  1362,
    1363,  1364,  1365,  1366,  1367,  1368,  1369,  1370,  1371,  1372,
    1373,  1374,  1375,  1376,  1377,  1378,  1379,  1380,  1381,  1382,
    1383,  1384,  1385,  1386,  1387,  1388,  1389,  1390,  1391,  1392,
    1393,  1394,  1398,  1399,  1400,  1401,  1402,  1403,  1404,  1405,
    1406,  1407,  1408,  1409,  1410,  1411,  1412,  1413,  1414,  1415,
    1416,  1417,  1418,  1419,  1420,  1421,  1422,  1426,  1427,  1431,
    1450,  1451,  1452,  1456,  1462,  1462,  1479,  1482,  1484,  1482,
    1492,  1494,  1492,  1509,  1518,  1527,  1540,  1541,  1542,  1543,
    1544,  1545,  1546,  1547,  1548,  1549,  1550,  1551,  1552,  1553,
    1554,  1555,  1556,  1557,  1558,  1559,  1561,  1559,  1576,  1581,
    1587,  1593,  1594,  1598,  1599,  1603,  1607,  1614,  1615,  1626,
    1630,  1633,  1641,  1641,  1641,  1644,  1650,  1653,  1657,  1661,
    1668,  1675,  1681,  1685,  1689,  1692,  1695,  1703,  1706,  1714,
    1720,  1721,  1722,  1726,  1727,  1731,  1732,  1736,  1741,  1749,
    1755,  1767,  1770,  1773,  1779,  1779,  1779,  1782,  1782,  1782,
    1787,  1787,  1787,  1795,  1795,  1795,  1801,  1811,  1822,  1837,
    1840,  1843,  1846,  1852,  1853,  1860,  1871,  1872,  1873,  1877,
    1878,  1879,  1880,  1881,  1885,  1890,  1898,  1899,  1903,  1910,
    1914,  1920,  1921,  1922,  1923,  1924,  1925,  1926,  1930,  1931,
    1932,  1933,  1934,  1935,  1936,  1937,  1938,  1939,  1940,  1941,
    1942,  1943,  1944,  1945,  1946,  1947,  1948,  1949,  1950,  1954,
    1960,  1971,  1977,  1988,  1992,  1999,  2002,  2002,  2002,  2007,
    2007,  2007,  2020,  2024,  2028,  2034,  2042,  2050,  2056,  2064,
    2064,  2064,  2071,  2075,  2084,  2092,  2100,  2104,  2107,  2115,
    2116,  2117,  2124,  2125,  2126,  2127,  2128,  2129,  2130,  2131,
    2132,  2133,  2134,  2135,  2136,  2137,  2138,  2139,  2140,  2141,
    2142,  2143,  2144,  2145,  2146,  2147,  2148,  2149,  2150,  2151,
    2152,  2153,  2154,  2155,  2156,  2157,  2158,  2159,  2165,  2166,
    2167,  2168,  2169,  2182,  2191,  2192,  2193,  2194,  2195,  2196,
    2197,  2198,  2199,  2200,  2201,  2202,  2203,  2204,  2207,  2207,
    2207,  2210,  2215,  2219,  2223,  2223,  2223,  2228,  2231,  2235,
    2235,  2235,  2240,  2243,  2244,  2245,  2246,  2247,  2248,  2249,
    2250,  2251,  2253,  2257,  2258,  2263,  2269,  2275,  2284,  2287,
    2290,  2299,  2300,  2301,  2302,  2303,  2304,  2305,  2309,  2313,
    2317,  2321,  2325,  2329,  2333,  2337,  2341,  2348,  2349,  2353,
    2354,  2355,  2359,  2360,  2364,  2365,  2366,  2370,  2371,  2375,
    2387,  2390,  2391,  2395,  2395,  2414,  2413,  2428,  2427,  2444,
    2456,  2465,  2475,  2476,  2477,  2478,  2479,  2483,  2486,  2495,
    2496,  2500,  2503,  2507,  2520,  2529,  2530,  2534,  2537,  2541,
    2554,  2555,  2559,  2565,  2571,  2580,  2583,  2590,  2593,  2599,
    2600,  2601,  2605,  2606,  2610,  2617,  2622,  2631,  2637,  2648,
    2655,  2664,  2667,  2670,  2677,  2680,  2685,  2696,  2699,  2704,
    2716,  2717,  2721,  2722,  2723,  2727,  2730,  2733,  2733,  2753,
    2756,  2756,  2774,  2779,  2787,  2788,  2792,  2795,  2808,  2825,
    2826,  2827,  2832,  2832,  2858,  2862,  2863,  2864,  2868,  2878,
    2881,  2887,  2888,  2892,  2893,  2897,  2898,  2902,  2904,  2909,
    2902,  2925,  2926,  2930,  2931,  2935,  2941,  2942,  2943,  2944,
    2948,  2949,  2950,  2954,  2957,  2963,  2965,  2970,  2963,  2991,
    2998,  3003,  3012,  3018,  3029,  3030,  3031,  3032,  3033,  3034,
    3035,  3036,  3037,  3038,  3039,  3040,  3041,  3042,  3043,  3044,
    3045,  3046,  3047,  3048,  3049,  3050,  3051,  3052,  3053,  3054,
    3055,  3059,  3060,  3061,  3062,  3063,  3064,  3065,  3066,  3070,
    3081,  3085,  3092,  3104,  3111,  3117,  3126,  3131,  3141,  3151,
    3161,  3174,  3175,  3176,  3177,  3178,  3182,  3186,  3186,  3186,
    3200,  3201,  3205,  3209,  3216,  3220,  3224,  3228,  3235,  3238,
    3256,  3257,  3261,  3262,  3263,  3264,  3265,  3265,  3265,  3269,
    3274,  3281,  3288,  3288,  3295,  3295,  3302,  3306,  3310,  3315,
    3320,  3325,  3330,  3334,  3338,  3343,  3347,  3351,  3356,  3356,
    3356,  3362,  3369,  3369,  3369,  3374,  3374,  3374,  3380,  3380,
    3380,  3385,  3390,  3390,  3390,  3395,  3395,  3395,  3404,  3409,
    3409,  3409,  3414,  3414,  3414,  3423,  3428,  3428,  3428,  3433,
    3433,  3433,  3442,  3442,  3442,  3448,  3448,  3448,  3457,  3460,
    3471,  3487,  3489,  3494,  3499,  3487,  3525,  3527,  3532,  3538,
    3525,  3564,  3566,  3571,  3576,  3564,  3617,  3618,  3619,  3620,
    3621,  3622,  3623,  3627,  3628,  3629,  3630,  3631,  3635,  3642,
    3649,  3655,  3661,  3668,  3675,  3681,  3690,  3693,  3699,  3707,
    3712,  3719,  3724,  3730,  3731,  3735,  3736,  3740,  3740,  3740,
    3748,  3748,  3748,  3755,  3755,  3755,  3766,  3766,  3766,  3773,
    3773,  3773,  3784,  3790,  3790,  3790,  3804,  3823,  3823,  3823,
    3833,  3833,  3833,  3847,  3847,  3847,  3861,  3870,  3870,  3870,
    3890,  3897,  3897,  3897,  3907,  3910,  3921,  3927,  3950,  3958,
    3978,  4003,  4004,  4008,  4009,  4014,  4017,  4027
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
  "\"<-\"", "\"??\"", "\"?.\"", "\"?[\"", "\"<|\"", "\"|>\"", "\":=\"",
  "\"<<<\"", "\">>>\"", "\"<<<=\"", "\">>>=\"", "\"=>\"", "\"::\"",
  "\"&&\"", "\"||\"", "\"^^\"", "\"&&=\"", "\"||=\"", "\"^^=\"", "\"..\"",
  "\"$$\"", "\"$i\"", "\"$v\"", "\"$b\"", "\"$a\"", "\"$t\"", "\"$c\"",
  "\"$f\"", "\"...\"", "\"integer constant\"", "\"long integer constant\"",
  "\"unsigned integer constant\"", "\"unsigned long integer constant\"",
  "\"unsigned int8 constant\"", "\"floating point constant\"",
  "\"double constant\"", "\"name\"", "\"new line, comma\"",
  "\"new line, semicolon\"", "\"start of the string\"", "STRING_CHARACTER",
  "STRING_CHARACTER_ESC", "\"end of the string\"", "\"{\"", "\"}\"",
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
  "require_module_name", "require_module", "is_public_module",
  "expect_declaration", "expect_list", "expect_error", "expression_label",
  "expression_goto", "elif_or_static_elif", "emit_semis",
  "optional_emit_semis", "expression_else", "$@3", "$@4",
  "if_or_static_if", "expression_else_one_liner",
  "expression_if_one_liner", "semis", "optional_semis",
  "expression_if_block", "$@5", "$@6", "$@7", "expression_else_block",
  "$@8", "$@9", "$@10", "expression_if_then_else", "$@11", "$@12",
  "expression_if_then_else_oneliner", "for_variable_name_with_pos_list",
  "expression_for_loop", "$@13", "expression_unsafe",
  "expression_while_loop", "$@14", "expression_with", "$@15",
  "expression_with_alias", "annotation_argument_value",
  "annotation_argument_value_list", "annotation_argument_name",
  "annotation_argument", "annotation_argument_list",
  "metadata_argument_list", "annotation_declaration_name",
  "annotation_declaration_basic", "annotation_declaration",
  "annotation_list", "optional_annotation_list",
  "optional_annotation_list_with_emit_semis",
  "optional_function_argument_list", "optional_function_type",
  "function_name", "das_type_name", "optional_template",
  "global_function_declaration", "optional_public_or_private_function",
  "function_declaration_header", "function_declaration", "$@16",
  "expression_block_finally", "$@17", "$@18", "expression_block", "$@19",
  "$@20", "expr_call_pipe_no_bracket", "expression_any", "$@21", "$@22",
  "expressions", "optional_expr_list", "optional_expr_map_tuple_list",
  "type_declaration_no_options_list", "name_in_namespace",
  "expression_delete", "new_type_declaration", "$@23", "$@24", "expr_new",
  "expression_break", "expression_continue", "expression_return",
  "expression_yield", "expression_try_catch", "kwd_let_var_or_nothing",
  "kwd_let", "optional_in_scope", "tuple_expansion",
  "tuple_expansion_variable_declaration", "expression_let", "expr_cast",
  "$@25", "$@26", "$@27", "$@28", "$@29", "$@30", "expr_type_decl", "$@31",
  "$@32", "expr_type_info", "expr_list", "block_or_simple_block",
  "block_or_lambda", "capture_entry", "capture_list",
  "optional_capture_list", "expr_full_block",
  "expr_full_block_assumed_piped", "expr_numeric_const",
  "expr_assign_no_bracket", "expr_named_call",
  "expr_method_call_no_bracket", "func_addr_name", "func_addr_expr",
  "$@33", "$@34", "$@35", "$@36", "expr_field_no_bracket", "$@37", "$@38",
  "expr_call", "expr", "expr_no_bracket", "$@39", "$@40", "$@41", "$@42",
  "$@43", "$@44", "expr_generator", "expr_mtag_no_bracket",
  "optional_field_annotation", "optional_override", "optional_constant",
  "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "struct_variable_declaration_list", "$@45", "$@46", "$@47",
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
  "global_variable_declaration_list", "$@48", "global_let", "$@49",
  "enum_expression", "commas", "enum_list",
  "optional_public_or_private_alias", "single_alias", "$@50",
  "alias_declaration", "optional_public_or_private_enum", "enum_name",
  "optional_enum_basic_type_declaration", "optional_commas", "emit_commas",
  "optional_emit_commas", "enum_declaration", "$@51", "$@52", "$@53",
  "optional_structure_parent", "optional_sealed", "structure_name",
  "class_or_struct", "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@54", "$@55", "$@56", "variable_name_with_pos_list",
  "basic_type_declaration", "enum_basic_type_declaration",
  "structure_type_declaration", "auto_type_declaration", "bitfield_bits",
  "bitfield_alias_bits", "bitfield_basic_type_declaration",
  "bitfield_type_declaration", "$@57", "$@58", "c_or_s", "table_type_pair",
  "dim_list", "type_declaration_no_options",
  "optional_expr_list_in_braces", "type_declaration_no_options_no_dim",
  "$@59", "$@60", "$@61", "$@62", "$@63", "$@64", "$@65", "$@66", "$@67",
  "$@68", "$@69", "$@70", "$@71", "$@72", "$@73", "$@74", "$@75", "$@76",
  "$@77", "$@78", "$@79", "$@80", "$@81", "$@82", "$@83", "$@84", "$@85",
  "$@86", "type_declaration", "tuple_alias_declaration", "$@87", "$@88",
  "$@89", "$@90", "variant_alias_declaration", "$@91", "$@92", "$@93",
  "$@94", "bitfield_alias_declaration", "$@95", "$@96", "$@97", "$@98",
  "make_decl", "make_decl_no_bracket", "make_struct_fields",
  "make_variant_dim", "make_struct_single", "make_struct_dim_list",
  "make_struct_dim_decl", "optional_make_struct_dim_decl",
  "use_initializer", "make_struct_decl", "$@99", "$@100", "$@101", "$@102",
  "$@103", "$@104", "$@105", "$@106", "$@107", "$@108", "make_tuple_call",
  "$@109", "$@110", "make_dim_decl", "$@111", "$@112", "$@113", "$@114",
  "$@115", "$@116", "$@117", "$@118", "$@119", "$@120",
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

#define YYPACT_NINF (-1535)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-840)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1535,    68, -1535, -1535,    37,   -52,   296,   471, -1535,    23,
   -1535, -1535, -1535, -1535,   297,   219, -1535, -1535, -1535, -1535,
      65,    65,    65, -1535,   345, -1535,    47, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535,    89, -1535,    14,
      94,    24, -1535,    73, -1535,   186,   121,    21, -1535, -1535,
   -1535,   194,    65, -1535, -1535,    47,   471,   471,   471,   207,
     254, -1535, -1535, -1535, -1535,   219,   219,   219,   206, -1535,
     756,   272, -1535, -1535, -1535, -1535,   321, -1535,   661, -1535,
     583,   135,    37,   311,   -52,   296,   296,   312,   296,   353,
   -1535,   441,   462, -1535, -1535, -1535,   656,   518,   567,   572,
   -1535,   605,   512, -1535, -1535,   -68,    37,   219,   219,   219,
     219,   361, -1535,   696,   706,   614,   625,   714, -1535, -1535,
     575, -1535, -1535, -1535, -1535, -1535,   747,   131,   579, -1535,
   -1535, -1535, -1535,   312,   312,   312,   731, -1535, -1535,   617,
   -1535, -1535,   603,   624,   361,   361, -1535, -1535,   657, -1535,
     204, -1535,   443,   682,   756, -1535,   662, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535,   689, -1535, -1535, -1535, -1535, -1535,
   -1535,   691, -1535, -1535, -1535,   833, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535,   230,   738, -1535,  8265,   779, -1535,   574,
     739, -1535, -1535, -1535, -1535, -1535,  9084, -1535,   729,   802,
     139,    37,   708,   749, -1535, -1535, -1535,   131, -1535, -1535,
     737,   748,   753,   719,   761,   763, -1535, -1535, -1535,   755,
   -1535, -1535, -1535, -1535, -1535,   504, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535,   765, -1535, -1535,
   -1535,   767,   768, -1535, -1535, -1535, -1535,   777,   778,   764,
     297,   -87, -1535, -1535, -1535, -1535,  4016,   740,   793, -1535,
   -1535, -1535, -1535, -1535, -1535,   799, -1535,   769,   773,  8453,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535,   938,   947, -1535,   784, -1535,
     361,   912,   739, -1535,   823,   361, -1535, -1535,   691,   361,
      37, -1535,   558, -1535, -1535, -1535, -1535, -1535,  7266, -1535,
   -1535,   825,   809,   119,   197,   198, -1535, -1535,  7266,    61,
   -1535,  5165, -1535, -1535, -1535,    17, -1535, -1535, -1535,    13,
   -1535,  5356,   795,   790, -1535,   789, -1535, -1535,  9222,  9261,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
     835,   808, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535,   987, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535,   859,   826, -1535,
   -1535,   -25,   -47,   885, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535,   821,   851, -1535,   419, -1535,   361,   865,
    8265, -1535,   -22,  8265,  8265,  8265,   849,   850, -1535, -1535,
     136,   297,   852,    60, -1535,   113,   834,   853,   855,   836,
     857,   839,   168,   860, -1535,   184,    28,   862,  8024,  8024,
     844,   845,   848,   854,   856,   858, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535,  8024,  8024,  8024,  8024,  8024,
    3637,  4019, -1535,   846, -1535, -1535, -1535, -1535,   866, -1535,
   -1535, -1535, -1535,   869, -1535, -1535, -1535,   644, -1535,   644,
     644,   867,  1439, -1535, -1535,   872, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535,  8265,  8265,   874,   870,  8265,   784,
    8265,   784,  8265,   784,  8358,   887,   875, -1535,  5165, -1535,
    8265,  7266,   876,   879, -1535, -1535, -1535, -1535, -1535,   880,
   -1535, -1535,   881,  5547, -1535,  4016, -1535,  8358,   887, -1535,
   -1535, -1535, -1535, -1535, -1535,  9293,  1575,  1342,   873, -1535,
      82,   877,    86,   886,  8265,  8265, -1535,  7646, -1535,   883,
   -1535, -1535,   297, -1535,   491,   878,  1010,   588, -1535, -1535,
   -1535,   394, -1535, -1535, -1535,  7266,   516,   569,   905,   276,
   -1535, -1535, -1535, -1535,   889, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535,   323, -1535,   915,   916,   918, -1535,
    5165,  8265,  7266,  7266, -1535, -1535,  7266, -1535,  7266, -1535,
    5165, -1535, -1535,  5165,   927, -1535,  8265,   610,   610,  7266,
    7266,  7266,  7266,  7266,  7266,   674,   610,   610,    -8,   610,
     610,   910,  1044,   920,   911,   331,   879,   941,   921,   372,
     361,  2103,   219,  1113,   922, -1535,   869, -1535, -1535, -1535,
   -1535,  8447,  8938,  8024,  8024, -1535, -1535,  8024,  8024,  8024,
    8024,   960,  8024,   -53,  7266,  8024,  8024,  8024,  8024,  7266,
    8024,  8024,  8024,  8024,  7835,  8024,  8024,  8024,  8024,  8024,
    8024,  8024,  8024,  8024,  8024,  9133,  7266,  4210,   589,   604,
   -1535, -1535,   961,   621,   -47,   635,   -47,   636,   -47,   144,
   -1535,   398,   793,   950, -1535,   534, -1535,  8265,   879,   544,
     793, -1535, -1535,  5738, -1535, -1535, -1535, -1535,   928,   965,
   -1535,    65, -1535,    65, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535,  7266, -1535, -1535,   451,   -64,   -64,   -64, -1535,
     793,   793,  8024,  8529, -1535,   972, -1535, -1535, -1535, -1535,
    7266,   973,   975,  8265,   -22, -1535,  7266,    65, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535,  8265,  8265,  8265,  8265,  3828,
     976,  7266,  8265, -1535, -1535, -1535,  8265,   879,  1022, -1535,
     964,   939,  8265,  8265,   940,  8265,   943,  8265,   879,  8265,
    8358,   879, -1535,   887,    98,   944,   946,   948,   951,   962,
     963, -1535,  7266,   480,   -28,   969, -1535,  7266, -1535,  7266,
   -1535,  7266,   981,   -51, -1535, -1535,   970,   985,   199, -1535,
   -1535,  5929,   149,  3446, -1535,   249,   988,   305,   990,   784,
   -1535,  1899,  1113,   971,   991, -1535, -1535,   982,   992, -1535,
   -1535,   425,   425,  1049,  1049,   974,   974,   993,   526,   994,
   -1535,   977,   120,   120,   872,   425,   425,  8529, -1535, -1535,
    8721,  8681,  8757,  8529,  9027,  1247,  8797,  8873,  8949,  1049,
    1049,   559,   559,   526,   526,   526,   -48,  7266,   998,   999,
     281,  7266,  1160,  1002,   989, -1535,   250, -1535, -1535, -1535,
     -90, -1535,  1024, -1535,  1027, -1535,  1028,  8265, -1535,  8358,
    8265, -1535,   887,   622,  1012,  1011,  8265,  7266, -1535, -1535,
    1040,   442, -1535,  8170, -1535,    58, -1535,  1014,  1016,  1130,
   -1535, -1535,   289, -1535, -1535, -1535,  8605,  2349,  1043, -1535,
     442,    36,  1017, -1535,  1176,   394,  7266,    65, -1535, -1535,
   -1535, -1535,   793,   306,  1082,   637,   564,   251,  1019,  1020,
     745,  1021,   642,  8265,  8358,   887,  1167,  1023,  1025,  8265,
    7266,  1029, -1535,  1179,  1267, -1535,  1467, -1535,  2033,  1035,
    2412,   758,  1036,  8265,   771,  1113, -1535, -1535, -1535, -1535,
   -1535,  1038,  1065,  1042,  1200,  1085,    25,   -28,  1048, -1535,
   -1535, -1535,  1045,    41,  7266,  7266,  8265,   784,  1051,  1046,
     964,   188, -1535,  1052,   316,  6120, -1535, -1535, -1535,   258,
     -47, -1535,  6311, -1535, -1535,  6502,  1092,  1097, -1535,    65,
    1088,  6693,   -84,  6884, -1535, -1535, -1535,    65,    65,  1255,
   -1535,   824, -1535, -1535,  1254, -1535, -1535,  1261, -1535,  1230,
      65, -1535,    65,    65,    65,    65,    65, -1535,  1207, -1535,
      65,  1692,   784, -1535,  7266, -1535,  7266,  4401,  7266, -1535,
    1094,  1075, -1535, -1535,  8024,  1076, -1535,  1078,  7266,  4592,
    1079, -1535,  1083, -1535,  4783, -1535,  5738, -1535, -1535, -1535,
    1115, -1535,  1118, -1535, -1535, -1535, -1535, -1535, -1535,   793,
   -1535, -1535,   793, -1535, -1535,  1011, -1535, -1535,   793, -1535,
    7266, -1535,   501, -1535, -1535, -1535,  1077, -1535,  1084, -1535,
    7266,  1121,  1122,  8265, -1535,  7266,  1086,  7266,   519, -1535,
    1124, -1535, -1535,  1280,   691, -1535,  1131, -1535,  7266,    65,
   -1535, -1535, -1535, -1535,  1095, -1535, -1535, -1535,  1093,  1134,
   -1535, -1535,  2422,   786,   804, -1535, -1535,  7266,  2433, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,  3379,
   -1535,   -73,  4974, -1535,  1127,  7266,  1136, -1535,   301,  5165,
     147,    43,   287,  7266,  7266,  7266,  1103,  1104,  3634,   -47,
     -28, -1535, -1535, -1535,   -51,  1105,  3446,  1145,  1146,  1110,
    1148,  1150, -1535,   313,   361,  7266, -1535,  1301,  7266, -1535,
    1142,  1143, -1535,  1144,  1163, -1535, -1535,  7266, -1535, -1535,
   -1535, -1535,  1123, -1535, -1535,  1125,  1126,  1129,  1133, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535,    84, -1535,  8024,  8024,
    8024,  8024,  8024,  8024,  8024,  8024,  8024,  8024,  7266,  8024,
    8024,  8024,  8024,  8024,  8024,  8024,   -47,  8265,  1120,  8265,
    1135, -1535,   335,  1137, -1535,  7266,  8605,  7266, -1535,  1139,
    3446, -1535,   337,  7266, -1535, -1535, -1535,   339, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535,  1156, -1535,  1116, -1535,
   -1535,  1149, -1535,  1281,     7, -1535,  1294, -1535, -1535,  1128,
    1161,   721,  1276,    65, -1535,    65, -1535,  1147,  1152, -1535,
   -1535,  7266,  1164, -1535, -1535, -1535, -1535,  1153,  1154,  1157,
    8024,  8024,  8024,  1159,  1274,  1168, -1535,  1169,  7075, -1535,
   -1535,   357, -1535, -1535,  1155, -1535,  1220, -1535,   359,  1341,
    1085,  5165,  7266,  7266,  1189, -1535, -1535, -1535, -1535, -1535,
    1211,    44, -1535,   346, -1535, -1535,  1231, -1535, -1535,   258,
   -1535,   885, -1535, -1535, -1535,  8265,  7266, -1535, -1535, -1535,
   -1535,  2626,  7266,  7266,   -28,  7266,  7266,  1085, -1535, -1535,
   -1535,  1439,  1439,  1439,  1439,  1439,  1439,  1439,  1439,  1439,
    1439,  1439, -1535, -1535,  1439,  1439,  1439,  1439,  1439,  1439,
    1439,   361,  3825, -1535,   652, -1535, -1535, -1535,  8265,  1195,
    1196, -1535,   414, -1535,  1197, -1535,  7266, -1535, -1535,  1237,
    7266, -1535, -1535, -1535,  8265, -1535, -1535,   530, -1535,    31,
   -1535, -1535,  1274,  1274,  1202,  1204,  1206,  1208,  1209,  5165,
   -1535,  7266,  1049,  1049,  1049,  5165, -1535, -1535,  1274,  1210,
    1274, -1535,  1212, -1535, -1535,  1245, -1535, -1535,  1213,  1251,
     377,   388, -1535, -1535,   363,   488, -1535,  5165,  1214,  1215,
   -1535, -1535, -1535,   793, -1535,  1228,  1217,  1218,    46,  1234,
    1235,   391,   -81,   885, -1535, -1535,   655, -1535, -1535,  1238,
   -1535, -1535, -1535, -1535,  1233,   329,  1407,    31, -1535, -1535,
     721,   122,   122, -1535,  7266,  1274,  1274,   564,  1240,  1242,
     879,   122,  1274,   564, -1535, -1535,  7266, -1535, -1535,  1243,
    7266,  7266, -1535,   488,   392, -1535, -1535,  1294,  1446,   361,
    5165,   361,   361,   535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535,  1407,   451,   564,  1286,
    1290, -1535,  1263,  1264,  1266,   122,   122,  1286,  1269, -1535,
   -1535,  1270,  1271,   564,  1272,  1273,  7266, -1535, -1535, -1535,
    1275, -1535,  7457,    65, -1535,   393, -1535, -1535,  8265,   -22,
   -1535,  2832,  9084, -1535, -1535, -1535, -1535,   399,  1268, -1535,
   -1535, -1535, -1535,  1277,  1279, -1535, -1535, -1535,  1282, -1535,
    1423,  1285,  1273,  7266, -1535, -1535, -1535, -1535, -1535,  1439,
   -1535,  1283,   361, -1535, -1535,   797,  7266,  1287,    65,  9084,
   -1535,   564, -1535, -1535, -1535,  7266, -1535,  1293,  1273, -1535,
     647,  7457, -1535,  7266,    65, -1535, -1535,   361,   411, -1535,
   -1535,  1288, -1535,   361, -1535, -1535,  1289, -1535,    65, -1535,
      65, -1535, -1535, -1535, -1535,  3038, -1535,  7266, -1535, -1535,
   -1535,  1291,  1296,  1295,  1294, -1535, -1535,  7457,   361, -1535,
   -1535,    65, -1535,  3244, -1535,  1296,  1292,   647,  1294, -1535,
   -1535
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   165,     1,   363,     0,     0,     0,   669,   364,     0,
     861,   851,   856,    20,     0,     0,    19,    16,    15,     3,
       0,     0,     0,     8,   705,     7,   650,     6,    11,     5,
       4,    13,    12,    14,   133,   134,   132,   142,   144,    49,
      65,    62,    63,     0,    51,     0,     0,    60,    50,   671,
     670,     0,     0,    26,    25,   650,   669,   669,   669,     0,
     337,    47,   149,   150,   151,     0,     0,     0,   152,   154,
     161,     0,   148,    21,    10,     9,   287,   687,     0,   651,
     652,     0,     0,     0,     0,     0,     0,    52,     0,     0,
      61,     0,     0,    58,   672,   674,    22,     0,     0,     0,
     339,     0,     0,   160,   155,     0,     0,     0,     0,     0,
       0,    74,   288,   290,   675,   697,   696,   700,   654,   653,
     660,   140,   141,   138,   139,   136,     0,     0,     0,   135,
     145,    66,    64,    54,    55,    53,    60,    57,    56,     0,
      23,    24,    27,   761,    74,    74,   338,    45,    48,   159,
       0,   156,   157,   158,   162,    72,    75,   166,   292,   291,
     294,   289,   677,   676,     0,   699,   698,   702,   701,   706,
     655,   577,    30,    31,    35,     0,   128,   129,   126,   127,
     125,   124,   130,     0,     0,    59,     0,     0,    29,     0,
     685,   852,   857,    46,   153,    73,     0,   678,   679,   693,
     657,     0,   578,     0,    32,    33,    34,     0,   143,   137,
       0,     0,     0,     0,     0,     0,   714,   734,   715,   750,
     716,   720,   721,   722,   723,   740,   727,   728,   729,   730,
     731,   732,   733,   735,   736,   737,   738,   821,   719,   726,
     739,   828,   835,   717,   724,   718,   725,     0,     0,     0,
       0,   749,   782,   785,   783,   784,   848,   778,   673,    28,
     764,   765,   762,   763,   683,   686,   862,     0,     0,     0,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,     0,     0,   173,   167,   261,
      74,     0,   685,   694,     0,    74,   659,   656,   577,    74,
       0,   639,   632,   661,   131,   786,   812,   815,     0,   818,
     808,     0,     0,   822,   829,   836,   842,   845,     0,   780,
     792,   331,   798,   803,   797,     0,   811,   807,   800,     0,
     802,     0,   779,     0,   684,     0,   853,   858,   252,   253,
     250,   176,   177,   179,   178,   180,   181,   182,   183,   209,
     210,   207,   208,   200,   211,   212,   201,   198,   199,   251,
     234,     0,   249,   213,   214,   215,   216,   187,   188,   189,
     184,   185,   186,   197,     0,   203,   204,   202,   195,   196,
     191,   190,   192,   193,   194,   175,   174,   233,     0,   205,
     206,   577,   170,   300,   741,   744,   747,   748,   742,   745,
     743,   746,   680,     0,   691,   707,     0,   146,    74,     0,
       0,   633,     0,     0,     0,     0,     0,     0,   478,   479,
       0,     0,     0,     0,   472,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   740,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   566,   411,   413,   412,
     414,   415,   416,   417,    41,     0,     0,     0,     0,     0,
     331,     0,   396,   397,   936,   476,   475,   553,   473,   546,
     545,   544,   543,   163,   549,   474,   548,   547,   520,   480,
     521,     0,   469,   525,   481,     0,   477,   873,   875,   874,
     470,   877,   876,   471,     0,     0,     0,   767,     0,   167,
       0,   167,     0,   167,     0,     0,     0,   794,     0,   791,
       0,     0,     0,   943,   389,   805,   806,   799,   801,     0,
     804,   775,     0,     0,   850,   849,   863,   611,   617,   254,
     256,   255,   257,   248,   232,   258,   235,   217,     0,   168,
     362,   602,   603,     0,     0,     0,   293,     0,   393,     0,
     295,   688,     0,   695,     0,     0,   634,   632,   658,   147,
     640,     0,   630,   631,   629,     0,     0,     0,     0,   772,
     897,   900,   342,   749,   346,   345,   351,   866,   872,   867,
     868,   869,   871,   870,     0,   383,     0,     0,     0,   927,
       0,     0,     0,     0,   374,   377,     0,   380,     0,   931,
       0,   909,   913,     0,     0,   903,     0,   508,   509,     0,
       0,     0,     0,     0,     0,     0,   485,   484,   522,   483,
     482,     0,     0,     0,     0,   337,   943,   943,     0,   398,
      74,     0,     0,   406,   397,   328,   163,   304,   305,   303,
     789,     0,     0,     0,     0,   510,   511,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   459,     0,     0,     0,     0,
     751,   766,     0,     0,   170,     0,   170,     0,   170,   337,
     609,     0,   607,     0,   615,     0,   752,     0,   943,     0,
     335,   390,   790,   944,   332,   796,   774,   777,     0,   756,
     612,    92,   618,    92,   259,   260,   237,   238,   240,   239,
     241,   242,   243,   244,   236,   245,   246,   247,   221,   222,
     224,   223,   225,   226,   227,   228,   219,   220,   229,   230,
     231,   218,     0,   360,   361,     0,   577,   577,   577,   169,
     172,   171,     0,   394,   328,   666,   692,   703,   590,   708,
       0,     0,     0,     0,     0,   647,     0,     0,   787,   813,
     816,    18,    17,   770,   771,     0,     0,     0,     0,   895,
       0,     0,     0,   917,   920,   923,     0,   943,     0,   934,
     943,     0,     0,     0,     0,     0,     0,     0,   943,     0,
       0,   943,   906,     0,     0,     0,     0,     0,     0,     0,
       0,    44,     0,    42,     0,     0,   916,     0,   621,     0,
     620,     0,     0,   944,   888,   513,     0,     0,   446,   443,
     445,   333,     0,   331,   462,     0,     0,     0,     0,   167,
     398,     0,   406,     0,     0,   532,   531,     0,     0,   533,
     537,   486,   487,   499,   500,   497,   498,     0,   526,     0,
     518,     0,   550,   551,   552,   488,   489,   555,   556,   557,
     504,   505,   506,   507,     0,     0,   502,   503,   501,   495,
     496,   491,   490,   492,   493,   494,     0,     0,     0,   452,
       0,     0,     0,     0,     0,   467,     0,   819,   809,   753,
       0,   823,     0,   830,     0,   837,     0,     0,   843,     0,
       0,   846,     0,     0,     0,   780,     0,     0,   391,   776,
     757,   681,    90,    93,   854,    93,   859,     0,     0,   709,
     599,   600,   622,   604,   606,   605,   395,     0,   662,   667,
     681,   593,     0,   636,   637,     0,     0,     0,   649,   788,
     814,   817,   773,     0,     0,     0,   896,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     944,     0,   523,     0,     0,   524,     0,   554,     0,     0,
       0,     0,     0,     0,     0,   406,   561,   562,   563,   564,
     565,     0,    38,     0,   108,     0,     0,     0,     0,   879,
     878,   512,     0,     0,     0,     0,     0,   167,     0,     0,
     943,     0,   463,     0,     0,     0,   466,   464,   164,     0,
     170,   330,   354,   352,   300,     0,     0,     0,   353,     0,
       0,     0,    74,     0,   325,   410,   306,     0,     0,     0,
     319,     0,   320,   314,     0,   311,   310,     0,   312,     0,
       0,   329,     0,    88,    89,    86,    87,   321,   366,   309,
       0,   418,   167,   528,     0,   534,     0,     0,     0,   516,
       0,     0,   538,   542,     0,     0,   519,     0,     0,     0,
       0,   453,     0,   460,     0,   514,     0,   468,   820,   810,
       0,   768,     0,   824,   826,   831,   833,   838,   840,   608,
     844,   610,   614,   847,   616,   780,   781,   793,   336,   392,
       0,   664,   682,   864,    91,   613,     0,   619,     0,   601,
       0,     0,     0,     0,   623,     0,     0,     0,   682,   689,
       0,   591,   704,     0,   577,   635,     0,   644,     0,     0,
     648,   898,   901,   343,     0,   348,   349,   347,     0,     0,
     386,   384,     0,     0,     0,   928,   926,   333,     0,   935,
     938,   375,   378,   381,   932,   930,   910,   914,   912,     0,
     904,    74,     0,    39,     0,     0,     0,   367,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   170,
       0,   937,   334,   465,     0,     0,   331,     0,     0,     0,
       0,     0,   404,     0,    74,     0,   355,     0,     0,   340,
       0,     0,   324,     0,     0,    69,   300,     0,   357,   328,
     322,   323,     0,    81,    82,     0,     0,     0,     0,   313,
     308,   315,   316,   317,   318,   365,     0,   307,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   170,     0,     0,     0,
       0,   441,     0,     0,   539,     0,   527,     0,   517,     0,
     331,   454,     0,     0,   515,   461,   457,     0,   755,   769,
     754,   827,   834,   841,   795,   758,   759,   665,     0,   855,
     860,     0,   711,   712,   625,   624,   296,   663,   668,     0,
       0,   584,   587,     0,   638,     0,   646,     0,     0,   344,
     350,     0,     0,   385,   918,   921,   924,     0,     0,     0,
       0,     0,     0,     0,   895,     0,   907,     0,     0,   300,
     567,     0,    36,    43,     0,   110,     0,   111,     0,   112,
       0,     0,     0,     0,     0,   881,   880,   444,   576,   447,
       0,     0,   439,     0,   401,   402,     0,   400,   399,     0,
     407,   300,   356,   300,   341,     0,     0,    67,    68,   117,
     358,     0,     0,     0,     0,     0,     0,     0,   641,   372,
     371,   430,   431,   433,   432,   434,   424,   425,   426,   435,
     436,   420,   421,   422,   423,   437,   438,   427,   428,   429,
     419,    74,     0,   575,     0,   573,   442,   570,     0,     0,
       0,   569,     0,   455,     0,   458,     0,   865,   710,     0,
       0,   297,   302,   690,     0,   585,   586,   587,   588,   579,
     594,   645,   895,   895,     0,     0,     0,     0,     0,   331,
     939,   333,   376,   379,   382,     0,   896,   911,   895,     0,
     895,   558,     0,   560,   568,    40,   109,   368,     0,     0,
       0,     0,   883,   882,     0,     0,   450,     0,     0,     0,
     405,   408,   359,   123,   122,     0,     0,     0,     0,     0,
       0,     0,     0,   300,   529,   535,     0,   574,   572,     0,
     571,   760,   713,   626,     0,     0,   582,   579,   580,   581,
     584,   894,   894,   387,     0,   895,   895,   886,     0,     0,
     943,   894,   895,   886,   559,    37,     0,   113,   114,     0,
       0,     0,   448,     0,     0,   440,   403,   296,    83,    74,
       0,    74,    74,   632,   373,   642,   643,   409,   530,   536,
     540,   456,   328,   592,   583,   595,   582,     0,     0,   891,
     943,   893,     0,     0,     0,   894,   894,   887,     0,   929,
     940,     0,     0,   886,     0,   941,     0,   885,   884,   451,
       0,   327,     0,     0,   105,     0,   300,   300,     0,     0,
     541,     0,     0,   597,   628,   627,   589,     0,   944,   892,
     899,   902,   388,     0,     0,   925,   933,   915,     0,   905,
       0,     0,   941,     0,    84,    88,    89,    86,    87,    85,
     107,    97,    74,   119,   121,     0,     0,     0,     0,     0,
     889,     0,   919,   922,   908,     0,   945,     0,   941,    94,
      76,     0,   300,     0,     0,   299,   596,    74,     0,   942,
     946,     0,   328,    74,    70,    71,     0,   106,     0,   116,
       0,   370,   300,   890,   947,     0,    77,     0,    98,   369,
     598,     0,   102,     0,   296,    99,    78,     0,    74,    96,
     328,     0,    79,     0,   103,   102,     0,    76,   296,    80,
     101
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1535, -1535,  -895,    -1, -1535, -1535, -1535, -1535, -1535,   884,
    1400, -1535, -1535, -1535, -1535, -1535, -1535,  1494, -1535, -1535,
   -1535,   243, -1535,  1363, -1535, -1535,  1418, -1535, -1535, -1535,
   -1535,  -140,  -184, -1535, -1535, -1535, -1535, -1534,   782,   783,
   -1535, -1535, -1535, -1535,  -177, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535,  -976, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535,  1303, -1535, -1535,   -45,  1405, -1535, -1535, -1535,   595,
     890,   868,   561,  -486,  -670, -1535,  -309, -1535, -1535, -1535,
   -1385, -1535, -1535, -1495, -1535, -1535, -1012, -1535, -1535, -1535,
   -1535, -1535, -1535,  -753,  -328, -1137,   810,   -13, -1535, -1535,
   -1535, -1535, -1535, -1524, -1522, -1513, -1511, -1535, -1535,  1514,
   -1535,  -985, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535,  -456, -1310,   366,   150, -1535,
    -793, -1535,   370, -1535, -1535, -1535, -1535, -1395, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535,   510,  1081, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535,  -163,    11,   -35,
      12,    96, -1535, -1535, -1535, -1535, -1535, -1535, -1535,   262,
    -504,  -761, -1535,  -509,  -772, -1535,  -928,   -34,   -31, -1535,
    -565,  -560, -1535, -1535, -1535, -1211, -1535,  1476, -1535, -1535,
   -1535, -1535, -1535,   396,   586, -1535,   958, -1535, -1535, -1535,
   -1535, -1535, -1535,   587, -1535,  1239, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535,  -168, -1535,  1109, -1535, -1535, -1535,  1315, -1535, -1535,
   -1535,  -569, -1535, -1535,  -330,  -887, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535,  -116, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535,  -811, -1424,  -607, -1535, -1535, -1210, -1248,
    1114, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535, -1535,
   -1535,  1162, -1535, -1535,  1166, -1535, -1535, -1535, -1535, -1535,
   -1535, -1535, -1535, -1535, -1535,   952, -1535,  -421,  1170, -1022,
    -620,  1171,  -418
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,   783,   784,    18,   142,    55,   188,    19,   175,
     181,  1465,  1184,  1342,   625,   475,   148,   476,   102,    21,
      22,    47,    48,    93,    23,    41,    42,  1047,  1048,  1656,
     156,   157,  1657,  1672,  1685,  1235,  1583,  1049,   933,   934,
    1640,  1652,  1671,  1641,  1676,  1680,  1686,  1677,  1050,  1051,
    1621,  1052,  1006,  1053,  1054,  1055,  1056,  1057,  1058,  1059,
    1060,   182,   183,    37,    38,    39,   202,    68,    69,    70,
      71,   643,    24,   402,   556,   298,   299,   113,    25,   160,
     300,   161,   196,  1432,  1504,  1627,   558,   559,  1136,   477,
    1061,  1229,  1485,   851,   633,  1019,   709,   478,  1062,   584,
     788,  1319,   479,  1063,  1064,  1065,  1066,  1067,   755,  1068,
    1246,  1188,  1389,  1069,   480,   802,  1330,   803,  1331,   805,
    1332,   481,   792,  1323,   482,   523,   560,   483,  1212,  1213,
     849,   484,   647,   485,  1070,   486,   487,   840,   488,  1016,
    1475,  1017,  1533,   489,   902,  1285,   490,   524,   492,  1267,
    1548,  1269,  1549,  1418,  1590,   493,   494,   550,  1510,  1555,
    1437,  1439,  1313,   951,  1144,  1592,  1629,   551,   552,   553,
     700,   701,   721,   704,   705,   723,   831,   940,   941,  1596,
     575,   422,   567,   312,  1492,   568,   313,    80,   120,   200,
     308,    27,   171,   949,  1122,   950,    51,    52,   139,    28,
     164,   198,   302,  1123,   265,   266,    29,   114,   765,  1309,
     563,   304,   305,   117,   169,   769,    30,    78,   199,   564,
     942,   495,   412,   253,   254,   910,   931,   190,   255,   692,
    1289,   919,   578,   342,   256,   519,   257,   423,   959,   520,
     707,   505,  1099,   424,   960,   425,   961,   504,  1098,   508,
    1103,   509,  1291,   510,  1105,   511,  1292,   512,  1107,   513,
    1293,   514,  1110,   515,  1113,   702,    31,    57,   267,   537,
    1126,    32,    58,   268,   538,  1128,    33,    56,   345,   719,
    1298,   586,   496,   637,  1568,   638,  1560,  1561,  1562,   969,
     497,   786,  1317,   787,  1318,   813,  1337,   993,  1459,   809,
    1334,   498,   810,  1335,   499,   973,  1446,   974,  1447,   975,
    1448,   796,  1327,   807,  1333,  1020,   640,   500,   501,  1611,
     714,   502,   503
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      17,    61,    72,   522,   191,   192,   776,   774,   203,   590,
     785,   947,   593,   535,  1135,   636,   832,   834,   252,    73,
      74,    75,  1217,   694,   912,   696,   914,   698,   916,   722,
    1328,  1191,  1024,   720,   846,  1390,  1121,   130,  1117,   540,
     542,   994,  1581,    89,  -165,  1189,   527,   614,  1614,   991,
     525,    95,    72,    72,    72,  1121,    34,    35,  1615,  1072,
    1616,  1481,   708,  1351,  1477,   596,  1540,   565,     2,  1617,
     258,  1618,   107,   108,   109,     3,  1140,  1508,    90,   554,
    1532,   155,   566,  1100,    13,  1195,  1457,   548,   924,   753,
     571,  1101,   155,   330,    72,    72,    72,    72,     4,  1574,
       5,   869,     6,  1012,    40,   572,  1085,  1658,     7,    79,
     870,   573,  1013,   331,  1102,  1086,   608,  1615,     8,  1616,
    1003,   663,   664,    16,     9,  1544,   548,  1338,  1617,   555,
    1618,   332,   754,   149,   828,  1004,   922,  1509,  1579,   201,
     926,   426,   427,  1681,   797,   416,   176,   177,    10,  1608,
     121,   122,   574,  1615,   808,  1616,   309,   811,   333,   334,
     403,   433,   597,   598,  1617,   415,  1618,   435,   828,   417,
      11,    12,  1005,   251,  1194,   252,   549,   977,   201,  1679,
     981,   830,   968,  1547,   343,   845,    53,    82,   989,   685,
     686,   992,   528,  1690,  1511,  1512,   526,    84,  1190,   307,
      36,  1141,  1181,  1164,   442,   443,    91,  1628,   615,   956,
    1521,   529,  1523,  1163,  1379,   830,  1190,  1190,    92,  1190,
     530,   703,   335,    13,  1361,    54,   336,  1297,  1294,   979,
      13,   906,   565,    13,    15,  1023,   725,   329,   445,   446,
     599,   517,  1142,  1297,  1647,  -825,   661,   566,   878,   663,
     664,   879,   252,    62,    14,   252,   252,   252,    85,   757,
     600,   518,    16,    81,  1353,   418,    15,  1565,  1566,    16,
      83,   798,    16,   337,  1573,    59,   634,   338,   569,   995,
     339,  1546,    63,   101,  1387,  1021,   814,   178,    87,  1388,
     758,   123,   179,   601,   180,  -825,   124,   126,   125,    60,
    -825,   126,  1563,   634,    13,   340,    88,   576,   577,   579,
    1349,  1572,  1021,   602,  1519,   828,   582,   685,   686,  -825,
     917,   829,  1558,  -832,  -839,  -449,    64,  1463,   133,   134,
     251,   135,    43,   967,   470,   127,   252,   252,   128,   332,
     252,   474,   252,    16,   252,   306,   252,  1350,   609,    44,
    1022,    65,   252,    76,   926,  1603,  1604,    94,    59,  1154,
    1214,  1482,   830,  1030,   612,  1470,   333,   334,   610,   252,
     100,    86,    45,  -832,  -839,  -449,  1430,    82,  -832,  -839,
    -449,    77,    60,    46,   613,  1207,   252,   252,   688,   689,
    1148,  1208,   693,   101,   695,  1363,   697,  -832,  -839,  -449,
    1202,  1159,  1491,   207,   710,   194,   106,   251,  1488,    66,
     251,   251,   251,  1114,   828,  1111,   828,   583,   594,    67,
    1352,  1209,  1025,  1096,  1096,   922,  1127,   332,   112,  1125,
     335,   208,  1210,   252,   336,  1090,    59,  1211,   760,   761,
     781,    13,    43,   955,  1091,   110,   651,   652,   252,   782,
    1026,  1097,  1155,   343,   333,   334,   963,   964,   828,    44,
      60,   830,  1132,   830,   829,  1133,   976,   131,  1134,  1422,
     101,   111,   983,   984,  1346,   986,  1381,   988,   110,   990,
      16,   337,    45,   856,   860,   338,  1369,  1151,   339,  1204,
     828,   251,   251,    46,    13,   251,  1530,   251,   874,   251,
     841,   251,  1347,   790,  1028,   830,   343,   251,  1096,    92,
    1096,    59,  1096,   340,  1370,  1205,   136,   903,   335,  1204,
     836,   572,   336,   791,   251,   837,   155,   573,    49,  1360,
    1096,  1199,  1096,    16,    50,    60,  1416,   830,  1423,   252,
    1425,   251,   251,   655,   656,  1478,  1531,   651,   652,   766,
    1346,   661,   838,   662,   663,   664,   665,   666,  1464,    13,
    1468,  1096,   781,    13,  1346,  1096,  1096,   565,   574,   337,
     775,   782,  1204,   338,  1623,  1624,   339,  1506,  1528,   918,
     651,   652,   566,   107,  1204,   109,  1266,  1204,   251,  1529,
    1637,   710,  1543,  1580,  1622,   922,  1411,  1438,    16,   938,
    1630,   340,    16,   251,   137,   252,   781,   680,   681,   682,
     683,   684,  1663,  1499,   939,   782,  1651,   252,   252,   252,
     252,  1272,   685,   686,   252,   138,   839,    59,   252,    72,
    1659,   651,   652,  1282,   252,   252,   836,   252,  1287,   252,
     118,   252,   252,  1162,   655,   656,   119,   204,   205,  1168,
    1670,    60,   661,   260,   662,   663,   664,   665,   666,  1653,
     103,   104,   105,  1179,  1296,   781,   115,   116,   261,   962,
    1654,  1655,   965,   262,   782,   263,   972,   655,   656,   147,
     189,   143,   948,   781,  -761,   661,  1198,   662,   663,   664,
     665,   666,   782,   343,   251,   767,   768,   778,   781,    13,
    1559,  1559,   151,   152,   153,   154,  1567,   782,   781,    13,
    1559,  1588,  1567,   140,   421,   921,  1083,   782,   634,   141,
     932,   165,   932,   685,   686,   925,  1341,  1021,   655,   656,
     144,   419,   166,  1348,   420,   145,   661,   421,    16,   663,
     664,   665,   666,   682,   683,   684,   343,  1597,    16,   252,
     779,   252,   252,   158,  1559,  1559,   685,   686,   252,   159,
     251,   772,  1567,   162,   773,   252,   343,   421,   146,   163,
     907,   167,   251,   251,   251,   251,   958,   168,  1435,   251,
     170,   343,   184,   251,  1436,   908,   781,    13,    90,   251,
     251,   186,   251,   187,   251,   782,   251,   251,   343,  1591,
     189,  1109,   911,  1115,  1112,   252,   252,   685,   686,   210,
    1118,   252,   343,   343,   343,   211,   913,   915,  1153,   343,
    1648,   212,   107,  1161,   193,   252,    16,   195,   491,   343,
     332,   213,   343,  1495,  1233,  1234,  1550,  1402,   516,   214,
    1403,   172,   173,   821,   822,   259,   472,   644,   252,   645,
    1046,   532,   197,   646,   215,   646,   646,   333,   334,   648,
     649,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   201,  1471,   107,   108,   109,  1665,
    1571,   209,  1226,   264,   251,   301,   251,   251,   303,   781,
      13,   310,   311,   251,   172,   173,   174,   315,   782,   318,
     251,   335,   781,    13,   572,   336,  1158,  1683,   316,    59,
     573,   782,  1124,   317,  1124,   781,    13,  1412,   341,  1177,
    1599,   319,   249,   320,   782,   323,  1046,   324,   325,    16,
     781,    13,  1180,    60,  1147,   321,  1150,   326,   327,   782,
     251,   251,    16,   344,   328,   252,   251,  1325,   781,    13,
     343,   574,   337,   399,   346,    16,   338,   782,   347,   339,
     251,  1312,   400,  1589,   401,  1326,   414,   404,   506,   507,
      16,   405,   250,   533,   536,   651,   652,   534,   543,  1520,
     204,   205,   206,   251,   340,   406,   407,   544,    16,   545,
     408,   409,   410,   411,    97,    98,    99,  1304,   943,   944,
     945,  1534,   546,   557,  1626,   547,   561,   562,   570,   580,
     581,   711,   595,   604,   603,   605,   606,   607,  1222,   608,
     611,  1339,   616,   718,   619,   620,  1230,  1231,   621,   639,
     703,   691,   713,   771,   622,   332,   623,   825,   624,  1239,
    1643,  1240,  1241,  1242,  1243,  1244,   641,   642,   650,  1247,
     651,   652,   687,   752,  1371,   690,   706,   712,   770,   715,
     716,   756,   333,   334,  1585,   777,   780,   759,   764,   789,
     653,   654,   655,   656,   657,   793,   794,   658,   795,   252,
     661,   252,   662,   663,   664,   665,   666,   812,   667,   668,
     824,   827,   799,   801,   833,   332,   804,   848,   806,   826,
     251,  1518,   835,   867,   909,   850,   920,   929,   930,   815,
     816,   817,   818,   819,   820,   948,   953,   980,   954,   970,
     982,   985,   333,   334,   987,   996,   335,   997,  1316,   998,
     336,  1073,   999,  1414,   678,   679,   680,   681,   682,   683,
     684,  1093,  1075,  1000,  1001,   653,   654,   655,   656,  1007,
    1014,   685,   686,  1131,   871,   661,  1079,   662,   663,   664,
     665,   666,  1011,   667,   668,  1015,   781,    13,  1095,  1027,
    1029,  1074,  1076,  1077,  1078,   782,   904,   337,  1088,  1089,
     332,   338,  1094,   978,   339,  1104,   335,   252,  1106,  1108,
     336,   518,   332,  1116,  1120,  1129,  1130,  1137,  1145,  1146,
    1156,  1157,  1160,   928,  1166,  1167,    16,   333,   334,   340,
    1170,   680,   681,   682,   683,   684,  1175,  1178,  1182,   333,
     334,  1183,  1185,  1186,  1223,  1193,   685,   686,  1187,  1192,
     252,  1200,  1201,  1203,   251,  1220,   251,   337,  1625,  1483,
    1221,   338,   937,  1152,   339,  1232,   252,  1236,   651,   652,
    1237,  1493,  1238,  1245,  1274,  1275,  1277,  1278,  1288,  1283,
     952,  1290,  1284,  1299,  1302,  1303,   957,  1310,  1311,   340,
    1300,   335,  1306,  1321,  1314,   336,  1320,  1322,  1343,  1345,
     332,   971,  1496,   335,  1357,  1358,  1362,   336,  1364,  1365,
    1366,  1367,  1440,  1368,  1441,  1373,  1375,  1376,  1505,  1378,
    1377,  1413,  1427,  1382,  1429,  1383,  1384,   333,   334,  1385,
    1426,  1431,  1002,  1386,  1433,  1434,  1415,  1008,  1417,  1009,
    1421,  1010,   337,  1438,  1456,  1445,   338,  1442,  1165,   339,
    1428,   799,  1443,  1449,   337,  1450,  1466,  1451,   338,  1455,
    1171,   339,   251,   653,   654,   655,   656,   657,  1458,  1460,
     658,   659,   660,   661,   340,   662,   663,   664,   665,   666,
    1046,   667,   668,  1467,  1469,   669,   340,   670,   671,   672,
    1474,   335,  1476,   673,  1479,   336,  1497,  1498,  1500,  1584,
    1502,  1586,  1587,  1513,  1514,   251,  1515,  1087,  1516,  1517,
    1522,  1092,  1525,  1524,  1527,  1535,  1536,  1526,  1538,  1539,
     252,   251,   674,  1084,   675,   676,   677,   678,   679,   680,
     681,   682,   683,   684,  1537,  1541,  1542,  1119,  1552,  1551,
    1554,  1569,   337,  1570,   685,   686,   338,  1576,  1172,   339,
     738,   739,   740,   741,   742,   743,   744,   745,  1582,  1204,
     651,   652,   839,  1598,  1600,  1601,  1149,  1602,  1631,   746,
    1605,  1606,  1607,  1609,   340,   747,  1635,  1610,  1632,  1613,
    1633,   129,  1642,  1634,  1636,   748,   749,   750,  1639,  1667,
    1169,  1545,  1650,  1645,  1664,    20,  1678,  1674,  1688,   185,
     332,  1675,   132,  1689,  1553,   935,   936,  1662,  1687,   823,
     314,   150,  1143,  1666,   852,    26,   751,   923,  1556,  1480,
     839,  1593,  1557,  1594,  1196,  1197,  1595,   333,   334,   617,
     618,    96,   847,  1507,  1308,   928,  1138,  1139,  1682,   585,
     322,   413,  1216,     0,   587,  1219,   626,   627,   628,   629,
     630,  1225,     0,  1228,   800,   653,   654,   655,   656,   657,
       0,     0,   658,   659,   660,   661,     0,   662,   663,   664,
     665,   666,     0,   667,   668,   251,     0,   669,     0,   670,
     671,   672,  1620,     0,  1268,   673,  1270,     0,  1273,     0,
    1046,   335,   588,     0,     0,   336,   589,     0,  1279,     0,
     591,   592,     0,     0,     0,     0,   928,     0,     0,     0,
       0,     0,     0,     0,   674,     0,   675,   676,   677,   678,
     679,   680,   681,   682,   683,   684,     0,  1646,     0,     0,
    1295,     0,     0,     0,     0,     0,   685,   686,   763,     0,
    1301,     0,   337,  1661,     0,  1305,   338,  1307,  1173,   339,
       0,     0,     0,     0,     0,     0,     0,  1668,  1315,  1669,
       0,     0,     0,     0,  1046,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   340,     0,     0,   799,     0,     0,
    1684,     0,  1046,   726,   727,   728,   729,   730,   731,   732,
     733,     0,     0,     0,     0,  1344,     0,     0,     0,     0,
       0,     0,   -85,  1354,  1355,  1356,     0,     0,   734,     0,
       0,     0,     0,   651,   652,     0,     0,     0,   735,   736,
     737,     0,     0,     0,     0,  1372,     0,     0,  1374,     0,
       0,     0,     0,     0,   861,   862,     0,  1380,   863,   864,
     865,   866,     0,   868,     0,     0,   872,   873,   875,   876,
     877,   880,   881,   882,   883,   885,   886,   887,   888,   889,
     890,   891,   892,   893,   894,   895,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1419,     0,  1420,     0,     0,
       0,     0,     0,  1424,     0,     0,     0,     0,     0,     0,
    1248,  1249,  1250,  1251,  1252,  1253,  1254,  1255,   653,   654,
     655,   656,   657,  1256,  1257,   658,   659,   660,   661,  1258,
     662,   663,   664,   665,   666,  1259,   667,   668,  1260,  1261,
     669,  1444,   670,   671,   672,  1262,  1263,  1264,   673,     0,
       0,     0,     0,   946,     0,     0,     0,     0,  1462,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1472,  1473,     0,     0,  1265,   674,     0,   675,
     676,   677,   678,   679,   680,   681,   682,   683,   684,     0,
       0,     0,     0,     0,     0,     0,  1484,     0,     0,   685,
     686,     0,  1486,  1487,     0,  1489,  1490,     0,     0,     0,
    1031,     0,     0,     0,   426,   427,     3,     0,  -118,  -104,
    -104,     0,  -115,     0,   428,   429,   430,   431,   432,     0,
       0,     0,     0,     0,   433,  1032,   434,  1033,  1034,     0,
     435,     0,  1071,     0,     0,     0,  1501,  1035,   436,  1036,
    1503,  -120,     0,  1037,   437,     0,     0,   438,     0,     8,
     439,  1038,     0,  1039,   440,     0,     0,  1040,  1041,     0,
       0,   799,     0,     0,  1042,     0,     0,   442,   443,     0,
     216,   217,   218,     0,   220,   221,   222,   223,   224,   444,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,     0,   238,   239,   240,     0,     0,   243,   244,   245,
     246,   445,   446,   447,  1043,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   448,   449,     0,
       0,     0,     0,     0,  1564,     0,     0,     0,  1071,     0,
       0,     0,     0,     0,     0,     0,  1575,     0,    59,     0,
    1577,  1578,     0,     0,     0,     0,   450,   451,   452,   453,
     454,     0,   455,     0,   456,   457,   458,   459,   460,   461,
     462,   463,    60,     0,    13,   464,   332,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   465,   466,   467,     0,    14,  1612,     0,   468,   469,
       0,     0,     0,   333,   334,     0,     0,   470,     0,   471,
       0,   472,   473,    16,  1044,  1045,     0,     0,   426,   427,
       0,     0,     0,     0,     0,     0,     0,     0,   428,   429,
     430,   431,   432,  1638,     0,     0,     0,     0,   433,     0,
     434,     0,     0,     0,   435,     0,  1644,     0,     0,     0,
       0,     0,   436,     0,     0,  1649,     0,     0,   437,     0,
       0,   438,     0,  1660,   439,     0,     0,   335,   440,     0,
       0,   336,     0,     0,     0,  1276,     0,     0,   441,     0,
       0,   442,   443,   842,   216,   217,   218,  1673,   220,   221,
     222,   223,   224,   444,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,     0,   238,   239,   240,     0,
       0,   243,   244,   245,   246,   445,   446,   447,   337,     0,
       0,     0,   338,     0,  1174,   339,     0,     0,     0,     0,
       0,   448,   449,     0,     0,     0,     0,     0,     0,     0,
     521,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     340,     0,    59,     0,     0,     0,     0,     0,     0,     0,
     450,   451,   452,   453,   454,     0,   455,   634,   456,   457,
     458,   459,   460,   461,   462,   463,   635,     0,     0,   464,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   465,   466,   467,     0,    14,
       0,     0,   468,   469,     0,     0,     0,     0,     0,     0,
       0,   843,     0,   471,   844,   472,   473,     0,   474,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1391,
    1392,  1393,  1394,  1395,  1396,  1397,  1398,  1399,  1400,  1401,
    1404,  1405,  1406,  1407,  1408,  1409,  1410,     0,     0,     0,
    1031,     0,     0,     0,   426,   427,     3,     0,  -118,  -104,
    -104,     0,  -115,     0,   428,   429,   430,   431,   432,     0,
       0,     0,     0,     0,   433,  1032,   434,  1033,  1034,     0,
     435,     0,     0,     0,     0,     0,     0,  1035,   436,  1036,
       0,  -120,     0,  1037,   437,     0,     0,   438,     0,     8,
     439,  1038,     0,  1039,   440,     0,     0,  1040,  1041,     0,
       0,  1452,  1453,  1454,  1042,     0,     0,   442,   443,     0,
     216,   217,   218,     0,   220,   221,   222,   223,   224,   444,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,     0,   238,   239,   240,   332,     0,   243,   244,   245,
     246,   445,   446,   447,  1043,   332,     0,     0,     0,     0,
       0,     0,  1071,     0,     0,     0,   332,   448,   449,     0,
       0,     0,   333,   334,     0,     0,     0,     0,     0,     0,
       0,     0,   333,   334,     0,     0,     0,     0,    59,     0,
       0,     0,     0,   333,   334,     0,   450,   451,   452,   453,
     454,     0,   455,     0,   456,   457,   458,   459,   460,   461,
     462,   463,    60,     0,    13,   464,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   465,   466,   467,     0,    14,   335,     0,   468,   469,
     336,     0,     0,     0,     0,     0,   335,   470,     0,   471,
     336,   472,   473,    16,  1044,  -301,     0,   335,     0,     0,
       0,   336,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   337,     0,     0,
       0,   338,     0,  1176,   339,     0,     0,   337,     0,     0,
       0,   338,     0,  1324,   339,     0,     0,     0,   337,     0,
       0,     0,   338,     0,  1329,   339,     0,     0,     0,   340,
       0,     0,     0,     0,     0,     0,     0,  1031,     0,   340,
       0,   426,   427,     3,     0,  -118,  -104,  -104,     0,  -115,
     340,   428,   429,   430,   431,   432,     0,     0,     0,     0,
       0,   433,  1032,   434,  1033,  1034,     0,   435,     0,     0,
       0,     0,     0,  1619,  1035,   436,  1036,     0,  -120,     0,
    1037,   437,  1071,     0,   438,     0,     8,   439,  1038,     0,
    1039,   440,     0,     0,  1040,  1041,     0,     0,     0,     0,
       0,  1042,     0,     0,   442,   443,     0,   216,   217,   218,
       0,   220,   221,   222,   223,   224,   444,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,     0,   238,
     239,   240,  1619,     0,   243,   244,   245,   246,   445,   446,
     447,  1043,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   448,   449,  1071,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1619,     0,
       0,     0,     0,     0,  1071,    59,     0,     0,     0,     0,
       0,     0,     0,   450,   451,   452,   453,   454,     0,   455,
       0,   456,   457,   458,   459,   460,   461,   462,   463,    60,
       0,    13,   464,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   465,   466,
     467,     0,    14,     0,     0,   468,   469,     0,     0,     0,
       0,     0,     0,     0,   470,     0,   471,     0,   472,   473,
      16,  1044,  -326,  1031,     0,     0,     0,   426,   427,     3,
       0,  -118,  -104,  -104,     0,  -115,     0,   428,   429,   430,
     431,   432,     0,     0,     0,     0,     0,   433,  1032,   434,
    1033,  1034,     0,   435,     0,     0,     0,     0,     0,     0,
    1035,   436,  1036,     0,  -120,     0,  1037,   437,     0,     0,
     438,     0,     8,   439,  1038,     0,  1039,   440,     0,     0,
    1040,  1041,     0,     0,     0,     0,     0,  1042,     0,     0,
     442,   443,     0,   216,   217,   218,     0,   220,   221,   222,
     223,   224,   444,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,     0,   238,   239,   240,     0,     0,
     243,   244,   245,   246,   445,   446,   447,  1043,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     448,   449,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,     0,     0,     0,     0,     0,     0,     0,   450,
     451,   452,   453,   454,     0,   455,     0,   456,   457,   458,
     459,   460,   461,   462,   463,    60,     0,    13,   464,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   465,   466,   467,     0,    14,     0,
       0,   468,   469,     0,     0,     0,     0,     0,     0,     0,
     470,     0,   471,     0,   472,   473,    16,  1044,  -298,  1031,
       0,     0,     0,   426,   427,     3,     0,  -118,  -104,  -104,
       0,  -115,     0,   428,   429,   430,   431,   432,     0,     0,
       0,     0,     0,   433,  1032,   434,  1033,  1034,     0,   435,
       0,     0,     0,     0,     0,     0,  1035,   436,  1036,     0,
    -120,     0,  1037,   437,     0,     0,   438,     0,     8,   439,
    1038,     0,  1039,   440,     0,     0,  1040,  1041,     0,     0,
       0,     0,     0,  1042,     0,     0,   442,   443,     0,   216,
     217,   218,     0,   220,   221,   222,   223,   224,   444,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
       0,   238,   239,   240,     0,     0,   243,   244,   245,   246,
     445,   446,   447,  1043,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   448,   449,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,     0,     0,
       0,     0,     0,     0,     0,   450,   451,   452,   453,   454,
       0,   455,     0,   456,   457,   458,   459,   460,   461,   462,
     463,    60,     0,    13,   464,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     465,   466,   467,     0,    14,     0,     0,   468,   469,     0,
       0,     0,     0,     0,     0,     0,   470,     0,   471,     0,
     472,   473,    16,  1044,   -95,  1031,     0,     0,     0,   426,
     427,     3,     0,  -118,  -104,  -104,     0,  -115,     0,   428,
     429,   430,   431,   432,     0,     0,     0,     0,     0,   433,
    1032,   434,  1033,  1034,     0,   435,     0,     0,     0,     0,
       0,     0,  1035,   436,  1036,     0,  -120,     0,  1037,   437,
       0,     0,   438,     0,     8,   439,  1038,     0,  1039,   440,
       0,     0,  1040,  1041,     0,     0,     0,     0,     0,  1042,
       0,     0,   442,   443,     0,   216,   217,   218,     0,   220,
     221,   222,   223,   224,   444,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,     0,   238,   239,   240,
       0,     0,   243,   244,   245,   246,   445,   446,   447,  1043,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   448,   449,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    59,     0,     0,     0,     0,     0,     0,
       0,   450,   451,   452,   453,   454,     0,   455,     0,   456,
     457,   458,   459,   460,   461,   462,   463,    60,     0,    13,
     464,     0,   332,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   465,   466,   467,     0,
      14,     0,     0,   468,   469,     0,     0,     0,     0,   333,
     334,     0,   470,     0,   471,     0,   472,   473,    16,  1044,
    -100,   426,   427,     0,     0,     0,     0,     0,     0,   631,
       0,   428,   429,   430,   431,   432,     0,     0,     0,     0,
       0,   433,     0,   434,     0,     0,     0,   435,     0,     0,
       0,     0,     0,     0,     0,   436,     0,     0,     0,     0,
       0,   437,     0,     0,   438,   632,     0,   439,     0,     0,
       0,   440,     0,   335,     0,     0,     0,   336,     0,     0,
       0,   441,     0,     0,   442,   443,     0,   216,   217,   218,
       0,   220,   221,   222,   223,   224,   444,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,     0,   238,
     239,   240,     0,     0,   243,   244,   245,   246,   445,   446,
     447,     0,     0,     0,   337,     0,     0,     0,   338,     0,
    1336,   339,     0,     0,   448,   449,     0,     0,     0,     0,
       0,     0,     0,   521,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    59,   340,     0,     0,     0,
       0,     0,     0,   450,   451,   452,   453,   454,     0,   455,
     634,   456,   457,   458,   459,   460,   461,   462,   463,   635,
       0,     0,   464,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   465,   466,
     467,     0,    14,     0,     0,   468,   469,     0,     0,     0,
       0,     0,   426,   427,   470,     0,   471,     0,   472,   473,
     631,   474,   428,   429,   430,   431,   432,     0,     0,     0,
       0,     0,   433,     0,   434,     0,     0,   332,   435,     0,
       0,     0,     0,     0,     0,     0,   436,     0,     0,     0,
       0,     0,   437,     0,     0,   438,   632,     0,   439,     0,
       0,     0,   440,     0,   333,   334,     0,     0,     0,     0,
       0,     0,   441,     0,     0,   442,   443,     0,   216,   217,
     218,     0,   220,   221,   222,   223,   224,   444,   226,   227,
     228,   229,   230,   231,   232,   233,   234,   235,   236,     0,
     238,   239,   240,     0,     0,   243,   244,   245,   246,   445,
     446,   447,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   448,   449,     0,   335,     0,
       0,     0,   336,     0,   521,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    59,     0,     0,     0,
       0,     0,     0,     0,   450,   451,   452,   453,   454,     0,
     455,     0,   456,   457,   458,   459,   460,   461,   462,   463,
      60,     0,     0,   464,     0,     0,     0,     0,     0,   337,
       0,     0,     0,   338,     0,  1359,   339,     0,     0,   465,
     466,   467,     0,    14,     0,     0,   468,   469,     0,     0,
       0,     0,     0,   426,   427,   470,     0,   471,     0,   472,
     473,   340,   474,   428,   429,   430,   431,   432,     0,     0,
       0,     0,     0,   433,     0,   434,     0,     0,   332,   435,
       0,     0,     0,     0,     0,     0,     0,   436,     0,     0,
       0,     0,     0,   437,     0,     0,   438,     0,     0,   439,
       0,     0,     0,   440,     0,   333,   334,     0,     0,     0,
       0,     0,     0,   441,     0,     0,   442,   443,   966,   216,
     217,   218,     0,   220,   221,   222,   223,   224,   444,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
       0,   238,   239,   240,     0,     0,   243,   244,   245,   246,
     445,   446,   447,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   448,   449,     0,   335,
       0,     0,     0,   336,     0,   521,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,     0,     0,
       0,     0,     0,     0,     0,   450,   451,   452,   453,   454,
       0,   455,   634,   456,   457,   458,   459,   460,   461,   462,
     463,   635,     0,     0,   464,     0,     0,     0,     0,     0,
     337,     0,     0,     0,   338,     0,  1494,   339,     0,     0,
     465,   466,   467,     0,    14,     0,     0,   468,   469,     0,
       0,     0,     0,     0,   426,   427,   470,     0,   471,     0,
     472,   473,   340,   474,   428,   429,   430,   431,   432,     0,
       0,     0,     0,     0,   433,     0,   434,     0,     0,   332,
     435,     0,     0,     0,     0,     0,     0,     0,   436,     0,
       0,     0,     0,     0,   437,     0,     0,   438,     0,     0,
     439,     0,     0,     0,   440,     0,   333,   334,     0,     0,
       0,     0,     0,     0,   441,     0,     0,   442,   443,     0,
     216,   217,   218,     0,   220,   221,   222,   223,   224,   444,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,     0,   238,   239,   240,     0,     0,   243,   244,   245,
     246,   445,   446,   447,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   448,   449,     0,
     335,     0,     0,     0,   336,     0,   521,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    59,     0,
       0,     0,     0,     0,     0,     0,   450,   451,   452,   453,
     454,     0,   455,   634,   456,   457,   458,   459,   460,   461,
     462,   463,   635,     0,     0,   464,     0,     0,     0,     0,
       0,   337,     0,     0,     0,   338,     0,     0,   339,     0,
       0,   465,   466,   467,     0,    14,     0,     0,   468,   469,
       0,     0,     0,     0,     0,   426,   427,   470,     0,   471,
       0,   472,   473,   340,   474,   428,   429,   430,   431,   432,
       0,     0,     0,     0,     0,   433,     0,   434,     0,     0,
       0,   435,     0,     0,     0,     0,     0,     0,     0,   436,
       0,     0,     0,     0,     0,   437,     0,     0,   438,     0,
       0,   439,     0,     0,     0,   440,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   441,     0,     0,   442,   443,
       0,   216,   217,   218,     0,   220,   221,   222,   223,   224,
     444,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,     0,   238,   239,   240,     0,     0,   243,   244,
     245,   246,   445,   446,   447,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   448,   449,
       0,     0,     0,     0,     0,     0,     0,   521,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    59,
       0,     0,     0,     0,     0,     0,     0,   450,   451,   452,
     453,   454,     0,   455,     0,   456,   457,   458,   459,   460,
     461,   462,   463,    60,     0,     0,   464,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   465,   466,   467,     0,    14,     0,     0,   468,
     469,     0,     0,     0,     0,     0,   426,   427,   470,     0,
     471,   905,   472,   473,     0,   474,   428,   429,   430,   431,
     432,     0,     0,     0,     0,     0,   433,     0,   434,     0,
       0,     0,   435,     0,     0,     0,     0,     0,     0,     0,
     436,     0,     0,     0,     0,     0,   437,     0,     0,   438,
       0,     0,   439,     0,     0,     0,   440,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   441,     0,     0,   442,
     443,     0,   216,   217,   218,     0,   220,   221,   222,   223,
     224,   444,   226,   227,   228,   229,   230,   231,   232,   233,
     234,   235,   236,     0,   238,   239,   240,     0,     0,   243,
     244,   245,   246,   445,   446,   447,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   448,
     449,     0,     0,     0,     0,     0,     0,     0,   521,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      59,     0,     0,     0,     0,     0,     0,     0,   450,   451,
     452,   453,   454,     0,   455,     0,   456,   457,   458,   459,
     460,   461,   462,   463,    60,     0,     0,   464,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   465,   466,   467,     0,    14,     0,     0,
     468,   469,     0,     0,     0,     0,     0,   426,   427,   470,
       0,   471,  1271,   472,   473,     0,   474,   428,   429,   430,
     431,   432,     0,     0,     0,     0,     0,   433,     0,   434,
       0,     0,     0,   435,     0,     0,     0,     0,     0,     0,
       0,   436,     0,     0,     0,     0,     0,   437,     0,     0,
     438,     0,     0,   439,     0,     0,     0,   440,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   441,     0,     0,
     442,   443,     0,   216,   217,   218,     0,   220,   221,   222,
     223,   224,   444,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,     0,   238,   239,   240,     0,     0,
     243,   244,   245,   246,   445,   446,   447,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     448,   449,     0,     0,     0,     0,     0,     0,     0,   521,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,     0,     0,     0,     0,     0,     0,     0,   450,
     451,   452,   453,   454,     0,   455,     0,   456,   457,   458,
     459,   460,   461,   462,   463,    60,     0,     0,   464,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   465,   466,   467,     0,    14,     0,
       0,   468,   469,     0,     0,     0,     0,     0,   426,   427,
    1280,     0,   471,  1281,   472,   473,     0,   474,   428,   429,
     430,   431,   432,     0,     0,     0,     0,     0,   433,     0,
     434,     0,     0,     0,   435,     0,     0,     0,     0,     0,
       0,     0,   436,     0,     0,     0,     0,     0,   437,     0,
       0,   438,     0,     0,   439,     0,     0,     0,   440,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   441,     0,
       0,   442,   443,     0,   216,   217,   218,     0,   220,   221,
     222,   223,   224,   444,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,     0,   238,   239,   240,     0,
       0,   243,   244,   245,   246,   445,   446,   447,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   448,   449,     0,     0,     0,     0,     0,     0,     0,
     521,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,     0,     0,     0,     0,     0,     0,     0,
     450,   451,   452,   453,   454,     0,   455,     0,   456,   457,
     458,   459,   460,   461,   462,   463,    60,     0,     0,   464,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   465,   466,   467,     0,    14,
       0,     0,   468,   469,     0,     0,     0,     0,     0,   426,
     427,   470,     0,   471,  1286,   472,   473,     0,   474,   428,
     429,   430,   431,   432,     0,     0,     0,     0,     0,   433,
       0,   434,     0,     0,     0,   435,     0,     0,     0,     0,
       0,     0,     0,   436,     0,     0,     0,     0,     0,   437,
       0,     0,   438,     0,     0,   439,     0,     0,     0,   440,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   441,
       0,     0,   442,   443,     0,   216,   217,   218,     0,   220,
     221,   222,   223,   224,   444,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,     0,   238,   239,   240,
       0,     0,   243,   244,   245,   246,   445,   446,   447,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   448,   449,     0,     0,     0,     0,     0,     0,
       0,   521,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    59,     0,     0,     0,     0,     0,     0,
       0,   450,   451,   452,   453,   454,     0,   455,     0,   456,
     457,   458,   459,   460,   461,   462,   463,    60,     0,     0,
     464,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   465,   466,   467,     0,
      14,     0,     0,   468,   469,     0,     0,     0,     0,     0,
     426,   427,   470,     0,   471,  1340,   472,   473,     0,   474,
     428,   429,   430,   431,   432,     0,     0,     0,     0,     0,
     433,     0,   434,     0,     0,     0,   435,     0,     0,     0,
       0,     0,     0,     0,   436,     0,     0,     0,     0,     0,
     437,     0,     0,   438,     0,     0,   439,     0,     0,     0,
     440,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     441,     0,     0,   442,   443,     0,   216,   217,   218,     0,
     220,   221,   222,   223,   224,   444,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,     0,   238,   239,
     240,     0,     0,   243,   244,   245,   246,   445,   446,   447,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   448,   449,     0,     0,     0,     0,     0,
       0,     0,   521,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    59,     0,     0,     0,     0,     0,
       0,     0,   450,   451,   452,   453,   454,     0,   455,     0,
     456,   457,   458,   459,   460,   461,   462,   463,    60,     0,
       0,   464,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   465,   466,   467,
       0,    14,     0,     0,   468,   469,     0,     0,     0,     0,
       0,   426,   427,   470,     0,   471,     0,   472,   473,     0,
     474,   428,   429,   430,   431,   432,     0,     0,     0,     0,
       0,   433,     0,   434,     0,     0,     0,   435,     0,     0,
       0,     0,     0,     0,     0,   436,     0,     0,     0,     0,
       0,   437,     0,     0,   438,     0,     0,   439,     0,     0,
       0,   440,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   441,     0,     0,   442,   443,     0,   216,   217,   218,
       0,   220,   221,   222,   223,   224,   444,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,     0,   238,
     239,   240,     0,     0,   243,   244,   245,   246,   445,   446,
     447,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   448,   449,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    59,     0,     0,     0,     0,
       0,     0,     0,   450,   451,   452,   453,   454,     0,   455,
       0,   456,   457,   458,   459,   460,   461,   462,   463,    60,
       0,     0,   464,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   465,   466,
     467,     0,    14,     0,     0,   468,   469,     0,     0,     0,
       0,     0,   426,   427,   470,   531,   471,     0,   472,   473,
       0,   474,   428,   429,   430,   431,   432,     0,     0,     0,
       0,     0,   433,     0,   434,     0,     0,     0,   435,     0,
       0,     0,     0,     0,     0,     0,   436,     0,     0,     0,
       0,     0,   437,     0,     0,   438,     0,     0,   439,     0,
       0,     0,   440,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   441,     0,     0,   442,   443,     0,   216,   217,
     218,     0,   220,   221,   222,   223,   224,   444,   226,   227,
     228,   229,   230,   231,   232,   233,   234,   235,   236,     0,
     238,   239,   240,     0,     0,   243,   244,   245,   246,   445,
     446,   447,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   448,   449,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    59,     0,     0,     0,
       0,     0,     0,     0,   450,   451,   452,   453,   454,     0,
     455,     0,   456,   457,   458,   459,   460,   461,   462,   463,
      60,     0,     0,   464,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   465,
     466,   467,     0,    14,     0,     0,   468,   469,     0,     0,
       0,     0,     0,   426,   427,   470,   717,   471,     0,   472,
     473,     0,   474,   428,   429,   430,   431,   432,     0,     0,
       0,     0,     0,   433,     0,   434,     0,     0,     0,   435,
       0,     0,     0,     0,     0,     0,     0,   436,     0,     0,
       0,     0,     0,   437,     0,     0,   438,     0,     0,   439,
       0,     0,     0,   440,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   441,     0,     0,   442,   443,     0,   216,
     217,   218,     0,   220,   221,   222,   223,   224,   444,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
       0,   238,   239,   240,     0,     0,   243,   244,   245,   246,
     445,   446,   447,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   448,   449,     0,     0,
       0,     0,     0,     0,     0,   927,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,     0,     0,
       0,     0,     0,     0,     0,   450,   451,   452,   453,   454,
       0,   455,     0,   456,   457,   458,   459,   460,   461,   462,
     463,    60,     0,     0,   464,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     465,   466,   467,     0,    14,     0,     0,   468,   469,     0,
       0,     0,     0,     0,   426,   427,   470,     0,   471,     0,
     472,   473,  1018,   474,   428,   429,   430,   431,   432,     0,
       0,     0,     0,     0,   433,     0,   434,     0,     0,     0,
     435,     0,     0,     0,     0,     0,     0,     0,   436,     0,
       0,     0,     0,     0,   437,     0,     0,   438,     0,     0,
     439,     0,     0,     0,   440,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   441,     0,     0,   442,   443,     0,
     216,   217,   218,     0,   220,   221,   222,   223,   224,   444,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,     0,   238,   239,   240,     0,     0,   243,   244,   245,
     246,   445,   446,   447,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   448,   449,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    59,     0,
       0,     0,     0,     0,     0,     0,   450,   451,   452,   453,
     454,     0,   455,     0,   456,   457,   458,   459,   460,   461,
     462,   463,    60,     0,     0,   464,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   465,   466,   467,     0,    14,     0,     0,   468,   469,
       0,     0,     0,     0,     0,   426,   427,   470,     0,   471,
       0,   472,   473,     0,   474,   428,   429,   430,   431,   432,
       0,     0,     0,     0,     0,   433,     0,   434,     0,     0,
       0,   435,     0,     0,     0,     0,     0,     0,     0,   436,
       0,     0,     0,     0,     0,   437,     0,     0,   438,     0,
       0,   439,     0,     0,     0,   440,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   441,     0,     0,   442,   443,
       0,   216,   217,   218,     0,   220,   221,   222,   223,   224,
     444,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,     0,   238,   239,   240,     0,     0,   243,   244,
     245,   246,   445,   446,   447,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   448,   449,
       0,     0,     0,     0,     0,     0,     0,   927,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    59,
       0,     0,     0,     0,     0,     0,     0,   450,   451,   452,
     453,   454,     0,   455,     0,   456,   457,   458,   459,   460,
     461,   462,   463,    60,     0,     0,   464,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   465,   466,   467,     0,    14,     0,     0,   468,
     469,     0,     0,     0,     0,     0,   426,   427,  1206,     0,
     471,     0,   472,   473,     0,   474,   428,   429,   430,   431,
     432,     0,     0,     0,     0,     0,   433,     0,   434,     0,
       0,     0,   435,     0,     0,     0,     0,     0,     0,     0,
     436,     0,     0,     0,     0,     0,   437,     0,     0,   438,
       0,     0,   439,     0,     0,     0,   440,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   441,     0,     0,   442,
     443,     0,   216,   217,   218,     0,   220,   221,   222,   223,
     224,   444,   226,   227,   228,   229,   230,   231,   232,   233,
     234,   235,   236,     0,   238,   239,   240,     0,     0,   243,
     244,   245,   246,   445,   446,   447,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   448,
     449,     0,     0,     0,     0,     0,     0,     0,  1215,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      59,     0,     0,     0,     0,     0,     0,     0,   450,   451,
     452,   453,   454,     0,   455,     0,   456,   457,   458,   459,
     460,   461,   462,   463,    60,     0,     0,   464,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   465,   466,   467,     0,    14,     0,     0,
     468,   469,     0,     0,     0,     0,     0,   426,   427,   470,
       0,   471,     0,   472,   473,     0,   474,   428,   429,   430,
     431,   432,     0,     0,     0,     0,     0,   433,     0,   434,
       0,     0,     0,   435,     0,     0,     0,     0,     0,     0,
       0,   436,     0,     0,     0,     0,     0,   437,     0,     0,
     438,     0,     0,   439,     0,     0,     0,   440,     0,     0,
       0,     0,     0,  1218,     0,     0,     0,   441,     0,     0,
     442,   443,     0,   216,   217,   218,     0,   220,   221,   222,
     223,   224,   444,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,     0,   238,   239,   240,     0,     0,
     243,   244,   245,   246,   445,   446,   447,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     448,   449,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,     0,     0,     0,     0,     0,     0,     0,   450,
     451,   452,   453,   454,     0,   455,     0,   456,   457,   458,
     459,   460,   461,   462,   463,    60,     0,     0,   464,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   465,   466,   467,     0,    14,     0,
       0,   468,   469,     0,     0,     0,     0,     0,   426,   427,
     470,     0,   471,     0,   472,   473,     0,   474,   428,   429,
     430,   431,   432,     0,     0,     0,     0,     0,   433,     0,
     434,     0,     0,     0,   435,     0,     0,     0,     0,     0,
       0,     0,   436,     0,     0,     0,     0,     0,   437,     0,
       0,   438,     0,     0,   439,     0,     0,     0,   440,     0,
       0,  1224,     0,     0,     0,     0,     0,     0,   441,     0,
       0,   442,   443,     0,   216,   217,   218,     0,   220,   221,
     222,   223,   224,   444,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,     0,   238,   239,   240,     0,
       0,   243,   244,   245,   246,   445,   446,   447,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   448,   449,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,     0,     0,     0,     0,     0,     0,     0,
     450,   451,   452,   453,   454,     0,   455,     0,   456,   457,
     458,   459,   460,   461,   462,   463,    60,     0,     0,   464,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   465,   466,   467,     0,    14,
       0,     0,   468,   469,     0,     0,     0,     0,     0,   426,
     427,   470,     0,   471,     0,   472,   473,     0,   474,   428,
     429,   430,   431,   432,     0,     0,     0,     0,     0,   433,
       0,   434,     0,     0,     0,   435,     0,     0,     0,     0,
       0,     0,     0,   436,     0,     0,     0,     0,     0,   437,
       0,     0,   438,     0,     0,   439,     0,     0,     0,   440,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   441,
       0,     0,   442,   443,     0,   216,   217,   218,     0,   220,
     221,   222,   223,   224,   444,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,     0,   238,   239,   240,
       0,     0,   243,   244,   245,   246,   445,   446,   447,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   448,   449,     0,     0,     0,     0,     0,     0,
       0,  1227,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    59,     0,     0,     0,     0,     0,     0,
       0,   450,   451,   452,   453,   454,     0,   455,     0,   456,
     457,   458,   459,   460,   461,   462,   463,    60,     0,     0,
     464,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   465,   466,   467,     0,
      14,     0,     0,   468,   469,     0,     0,     0,     0,     0,
     426,   427,   470,     0,   471,     0,   472,   473,     0,   474,
     428,   429,   430,   431,   432,     0,     0,     0,     0,     0,
     433,     0,   434,     0,     0,     0,   435,     0,     0,     0,
       0,     0,     0,     0,   436,     0,     0,     0,     0,     0,
     437,     0,     0,   438,     0,     0,   439,     0,     0,     0,
     440,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     441,     0,     0,   442,   443,     0,   216,   217,   218,     0,
     220,   221,   222,   223,   224,   444,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,     0,   238,   239,
     240,     0,     0,   243,   244,   245,   246,   445,   446,   447,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   448,   449,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    59,     0,     0,     0,     0,     0,
       0,     0,   450,   451,   452,   453,   454,     0,   455,     0,
     456,   457,   458,   459,   460,   461,   462,   463,    60,     0,
       0,   464,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   465,   466,   467,
       0,    14,     0,     0,   468,   469,     0,     0,     0,     0,
       0,   426,   427,   470,     0,   471,  1461,   472,   473,     0,
     474,   428,   429,   430,   431,   432,     0,     0,     0,     0,
       0,   433,     0,   434,     0,     0,     0,   435,     0,     0,
       0,     0,     0,     0,     0,   436,     0,     0,     0,     0,
       0,   437,     0,     0,   438,     0,     0,   439,     0,     0,
       0,   440,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   441,     0,     0,   442,   443,     0,   216,   217,   218,
       0,   220,   221,   222,   223,   224,   444,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,     0,   238,
     239,   240,     0,     0,   243,   244,   245,   246,   445,   446,
     447,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   448,   449,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    59,     0,     0,     0,     0,
       0,     0,     0,   450,   451,   452,   453,   454,     0,   455,
       0,   456,   457,   458,   459,   460,   461,   462,   463,    60,
       0,     0,   464,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   465,   466,
     467,     0,    14,     0,     0,   468,   469,     0,     0,     0,
       0,     0,   426,   427,   470,     0,   471,     0,   472,   473,
       0,   474,   428,   429,   430,   431,   432,     0,     0,     0,
       0,     0,   433,  1032,   434,  1033,     0,     0,   435,     0,
       0,     0,     0,     0,     0,     0,   436,     0,     0,     0,
       0,     0,   437,     0,     0,   438,     0,     0,   439,  1038,
       0,     0,   440,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   441,     0,     0,   442,   443,     0,   216,   217,
     218,     0,   220,   221,   222,   223,   224,   444,   226,   227,
     228,   229,   230,   231,   232,   233,   234,   235,   236,     0,
     238,   239,   240,     0,     0,   243,   244,   245,   246,   445,
     446,   447,  1043,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   448,   449,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    59,     0,     0,     0,
       0,     0,     0,     0,   450,   451,   452,   453,   454,     0,
     455,     0,   456,   457,   458,   459,   460,   461,   462,   463,
      60,     0,     0,   464,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   465,
     466,   467,     0,    14,     0,     0,   468,   469,     0,     0,
       0,   426,   427,     0,     0,   470,     0,   471,     0,   472,
     473,   428,   429,   430,   431,   432,     0,     0,     0,     0,
       0,   433,     0,   434,     0,     0,     0,   435,     0,     0,
       0,     0,     0,     0,     0,   436,     0,     0,     0,     0,
       0,   437,     0,     0,   438,     0,     0,   439,     0,     0,
       0,   440,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   441,     0,     0,   442,   443,     0,   216,   217,   218,
       0,   220,   221,   222,   223,   224,   444,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,     0,   238,
     239,   240,     0,     0,   243,   244,   245,   246,   445,   446,
     447,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   448,   449,     0,     0,     0,     0,
       0,     0,     0,   762,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    59,     0,     0,     0,     0,
       0,     0,     0,   450,   451,   452,   453,   454,     0,   455,
       0,   456,   457,   458,   459,   460,   461,   462,   463,    60,
       0,     0,   464,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   465,   466,
     467,     0,    14,     0,     0,   468,   469,     0,     0,     0,
     426,   427,     0,     0,   470,     0,   471,     0,   472,   473,
     428,   429,   430,   431,   432,     0,     0,   884,     0,     0,
     433,     0,   434,     0,     0,     0,   435,     0,     0,     0,
       0,     0,     0,     0,   436,     0,     0,     0,     0,     0,
     437,     0,     0,   438,     0,     0,   439,     0,     0,     0,
     440,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     441,     0,     0,   442,   443,     0,   216,   217,   218,     0,
     220,   221,   222,   223,   224,   444,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,     0,   238,   239,
     240,     0,     0,   243,   244,   245,   246,   445,   446,   447,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   448,   449,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    59,     0,     0,     0,     0,     0,
       0,     0,   450,   451,   452,   453,   454,     0,   455,     0,
     456,   457,   458,   459,   460,   461,   462,   463,    60,     0,
       0,   464,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   465,   466,   467,
       0,    14,     0,     0,   468,   469,     0,     0,     0,   426,
     427,     0,     0,   470,     0,   471,     0,   472,   473,   428,
     429,   430,   431,   432,     0,     0,     0,     0,     0,   433,
       0,   434,     0,     0,     0,   435,     0,     0,     0,     0,
       0,     0,     0,   436,     0,     0,     0,     0,     0,   437,
       0,     0,   438,     0,     0,   439,     0,     0,     0,   440,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   441,
       0,     0,   442,   443,     0,   216,   217,   218,     0,   220,
     221,   222,   223,   224,   444,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,     0,   238,   239,   240,
       0,     0,   243,   244,   245,   246,   445,   446,   447,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   448,   449,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    59,     0,     0,     0,     0,     0,     0,
       0,   450,   451,   452,   453,   454,     0,   455,     0,   456,
     457,   458,   459,   460,   461,   462,   463,    60,     0,   210,
     464,     0,     0,     0,     0,   211,     0,     0,     0,     0,
       0,   212,     0,     0,     0,     0,   465,   466,   467,     0,
      14,   213,     0,   468,   469,     0,     0,     0,     0,   214,
       0,     0,   470,     0,   471,     0,   472,   473,     0,     0,
       0,     0,     0,     0,   215,     0,     0,     0,     0,     0,
       0,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   210,     0,     0,     0,     0,     0,
     211,     0,     0,     0,     0,     0,   212,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   213,     0,     0,    59,
       0,     0,     0,     0,   214,     0,     0,     0,     0,     0,
       0,     0,   249,     0,     0,     0,     0,     0,     0,   215,
       0,     0,     0,   699,     0,    13,   216,   217,   218,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,     0,
       0,     0,   250,     0,    16,     0,     0,   210,     0,     0,
       0,     0,     0,   211,     0,     0,     0,     0,     0,   212,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   213,
       0,     0,     0,     0,    59,     0,     0,   214,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   249,     0,     0,
       0,     0,   215,     0,     0,     0,     0,     0,    60,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,     0,     0,     0,     0,   853,   250,     0,     0,
       0,     0,     0,     0,   348,   349,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   350,     0,     0,     0,     0,     0,    59,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     249,     0,     0,     0,     0,     0,     0,     0,   216,   217,
     218,   699,   220,   221,   222,   223,   224,   444,   226,   227,
     228,   229,   230,   231,   232,   233,   234,   235,   236,     0,
     238,   239,   240,     0,     0,   243,   244,   245,   246,     0,
     651,   652,     0,     0,     0,     0,     0,     0,     0,     0,
     250,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,     0,
       0,   369,   370,   371,     0,     0,   372,   373,   374,   375,
     376,     0,     0,   377,   378,   379,   380,   381,   382,   383,
       0,   854,     0,     0,     0,     0,     0,     0,     0,     0,
     855,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   651,   652,   384,     0,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
       0,     0,   395,   396,     0,   653,   654,   655,   656,   657,
     397,   398,   658,   659,   660,   661,     0,   662,   663,   664,
     665,   666,     0,   667,   668,     0,     0,  -840,     0,   670,
     671,   672,     0,     0,     0,  -840,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   651,   652,   674,     0,   675,   676,   677,   678,
     679,   680,   681,   682,   683,   684,     0,     0,     0,     0,
       0,   653,   654,   655,   656,   657,   685,   686,   658,   659,
     660,   661,     0,   662,   663,   664,   665,   666,     0,   667,
     668,     0,   651,   652,     0,   670,   671,   672,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   651,   652,
     674,     0,   675,   676,   677,   678,   679,   680,   681,   682,
     683,   684,     0,     0,     0,     0,     0,   653,   654,   655,
     656,   657,   685,   686,   658,   659,   660,   661,     0,   662,
     663,   664,   665,   666,     0,   667,   668,     0,   651,   652,
       0,   670,     0,   672,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   653,   654,   655,
     656,   657,     0,     0,   658,   659,   660,   661,     0,   662,
     663,   664,   665,   666,     0,   667,   668,     0,   675,   676,
     677,   678,   679,   680,   681,   682,   683,   684,     0,     0,
       0,     0,     0,   653,   654,   655,   656,   657,   685,   686,
     658,   659,   660,   661,     0,   662,   663,   664,   665,   666,
       0,   667,   668,     0,   651,   652,     0,   670,   675,   676,
     677,   678,   679,   680,   681,   682,   683,   684,     0,     0,
       0,     0,     0,   653,   654,   655,   656,   657,   685,   686,
     658,   659,   660,   661,     0,   662,   663,   664,   665,   666,
       0,   667,   668,     0,   675,   676,   677,   678,   679,   680,
     681,   682,   683,   684,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   685,   686,     0,   857,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     651,   652,     0,     0,     0,   676,   677,   678,   679,   680,
     681,   682,   683,   684,     0,     0,     0,     0,     0,   653,
     654,   655,   656,   657,   685,   686,   658,   659,   660,   661,
       0,   662,   663,   664,   665,   666,     0,   667,   668,   216,
     217,   218,     0,   220,   221,   222,   223,   224,   444,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
       0,   238,   239,   240,     0,     0,   243,   244,   245,   246,
       0,     0,     0,     0,     0,     0,  1080,     0,     0,     0,
       0,     0,   677,   678,   679,   680,   681,   682,   683,   684,
       0,     0,     0,     0,     0,   653,   654,   655,   656,   657,
     685,   686,   658,   659,   660,   661,     0,   662,   663,   664,
     665,   666,     0,   667,   668,     0,     0,     0,     0,     0,
       0,     0,   858,     0,     0,     0,     0,     0,   216,   217,
     218,   859,   220,   221,   222,   223,   224,   444,   226,   227,
     228,   229,   230,   231,   232,   233,   234,   235,   236,   269,
     238,   239,   240,     0,     0,   243,   244,   245,   246,   678,
     679,   680,   681,   682,   683,   684,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   685,   686,     0,     0,
       0,     0,     0,     0,     0,   270,     0,   271,     0,   272,
     273,   274,   275,   276,     0,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,     0,   288,   289,   290,
       0,  1081,   291,   292,   293,   294,     0,     0,     0,     0,
    1082,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   295,   296,   216,   217,   218,     0,   220,   221,
     222,   223,   224,   444,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,     0,   238,   239,   240,     0,
       0,   243,   244,   245,   246,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   297,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   896,   897,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   898,     0,     0,
       0,     0,     0,   270,     0,   271,   899,   272,   273,   274,
     275,   276,     0,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,     0,   288,   289,   290,     0,     0,
     291,   292,   293,   294,     0,     0,     0,     0,     0,     0,
     900,   901,   270,     0,   271,     0,   272,   273,   274,   275,
     276,     0,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,     0,   288,   289,   290,     0,     0,   291,
     292,   293,   294,     0,   270,     0,   271,     0,   272,   273,
     274,   275,   276,     0,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   539,   288,   289,   290,     0,
       0,   291,   292,   293,   294,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   541,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   724
};

static const yytype_int16 yycheck[] =
{
       1,    14,    15,   331,   144,   145,   571,   567,   171,   430,
     579,   764,   430,   343,   942,   471,   636,   637,   186,    20,
      21,    22,  1034,   509,   694,   511,   696,   513,   698,   538,
    1167,  1007,   843,   537,   641,  1246,   931,    82,   925,   348,
     349,   813,  1537,    22,     8,    20,    33,    19,  1582,   810,
      33,    52,    65,    66,    67,   950,    19,    20,  1582,   852,
    1582,  1371,   518,    20,    20,     5,    20,   148,     0,  1582,
     186,  1582,   140,   141,   142,     7,    40,    46,    57,   126,
    1475,   165,   163,   173,   165,  1013,  1334,   151,   708,     7,
     420,   181,   165,   180,   107,   108,   109,   110,    30,  1523,
      32,   154,    34,   154,   156,   127,   154,  1641,    40,    62,
     163,   133,   163,   200,   204,   163,   200,  1641,    50,  1641,
     148,   129,   130,   204,    56,   206,   151,   200,  1641,   176,
    1641,    33,    50,   201,   127,   163,   705,   106,  1533,   203,
     709,     5,     6,  1677,   600,   308,    15,    16,    80,  1573,
      15,    16,   174,  1677,   610,  1677,   201,   613,    60,    61,
     300,    25,   102,   103,  1677,   305,  1677,    31,   127,   309,
     102,   103,   200,   186,   133,   343,   201,   797,   203,  1674,
     800,   174,   789,  1493,   177,   641,   163,   173,   808,   197,
     198,   811,   179,  1688,  1442,  1443,   179,   173,   173,   200,
     163,   165,   995,   975,    68,    69,   185,  1592,   180,   774,
    1458,   198,  1460,   974,  1226,   174,   173,   173,   197,   173,
     207,   163,   124,   165,  1200,   202,   128,  1122,  1115,   798,
     165,   687,   148,   165,   198,   842,   545,   250,   102,   103,
     180,   180,   206,  1138,  1629,   126,   126,   163,   669,   129,
     130,   669,   420,    34,   186,   423,   424,   425,   185,   173,
     200,   200,   204,   174,  1192,   310,   198,  1515,  1516,   204,
     176,   601,   204,   175,  1522,   139,   154,   179,   418,   181,
     182,  1492,    63,   139,   200,   163,   616,   156,    45,   205,
     204,   156,   161,   180,   163,   176,   161,   166,   163,   163,
     181,   166,  1512,   154,   165,   207,   185,   423,   424,   425,
     163,  1521,   163,   200,  1451,   127,   180,   197,   198,   200,
     176,   133,   200,   126,   126,   126,   107,  1339,    85,    86,
     343,    88,   146,   789,   198,   200,   504,   505,   203,    33,
     508,   205,   510,   204,   512,   206,   514,   200,   180,   163,
     201,   132,   520,     8,   923,  1565,  1566,   163,   139,   966,
    1030,  1373,   174,   849,   180,  1350,    60,    61,   200,   537,
     163,   185,   186,   176,   176,   176,  1304,   173,   181,   181,
     181,    36,   163,   197,   200,   127,   554,   555,   504,   505,
     955,   133,   508,   139,   510,  1206,   512,   200,   200,   200,
    1020,   970,  1387,   173,   520,   201,   200,   420,  1384,   190,
     423,   424,   425,   922,   127,   919,   127,   430,   431,   200,
     133,   163,   173,   173,   173,   994,   935,    33,   107,   933,
     124,   201,   174,   601,   128,   154,   139,   179,   554,   555,
     164,   165,   146,   773,   163,   173,    21,    22,   616,   173,
     201,   201,   201,   177,    60,    61,   786,   787,   127,   163,
     163,   174,   173,   174,   133,   176,   796,   156,   179,  1280,
     139,   199,   802,   803,   173,   805,  1229,   807,   173,   809,
     204,   175,   186,   651,   652,   179,   173,   181,   182,   173,
     127,   504,   505,   197,   165,   508,   133,   510,   666,   512,
     640,   514,   201,   180,   199,   174,   177,   520,   173,   197,
     173,   139,   173,   207,   201,   199,   163,   685,   124,   173,
     148,   127,   128,   200,   537,   153,   165,   133,    57,  1199,
     173,  1017,   173,   204,    63,   163,   201,   174,   201,   707,
     201,   554,   555,   118,   119,   199,  1474,    21,    22,   562,
     173,   126,   180,   128,   129,   130,   131,   132,   201,   165,
     201,   173,   164,   165,   173,   173,   173,   148,   174,   175,
     571,   173,   173,   179,  1586,  1587,   182,    47,   201,   181,
      21,    22,   163,   140,   173,   142,  1072,   173,   601,   201,
    1612,   707,   201,   201,   201,  1164,  1266,    67,   204,   148,
     201,   207,   204,   616,   163,   773,   164,   182,   183,   184,
     185,   186,   201,   199,   163,   173,  1638,   785,   786,   787,
     788,  1077,   197,   198,   792,   163,   639,   139,   796,   642,
    1642,    21,    22,  1089,   802,   803,   148,   805,  1094,   807,
      57,   809,   810,   973,   118,   119,    63,   167,   168,   979,
    1662,   163,   126,    79,   128,   129,   130,   131,   132,    12,
      65,    66,    67,   993,   163,   164,     5,     6,    94,   785,
      23,    24,   788,    99,   173,   101,   792,   118,   119,   167,
     176,   163,   163,   164,   180,   126,  1016,   128,   129,   130,
     131,   132,   173,   177,   707,   204,   205,   181,   164,   165,
    1511,  1512,   107,   108,   109,   110,  1517,   173,   164,   165,
    1521,   176,  1523,    57,   179,   181,   884,   173,   154,    63,
     721,   107,   723,   197,   198,   181,  1182,   163,   118,   119,
     163,   173,   107,  1189,   176,   163,   126,   179,   204,   129,
     130,   131,   132,   184,   185,   186,   177,  1558,   204,   917,
     181,   919,   920,    57,  1565,  1566,   197,   198,   926,    63,
     773,   173,  1573,    57,   176,   933,   177,   179,   163,    63,
     181,    57,   785,   786,   787,   788,   777,    63,    57,   792,
     205,   177,   203,   796,    63,   181,   164,   165,    57,   802,
     803,   174,   805,   190,   807,   173,   809,   810,   177,  1552,
     176,   917,   181,   181,   920,   973,   974,   197,   198,    19,
     926,   979,   177,   177,   177,    25,   181,   181,   181,   177,
    1631,    31,   140,   181,   167,   993,   204,   165,   318,   177,
      33,    41,   177,   181,    10,    11,   181,  1258,   328,    49,
    1258,   167,   168,   169,   170,    66,   202,   203,  1016,   205,
     851,   341,   163,   487,    64,   489,   490,    60,    61,   489,
     490,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   203,  1351,   140,   141,   142,  1652,
    1520,   163,  1042,   164,   917,   176,   919,   920,   106,   164,
     165,   203,   163,   926,   167,   168,   169,   180,   173,   200,
     933,   124,   164,   165,   127,   128,   181,  1680,   180,   139,
     133,   173,   933,   180,   935,   164,   165,  1267,   198,   181,
    1560,   180,   152,   180,   173,   180,   947,   180,   180,   204,
     164,   165,   181,   163,   955,   200,   957,   180,   180,   173,
     973,   974,   204,   164,   200,  1133,   979,   181,   164,   165,
     177,   174,   175,    35,   205,   204,   179,   173,   205,   182,
     993,  1144,    35,  1543,   200,   181,   163,    75,   163,   180,
     204,    79,   202,   198,   205,    21,    22,   207,   163,  1455,
     167,   168,   169,  1016,   207,    93,    94,   199,   204,    22,
      98,    99,   100,   101,    56,    57,    58,  1133,   756,   757,
     758,  1477,   163,   138,  1589,   199,   205,   176,   163,   180,
     180,   521,   180,   180,   200,   180,   200,   180,  1039,   200,
     180,  1181,   180,   533,   200,   200,  1047,  1048,   200,   203,
     163,   181,   173,    43,   200,    33,   200,    13,   200,  1060,
    1625,  1062,  1063,  1064,  1065,  1066,   200,   198,   201,  1070,
      21,    22,   200,   200,  1214,   201,   201,   201,   200,   199,
     199,   204,    60,    61,  1540,   575,   181,   201,   205,   200,
     116,   117,   118,   119,   120,   180,   180,   123,   180,  1267,
     126,  1269,   128,   129,   130,   131,   132,   180,   134,   135,
     200,   200,   602,   603,   173,    33,   606,     4,   608,   199,
    1133,  1449,   201,   163,   163,   203,   176,   199,   163,   619,
     620,   621,   622,   623,   624,   163,   163,   173,   163,   163,
     201,   201,    60,    61,   201,   201,   124,   201,  1149,   201,
     128,   180,   201,  1269,   180,   181,   182,   183,   184,   185,
     186,     1,   180,   201,   201,   116,   117,   118,   119,   200,
     200,   197,   198,    43,   664,   126,   199,   128,   129,   130,
     131,   132,   201,   134,   135,   200,   164,   165,   199,   201,
     200,   200,   200,   200,   200,   173,   686,   175,   200,   200,
      33,   179,   200,   181,   182,   181,   124,  1375,   181,   181,
     128,   200,    33,   201,   174,   201,   200,   174,   201,    43,
     201,   201,   201,   713,   201,   200,   204,    60,    61,   207,
     201,   182,   183,   184,   185,   186,   201,   201,   200,    60,
      61,   176,   200,    43,   156,   200,   197,   198,   163,   201,
    1418,   200,   206,   201,  1267,   163,  1269,   175,  1588,  1375,
     163,   179,   752,   181,   182,    10,  1434,    13,    21,    22,
       9,  1411,    42,    66,   180,   200,   200,   199,   163,   200,
     770,   163,   199,   206,   163,   163,   776,   163,     8,   207,
     206,   124,   206,   200,   163,   128,   201,   163,   171,   163,
      33,   791,  1418,   124,   201,   201,   201,   128,   163,   163,
     200,   163,  1313,   163,  1315,    14,   174,   174,  1434,   156,
     176,   201,   206,   200,    43,   200,   200,    60,    61,   200,
     174,    37,   822,   200,   206,   174,   201,   827,   201,   829,
     201,   831,   175,    67,    70,   181,   179,   200,   181,   182,
     201,   841,   200,   200,   175,   201,   201,   200,   179,   200,
     181,   182,  1375,   116,   117,   118,   119,   120,   200,   200,
     123,   124,   125,   126,   207,   128,   129,   130,   131,   132,
    1381,   134,   135,   163,    43,   138,   207,   140,   141,   142,
     201,   124,   181,   146,   163,   128,   201,   201,   201,  1539,
     163,  1541,  1542,   201,   200,  1418,   200,   897,   200,   200,
     200,   901,   167,   201,   163,   201,   201,   204,   201,   201,
    1588,  1434,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   206,   201,   201,   927,   205,   201,
      33,   201,   175,   201,   197,   198,   179,   204,   181,   182,
     108,   109,   110,   111,   112,   113,   114,   115,    12,   173,
      21,    22,  1475,   173,   201,   201,   956,   201,   200,   127,
     201,   201,   201,   201,   207,   133,    53,   204,   201,   204,
     201,    81,  1622,   201,   199,   143,   144,   145,   205,   200,
     980,  1492,   199,   206,   206,     1,   201,   206,   206,   136,
      33,   205,    84,  1687,  1505,   723,   723,  1647,  1685,   625,
     207,   106,   951,  1653,   646,     1,   174,   707,  1507,  1369,
    1533,  1556,  1510,  1557,  1014,  1015,  1557,    60,    61,   448,
     449,    55,   642,  1437,  1138,  1025,   950,   950,  1678,   430,
     225,   302,  1032,    -1,   430,  1035,   465,   466,   467,   468,
     469,  1041,    -1,  1043,   602,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,  1588,    -1,   138,    -1,   140,
     141,   142,  1583,    -1,  1074,   146,  1076,    -1,  1078,    -1,
    1591,   124,   430,    -1,    -1,   128,   430,    -1,  1088,    -1,
     430,   430,    -1,    -1,    -1,    -1,  1096,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,  1628,    -1,    -1,
    1120,    -1,    -1,    -1,    -1,    -1,   197,   198,   557,    -1,
    1130,    -1,   175,  1644,    -1,  1135,   179,  1137,   181,   182,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1658,  1148,  1660,
      -1,    -1,    -1,    -1,  1665,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   207,    -1,    -1,  1167,    -1,    -1,
    1681,    -1,  1683,   108,   109,   110,   111,   112,   113,   114,
     115,    -1,    -1,    -1,    -1,  1185,    -1,    -1,    -1,    -1,
      -1,    -1,    10,  1193,  1194,  1195,    -1,    -1,   133,    -1,
      -1,    -1,    -1,    21,    22,    -1,    -1,    -1,   143,   144,
     145,    -1,    -1,    -1,    -1,  1215,    -1,    -1,  1218,    -1,
      -1,    -1,    -1,    -1,   653,   654,    -1,  1227,   657,   658,
     659,   660,    -1,   662,    -1,    -1,   665,   666,   667,   668,
     669,   670,   671,   672,   673,   674,   675,   676,   677,   678,
     679,   680,   681,   682,   683,   684,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1275,    -1,  1277,    -1,    -1,
      -1,    -1,    -1,  1283,    -1,    -1,    -1,    -1,    -1,    -1,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,  1321,   140,   141,   142,   143,   144,   145,   146,    -1,
      -1,    -1,    -1,   762,    -1,    -1,    -1,    -1,  1338,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1352,  1353,    -1,    -1,   174,   175,    -1,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1376,    -1,    -1,   197,
     198,    -1,  1382,  1383,    -1,  1385,  1386,    -1,    -1,    -1,
       1,    -1,    -1,    -1,     5,     6,     7,    -1,     9,    10,
      11,    -1,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      31,    -1,   851,    -1,    -1,    -1,  1426,    38,    39,    40,
    1430,    42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,
      51,    52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,
      -1,  1451,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,  1514,    -1,    -1,    -1,   947,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1526,    -1,   139,    -1,
    1530,  1531,    -1,    -1,    -1,    -1,   147,   148,   149,   150,
     151,    -1,   153,    -1,   155,   156,   157,   158,   159,   160,
     161,   162,   163,    -1,   165,   166,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   182,   183,   184,    -1,   186,  1576,    -1,   189,   190,
      -1,    -1,    -1,    60,    61,    -1,    -1,   198,    -1,   200,
      -1,   202,   203,   204,   205,   206,    -1,    -1,     5,     6,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,    16,
      17,    18,    19,  1613,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,  1626,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,  1635,    -1,    -1,    45,    -1,
      -1,    48,    -1,  1643,    51,    -1,    -1,   124,    55,    -1,
      -1,   128,    -1,    -1,    -1,  1084,    -1,    -1,    65,    -1,
      -1,    68,    69,    70,    71,    72,    73,  1667,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   175,    -1,
      -1,    -1,   179,    -1,   181,   182,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     207,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     147,   148,   149,   150,   151,    -1,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,    -1,    -1,   166,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,   186,
      -1,    -1,   189,   190,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,    -1,   200,   201,   202,   203,    -1,   205,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1248,
    1249,  1250,  1251,  1252,  1253,  1254,  1255,  1256,  1257,  1258,
    1259,  1260,  1261,  1262,  1263,  1264,  1265,    -1,    -1,    -1,
       1,    -1,    -1,    -1,     5,     6,     7,    -1,     9,    10,
      11,    -1,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    40,
      -1,    42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,
      51,    52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,
      -1,  1330,  1331,  1332,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    33,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,    33,    -1,    -1,    -1,    -1,
      -1,    -1,  1381,    -1,    -1,    -1,    33,   118,   119,    -1,
      -1,    -1,    60,    61,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    60,    61,    -1,    -1,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    60,    61,    -1,   147,   148,   149,   150,
     151,    -1,   153,    -1,   155,   156,   157,   158,   159,   160,
     161,   162,   163,    -1,   165,   166,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   182,   183,   184,    -1,   186,   124,    -1,   189,   190,
     128,    -1,    -1,    -1,    -1,    -1,   124,   198,    -1,   200,
     128,   202,   203,   204,   205,   206,    -1,   124,    -1,    -1,
      -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,    -1,
      -1,   179,    -1,   181,   182,    -1,    -1,   175,    -1,    -1,
      -1,   179,    -1,   181,   182,    -1,    -1,    -1,   175,    -1,
      -1,    -1,   179,    -1,   181,   182,    -1,    -1,    -1,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,   207,
      -1,     5,     6,     7,    -1,     9,    10,    11,    -1,    13,
     207,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    26,    27,    28,    29,    -1,    31,    -1,    -1,
      -1,    -1,    -1,  1582,    38,    39,    40,    -1,    42,    -1,
      44,    45,  1591,    -1,    48,    -1,    50,    51,    52,    -1,
      54,    55,    -1,    -1,    58,    59,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,  1641,    -1,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   118,   119,  1665,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1677,    -1,
      -1,    -1,    -1,    -1,  1683,   139,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,   153,
      -1,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,   165,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,
     184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   198,    -1,   200,    -1,   202,   203,
     204,   205,   206,     1,    -1,    -1,    -1,     5,     6,     7,
      -1,     9,    10,    11,    -1,    13,    -1,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    29,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      38,    39,    40,    -1,    42,    -1,    44,    45,    -1,    -1,
      48,    -1,    50,    51,    52,    -1,    54,    55,    -1,    -1,
      58,    59,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,
     148,   149,   150,   151,    -1,   153,    -1,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,   165,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   182,   183,   184,    -1,   186,    -1,
      -1,   189,   190,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     198,    -1,   200,    -1,   202,   203,   204,   205,   206,     1,
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
     102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,
      -1,   153,    -1,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,   165,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     182,   183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   198,    -1,   200,    -1,
     202,   203,   204,   205,   206,     1,    -1,    -1,    -1,     5,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   147,   148,   149,   150,   151,    -1,   153,    -1,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    -1,   165,
     166,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,
     186,    -1,    -1,   189,   190,    -1,    -1,    -1,    -1,    60,
      61,    -1,   198,    -1,   200,    -1,   202,   203,   204,   205,
     206,     5,     6,    -1,    -1,    -1,    -1,    -1,    -1,    13,
      -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    49,    -1,    51,    -1,    -1,
      -1,    55,    -1,   124,    -1,    -1,    -1,   128,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,   175,    -1,    -1,    -1,   179,    -1,
     181,   182,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   139,   207,    -1,    -1,    -1,
      -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,
     184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,
      -1,    -1,     5,     6,   198,    -1,   200,    -1,   202,   203,
      13,   205,    15,    16,    17,    18,    19,    -1,    -1,    -1,
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
      -1,    -1,   128,    -1,   127,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,
     153,    -1,   155,   156,   157,   158,   159,   160,   161,   162,
     163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,   175,
      -1,    -1,    -1,   179,    -1,   181,   182,    -1,    -1,   182,
     183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,
      -1,    -1,    -1,     5,     6,   198,    -1,   200,    -1,   202,
     203,   207,   205,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    33,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    60,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    70,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,   124,
      -1,    -1,    -1,   128,    -1,   127,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,
      -1,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
     175,    -1,    -1,    -1,   179,    -1,   181,   182,    -1,    -1,
     182,   183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,
      -1,    -1,    -1,    -1,     5,     6,   198,    -1,   200,    -1,
     202,   203,   207,   205,    15,    16,    17,    18,    19,    -1,
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
     124,    -1,    -1,    -1,   128,    -1,   127,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,
     151,    -1,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,    -1,    -1,   166,    -1,    -1,    -1,    -1,
      -1,   175,    -1,    -1,    -1,   179,    -1,    -1,   182,    -1,
      -1,   182,   183,   184,    -1,   186,    -1,    -1,   189,   190,
      -1,    -1,    -1,    -1,    -1,     5,     6,   198,    -1,   200,
      -1,   202,   203,   207,   205,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,
     150,   151,    -1,   153,    -1,   155,   156,   157,   158,   159,
     160,   161,   162,   163,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   182,   183,   184,    -1,   186,    -1,    -1,   189,
     190,    -1,    -1,    -1,    -1,    -1,     5,     6,   198,    -1,
     200,   201,   202,   203,    -1,   205,    15,    16,    17,    18,
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
     139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,   148,
     149,   150,   151,    -1,   153,    -1,   155,   156,   157,   158,
     159,   160,   161,   162,   163,    -1,    -1,   166,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   182,   183,   184,    -1,   186,    -1,    -1,
     189,   190,    -1,    -1,    -1,    -1,    -1,     5,     6,   198,
      -1,   200,   201,   202,   203,    -1,   205,    15,    16,    17,
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
      -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,
     148,   149,   150,   151,    -1,   153,    -1,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   182,   183,   184,    -1,   186,    -1,
      -1,   189,   190,    -1,    -1,    -1,    -1,    -1,     5,     6,
     198,    -1,   200,   201,   202,   203,    -1,   205,    15,    16,
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
      -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     147,   148,   149,   150,   151,    -1,   153,    -1,   155,   156,
     157,   158,   159,   160,   161,   162,   163,    -1,    -1,   166,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,   186,
      -1,    -1,   189,   190,    -1,    -1,    -1,    -1,    -1,     5,
       6,   198,    -1,   200,   201,   202,   203,    -1,   205,    15,
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
      -1,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   147,   148,   149,   150,   151,    -1,   153,    -1,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,
     186,    -1,    -1,   189,   190,    -1,    -1,    -1,    -1,    -1,
       5,     6,   198,    -1,   200,   201,   202,   203,    -1,   205,
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
      -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   147,   148,   149,   150,   151,    -1,   153,    -1,
     155,   156,   157,   158,   159,   160,   161,   162,   163,    -1,
      -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,
      -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,    -1,
      -1,     5,     6,   198,    -1,   200,    -1,   202,   203,    -1,
     205,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,   153,
      -1,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,
     184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,
      -1,    -1,     5,     6,   198,   199,   200,    -1,   202,   203,
      -1,   205,    15,    16,    17,    18,    19,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,
     153,    -1,   155,   156,   157,   158,   159,   160,   161,   162,
     163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,
     183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,
      -1,    -1,    -1,     5,     6,   198,   199,   200,    -1,   202,
     203,    -1,   205,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,
      -1,   153,    -1,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     182,   183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,
      -1,    -1,    -1,    -1,     5,     6,   198,    -1,   200,    -1,
     202,   203,    13,   205,    15,    16,    17,    18,    19,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,
     151,    -1,   153,    -1,   155,   156,   157,   158,   159,   160,
     161,   162,   163,    -1,    -1,   166,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   182,   183,   184,    -1,   186,    -1,    -1,   189,   190,
      -1,    -1,    -1,    -1,    -1,     5,     6,   198,    -1,   200,
      -1,   202,   203,    -1,   205,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,
     150,   151,    -1,   153,    -1,   155,   156,   157,   158,   159,
     160,   161,   162,   163,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   182,   183,   184,    -1,   186,    -1,    -1,   189,
     190,    -1,    -1,    -1,    -1,    -1,     5,     6,   198,    -1,
     200,    -1,   202,   203,    -1,   205,    15,    16,    17,    18,
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
     139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,   148,
     149,   150,   151,    -1,   153,    -1,   155,   156,   157,   158,
     159,   160,   161,   162,   163,    -1,    -1,   166,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   182,   183,   184,    -1,   186,    -1,    -1,
     189,   190,    -1,    -1,    -1,    -1,    -1,     5,     6,   198,
      -1,   200,    -1,   202,   203,    -1,   205,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    61,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,
     148,   149,   150,   151,    -1,   153,    -1,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   182,   183,   184,    -1,   186,    -1,
      -1,   189,   190,    -1,    -1,    -1,    -1,    -1,     5,     6,
     198,    -1,   200,    -1,   202,   203,    -1,   205,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,
      -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     147,   148,   149,   150,   151,    -1,   153,    -1,   155,   156,
     157,   158,   159,   160,   161,   162,   163,    -1,    -1,   166,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,   186,
      -1,    -1,   189,   190,    -1,    -1,    -1,    -1,    -1,     5,
       6,   198,    -1,   200,    -1,   202,   203,    -1,   205,    15,
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
      -1,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   147,   148,   149,   150,   151,    -1,   153,    -1,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,
     186,    -1,    -1,   189,   190,    -1,    -1,    -1,    -1,    -1,
       5,     6,   198,    -1,   200,    -1,   202,   203,    -1,   205,
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
      -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   147,   148,   149,   150,   151,    -1,   153,    -1,
     155,   156,   157,   158,   159,   160,   161,   162,   163,    -1,
      -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,
      -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,    -1,
      -1,     5,     6,   198,    -1,   200,   201,   202,   203,    -1,
     205,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,   153,
      -1,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,
     184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,
      -1,    -1,     5,     6,   198,    -1,   200,    -1,   202,   203,
      -1,   205,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    52,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,
     153,    -1,   155,   156,   157,   158,   159,   160,   161,   162,
     163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,
     183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,
      -1,     5,     6,    -1,    -1,   198,    -1,   200,    -1,   202,
     203,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,   153,
      -1,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,
     184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,
       5,     6,    -1,    -1,   198,    -1,   200,    -1,   202,   203,
      15,    16,    17,    18,    19,    -1,    -1,    22,    -1,    -1,
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
      -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   147,   148,   149,   150,   151,    -1,   153,    -1,
     155,   156,   157,   158,   159,   160,   161,   162,   163,    -1,
      -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,
      -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,     5,
       6,    -1,    -1,   198,    -1,   200,    -1,   202,   203,    15,
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
      -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   147,   148,   149,   150,   151,    -1,   153,    -1,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    -1,    19,
     166,    -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,
     186,    41,    -1,   189,   190,    -1,    -1,    -1,    -1,    49,
      -1,    -1,   198,    -1,   200,    -1,   202,   203,    -1,    -1,
      -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      -1,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,   139,
      -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   152,    -1,    -1,    -1,    -1,    -1,    -1,    64,
      -1,    -1,    -1,   163,    -1,   165,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,    -1,
      -1,    -1,   202,    -1,   204,    -1,    -1,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,
      -1,    -1,    -1,    -1,   139,    -1,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   152,    -1,    -1,
      -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,   163,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,    -1,    -1,    -1,    -1,    19,   202,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     152,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,
      73,   163,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,    -1,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,    -1,
      -1,   128,   129,   130,    -1,    -1,   133,   134,   135,   136,
     137,    -1,    -1,   140,   141,   142,   143,   144,   145,   146,
      -1,   154,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     163,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,   189,   190,    -1,   116,   117,   118,   119,   120,
     197,   198,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,   138,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,   197,   198,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    21,    22,    -1,   140,   141,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,   197,   198,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    21,    22,
      -1,   140,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,   197,   198,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    21,    22,    -1,   140,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,   197,   198,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   197,   198,    -1,    19,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,    -1,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,   116,
     117,   118,   119,   120,   197,   198,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      -1,    -1,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,   116,   117,   118,   119,   120,
     197,   198,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   154,    -1,    -1,    -1,    -1,    -1,    71,    72,
      73,   163,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    35,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   197,   198,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    -1,    73,    -1,    75,
      76,    77,    78,    79,    -1,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,   154,    98,    99,   100,   101,    -1,    -1,    -1,    -1,
     163,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   118,   119,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   163,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   129,   130,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,    -1,
      -1,    -1,    -1,    71,    -1,    73,   163,    75,    76,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,    -1,
     197,   198,    71,    -1,    73,    -1,    75,    76,    77,    78,
      79,    -1,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,    -1,    71,    -1,    73,    -1,    75,    76,
      77,    78,    79,    -1,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,   163,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   163,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   163
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   209,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   102,   103,   165,   186,   198,   204,   211,   212,   216,
     225,   227,   228,   232,   280,   286,   317,   399,   407,   414,
     424,   474,   479,   484,    19,    20,   163,   271,   272,   273,
     156,   233,   234,   146,   163,   186,   197,   229,   230,    57,
      63,   404,   405,   163,   202,   214,   485,   475,   480,   139,
     163,   305,    34,    63,   107,   132,   190,   200,   275,   276,
     277,   278,   305,   211,   211,   211,     8,    36,   425,    62,
     395,   174,   173,   176,   173,   185,   185,   229,   185,    22,
      57,   185,   197,   231,   163,   211,   395,   404,   404,   404,
     163,   139,   226,   277,   277,   277,   200,   140,   141,   142,
     173,   199,   107,   285,   415,     5,     6,   421,    57,    63,
     396,    15,    16,   156,   161,   163,   166,   200,   203,   218,
     272,   156,   234,   229,   229,   229,   163,   163,   163,   406,
      57,    63,   213,   163,   163,   163,   163,   167,   224,   201,
     273,   277,   277,   277,   277,   165,   238,   239,    57,    63,
     287,   289,    57,    63,   408,   107,   107,    57,    63,   422,
     205,   400,   167,   168,   169,   217,    15,    16,   156,   161,
     163,   218,   269,   270,   203,   231,   174,   190,   215,   176,
     435,   239,   239,   167,   201,   165,   290,   163,   409,   426,
     397,   203,   274,   365,   167,   168,   169,   173,   201,   163,
      19,    25,    31,    41,    49,    64,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   152,
     202,   305,   429,   431,   432,   436,   442,   444,   473,    66,
      79,    94,    99,   101,   164,   412,   413,   476,   481,    35,
      71,    73,    75,    76,    77,    78,    79,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    93,    94,
      95,    98,    99,   100,   101,   118,   119,   163,   283,   284,
     288,   176,   410,   106,   419,   420,   206,   211,   398,   272,
     203,   163,   391,   394,   269,   180,   180,   180,   200,   180,
     180,   200,   435,   180,   180,   180,   180,   180,   200,   305,
     180,   200,    33,    60,    61,   124,   128,   175,   179,   182,
     207,   198,   441,   177,   164,   486,   205,   205,    21,    22,
      38,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   128,
     129,   130,   133,   134,   135,   136,   137,   140,   141,   142,
     143,   144,   145,   146,   175,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   189,   190,   197,   198,    35,
      35,   200,   281,   239,    75,    79,    93,    94,    98,    99,
     100,   101,   430,   413,   163,   239,   365,   239,   272,   173,
     176,   179,   389,   445,   451,   453,     5,     6,    15,    16,
      17,    18,    19,    25,    27,    31,    39,    45,    48,    51,
      55,    65,    68,    69,    80,   102,   103,   104,   118,   119,
     147,   148,   149,   150,   151,   153,   155,   156,   157,   158,
     159,   160,   161,   162,   166,   182,   183,   184,   189,   190,
     198,   200,   202,   203,   205,   223,   225,   297,   305,   310,
     322,   329,   332,   335,   339,   341,   343,   344,   346,   351,
     354,   355,   356,   363,   364,   429,   490,   498,   509,   512,
     525,   526,   529,   530,   455,   449,   163,   180,   457,   459,
     461,   463,   465,   467,   469,   471,   355,   180,   200,   443,
     447,   127,   302,   333,   355,    33,   179,    33,   179,   198,
     207,   199,   355,   198,   207,   442,   205,   477,   482,   163,
     284,   163,   284,   163,   199,    22,   163,   199,   151,   201,
     365,   375,   376,   377,   126,   176,   282,   138,   294,   295,
     334,   205,   176,   418,   427,   148,   163,   390,   393,   239,
     163,   442,   127,   133,   174,   388,   473,   473,   440,   473,
     180,   180,   180,   305,   307,   431,   489,   498,   509,   512,
     525,   526,   529,   530,   305,   180,     5,   102,   103,   180,
     200,   180,   200,   200,   180,   180,   200,   180,   200,   180,
     200,   180,   180,   200,    19,   180,   180,   356,   356,   200,
     200,   200,   200,   200,   200,   222,   356,   356,   356,   356,
     356,    13,    49,   302,   154,   163,   333,   491,   493,   203,
     524,   200,   198,   279,   203,   205,   335,   340,   340,   340,
     201,    21,    22,   116,   117,   118,   119,   120,   123,   124,
     125,   126,   128,   129,   130,   131,   132,   134,   135,   138,
     140,   141,   142,   146,   175,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   197,   198,   200,   473,   473,
     201,   181,   437,   473,   281,   473,   281,   473,   281,   163,
     378,   379,   473,   163,   381,   382,   201,   448,   333,   304,
     473,   355,   201,   173,   528,   199,   199,   199,   355,   487,
     378,   380,   381,   383,   163,   284,   108,   109,   110,   111,
     112,   113,   114,   115,   133,   143,   144,   145,   108,   109,
     110,   111,   112,   113,   114,   115,   127,   133,   143,   144,
     145,   174,   200,     7,    50,   316,   204,   173,   204,   201,
     473,   473,   127,   356,   205,   416,   305,   204,   205,   423,
     200,    43,   173,   176,   389,   211,   388,   355,   181,   181,
     181,   164,   173,   210,   211,   439,   499,   501,   308,   200,
     180,   200,   330,   180,   180,   180,   519,   333,   442,   355,
     523,   355,   323,   325,   355,   327,   355,   521,   333,   507,
     510,   333,   180,   503,   442,   355,   355,   355,   355,   355,
     355,   169,   170,   217,   200,    13,   199,   200,   127,   133,
     174,   384,   528,   173,   528,   201,   148,   153,   180,   305,
     345,   239,    70,   198,   201,   333,   493,   278,     4,   338,
     203,   301,   279,    19,   154,   163,   429,    19,   154,   163,
     429,   356,   356,   356,   356,   356,   356,   163,   356,   154,
     163,   355,   356,   356,   429,   356,   356,   356,   525,   530,
     356,   356,   356,   356,    22,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   129,   130,   154,   163,
     197,   198,   352,   429,   355,   201,   333,   181,   181,   163,
     433,   181,   282,   181,   282,   181,   282,   176,   181,   439,
     176,   181,   439,   304,   528,   181,   439,   127,   355,   199,
     163,   434,   211,   246,   247,   246,   247,   355,   148,   163,
     385,   386,   428,   377,   377,   377,   356,   301,   163,   401,
     403,   371,   355,   163,   163,   442,   388,   355,   211,   446,
     452,   454,   473,   442,   442,   473,    70,   333,   493,   497,
     163,   355,   473,   513,   515,   517,   442,   528,   181,   439,
     173,   528,   201,   442,   442,   201,   442,   201,   442,   528,
     442,   379,   528,   505,   382,   181,   201,   201,   201,   201,
     201,   201,   355,   148,   163,   200,   260,   200,   355,   355,
     355,   201,   154,   163,   200,   200,   347,   349,    13,   303,
     523,   163,   201,   493,   491,   173,   201,   201,   199,   200,
     281,     1,    26,    28,    29,    38,    40,    44,    52,    54,
      58,    59,    65,   105,   205,   206,   211,   235,   236,   245,
     256,   257,   259,   261,   262,   263,   264,   265,   266,   267,
     268,   298,   306,   311,   312,   313,   314,   315,   317,   321,
     342,   356,   338,   180,   200,   180,   200,   200,   200,   199,
      19,   154,   163,   429,   176,   154,   163,   355,   200,   200,
     154,   163,   355,     1,   200,   199,   173,   201,   456,   450,
     173,   181,   204,   458,   181,   462,   181,   466,   181,   473,
     470,   378,   473,   472,   381,   181,   201,   443,   473,   355,
     174,   210,   402,   411,   211,   378,   478,   381,   483,   201,
     200,    43,   173,   176,   179,   384,   296,   174,   402,   411,
      40,   165,   206,   280,   372,   201,    43,   211,   388,   355,
     211,   181,   181,   181,   493,   201,   201,   201,   181,   439,
     201,   181,   442,   379,   382,   181,   201,   200,   442,   355,
     201,   181,   181,   181,   181,   201,   181,   181,   201,   442,
     181,   338,   200,   176,   220,   200,    43,   163,   319,    20,
     173,   260,   201,   200,   133,   384,   355,   355,   442,   281,
     200,   206,   528,   201,   173,   199,   198,   127,   133,   163,
     174,   179,   336,   337,   282,   127,   355,   294,    61,   355,
     163,   163,   211,   156,    58,   355,   239,   127,   355,   299,
     211,   211,    10,    10,    11,   243,    13,     9,    42,   211,
     211,   211,   211,   211,   211,    66,   318,   211,   108,   109,
     110,   111,   112,   113,   114,   115,   121,   122,   127,   133,
     136,   137,   143,   144,   145,   174,   281,   357,   355,   359,
     355,   201,   333,   355,   180,   200,   356,   200,   199,   355,
     198,   201,   333,   200,   199,   353,   201,   333,   163,   438,
     163,   460,   464,   468,   443,   355,   163,   210,   488,   206,
     206,   355,   163,   163,   473,   355,   206,   355,   401,   417,
     163,     8,   365,   370,   163,   355,   211,   500,   502,   309,
     201,   200,   163,   331,   181,   181,   181,   520,   303,   181,
     324,   326,   328,   522,   508,   511,   181,   504,   200,   239,
     201,   333,   221,   171,   355,   163,   173,   201,   333,   163,
     200,    20,   133,   384,   355,   355,   355,   201,   201,   181,
     282,   260,   201,   491,   163,   163,   200,   163,   163,   173,
     201,   239,   355,    14,   355,   174,   174,   176,   156,   294,
     355,   301,   200,   200,   200,   200,   200,   200,   205,   320,
     393,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   525,   530,   356,   356,   356,   356,   356,   356,
     356,   282,   442,   201,   473,   201,   201,   201,   361,   355,
     355,   201,   491,   201,   355,   201,   174,   206,   201,    43,
     384,    37,   291,   206,   174,    57,    63,   368,    67,   369,
     211,   211,   200,   200,   355,   181,   514,   516,   518,   200,
     201,   200,   356,   356,   356,   200,    70,   497,   200,   506,
     200,   201,   355,   294,   201,   219,   201,   163,   201,    43,
     319,   333,   355,   355,   201,   348,   181,    20,   199,   163,
     336,   334,   294,   473,   355,   300,   355,   355,   260,   355,
     355,   319,   392,   239,   181,   181,   473,   201,   201,   199,
     201,   355,   163,   355,   292,   473,    47,   369,    46,   106,
     366,   497,   497,   201,   200,   200,   200,   200,   302,   303,
     333,   497,   200,   497,   201,   167,   204,   163,   201,   201,
     133,   384,   345,   350,   333,   201,   201,   206,   201,   201,
      20,   201,   201,   201,   206,   211,   393,   334,   358,   360,
     181,   201,   205,   211,    33,   367,   366,   368,   200,   491,
     494,   495,   496,   496,   355,   497,   497,   491,   492,   201,
     201,   528,   496,   497,   492,   355,   204,   355,   355,   345,
     201,   291,    12,   244,   239,   333,   239,   239,   176,   389,
     362,   301,   373,   367,   385,   386,   387,   491,   173,   528,
     201,   201,   201,   496,   496,   201,   201,   201,   492,   201,
     204,   527,   355,   204,   245,   311,   312,   313,   314,   356,
     211,   258,   201,   294,   294,   442,   388,   293,   288,   374,
     201,   200,   201,   201,   201,    53,   199,   527,   355,   205,
     248,   251,   239,   388,   355,   206,   211,   288,   491,   355,
     199,   527,   249,    12,    23,    24,   237,   240,   245,   294,
     355,   211,   239,   201,   206,   301,   239,   200,   211,   211,
     294,   250,   241,   355,   206,   205,   252,   255,   201,   291,
     253,   245,   239,   301,   211,   242,   254,   252,   206,   240,
     291
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   208,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   210,   210,   211,
     211,   212,   213,   213,   213,   214,   214,   215,   215,   216,
     217,   217,   217,   217,   218,   218,   219,   219,   220,   221,
     220,   222,   222,   222,   223,   224,   224,   226,   225,   227,
     228,   229,   229,   229,   229,   229,   229,   229,   230,   230,
     231,   231,   232,   233,   233,   234,   234,   235,   236,   236,
     237,   237,   238,   238,   239,   239,   240,   241,   240,   242,
     240,   243,   243,   244,   244,   245,   245,   245,   245,   245,
     246,   246,   247,   247,   249,   250,   248,   251,   248,   253,
     254,   252,   255,   252,   257,   258,   256,   259,   260,   260,
     260,   260,   260,   260,   260,   262,   261,   263,   265,   264,
     267,   266,   268,   268,   269,   269,   269,   269,   269,   269,
     270,   270,   271,   271,   271,   272,   272,   272,   272,   272,
     272,   272,   272,   272,   273,   273,   274,   274,   275,   275,
     275,   275,   276,   276,   277,   277,   277,   277,   277,   277,
     277,   278,   278,   279,   279,   280,   280,   281,   281,   281,
     282,   282,   282,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   285,   285,   286,
     287,   287,   287,   288,   290,   289,   291,   292,   293,   291,
     295,   296,   294,   297,   297,   297,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   299,   300,   298,   301,   301,
     301,   302,   302,   303,   303,   304,   304,   305,   305,   305,
     306,   306,   308,   309,   307,   307,   310,   310,   310,   310,
     310,   310,   311,   312,   313,   313,   313,   314,   314,   315,
     316,   316,   316,   317,   317,   318,   318,   319,   319,   320,
     320,   321,   321,   321,   323,   324,   322,   325,   326,   322,
     327,   328,   322,   330,   331,   329,   332,   332,   332,   333,
     333,   333,   333,   334,   334,   334,   335,   335,   335,   336,
     336,   336,   336,   336,   337,   337,   338,   338,   339,   340,
     340,   341,   341,   341,   341,   341,   341,   341,   342,   342,
     342,   342,   342,   342,   342,   342,   342,   342,   342,   342,
     342,   342,   342,   342,   342,   342,   342,   342,   342,   343,
     343,   344,   344,   345,   345,   346,   347,   348,   346,   349,
     350,   346,   351,   351,   351,   351,   351,   351,   351,   352,
     353,   351,   354,   354,   354,   354,   354,   354,   354,   355,
     355,   355,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   357,   358,
     356,   356,   356,   356,   359,   360,   356,   356,   356,   361,
     362,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   363,   363,
     363,   364,   364,   364,   364,   364,   364,   364,   364,   364,
     364,   364,   364,   364,   364,   364,   364,   365,   365,   366,
     366,   366,   367,   367,   368,   368,   368,   369,   369,   370,
     371,   371,   371,   372,   371,   373,   371,   374,   371,   375,
     376,   376,   377,   377,   377,   377,   377,   378,   378,   379,
     379,   380,   380,   380,   381,   382,   382,   383,   383,   383,
     384,   384,   385,   385,   385,   386,   386,   387,   387,   388,
     388,   388,   389,   389,   390,   390,   390,   390,   390,   391,
     391,   392,   392,   392,   393,   393,   393,   394,   394,   394,
     395,   395,   396,   396,   396,   397,   397,   398,   397,   399,
     400,   399,   401,   401,   402,   402,   403,   403,   403,   404,
     404,   404,   406,   405,   407,   408,   408,   408,   409,   410,
     410,   411,   411,   412,   412,   413,   413,   415,   416,   417,
     414,   418,   418,   419,   419,   420,   421,   421,   421,   421,
     422,   422,   422,   423,   423,   425,   426,   427,   424,   428,
     428,   428,   428,   428,   429,   429,   429,   429,   429,   429,
     429,   429,   429,   429,   429,   429,   429,   429,   429,   429,
     429,   429,   429,   429,   429,   429,   429,   429,   429,   429,
     429,   430,   430,   430,   430,   430,   430,   430,   430,   431,
     432,   432,   432,   433,   433,   433,   434,   434,   434,   434,
     434,   435,   435,   435,   435,   435,   436,   437,   438,   436,
     439,   439,   440,   440,   441,   441,   441,   441,   442,   442,
     443,   443,   444,   444,   444,   444,   445,   446,   444,   444,
     444,   444,   447,   444,   448,   444,   444,   444,   444,   444,
     444,   444,   444,   444,   444,   444,   444,   444,   449,   450,
     444,   444,   451,   452,   444,   453,   454,   444,   455,   456,
     444,   444,   457,   458,   444,   459,   460,   444,   444,   461,
     462,   444,   463,   464,   444,   444,   465,   466,   444,   467,
     468,   444,   469,   470,   444,   471,   472,   444,   473,   473,
     473,   475,   476,   477,   478,   474,   480,   481,   482,   483,
     479,   485,   486,   487,   488,   484,   489,   489,   489,   489,
     489,   489,   489,   490,   490,   490,   490,   490,   491,   491,
     491,   491,   491,   491,   491,   491,   492,   492,   493,   494,
     494,   495,   495,   496,   496,   497,   497,   499,   500,   498,
     501,   502,   498,   503,   504,   498,   505,   506,   498,   507,
     508,   498,   509,   510,   511,   509,   512,   513,   514,   512,
     515,   516,   512,   517,   518,   512,   512,   519,   520,   512,
     512,   521,   522,   512,   523,   523,   524,   525,   526,   526,
     526,   527,   527,   528,   528,   529,   529,   530
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     3,
       3,     2,     2,     2,     2,     2,     2,     1,     1,     1,
       1,     2,     0,     1,     1,     1,     1,     0,     2,     5,
       1,     1,     2,     2,     3,     2,     0,     2,     0,     0,
       3,     0,     2,     5,     3,     1,     2,     0,     4,     2,
       2,     1,     2,     3,     3,     3,     3,     3,     2,     4,
       0,     1,     2,     1,     3,     1,     3,     3,     3,     2,
       1,     1,     1,     2,     0,     1,     0,     0,     4,     0,
       8,     1,     1,     0,     2,     1,     1,     1,     1,     1,
       1,     2,     0,     1,     0,     0,     6,     0,     3,     0,
       0,     6,     0,     3,     0,     0,     9,     7,     1,     4,
       3,     3,     3,     5,     5,     0,     9,     3,     0,     7,
       0,     7,     4,     4,     1,     1,     1,     1,     1,     1,
       1,     3,     1,     1,     1,     3,     3,     5,     3,     3,
       3,     3,     1,     5,     1,     3,     3,     4,     1,     1,
       1,     1,     1,     4,     1,     2,     3,     3,     3,     3,
       2,     1,     3,     0,     3,     0,     4,     0,     2,     3,
       0,     2,     2,     1,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     3,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     3,     2,     2,     3,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     3,     2,
       2,     2,     2,     2,     3,     3,     3,     3,     3,     4,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     1,     4,
       0,     1,     1,     3,     0,     5,     0,     0,     0,     6,
       0,     0,     6,     2,     2,     2,     1,     2,     2,     1,
       1,     1,     1,     2,     1,     2,     2,     2,     2,     1,
       1,     1,     2,     2,     2,     0,     0,     6,     0,     2,
       2,     0,     2,     0,     2,     1,     3,     1,     3,     2,
       2,     3,     0,     0,     5,     1,     2,     5,     5,     5,
       6,     2,     1,     1,     1,     2,     3,     2,     3,     4,
       1,     1,     0,     1,     1,     1,     0,     1,     3,     8,
       7,     3,     3,     5,     0,     0,     7,     0,     0,     7,
       0,     0,     7,     0,     0,     6,     5,     8,    10,     1,
       2,     3,     4,     1,     2,     3,     1,     1,     2,     2,
       2,     2,     2,     4,     1,     3,     0,     4,     7,     7,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     6,
       8,     5,     6,     1,     4,     3,     0,     0,     8,     0,
       0,     9,     3,     4,     5,     6,     8,     5,     6,     0,
       0,     5,     3,     4,     4,     5,     4,     3,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     2,     4,     3,     4,     5,     4,     5,     3,     4,
       1,     1,     2,     4,     4,     1,     3,     5,     0,     0,
       8,     3,     3,     3,     0,     0,     8,     3,     4,     0,
       0,     9,     4,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     3,     1,     4,     3,     3,     3,     7,     8,
       7,     4,     4,     4,     4,     4,     1,     6,     7,     6,
       6,     7,     7,     6,     7,     6,     6,     0,     1,     0,
       1,     1,     0,     1,     0,     1,     1,     0,     1,     5,
       0,     2,     6,     0,     4,     0,     9,     0,    11,     3,
       3,     4,     1,     1,     3,     3,     3,     1,     3,     1,
       3,     0,     1,     3,     3,     1,     3,     0,     1,     3,
       1,     1,     1,     2,     3,     3,     5,     1,     1,     1,
       1,     1,     0,     1,     1,     4,     3,     3,     5,     1,
       3,     0,     2,     2,     4,     6,     5,     4,     6,     5,
       0,     1,     0,     1,     1,     0,     2,     0,     4,     6,
       0,     6,     1,     3,     1,     2,     0,     1,     3,     0,
       1,     1,     0,     5,     3,     0,     1,     1,     1,     0,
       2,     0,     1,     1,     2,     0,     1,     0,     0,     0,
      13,     0,     2,     0,     1,     3,     1,     1,     2,     2,
       0,     1,     1,     1,     3,     0,     0,     0,     9,     1,
       4,     3,     3,     5,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     4,     4,     1,     3,     3,     0,     1,     3,     3,
       5,     0,     2,     2,     2,     2,     4,     0,     0,     7,
       1,     1,     1,     3,     3,     2,     4,     3,     1,     2,
       0,     4,     1,     1,     1,     1,     0,     0,     6,     4,
       4,     3,     0,     6,     0,     7,     4,     2,     2,     3,
       2,     3,     2,     2,     3,     3,     3,     2,     0,     0,
       6,     2,     0,     0,     6,     0,     0,     6,     0,     0,
       6,     1,     0,     0,     6,     0,     0,     7,     1,     0,
       0,     6,     0,     0,     7,     1,     0,     0,     6,     0,
       0,     7,     0,     0,     6,     0,     0,     6,     1,     3,
       3,     0,     0,     0,     0,    12,     0,     0,     0,     0,
      12,     0,     0,     0,     0,    13,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       5,     5,     6,     6,     8,     8,     0,     1,     2,     3,
       5,     1,     2,     1,     0,     0,     1,     0,     0,    10,
       0,     0,    10,     0,     0,    10,     0,     0,    11,     0,
       0,     7,     5,     0,     0,    10,     3,     0,     0,    11,
       0,     0,    11,     0,     0,    10,     5,     0,     0,     9,
       5,     0,     0,    10,     1,     3,     0,     5,     5,     7,
       9,     0,     3,     0,     1,    11,    12,    13
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
                das2_yyerror(scanner,"module name has to be first declaration",tokAt(scanner,(yylsp[0])), CompilationError::syntax_error);
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
                CompilationError::module_already_has_a_name);
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
        if ( err ) das2_yyerror(scanner,"invalid escape sequence",tokAt(scanner,(yylsp[-1])), CompilationError::invalid_escape_sequence);
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
                CompilationError::unsupported_read_macro);
        } else if ( macros.size()>1 ) {
            string options;
            for ( auto & x : macros ) {
                options += "\t" + x->module->name + "::" + x->name + "\n";
            }
            das2_yyerror(scanner,"too many options for the reader macro " + *(yyvsp[0].s) +  "\n" + options, tokAt(scanner,(yylsp[0])),
                CompilationError::unsupported_read_macro);
        } else if ( yychar != '~' ) {
            das2_yyerror(scanner,"expecting ~ after the reader macro", tokAt(scanner,(yylsp[0])),
                CompilationError::syntax_error);
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
                yyextra->g_Program->options.push_back(opt);
            } else {
                das2_yyerror(scanner,"option " + opt.name + " is not allowed here",
                    tokAt(scanner,(yylsp[0])), CompilationError::invalid_option);
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

  case 58: /* require_module: require_module_name is_public_module  */
                                                         {
        ast_requireModule(scanner,(yyvsp[-1].s),nullptr,(yyvsp[0].b),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 59: /* require_module: require_module_name "as" "name" is_public_module  */
                                                                              {
        ast_requireModule(scanner,(yyvsp[-3].s),(yyvsp[-1].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 60: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 61: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 65: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 66: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 67: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 68: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 69: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 70: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 71: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 76: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 77: /* $@3: %empty  */
                                           {
    }
    break;

  case 78: /* expression_else: "else" optional_emit_semis $@3 expression_else_block  */
                                   {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 79: /* $@4: %empty  */
                                                                        {
    }
    break;

  case 80: /* expression_else: elif_or_static_elif '(' expr ')' optional_emit_semis $@4 expression_else_block expression_else  */
                                                         {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-7])),(yyvsp[-5].pExpression),(yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-7].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 81: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 82: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 83: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 84: /* expression_else_one_liner: "else" expression_if_one_liner  */
                                                      {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 85: /* expression_if_one_liner: expr_no_bracket  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 86: /* expression_if_one_liner: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 87: /* expression_if_one_liner: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 88: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 89: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 94: /* $@5: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 95: /* $@6: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 96: /* expression_if_block: '{' $@5 expressions $@6 '}' expression_block_finally  */
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

  case 97: /* $@7: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 98: /* expression_if_block: $@7 expression_if_one_liner SEMICOLON  */
                                               {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 99: /* $@8: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 100: /* $@9: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 101: /* expression_else_block: '{' $@8 expressions $@9 '}' expression_block_finally  */
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

  case 102: /* $@10: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 103: /* expression_else_block: $@10 expression_if_one_liner SEMICOLON  */
                                               {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 104: /* $@11: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 105: /* $@12: %empty  */
                                                                  {
    }
    break;

  case 106: /* expression_if_then_else: $@11 if_or_static_if '(' expr ')' optional_emit_semis $@12 expression_if_block expression_else  */
                                                       {
        yyextra->das_keyword = false;
        auto blk = (yyvsp[-1].pExpression)->rtti_isBlock() ? static_cast<ExprBlock *>((yyvsp[-1].pExpression)) : ast_wrapInBlock((yyvsp[-1].pExpression));
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-7])),(yyvsp[-5].pExpression),blk,(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-7].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 107: /* expression_if_then_else_oneliner: expression_if_one_liner "if" '(' expr ')' expression_else_one_liner SEMICOLON  */
                                                                                                                      {
        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ast_wrapInBlock((yyvsp[-6].pExpression)),(yyvsp[-1].pExpression) ? ast_wrapInBlock((yyvsp[-1].pExpression)) : nullptr);
    }
    break;

  case 108: /* for_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 109: /* for_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 110: /* for_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 111: /* for_variable_name_with_pos_list: '(' tuple_expansion ')'  */
                                       {
        auto pSL = new vector<VariableNameAndPosition>();
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        pSL->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 112: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 113: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 114: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' '(' tuple_expansion ')'  */
                                                                                 {
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
    }
    break;

  case 115: /* $@13: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 116: /* expression_for_loop: $@13 "for" '(' for_variable_name_with_pos_list "in" expr_list ')' optional_emit_semis expression_block  */
                                                                                                                                     {
        yyextra->das_keyword = false;
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-5].pNameWithPosList),(yyvsp[-3].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 117: /* expression_unsafe: "unsafe" optional_emit_semis expression_block  */
                                                                    {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-2])));
        pUnsafe->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 118: /* $@14: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 119: /* expression_while_loop: $@14 "while" '(' expr ')' optional_emit_semis expression_block  */
                                                                                         {
        yyextra->das_keyword = false;
        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-5])));
        pWhile->cond = (yyvsp[-3].pExpression);
        pWhile->body = (yyvsp[0].pExpression);
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 120: /* $@15: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 121: /* expression_with: $@15 "with" '(' expr ')' optional_emit_semis expression_block  */
                                                                                   {
        yyextra->das_keyword = false;
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-5])));
        pWith->with = (yyvsp[-3].pExpression);
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 122: /* expression_with_alias: "assume" "name" '=' expr  */
                                                      {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), ExpressionPtr((yyvsp[0].pExpression)));
        delete (yyvsp[-2].s);
    }
    break;

  case 123: /* expression_with_alias: "typedef" "name" '=' type_declaration  */
                                                                {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), TypeDeclPtr((yyvsp[0].pTypeDecl)));
        delete (yyvsp[-2].s);
    }
    break;

  case 124: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 125: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 126: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 127: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 128: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 129: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 130: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 131: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 132: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 133: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 134: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 135: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 136: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 137: /* annotation_argument: annotation_argument_name '=' '@' '@' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[0].s); delete (yyvsp[-4].s); }
    break;

  case 138: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 139: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 140: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 141: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 142: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 143: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                          {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[-4].s); }
    }
    break;

  case 144: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 145: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 146: /* metadata_argument_list: '@' annotation_argument optional_emit_semis  */
                                                         {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[-1].aa));
    }
    break;

  case 147: /* metadata_argument_list: metadata_argument_list '@' annotation_argument optional_emit_semis  */
                                                                                      {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-3].aaList),(yyvsp[-1].aa));
    }
    break;

  case 148: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 149: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 150: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 151: /* annotation_declaration_name: "template"  */
                                    { (yyval.s) = new string("template"); }
    break;

  case 152: /* annotation_declaration_basic: annotation_declaration_name  */
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
            (yyval.fa)->annotation = new Annotation(*(yyvsp[0].s));
            das2_yyerror(scanner,"annotation " + *(yyvsp[0].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[0])), CompilationError::invalid_annotation);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 153: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
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
            (yyval.fa)->annotation = new Annotation(*(yyvsp[-3].s));
            das2_yyerror(scanner,"annotation " + *(yyvsp[-3].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[-3])), CompilationError::invalid_annotation);
        }
        swap ( (yyval.fa)->arguments, *(yyvsp[-1].aaList) );
        delete (yyvsp[-1].aaList);
        delete (yyvsp[-3].s);
    }
    break;

  case 154: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 155: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[0].fa); (yyvsp[0].fa) = nullptr;
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 156: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::And,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 157: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
                                                                            {
        if ( !(yyvsp[-2].fa)->annotation || !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        if ( !(yyvsp[0].fa)->annotation || !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Or,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 158: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Xor,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 159: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 160: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 161: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 162: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 163: /* optional_annotation_list: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 164: /* optional_annotation_list: '[' annotation_list ']'  */
                                       { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 165: /* optional_annotation_list_with_emit_semis: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 166: /* optional_annotation_list_with_emit_semis: '[' annotation_list ']' optional_emit_semis  */
                                                          { (yyval.faList) = (yyvsp[-2].faList); }
    break;

  case 167: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 168: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 169: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 170: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 171: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 172: /* optional_function_type: "->" type_declaration  */
                                           {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 173: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 174: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 175: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 176: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 177: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 178: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 179: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 180: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 181: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 182: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 183: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 184: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 185: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 186: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 187: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 188: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 189: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 190: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 191: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 192: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 193: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 194: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 195: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 196: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 197: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 198: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 199: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 200: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 201: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 202: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 203: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 204: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 205: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 206: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 207: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 208: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 209: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 210: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 211: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 212: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 213: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 214: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 215: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 216: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 217: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 218: /* function_name: "operator" '[' ']' '='  */
                                 { (yyval.s) = new string("[]="); }
    break;

  case 219: /* function_name: "operator" '[' ']' "<-"  */
                                    { (yyval.s) = new string("[]<-"); }
    break;

  case 220: /* function_name: "operator" '[' ']' ":="  */
                                      { (yyval.s) = new string("[]:="); }
    break;

  case 221: /* function_name: "operator" '[' ']' "+="  */
                                     { (yyval.s) = new string("[]+="); }
    break;

  case 222: /* function_name: "operator" '[' ']' "-="  */
                                     { (yyval.s) = new string("[]-="); }
    break;

  case 223: /* function_name: "operator" '[' ']' "*="  */
                                     { (yyval.s) = new string("[]*="); }
    break;

  case 224: /* function_name: "operator" '[' ']' "/="  */
                                     { (yyval.s) = new string("[]/="); }
    break;

  case 225: /* function_name: "operator" '[' ']' "%="  */
                                     { (yyval.s) = new string("[]%="); }
    break;

  case 226: /* function_name: "operator" '[' ']' "&="  */
                                     { (yyval.s) = new string("[]&="); }
    break;

  case 227: /* function_name: "operator" '[' ']' "|="  */
                                     { (yyval.s) = new string("[]|="); }
    break;

  case 228: /* function_name: "operator" '[' ']' "^="  */
                                     { (yyval.s) = new string("[]^="); }
    break;

  case 229: /* function_name: "operator" '[' ']' "&&="  */
                                        { (yyval.s) = new string("[]&&="); }
    break;

  case 230: /* function_name: "operator" '[' ']' "||="  */
                                        { (yyval.s) = new string("[]||="); }
    break;

  case 231: /* function_name: "operator" '[' ']' "^^="  */
                                        { (yyval.s) = new string("[]^^="); }
    break;

  case 232: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 233: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 234: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 235: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 236: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 237: /* function_name: "operator" '.' "name" "+="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`+="); delete (yyvsp[-1].s); }
    break;

  case 238: /* function_name: "operator" '.' "name" "-="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`-="); delete (yyvsp[-1].s); }
    break;

  case 239: /* function_name: "operator" '.' "name" "*="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`*="); delete (yyvsp[-1].s); }
    break;

  case 240: /* function_name: "operator" '.' "name" "/="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`/="); delete (yyvsp[-1].s); }
    break;

  case 241: /* function_name: "operator" '.' "name" "%="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`%="); delete (yyvsp[-1].s); }
    break;

  case 242: /* function_name: "operator" '.' "name" "&="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&="); delete (yyvsp[-1].s); }
    break;

  case 243: /* function_name: "operator" '.' "name" "|="  */
                                          { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`|="); delete (yyvsp[-1].s); }
    break;

  case 244: /* function_name: "operator" '.' "name" "^="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^="); delete (yyvsp[-1].s); }
    break;

  case 245: /* function_name: "operator" '.' "name" "&&="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&&="); delete (yyvsp[-1].s); }
    break;

  case 246: /* function_name: "operator" '.' "name" "||="  */
                                            { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`||="); delete (yyvsp[-1].s); }
    break;

  case 247: /* function_name: "operator" '.' "name" "^^="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^^="); delete (yyvsp[-1].s); }
    break;

  case 248: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 249: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 250: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 251: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 252: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 253: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 254: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 255: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 256: /* function_name: "operator" "is" das_type_name  */
                                                { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 257: /* function_name: "operator" "as" das_type_name  */
                                                { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 258: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 259: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 260: /* function_name: "operator" '?' "as" das_type_name  */
                                                    { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 261: /* function_name: das_type_name  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 262: /* das_type_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 263: /* das_type_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 264: /* das_type_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 265: /* das_type_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 266: /* das_type_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 267: /* das_type_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 268: /* das_type_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 269: /* das_type_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 270: /* das_type_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 271: /* das_type_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 272: /* das_type_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 273: /* das_type_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 274: /* das_type_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 275: /* das_type_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 276: /* das_type_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 277: /* das_type_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 278: /* das_type_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 279: /* das_type_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 280: /* das_type_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 281: /* das_type_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 282: /* das_type_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 283: /* das_type_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 284: /* das_type_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 285: /* das_type_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 286: /* das_type_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 287: /* optional_template: %empty  */
                                        { (yyval.b) = false; }
    break;

  case 288: /* optional_template: "template"  */
                                        { (yyval.b) = true; }
    break;

  case 289: /* global_function_declaration: optional_annotation_list_with_emit_semis "def" optional_template function_declaration  */
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
                        CompilationError::function_already_declared);
            }
        }
        (yyvsp[0].pFuncDecl)->delRef();
    }
    break;

  case 290: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 291: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 292: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 293: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 294: /* $@16: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 295: /* function_declaration: optional_public_or_private_function $@16 function_declaration_header optional_emit_semis block_or_simple_block  */
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

  case 296: /* expression_block_finally: %empty  */
        {
        (yyval.pExpression) = nullptr;
    }
    break;

  case 297: /* $@17: %empty  */
                  {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 298: /* $@18: %empty  */
                             {
        yyextra->pop_nesteds();
    }
    break;

  case 299: /* expression_block_finally: "finally" $@17 '{' expressions $@18 '}'  */
          {
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 300: /* $@19: %empty  */
        {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 301: /* $@20: %empty  */
                                      {
        yyextra->pop_nesteds();
    }
    break;

  case 302: /* expression_block: $@19 '{' expressions $@20 '}' expression_block_finally  */
                                        {
        (yyval.pExpression) = (yyvsp[-3].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-4]),(yylsp[0]));
        if ( (yyvsp[0].pExpression) ) {
            auto pF = (ExprBlock *) (yyvsp[0].pExpression);
            auto pB = (ExprBlock *) (yyval.pExpression);
            swap ( pB->finalList, pF->list );
            // gc_node — don't delete Expression
        }
    }
    break;

  case 303: /* expr_call_pipe_no_bracket: expr_call expr_full_block_assumed_piped  */
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

  case 304: /* expr_call_pipe_no_bracket: expr_method_call_no_bracket expr_full_block_assumed_piped  */
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

  case 305: /* expr_call_pipe_no_bracket: expr_field_no_bracket expr_full_block_assumed_piped  */
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

  case 306: /* expression_any: SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 307: /* expression_any: expr_assign_no_bracket SEMICOLON  */
                                                    { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 308: /* expression_any: expression_delete SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 309: /* expression_any: expression_let  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 310: /* expression_any: expression_while_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 311: /* expression_any: expression_unsafe  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 312: /* expression_any: expression_with  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 313: /* expression_any: expression_with_alias SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 314: /* expression_any: expression_for_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 315: /* expression_any: expression_break SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 316: /* expression_any: expression_continue SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 317: /* expression_any: expression_return SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 318: /* expression_any: expression_yield SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 319: /* expression_any: expression_if_then_else  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 320: /* expression_any: expression_if_then_else_oneliner  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 321: /* expression_any: expression_try_catch  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 322: /* expression_any: expression_label SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 323: /* expression_any: expression_goto SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 324: /* expression_any: "pass" SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 325: /* $@21: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 326: /* $@22: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 327: /* expression_any: '{' $@21 expressions $@22 '}' expression_block_finally  */
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

  case 328: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 329: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 330: /* expressions: expressions error  */
                                 {
        (void)(yyvsp[-1].pExpression); /* gc_node — don't delete Expression */ (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 331: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 332: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 333: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 334: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 335: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 336: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 337: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 338: /* name_in_namespace: "name" "::" "name"  */
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

  case 339: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 340: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 341: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 342: /* $@23: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 343: /* $@24: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 344: /* new_type_declaration: '<' $@23 type_declaration '>' $@24  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 345: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 346: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 347: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 348: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 349: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 350: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 351: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 352: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 353: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 354: /* expression_return: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 355: /* expression_return: "return" expr  */
                                      {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 356: /* expression_return: "return" "<-" expr  */
                                             {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 357: /* expression_yield: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 358: /* expression_yield: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 359: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 360: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 361: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 362: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 363: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 364: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 365: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 366: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 367: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 368: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 369: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 370: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr SEMICOLON  */
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

  case 371: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 372: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 373: /* expression_let: kwd_let optional_in_scope '{' variable_declaration_list '}'  */
                                                                               {
        (yyval.pExpression) = ast_LetList(scanner,(yyvsp[-4].b),(yyvsp[-3].b),*(yyvsp[-1].pVarDeclList),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 374: /* $@25: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 375: /* $@26: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 376: /* expr_cast: "cast" '<' $@25 type_declaration_no_options '>' $@26 expr_no_bracket  */
                                                                                                                                                           {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
    }
    break;

  case 377: /* $@27: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 378: /* $@28: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 379: /* expr_cast: "upcast" '<' $@27 type_declaration_no_options '>' $@28 expr_no_bracket  */
                                                                                                                                                             {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 380: /* $@29: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 381: /* $@30: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 382: /* expr_cast: "reinterpret" '<' $@29 type_declaration_no_options '>' $@30 expr_no_bracket  */
                                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 383: /* $@31: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 384: /* $@32: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 385: /* expr_type_decl: "type" '<' $@31 type_declaration '>' $@32  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 386: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
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

  case 387: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
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

  case 388: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" c_or_s "name" '>' '(' expr ')'  */
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

  case 389: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 390: /* expr_list: "<-" expr  */
                             {
            (yyval.pExpression) = ast_makeMoveArgument(scanner, (yyvsp[0].pExpression), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 391: /* expr_list: expr_list ',' expr  */
                                        {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 392: /* expr_list: expr_list ',' "<-" expr  */
                                                   {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-3])),(yyvsp[-3].pExpression),ast_makeMoveArgument(scanner, (yyvsp[0].pExpression), tokAt(scanner,(yylsp[0]))));
    }
    break;

  case 393: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 394: /* block_or_simple_block: "=>" expr_no_bracket  */
                                                   {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 395: /* block_or_simple_block: "=>" "<-" expr_no_bracket  */
                                                          {
            auto retE = new ExprReturn(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 396: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 397: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 398: /* block_or_lambda: '@' '@'  */
                { (yyval.i) = 2;   /* local function */ }
    break;

  case 399: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 400: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 401: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 402: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 403: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 404: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 405: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 406: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 407: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 408: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis block_or_simple_block  */
                                                                                                                {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 409: /* expr_full_block_assumed_piped: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis block_or_simple_block  */
                                                                                                                {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 410: /* expr_full_block_assumed_piped: '{' expressions '}'  */
                                   {
        (yyval.pExpression) = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,new TypeDecl(Type::autoinfer),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-1])),tokAt(scanner,(yylsp[-1])),LineInfo());
    }
    break;

  case 411: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 412: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 413: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 414: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 415: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 416: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 417: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 418: /* expr_assign_no_bracket: expr_no_bracket  */
                                                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 419: /* expr_assign_no_bracket: expr_no_bracket '=' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 420: /* expr_assign_no_bracket: expr_no_bracket "<-" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 421: /* expr_assign_no_bracket: expr_no_bracket "<-" make_table_decl  */
                                                                   { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 422: /* expr_assign_no_bracket: expr_no_bracket "<-" array_comprehension  */
                                                                     { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 423: /* expr_assign_no_bracket: expr_no_bracket ":=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 424: /* expr_assign_no_bracket: expr_no_bracket "&=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 425: /* expr_assign_no_bracket: expr_no_bracket "|=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 426: /* expr_assign_no_bracket: expr_no_bracket "^=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 427: /* expr_assign_no_bracket: expr_no_bracket "&&=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 428: /* expr_assign_no_bracket: expr_no_bracket "||=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 429: /* expr_assign_no_bracket: expr_no_bracket "^^=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 430: /* expr_assign_no_bracket: expr_no_bracket "+=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 431: /* expr_assign_no_bracket: expr_no_bracket "-=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 432: /* expr_assign_no_bracket: expr_no_bracket "*=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 433: /* expr_assign_no_bracket: expr_no_bracket "/=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 434: /* expr_assign_no_bracket: expr_no_bracket "%=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 435: /* expr_assign_no_bracket: expr_no_bracket "<<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 436: /* expr_assign_no_bracket: expr_no_bracket ">>=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 437: /* expr_assign_no_bracket: expr_no_bracket "<<<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 438: /* expr_assign_no_bracket: expr_no_bracket ">>>=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 439: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 440: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 441: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' ')'  */
                                                                    {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 442: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' expr_list ')'  */
                                                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 443: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 444: /* func_addr_name: "$i" '(' expr ')'  */
                                          {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 445: /* func_addr_expr: '@' '@' func_addr_name  */
                                          {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 446: /* $@33: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 447: /* $@34: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 448: /* func_addr_expr: '@' '@' '<' $@33 type_declaration_no_options '>' $@34 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 449: /* $@35: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 450: /* $@36: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 451: /* func_addr_expr: '@' '@' '<' $@35 optional_function_argument_list optional_function_type '>' $@36 func_addr_name  */
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

  case 452: /* expr_field_no_bracket: expr_no_bracket '.' "name"  */
                                                         {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 453: /* expr_field_no_bracket: expr_no_bracket '.' '.' "name"  */
                                                             {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 454: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' ')'  */
                                                                 {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 455: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' expr_list ')'  */
                                                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 456: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->methodCall = true;
        nc->arguments = (yyvsp[-2].pMakeStruct);
        nc->nonNamedArguments.push_back((yyvsp[-7].pExpression));
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 457: /* expr_field_no_bracket: expr_no_bracket '.' basic_type_declaration '(' ')'  */
                                                                                   {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 458: /* expr_field_no_bracket: expr_no_bracket '.' basic_type_declaration '(' expr_list ')'  */
                                                                                                        {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 459: /* $@37: %empty  */
                                          { yyextra->das_suppress_errors=true; }
    break;

  case 460: /* $@38: %empty  */
                                                                                       { yyextra->das_suppress_errors=false; }
    break;

  case 461: /* expr_field_no_bracket: expr_no_bracket '.' $@37 error $@38  */
                                                                                                                               {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), "");
        yyerrok;
    }
    break;

  case 462: /* expr_call: name_in_namespace '(' ')'  */
                                               {
            (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),*(yyvsp[-2].s));
            delete (yyvsp[-2].s);
    }
    break;

  case 463: /* expr_call: name_in_namespace '(' "uninitialized" ')'  */
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

  case 464: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
                                                               {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 465: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
                                                                                 {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-4])),*(yyvsp[-4].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-4].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 466: /* expr_call: name_in_namespace '(' expr_list ')'  */
                                                                    {
            (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),*(yyvsp[-3].s)),(yyvsp[-1].pExpression));
            delete (yyvsp[-3].s);
    }
    break;

  case 467: /* expr_call: basic_type_declaration '(' ')'  */
                                                    {
        (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-2].type)));
    }
    break;

  case 468: /* expr_call: basic_type_declaration '(' expr_list ')'  */
                                                                         {
        (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-3].type))),(yyvsp[-1].pExpression));
    }
    break;

  case 469: /* expr: expr_no_bracket  */
                                       { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 470: /* expr: make_table_decl  */
                                     { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 471: /* expr: array_comprehension  */
                                     { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 472: /* expr_no_bracket: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 473: /* expr_no_bracket: name_in_namespace  */
                                              { (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 474: /* expr_no_bracket: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 475: /* expr_no_bracket: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 476: /* expr_no_bracket: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 477: /* expr_no_bracket: make_decl_no_bracket  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 478: /* expr_no_bracket: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 479: /* expr_no_bracket: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 480: /* expr_no_bracket: expr_field_no_bracket  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 481: /* expr_no_bracket: expr_mtag_no_bracket  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 482: /* expr_no_bracket: '!' expr_no_bracket  */
                                                         { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",(yyvsp[0].pExpression)); }
    break;

  case 483: /* expr_no_bracket: '~' expr_no_bracket  */
                                                         { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",(yyvsp[0].pExpression)); }
    break;

  case 484: /* expr_no_bracket: '+' expr_no_bracket  */
                                                             { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",(yyvsp[0].pExpression)); }
    break;

  case 485: /* expr_no_bracket: '-' expr_no_bracket  */
                                                             { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",(yyvsp[0].pExpression)); }
    break;

  case 486: /* expr_no_bracket: expr_no_bracket "<<" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 487: /* expr_no_bracket: expr_no_bracket ">>" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 488: /* expr_no_bracket: expr_no_bracket "<<<" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 489: /* expr_no_bracket: expr_no_bracket ">>>" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 490: /* expr_no_bracket: expr_no_bracket '+' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 491: /* expr_no_bracket: expr_no_bracket '-' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 492: /* expr_no_bracket: expr_no_bracket '*' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 493: /* expr_no_bracket: expr_no_bracket '/' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 494: /* expr_no_bracket: expr_no_bracket '%' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 495: /* expr_no_bracket: expr_no_bracket '<' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 496: /* expr_no_bracket: expr_no_bracket '>' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 497: /* expr_no_bracket: expr_no_bracket "==" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 498: /* expr_no_bracket: expr_no_bracket "!=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 499: /* expr_no_bracket: expr_no_bracket "<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 500: /* expr_no_bracket: expr_no_bracket ">=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 501: /* expr_no_bracket: expr_no_bracket '&' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 502: /* expr_no_bracket: expr_no_bracket '|' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 503: /* expr_no_bracket: expr_no_bracket '^' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 504: /* expr_no_bracket: expr_no_bracket "&&" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 505: /* expr_no_bracket: expr_no_bracket "||" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 506: /* expr_no_bracket: expr_no_bracket "^^" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 507: /* expr_no_bracket: expr_no_bracket ".." expr_no_bracket  */
                                                                   {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 508: /* expr_no_bracket: "++" expr_no_bracket  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", (yyvsp[0].pExpression)); }
    break;

  case 509: /* expr_no_bracket: "--" expr_no_bracket  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", (yyvsp[0].pExpression)); }
    break;

  case 510: /* expr_no_bracket: expr_no_bracket "++"  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 511: /* expr_no_bracket: expr_no_bracket "--"  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 512: /* expr_no_bracket: '(' expr_list optional_comma ')'  */
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

  case 513: /* expr_no_bracket: '(' make_struct_single ')'  */
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

  case 514: /* expr_no_bracket: expr_no_bracket '[' expr ']'  */
                                                            { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 515: /* expr_no_bracket: expr_no_bracket '.' '[' expr ']'  */
                                                                { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 516: /* expr_no_bracket: expr_no_bracket "?[" expr ']'  */
                                                            { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 517: /* expr_no_bracket: expr_no_bracket '.' "?[" expr ']'  */
                                                                { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 518: /* expr_no_bracket: expr_no_bracket "?." "name"  */
                                                            { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 519: /* expr_no_bracket: expr_no_bracket '.' "?." "name"  */
                                                                { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 520: /* expr_no_bracket: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 521: /* expr_no_bracket: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 522: /* expr_no_bracket: '*' expr_no_bracket  */
                                                              { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 523: /* expr_no_bracket: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 524: /* expr_no_bracket: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 525: /* expr_no_bracket: expr_generator  */
                                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 526: /* expr_no_bracket: expr_no_bracket "??" expr_no_bracket  */
                                                                         { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 527: /* expr_no_bracket: expr_no_bracket '?' expr_no_bracket ':' expr_no_bracket  */
                                                                                           {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        }
    break;

  case 528: /* $@39: %empty  */
                                                          { yyextra->das_arrow_depth ++; }
    break;

  case 529: /* $@40: %empty  */
                                                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 530: /* expr_no_bracket: expr_no_bracket "is" "type" '<' $@39 type_declaration_no_options '>' $@40  */
                                                                                                                                                                  {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 531: /* expr_no_bracket: expr_no_bracket "is" basic_type_declaration  */
                                                                          {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 532: /* expr_no_bracket: expr_no_bracket "is" "name"  */
                                                         {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 533: /* expr_no_bracket: expr_no_bracket "as" "name"  */
                                                         {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 534: /* $@41: %empty  */
                                                          { yyextra->das_arrow_depth ++; }
    break;

  case 535: /* $@42: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 536: /* expr_no_bracket: expr_no_bracket "as" "type" '<' $@41 type_declaration '>' $@42  */
                                                                                                                                                       {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 537: /* expr_no_bracket: expr_no_bracket "as" basic_type_declaration  */
                                                                          {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 538: /* expr_no_bracket: expr_no_bracket '?' "as" "name"  */
                                                             {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 539: /* $@43: %empty  */
                                                              { yyextra->das_arrow_depth ++; }
    break;

  case 540: /* $@44: %empty  */
                                                                                                                          { yyextra->das_arrow_depth --; }
    break;

  case 541: /* expr_no_bracket: expr_no_bracket '?' "as" "type" '<' $@43 type_declaration '>' $@44  */
                                                                                                                                                           {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 542: /* expr_no_bracket: expr_no_bracket '?' "as" basic_type_declaration  */
                                                                              {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 543: /* expr_no_bracket: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 544: /* expr_no_bracket: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 545: /* expr_no_bracket: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 546: /* expr_no_bracket: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 547: /* expr_no_bracket: expr_method_call_no_bracket  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 548: /* expr_no_bracket: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 549: /* expr_no_bracket: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 550: /* expr_no_bracket: expr_no_bracket "<|" expr_no_bracket  */
                                                                      { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 551: /* expr_no_bracket: expr_no_bracket "|>" expr_no_bracket  */
                                                                      { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 552: /* expr_no_bracket: expr_no_bracket "|>" basic_type_declaration  */
                                                                     {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 553: /* expr_no_bracket: expr_call_pipe_no_bracket  */
                                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 554: /* expr_no_bracket: "unsafe" '(' expr ')'  */
                                         {
            (yyvsp[-1].pExpression)->alwaysSafe = true;
            (yyvsp[-1].pExpression)->userSaidItsSafe = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    break;

  case 555: /* expr_no_bracket: expr_no_bracket "=>" expr_no_bracket  */
                                                               {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 556: /* expr_no_bracket: expr_no_bracket "=>" make_table_decl  */
                                                               {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 557: /* expr_no_bracket: expr_no_bracket "=>" array_comprehension  */
                                                                   {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 558: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 559: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 560: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list optional_emit_semis expression_block  */
                                                                                                                                                  {
        auto closure = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),(yyvsp[0].pExpression));
        ((ExprBlock *)(yyvsp[0].pExpression))->returnType = new TypeDecl(Type::autoinfer);
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),closure,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 561: /* expr_mtag_no_bracket: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 562: /* expr_mtag_no_bracket: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 563: /* expr_mtag_no_bracket: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 564: /* expr_mtag_no_bracket: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 565: /* expr_mtag_no_bracket: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 566: /* expr_mtag_no_bracket: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 567: /* expr_mtag_no_bracket: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 568: /* expr_mtag_no_bracket: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 569: /* expr_mtag_no_bracket: expr_no_bracket '.' "$f" '(' expr ')'  */
                                                                           {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 570: /* expr_mtag_no_bracket: expr_no_bracket "?." "$f" '(' expr ')'  */
                                                                            {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 571: /* expr_mtag_no_bracket: expr_no_bracket '.' '.' "$f" '(' expr ')'  */
                                                                               {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 572: /* expr_mtag_no_bracket: expr_no_bracket '.' "?." "$f" '(' expr ')'  */
                                                                                {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 573: /* expr_mtag_no_bracket: expr_no_bracket "as" "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 574: /* expr_mtag_no_bracket: expr_no_bracket '?' "as" "$f" '(' expr ')'  */
                                                                                  {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 575: /* expr_mtag_no_bracket: expr_no_bracket "is" "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 576: /* expr_mtag_no_bracket: '@' '@' "$c" '(' expr ')'  */
                                                         {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 577: /* optional_field_annotation: %empty  */
                                      { (yyval.aaList) = nullptr; }
    break;

  case 578: /* optional_field_annotation: metadata_argument_list  */
                                      { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 579: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 580: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 581: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 582: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 583: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 584: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 585: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 586: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 587: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 588: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 589: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 590: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 591: /* struct_variable_declaration_list: struct_variable_declaration_list "new line, semicolon"  */
                                                                 { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 592: /* struct_variable_declaration_list: struct_variable_declaration_list "typedef" "name" '=' type_declaration SEMICOLON  */
                                                                                                                {
        (yyval.pVarDeclList) = (yyvsp[-5].pVarDeclList);
        ast_structureAlias(scanner,(yyvsp[-3].s),(yyvsp[-1].pTypeDecl),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 593: /* $@45: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 594: /* struct_variable_declaration_list: struct_variable_declaration_list $@45 structure_variable_declaration SEMICOLON  */
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

  case 595: /* $@46: %empty  */
                                                                                                                     {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 596: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable "abstract" optional_constant $@46 function_declaration_header SEMICOLON  */
                                                          {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 597: /* $@47: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 598: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@47 function_declaration_header optional_emit_semis expression_block  */
                                                                                            {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-10].pVarDeclList),(yyvsp[-9].faList),(yyvsp[-6].b),(yyvsp[-7].b),(yyvsp[-5].i),(yyvsp[-4].b),(yyvsp[-2].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-8]),(yylsp[0])),tokAt(scanner,(yylsp[-9])));
            }
    break;

  case 599: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 600: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 601: /* function_argument_declaration_type: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 602: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 603: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 604: /* function_argument_list: function_argument_declaration_no_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 605: /* function_argument_list: function_argument_declaration_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 606: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 607: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 608: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 609: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 610: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                       { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 611: /* tuple_alias_type_list: %empty  */
      {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 612: /* tuple_alias_type_list: tuple_type  */
                       {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 613: /* tuple_alias_type_list: tuple_alias_type_list semis tuple_type  */
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

  case 614: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 615: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 616: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 617: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 618: /* variant_alias_type_list: variant_type  */
                         {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 619: /* variant_alias_type_list: variant_alias_type_list semis variant_type  */
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

  case 620: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 621: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 622: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 623: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 624: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 625: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 626: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 627: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 628: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 629: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 630: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 631: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 632: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 633: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 634: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 635: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 636: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 637: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 638: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 639: /* global_let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 640: /* global_let_variable_name_with_pos_list: global_let_variable_name_with_pos_list ',' "name"  */
                                                                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 641: /* variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 642: /* variable_declaration_list: variable_declaration_list SEMICOLON  */
                                                  {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 643: /* variable_declaration_list: variable_declaration_list let_variable_declaration  */
                                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
        (yyvsp[-1].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 644: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 645: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 646: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 647: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                         {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 648: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                               {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 649: /* global_let_variable_declaration: global_let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 650: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 651: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 652: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 653: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 654: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 655: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 656: /* global_variable_declaration_list: global_variable_declaration_list SEMICOLON  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 657: /* $@48: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 658: /* global_variable_declaration_list: global_variable_declaration_list $@48 optional_field_annotation let_variable_declaration  */
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

  case 659: /* global_let: kwd_let optional_shared optional_public_or_private_variable '{' global_variable_declaration_list '}'  */
                                                                                                                                       {
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 660: /* $@49: %empty  */
                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 661: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@49 optional_field_annotation global_let_variable_declaration  */
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

  case 662: /* enum_expression: "name"  */
                   {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        delete (yyvsp[0].s);
    }
    break;

  case 663: /* enum_expression: "name" '=' expr  */
                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[-2].s),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-2])));
        delete (yyvsp[-2].s);
    }
    break;

  case 666: /* enum_list: %empty  */
        {
        (yyval.pEnumList) = new Enumeration();
    }
    break;

  case 667: /* enum_list: enum_expression  */
                            {
        (yyval.pEnumList) = new Enumeration();
        if ( !(yyval.pEnumList)->add((yyvsp[0].pEnumPair)->name,(yyvsp[0].pEnumPair)->expr,(yyvsp[0].pEnumPair)->at) ) {
            das2_yyerror(scanner,"enumeration already declared " + (yyvsp[0].pEnumPair)->name, (yyvsp[0].pEnumPair)->at,
                CompilationError::enumeration_value_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            for ( auto & crd : yyextra->g_CommentReaders ) {
                crd->afterEnumerationEntry((yyvsp[0].pEnumPair)->name.c_str(), (yyvsp[0].pEnumPair)->at);
            }
        }
        delete (yyvsp[0].pEnumPair);
    }
    break;

  case 668: /* enum_list: enum_list commas enum_expression  */
                                                 {
        if ( !(yyvsp[-2].pEnumList)->add((yyvsp[0].pEnumPair)->name,(yyvsp[0].pEnumPair)->expr,(yyvsp[0].pEnumPair)->at) ) {
            das2_yyerror(scanner,"enumeration already declared " + (yyvsp[0].pEnumPair)->name, (yyvsp[0].pEnumPair)->at,
                CompilationError::enumeration_value_already_declared);
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

  case 669: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 670: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 671: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 672: /* $@50: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 673: /* single_alias: optional_public_or_private_alias "name" $@50 '=' type_declaration  */
                                  {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        (yyvsp[0].pTypeDecl)->isPrivateAlias = !(yyvsp[-4].b);
        if ( (yyvsp[0].pTypeDecl)->baseType == Type::alias ) {
            das2_yyerror(scanner,"alias cannot be defined in terms of another alias "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::invalid_type);
        }
        (yyvsp[0].pTypeDecl)->alias = *(yyvsp[-3].s);
        if ( !yyextra->g_Program->addAlias((yyvsp[0].pTypeDecl)) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterAlias((yyvsp[-3].s)->c_str(),pubename);
        }
        delete (yyvsp[-3].s);
    }
    break;

  case 675: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 676: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 677: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 678: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 679: /* optional_enum_basic_type_declaration: %empty  */
        {
        (yyval.type) = Type::tInt;
    }
    break;

  case 680: /* optional_enum_basic_type_declaration: ':' enum_basic_type_declaration  */
                                              {
        (yyval.type) = (yyvsp[0].type);
    }
    break;

  case 687: /* $@51: %empty  */
                                                                     {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 688: /* $@52: %empty  */
                                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 689: /* $@53: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 690: /* enum_declaration: optional_annotation_list_with_emit_semis "enum" $@51 optional_public_or_private_enum enum_name optional_enum_basic_type_declaration optional_emit_commas '{' $@52 enum_list optional_commas $@53 '}'  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-8].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-12].faList),tokAt(scanner,(yylsp[-12])),(yyvsp[-9].b),(yyvsp[-8].pEnum),(yyvsp[-3].pEnumList),(yyvsp[-7].type));
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

  case 703: /* optional_struct_variable_declaration_list: ';'  */
            {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 704: /* optional_struct_variable_declaration_list: '{' struct_variable_declaration_list '}'  */
                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 705: /* $@54: %empty  */
                                                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 706: /* $@55: %empty  */
                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 707: /* $@56: %empty  */
                                             {
        if ( (yyvsp[-1].pStructure) ) {
            (yyvsp[-1].pStructure)->isClass = (yyvsp[-4].i)==CorS_Class || (yyvsp[-4].i)==CorS_ClassTemplate;
            (yyvsp[-1].pStructure)->isTemplate = (yyvsp[-4].i)==CorS_ClassTemplate || (yyvsp[-4].i)==CorS_StructTemplate;
            (yyvsp[-1].pStructure)->privateStructure = !(yyvsp[-3].b);
        }
    }
    break;

  case 708: /* structure_declaration: optional_annotation_list_with_emit_semis $@54 class_or_struct optional_public_or_private_structure $@55 structure_name optional_emit_semis $@56 optional_struct_variable_declaration_list  */
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

  case 709: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 710: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 711: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 712: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 713: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 714: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 715: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 716: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 717: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 718: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 719: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 720: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 721: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 722: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 723: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 724: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 725: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 726: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 727: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 728: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 729: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 730: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 731: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 732: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 733: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 734: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 735: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 736: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 737: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 738: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 739: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 740: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 741: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 742: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 743: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 744: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 745: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 746: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 747: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 748: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 749: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 750: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 751: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 752: /* auto_type_declaration: "$t" '(' expr ')'  */
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

  case 753: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 754: /* bitfield_bits: bitfield_bits ';' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 755: /* bitfield_bits: bitfield_bits ',' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 756: /* bitfield_alias_bits: %empty  */
        {
        auto pSL = new vector<tuple<string,Expression *>>();
        (yyval.pNameExprList) = pSL;

    }
    break;

  case 757: /* bitfield_alias_bits: "name"  */
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

  case 758: /* bitfield_alias_bits: "name" '=' expr  */
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

  case 759: /* bitfield_alias_bits: bitfield_alias_bits commas "name"  */
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

  case 760: /* bitfield_alias_bits: bitfield_alias_bits commas "name" '=' expr  */
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

  case 761: /* bitfield_basic_type_declaration: %empty  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 762: /* bitfield_basic_type_declaration: ':' "uint8"  */
                             { (yyval.type) = Type::tBitfield8; }
    break;

  case 763: /* bitfield_basic_type_declaration: ':' "uint16"  */
                             { (yyval.type) = Type::tBitfield16; }
    break;

  case 764: /* bitfield_basic_type_declaration: ':' "uint"  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 765: /* bitfield_basic_type_declaration: ':' "uint64"  */
                             { (yyval.type) = Type::tBitfield64; }
    break;

  case 766: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' '>'  */
                                                                          {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-2].type));
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-2]));
    }
    break;

  case 767: /* $@57: %empty  */
                                                                     { yyextra->das_arrow_depth ++; }
    break;

  case 768: /* $@58: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 769: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' $@57 bitfield_bits '>' $@58  */
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

  case 772: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 773: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 774: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 775: /* dim_list: '[' ']'  */
                {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), nullptr);
    }
    break;

  case 776: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 777: /* dim_list: dim_list '[' ']'  */
                              {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), nullptr);
    }
    break;

  case 778: /* type_declaration_no_options: type_declaration_no_options_no_dim  */
                                                     {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 779: /* type_declaration_no_options: type_declaration_no_options_no_dim dim_list  */
                                                                       {
        if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeDecl ) {
            das2_yyerror(scanner,"type declaration can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_type);
        } else if ( (yyvsp[-1].pTypeDecl)->baseType==Type::typeMacro ) {
            das2_yyerror(scanner,"macro can`t be used as array base type",tokAt(scanner,(yylsp[-1])),
                CompilationError::invalid_type);
        }
        (yyvsp[-1].pTypeDecl)->dim.insert((yyvsp[-1].pTypeDecl)->dim.begin(), (yyvsp[0].pTypeDecl)->dim.begin(), (yyvsp[0].pTypeDecl)->dim.end());
        (yyvsp[-1].pTypeDecl)->dimExpr.insert((yyvsp[-1].pTypeDecl)->dimExpr.begin(), (yyvsp[0].pTypeDecl)->dimExpr.begin(), (yyvsp[0].pTypeDecl)->dimExpr.end());
        (yyvsp[-1].pTypeDecl)->removeDim = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyvsp[0].pTypeDecl)->dimExpr.clear();
    }
    break;

  case 780: /* optional_expr_list_in_braces: %empty  */
            { (yyval.pExpression) = nullptr; }
    break;

  case 781: /* optional_expr_list_in_braces: '(' expr_list optional_comma ')'  */
                                                { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 782: /* type_declaration_no_options_no_dim: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 783: /* type_declaration_no_options_no_dim: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 784: /* type_declaration_no_options_no_dim: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 785: /* type_declaration_no_options_no_dim: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 786: /* $@59: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 787: /* $@60: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 788: /* type_declaration_no_options_no_dim: "type" '<' $@59 type_declaration '>' $@60  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 789: /* type_declaration_no_options_no_dim: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 790: /* type_declaration_no_options_no_dim: name_in_namespace '(' optional_expr_list ')'  */
                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-3])), *(yyvsp[-3].s)));
        delete (yyvsp[-3].s);
    }
    break;

  case 791: /* type_declaration_no_options_no_dim: '$' name_in_namespace optional_expr_list_in_braces  */
                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-1]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-1])), *(yyvsp[-1].s)));
        delete (yyvsp[-1].s);
    }
    break;

  case 792: /* $@61: %empty  */
                                    { yyextra->das_arrow_depth ++; }
    break;

  case 793: /* type_declaration_no_options_no_dim: name_in_namespace '<' $@61 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 794: /* $@62: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 795: /* type_declaration_no_options_no_dim: '$' name_in_namespace '<' $@62 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 796: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 797: /* type_declaration_no_options_no_dim: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 798: /* type_declaration_no_options_no_dim: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 799: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 800: /* type_declaration_no_options_no_dim: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 801: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 802: /* type_declaration_no_options_no_dim: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 803: /* type_declaration_no_options_no_dim: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 804: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 805: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 806: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 807: /* type_declaration_no_options_no_dim: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 808: /* $@63: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 809: /* $@64: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 810: /* type_declaration_no_options_no_dim: "smart_ptr" '<' $@63 type_declaration '>' $@64  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 811: /* type_declaration_no_options_no_dim: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 812: /* $@65: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 813: /* $@66: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 814: /* type_declaration_no_options_no_dim: "array" '<' $@65 type_declaration '>' $@66  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 815: /* $@67: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 816: /* $@68: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 817: /* type_declaration_no_options_no_dim: "table" '<' $@67 table_type_pair '>' $@68  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 818: /* $@69: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 819: /* $@70: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 820: /* type_declaration_no_options_no_dim: "iterator" '<' $@69 type_declaration '>' $@70  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 821: /* type_declaration_no_options_no_dim: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 822: /* $@71: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 823: /* $@72: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 824: /* type_declaration_no_options_no_dim: "block" '<' $@71 type_declaration '>' $@72  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 825: /* $@73: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 826: /* $@74: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 827: /* type_declaration_no_options_no_dim: "block" '<' $@73 optional_function_argument_list optional_function_type '>' $@74  */
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

  case 828: /* type_declaration_no_options_no_dim: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 829: /* $@75: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 830: /* $@76: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 831: /* type_declaration_no_options_no_dim: "function" '<' $@75 type_declaration '>' $@76  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 832: /* $@77: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 833: /* $@78: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 834: /* type_declaration_no_options_no_dim: "function" '<' $@77 optional_function_argument_list optional_function_type '>' $@78  */
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

  case 835: /* type_declaration_no_options_no_dim: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = new TypeDecl(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 836: /* $@79: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 837: /* $@80: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 838: /* type_declaration_no_options_no_dim: "lambda" '<' $@79 type_declaration '>' $@80  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 839: /* $@81: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 840: /* $@82: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 841: /* type_declaration_no_options_no_dim: "lambda" '<' $@81 optional_function_argument_list optional_function_type '>' $@82  */
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

  case 842: /* $@83: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 843: /* $@84: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 844: /* type_declaration_no_options_no_dim: "tuple" '<' $@83 tuple_type_list '>' $@84  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 845: /* $@85: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 846: /* $@86: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 847: /* type_declaration_no_options_no_dim: "variant" '<' $@85 variant_type_list '>' $@86  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 848: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 849: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 850: /* type_declaration: type_declaration '|' '#'  */
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

  case 851: /* $@87: %empty  */
                   {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 852: /* $@88: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 853: /* $@89: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 854: /* $@90: %empty  */
                                                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 855: /* tuple_alias_declaration: "tuple" $@87 optional_public_or_private_alias "name" optional_emit_semis $@88 '{' $@89 tuple_alias_type_list optional_semis $@90 '}'  */
          {
        auto vtype = new TypeDecl(Type::tTuple);
        vtype->alias = *(yyvsp[-8].s);
        vtype->at = tokAt(scanner,(yylsp[-8]));
        vtype->isPrivateAlias = !(yyvsp[-9].b);
        varDeclToTypeDecl(scanner, vtype, (yyvsp[-3].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-8].s),tokAt(scanner,(yylsp[-8])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-8]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTuple((yyvsp[-8].s)->c_str(),atvname);
        }
        delete (yyvsp[-8].s);
    }
    break;

  case 856: /* $@91: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 857: /* $@92: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 858: /* $@93: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 859: /* $@94: %empty  */
                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 860: /* variant_alias_declaration: "variant" $@91 optional_public_or_private_alias "name" optional_emit_semis $@92 '{' $@93 variant_alias_type_list optional_semis $@94 '}'  */
          {
        auto vtype = new TypeDecl(Type::tVariant);
        vtype->alias = *(yyvsp[-8].s);
        vtype->at = tokAt(scanner,(yylsp[-8]));
        vtype->isPrivateAlias = !(yyvsp[-9].b);
        varDeclToTypeDecl(scanner, vtype, (yyvsp[-3].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-3].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-8].s),tokAt(scanner,(yylsp[-8])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-8]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariant((yyvsp[-8].s)->c_str(),atvname);
        }
        delete (yyvsp[-8].s);
    }
    break;

  case 861: /* $@95: %empty  */
                      {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 862: /* $@96: %empty  */
                                                                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 863: /* $@97: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 864: /* $@98: %empty  */
                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-7]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 865: /* bitfield_alias_declaration: "bitfield" $@95 optional_public_or_private_alias "name" bitfield_basic_type_declaration optional_emit_commas $@96 '{' $@97 bitfield_alias_bits optional_commas $@98 '}'  */
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
                CompilationError::invalid_type);
        }
        for ( auto & p : *(yyvsp[-3].pNameExprList) ) {
            if ( get<1>(p) ) {
                ast_globalBitfieldConst ( scanner, btype, (yyvsp[-10].b), get<0>(p), get<1>(p) );
            }
        }
        if ( !yyextra->g_Program->addAlias(btype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-9].s),tokAt(scanner,(yylsp[-9])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-9]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfield((yyvsp[-9].s)->c_str(),atvname);
        }
        delete (yyvsp[-9].s);
        delete (yyvsp[-3].pNameExprList);
    }
    break;

  case 866: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 867: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 868: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 869: /* make_decl: make_table_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 870: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 871: /* make_decl: table_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 872: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 873: /* make_decl_no_bracket: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 874: /* make_decl_no_bracket: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 875: /* make_decl_no_bracket: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 876: /* make_decl_no_bracket: table_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 877: /* make_decl_no_bracket: make_table_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 878: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 879: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 880: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 881: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 882: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 883: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 884: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 885: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = new MakeFieldDecl(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 886: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 887: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 888: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 889: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 890: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 891: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 892: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 893: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 894: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 895: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 896: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 897: /* $@99: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 898: /* $@100: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 899: /* make_struct_decl: "struct" '<' $@99 type_declaration_no_options '>' $@100 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                      {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 900: /* $@101: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 901: /* $@102: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 902: /* make_struct_decl: "class" '<' $@101 type_declaration_no_options '>' $@102 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                     {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 903: /* $@103: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 904: /* $@104: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 905: /* make_struct_decl: "variant" '<' $@103 variant_type_list '>' $@104 '(' use_initializer make_variant_dim ')'  */
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

  case 906: /* $@105: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 907: /* $@106: %empty  */
                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 908: /* make_struct_decl: "variant" "type" '<' $@105 type_declaration_no_options '>' $@106 '(' use_initializer make_variant_dim ')'  */
                                                                                                                                                                                                    {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-10]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 909: /* $@107: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 910: /* $@108: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 911: /* make_struct_decl: "default" '<' $@107 type_declaration_no_options '>' $@108 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 912: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = new TypeDecl(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 913: /* $@109: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 914: /* $@110: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 915: /* make_tuple_call: "tuple" '<' $@109 tuple_type_list '>' $@110 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 916: /* make_dim_decl: '[' optional_expr_list ']'  */
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

  case 917: /* $@111: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 918: /* $@112: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 919: /* make_dim_decl: "array" "struct" '<' $@111 type_declaration_no_options '>' $@112 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 920: /* $@113: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 921: /* $@114: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 922: /* make_dim_decl: "array" "tuple" '<' $@113 tuple_type_list '>' $@114 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 923: /* $@115: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 924: /* $@116: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 925: /* make_dim_decl: "array" "variant" '<' $@115 variant_type_list '>' $@116 '(' make_variant_dim ')'  */
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

  case 926: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
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

  case 927: /* $@117: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 928: /* $@118: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 929: /* make_dim_decl: "array" '<' $@117 type_declaration_no_options '>' $@118 '(' optional_expr_list ')'  */
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

  case 930: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 931: /* $@119: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 932: /* $@120: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 933: /* make_dim_decl: "fixed_array" '<' $@119 type_declaration_no_options '>' $@120 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 934: /* expr_map_tuple_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 935: /* expr_map_tuple_list: expr_map_tuple_list ',' expr  */
                                                      {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 936: /* push_table_nesting: %empty  */
                    {
        yyextra->das_nested_parentheses ++;
    }
    break;

  case 937: /* make_table_decl: '{' push_table_nesting optional_emit_semis optional_expr_map_tuple_list '}'  */
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

  case 938: /* make_table_call: "table" '(' expr_map_tuple_list optional_comma ')'  */
                                                                             {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = new TypeDecl(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 939: /* make_table_call: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 940: /* make_table_call: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 941: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 942: /* array_comprehension_where: ';' "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 943: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 944: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 945: /* table_comprehension: '[' "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                               {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 946: /* table_comprehension: '[' "iterator" "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 947: /* array_comprehension: '{' push_table_nesting optional_emit_semis "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where '}'  */
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
                CompilationError::syntax_error);
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


