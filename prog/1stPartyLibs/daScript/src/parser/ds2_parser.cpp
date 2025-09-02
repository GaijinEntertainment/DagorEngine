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
  YYSYMBOL_if_or_static_if = 241,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 242, /* expression_else_one_liner  */
  YYSYMBOL_expression_if_one_liner = 243,  /* expression_if_one_liner  */
  YYSYMBOL_semis = 244,                    /* semis  */
  YYSYMBOL_optional_semis = 245,           /* optional_semis  */
  YYSYMBOL_expression_if_block = 246,      /* expression_if_block  */
  YYSYMBOL_247_3 = 247,                    /* $@3  */
  YYSYMBOL_248_4 = 248,                    /* $@4  */
  YYSYMBOL_249_5 = 249,                    /* $@5  */
  YYSYMBOL_expression_else_block = 250,    /* expression_else_block  */
  YYSYMBOL_251_6 = 251,                    /* $@6  */
  YYSYMBOL_252_7 = 252,                    /* $@7  */
  YYSYMBOL_253_8 = 253,                    /* $@8  */
  YYSYMBOL_expression_if_then_else = 254,  /* expression_if_then_else  */
  YYSYMBOL_255_9 = 255,                    /* $@9  */
  YYSYMBOL_expression_if_then_else_oneliner = 256, /* expression_if_then_else_oneliner  */
  YYSYMBOL_for_variable_name_with_pos_list = 257, /* for_variable_name_with_pos_list  */
  YYSYMBOL_expression_for_loop = 258,      /* expression_for_loop  */
  YYSYMBOL_259_10 = 259,                   /* $@10  */
  YYSYMBOL_expression_unsafe = 260,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 261,    /* expression_while_loop  */
  YYSYMBOL_262_11 = 262,                   /* $@11  */
  YYSYMBOL_expression_with = 263,          /* expression_with  */
  YYSYMBOL_264_12 = 264,                   /* $@12  */
  YYSYMBOL_expression_with_alias = 265,    /* expression_with_alias  */
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
  YYSYMBOL_optional_annotation_list_with_emit_semis = 277, /* optional_annotation_list_with_emit_semis  */
  YYSYMBOL_optional_function_argument_list = 278, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 279,   /* optional_function_type  */
  YYSYMBOL_function_name = 280,            /* function_name  */
  YYSYMBOL_global_function_declaration = 281, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 282, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 283, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 284,     /* function_declaration  */
  YYSYMBOL_285_13 = 285,                   /* $@13  */
  YYSYMBOL_expression_block_finally = 286, /* expression_block_finally  */
  YYSYMBOL_287_14 = 287,                   /* $@14  */
  YYSYMBOL_288_15 = 288,                   /* $@15  */
  YYSYMBOL_expression_block = 289,         /* expression_block  */
  YYSYMBOL_290_16 = 290,                   /* $@16  */
  YYSYMBOL_291_17 = 291,                   /* $@17  */
  YYSYMBOL_expr_call_pipe = 292,           /* expr_call_pipe  */
  YYSYMBOL_expression_any = 293,           /* expression_any  */
  YYSYMBOL_expressions = 294,              /* expressions  */
  YYSYMBOL_optional_expr_list = 295,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_map_tuple_list = 296, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 297, /* type_declaration_no_options_list  */
  YYSYMBOL_name_in_namespace = 298,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 299,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 300,     /* new_type_declaration  */
  YYSYMBOL_301_18 = 301,                   /* $@18  */
  YYSYMBOL_302_19 = 302,                   /* $@19  */
  YYSYMBOL_expr_new = 303,                 /* expr_new  */
  YYSYMBOL_expression_break = 304,         /* expression_break  */
  YYSYMBOL_expression_continue = 305,      /* expression_continue  */
  YYSYMBOL_expression_return = 306,        /* expression_return  */
  YYSYMBOL_expression_yield = 307,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 308,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 309,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 310,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 311,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 312,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 313, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 314,           /* expression_let  */
  YYSYMBOL_expr_cast = 315,                /* expr_cast  */
  YYSYMBOL_316_20 = 316,                   /* $@20  */
  YYSYMBOL_317_21 = 317,                   /* $@21  */
  YYSYMBOL_318_22 = 318,                   /* $@22  */
  YYSYMBOL_319_23 = 319,                   /* $@23  */
  YYSYMBOL_320_24 = 320,                   /* $@24  */
  YYSYMBOL_321_25 = 321,                   /* $@25  */
  YYSYMBOL_expr_type_decl = 322,           /* expr_type_decl  */
  YYSYMBOL_323_26 = 323,                   /* $@26  */
  YYSYMBOL_324_27 = 324,                   /* $@27  */
  YYSYMBOL_expr_type_info = 325,           /* expr_type_info  */
  YYSYMBOL_expr_list = 326,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 327,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 328,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 329,            /* capture_entry  */
  YYSYMBOL_capture_list = 330,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 331,    /* optional_capture_list  */
  YYSYMBOL_expr_full_block = 332,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 333, /* expr_full_block_assumed_piped  */
  YYSYMBOL_expr_numeric_const = 334,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign = 335,              /* expr_assign  */
  YYSYMBOL_expr_named_call = 336,          /* expr_named_call  */
  YYSYMBOL_expr_method_call = 337,         /* expr_method_call  */
  YYSYMBOL_func_addr_name = 338,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 339,           /* func_addr_expr  */
  YYSYMBOL_340_28 = 340,                   /* $@28  */
  YYSYMBOL_341_29 = 341,                   /* $@29  */
  YYSYMBOL_342_30 = 342,                   /* $@30  */
  YYSYMBOL_343_31 = 343,                   /* $@31  */
  YYSYMBOL_expr_field = 344,               /* expr_field  */
  YYSYMBOL_345_32 = 345,                   /* $@32  */
  YYSYMBOL_346_33 = 346,                   /* $@33  */
  YYSYMBOL_expr_call = 347,                /* expr_call  */
  YYSYMBOL_expr = 348,                     /* expr  */
  YYSYMBOL_349_34 = 349,                   /* $@34  */
  YYSYMBOL_350_35 = 350,                   /* $@35  */
  YYSYMBOL_351_36 = 351,                   /* $@36  */
  YYSYMBOL_352_37 = 352,                   /* $@37  */
  YYSYMBOL_353_38 = 353,                   /* $@38  */
  YYSYMBOL_354_39 = 354,                   /* $@39  */
  YYSYMBOL_expr_generator = 355,           /* expr_generator  */
  YYSYMBOL_expr_mtag = 356,                /* expr_mtag  */
  YYSYMBOL_optional_field_annotation = 357, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 358,        /* optional_override  */
  YYSYMBOL_optional_constant = 359,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 360, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 361, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 362, /* structure_variable_declaration  */
  YYSYMBOL_struct_variable_declaration_list = 363, /* struct_variable_declaration_list  */
  YYSYMBOL_364_40 = 364,                   /* $@40  */
  YYSYMBOL_365_41 = 365,                   /* $@41  */
  YYSYMBOL_366_42 = 366,                   /* $@42  */
  YYSYMBOL_function_argument_declaration_no_type = 367, /* function_argument_declaration_no_type  */
  YYSYMBOL_function_argument_declaration_type = 368, /* function_argument_declaration_type  */
  YYSYMBOL_function_argument_list = 369,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 370,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 371,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 372,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 373,             /* variant_type  */
  YYSYMBOL_variant_type_list = 374,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 375,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 376,             /* copy_or_move  */
  YYSYMBOL_variable_declaration_no_type = 377, /* variable_declaration_no_type  */
  YYSYMBOL_variable_declaration_type = 378, /* variable_declaration_type  */
  YYSYMBOL_variable_declaration = 379,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 380,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 381,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 382, /* let_variable_name_with_pos_list  */
  YYSYMBOL_global_let_variable_name_with_pos_list = 383, /* global_let_variable_name_with_pos_list  */
  YYSYMBOL_variable_declaration_list = 384, /* variable_declaration_list  */
  YYSYMBOL_let_variable_declaration = 385, /* let_variable_declaration  */
  YYSYMBOL_global_let_variable_declaration = 386, /* global_let_variable_declaration  */
  YYSYMBOL_optional_shared = 387,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 388, /* optional_public_or_private_variable  */
  YYSYMBOL_global_variable_declaration_list = 389, /* global_variable_declaration_list  */
  YYSYMBOL_390_43 = 390,                   /* $@43  */
  YYSYMBOL_global_let = 391,               /* global_let  */
  YYSYMBOL_392_44 = 392,                   /* $@44  */
  YYSYMBOL_enum_expression = 393,          /* enum_expression  */
  YYSYMBOL_commas = 394,                   /* commas  */
  YYSYMBOL_enum_list = 395,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 396, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 397,             /* single_alias  */
  YYSYMBOL_398_45 = 398,                   /* $@45  */
  YYSYMBOL_alias_declaration = 399,        /* alias_declaration  */
  YYSYMBOL_optional_public_or_private_enum = 400, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 401,                /* enum_name  */
  YYSYMBOL_optional_enum_basic_type_declaration = 402, /* optional_enum_basic_type_declaration  */
  YYSYMBOL_optional_commas = 403,          /* optional_commas  */
  YYSYMBOL_emit_commas = 404,              /* emit_commas  */
  YYSYMBOL_optional_emit_commas = 405,     /* optional_emit_commas  */
  YYSYMBOL_enum_declaration = 406,         /* enum_declaration  */
  YYSYMBOL_407_46 = 407,                   /* $@46  */
  YYSYMBOL_408_47 = 408,                   /* $@47  */
  YYSYMBOL_409_48 = 409,                   /* $@48  */
  YYSYMBOL_optional_structure_parent = 410, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 411,          /* optional_sealed  */
  YYSYMBOL_structure_name = 412,           /* structure_name  */
  YYSYMBOL_class_or_struct = 413,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 414, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 415, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 416,    /* structure_declaration  */
  YYSYMBOL_417_49 = 417,                   /* $@49  */
  YYSYMBOL_418_50 = 418,                   /* $@50  */
  YYSYMBOL_419_51 = 419,                   /* $@51  */
  YYSYMBOL_variable_name_with_pos_list = 420, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 421,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 422, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 423, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 424,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 425,            /* bitfield_bits  */
  YYSYMBOL_bitfield_alias_bits = 426,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_type_declaration = 427, /* bitfield_type_declaration  */
  YYSYMBOL_428_52 = 428,                   /* $@52  */
  YYSYMBOL_429_53 = 429,                   /* $@53  */
  YYSYMBOL_c_or_s = 430,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 431,          /* table_type_pair  */
  YYSYMBOL_dim_list = 432,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 433, /* type_declaration_no_options  */
  YYSYMBOL_type_declaration_no_options_no_dim = 434, /* type_declaration_no_options_no_dim  */
  YYSYMBOL_435_54 = 435,                   /* $@54  */
  YYSYMBOL_436_55 = 436,                   /* $@55  */
  YYSYMBOL_437_56 = 437,                   /* $@56  */
  YYSYMBOL_438_57 = 438,                   /* $@57  */
  YYSYMBOL_439_58 = 439,                   /* $@58  */
  YYSYMBOL_440_59 = 440,                   /* $@59  */
  YYSYMBOL_441_60 = 441,                   /* $@60  */
  YYSYMBOL_442_61 = 442,                   /* $@61  */
  YYSYMBOL_443_62 = 443,                   /* $@62  */
  YYSYMBOL_444_63 = 444,                   /* $@63  */
  YYSYMBOL_445_64 = 445,                   /* $@64  */
  YYSYMBOL_446_65 = 446,                   /* $@65  */
  YYSYMBOL_447_66 = 447,                   /* $@66  */
  YYSYMBOL_448_67 = 448,                   /* $@67  */
  YYSYMBOL_449_68 = 449,                   /* $@68  */
  YYSYMBOL_450_69 = 450,                   /* $@69  */
  YYSYMBOL_451_70 = 451,                   /* $@70  */
  YYSYMBOL_452_71 = 452,                   /* $@71  */
  YYSYMBOL_453_72 = 453,                   /* $@72  */
  YYSYMBOL_454_73 = 454,                   /* $@73  */
  YYSYMBOL_455_74 = 455,                   /* $@74  */
  YYSYMBOL_456_75 = 456,                   /* $@75  */
  YYSYMBOL_457_76 = 457,                   /* $@76  */
  YYSYMBOL_458_77 = 458,                   /* $@77  */
  YYSYMBOL_459_78 = 459,                   /* $@78  */
  YYSYMBOL_460_79 = 460,                   /* $@79  */
  YYSYMBOL_461_80 = 461,                   /* $@80  */
  YYSYMBOL_type_declaration = 462,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 463,  /* tuple_alias_declaration  */
  YYSYMBOL_464_81 = 464,                   /* $@81  */
  YYSYMBOL_465_82 = 465,                   /* $@82  */
  YYSYMBOL_466_83 = 466,                   /* $@83  */
  YYSYMBOL_467_84 = 467,                   /* $@84  */
  YYSYMBOL_variant_alias_declaration = 468, /* variant_alias_declaration  */
  YYSYMBOL_469_85 = 469,                   /* $@85  */
  YYSYMBOL_470_86 = 470,                   /* $@86  */
  YYSYMBOL_471_87 = 471,                   /* $@87  */
  YYSYMBOL_472_88 = 472,                   /* $@88  */
  YYSYMBOL_bitfield_alias_declaration = 473, /* bitfield_alias_declaration  */
  YYSYMBOL_474_89 = 474,                   /* $@89  */
  YYSYMBOL_475_90 = 475,                   /* $@90  */
  YYSYMBOL_476_91 = 476,                   /* $@91  */
  YYSYMBOL_477_92 = 477,                   /* $@92  */
  YYSYMBOL_make_decl = 478,                /* make_decl  */
  YYSYMBOL_make_struct_fields = 479,       /* make_struct_fields  */
  YYSYMBOL_make_variant_dim = 480,         /* make_variant_dim  */
  YYSYMBOL_make_struct_single = 481,       /* make_struct_single  */
  YYSYMBOL_make_struct_dim_list = 482,     /* make_struct_dim_list  */
  YYSYMBOL_make_struct_dim_decl = 483,     /* make_struct_dim_decl  */
  YYSYMBOL_optional_make_struct_dim_decl = 484, /* optional_make_struct_dim_decl  */
  YYSYMBOL_use_initializer = 485,          /* use_initializer  */
  YYSYMBOL_make_struct_decl = 486,         /* make_struct_decl  */
  YYSYMBOL_487_93 = 487,                   /* $@93  */
  YYSYMBOL_488_94 = 488,                   /* $@94  */
  YYSYMBOL_489_95 = 489,                   /* $@95  */
  YYSYMBOL_490_96 = 490,                   /* $@96  */
  YYSYMBOL_491_97 = 491,                   /* $@97  */
  YYSYMBOL_492_98 = 492,                   /* $@98  */
  YYSYMBOL_493_99 = 493,                   /* $@99  */
  YYSYMBOL_494_100 = 494,                  /* $@100  */
  YYSYMBOL_make_map_tuple = 495,           /* make_map_tuple  */
  YYSYMBOL_make_tuple_call = 496,          /* make_tuple_call  */
  YYSYMBOL_497_101 = 497,                  /* $@101  */
  YYSYMBOL_498_102 = 498,                  /* $@102  */
  YYSYMBOL_make_dim_decl = 499,            /* make_dim_decl  */
  YYSYMBOL_500_103 = 500,                  /* $@103  */
  YYSYMBOL_501_104 = 501,                  /* $@104  */
  YYSYMBOL_502_105 = 502,                  /* $@105  */
  YYSYMBOL_503_106 = 503,                  /* $@106  */
  YYSYMBOL_504_107 = 504,                  /* $@107  */
  YYSYMBOL_505_108 = 505,                  /* $@108  */
  YYSYMBOL_506_109 = 506,                  /* $@109  */
  YYSYMBOL_507_110 = 507,                  /* $@110  */
  YYSYMBOL_508_111 = 508,                  /* $@111  */
  YYSYMBOL_509_112 = 509,                  /* $@112  */
  YYSYMBOL_expr_map_tuple_list = 510,      /* expr_map_tuple_list  */
  YYSYMBOL_make_table_decl = 511,          /* make_table_decl  */
  YYSYMBOL_512_113 = 512,                  /* $@113  */
  YYSYMBOL_array_comprehension_where = 513, /* array_comprehension_where  */
  YYSYMBOL_optional_comma = 514,           /* optional_comma  */
  YYSYMBOL_array_comprehension = 515       /* array_comprehension  */
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
#define YYLAST   12062

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  208
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  308
/* YYNRULES -- Number of rules.  */
#define YYNRULES  880
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1589

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
       0,   560,   560,   561,   566,   567,   568,   569,   570,   571,
     572,   573,   574,   575,   576,   577,   578,   582,   583,   587,
     588,   592,   598,   599,   600,   604,   605,   609,   610,   614,
     633,   634,   635,   636,   640,   641,   645,   646,   650,   651,
     651,   655,   660,   669,   684,   700,   705,   713,   713,   752,
     770,   774,   777,   781,   787,   796,   799,   805,   806,   810,
     814,   815,   819,   822,   828,   834,   837,   843,   844,   848,
     849,   853,   854,   858,   859,   860,   868,   869,   873,   874,
     880,   881,   882,   883,   884,   888,   889,   893,   894,   898,
     900,   898,   912,   912,   920,   922,   920,   934,   934,   942,
     942,   954,   961,   968,   973,   982,   990,   996,  1004,  1014,
    1014,  1023,  1031,  1031,  1044,  1044,  1056,  1063,  1064,  1065,
    1066,  1067,  1068,  1072,  1077,  1085,  1086,  1087,  1091,  1092,
    1093,  1094,  1095,  1096,  1097,  1098,  1104,  1107,  1113,  1116,
    1122,  1123,  1124,  1125,  1129,  1142,  1160,  1163,  1171,  1182,
    1193,  1204,  1207,  1214,  1218,  1225,  1226,  1230,  1231,  1235,
    1236,  1237,  1241,  1244,  1251,  1255,  1256,  1257,  1258,  1259,
    1260,  1261,  1262,  1263,  1264,  1265,  1266,  1267,  1268,  1269,
    1270,  1271,  1272,  1273,  1274,  1275,  1276,  1277,  1278,  1279,
    1280,  1281,  1282,  1283,  1284,  1285,  1286,  1287,  1288,  1289,
    1290,  1291,  1292,  1293,  1294,  1295,  1296,  1297,  1298,  1299,
    1300,  1301,  1302,  1303,  1304,  1305,  1306,  1307,  1308,  1309,
    1310,  1311,  1312,  1313,  1314,  1315,  1316,  1317,  1318,  1319,
    1320,  1321,  1322,  1323,  1324,  1325,  1326,  1327,  1328,  1329,
    1330,  1331,  1332,  1333,  1334,  1335,  1336,  1337,  1338,  1339,
    1340,  1341,  1342,  1343,  1344,  1345,  1346,  1347,  1348,  1349,
    1354,  1372,  1373,  1374,  1378,  1384,  1384,  1401,  1404,  1406,
    1404,  1414,  1416,  1414,  1431,  1444,  1445,  1446,  1447,  1448,
    1449,  1450,  1451,  1452,  1453,  1454,  1455,  1456,  1457,  1458,
    1459,  1460,  1461,  1462,  1466,  1471,  1477,  1483,  1484,  1488,
    1489,  1493,  1497,  1504,  1505,  1516,  1520,  1523,  1531,  1531,
    1531,  1534,  1540,  1543,  1547,  1551,  1558,  1565,  1571,  1575,
    1579,  1582,  1585,  1593,  1596,  1604,  1610,  1611,  1612,  1616,
    1617,  1621,  1622,  1626,  1631,  1639,  1645,  1657,  1660,  1663,
    1669,  1669,  1669,  1672,  1672,  1672,  1677,  1677,  1677,  1685,
    1685,  1685,  1691,  1701,  1712,  1727,  1730,  1736,  1737,  1744,
    1755,  1756,  1757,  1761,  1762,  1763,  1764,  1765,  1769,  1774,
    1782,  1783,  1787,  1794,  1798,  1804,  1805,  1806,  1807,  1808,
    1809,  1810,  1814,  1815,  1816,  1817,  1818,  1819,  1820,  1821,
    1822,  1823,  1824,  1825,  1826,  1827,  1828,  1829,  1830,  1831,
    1832,  1836,  1843,  1855,  1860,  1870,  1874,  1881,  1884,  1884,
    1884,  1889,  1889,  1889,  1902,  1906,  1910,  1915,  1922,  1931,
    1936,  1943,  1943,  1943,  1950,  1954,  1963,  1971,  1979,  1983,
    1986,  1992,  1993,  1994,  1995,  1996,  1997,  1998,  1999,  2000,
    2001,  2002,  2003,  2004,  2005,  2006,  2007,  2008,  2009,  2010,
    2011,  2012,  2013,  2014,  2015,  2016,  2017,  2018,  2019,  2020,
    2021,  2022,  2023,  2024,  2025,  2026,  2027,  2033,  2034,  2035,
    2036,  2037,  2050,  2059,  2060,  2061,  2062,  2063,  2064,  2065,
    2066,  2067,  2068,  2069,  2070,  2071,  2072,  2075,  2075,  2075,
    2078,  2083,  2087,  2091,  2091,  2091,  2096,  2099,  2103,  2103,
    2103,  2108,  2111,  2112,  2113,  2114,  2115,  2116,  2117,  2118,
    2119,  2121,  2125,  2126,  2134,  2137,  2140,  2149,  2150,  2151,
    2152,  2153,  2154,  2155,  2159,  2163,  2167,  2171,  2175,  2179,
    2183,  2187,  2191,  2198,  2199,  2203,  2204,  2205,  2209,  2210,
    2214,  2215,  2216,  2220,  2221,  2225,  2236,  2239,  2240,  2240,
    2259,  2258,  2273,  2272,  2289,  2301,  2310,  2320,  2321,  2322,
    2323,  2324,  2328,  2331,  2340,  2341,  2345,  2348,  2352,  2366,
    2375,  2376,  2380,  2383,  2387,  2401,  2402,  2406,  2412,  2418,
    2427,  2430,  2437,  2440,  2446,  2447,  2448,  2452,  2453,  2457,
    2464,  2469,  2478,  2484,  2495,  2502,  2511,  2514,  2517,  2524,
    2527,  2532,  2543,  2546,  2551,  2563,  2564,  2568,  2569,  2570,
    2574,  2577,  2580,  2580,  2600,  2603,  2603,  2621,  2626,  2634,
    2635,  2639,  2642,  2655,  2672,  2673,  2674,  2679,  2679,  2705,
    2709,  2710,  2711,  2715,  2725,  2728,  2734,  2735,  2739,  2740,
    2744,  2745,  2749,  2751,  2756,  2749,  2772,  2773,  2777,  2778,
    2782,  2788,  2789,  2790,  2791,  2795,  2796,  2797,  2801,  2804,
    2810,  2812,  2817,  2810,  2838,  2845,  2850,  2859,  2865,  2876,
    2877,  2878,  2879,  2880,  2881,  2882,  2883,  2884,  2885,  2886,
    2887,  2888,  2889,  2890,  2891,  2892,  2893,  2894,  2895,  2896,
    2897,  2898,  2899,  2900,  2901,  2902,  2906,  2907,  2908,  2909,
    2910,  2911,  2912,  2913,  2917,  2928,  2932,  2939,  2951,  2958,
    2967,  2972,  2982,  2995,  2995,  2995,  3008,  3009,  3013,  3017,
    3024,  3028,  3032,  3036,  3043,  3046,  3064,  3065,  3066,  3067,
    3068,  3068,  3068,  3072,  3077,  3084,  3084,  3091,  3095,  3099,
    3104,  3109,  3114,  3119,  3123,  3127,  3132,  3136,  3140,  3145,
    3145,  3145,  3151,  3158,  3158,  3158,  3163,  3163,  3163,  3169,
    3169,  3169,  3174,  3179,  3179,  3179,  3184,  3184,  3184,  3193,
    3198,  3198,  3198,  3203,  3203,  3203,  3212,  3217,  3217,  3217,
    3222,  3222,  3222,  3231,  3231,  3231,  3237,  3237,  3237,  3246,
    3249,  3260,  3276,  3278,  3283,  3288,  3276,  3314,  3316,  3321,
    3327,  3314,  3353,  3355,  3360,  3365,  3353,  3396,  3397,  3398,
    3399,  3400,  3404,  3411,  3418,  3424,  3430,  3437,  3444,  3450,
    3459,  3462,  3468,  3476,  3481,  3488,  3493,  3499,  3500,  3504,
    3505,  3509,  3509,  3509,  3517,  3517,  3517,  3524,  3524,  3524,
    3535,  3535,  3535,  3546,  3552,  3558,  3564,  3564,  3564,  3578,
    3597,  3597,  3597,  3607,  3607,  3607,  3621,  3621,  3621,  3635,
    3644,  3644,  3644,  3664,  3671,  3671,  3671,  3681,  3684,  3690,
    3690,  3712,  3720,  3740,  3765,  3766,  3770,  3771,  3775,  3778,
    3781
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
  "optional_emit_semis", "expression_else", "if_or_static_if",
  "expression_else_one_liner", "expression_if_one_liner", "semis",
  "optional_semis", "expression_if_block", "$@3", "$@4", "$@5",
  "expression_else_block", "$@6", "$@7", "$@8", "expression_if_then_else",
  "$@9", "expression_if_then_else_oneliner",
  "for_variable_name_with_pos_list", "expression_for_loop", "$@10",
  "expression_unsafe", "expression_while_loop", "$@11", "expression_with",
  "$@12", "expression_with_alias", "annotation_argument_value",
  "annotation_argument_value_list", "annotation_argument_name",
  "annotation_argument", "annotation_argument_list",
  "metadata_argument_list", "annotation_declaration_name",
  "annotation_declaration_basic", "annotation_declaration",
  "annotation_list", "optional_annotation_list",
  "optional_annotation_list_with_emit_semis",
  "optional_function_argument_list", "optional_function_type",
  "function_name", "global_function_declaration",
  "optional_public_or_private_function", "function_declaration_header",
  "function_declaration", "$@13", "expression_block_finally", "$@14",
  "$@15", "expression_block", "$@16", "$@17", "expr_call_pipe",
  "expression_any", "expressions", "optional_expr_list",
  "optional_expr_map_tuple_list", "type_declaration_no_options_list",
  "name_in_namespace", "expression_delete", "new_type_declaration", "$@18",
  "$@19", "expr_new", "expression_break", "expression_continue",
  "expression_return", "expression_yield", "expression_try_catch",
  "kwd_let_var_or_nothing", "kwd_let", "optional_in_scope",
  "tuple_expansion", "tuple_expansion_variable_declaration",
  "expression_let", "expr_cast", "$@20", "$@21", "$@22", "$@23", "$@24",
  "$@25", "expr_type_decl", "$@26", "$@27", "expr_type_info", "expr_list",
  "block_or_simple_block", "block_or_lambda", "capture_entry",
  "capture_list", "optional_capture_list", "expr_full_block",
  "expr_full_block_assumed_piped", "expr_numeric_const", "expr_assign",
  "expr_named_call", "expr_method_call", "func_addr_name",
  "func_addr_expr", "$@28", "$@29", "$@30", "$@31", "expr_field", "$@32",
  "$@33", "expr_call", "expr", "$@34", "$@35", "$@36", "$@37", "$@38",
  "$@39", "expr_generator", "expr_mtag", "optional_field_annotation",
  "optional_override", "optional_constant",
  "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "struct_variable_declaration_list", "$@40", "$@41", "$@42",
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
  "global_variable_declaration_list", "$@43", "global_let", "$@44",
  "enum_expression", "commas", "enum_list",
  "optional_public_or_private_alias", "single_alias", "$@45",
  "alias_declaration", "optional_public_or_private_enum", "enum_name",
  "optional_enum_basic_type_declaration", "optional_commas", "emit_commas",
  "optional_emit_commas", "enum_declaration", "$@46", "$@47", "$@48",
  "optional_structure_parent", "optional_sealed", "structure_name",
  "class_or_struct", "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@49", "$@50", "$@51", "variable_name_with_pos_list",
  "basic_type_declaration", "enum_basic_type_declaration",
  "structure_type_declaration", "auto_type_declaration", "bitfield_bits",
  "bitfield_alias_bits", "bitfield_type_declaration", "$@52", "$@53",
  "c_or_s", "table_type_pair", "dim_list", "type_declaration_no_options",
  "type_declaration_no_options_no_dim", "$@54", "$@55", "$@56", "$@57",
  "$@58", "$@59", "$@60", "$@61", "$@62", "$@63", "$@64", "$@65", "$@66",
  "$@67", "$@68", "$@69", "$@70", "$@71", "$@72", "$@73", "$@74", "$@75",
  "$@76", "$@77", "$@78", "$@79", "$@80", "type_declaration",
  "tuple_alias_declaration", "$@81", "$@82", "$@83", "$@84",
  "variant_alias_declaration", "$@85", "$@86", "$@87", "$@88",
  "bitfield_alias_declaration", "$@89", "$@90", "$@91", "$@92",
  "make_decl", "make_struct_fields", "make_variant_dim",
  "make_struct_single", "make_struct_dim_list", "make_struct_dim_decl",
  "optional_make_struct_dim_decl", "use_initializer", "make_struct_decl",
  "$@93", "$@94", "$@95", "$@96", "$@97", "$@98", "$@99", "$@100",
  "make_map_tuple", "make_tuple_call", "$@101", "$@102", "make_dim_decl",
  "$@103", "$@104", "$@105", "$@106", "$@107", "$@108", "$@109", "$@110",
  "$@111", "$@112", "expr_map_tuple_list", "make_table_decl", "$@113",
  "array_comprehension_where", "optional_comma", "array_comprehension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1406)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-781)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1406,    68, -1406, -1406,    28,   -45,   155,   448, -1406,   -53,
   -1406, -1406, -1406, -1406,   142,   258, -1406, -1406, -1406, -1406,
     -66,   -66,   -66, -1406,   240, -1406,    61, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406,   -38, -1406,    -6,
     -23,    51, -1406, -1406,   155,    31, -1406, -1406, -1406,    66,
     -66, -1406, -1406,    61,   448,   448,   448,   125,   180, -1406,
   -1406, -1406, -1406,   258,   258,   258,   172, -1406,   700,   202,
   -1406, -1406, -1406, -1406,   510, -1406,    74, -1406,   514,    41,
      28,   199,   -45,   190,   288, -1406,   308,   310, -1406, -1406,
   -1406,   533,   318,   334,   381, -1406,   394,   357, -1406, -1406,
     -68,    28,   258,   258,   258,   258,   426, -1406, -1406, -1406,
   -1406,   615, -1406, -1406,   531,   623, -1406, -1406,   416, -1406,
   -1406, -1406, -1406, -1406,   711,   112, -1406, -1406, -1406, -1406,
     554, -1406, -1406,   455, -1406, -1406,   446,   539,   426,   426,
   -1406, -1406,   547, -1406,   110, -1406,   -33,   614,   700, -1406,
     578, -1406, 10953, -1406, -1406,   620, -1406, -1406, -1406, -1406,
   -1406, -1406,   557, -1406, -1406, -1406,   735, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406,   114, -1406,  7439,   697, -1406, -1406,
     624, -1406, -1406, -1406, -1406, -1406, -1406, 11065, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406,   759,   769, -1406,   596,   426, -1406,   626,
     702,   274,    28,   630,   686, -1406, -1406, -1406,   112, -1406,
     687,   688,   695,   685,   710,   716, -1406, -1406, -1406,   708,
   -1406, -1406, -1406, -1406, -1406,   717, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406,   730, -1406, -1406,
   -1406,   732,   741, -1406, -1406, -1406, -1406,   746,   747,   728,
     142, -1406, -1406, -1406, -1406, -1406,   338,   731,   753, -1406,
   -1406,   727,   729,   734,   772,   773, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406,   781,   748, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
     923, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406,   787,   752, -1406, -1406,  -107,   776, -1406,
     937,   539, -1406,   790,   426, -1406, -1406,   557,   426,    28,
   -1406,   506, -1406, -1406, -1406, -1406, -1406,  7007, -1406, -1406,
     791, -1406,   488,   489,   511, -1406, -1406,  7007,   -15, -1406,
   -1406, -1406,    27, -1406, -1406, -1406,    38, -1406,  4142,   757,
    1432, -1406, -1406, -1406, -1406, -1406, -1406, -1406,   794,   744,
   -1406,   758, -1406,    69,   755,   -92,   762,  7439, -1406, -1406,
     760, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
     764,   796, -1406,    77, -1406,   426,   807,  7439, -1406,   136,
    7439,  7439,  7439,   793,   795, -1406, -1406,    72,   142,   797,
      59, -1406,    58,   778,   799,   803,   784,   805,   786,   188,
     808, -1406,   222,   809,   811,  7007,  7007,   800,   801,   806,
     815,   819,   820, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406,  7007,  7007,  7007,  7007,  7007,  3760,  4333, -1406,
     818,   963, -1406, -1406, -1406,   822, -1406, -1406, -1406, -1406,
     789, -1406, -1406, -1406, -1406, -1406, -1406,   681,  1943, -1406,
   -1406,   823, -1406, -1406, -1406, -1406, -1406, -1406,  7439,  7439,
     826,   861,  7439,   596,  7439,   596,  7439,   596,  7532,   865,
    7621, -1406,  7007, -1406, -1406, -1406, -1406,   830, -1406, -1406,
   10545,  4524, -1406,   338,   870,  7532,   865, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
    7007, -1406, -1406,   175,   -97,   -97,   -97, -1406,   753, -1406,
   -1406,   142, -1406,   656,   832,   991,   644, -1406, -1406, -1406,
     640, -1406, -1406, -1406,  7007,   346,   432,   858,   377, -1406,
   -1406, -1406,   841, -1406, -1406,   244, -1406,   862,   863,   864,
   -1406,  7007,  7439,  7007,  7007, -1406, -1406,  7007, -1406,  7007,
   -1406,  7007, -1406, -1406,  7007, -1406,  7439,    64,    64,  7007,
    7007,  7007,  7007,  7007,  7007,   696,    64,    64,   -46,    64,
      64,   845,  1033,   848,   875, 11397,   851,   298,   875,   880,
     853,   351,   852,   426,  3378,   258,  1051,   854, -1406,   789,
   -1406, 11220, 11386,  7007,  7007, -1406, -1406,  7007,  7007,  7007,
    7007,   893,  7007,   284,  7007,  7007,  7007,  7007,  7007,  7007,
    7007,  7007,  7007,  4715,  7007,  7007,  7007,  7007,  7007,  7007,
    7007,  7007,  7007,  7007, 11864,  7007, -1406,  4906,   451,   481,
   -1406, -1406,   120,   555,   776,   588,   776,   597,   776,   -10,
   -1406,   311,   753,   895, -1406,   391, -1406,  7439,   871, -1406,
   -1406, -1406, 10629, -1406,   330, -1406,   -66, -1406,   -66,  7707,
     873,  1031, -1406, -1406,   237, -1406, -1406, -1406,  2161,   912,
   -1406, -1406, -1406, -1406,  7007,   915,   916,  7439,   136, -1406,
    7007,   972, -1406, -1406, -1406, -1406, -1406, -1406, -1406,  7439,
    7439,  7439,  7439,  3951,   917,  7007,  7439, -1406, -1406, -1406,
    7439,   875,   502, 11148, -1406,   910,  7793,  7439,  7439,  7879,
    7439,  7965,  7439,   875,  7439,  7532,   875,   865,   253,  8051,
    8137,  8223,  8309,  8395,  8481, -1406,  7007,   726,   116,   886,
   -1406,  7007, -1406,  7007, -1406,  7007, -1406,  7007,   898,   477,
   -1406, -1406,   887,   894,   529, -1406, -1406,   116,  7007,   -59,
    3569, -1406,   133,   904,   206,   908,   596, -1406,  2367,  1051,
     929,   911, -1406, -1406,   930,   919, -1406, -1406,   627,   627,
    1212,  1212,  1815,  1815,   920,   221,   921, -1406, 10713,   164,
     164,   823,   627,   627, 11627, 11512, 11595, 11480, 11775, 11231,
   11710,  1176,   879,  1212,  1212,   324,   324,   221,   221,   221,
     496,  7007,   922,   924,   559,  7007,  1114,   926, 10797, -1406,
     147, -1406, -1406, -1406,   953, -1406,   936, -1406,   942, -1406,
     947,  7439, -1406,  7532,  7439, -1406,   865,   420,   753, -1406,
   -1406, -1406,   567, -1406, -1406,  7344, -1406,   -20, -1406, -1406,
    7007,   966,   967,  7439, -1406,  7007, -1406,  5097, -1406, -1406,
    5288,   968, -1406,   -66,   977,  5479,     3,  5670, -1406,   -66,
     -66,  1124, -1406,   761, -1406, -1406,  1122, -1406, -1406,  1131,
   -1406,  1099,   -66,   939, -1406,   -66,   -66,   -66,   -66,   -66,
   -1406,  1076, -1406,   -66,  1742,   969, -1406,   330,    21,  8567,
   -1406,  1103,   640,  7007,   972, -1406, -1406, -1406, -1406,   753,
     600,   767,   603,   584,   276,   958,   959,   441,  8653,   653,
    7439,  7532,   865,   785,   960,   948,  7439,  7007,  7007,   961,
   -1406,   957,  1265, -1406,  1604, -1406,  1637,   964,  1924,   490,
     965,   612,  1051, -1406, -1406, -1406, -1406, -1406,   971, 11314,
     973,  1129,  1004,    25,   116, 11397,  8739, 11397, 11397, -1406,
     974,   138,  7007,  7007,  7439,   596,    26,   975,   910,   170,
   -1406,   976,   327,  7198, -1406, -1406, -1406,   300,   776, -1406,
     596, -1406,  7007, -1406,  7007,  5861,  7007, -1406,   995,   978,
   -1406, -1406,  7007,   979, -1406, 10881,  7007,  6052,   980, -1406,
   10965, -1406,  6243, -1406,  7007, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406,   753, -1406, -1406,   753,
   -1406, -1406,   983,  7439, -1406, -1406,   984, -1406, -1406,   988,
   -1406,   989,  8825, -1406,  1142,   -40, 11397,  7007, 11397,  1172,
    7007, 11397,  1015, -1406,  1020,  1043, 11397, -1406,  7007, 11397,
   -1406, -1406,  1000, -1406, -1406,  1001,  1002,  1009,  1010, -1406,
    1174, -1406, -1406, -1406, -1406, -1406, -1406,   154, -1406,  7007,
    7007,  7007,  7007,  7007,  7007,  7007,  7007,  7007,  7007,  7007,
    7007,  7007,  7007,  7007,  7007,  7007,  7007,  7007,   628, -1406,
   -1406, -1406,  1204,   557, -1406,  1050, -1406,  7007,   972, -1406,
   -1406, -1406, -1406,  1013, -1406, -1406, -1406,  1016,  1052, -1406,
   -1406,  3120,   622,   625, -1406, -1406,  7007,  3757, 11397, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
      56,  6434, -1406,  1046,  7007,  1055, -1406,   286,  7007,   -70,
      42,   267,  7007,  7007,  7007,  8911,  8997,  3948,   776,  7007,
   -1406, -1406, -1406,   477,  1018,  3569,  1057,  1059,  1023,  1061,
    1062, -1406,   295,   426,   776,  7439,  9083,  7439,  9169, -1406,
     297,  9255, -1406,  7007,  1560,  7007, -1406,  9341,  3569, -1406,
     328,  7007, -1406, -1406, -1406,   333, -1406, -1406, -1406,  7007,
     753, -1406, -1406, -1406, -1406,  1063,  7007, 11397, -1406, 11397,
    7007, -1406, -1406, -1406, 11397,  7007,  7007,   116,  7007,  7007,
   -1406, -1406,  1004, -1406, -1406, -1406, 11397, 11397, 11397, 11397,
   11397, 11397, 11397, 11397, 11397, 11397, 11397, 11397, 11397, 11397,
   11397, 11397, 11397, 11397, 11397, -1406,  1021,   639,  1161,   -66,
   -1406,   972, -1406,  1029,  1030, -1406, -1406,  7007,  1054, -1406,
   -1406, -1406, -1406,  1032,  1035,  1039,  7007,  7007,  7007,  1040,
    1175,  1047,  1071,  6625, -1406, -1406,   352, -1406, -1406,  9427,
   -1406,  1068, -1406,   359,  1229,  1004,  7007,  7007,  7007,  9513,
   11397, 11397, -1406, -1406, -1406,  1092,   367, -1406,   379, -1406,
   -1406,  1111, -1406, -1406,   300, -1406,  1137,   426,  4139, -1406,
     655, -1406, -1406, -1406,  7439,  9599,  9685, -1406,   452, -1406,
    9771, -1406,  1075, -1406, 11397, -1406, 11397,  9857,  9943,    45,
   10029, 10115,  1072,   393,   126, -1406, -1406, -1406,   373, -1406,
      15, -1406, -1406,  1175,  1175, 10201,  1078,  1080,  1083,  1084,
    7007, -1406,  7007,  1212,  1212,  1212,  7007, -1406, -1406,  1175,
    1175, -1406, 10287, -1406, -1406,  1118, -1406, -1406,  1082,  1125,
     397,   402, 11397, 11397,   290,   505, -1406,  1085,  1086,  1089,
   -1406,  6816, -1406, -1406, -1406, -1406, -1406,   658, -1406, -1406,
    1090, -1406, -1406,  1285,   426,  7007,   426,   426, -1406,   573,
   -1406, -1406, -1406,  1270,    15, -1406, -1406,   639,   282,   282,
   -1406,  7007,  1175,  1175,   584,  1108,  1112,   875,   282,   584,
   -1406, -1406,  7007, -1406, -1406,  1110,  7007,  7007, -1406,   505,
    7007, -1406, -1406,  7007, 11480, -1406, -1406, -1406, -1406, -1406,
    3187,   -66,  1115,   406, -1406, -1406,  2573,  7439,   136, -1406,
   -1406,  1270,   175,   584,  1143,  1144, -1406,  1121,  1126, 10373,
     282,   282,  1143,  1132, -1406, -1406,  1133,  1134,  1135,  7528,
    7007, 11397, 11397, -1406,  1119,  1560, -1406, -1406, -1406, -1406,
   -1406, -1406, 11397, -1406,   313,  1127,  3187,   426, -1406, -1406,
    1109,    -2,  7007, 10953, -1406, -1406, -1406, -1406,   437,  1145,
   -1406, -1406, -1406, -1406,  1136,  1147, -1406, -1406, -1406, -1406,
    1266,  1150,  7528,  1146,   426, -1406, -1406,  1151, -1406, -1406,
     -66, -1406, -1406,  7007,   972,   -66, 10953, -1406,   584, -1406,
   -1406,  7007, -1406,  1164, -1406,  1159,  7007,  2779, -1406, -1406,
     972, -1406, -1406,   426,   442, 11397, -1406, -1406,  1160,  3187,
   10459,  1162, -1406, -1406, -1406, -1406,   -66,   426,  1174, -1406,
    2985, -1406,  1159, -1406,  1163,   313,  1174, -1406, -1406
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   157,     1,   329,     0,     0,     0,   624,   330,     0,
     802,   792,   797,    20,     0,     0,    19,    16,    15,     3,
       0,     0,     0,     8,   660,     7,   605,     6,    11,     5,
       4,    13,    12,    14,   126,   127,   125,   134,   136,    49,
      62,    59,    60,    51,     0,    57,    50,   626,   625,     0,
       0,    26,    25,   605,   624,   624,   624,     0,   303,    47,
     141,   142,   143,     0,     0,     0,   144,   146,   153,     0,
     140,    21,    10,     9,   261,   642,     0,   606,   607,     0,
       0,     0,     0,    52,     0,    58,     0,     0,    55,   627,
     629,    22,     0,     0,     0,   305,     0,     0,   152,   147,
       0,     0,     0,     0,     0,     0,    71,   263,   262,   265,
     260,   630,   652,   651,     0,   655,   609,   608,   615,   132,
     133,   130,   131,   129,     0,     0,   128,   137,    63,    61,
      57,    54,    53,     0,    23,    24,    27,   640,    71,    71,
     304,    45,    48,   151,     0,   148,   149,   150,   154,    69,
      72,   158,     0,   632,   631,     0,   654,   653,   657,   656,
     661,   610,   533,    30,    31,    35,     0,   121,   122,   119,
     120,   118,   117,   123,     0,    56,     0,     0,    29,   638,
     641,   803,   793,   798,    46,   145,    70,     0,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,     0,     0,   164,   159,    71,   633,   634,
     648,   612,     0,   534,     0,    32,    33,    34,     0,   135,
       0,     0,     0,     0,     0,     0,   669,   689,   670,   705,
     671,   675,   676,   677,   678,   695,   682,   683,   684,   685,
     686,   687,   688,   690,   691,   692,   693,   762,   674,   681,
     694,   769,   776,   672,   679,   673,   680,     0,     0,     0,
       0,   704,   726,   729,   727,   728,   789,   724,   628,    28,
     639,     0,     0,     0,   229,   230,   227,   167,   168,   170,
     169,   171,   172,   173,   174,   200,   201,   198,   199,   191,
     202,   203,   192,   189,   190,   228,   211,     0,   226,   204,
     205,   206,   207,   178,   179,   180,   175,   176,   177,   188,
       0,   194,   195,   193,   186,   187,   182,   181,   183,   184,
     185,   166,   165,   210,     0,   196,   197,   533,   162,   271,
       0,   640,   649,     0,    71,   614,   611,   533,    71,     0,
     594,   587,   616,   124,   730,   753,   756,     0,   759,   749,
       0,   713,   763,   770,   777,   783,   786,     0,     0,   739,
     744,   738,     0,   752,   748,   741,     0,   743,     0,   725,
       0,   804,   794,   799,   231,   232,   225,   209,   233,   212,
     208,     0,   160,   328,   557,   558,     0,     0,   264,   266,
       0,   696,   699,   702,   703,   697,   700,   698,   701,   635,
       0,   646,   662,     0,   138,    71,     0,     0,   588,     0,
       0,     0,     0,     0,     0,   437,   438,     0,     0,     0,
       0,   431,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   695,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   522,   375,   377,   376,   378,   379,   380,
     381,    41,     0,     0,     0,     0,     0,   297,     0,   360,
     361,   869,   435,   434,   512,   432,   505,   504,   503,   502,
     155,   508,   433,   507,   506,   479,   439,   480,     0,   484,
     440,     0,   436,   807,   811,   808,   809,   810,     0,     0,
       0,     0,     0,   159,     0,   159,     0,   159,     0,     0,
       0,   735,   297,   746,   747,   740,   742,     0,   745,   721,
       0,     0,   791,   790,   710,   566,   572,   234,   214,   215,
     217,   216,   218,   219,   220,   221,   213,   222,   223,   224,
       0,   326,   327,     0,   533,   533,   533,   161,   163,   294,
     643,     0,   650,     0,     0,   589,   587,   613,   139,   595,
       0,   585,   586,   584,     0,     0,     0,     0,   718,   831,
     834,   308,   312,   311,   317,     0,   349,     0,     0,     0,
     860,     0,     0,     0,     0,   340,   343,     0,   346,     0,
     864,     0,   840,   846,     0,   837,     0,   467,   468,     0,
       0,     0,     0,     0,     0,     0,   444,   443,   481,   442,
     441,     0,     0,     0,   876,   355,     0,   303,   876,   876,
       0,   362,     0,    71,     0,     0,   370,   361,   294,   155,
     274,     0,     0,     0,     0,   469,   470,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   421,     0,   733,     0,     0,     0,
     706,   708,     0,     0,   162,     0,   162,     0,   162,   303,
     564,     0,   562,     0,   570,     0,   707,     0,     0,   737,
     720,   723,     0,   711,   636,   567,    87,   573,    87,     0,
       0,   664,   554,   555,   577,   559,   561,   560,     0,   621,
     647,   658,   546,   663,     0,     0,     0,     0,     0,   602,
       0,     0,   731,   754,   757,    18,    17,   716,   717,     0,
       0,     0,     0,   829,     0,     0,     0,   850,   853,   856,
       0,   876,     0,   844,   867,   876,     0,     0,     0,     0,
       0,     0,     0,   876,     0,     0,   876,     0,     0,     0,
       0,     0,     0,     0,     0,    44,     0,    42,     0,     0,
     849,   877,   298,     0,   576,     0,   575,     0,     0,   877,
     822,   472,     0,     0,   408,   405,   407,     0,   299,     0,
     297,   424,     0,     0,     0,     0,   159,   362,     0,   370,
       0,     0,   491,   490,     0,     0,   492,   496,   445,   446,
     458,   459,   456,   457,     0,   485,     0,   477,     0,   509,
     510,   511,   447,   448,   463,   464,   465,   466,     0,     0,
     461,   462,   460,   454,   455,   450,   449,   451,   452,   453,
       0,     0,     0,   414,     0,     0,     0,     0,     0,   429,
       0,   760,   750,   714,     0,   764,     0,   771,     0,   778,
       0,     0,   784,     0,     0,   787,     0,     0,   301,   734,
     722,   619,   637,   805,    85,    88,   795,    88,   800,   556,
       0,     0,     0,     0,   578,     0,   296,   320,   318,   271,
       0,     0,   319,     0,     0,     0,    71,     0,   275,     0,
       0,     0,   288,     0,   289,   283,     0,   280,   279,     0,
     281,     0,     0,     0,   295,     0,    83,    84,    81,    82,
     290,   332,   278,     0,   382,   617,   622,   636,   548,     0,
     591,   592,     0,     0,     0,   604,   732,   755,   758,   719,
       0,     0,     0,   830,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   877,     0,
     482,     0,     0,   483,     0,   513,     0,     0,     0,     0,
       0,     0,   370,   517,   518,   519,   520,   521,     0,    38,
       0,   102,     0,     0,     0,   356,     0,   813,   812,   471,
       0,     0,     0,     0,     0,   159,     0,     0,   876,     0,
     425,     0,     0,     0,   428,   426,   156,     0,   162,   374,
     159,   487,     0,   493,     0,     0,     0,   475,     0,     0,
     497,   501,     0,     0,   478,     0,     0,     0,     0,   415,
       0,   422,     0,   473,     0,   430,   761,   751,   715,   709,
     765,   767,   772,   774,   779,   781,   563,   785,   565,   569,
     788,   571,     0,     0,   712,   620,     0,    86,   568,     0,
     574,     0,     0,   666,   667,   580,   579,     0,   321,     0,
       0,   306,     0,   293,     0,     0,    66,   271,     0,   323,
     291,   292,     0,    76,    77,     0,     0,     0,     0,   282,
     267,   277,   284,   285,   286,   287,   331,     0,   276,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   637,   644,
     547,   659,     0,   533,   590,     0,   599,     0,     0,   603,
     832,   835,   309,     0,   314,   315,   313,     0,     0,   352,
     350,     0,     0,     0,   861,   859,   299,     0,   843,   868,
     871,   341,   344,   347,   865,   863,   841,   847,   845,   838,
      71,     0,    39,     0,     0,     0,   333,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,     0,
     870,   300,   427,     0,     0,   297,     0,     0,     0,     0,
       0,   368,     0,    71,   162,     0,     0,     0,     0,   403,
       0,     0,   498,     0,   486,     0,   476,     0,   297,   416,
       0,     0,   474,   423,   419,     0,   768,   775,   782,   297,
     302,   806,   796,   801,   665,     0,     0,   322,   271,   307,
       0,    64,    65,   111,   324,     0,     0,     0,     0,     0,
     268,   273,     0,   596,   338,   337,   392,   393,   395,   394,
     396,   386,   387,   388,   397,   398,   384,   385,   399,   400,
     389,   390,   391,   383,   618,   623,     0,   540,   543,     0,
     593,     0,   601,     0,     0,   310,   316,     0,     0,   351,
     851,   854,   857,     0,     0,     0,     0,     0,     0,     0,
     829,     0,     0,     0,   271,   523,     0,    36,    43,     0,
     104,     0,   105,     0,   106,     0,     0,     0,     0,     0,
     815,   814,   406,   532,   409,     0,     0,   401,     0,   365,
     366,     0,   364,   363,     0,   371,   271,    71,     0,   531,
       0,   529,   404,   526,     0,     0,     0,   525,     0,   417,
       0,   420,     0,   668,   581,   325,   116,     0,     0,     0,
       0,     0,     0,     0,     0,   645,   541,   542,   543,   544,
     535,   549,   600,   829,   829,     0,     0,     0,     0,     0,
     297,   872,   299,   342,   345,   348,     0,   830,   842,   829,
     829,   514,     0,   516,   524,    40,   103,   334,     0,     0,
       0,     0,   817,   816,     0,     0,   412,     0,     0,     0,
     369,     0,   357,   372,   271,   488,   494,     0,   530,   528,
       0,   527,   736,    78,    71,     0,    71,    71,   294,   587,
     339,   597,   598,   538,   535,   536,   537,   540,   828,   828,
     353,     0,   829,   829,   820,     0,     0,   876,   828,   820,
     515,    37,     0,   107,   108,     0,     0,     0,   410,     0,
       0,   402,   367,     0,   358,   373,   489,   495,   499,   418,
       0,     0,    92,     0,   271,   271,     0,     0,     0,   539,
     550,   538,     0,     0,   825,   876,   827,     0,     0,     0,
     828,   828,   821,     0,   862,   873,     0,     0,     0,   874,
       0,   819,   818,   413,   874,   359,   500,    79,    83,    84,
      81,    82,    80,   101,    73,     0,     0,    71,   113,   115,
       0,     0,     0,     0,   552,   583,   582,   545,     0,   877,
     826,   833,   836,   354,     0,     0,   858,   866,   848,   839,
       0,     0,   874,     0,    71,    67,    68,     0,   100,   294,
       0,   271,   270,     0,     0,     0,     0,   823,     0,   852,
     855,     0,   878,     0,   880,    97,     0,     0,    93,   110,
       0,   336,   551,    71,     0,   875,   879,    74,     0,     0,
       0,     0,   335,   271,   824,   294,     0,    71,   267,   553,
       0,    98,    97,    91,     0,    73,   267,    75,    96
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1406, -1406,  -672,    -1, -1406, -1406, -1406, -1406, -1406,   745,
    1287, -1406, -1406, -1406, -1406, -1406, -1406,  1366, -1406, -1406,
   -1406,  1327, -1406,  1246, -1406, -1406,  1296, -1406, -1406, -1406,
   -1406,  -135,  -205, -1406, -1406, -1405,   689,   690, -1406, -1406,
   -1406, -1406,  -199, -1406, -1406, -1406, -1406, -1406, -1406,  -770,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,  1156, -1406,
   -1406,   -64,  1284, -1406, -1406, -1406,   484,   765,   771,   458,
    -467,  -648, -1406, -1406, -1406, -1359, -1406, -1406, -1121, -1406,
   -1406,  -865, -1406, -1406, -1406, -1406,  -619,  -498, -1112, -1406,
     -13, -1406, -1406, -1406, -1406, -1406, -1393, -1391, -1390, -1346,
   -1406, -1406,  1390, -1406, -1200, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,  -435, -1406,
     914,    79, -1406,  -762, -1406, -1406, -1406, -1406, -1406, -1406,
   -1329, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
     604, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,  -155,
     -19,   -67,   -21,    49, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406,   361,  -493,  -714, -1406,  -508,  -718, -1406,  -669,   -61,
     -60, -1406,  -549,  -546, -1406, -1406, -1406, -1072, -1406,  1355,
   -1406, -1406, -1406, -1406, -1406,   296,   486, -1406,   869, -1406,
   -1406, -1406, -1406, -1406, -1406,   491, -1406,  1074, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406,  -113, -1406,   990, -1406, -1406, -1406, -1406,
   -1406, -1406,  -541, -1406, -1406,  -374, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406,  -171, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
     992,  -782,   -18,  -601, -1406, -1406, -1185, -1133, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406,  -943, -1406, -1406,
   -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406, -1406,
   -1406, -1406,   833, -1406, -1406, -1377,  -606, -1406
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,   727,   728,    18,   136,    53,   178,    19,   166,
     172,  1385,  1163,  1297,   605,   472,   142,   473,    97,    21,
      22,    45,    46,    88,    23,    41,    42,   899,   900,  1537,
     150,   151,  1538,  1085,  1461,   901,   875,   876,  1504,  1505,
    1571,  1506,  1567,  1568,  1584,  1569,   902,   903,   904,   983,
     905,   906,   907,   908,   909,   910,   911,   912,   173,   174,
      37,    38,    39,   223,    66,    67,    68,    69,   626,    24,
     338,   398,   216,    25,   109,   217,   110,   152,  1241,  1352,
    1510,   399,   400,   913,   474,   914,   708,   613,   997,   867,
     475,   915,   572,   732,  1275,   476,   916,   917,   918,   919,
     920,   543,   921,  1097,  1167,  1244,   922,   477,   747,  1286,
     748,  1287,   750,  1288,   478,   736,  1279,   479,   614,  1403,
     480,  1191,  1192,   796,   481,   630,   482,   923,   483,   484,
     786,   485,   994,  1395,   995,  1449,   486,   846,  1213,   487,
     615,  1195,  1456,  1197,  1457,  1334,  1496,   489,   490,   393,
    1427,  1470,  1358,  1360,  1269,   928,  1123,  1513,  1546,   394,
     395,   396,   680,   681,   696,   684,   685,   698,   777,   702,
     703,  1517,   564,   419,   556,   351,  1354,   557,   352,    78,
     118,   221,   347,    27,   162,   926,   872,   927,    49,    50,
     133,    28,   155,   219,   341,   873,   180,   181,    29,   111,
     709,  1266,   552,   343,   344,   115,   160,   713,    30,    76,
     220,   553,   704,   491,   409,   273,   274,   672,   694,   275,
     501,  1038,   863,   567,   379,   276,   277,   420,   936,   687,
     499,  1037,   421,   937,   422,   938,   498,  1036,   502,  1040,
     503,  1216,   504,  1042,   505,  1217,   506,  1044,   507,  1218,
     508,  1047,   509,  1050,   682,    31,    55,   282,   525,  1059,
      32,    56,   283,   526,  1061,    33,    54,   281,   524,  1056,
     492,   619,  1483,   620,  1475,  1476,  1477,   946,   493,   730,
    1273,   731,  1274,   757,  1292,   754,  1290,   744,   494,   755,
    1291,   495,   950,  1367,   951,  1368,   952,  1369,   740,  1283,
     752,  1289,   998,   496,   623,  1531,   772,   497
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      17,    59,    70,   182,   183,   278,   523,   224,  1002,   798,
     718,   720,   778,   780,   688,  1149,   127,   996,   697,    71,
      72,    73,   871,   793,  1069,  1245,   856,   729,   858,  -157,
     860,   369,   695,   618,  1284,   885,   674,  1010,   676,   971,
     678,   969,  1353,   560,   391,  1168,  1179,    34,    35,    90,
      70,    70,    70,    84,   391,  1497,   119,   120,   370,   371,
     513,  1425,  1306,   272,   577,  1415,  1448,  1498,     2,  1499,
    1500,   515,   102,   103,   104,     3,   541,   423,   424,   112,
     113,   545,   339,   643,   644,   631,   632,   774,    85,    70,
      70,    70,    70,  1304,   392,   616,   222,   430,     4,    13,
       5,  1540,     6,   432,   999,  1390,   222,   102,     7,   104,
      51,    40,   546,  1498,  1501,  1499,  1500,  1533,     8,   542,
    1493,  1426,   372,    77,     9,   561,   373,   167,   168,    96,
    1305,   562,   945,   143,   776,   954,    79,   380,    16,   959,
     439,   440,  1000,   683,   866,    13,   741,   967,    10,    52,
     970,   664,   665,    81,  1545,  1553,   753,  1378,   348,   756,
    1501,   578,   579,   271,  1576,   511,   861,    80,   149,   933,
      11,    12,   563,   374,   442,   443,  1498,   375,  1499,  1500,
     376,   114,   635,   636,    16,   512,  1120,  1563,  1001,   792,
     641,    36,   413,   643,   644,   645,   646,   121,  1169,  1169,
    1055,   956,   122,   589,   123,   377,   514,   124,   742,   412,
    1160,    57,  1233,   414,  1170,  1169,    86,   516,  1169,    15,
     346,   149,   758,  1501,    82,   554,   548,  1121,    87,    89,
    1428,  1429,   850,    13,  1143,    58,   517,  1142,   582,   580,
     555,   125,   631,   632,  1478,   518,  1438,  1439,    74,   565,
     566,   568,   571,  1487,    14,   871,  1293,   368,   583,   581,
    1436,   664,   665,   561,   980,   774,    15,   272,   169,   562,
     467,  1173,    16,   170,   554,   171,    75,   471,   124,   981,
     558,    57,  1422,    80,   272,   415,   369,   228,    95,   555,
     641,    13,    60,   643,   644,  1524,  1525,   774,   944,  1480,
    1481,   853,   554,   775,   272,    58,  1003,   272,   272,   272,
     563,   185,   776,   370,   371,   229,   982,   555,    43,    96,
    1034,    61,  1174,   700,   854,  1534,  1053,   668,   669,  1008,
      16,   673,  1420,   675,  1004,   677,  1535,  1536,   701,   635,
     636,    44,  1133,   932,   776,   631,   632,   641,  1035,   642,
     643,   644,   645,   646,  1242,   128,   940,   941,  1051,  1243,
    1193,   664,   665,  1345,   774,    62,   953,   271,   590,  1060,
    1048,   369,   101,   961,   962,   105,   964,   372,   966,   105,
     968,   373,  1058,  1127,   271,   272,   272,    87,   591,   272,
      63,   272,  1181,   272,   774,   272,  1226,    57,   370,   371,
    1307,   106,   593,  1318,   271,  1006,  1138,   271,   271,   271,
     882,   776,   272,   883,   271,   575,   884,   774,   664,   665,
    1423,    58,   594,  1446,   734,   774,  1338,  1186,   374,  1383,
     866,   775,   375,  1187,   972,   376,   616,    96,   816,    13,
    1359,   776,   635,   636,   735,   999,  1055,   817,    64,  1034,
     641,   130,   642,   643,   644,   645,   646,  1583,    65,  1301,
     377,  1402,   372,  1188,   776,  1588,   373,  1349,  1324,   272,
    1034,   131,   776,   132,  1189,   725,    13,  1134,    16,  1190,
     345,   137,  1473,   272,   726,   271,   271,  1302,   788,   271,
      57,   271,   862,   271,   725,   271,  1325,   138,  1332,   782,
    1183,  1034,  1308,   726,   783,    47,  1034,  1494,   661,   662,
     663,    48,   271,   374,    58,    16,   868,   375,   803,   807,
     376,   664,   665,   380,   141,  1034,  1184,   722,  1178,  1339,
    1315,   784,  1034,   821,  1341,   369,   156,   157,   710,  1455,
    1034,   725,    13,  1194,   139,   377,  1327,    98,    99,   100,
     726,   847,  1183,  1384,   380,   725,    13,   140,   939,   719,
    1388,   942,   370,   371,   726,   949,  1301,   107,  1397,   271,
    1301,   116,   865,   108,   272,  1034,  1141,   117,  1398,  1034,
    1200,    16,  1147,   271,   725,    13,   145,   146,   147,   148,
     134,   149,  1210,   726,  1419,    16,   135,  1215,  1444,  1508,
    1509,  1052,   866,  1445,   272,   725,    13,  1507,   785,   380,
    1183,    85,    70,   723,   726,  1183,   272,   272,   272,   272,
    1177,   161,  1137,   272,    16,  1183,   372,   272,   380,   176,
     373,   990,   851,   369,   272,   272,   177,   272,  1547,   272,
     991,   272,   272,  1574,    57,    16,  1474,  1474,   631,   632,
    1023,  1410,  1482,   782,   725,    13,  1474,  1482,   380,  1024,
     370,   371,   852,   726,  -766,  -773,   725,    13,    58,  -766,
    -773,  1157,   153,   369,   271,   726,  1559,   374,   154,   416,
     158,   375,   417,   955,   376,   418,   159,  -780,  -766,  -773,
    1046,  1518,  -780,  1049,    16,   874,  1356,   874,  1474,  1474,
     370,   371,  1357,   179,   271,  -411,    16,   898,  1579,   377,
    -411,  -780,  1065,  1028,   184,  1021,   271,   271,   271,   271,
     935,  1342,  1029,   271,   372,  1447,  1296,   271,   373,  -411,
    1054,   725,   380,  1303,   271,   271,   855,   271,   616,   271,
     726,   271,   271,   186,  1316,   635,   636,   999,   272,  1467,
     272,   272,   418,   641,   102,   642,   643,   644,   645,   646,
     222,  1077,   272,   279,   372,   380,  1564,   561,   373,   857,
     272,  1083,  1084,   562,   380,   374,   725,    13,   859,   375,
     380,  1130,   376,   218,  1132,   726,   725,    13,   280,   725,
      13,   925,   725,  1159,   335,   726,   337,   898,   726,  1466,
     369,   726,   340,  1281,   336,    13,  1282,   377,   342,   659,
     660,   661,   662,   663,   563,   374,    16,   716,   369,   375,
     717,  1328,   376,   418,   664,   665,    16,   370,   371,    16,
     380,  1486,   380,   349,  1140,   380,  1406,   272,   272,  1458,
     102,   103,   104,   272,    16,   370,   371,   377,   271,   350,
     271,   271,   528,   529,   530,   531,   532,   533,   534,   535,
     711,   712,   271,   163,   164,   765,   766,   354,   355,  1520,
     271,  1391,  1435,  1468,  1057,   356,  1057,   536,   163,   164,
     165,   272,  1220,   469,   627,   357,   628,   537,   538,   539,
     358,   372,  1073,   225,   226,   373,   359,   361,  1080,  1081,
     631,   632,   225,   226,   227,   705,   706,   707,   360,   372,
     362,  1089,   363,   373,  1091,  1092,  1093,  1094,  1095,  1512,
    1557,   364,  1098,    92,    93,    94,   365,   366,   367,   378,
     380,  1126,   381,  1129,   382,   384,   385,   271,   271,   383,
     272,  1437,   374,   271,   386,   388,   375,   387,  1131,   376,
     389,   390,   397,   411,   500,   521,  1580,   527,   540,   544,
     374,   488,  1543,   547,   375,   549,  1144,   376,  1268,   550,
     559,   510,   551,   569,   377,   570,   622,   576,   584,   585,
    1463,   271,   520,   586,   587,   588,   589,   625,   592,   595,
     369,   596,   377,   631,   632,   633,   634,   635,   636,   637,
     599,   600,   638,   639,   640,   641,   601,   642,   643,   644,
     645,   646,   401,   647,   648,   602,   402,   370,   371,   603,
     604,   621,   624,   667,   671,  1294,  1330,   670,   683,   689,
     403,   404,   714,   693,   715,   405,   406,   407,   408,   724,
     271,   733,   737,   738,   739,   768,   769,   770,   771,   597,
     598,   773,   787,   779,   781,   795,   814,   797,  1326,   657,
     658,   659,   660,   661,   662,   663,   606,   607,   608,   609,
     610,   864,   869,   880,   881,   925,   664,   665,   930,   931,
     947,   372,   272,   958,   272,   373,   984,   992,   633,   634,
     635,   636,   637,  1511,   993,   638,   639,   640,   641,   989,
     642,   643,   644,   645,   646,  1005,   647,   648,  1007,  1011,
    1013,  1012,   649,   650,   651,  1031,  1039,  1041,   652,  1014,
    1015,  1016,  1026,  1043,  1027,   692,  1032,  1272,  1045,  1063,
    1064,  1072,   374,  1074,  1082,  1086,   375,    13,  1151,   376,
    1087,  1088,  1096,  1117,   699,  1090,  1125,   653,  1146,   654,
     655,   656,   657,   658,   659,   660,   661,   662,   663,  1135,
    1136,  1145,  1150,  1407,   377,  1155,  1158,  1166,   721,   664,
     665,  1161,  1165,  1164,  1172,  1202,    16,  1182,  1203,  1205,
    1211,  1180,   271,  1219,   271,  1225,  1228,   743,   746,  1230,
    1221,   749,  1404,   751,  1222,  1223,  1231,   631,   632,  1232,
    1235,  1236,  1237,   759,   760,   761,   762,   763,   764,  1238,
    1239,  1240,  1267,  1270,  1276,  1278,  1277,  1298,  1300,  1317,
    1319,   272,  1320,  1321,  1322,  1323,  1343,  1355,  1359,  1363,
    1364,  1387,  1370,   631,   632,  1366,  1371,   808,   809,  1372,
    1376,   810,   811,   812,   813,  1377,   815,  1379,   818,   819,
     820,   822,   823,   824,   825,   826,   827,   829,   830,   831,
     832,   833,   834,   835,   836,   837,   838,   839,  1361,   848,
    1362,  1380,  1389,  1396,  1399,  1401,  1412,  1418,  1431,  1462,
    1432,  1464,  1465,  1433,  1434,  1441,  1442,  1451,  1443,  1450,
    1452,  1459,   633,   634,   635,   636,   637,  1460,   369,   638,
     639,   640,   641,  1469,   642,   643,   644,   645,   646,  1484,
     647,   648,   924,  1485,  1490,  1542,  1183,  1519,   929,  1551,
     -89,   271,  1521,  1530,   934,   370,   371,  1522,   633,   634,
     635,   636,  1539,  1526,  1527,  1528,  1529,  1549,   641,   948,
     642,   643,   644,   645,   646,  1548,   647,   648,  1550,  1552,
     767,  1556,  1554,  1421,   272,   656,   657,   658,   659,   660,
     661,   662,   663,  1566,   -94,  1575,   126,    20,  1578,  1586,
     979,    83,  1541,   664,   665,   985,   175,   986,   129,   987,
    1587,   988,   785,  1585,   353,   144,  1122,   877,   878,   372,
     794,    26,   743,   373,   659,   660,   661,   662,   663,  1555,
     799,   629,   924,  1400,  1514,  1471,  1472,  1424,    91,   664,
     665,  1515,  1516,  1118,  1265,   410,   745,   573,  1119,   574,
       0,  1488,     0,     0,     0,     0,     0,     0,  1573,     0,
       0,     0,     0,     0,     0,     0,   785,     0,     0,     0,
     374,     0,  1582,     0,   375,  1025,  1152,   376,     0,  1030,
       0,   230,     0,     0,   271,     0,     0,   231,     0,     0,
    1503,     0,     0,   232,     0,   898,     0,     0,     0,     0,
       0,     0,   377,   233,     0,     0,     0,     0,     0,     0,
       0,   234,     0,     0,  1062,     0,     0,     0,     0,  1066,
       0,  1068,     0,     0,  1071,     0,   235,     0,     0,  1076,
       0,  1079,     0,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,     0,  1128,     0,  1558,
       0,     0,     0,  1561,  1562,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   898,     0,     0,  1572,
       0,  1148,   743,     0,     0,     0,     0,     0,     0,     0,
       0,    57,     0,     0,     0,  1581,     0,     0,     0,   898,
       0,   631,   632,     0,   269,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    58,  1175,  1176,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   985,     0,     0,
       0,     0,     0,     0,     0,     0,  1196,     0,  1198,     0,
    1201,     0,     0,     0,     0,     0,  1204,     0,     0,     0,
    1207,     0,     0,     0,   270,     0,     0,   369,   985,   522,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   370,   371,     0,     0,     0,     0,
     369,  1227,     0,     0,  1229,     0,   633,   634,   635,   636,
     637,     0,  1234,   638,   639,   640,   641,     0,   642,   643,
     644,   645,   646,     0,   647,   648,     0,   370,   371,     0,
     649,   650,   651,  1246,  1247,  1248,  1249,  1250,  1251,  1252,
    1253,  1254,  1255,  1256,  1257,  1258,  1259,  1260,  1261,  1262,
    1263,  1264,     0,     0,     0,     0,     0,     0,   372,     0,
       0,  1271,   373,     0,     0,   653,     0,   654,   655,   656,
     657,   658,   659,   660,   661,   662,   663,     0,     0,     0,
     743,     0,   -80,     0,     0,     0,     0,   664,   665,     0,
       0,   372,     0,   631,   632,   373,     0,     0,  1299,     0,
       0,     0,     0,     0,     0,     0,  1309,  1310,  1311,   374,
       0,     0,     0,   375,     0,  1153,   376,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1335,     0,  1336,
       0,   377,   374,     0,     0,  1340,   375,     0,  1154,   376,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1344,     0,     0,     0,  1346,     0,   631,   632,     0,  1347,
    1348,     0,  1350,  1351,   377,     0,     0,     0,     0,     0,
    1099,  1100,  1101,  1102,  1103,  1104,  1105,  1106,   633,   634,
     635,   636,   637,  1107,  1108,   638,   639,   640,   641,  1109,
     642,   643,   644,   645,   646,  1110,   647,   648,  1111,  1112,
       0,  1365,   649,   650,   651,  1113,  1114,  1115,   652,     0,
    1373,  1374,  1375,     0,     0,     0,     0,  1382,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1392,  1393,     0,     0,     0,  1116,   653,     0,   654,
     655,   656,   657,   658,   659,   660,   661,   662,   663,     0,
       0,   633,   634,   635,   636,   637,     0,     0,   638,   664,
     665,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,     0,     0,     0,     0,     0,     0,   369,     0,     0,
       0,     0,     0,     0,   631,   632,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   743,     0,     0,     0,
       0,     0,     0,     0,   370,   371,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   657,   658,   659,   660,   661,
     662,   663,     0,     0,     0,  1454,     0,     0,     0,     0,
       0,     0,   664,   665,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1479,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1489,     0,   372,     0,
    1491,  1492,   373,     0,   743,     0,     0,  1495,     0,   633,
     634,   635,   636,   637,  1502,     0,   638,   639,   640,   641,
     924,   642,   643,   644,   645,   646,     0,   647,   648,     0,
       0,     0,     0,   649,   650,   651,     0,     0,     0,   652,
       0,     0,     0,     0,  1532,     0,     0,     0,     0,   374,
       0,     0,     0,   375,     0,  1156,   376,     0,     0,     0,
    1502,     0,     0,     0,     0,     0,  1544,     0,   653,     0,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
       0,   377,     0,     0,     0,     0,     0,     0,     0,     0,
     664,   665,     0,     0,   666,     0,     0,  1560,     0,     0,
       0,     0,     0,     0,     0,  1565,     0,     0,     0,     0,
    1570,   924,   886,     0,     0,     0,   423,   424,     3,     0,
    -112,   -99,   -99,  1502,  -109,     0,   425,   426,   427,   428,
     429,     0,     0,     0,   924,     0,   430,   887,   431,   888,
     889,     0,   432,     0,     0,     0,     0,     0,     0,   890,
     433,     0,     0,  -114,     0,   891,   434,     0,     0,   435,
       0,     8,   436,   892,     0,   893,   437,     0,     0,   894,
     895,     0,     0,     0,     0,     0,   896,     0,     0,   439,
     440,     0,   236,   237,   238,     0,   240,   241,   242,   243,
     244,   441,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,     0,   258,   259,   260,     0,     0,   263,
     264,   265,   266,   442,   443,   444,   897,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   445,
     446,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      57,     0,     0,     0,     0,     0,     0,     0,   447,   448,
     449,   450,   451,     0,   452,     0,   453,   454,   455,   456,
     457,   458,   459,   460,    58,     0,    13,   461,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   462,   463,   464,     0,    14,     0,     0,
     465,   466,     0,     0,     0,     0,     0,     0,     0,   467,
       0,   468,     0,   469,   470,    16,   471,  -272,   886,     0,
       0,     0,   423,   424,     3,     0,  -112,   -99,   -99,     0,
    -109,     0,   425,   426,   427,   428,   429,     0,     0,     0,
       0,     0,   430,   887,   431,   888,   889,     0,   432,     0,
       0,     0,     0,     0,     0,   890,   433,     0,     0,  -114,
       0,   891,   434,     0,     0,   435,     0,     8,   436,   892,
       0,   893,   437,     0,     0,   894,   895,     0,     0,     0,
       0,     0,   896,     0,     0,   439,   440,     0,   236,   237,
     238,     0,   240,   241,   242,   243,   244,   441,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,     0,
     258,   259,   260,     0,     0,   263,   264,   265,   266,   442,
     443,   444,   897,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   445,   446,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    57,     0,     0,     0,
       0,     0,     0,     0,   447,   448,   449,   450,   451,     0,
     452,     0,   453,   454,   455,   456,   457,   458,   459,   460,
      58,     0,    13,   461,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   462,
     463,   464,     0,    14,     0,     0,   465,   466,     0,     0,
       0,     0,     0,     0,     0,   467,     0,   468,     0,   469,
     470,    16,   471,  1009,   886,     0,     0,     0,   423,   424,
       3,     0,  -112,   -99,   -99,     0,  -109,     0,   425,   426,
     427,   428,   429,     0,     0,     0,     0,     0,   430,   887,
     431,   888,   889,     0,   432,     0,     0,     0,     0,     0,
       0,   890,   433,     0,     0,  -114,     0,   891,   434,     0,
       0,   435,     0,     8,   436,   892,     0,   893,   437,     0,
       0,   894,   895,     0,     0,     0,     0,     0,   896,     0,
       0,   439,   440,     0,   236,   237,   238,     0,   240,   241,
     242,   243,   244,   441,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,   256,     0,   258,   259,   260,     0,
       0,   263,   264,   265,   266,   442,   443,   444,   897,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   445,   446,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    57,     0,     0,     0,     0,     0,     0,     0,
     447,   448,   449,   450,   451,     0,   452,     0,   453,   454,
     455,   456,   457,   458,   459,   460,    58,     0,    13,   461,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   462,   463,   464,     0,    14,
       0,     0,   465,   466,     0,     0,     0,     0,     0,     0,
       0,   467,     0,   468,     0,   469,   470,    16,   471,  -269,
     886,     0,     0,     0,   423,   424,     3,     0,  -112,   -99,
     -99,     0,  -109,     0,   425,   426,   427,   428,   429,     0,
       0,     0,     0,     0,   430,   887,   431,   888,   889,     0,
     432,     0,     0,     0,     0,     0,     0,   890,   433,     0,
       0,  -114,     0,   891,   434,     0,     0,   435,     0,     8,
     436,   892,     0,   893,   437,     0,     0,   894,   895,     0,
       0,     0,     0,     0,   896,     0,     0,   439,   440,     0,
     236,   237,   238,     0,   240,   241,   242,   243,   244,   441,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,     0,   258,   259,   260,     0,     0,   263,   264,   265,
     266,   442,   443,   444,   897,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   445,   446,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    57,     0,
       0,     0,     0,     0,     0,     0,   447,   448,   449,   450,
     451,     0,   452,     0,   453,   454,   455,   456,   457,   458,
     459,   460,    58,     0,    13,   461,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   462,   463,   464,     0,    14,     0,     0,   465,   466,
       0,     0,     0,     0,     0,     0,     0,   467,     0,   468,
       0,   469,   470,    16,   471,   -90,   886,     0,     0,     0,
     423,   424,     3,     0,  -112,   -99,   -99,     0,  -109,     0,
     425,   426,   427,   428,   429,     0,     0,     0,     0,     0,
     430,   887,   431,   888,   889,     0,   432,     0,     0,     0,
       0,     0,     0,   890,   433,     0,     0,  -114,     0,   891,
     434,     0,     0,   435,     0,     8,   436,   892,     0,   893,
     437,     0,     0,   894,   895,     0,     0,     0,     0,     0,
     896,     0,     0,   439,   440,     0,   236,   237,   238,     0,
     240,   241,   242,   243,   244,   441,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,     0,   258,   259,
     260,     0,     0,   263,   264,   265,   266,   442,   443,   444,
     897,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   445,   446,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    57,     0,     0,     0,     0,     0,
       0,     0,   447,   448,   449,   450,   451,     0,   452,     0,
     453,   454,   455,   456,   457,   458,   459,   460,    58,     0,
      13,   461,     0,   369,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   462,   463,   464,
       0,    14,     0,     0,   465,   466,     0,     0,     0,     0,
     370,   371,     0,   467,     0,   468,     0,   469,   470,    16,
     471,   -95,   423,   424,     0,     0,     0,     0,     0,     0,
       0,     0,   425,   426,   427,   428,   429,     0,     0,     0,
       0,     0,   430,   887,   431,   888,     0,     0,   432,     0,
       0,     0,     0,     0,     0,     0,   433,     0,     0,     0,
       0,     0,   434,     0,     0,   435,     0,     0,   436,   892,
       0,     0,   437,     0,   372,     0,     0,     0,   373,     0,
       0,     0,   438,     0,     0,   439,   440,     0,   236,   237,
     238,     0,   240,   241,   242,   243,   244,   441,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,     0,
     258,   259,   260,     0,     0,   263,   264,   265,   266,   442,
     443,   444,   897,     0,     0,   374,     0,     0,     0,   375,
       0,  1280,   376,     0,     0,   445,   446,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    57,   377,     0,     0,
       0,     0,     0,     0,   447,   448,   449,   450,   451,     0,
     452,     0,   453,   454,   455,   456,   457,   458,   459,   460,
      58,     0,     0,   461,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   462,
     463,   464,     0,    14,     0,     0,   465,   466,     0,     0,
       0,     0,     0,   423,   424,   467,     0,   468,     0,   469,
     470,     0,   471,   425,   426,   427,   428,   429,     0,     0,
       0,     0,     0,   430,     0,   431,     0,     0,     0,   432,
       0,     0,     0,     0,     0,     0,     0,   433,     0,     0,
       0,     0,     0,   434,     0,     0,   435,     0,     0,   436,
       0,     0,     0,   437,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   438,     0,     0,   439,   440,   789,   236,
     237,   238,     0,   240,   241,   242,   243,   244,   441,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
       0,   258,   259,   260,     0,     0,   263,   264,   265,   266,
     442,   443,   444,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   445,   446,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    57,     0,     0,
       0,     0,     0,     0,     0,   447,   448,   449,   450,   451,
       0,   452,   616,   453,   454,   455,   456,   457,   458,   459,
     460,   617,     0,     0,   461,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     462,   463,   464,     0,    14,     0,     0,   465,   466,     0,
       0,     0,     0,     0,   423,   424,   790,     0,   468,   791,
     469,   470,   611,   471,   425,   426,   427,   428,   429,     0,
       0,     0,     0,     0,   430,     0,   431,     0,     0,     0,
     432,     0,     0,     0,     0,     0,     0,     0,   433,     0,
       0,     0,     0,     0,   434,     0,     0,   435,   612,     0,
     436,     0,     0,     0,   437,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   438,     0,     0,   439,   440,     0,
     236,   237,   238,     0,   240,   241,   242,   243,   244,   441,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,     0,   258,   259,   260,     0,     0,   263,   264,   265,
     266,   442,   443,   444,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   445,   446,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    57,     0,
       0,     0,     0,     0,     0,     0,   447,   448,   449,   450,
     451,     0,   452,   616,   453,   454,   455,   456,   457,   458,
     459,   460,   617,     0,     0,   461,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   462,   463,   464,     0,    14,     0,     0,   465,   466,
       0,     0,     0,     0,     0,   423,   424,   467,     0,   468,
       0,   469,   470,   611,   471,   425,   426,   427,   428,   429,
       0,     0,     0,     0,     0,   430,     0,   431,     0,     0,
     369,   432,     0,     0,     0,     0,     0,     0,     0,   433,
       0,     0,     0,     0,     0,   434,     0,     0,   435,   612,
       0,   436,     0,     0,     0,   437,     0,   370,   371,     0,
       0,     0,     0,     0,     0,   438,     0,     0,   439,   440,
       0,   236,   237,   238,     0,   240,   241,   242,   243,   244,
     441,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,     0,   258,   259,   260,     0,     0,   263,   264,
     265,   266,   442,   443,   444,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   445,   446,
       0,   372,     0,     0,     0,   373,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    57,
       0,     0,     0,     0,     0,     0,     0,   447,   448,   449,
     450,   451,     0,   452,     0,   453,   454,   455,   456,   457,
     458,   459,   460,    58,     0,     0,   461,     0,     0,     0,
       0,     0,   374,     0,     0,     0,   375,     0,  1285,   376,
       0,     0,   462,   463,   464,     0,    14,     0,     0,   465,
     466,     0,     0,     0,     0,     0,   423,   424,   467,     0,
     468,     0,   469,   470,   377,   471,   425,   426,   427,   428,
     429,     0,     0,     0,     0,     0,   430,     0,   431,     0,
       0,   369,   432,     0,     0,     0,     0,     0,     0,     0,
     433,     0,     0,     0,     0,     0,   434,     0,     0,   435,
       0,     0,   436,     0,     0,     0,   437,     0,   370,   371,
       0,     0,     0,     0,     0,     0,   438,     0,     0,   439,
     440,   943,   236,   237,   238,     0,   240,   241,   242,   243,
     244,   441,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,     0,   258,   259,   260,     0,     0,   263,
     264,   265,   266,   442,   443,   444,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   445,
     446,     0,   372,     0,     0,     0,   373,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      57,     0,     0,     0,     0,     0,     0,     0,   447,   448,
     449,   450,   451,     0,   452,   616,   453,   454,   455,   456,
     457,   458,   459,   460,   617,     0,     0,   461,     0,     0,
       0,     0,     0,   374,     0,     0,     0,   375,     0,  1314,
     376,     0,     0,   462,   463,   464,     0,    14,     0,     0,
     465,   466,     0,     0,     0,     0,     0,   423,   424,   467,
       0,   468,     0,   469,   470,   377,   471,   425,   426,   427,
     428,   429,     0,     0,     0,     0,     0,   430,     0,   431,
       0,     0,   369,   432,     0,     0,     0,     0,     0,     0,
       0,   433,     0,     0,     0,     0,     0,   434,     0,     0,
     435,     0,     0,   436,     0,     0,     0,   437,     0,   370,
     371,     0,     0,     0,     0,     0,     0,   438,     0,     0,
     439,   440,     0,   236,   237,   238,     0,   240,   241,   242,
     243,   244,   441,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,     0,   258,   259,   260,     0,     0,
     263,   264,   265,   266,   442,   443,   444,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     445,   446,     0,   372,     0,     0,     0,   373,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    57,     0,     0,     0,     0,     0,     0,     0,   447,
     448,   449,   450,   451,     0,   452,     0,   453,   454,   455,
     456,   457,   458,   459,   460,    58,     0,     0,   461,     0,
       0,     0,     0,     0,   374,     0,     0,     0,   375,     0,
    1405,   376,     0,     0,   462,   463,   464,     0,    14,     0,
       0,   465,   466,     0,     0,     0,     0,     0,   423,   424,
     467,   519,   468,     0,   469,   470,   377,   471,   425,   426,
     427,   428,   429,     0,     0,     0,     0,     0,   430,     0,
     431,     0,     0,     0,   432,     0,     0,     0,     0,     0,
       0,     0,   433,     0,     0,     0,     0,     0,   434,     0,
       0,   435,     0,     0,   436,     0,     0,     0,   437,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   438,     0,
       0,   439,   440,     0,   236,   237,   238,     0,   240,   241,
     242,   243,   244,   441,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,   256,     0,   258,   259,   260,     0,
       0,   263,   264,   265,   266,   442,   443,   444,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   445,   446,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    57,     0,     0,     0,     0,     0,     0,     0,
     447,   448,   449,   450,   451,     0,   452,   616,   453,   454,
     455,   456,   457,   458,   459,   460,   617,     0,     0,   461,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   462,   463,   464,     0,    14,
       0,     0,   465,   466,     0,     0,     0,     0,     0,   423,
     424,   467,     0,   468,     0,   469,   470,     0,   471,   425,
     426,   427,   428,   429,     0,     0,     0,     0,     0,   430,
       0,   431,     0,     0,     0,   432,     0,     0,     0,     0,
       0,     0,     0,   433,     0,     0,     0,     0,     0,   434,
       0,     0,   435,     0,     0,   436,     0,     0,     0,   437,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   438,
       0,     0,   439,   440,     0,   236,   237,   238,     0,   240,
     241,   242,   243,   244,   441,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,     0,   258,   259,   260,
       0,     0,   263,   264,   265,   266,   442,   443,   444,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   445,   446,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    57,     0,     0,     0,     0,     0,     0,
       0,   447,   448,   449,   450,   451,     0,   452,     0,   453,
     454,   455,   456,   457,   458,   459,   460,    58,     0,     0,
     461,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   462,   463,   464,     0,
      14,     0,     0,   465,   466,     0,     0,     0,     0,     0,
     423,   424,   467,   691,   468,     0,   469,   470,     0,   471,
     425,   426,   427,   428,   429,     0,     0,   828,     0,     0,
     430,     0,   431,     0,     0,     0,   432,     0,     0,     0,
       0,     0,     0,     0,   433,     0,     0,     0,     0,     0,
     434,     0,     0,   435,     0,     0,   436,     0,     0,     0,
     437,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     438,     0,     0,   439,   440,     0,   236,   237,   238,     0,
     240,   241,   242,   243,   244,   441,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,     0,   258,   259,
     260,     0,     0,   263,   264,   265,   266,   442,   443,   444,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   445,   446,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    57,     0,     0,     0,     0,     0,
       0,     0,   447,   448,   449,   450,   451,     0,   452,     0,
     453,   454,   455,   456,   457,   458,   459,   460,    58,     0,
       0,   461,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   462,   463,   464,
       0,    14,     0,     0,   465,   466,     0,     0,     0,     0,
       0,   423,   424,   467,     0,   468,     0,   469,   470,     0,
     471,   425,   426,   427,   428,   429,     0,     0,     0,     0,
       0,   430,     0,   431,     0,     0,     0,   432,     0,     0,
       0,     0,     0,     0,     0,   433,     0,     0,     0,     0,
       0,   434,     0,     0,   435,     0,     0,   436,     0,     0,
       0,   437,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   438,     0,     0,   439,   440,     0,   236,   237,   238,
       0,   240,   241,   242,   243,   244,   441,   246,   247,   248,
     249,   250,   251,   252,   253,   254,   255,   256,     0,   258,
     259,   260,     0,     0,   263,   264,   265,   266,   442,   443,
     444,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   445,   446,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    57,     0,     0,     0,     0,
       0,     0,     0,   447,   448,   449,   450,   451,     0,   452,
       0,   453,   454,   455,   456,   457,   458,   459,   460,    58,
       0,     0,   461,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   462,   463,
     464,     0,    14,     0,     0,   465,   466,     0,     0,     0,
       0,     0,   423,   424,   467,     0,   468,   849,   469,   470,
       0,   471,   425,   426,   427,   428,   429,     0,     0,     0,
       0,     0,   430,     0,   431,     0,     0,     0,   432,     0,
       0,     0,     0,     0,     0,     0,   433,     0,     0,     0,
       0,     0,   434,     0,     0,   435,     0,     0,   436,     0,
       0,     0,   437,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   438,     0,     0,   439,   440,     0,   236,   237,
     238,     0,   240,   241,   242,   243,   244,   441,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,     0,
     258,   259,   260,     0,     0,   263,   264,   265,   266,   442,
     443,   444,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   445,   446,     0,     0,     0,
       0,     0,     0,     0,  1067,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    57,     0,     0,     0,
       0,     0,     0,     0,   447,   448,   449,   450,   451,     0,
     452,     0,   453,   454,   455,   456,   457,   458,   459,   460,
      58,     0,     0,   461,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   462,
     463,   464,     0,    14,     0,     0,   465,   466,     0,     0,
       0,     0,     0,   423,   424,   467,     0,   468,     0,   469,
     470,     0,   471,   425,   426,   427,   428,   429,     0,     0,
       0,     0,     0,   430,     0,   431,     0,     0,     0,   432,
       0,     0,     0,     0,     0,     0,     0,   433,     0,     0,
       0,     0,     0,   434,     0,     0,   435,     0,     0,   436,
       0,     0,     0,   437,     0,     0,     0,     0,     0,  1070,
       0,     0,     0,   438,     0,     0,   439,   440,     0,   236,
     237,   238,     0,   240,   241,   242,   243,   244,   441,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
       0,   258,   259,   260,     0,     0,   263,   264,   265,   266,
     442,   443,   444,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   445,   446,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    57,     0,     0,
       0,     0,     0,     0,     0,   447,   448,   449,   450,   451,
       0,   452,     0,   453,   454,   455,   456,   457,   458,   459,
     460,    58,     0,     0,   461,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     462,   463,   464,     0,    14,     0,     0,   465,   466,     0,
       0,     0,     0,     0,   423,   424,   467,     0,   468,     0,
     469,   470,     0,   471,   425,   426,   427,   428,   429,     0,
       0,     0,     0,     0,   430,     0,   431,     0,     0,     0,
     432,     0,     0,     0,     0,     0,     0,     0,   433,     0,
       0,     0,     0,     0,   434,     0,     0,   435,     0,     0,
     436,     0,     0,     0,   437,     0,     0,  1075,     0,     0,
       0,     0,     0,     0,   438,     0,     0,   439,   440,     0,
     236,   237,   238,     0,   240,   241,   242,   243,   244,   441,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,     0,   258,   259,   260,     0,     0,   263,   264,   265,
     266,   442,   443,   444,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   445,   446,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    57,     0,
       0,     0,     0,     0,     0,     0,   447,   448,   449,   450,
     451,     0,   452,     0,   453,   454,   455,   456,   457,   458,
     459,   460,    58,     0,     0,   461,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   462,   463,   464,     0,    14,     0,     0,   465,   466,
       0,     0,     0,     0,     0,   423,   424,   467,     0,   468,
       0,   469,   470,     0,   471,   425,   426,   427,   428,   429,
       0,     0,     0,     0,     0,   430,     0,   431,     0,     0,
       0,   432,     0,     0,     0,     0,     0,     0,     0,   433,
       0,     0,     0,     0,     0,   434,     0,     0,   435,     0,
       0,   436,     0,     0,     0,   437,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   438,     0,     0,   439,   440,
       0,   236,   237,   238,     0,   240,   241,   242,   243,   244,
     441,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,     0,   258,   259,   260,     0,     0,   263,   264,
     265,   266,   442,   443,   444,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   445,   446,
       0,     0,     0,     0,     0,     0,     0,  1078,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    57,
       0,     0,     0,     0,     0,     0,     0,   447,   448,   449,
     450,   451,     0,   452,     0,   453,   454,   455,   456,   457,
     458,   459,   460,    58,     0,     0,   461,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   462,   463,   464,     0,    14,     0,     0,   465,
     466,     0,     0,     0,     0,     0,   423,   424,   467,     0,
     468,     0,   469,   470,     0,   471,   425,   426,   427,   428,
     429,     0,     0,     0,     0,     0,   430,     0,   431,     0,
       0,     0,   432,     0,     0,     0,     0,     0,     0,     0,
     433,     0,     0,     0,     0,     0,   434,     0,     0,   435,
       0,     0,   436,     0,     0,     0,   437,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   438,     0,     0,   439,
     440,     0,   236,   237,   238,     0,   240,   241,   242,   243,
     244,   441,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,     0,   258,   259,   260,     0,     0,   263,
     264,   265,   266,   442,   443,   444,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   445,
     446,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      57,     0,     0,     0,     0,     0,     0,     0,   447,   448,
     449,   450,   451,     0,   452,     0,   453,   454,   455,   456,
     457,   458,   459,   460,    58,     0,     0,   461,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   462,   463,   464,     0,    14,     0,     0,
     465,   466,     0,     0,     0,     0,     0,   423,   424,   467,
       0,   468,  1199,   469,   470,     0,   471,   425,   426,   427,
     428,   429,     0,     0,     0,     0,     0,   430,     0,   431,
       0,     0,     0,   432,     0,     0,     0,     0,     0,     0,
       0,   433,     0,     0,     0,     0,     0,   434,     0,     0,
     435,     0,     0,   436,     0,     0,     0,   437,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   438,     0,     0,
     439,   440,     0,   236,   237,   238,     0,   240,   241,   242,
     243,   244,   441,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,     0,   258,   259,   260,     0,     0,
     263,   264,   265,   266,   442,   443,   444,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     445,   446,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    57,     0,     0,     0,     0,     0,     0,     0,   447,
     448,   449,   450,   451,     0,   452,     0,   453,   454,   455,
     456,   457,   458,   459,   460,    58,     0,     0,   461,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   462,   463,   464,     0,    14,     0,
       0,   465,   466,     0,     0,     0,     0,     0,   423,   424,
    1208,     0,   468,  1209,   469,   470,     0,   471,   425,   426,
     427,   428,   429,     0,     0,     0,     0,     0,   430,     0,
     431,     0,     0,     0,   432,     0,     0,     0,     0,     0,
       0,     0,   433,     0,     0,     0,     0,     0,   434,     0,
       0,   435,     0,     0,   436,     0,     0,     0,   437,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   438,     0,
       0,   439,   440,     0,   236,   237,   238,     0,   240,   241,
     242,   243,   244,   441,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,   256,     0,   258,   259,   260,     0,
       0,   263,   264,   265,   266,   442,   443,   444,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   445,   446,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    57,     0,     0,     0,     0,     0,     0,     0,
     447,   448,   449,   450,   451,     0,   452,     0,   453,   454,
     455,   456,   457,   458,   459,   460,    58,     0,     0,   461,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   462,   463,   464,     0,    14,
       0,     0,   465,   466,     0,     0,     0,     0,     0,   423,
     424,   467,     0,   468,  1214,   469,   470,     0,   471,   425,
     426,   427,   428,   429,     0,     0,     0,     0,     0,   430,
       0,   431,     0,     0,     0,   432,     0,     0,     0,     0,
       0,     0,     0,   433,     0,     0,     0,     0,     0,   434,
       0,     0,   435,     0,     0,   436,     0,     0,     0,   437,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   438,
       0,     0,   439,   440,     0,   236,   237,   238,     0,   240,
     241,   242,   243,   244,   441,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,     0,   258,   259,   260,
       0,     0,   263,   264,   265,   266,   442,   443,   444,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   445,   446,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    57,     0,     0,     0,     0,     0,     0,
       0,   447,   448,   449,   450,   451,     0,   452,     0,   453,
     454,   455,   456,   457,   458,   459,   460,    58,     0,     0,
     461,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   462,   463,   464,     0,
      14,     0,     0,   465,   466,     0,     0,     0,     0,     0,
     423,   424,   467,     0,   468,  1295,   469,   470,     0,   471,
     425,   426,   427,   428,   429,     0,     0,     0,     0,     0,
     430,     0,   431,     0,     0,     0,   432,     0,     0,     0,
       0,     0,     0,     0,   433,     0,     0,     0,     0,     0,
     434,     0,     0,   435,     0,     0,   436,     0,     0,     0,
     437,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     438,     0,     0,   439,   440,     0,   236,   237,   238,     0,
     240,   241,   242,   243,   244,   441,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,     0,   258,   259,
     260,     0,     0,   263,   264,   265,   266,   442,   443,   444,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   445,   446,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    57,     0,     0,     0,     0,     0,
       0,     0,   447,   448,   449,   450,   451,     0,   452,     0,
     453,   454,   455,   456,   457,   458,   459,   460,    58,     0,
       0,   461,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   462,   463,   464,
       0,    14,     0,     0,   465,   466,     0,     0,     0,     0,
       0,   423,   424,   467,     0,   468,  1381,   469,   470,     0,
     471,   425,   426,   427,   428,   429,     0,     0,     0,     0,
       0,   430,     0,   431,     0,     0,     0,   432,     0,     0,
       0,     0,     0,     0,     0,   433,     0,     0,     0,     0,
       0,   434,     0,     0,   435,     0,     0,   436,     0,     0,
       0,   437,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   438,     0,     0,   439,   440,     0,   236,   237,   238,
       0,   240,   241,   242,   243,   244,   441,   246,   247,   248,
     249,   250,   251,   252,   253,   254,   255,   256,     0,   258,
     259,   260,     0,     0,   263,   264,   265,   266,   442,   443,
     444,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   445,   446,     0,     0,     0,     0,
       0,     0,     0,  1453,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    57,     0,     0,     0,     0,
       0,     0,     0,   447,   448,   449,   450,   451,     0,   452,
       0,   453,   454,   455,   456,   457,   458,   459,   460,    58,
       0,     0,   461,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   462,   463,
     464,     0,    14,     0,     0,   465,   466,     0,     0,     0,
       0,     0,   423,   424,   467,     0,   468,     0,   469,   470,
       0,   471,   425,   426,   427,   428,   429,     0,     0,     0,
       0,     0,   430,     0,   431,     0,     0,     0,   432,     0,
       0,     0,     0,     0,     0,     0,   433,     0,     0,     0,
       0,     0,   434,     0,     0,   435,     0,     0,   436,     0,
       0,     0,   437,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   438,     0,     0,   439,   440,     0,   236,   237,
     238,     0,   240,   241,   242,   243,   244,   441,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,     0,
     258,   259,   260,     0,     0,   263,   264,   265,   266,   442,
     443,   444,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   445,   446,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    57,     0,     0,     0,
       0,     0,     0,     0,   447,   448,   449,   450,   451,     0,
     452,     0,   453,   454,   455,   456,   457,   458,   459,   460,
      58,     0,     0,   461,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   462,
     463,   464,     0,    14,     0,     0,   465,   466,     0,     0,
       0,     0,     0,   423,   424,   467,     0,   468,     0,   469,
     470,     0,   471,   425,   426,   427,   428,   429,     0,     0,
       0,     0,     0,   430,     0,   431,     0,     0,     0,   432,
       0,     0,     0,     0,     0,     0,     0,   433,     0,     0,
       0,     0,     0,   434,     0,     0,   435,     0,     0,   436,
       0,     0,     0,   437,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   438,     0,     0,   439,   440,     0,   236,
     237,   238,     0,   240,   241,   242,   243,   244,   441,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
       0,   258,   259,   260,     0,     0,   263,   264,   265,   266,
     442,   443,   444,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   445,   446,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    57,     0,     0,
       0,     0,     0,     0,     0,   447,   448,   449,   450,   451,
       0,   452,     0,   453,   454,   455,   456,   457,   458,   459,
     460,    58,     0,   230,   461,     0,     0,     0,     0,   231,
       0,     0,     0,     0,     0,   232,     0,     0,     0,     0,
     462,   463,   464,     0,    14,   233,     0,   465,   466,     0,
       0,     0,     0,   234,     0,     0,  1185,     0,   468,     0,
     469,   470,     0,   471,     0,     0,     0,     0,   235,     0,
       0,     0,     0,     0,     0,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   230,     0,
       0,     0,     0,     0,   231,     0,     0,     0,     0,     0,
     232,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     233,     0,     0,    57,     0,     0,     0,     0,   234,     0,
       0,     0,     0,     0,     0,     0,   269,     0,     0,     0,
       0,     0,     0,   235,     0,     0,     0,   679,     0,    13,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,     0,     0,     0,   270,     0,    16,   631,
     632,   230,     0,     0,     0,     0,     0,   231,     0,     0,
       0,     0,     0,   232,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   233,     0,     0,     0,     0,    57,     0,
       0,   234,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   269,     0,     0,     0,     0,   235,     0,     0,     0,
       0,     0,    58,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,     0,     0,     0,     0,
       0,   270,   631,   632,   633,   634,   635,   636,   637,     0,
       0,   638,   639,   640,   641,     0,   642,   643,   644,   645,
     646,     0,   647,   648,     0,     0,     0,     0,   649,   650,
     651,    57,     0,     0,   652,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   269,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   679,     0,     0,     0,     0,
       0,     0,     0,   653,     0,   654,   655,   656,   657,   658,
     659,   660,   661,   662,   663,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   664,   665,     0,   631,   632,
       0,     0,  1530,     0,   270,     0,     0,   633,   634,   635,
     636,   637,     0,     0,   638,   639,   640,   641,     0,   642,
     643,   644,   645,   646,     0,   647,   648,     0,     0,     0,
       0,   649,   650,   651,     0,     0,     0,   652,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   653,     0,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,     0,     0,
       0,     0,     0,     0,   631,   632,     0,     0,   664,   665,
       0,     0,   686,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,     0,     0,     0,     0,   649,   650,   651,
       0,     0,     0,   652,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   653,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,     0,     0,     0,     0,
     631,   632,     0,     0,   664,   665,     0,     0,   879,   633,
     634,   635,   636,   637,     0,     0,   638,   639,   640,   641,
       0,   642,   643,   644,   645,   646,     0,   647,   648,     0,
       0,     0,     0,   649,   650,   651,     0,     0,     0,   652,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   653,     0,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
       0,     0,     0,     0,     0,     0,   631,   632,     0,     0,
     664,   665,     0,     0,   960,   633,   634,   635,   636,   637,
       0,     0,   638,   639,   640,   641,     0,   642,   643,   644,
     645,   646,     0,   647,   648,     0,     0,     0,     0,   649,
     650,   651,     0,     0,     0,   652,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   653,     0,   654,   655,   656,   657,
     658,   659,   660,   661,   662,   663,     0,     0,     0,     0,
       0,     0,   631,   632,     0,     0,   664,   665,     0,     0,
     963,   633,   634,   635,   636,   637,     0,     0,   638,   639,
     640,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,     0,     0,     0,     0,   649,   650,   651,     0,     0,
       0,   652,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     653,     0,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,     0,     0,     0,     0,     0,     0,   631,   632,
       0,     0,   664,   665,     0,     0,   965,   633,   634,   635,
     636,   637,     0,     0,   638,   639,   640,   641,     0,   642,
     643,   644,   645,   646,     0,   647,   648,     0,     0,     0,
       0,   649,   650,   651,     0,     0,     0,   652,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   653,     0,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,     0,     0,
       0,     0,     0,     0,   631,   632,     0,     0,   664,   665,
       0,     0,   973,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,     0,     0,     0,     0,   649,   650,   651,
       0,     0,     0,   652,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   653,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,     0,     0,     0,     0,
     631,   632,     0,     0,   664,   665,     0,     0,   974,   633,
     634,   635,   636,   637,     0,     0,   638,   639,   640,   641,
       0,   642,   643,   644,   645,   646,     0,   647,   648,     0,
       0,     0,     0,   649,   650,   651,     0,     0,     0,   652,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   653,     0,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
       0,     0,     0,     0,     0,     0,   631,   632,     0,     0,
     664,   665,     0,     0,   975,   633,   634,   635,   636,   637,
       0,     0,   638,   639,   640,   641,     0,   642,   643,   644,
     645,   646,     0,   647,   648,     0,     0,     0,     0,   649,
     650,   651,     0,     0,     0,   652,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   653,     0,   654,   655,   656,   657,
     658,   659,   660,   661,   662,   663,     0,     0,     0,     0,
       0,     0,   631,   632,     0,     0,   664,   665,     0,     0,
     976,   633,   634,   635,   636,   637,     0,     0,   638,   639,
     640,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,     0,     0,     0,     0,   649,   650,   651,     0,     0,
       0,   652,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     653,     0,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,     0,     0,     0,     0,     0,     0,   631,   632,
       0,     0,   664,   665,     0,     0,   977,   633,   634,   635,
     636,   637,     0,     0,   638,   639,   640,   641,     0,   642,
     643,   644,   645,   646,     0,   647,   648,     0,     0,     0,
       0,   649,   650,   651,     0,     0,     0,   652,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   653,     0,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,     0,     0,
       0,     0,     0,     0,   631,   632,     0,     0,   664,   665,
       0,     0,   978,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,     0,     0,     0,     0,   649,   650,   651,
       0,     0,     0,   652,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   653,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,     0,     0,     0,     0,
     631,   632,     0,     0,   664,   665,     0,     0,  1124,   633,
     634,   635,   636,   637,     0,     0,   638,   639,   640,   641,
       0,   642,   643,   644,   645,   646,     0,   647,   648,     0,
       0,     0,     0,   649,   650,   651,     0,     0,     0,   652,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   653,     0,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
       0,     0,     0,     0,     0,     0,   631,   632,     0,     0,
     664,   665,     0,     0,  1139,   633,   634,   635,   636,   637,
       0,     0,   638,   639,   640,   641,     0,   642,   643,   644,
     645,   646,     0,   647,   648,     0,     0,     0,     0,   649,
     650,   651,     0,     0,     0,   652,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   653,     0,   654,   655,   656,   657,
     658,   659,   660,   661,   662,   663,     0,     0,     0,     0,
       0,     0,   631,   632,     0,     0,   664,   665,     0,     0,
    1171,   633,   634,   635,   636,   637,     0,     0,   638,   639,
     640,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,     0,     0,     0,     0,   649,   650,   651,     0,     0,
       0,   652,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     653,     0,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,     0,     0,     0,     0,     0,     0,   631,   632,
       0,     0,   664,   665,     0,     0,  1224,   633,   634,   635,
     636,   637,     0,     0,   638,   639,   640,   641,     0,   642,
     643,   644,   645,   646,     0,   647,   648,     0,     0,     0,
       0,   649,   650,   651,     0,     0,     0,   652,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   653,     0,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,     0,     0,
       0,     0,     0,     0,   631,   632,     0,     0,   664,   665,
       0,     0,  1312,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,     0,     0,     0,     0,   649,   650,   651,
       0,     0,     0,   652,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   653,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,     0,     0,     0,     0,
     631,   632,     0,     0,   664,   665,     0,     0,  1313,   633,
     634,   635,   636,   637,     0,     0,   638,   639,   640,   641,
       0,   642,   643,   644,   645,   646,     0,   647,   648,     0,
       0,     0,     0,   649,   650,   651,     0,     0,     0,   652,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   653,     0,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
       0,     0,     0,     0,     0,     0,   631,   632,     0,     0,
     664,   665,     0,     0,  1329,   633,   634,   635,   636,   637,
       0,     0,   638,   639,   640,   641,     0,   642,   643,   644,
     645,   646,     0,   647,   648,     0,     0,     0,     0,   649,
     650,   651,     0,     0,     0,   652,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   653,     0,   654,   655,   656,   657,
     658,   659,   660,   661,   662,   663,     0,     0,     0,     0,
       0,     0,   631,   632,     0,     0,   664,   665,     0,     0,
    1331,   633,   634,   635,   636,   637,     0,     0,   638,   639,
     640,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,     0,     0,     0,     0,   649,   650,   651,     0,     0,
       0,   652,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     653,     0,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,     0,     0,     0,     0,     0,     0,   631,   632,
       0,     0,   664,   665,     0,     0,  1333,   633,   634,   635,
     636,   637,     0,     0,   638,   639,   640,   641,     0,   642,
     643,   644,   645,   646,     0,   647,   648,     0,     0,     0,
       0,   649,   650,   651,     0,     0,     0,   652,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   653,     0,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,     0,     0,
       0,     0,     0,     0,   631,   632,     0,     0,   664,   665,
       0,     0,  1337,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,     0,     0,     0,     0,   649,   650,   651,
       0,     0,     0,   652,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   653,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,     0,     0,     0,     0,
     631,   632,     0,     0,   664,   665,     0,     0,  1386,   633,
     634,   635,   636,   637,     0,     0,   638,   639,   640,   641,
       0,   642,   643,   644,   645,   646,     0,   647,   648,     0,
       0,     0,     0,   649,   650,   651,     0,     0,     0,   652,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   653,     0,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
       0,     0,     0,     0,     0,     0,   631,   632,     0,     0,
     664,   665,     0,     0,  1394,   633,   634,   635,   636,   637,
       0,     0,   638,   639,   640,   641,     0,   642,   643,   644,
     645,   646,     0,   647,   648,     0,     0,     0,     0,   649,
     650,   651,     0,     0,     0,   652,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   653,     0,   654,   655,   656,   657,
     658,   659,   660,   661,   662,   663,     0,     0,     0,     0,
       0,     0,   631,   632,     0,     0,   664,   665,     0,     0,
    1408,   633,   634,   635,   636,   637,     0,     0,   638,   639,
     640,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,     0,     0,     0,     0,   649,   650,   651,     0,     0,
       0,   652,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     653,     0,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,     0,     0,     0,     0,     0,     0,   631,   632,
       0,     0,   664,   665,     0,     0,  1409,   633,   634,   635,
     636,   637,     0,     0,   638,   639,   640,   641,     0,   642,
     643,   644,   645,   646,     0,   647,   648,     0,     0,     0,
       0,   649,   650,   651,     0,     0,     0,   652,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   653,     0,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,     0,     0,
       0,     0,     0,     0,   631,   632,     0,     0,   664,   665,
       0,     0,  1411,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,     0,     0,     0,     0,   649,   650,   651,
       0,     0,     0,   652,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   653,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,     0,     0,     0,     0,
     631,   632,     0,     0,   664,   665,     0,     0,  1413,   633,
     634,   635,   636,   637,     0,     0,   638,   639,   640,   641,
       0,   642,   643,   644,   645,   646,     0,   647,   648,     0,
       0,     0,     0,   649,   650,   651,     0,     0,     0,   652,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   653,     0,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
       0,     0,     0,     0,     0,     0,   631,   632,     0,     0,
     664,   665,     0,     0,  1414,   633,   634,   635,   636,   637,
       0,     0,   638,   639,   640,   641,     0,   642,   643,   644,
     645,   646,     0,   647,   648,     0,     0,     0,     0,   649,
     650,   651,     0,     0,     0,   652,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   653,     0,   654,   655,   656,   657,
     658,   659,   660,   661,   662,   663,     0,     0,     0,     0,
       0,     0,   631,   632,     0,     0,   664,   665,     0,     0,
    1416,   633,   634,   635,   636,   637,     0,     0,   638,   639,
     640,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,     0,     0,     0,     0,   649,   650,   651,     0,     0,
       0,   652,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     653,     0,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,     0,     0,     0,     0,     0,     0,   631,   632,
       0,     0,   664,   665,     0,     0,  1417,   633,   634,   635,
     636,   637,     0,     0,   638,   639,   640,   641,     0,   642,
     643,   644,   645,   646,     0,   647,   648,     0,     0,     0,
       0,   649,   650,   651,     0,     0,     0,   652,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   653,     0,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,     0,     0,
       0,     0,     0,     0,   631,   632,     0,     0,   664,   665,
       0,     0,  1430,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,     0,     0,     0,     0,   649,   650,   651,
       0,     0,     0,   652,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   653,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,     0,     0,     0,     0,
     631,   632,     0,     0,   664,   665,     0,     0,  1440,   633,
     634,   635,   636,   637,     0,     0,   638,   639,   640,   641,
       0,   642,   643,   644,   645,   646,     0,   647,   648,     0,
       0,     0,     0,   649,   650,   651,     0,     0,     0,   652,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   653,     0,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
       0,     0,     0,     0,     0,     0,   631,   632,     0,     0,
     664,   665,     0,     0,  1523,   633,   634,   635,   636,   637,
       0,     0,   638,   639,   640,   641,     0,   642,   643,   644,
     645,   646,     0,   647,   648,     0,     0,     0,     0,   649,
     650,   651,     0,     0,     0,   652,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   653,     0,   654,   655,   656,   657,
     658,   659,   660,   661,   662,   663,     0,     0,     0,     0,
     631,   632,     0,     0,     0,     0,   664,   665,     0,     0,
    1577,   633,   634,   635,   636,   637,     0,     0,   638,   639,
     640,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,     0,     0,     0,     0,   649,   650,   651,     0,     0,
       0,   652,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     653,     0,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,     0,     0,   631,   632,     0,     0,     0,     0,
       0,     0,   664,   665,   690,   633,   634,   635,   636,   637,
       0,     0,   638,   639,   640,   641,     0,   642,   643,   644,
     645,   646,     0,   647,   648,     0,     0,     0,     0,   649,
     650,   651,     0,     0,     0,   652,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   653,     0,   654,   655,   656,   657,
     658,   659,   660,   661,   662,   663,     0,     0,   631,   632,
       0,     0,     0,     0,     0,     0,   664,   665,   870,   633,
     634,   635,   636,   637,     0,     0,   638,   639,   640,   641,
       0,   642,   643,   644,   645,   646,     0,   647,   648,     0,
       0,     0,     0,   649,   650,   651,     0,     0,     0,   652,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   653,     0,
     654,   655,   656,   657,   658,   659,   660,   661,   662,   663,
       0,     0,   631,   632,     0,     0,     0,     0,     0,     0,
     664,   665,  1017,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,     0,     0,     0,     0,   649,   650,   651,
       0,     0,     0,   652,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   653,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,   631,   632,   187,     0,
       0,     0,     0,     0,   664,   665,  1033,   633,   634,   635,
     636,   637,     0,     0,   638,   639,   640,   641,     0,   642,
     643,   644,   645,   646,     0,   647,   648,     0,     0,     0,
       0,   649,   650,   651,   188,     0,   189,   652,   190,   191,
     192,   193,   194,     0,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,     0,   206,   207,   208,     0,
       0,   209,   210,   211,   212,     0,   653,     0,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,     0,     0,
       0,   213,   214,     0,     0,     0,     0,     0,   664,   665,
    1206,   633,   634,   635,   636,   637,   284,   285,   638,   639,
     640,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,     0,     0,   286,     0,   649,   650,   651,     0,     0,
       0,   652,     0,     0,     0,     0,   215,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     653,     0,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   664,   665,  1212,     0,     0,     0,     0,   631,
     632,     0,     0,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,     0,     0,   305,   306,   307,     0,     0,   308,   309,
     310,   311,   312,     0,     0,   313,   314,   315,   316,   317,
     318,   319,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   800,
     320,     0,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   631,   632,   331,   332,     0,     0,     0,     0,
       0,     0,   333,   334,   633,   634,   635,   636,   637,     0,
       0,   638,   639,   640,   641,     0,   642,   643,   644,   645,
     646,     0,   647,   648,     0,     0,   957,     0,   649,   650,
     651,   236,   237,   238,   652,   240,   241,   242,   243,   244,
     441,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,     0,   258,   259,   260,     0,     0,   263,   264,
     265,   266,     0,   653,     0,   654,   655,   656,   657,   658,
     659,   660,   661,   662,   663,   631,   632,     0,     0,     0,
       0,     0,     0,     0,     0,   664,   665,   633,   634,   635,
     636,   637,     0,     0,   638,   639,   640,   641,     0,   642,
     643,   644,   645,   646,     0,   647,   648,     0,     0,     0,
       0,   649,   650,   651,   801,     0,     0,   652,     0,     0,
       0,     0,     0,   802,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   804,   653,  1022,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,   631,   632,
       0,     0,     0,     0,     0,     0,     0,     0,   664,   665,
     633,   634,   635,   636,   637,     0,     0,   638,   639,   640,
     641,     0,   642,   643,   644,   645,   646,     0,   647,   648,
       0,     0,     0,     0,   649,   650,   651,   236,   237,   238,
     652,   240,   241,   242,   243,   244,   441,   246,   247,   248,
     249,   250,   251,   252,   253,   254,   255,   256,     0,   258,
     259,   260,     0,     0,   263,   264,   265,   266,     0,   653,
    1162,   654,   655,   656,   657,   658,   659,   660,   661,   662,
     663,   631,   632,     0,     0,     0,     0,     0,     0,     0,
       0,   664,   665,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,   631,   632,     0,     0,   649,   650,   651,
     805,     0,     0,   652,     0,     0,     0,     0,     0,   806,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   653,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   664,   665,   633,   634,   635,   636,
     637,     0,     0,   638,   639,   640,   641,     0,   642,   643,
     644,   645,   646,     0,   647,   648,   631,   632,     0,     0,
     649,   650,   651,     0,     0,     0,  -781,     0,   633,   634,
     635,   636,   637,     0,     0,   638,   639,   640,   641,     0,
     642,   643,   644,   645,   646,     0,   647,   648,   631,   632,
       0,     0,   649,     0,   651,   653,     0,   654,   655,   656,
     657,   658,   659,   660,   661,   662,   663,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   664,   665,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   654,
     655,   656,   657,   658,   659,   660,   661,   662,   663,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   664,
     665,   633,   634,   635,   636,   637,     0,     0,   638,   639,
     640,   641,     0,   642,   643,   644,   645,   646,     0,   647,
     648,   631,   632,     0,     0,   649,     0,     0,     0,     0,
       0,     0,     0,   633,   634,   635,   636,   637,     0,     0,
     638,   639,   640,   641,     0,   642,   643,   644,   645,   646,
       0,   647,   648,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   654,   655,   656,   657,   658,   659,   660,   661,
     662,   663,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   664,   665,  1018,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   654,   655,   656,   657,   658,   659,
     660,   661,   662,   663,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   664,   665,   633,   634,   635,   636,
     637,     0,     0,   638,   639,   640,   641,     0,   642,   643,
     644,   645,   646,     0,   647,   648,   236,   237,   238,     0,
     240,   241,   242,   243,   244,   441,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,     0,   258,   259,
     260,     0,     0,   263,   264,   265,   266,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   655,   656,
     657,   658,   659,   660,   661,   662,   663,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   664,   665,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1019,
       0,     0,     0,     0,     0,   236,   237,   238,  1020,   240,
     241,   242,   243,   244,   441,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,     0,   258,   259,   260,
       0,     0,   263,   264,   265,   266,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   840,   841,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   842,     0,
       0,     0,     0,     0,     0,     0,     0,   843,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   844,   845
};

