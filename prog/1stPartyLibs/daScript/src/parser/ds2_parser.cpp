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
  YYSYMBOL_make_table_decl = 524,          /* make_table_decl  */
  YYSYMBOL_525_121 = 525,                  /* $@121  */
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
#define YYLAST   9312

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  208
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  323
/* YYNRULES -- Number of rules.  */
#define YYNRULES  944
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1683

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
     772,   776,   779,   783,   789,   798,   801,   807,   808,   812,
     816,   817,   821,   824,   830,   836,   839,   845,   846,   850,
     851,   855,   856,   860,   861,   861,   867,   867,   878,   879,
     883,   884,   890,   891,   892,   893,   894,   898,   899,   903,
     904,   908,   910,   908,   922,   922,   938,   940,   938,   952,
     952,   968,   970,   968,   983,   990,   997,  1002,  1011,  1019,
    1025,  1033,  1043,  1043,  1052,  1060,  1060,  1073,  1073,  1085,
    1089,  1095,  1096,  1097,  1098,  1099,  1100,  1104,  1109,  1117,
    1118,  1119,  1123,  1124,  1125,  1126,  1127,  1128,  1129,  1130,
    1131,  1137,  1140,  1146,  1149,  1155,  1156,  1157,  1158,  1162,
    1180,  1203,  1206,  1216,  1231,  1246,  1261,  1264,  1271,  1275,
    1282,  1283,  1287,  1288,  1292,  1293,  1294,  1298,  1301,  1305,
    1312,  1316,  1317,  1318,  1319,  1320,  1321,  1322,  1323,  1324,
    1325,  1326,  1327,  1328,  1329,  1330,  1331,  1332,  1333,  1334,
    1335,  1336,  1337,  1338,  1339,  1340,  1341,  1342,  1343,  1344,
    1345,  1346,  1347,  1348,  1349,  1350,  1351,  1352,  1353,  1354,
    1355,  1356,  1357,  1358,  1359,  1360,  1361,  1362,  1363,  1364,
    1365,  1366,  1367,  1368,  1369,  1370,  1371,  1372,  1373,  1374,
    1375,  1376,  1377,  1378,  1379,  1380,  1381,  1382,  1383,  1384,
    1385,  1386,  1387,  1388,  1389,  1390,  1391,  1392,  1393,  1394,
    1395,  1396,  1397,  1398,  1399,  1400,  1401,  1402,  1403,  1407,
    1408,  1409,  1410,  1411,  1412,  1413,  1414,  1415,  1416,  1417,
    1418,  1419,  1420,  1421,  1422,  1423,  1424,  1425,  1426,  1427,
    1428,  1429,  1430,  1431,  1435,  1436,  1440,  1459,  1460,  1461,
    1465,  1471,  1471,  1488,  1491,  1493,  1491,  1501,  1503,  1501,
    1518,  1527,  1536,  1549,  1550,  1551,  1552,  1553,  1554,  1555,
    1556,  1557,  1558,  1559,  1560,  1561,  1562,  1563,  1564,  1565,
    1566,  1567,  1568,  1570,  1568,  1585,  1590,  1596,  1602,  1603,
    1607,  1608,  1612,  1616,  1623,  1624,  1635,  1639,  1642,  1650,
    1650,  1650,  1653,  1659,  1662,  1666,  1670,  1677,  1684,  1690,
    1694,  1698,  1701,  1704,  1712,  1715,  1723,  1729,  1730,  1731,
    1735,  1736,  1740,  1741,  1745,  1750,  1758,  1764,  1776,  1779,
    1782,  1788,  1788,  1788,  1791,  1791,  1791,  1796,  1796,  1796,
    1804,  1804,  1804,  1810,  1820,  1831,  1846,  1849,  1852,  1855,
    1861,  1862,  1869,  1880,  1881,  1882,  1886,  1887,  1888,  1889,
    1890,  1894,  1899,  1907,  1908,  1912,  1919,  1923,  1929,  1930,
    1931,  1932,  1933,  1934,  1935,  1939,  1940,  1941,  1942,  1943,
    1944,  1945,  1946,  1947,  1948,  1949,  1950,  1951,  1952,  1953,
    1954,  1955,  1956,  1957,  1958,  1959,  1963,  1970,  1982,  1988,
    1999,  2003,  2010,  2013,  2013,  2013,  2018,  2018,  2018,  2031,
    2035,  2039,  2045,  2053,  2062,  2068,  2076,  2076,  2076,  2083,
    2087,  2096,  2104,  2112,  2116,  2119,  2127,  2128,  2129,  2136,
    2137,  2138,  2139,  2140,  2141,  2142,  2143,  2144,  2145,  2146,
    2147,  2148,  2149,  2150,  2151,  2152,  2153,  2154,  2155,  2156,
    2157,  2158,  2159,  2160,  2161,  2162,  2163,  2164,  2165,  2166,
    2167,  2168,  2169,  2170,  2171,  2177,  2178,  2179,  2180,  2181,
    2194,  2203,  2204,  2205,  2206,  2207,  2208,  2209,  2210,  2211,
    2212,  2213,  2214,  2215,  2216,  2219,  2219,  2219,  2222,  2227,
    2231,  2235,  2235,  2235,  2240,  2243,  2247,  2247,  2247,  2252,
    2255,  2256,  2257,  2258,  2259,  2260,  2261,  2262,  2263,  2265,
    2269,  2270,  2275,  2281,  2287,  2296,  2299,  2302,  2311,  2312,
    2313,  2314,  2315,  2316,  2317,  2321,  2325,  2329,  2333,  2337,
    2341,  2345,  2349,  2353,  2360,  2361,  2365,  2366,  2367,  2371,
    2372,  2376,  2377,  2378,  2382,  2383,  2387,  2399,  2402,  2403,
    2407,  2407,  2426,  2425,  2440,  2439,  2456,  2468,  2477,  2487,
    2488,  2489,  2490,  2491,  2495,  2498,  2507,  2508,  2512,  2515,
    2519,  2532,  2541,  2542,  2546,  2549,  2553,  2566,  2567,  2571,
    2577,  2583,  2592,  2595,  2602,  2605,  2611,  2612,  2613,  2617,
    2618,  2622,  2629,  2634,  2643,  2649,  2660,  2667,  2676,  2679,
    2682,  2689,  2692,  2697,  2708,  2711,  2716,  2728,  2729,  2733,
    2734,  2735,  2739,  2742,  2745,  2745,  2765,  2768,  2768,  2786,
    2791,  2799,  2800,  2804,  2807,  2820,  2837,  2838,  2839,  2844,
    2844,  2870,  2874,  2875,  2876,  2880,  2890,  2893,  2899,  2900,
    2904,  2905,  2909,  2910,  2914,  2916,  2921,  2914,  2937,  2938,
    2942,  2943,  2947,  2953,  2954,  2955,  2956,  2960,  2961,  2962,
    2966,  2969,  2975,  2977,  2982,  2975,  3003,  3010,  3015,  3024,
    3030,  3041,  3042,  3043,  3044,  3045,  3046,  3047,  3048,  3049,
    3050,  3051,  3052,  3053,  3054,  3055,  3056,  3057,  3058,  3059,
    3060,  3061,  3062,  3063,  3064,  3065,  3066,  3067,  3071,  3072,
    3073,  3074,  3075,  3076,  3077,  3078,  3082,  3093,  3097,  3104,
    3116,  3123,  3129,  3138,  3143,  3153,  3163,  3173,  3186,  3187,
    3188,  3189,  3190,  3194,  3198,  3198,  3198,  3212,  3213,  3217,
    3221,  3228,  3232,  3236,  3240,  3247,  3250,  3269,  3270,  3274,
    3275,  3276,  3277,  3278,  3278,  3278,  3282,  3287,  3294,  3301,
    3301,  3308,  3308,  3315,  3319,  3323,  3328,  3333,  3338,  3343,
    3347,  3351,  3356,  3360,  3364,  3369,  3369,  3369,  3375,  3382,
    3382,  3382,  3387,  3387,  3387,  3393,  3393,  3393,  3398,  3403,
    3403,  3403,  3408,  3408,  3408,  3417,  3422,  3422,  3422,  3427,
    3427,  3427,  3436,  3441,  3441,  3441,  3446,  3446,  3446,  3455,
    3455,  3455,  3461,  3461,  3461,  3470,  3473,  3484,  3500,  3502,
    3507,  3512,  3500,  3538,  3540,  3545,  3551,  3538,  3577,  3579,
    3584,  3589,  3577,  3630,  3631,  3632,  3633,  3634,  3635,  3636,
    3640,  3641,  3642,  3643,  3644,  3648,  3655,  3662,  3668,  3674,
    3681,  3688,  3694,  3703,  3706,  3712,  3720,  3725,  3732,  3737,
    3743,  3744,  3748,  3749,  3753,  3753,  3753,  3761,  3761,  3761,
    3768,  3768,  3768,  3779,  3779,  3779,  3786,  3786,  3786,  3797,
    3803,  3803,  3803,  3817,  3836,  3836,  3836,  3846,  3846,  3846,
    3860,  3860,  3860,  3874,  3883,  3883,  3883,  3903,  3910,  3910,
    3910,  3920,  3923,  3929,  3929,  3954,  3962,  3982,  4007,  4008,
    4012,  4013,  4018,  4021,  4028
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
  "expr_map_tuple_list", "make_table_decl", "$@121", "make_table_call",
  "array_comprehension_where", "optional_comma", "table_comprehension",
  "array_comprehension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1517)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-837)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1517,   101, -1517, -1517,    75,   -50,   122,   443, -1517,   -33,
   -1517, -1517, -1517, -1517,   476,   473, -1517, -1517, -1517, -1517,
     103,   103,   103, -1517,    80, -1517,   121, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517,    12, -1517,     0,
     181,   124, -1517, -1517,   122,    17, -1517, -1517, -1517,   221,
     103, -1517, -1517,   121,   443,   443,   443,   304,   298, -1517,
   -1517, -1517, -1517,   473,   473,   473,   275, -1517,   704,  -101,
   -1517, -1517, -1517, -1517,   374, -1517,   535, -1517,   514,    98,
      75,   354,   -50,   357,   407, -1517,   459,   482, -1517, -1517,
   -1517,   685,   507,   515,   539, -1517,   553,   560, -1517, -1517,
     -30,    75,   473,   473,   473,   473,   573, -1517,   730,   742,
     623,   673,   750, -1517, -1517,   562, -1517, -1517, -1517, -1517,
   -1517,   684,    84,   600, -1517, -1517, -1517, -1517,   758, -1517,
   -1517,   646, -1517, -1517,   636,   651,   573,   573, -1517, -1517,
     671, -1517,    99, -1517,   601,   733,   704, -1517,   715, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517,   725, -1517, -1517, -1517,
   -1517, -1517, -1517,   679, -1517, -1517, -1517,   826, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517,   145,   748, -1517,  8286,   836,
   -1517,   638,   756, -1517, -1517, -1517, -1517, -1517,  9002, -1517,
     738,   819,   186,    75,   727,   764, -1517, -1517, -1517,    84,
   -1517, -1517,   754,   760,   767,   737,   768,   771, -1517, -1517,
   -1517,   744, -1517, -1517, -1517, -1517, -1517,   243, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,   772,
   -1517, -1517, -1517,   776,   778, -1517, -1517, -1517, -1517,   783,
     796,   777,   476,    -2, -1517, -1517, -1517, -1517,   368,   735,
     805, -1517, -1517, -1517, -1517, -1517, -1517,   821, -1517,   782,
     784,  8474, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517,   948,   955, -1517,
     802, -1517,   573,   867,   756, -1517,   840,   573, -1517, -1517,
     679,   573,    75, -1517,   501, -1517, -1517, -1517, -1517, -1517,
    7166, -1517, -1517,   841,   825,   -55,    70,   217, -1517, -1517,
    7166,   337, -1517,  5256, -1517, -1517, -1517,    21, -1517, -1517,
   -1517,    15, -1517,  5447,   808,  1732, -1517,   807, -1517, -1517,
    1769,  9093, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517,   850,   816, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517,   996, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,   861,
     829, -1517, -1517,   -49,   -73, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517,   824,   855, -1517,    24, -1517,
     573,   869,  8286, -1517,   202,  8286,  8286,  8286,   853,   854,
   -1517, -1517,    36,   476,   859,    53, -1517,   406,   842,   860,
     863,   844,   865,   846,   408,   877, -1517,   488,    28,   879,
    7924,  7924,   847,   849,   862,   864,   868,   878, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517,  7924,  7924,  7924,
    7924,  7924,  3728,  4110, -1517,   838,  1047, -1517, -1517, -1517,
     881, -1517, -1517, -1517, -1517,   871, -1517, -1517, -1517,   716,
   -1517,   716,   716,   866,  8550, -1517, -1517,   882, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517,  8286,  8286,   883,   880,
    8286,   802,  8286,   802,  8286,   802,  8379,   900,   884, -1517,
    5256, -1517,  8286,  7166,   885,   906, -1517, -1517, -1517, -1517,
   -1517,   888, -1517, -1517,   895,  5638, -1517,   368, -1517,  8379,
     900, -1517, -1517, -1517, -1517, -1517, -1517,  9149,  1348,  1833,
     890, -1517,    73,   892,   -87,   894,  8286,  8286, -1517, -1517,
     896, -1517,   476, -1517,   380,   897,  1040,   598, -1517, -1517,
   -1517,   425, -1517, -1517, -1517,  7166,   263,   442,   918,   291,
   -1517, -1517, -1517, -1517,   902, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517,   489, -1517,   920,   923,   925, -1517,
    5256,  8286,  7166,  7166, -1517, -1517,  7166, -1517,  7166, -1517,
    5256, -1517, -1517,  5256,   926, -1517,  8286,   317,   317,  7166,
    7166,  7166,  7166,  7166,  7166,   739,   317,   317,    -5,   317,
     317,   907,  1095,   911,   912,   157,   906,   938,   913,   382,
     917,   573,  3346,   473,  1109,   922, -1517,   871, -1517, -1517,
   -1517, -1517,  8468,  8882,  7924,  7924, -1517, -1517,  7924,  7924,
    7924,  7924,   960,  7924,   296,  7166,  7924,  7924,  7924,  7924,
    7166,  7924,  7924,  7924,  7924,  7546,  7924,  7924,  7924,  7924,
    7924,  7924,  7924,  7924,  7924,  7924,  9060,  7166,  4301,   528,
     577, -1517, -1517,   964,   589,   -73,   666,   -73,   677,   -73,
      97, -1517,   392,   805,   952, -1517,   430, -1517,  8286,   906,
     445,   805, -1517, -1517,  5829, -1517, -1517, -1517, -1517,   930,
     967, -1517,   103, -1517,   103, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517,  7166, -1517, -1517,   225,   -76,   -76,   -76,
   -1517,   805,   805, -1517,   968, -1517, -1517, -1517, -1517,  7166,
     975,   976,  8286,   202, -1517,  7166,   103, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517,  8286,  8286,  8286,  8286,  3919,   977,
    7166,  8286, -1517, -1517, -1517,  8286,   906,   522, -1517,   969,
     942,  8286,  8286,   943,  8286,   944,  8286,   906,  8286,  8379,
     906, -1517,   900,   151,   946,   947,   954,   966,   970,   972,
   -1517,  7166,   470,   -66,   949, -1517,  7166, -1517,  7166, -1517,
    7166,   979,   334, -1517, -1517,   956,   961,   245, -1517, -1517,
     -66,  7166,    26,  3537, -1517,   252,   982,    94,   978,   802,
   -1517,  2008,  1109,   990,   984, -1517, -1517,  1005,   991, -1517,
   -1517,   693,   693,  1071,  1071,   891,   891,   993,   184,   994,
   -1517,   997,   162,   162,   882,   693,   693,  8626, -1517, -1517,
    1773,  1484,  8702,  8626,  8971,  2099,  8665,  8741,  8817,  1071,
    1071,   731,   731,   184,   184,   184,   348,  7166,   995,   998,
     418,  7166,  1145,  1007,  1009, -1517,   259, -1517, -1517, -1517,
     261, -1517,  1029, -1517,  1041, -1517,  1042,  8286, -1517,  8379,
    8286, -1517,   900,   502,  1020,  1024,  8286,  7166, -1517, -1517,
    1051,   471, -1517,  8191, -1517,   100, -1517,  1025,  1027,  1185,
   -1517, -1517,   297, -1517, -1517, -1517,  2320,  1055, -1517,   471,
      25,  1033, -1517,  1192,   425,  7166,   103, -1517, -1517, -1517,
   -1517,   805,   660,   757,   688,   565,   279,  1035,  1037,   527,
    1043,   689,  8286,  8379,   900,  1058,  1045,  1049,  8286,  7166,
    1050, -1517,  1233,  1238, -1517,  1248, -1517,  1294,  1057,  1344,
     627,  1059,  8286,   633,  1109, -1517, -1517, -1517, -1517, -1517,
    1061,  1065,  1062,  1199,  1087,    42,   -66,  1063, -1517, -1517,
   -1517,  1070,   222,  7166,  7166,  8286,   802,    46,  1053,   969,
     242, -1517,  1073,   168,  6020, -1517, -1517, -1517,   300,   -73,
   -1517,  6211, -1517, -1517,  6402,  1112,  1113, -1517,   103,  1116,
    6593,   111,  6784, -1517, -1517, -1517,   103,   103,  1267, -1517,
     785, -1517, -1517,  1266, -1517, -1517,  1271, -1517,  1240,   103,
   -1517,   103,   103,   103,   103,   103, -1517,  1217, -1517,   103,
    8023,   802, -1517,  7166, -1517,  7166,  4492,  7166, -1517,  1104,
    1085, -1517, -1517,  7924,  1086, -1517,  1088,  7166,  4683,  1090,
   -1517,  1089, -1517,  4874, -1517,  5829, -1517, -1517, -1517,  1128,
   -1517,  1129, -1517, -1517, -1517, -1517, -1517, -1517,   805, -1517,
   -1517,   805, -1517, -1517,  1024, -1517, -1517,   805, -1517,  7166,
   -1517,   571, -1517, -1517, -1517,  1091, -1517,  1094, -1517,  7166,
    1132,  1133,  8286, -1517,  7166,  1096,  7166,   596, -1517,  1141,
   -1517, -1517,  1297,   679, -1517,  1144, -1517,  7166,   103, -1517,
   -1517, -1517, -1517,  1110, -1517, -1517, -1517,  1114,  1149, -1517,
   -1517,  1514,   691,   719, -1517, -1517,  7166,  1530, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,  2253, -1517,
     185,  5065, -1517,  1142,  7166,  1152, -1517,   281,  5256,    89,
      48,   256,  7166,  7166,  7166,  1115,  1117,  3279,   -73,  5256,
   -1517, -1517, -1517,   334,  1120,  3537,  1154,  1159,  1153,  1166,
    1189, -1517,   345,   573,  7166, -1517,  1342,  7166, -1517,  1184,
    1186, -1517,  1183,  1207, -1517, -1517,  7166, -1517, -1517, -1517,
   -1517,  1167, -1517, -1517,  1168,  1170,  1171,  1173, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517,   123, -1517,  7924,  7924,  7924,
    7924,  7924,  7924,  7924,  7924,  7924,  7924,  7166,  7924,  7924,
    7924,  7924,  7924,  7924,  7924,   -73,  8286,  1177,  8286,  1178,
   -1517,   375,  1179, -1517,  7166,  1034,  7166, -1517,  1180,  3537,
   -1517,   378,  7166, -1517, -1517, -1517,   388, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517,  1200, -1517,  1181, -1517, -1517,
    1182, -1517,  1341,   147, -1517,  1349, -1517, -1517,  1190,  1214,
     753,  1322,   103, -1517,   103, -1517,  1195,  1197, -1517, -1517,
    7166,  1209, -1517, -1517, -1517, -1517,  1198,  1191,  1203,  7924,
    7924,  7924,  1206,  1330,  1211, -1517,  1216,  6975, -1517, -1517,
     390, -1517, -1517,  1208, -1517,  1244, -1517,   401,  1378,  1087,
    5256,  7166,  7166,  1224, -1517, -1517, -1517, -1517, -1517,  1250,
     451, -1517,   207, -1517, -1517,  1269, -1517, -1517,   300, -1517,
    1295, -1517, -1517, -1517,  8286,  7166, -1517, -1517, -1517, -1517,
    2526,  7166,  7166,   -66,  7166,  7166,  1087, -1517, -1517, -1517,
    8550,  8550,  8550,  8550,  8550,  8550,  8550,  8550,  8550,  8550,
    8550, -1517, -1517,  8550,  8550,  8550,  8550,  8550,  8550,  8550,
     573,  3725, -1517,   712, -1517, -1517, -1517,  8286,  1234,  1235,
   -1517,   332, -1517,  1236, -1517,  7166, -1517, -1517,  1276,  7166,
   -1517, -1517, -1517,  8286, -1517, -1517,   632, -1517,     9, -1517,
   -1517,  1330,  1330,  1241,  1243,  1247,  1249,  1252,  5256, -1517,
    7166,  1071,  1071,  1071,  5256, -1517, -1517,  1330,  1253,  1330,
   -1517,  1263, -1517, -1517,  1274, -1517, -1517,  1230,  1281,   454,
     458, -1517, -1517,   267,   559, -1517,  1246,  1264,  1265, -1517,
    7735, -1517, -1517, -1517,   805, -1517,  1261,  1270,  1273,    64,
    1277,  1278,   460,   -84, -1517, -1517, -1517,   724, -1517, -1517,
    1283, -1517, -1517, -1517, -1517,  1272,   274,  1421,     9, -1517,
   -1517,   753,   106,   106, -1517,  7166,  1330,  1330,   565,  1285,
    1287,   906,   106,  1330,   565, -1517, -1517,  7166, -1517, -1517,
    1286,  7166,  7166, -1517,   559,  7166, -1517, -1517,  7924,  8626,
    1349,  1477,   573,  5256,   573,   573,   -83, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,  1421,
     225,   565,  1321,  1323, -1517,  1296,  1298,  1299,   106,   106,
    1321,  1301, -1517, -1517,  1302,  1303,   565,  1306,  1305,  7166,
   -1517, -1517, -1517,  1305,  1034, -1517,  7357,   103, -1517,   475,
   -1517, -1517,  8286,   202, -1517,  2732,  9002, -1517, -1517, -1517,
   -1517,   481,  1310, -1517, -1517, -1517, -1517,  1311,  1313, -1517,
   -1517, -1517,  1314, -1517,  1465,  1325,  1305,  1315, -1517, -1517,
   -1517, -1517, -1517,  8550, -1517,  1317,   573, -1517, -1517,   287,
    7166,  1324,   103,  9002, -1517,   565, -1517, -1517, -1517,  7166,
   -1517,  1328, -1517, -1517,   700,  7357, -1517,  7166,   103, -1517,
   -1517,   573,   483, -1517, -1517, -1517,   573, -1517, -1517,  1320,
   -1517,   103, -1517,   103, -1517, -1517, -1517,  2938, -1517,  7166,
   -1517, -1517, -1517,  1326,  1329,  1327,  1349, -1517, -1517,  7357,
     573, -1517, -1517,   103, -1517,  3144, -1517,  1329,  1332,   700,
    1349, -1517, -1517
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   162,     1,   360,     0,     0,     0,   666,   361,     0,
     858,   848,   853,    20,     0,     0,    19,    16,    15,     3,
       0,     0,     0,     8,   702,     7,   647,     6,    11,     5,
       4,    13,    12,    14,   130,   131,   129,   139,   141,    49,
      62,    59,    60,    51,     0,    57,    50,   668,   667,     0,
       0,    26,    25,   647,   666,   666,   666,     0,   334,    47,
     146,   147,   148,     0,     0,     0,   149,   151,   158,     0,
     145,    21,    10,     9,   284,   684,     0,   648,   649,     0,
       0,     0,     0,    52,     0,    58,     0,     0,    55,   669,
     671,    22,     0,     0,     0,   336,     0,     0,   157,   152,
       0,     0,     0,     0,     0,     0,    71,   285,   287,   672,
     694,   693,   697,   651,   650,   657,   137,   138,   135,   136,
     133,     0,     0,     0,   132,   142,    63,    61,    57,    54,
      53,     0,    23,    24,    27,   758,    71,    71,   335,    45,
      48,   156,     0,   153,   154,   155,   159,    69,    72,   163,
     289,   288,   291,   286,   674,   673,     0,   696,   695,   699,
     698,   703,   652,   574,    30,    31,    35,     0,   125,   126,
     123,   124,   122,   121,   127,     0,     0,    56,     0,     0,
      29,     0,   682,   849,   854,    46,   150,    70,     0,   675,
     676,   690,   654,     0,   575,     0,    32,    33,    34,     0,
     140,   134,     0,     0,     0,     0,     0,     0,   711,   731,
     712,   747,   713,   717,   718,   719,   720,   737,   724,   725,
     726,   727,   728,   729,   730,   732,   733,   734,   735,   818,
     716,   723,   736,   825,   832,   714,   721,   715,   722,     0,
       0,     0,     0,   746,   779,   782,   780,   781,   845,   775,
     670,    28,   761,   762,   759,   760,   680,   683,   859,     0,
       0,     0,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,     0,     0,   170,
     164,   258,    71,     0,   682,   691,     0,    71,   656,   653,
     574,    71,     0,   636,   629,   658,   128,   783,   809,   812,
       0,   815,   805,     0,     0,   819,   826,   833,   839,   842,
       0,   777,   789,   328,   795,   800,   794,     0,   808,   804,
     797,     0,   799,     0,   776,     0,   681,     0,   850,   855,
     249,   250,   247,   173,   174,   176,   175,   177,   178,   179,
     180,   206,   207,   204,   205,   197,   208,   209,   198,   195,
     196,   248,   231,     0,   246,   210,   211,   212,   213,   184,
     185,   186,   181,   182,   183,   194,     0,   200,   201,   199,
     192,   193,   188,   187,   189,   190,   191,   172,   171,   230,
       0,   202,   203,   574,   167,   297,   738,   741,   744,   745,
     739,   742,   740,   743,   677,     0,   688,   704,     0,   143,
      71,     0,     0,   630,     0,     0,     0,     0,     0,     0,
     475,   476,     0,     0,     0,     0,   469,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   737,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   563,   408,
     410,   409,   411,   412,   413,   414,    41,     0,     0,     0,
       0,     0,   328,     0,   393,   394,   933,   473,   472,   550,
     470,   543,   542,   541,   540,   160,   546,   471,   545,   544,
     517,   477,   518,     0,   466,   522,   478,     0,   474,   870,
     872,   871,   467,   874,   873,   468,     0,     0,     0,   764,
       0,   164,     0,   164,     0,   164,     0,     0,     0,   791,
       0,   788,     0,     0,     0,   940,   386,   802,   803,   796,
     798,     0,   801,   772,     0,     0,   847,   846,   860,   608,
     614,   251,   253,   252,   254,   245,   229,   255,   232,   214,
       0,   165,   359,   599,   600,     0,     0,     0,   290,   292,
       0,   685,     0,   692,     0,     0,   631,   629,   655,   144,
     637,     0,   627,   628,   626,     0,     0,     0,     0,   769,
     894,   897,   339,   746,   343,   342,   348,   863,   869,   864,
     865,   866,   868,   867,     0,   380,     0,     0,     0,   924,
       0,     0,     0,     0,   371,   374,     0,   377,     0,   928,
       0,   906,   910,     0,     0,   900,     0,   505,   506,     0,
       0,     0,     0,     0,     0,     0,   482,   481,   519,   480,
     479,     0,     0,     0,     0,   334,   940,   940,     0,   395,
       0,    71,     0,     0,   403,   394,   325,   160,   301,   302,
     300,   786,     0,     0,     0,     0,   507,   508,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   456,     0,     0,     0,
       0,   748,   763,     0,     0,   167,     0,   167,     0,   167,
     334,   606,     0,   604,     0,   612,     0,   749,     0,   940,
       0,   332,   387,   787,   941,   329,   793,   771,   774,     0,
     753,   609,    89,   615,    89,   256,   257,   234,   235,   237,
     236,   238,   239,   240,   241,   233,   242,   243,   244,   218,
     219,   221,   220,   222,   223,   224,   225,   216,   217,   226,
     227,   228,   215,     0,   357,   358,     0,   574,   574,   574,
     166,   169,   168,   325,   663,   689,   700,   587,   705,     0,
       0,     0,     0,     0,   644,     0,     0,   784,   810,   813,
      18,    17,   767,   768,     0,     0,     0,     0,   892,     0,
       0,     0,   914,   917,   920,     0,   940,     0,   931,   940,
       0,     0,     0,     0,     0,     0,     0,   940,     0,     0,
     940,   903,     0,     0,     0,     0,     0,     0,     0,     0,
      44,     0,    42,     0,     0,   913,     0,   618,     0,   617,
       0,     0,   941,   885,   510,     0,     0,   443,   440,   442,
       0,   330,     0,   328,   459,     0,     0,     0,     0,   164,
     395,     0,   403,     0,     0,   529,   528,     0,     0,   530,
     534,   483,   484,   496,   497,   494,   495,     0,   523,     0,
     515,     0,   547,   548,   549,   485,   486,   552,   553,   554,
     501,   502,   503,   504,     0,     0,   499,   500,   498,   492,
     493,   488,   487,   489,   490,   491,     0,     0,     0,   449,
       0,     0,     0,     0,     0,   464,     0,   816,   806,   750,
       0,   820,     0,   827,     0,   834,     0,     0,   840,     0,
       0,   843,     0,     0,     0,   777,     0,     0,   388,   773,
     754,   678,    87,    90,   851,    90,   856,     0,     0,   706,
     596,   597,   619,   601,   603,   602,     0,   659,   664,   678,
     590,     0,   633,   634,     0,     0,     0,   646,   785,   811,
     814,   770,     0,     0,     0,   893,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   941,
       0,   520,     0,     0,   521,     0,   551,     0,     0,     0,
       0,     0,     0,     0,   403,   558,   559,   560,   561,   562,
       0,    38,     0,   105,     0,     0,     0,     0,   876,   875,
     509,     0,     0,     0,     0,     0,   164,     0,     0,   940,
       0,   460,     0,     0,     0,   463,   461,   161,     0,   167,
     327,   351,   349,   297,     0,     0,     0,   350,     0,     0,
       0,    71,     0,   322,   407,   303,     0,     0,     0,   316,
       0,   317,   311,     0,   308,   307,     0,   309,     0,     0,
     326,     0,    85,    86,    83,    84,   318,   363,   306,     0,
     415,   164,   525,     0,   531,     0,     0,     0,   513,     0,
       0,   535,   539,     0,     0,   516,     0,     0,     0,     0,
     450,     0,   457,     0,   511,     0,   465,   817,   807,     0,
     765,     0,   821,   823,   828,   830,   835,   837,   605,   841,
     607,   611,   844,   613,   777,   778,   790,   333,   389,     0,
     661,   679,   861,    88,   610,     0,   616,     0,   598,     0,
       0,     0,     0,   620,     0,     0,     0,   679,   686,     0,
     588,   701,     0,   574,   632,     0,   641,     0,     0,   645,
     895,   898,   340,     0,   345,   346,   344,     0,     0,   383,
     381,     0,     0,     0,   925,   923,   330,     0,   932,   935,
     372,   375,   378,   929,   927,   907,   911,   909,     0,   901,
      71,     0,    39,     0,     0,     0,   364,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   167,     0,
     934,   331,   462,     0,     0,   328,     0,     0,     0,     0,
       0,   401,     0,    71,     0,   352,     0,     0,   337,     0,
       0,   321,     0,     0,    66,   297,     0,   354,   325,   319,
     320,     0,    78,    79,     0,     0,     0,     0,   310,   305,
     312,   313,   314,   315,   362,     0,   304,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   167,     0,     0,     0,     0,
     438,     0,     0,   536,     0,   524,     0,   514,     0,   328,
     451,     0,     0,   512,   458,   454,     0,   752,   766,   751,
     824,   831,   838,   792,   755,   756,   662,     0,   852,   857,
       0,   708,   709,   622,   621,   293,   660,   665,     0,     0,
     581,   584,     0,   635,     0,   643,     0,     0,   341,   347,
       0,     0,   382,   915,   918,   921,     0,     0,     0,     0,
       0,     0,     0,   892,     0,   904,     0,     0,   297,   564,
       0,    36,    43,     0,   107,     0,   108,     0,   109,     0,
       0,     0,     0,     0,   878,   877,   441,   573,   444,     0,
       0,   436,     0,   398,   399,     0,   397,   396,     0,   404,
     297,   353,   297,   338,     0,     0,    64,    65,   114,   355,
       0,     0,     0,     0,     0,     0,     0,   638,   369,   368,
     427,   428,   430,   429,   431,   421,   422,   423,   432,   433,
     417,   418,   419,   420,   434,   435,   424,   425,   426,   416,
      71,     0,   572,     0,   570,   439,   567,     0,     0,     0,
     566,     0,   452,     0,   455,     0,   862,   707,     0,     0,
     294,   299,   687,     0,   582,   583,   584,   585,   576,   591,
     642,   892,   892,     0,     0,     0,     0,     0,   328,   936,
     330,   373,   376,   379,     0,   893,   908,   892,     0,   892,
     555,     0,   557,   565,    40,   106,   365,     0,     0,     0,
       0,   880,   879,     0,     0,   447,     0,     0,     0,   402,
       0,   390,   405,   356,   120,   119,     0,     0,     0,     0,
       0,     0,     0,     0,   297,   526,   532,     0,   571,   569,
       0,   568,   757,   710,   623,     0,     0,   579,   576,   577,
     578,   581,   891,   891,   384,     0,   892,   892,   883,     0,
       0,   940,   891,   892,   883,   556,    37,     0,   110,   111,
       0,     0,     0,   445,     0,     0,   437,   400,     0,   391,
     293,    80,    71,     0,    71,    71,   629,   370,   639,   640,
     406,   527,   533,   537,   453,   325,   589,   580,   592,   579,
       0,     0,   888,   940,   890,     0,     0,     0,   891,   891,
     884,     0,   926,   937,     0,     0,   883,     0,   938,     0,
     882,   881,   448,   938,   392,   324,     0,     0,   102,     0,
     297,   297,     0,     0,   538,     0,     0,   594,   625,   624,
     586,     0,   941,   889,   896,   899,   385,     0,     0,   922,
     930,   912,     0,   902,     0,     0,   938,     0,    81,    85,
      86,    83,    84,    82,   104,    94,    71,   116,   118,     0,
       0,     0,     0,     0,   886,     0,   916,   919,   905,     0,
     942,     0,   944,    91,    73,     0,   297,     0,     0,   296,
     593,    71,     0,   939,   943,   325,    71,    67,    68,     0,
     103,     0,   113,     0,   367,   297,   887,     0,    74,     0,
      95,   366,   595,     0,    99,     0,   293,    96,    75,     0,
      71,    93,   325,     0,    76,     0,   100,    99,     0,    73,
     293,    77,    98
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1517, -1517,  -883,    -1, -1517, -1517, -1517, -1517, -1517,   914,
    1452, -1517, -1517, -1517, -1517, -1517, -1517,  1532, -1517, -1517,
   -1517,  1492, -1517,  1411, -1517, -1517,  1458, -1517, -1517, -1517,
   -1517,  -133,  -138, -1517, -1517, -1517, -1517, -1516,   828,   832,
   -1517, -1517, -1517, -1517,  -129, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517,  -814, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517,  1350, -1517, -1517,   -44,  1449, -1517, -1517, -1517,   464,
     919,   916,   614,  -474,  -663, -1517,  -295, -1517, -1517, -1517,
   -1269, -1517, -1517, -1471, -1517, -1517,  -983, -1517, -1517, -1517,
   -1517, -1517, -1517,  -741,  -317, -1131,   857,   -13, -1517, -1517,
   -1517, -1517, -1517, -1507, -1506, -1503, -1499, -1517, -1517,  1555,
   -1517, -1012, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517,  -453, -1517,   499,   199, -1517,
    -764, -1517,   351, -1517, -1517, -1517, -1517, -1357, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517,   361,   675, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517,  -154,    60,    10,
      59,   135, -1517, -1517, -1517, -1517, -1517, -1517, -1517,   249,
    -501,  -756, -1517,  -507,  -767, -1517,  -919,    14,    16, -1517,
    -553,  -550, -1517, -1517, -1517, -1203, -1517,  1509, -1517, -1517,
   -1517, -1517, -1517,   440,   631, -1517,   945, -1517, -1517, -1517,
   -1517, -1517, -1517,   634, -1517,  1284, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517,  -134, -1517,  1150, -1517, -1517, -1517,  1354, -1517, -1517,
   -1517,  -552, -1517, -1517,  -272,  -878, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517,  -160, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517,  -757, -1405,  -602, -1517, -1517, -1264, -1289,
    1157, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517, -1517,
   -1517,  1158, -1517, -1517,  1160, -1517, -1517, -1517, -1517, -1517,
   -1517, -1517, -1517, -1517, -1517,   989,  -417, -1517,  1161, -1488,
    -612,  1163,  -411
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,   772,   773,    18,   134,    53,   180,    19,   167,
     173,  1454,  1173,  1331,   615,   467,   140,   468,    97,    21,
      22,    45,    46,    88,    23,    41,    42,  1036,  1037,  1649,
     148,   149,  1650,  1664,  1677,  1224,  1577,  1038,   923,   924,
    1634,  1645,  1663,  1635,  1668,  1672,  1678,  1669,  1039,  1040,
    1615,  1041,   995,  1042,  1043,  1044,  1045,  1046,  1047,  1048,
    1049,   174,   175,    37,    38,    39,   194,    66,    67,    68,
      69,   634,    24,   394,   548,   290,   291,   108,    25,   152,
     292,   153,   188,  1421,  1495,  1621,   549,   550,  1125,   469,
    1050,  1218,  1476,   841,   623,  1008,   700,   470,  1051,   574,
     777,  1308,   471,  1052,  1053,  1054,  1055,  1056,   746,  1057,
    1235,  1177,  1378,  1058,   472,   791,  1319,   792,  1320,   794,
    1321,   473,   781,  1312,   474,   515,  1472,   475,  1201,  1202,
     839,   476,   638,   477,  1059,   478,   479,   829,   480,  1005,
    1464,  1006,  1524,   481,   892,  1274,   482,   516,   484,  1256,
    1541,  1258,  1542,  1407,  1584,   485,   486,   542,  1501,  1548,
    1426,  1428,  1302,   940,  1133,  1586,  1623,   543,   544,   545,
     691,   692,   712,   695,   696,   714,   820,   930,   931,  1590,
     565,   414,   557,   304,  1483,   558,   305,    78,   115,   192,
     300,    27,   163,   938,  1111,   939,    49,    50,   131,    28,
     156,   190,   294,  1112,   257,   258,    29,   109,   754,  1298,
     553,   296,   297,   112,   161,   758,    30,    76,   191,   554,
     932,   487,   404,   245,   246,   900,   921,   182,   247,   683,
    1278,   909,   568,   334,   248,   511,   249,   415,   948,   512,
     698,   497,  1088,   416,   949,   417,   950,   496,  1087,   500,
    1092,   501,  1280,   502,  1094,   503,  1281,   504,  1096,   505,
    1282,   506,  1099,   507,  1102,   693,    31,    55,   259,   529,
    1115,    32,    56,   260,   530,  1117,    33,    54,   337,   710,
    1287,   576,   488,   627,  1561,   628,  1553,  1554,  1555,   958,
     489,   775,  1306,   776,  1307,   802,  1326,   982,  1448,   798,
    1323,   490,   799,  1324,   491,   962,  1435,   963,  1436,   964,
    1437,   785,  1316,   796,  1322,  1009,   492,   631,   493,  1605,
     705,   494,   495
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      17,    59,    70,   183,   184,   580,   514,   763,   765,   195,
     626,   583,   936,  1124,   821,   823,  1007,   774,   250,    71,
      72,    73,   902,   713,   904,  1317,   906,   685,   711,   687,
     836,   689,  1379,  -162,  1446,   983,   125,  1106,  1110,    84,
    1206,   418,   419,   980,   244,   532,   534,   604,   519,    90,
      70,    70,    70,   546,   517,  1499,  1110,   699,   586,  1575,
    1608,   425,  1178,   527,   555,  1129,  1189,   427,  1340,  1609,
    1610,  -822,   105,  1611,    85,   540,  1013,  1612,  1061,   556,
     744,    13,   992,  1184,  1533,  1607,   748,   914,    74,    70,
      70,    70,    70,  1582,    34,    35,   413,   993,   106,   168,
     169,     2,   540,   547,   434,   435,    40,  1523,     3,  1567,
     102,   103,   104,   116,   117,  1500,    75,   749,  1631,  1651,
      16,  -822,  1537,   745,   654,   655,  -822,   193,  1609,  1610,
      51,     4,  1611,     5,   994,     6,  1612,   786,   437,   438,
     561,     7,  1502,  1503,   912,  -822,   408,   797,   916,   301,
     800,     8,   541,  1673,   193,   587,   588,     9,  1512,   395,
    1514,  1602,  1609,  1610,   407,   243,  1611,  1572,   409,    52,
    1612,   141,   555,    80,   966,    57,   957,   970,   322,   835,
     624,    10,  1180,    77,   324,   978,    79,   556,   981,  1010,
    1130,   299,   676,   677,   520,  1671,  -829,  1153,   323,    58,
     518,   244,    86,    11,    12,   642,   643,  1152,   605,  1682,
     945,   325,   326,   521,    87,  1179,   572,  1558,  1559,  1179,
    1170,  1179,   522,    15,  1566,   896,  1283,  1011,  1286,   321,
    1012,  1131,  1368,   589,   462,   968,    96,  1179,    36,  1556,
     170,   466,   716,   868,  1286,   171,  -829,   172,  1565,   869,
     121,  -829,  1338,   590,   118,   566,   567,   569,   410,   119,
     624,   120,  1342,   694,   121,    13,    13,   105,    13,  1010,
    -829,   555,    80,   907,   817,   327,   147,   559,   244,   328,
    1539,   244,   244,   244,   817,    43,   556,    14,   652,  1339,
     818,   654,   655,  1017,  1597,  1598,    96,    82,   122,    15,
     186,   123,   646,   647,    16,    16,  1551,    16,    44,  1510,
     652,   598,   653,   654,   655,   656,   657,  1622,   199,   787,
     324,   819,   243,  1376,   335,   956,   329,  1459,  1377,   562,
     330,   819,   984,   331,   803,   563,   679,   680,   642,   643,
     684,  1193,   686,  -836,   688,  1452,   200,   325,   326,   817,
     147,    13,   701,  1143,  1641,  1183,  1203,    81,   332,   676,
     677,   916,   244,   244,  1482,  1019,   244,  1194,   244,   817,
     244,  -446,   244,   928,  1419,   818,   564,  1471,   244,  1473,
    1193,   676,   677,   817,    89,  1327,   751,   752,   929,  1341,
      16,  1137,   298,  -836,   817,   244,   819,  1191,  -836,   243,
    1521,   324,   243,   243,   243,  1103,  1467,  1148,  1100,   573,
     584,   327,   244,   244,   562,   328,   819,  -836,  1116,   181,
     563,  -446,  1114,  -758,   817,  1014,  -446,  1196,   325,   326,
     819,   912,  1085,  1197,  1089,   646,   647,    96,  1352,    13,
     335,   819,  1090,   652,   767,  -446,   654,   655,   656,   657,
     859,   335,  1085,  1015,  1335,   770,    13,   244,   324,   860,
    1086,   564,   329,  1198,   771,  1091,   330,    95,   335,   331,
    1121,   819,   244,  1122,  1199,   101,  1123,  1370,    16,  1200,
    1144,   107,  1336,   243,   243,   325,   326,   243,  1001,   243,
     944,   243,   327,   243,   332,    16,   328,  1002,   831,   243,
      47,  1540,  1074,   952,   953,  1193,    48,    60,   846,   850,
     126,  1075,  1411,   965,   676,   677,   243,   509,  1358,   972,
     973,    57,   975,   864,   977,  1349,   979,    98,    99,   100,
     825,  1490,  1188,   243,   243,   826,    61,   510,   701,   755,
     110,   111,   893,   329,  1522,    58,  1359,   330,  1085,   327,
     331,  1085,   562,   328,    87,   324,   770,    13,   563,  1479,
     764,  1085,   827,  1085,   244,   771,   143,   144,   145,   146,
     128,   113,  1079,   908,  1085,   332,  1405,   114,   243,  1412,
      62,  1080,   325,   326,   756,   757,   591,  1255,   599,  1414,
      13,  1453,  1400,   243,   770,    13,    16,  1617,  1618,   564,
     329,   912,  1457,   771,   330,    63,   592,   331,   600,   770,
      13,   911,    57,  1261,   951,    57,   828,   954,   771,   335,
      70,   961,   129,   768,  1085,  1271,   915,  1335,   244,    16,
    1276,  1085,   332,  1335,    16,   770,    58,   196,   197,    58,
     244,   244,   244,   244,   771,   130,   327,   244,  1085,    16,
     328,   244,  1466,  1652,  1193,  1519,  1193,   244,   244,  1520,
     244,  1536,   244,    64,   244,   244,   770,    13,   602,   779,
     135,   483,  1662,    65,   411,   771,  1616,   412,   136,  1497,
     413,   508,  1624,  1104,  1656,   243,   770,    13,   603,   780,
    1151,   770,    13,   324,   524,   771,  1157,   329,    57,  1427,
     771,   330,   137,   967,   331,   335,    16,   825,  1147,   897,
    1168,   922,  1646,   922,   642,   643,   138,   252,  1330,   624,
     325,   326,    58,  1647,  1648,  1337,    16,   139,  1010,   332,
     157,    16,   253,  1187,  1285,   770,  1350,   254,   147,   255,
    1072,   102,   132,   104,   771,  1552,  1552,  1098,   133,   243,
    1101,  1560,   642,   643,   335,  1552,  1107,  1560,   898,   937,
     770,   243,   243,   243,   243,   947,   335,   162,   243,   771,
     901,   761,   243,   244,   762,   244,   244,   413,   243,   243,
     158,   243,   244,   243,   327,   243,   243,   150,   328,   244,
     324,   770,    13,   151,  1591,  1222,  1223,   770,    13,   154,
     771,  1552,  1552,   176,  1585,   155,   771,   159,  1166,  1560,
    1424,   646,   647,   160,  1169,    85,  1425,   325,   326,   652,
     178,   653,   654,   655,   656,   657,   179,   181,   244,   244,
    1391,    16,   639,   640,   244,   329,  1392,    16,   185,   330,
    1035,  1140,   331,   335,   102,   103,   104,   903,   244,   646,
     647,   164,   165,   166,   335,   770,    13,   652,   905,   653,
     654,   655,   656,   657,   771,   335,   335,   332,  1642,  1142,
    1150,   244,  1314,   102,   702,   671,   672,   673,   674,   675,
     187,   327,   193,   770,    13,   328,   709,  1460,   189,   335,
     676,   677,   771,  1486,   243,    16,   243,   243,  1215,  1564,
    1315,   335,   251,   243,  1657,  1543,   164,   165,   810,   811,
     243,   201,   642,   643,   293,   673,   674,   675,   464,   635,
     256,   636,  1113,    16,  1113,   295,   766,   303,   676,   677,
     302,  1675,   329,   333,   307,  1035,   330,   310,  1141,   331,
     308,  1593,   396,  1136,   313,  1139,   397,   309,   311,   243,
     243,   312,   315,   788,   790,   243,   316,   793,   317,   795,
     398,   399,  1293,   318,   332,   400,   401,   402,   403,   243,
     804,   805,   806,   807,   808,   809,   319,   320,   637,  1301,
     637,   637,   335,   391,  1401,   336,  1583,   338,   244,   339,
     392,  1511,   243,   196,   197,   198,   933,   934,   935,    92,
      93,    94,   393,   406,   498,   499,   525,   644,   645,   646,
     647,   648,   528,   535,   649,   536,   861,   652,   537,   653,
     654,   655,   656,   657,   538,   658,   659,  1211,   539,   551,
    1620,   552,   560,   570,   571,  1219,  1220,  1328,   894,   585,
     594,   629,   593,   595,   596,   597,   598,   609,  1228,   610,
    1229,  1230,  1231,  1232,  1233,   642,   643,   601,  1236,   606,
     630,   682,   611,   694,   612,   918,  1637,   641,   613,   633,
    1360,   669,   670,   671,   672,   673,   674,   675,   614,   704,
    1579,   632,   678,   760,   681,   697,   703,   706,   676,   677,
     743,   324,   642,   643,   707,   750,   747,   759,  1403,   769,
     782,   753,   778,   783,   927,   784,   801,   813,   814,   243,
     815,   822,   816,   838,   824,   607,   608,   830,   325,   326,
     941,  1509,   244,   857,   244,   840,   946,   899,   910,   919,
     920,   937,   616,   617,   618,   619,   620,  1305,   942,   943,
     959,   960,   969,   971,   974,   976,  1082,   985,   986,   996,
     644,   645,   646,   647,   648,   987,  1003,   649,   650,   651,
     652,  1004,   653,   654,   655,   656,   657,   988,   658,   659,
    1062,   989,   991,   990,   661,   662,   663,   997,  1018,   998,
    1000,   999,   327,  1016,  1063,  1064,   328,   644,   645,   646,
     647,  1065,   788,  1066,  1067,  1077,  1068,   652,  1078,   653,
     654,   655,   656,   657,  1474,   658,   659,  1083,  1084,   665,
    1093,   666,   667,   668,   669,   670,   671,   672,   673,   674,
     675,  1105,  1095,  1097,   510,  1109,  1118,  1119,  1120,  1126,
     244,   676,   677,   329,  1134,  1135,  1145,   330,  1146,  1154,
     331,  1172,  1175,   243,  1149,   243,  1155,  1487,  1076,  1156,
    1176,  1159,  1081,   671,   672,   673,   674,   675,  1164,  1190,
    1167,  1171,  1174,  1496,  1181,   332,   324,  1484,   676,   677,
    1182,   324,  1212,   244,  1192,  1209,  1210,  1221,  1108,  1225,
    1226,   324,  1227,  1234,  1263,  1264,  1266,  1267,  1273,   244,
    1272,  1277,  1279,   325,   326,  1291,  1292,  1288,   325,   326,
    1289,  1429,  1295,  1430,  1299,  1300,  1138,  1303,   325,   326,
    1619,  1309,  1311,  1332,  1310,  1334,  1346,  1353,  1347,   851,
     852,  1351,  1354,   853,   854,   855,   856,   324,   858,  1356,
    1158,   862,   863,   865,   866,   867,   870,   871,   872,   873,
     875,   876,   877,   878,   879,   880,   881,   882,   883,   884,
     885,   243,  1357,  1355,   325,   326,  1362,   327,  1364,  1366,
    1365,   328,   327,  1367,  1185,  1186,   328,  1371,  1372,  1035,
    1373,  1374,   327,  1375,  1415,   918,   328,   324,  1402,  1404,
    1406,  1410,  1205,  1417,  1418,  1208,  1420,  1416,  1423,  1427,
    1434,  1214,  1439,  1217,   243,  1431,  1422,  1432,  1438,  1578,
    1445,  1580,  1581,  1440,   325,   326,  1444,  1456,   329,  1455,
     243,  1447,   330,   329,  1160,   331,  1449,   330,   327,  1161,
     331,  1458,   328,   329,  1257,  1463,  1259,   330,  1262,  1162,
     331,  1465,  1468,  1470,  1517,  1488,  1489,  1491,  1268,  1493,
     332,  1516,  1504,  1505,  1518,   332,   918,  1506,   244,  1507,
    1525,   828,  1508,  1513,  1547,   332,   717,   718,   719,   720,
     721,   722,   723,   724,  1515,  1526,  1527,  1530,   327,   329,
    1284,  1531,   328,   330,  1532,  1163,   331,  1545,  1534,  1535,
    1290,   725,  1538,  1636,  1544,  1294,  1562,  1296,  1563,  1576,
    1569,   726,   727,   728,  1193,  1546,  1592,  1594,  1304,  1595,
    1596,   332,  1599,  1600,  1601,   642,   643,  1603,  1655,  1604,
    1625,   828,  1626,  1658,  1627,  1628,  1060,   788,  1629,   329,
    1659,  1632,  1633,   330,  1630,  1165,   331,  1644,  1670,   812,
    1639,   124,  1666,    20,  1667,  1333,    83,  1674,  1680,   177,
     127,  1681,   925,  1343,  1344,  1345,   926,   324,  1679,   306,
     142,   332,   837,   842,  1132,   913,    26,  1469,  1549,  1587,
    1550,  1498,    91,   324,  1588,  1361,  1589,  1297,  1363,   243,
    1127,   314,   575,  1128,   325,   326,  1614,  1369,   405,   577,
     578,   789,   579,   581,  1035,   582,     0,     0,     0,     0,
     325,   326,     0,     0,     0,     0,     0,     0,     0,     0,
     644,   645,   646,   647,   648,     0,     0,   649,   650,   651,
     652,  1060,   653,   654,   655,   656,   657,     0,   658,   659,
       0,  1640,     0,     0,   661,  1408,   663,  1409,     0,     0,
       0,     0,     0,  1413,     0,     0,     0,  1654,   327,     0,
       0,     0,   328,     0,     0,     0,     0,     0,     0,     0,
    1660,     0,  1661,     0,   327,     0,  1035,     0,   328,     0,
       0,   666,   667,   668,   669,   670,   671,   672,   673,   674,
     675,  1433,  1676,     0,  1035,     0,     0,     0,     0,     0,
       0,   676,   677,     0,     0,     0,     0,     0,  1451,   329,
       0,     0,     0,   330,     0,  1313,   331,     0,     0,     0,
       0,     0,  1461,  1462,     0,   329,     0,     0,     0,   330,
       0,  1318,   331,     0,     0,     0,     0,     0,     0,     0,
       0,   332,     0,     0,     0,     0,  1475,     0,     0,     0,
       0,     0,  1477,  1478,     0,  1480,  1481,   332,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1265,     0,
       0,   202,     0,     0,     0,     0,     0,   203,     0,     0,
       0,     0,     0,   204,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   205,     0,     0,  1492,     0,     0,     0,
    1494,   206,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   642,   643,   207,     0,     0,     0,
       0,   788,     0,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,     0,     0,     0,     0,
     262,     0,   263,     0,   264,   265,   266,   267,   268,     0,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,     0,   280,   281,   282,     0,  1557,   283,   284,   285,
     286,    57,     0,     0,     0,     0,     0,     0,  1568,     0,
       0,     0,  1570,  1571,   241,     0,  1573,     0,     0,   644,
     645,   646,   647,   648,     0,    58,   649,   650,   651,   652,
       0,   653,   654,   655,   656,   657,     0,   658,   659,     0,
       0,     0,  1380,  1381,  1382,  1383,  1384,  1385,  1386,  1387,
    1388,  1389,  1390,  1393,  1394,  1395,  1396,  1397,  1398,  1399,
    1606,     0,   531,     0,   242,     0,     0,     0,     0,   526,
       0,   729,   730,   731,   732,   733,   734,   735,   736,     0,
     666,   667,   668,   669,   670,   671,   672,   673,   674,   675,
     737,     0,     0,     0,     0,     0,   738,     0,     0,     0,
     676,   677,     0,     0,     0,     0,   739,   740,   741,     0,
       0,  1638,     0,     0,     0,     0,     0,     0,     0,     0,
    1643,     0,     0,     0,  1441,  1442,  1443,     0,  1653,     0,
       0,     0,     0,     0,     0,     0,     0,   742,     0,  1020,
       0,     0,     0,   418,   419,     3,     0,  -115,  -101,  -101,
    1665,  -112,     0,   420,   421,   422,   423,   424,     0,     0,
       0,     0,     0,   425,  1021,   426,  1022,  1023,     0,   427,
       0,     0,     0,     0,     0,  1060,  1024,   428,  1025,     0,
    -117,     0,  1026,   429,     0,     0,   430,     0,     8,   431,
    1027,     0,  1028,   432,     0,     0,  1029,  1030,     0,     0,
       0,     0,     0,  1031,     0,     0,   434,   435,     0,   208,
     209,   210,     0,   212,   213,   214,   215,   216,   436,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
       0,   230,   231,   232,     0,     0,   235,   236,   237,   238,
     437,   438,   439,  1032,     0,     0,     0,     0,     0,     0,
     642,   643,     0,     0,     0,     0,   440,   441,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1529,     0,    57,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,   445,   446,
       0,   447,     0,   448,   449,   450,   451,   452,   453,   454,
     455,    58,     0,    13,   456,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     457,   458,   459,     0,    14,     0,     0,   460,   461,     0,
       0,     0,     0,  1574,     0,     0,   462,     0,   463,     0,
     464,   465,    16,  1033,  1034,   644,   645,   646,   647,   648,
       0,     0,   649,   650,   651,   652,     0,   653,   654,   655,
     656,   657,     0,   658,   659,     0,     0,   660,     0,   661,
     662,   663,     0,     0,     0,   664,     0,     0,     0,     0,
       0,  1613,     0,     0,     0,     0,     0,     0,     0,     0,
    1060,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   665,  1073,   666,   667,   668,   669,
     670,   671,   672,   673,   674,   675,   324,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   676,   677,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1613,     0,     0,   325,   326,     0,     0,     0,     0,     0,
       0,  1020,     0,     0,     0,   418,   419,     3,     0,  -115,
    -101,  -101,  1060,  -112,     0,   420,   421,   422,   423,   424,
       0,     0,     0,     0,  1613,   425,  1021,   426,  1022,  1023,
    1060,   427,     0,     0,     0,     0,     0,     0,  1024,   428,
    1025,     0,  -117,     0,  1026,   429,     0,     0,   430,     0,
       8,   431,  1027,     0,  1028,   432,     0,   327,  1029,  1030,
       0,   328,     0,     0,     0,  1031,     0,     0,   434,   435,
       0,   208,   209,   210,     0,   212,   213,   214,   215,   216,
     436,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,     0,   230,   231,   232,     0,     0,   235,   236,
     237,   238,   437,   438,   439,  1032,     0,     0,   329,     0,
       0,     0,   330,     0,  1325,   331,     0,     0,   440,   441,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    57,
     332,     0,     0,     0,     0,     0,     0,   442,   443,   444,
     445,   446,     0,   447,     0,   448,   449,   450,   451,   452,
     453,   454,   455,    58,     0,    13,   456,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   457,   458,   459,     0,    14,     0,     0,   460,
     461,     0,     0,     0,     0,     0,     0,     0,   462,     0,
     463,     0,   464,   465,    16,  1033,  -298,  1020,     0,     0,
       0,   418,   419,     3,     0,  -115,  -101,  -101,     0,  -112,
       0,   420,   421,   422,   423,   424,     0,     0,     0,     0,
       0,   425,  1021,   426,  1022,  1023,     0,   427,     0,     0,
       0,     0,     0,     0,  1024,   428,  1025,     0,  -117,     0,
    1026,   429,     0,     0,   430,     0,     8,   431,  1027,     0,
    1028,   432,     0,     0,  1029,  1030,     0,     0,     0,     0,
       0,  1031,     0,     0,   434,   435,     0,   208,   209,   210,
       0,   212,   213,   214,   215,   216,   436,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,     0,   230,
     231,   232,     0,     0,   235,   236,   237,   238,   437,   438,
     439,  1032,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   440,   441,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    57,     0,     0,     0,     0,
       0,     0,     0,   442,   443,   444,   445,   446,     0,   447,
       0,   448,   449,   450,   451,   452,   453,   454,   455,    58,
       0,    13,   456,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   457,   458,
     459,     0,    14,     0,     0,   460,   461,     0,     0,     0,
       0,     0,     0,     0,   462,     0,   463,     0,   464,   465,
      16,  1033,  -323,  1020,     0,     0,     0,   418,   419,     3,
       0,  -115,  -101,  -101,     0,  -112,     0,   420,   421,   422,
     423,   424,     0,     0,     0,     0,     0,   425,  1021,   426,
    1022,  1023,     0,   427,     0,     0,     0,     0,     0,     0,
    1024,   428,  1025,     0,  -117,     0,  1026,   429,     0,     0,
     430,     0,     8,   431,  1027,     0,  1028,   432,     0,     0,
    1029,  1030,     0,     0,     0,     0,     0,  1031,     0,     0,
     434,   435,     0,   208,   209,   210,     0,   212,   213,   214,
     215,   216,   436,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,     0,   230,   231,   232,     0,     0,
     235,   236,   237,   238,   437,   438,   439,  1032,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     440,   441,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    57,     0,     0,     0,     0,     0,     0,     0,   442,
     443,   444,   445,   446,     0,   447,     0,   448,   449,   450,
     451,   452,   453,   454,   455,    58,     0,    13,   456,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   457,   458,   459,     0,    14,     0,
       0,   460,   461,     0,     0,     0,     0,     0,     0,     0,
     462,     0,   463,     0,   464,   465,    16,  1033,  -295,  1020,
       0,     0,     0,   418,   419,     3,     0,  -115,  -101,  -101,
       0,  -112,     0,   420,   421,   422,   423,   424,     0,     0,
       0,     0,     0,   425,  1021,   426,  1022,  1023,     0,   427,
       0,     0,     0,     0,     0,     0,  1024,   428,  1025,     0,
    -117,     0,  1026,   429,     0,     0,   430,     0,     8,   431,
    1027,     0,  1028,   432,     0,     0,  1029,  1030,     0,     0,
       0,     0,     0,  1031,     0,     0,   434,   435,     0,   208,
     209,   210,     0,   212,   213,   214,   215,   216,   436,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
       0,   230,   231,   232,     0,     0,   235,   236,   237,   238,
     437,   438,   439,  1032,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   440,   441,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    57,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,   445,   446,
       0,   447,     0,   448,   449,   450,   451,   452,   453,   454,
     455,    58,     0,    13,   456,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     457,   458,   459,     0,    14,     0,     0,   460,   461,     0,
       0,     0,     0,     0,     0,     0,   462,     0,   463,     0,
     464,   465,    16,  1033,   -92,  1020,     0,     0,     0,   418,
     419,     3,     0,  -115,  -101,  -101,     0,  -112,     0,   420,
     421,   422,   423,   424,     0,     0,     0,     0,     0,   425,
    1021,   426,  1022,  1023,     0,   427,     0,     0,     0,     0,
       0,     0,  1024,   428,  1025,     0,  -117,     0,  1026,   429,
       0,     0,   430,     0,     8,   431,  1027,     0,  1028,   432,
       0,     0,  1029,  1030,     0,     0,     0,     0,     0,  1031,
       0,     0,   434,   435,     0,   208,   209,   210,     0,   212,
     213,   214,   215,   216,   436,   218,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,     0,   230,   231,   232,
       0,     0,   235,   236,   237,   238,   437,   438,   439,  1032,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   440,   441,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    57,     0,     0,     0,     0,     0,     0,
       0,   442,   443,   444,   445,   446,     0,   447,     0,   448,
     449,   450,   451,   452,   453,   454,   455,    58,     0,    13,
     456,     0,   324,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   457,   458,   459,     0,
      14,     0,     0,   460,   461,     0,     0,     0,     0,   325,
     326,     0,   462,     0,   463,     0,   464,   465,    16,  1033,
     -97,   418,   419,     0,     0,     0,     0,     0,     0,     0,
       0,   420,   421,   422,   423,   424,     0,     0,     0,     0,
       0,   425,     0,   426,     0,     0,     0,   427,     0,     0,
       0,     0,     0,     0,     0,   428,     0,     0,     0,     0,
       0,   429,     0,     0,   430,     0,     0,   431,     0,     0,
       0,   432,     0,   327,     0,     0,     0,   328,     0,     0,
       0,   433,     0,     0,   434,   435,   832,   208,   209,   210,
       0,   212,   213,   214,   215,   216,   436,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,     0,   230,
     231,   232,     0,     0,   235,   236,   237,   238,   437,   438,
     439,     0,     0,     0,   329,     0,     0,     0,   330,     0,
    1348,   331,     0,     0,   440,   441,     0,     0,     0,     0,
       0,     0,     0,   513,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    57,   332,     0,     0,     0,
       0,     0,     0,   442,   443,   444,   445,   446,     0,   447,
     624,   448,   449,   450,   451,   452,   453,   454,   455,   625,
       0,     0,   456,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   457,   458,
     459,     0,    14,     0,     0,   460,   461,     0,     0,     0,
       0,     0,   418,   419,   833,     0,   463,   834,   464,   465,
     621,   466,   420,   421,   422,   423,   424,     0,     0,     0,
       0,     0,   425,     0,   426,     0,     0,     0,   427,     0,
       0,     0,     0,     0,     0,     0,   428,     0,     0,     0,
       0,     0,   429,     0,     0,   430,   622,     0,   431,     0,
       0,     0,   432,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   433,     0,     0,   434,   435,     0,   208,   209,
     210,     0,   212,   213,   214,   215,   216,   436,   218,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,     0,
     230,   231,   232,     0,     0,   235,   236,   237,   238,   437,
     438,   439,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   440,   441,     0,     0,     0,
       0,     0,     0,     0,   513,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    57,     0,     0,     0,
       0,     0,     0,     0,   442,   443,   444,   445,   446,     0,
     447,   624,   448,   449,   450,   451,   452,   453,   454,   455,
     625,     0,     0,   456,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   457,
     458,   459,     0,    14,     0,     0,   460,   461,     0,     0,
       0,     0,     0,   418,   419,   462,     0,   463,     0,   464,
     465,   621,   466,   420,   421,   422,   423,   424,     0,     0,
       0,     0,     0,   425,     0,   426,     0,     0,   324,   427,
       0,     0,     0,     0,     0,     0,     0,   428,     0,     0,
       0,     0,     0,   429,     0,     0,   430,   622,     0,   431,
       0,     0,     0,   432,     0,   325,   326,     0,     0,     0,
       0,     0,     0,   433,     0,     0,   434,   435,     0,   208,
     209,   210,     0,   212,   213,   214,   215,   216,   436,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
       0,   230,   231,   232,     0,     0,   235,   236,   237,   238,
     437,   438,   439,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   440,   441,     0,   327,
       0,     0,     0,   328,     0,   513,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    57,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,   445,   446,
       0,   447,     0,   448,   449,   450,   451,   452,   453,   454,
     455,    58,     0,     0,   456,     0,     0,     0,     0,     0,
     329,     0,     0,     0,   330,     0,  1485,   331,     0,     0,
     457,   458,   459,     0,    14,     0,     0,   460,   461,     0,
       0,     0,     0,     0,   418,   419,   462,     0,   463,     0,
     464,   465,   332,   466,   420,   421,   422,   423,   424,     0,
       0,     0,     0,     0,   425,     0,   426,     0,     0,     0,
     427,     0,     0,     0,     0,     0,     0,     0,   428,     0,
       0,     0,     0,     0,   429,     0,     0,   430,     0,     0,
     431,     0,     0,     0,   432,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   433,     0,     0,   434,   435,   955,
     208,   209,   210,     0,   212,   213,   214,   215,   216,   436,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,     0,   230,   231,   232,     0,     0,   235,   236,   237,
     238,   437,   438,   439,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   440,   441,     0,
       0,     0,     0,     0,     0,     0,   513,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    57,     0,
       0,     0,     0,     0,     0,     0,   442,   443,   444,   445,
     446,     0,   447,   624,   448,   449,   450,   451,   452,   453,
     454,   455,   625,     0,     0,   456,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   457,   458,   459,     0,    14,     0,     0,   460,   461,
       0,     0,     0,     0,     0,   418,   419,   462,     0,   463,
       0,   464,   465,     0,   466,   420,   421,   422,   423,   424,
       0,     0,     0,     0,     0,   425,     0,   426,     0,     0,
       0,   427,     0,     0,     0,     0,     0,     0,     0,   428,
       0,     0,     0,     0,     0,   429,     0,     0,   430,     0,
       0,   431,     0,     0,     0,   432,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   433,     0,     0,   434,   435,
       0,   208,   209,   210,     0,   212,   213,   214,   215,   216,
     436,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,     0,   230,   231,   232,     0,     0,   235,   236,
     237,   238,   437,   438,   439,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   440,   441,
       0,     0,     0,     0,     0,     0,     0,   513,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    57,
       0,     0,     0,     0,     0,     0,     0,   442,   443,   444,
     445,   446,     0,   447,   624,   448,   449,   450,   451,   452,
     453,   454,   455,   625,     0,     0,   456,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   457,   458,   459,     0,    14,     0,     0,   460,
     461,     0,     0,     0,     0,     0,   418,   419,   462,     0,
     463,     0,   464,   465,     0,   466,   420,   421,   422,   423,
     424,     0,     0,     0,     0,     0,   425,     0,   426,     0,
       0,     0,   427,     0,     0,     0,     0,     0,     0,     0,
     428,     0,     0,     0,     0,     0,   429,     0,     0,   430,
       0,     0,   431,     0,     0,     0,   432,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   433,     0,     0,   434,
     435,     0,   208,   209,   210,     0,   212,   213,   214,   215,
     216,   436,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,     0,   230,   231,   232,     0,     0,   235,
     236,   237,   238,   437,   438,   439,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   440,
     441,     0,     0,     0,     0,     0,     0,     0,   513,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      57,     0,     0,     0,     0,     0,     0,     0,   442,   443,
     444,   445,   446,     0,   447,     0,   448,   449,   450,   451,
     452,   453,   454,   455,    58,     0,     0,   456,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   457,   458,   459,     0,    14,     0,     0,
     460,   461,     0,     0,     0,     0,     0,   418,   419,   462,
       0,   463,   895,   464,   465,     0,   466,   420,   421,   422,
     423,   424,     0,     0,     0,     0,     0,   425,     0,   426,
       0,     0,     0,   427,     0,     0,     0,     0,     0,     0,
       0,   428,     0,     0,     0,     0,     0,   429,     0,     0,
     430,     0,     0,   431,     0,     0,     0,   432,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   433,     0,     0,
     434,   435,     0,   208,   209,   210,     0,   212,   213,   214,
     215,   216,   436,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,     0,   230,   231,   232,     0,     0,
     235,   236,   237,   238,   437,   438,   439,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     440,   441,     0,     0,     0,     0,     0,     0,     0,   513,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    57,     0,     0,     0,     0,     0,     0,     0,   442,
     443,   444,   445,   446,     0,   447,     0,   448,   449,   450,
     451,   452,   453,   454,   455,    58,     0,     0,   456,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   457,   458,   459,     0,    14,     0,
       0,   460,   461,     0,     0,     0,     0,     0,   418,   419,
     462,     0,   463,  1260,   464,   465,     0,   466,   420,   421,
     422,   423,   424,     0,     0,     0,     0,     0,   425,     0,
     426,     0,     0,     0,   427,     0,     0,     0,     0,     0,
       0,     0,   428,     0,     0,     0,     0,     0,   429,     0,
       0,   430,     0,     0,   431,     0,     0,     0,   432,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   433,     0,
       0,   434,   435,     0,   208,   209,   210,     0,   212,   213,
     214,   215,   216,   436,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,     0,   230,   231,   232,     0,
       0,   235,   236,   237,   238,   437,   438,   439,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   440,   441,     0,     0,     0,     0,     0,     0,     0,
     513,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    57,     0,     0,     0,     0,     0,     0,     0,
     442,   443,   444,   445,   446,     0,   447,     0,   448,   449,
     450,   451,   452,   453,   454,   455,    58,     0,     0,   456,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   457,   458,   459,     0,    14,
       0,     0,   460,   461,     0,     0,     0,     0,     0,   418,
     419,  1269,     0,   463,  1270,   464,   465,     0,   466,   420,
     421,   422,   423,   424,     0,     0,     0,     0,     0,   425,
       0,   426,     0,     0,     0,   427,     0,     0,     0,     0,
       0,     0,     0,   428,     0,     0,     0,     0,     0,   429,
       0,     0,   430,     0,     0,   431,     0,     0,     0,   432,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   433,
       0,     0,   434,   435,     0,   208,   209,   210,     0,   212,
     213,   214,   215,   216,   436,   218,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,     0,   230,   231,   232,
       0,     0,   235,   236,   237,   238,   437,   438,   439,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   440,   441,     0,     0,     0,     0,     0,     0,
       0,   513,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    57,     0,     0,     0,     0,     0,     0,
       0,   442,   443,   444,   445,   446,     0,   447,     0,   448,
     449,   450,   451,   452,   453,   454,   455,    58,     0,     0,
     456,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   457,   458,   459,     0,
      14,     0,     0,   460,   461,     0,     0,     0,     0,     0,
     418,   419,   462,     0,   463,  1275,   464,   465,     0,   466,
     420,   421,   422,   423,   424,     0,     0,     0,     0,     0,
     425,     0,   426,     0,     0,     0,   427,     0,     0,     0,
       0,     0,     0,     0,   428,     0,     0,     0,     0,     0,
     429,     0,     0,   430,     0,     0,   431,     0,     0,     0,
     432,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     433,     0,     0,   434,   435,     0,   208,   209,   210,     0,
     212,   213,   214,   215,   216,   436,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,     0,   230,   231,
     232,     0,     0,   235,   236,   237,   238,   437,   438,   439,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   440,   441,     0,     0,     0,     0,     0,
       0,     0,   513,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    57,     0,     0,     0,     0,     0,
       0,     0,   442,   443,   444,   445,   446,     0,   447,     0,
     448,   449,   450,   451,   452,   453,   454,   455,    58,     0,
       0,   456,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   457,   458,   459,
       0,    14,     0,     0,   460,   461,     0,     0,     0,     0,
       0,   418,   419,   462,     0,   463,  1329,   464,   465,     0,
     466,   420,   421,   422,   423,   424,     0,     0,     0,     0,
       0,   425,     0,   426,     0,     0,     0,   427,     0,     0,
       0,     0,     0,     0,     0,   428,     0,     0,     0,     0,
       0,   429,     0,     0,   430,     0,     0,   431,     0,     0,
       0,   432,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   433,     0,     0,   434,   435,     0,   208,   209,   210,
       0,   212,   213,   214,   215,   216,   436,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,     0,   230,
     231,   232,     0,     0,   235,   236,   237,   238,   437,   438,
     439,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   440,   441,     0,     0,     0,     0,
       0,     0,     0,   513,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    57,     0,     0,     0,     0,
       0,     0,     0,   442,   443,   444,   445,   446,     0,   447,
       0,   448,   449,   450,   451,   452,   453,   454,   455,    58,
       0,     0,   456,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   457,   458,
     459,     0,    14,     0,     0,   460,   461,     0,     0,     0,
       0,     0,   418,   419,   462,     0,   463,     0,   464,   465,
       0,   466,   420,   421,   422,   423,   424,     0,     0,     0,
       0,     0,   425,     0,   426,     0,     0,     0,   427,     0,
       0,     0,     0,     0,     0,     0,   428,     0,     0,     0,
       0,     0,   429,     0,     0,   430,     0,     0,   431,     0,
       0,     0,   432,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   433,     0,     0,   434,   435,     0,   208,   209,
     210,     0,   212,   213,   214,   215,   216,   436,   218,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,     0,
     230,   231,   232,     0,     0,   235,   236,   237,   238,   437,
     438,   439,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   440,   441,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    57,     0,     0,     0,
       0,     0,     0,     0,   442,   443,   444,   445,   446,     0,
     447,     0,   448,   449,   450,   451,   452,   453,   454,   455,
      58,     0,     0,   456,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   457,
     458,   459,     0,    14,     0,     0,   460,   461,     0,     0,
       0,     0,     0,   418,   419,   462,   523,   463,     0,   464,
     465,     0,   466,   420,   421,   422,   423,   424,     0,     0,
       0,     0,     0,   425,     0,   426,     0,     0,     0,   427,
       0,     0,     0,     0,     0,     0,     0,   428,     0,     0,
       0,     0,     0,   429,     0,     0,   430,     0,     0,   431,
       0,     0,     0,   432,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   433,     0,     0,   434,   435,     0,   208,
     209,   210,     0,   212,   213,   214,   215,   216,   436,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
       0,   230,   231,   232,     0,     0,   235,   236,   237,   238,
     437,   438,   439,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   440,   441,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    57,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,   445,   446,
       0,   447,     0,   448,   449,   450,   451,   452,   453,   454,
     455,    58,     0,     0,   456,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     457,   458,   459,     0,    14,     0,     0,   460,   461,     0,
       0,     0,     0,     0,   418,   419,   462,   708,   463,     0,
     464,   465,     0,   466,   420,   421,   422,   423,   424,     0,
       0,     0,     0,     0,   425,     0,   426,     0,     0,     0,
     427,     0,     0,     0,     0,     0,     0,     0,   428,     0,
       0,     0,     0,     0,   429,     0,     0,   430,     0,     0,
     431,     0,     0,     0,   432,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   433,     0,     0,   434,   435,     0,
     208,   209,   210,     0,   212,   213,   214,   215,   216,   436,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,     0,   230,   231,   232,     0,     0,   235,   236,   237,
     238,   437,   438,   439,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   440,   441,     0,
       0,     0,     0,     0,     0,     0,   917,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    57,     0,
       0,     0,     0,     0,     0,     0,   442,   443,   444,   445,
     446,     0,   447,     0,   448,   449,   450,   451,   452,   453,
     454,   455,    58,     0,     0,   456,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   457,   458,   459,     0,    14,     0,     0,   460,   461,
       0,     0,     0,     0,     0,   418,   419,   462,     0,   463,
       0,   464,   465,     0,   466,   420,   421,   422,   423,   424,
       0,     0,     0,     0,     0,   425,     0,   426,     0,     0,
       0,   427,     0,     0,     0,     0,     0,     0,     0,   428,
       0,     0,     0,     0,     0,   429,     0,     0,   430,     0,
       0,   431,     0,     0,     0,   432,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   433,     0,     0,   434,   435,
       0,   208,   209,   210,     0,   212,   213,   214,   215,   216,
     436,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,     0,   230,   231,   232,     0,     0,   235,   236,
     237,   238,   437,   438,   439,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   440,   441,
       0,     0,     0,     0,     0,     0,     0,   917,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    57,
       0,     0,     0,     0,     0,     0,     0,   442,   443,   444,
     445,   446,     0,   447,     0,   448,   449,   450,   451,   452,
     453,   454,   455,    58,     0,     0,   456,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   457,   458,   459,     0,    14,     0,     0,   460,
     461,     0,     0,     0,     0,     0,   418,   419,  1195,     0,
     463,     0,   464,   465,     0,   466,   420,   421,   422,   423,
     424,     0,     0,     0,     0,     0,   425,     0,   426,     0,
       0,     0,   427,     0,     0,     0,     0,     0,     0,     0,
     428,     0,     0,     0,     0,     0,   429,     0,     0,   430,
       0,     0,   431,     0,     0,     0,   432,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   433,     0,     0,   434,
     435,     0,   208,   209,   210,     0,   212,   213,   214,   215,
     216,   436,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,     0,   230,   231,   232,     0,     0,   235,
     236,   237,   238,   437,   438,   439,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   440,
     441,     0,     0,     0,     0,     0,     0,     0,  1204,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      57,     0,     0,     0,     0,     0,     0,     0,   442,   443,
     444,   445,   446,     0,   447,     0,   448,   449,   450,   451,
     452,   453,   454,   455,    58,     0,     0,   456,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   457,   458,   459,     0,    14,     0,     0,
     460,   461,     0,     0,     0,     0,     0,   418,   419,   462,
       0,   463,     0,   464,   465,     0,   466,   420,   421,   422,
     423,   424,     0,     0,     0,     0,     0,   425,     0,   426,
       0,     0,     0,   427,     0,     0,     0,     0,     0,     0,
       0,   428,     0,     0,     0,     0,     0,   429,     0,     0,
     430,     0,     0,   431,     0,     0,     0,   432,     0,     0,
       0,     0,     0,  1207,     0,     0,     0,   433,     0,     0,
     434,   435,     0,   208,   209,   210,     0,   212,   213,   214,
     215,   216,   436,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,     0,   230,   231,   232,     0,     0,
     235,   236,   237,   238,   437,   438,   439,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     440,   441,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    57,     0,     0,     0,     0,     0,     0,     0,   442,
     443,   444,   445,   446,     0,   447,     0,   448,   449,   450,
     451,   452,   453,   454,   455,    58,     0,     0,   456,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   457,   458,   459,     0,    14,     0,
       0,   460,   461,     0,     0,     0,     0,     0,   418,   419,
     462,     0,   463,     0,   464,   465,     0,   466,   420,   421,
     422,   423,   424,     0,     0,     0,     0,     0,   425,     0,
     426,     0,     0,     0,   427,     0,     0,     0,     0,     0,
       0,     0,   428,     0,     0,     0,     0,     0,   429,     0,
       0,   430,     0,     0,   431,     0,     0,     0,   432,     0,
       0,  1213,     0,     0,     0,     0,     0,     0,   433,     0,
       0,   434,   435,     0,   208,   209,   210,     0,   212,   213,
     214,   215,   216,   436,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,     0,   230,   231,   232,     0,
       0,   235,   236,   237,   238,   437,   438,   439,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   440,   441,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    57,     0,     0,     0,     0,     0,     0,     0,
     442,   443,   444,   445,   446,     0,   447,     0,   448,   449,
     450,   451,   452,   453,   454,   455,    58,     0,     0,   456,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   457,   458,   459,     0,    14,
       0,     0,   460,   461,     0,     0,     0,     0,     0,   418,
     419,   462,     0,   463,     0,   464,   465,     0,   466,   420,
     421,   422,   423,   424,     0,     0,     0,     0,     0,   425,
       0,   426,     0,     0,     0,   427,     0,     0,     0,     0,
       0,     0,     0,   428,     0,     0,     0,     0,     0,   429,
       0,     0,   430,     0,     0,   431,     0,     0,     0,   432,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   433,
       0,     0,   434,   435,     0,   208,   209,   210,     0,   212,
     213,   214,   215,   216,   436,   218,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,     0,   230,   231,   232,
       0,     0,   235,   236,   237,   238,   437,   438,   439,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   440,   441,     0,     0,     0,     0,     0,     0,
       0,  1216,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    57,     0,     0,     0,     0,     0,     0,
       0,   442,   443,   444,   445,   446,     0,   447,     0,   448,
     449,   450,   451,   452,   453,   454,   455,    58,     0,     0,
     456,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   457,   458,   459,     0,
      14,     0,     0,   460,   461,     0,     0,     0,     0,     0,
     418,   419,   462,     0,   463,     0,   464,   465,     0,   466,
     420,   421,   422,   423,   424,     0,     0,     0,     0,     0,
     425,     0,   426,     0,     0,     0,   427,     0,     0,     0,
       0,     0,     0,     0,   428,     0,     0,     0,     0,     0,
     429,     0,     0,   430,     0,     0,   431,     0,     0,     0,
     432,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     433,     0,     0,   434,   435,     0,   208,   209,   210,     0,
     212,   213,   214,   215,   216,   436,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,     0,   230,   231,
     232,     0,     0,   235,   236,   237,   238,   437,   438,   439,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   440,   441,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    57,     0,     0,     0,     0,     0,
       0,     0,   442,   443,   444,   445,   446,     0,   447,     0,
     448,   449,   450,   451,   452,   453,   454,   455,    58,     0,
       0,   456,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   457,   458,   459,
       0,    14,     0,     0,   460,   461,     0,     0,     0,     0,
       0,   418,   419,   462,     0,   463,  1450,   464,   465,     0,
     466,   420,   421,   422,   423,   424,     0,     0,     0,     0,
       0,   425,     0,   426,     0,     0,     0,   427,     0,     0,
       0,     0,     0,     0,     0,   428,     0,     0,     0,     0,
       0,   429,     0,     0,   430,     0,     0,   431,     0,     0,
       0,   432,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   433,     0,     0,   434,   435,     0,   208,   209,   210,
       0,   212,   213,   214,   215,   216,   436,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,     0,   230,
     231,   232,     0,     0,   235,   236,   237,   238,   437,   438,
     439,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   440,   441,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    57,     0,     0,     0,     0,
       0,     0,     0,   442,   443,   444,   445,   446,     0,   447,
       0,   448,   449,   450,   451,   452,   453,   454,   455,    58,
       0,     0,   456,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   457,   458,
     459,     0,    14,     0,     0,   460,   461,     0,     0,     0,
       0,     0,   418,   419,   462,     0,   463,     0,   464,   465,
       0,   466,   420,   421,   422,   423,   424,     0,     0,     0,
       0,     0,   425,  1021,   426,  1022,     0,     0,   427,     0,
       0,     0,     0,     0,     0,     0,   428,     0,     0,     0,
       0,     0,   429,     0,     0,   430,     0,     0,   431,  1027,
       0,     0,   432,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   433,     0,     0,   434,   435,     0,   208,   209,
     210,     0,   212,   213,   214,   215,   216,   436,   218,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,     0,
     230,   231,   232,     0,     0,   235,   236,   237,   238,   437,
     438,   439,  1032,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   440,   441,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    57,     0,     0,     0,
       0,     0,     0,     0,   442,   443,   444,   445,   446,     0,
     447,     0,   448,   449,   450,   451,   452,   453,   454,   455,
      58,     0,     0,   456,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   457,
     458,   459,     0,    14,     0,     0,   460,   461,     0,     0,
       0,   418,   419,     0,     0,   462,     0,   463,     0,   464,
     465,   420,   421,   422,   423,   424,     0,     0,   874,     0,
       0,   425,     0,   426,     0,     0,     0,   427,     0,     0,
       0,     0,     0,     0,     0,   428,     0,     0,     0,     0,
       0,   429,     0,     0,   430,     0,     0,   431,     0,     0,
       0,   432,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   433,     0,     0,   434,   435,     0,   208,   209,   210,
       0,   212,   213,   214,   215,   216,   436,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,     0,   230,
     231,   232,     0,     0,   235,   236,   237,   238,   437,   438,
     439,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   440,   441,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    57,     0,     0,     0,     0,
       0,     0,     0,   442,   443,   444,   445,   446,     0,   447,
       0,   448,   449,   450,   451,   452,   453,   454,   455,    58,
       0,     0,   456,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   457,   458,
     459,     0,    14,     0,     0,   460,   461,     0,     0,     0,
     418,   419,     0,     0,   462,     0,   463,     0,   464,   465,
     420,   421,   422,   423,   424,     0,     0,     0,     0,     0,
     425,     0,   426,     0,     0,     0,   427,     0,     0,     0,
       0,     0,     0,     0,   428,     0,     0,     0,     0,     0,
     429,     0,     0,   430,     0,     0,   431,     0,     0,     0,
     432,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     433,     0,     0,   434,   435,     0,   208,   209,   210,     0,
     212,   213,   214,   215,   216,   436,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,     0,   230,   231,
     232,     0,     0,   235,   236,   237,   238,   437,   438,   439,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   440,   441,     0,     0,     0,     0,     0,
       0,     0,  1528,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    57,     0,     0,     0,     0,     0,
       0,     0,   442,   443,   444,   445,   446,     0,   447,     0,
     448,   449,   450,   451,   452,   453,   454,   455,    58,     0,
       0,   456,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   457,   458,   459,
       0,    14,     0,     0,   460,   461,     0,     0,     0,   418,
     419,     0,     0,   462,     0,   463,     0,   464,   465,   420,
     421,   422,   423,   424,     0,     0,     0,     0,     0,   425,
       0,   426,     0,     0,     0,   427,     0,     0,     0,     0,
       0,     0,     0,   428,     0,     0,     0,     0,     0,   429,
       0,     0,   430,     0,     0,   431,     0,     0,     0,   432,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   433,
       0,     0,   434,   435,     0,   208,   209,   210,     0,   212,
     213,   214,   215,   216,   436,   218,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,     0,   230,   231,   232,
       0,     0,   235,   236,   237,   238,   437,   438,   439,     0,
       0,     0,     0,   -82,     0,     0,     0,     0,     0,     0,
       0,     0,   440,   441,   642,   643,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    57,     0,     0,     0,     0,     0,     0,
       0,   442,   443,   444,   445,   446,     0,   447,     0,   448,
     449,   450,   451,   452,   453,   454,   455,    58,     0,     0,
     456,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   457,   458,   459,     0,
      14,     0,     0,   460,   461,     0,     0,     0,     0,     0,
       0,     0,   462,     0,   463,     0,   464,   465,     0,     0,
       0,  1237,  1238,  1239,  1240,  1241,  1242,  1243,  1244,   644,
     645,   646,   647,   648,  1245,  1246,   649,   650,   651,   652,
    1247,   653,   654,   655,   656,   657,  1248,   658,   659,  1249,
    1250,   660,     0,   661,   662,   663,  1251,  1252,  1253,   664,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1254,   665,     0,
     666,   667,   668,   669,   670,   671,   672,   673,   674,   675,
     202,     0,     0,     0,     0,     0,   203,     0,     0,     0,
     676,   677,   204,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   205,     0,     0,     0,     0,     0,     0,     0,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   207,     0,     0,     0,     0,
       0,     0,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   202,     0,     0,     0,     0,
       0,   203,     0,     0,     0,     0,     0,   204,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   205,     0,     0,
      57,     0,     0,     0,     0,   206,     0,     0,     0,     0,
       0,     0,     0,   241,     0,     0,     0,     0,     0,     0,
     207,     0,     0,     0,   690,     0,    13,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
       0,     0,     0,   242,     0,    16,     0,     0,   202,     0,
       0,     0,     0,     0,   203,     0,     0,     0,     0,     0,
     204,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     205,     0,     0,     0,     0,    57,     0,     0,   206,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   241,     0,
       0,     0,     0,   207,     0,     0,     0,     0,     0,    58,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,   229,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,     0,     0,     0,     0,   843,   242,     0,
       0,     0,     0,     0,     0,   340,   341,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   342,     0,     0,     0,     0,     0,    57,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   241,     0,     0,     0,     0,     0,     0,     0,   208,
     209,   210,   690,   212,   213,   214,   215,   216,   436,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
       0,   230,   231,   232,     0,     0,   235,   236,   237,   238,
       0,   642,   643,     0,     0,     0,     0,     0,     0,     0,
       0,   242,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   360,
       0,     0,   361,   362,   363,     0,     0,   364,   365,   366,
     367,   368,     0,     0,   369,   370,   371,   372,   373,   374,
     375,     0,   844,     0,     0,     0,     0,     0,     0,     0,
       0,   845,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   642,   643,   376,
       0,   377,   378,   379,   380,   381,   382,   383,   384,   385,
     386,     0,     0,   387,   388,     0,   644,   645,   646,   647,
     648,   389,   390,   649,   650,   651,   652,     0,   653,   654,
     655,   656,   657,     0,   658,   659,   642,   643,   660,     0,
     661,   662,   663,     0,     0,     0,   664,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   642,   643,   665,     0,   666,   667,   668,
     669,   670,   671,   672,   673,   674,   675,     0,     0,     0,
       0,     0,   644,   645,   646,   647,   648,   676,   677,   649,
     650,   651,   652,     0,   653,   654,   655,   656,   657,     0,
     658,   659,   642,   643,  -837,     0,   661,   662,   663,     0,
       0,     0,  -837,     0,     0,     0,     0,     0,     0,     0,
       0,   644,   645,   646,   647,   648,     0,     0,   649,   650,
     651,   652,     0,   653,   654,   655,   656,   657,     0,   658,
     659,   665,     0,   666,   667,   668,   669,   670,   671,   672,
     673,   674,   675,     0,     0,     0,     0,     0,   644,   645,
     646,   647,   648,   676,   677,   649,   650,   651,   652,     0,
     653,   654,   655,   656,   657,     0,   658,   659,   642,   643,
       0,     0,   661,   667,   668,   669,   670,   671,   672,   673,
     674,   675,     0,     0,     0,     0,     0,   644,   645,   646,
     647,   648,   676,   677,   649,   650,   651,   652,     0,   653,
     654,   655,   656,   657,     0,   658,   659,     0,     0,   666,
     667,   668,   669,   670,   671,   672,   673,   674,   675,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   676,
     677,   847,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     668,   669,   670,   671,   672,   673,   674,   675,     0,     0,
       0,     0,     0,   644,   645,   646,   647,   648,   676,   677,
     649,   650,   651,   652,     0,   653,   654,   655,   656,   657,
       0,   658,   659,   208,   209,   210,     0,   212,   213,   214,
     215,   216,   436,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,     0,   230,   231,   232,     0,     0,
     235,   236,   237,   238,     0,     0,     0,     0,     0,     0,
    1069,     0,     0,     0,     0,     0,     0,   669,   670,   671,
     672,   673,   674,   675,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   676,   677,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   848,   261,     0,     0,
       0,     0,   208,   209,   210,   849,   212,   213,   214,   215,
     216,   436,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,     0,   230,   231,   232,     0,     0,   235,
     236,   237,   238,   262,     0,   263,     0,   264,   265,   266,
     267,   268,     0,   269,   270,   271,   272,   273,   274,   275,
     276,   277,   278,   279,     0,   280,   281,   282,     0,     0,
     283,   284,   285,   286,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     287,   288,     0,     0,     0,  1070,     0,     0,     0,     0,
       0,   208,   209,   210,  1071,   212,   213,   214,   215,   216,
     436,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,     0,   230,   231,   232,     0,     0,   235,   236,
     237,   238,     0,     0,   262,   289,   263,     0,   264,   265,
     266,   267,   268,     0,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,     0,   280,   281,   282,   886,
     887,   283,   284,   285,   286,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   888,     0,     0,     0,     0,     0,
     262,     0,   263,   889,   264,   265,   266,   267,   268,     0,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,     0,   280,   281,   282,     0,     0,   283,   284,   285,
     286,     0,     0,     0,     0,     0,   533,   890,   891,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   715
};

static const yytype_int16 yycheck[] =
{
       1,    14,    15,   136,   137,   422,   323,   557,   561,   163,
     463,   422,   753,   932,   626,   627,   830,   569,   178,    20,
      21,    22,   685,   530,   687,  1156,   689,   501,   529,   503,
     632,   505,  1235,     8,  1323,   802,    80,   915,   921,    22,
    1023,     5,     6,   799,   178,   340,   341,    19,    33,    50,
      63,    64,    65,   126,    33,    46,   939,   510,     5,  1530,
    1576,    25,    20,   335,   148,    40,    20,    31,    20,  1576,
    1576,   126,   173,  1576,    57,   151,   833,  1576,   842,   163,
       7,   165,   148,  1002,    20,  1573,   173,   699,     8,   102,
     103,   104,   105,   176,    19,    20,   179,   163,   199,    15,
      16,     0,   151,   176,    68,    69,   156,  1464,     7,  1514,
     140,   141,   142,    15,    16,   106,    36,   204,  1606,  1635,
     204,   176,   206,    50,   129,   130,   181,   203,  1635,  1635,
     163,    30,  1635,    32,   200,    34,  1635,   590,   102,   103,
     412,    40,  1431,  1432,   696,   200,   300,   600,   700,   193,
     603,    50,   201,  1669,   203,   102,   103,    56,  1447,   292,
    1449,  1566,  1669,  1669,   297,   178,  1669,  1524,   301,   202,
    1669,   201,   148,   173,   786,   139,   778,   789,   180,   632,
     154,    80,   996,    62,    33,   797,   174,   163,   800,   163,
     165,   192,   197,   198,   179,  1666,   126,   964,   200,   163,
     179,   335,   185,   102,   103,    21,    22,   963,   180,  1680,
     763,    60,    61,   198,   197,   173,   180,  1506,  1507,   173,
     984,   173,   207,   198,  1513,   678,  1104,   201,  1111,   242,
     832,   206,  1215,   180,   198,   787,   139,   173,   163,  1503,
     156,   205,   537,   660,  1127,   161,   176,   163,  1512,   660,
     166,   181,   163,   200,   156,   415,   416,   417,   302,   161,
     154,   163,  1181,   163,   166,   165,   165,   173,   165,   163,
     200,   148,   173,   176,   127,   124,   165,   410,   412,   128,
    1483,   415,   416,   417,   127,   163,   163,   186,   126,   200,
     133,   129,   130,   199,  1558,  1559,   139,   173,   200,   198,
     201,   203,   118,   119,   204,   204,   200,   204,   186,  1440,
     126,   200,   128,   129,   130,   131,   132,  1586,   173,   591,
      33,   174,   335,   200,   177,   778,   175,  1339,   205,   127,
     179,   174,   181,   182,   606,   133,   496,   497,    21,    22,
     500,   173,   502,   126,   504,  1328,   201,    60,    61,   127,
     165,   165,   512,   955,  1623,   133,  1019,   176,   207,   197,
     198,   913,   496,   497,  1376,   839,   500,   199,   502,   127,
     504,   126,   506,   148,  1293,   133,   174,  1360,   512,  1362,
     173,   197,   198,   127,   163,   200,   546,   547,   163,   133,
     204,   944,   206,   176,   127,   529,   174,  1009,   181,   412,
     133,    33,   415,   416,   417,   912,   199,   959,   909,   422,
     423,   124,   546,   547,   127,   128,   174,   200,   925,   176,
     133,   176,   923,   180,   127,   173,   181,   127,    60,    61,
     174,   983,   173,   133,   173,   118,   119,   139,  1195,   165,
     177,   174,   181,   126,   181,   200,   129,   130,   131,   132,
     154,   177,   173,   201,   173,   164,   165,   591,    33,   163,
     201,   174,   175,   163,   173,   204,   179,   163,   177,   182,
     173,   174,   606,   176,   174,   200,   179,  1218,   204,   179,
     201,   107,   201,   496,   497,    60,    61,   500,   154,   502,
     762,   504,   124,   506,   207,   204,   128,   163,   631,   512,
      57,  1484,   154,   775,   776,   173,    63,    34,   642,   643,
     156,   163,  1269,   785,   197,   198,   529,   180,   173,   791,
     792,   139,   794,   657,   796,  1188,   798,    63,    64,    65,
     148,   199,  1006,   546,   547,   153,    63,   200,   698,   552,
       5,     6,   676,   175,  1463,   163,   201,   179,   173,   124,
     182,   173,   127,   128,   197,    33,   164,   165,   133,  1373,
     561,   173,   180,   173,   698,   173,   102,   103,   104,   105,
     163,    57,   154,   181,   173,   207,   201,    63,   591,   201,
     107,   163,    60,    61,   204,   205,   180,  1061,   180,   201,
     165,   201,  1255,   606,   164,   165,   204,  1580,  1581,   174,
     175,  1153,   201,   173,   179,   132,   200,   182,   200,   164,
     165,   181,   139,  1066,   774,   139,   629,   777,   173,   177,
     633,   781,   163,   181,   173,  1078,   181,   173,   762,   204,
    1083,   173,   207,   173,   204,   164,   163,   167,   168,   163,
     774,   775,   776,   777,   173,   163,   124,   781,   173,   204,
     128,   785,   201,  1636,   173,   201,   173,   791,   792,   201,
     794,   201,   796,   190,   798,   799,   164,   165,   180,   180,
     163,   310,  1655,   200,   173,   173,   201,   176,   163,    47,
     179,   320,   201,   181,   201,   698,   164,   165,   200,   200,
     962,   164,   165,    33,   333,   173,   968,   175,   139,    67,
     173,   179,   163,   181,   182,   177,   204,   148,   181,   181,
     982,   712,    12,   714,    21,    22,   163,    79,  1171,   154,
      60,    61,   163,    23,    24,  1178,   204,   167,   163,   207,
     107,   204,    94,  1005,   163,   164,  1189,    99,   165,   101,
     874,   140,    57,   142,   173,  1502,  1503,   907,    63,   762,
     910,  1508,    21,    22,   177,  1512,   916,  1514,   181,   163,
     164,   774,   775,   776,   777,   766,   177,   205,   781,   173,
     181,   173,   785,   907,   176,   909,   910,   179,   791,   792,
     107,   794,   916,   796,   124,   798,   799,    57,   128,   923,
      33,   164,   165,    63,  1551,    10,    11,   164,   165,    57,
     173,  1558,  1559,   203,  1545,    63,   173,    57,   181,  1566,
      57,   118,   119,    63,   181,    57,    63,    60,    61,   126,
     174,   128,   129,   130,   131,   132,   190,   176,   962,   963,
    1247,   204,   481,   482,   968,   175,  1247,   204,   167,   179,
     841,   181,   182,   177,   140,   141,   142,   181,   982,   118,
     119,   167,   168,   169,   177,   164,   165,   126,   181,   128,
     129,   130,   131,   132,   173,   177,   177,   207,  1625,   181,
     181,  1005,   181,   140,   513,   182,   183,   184,   185,   186,
     165,   124,   203,   164,   165,   128,   525,  1340,   163,   177,
     197,   198,   173,   181,   907,   204,   909,   910,  1031,  1511,
     181,   177,    66,   916,  1645,   181,   167,   168,   169,   170,
     923,   163,    21,    22,   176,   184,   185,   186,   202,   203,
     164,   205,   923,   204,   925,   106,   565,   163,   197,   198,
     203,  1672,   175,   198,   180,   936,   179,   200,   181,   182,
     180,  1553,    75,   944,   200,   946,    79,   180,   180,   962,
     963,   180,   180,   592,   593,   968,   180,   596,   180,   598,
      93,    94,  1122,   180,   207,    98,    99,   100,   101,   982,
     609,   610,   611,   612,   613,   614,   180,   200,   479,  1133,
     481,   482,   177,    35,  1256,   164,  1536,   205,  1122,   205,
      35,  1444,  1005,   167,   168,   169,   747,   748,   749,    54,
      55,    56,   200,   163,   163,   180,   198,   116,   117,   118,
     119,   120,   205,   163,   123,   199,   655,   126,    22,   128,
     129,   130,   131,   132,   163,   134,   135,  1028,   199,   205,
    1583,   176,   163,   180,   180,  1036,  1037,  1170,   677,   180,
     180,   203,   200,   180,   200,   180,   200,   200,  1049,   200,
    1051,  1052,  1053,  1054,  1055,    21,    22,   180,  1059,   180,
      13,   181,   200,   163,   200,   704,  1619,   201,   200,   198,
    1203,   180,   181,   182,   183,   184,   185,   186,   200,   173,
    1533,   200,   200,    43,   201,   201,   201,   199,   197,   198,
     200,    33,    21,    22,   199,   201,   204,   200,  1258,   181,
     180,   205,   200,   180,   743,   180,   180,   200,    13,  1122,
     199,   173,   200,     4,   201,   440,   441,   200,    60,    61,
     759,  1438,  1256,   163,  1258,   203,   765,   163,   176,   199,
     163,   163,   457,   458,   459,   460,   461,  1138,   163,   163,
     163,   780,   173,   201,   201,   201,     1,   201,   201,   200,
     116,   117,   118,   119,   120,   201,   200,   123,   124,   125,
     126,   200,   128,   129,   130,   131,   132,   201,   134,   135,
     180,   201,   811,   201,   140,   141,   142,   816,   200,   818,
     201,   820,   124,   201,   200,   180,   128,   116,   117,   118,
     119,   200,   831,   200,   200,   200,   199,   126,   200,   128,
     129,   130,   131,   132,  1364,   134,   135,   200,   199,   175,
     181,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   201,   181,   181,   200,   174,   201,   200,    43,   174,
    1364,   197,   198,   175,   201,    43,   201,   179,   201,   181,
     182,   176,    43,  1256,   201,  1258,   201,  1407,   887,   200,
     163,   201,   891,   182,   183,   184,   185,   186,   201,   206,
     201,   200,   200,  1423,   201,   207,    33,  1400,   197,   198,
     200,    33,   156,  1407,   201,   163,   163,    10,   917,    13,
       9,    33,    42,    66,   180,   200,   200,   199,   199,  1423,
     200,   163,   163,    60,    61,   163,   163,   206,    60,    61,
     206,  1302,   206,  1304,   163,     8,   945,   163,    60,    61,
    1582,   201,   163,   171,   200,   163,   201,   163,   201,   644,
     645,   201,   163,   648,   649,   650,   651,    33,   653,   163,
     969,   656,   657,   658,   659,   660,   661,   662,   663,   664,
     665,   666,   667,   668,   669,   670,   671,   672,   673,   674,
     675,  1364,   163,   200,    60,    61,    14,   124,   174,   176,
     174,   128,   124,   156,  1003,  1004,   128,   200,   200,  1370,
     200,   200,   124,   200,   174,  1014,   128,    33,   201,   201,
     201,   201,  1021,   201,    43,  1024,    37,   206,   174,    67,
     181,  1030,   201,  1032,  1407,   200,   206,   200,   200,  1532,
      70,  1534,  1535,   200,    60,    61,   200,   163,   175,   201,
    1423,   200,   179,   175,   181,   182,   200,   179,   124,   181,
     182,    43,   128,   175,  1063,   201,  1065,   179,  1067,   181,
     182,   181,   163,   138,   204,   201,   201,   201,  1077,   163,
     207,   167,   201,   200,   163,   207,  1085,   200,  1582,   200,
     204,  1464,   200,   200,    33,   207,   108,   109,   110,   111,
     112,   113,   114,   115,   201,   201,   201,   206,   124,   175,
    1109,   201,   128,   179,   201,   181,   182,   205,   201,   201,
    1119,   133,  1483,  1616,   201,  1124,   201,  1126,   201,    12,
     204,   143,   144,   145,   173,  1496,   173,   201,  1137,   201,
     201,   207,   201,   201,   201,    21,    22,   201,  1641,   204,
     200,  1524,   201,  1646,   201,   201,   841,  1156,    53,   175,
     200,   206,   205,   179,   199,   181,   182,   199,   201,   615,
     206,    79,   206,     1,   205,  1174,    44,  1670,   206,   128,
      82,  1679,   714,  1182,  1183,  1184,   714,    33,  1677,   199,
     101,   207,   633,   637,   940,   698,     1,  1358,  1498,  1549,
    1501,  1426,    53,    33,  1550,  1204,  1550,  1127,  1207,  1582,
     939,   217,   422,   939,    60,    61,  1577,  1216,   294,   422,
     422,   592,   422,   422,  1585,   422,    -1,    -1,    -1,    -1,
      60,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,   936,   128,   129,   130,   131,   132,    -1,   134,   135,
      -1,  1622,    -1,    -1,   140,  1264,   142,  1266,    -1,    -1,
      -1,    -1,    -1,  1272,    -1,    -1,    -1,  1638,   124,    -1,
      -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1651,    -1,  1653,    -1,   124,    -1,  1657,    -1,   128,    -1,
      -1,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,  1310,  1673,    -1,  1675,    -1,    -1,    -1,    -1,    -1,
      -1,   197,   198,    -1,    -1,    -1,    -1,    -1,  1327,   175,
      -1,    -1,    -1,   179,    -1,   181,   182,    -1,    -1,    -1,
      -1,    -1,  1341,  1342,    -1,   175,    -1,    -1,    -1,   179,
      -1,   181,   182,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   207,    -1,    -1,    -1,    -1,  1365,    -1,    -1,    -1,
      -1,    -1,  1371,  1372,    -1,  1374,  1375,   207,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1073,    -1,
      -1,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,  1415,    -1,    -1,    -1,
    1419,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    64,    -1,    -1,    -1,
      -1,  1440,    -1,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    -1,    -1,    -1,    -1,
      71,    -1,    73,    -1,    75,    76,    77,    78,    79,    -1,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,  1505,    98,    99,   100,
     101,   139,    -1,    -1,    -1,    -1,    -1,    -1,  1517,    -1,
      -1,    -1,  1521,  1522,   152,    -1,  1525,    -1,    -1,   116,
     117,   118,   119,   120,    -1,   163,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,  1237,  1238,  1239,  1240,  1241,  1242,  1243,  1244,
    1245,  1246,  1247,  1248,  1249,  1250,  1251,  1252,  1253,  1254,
    1569,    -1,   163,    -1,   202,    -1,    -1,    -1,    -1,   207,
      -1,   108,   109,   110,   111,   112,   113,   114,   115,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     127,    -1,    -1,    -1,    -1,    -1,   133,    -1,    -1,    -1,
     197,   198,    -1,    -1,    -1,    -1,   143,   144,   145,    -1,
      -1,  1620,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1629,    -1,    -1,    -1,  1319,  1320,  1321,    -1,  1637,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,    -1,     1,
      -1,    -1,    -1,     5,     6,     7,    -1,     9,    10,    11,
    1659,    13,    -1,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,    31,
      -1,    -1,    -1,    -1,    -1,  1370,    38,    39,    40,    -1,
      42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,    51,
      52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1470,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,
      -1,   153,    -1,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,   165,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     182,   183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,
      -1,    -1,    -1,  1528,    -1,    -1,   198,    -1,   200,    -1,
     202,   203,   204,   205,   206,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,   138,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,  1576,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1585,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   197,   198,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1635,    -1,    -1,    60,    61,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,     5,     6,     7,    -1,     9,
      10,    11,  1657,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,  1669,    25,    26,    27,    28,    29,
    1675,    31,    -1,    -1,    -1,    -1,    -1,    -1,    38,    39,
      40,    -1,    42,    -1,    44,    45,    -1,    -1,    48,    -1,
      50,    51,    52,    -1,    54,    55,    -1,   124,    58,    59,
      -1,   128,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,   105,    -1,    -1,   175,    -1,
      -1,    -1,   179,    -1,   181,   182,    -1,    -1,   118,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,
     207,    -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,
     150,   151,    -1,   153,    -1,   155,   156,   157,   158,   159,
     160,   161,   162,   163,    -1,   165,   166,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   182,   183,   184,    -1,   186,    -1,    -1,   189,
     190,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,    -1,
     200,    -1,   202,   203,   204,   205,   206,     1,    -1,    -1,
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
     104,   105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,
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
     206,     5,     6,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,   124,    -1,    -1,    -1,   128,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    70,    71,    72,    73,
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
      -1,    -1,     5,     6,   198,    -1,   200,   201,   202,   203,
      13,   205,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    49,    -1,    51,    -1,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,
     153,   154,   155,   156,   157,   158,   159,   160,   161,   162,
     163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,
     183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,
      -1,    -1,    -1,     5,     6,   198,    -1,   200,    -1,   202,
     203,    13,   205,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    33,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    49,    -1,    51,
      -1,    -1,    -1,    55,    -1,    60,    61,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,   124,
      -1,    -1,    -1,   128,    -1,   127,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,
      -1,   153,    -1,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
     175,    -1,    -1,    -1,   179,    -1,   181,   182,    -1,    -1,
     182,   183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,
      -1,    -1,    -1,    -1,     5,     6,   198,    -1,   200,    -1,
     202,   203,   207,   205,    15,    16,    17,    18,    19,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,   127,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,
     151,    -1,   153,   154,   155,   156,   157,   158,   159,   160,
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
     150,   151,    -1,   153,   154,   155,   156,   157,   158,   159,
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
      -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,   153,
      -1,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,
     184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,
      -1,    -1,     5,     6,   198,    -1,   200,    -1,   202,   203,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,
      -1,   153,    -1,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     182,   183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,
      -1,    -1,    -1,    -1,     5,     6,   198,   199,   200,    -1,
     202,   203,    -1,   205,    15,    16,    17,    18,    19,    -1,
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
     203,    15,    16,    17,    18,    19,    -1,    -1,    22,    -1,
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
       5,     6,    -1,    -1,   198,    -1,   200,    -1,   202,   203,
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
      -1,    -1,    -1,    10,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   118,   119,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   147,   148,   149,   150,   151,    -1,   153,    -1,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,
     186,    -1,    -1,   189,   190,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   198,    -1,   200,    -1,   202,   203,    -1,    -1,
      -1,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,    -1,   140,   141,   142,   143,   144,   145,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   174,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,
     197,   198,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    -1,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,
     139,    -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   152,    -1,    -1,    -1,    -1,    -1,    -1,
      64,    -1,    -1,    -1,   163,    -1,   165,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
      -1,    -1,    -1,   202,    -1,   204,    -1,    -1,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    -1,    -1,    -1,    -1,   139,    -1,    -1,    49,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   152,    -1,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,   163,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,    -1,    -1,    -1,    -1,    19,   202,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    38,    -1,    -1,    -1,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   152,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,
      72,    73,   163,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
      -1,    -1,   128,   129,   130,    -1,    -1,   133,   134,   135,
     136,   137,    -1,    -1,   140,   141,   142,   143,   144,   145,
     146,    -1,   154,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   163,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,   175,
      -1,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,    -1,    -1,   189,   190,    -1,   116,   117,   118,   119,
     120,   197,   198,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,   132,    -1,   134,   135,    21,    22,   138,    -1,
     140,   141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,   175,    -1,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,    -1,    -1,    -1,
      -1,    -1,   116,   117,   118,   119,   120,   197,   198,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,   132,    -1,
     134,   135,    21,    22,   138,    -1,   140,   141,   142,    -1,
      -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,   175,    -1,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,   197,   198,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,   132,    -1,   134,   135,    21,    22,
      -1,    -1,   140,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,   116,   117,   118,
     119,   120,   197,   198,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   197,
     198,    19,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,   197,   198,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    -1,    -1,    -1,    -1,    -1,    -1,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   197,   198,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   154,    35,    -1,    -1,
      -1,    -1,    71,    72,    73,   163,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,    71,    -1,    73,    -1,    75,    76,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,    -1,    -1,   154,    -1,    -1,    -1,    -1,
      -1,    71,    72,    73,   163,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,    -1,    -1,    71,   163,    73,    -1,    75,    76,
      77,    78,    79,    -1,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,   129,
     130,    98,    99,   100,   101,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   154,    -1,    -1,    -1,    -1,    -1,
      71,    -1,    73,   163,    75,    76,    77,    78,    79,    -1,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,    -1,    -1,    -1,    -1,    -1,   163,   197,   198,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   163
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   209,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   102,   103,   165,   186,   198,   204,   211,   212,   216,
     225,   227,   228,   232,   280,   286,   317,   399,   407,   414,
     424,   474,   479,   484,    19,    20,   163,   271,   272,   273,
     156,   233,   234,   163,   186,   229,   230,    57,    63,   404,
     405,   163,   202,   214,   485,   475,   480,   139,   163,   305,
      34,    63,   107,   132,   190,   200,   275,   276,   277,   278,
     305,   211,   211,   211,     8,    36,   425,    62,   395,   174,
     173,   176,   173,   229,    22,    57,   185,   197,   231,   163,
     211,   395,   404,   404,   404,   163,   139,   226,   277,   277,
     277,   200,   140,   141,   142,   173,   199,   107,   285,   415,
       5,     6,   421,    57,    63,   396,    15,    16,   156,   161,
     163,   166,   200,   203,   218,   272,   156,   234,   163,   163,
     163,   406,    57,    63,   213,   163,   163,   163,   163,   167,
     224,   201,   273,   277,   277,   277,   277,   165,   238,   239,
      57,    63,   287,   289,    57,    63,   408,   107,   107,    57,
      63,   422,   205,   400,   167,   168,   169,   217,    15,    16,
     156,   161,   163,   218,   269,   270,   203,   231,   174,   190,
     215,   176,   435,   239,   239,   167,   201,   165,   290,   163,
     409,   426,   397,   203,   274,   365,   167,   168,   169,   173,
     201,   163,    19,    25,    31,    41,    49,    64,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   152,   202,   305,   429,   431,   432,   436,   442,   444,
     473,    66,    79,    94,    99,   101,   164,   412,   413,   476,
     481,    35,    71,    73,    75,    76,    77,    78,    79,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      93,    94,    95,    98,    99,   100,   101,   118,   119,   163,
     283,   284,   288,   176,   410,   106,   419,   420,   206,   211,
     398,   272,   203,   163,   391,   394,   269,   180,   180,   180,
     200,   180,   180,   200,   435,   180,   180,   180,   180,   180,
     200,   305,   180,   200,    33,    60,    61,   124,   128,   175,
     179,   182,   207,   198,   441,   177,   164,   486,   205,   205,
      21,    22,    38,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   128,   129,   130,   133,   134,   135,   136,   137,   140,
     141,   142,   143,   144,   145,   146,   175,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   189,   190,   197,
     198,    35,    35,   200,   281,   239,    75,    79,    93,    94,
      98,    99,   100,   101,   430,   413,   163,   239,   365,   239,
     272,   173,   176,   179,   389,   445,   451,   453,     5,     6,
      15,    16,    17,    18,    19,    25,    27,    31,    39,    45,
      48,    51,    55,    65,    68,    69,    80,   102,   103,   104,
     118,   119,   147,   148,   149,   150,   151,   153,   155,   156,
     157,   158,   159,   160,   161,   162,   166,   182,   183,   184,
     189,   190,   198,   200,   202,   203,   205,   223,   225,   297,
     305,   310,   322,   329,   332,   335,   339,   341,   343,   344,
     346,   351,   354,   355,   356,   363,   364,   429,   490,   498,
     509,   512,   524,   526,   529,   530,   455,   449,   163,   180,
     457,   459,   461,   463,   465,   467,   469,   471,   355,   180,
     200,   443,   447,   127,   302,   333,   355,    33,   179,    33,
     179,   198,   207,   199,   355,   198,   207,   442,   205,   477,
     482,   163,   284,   163,   284,   163,   199,    22,   163,   199,
     151,   201,   365,   375,   376,   377,   126,   176,   282,   294,
     295,   205,   176,   418,   427,   148,   163,   390,   393,   239,
     163,   442,   127,   133,   174,   388,   473,   473,   440,   473,
     180,   180,   180,   305,   307,   431,   489,   498,   509,   512,
     524,   526,   529,   530,   305,   180,     5,   102,   103,   180,
     200,   180,   200,   200,   180,   180,   200,   180,   200,   180,
     200,   180,   180,   200,    19,   180,   180,   356,   356,   200,
     200,   200,   200,   200,   200,   222,   356,   356,   356,   356,
     356,    13,    49,   302,   154,   163,   333,   491,   493,   203,
      13,   525,   200,   198,   279,   203,   205,   335,   340,   340,
     340,   201,    21,    22,   116,   117,   118,   119,   120,   123,
     124,   125,   126,   128,   129,   130,   131,   132,   134,   135,
     138,   140,   141,   142,   146,   175,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   197,   198,   200,   473,
     473,   201,   181,   437,   473,   281,   473,   281,   473,   281,
     163,   378,   379,   473,   163,   381,   382,   201,   448,   333,
     304,   473,   355,   201,   173,   528,   199,   199,   199,   355,
     487,   378,   380,   381,   383,   163,   284,   108,   109,   110,
     111,   112,   113,   114,   115,   133,   143,   144,   145,   108,
     109,   110,   111,   112,   113,   114,   115,   127,   133,   143,
     144,   145,   174,   200,     7,    50,   316,   204,   173,   204,
     201,   473,   473,   205,   416,   305,   204,   205,   423,   200,
      43,   173,   176,   389,   211,   388,   355,   181,   181,   181,
     164,   173,   210,   211,   439,   499,   501,   308,   200,   180,
     200,   330,   180,   180,   180,   519,   333,   442,   355,   523,
     355,   323,   325,   355,   327,   355,   521,   333,   507,   510,
     333,   180,   503,   442,   355,   355,   355,   355,   355,   355,
     169,   170,   217,   200,    13,   199,   200,   127,   133,   174,
     384,   528,   173,   528,   201,   148,   153,   180,   305,   345,
     200,   239,    70,   198,   201,   333,   493,   278,     4,   338,
     203,   301,   279,    19,   154,   163,   429,    19,   154,   163,
     429,   356,   356,   356,   356,   356,   356,   163,   356,   154,
     163,   355,   356,   356,   429,   356,   356,   356,   524,   530,
     356,   356,   356,   356,    22,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   129,   130,   154,   163,
     197,   198,   352,   429,   355,   201,   333,   181,   181,   163,
     433,   181,   282,   181,   282,   181,   282,   176,   181,   439,
     176,   181,   439,   304,   528,   181,   439,   127,   355,   199,
     163,   434,   211,   246,   247,   246,   247,   355,   148,   163,
     385,   386,   428,   377,   377,   377,   301,   163,   401,   403,
     371,   355,   163,   163,   442,   388,   355,   211,   446,   452,
     454,   473,   442,   442,   473,    70,   333,   493,   497,   163,
     355,   473,   513,   515,   517,   442,   528,   181,   439,   173,
     528,   201,   442,   442,   201,   442,   201,   442,   528,   442,
     379,   528,   505,   382,   181,   201,   201,   201,   201,   201,
     201,   355,   148,   163,   200,   260,   200,   355,   355,   355,
     201,   154,   163,   200,   200,   347,   349,   260,   303,   523,
     163,   201,   493,   491,   173,   201,   201,   199,   200,   281,
       1,    26,    28,    29,    38,    40,    44,    52,    54,    58,
      59,    65,   105,   205,   206,   211,   235,   236,   245,   256,
     257,   259,   261,   262,   263,   264,   265,   266,   267,   268,
     298,   306,   311,   312,   313,   314,   315,   317,   321,   342,
     356,   338,   180,   200,   180,   200,   200,   200,   199,    19,
     154,   163,   429,   176,   154,   163,   355,   200,   200,   154,
     163,   355,     1,   200,   199,   173,   201,   456,   450,   173,
     181,   204,   458,   181,   462,   181,   466,   181,   473,   470,
     378,   473,   472,   381,   181,   201,   443,   473,   355,   174,
     210,   402,   411,   211,   378,   478,   381,   483,   201,   200,
      43,   173,   176,   179,   384,   296,   174,   402,   411,    40,
     165,   206,   280,   372,   201,    43,   211,   388,   355,   211,
     181,   181,   181,   493,   201,   201,   201,   181,   439,   201,
     181,   442,   379,   382,   181,   201,   200,   442,   355,   201,
     181,   181,   181,   181,   201,   181,   181,   201,   442,   181,
     338,   200,   176,   220,   200,    43,   163,   319,    20,   173,
     260,   201,   200,   133,   384,   355,   355,   442,   281,    20,
     206,   528,   201,   173,   199,   198,   127,   133,   163,   174,
     179,   336,   337,   282,   127,   355,   294,    61,   355,   163,
     163,   211,   156,    58,   355,   239,   127,   355,   299,   211,
     211,    10,    10,    11,   243,    13,     9,    42,   211,   211,
     211,   211,   211,   211,    66,   318,   211,   108,   109,   110,
     111,   112,   113,   114,   115,   121,   122,   127,   133,   136,
     137,   143,   144,   145,   174,   281,   357,   355,   359,   355,
     201,   333,   355,   180,   200,   356,   200,   199,   355,   198,
     201,   333,   200,   199,   353,   201,   333,   163,   438,   163,
     460,   464,   468,   443,   355,   163,   210,   488,   206,   206,
     355,   163,   163,   473,   355,   206,   355,   401,   417,   163,
       8,   365,   370,   163,   355,   211,   500,   502,   309,   201,
     200,   163,   331,   181,   181,   181,   520,   303,   181,   324,
     326,   328,   522,   508,   511,   181,   504,   200,   239,   201,
     333,   221,   171,   355,   163,   173,   201,   333,   163,   200,
      20,   133,   384,   355,   355,   355,   201,   201,   181,   282,
     333,   201,   491,   163,   163,   200,   163,   163,   173,   201,
     239,   355,    14,   355,   174,   174,   176,   156,   294,   355,
     301,   200,   200,   200,   200,   200,   200,   205,   320,   393,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   524,   530,   356,   356,   356,   356,   356,   356,   356,
     282,   442,   201,   473,   201,   201,   201,   361,   355,   355,
     201,   491,   201,   355,   201,   174,   206,   201,    43,   384,
      37,   291,   206,   174,    57,    63,   368,    67,   369,   211,
     211,   200,   200,   355,   181,   514,   516,   518,   200,   201,
     200,   356,   356,   356,   200,    70,   497,   200,   506,   200,
     201,   355,   294,   201,   219,   201,   163,   201,    43,   319,
     333,   355,   355,   201,   348,   181,   201,   199,   163,   336,
     138,   294,   334,   294,   473,   355,   300,   355,   355,   260,
     355,   355,   319,   392,   239,   181,   181,   473,   201,   201,
     199,   201,   355,   163,   355,   292,   473,    47,   369,    46,
     106,   366,   497,   497,   201,   200,   200,   200,   200,   302,
     303,   333,   497,   200,   497,   201,   167,   204,   163,   201,
     201,   133,   384,   345,   350,   204,   201,   201,   127,   356,
     206,   201,   201,    20,   201,   201,   201,   206,   211,   393,
     294,   358,   360,   181,   201,   205,   211,    33,   367,   366,
     368,   200,   491,   494,   495,   496,   496,   355,   497,   497,
     491,   492,   201,   201,   528,   496,   497,   492,   355,   204,
     355,   355,   345,   355,   356,   291,    12,   244,   239,   333,
     239,   239,   176,   389,   362,   301,   373,   367,   385,   386,
     387,   491,   173,   528,   201,   201,   201,   496,   496,   201,
     201,   201,   492,   201,   204,   527,   355,   527,   245,   311,
     312,   313,   314,   356,   211,   258,   201,   294,   294,   442,
     388,   293,   288,   374,   201,   200,   201,   201,   201,    53,
     199,   527,   206,   205,   248,   251,   239,   388,   355,   206,
     211,   288,   491,   355,   199,   249,    12,    23,    24,   237,
     240,   245,   294,   355,   211,   239,   201,   301,   239,   200,
     211,   211,   294,   250,   241,   355,   206,   205,   252,   255,
     201,   291,   253,   245,   239,   301,   211,   242,   254,   252,
     206,   240,   291
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   208,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   210,   210,   211,
     211,   212,   213,   213,   213,   214,   214,   215,   215,   216,
     217,   217,   217,   217,   218,   218,   219,   219,   220,   221,
     220,   222,   222,   222,   223,   224,   224,   226,   225,   227,
     228,   229,   229,   229,   229,   230,   230,   231,   231,   232,
     233,   233,   234,   234,   235,   236,   236,   237,   237,   238,
     238,   239,   239,   240,   241,   240,   242,   240,   243,   243,
     244,   244,   245,   245,   245,   245,   245,   246,   246,   247,
     247,   249,   250,   248,   251,   248,   253,   254,   252,   255,
     252,   257,   258,   256,   259,   260,   260,   260,   260,   260,
     260,   260,   262,   261,   263,   265,   264,   267,   266,   268,
     268,   269,   269,   269,   269,   269,   269,   270,   270,   271,
     271,   271,   272,   272,   272,   272,   272,   272,   272,   272,
     272,   273,   273,   274,   274,   275,   275,   275,   275,   276,
     276,   277,   277,   277,   277,   277,   277,   277,   278,   278,
     279,   279,   280,   280,   281,   281,   281,   282,   282,   282,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   285,   285,   286,   287,   287,   287,
     288,   290,   289,   291,   292,   293,   291,   295,   296,   294,
     297,   297,   297,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   299,   300,   298,   301,   301,   301,   302,   302,
     303,   303,   304,   304,   305,   305,   305,   306,   306,   308,
     309,   307,   307,   310,   310,   310,   310,   310,   310,   311,
     312,   313,   313,   313,   314,   314,   315,   316,   316,   316,
     317,   317,   318,   318,   319,   319,   320,   320,   321,   321,
     321,   323,   324,   322,   325,   326,   322,   327,   328,   322,
     330,   331,   329,   332,   332,   332,   333,   333,   333,   333,
     334,   334,   334,   335,   335,   335,   336,   336,   336,   336,
     336,   337,   337,   338,   338,   339,   340,   340,   341,   341,
     341,   341,   341,   341,   341,   342,   342,   342,   342,   342,
     342,   342,   342,   342,   342,   342,   342,   342,   342,   342,
     342,   342,   342,   342,   342,   342,   343,   343,   344,   344,
     345,   345,   346,   347,   348,   346,   349,   350,   346,   351,
     351,   351,   351,   351,   351,   351,   352,   353,   351,   354,
     354,   354,   354,   354,   354,   354,   355,   355,   355,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   357,   358,   356,   356,   356,
     356,   359,   360,   356,   356,   356,   361,   362,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   356,   356,   363,   363,   363,   364,   364,
     364,   364,   364,   364,   364,   364,   364,   364,   364,   364,
     364,   364,   364,   364,   365,   365,   366,   366,   366,   367,
     367,   368,   368,   368,   369,   369,   370,   371,   371,   371,
     372,   371,   373,   371,   374,   371,   375,   376,   376,   377,
     377,   377,   377,   377,   378,   378,   379,   379,   380,   380,
     380,   381,   382,   382,   383,   383,   383,   384,   384,   385,
     385,   385,   386,   386,   387,   387,   388,   388,   388,   389,
     389,   390,   390,   390,   390,   390,   391,   391,   392,   392,
     392,   393,   393,   393,   394,   394,   394,   395,   395,   396,
     396,   396,   397,   397,   398,   397,   399,   400,   399,   401,
     401,   402,   402,   403,   403,   403,   404,   404,   404,   406,
     405,   407,   408,   408,   408,   409,   410,   410,   411,   411,
     412,   412,   413,   413,   415,   416,   417,   414,   418,   418,
     419,   419,   420,   421,   421,   421,   421,   422,   422,   422,
     423,   423,   425,   426,   427,   424,   428,   428,   428,   428,
     428,   429,   429,   429,   429,   429,   429,   429,   429,   429,
     429,   429,   429,   429,   429,   429,   429,   429,   429,   429,
     429,   429,   429,   429,   429,   429,   429,   429,   430,   430,
     430,   430,   430,   430,   430,   430,   431,   432,   432,   432,
     433,   433,   433,   434,   434,   434,   434,   434,   435,   435,
     435,   435,   435,   436,   437,   438,   436,   439,   439,   440,
     440,   441,   441,   441,   441,   442,   442,   443,   443,   444,
     444,   444,   444,   445,   446,   444,   444,   444,   444,   447,
     444,   448,   444,   444,   444,   444,   444,   444,   444,   444,
     444,   444,   444,   444,   444,   449,   450,   444,   444,   451,
     452,   444,   453,   454,   444,   455,   456,   444,   444,   457,
     458,   444,   459,   460,   444,   444,   461,   462,   444,   463,
     464,   444,   444,   465,   466,   444,   467,   468,   444,   469,
     470,   444,   471,   472,   444,   473,   473,   473,   475,   476,
     477,   478,   474,   480,   481,   482,   483,   479,   485,   486,
     487,   488,   484,   489,   489,   489,   489,   489,   489,   489,
     490,   490,   490,   490,   490,   491,   491,   491,   491,   491,
     491,   491,   491,   492,   492,   493,   494,   494,   495,   495,
     496,   496,   497,   497,   499,   500,   498,   501,   502,   498,
     503,   504,   498,   505,   506,   498,   507,   508,   498,   509,
     510,   511,   509,   512,   513,   514,   512,   515,   516,   512,
     517,   518,   512,   512,   519,   520,   512,   512,   521,   522,
     512,   523,   523,   525,   524,   526,   526,   526,   527,   527,
     528,   528,   529,   529,   530
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     3,
       3,     2,     2,     2,     2,     2,     2,     1,     1,     1,
       1,     2,     0,     1,     1,     1,     1,     0,     2,     5,
       1,     1,     2,     2,     3,     2,     0,     2,     0,     0,
       3,     0,     2,     5,     3,     1,     2,     0,     4,     2,
       2,     1,     2,     3,     3,     2,     4,     0,     1,     2,
       1,     3,     1,     3,     3,     3,     2,     1,     1,     1,
       2,     0,     1,     0,     0,     4,     0,     8,     1,     1,
       0,     2,     1,     1,     1,     1,     1,     1,     2,     0,
       1,     0,     0,     6,     0,     3,     0,     0,     6,     0,
       3,     0,     0,     9,     7,     1,     4,     3,     3,     3,
       5,     5,     0,     9,     3,     0,     7,     0,     7,     4,
       4,     1,     1,     1,     1,     1,     1,     1,     3,     1,
       1,     1,     3,     3,     5,     3,     3,     3,     3,     1,
       5,     1,     3,     3,     4,     1,     1,     1,     1,     1,
       4,     1,     2,     3,     3,     3,     3,     2,     1,     3,
       0,     3,     0,     4,     0,     2,     3,     0,     2,     2,
       1,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     3,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     3,
       2,     2,     3,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     3,     2,     2,     2,     2,
       2,     3,     3,     3,     3,     3,     4,     4,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     1,     4,     0,     1,     1,
       3,     0,     5,     0,     0,     0,     6,     0,     0,     6,
       2,     2,     2,     1,     2,     2,     1,     1,     1,     1,
       2,     1,     2,     2,     2,     2,     1,     1,     1,     2,
       2,     2,     0,     0,     6,     0,     2,     2,     0,     2,
       0,     2,     1,     3,     1,     3,     2,     2,     3,     0,
       0,     5,     1,     2,     5,     5,     5,     6,     2,     1,
       1,     1,     2,     3,     2,     3,     4,     1,     1,     0,
       1,     1,     1,     0,     1,     3,     8,     7,     3,     3,
       5,     0,     0,     7,     0,     0,     7,     0,     0,     7,
       0,     0,     6,     5,     8,    10,     1,     2,     3,     4,
       1,     2,     3,     1,     1,     2,     2,     2,     2,     2,
       4,     1,     3,     0,     4,     7,     7,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     6,     8,     5,     6,
       1,     4,     3,     0,     0,     8,     0,     0,     9,     3,
       4,     5,     6,     8,     5,     6,     0,     0,     5,     3,
       4,     4,     5,     4,     3,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     2,     2,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     2,     2,     2,     4,
       3,     4,     5,     4,     5,     3,     4,     1,     1,     2,
       4,     4,     1,     3,     5,     0,     0,     8,     3,     3,
       3,     0,     0,     8,     3,     4,     0,     0,     9,     4,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     3,
       1,     4,     3,     3,     3,     7,     8,     7,     4,     4,
       4,     4,     4,     1,     6,     7,     6,     6,     7,     7,
       6,     7,     6,     6,     0,     1,     0,     1,     1,     0,
       1,     0,     1,     1,     0,     1,     5,     0,     2,     6,
       0,     4,     0,     9,     0,    11,     3,     3,     4,     1,
       1,     3,     3,     3,     1,     3,     1,     3,     0,     1,
       3,     3,     1,     3,     0,     1,     3,     1,     1,     1,
       2,     3,     3,     5,     1,     1,     1,     1,     1,     0,
       1,     1,     4,     3,     3,     5,     1,     3,     0,     2,
       2,     4,     6,     5,     4,     6,     5,     0,     1,     0,
       1,     1,     0,     2,     0,     4,     6,     0,     6,     1,
       3,     1,     2,     0,     1,     3,     0,     1,     1,     0,
       5,     3,     0,     1,     1,     1,     0,     2,     0,     1,
       1,     2,     0,     1,     0,     0,     0,    13,     0,     2,
       0,     1,     3,     1,     1,     2,     2,     0,     1,     1,
       1,     3,     0,     0,     0,     9,     1,     4,     3,     3,
       5,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     4,
       1,     3,     3,     0,     1,     3,     3,     5,     0,     2,
       2,     2,     2,     4,     0,     0,     7,     1,     1,     1,
       3,     3,     2,     4,     3,     1,     2,     0,     4,     1,
       1,     1,     1,     0,     0,     6,     4,     4,     3,     0,
       6,     0,     7,     4,     2,     2,     3,     2,     3,     2,
       2,     3,     3,     3,     2,     0,     0,     6,     2,     0,
       0,     6,     0,     0,     6,     0,     0,     6,     1,     0,
       0,     6,     0,     0,     7,     1,     0,     0,     6,     0,
       0,     7,     1,     0,     0,     6,     0,     0,     7,     0,
       0,     6,     0,     0,     6,     1,     3,     3,     0,     0,
       0,     0,    12,     0,     0,     0,     0,    12,     0,     0,
       0,     0,    13,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     5,     5,     6,
       6,     8,     8,     0,     1,     2,     3,     5,     1,     2,
       1,     0,     0,     1,     0,     0,    10,     0,     0,    10,
       0,     0,    10,     0,     0,    11,     0,     0,     7,     5,
       0,     0,    10,     3,     0,     0,    11,     0,     0,    11,
       0,     0,    10,     5,     0,     0,     9,     5,     0,     0,
      10,     1,     3,     0,     5,     5,     7,     9,     0,     3,
       0,     1,    11,    12,    11
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
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_string_builder: /* string_builder  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_reader: /* expr_reader  */
            { delete ((*yyvaluep).pExpression); }
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

    case YYSYMBOL_expression_if_block: /* expression_if_block  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_else_block: /* expression_else_block  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_if_then_else: /* expression_if_then_else  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_if_then_else_oneliner: /* expression_if_then_else_oneliner  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_for_variable_name_with_pos_list: /* for_variable_name_with_pos_list  */
            { delete ((*yyvaluep).pNameWithPosList); }
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

    case YYSYMBOL_optional_annotation_list_with_emit_semis: /* optional_annotation_list_with_emit_semis  */
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
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_block: /* expression_block  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_call_pipe_no_bracket: /* expr_call_pipe_no_bracket  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expression_any: /* expression_any  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expressions: /* expressions  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_optional_expr_list: /* optional_expr_list  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_optional_expr_map_tuple_list: /* optional_expr_map_tuple_list  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_type_declaration_no_options_list: /* type_declaration_no_options_list  */
            { deleteTypeDeclarationList(((*yyvaluep).pTypeDeclList)); }
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

    case YYSYMBOL_expression_return: /* expression_return  */
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

    case YYSYMBOL_expr_full_block: /* expr_full_block  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_full_block_assumed_piped: /* expr_full_block_assumed_piped  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_numeric_const: /* expr_numeric_const  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_assign_no_bracket: /* expr_assign_no_bracket  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_named_call: /* expr_named_call  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_method_call_no_bracket: /* expr_method_call_no_bracket  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_func_addr_name: /* func_addr_name  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_func_addr_expr: /* func_addr_expr  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_field_no_bracket: /* expr_field_no_bracket  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_call: /* expr_call  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr: /* expr  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_no_bracket: /* expr_no_bracket  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_generator: /* expr_generator  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_mtag_no_bracket: /* expr_mtag_no_bracket  */
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
            { delete ((*yyvaluep).pEnumList); }
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

    case YYSYMBOL_optional_expr_list_in_braces: /* optional_expr_list_in_braces  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_type_declaration_no_options_no_dim: /* type_declaration_no_options_no_dim  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_type_declaration: /* type_declaration  */
            { delete ((*yyvaluep).pTypeDecl); }
        break;

    case YYSYMBOL_make_decl: /* make_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_decl_no_bracket: /* make_decl_no_bracket  */
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

    case YYSYMBOL_make_struct_dim_list: /* make_struct_dim_list  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_struct_dim_decl: /* make_struct_dim_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_optional_make_struct_dim_decl: /* optional_make_struct_dim_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_struct_decl: /* make_struct_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_tuple_call: /* make_tuple_call  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_dim_decl: /* make_dim_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_expr_map_tuple_list: /* expr_map_tuple_list  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_table_decl: /* make_table_decl  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_make_table_call: /* make_table_call  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_array_comprehension_where: /* array_comprehension_where  */
            { delete ((*yyvaluep).pExpression); }
        break;

    case YYSYMBOL_table_comprehension: /* table_comprehension  */
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
        delete (yyvsp[-1].pExpression);    // we do nothing, we don't even attemp to 'visit'
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
        auto sc = make_smart<ExprConstString>(tokAt(scanner,(yylsp[0])),esconst);
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
            call_fmt->arguments.push_back(make_smart<ExprConstString>(tokAt(scanner,(yylsp[-1])),":" + *(yyvsp[-1].s)));
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
            yyextra->g_ReaderMacro = macros.back().get();
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

  case 53: /* require_module_name: require_module_name '.' "name"  */
                                                {
        *(yyvsp[-2].s) += ".";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 54: /* require_module_name: require_module_name '/' "name"  */
                                                {
        *(yyvsp[-2].s) += "/";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 55: /* require_module: require_module_name is_public_module  */
                                                         {
        ast_requireModule(scanner,(yyvsp[-1].s),nullptr,(yyvsp[0].b),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 56: /* require_module: require_module_name "as" "name" is_public_module  */
                                                                              {
        ast_requireModule(scanner,(yyvsp[-3].s),(yyvsp[-1].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 57: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 58: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 62: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 63: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 64: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 65: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 66: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 67: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 68: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 73: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 74: /* $@3: %empty  */
                                           {
        yyextra->das_oneliner_line = tokAt(scanner,(yylsp[-1])).last_line;
    }
    break;

  case 75: /* expression_else: "else" optional_emit_semis $@3 expression_else_block  */
                                   {
        yyextra->das_oneliner_line = -1;
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 76: /* $@4: %empty  */
                                                                        {
        yyextra->das_oneliner_line = tokAt(scanner,(yylsp[-1])).last_line;
    }
    break;

  case 77: /* expression_else: elif_or_static_elif '(' expr ')' optional_emit_semis $@4 expression_else_block expression_else  */
                                                         {
        yyextra->das_oneliner_line = -1;
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-7])),(yyvsp[-5].pExpression),(yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-7].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 78: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 79: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 80: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 81: /* expression_else_one_liner: "else" expression_if_one_liner  */
                                                      {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 82: /* expression_if_one_liner: expr_no_bracket  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 83: /* expression_if_one_liner: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 84: /* expression_if_one_liner: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 85: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 86: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 91: /* $@5: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 92: /* $@6: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 93: /* expression_if_block: '{' $@5 expressions $@6 '}' expression_block_finally  */
                                        {
        (yyval.pExpression) = (yyvsp[-3].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-5]),(yylsp[0]));
        if ( (yyvsp[0].pExpression) ) {
            auto pF = (ExprBlock *) (yyvsp[0].pExpression);
            auto pB = (ExprBlock *) (yyval.pExpression);
            swap ( pB->finalList, pF->list );
            delete pF;
        }
    }
    break;

  case 94: /* $@7: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 95: /* expression_if_block: $@7 expression_if_one_liner SEMICOLON  */
                                               {
        if ( yyextra->das_oneliner_line != -1 && (yyvsp[-1].pExpression)->at.line != yyextra->das_oneliner_line ) {
            if ( yyextra->g_Program->policies.temp_one_liner_warning ) {
                (*daScriptEnvironment::getBound()->g_compilerLog) << (yyvsp[-1].pExpression)->at.describe() << ": *warning* if one-liner body should be on the same line\n";
            } else {
                das2_yyerror(scanner,"if one-liner body must be on the same line",
                    (yyvsp[-1].pExpression)->at, CompilationError::syntax_error);
            }
        }
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 96: /* $@8: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 97: /* $@9: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 98: /* expression_else_block: '{' $@8 expressions $@9 '}' expression_block_finally  */
                                        {
        (yyval.pExpression) = (yyvsp[-3].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-5]),(yylsp[0]));
        if ( (yyvsp[0].pExpression) ) {
            auto pF = (ExprBlock *) (yyvsp[0].pExpression);
            auto pB = (ExprBlock *) (yyval.pExpression);
            swap ( pB->finalList, pF->list );
            delete pF;
        }
    }
    break;

  case 99: /* $@10: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 100: /* expression_else_block: $@10 expression_if_one_liner SEMICOLON  */
                                               {
        if ( yyextra->das_oneliner_line != -1 && (yyvsp[-1].pExpression)->at.line != yyextra->das_oneliner_line ) {
            if ( yyextra->g_Program->policies.temp_one_liner_warning ) {
                (*daScriptEnvironment::getBound()->g_compilerLog) << (yyvsp[-1].pExpression)->at.describe() << ": *warning* else one-liner body should be on the same line\n";
            } else {
                das2_yyerror(scanner,"else one-liner body must be on the same line",
                    (yyvsp[-1].pExpression)->at, CompilationError::syntax_error);
            }
        }
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 101: /* $@11: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 102: /* $@12: %empty  */
                                                                  {
        yyextra->das_oneliner_line = tokAt(scanner,(yylsp[-1])).last_line;
    }
    break;

  case 103: /* expression_if_then_else: $@11 if_or_static_if '(' expr ')' optional_emit_semis $@12 expression_if_block expression_else  */
                                                       {
        yyextra->das_keyword = false;
        yyextra->das_oneliner_line = -1;
        auto blk = (yyvsp[-1].pExpression)->rtti_isBlock() ? static_cast<ExprBlock *>((yyvsp[-1].pExpression)) : ast_wrapInBlock((yyvsp[-1].pExpression));
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-7])),(yyvsp[-5].pExpression),blk,(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-7].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 104: /* expression_if_then_else_oneliner: expression_if_one_liner "if" '(' expr ')' expression_else_one_liner SEMICOLON  */
                                                                                                                      {
        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ast_wrapInBlock((yyvsp[-6].pExpression)),(yyvsp[-1].pExpression) ? ast_wrapInBlock((yyvsp[-1].pExpression)) : nullptr);
    }
    break;

  case 105: /* for_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 106: /* for_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 107: /* for_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 108: /* for_variable_name_with_pos_list: '(' tuple_expansion ')'  */
                                       {
        auto pSL = new vector<VariableNameAndPosition>();
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        pSL->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 109: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 110: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 111: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' '(' tuple_expansion ')'  */
                                                                                 {
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
    }
    break;

  case 112: /* $@13: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 113: /* expression_for_loop: $@13 "for" '(' for_variable_name_with_pos_list "in" expr_list ')' optional_emit_semis expression_block  */
                                                                                                                                     {
        yyextra->das_keyword = false;
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-5].pNameWithPosList),(yyvsp[-3].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 114: /* expression_unsafe: "unsafe" optional_emit_semis expression_block  */
                                                                    {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-2])));
        pUnsafe->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 115: /* $@14: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 116: /* expression_while_loop: $@14 "while" '(' expr ')' optional_emit_semis expression_block  */
                                                                                         {
        yyextra->das_keyword = false;
        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-5])));
        pWhile->cond = (yyvsp[-3].pExpression);
        pWhile->body = (yyvsp[0].pExpression);
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 117: /* $@15: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 118: /* expression_with: $@15 "with" '(' expr ')' optional_emit_semis expression_block  */
                                                                                   {
        yyextra->das_keyword = false;
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-5])));
        pWith->with = (yyvsp[-3].pExpression);
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 119: /* expression_with_alias: "assume" "name" '=' expr  */
                                                      {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), ExpressionPtr((yyvsp[0].pExpression)));
        delete (yyvsp[-2].s);
    }
    break;

  case 120: /* expression_with_alias: "typedef" "name" '=' type_declaration  */
                                                                {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), TypeDeclPtr((yyvsp[0].pTypeDecl)));
    }
    break;

  case 121: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 122: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 123: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 124: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 125: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 126: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 127: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 128: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 129: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 130: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 131: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 132: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 133: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 134: /* annotation_argument: annotation_argument_name '=' '@' '@' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[0].s); delete (yyvsp[-4].s); }
    break;

  case 135: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 136: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 137: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 138: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 139: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 140: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                          {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[-4].s); }
    }
    break;

  case 141: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 142: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 143: /* metadata_argument_list: '@' annotation_argument optional_emit_semis  */
                                                         {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[-1].aa));
    }
    break;

  case 144: /* metadata_argument_list: metadata_argument_list '@' annotation_argument optional_emit_semis  */
                                                                                      {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-3].aaList),(yyvsp[-1].aa));
    }
    break;

  case 145: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 146: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 147: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 148: /* annotation_declaration_name: "template"  */
                                    { (yyval.s) = new string("template"); }
    break;

  case 149: /* annotation_declaration_basic: annotation_declaration_name  */
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

  case 150: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
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

  case 151: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 152: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[0].fa); (yyvsp[0].fa) = nullptr;
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 153: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::And,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 154: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
                                                                            {
        if ( !(yyvsp[-2].fa)->annotation || !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        if ( !(yyvsp[0].fa)->annotation || !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Or,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 155: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation);
            delete (yyvsp[-2].fa); (yyvsp[-2].fa) = nullptr;
        }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Xor,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 156: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 157: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 158: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 159: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 160: /* optional_annotation_list: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 161: /* optional_annotation_list: '[' annotation_list ']'  */
                                       { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 162: /* optional_annotation_list_with_emit_semis: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 163: /* optional_annotation_list_with_emit_semis: '[' annotation_list ']' optional_emit_semis  */
                                                          { (yyval.faList) = (yyvsp[-2].faList); }
    break;

  case 164: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 165: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 166: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 167: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 168: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 169: /* optional_function_type: "->" type_declaration  */
                                           {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 170: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 171: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 172: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 173: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 174: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 175: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 176: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 177: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 178: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 179: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 180: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 181: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 182: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 183: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 184: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 185: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 186: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 187: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 188: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 189: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 190: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 191: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 192: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 193: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 194: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 195: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 196: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 197: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 198: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 199: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 200: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 201: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 202: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 203: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 204: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 205: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 206: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 207: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 208: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 209: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 210: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 211: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 212: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 213: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 214: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 215: /* function_name: "operator" '[' ']' '='  */
                                 { (yyval.s) = new string("[]="); }
    break;

  case 216: /* function_name: "operator" '[' ']' "<-"  */
                                    { (yyval.s) = new string("[]<-"); }
    break;

  case 217: /* function_name: "operator" '[' ']' ":="  */
                                      { (yyval.s) = new string("[]:="); }
    break;

  case 218: /* function_name: "operator" '[' ']' "+="  */
                                     { (yyval.s) = new string("[]+="); }
    break;

  case 219: /* function_name: "operator" '[' ']' "-="  */
                                     { (yyval.s) = new string("[]-="); }
    break;

  case 220: /* function_name: "operator" '[' ']' "*="  */
                                     { (yyval.s) = new string("[]*="); }
    break;

  case 221: /* function_name: "operator" '[' ']' "/="  */
                                     { (yyval.s) = new string("[]/="); }
    break;

  case 222: /* function_name: "operator" '[' ']' "%="  */
                                     { (yyval.s) = new string("[]%="); }
    break;

  case 223: /* function_name: "operator" '[' ']' "&="  */
                                     { (yyval.s) = new string("[]&="); }
    break;

  case 224: /* function_name: "operator" '[' ']' "|="  */
                                     { (yyval.s) = new string("[]|="); }
    break;

  case 225: /* function_name: "operator" '[' ']' "^="  */
                                     { (yyval.s) = new string("[]^="); }
    break;

  case 226: /* function_name: "operator" '[' ']' "&&="  */
                                        { (yyval.s) = new string("[]&&="); }
    break;

  case 227: /* function_name: "operator" '[' ']' "||="  */
                                        { (yyval.s) = new string("[]||="); }
    break;

  case 228: /* function_name: "operator" '[' ']' "^^="  */
                                        { (yyval.s) = new string("[]^^="); }
    break;

  case 229: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 230: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 231: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 232: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 233: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 234: /* function_name: "operator" '.' "name" "+="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`+="); delete (yyvsp[-1].s); }
    break;

  case 235: /* function_name: "operator" '.' "name" "-="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`-="); delete (yyvsp[-1].s); }
    break;

  case 236: /* function_name: "operator" '.' "name" "*="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`*="); delete (yyvsp[-1].s); }
    break;

  case 237: /* function_name: "operator" '.' "name" "/="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`/="); delete (yyvsp[-1].s); }
    break;

  case 238: /* function_name: "operator" '.' "name" "%="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`%="); delete (yyvsp[-1].s); }
    break;

  case 239: /* function_name: "operator" '.' "name" "&="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&="); delete (yyvsp[-1].s); }
    break;

  case 240: /* function_name: "operator" '.' "name" "|="  */
                                          { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`|="); delete (yyvsp[-1].s); }
    break;

  case 241: /* function_name: "operator" '.' "name" "^="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^="); delete (yyvsp[-1].s); }
    break;

  case 242: /* function_name: "operator" '.' "name" "&&="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&&="); delete (yyvsp[-1].s); }
    break;

  case 243: /* function_name: "operator" '.' "name" "||="  */
                                            { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`||="); delete (yyvsp[-1].s); }
    break;

  case 244: /* function_name: "operator" '.' "name" "^^="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^^="); delete (yyvsp[-1].s); }
    break;

  case 245: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 246: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 247: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 248: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 249: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 250: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 251: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 252: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 253: /* function_name: "operator" "is" das_type_name  */
                                                { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 254: /* function_name: "operator" "as" das_type_name  */
                                                { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 255: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 256: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 257: /* function_name: "operator" '?' "as" das_type_name  */
                                                    { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 258: /* function_name: das_type_name  */
                            { (yyval.s) = (yyvsp[0].s); }
    break;

  case 259: /* das_type_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 260: /* das_type_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 261: /* das_type_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 262: /* das_type_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 263: /* das_type_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 264: /* das_type_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 265: /* das_type_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 266: /* das_type_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 267: /* das_type_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 268: /* das_type_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 269: /* das_type_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 270: /* das_type_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 271: /* das_type_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 272: /* das_type_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 273: /* das_type_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 274: /* das_type_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 275: /* das_type_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 276: /* das_type_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 277: /* das_type_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 278: /* das_type_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 279: /* das_type_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 280: /* das_type_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 281: /* das_type_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 282: /* das_type_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 283: /* das_type_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 284: /* optional_template: %empty  */
                                        { (yyval.b) = false; }
    break;

  case 285: /* optional_template: "template"  */
                                        { (yyval.b) = true; }
    break;

  case 286: /* global_function_declaration: optional_annotation_list_with_emit_semis "def" optional_template function_declaration  */
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

  case 287: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 288: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 289: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 290: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 291: /* $@16: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 292: /* function_declaration: optional_public_or_private_function $@16 function_declaration_header optional_emit_semis expression_block  */
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

  case 293: /* expression_block_finally: %empty  */
        {
        (yyval.pExpression) = nullptr;
    }
    break;

  case 294: /* $@17: %empty  */
                  {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 295: /* $@18: %empty  */
                             {
        yyextra->pop_nesteds();
    }
    break;

  case 296: /* expression_block_finally: "finally" $@17 '{' expressions $@18 '}'  */
          {
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 297: /* $@19: %empty  */
        {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 298: /* $@20: %empty  */
                                      {
        yyextra->pop_nesteds();
    }
    break;

  case 299: /* expression_block: $@19 '{' expressions $@20 '}' expression_block_finally  */
                                        {
        (yyval.pExpression) = (yyvsp[-3].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-4]),(yylsp[0]));
        if ( (yyvsp[0].pExpression) ) {
            auto pF = (ExprBlock *) (yyvsp[0].pExpression);
            auto pB = (ExprBlock *) (yyval.pExpression);
            swap ( pB->finalList, pF->list );
            delete pF;
        }
    }
    break;

  case 300: /* expr_call_pipe_no_bracket: expr_call expr_full_block_assumed_piped  */
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

  case 301: /* expr_call_pipe_no_bracket: expr_method_call_no_bracket expr_full_block_assumed_piped  */
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

  case 302: /* expr_call_pipe_no_bracket: expr_field_no_bracket expr_full_block_assumed_piped  */
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

  case 303: /* expression_any: SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 304: /* expression_any: expr_assign_no_bracket SEMICOLON  */
                                                    { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 305: /* expression_any: expression_delete SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 306: /* expression_any: expression_let  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 307: /* expression_any: expression_while_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 308: /* expression_any: expression_unsafe  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 309: /* expression_any: expression_with  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 310: /* expression_any: expression_with_alias SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 311: /* expression_any: expression_for_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 312: /* expression_any: expression_break SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 313: /* expression_any: expression_continue SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 314: /* expression_any: expression_return SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 315: /* expression_any: expression_yield SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 316: /* expression_any: expression_if_then_else  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 317: /* expression_any: expression_if_then_else_oneliner  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 318: /* expression_any: expression_try_catch  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 319: /* expression_any: expression_label SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 320: /* expression_any: expression_goto SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 321: /* expression_any: "pass" SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 322: /* $@21: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 323: /* $@22: %empty  */
                         {
        yyextra->pop_nesteds();
    }
    break;

  case 324: /* expression_any: '{' $@21 expressions $@22 '}' expression_block_finally  */
                                        {
        (yyval.pExpression) = (yyvsp[-3].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-5]),(yylsp[0]));
        if ( (yyvsp[0].pExpression) ) {
            auto pF = (ExprBlock *) (yyvsp[0].pExpression);
            auto pB = (ExprBlock *) (yyval.pExpression);
            swap ( pB->finalList, pF->list );
            delete pF;
        }
    }
    break;

  case 325: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 326: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 327: /* expressions: expressions error  */
                                 {
        delete (yyvsp[-1].pExpression); (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 328: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 329: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
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

  case 334: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 335: /* name_in_namespace: "name" "::" "name"  */
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

  case 336: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 337: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 338: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 339: /* $@23: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 340: /* $@24: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 341: /* new_type_declaration: '<' $@23 type_declaration '>' $@24  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 342: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 343: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 344: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 345: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 346: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 347: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 348: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 349: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 350: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 351: /* expression_return: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 352: /* expression_return: "return" expr  */
                                      {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 353: /* expression_return: "return" "<-" expr  */
                                             {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 354: /* expression_yield: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 355: /* expression_yield: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 356: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 357: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 358: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 359: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 360: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 361: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 362: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 363: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 364: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 365: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 366: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 367: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr SEMICOLON  */
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

  case 368: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 369: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 370: /* expression_let: kwd_let optional_in_scope '{' variable_declaration_list '}'  */
                                                                               {
        (yyval.pExpression) = ast_LetList(scanner,(yyvsp[-4].b),(yyvsp[-3].b),*(yyvsp[-1].pVarDeclList),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 371: /* $@25: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 372: /* $@26: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 373: /* expr_cast: "cast" '<' $@25 type_declaration_no_options '>' $@26 expr_no_bracket  */
                                                                                                                                                           {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
    }
    break;

  case 374: /* $@27: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 375: /* $@28: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 376: /* expr_cast: "upcast" '<' $@27 type_declaration_no_options '>' $@28 expr_no_bracket  */
                                                                                                                                                             {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 377: /* $@29: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 378: /* $@30: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 379: /* expr_cast: "reinterpret" '<' $@29 type_declaration_no_options '>' $@30 expr_no_bracket  */
                                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 380: /* $@31: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 381: /* $@32: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 382: /* expr_type_decl: "type" '<' $@31 type_declaration '>' $@32  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 383: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
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

  case 384: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
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

  case 385: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" c_or_s "name" '>' '(' expr ')'  */
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

  case 386: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 387: /* expr_list: "<-" expr  */
                             {
            (yyval.pExpression) = ast_makeMoveArgument(scanner, (yyvsp[0].pExpression), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 388: /* expr_list: expr_list ',' expr  */
                                        {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 389: /* expr_list: expr_list ',' "<-" expr  */
                                                   {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-3])),(yyvsp[-3].pExpression),ast_makeMoveArgument(scanner, (yyvsp[0].pExpression), tokAt(scanner,(yylsp[0]))));
    }
    break;

  case 390: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 391: /* block_or_simple_block: "=>" expr_no_bracket  */
                                                   {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 392: /* block_or_simple_block: "=>" "<-" expr_no_bracket  */
                                                          {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 393: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 394: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 395: /* block_or_lambda: '@' '@'  */
                { (yyval.i) = 2;   /* local function */ }
    break;

  case 396: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 397: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 398: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 399: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 400: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 401: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 402: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 403: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 404: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 405: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis block_or_simple_block  */
                                                                                                                {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 406: /* expr_full_block_assumed_piped: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis expression_block  */
                                                                                                           {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 407: /* expr_full_block_assumed_piped: '{' expressions '}'  */
                                   {
        (yyval.pExpression) = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,new TypeDecl(Type::autoinfer),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-1])),tokAt(scanner,(yylsp[-1])),LineInfo());
    }
    break;

  case 408: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 409: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 410: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 411: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 412: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 413: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 414: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 415: /* expr_assign_no_bracket: expr_no_bracket  */
                                                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 416: /* expr_assign_no_bracket: expr_no_bracket '=' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 417: /* expr_assign_no_bracket: expr_no_bracket "<-" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 418: /* expr_assign_no_bracket: expr_no_bracket "<-" make_table_decl  */
                                                                   { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 419: /* expr_assign_no_bracket: expr_no_bracket "<-" array_comprehension  */
                                                                     { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 420: /* expr_assign_no_bracket: expr_no_bracket ":=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 421: /* expr_assign_no_bracket: expr_no_bracket "&=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 422: /* expr_assign_no_bracket: expr_no_bracket "|=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 423: /* expr_assign_no_bracket: expr_no_bracket "^=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 424: /* expr_assign_no_bracket: expr_no_bracket "&&=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 425: /* expr_assign_no_bracket: expr_no_bracket "||=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 426: /* expr_assign_no_bracket: expr_no_bracket "^^=" expr_no_bracket  */
                                                                      { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 427: /* expr_assign_no_bracket: expr_no_bracket "+=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 428: /* expr_assign_no_bracket: expr_no_bracket "-=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 429: /* expr_assign_no_bracket: expr_no_bracket "*=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 430: /* expr_assign_no_bracket: expr_no_bracket "/=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 431: /* expr_assign_no_bracket: expr_no_bracket "%=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 432: /* expr_assign_no_bracket: expr_no_bracket "<<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 433: /* expr_assign_no_bracket: expr_no_bracket ">>=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 434: /* expr_assign_no_bracket: expr_no_bracket "<<<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 435: /* expr_assign_no_bracket: expr_no_bracket ">>>=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 436: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 437: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 438: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' ')'  */
                                                                    {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 439: /* expr_method_call_no_bracket: expr_no_bracket "->" "name" '(' expr_list ')'  */
                                                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 440: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 441: /* func_addr_name: "$i" '(' expr ')'  */
                                          {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 442: /* func_addr_expr: '@' '@' func_addr_name  */
                                          {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 443: /* $@33: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 444: /* $@34: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 445: /* func_addr_expr: '@' '@' '<' $@33 type_declaration_no_options '>' $@34 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value.get()));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 446: /* $@35: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 447: /* $@36: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 448: /* func_addr_expr: '@' '@' '<' $@35 optional_function_argument_list optional_function_type '>' $@36 func_addr_name  */
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

  case 449: /* expr_field_no_bracket: expr_no_bracket '.' "name"  */
                                                         {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 450: /* expr_field_no_bracket: expr_no_bracket '.' '.' "name"  */
                                                             {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 451: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' ')'  */
                                                                 {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 452: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' expr_list ')'  */
                                                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 453: /* expr_field_no_bracket: expr_no_bracket '.' "name" '(' '[' make_struct_fields ']' ')'  */
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

  case 454: /* expr_field_no_bracket: expr_no_bracket '.' basic_type_declaration '(' ')'  */
                                                                                   {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-4]),(yyloc));
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 455: /* expr_field_no_bracket: expr_no_bracket '.' basic_type_declaration '(' expr_list ')'  */
                                                                                                        {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        pInvoke->atEnclosure = tokRangeAt(scanner,(yylsp[-5]),(yyloc));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 456: /* $@37: %empty  */
                                          { yyextra->das_suppress_errors=true; }
    break;

  case 457: /* $@38: %empty  */
                                                                                       { yyextra->das_suppress_errors=false; }
    break;

  case 458: /* expr_field_no_bracket: expr_no_bracket '.' $@37 error $@38  */
                                                                                                                               {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), "");
        yyerrok;
    }
    break;

  case 459: /* expr_call: name_in_namespace '(' ')'  */
                                               {
            (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),*(yyvsp[-2].s));
            delete (yyvsp[-2].s);
    }
    break;

  case 460: /* expr_call: name_in_namespace '(' "uninitialized" ')'  */
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

  case 461: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
                                                               {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 462: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
                                                                                 {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-4])),*(yyvsp[-4].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-4].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 463: /* expr_call: name_in_namespace '(' expr_list ')'  */
                                                                    {
            (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),*(yyvsp[-3].s)),(yyvsp[-1].pExpression));
            delete (yyvsp[-3].s);
    }
    break;

  case 464: /* expr_call: basic_type_declaration '(' ')'  */
                                                    {
        (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-2].type)));
    }
    break;

  case 465: /* expr_call: basic_type_declaration '(' expr_list ')'  */
                                                                         {
        (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-3].type))),(yyvsp[-1].pExpression));
    }
    break;

  case 466: /* expr: expr_no_bracket  */
                                       { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 467: /* expr: make_table_decl  */
                                     { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 468: /* expr: array_comprehension  */
                                     { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 469: /* expr_no_bracket: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 470: /* expr_no_bracket: name_in_namespace  */
                                              { (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 471: /* expr_no_bracket: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 472: /* expr_no_bracket: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 473: /* expr_no_bracket: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 474: /* expr_no_bracket: make_decl_no_bracket  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 475: /* expr_no_bracket: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 476: /* expr_no_bracket: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 477: /* expr_no_bracket: expr_field_no_bracket  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 478: /* expr_no_bracket: expr_mtag_no_bracket  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 479: /* expr_no_bracket: '!' expr_no_bracket  */
                                                         { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",(yyvsp[0].pExpression)); }
    break;

  case 480: /* expr_no_bracket: '~' expr_no_bracket  */
                                                         { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",(yyvsp[0].pExpression)); }
    break;

  case 481: /* expr_no_bracket: '+' expr_no_bracket  */
                                                             { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",(yyvsp[0].pExpression)); }
    break;

  case 482: /* expr_no_bracket: '-' expr_no_bracket  */
                                                             { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",(yyvsp[0].pExpression)); }
    break;

  case 483: /* expr_no_bracket: expr_no_bracket "<<" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 484: /* expr_no_bracket: expr_no_bracket ">>" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 485: /* expr_no_bracket: expr_no_bracket "<<<" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 486: /* expr_no_bracket: expr_no_bracket ">>>" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 487: /* expr_no_bracket: expr_no_bracket '+' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 488: /* expr_no_bracket: expr_no_bracket '-' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 489: /* expr_no_bracket: expr_no_bracket '*' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 490: /* expr_no_bracket: expr_no_bracket '/' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 491: /* expr_no_bracket: expr_no_bracket '%' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 492: /* expr_no_bracket: expr_no_bracket '<' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 493: /* expr_no_bracket: expr_no_bracket '>' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 494: /* expr_no_bracket: expr_no_bracket "==" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 495: /* expr_no_bracket: expr_no_bracket "!=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 496: /* expr_no_bracket: expr_no_bracket "<=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 497: /* expr_no_bracket: expr_no_bracket ">=" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 498: /* expr_no_bracket: expr_no_bracket '&' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 499: /* expr_no_bracket: expr_no_bracket '|' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 500: /* expr_no_bracket: expr_no_bracket '^' expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 501: /* expr_no_bracket: expr_no_bracket "&&" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 502: /* expr_no_bracket: expr_no_bracket "||" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 503: /* expr_no_bracket: expr_no_bracket "^^" expr_no_bracket  */
                                                                   { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 504: /* expr_no_bracket: expr_no_bracket ".." expr_no_bracket  */
                                                                   {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 505: /* expr_no_bracket: "++" expr_no_bracket  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", (yyvsp[0].pExpression)); }
    break;

  case 506: /* expr_no_bracket: "--" expr_no_bracket  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", (yyvsp[0].pExpression)); }
    break;

  case 507: /* expr_no_bracket: expr_no_bracket "++"  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 508: /* expr_no_bracket: expr_no_bracket "--"  */
                                                            { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", (yyvsp[-1].pExpression)); }
    break;

  case 509: /* expr_no_bracket: '(' expr_list optional_comma ')'  */
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

  case 510: /* expr_no_bracket: '(' make_struct_single ')'  */
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

  case 511: /* expr_no_bracket: expr_no_bracket '[' expr ']'  */
                                                            { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 512: /* expr_no_bracket: expr_no_bracket '.' '[' expr ']'  */
                                                                { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 513: /* expr_no_bracket: expr_no_bracket "?[" expr ']'  */
                                                            { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 514: /* expr_no_bracket: expr_no_bracket '.' "?[" expr ']'  */
                                                                { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 515: /* expr_no_bracket: expr_no_bracket "?." "name"  */
                                                            { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 516: /* expr_no_bracket: expr_no_bracket '.' "?." "name"  */
                                                                { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 517: /* expr_no_bracket: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 518: /* expr_no_bracket: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 519: /* expr_no_bracket: '*' expr_no_bracket  */
                                                              { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 520: /* expr_no_bracket: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 521: /* expr_no_bracket: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 522: /* expr_no_bracket: expr_generator  */
                                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 523: /* expr_no_bracket: expr_no_bracket "??" expr_no_bracket  */
                                                                         { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 524: /* expr_no_bracket: expr_no_bracket '?' expr_no_bracket ':' expr_no_bracket  */
                                                                                           {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        }
    break;

  case 525: /* $@39: %empty  */
                                                          { yyextra->das_arrow_depth ++; }
    break;

  case 526: /* $@40: %empty  */
                                                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 527: /* expr_no_bracket: expr_no_bracket "is" "type" '<' $@39 type_declaration_no_options '>' $@40  */
                                                                                                                                                                  {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 528: /* expr_no_bracket: expr_no_bracket "is" basic_type_declaration  */
                                                                          {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 529: /* expr_no_bracket: expr_no_bracket "is" "name"  */
                                                         {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 530: /* expr_no_bracket: expr_no_bracket "as" "name"  */
                                                         {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 531: /* $@41: %empty  */
                                                          { yyextra->das_arrow_depth ++; }
    break;

  case 532: /* $@42: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 533: /* expr_no_bracket: expr_no_bracket "as" "type" '<' $@41 type_declaration '>' $@42  */
                                                                                                                                                       {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 534: /* expr_no_bracket: expr_no_bracket "as" basic_type_declaration  */
                                                                          {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 535: /* expr_no_bracket: expr_no_bracket '?' "as" "name"  */
                                                             {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 536: /* $@43: %empty  */
                                                              { yyextra->das_arrow_depth ++; }
    break;

  case 537: /* $@44: %empty  */
                                                                                                                          { yyextra->das_arrow_depth --; }
    break;

  case 538: /* expr_no_bracket: expr_no_bracket '?' "as" "type" '<' $@43 type_declaration '>' $@44  */
                                                                                                                                                           {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 539: /* expr_no_bracket: expr_no_bracket '?' "as" basic_type_declaration  */
                                                                              {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 540: /* expr_no_bracket: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 541: /* expr_no_bracket: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 542: /* expr_no_bracket: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 543: /* expr_no_bracket: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 544: /* expr_no_bracket: expr_method_call_no_bracket  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 545: /* expr_no_bracket: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 546: /* expr_no_bracket: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 547: /* expr_no_bracket: expr_no_bracket "<|" expr_no_bracket  */
                                                                      { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 548: /* expr_no_bracket: expr_no_bracket "|>" expr_no_bracket  */
                                                                      { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 549: /* expr_no_bracket: expr_no_bracket "|>" basic_type_declaration  */
                                                                     {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 550: /* expr_no_bracket: expr_call_pipe_no_bracket  */
                                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 551: /* expr_no_bracket: "unsafe" '(' expr ')'  */
                                         {
            (yyvsp[-1].pExpression)->alwaysSafe = true;
            (yyvsp[-1].pExpression)->userSaidItsSafe = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    break;

  case 552: /* expr_no_bracket: expr_no_bracket "=>" expr_no_bracket  */
                                                               {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 553: /* expr_no_bracket: expr_no_bracket "=>" make_table_decl  */
                                                               {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 554: /* expr_no_bracket: expr_no_bracket "=>" array_comprehension  */
                                                                   {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 555: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 556: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 557: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list optional_emit_semis expression_block  */
                                                                                                                                                  {
        auto closure = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),(yyvsp[0].pExpression));
        ((ExprBlock *)(yyvsp[0].pExpression))->returnType = make_smart<TypeDecl>(Type::autoinfer);
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),closure,tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 558: /* expr_mtag_no_bracket: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 559: /* expr_mtag_no_bracket: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 560: /* expr_mtag_no_bracket: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 561: /* expr_mtag_no_bracket: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 562: /* expr_mtag_no_bracket: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 563: /* expr_mtag_no_bracket: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 564: /* expr_mtag_no_bracket: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 565: /* expr_mtag_no_bracket: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 566: /* expr_mtag_no_bracket: expr_no_bracket '.' "$f" '(' expr ')'  */
                                                                           {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 567: /* expr_mtag_no_bracket: expr_no_bracket "?." "$f" '(' expr ')'  */
                                                                            {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 568: /* expr_mtag_no_bracket: expr_no_bracket '.' '.' "$f" '(' expr ')'  */
                                                                               {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 569: /* expr_mtag_no_bracket: expr_no_bracket '.' "?." "$f" '(' expr ')'  */
                                                                                {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 570: /* expr_mtag_no_bracket: expr_no_bracket "as" "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 571: /* expr_mtag_no_bracket: expr_no_bracket '?' "as" "$f" '(' expr ')'  */
                                                                                  {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 572: /* expr_mtag_no_bracket: expr_no_bracket "is" "$f" '(' expr ')'  */
                                                                              {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 573: /* expr_mtag_no_bracket: '@' '@' "$c" '(' expr ')'  */
                                                         {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 574: /* optional_field_annotation: %empty  */
                                      { (yyval.aaList) = nullptr; }
    break;

  case 575: /* optional_field_annotation: metadata_argument_list  */
                                      { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 576: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 577: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 578: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 579: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 580: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 581: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 582: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 583: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 584: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 585: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 586: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 587: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 588: /* struct_variable_declaration_list: struct_variable_declaration_list "new line, semicolon"  */
                                                                 { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 589: /* struct_variable_declaration_list: struct_variable_declaration_list "typedef" "name" '=' type_declaration SEMICOLON  */
                                                                                                                {
        (yyval.pVarDeclList) = (yyvsp[-5].pVarDeclList);
        ast_structureAlias(scanner,(yyvsp[-3].s),(yyvsp[-1].pTypeDecl),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 590: /* $@45: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 591: /* struct_variable_declaration_list: struct_variable_declaration_list $@45 structure_variable_declaration SEMICOLON  */
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

  case 592: /* $@46: %empty  */
                                                                                                                     {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 593: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable "abstract" optional_constant $@46 function_declaration_header SEMICOLON  */
                                                          {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 594: /* $@47: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 595: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@47 function_declaration_header optional_emit_semis expression_block  */
                                                                                            {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-10].pVarDeclList),(yyvsp[-9].faList),(yyvsp[-6].b),(yyvsp[-7].b),(yyvsp[-5].i),(yyvsp[-4].b),(yyvsp[-2].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-8]),(yylsp[0])),tokAt(scanner,(yylsp[-9])));
            }
    break;

  case 596: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 597: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 598: /* function_argument_declaration_type: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 599: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 600: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 601: /* function_argument_list: function_argument_declaration_no_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 602: /* function_argument_list: function_argument_declaration_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 603: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 604: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 605: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 606: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 607: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                       { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 608: /* tuple_alias_type_list: %empty  */
      {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 609: /* tuple_alias_type_list: tuple_type  */
                       {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 610: /* tuple_alias_type_list: tuple_alias_type_list semis tuple_type  */
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

  case 611: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 612: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 613: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 614: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 615: /* variant_alias_type_list: variant_type  */
                         {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 616: /* variant_alias_type_list: variant_alias_type_list semis variant_type  */
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

  case 617: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 618: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 619: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 620: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 621: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 622: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 623: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 624: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 625: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 626: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 627: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 628: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 629: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 630: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 631: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 632: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 633: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 634: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 635: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 636: /* global_let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 637: /* global_let_variable_name_with_pos_list: global_let_variable_name_with_pos_list ',' "name"  */
                                                                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 638: /* variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 639: /* variable_declaration_list: variable_declaration_list SEMICOLON  */
                                                  {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 640: /* variable_declaration_list: variable_declaration_list let_variable_declaration  */
                                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
        (yyvsp[-1].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 641: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 642: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 643: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 644: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                         {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 645: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                               {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 646: /* global_let_variable_declaration: global_let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 647: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 648: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 649: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 650: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 651: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 652: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 653: /* global_variable_declaration_list: global_variable_declaration_list SEMICOLON  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 654: /* $@48: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 655: /* global_variable_declaration_list: global_variable_declaration_list $@48 optional_field_annotation let_variable_declaration  */
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

  case 656: /* global_let: kwd_let optional_shared optional_public_or_private_variable '{' global_variable_declaration_list '}'  */
                                                                                                                                       {
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 657: /* $@49: %empty  */
                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 658: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@49 optional_field_annotation global_let_variable_declaration  */
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

  case 659: /* enum_expression: "name"  */
                   {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        delete (yyvsp[0].s);
    }
    break;

  case 660: /* enum_expression: "name" '=' expr  */
                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[-2].s),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-2])));
        delete (yyvsp[-2].s);
    }
    break;

  case 663: /* enum_list: %empty  */
        {
        (yyval.pEnumList) = new Enumeration();
    }
    break;

  case 664: /* enum_list: enum_expression  */
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

  case 665: /* enum_list: enum_list commas enum_expression  */
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

  case 666: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 667: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 668: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 669: /* $@50: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 670: /* single_alias: optional_public_or_private_alias "name" $@50 '=' type_declaration  */
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

  case 672: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 673: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 674: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 675: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 676: /* optional_enum_basic_type_declaration: %empty  */
        {
        (yyval.type) = Type::tInt;
    }
    break;

  case 677: /* optional_enum_basic_type_declaration: ':' enum_basic_type_declaration  */
                                              {
        (yyval.type) = (yyvsp[0].type);
    }
    break;

  case 684: /* $@51: %empty  */
                                                                     {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 685: /* $@52: %empty  */
                                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 686: /* $@53: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 687: /* enum_declaration: optional_annotation_list_with_emit_semis "enum" $@51 optional_public_or_private_enum enum_name optional_enum_basic_type_declaration optional_emit_commas '{' $@52 enum_list optional_commas $@53 '}'  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-8].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-12].faList),tokAt(scanner,(yylsp[-12])),(yyvsp[-9].b),(yyvsp[-8].pEnum),(yyvsp[-3].pEnumList),(yyvsp[-7].type));
    }
    break;

  case 688: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 689: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 690: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 691: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 692: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 693: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 694: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 695: /* class_or_struct: "class" "template"  */
                                  { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 696: /* class_or_struct: "struct" "template"  */
                                  { (yyval.i) = CorS_StructTemplate; }
    break;

  case 697: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 698: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 699: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 700: /* optional_struct_variable_declaration_list: ';'  */
            {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 701: /* optional_struct_variable_declaration_list: '{' struct_variable_declaration_list '}'  */
                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 702: /* $@54: %empty  */
                                                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 703: /* $@55: %empty  */
                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 704: /* $@56: %empty  */
                                             {
        if ( (yyvsp[-1].pStructure) ) {
            (yyvsp[-1].pStructure)->isClass = (yyvsp[-4].i)==CorS_Class || (yyvsp[-4].i)==CorS_ClassTemplate;
            (yyvsp[-1].pStructure)->isTemplate = (yyvsp[-4].i)==CorS_ClassTemplate || (yyvsp[-4].i)==CorS_StructTemplate;
            (yyvsp[-1].pStructure)->privateStructure = !(yyvsp[-3].b);
        }
    }
    break;

  case 705: /* structure_declaration: optional_annotation_list_with_emit_semis $@54 class_or_struct optional_public_or_private_structure $@55 structure_name optional_emit_semis $@56 optional_struct_variable_declaration_list  */
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

  case 706: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 707: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
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

  case 749: /* auto_type_declaration: "$t" '(' expr ')'  */
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

  case 751: /* bitfield_bits: bitfield_bits ';' "name"  */
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

  case 753: /* bitfield_alias_bits: %empty  */
        {
        auto pSL = new vector<tuple<string,Expression *>>();
        (yyval.pNameExprList) = pSL;

    }
    break;

  case 754: /* bitfield_alias_bits: "name"  */
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

  case 755: /* bitfield_alias_bits: "name" '=' expr  */
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

  case 756: /* bitfield_alias_bits: bitfield_alias_bits commas "name"  */
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

  case 757: /* bitfield_alias_bits: bitfield_alias_bits commas "name" '=' expr  */
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

  case 758: /* bitfield_basic_type_declaration: %empty  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 759: /* bitfield_basic_type_declaration: ':' "uint8"  */
                             { (yyval.type) = Type::tBitfield8; }
    break;

  case 760: /* bitfield_basic_type_declaration: ':' "uint16"  */
                             { (yyval.type) = Type::tBitfield16; }
    break;

  case 761: /* bitfield_basic_type_declaration: ':' "uint"  */
                             { (yyval.type) = Type::tBitfield; }
    break;

  case 762: /* bitfield_basic_type_declaration: ':' "uint64"  */
                             { (yyval.type) = Type::tBitfield64; }
    break;

  case 763: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' '>'  */
                                                                          {
            (yyval.pTypeDecl) = new TypeDecl((yyvsp[-2].type));
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-2]));
    }
    break;

  case 764: /* $@57: %empty  */
                                                                     { yyextra->das_arrow_depth ++; }
    break;

  case 765: /* $@58: %empty  */
                                                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 766: /* bitfield_type_declaration: "bitfield" bitfield_basic_type_declaration '<' $@57 bitfield_bits '>' $@58  */
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

  case 769: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 770: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 771: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 772: /* dim_list: '[' ']'  */
                {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), nullptr);
    }
    break;

  case 773: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 774: /* dim_list: dim_list '[' ']'  */
                              {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), nullptr);
    }
    break;

  case 775: /* type_declaration_no_options: type_declaration_no_options_no_dim  */
                                                     {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 776: /* type_declaration_no_options: type_declaration_no_options_no_dim dim_list  */
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
        delete (yyvsp[0].pTypeDecl);
    }
    break;

  case 777: /* optional_expr_list_in_braces: %empty  */
            { (yyval.pExpression) = nullptr; }
    break;

  case 778: /* optional_expr_list_in_braces: '(' expr_list optional_comma ')'  */
                                                { (yyval.pExpression) = (yyvsp[-2].pExpression); }
    break;

  case 779: /* type_declaration_no_options_no_dim: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 780: /* type_declaration_no_options_no_dim: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 781: /* type_declaration_no_options_no_dim: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 782: /* type_declaration_no_options_no_dim: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 783: /* $@59: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 784: /* $@60: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 785: /* type_declaration_no_options_no_dim: "type" '<' $@59 type_declaration '>' $@60  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 786: /* type_declaration_no_options_no_dim: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 787: /* type_declaration_no_options_no_dim: name_in_namespace '(' optional_expr_list ')'  */
                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-3])), *(yyvsp[-3].s)));
        delete (yyvsp[-3].s);
    }
    break;

  case 788: /* type_declaration_no_options_no_dim: '$' name_in_namespace optional_expr_list_in_braces  */
                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-1]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-1])), *(yyvsp[-1].s)));
        delete (yyvsp[-1].s);
    }
    break;

  case 789: /* $@61: %empty  */
                                    { yyextra->das_arrow_depth ++; }
    break;

  case 790: /* type_declaration_no_options_no_dim: name_in_namespace '<' $@61 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 791: /* $@62: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 792: /* type_declaration_no_options_no_dim: '$' name_in_namespace '<' $@62 type_declaration_no_options_list '>' optional_expr_list_in_braces  */
                                                                                                                                                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-5]), (yylsp[0]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-2].pTypeDeclList),(yyvsp[0].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-5])), *(yyvsp[-5].s)));
        delete (yyvsp[-5].s);
    }
    break;

  case 793: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 794: /* type_declaration_no_options_no_dim: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 795: /* type_declaration_no_options_no_dim: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 796: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 797: /* type_declaration_no_options_no_dim: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 798: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 799: /* type_declaration_no_options_no_dim: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 800: /* type_declaration_no_options_no_dim: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 801: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 802: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 803: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 804: /* type_declaration_no_options_no_dim: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 805: /* $@63: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 806: /* $@64: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 807: /* type_declaration_no_options_no_dim: "smart_ptr" '<' $@63 type_declaration '>' $@64  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 808: /* type_declaration_no_options_no_dim: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 809: /* $@65: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 810: /* $@66: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 811: /* type_declaration_no_options_no_dim: "array" '<' $@65 type_declaration '>' $@66  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 812: /* $@67: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 813: /* $@68: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 814: /* type_declaration_no_options_no_dim: "table" '<' $@67 table_type_pair '>' $@68  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 815: /* $@69: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 816: /* $@70: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 817: /* type_declaration_no_options_no_dim: "iterator" '<' $@69 type_declaration '>' $@70  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 818: /* type_declaration_no_options_no_dim: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 819: /* $@71: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 820: /* $@72: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 821: /* type_declaration_no_options_no_dim: "block" '<' $@71 type_declaration '>' $@72  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 822: /* $@73: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 823: /* $@74: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 824: /* type_declaration_no_options_no_dim: "block" '<' $@73 optional_function_argument_list optional_function_type '>' $@74  */
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

  case 825: /* type_declaration_no_options_no_dim: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 826: /* $@75: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 827: /* $@76: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 828: /* type_declaration_no_options_no_dim: "function" '<' $@75 type_declaration '>' $@76  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 829: /* $@77: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 830: /* $@78: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 831: /* type_declaration_no_options_no_dim: "function" '<' $@77 optional_function_argument_list optional_function_type '>' $@78  */
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

  case 832: /* type_declaration_no_options_no_dim: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 833: /* $@79: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 834: /* $@80: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 835: /* type_declaration_no_options_no_dim: "lambda" '<' $@79 type_declaration '>' $@80  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 836: /* $@81: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 837: /* $@82: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 838: /* type_declaration_no_options_no_dim: "lambda" '<' $@81 optional_function_argument_list optional_function_type '>' $@82  */
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

  case 839: /* $@83: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 840: /* $@84: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 841: /* type_declaration_no_options_no_dim: "tuple" '<' $@83 tuple_type_list '>' $@84  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 842: /* $@85: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 843: /* $@86: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 844: /* type_declaration_no_options_no_dim: "variant" '<' $@85 variant_type_list '>' $@86  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 845: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 846: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 847: /* type_declaration: type_declaration '|' '#'  */
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

  case 848: /* $@87: %empty  */
                   {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 849: /* $@88: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 850: /* $@89: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 851: /* $@90: %empty  */
                                                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 852: /* tuple_alias_declaration: "tuple" $@87 optional_public_or_private_alias "name" optional_emit_semis $@88 '{' $@89 tuple_alias_type_list optional_semis $@90 '}'  */
          {
        auto vtype = make_smart<TypeDecl>(Type::tTuple);
        vtype->alias = *(yyvsp[-8].s);
        vtype->at = tokAt(scanner,(yylsp[-8]));
        vtype->isPrivateAlias = !(yyvsp[-9].b);
        varDeclToTypeDecl(scanner, vtype.get(), (yyvsp[-3].pVarDeclList), true);
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

  case 853: /* $@91: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 854: /* $@92: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 855: /* $@93: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 856: /* $@94: %empty  */
                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 857: /* variant_alias_declaration: "variant" $@91 optional_public_or_private_alias "name" optional_emit_semis $@92 '{' $@93 variant_alias_type_list optional_semis $@94 '}'  */
          {
        auto vtype = make_smart<TypeDecl>(Type::tVariant);
        vtype->alias = *(yyvsp[-8].s);
        vtype->at = tokAt(scanner,(yylsp[-8]));
        vtype->isPrivateAlias = !(yyvsp[-9].b);
        varDeclToTypeDecl(scanner, vtype.get(), (yyvsp[-3].pVarDeclList), true);
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

  case 858: /* $@95: %empty  */
                      {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 859: /* $@96: %empty  */
                                                                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 860: /* $@97: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 861: /* $@98: %empty  */
                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-7]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 862: /* bitfield_alias_declaration: "bitfield" $@95 optional_public_or_private_alias "name" bitfield_basic_type_declaration optional_emit_commas $@96 '{' $@97 bitfield_alias_bits optional_commas $@98 '}'  */
          {
        auto btype = make_smart<TypeDecl>((yyvsp[-8].type));
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

  case 863: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 864: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 865: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 866: /* make_decl: make_table_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 867: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 868: /* make_decl: table_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 869: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 870: /* make_decl_no_bracket: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 871: /* make_decl_no_bracket: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 872: /* make_decl_no_bracket: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 873: /* make_decl_no_bracket: table_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 874: /* make_decl_no_bracket: make_table_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 875: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 876: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 877: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 878: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 879: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 880: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 881: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 882: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 883: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 884: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 885: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 886: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 887: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 888: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 889: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 890: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 891: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 892: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 893: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 894: /* $@99: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 895: /* $@100: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 896: /* make_struct_decl: "struct" '<' $@99 type_declaration_no_options '>' $@100 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                      {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 897: /* $@101: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 898: /* $@102: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 899: /* make_struct_decl: "class" '<' $@101 type_declaration_no_options '>' $@102 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                     {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 900: /* $@103: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 901: /* $@104: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 902: /* make_struct_decl: "variant" '<' $@103 variant_type_list '>' $@104 '(' use_initializer make_variant_dim ')'  */
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

  case 903: /* $@105: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 904: /* $@106: %empty  */
                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 905: /* make_struct_decl: "variant" "type" '<' $@105 type_declaration_no_options '>' $@106 '(' use_initializer make_variant_dim ')'  */
                                                                                                                                                                                                    {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-10]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 906: /* $@107: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 907: /* $@108: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 908: /* make_struct_decl: "default" '<' $@107 type_declaration_no_options '>' $@108 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 909: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = make_smart<TypeDecl>(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 910: /* $@109: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 911: /* $@110: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 912: /* make_tuple_call: "tuple" '<' $@109 tuple_type_list '>' $@110 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 913: /* make_dim_decl: '[' optional_expr_list ']'  */
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

  case 914: /* $@111: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 915: /* $@112: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 916: /* make_dim_decl: "array" "struct" '<' $@111 type_declaration_no_options '>' $@112 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 917: /* $@113: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 918: /* $@114: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 919: /* make_dim_decl: "array" "tuple" '<' $@113 tuple_type_list '>' $@114 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 920: /* $@115: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 921: /* $@116: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 922: /* make_dim_decl: "array" "variant" '<' $@115 variant_type_list '>' $@116 '(' make_variant_dim ')'  */
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

  case 923: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
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

  case 924: /* $@117: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 925: /* $@118: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 926: /* make_dim_decl: "array" '<' $@117 type_declaration_no_options '>' $@118 '(' optional_expr_list ')'  */
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

  case 927: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 928: /* $@119: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 929: /* $@120: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 930: /* make_dim_decl: "fixed_array" '<' $@119 type_declaration_no_options '>' $@120 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 931: /* expr_map_tuple_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 932: /* expr_map_tuple_list: expr_map_tuple_list ',' expr  */
                                                      {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 933: /* $@121: %empty  */
                  {
        yyextra->das_nested_parentheses ++;
    }
    break;

  case 934: /* make_table_decl: '{' $@121 optional_emit_semis optional_expr_map_tuple_list '}'  */
                                                                      {
        yyextra->das_nested_parentheses --;
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-4])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
            auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_table_move");
            ttm->arguments.push_back(mka);
            (yyval.pExpression) = ttm;
        } else {
            auto mks = new ExprMakeStruct();
            mks->at = tokAt(scanner,(yylsp[-4]));
            mks->makeType = make_smart<TypeDecl>(Type::tTable);
            mks->makeType->firstType = make_smart<TypeDecl>(Type::autoinfer);
            mks->makeType->secondType = make_smart<TypeDecl>(Type::autoinfer);
            mks->useInitializer = true;
            mks->alwaysUseInitializer = true;
            (yyval.pExpression) = mks;
        }
    }
    break;

  case 935: /* make_table_call: "table" '(' expr_map_tuple_list optional_comma ')'  */
                                                                             {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 936: /* make_table_call: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 937: /* make_table_call: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 938: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 939: /* array_comprehension_where: ';' "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 940: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 941: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 942: /* table_comprehension: '[' "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                               {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 943: /* table_comprehension: '[' "iterator" "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 944: /* array_comprehension: '{' "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where '}'  */
                                                                                                                                                               {
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