static const yytype_int16 yycheck[] =
{
       1,    14,    15,   138,   139,   176,   380,   162,   790,   628,
     556,   560,   618,   619,   512,   958,    80,   787,   526,    20,
      21,    22,   694,   624,   889,  1097,   674,   568,   676,     8,
     678,    33,   525,   468,  1146,   704,   503,   799,   505,   757,
     507,   755,  1242,   417,   151,    20,    20,    19,    20,    50,
      63,    64,    65,    22,   151,  1460,    15,    16,    60,    61,
      33,    46,    20,   176,     5,    20,  1395,  1460,     0,  1460,
    1460,    33,   140,   141,   142,     7,     7,     5,     6,     5,
       6,   173,   217,   129,   130,    21,    22,   127,    57,   102,
     103,   104,   105,   163,   201,   154,   203,    25,    30,   165,
      32,  1506,    34,    31,   163,  1305,   203,   140,    40,   142,
     163,   156,   204,  1506,  1460,  1506,  1506,  1494,    50,    50,
    1449,   106,   124,    62,    56,   127,   128,    15,    16,   139,
     200,   133,   733,   201,   174,   741,   174,   177,   204,   745,
      68,    69,   201,   163,   685,   165,   581,   753,    80,   202,
     756,   197,   198,   176,  1513,  1532,   591,  1290,   222,   594,
    1506,   102,   103,   176,  1569,   180,   176,   173,   165,   718,
     102,   103,   174,   175,   102,   103,  1569,   179,  1569,  1569,
     182,   107,   118,   119,   204,   200,   165,  1546,   789,   624,
     126,   163,   347,   129,   130,   131,   132,   156,   173,   173,
     872,   742,   161,   200,   163,   207,   179,   166,   582,   344,
     972,   139,  1077,   348,   984,   173,   185,   179,   173,   198,
     221,   165,   596,  1569,   173,   148,   397,   206,   197,   163,
    1363,  1364,   667,   165,   952,   163,   198,   951,   180,   180,
     163,   200,    21,    22,  1429,   207,  1379,  1380,     8,   420,
     421,   422,   180,  1438,   186,   927,   200,   270,   200,   200,
    1372,   197,   198,   127,   148,   127,   198,   380,   156,   133,
     198,   133,   204,   161,   148,   163,    36,   205,   166,   163,
     415,   139,  1354,   173,   397,   349,    33,   173,   163,   163,
     126,   165,    34,   129,   130,  1480,  1481,   127,   733,  1432,
    1433,   181,   148,   133,   417,   163,   173,   420,   421,   422,
     174,   201,   174,    60,    61,   201,   200,   163,   163,   139,
     173,    63,   991,   148,   204,    12,   867,   498,   499,   796,
     204,   502,   206,   504,   201,   506,    23,    24,   163,   118,
     119,   186,   943,   717,   174,    21,    22,   126,   201,   128,
     129,   130,   131,   132,   200,   156,   730,   731,   866,   205,
    1008,   197,   198,  1228,   127,   107,   740,   380,   180,   877,
     863,    33,   200,   747,   748,   173,   750,   124,   752,   173,
     754,   128,   875,   932,   397,   498,   499,   197,   200,   502,
     132,   504,   998,   506,   127,   508,  1065,   139,    60,    61,
     133,   199,   180,  1185,   417,   199,   947,   420,   421,   422,
     173,   174,   525,   176,   427,   428,   179,   127,   197,   198,
      47,   163,   200,   133,   180,   127,  1208,   127,   175,  1294,
     971,   133,   179,   133,   181,   182,   154,   139,   154,   165,
      67,   174,   118,   119,   200,   163,  1118,   163,   190,   173,
     126,   163,   128,   129,   130,   131,   132,  1578,   200,   173,
     207,  1326,   124,   163,   174,  1586,   128,  1237,   173,   582,
     173,   163,   174,   163,   174,   164,   165,   201,   204,   179,
     206,   163,   200,   596,   173,   498,   499,   201,   623,   502,
     139,   504,   181,   506,   164,   508,   201,   163,   201,   148,
     173,   173,  1171,   173,   153,    57,   173,  1450,   184,   185,
     186,    63,   525,   175,   163,   204,   687,   179,   631,   632,
     182,   197,   198,   177,   167,   173,   199,   181,   995,   201,
    1178,   180,   173,   646,   201,    33,     5,     6,   551,  1404,
     173,   164,   165,  1010,   163,   207,  1194,    63,    64,    65,
     173,   664,   173,   201,   177,   164,   165,   163,   729,   560,
     201,   732,    60,    61,   173,   736,   173,    57,   201,   582,
     173,    57,   181,    63,   687,   173,   950,    63,   199,   173,
    1015,   204,   956,   596,   164,   165,   102,   103,   104,   105,
      57,   165,  1027,   173,   201,   204,    63,  1032,   201,  1464,
    1465,   181,  1143,   201,   717,   164,   165,   201,   621,   177,
     173,    57,   625,   181,   173,   173,   729,   730,   731,   732,
     994,   205,   181,   736,   204,   173,   124,   740,   177,   174,
     128,   154,   181,    33,   747,   748,   190,   750,   201,   752,
     163,   754,   755,   201,   139,   204,  1428,  1429,    21,    22,
     154,   199,  1434,   148,   164,   165,  1438,  1439,   177,   163,
      60,    61,   181,   173,   176,   176,   164,   165,   163,   181,
     181,   181,    57,    33,   687,   173,  1541,   175,    63,   173,
      57,   179,   176,   181,   182,   179,    63,   176,   200,   200,
     861,  1473,   181,   864,   204,   696,    57,   698,  1480,  1481,
      60,    61,    63,   164,   717,   176,   204,   708,  1573,   207,
     181,   200,   883,   154,   167,   828,   729,   730,   731,   732,
     721,  1219,   163,   736,   124,  1394,  1161,   740,   128,   200,
     163,   164,   177,  1168,   747,   748,   181,   750,   154,   752,
     173,   754,   755,   165,  1179,   118,   119,   163,   861,   176,
     863,   864,   179,   126,   140,   128,   129,   130,   131,   132,
     203,   896,   875,    66,   124,   177,  1548,   127,   128,   181,
     883,    10,    11,   133,   177,   175,   164,   165,   181,   179,
     177,   181,   182,   163,   181,   173,   164,   165,   164,   164,
     165,   163,   164,   181,    35,   173,   200,   798,   173,  1418,
      33,   173,   176,   181,    35,   165,   181,   207,   106,   182,
     183,   184,   185,   186,   174,   175,   204,   173,    33,   179,
     176,  1195,   182,   179,   197,   198,   204,    60,    61,   204,
     177,  1437,   177,   203,   181,   177,   181,   950,   951,   181,
     140,   141,   142,   956,   204,    60,    61,   207,   861,   163,
     863,   864,   108,   109,   110,   111,   112,   113,   114,   115,
     204,   205,   875,   167,   168,   169,   170,   180,   180,  1475,
     883,  1306,  1370,  1419,   875,   180,   877,   133,   167,   168,
     169,   994,  1053,   202,   203,   200,   205,   143,   144,   145,
     180,   124,   893,   167,   168,   128,   180,   180,   899,   900,
      21,    22,   167,   168,   169,   544,   545,   546,   200,   124,
     180,   912,   180,   128,   915,   916,   917,   918,   919,  1468,
    1539,   180,   923,    54,    55,    56,   180,   180,   200,   198,
     177,   932,   205,   934,   205,   163,   163,   950,   951,   205,
    1053,  1376,   175,   956,   163,    22,   179,   199,   181,   182,
     163,   199,   176,   163,   163,   198,  1575,   163,   200,   204,
     175,   357,  1511,   201,   179,   205,   181,   182,  1123,   205,
     163,   367,   176,   180,   207,   180,    13,   180,   200,   180,
    1415,   994,   378,   180,   200,   180,   200,   198,   180,   180,
      33,   180,   207,    21,    22,   116,   117,   118,   119,   120,
     200,   200,   123,   124,   125,   126,   200,   128,   129,   130,
     131,   132,    75,   134,   135,   200,    79,    60,    61,   200,
     200,   203,   200,   200,   163,  1160,  1197,   201,   163,   199,
      93,    94,   200,   163,    43,    98,    99,   100,   101,   181,
    1053,   200,   180,   180,   180,   200,    13,   199,   173,   445,
     446,   200,   200,   173,   201,     4,   163,   203,  1193,   180,
     181,   182,   183,   184,   185,   186,   462,   463,   464,   465,
     466,   176,   201,   200,    43,   163,   197,   198,   163,   163,
     163,   124,  1195,   173,  1197,   128,   200,   200,   116,   117,
     118,   119,   120,  1467,   200,   123,   124,   125,   126,   201,
     128,   129,   130,   131,   132,   201,   134,   135,   200,   180,
     180,   200,   140,   141,   142,     1,   163,   181,   146,   200,
     200,   200,   200,   181,   200,   521,   200,  1128,   181,   163,
     163,   163,   175,   156,    10,    13,   179,   165,   181,   182,
       9,    42,    66,   174,   540,   206,    43,   175,   200,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   201,
     201,   201,   201,  1334,   207,   201,   201,   163,   564,   197,
     198,   200,    43,   200,   200,   180,   204,   201,   200,   200,
     200,   206,  1195,   200,  1197,    43,    14,   583,   584,   174,
     206,   587,  1327,   589,   206,   206,   176,    21,    22,   156,
     200,   200,   200,   599,   600,   601,   602,   603,   604,   200,
     200,    37,     8,   163,   201,   163,   200,   171,   163,   201,
     163,  1334,   163,   200,   163,   163,   163,   206,    67,   200,
     200,   163,   200,    21,    22,   181,   201,   633,   634,   200,
     200,   637,   638,   639,   640,    70,   642,   200,   644,   645,
     646,   647,   648,   649,   650,   651,   652,   653,   654,   655,
     656,   657,   658,   659,   660,   661,   662,   663,  1269,   665,
    1271,   200,    43,   181,   163,   138,   201,   205,   200,  1414,
     200,  1416,  1417,   200,   200,   167,   204,   201,   163,   204,
     201,   201,   116,   117,   118,   119,   120,    12,    33,   123,
     124,   125,   126,    33,   128,   129,   130,   131,   132,   201,
     134,   135,   708,   201,   204,   206,   173,   173,   714,    53,
     205,  1334,   201,   204,   720,    60,    61,   201,   116,   117,
     118,   119,   205,   201,   201,   201,   201,   201,   126,   735,
     128,   129,   130,   131,   132,   200,   134,   135,   201,   199,
     605,   200,   206,  1354,  1467,   179,   180,   181,   182,   183,
     184,   185,   186,   199,   205,   205,    79,     1,   206,   206,
     766,    44,  1507,   197,   198,   771,   130,   773,    82,   775,
    1585,   777,  1395,  1582,   228,   101,   928,   698,   698,   124,
     625,     1,   788,   128,   182,   183,   184,   185,   186,  1534,
     629,   487,   798,  1324,  1471,  1424,  1427,  1358,    53,   197,
     198,  1472,  1472,   927,  1118,   341,   583,   427,   927,   427,
      -1,  1439,    -1,    -1,    -1,    -1,    -1,    -1,  1563,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1449,    -1,    -1,    -1,
     175,    -1,  1577,    -1,   179,   841,   181,   182,    -1,   845,
      -1,    19,    -1,    -1,  1467,    -1,    -1,    25,    -1,    -1,
    1461,    -1,    -1,    31,    -1,  1466,    -1,    -1,    -1,    -1,
      -1,    -1,   207,    41,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    -1,    -1,   880,    -1,    -1,    -1,    -1,   885,
      -1,   887,    -1,    -1,   890,    -1,    64,    -1,    -1,   895,
      -1,   897,    -1,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    -1,   933,    -1,  1540,
      -1,    -1,    -1,  1544,  1545,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1557,    -1,    -1,  1560,
      -1,   957,   958,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   139,    -1,    -1,    -1,  1576,    -1,    -1,    -1,  1580,
      -1,    21,    22,    -1,   152,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   163,   992,   993,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1003,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1012,    -1,  1014,    -1,
    1016,    -1,    -1,    -1,    -1,    -1,  1022,    -1,    -1,    -1,
    1026,    -1,    -1,    -1,   202,    -1,    -1,    33,  1034,   207,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,    -1,
      33,  1067,    -1,    -1,  1070,    -1,   116,   117,   118,   119,
     120,    -1,  1078,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,   132,    -1,   134,   135,    -1,    60,    61,    -1,
     140,   141,   142,  1099,  1100,  1101,  1102,  1103,  1104,  1105,
    1106,  1107,  1108,  1109,  1110,  1111,  1112,  1113,  1114,  1115,
    1116,  1117,    -1,    -1,    -1,    -1,    -1,    -1,   124,    -1,
      -1,  1127,   128,    -1,    -1,   175,    -1,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,    -1,    -1,    -1,
    1146,    -1,    10,    -1,    -1,    -1,    -1,   197,   198,    -1,
      -1,   124,    -1,    21,    22,   128,    -1,    -1,  1164,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1172,  1173,  1174,   175,
      -1,    -1,    -1,   179,    -1,   181,   182,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1203,    -1,  1205,
      -1,   207,   175,    -1,    -1,  1211,   179,    -1,   181,   182,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1226,    -1,    -1,    -1,  1230,    -1,    21,    22,    -1,  1235,
    1236,    -1,  1238,  1239,   207,    -1,    -1,    -1,    -1,    -1,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
      -1,  1277,   140,   141,   142,   143,   144,   145,   146,    -1,
    1286,  1287,  1288,    -1,    -1,    -1,    -1,  1293,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1307,  1308,    -1,    -1,    -1,   174,   175,    -1,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,    -1,
      -1,   116,   117,   118,   119,   120,    -1,    -1,   123,   197,
     198,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,    -1,    -1,    33,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1372,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,  1401,    -1,    -1,    -1,    -1,
      -1,    -1,   197,   198,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1431,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1442,    -1,   124,    -1,
    1446,  1447,   128,    -1,  1450,    -1,    -1,  1453,    -1,   116,
     117,   118,   119,   120,  1460,    -1,   123,   124,   125,   126,
    1466,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,   140,   141,   142,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,  1490,    -1,    -1,    -1,    -1,   175,
      -1,    -1,    -1,   179,    -1,   181,   182,    -1,    -1,    -1,
    1506,    -1,    -1,    -1,    -1,    -1,  1512,    -1,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,   207,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     197,   198,    -1,    -1,   201,    -1,    -1,  1543,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1551,    -1,    -1,    -1,    -1,
    1556,  1557,     1,    -1,    -1,    -1,     5,     6,     7,    -1,
       9,    10,    11,  1569,    13,    -1,    15,    16,    17,    18,
      19,    -1,    -1,    -1,  1580,    -1,    25,    26,    27,    28,
      29,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      39,    -1,    -1,    42,    -1,    44,    45,    -1,    -1,    48,
      -1,    50,    51,    52,    -1,    54,    55,    -1,    -1,    58,
      59,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    -1,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,   105,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,   148,
     149,   150,   151,    -1,   153,    -1,   155,   156,   157,   158,
     159,   160,   161,   162,   163,    -1,   165,   166,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   182,   183,   184,    -1,   186,    -1,    -1,
     189,   190,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,
      -1,   200,    -1,   202,   203,   204,   205,   206,     1,    -1,
      -1,    -1,     5,     6,     7,    -1,     9,    10,    11,    -1,
      13,    -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    29,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    39,    -1,    -1,    42,
      -1,    44,    45,    -1,    -1,    48,    -1,    50,    51,    52,
      -1,    54,    55,    -1,    -1,    58,    59,    -1,    -1,    -1,
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
     163,    -1,   165,   166,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,
     183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   198,    -1,   200,    -1,   202,
     203,   204,   205,   206,     1,    -1,    -1,    -1,     5,     6,
       7,    -1,     9,    10,    11,    -1,    13,    -1,    15,    16,
      17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    29,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    39,    -1,    -1,    42,    -1,    44,    45,    -1,
      -1,    48,    -1,    50,    51,    52,    -1,    54,    55,    -1,
      -1,    58,    59,    -1,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    71,    72,    73,    -1,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,   102,   103,   104,   105,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     147,   148,   149,   150,   151,    -1,   153,    -1,   155,   156,
     157,   158,   159,   160,   161,   162,   163,    -1,   165,   166,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,   186,
      -1,    -1,   189,   190,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,    -1,   200,    -1,   202,   203,   204,   205,   206,
       1,    -1,    -1,    -1,     5,     6,     7,    -1,     9,    10,
      11,    -1,    13,    -1,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    -1,
      -1,    42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,
      51,    52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,
      -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,   102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,
     151,    -1,   153,    -1,   155,   156,   157,   158,   159,   160,
     161,   162,   163,    -1,   165,   166,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   182,   183,   184,    -1,   186,    -1,    -1,   189,   190,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,    -1,   200,
      -1,   202,   203,   204,   205,   206,     1,    -1,    -1,    -1,
       5,     6,     7,    -1,     9,    10,    11,    -1,    13,    -1,
      15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,
      25,    26,    27,    28,    29,    -1,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    39,    -1,    -1,    42,    -1,    44,
      45,    -1,    -1,    48,    -1,    50,    51,    52,    -1,    54,
      55,    -1,    -1,    58,    59,    -1,    -1,    -1,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   147,   148,   149,   150,   151,    -1,   153,    -1,
     155,   156,   157,   158,   159,   160,   161,   162,   163,    -1,
     165,   166,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,
      -1,   186,    -1,    -1,   189,   190,    -1,    -1,    -1,    -1,
      60,    61,    -1,   198,    -1,   200,    -1,   202,   203,   204,
     205,   206,     5,     6,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    15,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,    52,
      -1,    -1,    55,    -1,   124,    -1,    -1,    -1,   128,    -1,
      -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,    72,
      73,    -1,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,   102,
     103,   104,   105,    -1,    -1,   175,    -1,    -1,    -1,   179,
      -1,   181,   182,    -1,    -1,   118,   119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   139,   207,    -1,    -1,
      -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,
     153,    -1,   155,   156,   157,   158,   159,   160,   161,   162,
     163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,
     183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,
      -1,    -1,    -1,     5,     6,   198,    -1,   200,    -1,   202,
     203,    -1,   205,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    51,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    70,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,
      -1,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     182,   183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,
      -1,    -1,    -1,    -1,     5,     6,   198,    -1,   200,   201,
     202,   203,    13,   205,    15,    16,    17,    18,    19,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,
     151,    -1,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,    -1,    -1,   166,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   182,   183,   184,    -1,   186,    -1,    -1,   189,   190,
      -1,    -1,    -1,    -1,    -1,     5,     6,   198,    -1,   200,
      -1,   202,   203,    13,   205,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      33,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    49,
      -1,    51,    -1,    -1,    -1,    55,    -1,    60,    61,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,   119,
      -1,   124,    -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,   148,   149,
     150,   151,    -1,   153,    -1,   155,   156,   157,   158,   159,
     160,   161,   162,   163,    -1,    -1,   166,    -1,    -1,    -1,
      -1,    -1,   175,    -1,    -1,    -1,   179,    -1,   181,   182,
      -1,    -1,   182,   183,   184,    -1,   186,    -1,    -1,   189,
     190,    -1,    -1,    -1,    -1,    -1,     5,     6,   198,    -1,
     200,    -1,   202,   203,   207,   205,    15,    16,    17,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,
      -1,    33,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    60,    61,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,    68,
      69,    70,    71,    72,    73,    -1,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    93,    94,    95,    -1,    -1,    98,
      99,   100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,    -1,   124,    -1,    -1,    -1,   128,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,   148,
     149,   150,   151,    -1,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,    -1,    -1,   166,    -1,    -1,
      -1,    -1,    -1,   175,    -1,    -1,    -1,   179,    -1,   181,
     182,    -1,    -1,   182,   183,   184,    -1,   186,    -1,    -1,
     189,   190,    -1,    -1,    -1,    -1,    -1,     5,     6,   198,
      -1,   200,    -1,   202,   203,   207,   205,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    33,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    -1,    -1,    -1,    55,    -1,    60,
      61,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,    -1,   124,    -1,    -1,    -1,   128,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   147,
     148,   149,   150,   151,    -1,   153,    -1,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,   166,    -1,
      -1,    -1,    -1,    -1,   175,    -1,    -1,    -1,   179,    -1,
     181,   182,    -1,    -1,   182,   183,   184,    -1,   186,    -1,
      -1,   189,   190,    -1,    -1,    -1,    -1,    -1,     5,     6,
     198,   199,   200,    -1,   202,   203,   207,   205,    15,    16,
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
      -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     147,   148,   149,   150,   151,    -1,   153,   154,   155,   156,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   147,   148,   149,   150,   151,    -1,   153,    -1,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    -1,    -1,
     166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   182,   183,   184,    -1,
     186,    -1,    -1,   189,   190,    -1,    -1,    -1,    -1,    -1,
       5,     6,   198,   199,   200,    -1,   202,   203,    -1,   205,
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
      -1,    -1,     5,     6,   198,    -1,   200,   201,   202,   203,
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
      -1,    -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,    -1,
     153,    -1,   155,   156,   157,   158,   159,   160,   161,   162,
     163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,
     183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,    -1,
      -1,    -1,    -1,     5,     6,   198,    -1,   200,    -1,   202,
     203,    -1,   205,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   147,   148,   149,   150,   151,
      -1,   153,    -1,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,   166,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     182,   183,   184,    -1,   186,    -1,    -1,   189,   190,    -1,
      -1,    -1,    -1,    -1,     5,     6,   198,    -1,   200,    -1,
     202,   203,    -1,   205,    15,    16,    17,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      51,    -1,    -1,    -1,    55,    -1,    -1,    58,    -1,    -1,
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
     119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
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
     118,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,     5,     6,   198,    -1,   200,    -1,   202,
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
     162,   163,    -1,    19,   166,    -1,    -1,    -1,    -1,    25,
      -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
     182,   183,   184,    -1,   186,    41,    -1,   189,   190,    -1,
      -1,    -1,    -1,    49,    -1,    -1,   198,    -1,   200,    -1,
     202,   203,    -1,   205,    -1,    -1,    -1,    -1,    64,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,
      -1,    -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    -1,    -1,   139,    -1,    -1,    -1,    -1,    49,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   152,    -1,    -1,    -1,
      -1,    -1,    -1,    64,    -1,    -1,    -1,   163,    -1,   165,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,    -1,    -1,    -1,   202,    -1,   204,    21,
      22,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    -1,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,   139,    -1,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   152,    -1,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    -1,   163,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    -1,    -1,    -1,    -1,
      -1,   202,    21,    22,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
     132,    -1,   134,   135,    -1,    -1,    -1,    -1,   140,   141,
     142,   139,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   152,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   163,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   197,   198,    -1,    21,    22,
      -1,    -1,   204,    -1,   202,    -1,    -1,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,   140,   141,   142,    -1,    -1,    -1,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,   197,   198,
      -1,    -1,   201,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,   140,   141,   142,
      -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   175,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,   197,   198,    -1,    -1,   201,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,   140,   141,   142,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
     197,   198,    -1,    -1,   201,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,   197,   198,    -1,    -1,
     201,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,   140,   141,   142,    -1,    -1,
      -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,   197,   198,    -1,    -1,   201,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,   140,   141,   142,    -1,    -1,    -1,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,   197,   198,
      -1,    -1,   201,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,   140,   141,   142,
      -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   175,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,   197,   198,    -1,    -1,   201,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,   140,   141,   142,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
     197,   198,    -1,    -1,   201,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,   197,   198,    -1,    -1,
     201,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,   140,   141,   142,    -1,    -1,
      -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,   197,   198,    -1,    -1,   201,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,   140,   141,   142,    -1,    -1,    -1,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,   197,   198,
      -1,    -1,   201,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,   140,   141,   142,
      -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   175,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,   197,   198,    -1,    -1,   201,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,   140,   141,   142,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
     197,   198,    -1,    -1,   201,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,   197,   198,    -1,    -1,
     201,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,   140,   141,   142,    -1,    -1,
      -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,   197,   198,    -1,    -1,   201,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,   140,   141,   142,    -1,    -1,    -1,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,   197,   198,
      -1,    -1,   201,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,   140,   141,   142,
      -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   175,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,   197,   198,    -1,    -1,   201,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,   140,   141,   142,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
     197,   198,    -1,    -1,   201,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,   197,   198,    -1,    -1,
     201,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,   140,   141,   142,    -1,    -1,
      -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,   197,   198,    -1,    -1,   201,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,   140,   141,   142,    -1,    -1,    -1,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,   197,   198,
      -1,    -1,   201,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,   140,   141,   142,
      -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   175,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,   197,   198,    -1,    -1,   201,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,   140,   141,   142,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
     197,   198,    -1,    -1,   201,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,   197,   198,    -1,    -1,
     201,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,   140,   141,   142,    -1,    -1,
      -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,   197,   198,    -1,    -1,   201,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,   140,   141,   142,    -1,    -1,    -1,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,   197,   198,
      -1,    -1,   201,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,   140,   141,   142,
      -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   175,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,   197,   198,    -1,    -1,   201,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,   140,   141,   142,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
     197,   198,    -1,    -1,   201,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,   197,   198,    -1,    -1,
     201,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,   140,   141,   142,    -1,    -1,
      -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,   197,   198,    -1,    -1,   201,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,   140,   141,   142,    -1,    -1,    -1,   146,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,   197,   198,
      -1,    -1,   201,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,   140,   141,   142,
      -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   175,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,   197,   198,    -1,    -1,   201,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,   140,   141,   142,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
     197,   198,    -1,    -1,   201,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,    -1,    -1,   197,   198,    -1,    -1,
     201,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,   140,   141,   142,    -1,    -1,
      -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,   197,   198,   199,   116,   117,   118,   119,   120,
      -1,    -1,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,   140,
     141,   142,    -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,   197,   198,   199,   116,
     117,   118,   119,   120,    -1,    -1,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,   140,   141,   142,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
     197,   198,   199,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,   140,   141,   142,
      -1,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   175,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    21,    22,    35,    -1,
      -1,    -1,    -1,    -1,   197,   198,   199,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,   140,   141,   142,    71,    -1,    73,   146,    75,    76,
      77,    78,    79,    -1,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    93,    94,    95,    -1,
      -1,    98,    99,   100,   101,    -1,   175,    -1,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,    -1,   197,   198,
     199,   116,   117,   118,   119,   120,    21,    22,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    38,    -1,   140,   141,   142,    -1,    -1,
      -1,   146,    -1,    -1,    -1,    -1,   163,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   197,   198,   199,    -1,    -1,    -1,    -1,    21,
      22,    -1,    -1,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,    -1,    -1,   128,   129,   130,    -1,    -1,   133,   134,
     135,   136,   137,    -1,    -1,   140,   141,   142,   143,   144,
     145,   146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,
     175,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    21,    22,   189,   190,    -1,    -1,    -1,    -1,
      -1,    -1,   197,   198,   116,   117,   118,   119,   120,    -1,
      -1,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
     132,    -1,   134,   135,    -1,    -1,   138,    -1,   140,   141,
     142,    71,    72,    73,   146,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,    -1,   175,    -1,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   197,   198,   116,   117,   118,
     119,   120,    -1,    -1,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,   140,   141,   142,   154,    -1,    -1,   146,    -1,    -1,
      -1,    -1,    -1,   163,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   197,   198,
     116,   117,   118,   119,   120,    -1,    -1,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,   132,    -1,   134,   135,
      -1,    -1,    -1,    -1,   140,   141,   142,    71,    72,    73,
     146,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,    -1,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   197,   198,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    21,    22,    -1,    -1,   140,   141,   142,
     154,    -1,    -1,   146,    -1,    -1,    -1,    -1,    -1,   163,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   175,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   197,   198,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,   132,    -1,   134,   135,    21,    22,    -1,    -1,
     140,   141,   142,    -1,    -1,    -1,   146,    -1,   116,   117,
     118,   119,   120,    -1,    -1,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,   132,    -1,   134,   135,    21,    22,
      -1,    -1,   140,    -1,   142,   175,    -1,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   197,   198,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   197,
     198,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,    -1,   134,
     135,    21,    22,    -1,    -1,   140,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,   117,   118,   119,   120,    -1,    -1,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   197,   198,    19,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   197,   198,   116,   117,   118,   119,
     120,    -1,    -1,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,   132,    -1,   134,   135,    71,    72,    73,    -1,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    93,    94,
      95,    -1,    -1,    98,    99,   100,   101,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   178,   179,
     180,   181,   182,   183,   184,   185,   186,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   197,   198,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   154,
      -1,    -1,    -1,    -1,    -1,    71,    72,    73,   163,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   129,   130,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   163,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   197,   198
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   209,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   102,   103,   165,   186,   198,   204,   211,   212,   216,
     225,   227,   228,   232,   277,   281,   310,   391,   399,   406,
     416,   463,   468,   473,    19,    20,   163,   268,   269,   270,
     156,   233,   234,   163,   186,   229,   230,    57,    63,   396,
     397,   163,   202,   214,   474,   464,   469,   139,   163,   298,
      34,    63,   107,   132,   190,   200,   272,   273,   274,   275,
     298,   211,   211,   211,     8,    36,   417,    62,   387,   174,
     173,   176,   173,   229,    22,    57,   185,   197,   231,   163,
     211,   387,   396,   396,   396,   163,   139,   226,   274,   274,
     274,   200,   140,   141,   142,   173,   199,    57,    63,   282,
     284,   407,     5,     6,   107,   413,    57,    63,   388,    15,
      16,   156,   161,   163,   166,   200,   218,   269,   156,   234,
     163,   163,   163,   398,    57,    63,   213,   163,   163,   163,
     163,   167,   224,   201,   270,   274,   274,   274,   274,   165,
     238,   239,   285,    57,    63,   400,     5,     6,    57,    63,
     414,   205,   392,   167,   168,   169,   217,    15,    16,   156,
     161,   163,   218,   266,   267,   231,   174,   190,   215,   164,
     404,   405,   239,   239,   167,   201,   165,    35,    71,    73,
      75,    76,    77,    78,    79,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    93,    94,    95,    98,
      99,   100,   101,   118,   119,   163,   280,   283,   163,   401,
     418,   389,   203,   271,   357,   167,   168,   169,   173,   201,
      19,    25,    31,    41,    49,    64,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   152,
     202,   298,   421,   423,   424,   427,   433,   434,   462,    66,
     164,   475,   465,   470,    21,    22,    38,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   128,   129,   130,   133,   134,
     135,   136,   137,   140,   141,   142,   143,   144,   145,   146,
     175,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   189,   190,   197,   198,    35,    35,   200,   278,   239,
     176,   402,   106,   411,   412,   206,   211,   390,   269,   203,
     163,   383,   386,   266,   180,   180,   180,   200,   180,   180,
     200,   180,   180,   180,   180,   180,   180,   200,   298,    33,
      60,    61,   124,   128,   175,   179,   182,   207,   198,   432,
     177,   205,   205,   205,   163,   163,   163,   199,    22,   163,
     199,   151,   201,   357,   367,   368,   369,   176,   279,   289,
     290,    75,    79,    93,    94,    98,    99,   100,   101,   422,
     405,   163,   239,   357,   239,   269,   173,   176,   179,   381,
     435,   440,   442,     5,     6,    15,    16,    17,    18,    19,
      25,    27,    31,    39,    45,    48,    51,    55,    65,    68,
      69,    80,   102,   103,   104,   118,   119,   147,   148,   149,
     150,   151,   153,   155,   156,   157,   158,   159,   160,   161,
     162,   166,   182,   183,   184,   189,   190,   198,   200,   202,
     203,   205,   223,   225,   292,   298,   303,   315,   322,   325,
     328,   332,   334,   336,   337,   339,   344,   347,   348,   355,
     356,   421,   478,   486,   496,   499,   511,   515,   444,   438,
     163,   428,   446,   448,   450,   452,   454,   456,   458,   460,
     348,   180,   200,    33,   179,    33,   179,   198,   207,   199,
     348,   198,   207,   433,   476,   466,   471,   163,   108,   109,
     110,   111,   112,   113,   114,   115,   133,   143,   144,   145,
     200,     7,    50,   309,   204,   173,   204,   201,   462,   205,
     205,   176,   410,   419,   148,   163,   382,   385,   239,   163,
     433,   127,   133,   174,   380,   462,   462,   431,   462,   180,
     180,   180,   300,   423,   478,   298,   180,     5,   102,   103,
     180,   200,   180,   200,   200,   180,   180,   200,   180,   200,
     180,   200,   180,   180,   200,   180,   180,   348,   348,   200,
     200,   200,   200,   200,   200,   222,   348,   348,   348,   348,
     348,    13,    49,   295,   326,   348,   154,   163,   326,   479,
     481,   203,    13,   512,   200,   198,   276,   203,   205,   328,
     333,    21,    22,   116,   117,   118,   119,   120,   123,   124,
     125,   126,   128,   129,   130,   131,   132,   134,   135,   140,
     141,   142,   146,   175,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   197,   198,   201,   200,   462,   462,
     201,   163,   425,   462,   278,   462,   278,   462,   278,   163,
     370,   371,   462,   163,   373,   374,   201,   437,   295,   199,
     199,   199,   348,   163,   426,   370,   372,   373,   375,   348,
     148,   163,   377,   378,   420,   369,   369,   369,   294,   408,
     298,   204,   205,   415,   200,    43,   173,   176,   381,   211,
     380,   348,   181,   181,   181,   164,   173,   210,   211,   430,
     487,   489,   301,   200,   180,   200,   323,   180,   180,   180,
     506,   326,   433,   348,   495,   510,   348,   316,   318,   348,
     320,   348,   508,   326,   493,   497,   326,   491,   433,   348,
     348,   348,   348,   348,   348,   169,   170,   217,   200,    13,
     199,   173,   514,   200,   127,   133,   174,   376,   514,   173,
     514,   201,   148,   153,   180,   298,   338,   200,   239,    70,
     198,   201,   326,   481,   275,     4,   331,   203,   294,   276,
      19,   154,   163,   421,    19,   154,   163,   421,   348,   348,
     348,   348,   348,   348,   163,   348,   154,   163,   348,   348,
     348,   421,   348,   348,   348,   348,   348,   348,    22,   348,
     348,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     129,   130,   154,   163,   197,   198,   345,   421,   348,   201,
     326,   181,   181,   181,   204,   181,   279,   181,   279,   181,
     279,   176,   181,   430,   176,   181,   430,   297,   462,   201,
     199,   210,   394,   403,   211,   244,   245,   244,   245,   201,
     200,    43,   173,   176,   179,   376,     1,    26,    28,    29,
      38,    44,    52,    54,    58,    59,    65,   105,   211,   235,
     236,   243,   254,   255,   256,   258,   259,   260,   261,   262,
     263,   264,   265,   291,   293,   299,   304,   305,   306,   307,
     308,   310,   314,   335,   348,   163,   393,   395,   363,   348,
     163,   163,   433,   380,   348,   211,   436,   441,   443,   462,
     433,   433,   462,    70,   326,   481,   485,   163,   348,   462,
     500,   502,   504,   433,   514,   181,   430,   138,   173,   514,
     201,   433,   433,   201,   433,   201,   433,   514,   433,   371,
     514,   374,   181,   201,   201,   201,   201,   201,   201,   348,
     148,   163,   200,   257,   200,   348,   348,   348,   348,   201,
     154,   163,   200,   200,   340,   342,   257,   296,   510,   163,
     201,   481,   479,   173,   201,   201,   199,   200,   278,   206,
     331,   180,   200,   180,   200,   200,   200,   199,    19,   154,
     163,   421,   176,   154,   163,   348,   200,   200,   154,   163,
     348,     1,   200,   199,   173,   201,   445,   439,   429,   163,
     447,   181,   451,   181,   455,   181,   462,   459,   370,   462,
     461,   373,   181,   430,   163,   210,   477,   211,   370,   467,
     373,   472,   348,   163,   163,   462,   348,   127,   348,   289,
      61,   348,   163,   211,   156,    58,   348,   239,   127,   348,
     211,   211,    10,    10,    11,   241,    13,     9,    42,   211,
     206,   211,   211,   211,   211,   211,    66,   311,   211,   108,
     109,   110,   111,   112,   113,   114,   115,   121,   122,   127,
     133,   136,   137,   143,   144,   145,   174,   174,   394,   403,
     165,   206,   277,   364,   201,    43,   211,   380,   348,   211,
     181,   181,   181,   481,   201,   201,   201,   181,   430,   201,
     181,   433,   371,   374,   181,   201,   200,   433,   348,   495,
     201,   181,   181,   181,   181,   201,   181,   181,   201,   181,
     331,   200,   176,   220,   200,    43,   163,   312,    20,   173,
     257,   201,   200,   133,   376,   348,   348,   433,   278,    20,
     206,   514,   201,   173,   199,   198,   127,   133,   163,   174,
     179,   329,   330,   279,   278,   349,   348,   351,   348,   201,
     326,   348,   180,   200,   348,   200,   199,   348,   198,   201,
     326,   200,   199,   346,   201,   326,   449,   453,   457,   200,
     462,   206,   206,   206,   201,    43,   376,   348,    14,   348,
     174,   176,   156,   289,   348,   200,   200,   200,   200,   200,
      37,   286,   200,   205,   313,   385,   348,   348,   348,   348,
     348,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     348,   348,   348,   348,   348,   393,   409,     8,   357,   362,
     163,   348,   211,   488,   490,   302,   201,   200,   163,   324,
     181,   181,   181,   507,   296,   181,   317,   319,   321,   509,
     494,   498,   492,   200,   239,   201,   326,   221,   171,   348,
     163,   173,   201,   326,   163,   200,    20,   133,   376,   348,
     348,   348,   201,   201,   181,   279,   326,   201,   479,   163,
     163,   200,   163,   163,   173,   201,   239,   279,   433,   201,
     462,   201,   201,   201,   353,   348,   348,   201,   479,   201,
     348,   201,   295,   163,   348,   289,   348,   348,   348,   257,
     348,   348,   287,   312,   384,   206,    57,    63,   360,    67,
     361,   211,   211,   200,   200,   348,   181,   501,   503,   505,
     200,   201,   200,   348,   348,   348,   200,    70,   485,   200,
     200,   201,   348,   289,   201,   219,   201,   163,   201,    43,
     312,   326,   348,   348,   201,   341,   181,   201,   199,   163,
     329,   138,   289,   327,   239,   181,   181,   462,   201,   201,
     199,   201,   201,   201,   201,    20,   201,   201,   205,   201,
     206,   211,   385,    47,   361,    46,   106,   358,   485,   485,
     201,   200,   200,   200,   200,   295,   296,   326,   485,   485,
     201,   167,   204,   163,   201,   201,   133,   376,   338,   343,
     204,   201,   201,   127,   348,   289,   350,   352,   181,   201,
      12,   242,   239,   326,   239,   239,   294,   176,   381,    33,
     359,   358,   360,   200,   479,   482,   483,   484,   484,   348,
     485,   485,   479,   480,   201,   201,   514,   484,   480,   348,
     204,   348,   348,   338,   495,   348,   354,   243,   304,   305,
     306,   307,   348,   211,   246,   247,   249,   201,   289,   289,
     288,   433,   380,   365,   359,   377,   378,   379,   479,   173,
     514,   201,   201,   201,   484,   484,   201,   201,   201,   201,
     204,   513,   348,   513,    12,    23,    24,   237,   240,   205,
     243,   239,   206,   380,   348,   283,   366,   201,   200,   201,
     201,    53,   199,   513,   206,   239,   200,   294,   211,   289,
     348,   211,   211,   283,   479,   348,   199,   250,   251,   253,
     348,   248,   211,   239,   201,   205,   243,   201,   206,   289,
     294,   211,   239,   286,   252,   250,   206,   240,   286
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
     238,   239,   239,   240,   240,   240,   241,   241,   242,   242,
     243,   243,   243,   243,   243,   244,   244,   245,   245,   247,
     248,   246,   249,   246,   251,   252,   250,   253,   250,   255,
     254,   256,   257,   257,   257,   257,   257,   257,   257,   259,
     258,   260,   262,   261,   264,   263,   265,   266,   266,   266,
     266,   266,   266,   267,   267,   268,   268,   268,   269,   269,
     269,   269,   269,   269,   269,   269,   270,   270,   271,   271,
     272,   272,   272,   272,   273,   273,   274,   274,   274,   274,
     274,   274,   274,   275,   275,   276,   276,   277,   277,   278,
     278,   278,   279,   279,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     281,   282,   282,   282,   283,   285,   284,   286,   287,   288,
     286,   290,   291,   289,   292,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   294,   294,   294,   295,   295,   296,
     296,   297,   297,   298,   298,   298,   299,   299,   301,   302,
     300,   300,   303,   303,   303,   303,   303,   303,   304,   305,
     306,   306,   306,   307,   307,   308,   309,   309,   309,   310,
     310,   311,   311,   312,   312,   313,   313,   314,   314,   314,
     316,   317,   315,   318,   319,   315,   320,   321,   315,   323,
     324,   322,   325,   325,   325,   326,   326,   327,   327,   327,
     328,   328,   328,   329,   329,   329,   329,   329,   330,   330,
     331,   331,   332,   333,   333,   334,   334,   334,   334,   334,
     334,   334,   335,   335,   335,   335,   335,   335,   335,   335,
     335,   335,   335,   335,   335,   335,   335,   335,   335,   335,
     335,   336,   336,   337,   337,   338,   338,   339,   340,   341,
     339,   342,   343,   339,   344,   344,   344,   344,   344,   344,
     344,   345,   346,   344,   347,   347,   347,   347,   347,   347,
     347,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     348,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     348,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     348,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     348,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     348,   348,   348,   348,   348,   348,   348,   349,   350,   348,
     348,   348,   348,   351,   352,   348,   348,   348,   353,   354,
     348,   348,   348,   348,   348,   348,   348,   348,   348,   348,
     348,   348,   348,   348,   355,   355,   355,   356,   356,   356,
     356,   356,   356,   356,   356,   356,   356,   356,   356,   356,
     356,   356,   356,   357,   357,   358,   358,   358,   359,   359,
     360,   360,   360,   361,   361,   362,   363,   363,   364,   363,
     365,   363,   366,   363,   367,   368,   368,   369,   369,   369,
     369,   369,   370,   370,   371,   371,   372,   372,   372,   373,
     374,   374,   375,   375,   375,   376,   376,   377,   377,   377,
     378,   378,   379,   379,   380,   380,   380,   381,   381,   382,
     382,   382,   382,   382,   383,   383,   384,   384,   384,   385,
     385,   385,   386,   386,   386,   387,   387,   388,   388,   388,
     389,   389,   390,   389,   391,   392,   391,   393,   393,   394,
     394,   395,   395,   395,   396,   396,   396,   398,   397,   399,
     400,   400,   400,   401,   402,   402,   403,   403,   404,   404,
     405,   405,   407,   408,   409,   406,   410,   410,   411,   411,
     412,   413,   413,   413,   413,   414,   414,   414,   415,   415,
     417,   418,   419,   416,   420,   420,   420,   420,   420,   421,
     421,   421,   421,   421,   421,   421,   421,   421,   421,   421,
     421,   421,   421,   421,   421,   421,   421,   421,   421,   421,
     421,   421,   421,   421,   421,   421,   422,   422,   422,   422,
     422,   422,   422,   422,   423,   424,   424,   424,   425,   425,
     426,   426,   426,   428,   429,   427,   430,   430,   431,   431,
     432,   432,   432,   432,   433,   433,   434,   434,   434,   434,
     435,   436,   434,   434,   434,   437,   434,   434,   434,   434,
     434,   434,   434,   434,   434,   434,   434,   434,   434,   438,
     439,   434,   434,   440,   441,   434,   442,   443,   434,   444,
     445,   434,   434,   446,   447,   434,   448,   449,   434,   434,
     450,   451,   434,   452,   453,   434,   434,   454,   455,   434,
     456,   457,   434,   458,   459,   434,   460,   461,   434,   462,
     462,   462,   464,   465,   466,   467,   463,   469,   470,   471,
     472,   468,   474,   475,   476,   477,   473,   478,   478,   478,
     478,   478,   479,   479,   479,   479,   479,   479,   479,   479,
     480,   480,   481,   482,   482,   483,   483,   484,   484,   485,
     485,   487,   488,   486,   489,   490,   486,   491,   492,   486,
     493,   494,   486,   495,   495,   496,   497,   498,   496,   499,
     500,   501,   499,   502,   503,   499,   504,   505,   499,   499,
     506,   507,   499,   499,   508,   509,   499,   510,   510,   512,
     511,   511,   511,   511,   513,   513,   514,   514,   515,   515,
     515
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
       2,     0,     1,     0,     3,     7,     1,     1,     0,     2,
       1,     1,     1,     1,     1,     1,     2,     0,     1,     0,
       0,     6,     0,     3,     0,     0,     6,     0,     3,     0,
       8,     7,     1,     4,     3,     3,     3,     5,     5,     0,
       9,     3,     0,     7,     0,     7,     4,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     1,     5,     1,     3,     3,     4,
       1,     1,     1,     1,     1,     4,     1,     2,     3,     3,
       3,     3,     2,     1,     3,     0,     3,     0,     4,     0,
       2,     3,     0,     2,     1,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     3,     3,
       2,     2,     3,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     3,     2,     2,     2,     2,
       2,     3,     3,     3,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     0,     1,     1,     3,     0,     5,     0,     0,     0,
       6,     0,     0,     6,     2,     1,     2,     2,     1,     1,
       1,     1,     2,     1,     2,     2,     2,     2,     1,     1,
       1,     2,     2,     2,     0,     2,     2,     0,     2,     0,
       2,     1,     3,     1,     3,     2,     2,     3,     0,     0,
       5,     1,     2,     5,     5,     5,     6,     2,     1,     1,
       1,     2,     3,     2,     3,     4,     1,     1,     0,     1,
       1,     1,     0,     1,     3,     8,     7,     3,     3,     5,
       0,     0,     7,     0,     0,     7,     0,     0,     7,     0,
       0,     6,     5,     8,    10,     1,     3,     1,     2,     3,
       1,     1,     2,     2,     2,     2,     2,     4,     1,     3,
       0,     4,     7,     7,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     6,     8,     5,     6,     1,     4,     3,     0,     0,
       8,     0,     0,     9,     3,     4,     5,     6,     8,     5,
       6,     0,     0,     5,     3,     4,     4,     5,     4,     3,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     4,     3,     4,     5,     4,     5,     3,     4,     1,
       1,     2,     4,     4,     1,     3,     5,     0,     0,     8,
       3,     3,     3,     0,     0,     8,     3,     4,     0,     0,
       9,     4,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     1,     4,     7,     8,     7,     4,     4,     4,
       4,     4,     1,     6,     7,     6,     6,     7,     7,     6,
       7,     6,     6,     0,     1,     0,     1,     1,     0,     1,
       0,     1,     1,     0,     1,     5,     0,     2,     0,     4,
       0,     9,     0,    11,     3,     3,     4,     1,     1,     3,
       3,     3,     1,     3,     1,     3,     0,     1,     3,     3,
       1,     3,     0,     1,     3,     1,     1,     1,     2,     3,
       3,     5,     1,     1,     1,     1,     1,     0,     1,     1,
       4,     3,     3,     5,     1,     3,     0,     2,     2,     4,
       6,     5,     4,     6,     5,     0,     1,     0,     1,     1,
       0,     2,     0,     4,     6,     0,     6,     1,     3,     1,
       2,     0,     1,     3,     0,     1,     1,     0,     5,     3,
       0,     1,     1,     1,     0,     2,     0,     1,     1,     2,
       0,     1,     0,     0,     0,    13,     0,     2,     0,     1,
       3,     1,     1,     2,     2,     0,     1,     1,     1,     3,
       0,     0,     0,     9,     1,     4,     3,     3,     5,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     4,     4,     1,     3,
       0,     1,     3,     0,     0,     6,     1,     1,     1,     3,
       3,     2,     4,     3,     1,     2,     1,     1,     1,     1,
       0,     0,     6,     4,     5,     0,     9,     4,     2,     2,
       3,     2,     3,     2,     2,     3,     3,     3,     2,     0,
       0,     6,     2,     0,     0,     6,     0,     0,     6,     0,
       0,     6,     1,     0,     0,     6,     0,     0,     7,     1,
       0,     0,     6,     0,     0,     7,     1,     0,     0,     6,
       0,     0,     7,     0,     0,     6,     0,     0,     6,     1,
       3,     3,     0,     0,     0,     0,    12,     0,     0,     0,
       0,    12,     0,     0,     0,     0,    12,     1,     1,     1,
       1,     1,     3,     3,     5,     5,     6,     6,     8,     8,
       0,     1,     2,     3,     5,     1,     2,     1,     0,     0,
       1,     0,     0,    10,     0,     0,    10,     0,     0,    10,
       0,     0,     7,     3,     1,     5,     0,     0,    10,     3,
       0,     0,    11,     0,     0,    11,     0,     0,    10,     5,
       0,     0,     9,     5,     0,     0,    10,     1,     3,     0,
       5,     5,     7,     9,     0,     3,     0,     1,    11,    12,
      11
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

    case YYSYMBOL_expr_call_pipe: /* expr_call_pipe  */
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

    case YYSYMBOL_expr_assign: /* expr_assign  */
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

    case YYSYMBOL_expr_generator: /* expr_generator  */
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

    case YYSYMBOL_type_declaration_no_options_no_dim: /* type_declaration_no_options_no_dim  */
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

    case YYSYMBOL_make_map_tuple: /* make_map_tuple  */
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
        if ( yyextra->g_Program->options.size() ) {
            for ( auto & opt : *(yyvsp[0].aaList) ) {
                if ( yyextra->g_Access->isOptionAllowed(opt.name, yyextra->g_Program->thisModuleName) ) {
                    yyextra->g_Program->options.push_back(opt);
                } else {
                    das2_yyerror(scanner,"option " + opt.name + " is not allowed here",
                        tokAt(scanner,(yylsp[0])), CompilationError::invalid_option);
                }
            }
        } else {
            swap ( yyextra->g_Program->options, *(yyvsp[0].aaList) );
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

  case 74: /* expression_else: "else" optional_emit_semis expression_else_block  */
                                                                     { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 75: /* expression_else: elif_or_static_elif '(' expr ')' optional_emit_semis expression_else_block expression_else  */
                                                                                                                           {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),(yyvsp[-1].pExpression),(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-6].b);
        (yyval.pExpression) = eite;
    }
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

  case 79: /* expression_else_one_liner: "else" expression_if_one_liner  */
                                                      {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 80: /* expression_if_one_liner: expr  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 81: /* expression_if_one_liner: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 82: /* expression_if_one_liner: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 83: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 84: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 89: /* $@3: %empty  */
        {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 90: /* $@4: %empty  */
                                      {
        yyextra->pop_nesteds();
    }
    break;

  case 91: /* expression_if_block: $@3 '{' expressions $@4 '}' expression_block_finally  */
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

  case 92: /* $@5: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 93: /* expression_if_block: $@5 expression_if_one_liner SEMICOLON  */
                                               {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 94: /* $@6: %empty  */
        {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 95: /* $@7: %empty  */
                                      {
        yyextra->pop_nesteds();
    }
    break;

  case 96: /* expression_else_block: $@6 '{' expressions $@7 '}' expression_block_finally  */
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

  case 97: /* $@8: %empty  */
       {
        yyextra->das_keyword = false;
    }
    break;

  case 98: /* expression_else_block: $@8 expression_if_one_liner SEMICOLON  */
                                               {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 99: /* $@9: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 100: /* expression_if_then_else: $@9 if_or_static_if '(' expr ')' optional_emit_semis expression_if_block expression_else  */
                                                                                                                   {
        yyextra->das_keyword = false;
        auto blk = (yyvsp[-1].pExpression)->rtti_isBlock() ? static_cast<ExprBlock *>((yyvsp[-1].pExpression)) : ast_wrapInBlock((yyvsp[-1].pExpression));
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),blk,(yyvsp[0].pExpression));
        eite->isStatic = (yyvsp[-6].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 101: /* expression_if_then_else_oneliner: expression_if_one_liner "if" '(' expr ')' expression_else_one_liner SEMICOLON  */
                                                                                                                      {
        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ast_wrapInBlock((yyvsp[-6].pExpression)),(yyvsp[-1].pExpression) ? ast_wrapInBlock((yyvsp[-1].pExpression)) : nullptr);
    }
    break;

  case 102: /* for_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 103: /* for_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 104: /* for_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 105: /* for_variable_name_with_pos_list: '(' tuple_expansion ')'  */
                                       {
        auto pSL = new vector<VariableNameAndPosition>();
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        pSL->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 106: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 107: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 108: /* for_variable_name_with_pos_list: for_variable_name_with_pos_list ',' '(' tuple_expansion ')'  */
                                                                                 {
        for ( auto & x : *(yyvsp[-1].pNameList) ) {
            das_checkName(scanner,x,tokAt(scanner,(yylsp[-1])));
        }
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition((yyvsp[-1].pNameList),tokAt(scanner,(yylsp[-1]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
    }
    break;

  case 109: /* $@10: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 110: /* expression_for_loop: $@10 "for" '(' for_variable_name_with_pos_list "in" expr_list ')' optional_emit_semis expression_block  */
                                                                                                                                     {
        yyextra->das_keyword = false;
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-5].pNameWithPosList),(yyvsp[-3].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-7])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 111: /* expression_unsafe: "unsafe" optional_emit_semis expression_block  */
                                                                    {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-2])));
        pUnsafe->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 112: /* $@11: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 113: /* expression_while_loop: $@11 "while" '(' expr ')' optional_emit_semis expression_block  */
                                                                                         {
        yyextra->das_keyword = false;
        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-5])));
        pWhile->cond = (yyvsp[-3].pExpression);
        pWhile->body = (yyvsp[0].pExpression);
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 114: /* $@12: %empty  */
        {
        yyextra->das_keyword = true;
    }
    break;

  case 115: /* expression_with: $@12 "with" '(' expr ')' optional_emit_semis expression_block  */
                                                                                   {
        yyextra->das_keyword = false;
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-5])));
        pWith->with = (yyvsp[-3].pExpression);
        pWith->body = (yyvsp[0].pExpression);
        (yyval.pExpression) = pWith;
    }
    break;

  case 116: /* expression_with_alias: "assume" "name" '=' expr  */
                                                      {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), (yyvsp[0].pExpression) );
        delete (yyvsp[-2].s);
    }
    break;

  case 117: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 118: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 119: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 120: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 121: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 122: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 123: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 124: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 125: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 126: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 127: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 128: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 129: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 130: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 131: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 132: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 133: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 134: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 135: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                          {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[-4].s); }
    }
    break;

  case 136: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 137: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 138: /* metadata_argument_list: '@' annotation_argument optional_emit_semis  */
                                                         {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[-1].aa));
    }
    break;

  case 139: /* metadata_argument_list: metadata_argument_list '@' annotation_argument optional_emit_semis  */
                                                                                      {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-3].aaList),(yyvsp[-1].aa));
    }
    break;

  case 140: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 141: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 142: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 143: /* annotation_declaration_name: "template"  */
                                    { (yyval.s) = new string("template"); }
    break;

  case 144: /* annotation_declaration_basic: annotation_declaration_name  */
                                          {
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner,(yylsp[0]));
        if ( yyextra->g_Access->isAnnotationAllowed(*(yyvsp[0].s), yyextra->g_Program->thisModuleName) ) {
            if ( auto ann = findAnnotation(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0]))) ) {
                (yyval.fa)->annotation = ann;
            }
        } else {
            das2_yyerror(scanner,"annotation " + *(yyvsp[0].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[0])), CompilationError::invalid_annotation);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 145: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
                                                                                 {
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner,(yylsp[-3]));
        if ( yyextra->g_Access->isAnnotationAllowed(*(yyvsp[-3].s), yyextra->g_Program->thisModuleName) ) {
            if ( auto ann = findAnnotation(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3]))) ) {
                (yyval.fa)->annotation = ann;
            }
        } else {
            das2_yyerror(scanner,"annotation " + *(yyvsp[-3].s) + " is not allowed here",
                        tokAt(scanner,(yylsp[-3])), CompilationError::invalid_annotation);
        }
        swap ( (yyval.fa)->arguments, *(yyvsp[-1].aaList) );
        delete (yyvsp[-1].aaList);
        delete (yyvsp[-3].s);
    }
    break;

  case 146: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 147: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 148: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation); }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::And,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 149: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
                                                                            {
        if ( !(yyvsp[-2].fa)->annotation || !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation); }
        if ( !(yyvsp[0].fa)->annotation || !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Or,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 150: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
                                                                              {
        if ( !(yyvsp[-2].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[-2].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[-2])),
                CompilationError::invalid_annotation); }
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Xor,(yyvsp[-2].fa),(yyvsp[0].fa));
    }
    break;

  case 151: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 152: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 153: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 154: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 155: /* optional_annotation_list: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 156: /* optional_annotation_list: '[' annotation_list ']'  */
                                       { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 157: /* optional_annotation_list_with_emit_semis: %empty  */
                                       { (yyval.faList) = nullptr; }
    break;

  case 158: /* optional_annotation_list_with_emit_semis: '[' annotation_list ']' optional_emit_semis  */
                                                          { (yyval.faList) = (yyvsp[-2].faList); }
    break;

  case 159: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 160: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 161: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 162: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 163: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 164: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 165: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 166: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 167: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 168: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 169: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 170: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 171: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 172: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 173: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 174: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 175: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 176: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 177: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 178: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 179: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 180: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 181: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 182: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 183: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 184: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 185: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 186: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 187: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 188: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 189: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 190: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 191: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 192: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 193: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 194: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 195: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 196: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 197: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 198: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 199: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 200: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 201: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 202: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 203: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 204: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 205: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 206: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 207: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 208: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 209: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 210: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 211: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 212: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 213: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 214: /* function_name: "operator" '.' "name" "+="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`+="); delete (yyvsp[-1].s); }
    break;

  case 215: /* function_name: "operator" '.' "name" "-="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`-="); delete (yyvsp[-1].s); }
    break;

  case 216: /* function_name: "operator" '.' "name" "*="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`*="); delete (yyvsp[-1].s); }
    break;

  case 217: /* function_name: "operator" '.' "name" "/="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`/="); delete (yyvsp[-1].s); }
    break;

  case 218: /* function_name: "operator" '.' "name" "%="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`%="); delete (yyvsp[-1].s); }
    break;

  case 219: /* function_name: "operator" '.' "name" "&="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&="); delete (yyvsp[-1].s); }
    break;

  case 220: /* function_name: "operator" '.' "name" "|="  */
                                          { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`|="); delete (yyvsp[-1].s); }
    break;

  case 221: /* function_name: "operator" '.' "name" "^="  */
                                           { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^="); delete (yyvsp[-1].s); }
    break;

  case 222: /* function_name: "operator" '.' "name" "&&="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`&&="); delete (yyvsp[-1].s); }
    break;

  case 223: /* function_name: "operator" '.' "name" "||="  */
                                            { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`||="); delete (yyvsp[-1].s); }
    break;

  case 224: /* function_name: "operator" '.' "name" "^^="  */
                                              { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`^^="); delete (yyvsp[-1].s); }
    break;

  case 225: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 226: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 227: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 228: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 229: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 230: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 231: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 232: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 233: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 234: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 235: /* function_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 236: /* function_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 237: /* function_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 238: /* function_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 239: /* function_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 240: /* function_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 241: /* function_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 242: /* function_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 243: /* function_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 244: /* function_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 245: /* function_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 246: /* function_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 247: /* function_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 248: /* function_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 249: /* function_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 250: /* function_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 251: /* function_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 252: /* function_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 253: /* function_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 254: /* function_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 255: /* function_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 256: /* function_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 257: /* function_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 258: /* function_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 259: /* function_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 260: /* global_function_declaration: optional_annotation_list_with_emit_semis "def" function_declaration  */
                                                                                                {
        (yyvsp[0].pFuncDecl)->atDecl = tokRangeAt(scanner,(yylsp[-1]),(yylsp[0]));
        assignDefaultArguments((yyvsp[0].pFuncDecl));
        runFunctionAnnotations(scanner, yyextra, (yyvsp[0].pFuncDecl), (yyvsp[-2].faList), tokAt(scanner,(yylsp[-2])));
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

  case 261: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 262: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 263: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 264: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 265: /* $@13: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 266: /* function_declaration: optional_public_or_private_function $@13 function_declaration_header optional_emit_semis expression_block  */
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

  case 267: /* expression_block_finally: %empty  */
        {
        (yyval.pExpression) = nullptr;
    }
    break;

  case 268: /* $@14: %empty  */
                  {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 269: /* $@15: %empty  */
                             {
        yyextra->pop_nesteds();
    }
    break;

  case 270: /* expression_block_finally: "finally" $@14 '{' expressions $@15 '}'  */
          {
        (yyval.pExpression) = (yyvsp[-2].pExpression);
    }
    break;

  case 271: /* $@16: %empty  */
        {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 272: /* $@17: %empty  */
                                      {
        yyextra->pop_nesteds();
    }
    break;

  case 273: /* expression_block: $@16 '{' expressions $@17 '}' expression_block_finally  */
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

  case 274: /* expr_call_pipe: expr_call expr_full_block_assumed_piped  */
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

  case 275: /* expression_any: SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 276: /* expression_any: expr_assign SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 277: /* expression_any: expression_delete SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 278: /* expression_any: expression_let  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 279: /* expression_any: expression_while_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 280: /* expression_any: expression_unsafe  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 281: /* expression_any: expression_with  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 282: /* expression_any: expression_with_alias SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 283: /* expression_any: expression_for_loop  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 284: /* expression_any: expression_break SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 285: /* expression_any: expression_continue SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 286: /* expression_any: expression_return SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 287: /* expression_any: expression_yield SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 288: /* expression_any: expression_if_then_else  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 289: /* expression_any: expression_if_then_else_oneliner  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 290: /* expression_any: expression_try_catch  */
                                                  { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 291: /* expression_any: expression_label SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 292: /* expression_any: expression_goto SEMICOLON  */
                                                  { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 293: /* expression_any: "pass" SEMICOLON  */
                                                  { (yyval.pExpression) = nullptr; }
    break;

  case 294: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 295: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back((yyvsp[0].pExpression));
        }
    }
    break;

  case 296: /* expressions: expressions error  */
                                 {
        delete (yyvsp[-1].pExpression); (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 297: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 298: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 299: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 300: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 301: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 302: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 303: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 304: /* name_in_namespace: "name" "::" "name"  */
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

  case 305: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 306: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
    }
    break;

  case 307: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 308: /* $@18: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 309: /* $@19: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 310: /* new_type_declaration: '<' $@18 type_declaration '>' $@19  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 311: /* new_type_declaration: structure_type_declaration  */
                                               {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 312: /* expr_new: "new" new_type_declaration  */
                                                       {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pTypeDecl),false);
    }
    break;

  case 313: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 314: /* expr_new: "new" new_type_declaration '(' expr_list ')'  */
                                                                                    {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),(yyvsp[-3].pTypeDecl),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 315: /* expr_new: "new" new_type_declaration '(' make_struct_single ')'  */
                                                                                      {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-3].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-4])),(yyvsp[-1].pExpression));
    }
    break;

  case 316: /* expr_new: "new" new_type_declaration '(' "uninitialized" make_struct_single ')'  */
                                                                                                        {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = false; // $init;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),(yyvsp[-1].pExpression));
    }
    break;

  case 317: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 318: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 319: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 320: /* expression_return: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 321: /* expression_return: "return" expr  */
                                      {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 322: /* expression_return: "return" "<-" expr  */
                                             {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 323: /* expression_yield: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 324: /* expression_yield: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 325: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 326: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 327: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 328: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 329: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 330: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 331: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 332: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 333: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 334: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 335: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 336: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr SEMICOLON  */
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

  case 337: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 338: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 339: /* expression_let: kwd_let optional_in_scope '{' variable_declaration_list '}'  */
                                                                               {
        (yyval.pExpression) = ast_LetList(scanner,(yyvsp[-4].b),(yyvsp[-3].b),*(yyvsp[-1].pVarDeclList),tokAt(scanner,(yylsp[-4])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 340: /* $@20: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 341: /* $@21: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 342: /* expr_cast: "cast" '<' $@20 type_declaration_no_options '>' $@21 expr  */
                                                                                                                                                {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
    }
    break;

  case 343: /* $@22: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 344: /* $@23: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 345: /* expr_cast: "upcast" '<' $@22 type_declaration_no_options '>' $@23 expr  */
                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 346: /* $@24: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 347: /* $@25: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 348: /* expr_cast: "reinterpret" '<' $@24 type_declaration_no_options '>' $@25 expr  */
                                                                                                                                                       {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),(yyvsp[0].pExpression),(yyvsp[-3].pTypeDecl));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 349: /* $@26: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 350: /* $@27: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 351: /* expr_type_decl: "type" '<' $@26 type_declaration '>' $@27  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 352: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
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

  case 353: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
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

  case 354: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" c_or_s "name" '>' '(' expr ')'  */
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

  case 355: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 356: /* expr_list: expr_list ',' expr  */
                                            {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 357: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 358: /* block_or_simple_block: "=>" expr  */
                                        {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-1])), (yyvsp[0].pExpression));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 359: /* block_or_simple_block: "=>" "<-" expr  */
                                               {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-2])), (yyvsp[0].pExpression));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 360: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 361: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 362: /* block_or_lambda: '@' '@'  */
                { (yyval.i) = 2;   /* local function */ }
    break;

  case 363: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 364: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 365: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 366: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 367: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 368: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 369: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 370: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 371: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 372: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis block_or_simple_block  */
                                                                                                                {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])));
    }
    break;

  case 373: /* expr_full_block_assumed_piped: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type optional_emit_semis expression_block  */
                                                                                                           {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-6].i),(yyvsp[-5].faList),(yyvsp[-4].pCaptList),(yyvsp[-3].pVarDeclList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-5])));
    }
    break;

  case 374: /* expr_full_block_assumed_piped: '{' expressions '}'  */
                                   {
        (yyval.pExpression) = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,new TypeDecl(Type::autoinfer),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-1])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 375: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 376: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 377: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 378: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 379: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 380: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 381: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 382: /* expr_assign: expr  */
                                             { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 383: /* expr_assign: expr '=' expr  */
                                             { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 384: /* expr_assign: expr "<-" expr  */
                                             { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 385: /* expr_assign: expr ":=" expr  */
                                             { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 386: /* expr_assign: expr "&=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 387: /* expr_assign: expr "|=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 388: /* expr_assign: expr "^=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 389: /* expr_assign: expr "&&=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 390: /* expr_assign: expr "||=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 391: /* expr_assign: expr "^^=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 392: /* expr_assign: expr "+=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 393: /* expr_assign: expr "-=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 394: /* expr_assign: expr "*=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 395: /* expr_assign: expr "/=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 396: /* expr_assign: expr "%=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 397: /* expr_assign: expr "<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 398: /* expr_assign: expr ">>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 399: /* expr_assign: expr "<<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 400: /* expr_assign: expr ">>>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 401: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 402: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 403: /* expr_method_call: expr "->" "name" '(' ')'  */
                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 404: /* expr_method_call: expr "->" "name" '(' expr_list ')'  */
                                                                              {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 405: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 406: /* func_addr_name: "$i" '(' expr ')'  */
                                          {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 407: /* func_addr_expr: '@' '@' func_addr_name  */
                                          {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 408: /* $@28: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 409: /* $@29: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 410: /* func_addr_expr: '@' '@' '<' $@28 type_declaration_no_options '>' $@29 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value.get()));
        expr->funcType = (yyvsp[-3].pTypeDecl);
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 411: /* $@30: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 412: /* $@31: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 413: /* func_addr_expr: '@' '@' '<' $@30 optional_function_argument_list optional_function_type '>' $@31 func_addr_name  */
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

  case 414: /* expr_field: expr '.' "name"  */
                                              {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 415: /* expr_field: expr '.' '.' "name"  */
                                                  {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 416: /* expr_field: expr '.' "name" '(' ')'  */
                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 417: /* expr_field: expr '.' "name" '(' expr_list ')'  */
                                                                           {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 418: /* expr_field: expr '.' "name" '(' '[' make_struct_fields ']' ')'  */
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

  case 421: /* $@32: %empty  */
                               { yyextra->das_suppress_errors=true; }
    break;

  case 422: /* $@33: %empty  */
                                                                            { yyextra->das_suppress_errors=false; }
    break;

  case 423: /* expr_field: expr '.' $@32 error $@33  */
                                                                                                                    {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), "");
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
            dd->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            dd->useInitializer = false;
            dd->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = dd;
    }
    break;

  case 426: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
                                                               {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-3]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
            delete (yyvsp[-3].s);
            (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 427: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
                                                                                 {
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
            ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[-4])),*(yyvsp[-4].s));
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
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",(yyvsp[0].pExpression)); }
    break;

  case 442: /* expr: '~' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",(yyvsp[0].pExpression)); }
    break;

  case 443: /* expr: '+' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",(yyvsp[0].pExpression)); }
    break;

  case 444: /* expr: '-' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",(yyvsp[0].pExpression)); }
    break;

  case 445: /* expr: expr "<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 446: /* expr: expr ">>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 447: /* expr: expr "<<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 448: /* expr: expr ">>>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 449: /* expr: expr '+' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 450: /* expr: expr '-' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 451: /* expr: expr '*' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 452: /* expr: expr '/' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 453: /* expr: expr '%' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 454: /* expr: expr '<' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 455: /* expr: expr '>' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 456: /* expr: expr "==" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 457: /* expr: expr "!=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 458: /* expr: expr "<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 459: /* expr: expr ">=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 460: /* expr: expr '&' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 461: /* expr: expr '|' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 462: /* expr: expr '^' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 463: /* expr: expr "&&" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 464: /* expr: expr "||" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 465: /* expr: expr "^^" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", (yyvsp[-2].pExpression), (yyvsp[0].pExpression)); }
    break;

  case 466: /* expr: expr ".." expr  */
                                             {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back((yyvsp[-2].pExpression));
        itv->arguments.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = itv;
    }
    break;

  case 467: /* expr: "++" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", (yyvsp[0].pExpression)); }
    break;

  case 468: /* expr: "--" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", (yyvsp[0].pExpression)); }
    break;

  case 469: /* expr: expr "++"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", (yyvsp[-1].pExpression)); }
    break;

  case 470: /* expr: expr "--"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", (yyvsp[-1].pExpression)); }
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

  case 472: /* expr: '(' make_struct_single ')'  */
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

  case 473: /* expr: expr '[' expr ']'  */
                                                 { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 474: /* expr: expr '.' '[' expr ']'  */
                                                     { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 475: /* expr: expr "?[" expr ']'  */
                                                 { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-3].pExpression), (yyvsp[-1].pExpression)); }
    break;

  case 476: /* expr: expr '.' "?[" expr ']'  */
                                                     { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), (yyvsp[-4].pExpression), (yyvsp[-1].pExpression), true); }
    break;

  case 477: /* expr: expr "?." "name"  */
                                                 { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-2].pExpression), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 478: /* expr: expr '.' "?." "name"  */
                                                     { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), (yyvsp[-3].pExpression), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 479: /* expr: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 480: /* expr: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 481: /* expr: '*' expr  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression)); }
    break;

  case 482: /* expr: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 483: /* expr: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression)); }
    break;

  case 484: /* expr: expr_generator  */
                                                   { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 485: /* expr: expr "??" expr  */
                                                   { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression)); }
    break;

  case 486: /* expr: expr '?' expr ':' expr  */
                                                          {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
        }
    break;

  case 487: /* $@34: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 488: /* $@35: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 489: /* expr: expr "is" "type" '<' $@34 type_declaration_no_options '>' $@35  */
                                                                                                                                                       {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),(yyvsp[-2].pTypeDecl));
    }
    break;

  case 490: /* expr: expr "is" basic_type_declaration  */
                                                               {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),vdecl);
    }
    break;

  case 491: /* expr: expr "is" "name"  */
                                              {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 492: /* expr: expr "as" "name"  */
                                              {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 493: /* $@36: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 494: /* $@37: %empty  */
                                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 495: /* expr: expr "as" "type" '<' $@36 type_declaration '>' $@37  */
                                                                                                                                            {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-7].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 496: /* expr: expr "as" basic_type_declaration  */
                                                               {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-2].pExpression),das_to_string((yyvsp[0].type)));
    }
    break;

  case 497: /* expr: expr '?' "as" "name"  */
                                                  {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 498: /* $@38: %empty  */
                                                   { yyextra->das_arrow_depth ++; }
    break;

  case 499: /* $@39: %empty  */
                                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 500: /* expr: expr '?' "as" "type" '<' $@38 type_declaration '>' $@39  */
                                                                                                                                                {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),(yyvsp[-8].pExpression),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 501: /* expr: expr '?' "as" basic_type_declaration  */
                                                                   {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),(yyvsp[-3].pExpression),das_to_string((yyvsp[0].type)));
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

  case 512: /* expr: expr_call_pipe  */
                             { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 513: /* expr: "unsafe" '(' expr ')'  */
                                         {
            (yyvsp[-1].pExpression)->alwaysSafe = true;
            (yyvsp[-1].pExpression)->userSaidItsSafe = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    break;

  case 514: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])));
    }
    break;

  case 515: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])));
    }
    break;

  case 516: /* expr_generator: "generator" '<' type_declaration_no_options '>' optional_capture_list optional_emit_semis expression_block  */
                                                                                                                                                  {
        auto closure = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),(yyvsp[0].pExpression));
        ((ExprBlock *)(yyvsp[0].pExpression))->returnType = make_smart<TypeDecl>(Type::autoinfer);
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),closure,tokAt(scanner,(yylsp[-6])));
    }
    break;

  case 517: /* expr_mtag: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 518: /* expr_mtag: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 519: /* expr_mtag: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 520: /* expr_mtag: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 521: /* expr_mtag: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 522: /* expr_mtag: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 523: /* expr_mtag: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 524: /* expr_mtag: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 525: /* expr_mtag: expr '.' "$f" '(' expr ')'  */
                                                                {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 526: /* expr_mtag: expr "?." "$f" '(' expr ')'  */
                                                                 {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-5].pExpression), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 527: /* expr_mtag: expr '.' '.' "$f" '(' expr ')'  */
                                                                    {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 528: /* expr_mtag: expr '.' "?." "$f" '(' expr ')'  */
                                                                     {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), (yyvsp[-6].pExpression), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 529: /* expr_mtag: expr "as" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 530: /* expr_mtag: expr '?' "as" "$f" '(' expr ')'  */
                                                                       {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-6].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 531: /* expr_mtag: expr "is" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),(yyvsp[-5].pExpression),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 532: /* expr_mtag: '@' '@' "$c" '(' expr ')'  */
                                                         {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 533: /* optional_field_annotation: %empty  */
                                      { (yyval.aaList) = nullptr; }
    break;

  case 534: /* optional_field_annotation: metadata_argument_list  */
                                      { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 535: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 536: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 537: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 538: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 539: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 540: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 541: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 542: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 543: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 544: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 545: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 546: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 547: /* struct_variable_declaration_list: struct_variable_declaration_list "new line, semicolon"  */
                                                                 { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 548: /* $@40: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 549: /* struct_variable_declaration_list: struct_variable_declaration_list $@40 structure_variable_declaration SEMICOLON  */
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

  case 550: /* $@41: %empty  */
                                                                                                                     {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 551: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable "abstract" optional_constant $@41 function_declaration_header SEMICOLON  */
                                                          {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyvsp[-1].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 552: /* $@42: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 553: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list_with_emit_semis "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@42 function_declaration_header optional_emit_semis expression_block  */
                                                                                            {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-2].pFuncDecl),tak);
                }
                (yyvsp[-2].pFuncDecl)->isTemplate = yyextra->g_thisStructure ? yyextra->g_thisStructure->isTemplate : false;
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-10].pVarDeclList),(yyvsp[-9].faList),(yyvsp[-6].b),(yyvsp[-7].b),(yyvsp[-5].i),(yyvsp[-4].b),(yyvsp[-2].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-8]),(yylsp[0])),tokAt(scanner,(yylsp[-9])));
            }
    break;

  case 554: /* function_argument_declaration_no_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_no_type  */
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

  case 555: /* function_argument_declaration_type: optional_field_annotation kwd_let_var_or_nothing variable_declaration_type  */
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

  case 556: /* function_argument_declaration_type: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))));
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 557: /* function_argument_list: function_argument_declaration_no_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 558: /* function_argument_list: function_argument_declaration_type  */
                                                                                      { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 559: /* function_argument_list: function_argument_declaration_no_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 560: /* function_argument_list: function_argument_declaration_type ';' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 561: /* function_argument_list: function_argument_declaration_type ',' function_argument_list  */
                                                                                      { (yyval.pVarDeclList) = (yyvsp[0].pVarDeclList); (yyvsp[0].pVarDeclList)->insert((yyvsp[0].pVarDeclList)->begin(),(yyvsp[-2].pVarDecl)); }
    break;

  case 562: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 563: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 564: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 565: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                       { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 566: /* tuple_alias_type_list: %empty  */
      {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 567: /* tuple_alias_type_list: tuple_type  */
                       {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 568: /* tuple_alias_type_list: tuple_alias_type_list semis tuple_type  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tokName = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *((yyvsp[0].pVarDecl)->pNameList) ) {
                    crd->afterTupleEntry(nl.name.c_str(), nl.at);
                }
            }
        }
    }
    break;

  case 569: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition(*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))));
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 570: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 571: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 572: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 573: /* variant_alias_type_list: variant_type  */
                         {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
        (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 574: /* variant_alias_type_list: variant_alias_type_list semis variant_type  */
                                                               {
        (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tokName = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) {
                for ( const auto & nl : *((yyvsp[0].pVarDecl)->pNameList) ) {
                    crd->afterVariantEntry(nl.name.c_str(), nl.at);
                }
            }
        }
    }
    break;

  case 575: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 576: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 577: /* variable_declaration_no_type: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 578: /* variable_declaration_no_type: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 579: /* variable_declaration_no_type: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 580: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 581: /* variable_declaration_type: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 582: /* variable_declaration: variable_declaration_type  */
                                        {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 583: /* variable_declaration: variable_declaration_no_type  */
                                           {
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 584: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 585: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 586: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 587: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 588: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 589: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 590: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 591: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 592: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 593: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 594: /* global_let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 595: /* global_let_variable_name_with_pos_list: global_let_variable_name_with_pos_list ',' "name"  */
                                                                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 596: /* variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 597: /* variable_declaration_list: variable_declaration_list SEMICOLON  */
                                                  {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 598: /* variable_declaration_list: variable_declaration_list let_variable_declaration  */
                                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
        (yyvsp[-1].pVarDeclList)->push_back((yyvsp[0].pVarDecl));
    }
    break;

  case 599: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 600: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                        {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 601: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 602: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options SEMICOLON  */
                                                                                                         {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 603: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                                               {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 604: /* global_let_variable_declaration: global_let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr SEMICOLON  */
                                                                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 605: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 606: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 607: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 608: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 609: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 610: /* global_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 611: /* global_variable_declaration_list: global_variable_declaration_list SEMICOLON  */
                                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 612: /* $@43: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 613: /* global_variable_declaration_list: global_variable_declaration_list $@43 optional_field_annotation let_variable_declaration  */
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

  case 614: /* global_let: kwd_let optional_shared optional_public_or_private_variable '{' global_variable_declaration_list '}'  */
                                                                                                                                       {
        ast_globalLetList(scanner,(yyvsp[-5].b),(yyvsp[-4].b),(yyvsp[-3].b),(yyvsp[-1].pVarDeclList));
    }
    break;

  case 615: /* $@44: %empty  */
                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 616: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@44 optional_field_annotation global_let_variable_declaration  */
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

  case 617: /* enum_expression: "name"  */
                   {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        delete (yyvsp[0].s);
    }
    break;

  case 618: /* enum_expression: "name" '=' expr  */
                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[-2].s),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-2])));
        delete (yyvsp[-2].s);
    }
    break;

  case 621: /* enum_list: %empty  */
        {
        (yyval.pEnum) = new Enumeration();
    }
    break;

  case 622: /* enum_list: enum_expression  */
                            {
        (yyval.pEnum) = new Enumeration();
        if ( !(yyval.pEnum)->add((yyvsp[0].pEnumPair)->name,(yyvsp[0].pEnumPair)->expr,(yyvsp[0].pEnumPair)->at) ) {
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

  case 623: /* enum_list: enum_list commas enum_expression  */
                                                 {
        if ( !(yyvsp[-2].pEnum)->add((yyvsp[0].pEnumPair)->name,(yyvsp[0].pEnumPair)->expr,(yyvsp[0].pEnumPair)->at) ) {
            das2_yyerror(scanner,"enumeration already declared " + (yyvsp[0].pEnumPair)->name, (yyvsp[0].pEnumPair)->at,
                CompilationError::enumeration_value_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            for ( auto & crd : yyextra->g_CommentReaders ) {
                crd->afterEnumerationEntry((yyvsp[0].pEnumPair)->name.c_str(), (yyvsp[0].pEnumPair)->at);
            }
        }
        delete (yyvsp[0].pEnumPair);
        (yyval.pEnum) = (yyvsp[-2].pEnum);
    }
    break;

  case 624: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 625: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 626: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 627: /* $@45: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 628: /* single_alias: optional_public_or_private_alias "name" $@45 '=' type_declaration  */
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

  case 630: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 631: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 632: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 633: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 634: /* optional_enum_basic_type_declaration: %empty  */
        {
        (yyval.type) = Type::tInt;
    }
    break;

  case 635: /* optional_enum_basic_type_declaration: ':' enum_basic_type_declaration  */
                                              {
        (yyval.type) = (yyvsp[0].type);
    }
    break;

  case 642: /* $@46: %empty  */
                                                                     {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 643: /* $@47: %empty  */
                                                                                                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 644: /* $@48: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 645: /* enum_declaration: optional_annotation_list_with_emit_semis "enum" $@46 optional_public_or_private_enum enum_name optional_enum_basic_type_declaration optional_emit_commas '{' $@47 enum_list optional_commas $@48 '}'  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-8].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-12].faList),tokAt(scanner,(yylsp[-12])),(yyvsp[-9].b),(yyvsp[-8].pEnum),(yyvsp[-3].pEnum),(yyvsp[-7].type));
    }
    break;

  case 646: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 647: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 648: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 649: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 650: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 651: /* class_or_struct: "class"  */
                    { (yyval.i) = CorS_Class; }
    break;

  case 652: /* class_or_struct: "struct"  */
                    { (yyval.i) = CorS_Struct; }
    break;

  case 653: /* class_or_struct: "template" "class"  */
                                 { (yyval.i) = CorS_ClassTemplate; }
    break;

  case 654: /* class_or_struct: "template" "struct"  */
                                 { (yyval.i) = CorS_StructTemplate; }
    break;

  case 655: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 656: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 657: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 658: /* optional_struct_variable_declaration_list: ';'  */
            {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 659: /* optional_struct_variable_declaration_list: '{' struct_variable_declaration_list '}'  */
                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 660: /* $@49: %empty  */
                                                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 661: /* $@50: %empty  */
                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 662: /* $@51: %empty  */
                                             {
        if ( (yyvsp[-1].pStructure) ) {
            (yyvsp[-1].pStructure)->isClass = (yyvsp[-4].i)==CorS_Class || (yyvsp[-4].i)==CorS_ClassTemplate;
            (yyvsp[-1].pStructure)->isTemplate = (yyvsp[-4].i)==CorS_ClassTemplate || (yyvsp[-4].i)==CorS_StructTemplate;
            (yyvsp[-1].pStructure)->privateStructure = !(yyvsp[-3].b);
        }
    }
    break;

  case 663: /* structure_declaration: optional_annotation_list_with_emit_semis $@49 class_or_struct optional_public_or_private_structure $@50 structure_name optional_emit_semis $@51 optional_struct_variable_declaration_list  */
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

  case 664: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 665: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition("``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)));
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 666: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 667: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))));
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 668: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))));
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 669: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 670: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 671: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 672: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 673: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 674: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 675: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 676: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 677: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 678: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 679: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 680: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 681: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 682: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 683: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 684: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 685: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 686: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 687: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 688: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 689: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 690: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 691: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 692: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 693: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 694: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 695: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 696: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 697: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 698: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 699: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 700: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 701: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 702: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 703: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 704: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 705: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 706: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 707: /* auto_type_declaration: "$t" '(' expr ')'  */
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

  case 708: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 709: /* bitfield_bits: bitfield_bits ';' "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 710: /* bitfield_alias_bits: %empty  */
        {
        auto pSL = new vector<string>();
        (yyval.pNameList) = pSL;

    }
    break;

  case 711: /* bitfield_alias_bits: "name"  */
                   {
        (yyval.pNameList) = new vector<string>();
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[0].s)->c_str(),atvname);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 712: /* bitfield_alias_bits: bitfield_alias_bits commas "name"  */
                                                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntry((yyvsp[0].s)->c_str(),atvname);
        }
        delete (yyvsp[0].s);
    }
    break;

  case 713: /* $@52: %empty  */
                                     { yyextra->das_arrow_depth ++; }
    break;

  case 714: /* $@53: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 715: /* bitfield_type_declaration: "bitfield" '<' $@52 bitfield_bits '>' $@53  */
                                                                                                                             {
            (yyval.pTypeDecl) = new TypeDecl(Type::tBitfield);
            (yyval.pTypeDecl)->argNames = *(yyvsp[-2].pNameList);
            if ( (yyval.pTypeDecl)->argNames.size()>32 ) {
                das2_yyerror(scanner,"only 32 different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-5])),
                    CompilationError::invalid_type);
            }
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
            delete (yyvsp[-2].pNameList);
    }
    break;

  case 718: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 719: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 720: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 721: /* dim_list: '[' ']'  */
                {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), nullptr);
    }
    break;

  case 722: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 723: /* dim_list: dim_list '[' ']'  */
                              {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), nullptr);
    }
    break;

  case 724: /* type_declaration_no_options: type_declaration_no_options_no_dim  */
                                                     {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 725: /* type_declaration_no_options: type_declaration_no_options_no_dim dim_list  */
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

  case 726: /* type_declaration_no_options_no_dim: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 727: /* type_declaration_no_options_no_dim: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 728: /* type_declaration_no_options_no_dim: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 729: /* type_declaration_no_options_no_dim: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 730: /* $@54: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 731: /* $@55: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 732: /* type_declaration_no_options_no_dim: "type" '<' $@54 type_declaration '>' $@55  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 733: /* type_declaration_no_options_no_dim: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 734: /* type_declaration_no_options_no_dim: '$' name_in_namespace '(' optional_expr_list ')'  */
                                                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-3])), *(yyvsp[-3].s)));
        delete (yyvsp[-3].s);
    }
    break;

  case 735: /* $@56: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 736: /* type_declaration_no_options_no_dim: '$' name_in_namespace '<' $@56 type_declaration_no_options_list '>' '(' optional_expr_list ')'  */
                                                                                                                                                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-4].pTypeDeclList),(yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-7])), *(yyvsp[-7].s)));
        delete (yyvsp[-7].s);
    }
    break;

  case 737: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '[' ']'  */
                                                          {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 738: /* type_declaration_no_options_no_dim: type_declaration_no_options "explicit"  */
                                                           {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 739: /* type_declaration_no_options_no_dim: type_declaration_no_options "const"  */
                                                        {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 740: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' "const"  */
                                                            {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 741: /* type_declaration_no_options_no_dim: type_declaration_no_options '&'  */
                                                  {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 742: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '&'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 743: /* type_declaration_no_options_no_dim: type_declaration_no_options '#'  */
                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 744: /* type_declaration_no_options_no_dim: type_declaration_no_options "implicit"  */
                                                           {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 745: /* type_declaration_no_options_no_dim: type_declaration_no_options '-' '#'  */
                                                      {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 746: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" "const"  */
                                                               {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 747: /* type_declaration_no_options_no_dim: type_declaration_no_options "==" '&'  */
                                                         {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 748: /* type_declaration_no_options_no_dim: type_declaration_no_options '?'  */
                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 749: /* $@57: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 750: /* $@58: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 751: /* type_declaration_no_options_no_dim: "smart_ptr" '<' $@57 type_declaration '>' $@58  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 752: /* type_declaration_no_options_no_dim: type_declaration_no_options "??"  */
                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 753: /* $@59: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 754: /* $@60: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 755: /* type_declaration_no_options_no_dim: "array" '<' $@59 type_declaration '>' $@60  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 756: /* $@61: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 757: /* $@62: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 758: /* type_declaration_no_options_no_dim: "table" '<' $@61 table_type_pair '>' $@62  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].aTypePair).firstType;
        (yyval.pTypeDecl)->secondType = (yyvsp[-2].aTypePair).secondType;
    }
    break;

  case 759: /* $@63: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 760: /* $@64: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 761: /* type_declaration_no_options_no_dim: "iterator" '<' $@63 type_declaration '>' $@64  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 762: /* type_declaration_no_options_no_dim: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 763: /* $@65: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 764: /* $@66: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 765: /* type_declaration_no_options_no_dim: "block" '<' $@65 type_declaration '>' $@66  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 766: /* $@67: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 767: /* $@68: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 768: /* type_declaration_no_options_no_dim: "block" '<' $@67 optional_function_argument_list optional_function_type '>' $@68  */
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

  case 769: /* type_declaration_no_options_no_dim: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 770: /* $@69: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 771: /* $@70: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 772: /* type_declaration_no_options_no_dim: "function" '<' $@69 type_declaration '>' $@70  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 773: /* $@71: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 774: /* $@72: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 775: /* type_declaration_no_options_no_dim: "function" '<' $@71 optional_function_argument_list optional_function_type '>' $@72  */
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

  case 776: /* type_declaration_no_options_no_dim: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tVoid);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 777: /* $@73: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 778: /* $@74: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 779: /* type_declaration_no_options_no_dim: "lambda" '<' $@73 type_declaration '>' $@74  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 780: /* $@75: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 781: /* $@76: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 782: /* type_declaration_no_options_no_dim: "lambda" '<' $@75 optional_function_argument_list optional_function_type '>' $@76  */
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

  case 783: /* $@77: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 784: /* $@78: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 785: /* type_declaration_no_options_no_dim: "tuple" '<' $@77 tuple_type_list '>' $@78  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 786: /* $@79: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 787: /* $@80: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 788: /* type_declaration_no_options_no_dim: "variant" '<' $@79 variant_type_list '>' $@80  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 789: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 790: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 791: /* type_declaration: type_declaration '|' '#'  */
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

  case 792: /* $@81: %empty  */
                   {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 793: /* $@82: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 794: /* $@83: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 795: /* $@84: %empty  */
                                                 {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 796: /* tuple_alias_declaration: "tuple" $@81 optional_public_or_private_alias "name" optional_emit_semis $@82 '{' $@83 tuple_alias_type_list optional_semis $@84 '}'  */
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

  case 797: /* $@85: %empty  */
                     {
        yyextra->push_nesteds(DAS_EMIT_SEMICOLON);
    }
    break;

  case 798: /* $@86: %empty  */
                                                                             {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 799: /* $@87: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 800: /* $@88: %empty  */
                                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 801: /* variant_alias_declaration: "variant" $@85 optional_public_or_private_alias "name" optional_emit_semis $@86 '{' $@87 variant_alias_type_list optional_semis $@88 '}'  */
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

  case 802: /* $@89: %empty  */
                      {
        yyextra->push_nesteds(DAS_EMIT_COMMA);
    }
    break;

  case 803: /* $@90: %empty  */
                                                                              {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 804: /* $@91: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 805: /* $@92: %empty  */
                                                {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
        yyextra->pop_nesteds();
    }
    break;

  case 806: /* bitfield_alias_declaration: "bitfield" $@89 optional_public_or_private_alias "name" optional_emit_commas $@90 '{' $@91 bitfield_alias_bits optional_commas $@92 '}'  */
          {
        auto btype = make_smart<TypeDecl>(Type::tBitfield);
        btype->alias = *(yyvsp[-8].s);
        btype->at = tokAt(scanner,(yylsp[-8]));
        btype->argNames = *(yyvsp[-3].pNameList);
        btype->isPrivateAlias = !(yyvsp[-9].b);
        if ( btype->argNames.size()>32 ) {
            das2_yyerror(scanner,"only 32 different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-8])),
                CompilationError::invalid_type);
        }
        if ( !yyextra->g_Program->addAlias(btype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-8].s),tokAt(scanner,(yylsp[-8])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-8]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfield((yyvsp[-8].s)->c_str(),atvname);
        }
        delete (yyvsp[-8].s);
        delete (yyvsp[-3].pNameList);
    }
    break;

  case 807: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 808: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 809: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 810: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 811: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 812: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 813: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 814: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 815: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),(yyvsp[0].pExpression),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 816: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 817: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 818: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),(yyvsp[-1].b),false);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 819: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",(yyvsp[0].pExpression),false,true);
        mfd->tag = (yyvsp[-3].pExpression);
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 820: /* make_variant_dim: %empty  */
       {
        (yyval.pExpression) = ast_makeStructToMakeVariant(nullptr, LineInfo());
    }
    break;

  case 821: /* make_variant_dim: make_struct_fields  */
                              {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 822: /* make_struct_single: make_struct_fields optional_comma  */
                                               {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 823: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 824: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 825: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 826: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 827: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 828: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 829: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 830: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 831: /* $@93: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 832: /* $@94: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 833: /* make_struct_decl: "struct" '<' $@93 type_declaration_no_options '>' $@94 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                      {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 834: /* $@95: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 835: /* $@96: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 836: /* make_struct_decl: "class" '<' $@95 type_declaration_no_options '>' $@96 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                     {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-6].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 837: /* $@97: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 838: /* $@98: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 839: /* make_struct_decl: "variant" '<' $@97 variant_type_list '>' $@98 '(' use_initializer make_variant_dim ')'  */
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

  case 840: /* $@99: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 841: /* $@100: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 842: /* make_struct_decl: "default" '<' $@99 type_declaration_no_options '>' $@100 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = (yyvsp[-3].pTypeDecl);
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 843: /* make_map_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back((yyvsp[-2].pExpression));
        mt->values.push_back((yyvsp[0].pExpression));
        (yyval.pExpression) = mt;
    }
    break;

  case 844: /* make_map_tuple: expr  */
                 {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 845: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = make_smart<TypeDecl>(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 846: /* $@101: %empty  */
                             { yyextra->das_force_oxford_comma=true; yyextra->das_arrow_depth ++; }
    break;

  case 847: /* $@102: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 848: /* make_tuple_call: "tuple" '<' $@101 tuple_type_list '>' $@102 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 849: /* make_dim_decl: '[' optional_expr_list ']'  */
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

  case 850: /* $@103: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 851: /* $@104: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 852: /* make_dim_decl: "array" "struct" '<' $@103 type_declaration_no_options '>' $@104 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 853: /* $@105: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 854: /* $@106: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 855: /* make_dim_decl: "array" "tuple" '<' $@105 tuple_type_list '>' $@106 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 856: /* $@107: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 857: /* $@108: %empty  */
                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 858: /* make_dim_decl: "array" "variant" '<' $@107 variant_type_list '>' $@108 '(' make_variant_dim ')'  */
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

  case 859: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
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

  case 860: /* $@109: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 861: /* $@110: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 862: /* make_dim_decl: "array" '<' $@109 type_declaration_no_options '>' $@110 '(' optional_expr_list ')'  */
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

  case 863: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 864: /* $@111: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 865: /* $@112: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 866: /* make_dim_decl: "fixed_array" '<' $@111 type_declaration_no_options '>' $@112 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = (yyvsp[-6].pTypeDecl);
        mka->gen2 = true;
        (yyval.pExpression) = mka;
    }
    break;

  case 867: /* expr_map_tuple_list: make_map_tuple  */
                                {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 868: /* expr_map_tuple_list: expr_map_tuple_list ',' make_map_tuple  */
                                                                {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),(yyvsp[-2].pExpression),(yyvsp[0].pExpression));
    }
    break;

  case 869: /* $@113: %empty  */
                  {
        yyextra->das_nested_parentheses ++;
    }
    break;

  case 870: /* make_table_decl: '{' $@113 optional_emit_semis optional_expr_map_tuple_list '}'  */
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

  case 871: /* make_table_decl: "table" '(' expr_map_tuple_list optional_comma ')'  */
                                                                             {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 872: /* make_table_decl: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 873: /* make_table_decl: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 874: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 875: /* array_comprehension_where: ';' "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 876: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 877: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 878: /* array_comprehension: '[' "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                               {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 879: /* array_comprehension: '[' "iterator" "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' expr array_comprehension_where ']'  */
                                                                                                                                                                            {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-9])),(yyvsp[-7].pNameWithPosList),(yyvsp[-5].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 880: /* array_comprehension: '{' "for" '(' for_variable_name_with_pos_list "in" expr_list ')' ';' make_map_tuple array_comprehension_where '}'  */
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


