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
  YYSYMBOL_RPIPE = 131,                    /* "|>"  */
  YYSYMBOL_CLONEEQU = 132,                 /* ":="  */
  YYSYMBOL_ROTL = 133,                     /* "<<<"  */
  YYSYMBOL_ROTR = 134,                     /* ">>>"  */
  YYSYMBOL_ROTLEQU = 135,                  /* "<<<="  */
  YYSYMBOL_ROTREQU = 136,                  /* ">>>="  */
  YYSYMBOL_MAPTO = 137,                    /* "=>"  */
  YYSYMBOL_COLCOL = 138,                   /* "::"  */
  YYSYMBOL_ANDAND = 139,                   /* "&&"  */
  YYSYMBOL_OROR = 140,                     /* "||"  */
  YYSYMBOL_XORXOR = 141,                   /* "^^"  */
  YYSYMBOL_ANDANDEQU = 142,                /* "&&="  */
  YYSYMBOL_OROREQU = 143,                  /* "||="  */
  YYSYMBOL_XORXOREQU = 144,                /* "^^="  */
  YYSYMBOL_DOTDOT = 145,                   /* ".."  */
  YYSYMBOL_MTAG_E = 146,                   /* "$$"  */
  YYSYMBOL_MTAG_I = 147,                   /* "$i"  */
  YYSYMBOL_MTAG_V = 148,                   /* "$v"  */
  YYSYMBOL_MTAG_B = 149,                   /* "$b"  */
  YYSYMBOL_MTAG_A = 150,                   /* "$a"  */
  YYSYMBOL_MTAG_T = 151,                   /* "$t"  */
  YYSYMBOL_MTAG_C = 152,                   /* "$c"  */
  YYSYMBOL_MTAG_F = 153,                   /* "$f"  */
  YYSYMBOL_MTAG_DOTDOTDOT = 154,           /* "..."  */
  YYSYMBOL_INTEGER = 155,                  /* "integer constant"  */
  YYSYMBOL_LONG_INTEGER = 156,             /* "long integer constant"  */
  YYSYMBOL_UNSIGNED_INTEGER = 157,         /* "unsigned integer constant"  */
  YYSYMBOL_UNSIGNED_LONG_INTEGER = 158,    /* "unsigned long integer constant"  */
  YYSYMBOL_UNSIGNED_INT8 = 159,            /* "unsigned int8 constant"  */
  YYSYMBOL_FLOAT = 160,                    /* "floating point constant"  */
  YYSYMBOL_DOUBLE = 161,                   /* "double constant"  */
  YYSYMBOL_NAME = 162,                     /* "name"  */
  YYSYMBOL_BEGIN_STRING = 163,             /* "start of the string"  */
  YYSYMBOL_STRING_CHARACTER = 164,         /* STRING_CHARACTER  */
  YYSYMBOL_STRING_CHARACTER_ESC = 165,     /* STRING_CHARACTER_ESC  */
  YYSYMBOL_END_STRING = 166,               /* "end of the string"  */
  YYSYMBOL_BEGIN_STRING_EXPR = 167,        /* "{"  */
  YYSYMBOL_END_STRING_EXPR = 168,          /* "}"  */
  YYSYMBOL_END_OF_READ = 169,              /* "end of failed eader macro"  */
  YYSYMBOL_170_begin_of_code_block_ = 170, /* "begin of code block"  */
  YYSYMBOL_171_end_of_code_block_ = 171,   /* "end of code block"  */
  YYSYMBOL_172_end_of_expression_ = 172,   /* "end of expression"  */
  YYSYMBOL_SEMICOLON_CUR_CUR = 173,        /* ";}}"  */
  YYSYMBOL_SEMICOLON_CUR_SQR = 174,        /* ";}]"  */
  YYSYMBOL_SEMICOLON_SQR_SQR = 175,        /* ";]]"  */
  YYSYMBOL_COMMA_SQR_SQR = 176,            /* ",]]"  */
  YYSYMBOL_COMMA_CUR_SQR = 177,            /* ",}]"  */
  YYSYMBOL_178_ = 178,                     /* ','  */
  YYSYMBOL_179_ = 179,                     /* '='  */
  YYSYMBOL_180_ = 180,                     /* '?'  */
  YYSYMBOL_181_ = 181,                     /* ':'  */
  YYSYMBOL_182_ = 182,                     /* '|'  */
  YYSYMBOL_183_ = 183,                     /* '^'  */
  YYSYMBOL_184_ = 184,                     /* '&'  */
  YYSYMBOL_185_ = 185,                     /* '<'  */
  YYSYMBOL_186_ = 186,                     /* '>'  */
  YYSYMBOL_187_ = 187,                     /* '-'  */
  YYSYMBOL_188_ = 188,                     /* '+'  */
  YYSYMBOL_189_ = 189,                     /* '*'  */
  YYSYMBOL_190_ = 190,                     /* '/'  */
  YYSYMBOL_191_ = 191,                     /* '%'  */
  YYSYMBOL_UNARY_MINUS = 192,              /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 193,               /* UNARY_PLUS  */
  YYSYMBOL_194_ = 194,                     /* '~'  */
  YYSYMBOL_195_ = 195,                     /* '!'  */
  YYSYMBOL_PRE_INC = 196,                  /* PRE_INC  */
  YYSYMBOL_PRE_DEC = 197,                  /* PRE_DEC  */
  YYSYMBOL_LLPIPE = 198,                   /* LLPIPE  */
  YYSYMBOL_POST_INC = 199,                 /* POST_INC  */
  YYSYMBOL_POST_DEC = 200,                 /* POST_DEC  */
  YYSYMBOL_DEREF = 201,                    /* DEREF  */
  YYSYMBOL_202_ = 202,                     /* '.'  */
  YYSYMBOL_203_ = 203,                     /* '['  */
  YYSYMBOL_204_ = 204,                     /* ']'  */
  YYSYMBOL_205_ = 205,                     /* '('  */
  YYSYMBOL_206_ = 206,                     /* ')'  */
  YYSYMBOL_207_ = 207,                     /* '$'  */
  YYSYMBOL_208_ = 208,                     /* '@'  */
  YYSYMBOL_209_ = 209,                     /* '#'  */
  YYSYMBOL_YYACCEPT = 210,                 /* $accept  */
  YYSYMBOL_program = 211,                  /* program  */
  YYSYMBOL_top_level_reader_macro = 212,   /* top_level_reader_macro  */
  YYSYMBOL_optional_public_or_private_module = 213, /* optional_public_or_private_module  */
  YYSYMBOL_module_name = 214,              /* module_name  */
  YYSYMBOL_module_declaration = 215,       /* module_declaration  */
  YYSYMBOL_character_sequence = 216,       /* character_sequence  */
  YYSYMBOL_string_constant = 217,          /* string_constant  */
  YYSYMBOL_string_builder_body = 218,      /* string_builder_body  */
  YYSYMBOL_string_builder = 219,           /* string_builder  */
  YYSYMBOL_reader_character_sequence = 220, /* reader_character_sequence  */
  YYSYMBOL_expr_reader = 221,              /* expr_reader  */
  YYSYMBOL_222_1 = 222,                    /* $@1  */
  YYSYMBOL_options_declaration = 223,      /* options_declaration  */
  YYSYMBOL_require_declaration = 224,      /* require_declaration  */
  YYSYMBOL_require_module_name = 225,      /* require_module_name  */
  YYSYMBOL_require_module = 226,           /* require_module  */
  YYSYMBOL_is_public_module = 227,         /* is_public_module  */
  YYSYMBOL_expect_declaration = 228,       /* expect_declaration  */
  YYSYMBOL_expect_list = 229,              /* expect_list  */
  YYSYMBOL_expect_error = 230,             /* expect_error  */
  YYSYMBOL_expression_label = 231,         /* expression_label  */
  YYSYMBOL_expression_goto = 232,          /* expression_goto  */
  YYSYMBOL_elif_or_static_elif = 233,      /* elif_or_static_elif  */
  YYSYMBOL_expression_else = 234,          /* expression_else  */
  YYSYMBOL_if_or_static_if = 235,          /* if_or_static_if  */
  YYSYMBOL_expression_else_one_liner = 236, /* expression_else_one_liner  */
  YYSYMBOL_expression_if_one_liner = 237,  /* expression_if_one_liner  */
  YYSYMBOL_expression_if_then_else = 238,  /* expression_if_then_else  */
  YYSYMBOL_expression_for_loop = 239,      /* expression_for_loop  */
  YYSYMBOL_expression_unsafe = 240,        /* expression_unsafe  */
  YYSYMBOL_expression_while_loop = 241,    /* expression_while_loop  */
  YYSYMBOL_expression_with = 242,          /* expression_with  */
  YYSYMBOL_expression_with_alias = 243,    /* expression_with_alias  */
  YYSYMBOL_annotation_argument_value = 244, /* annotation_argument_value  */
  YYSYMBOL_annotation_argument_value_list = 245, /* annotation_argument_value_list  */
  YYSYMBOL_annotation_argument_name = 246, /* annotation_argument_name  */
  YYSYMBOL_annotation_argument = 247,      /* annotation_argument  */
  YYSYMBOL_annotation_argument_list = 248, /* annotation_argument_list  */
  YYSYMBOL_metadata_argument_list = 249,   /* metadata_argument_list  */
  YYSYMBOL_annotation_declaration_name = 250, /* annotation_declaration_name  */
  YYSYMBOL_annotation_declaration_basic = 251, /* annotation_declaration_basic  */
  YYSYMBOL_annotation_declaration = 252,   /* annotation_declaration  */
  YYSYMBOL_annotation_list = 253,          /* annotation_list  */
  YYSYMBOL_optional_annotation_list = 254, /* optional_annotation_list  */
  YYSYMBOL_optional_function_argument_list = 255, /* optional_function_argument_list  */
  YYSYMBOL_optional_function_type = 256,   /* optional_function_type  */
  YYSYMBOL_function_name = 257,            /* function_name  */
  YYSYMBOL_global_function_declaration = 258, /* global_function_declaration  */
  YYSYMBOL_optional_public_or_private_function = 259, /* optional_public_or_private_function  */
  YYSYMBOL_function_declaration_header = 260, /* function_declaration_header  */
  YYSYMBOL_function_declaration = 261,     /* function_declaration  */
  YYSYMBOL_262_2 = 262,                    /* $@2  */
  YYSYMBOL_expression_block = 263,         /* expression_block  */
  YYSYMBOL_expr_call_pipe = 264,           /* expr_call_pipe  */
  YYSYMBOL_expression_any = 265,           /* expression_any  */
  YYSYMBOL_expressions = 266,              /* expressions  */
  YYSYMBOL_optional_expr_list = 267,       /* optional_expr_list  */
  YYSYMBOL_optional_expr_map_tuple_list = 268, /* optional_expr_map_tuple_list  */
  YYSYMBOL_type_declaration_no_options_list = 269, /* type_declaration_no_options_list  */
  YYSYMBOL_name_in_namespace = 270,        /* name_in_namespace  */
  YYSYMBOL_expression_delete = 271,        /* expression_delete  */
  YYSYMBOL_new_type_declaration = 272,     /* new_type_declaration  */
  YYSYMBOL_273_3 = 273,                    /* $@3  */
  YYSYMBOL_274_4 = 274,                    /* $@4  */
  YYSYMBOL_expr_new = 275,                 /* expr_new  */
  YYSYMBOL_expression_break = 276,         /* expression_break  */
  YYSYMBOL_expression_continue = 277,      /* expression_continue  */
  YYSYMBOL_expression_return = 278,        /* expression_return  */
  YYSYMBOL_expression_yield = 279,         /* expression_yield  */
  YYSYMBOL_expression_try_catch = 280,     /* expression_try_catch  */
  YYSYMBOL_kwd_let_var_or_nothing = 281,   /* kwd_let_var_or_nothing  */
  YYSYMBOL_kwd_let = 282,                  /* kwd_let  */
  YYSYMBOL_optional_in_scope = 283,        /* optional_in_scope  */
  YYSYMBOL_tuple_expansion = 284,          /* tuple_expansion  */
  YYSYMBOL_tuple_expansion_variable_declaration = 285, /* tuple_expansion_variable_declaration  */
  YYSYMBOL_expression_let = 286,           /* expression_let  */
  YYSYMBOL_expr_cast = 287,                /* expr_cast  */
  YYSYMBOL_288_5 = 288,                    /* $@5  */
  YYSYMBOL_289_6 = 289,                    /* $@6  */
  YYSYMBOL_290_7 = 290,                    /* $@7  */
  YYSYMBOL_291_8 = 291,                    /* $@8  */
  YYSYMBOL_292_9 = 292,                    /* $@9  */
  YYSYMBOL_293_10 = 293,                   /* $@10  */
  YYSYMBOL_expr_type_decl = 294,           /* expr_type_decl  */
  YYSYMBOL_295_11 = 295,                   /* $@11  */
  YYSYMBOL_296_12 = 296,                   /* $@12  */
  YYSYMBOL_expr_type_info = 297,           /* expr_type_info  */
  YYSYMBOL_expr_list = 298,                /* expr_list  */
  YYSYMBOL_block_or_simple_block = 299,    /* block_or_simple_block  */
  YYSYMBOL_block_or_lambda = 300,          /* block_or_lambda  */
  YYSYMBOL_capture_entry = 301,            /* capture_entry  */
  YYSYMBOL_capture_list = 302,             /* capture_list  */
  YYSYMBOL_optional_capture_list = 303,    /* optional_capture_list  */
  YYSYMBOL_expr_full_block = 304,          /* expr_full_block  */
  YYSYMBOL_expr_full_block_assumed_piped = 305, /* expr_full_block_assumed_piped  */
  YYSYMBOL_expr_numeric_const = 306,       /* expr_numeric_const  */
  YYSYMBOL_expr_assign = 307,              /* expr_assign  */
  YYSYMBOL_expr_named_call = 308,          /* expr_named_call  */
  YYSYMBOL_expr_method_call = 309,         /* expr_method_call  */
  YYSYMBOL_func_addr_name = 310,           /* func_addr_name  */
  YYSYMBOL_func_addr_expr = 311,           /* func_addr_expr  */
  YYSYMBOL_312_13 = 312,                   /* $@13  */
  YYSYMBOL_313_14 = 313,                   /* $@14  */
  YYSYMBOL_314_15 = 314,                   /* $@15  */
  YYSYMBOL_315_16 = 315,                   /* $@16  */
  YYSYMBOL_expr_field = 316,               /* expr_field  */
  YYSYMBOL_317_17 = 317,                   /* $@17  */
  YYSYMBOL_318_18 = 318,                   /* $@18  */
  YYSYMBOL_expr_call = 319,                /* expr_call  */
  YYSYMBOL_expr = 320,                     /* expr  */
  YYSYMBOL_321_19 = 321,                   /* $@19  */
  YYSYMBOL_322_20 = 322,                   /* $@20  */
  YYSYMBOL_323_21 = 323,                   /* $@21  */
  YYSYMBOL_324_22 = 324,                   /* $@22  */
  YYSYMBOL_325_23 = 325,                   /* $@23  */
  YYSYMBOL_326_24 = 326,                   /* $@24  */
  YYSYMBOL_expr_mtag = 327,                /* expr_mtag  */
  YYSYMBOL_optional_field_annotation = 328, /* optional_field_annotation  */
  YYSYMBOL_optional_override = 329,        /* optional_override  */
  YYSYMBOL_optional_constant = 330,        /* optional_constant  */
  YYSYMBOL_optional_public_or_private_member_variable = 331, /* optional_public_or_private_member_variable  */
  YYSYMBOL_optional_static_member_variable = 332, /* optional_static_member_variable  */
  YYSYMBOL_structure_variable_declaration = 333, /* structure_variable_declaration  */
  YYSYMBOL_struct_variable_declaration_list = 334, /* struct_variable_declaration_list  */
  YYSYMBOL_335_25 = 335,                   /* $@25  */
  YYSYMBOL_336_26 = 336,                   /* $@26  */
  YYSYMBOL_337_27 = 337,                   /* $@27  */
  YYSYMBOL_function_argument_declaration = 338, /* function_argument_declaration  */
  YYSYMBOL_function_argument_list = 339,   /* function_argument_list  */
  YYSYMBOL_tuple_type = 340,               /* tuple_type  */
  YYSYMBOL_tuple_type_list = 341,          /* tuple_type_list  */
  YYSYMBOL_tuple_alias_type_list = 342,    /* tuple_alias_type_list  */
  YYSYMBOL_variant_type = 343,             /* variant_type  */
  YYSYMBOL_variant_type_list = 344,        /* variant_type_list  */
  YYSYMBOL_variant_alias_type_list = 345,  /* variant_alias_type_list  */
  YYSYMBOL_copy_or_move = 346,             /* copy_or_move  */
  YYSYMBOL_variable_declaration = 347,     /* variable_declaration  */
  YYSYMBOL_copy_or_move_or_clone = 348,    /* copy_or_move_or_clone  */
  YYSYMBOL_optional_ref = 349,             /* optional_ref  */
  YYSYMBOL_let_variable_name_with_pos_list = 350, /* let_variable_name_with_pos_list  */
  YYSYMBOL_global_let_variable_name_with_pos_list = 351, /* global_let_variable_name_with_pos_list  */
  YYSYMBOL_let_variable_declaration = 352, /* let_variable_declaration  */
  YYSYMBOL_global_let_variable_declaration = 353, /* global_let_variable_declaration  */
  YYSYMBOL_optional_shared = 354,          /* optional_shared  */
  YYSYMBOL_optional_public_or_private_variable = 355, /* optional_public_or_private_variable  */
  YYSYMBOL_global_let = 356,               /* global_let  */
  YYSYMBOL_357_28 = 357,                   /* $@28  */
  YYSYMBOL_enum_expression = 358,          /* enum_expression  */
  YYSYMBOL_enum_list = 359,                /* enum_list  */
  YYSYMBOL_optional_public_or_private_alias = 360, /* optional_public_or_private_alias  */
  YYSYMBOL_single_alias = 361,             /* single_alias  */
  YYSYMBOL_362_29 = 362,                   /* $@29  */
  YYSYMBOL_alias_declaration = 363,        /* alias_declaration  */
  YYSYMBOL_optional_public_or_private_enum = 364, /* optional_public_or_private_enum  */
  YYSYMBOL_enum_name = 365,                /* enum_name  */
  YYSYMBOL_optional_enum_basic_type_declaration = 366, /* optional_enum_basic_type_declaration  */
  YYSYMBOL_enum_declaration = 367,         /* enum_declaration  */
  YYSYMBOL_368_30 = 368,                   /* $@30  */
  YYSYMBOL_369_31 = 369,                   /* $@31  */
  YYSYMBOL_optional_structure_parent = 370, /* optional_structure_parent  */
  YYSYMBOL_optional_sealed = 371,          /* optional_sealed  */
  YYSYMBOL_structure_name = 372,           /* structure_name  */
  YYSYMBOL_class_or_struct = 373,          /* class_or_struct  */
  YYSYMBOL_optional_public_or_private_structure = 374, /* optional_public_or_private_structure  */
  YYSYMBOL_optional_struct_variable_declaration_list = 375, /* optional_struct_variable_declaration_list  */
  YYSYMBOL_structure_declaration = 376,    /* structure_declaration  */
  YYSYMBOL_377_32 = 377,                   /* $@32  */
  YYSYMBOL_378_33 = 378,                   /* $@33  */
  YYSYMBOL_variable_name_with_pos_list = 379, /* variable_name_with_pos_list  */
  YYSYMBOL_basic_type_declaration = 380,   /* basic_type_declaration  */
  YYSYMBOL_enum_basic_type_declaration = 381, /* enum_basic_type_declaration  */
  YYSYMBOL_structure_type_declaration = 382, /* structure_type_declaration  */
  YYSYMBOL_auto_type_declaration = 383,    /* auto_type_declaration  */
  YYSYMBOL_bitfield_bits = 384,            /* bitfield_bits  */
  YYSYMBOL_bitfield_alias_bits = 385,      /* bitfield_alias_bits  */
  YYSYMBOL_bitfield_type_declaration = 386, /* bitfield_type_declaration  */
  YYSYMBOL_387_34 = 387,                   /* $@34  */
  YYSYMBOL_388_35 = 388,                   /* $@35  */
  YYSYMBOL_c_or_s = 389,                   /* c_or_s  */
  YYSYMBOL_table_type_pair = 390,          /* table_type_pair  */
  YYSYMBOL_dim_list = 391,                 /* dim_list  */
  YYSYMBOL_type_declaration_no_options = 392, /* type_declaration_no_options  */
  YYSYMBOL_type_declaration_no_options_no_dim = 393, /* type_declaration_no_options_no_dim  */
  YYSYMBOL_394_36 = 394,                   /* $@36  */
  YYSYMBOL_395_37 = 395,                   /* $@37  */
  YYSYMBOL_396_38 = 396,                   /* $@38  */
  YYSYMBOL_397_39 = 397,                   /* $@39  */
  YYSYMBOL_398_40 = 398,                   /* $@40  */
  YYSYMBOL_399_41 = 399,                   /* $@41  */
  YYSYMBOL_400_42 = 400,                   /* $@42  */
  YYSYMBOL_401_43 = 401,                   /* $@43  */
  YYSYMBOL_402_44 = 402,                   /* $@44  */
  YYSYMBOL_403_45 = 403,                   /* $@45  */
  YYSYMBOL_404_46 = 404,                   /* $@46  */
  YYSYMBOL_405_47 = 405,                   /* $@47  */
  YYSYMBOL_406_48 = 406,                   /* $@48  */
  YYSYMBOL_407_49 = 407,                   /* $@49  */
  YYSYMBOL_408_50 = 408,                   /* $@50  */
  YYSYMBOL_409_51 = 409,                   /* $@51  */
  YYSYMBOL_410_52 = 410,                   /* $@52  */
  YYSYMBOL_411_53 = 411,                   /* $@53  */
  YYSYMBOL_412_54 = 412,                   /* $@54  */
  YYSYMBOL_413_55 = 413,                   /* $@55  */
  YYSYMBOL_414_56 = 414,                   /* $@56  */
  YYSYMBOL_415_57 = 415,                   /* $@57  */
  YYSYMBOL_416_58 = 416,                   /* $@58  */
  YYSYMBOL_417_59 = 417,                   /* $@59  */
  YYSYMBOL_418_60 = 418,                   /* $@60  */
  YYSYMBOL_419_61 = 419,                   /* $@61  */
  YYSYMBOL_420_62 = 420,                   /* $@62  */
  YYSYMBOL_type_declaration = 421,         /* type_declaration  */
  YYSYMBOL_tuple_alias_declaration = 422,  /* tuple_alias_declaration  */
  YYSYMBOL_423_63 = 423,                   /* $@63  */
  YYSYMBOL_424_64 = 424,                   /* $@64  */
  YYSYMBOL_425_65 = 425,                   /* $@65  */
  YYSYMBOL_variant_alias_declaration = 426, /* variant_alias_declaration  */
  YYSYMBOL_427_66 = 427,                   /* $@66  */
  YYSYMBOL_428_67 = 428,                   /* $@67  */
  YYSYMBOL_429_68 = 429,                   /* $@68  */
  YYSYMBOL_bitfield_alias_declaration = 430, /* bitfield_alias_declaration  */
  YYSYMBOL_431_69 = 431,                   /* $@69  */
  YYSYMBOL_432_70 = 432,                   /* $@70  */
  YYSYMBOL_433_71 = 433,                   /* $@71  */
  YYSYMBOL_make_decl = 434,                /* make_decl  */
  YYSYMBOL_make_struct_fields = 435,       /* make_struct_fields  */
  YYSYMBOL_make_variant_dim = 436,         /* make_variant_dim  */
  YYSYMBOL_make_struct_single = 437,       /* make_struct_single  */
  YYSYMBOL_make_struct_dim_list = 438,     /* make_struct_dim_list  */
  YYSYMBOL_make_struct_dim_decl = 439,     /* make_struct_dim_decl  */
  YYSYMBOL_optional_make_struct_dim_decl = 440, /* optional_make_struct_dim_decl  */
  YYSYMBOL_use_initializer = 441,          /* use_initializer  */
  YYSYMBOL_make_struct_decl = 442,         /* make_struct_decl  */
  YYSYMBOL_443_72 = 443,                   /* $@72  */
  YYSYMBOL_444_73 = 444,                   /* $@73  */
  YYSYMBOL_445_74 = 445,                   /* $@74  */
  YYSYMBOL_446_75 = 446,                   /* $@75  */
  YYSYMBOL_447_76 = 447,                   /* $@76  */
  YYSYMBOL_448_77 = 448,                   /* $@77  */
  YYSYMBOL_449_78 = 449,                   /* $@78  */
  YYSYMBOL_450_79 = 450,                   /* $@79  */
  YYSYMBOL_make_map_tuple = 451,           /* make_map_tuple  */
  YYSYMBOL_make_tuple_call = 452,          /* make_tuple_call  */
  YYSYMBOL_453_80 = 453,                   /* $@80  */
  YYSYMBOL_454_81 = 454,                   /* $@81  */
  YYSYMBOL_make_dim_decl = 455,            /* make_dim_decl  */
  YYSYMBOL_456_82 = 456,                   /* $@82  */
  YYSYMBOL_457_83 = 457,                   /* $@83  */
  YYSYMBOL_458_84 = 458,                   /* $@84  */
  YYSYMBOL_459_85 = 459,                   /* $@85  */
  YYSYMBOL_460_86 = 460,                   /* $@86  */
  YYSYMBOL_461_87 = 461,                   /* $@87  */
  YYSYMBOL_462_88 = 462,                   /* $@88  */
  YYSYMBOL_463_89 = 463,                   /* $@89  */
  YYSYMBOL_464_90 = 464,                   /* $@90  */
  YYSYMBOL_465_91 = 465,                   /* $@91  */
  YYSYMBOL_expr_map_tuple_list = 466,      /* expr_map_tuple_list  */
  YYSYMBOL_make_table_decl = 467,          /* make_table_decl  */
  YYSYMBOL_array_comprehension_where = 468, /* array_comprehension_where  */
  YYSYMBOL_optional_comma = 469,           /* optional_comma  */
  YYSYMBOL_array_comprehension = 470       /* array_comprehension  */
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
#define YYLAST   11071

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  210
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  261
/* YYNRULES -- Number of rules.  */
#define YYNRULES  782
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1436

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   437


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
       2,     2,     2,   195,     2,   209,   207,   191,   184,     2,
     205,   206,   189,   188,   178,   187,   202,   190,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   181,   172,
     185,   179,   186,   180,   208,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   203,     2,   204,   183,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   170,   182,   171,   194,     2,     2,     2,
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
     165,   166,   167,   168,   169,   173,   174,   175,   176,   177,
     192,   193,   196,   197,   198,   199,   200,   201
};

#if DAS2_YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   533,   533,   534,   539,   540,   541,   542,   543,   544,
     545,   546,   547,   548,   549,   550,   551,   555,   561,   562,
     563,   567,   568,   572,   590,   591,   592,   593,   597,   598,
     602,   607,   616,   624,   640,   645,   653,   653,   692,   710,
     714,   717,   721,   727,   736,   739,   745,   746,   750,   754,
     755,   759,   762,   768,   774,   777,   783,   784,   788,   789,
     790,   799,   800,   804,   805,   811,   812,   813,   814,   815,
     819,   825,   831,   837,   845,   855,   864,   871,   872,   873,
     874,   875,   876,   880,   885,   893,   894,   895,   899,   900,
     901,   902,   903,   904,   905,   906,   912,   915,   921,   924,
     930,   931,   932,   936,   949,   967,   970,   978,   989,  1000,
    1011,  1014,  1021,  1025,  1032,  1033,  1037,  1038,  1039,  1043,
    1046,  1053,  1057,  1058,  1059,  1060,  1061,  1062,  1063,  1064,
    1065,  1066,  1067,  1068,  1069,  1070,  1071,  1072,  1073,  1074,
    1075,  1076,  1077,  1078,  1079,  1080,  1081,  1082,  1083,  1084,
    1085,  1086,  1087,  1088,  1089,  1090,  1091,  1092,  1093,  1094,
    1095,  1096,  1097,  1098,  1099,  1100,  1101,  1102,  1103,  1104,
    1105,  1106,  1107,  1108,  1109,  1110,  1111,  1112,  1113,  1114,
    1115,  1116,  1117,  1118,  1119,  1120,  1121,  1122,  1123,  1124,
    1125,  1126,  1127,  1128,  1129,  1130,  1131,  1132,  1133,  1134,
    1135,  1136,  1137,  1138,  1139,  1140,  1145,  1163,  1164,  1165,
    1169,  1175,  1175,  1192,  1196,  1207,  1220,  1221,  1222,  1223,
    1224,  1225,  1226,  1227,  1228,  1229,  1230,  1231,  1232,  1233,
    1234,  1235,  1236,  1237,  1241,  1246,  1252,  1258,  1259,  1263,
    1264,  1268,  1272,  1279,  1280,  1291,  1295,  1298,  1306,  1306,
    1306,  1312,  1315,  1319,  1323,  1330,  1336,  1340,  1344,  1347,
    1350,  1358,  1361,  1369,  1375,  1376,  1377,  1381,  1382,  1386,
    1387,  1391,  1396,  1404,  1410,  1422,  1425,  1431,  1431,  1431,
    1434,  1434,  1434,  1439,  1439,  1439,  1447,  1447,  1447,  1453,
    1463,  1474,  1489,  1492,  1498,  1499,  1506,  1517,  1518,  1519,
    1523,  1524,  1525,  1526,  1527,  1531,  1536,  1544,  1545,  1549,
    1556,  1560,  1566,  1567,  1568,  1569,  1570,  1571,  1572,  1576,
    1577,  1578,  1579,  1580,  1581,  1582,  1583,  1584,  1585,  1586,
    1587,  1588,  1589,  1590,  1591,  1592,  1593,  1594,  1598,  1605,
    1617,  1622,  1632,  1636,  1643,  1646,  1646,  1646,  1651,  1651,
    1651,  1664,  1668,  1672,  1677,  1684,  1689,  1696,  1696,  1696,
    1703,  1707,  1717,  1726,  1735,  1739,  1742,  1748,  1749,  1750,
    1751,  1752,  1753,  1754,  1755,  1756,  1757,  1758,  1759,  1760,
    1761,  1762,  1763,  1764,  1765,  1766,  1767,  1768,  1769,  1770,
    1771,  1772,  1773,  1774,  1775,  1776,  1777,  1778,  1779,  1780,
    1781,  1782,  1783,  1789,  1790,  1791,  1792,  1793,  1806,  1807,
    1808,  1809,  1810,  1811,  1812,  1813,  1814,  1815,  1816,  1817,
    1820,  1823,  1828,  1829,  1832,  1832,  1832,  1835,  1840,  1844,
    1848,  1848,  1848,  1853,  1856,  1860,  1860,  1860,  1865,  1868,
    1869,  1870,  1871,  1872,  1873,  1874,  1875,  1876,  1878,  1882,
    1883,  1891,  1892,  1893,  1894,  1895,  1896,  1897,  1901,  1905,
    1909,  1913,  1917,  1921,  1925,  1929,  1933,  1940,  1941,  1945,
    1946,  1947,  1951,  1952,  1956,  1957,  1958,  1962,  1963,  1967,
    1978,  1981,  1981,  2000,  1999,  2013,  2012,  2028,  2037,  2047,
    2048,  2052,  2055,  2064,  2065,  2069,  2072,  2075,  2091,  2100,
    2101,  2105,  2108,  2111,  2125,  2126,  2130,  2136,  2142,  2145,
    2149,  2158,  2159,  2160,  2164,  2165,  2169,  2176,  2181,  2190,
    2196,  2207,  2214,  2224,  2227,  2232,  2243,  2246,  2251,  2263,
    2264,  2268,  2269,  2270,  2274,  2274,  2292,  2296,  2303,  2306,
    2319,  2336,  2337,  2338,  2343,  2343,  2369,  2373,  2374,  2375,
    2379,  2389,  2392,  2398,  2403,  2398,  2418,  2419,  2423,  2424,
    2428,  2434,  2435,  2439,  2440,  2441,  2445,  2448,  2454,  2459,
    2454,  2473,  2480,  2485,  2494,  2500,  2511,  2512,  2513,  2514,
    2515,  2516,  2517,  2518,  2519,  2520,  2521,  2522,  2523,  2524,
    2525,  2526,  2527,  2528,  2529,  2530,  2531,  2532,  2533,  2534,
    2535,  2536,  2537,  2541,  2542,  2543,  2544,  2545,  2546,  2547,
    2548,  2552,  2563,  2567,  2574,  2586,  2593,  2602,  2607,  2617,
    2630,  2630,  2630,  2643,  2644,  2648,  2652,  2659,  2663,  2667,
    2671,  2678,  2681,  2699,  2700,  2701,  2702,  2703,  2703,  2703,
    2707,  2712,  2719,  2719,  2726,  2730,  2734,  2739,  2744,  2749,
    2754,  2758,  2762,  2767,  2771,  2775,  2780,  2780,  2780,  2786,
    2793,  2793,  2793,  2798,  2798,  2798,  2804,  2804,  2804,  2809,
    2813,  2813,  2813,  2818,  2818,  2818,  2827,  2831,  2831,  2831,
    2836,  2836,  2836,  2845,  2849,  2849,  2849,  2854,  2854,  2854,
    2863,  2863,  2863,  2869,  2869,  2869,  2878,  2881,  2892,  2908,
    2913,  2918,  2908,  2943,  2948,  2954,  2943,  2979,  2984,  2989,
    2979,  3019,  3020,  3021,  3022,  3023,  3027,  3034,  3041,  3047,
    3053,  3060,  3067,  3073,  3082,  3088,  3096,  3101,  3108,  3113,
    3119,  3120,  3124,  3125,  3129,  3129,  3129,  3137,  3137,  3137,
    3144,  3144,  3144,  3151,  3151,  3151,  3162,  3168,  3174,  3180,
    3180,  3180,  3190,  3198,  3198,  3198,  3208,  3208,  3208,  3218,
    3218,  3218,  3228,  3236,  3236,  3236,  3255,  3262,  3262,  3262,
    3272,  3275,  3281,  3289,  3297,  3317,  3342,  3343,  3347,  3348,
    3353,  3356,  3359
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
  "\"?[\"", "\"<|\"", "\"|>\"", "\":=\"", "\"<<<\"", "\">>>\"", "\"<<<=\"",
  "\">>>=\"", "\"=>\"", "\"::\"", "\"&&\"", "\"||\"", "\"^^\"", "\"&&=\"",
  "\"||=\"", "\"^^=\"", "\"..\"", "\"$$\"", "\"$i\"", "\"$v\"", "\"$b\"",
  "\"$a\"", "\"$t\"", "\"$c\"", "\"$f\"", "\"...\"",
  "\"integer constant\"", "\"long integer constant\"",
  "\"unsigned integer constant\"", "\"unsigned long integer constant\"",
  "\"unsigned int8 constant\"", "\"floating point constant\"",
  "\"double constant\"", "\"name\"", "\"start of the string\"",
  "STRING_CHARACTER", "STRING_CHARACTER_ESC", "\"end of the string\"",
  "\"{\"", "\"}\"", "\"end of failed eader macro\"",
  "\"begin of code block\"", "\"end of code block\"",
  "\"end of expression\"", "\";}}\"", "\";}]\"", "\";]]\"", "\",]]\"",
  "\",}]\"", "','", "'='", "'?'", "':'", "'|'", "'^'", "'&'", "'<'", "'>'",
  "'-'", "'+'", "'*'", "'/'", "'%'", "UNARY_MINUS", "UNARY_PLUS", "'~'",
  "'!'", "PRE_INC", "PRE_DEC", "LLPIPE", "POST_INC", "POST_DEC", "DEREF",
  "'.'", "'['", "']'", "'('", "')'", "'$'", "'@'", "'#'", "$accept",
  "program", "top_level_reader_macro", "optional_public_or_private_module",
  "module_name", "module_declaration", "character_sequence",
  "string_constant", "string_builder_body", "string_builder",
  "reader_character_sequence", "expr_reader", "$@1", "options_declaration",
  "require_declaration", "require_module_name", "require_module",
  "is_public_module", "expect_declaration", "expect_list", "expect_error",
  "expression_label", "expression_goto", "elif_or_static_elif",
  "expression_else", "if_or_static_if", "expression_else_one_liner",
  "expression_if_one_liner", "expression_if_then_else",
  "expression_for_loop", "expression_unsafe", "expression_while_loop",
  "expression_with", "expression_with_alias", "annotation_argument_value",
  "annotation_argument_value_list", "annotation_argument_name",
  "annotation_argument", "annotation_argument_list",
  "metadata_argument_list", "annotation_declaration_name",
  "annotation_declaration_basic", "annotation_declaration",
  "annotation_list", "optional_annotation_list",
  "optional_function_argument_list", "optional_function_type",
  "function_name", "global_function_declaration",
  "optional_public_or_private_function", "function_declaration_header",
  "function_declaration", "$@2", "expression_block", "expr_call_pipe",
  "expression_any", "expressions", "optional_expr_list",
  "optional_expr_map_tuple_list", "type_declaration_no_options_list",
  "name_in_namespace", "expression_delete", "new_type_declaration", "$@3",
  "$@4", "expr_new", "expression_break", "expression_continue",
  "expression_return", "expression_yield", "expression_try_catch",
  "kwd_let_var_or_nothing", "kwd_let", "optional_in_scope",
  "tuple_expansion", "tuple_expansion_variable_declaration",
  "expression_let", "expr_cast", "$@5", "$@6", "$@7", "$@8", "$@9", "$@10",
  "expr_type_decl", "$@11", "$@12", "expr_type_info", "expr_list",
  "block_or_simple_block", "block_or_lambda", "capture_entry",
  "capture_list", "optional_capture_list", "expr_full_block",
  "expr_full_block_assumed_piped", "expr_numeric_const", "expr_assign",
  "expr_named_call", "expr_method_call", "func_addr_name",
  "func_addr_expr", "$@13", "$@14", "$@15", "$@16", "expr_field", "$@17",
  "$@18", "expr_call", "expr", "$@19", "$@20", "$@21", "$@22", "$@23",
  "$@24", "expr_mtag", "optional_field_annotation", "optional_override",
  "optional_constant", "optional_public_or_private_member_variable",
  "optional_static_member_variable", "structure_variable_declaration",
  "struct_variable_declaration_list", "$@25", "$@26", "$@27",
  "function_argument_declaration", "function_argument_list", "tuple_type",
  "tuple_type_list", "tuple_alias_type_list", "variant_type",
  "variant_type_list", "variant_alias_type_list", "copy_or_move",
  "variable_declaration", "copy_or_move_or_clone", "optional_ref",
  "let_variable_name_with_pos_list",
  "global_let_variable_name_with_pos_list", "let_variable_declaration",
  "global_let_variable_declaration", "optional_shared",
  "optional_public_or_private_variable", "global_let", "$@28",
  "enum_expression", "enum_list", "optional_public_or_private_alias",
  "single_alias", "$@29", "alias_declaration",
  "optional_public_or_private_enum", "enum_name",
  "optional_enum_basic_type_declaration", "enum_declaration", "$@30",
  "$@31", "optional_structure_parent", "optional_sealed", "structure_name",
  "class_or_struct", "optional_public_or_private_structure",
  "optional_struct_variable_declaration_list", "structure_declaration",
  "$@32", "$@33", "variable_name_with_pos_list", "basic_type_declaration",
  "enum_basic_type_declaration", "structure_type_declaration",
  "auto_type_declaration", "bitfield_bits", "bitfield_alias_bits",
  "bitfield_type_declaration", "$@34", "$@35", "c_or_s", "table_type_pair",
  "dim_list", "type_declaration_no_options",
  "type_declaration_no_options_no_dim", "$@36", "$@37", "$@38", "$@39",
  "$@40", "$@41", "$@42", "$@43", "$@44", "$@45", "$@46", "$@47", "$@48",
  "$@49", "$@50", "$@51", "$@52", "$@53", "$@54", "$@55", "$@56", "$@57",
  "$@58", "$@59", "$@60", "$@61", "$@62", "type_declaration",
  "tuple_alias_declaration", "$@63", "$@64", "$@65",
  "variant_alias_declaration", "$@66", "$@67", "$@68",
  "bitfield_alias_declaration", "$@69", "$@70", "$@71", "make_decl",
  "make_struct_fields", "make_variant_dim", "make_struct_single",
  "make_struct_dim_list", "make_struct_dim_decl",
  "optional_make_struct_dim_decl", "use_initializer", "make_struct_decl",
  "$@72", "$@73", "$@74", "$@75", "$@76", "$@77", "$@78", "$@79",
  "make_map_tuple", "make_tuple_call", "$@80", "$@81", "make_dim_decl",
  "$@82", "$@83", "$@84", "$@85", "$@86", "$@87", "$@88", "$@89", "$@90",
  "$@91", "expr_map_tuple_list", "make_table_decl",
  "array_comprehension_where", "optional_comma", "array_comprehension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1260)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-688)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
   -1260,    26, -1260, -1260,    77,   -65,    78,     2, -1260,  -108,
       2,     2,     2, -1260,   -28,    16, -1260, -1260,   -94,    20,
   -1260, -1260,   145, -1260,    33, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260,   121, -1260,    93,   111,   132,
   -1260, -1260,    78,    10, -1260, -1260, -1260,   140,   171, -1260,
   -1260,    33,   235,   238,   286,   287,   224, -1260, -1260, -1260,
      16,    16,    16,   212, -1260,   533,   126, -1260, -1260, -1260,
   -1260, -1260,   347,   500,   527, -1260,   536,    13,    77,   279,
     -65,   244,   302, -1260,   303,   319, -1260, -1260, -1260,   538,
   -1260, -1260, -1260, -1260,   321,   341, -1260, -1260,   -71,    77,
      16,    16,    16,    16, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260,   358, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260,   513,    98, -1260, -1260, -1260, -1260,   473,
   -1260, -1260,   343, -1260, -1260, -1260,   361,   375,   412, -1260,
   -1260,   421, -1260,    95, -1260,   113,   464,   533, 10900, -1260,
     430,   507,   414, -1260, -1260, -1260,   518, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260,   105, -1260,  1518, -1260, -1260, -1260,
   -1260, -1260,  9476, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,   588,   600,
   -1260,   431,   491,   396,   505, -1260,   485, -1260,    77,   472,
     508, -1260, -1260, -1260,    98, -1260,   453,   501,   509,   482,
     510,   516, -1260, -1260, -1260,   487, -1260, -1260, -1260, -1260,
   -1260,   517, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260,   520, -1260, -1260, -1260,   522,   523, -1260,
   -1260, -1260, -1260,   524,   525,   488,   -28, -1260, -1260, -1260,
   -1260, -1260, -1260,   519,   529,   542, -1260, -1260,   550,   551,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
     552,   494, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260,   693, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260,   554,   514, -1260,
   -1260,   -97,   539, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260,   546,   547, -1260,    77, -1260,
     405, -1260, -1260, -1260, -1260, -1260,  6104, -1260, -1260,   557,
   -1260,   254,   255,   264, -1260, -1260,  6104,   -26, -1260, -1260,
   -1260,    -2, -1260, -1260, -1260,    42,  3192, -1260,   526,  1234,
   -1260,   545,  1335,   320, -1260, -1260, -1260, -1260,   568,   599,
   -1260,   528, -1260,    55, -1260,    92,  1518, -1260,  1808,   574,
     -28, -1260, -1260, -1260, -1260,   575,  1518, -1260,    37,  1518,
    1518,  1518,   553,   555, -1260, -1260,    87,   -28,   556,    22,
   -1260,   -25,   534,   560,   561,   537,   565,   558,    58,   567,
   -1260,   129,   569,   570,  6104,  6104,   559,   562,   563,   566,
     571,   573, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260,  3388,  6104,  6104,  6104,  6104,  6104,  2998,  6104, -1260,
     548, -1260, -1260, -1260,   578, -1260, -1260, -1260, -1260,   577,
   -1260, -1260, -1260, -1260, -1260, -1260,   -85,  6441, -1260,   580,
   -1260, -1260, -1260, -1260, -1260, -1260,  1518,  1518,   564,   604,
    1518,   431,  1518,   431,  1518,   431,  6447,   610,  6477, -1260,
    6104, -1260, -1260, -1260, -1260,   584, -1260, -1260,  8929,  3582,
   -1260, -1260,   611, -1260,   -61, -1260, -1260,   249, -1260,   529,
     618,   609,   249, -1260,   620, -1260, -1260,  6104, -1260, -1260,
     108,   -89, -1260,   529, -1260,   589, -1260, -1260,   590,  3776,
   -1260,   491,  3970,   591,   635, -1260,   626,   644,  4164,   -44,
    4358,   770, -1260,   637,   638,   606,   805, -1260, -1260, -1260,
   -1260, -1260,   647, -1260,   648,   649,   650,   651,   652, -1260,
     750, -1260,   653,  9360,   654, -1260,   656, -1260,    17, -1260,
       5, -1260, -1260, -1260,  6104,   298,   395,   640,   369, -1260,
   -1260, -1260,   623,   625, -1260,   174, -1260,   646,   655,   657,
   -1260,  6104,  1518,  6104,  6104, -1260, -1260,  6104, -1260,  6104,
   -1260,  6104, -1260, -1260,  6104, -1260,  1518,   326,   326,  6104,
    6104,  6104,  6104,  6104,  6104,   502,   108,  9507, -1260,   658,
     326,   326,   -86,   326,   326,   108,   819,   660, 10164,   660,
     147,  2606,   831, -1260,   631,   577, -1260, 10659, 10747,  6104,
    6104, -1260, -1260,  6104,  6104,  6104,  6104,   681,  6104,   134,
    6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,  4552,
    6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,
   10868,  6104, -1260,  4746,   416,   462, -1260, -1260,   251,   467,
     539,   468,   539,   469,   539, -1260,   247, -1260,   313, -1260,
    1518,   639,   660, -1260, -1260, -1260,  9019, -1260,   670,  1518,
   -1260, -1260,  1518, -1260, -1260,  6569,   641,   801, -1260,   -77,
   -1260,  6104,   108,  6104, 10164,   833,  6104, 10164,  6104,   671,
   -1260,   672,   697, 10164, -1260,  6104, 10164,   684, -1260, -1260,
    6104,   662, -1260, -1260, -1260, -1260, -1260, -1260, -1260,   -90,
   -1260,  6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,
    6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,  6104,
     574, -1260, -1260,   847,   414, -1260,  6104,  9606, -1260, -1260,
   -1260,  1518,  1518,  1518,  1518,   786,  6104,   695,  6104,  1518,
   -1260, -1260, -1260,  1518,   660,   339,   658,  6668,  1518,  1518,
    6699,  1518,  6798,  1518,   660,  1518,  1518,   660,  1518,   674,
    6890,  6923,  7015,  7114,  7145,  7244, -1260,  6104,   200,    21,
    6104,  6104,   687,    24,   108,  6104,   659,   663,   665,   666,
     272, -1260, -1260,   -62,   667,    65,  2802, -1260,   155,   683,
     668,   676,   431,  2013, -1260,   831,   688,   677, -1260, -1260,
     690,   680, -1260, -1260,   149,   149,  1045,  1045,   827,   827,
     685,   284,   686, -1260,  9050,   -45,   -45,   580,   149,   149,
   10481, 10367, 10399, 10253, 10780,  9695, 10513, 10595,   385,  1045,
    1045,   675,   675,   284,   284,   284,   285,  6104,   689,   692,
     299,  6104,   867,   694,  9140, -1260,   167, -1260, -1260,   714,
   -1260, -1260,   700, -1260,   702, -1260,   703, -1260,  6447, -1260,
     610,   362,   529, -1260, -1260, -1260, -1260,   529,   529, -1260,
    6104,   717, -1260,   718, -1260,  1518, -1260,  6104,  7336,    32,
   10164,   491, 10164,  7369,  6104, -1260, -1260, 10164, -1260,  7461,
    6104,   698,   850,   736, -1260,   413, -1260, 10164, 10164, 10164,
   10164, 10164, 10164, 10164, 10164, 10164, 10164, 10164, 10164, 10164,
   10164, 10164, 10164, 10164, 10164, 10164, -1260,   729,   549,   835,
     732,  9727, -1260, -1260, -1260, -1260,   529,   719,   720,   470,
   -1260,   -20,   701,   364,  7560,   476,  1518,  1518,  1518,   723,
     704,   706,  1518,   712, -1260,   734,   735, -1260,   737, -1260,
     738,   716,   739,   742,   724,   743,   831, -1260, -1260, -1260,
   -1260, -1260,   726,  9809,  6104, 10164, -1260, -1260,  6104,    35,
   10164, -1260, -1260,  6104,  6104,  1518,   431,    40, -1260,   727,
    6104,  6104,  6104,   176,  6298, -1260,   371, -1260,   124,   539,
   -1260,   431, -1260,  6104, -1260,  6104,  4940,  6104, -1260,   747,
     730, -1260, -1260,  6104,   731, -1260,  9171,  6104,  5134,   733,
   -1260,  9261, -1260,  5328, -1260,  6104, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260,   745,  1518,  7591, -1260,   891,   -52, 10164,   491,  6104,
   -1260,   491, 10164,  2218,   491,  7690,  6104,   775, -1260,   168,
     777,  1518,    37, -1260, -1260, -1260,   274, -1260,    -8, -1260,
   -1260, -1260, -1260, -1260, -1260,   741, -1260,   746,   778, -1260,
   -1260,   755,   762,   767, -1260, -1260,  6104,   773, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,    54,  5522,
   -1260,   442,   461,  6104,  7782,  7815,   779,   539, -1260,  7907,
   10164, 10164,   760,  2802,   764,   136,   809,   810,   768,   812,
     813, -1260,   172,     4,   539,  1518,  8006,  1518,  8037, -1260,
     175,  8136, -1260,  6104, 10285,  6104, -1260,  8228, -1260,   178,
    6104, -1260, -1260, -1260,   182, -1260, -1260, -1260,  6104,   529,
   -1260,   814,  6104, -1260,   188, -1260, -1260,   455,   955,  8261,
   -1260,   815,   -19,   935,   156,  6104,   946,    -8, -1260, -1260,
     549,   776,   780, -1260, -1260,  6104,   794, -1260, -1260, -1260,
   -1260,   781,   783,   658,   782,  6104,  6104,  6104,   785,   786,
     787,   789,  5716, -1260, -1260,   189,  6104,  6104,   465, -1260,
   -1260, -1260,   796,   163, -1260,   179,  6104,  6104,  6104, -1260,
   -1260,   821, -1260, -1260,   124, -1260,  5910, -1260, -1260,   491,
     798, -1260,   477, -1260, -1260, -1260,  1518,  8353,  8452, -1260,
   -1260,  8483, -1260,   790, -1260, 10164,   491,   491, -1260, -1260,
     792, -1260,  2412,   823, -1260, -1260,  1518,    37,   836, -1260,
    6104,  9841, -1260, -1260,   946,   108,   786,   786,  8582,   797,
     800,   802,   803,  6104, -1260, -1260,  6104,  1045,  1045,  1045,
    6104, -1260,   786,   403, -1260,  8674, -1260,   834,  9923,  6104,
     258, -1260,  6104,  6104,   793,  8707, 10164, 10164,   804, -1260,
    6104, 10253, -1260, -1260, -1260,   478, -1260, -1260, -1260, -1260,
   -1260, -1260,  6104, -1260, -1260, -1260, -1260, -1260, 10164, -1260,
      37,  6104, -1260,  9957, -1260, 10900, -1260, -1260,   -59,   -59,
   -1260,  6104,   786,   786,   403,   816,   817,   660,   -59,   683,
     818, -1260,   966,   849,   822,  9923, -1260,   258, 10164, 10164,
   -1260,   165, -1260, 10285, -1260, -1260, -1260,  8799,  6104, 10039,
   -1260,   853, 10900,   403,   683,   843, -1260,   825,   826,  8898,
     -59,   -59,   828, -1260, -1260,   829,   830, -1260,  6104, -1260,
   -1260,   824, -1260,  6104,  6104, -1260,   491, 10128, -1260, -1260,
     491,   193,   832, -1260, -1260, -1260, -1260,   837,   838, -1260,
   -1260, -1260, 10164, -1260, 10164, 10164,   455, -1260, -1260, -1260,
     403, -1260, -1260, -1260,   197, -1260
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,   114,     1,   267,     0,     0,     0,   541,   268,     0,
     541,   541,   541,    16,     0,     0,    15,     3,     0,     0,
       9,     8,     0,     7,   529,     6,    11,     5,     4,    13,
      12,    14,    86,    87,    85,    94,    96,    38,    51,    48,
      49,    40,     0,    46,    39,   543,   542,     0,     0,    22,
      21,   529,     0,     0,     0,     0,   243,    36,   101,   102,
       0,     0,     0,   103,   105,   112,     0,   100,    17,    10,
     562,   561,   207,   547,   563,   530,   531,     0,     0,     0,
       0,    41,     0,    47,     0,     0,    44,   544,   546,    18,
     707,   699,   703,   245,     0,     0,   111,   106,     0,     0,
       0,     0,     0,     0,   115,   209,   208,   211,   206,   549,
     548,     0,   565,   564,   568,   533,   532,   534,    92,    93,
      90,    91,    89,     0,     0,    88,    97,    52,    50,    46,
      43,    42,     0,    19,    20,    23,     0,     0,     0,   244,
      34,    37,   110,     0,   107,   108,   109,   113,     0,   550,
     551,   558,   467,    24,    25,    29,     0,    81,    82,    79,
      80,    78,    77,    83,     0,    45,     0,   708,   700,   704,
      35,   104,     0,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   205,     0,     0,
     121,   116,     0,     0,     0,   559,     0,   569,     0,   468,
       0,    26,    27,    28,     0,    95,     0,     0,     0,     0,
       0,     0,   576,   596,   577,   612,   578,   582,   583,   584,
     585,   602,   589,   590,   591,   592,   593,   594,   595,   597,
     598,   599,   600,   669,   581,   588,   601,   676,   683,   579,
     586,   580,   587,     0,     0,     0,     0,   611,   633,   636,
     634,   635,   696,   631,   545,   617,   495,   501,   175,   176,
     173,   124,   125,   127,   126,   128,   129,   130,   131,   157,
     158,   155,   156,   148,   159,   160,   149,   146,   147,   174,
     168,     0,   172,   161,   162,   163,   164,   135,   136,   137,
     132,   133,   134,   145,     0,   151,   152,   150,   143,   144,
     139,   138,   140,   141,   142,   123,   122,   167,     0,   153,
     154,   467,   119,   234,   212,   603,   606,   609,   610,   604,
     607,   605,   608,   552,   553,   556,   566,    98,     0,   521,
     514,   535,    84,   637,   660,   663,     0,   666,   656,     0,
     620,   670,   677,   684,   690,   693,     0,     0,   646,   651,
     645,     0,   659,   655,   648,     0,     0,   650,   632,     0,
     618,   778,   701,   705,   177,   178,   171,   166,   179,   169,
     165,     0,   117,   266,   489,     0,     0,   210,     0,   538,
       0,   560,   480,   570,    99,     0,     0,   515,     0,     0,
       0,     0,     0,     0,   373,   374,     0,     0,     0,     0,
     367,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     602,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   456,   312,   314,   313,   315,   316,   317,   318,
      30,     0,     0,     0,     0,     0,     0,     0,     0,   297,
     298,   371,   370,   449,   368,   442,   441,   440,   439,   114,
     445,   369,   444,   443,   414,   375,   415,     0,   376,     0,
     372,   711,   715,   712,   713,   714,     0,     0,     0,     0,
       0,   116,     0,   116,     0,   116,     0,     0,     0,   642,
     237,   653,   654,   647,   649,     0,   652,   628,     0,     0,
     698,   697,   779,   709,   243,   624,   623,     0,   496,   491,
       0,     0,     0,   502,     0,   180,   170,     0,   264,   265,
       0,   467,   118,   120,   236,     0,    61,    62,     0,   258,
     256,     0,     0,     0,     0,   257,     0,     0,     0,     0,
       0,   213,   216,     0,     0,     0,     0,   229,   224,   221,
     220,   222,     0,   235,     0,    68,    69,    66,    67,   230,
     270,   219,     0,    65,   536,   539,   778,   557,   481,   522,
       0,   512,   513,   511,     0,     0,     0,     0,   625,   734,
     737,   248,     0,   251,   255,     0,   286,     0,     0,     0,
     763,     0,     0,     0,     0,   277,   280,     0,   283,     0,
     767,     0,   743,   749,     0,   740,     0,   403,   404,     0,
       0,     0,     0,     0,     0,     0,     0,   747,   770,   778,
     380,   379,   416,   378,   377,     0,     0,   778,   292,   778,
     299,     0,   307,   234,   298,   114,   215,     0,     0,     0,
       0,   405,   406,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     357,     0,   640,     0,     0,     0,   613,   615,     0,     0,
     119,     0,   119,     0,   119,   493,     0,   499,     0,   614,
       0,     0,   778,   644,   627,   630,     0,   619,     0,     0,
     497,   702,     0,   503,   706,     0,     0,   571,   487,   506,
     490,     0,     0,     0,   259,     0,     0,   246,     0,     0,
     233,     0,     0,    55,    73,     0,   261,     0,   231,   232,
       0,     0,   223,   218,   225,   226,   227,   228,   269,     0,
     217,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     779,   554,   567,     0,   467,   526,     0,     0,   638,   661,
     664,     0,     0,     0,     0,   732,   237,     0,     0,     0,
     753,   756,   759,     0,   778,     0,   778,     0,     0,     0,
       0,     0,     0,     0,   778,     0,     0,   778,     0,     0,
       0,     0,     0,     0,     0,     0,    33,     0,    31,     0,
       0,   779,     0,     0,     0,   779,     0,     0,     0,     0,
     345,   342,   344,     0,     0,   243,     0,   360,     0,   725,
       0,     0,   116,     0,   299,   307,     0,     0,   428,   427,
       0,     0,   429,   433,   381,   382,   394,   395,   392,   393,
       0,   422,     0,   412,     0,   446,   447,   448,   383,   384,
     399,   400,   401,   402,     0,     0,   397,   398,   396,   390,
     391,   386,   385,   387,   388,   389,     0,     0,     0,   351,
       0,     0,     0,     0,     0,   365,     0,   667,   657,     0,
     621,   671,     0,   678,     0,   685,     0,   691,     0,   694,
       0,     0,   241,   641,   238,   629,   710,   492,   498,   488,
       0,     0,   505,     0,   504,     0,   507,     0,     0,     0,
     260,     0,   247,     0,     0,    53,    54,   262,   234,     0,
       0,     0,   516,     0,   276,   514,   275,   329,   330,   332,
     331,   333,   323,   324,   325,   334,   335,   321,   322,   336,
     337,   326,   327,   328,   320,   537,   540,     0,   474,   477,
       0,     0,   528,   639,   662,   665,   626,     0,     0,     0,
     733,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   417,     0,     0,   418,     0,   450,
       0,     0,     0,     0,     0,     0,   307,   451,   452,   453,
     454,   455,     0,     0,     0,   746,   771,   772,     0,     0,
     293,   752,   407,     0,     0,     0,   116,     0,   361,     0,
       0,     0,     0,     0,     0,   364,     0,   362,     0,   119,
     311,   116,   424,     0,   430,     0,     0,     0,   410,     0,
       0,   434,   438,     0,     0,   413,     0,     0,     0,     0,
     352,     0,   358,     0,   408,     0,   366,   668,   658,   616,
     622,   672,   674,   679,   681,   686,   688,   692,   494,   695,
     500,     0,     0,     0,   573,   574,   508,   510,     0,     0,
     263,     0,    76,     0,     0,     0,     0,     0,   271,     0,
       0,     0,     0,   555,   475,   476,   477,   478,   469,   482,
     527,   735,   738,   249,   253,     0,   252,     0,     0,   289,
     287,     0,     0,     0,   764,   762,   239,     0,   773,   278,
     281,   284,   768,   766,   744,   750,   748,   741,     0,     0,
      32,     0,     0,     0,     0,     0,     0,   119,   363,     0,
     717,   716,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   305,     0,     0,   119,     0,     0,     0,     0,   340,
       0,     0,   435,     0,   423,     0,   411,     0,   353,     0,
       0,   409,   359,   355,     0,   675,   682,   689,   237,   242,
     572,     0,     0,    74,     0,    75,   214,    58,    63,     0,
     518,     0,   514,   519,     0,     0,   472,   469,   470,   471,
     474,     0,     0,   250,   254,     0,     0,   288,   754,   757,
     760,     0,     0,   778,     0,     0,     0,     0,     0,   732,
       0,     0,     0,   421,   457,     0,     0,     0,     0,   343,
     466,   346,     0,     0,   338,     0,     0,     0,     0,   302,
     303,     0,   301,   300,     0,   308,     0,   294,   309,     0,
       0,   465,     0,   463,   341,   460,     0,     0,     0,   459,
     354,     0,   356,     0,   575,   509,     0,     0,    56,    57,
       0,    70,     0,     0,   517,   272,     0,     0,     0,   523,
       0,     0,   473,   483,   472,     0,   732,   732,     0,     0,
       0,     0,     0,   237,   774,   240,   239,   279,   282,   285,
       0,   745,   732,     0,   419,     0,   458,   776,   776,     0,
       0,   349,     0,     0,     0,     0,   719,   718,     0,   306,
       0,   295,   310,   425,   431,     0,   464,   462,   461,   643,
      72,    59,     0,    64,    68,    69,    66,    67,    65,    71,
       0,     0,   520,     0,   525,     0,   485,   479,   731,   731,
     290,     0,   732,   732,     0,     0,     0,   778,   731,   724,
       0,   420,     0,     0,     0,   776,   347,     0,   721,   720,
     339,     0,   304,   296,   426,   432,   436,     0,     0,     0,
     524,     0,     0,     0,   728,   778,   730,     0,     0,     0,
     731,   731,     0,   765,   775,     0,     0,   742,     0,   782,
     780,     0,   350,     0,     0,   437,     0,     0,   274,   484,
       0,     0,   779,   729,   736,   739,   291,     0,     0,   761,
     769,   751,   777,   781,   723,   722,    58,   273,   486,   726,
       0,   755,   758,    60,     0,   727
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1260, -1260, -1260, -1260, -1260, -1260,   418,   950, -1260, -1260,
   -1260,  1037, -1260, -1260, -1260,   997, -1260,   911, -1260, -1260,
     961, -1260, -1260, -1260,  -384, -1260, -1260,  -226, -1260, -1260,
   -1260, -1260, -1260, -1260,   839, -1260, -1260,   -63,   949, -1260,
   -1260, -1260,   329, -1260,  -429,  -463,  -661, -1260, -1260, -1260,
   -1151, -1260, -1260,  -523, -1260, -1260,  -621,  -770,  -247, -1260,
     -14, -1260, -1260, -1260, -1260, -1260,  -222,  -221,  -220,  -218,
   -1260, -1260,  1054, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,  -408, -1260,
     593,  -188, -1260,  -801, -1260, -1260, -1260, -1260, -1260, -1260,
   -1259, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
     471, -1260, -1260, -1260, -1260, -1260, -1260, -1260,  -147,  -140,
    -224,  -139,   -34, -1260, -1260, -1260, -1260, -1260,   572, -1260,
    -472, -1260, -1260,  -476, -1260, -1260,  -706,  -216,  -553,  -911,
   -1260, -1260, -1260, -1260,  1012, -1260, -1260, -1260,   311, -1260,
     679, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,  -580,
    -164, -1260,   669, -1260, -1260, -1260, -1260, -1260, -1260,  -369,
   -1260, -1260,  -229, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260,  -153, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260,   673,  -622,  -282,  -786, -1260, -1260,
   -1065, -1138, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260,  -804, -1260, -1260, -1260, -1260, -1260, -1260, -1260, -1260,
   -1260, -1260, -1260, -1260, -1260, -1260,  -406, -1260, -1244,  -556,
   -1260
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    16,   135,    51,    17,   156,   162,   615,   451,
     141,   452,    95,    19,    20,    43,    44,    86,    21,    39,
      40,   543,   544,  1270,  1271,   545,  1273,   546,   547,   548,
     549,   550,   551,   552,   163,   164,    35,    36,    37,   209,
      63,    64,    65,    66,    22,   322,   387,   201,    23,   107,
     202,   108,   148,   324,   453,   553,   388,   691,  1212,   901,
     454,   554,   582,   774,  1203,   455,   555,   556,   557,   558,
     559,   520,   560,   739,  1089,   934,   561,   456,   788,  1215,
     789,  1216,   791,  1217,   457,   779,  1207,   458,   692,  1248,
     459,  1151,  1152,   832,   460,   636,   461,   562,   462,   463,
     822,   464,  1015,  1310,  1016,  1367,   465,   882,  1172,   466,
     628,  1155,  1374,  1157,  1375,  1256,  1405,   468,   383,  1200,
    1283,  1096,  1098,   960,   568,   764,  1345,  1382,   384,   385,
     507,   686,   372,   512,   688,   373,  1022,   708,   574,   398,
     935,   340,   936,   341,    76,   117,    25,   152,   565,   566,
      47,    48,   132,    26,   111,   150,   204,    27,   389,   957,
     391,   206,   207,    74,   114,   393,    28,   151,   336,   709,
     469,   333,   259,   260,   678,   371,   261,   479,  1060,   508,
     577,   368,   262,   263,   399,   963,   690,   477,  1058,   400,
     964,   401,   965,   476,  1057,   480,  1061,   481,  1175,   482,
    1063,   483,  1176,   484,  1065,   485,  1177,   486,  1067,   487,
    1069,   509,    29,   137,   266,   510,    30,   138,   267,   514,
      31,   136,   265,   698,   470,  1384,  1360,   830,  1385,  1386,
    1387,   971,   471,   772,  1201,   773,  1202,   798,  1221,   795,
    1219,   618,   472,   796,  1220,   473,   976,  1290,   977,  1291,
     978,  1292,   783,  1211,   793,  1218,  1213,   474,  1363,   503,
     475
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      57,    67,   258,   917,   513,   210,   972,  1006,   715,   829,
     761,   687,   833,   264,   685,   126,   724,   766,   680,   892,
     682,   894,   684,   896,  1092,  -114,     2,   587,   118,   119,
     632,   491,    82,     3,  1031,   619,   809,  1019,  1198,   627,
     629,  1004,   649,   650,  1008,   813,    67,    67,    67,   912,
      58,  1366,  1079,   381,    49,  1133,     4,   931,     5,    45,
       6,   381,   518,   812,  1364,    46,     7,    83,   100,   101,
     102,   816,   932,   817,   912,   493,     8,    94,    68,    59,
     647,  1301,     9,   649,   650,   633,    67,    67,    67,    67,
      38,   824,   402,   403,   824,    75,    32,    33,  1199,    50,
    1017,   913,   914,  1017,   915,   519,    10,   916,  1402,   382,
      55,   208,   409,   157,   158,   933,   670,   671,   411,   208,
     699,  1401,   449,   634,   588,   589,   323,   914,    11,    12,
     369,   571,   919,   824,    56,   142,   904,   572,   700,   763,
     501,  1246,  1017,   703,  1018,   337,  1383,    60,  1348,  1349,
      70,    71,   257,    72,    55,   418,   419,   670,   671,   489,
     592,   599,  1276,   571,  1358,   397,   912,   570,   120,   572,
     637,   638,  1021,   121,   323,   122,   123,   765,    56,   490,
     593,    73,   492,   784,   573,  1105,  1104,   786,   762,   421,
     422,   912,    69,   794,  1381,  1128,   797,  1021,    13,   913,
      84,   829,   913,    94,  1023,   258,   835,   590,   258,   771,
     913,    61,    85,   913,  1390,  1391,   573,    14,   124,   914,
      15,    62,   258,   828,   323,    55,   494,   591,   980,    15,
     983,  1410,   258,   523,  1009,   258,   258,   258,   991,    34,
      41,   994,   357,   600,   914,   495,   575,   576,   578,    56,
    1146,   496,   100,   159,   102,   706,  1147,   441,   160,  1222,
     161,   123,   912,   601,   521,   886,   641,   642,  1237,    42,
     707,    78,   581,    78,   647,   394,   648,   649,   650,   651,
     652,  1277,   571,   214,  1388,    55,  1148,   852,   572,   912,
     447,   912,    79,  1396,   818,  1312,   853,  1403,   522,   819,
      77,   171,    87,  1149,   103,   637,   638,  1083,  1150,    56,
      80,   215,   258,   258,   603,   914,   258,   898,   258,   900,
     258,  1196,   258,   674,   675,  1417,  1418,   679,  1279,   681,
     104,   683,   820,  1024,   604,   573,   665,   666,   667,   668,
     669,  1097,   914,    88,   914,  1055,  1191,   637,   638,   829,
    1244,   670,   671,  1055,  1026,   257,  1055,  1026,   257,   777,
    1055,  1025,    94,   785,   211,   212,  1055,  1055,  1153,  1029,
    1182,  1026,   257,  1056,  1192,  1026,   567,   799,  1245,   778,
    1142,  1254,   257,  1314,  1260,   257,   257,   257,  1262,    96,
      97,    98,   257,   585,  1266,  1306,    55,    90,  1080,  1429,
      91,   641,   642,  1435,   105,   818,   637,   638,  1263,   647,
     106,   648,   649,   650,   651,   652,   982,    99,   627,   505,
      56,   505,  1307,   889,  1070,   506,  1068,   506,   258,   144,
     145,   146,   147,   897,   127,  -673,  -680,   890,  1044,  1238,
    -673,  -680,   258,   641,   642,  -687,    85,  1045,    92,    93,
    -687,   647,  1049,  -348,   649,   650,   651,   652,  -348,  -673,
    -680,  1050,   257,   257,   129,   130,   257,  1267,   257,  -687,
     257,   325,   257,   839,   843,   326,  1232,  -348,  1268,  1269,
     369,   131,   511,   139,   768,   505,   670,   671,   857,   327,
     328,   506,   505,  1249,   329,   330,   331,   332,   506,   899,
     639,   640,   641,   642,   643,   140,   883,   644,   645,   646,
     647,   505,   648,   649,   650,   651,   652,   506,   653,   654,
     149,  1235,   166,  1355,  1144,   981,   258,  1313,   670,   671,
      83,   167,  1072,  1145,   505,   258,   505,   902,   258,  1195,
     506,   505,   506,   967,   968,   168,   907,   506,  1071,   908,
    1107,   369,   358,  1137,   979,  1183,   824,   109,  1185,   985,
     986,  1187,   988,   110,   990,  1017,   992,   993,  1154,   995,
     663,   664,   665,   666,   667,   668,   669,   369,   257,   359,
     360,   769,   169,   395,   112,   170,   396,   670,   671,   397,
     113,  1090,   257,   115,  1091,   133,  1131,   397,   369,   116,
    1132,   134,   887,   100,  1108,  1223,  1094,   258,   258,   258,
     258,   203,  1095,   205,  1226,   258,   821,   959,   966,   258,
    1055,   969,   208,   319,   258,   258,   975,   258,  1160,   258,
    1247,   258,   258,  1227,   258,   320,   321,  1309,   343,  1055,
    1169,  1280,   361,  1055,   369,  1174,   362,   335,   888,   369,
     369,   369,   369,   891,   893,   895,  1103,  1295,   369,   369,
     369,   323,  1110,  1324,  1376,  1404,   153,   154,   806,   807,
     339,  1184,   100,   101,   102,   334,   257,   153,   154,   155,
     338,  1359,   211,   212,   213,   257,   344,   346,   257,    52,
      53,    54,   349,   356,   345,   347,   637,   638,   377,   363,
    1042,   348,   350,   364,   370,   351,   365,   352,   353,   354,
     355,   369,   374,   375,   376,   378,   379,   392,   380,   478,
     386,  1225,   366,   502,  1341,  1228,  1322,   390,   367,   499,
     515,   516,  1359,   517,   258,   627,   564,   569,   579,   594,
     580,   586,   597,  1330,  1331,   595,   596,  1111,  1112,  1113,
     598,   258,   602,  1117,   605,   606,   630,   257,   257,   257,
     257,  1411,  1076,   599,   609,   257,   677,   610,   611,   257,
     676,   612,   511,   697,   257,   257,   613,   257,   614,   257,
      15,   257,   257,   631,   257,   673,  1136,  1378,   693,   701,
     702,   704,   641,   642,   711,   712,   718,   719,   720,   721,
     647,  1395,   648,   649,   650,   651,   652,   727,  1434,   728,
     729,   730,   258,   258,   258,   731,   738,   467,   258,   732,
     733,   734,   735,   736,   737,   740,   770,   488,   775,  1413,
     776,   780,   814,   759,   760,   831,   811,   498,   815,   834,
     781,   906,   782,   850,   911,   903,   910,   921,   637,   638,
     924,   258,   926,   925,   928,   958,   970,   973,  1007,   563,
     996,  1026,  1194,  1011,   667,   668,   669,   930,  1052,  1012,
    1013,  1014,  1020,  1032,  1027,  1034,  1059,   670,   671,  1074,
    1075,  1028,  1033,  1426,   257,  1035,  1062,  1428,  1064,  1066,
    1036,  1037,  1357,  1087,  1047,   607,   608,  1048,  1088,  1053,
    1093,   257,  1097,  1086,  1099,  1101,  1102,  1106,   258,  1114,
    1115,  1116,   617,   620,   621,   622,   623,   624,  1118,  1179,
    1119,  1120,  1123,  1121,  1122,  1124,  1250,   258,  1125,  1127,
    1126,  1129,  1162,  1138,  1181,  1163,  1165,  1190,  1170,  1193,
    1206,  1208,   639,   640,   641,   642,   643,  1204,  1209,   644,
    1178,  1205,   647,  1210,   648,   649,   650,   651,   652,  1214,
     653,   654,   257,   257,   257,  1231,  1234,  1272,   257,  1236,
     696,  1239,  1240,  1241,  1242,  1243,  1264,  1275,  1278,  1282,
    1289,  1286,  1311,  1318,  1323,  1287,  1293,  1296,   705,  1294,
    1300,   258,  1302,   258,  1303,  1339,  1329,  1332,  1342,  1370,
     714,   257,  1351,   717,  1252,  1352,  1362,  1353,  1354,   723,
    1372,   726,   663,   664,   665,   666,   667,   668,   669,  1398,
    1399,  1412,  1393,  1394,  1397,  1409,  1400,   125,  1423,   670,
     671,  1414,  1415,   808,  1419,  1420,  1421,  1430,    18,    81,
     165,   128,  1433,  1431,  1432,   767,  1333,  1340,   143,  1356,
    1334,  1335,  1336,   342,  1337,    24,  1319,  1284,   257,   635,
    1346,  1285,  1197,    89,   617,   787,   637,   638,   790,  1347,
     792,   956,  1392,     0,     0,   583,     0,   257,     0,   584,
     800,   801,   802,   803,   804,   805,     0,     0,     0,     0,
       0,     0,   258,   710,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1325,     0,     0,     0,     0,     0,     0,
     844,   845,   258,     0,   846,   847,   848,   849,     0,   851,
       0,   854,   855,   856,   858,   859,   860,   861,   862,   863,
     865,   866,   867,   868,   869,   870,   871,   872,   873,   874,
     875,   257,   884,   257,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     639,   640,   641,   642,     0,     0,     0,     0,     0,     0,
     647,     0,   648,   649,   650,   651,   652,     0,   653,   654,
       0,     0,   918,     0,   920,     0,     0,   922,     0,   923,
       0,     0,     0,     0,     0,     0,   927,     0,     0,     0,
       0,   929,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   937,   938,   939,   940,   941,   942,   943,   944,
     945,   946,   947,   948,   949,   950,   951,   952,   953,   954,
     955,     0,   665,   666,   667,   668,   669,   961,     0,     0,
       0,     0,   257,     0,     0,     0,     0,   670,   671,   974,
       0,     0,     0,   216,     0,     0,     0,     0,     0,   217,
       0,     0,   257,     0,     0,   218,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   219,     0,     0,  1003,     0,
       0,  1005,   617,   220,     0,     0,  1010,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   821,     0,   221,     0,
       0,     0,     0,     0,   563,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   253,   254,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1046,     0,
       0,     0,  1051,   821,   216,     0,     0,     0,     0,     0,
     217,     0,     0,     0,     0,     0,   218,     0,     0,     0,
       0,     0,    55,     0,     0,     0,   219,     0,     0,     0,
       0,  1073,     0,     0,   220,   255,     0,     0,  1077,     0,
       0,     0,     0,     0,     0,  1082,    56,     0,     0,   221,
       0,  1085,     0,     0,     0,     0,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,   252,   253,   254,     0,
       0,   256,     0,   500,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    55,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1134,  1135,   255,     0,     0,     0,
       0,  1139,  1140,  1141,     0,  1010,     0,   504,     0,     0,
       0,     0,     0,     0,  1156,     0,  1158,   505,  1161,     0,
       0,     0,     0,   506,  1164,     0,     0,     0,  1167,     0,
       0,     0,     0,     0,     0,     0,  1010,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   216,     0,     0,
       0,     0,   256,   217,     0,     0,     0,     0,     0,   218,
       0,     0,     0,     0,   563,     0,     0,  1189,     0,   219,
       0,     0,     0,     0,     0,     0,     0,   220,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   221,     0,     0,     0,     0,   617,     0,   222,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1257,     0,  1258,     0,     0,     0,
       0,  1261,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1265,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1281,     0,     0,   255,
       0,     0,     0,     0,     0,     0,  1288,     0,     0,     0,
      56,     0,     0,     0,     0,     0,  1297,  1298,  1299,     0,
       0,     0,     0,  1305,     0,     0,     0,   617,  1308,     0,
       0,     0,     0,     0,     0,     0,     0,  1315,  1316,  1317,
       0,     0,     0,     0,     0,     0,     0,  1321,     0,     0,
       0,     0,     0,     0,     0,   256,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1338,     0,     0,     0,     0,     0,     0,
       0,  1343,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   617,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1365,     0,     0,  1368,  1369,     0,     0,     0,     0,     0,
       0,  1373,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1377,     0,     0,     0,     0,     0,   524,
       0,     0,  1379,   402,   403,     3,     0,   525,   526,   527,
       0,   528,  1389,   404,   405,   406,   407,   408,     0,     0,
       0,     0,     0,   409,   529,   410,   530,   531,     0,   411,
       0,     0,     0,     0,     0,     0,   532,   412,     0,  1407,
     533,     0,   534,   413,     0,     0,   414,     0,     8,   415,
     535,     0,   536,   416,     0,     0,   537,   538,     0,  1422,
       0,     0,     0,   539,  1424,  1425,   418,   419,     0,   222,
     223,   224,     0,   226,   227,   228,   229,   230,   420,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   244,   245,   246,     0,     0,   249,   250,   251,   252,
     421,   422,   423,   540,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   424,   425,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,   426,   427,   428,   429,   430,     0,
     431,     0,   432,   433,   434,   435,   436,   437,   438,   439,
      56,   440,     0,     0,     0,     0,     0,     0,   441,   541,
     542,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,     0,    14,
       0,     0,   445,   446,     0,     0,     0,     0,     0,     0,
       0,   447,     0,   448,   524,   449,   450,     0,   402,   403,
       3,     0,   525,   526,   527,     0,   528,     0,   404,   405,
     406,   407,   408,     0,     0,     0,     0,     0,   409,   529,
     410,   530,   531,     0,   411,     0,     0,     0,     0,     0,
       0,   532,   412,     0,     0,   533,     0,   534,   413,     0,
       0,   414,     0,     8,   415,   535,     0,   536,   416,     0,
       0,   537,   538,     0,     0,     0,     0,     0,   539,     0,
       0,   418,   419,     0,   222,   223,   224,     0,   226,   227,
     228,   229,   230,   420,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,     0,   244,   245,   246,     0,
       0,   249,   250,   251,   252,   421,   422,   423,   540,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     424,   425,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    55,     0,     0,     0,     0,     0,     0,     0,   426,
     427,   428,   429,   430,     0,   431,     0,   432,   433,   434,
     435,   436,   437,   438,   439,    56,   440,     0,     0,     0,
       0,     0,     0,   441,  1030,   542,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     442,   443,   444,     0,    14,     0,     0,   445,   446,     0,
       0,     0,     0,     0,     0,     0,   447,     0,   448,   524,
     449,   450,     0,   402,   403,     3,     0,   525,   526,   527,
       0,   528,     0,   404,   405,   406,   407,   408,     0,     0,
       0,     0,     0,   409,   529,   410,   530,   531,     0,   411,
       0,     0,     0,     0,     0,     0,   532,   412,     0,     0,
     533,     0,   534,   413,     0,     0,   414,     0,     8,   415,
     535,     0,   536,   416,     0,     0,   537,   538,     0,     0,
       0,     0,     0,   539,     0,     0,   418,   419,     0,   222,
     223,   224,     0,   226,   227,   228,   229,   230,   420,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   244,   245,   246,     0,     0,   249,   250,   251,   252,
     421,   422,   423,   540,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   424,   425,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,   426,   427,   428,   429,   430,     0,
     431,     0,   432,   433,   434,   435,   436,   437,   438,   439,
      56,   440,     0,     0,     0,     0,     0,     0,   441,  1186,
     542,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,     0,    14,
       0,     0,   445,   446,     0,     0,     0,   402,   403,     0,
       0,   447,     0,   448,     0,   449,   450,   404,   405,   406,
     407,   408,     0,     0,     0,     0,     0,   409,   529,   410,
     530,     0,     0,   411,     0,     0,     0,     0,     0,     0,
       0,   412,     0,     0,     0,     0,     0,   413,     0,     0,
     414,     0,     0,   415,   535,     0,     0,   416,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   417,     0,     0,
     418,   419,     0,   222,   223,   224,     0,   226,   227,   228,
     229,   230,   420,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   244,   245,   246,     0,     0,
     249,   250,   251,   252,   421,   422,   423,   540,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   424,
     425,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      55,     0,     0,     0,     0,     0,     0,     0,   426,   427,
     428,   429,   430,     0,   431,     0,   432,   433,   434,   435,
     436,   437,   438,   439,    56,   440,     0,     0,     0,     0,
       0,     0,   441,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   442,
     443,   444,     0,    14,     0,     0,   445,   446,     0,     0,
       0,   402,   403,     0,     0,   447,     0,   448,     0,   449,
     450,   404,   405,   406,   407,   408,     0,     0,     0,     0,
       0,   409,     0,   410,     0,     0,     0,   411,     0,     0,
       0,     0,     0,     0,     0,   412,     0,     0,     0,     0,
       0,   413,     0,     0,   414,     0,     0,   415,     0,     0,
       0,   416,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   417,     0,     0,   418,   419,   823,   222,   223,   224,
       0,   226,   227,   228,   229,   230,   420,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,     0,   244,
     245,   246,     0,     0,   249,   250,   251,   252,   421,   422,
     423,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   424,   425,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,   426,   427,   428,   429,   430,     0,   431,   824,
     432,   433,   434,   435,   436,   437,   438,   439,   825,   440,
       0,     0,     0,     0,     0,     0,   441,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   442,   443,   444,     0,    14,     0,     0,
     445,   446,     0,     0,     0,     0,     0,   402,   403,   826,
       0,   448,   827,   449,   450,   625,     0,   404,   405,   406,
     407,   408,     0,     0,     0,     0,     0,   409,     0,   410,
       0,     0,     0,   411,     0,     0,     0,     0,     0,     0,
       0,   412,     0,     0,     0,     0,     0,   413,     0,     0,
     414,   626,     0,   415,     0,     0,     0,   416,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   417,     0,     0,
     418,   419,     0,   222,   223,   224,     0,   226,   227,   228,
     229,   230,   420,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   244,   245,   246,     0,     0,
     249,   250,   251,   252,   421,   422,   423,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   424,
     425,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      55,     0,     0,     0,     0,     0,     0,     0,   426,   427,
     428,   429,   430,     0,   431,   824,   432,   433,   434,   435,
     436,   437,   438,   439,   825,   440,     0,     0,     0,     0,
       0,     0,   441,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   442,
     443,   444,     0,    14,     0,     0,   445,   446,     0,     0,
       0,     0,     0,   402,   403,   447,     0,   448,     0,   449,
     450,   625,     0,   404,   405,   406,   407,   408,     0,     0,
       0,     0,     0,   409,     0,   410,     0,     0,     0,   411,
       0,     0,     0,     0,     0,     0,     0,   412,     0,     0,
       0,     0,     0,   413,     0,     0,   414,   626,     0,   415,
       0,     0,     0,   416,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   417,     0,     0,   418,   419,     0,   222,
     223,   224,     0,   226,   227,   228,   229,   230,   420,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   244,   245,   246,     0,     0,   249,   250,   251,   252,
     421,   422,   423,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   424,   425,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,   426,   427,   428,   429,   430,     0,
     431,     0,   432,   433,   434,   435,   436,   437,   438,   439,
      56,   440,     0,     0,     0,     0,     0,     0,   441,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,     0,    14,
       0,     0,   445,   446,     0,     0,     0,   402,   403,     0,
       0,   447,     0,   448,     0,   449,   450,   404,   405,   406,
     407,   408,     0,     0,     0,     0,     0,   409,     0,   410,
       0,     0,     0,   411,     0,     0,     0,     0,     0,     0,
       0,   412,     0,     0,     0,     0,     0,   413,     0,     0,
     414,     0,     0,   415,     0,     0,     0,   416,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   417,     0,     0,
     418,   419,     0,   222,   223,   224,     0,   226,   227,   228,
     229,   230,   420,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   244,   245,   246,     0,     0,
     249,   250,   251,   252,   421,   422,   423,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   424,
     425,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      55,     0,     0,     0,     0,     0,     0,     0,   426,   427,
     428,   429,   430,     0,   431,     0,   432,   433,   434,   435,
     436,   437,   438,   439,    56,   440,     0,     0,     0,     0,
       0,     0,   441,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   442,
     443,   444,     0,    14,     0,     0,   445,   446,     0,     0,
       0,     0,     0,   402,   403,   447,   497,   448,     0,   449,
     450,   616,     0,   404,   405,   406,   407,   408,     0,     0,
       0,     0,     0,   409,     0,   410,     0,     0,     0,   411,
       0,     0,     0,     0,     0,     0,     0,   412,     0,     0,
       0,     0,     0,   413,     0,     0,   414,     0,     0,   415,
       0,     0,     0,   416,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   417,     0,     0,   418,   419,     0,   222,
     223,   224,     0,   226,   227,   228,   229,   230,   420,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   244,   245,   246,     0,     0,   249,   250,   251,   252,
     421,   422,   423,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   424,   425,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,   426,   427,   428,   429,   430,     0,
     431,     0,   432,   433,   434,   435,   436,   437,   438,   439,
      56,   440,     0,     0,     0,     0,     0,     0,   441,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,     0,    14,
       0,     0,   445,   446,     0,     0,     0,   402,   403,     0,
       0,   447,     0,   448,     0,   449,   450,   404,   405,   406,
     407,   408,     0,     0,     0,     0,     0,   409,     0,   410,
       0,     0,     0,   411,     0,     0,     0,     0,     0,     0,
       0,   412,     0,     0,     0,     0,     0,   413,     0,     0,
     414,     0,     0,   415,     0,     0,     0,   416,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   417,     0,     0,
     418,   419,     0,   222,   223,   224,     0,   226,   227,   228,
     229,   230,   420,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   244,   245,   246,     0,     0,
     249,   250,   251,   252,   421,   422,   423,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   424,
     425,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      55,     0,     0,     0,     0,     0,     0,     0,   426,   427,
     428,   429,   430,     0,   431,     0,   432,   433,   434,   435,
     436,   437,   438,   439,    56,   440,     0,     0,     0,     0,
       0,     0,   441,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   442,
     443,   444,     0,    14,     0,     0,   445,   446,     0,     0,
       0,   402,   403,     0,     0,   447,   695,   448,     0,   449,
     450,   404,   405,   406,   407,   408,     0,     0,     0,     0,
       0,   409,     0,   410,     0,     0,     0,   411,     0,     0,
       0,     0,     0,     0,     0,   412,     0,     0,     0,     0,
       0,   413,     0,     0,   414,     0,     0,   415,     0,     0,
       0,   416,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   417,     0,     0,   418,   419,     0,   222,   223,   224,
       0,   226,   227,   228,   229,   230,   420,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,     0,   244,
     245,   246,     0,     0,   249,   250,   251,   252,   421,   422,
     423,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   424,   425,     0,     0,     0,     0,     0,
       0,     0,   713,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,   426,   427,   428,   429,   430,     0,   431,     0,
     432,   433,   434,   435,   436,   437,   438,   439,    56,   440,
       0,     0,     0,     0,     0,     0,   441,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   442,   443,   444,     0,    14,     0,     0,
     445,   446,     0,     0,     0,   402,   403,     0,     0,   447,
       0,   448,     0,   449,   450,   404,   405,   406,   407,   408,
       0,     0,     0,     0,     0,   409,     0,   410,     0,     0,
       0,   411,     0,     0,     0,     0,     0,     0,     0,   412,
       0,     0,     0,     0,     0,   413,     0,     0,   414,     0,
       0,   415,     0,     0,     0,   416,     0,     0,     0,     0,
       0,   716,     0,     0,     0,   417,     0,     0,   418,   419,
       0,   222,   223,   224,     0,   226,   227,   228,   229,   230,
     420,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,     0,   244,   245,   246,     0,     0,   249,   250,
     251,   252,   421,   422,   423,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   424,   425,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    55,     0,
       0,     0,     0,     0,     0,     0,   426,   427,   428,   429,
     430,     0,   431,     0,   432,   433,   434,   435,   436,   437,
     438,   439,    56,   440,     0,     0,     0,     0,     0,     0,
     441,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   442,   443,   444,
       0,    14,     0,     0,   445,   446,     0,     0,     0,   402,
     403,     0,     0,   447,     0,   448,     0,   449,   450,   404,
     405,   406,   407,   408,     0,     0,     0,     0,     0,   409,
       0,   410,     0,     0,     0,   411,     0,     0,     0,     0,
       0,     0,     0,   412,     0,     0,     0,     0,     0,   413,
       0,     0,   414,     0,     0,   415,     0,     0,     0,   416,
       0,     0,   722,     0,     0,     0,     0,     0,     0,   417,
       0,     0,   418,   419,     0,   222,   223,   224,     0,   226,
     227,   228,   229,   230,   420,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,     0,   244,   245,   246,
       0,     0,   249,   250,   251,   252,   421,   422,   423,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   424,   425,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    55,     0,     0,     0,     0,     0,     0,     0,
     426,   427,   428,   429,   430,     0,   431,     0,   432,   433,
     434,   435,   436,   437,   438,   439,    56,   440,     0,     0,
       0,     0,     0,     0,   441,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   442,   443,   444,     0,    14,     0,     0,   445,   446,
       0,     0,     0,   402,   403,     0,     0,   447,     0,   448,
       0,   449,   450,   404,   405,   406,   407,   408,     0,     0,
       0,     0,     0,   409,     0,   410,     0,     0,     0,   411,
       0,     0,     0,     0,     0,     0,     0,   412,     0,     0,
       0,     0,     0,   413,     0,     0,   414,     0,     0,   415,
       0,     0,     0,   416,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   417,     0,     0,   418,   419,     0,   222,
     223,   224,     0,   226,   227,   228,   229,   230,   420,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   244,   245,   246,     0,     0,   249,   250,   251,   252,
     421,   422,   423,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   424,   425,     0,     0,     0,
       0,     0,     0,     0,   725,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,   426,   427,   428,   429,   430,     0,
     431,     0,   432,   433,   434,   435,   436,   437,   438,   439,
      56,   440,     0,     0,     0,     0,     0,     0,   441,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,     0,    14,
       0,     0,   445,   446,     0,     0,     0,   402,   403,     0,
       0,   447,     0,   448,     0,   449,   450,   404,   405,   406,
     407,   408,     0,     0,   864,     0,     0,   409,     0,   410,
       0,     0,     0,   411,     0,     0,     0,     0,     0,     0,
       0,   412,     0,     0,     0,     0,     0,   413,     0,     0,
     414,     0,     0,   415,     0,     0,     0,   416,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   417,     0,     0,
     418,   419,     0,   222,   223,   224,     0,   226,   227,   228,
     229,   230,   420,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   244,   245,   246,     0,     0,
     249,   250,   251,   252,   421,   422,   423,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   424,
     425,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      55,     0,     0,     0,     0,     0,     0,     0,   426,   427,
     428,   429,   430,     0,   431,     0,   432,   433,   434,   435,
     436,   437,   438,   439,    56,   440,     0,     0,     0,     0,
       0,     0,   441,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   442,
     443,   444,     0,    14,     0,     0,   445,   446,     0,     0,
       0,   402,   403,     0,     0,   447,     0,   448,     0,   449,
     450,   404,   405,   406,   407,   408,     0,     0,     0,     0,
       0,   409,     0,   410,     0,     0,     0,   411,     0,     0,
       0,     0,     0,     0,     0,   412,     0,     0,     0,     0,
       0,   413,     0,     0,   414,     0,     0,   415,     0,     0,
       0,   416,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   417,     0,     0,   418,   419,     0,   222,   223,   224,
       0,   226,   227,   228,   229,   230,   420,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,     0,   244,
     245,   246,     0,     0,   249,   250,   251,   252,   421,   422,
     423,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   424,   425,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,   426,   427,   428,   429,   430,     0,   431,     0,
     432,   433,   434,   435,   436,   437,   438,   439,    56,   440,
       0,     0,     0,     0,     0,     0,   441,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   442,   443,   444,     0,    14,     0,     0,
     445,   446,     0,     0,     0,   402,   403,     0,     0,   447,
       0,   448,   885,   449,   450,   404,   405,   406,   407,   408,
       0,     0,     0,     0,     0,   409,     0,   410,     0,     0,
       0,   411,     0,     0,     0,     0,     0,     0,     0,   412,
       0,     0,     0,     0,     0,   413,     0,     0,   414,     0,
       0,   415,     0,     0,     0,   416,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   417,     0,     0,   418,   419,
       0,   222,   223,   224,     0,   226,   227,   228,   229,   230,
     420,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,     0,   244,   245,   246,     0,     0,   249,   250,
     251,   252,   421,   422,   423,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   424,   425,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    55,     0,
       0,     0,     0,     0,     0,     0,   426,   427,   428,   429,
     430,     0,   431,     0,   432,   433,   434,   435,   436,   437,
     438,   439,    56,   440,     0,     0,     0,     0,     0,     0,
     441,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   442,   443,   444,
       0,    14,     0,     0,   445,   446,     0,     0,     0,   402,
     403,     0,     0,   447,     0,   448,  1159,   449,   450,   404,
     405,   406,   407,   408,     0,     0,     0,     0,     0,   409,
       0,   410,     0,     0,     0,   411,     0,     0,     0,     0,
       0,     0,     0,   412,     0,     0,     0,     0,     0,   413,
       0,     0,   414,     0,     0,   415,     0,     0,     0,   416,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   417,
       0,     0,   418,   419,     0,   222,   223,   224,     0,   226,
     227,   228,   229,   230,   420,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,     0,   244,   245,   246,
       0,     0,   249,   250,   251,   252,   421,   422,   423,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   424,   425,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    55,     0,     0,     0,     0,     0,     0,     0,
     426,   427,   428,   429,   430,     0,   431,     0,   432,   433,
     434,   435,   436,   437,   438,   439,    56,   440,     0,     0,
       0,     0,     0,     0,   441,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   442,   443,   444,     0,    14,     0,     0,   445,   446,
       0,     0,     0,   402,   403,     0,     0,   447,     0,   448,
    1168,   449,   450,   404,   405,   406,   407,   408,     0,     0,
       0,     0,     0,   409,     0,   410,     0,     0,     0,   411,
       0,     0,     0,     0,     0,     0,     0,   412,     0,     0,
       0,     0,     0,   413,     0,     0,   414,     0,     0,   415,
       0,     0,     0,   416,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   417,     0,     0,   418,   419,     0,   222,
     223,   224,     0,   226,   227,   228,   229,   230,   420,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   244,   245,   246,     0,     0,   249,   250,   251,   252,
     421,   422,   423,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   424,   425,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,   426,   427,   428,   429,   430,     0,
     431,     0,   432,   433,   434,   435,   436,   437,   438,   439,
      56,   440,     0,     0,     0,     0,     0,     0,   441,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   442,   443,   444,     0,    14,
       0,     0,   445,   446,     0,     0,     0,   402,   403,     0,
       0,   447,     0,   448,  1173,   449,   450,   404,   405,   406,
     407,   408,     0,     0,     0,     0,     0,   409,     0,   410,
       0,     0,     0,   411,     0,     0,     0,     0,     0,     0,
       0,   412,     0,     0,     0,     0,     0,   413,     0,     0,
     414,     0,     0,   415,     0,     0,     0,   416,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   417,     0,     0,
     418,   419,     0,   222,   223,   224,     0,   226,   227,   228,
     229,   230,   420,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   244,   245,   246,     0,     0,
     249,   250,   251,   252,   421,   422,   423,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   424,
     425,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      55,     0,     0,     0,     0,     0,     0,     0,   426,   427,
     428,   429,   430,     0,   431,     0,   432,   433,   434,   435,
     436,   437,   438,   439,    56,   440,     0,     0,     0,     0,
       0,     0,   441,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   442,
     443,   444,     0,    14,     0,     0,   445,   446,     0,     0,
       0,   402,   403,     0,     0,   447,     0,   448,  1224,   449,
     450,   404,   405,   406,   407,   408,     0,     0,     0,     0,
       0,   409,     0,   410,     0,     0,     0,   411,     0,     0,
       0,     0,     0,     0,     0,   412,     0,     0,     0,     0,
       0,   413,     0,     0,   414,     0,     0,   415,     0,     0,
       0,   416,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   417,     0,     0,   418,   419,     0,   222,   223,   224,
       0,   226,   227,   228,   229,   230,   420,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,     0,   244,
     245,   246,     0,     0,   249,   250,   251,   252,   421,   422,
     423,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   424,   425,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,   426,   427,   428,   429,   430,     0,   431,     0,
     432,   433,   434,   435,   436,   437,   438,   439,    56,   440,
       0,     0,     0,     0,     0,     0,   441,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   442,   443,   444,     0,    14,     0,     0,
     445,   446,     0,     0,     0,   402,   403,     0,     0,   447,
       0,   448,  1304,   449,   450,   404,   405,   406,   407,   408,
       0,     0,     0,     0,     0,   409,     0,   410,     0,     0,
       0,   411,     0,     0,     0,     0,     0,     0,     0,   412,
       0,     0,     0,     0,     0,   413,     0,     0,   414,     0,
       0,   415,     0,     0,     0,   416,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   417,     0,     0,   418,   419,
       0,   222,   223,   224,     0,   226,   227,   228,   229,   230,
     420,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,     0,   244,   245,   246,     0,     0,   249,   250,
     251,   252,   421,   422,   423,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   424,   425,     0,
       0,     0,     0,     0,     0,     0,  1320,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    55,     0,
       0,     0,     0,     0,     0,     0,   426,   427,   428,   429,
     430,     0,   431,     0,   432,   433,   434,   435,   436,   437,
     438,   439,    56,   440,     0,     0,     0,     0,     0,     0,
     441,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   442,   443,   444,
       0,    14,     0,     0,   445,   446,     0,     0,     0,   402,
     403,     0,     0,   447,     0,   448,     0,   449,   450,   404,
     405,   406,   407,   408,     0,     0,     0,     0,     0,   409,
       0,   410,     0,     0,     0,   411,     0,     0,     0,     0,
       0,     0,     0,   412,     0,     0,     0,     0,     0,   413,
       0,     0,   414,     0,     0,   415,     0,     0,     0,   416,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   417,
       0,     0,   418,   419,     0,   222,   223,   224,     0,   226,
     227,   228,   229,   230,   420,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,     0,   244,   245,   246,
       0,     0,   249,   250,   251,   252,   421,   422,   423,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   424,   425,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    55,     0,     0,     0,     0,     0,     0,     0,
     426,   427,   428,   429,   430,     0,   431,     0,   432,   433,
     434,   435,   436,   437,   438,   439,    56,   440,     0,     0,
       0,     0,     0,     0,   441,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   442,   443,   444,     0,    14,     0,     0,   445,   446,
       0,     0,     0,   402,   403,     0,     0,   447,     0,   448,
       0,   449,   450,   404,   405,   406,   407,   408,     0,     0,
       0,     0,     0,   409,     0,   410,     0,     0,     0,   411,
       0,     0,     0,     0,     0,     0,     0,   412,     0,     0,
       0,     0,     0,   413,     0,     0,   414,     0,     0,   415,
       0,     0,     0,   416,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   417,     0,     0,   418,   419,     0,   222,
     223,   224,     0,   226,   227,   228,   229,   230,   420,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   244,   245,   246,     0,     0,   249,   250,   251,   252,
     421,   422,   423,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   424,   425,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,   426,   427,   428,   429,   430,     0,
     431,     0,   432,   433,   434,   435,   436,   437,   438,   439,
      56,   440,   637,   638,     0,     0,   216,     0,   441,     0,
       0,     0,   217,     0,     0,     0,     0,     0,   218,     0,
       0,     0,     0,     0,     0,   442,   443,   444,   219,    14,
       0,     0,   445,   446,     0,     0,   220,     0,   637,   638,
       0,  1143,     0,   448,     0,   449,   450,     0,     0,     0,
       0,   221,     0,     0,     0,     0,     0,     0,   222,   223,
     224,   225,   226,   227,   228,   229,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,   251,   252,   253,
     254,     0,     0,     0,     0,     0,   639,   640,   641,   642,
     643,     0,     0,   644,   645,   646,   647,     0,   648,   649,
     650,   651,   652,     0,   653,   654,     0,     0,     0,     0,
     655,   656,   657,     0,     0,    55,   658,     0,     0,     0,
     637,   638,   639,   640,   641,   642,   643,     0,   255,   644,
     645,   646,   647,     0,   648,   649,   650,   651,   652,   504,
     653,   654,     0,     0,     0,     0,   655,   656,   657,     0,
       0,   659,   658,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   670,   671,     0,     0,   672,     0,     0,
       0,     0,     0,     0,   256,     0,     0,   659,     0,   660,
     661,   662,   663,   664,   665,   666,   667,   668,   669,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   670,
     671,     0,     0,   689,   639,   640,   641,   642,   643,   637,
     638,   644,   645,   646,   647,     0,   648,   649,   650,   651,
     652,     0,   653,   654,     0,     0,     0,     0,   655,   656,
     657,     0,     0,     0,   658,     0,     0,     0,     0,     0,
     637,   638,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   659,
       0,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   670,   671,     0,     0,   909,     0,     0,     0,     0,
       0,     0,     0,   639,   640,   641,   642,   643,     0,     0,
     644,   645,   646,   647,     0,   648,   649,   650,   651,   652,
       0,   653,   654,     0,     0,     0,     0,   655,   656,   657,
       0,     0,     0,   658,   639,   640,   641,   642,   643,   637,
     638,   644,   645,   646,   647,     0,   648,   649,   650,   651,
     652,     0,   653,   654,     0,     0,     0,     0,   655,   656,
     657,     0,     0,     0,   658,     0,     0,     0,   659,     0,
     660,   661,   662,   663,   664,   665,   666,   667,   668,   669,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     670,   671,     0,     0,   984,     0,     0,     0,     0,   659,
       0,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   670,   671,     0,     0,   987,     0,     0,     0,     0,
       0,   637,   638,   639,   640,   641,   642,   643,     0,     0,
     644,   645,   646,   647,     0,   648,   649,   650,   651,   652,
       0,   653,   654,     0,     0,     0,     0,   655,   656,   657,
       0,     0,     0,   658,   637,   638,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   659,     0,
     660,   661,   662,   663,   664,   665,   666,   667,   668,   669,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     670,   671,     0,     0,   989,   639,   640,   641,   642,   643,
       0,     0,   644,   645,   646,   647,     0,   648,   649,   650,
     651,   652,     0,   653,   654,     0,     0,     0,     0,   655,
     656,   657,     0,     0,     0,   658,   637,   638,   639,   640,
     641,   642,   643,     0,     0,   644,   645,   646,   647,     0,
     648,   649,   650,   651,   652,     0,   653,   654,     0,     0,
       0,     0,   655,   656,   657,     0,     0,     0,   658,     0,
     659,     0,   660,   661,   662,   663,   664,   665,   666,   667,
     668,   669,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   670,   671,     0,     0,   997,     0,     0,     0,
       0,     0,     0,   659,     0,   660,   661,   662,   663,   664,
     665,   666,   667,   668,   669,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   670,   671,     0,     0,   998,
     639,   640,   641,   642,   643,   637,   638,   644,   645,   646,
     647,     0,   648,   649,   650,   651,   652,     0,   653,   654,
       0,     0,     0,     0,   655,   656,   657,     0,     0,     0,
     658,     0,     0,     0,     0,     0,   637,   638,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   659,     0,   660,   661,   662,
     663,   664,   665,   666,   667,   668,   669,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   670,   671,     0,
       0,   999,     0,     0,     0,     0,     0,     0,     0,   639,
     640,   641,   642,   643,     0,     0,   644,   645,   646,   647,
       0,   648,   649,   650,   651,   652,     0,   653,   654,     0,
       0,     0,     0,   655,   656,   657,     0,     0,     0,   658,
     639,   640,   641,   642,   643,   637,   638,   644,   645,   646,
     647,     0,   648,   649,   650,   651,   652,     0,   653,   654,
       0,     0,     0,     0,   655,   656,   657,     0,     0,     0,
     658,     0,     0,     0,   659,     0,   660,   661,   662,   663,
     664,   665,   666,   667,   668,   669,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   670,   671,     0,     0,
    1000,     0,     0,     0,     0,   659,     0,   660,   661,   662,
     663,   664,   665,   666,   667,   668,   669,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   670,   671,     0,
       0,  1001,     0,     0,     0,     0,     0,   637,   638,   639,
     640,   641,   642,   643,     0,     0,   644,   645,   646,   647,
       0,   648,   649,   650,   651,   652,     0,   653,   654,     0,
       0,     0,     0,   655,   656,   657,     0,     0,     0,   658,
     637,   638,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   659,     0,   660,   661,   662,   663,
     664,   665,   666,   667,   668,   669,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   670,   671,     0,     0,
    1002,   639,   640,   641,   642,   643,     0,     0,   644,   645,
     646,   647,     0,   648,   649,   650,   651,   652,     0,   653,
     654,     0,     0,     0,     0,   655,   656,   657,     0,     0,
       0,   658,   637,   638,   639,   640,   641,   642,   643,     0,
       0,   644,   645,   646,   647,     0,   648,   649,   650,   651,
     652,     0,   653,   654,     0,     0,     0,     0,   655,   656,
     657,     0,     0,     0,   658,     0,   659,     0,   660,   661,
     662,   663,   664,   665,   666,   667,   668,   669,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   670,   671,
       0,     0,  1078,     0,     0,     0,     0,     0,     0,   659,
       0,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   670,   671,     0,     0,  1081,   639,   640,   641,   642,
     643,   637,   638,   644,   645,   646,   647,     0,   648,   649,
     650,   651,   652,     0,   653,   654,     0,     0,     0,     0,
     655,   656,   657,     0,     0,     0,   658,     0,     0,     0,
       0,     0,   637,   638,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   659,     0,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   670,   671,     0,     0,  1084,     0,     0,
       0,     0,     0,     0,     0,   639,   640,   641,   642,   643,
       0,     0,   644,   645,   646,   647,     0,   648,   649,   650,
     651,   652,     0,   653,   654,     0,     0,     0,     0,   655,
     656,   657,     0,     0,     0,   658,   639,   640,   641,   642,
     643,   637,   638,   644,   645,   646,   647,     0,   648,   649,
     650,   651,   652,     0,   653,   654,     0,     0,     0,     0,
     655,   656,   657,     0,     0,     0,   658,     0,     0,     0,
     659,     0,   660,   661,   662,   663,   664,   665,   666,   667,
     668,   669,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   670,   671,     0,     0,  1109,     0,     0,     0,
       0,   659,     0,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   670,   671,     0,     0,  1180,     0,     0,
       0,     0,     0,   637,   638,   639,   640,   641,   642,   643,
       0,     0,   644,   645,   646,   647,     0,   648,   649,   650,
     651,   652,     0,   653,   654,     0,     0,     0,     0,   655,
     656,   657,     0,     0,     0,   658,   637,   638,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     659,     0,   660,   661,   662,   663,   664,   665,   666,   667,
     668,   669,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   670,   671,     0,     0,  1188,   639,   640,   641,
     642,   643,     0,     0,   644,   645,   646,   647,     0,   648,
     649,   650,   651,   652,     0,   653,   654,     0,     0,     0,
       0,   655,   656,   657,     0,     0,     0,   658,   637,   638,
     639,   640,   641,   642,   643,     0,     0,   644,   645,   646,
     647,     0,   648,   649,   650,   651,   652,     0,   653,   654,
       0,     0,     0,     0,   655,   656,   657,     0,     0,     0,
     658,     0,   659,     0,   660,   661,   662,   663,   664,   665,
     666,   667,   668,   669,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   670,   671,     0,     0,  1229,     0,
       0,     0,     0,     0,     0,   659,     0,   660,   661,   662,
     663,   664,   665,   666,   667,   668,   669,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   670,   671,     0,
       0,  1230,   639,   640,   641,   642,   643,   637,   638,   644,
     645,   646,   647,     0,   648,   649,   650,   651,   652,     0,
     653,   654,     0,     0,     0,     0,   655,   656,   657,     0,
       0,     0,   658,     0,     0,     0,     0,     0,   637,   638,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   659,     0,   660,
     661,   662,   663,   664,   665,   666,   667,   668,   669,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   670,
     671,     0,     0,  1233,     0,     0,     0,     0,     0,     0,
       0,   639,   640,   641,   642,   643,     0,     0,   644,   645,
     646,   647,     0,   648,   649,   650,   651,   652,     0,   653,
     654,     0,     0,     0,     0,   655,   656,   657,     0,     0,
       0,   658,   639,   640,   641,   642,   643,   637,   638,   644,
     645,   646,   647,     0,   648,   649,   650,   651,   652,     0,
     653,   654,     0,     0,     0,     0,   655,   656,   657,     0,
       0,     0,   658,     0,     0,     0,   659,     0,   660,   661,
     662,   663,   664,   665,   666,   667,   668,   669,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   670,   671,
       0,     0,  1251,     0,     0,     0,     0,   659,     0,   660,
     661,   662,   663,   664,   665,   666,   667,   668,   669,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   670,
     671,     0,     0,  1253,     0,     0,     0,     0,     0,   637,
     638,   639,   640,   641,   642,   643,     0,     0,   644,   645,
     646,   647,     0,   648,   649,   650,   651,   652,     0,   653,
     654,     0,     0,     0,     0,   655,   656,   657,     0,     0,
       0,   658,   637,   638,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   659,     0,   660,   661,
     662,   663,   664,   665,   666,   667,   668,   669,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   670,   671,
       0,     0,  1255,   639,   640,   641,   642,   643,     0,     0,
     644,   645,   646,   647,     0,   648,   649,   650,   651,   652,
       0,   653,   654,     0,     0,     0,     0,   655,   656,   657,
       0,     0,     0,   658,   637,   638,   639,   640,   641,   642,
     643,     0,     0,   644,   645,   646,   647,     0,   648,   649,
     650,   651,   652,     0,   653,   654,     0,     0,     0,     0,
     655,   656,   657,     0,     0,     0,   658,     0,   659,     0,
     660,   661,   662,   663,   664,   665,   666,   667,   668,   669,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     670,   671,     0,     0,  1259,     0,     0,     0,     0,     0,
       0,   659,     0,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   670,   671,     0,     0,  1274,   639,   640,
     641,   642,   643,   637,   638,   644,   645,   646,   647,     0,
     648,   649,   650,   651,   652,     0,   653,   654,     0,     0,
       0,     0,   655,   656,   657,     0,     0,     0,   658,     0,
       0,     0,     0,     0,   637,   638,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   659,     0,   660,   661,   662,   663,   664,
     665,   666,   667,   668,   669,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   670,   671,     0,     0,  1326,
       0,     0,     0,     0,     0,     0,     0,   639,   640,   641,
     642,   643,     0,     0,   644,   645,   646,   647,     0,   648,
     649,   650,   651,   652,     0,   653,   654,     0,     0,     0,
       0,   655,   656,   657,     0,     0,     0,   658,   639,   640,
     641,   642,   643,   637,   638,   644,   645,   646,   647,     0,
     648,   649,   650,   651,   652,     0,   653,   654,     0,     0,
       0,     0,   655,   656,   657,     0,     0,     0,   658,     0,
       0,     0,   659,     0,   660,   661,   662,   663,   664,   665,
     666,   667,   668,   669,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   670,   671,     0,     0,  1327,     0,
       0,     0,     0,   659,     0,   660,   661,   662,   663,   664,
     665,   666,   667,   668,   669,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   670,   671,     0,     0,  1328,
       0,     0,     0,     0,     0,   637,   638,   639,   640,   641,
     642,   643,     0,     0,   644,   645,   646,   647,     0,   648,
     649,   650,   651,   652,     0,   653,   654,     0,     0,     0,
       0,   655,   656,   657,     0,     0,     0,   658,   637,   638,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   659,     0,   660,   661,   662,   663,   664,   665,
     666,   667,   668,   669,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   670,   671,     0,     0,  1350,   639,
     640,   641,   642,   643,     0,     0,   644,   645,   646,   647,
       0,   648,   649,   650,   651,   652,     0,   653,   654,     0,
       0,     0,     0,   655,   656,   657,     0,     0,     0,   658,
     637,   638,   639,   640,   641,   642,   643,     0,     0,   644,
     645,   646,   647,     0,   648,   649,   650,   651,   652,     0,
     653,   654,     0,     0,     0,     0,   655,   656,   657,     0,
       0,     0,   658,     0,   659,     0,   660,   661,   662,   663,
     664,   665,   666,   667,   668,   669,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   670,   671,     0,     0,
    1361,     0,     0,     0,     0,     0,     0,   659,     0,   660,
     661,   662,   663,   664,   665,   666,   667,   668,   669,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   670,
     671,     0,     0,  1371,   639,   640,   641,   642,   643,   637,
     638,   644,   645,   646,   647,     0,   648,   649,   650,   651,
     652,     0,   653,   654,     0,     0,     0,     0,   655,   656,
     657,     0,     0,     0,   658,     0,     0,     0,     0,     0,
     637,   638,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   659,
       0,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   670,   671,     0,     0,  1406,     0,     0,     0,     0,
       0,     0,     0,   639,   640,   641,   642,   643,     0,     0,
     644,   645,   646,   647,     0,   648,   649,   650,   651,   652,
       0,   653,   654,     0,     0,     0,     0,   655,   656,   657,
     637,   638,     0,   658,   639,   640,   641,   642,   643,     0,
       0,   644,   645,   646,   647,     0,   648,   649,   650,   651,
     652,     0,   653,   654,     0,     0,     0,     0,   655,   656,
     657,   637,   638,     0,   658,     0,     0,     0,   659,     0,
     660,   661,   662,   663,   664,   665,   666,   667,   668,   669,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     670,   671,     0,     0,  1416,     0,     0,     0,     0,   659,
       0,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   670,   671,   694,   639,   640,   641,   642,   643,     0,
       0,   644,   645,   646,   647,     0,   648,   649,   650,   651,
     652,     0,   653,   654,     0,     0,     0,     0,   655,   656,
     657,   637,   638,     0,   658,   639,   640,   641,   642,   643,
       0,     0,   644,   645,   646,   647,     0,   648,   649,   650,
     651,   652,     0,   653,   654,     0,     0,     0,     0,   655,
     656,   657,   637,   638,     0,   658,     0,     0,     0,   659,
       0,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   670,   671,   905,     0,     0,     0,     0,     0,     0,
     659,     0,   660,   661,   662,   663,   664,   665,   666,   667,
     668,   669,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   670,   671,  1038,   639,   640,   641,   642,   643,
       0,     0,   644,   645,   646,   647,     0,   648,   649,   650,
     651,   652,     0,   653,   654,     0,     0,     0,     0,   655,
     656,   657,   637,   638,     0,   658,   639,   640,   641,   642,
     643,     0,     0,   644,   645,   646,   647,     0,   648,   649,
     650,   651,   652,     0,   653,   654,     0,     0,     0,     0,
     655,   656,   657,     0,     0,     0,   658,     0,     0,     0,
     659,     0,   660,   661,   662,   663,   664,   665,   666,   667,
     668,   669,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   670,   671,  1054,     0,     0,     0,     0,     0,
       0,   659,     0,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   670,   671,  1166,   639,   640,   641,   642,
     643,   637,   638,   644,   645,   646,   647,     0,   648,   649,
     650,   651,   652,     0,   653,   654,     0,     0,     0,     0,
     655,   656,   657,     0,     0,     0,   658,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   659,     0,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   670,   671,  1171,     0,   741,   742,   743,
     744,   745,   746,   747,   748,   639,   640,   641,   642,   643,
     749,   750,   644,   645,   646,   647,   751,   648,   649,   650,
     651,   652,   752,   653,   654,   753,   754,   268,   269,   655,
     656,   657,   755,   756,   757,   658,     0,     0,     0,     0,
       0,     0,     0,     0,   270,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   637,   638,
       0,     0,  -319,     0,     0,     0,     0,     0,     0,   758,
     659,     0,   660,   661,   662,   663,   664,   665,   666,   667,
     668,   669,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   670,   671,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,     0,     0,   289,   290,   291,     0,     0,   292,   293,
     294,   295,   296,     0,     0,   297,   298,   299,   300,   301,
     302,   303,   639,   640,   641,   642,   643,   637,   638,   644,
     645,   646,   647,     0,   648,   649,   650,   651,   652,     0,
     653,   654,     0,     0,   810,     0,   655,   656,   657,     0,
       0,     0,   658,     0,     0,     0,   304,     0,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,     0,     0,
     315,   316,     0,     0,     0,     0,     0,     0,   317,   318,
       0,     0,     0,     0,     0,     0,     0,   659,     0,   660,
     661,   662,   663,   664,   665,   666,   667,   668,   669,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   670,
     671,     0,     0,     0,     0,     0,   637,   638,     0,     0,
       0,   639,   640,   641,   642,   643,     0,     0,   644,   645,
     646,   647,     0,   648,   649,   650,   651,   652,     0,   653,
     654,     0,     0,     0,     0,   655,   656,   657,   637,   638,
       0,   658,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   962,     0,
       0,     0,     0,     0,     0,     0,   659,     0,   660,   661,
     662,   663,   664,   665,   666,   667,   668,   669,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   670,   671,
     639,   640,   641,   642,   643,     0,     0,   644,   645,   646,
     647,     0,   648,   649,   650,   651,   652,     0,   653,   654,
     637,   638,     0,     0,   655,   656,   657,     0,     0,     0,
     658,     0,   639,   640,   641,   642,   643,     0,     0,   644,
     645,   646,   647,     0,   648,   649,   650,   651,   652,     0,
     653,   654,   637,   638,     0,     0,   655,   656,   657,     0,
       0,     0,   658,     0,     0,   659,  1043,   660,   661,   662,
     663,   664,   665,   666,   667,   668,   669,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   670,   671,  1100,
       0,     0,     0,     0,     0,     0,     0,   659,     0,   660,
     661,   662,   663,   664,   665,   666,   667,   668,   669,     0,
       0,     0,     0,     0,   639,   640,   641,   642,   643,   670,
     671,   644,   645,   646,   647,     0,   648,   649,   650,   651,
     652,     0,   653,   654,   637,   638,     0,     0,   655,   656,
     657,     0,     0,     0,   658,     0,   639,   640,   641,   642,
     643,     0,     0,   644,   645,   646,   647,     0,   648,   649,
     650,   651,   652,     0,   653,   654,     0,  1130,   637,   638,
     655,   656,   657,     0,     0,     0,   658,     0,     0,   659,
       0,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   670,   671,  1344,     0,     0,     0,     0,     0,     0,
       0,   659,     0,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,     0,     0,     0,     0,     0,   639,   640,
     641,   642,   643,   670,   671,   644,   645,   646,   647,     0,
     648,   649,   650,   651,   652,     0,   653,   654,     0,     0,
     637,   638,   655,   656,   657,     0,     0,     0,   658,     0,
       0,     0,   639,   640,   641,   642,   643,     0,     0,   644,
     645,   646,   647,     0,   648,   649,   650,   651,   652,     0,
     653,   654,     0,     0,     0,  1362,   655,   656,   657,     0,
       0,     0,   658,   659,     0,   660,   661,   662,   663,   664,
     665,   666,   667,   668,   669,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   670,   671,     0,     0,  1380,
       0,     0,     0,     0,     0,     0,     0,   659,     0,   660,
     661,   662,   663,   664,   665,   666,   667,   668,   669,   637,
     638,     0,     0,     0,   639,   640,   641,   642,   643,   670,
     671,   644,   645,   646,   647,     0,   648,   649,   650,   651,
     652,     0,   653,   654,     0,     0,     0,     0,   655,   656,
     657,     0,     0,     0,   658,   637,   638,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1408,     0,     0,     0,     0,     0,     0,     0,   659,
       0,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   670,   671,   639,   640,   641,   642,   643,     0,     0,
     644,   645,   646,   647,     0,   648,   649,   650,   651,   652,
       0,   653,   654,     0,     0,     0,     0,   655,   656,   657,
       0,     0,     0,   658,   637,   638,     0,     0,     0,   639,
     640,   641,   642,   643,     0,     0,   644,   645,   646,   647,
       0,   648,   649,   650,   651,   652,     0,   653,   654,     0,
    1427,     0,     0,   655,   656,   657,   637,   638,   659,   658,
     660,   661,   662,   663,   664,   665,   666,   667,   668,   669,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     670,   671,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   659,     0,   660,   661,   662,   663,
     664,   665,   666,   667,   668,   669,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   670,   671,   639,   640,
     641,   642,   643,     0,     0,   644,   645,   646,   647,     0,
     648,   649,   650,   651,   652,     0,   653,   654,   637,   638,
       0,     0,   655,   656,   657,     0,     0,     0,  -688,     0,
     639,   640,   641,   642,   643,     0,     0,   644,   645,   646,
     647,     0,   648,   649,   650,   651,   652,     0,   653,   654,
     637,   638,     0,     0,   655,   656,   657,     0,     0,     0,
       0,     0,     0,   659,     0,   660,   661,   662,   663,   664,
     665,   666,   667,   668,   669,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   670,   671,     0,     0,     0,
       0,     0,     0,     0,     0,   659,     0,   660,   661,   662,
     663,   664,   665,   666,   667,   668,   669,     0,     0,     0,
       0,     0,   639,   640,   641,   642,   643,   670,   671,   644,
     645,   646,   647,     0,   648,   649,   650,   651,   652,     0,
     653,   654,   637,   638,     0,     0,   655,     0,   657,     0,
       0,     0,     0,     0,   639,   640,   641,   642,   643,     0,
       0,   644,   645,   646,   647,     0,   648,   649,   650,   651,
     652,     0,   653,   654,   637,   638,     0,     0,   655,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   660,
     661,   662,   663,   664,   665,   666,   667,   668,   669,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   670,
     671,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,     0,     0,     0,     0,     0,   639,   640,   641,   642,
     643,   670,   671,   644,   645,   646,   647,     0,   648,   649,
     650,   651,   652,     0,   653,   654,   637,   638,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   639,   640,
     641,   642,   643,     0,     0,   644,   645,   646,   647,     0,
     648,   649,   650,   651,   652,     0,   653,   654,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,     0,     0,     0,     0,     0,   836,     0,
       0,     0,     0,   670,   671,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   661,   662,   663,   664,
     665,   666,   667,   668,   669,     0,     0,     0,     0,     0,
     639,   640,   641,   642,   643,   670,   671,   644,   645,   646,
     647,     0,   648,   649,   650,   651,   652,     0,   653,   654,
     222,   223,   224,     0,   226,   227,   228,   229,   230,   420,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,     0,   244,   245,   246,     0,     0,   249,   250,   251,
     252,     0,     0,     0,     0,     0,   840,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   662,
     663,   664,   665,   666,   667,   668,   669,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   670,   671,  1039,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   837,     0,     0,     0,     0,     0,   222,   223,
     224,   838,   226,   227,   228,   229,   230,   420,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,     0,
     244,   245,   246,     0,     0,   249,   250,   251,   252,     0,
       0,   222,   223,   224,     0,   226,   227,   228,   229,   230,
     420,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,     0,   244,   245,   246,     0,     0,   249,   250,
     251,   252,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     841,     0,     0,     0,     0,     0,     0,     0,     0,   842,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1040,     0,   172,     0,     0,     0,   222,
     223,   224,  1041,   226,   227,   228,   229,   230,   420,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   244,   245,   246,     0,     0,   249,   250,   251,   252,
       0,   173,     0,   174,     0,   175,   176,   177,   178,   179,
       0,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,     0,   191,   192,   193,   876,   877,   194,   195,
     196,   197,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   198,   199,     0,
       0,   878,     0,     0,     0,     0,     0,     0,     0,     0,
     879,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   200,     0,     0,     0,     0,     0,     0,     0,
     880,   881
};

static const yytype_int16 yycheck[] =
{
      14,    15,   166,   709,   373,   152,   776,   811,   531,   631,
     566,   487,   633,   166,   486,    78,   539,   570,   481,   680,
     483,   682,   485,   684,   935,     8,     0,     5,    15,    16,
     459,    33,    22,     7,   835,   441,   616,   823,    46,   447,
     448,    20,   128,   129,    20,   625,    60,    61,    62,   126,
      34,  1310,    20,   150,   162,    20,    30,   147,    32,    57,
      34,   150,     7,   619,  1308,    63,    40,    57,   139,   140,
     141,   627,   162,   629,   126,    33,    50,   138,   172,    63,
     125,  1219,    56,   128,   129,   170,   100,   101,   102,   103,
     155,   153,     5,     6,   153,    62,    19,    20,   106,   207,
     162,   178,   179,   162,   181,    50,    80,   184,  1367,   206,
     138,   208,    25,    15,    16,   205,   202,   203,    31,   208,
     181,  1365,   207,   208,   102,   103,   170,   179,   102,   103,
     182,   126,   712,   153,   162,   206,   692,   132,   507,   568,
     369,   137,   162,   512,   206,   208,   205,   131,  1286,  1287,
       5,     6,   166,     8,   138,    68,    69,   202,   203,   185,
     185,   205,   181,   126,  1302,   184,   126,   396,   155,   132,
      21,    22,   132,   160,   170,   162,   163,   172,   162,   205,
     205,    36,   184,   591,   179,   971,   206,   593,   171,   102,
     103,   126,   172,   601,  1345,   996,   604,   132,   172,   178,
     190,   823,   178,   138,   826,   369,   635,   185,   372,   578,
     178,   195,   202,   178,  1352,  1353,   179,   191,   205,   179,
     203,   205,   386,   631,   170,   138,   184,   205,   784,   203,
     786,  1382,   396,   386,   814,   399,   400,   401,   794,   162,
     162,   797,   256,   185,   179,   203,   399,   400,   401,   162,
     126,   209,   139,   155,   141,   147,   132,   170,   160,   205,
     162,   163,   126,   205,   172,   673,   117,   118,   132,   191,
     162,   178,   185,   178,   125,   338,   127,   128,   129,   130,
     131,  1192,   126,   178,  1349,   138,   162,   153,   132,   126,
     203,   126,   181,  1358,   147,   132,   162,   132,   206,   152,
     179,   206,   162,   179,   178,    21,    22,   928,   184,   162,
     178,   206,   476,   477,   185,   179,   480,   686,   482,   688,
     484,    47,   486,   476,   477,  1390,  1391,   480,   172,   482,
     204,   484,   185,   178,   205,   179,   187,   188,   189,   190,
     191,    67,   179,   172,   179,   178,   178,    21,    22,   971,
     178,   202,   203,   178,   178,   369,   178,   178,   372,   185,
     178,   206,   138,   592,   164,   165,   178,   178,  1029,   832,
    1076,   178,   386,   206,   206,   178,   390,   606,   206,   205,
     204,   206,   396,   204,   206,   399,   400,   401,   206,    60,
      61,    62,   406,   407,   206,   206,   138,   162,   921,   206,
     162,   117,   118,   206,    57,   147,    21,    22,  1178,   125,
      63,   127,   128,   129,   130,   131,   785,   205,   826,   172,
     162,   172,  1226,   172,   900,   178,   898,   178,   592,   100,
     101,   102,   103,   186,   155,   181,   181,   186,   153,  1145,
     186,   186,   606,   117,   118,   181,   202,   162,   162,   162,
     186,   125,   153,   181,   128,   129,   130,   131,   186,   205,
     205,   162,   476,   477,   162,   162,   480,    12,   482,   205,
     484,    75,   486,   637,   638,    79,  1137,   205,    23,    24,
     182,   162,   162,   162,   186,   172,   202,   203,   652,    93,
      94,   178,   172,  1154,    98,    99,   100,   101,   178,   186,
     115,   116,   117,   118,   119,   164,   670,   122,   123,   124,
     125,   172,   127,   128,   129,   130,   131,   178,   133,   134,
     162,  1143,   179,  1293,   153,   186,   690,  1233,   202,   203,
      57,   170,   901,   162,   172,   699,   172,   690,   702,  1092,
     178,   172,   178,   772,   773,   170,   699,   178,   186,   702,
     186,   182,    33,  1016,   783,  1078,   153,    57,  1081,   788,
     789,  1084,   791,    63,   793,   162,   795,   796,  1031,   798,
     185,   186,   187,   188,   189,   190,   191,   182,   592,    60,
      61,   186,   170,   178,    57,   164,   181,   202,   203,   184,
      63,   178,   606,    57,   181,    57,  1004,   184,   182,    63,
    1008,    63,   186,   139,   973,  1128,    57,   771,   772,   773,
     774,   181,    63,   106,   172,   779,   630,   764,   771,   783,
     178,   774,   208,    35,   788,   789,   779,   791,  1036,   793,
    1153,   795,   796,   172,   798,    35,   205,   172,   185,   178,
    1048,  1194,   123,   178,   182,  1053,   127,   162,   186,   182,
     182,   182,   182,   186,   186,   186,   186,  1213,   182,   182,
     182,   170,   186,   186,   186,  1371,   164,   165,   166,   167,
     162,  1079,   139,   140,   141,   170,   690,   164,   165,   166,
     208,  1303,   164,   165,   166,   699,   185,   205,   702,    10,
      11,    12,   205,   205,   185,   185,    21,    22,   204,   180,
     864,   185,   185,   184,   162,   185,   187,   185,   185,   185,
     185,   182,   162,   162,   162,    22,   162,   170,   204,   162,
     181,  1129,   203,   178,  1277,  1133,  1249,   181,   209,   203,
     162,   132,  1354,   205,   898,  1143,   162,   162,   185,   205,
     185,   185,   205,  1266,  1267,   185,   185,   976,   977,   978,
     185,   915,   185,   982,   185,   185,   208,   771,   772,   773,
     774,  1383,   915,   205,   205,   779,   162,   205,   205,   783,
     206,   205,   162,   162,   788,   789,   205,   791,   205,   793,
     203,   795,   796,   205,   798,   205,  1015,  1340,   204,   171,
     181,   171,   117,   118,   205,   205,   205,   162,   172,   155,
     125,  1357,   127,   128,   129,   130,   131,    37,  1430,   172,
     172,   205,   976,   977,   978,    10,    66,   346,   982,   172,
     172,   172,   172,   172,   172,   172,   186,   356,   205,  1385,
     205,   185,    13,   179,   178,     4,   178,   366,   178,   208,
     185,   171,   185,   162,    43,   206,   205,    14,    21,    22,
     179,  1015,   155,   181,   170,     8,    70,   162,   171,   388,
     186,   178,  1091,   204,   189,   190,   191,   205,     1,   206,
     205,   205,   205,   185,   206,   185,   162,   202,   203,   162,
     162,   205,   205,  1406,   898,   205,   186,  1410,   186,   186,
     205,   205,  1300,    43,   205,   424,   425,   205,   162,   205,
     171,   915,    67,   205,   172,   186,   186,   206,  1072,   186,
     206,   205,   441,   442,   443,   444,   445,   446,   206,  1072,
     186,   186,   206,   186,   186,   186,  1155,  1091,   186,   186,
     206,   205,   185,   206,    43,   205,   205,   162,   205,   162,
     162,   186,   115,   116,   117,   118,   119,   206,   186,   122,
     205,   205,   125,   186,   127,   128,   129,   130,   131,   186,
     133,   134,   976,   977,   978,   186,   206,    12,   982,   205,
     499,   162,   162,   205,   162,   162,   162,   162,    43,    33,
     186,   205,   186,   162,   186,   205,   205,   205,   517,   206,
     205,  1155,   205,  1157,   205,   172,   206,   205,   162,   206,
     529,  1015,   205,   532,  1157,   205,   172,   205,   205,   538,
     206,   540,   185,   186,   187,   188,   189,   190,   191,    53,
     171,   178,   206,   206,   206,   172,   204,    77,   204,   202,
     203,   206,   206,   615,   206,   206,   206,   205,     1,    42,
     129,    80,  1426,   206,   206,   574,  1272,  1276,    99,  1296,
    1272,  1272,  1272,   214,  1272,     1,  1244,  1197,  1072,   466,
    1284,  1200,  1096,    51,   593,   594,    21,    22,   597,  1285,
     599,   760,  1354,    -1,    -1,   406,    -1,  1091,    -1,   406,
     609,   610,   611,   612,   613,   614,    -1,    -1,    -1,    -1,
      -1,    -1,  1256,   521,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1256,    -1,    -1,    -1,    -1,    -1,    -1,
     639,   640,  1276,    -1,   643,   644,   645,   646,    -1,   648,
      -1,   650,   651,   652,   653,   654,   655,   656,   657,   658,
     659,   660,   661,   662,   663,   664,   665,   666,   667,   668,
     669,  1155,   671,  1157,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,    -1,    -1,    -1,    -1,    -1,    -1,
     125,    -1,   127,   128,   129,   130,   131,    -1,   133,   134,
      -1,    -1,   711,    -1,   713,    -1,    -1,   716,    -1,   718,
      -1,    -1,    -1,    -1,    -1,    -1,   725,    -1,    -1,    -1,
      -1,   730,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   741,   742,   743,   744,   745,   746,   747,   748,
     749,   750,   751,   752,   753,   754,   755,   756,   757,   758,
     759,    -1,   187,   188,   189,   190,   191,   766,    -1,    -1,
      -1,    -1,  1256,    -1,    -1,    -1,    -1,   202,   203,   778,
      -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    -1,  1276,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,   807,    -1,
      -1,   810,   811,    49,    -1,    -1,   815,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1310,    -1,    64,    -1,
      -1,    -1,    -1,    -1,   833,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   877,    -1,
      -1,    -1,   881,  1367,    19,    -1,    -1,    -1,    -1,    -1,
      25,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,
      -1,    -1,   138,    -1,    -1,    -1,    41,    -1,    -1,    -1,
      -1,   910,    -1,    -1,    49,   151,    -1,    -1,   917,    -1,
      -1,    -1,    -1,    -1,    -1,   924,   162,    -1,    -1,    64,
      -1,   930,    -1,    -1,    -1,    -1,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,    -1,
      -1,   207,    -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1013,  1014,   151,    -1,    -1,    -1,
      -1,  1020,  1021,  1022,    -1,  1024,    -1,   162,    -1,    -1,
      -1,    -1,    -1,    -1,  1033,    -1,  1035,   172,  1037,    -1,
      -1,    -1,    -1,   178,  1043,    -1,    -1,    -1,  1047,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1055,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,
      -1,    -1,   207,    25,    -1,    -1,    -1,    -1,    -1,    31,
      -1,    -1,    -1,    -1,  1083,    -1,    -1,  1086,    -1,    41,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    64,    -1,    -1,    -1,    -1,  1116,    -1,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1163,    -1,  1165,    -1,    -1,    -1,
      -1,  1170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1182,    -1,    -1,   138,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1195,    -1,    -1,   151,
      -1,    -1,    -1,    -1,    -1,    -1,  1205,    -1,    -1,    -1,
     162,    -1,    -1,    -1,    -1,    -1,  1215,  1216,  1217,    -1,
      -1,    -1,    -1,  1222,    -1,    -1,    -1,  1226,  1227,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1236,  1237,  1238,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1246,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1272,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1280,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1296,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1309,    -1,    -1,  1312,  1313,    -1,    -1,    -1,    -1,    -1,
      -1,  1320,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1332,    -1,    -1,    -1,    -1,    -1,     1,
      -1,    -1,  1341,     5,     6,     7,    -1,     9,    10,    11,
      -1,    13,  1351,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    -1,  1378,
      42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,    51,
      52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,  1398,
      -1,    -1,    -1,    65,  1403,  1404,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   117,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   146,   147,   148,   149,   150,    -1,
     152,    -1,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,    -1,    -1,    -1,    -1,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   187,   188,   189,    -1,   191,
      -1,    -1,   194,   195,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   203,    -1,   205,     1,   207,   208,    -1,     5,     6,
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
     117,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,
     147,   148,   149,   150,    -1,   152,    -1,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,    -1,    -1,    -1,
      -1,    -1,    -1,   170,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     187,   188,   189,    -1,   191,    -1,    -1,   194,   195,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   203,    -1,   205,     1,
     207,   208,    -1,     5,     6,     7,    -1,     9,    10,    11,
      -1,    13,    -1,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    -1,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    -1,    -1,
      42,    -1,    44,    45,    -1,    -1,    48,    -1,    50,    51,
      52,    -1,    54,    55,    -1,    -1,    58,    59,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    71,
      72,    73,    -1,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   117,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   146,   147,   148,   149,   150,    -1,
     152,    -1,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,    -1,    -1,    -1,    -1,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   187,   188,   189,    -1,   191,
      -1,    -1,   194,   195,    -1,    -1,    -1,     5,     6,    -1,
      -1,   203,    -1,   205,    -1,   207,   208,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    51,    52,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
     118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,   147,
     148,   149,   150,    -1,   152,    -1,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,    -1,    -1,
      -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   187,
     188,   189,    -1,   191,    -1,    -1,   194,   195,    -1,    -1,
      -1,     5,     6,    -1,    -1,   203,    -1,   205,    -1,   207,
     208,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
      -1,    25,    -1,    27,    -1,    -1,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    48,    -1,    -1,    51,    -1,    -1,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    -1,    68,    69,    70,    71,    72,    73,
      -1,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    93,
      94,    95,    -1,    -1,    98,    99,   100,   101,   102,   103,
     104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   117,   118,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   146,   147,   148,   149,   150,    -1,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,    -1,    -1,    -1,    -1,   170,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   187,   188,   189,    -1,   191,    -1,    -1,
     194,   195,    -1,    -1,    -1,    -1,    -1,     5,     6,   203,
      -1,   205,   206,   207,   208,    13,    -1,    15,    16,    17,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    -1,    27,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    49,    -1,    51,    -1,    -1,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    71,    72,    73,    -1,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    93,    94,    95,    -1,    -1,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
     118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,   147,
     148,   149,   150,    -1,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,    -1,    -1,
      -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   187,
     188,   189,    -1,   191,    -1,    -1,   194,   195,    -1,    -1,
      -1,    -1,    -1,     5,     6,   203,    -1,   205,    -1,   207,
     208,    13,    -1,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   146,   147,   148,   149,   150,    -1,
     152,    -1,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,    -1,    -1,    -1,    -1,   170,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   187,   188,   189,    -1,   191,
      -1,    -1,   194,   195,    -1,    -1,    -1,     5,     6,    -1,
      -1,   203,    -1,   205,    -1,   207,   208,    15,    16,    17,
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
     138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,   147,
     148,   149,   150,    -1,   152,    -1,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,    -1,    -1,
      -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   187,
     188,   189,    -1,   191,    -1,    -1,   194,   195,    -1,    -1,
      -1,    -1,    -1,     5,     6,   203,   204,   205,    -1,   207,
     208,    13,    -1,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   146,   147,   148,   149,   150,    -1,
     152,    -1,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,    -1,    -1,    -1,    -1,   170,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   187,   188,   189,    -1,   191,
      -1,    -1,   194,   195,    -1,    -1,    -1,     5,     6,    -1,
      -1,   203,    -1,   205,    -1,   207,   208,    15,    16,    17,
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
     138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,   147,
     148,   149,   150,    -1,   152,    -1,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,    -1,    -1,
      -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   187,
     188,   189,    -1,   191,    -1,    -1,   194,   195,    -1,    -1,
      -1,     5,     6,    -1,    -1,   203,   204,   205,    -1,   207,
     208,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,   126,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   146,   147,   148,   149,   150,    -1,   152,    -1,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,    -1,    -1,    -1,    -1,   170,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   187,   188,   189,    -1,   191,    -1,    -1,
     194,   195,    -1,    -1,    -1,     5,     6,    -1,    -1,   203,
      -1,   205,    -1,   207,   208,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    -1,    27,    -1,    -1,
      -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    51,    -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,
      -1,    61,    -1,    -1,    -1,    65,    -1,    -1,    68,    69,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   146,   147,   148,   149,
     150,    -1,   152,    -1,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,    -1,    -1,    -1,    -1,    -1,    -1,
     170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   187,   188,   189,
      -1,   191,    -1,    -1,   194,   195,    -1,    -1,    -1,     5,
       6,    -1,    -1,   203,    -1,   205,    -1,   207,   208,    15,
      16,    17,    18,    19,    -1,    -1,    -1,    -1,    -1,    25,
      -1,    27,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    -1,    51,    -1,    -1,    -1,    55,
      -1,    -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    71,    72,    73,    -1,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    93,    94,    95,
      -1,    -1,    98,    99,   100,   101,   102,   103,   104,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   117,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     146,   147,   148,   149,   150,    -1,   152,    -1,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    -1,    -1,
      -1,    -1,    -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   187,   188,   189,    -1,   191,    -1,    -1,   194,   195,
      -1,    -1,    -1,     5,     6,    -1,    -1,   203,    -1,   205,
      -1,   207,   208,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   146,   147,   148,   149,   150,    -1,
     152,    -1,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,    -1,    -1,    -1,    -1,   170,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   187,   188,   189,    -1,   191,
      -1,    -1,   194,   195,    -1,    -1,    -1,     5,     6,    -1,
      -1,   203,    -1,   205,    -1,   207,   208,    15,    16,    17,
      18,    19,    -1,    -1,    22,    -1,    -1,    25,    -1,    27,
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
     138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,   147,
     148,   149,   150,    -1,   152,    -1,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,    -1,    -1,
      -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   187,
     188,   189,    -1,   191,    -1,    -1,   194,   195,    -1,    -1,
      -1,     5,     6,    -1,    -1,   203,    -1,   205,    -1,   207,
     208,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   146,   147,   148,   149,   150,    -1,   152,    -1,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,    -1,    -1,    -1,    -1,   170,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   187,   188,   189,    -1,   191,    -1,    -1,
     194,   195,    -1,    -1,    -1,     5,     6,    -1,    -1,   203,
      -1,   205,   206,   207,   208,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   146,   147,   148,   149,
     150,    -1,   152,    -1,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,    -1,    -1,    -1,    -1,    -1,    -1,
     170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   187,   188,   189,
      -1,   191,    -1,    -1,   194,   195,    -1,    -1,    -1,     5,
       6,    -1,    -1,   203,    -1,   205,   206,   207,   208,    15,
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
      -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     146,   147,   148,   149,   150,    -1,   152,    -1,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    -1,    -1,
      -1,    -1,    -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   187,   188,   189,    -1,   191,    -1,    -1,   194,   195,
      -1,    -1,    -1,     5,     6,    -1,    -1,   203,    -1,   205,
     206,   207,   208,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   146,   147,   148,   149,   150,    -1,
     152,    -1,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    -1,    -1,    -1,    -1,    -1,    -1,   170,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   187,   188,   189,    -1,   191,
      -1,    -1,   194,   195,    -1,    -1,    -1,     5,     6,    -1,
      -1,   203,    -1,   205,   206,   207,   208,    15,    16,    17,
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
     138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,   147,
     148,   149,   150,    -1,   152,    -1,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,    -1,    -1,    -1,    -1,
      -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   187,
     188,   189,    -1,   191,    -1,    -1,   194,   195,    -1,    -1,
      -1,     5,     6,    -1,    -1,   203,    -1,   205,   206,   207,
     208,    15,    16,    17,    18,    19,    -1,    -1,    -1,    -1,
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
      -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   146,   147,   148,   149,   150,    -1,   152,    -1,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
      -1,    -1,    -1,    -1,    -1,    -1,   170,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   187,   188,   189,    -1,   191,    -1,    -1,
     194,   195,    -1,    -1,    -1,     5,     6,    -1,    -1,   203,
      -1,   205,   206,   207,   208,    15,    16,    17,    18,    19,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   146,   147,   148,   149,
     150,    -1,   152,    -1,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,    -1,    -1,    -1,    -1,    -1,    -1,
     170,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   187,   188,   189,
      -1,   191,    -1,    -1,   194,   195,    -1,    -1,    -1,     5,
       6,    -1,    -1,   203,    -1,   205,    -1,   207,   208,    15,
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
      -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     146,   147,   148,   149,   150,    -1,   152,    -1,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,    -1,    -1,
      -1,    -1,    -1,    -1,   170,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   187,   188,   189,    -1,   191,    -1,    -1,   194,   195,
      -1,    -1,    -1,     5,     6,    -1,    -1,   203,    -1,   205,
      -1,   207,   208,    15,    16,    17,    18,    19,    -1,    -1,
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
      -1,    -1,    -1,    -1,    -1,    -1,   138,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   146,   147,   148,   149,   150,    -1,
     152,    -1,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,    21,    22,    -1,    -1,    19,    -1,   170,    -1,
      -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    -1,   187,   188,   189,    41,   191,
      -1,    -1,   194,   195,    -1,    -1,    49,    -1,    21,    22,
      -1,   203,    -1,   205,    -1,   207,   208,    -1,    -1,    -1,
      -1,    64,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,    -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,
     119,    -1,    -1,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,
     139,   140,   141,    -1,    -1,   138,   145,    -1,    -1,    -1,
      21,    22,   115,   116,   117,   118,   119,    -1,   151,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,   131,   162,
     133,   134,    -1,    -1,    -1,    -1,   139,   140,   141,    -1,
      -1,   180,   145,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,    -1,    -1,   206,    -1,    -1,
      -1,    -1,    -1,    -1,   207,    -1,    -1,   180,    -1,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,    -1,    -1,   206,   115,   116,   117,   118,   119,    21,
      22,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,
     141,    -1,    -1,    -1,   145,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   180,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,    -1,    -1,   206,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,   116,   117,   118,   119,    -1,    -1,
     122,   123,   124,   125,    -1,   127,   128,   129,   130,   131,
      -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,   141,
      -1,    -1,    -1,   145,   115,   116,   117,   118,   119,    21,
      22,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,
     141,    -1,    -1,    -1,   145,    -1,    -1,    -1,   180,    -1,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203,    -1,    -1,   206,    -1,    -1,    -1,    -1,   180,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,    -1,    -1,   206,    -1,    -1,    -1,    -1,
      -1,    21,    22,   115,   116,   117,   118,   119,    -1,    -1,
     122,   123,   124,   125,    -1,   127,   128,   129,   130,   131,
      -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,   141,
      -1,    -1,    -1,   145,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   180,    -1,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203,    -1,    -1,   206,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,
     140,   141,    -1,    -1,    -1,   145,    21,    22,   115,   116,
     117,   118,   119,    -1,    -1,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,   131,    -1,   133,   134,    -1,    -1,
      -1,    -1,   139,   140,   141,    -1,    -1,    -1,   145,    -1,
     180,    -1,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,    -1,    -1,   206,    -1,    -1,    -1,
      -1,    -1,    -1,   180,    -1,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,   203,    -1,    -1,   206,
     115,   116,   117,   118,   119,    21,    22,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,   131,    -1,   133,   134,
      -1,    -1,    -1,    -1,   139,   140,   141,    -1,    -1,    -1,
     145,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   180,    -1,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,    -1,
      -1,   206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,
     116,   117,   118,   119,    -1,    -1,   122,   123,   124,   125,
      -1,   127,   128,   129,   130,   131,    -1,   133,   134,    -1,
      -1,    -1,    -1,   139,   140,   141,    -1,    -1,    -1,   145,
     115,   116,   117,   118,   119,    21,    22,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,   131,    -1,   133,   134,
      -1,    -1,    -1,    -1,   139,   140,   141,    -1,    -1,    -1,
     145,    -1,    -1,    -1,   180,    -1,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,    -1,    -1,
     206,    -1,    -1,    -1,    -1,   180,    -1,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,    -1,
      -1,   206,    -1,    -1,    -1,    -1,    -1,    21,    22,   115,
     116,   117,   118,   119,    -1,    -1,   122,   123,   124,   125,
      -1,   127,   128,   129,   130,   131,    -1,   133,   134,    -1,
      -1,    -1,    -1,   139,   140,   141,    -1,    -1,    -1,   145,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   180,    -1,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,    -1,    -1,
     206,   115,   116,   117,   118,   119,    -1,    -1,   122,   123,
     124,   125,    -1,   127,   128,   129,   130,   131,    -1,   133,
     134,    -1,    -1,    -1,    -1,   139,   140,   141,    -1,    -1,
      -1,   145,    21,    22,   115,   116,   117,   118,   119,    -1,
      -1,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,
     141,    -1,    -1,    -1,   145,    -1,   180,    -1,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
      -1,    -1,   206,    -1,    -1,    -1,    -1,    -1,    -1,   180,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,    -1,    -1,   206,   115,   116,   117,   118,
     119,    21,    22,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,
     139,   140,   141,    -1,    -1,    -1,   145,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   180,    -1,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,    -1,    -1,   206,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,
     140,   141,    -1,    -1,    -1,   145,   115,   116,   117,   118,
     119,    21,    22,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,
     139,   140,   141,    -1,    -1,    -1,   145,    -1,    -1,    -1,
     180,    -1,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,    -1,    -1,   206,    -1,    -1,    -1,
      -1,   180,    -1,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,    -1,    -1,   206,    -1,    -1,
      -1,    -1,    -1,    21,    22,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,
     140,   141,    -1,    -1,    -1,   145,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     180,    -1,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,    -1,    -1,   206,   115,   116,   117,
     118,   119,    -1,    -1,   122,   123,   124,   125,    -1,   127,
     128,   129,   130,   131,    -1,   133,   134,    -1,    -1,    -1,
      -1,   139,   140,   141,    -1,    -1,    -1,   145,    21,    22,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,   131,    -1,   133,   134,
      -1,    -1,    -1,    -1,   139,   140,   141,    -1,    -1,    -1,
     145,    -1,   180,    -1,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   202,   203,    -1,    -1,   206,    -1,
      -1,    -1,    -1,    -1,    -1,   180,    -1,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,    -1,
      -1,   206,   115,   116,   117,   118,   119,    21,    22,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,   131,    -1,
     133,   134,    -1,    -1,    -1,    -1,   139,   140,   141,    -1,
      -1,    -1,   145,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   180,    -1,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,    -1,    -1,   206,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   115,   116,   117,   118,   119,    -1,    -1,   122,   123,
     124,   125,    -1,   127,   128,   129,   130,   131,    -1,   133,
     134,    -1,    -1,    -1,    -1,   139,   140,   141,    -1,    -1,
      -1,   145,   115,   116,   117,   118,   119,    21,    22,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,   131,    -1,
     133,   134,    -1,    -1,    -1,    -1,   139,   140,   141,    -1,
      -1,    -1,   145,    -1,    -1,    -1,   180,    -1,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
      -1,    -1,   206,    -1,    -1,    -1,    -1,   180,    -1,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,    -1,    -1,   206,    -1,    -1,    -1,    -1,    -1,    21,
      22,   115,   116,   117,   118,   119,    -1,    -1,   122,   123,
     124,   125,    -1,   127,   128,   129,   130,   131,    -1,   133,
     134,    -1,    -1,    -1,    -1,   139,   140,   141,    -1,    -1,
      -1,   145,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   180,    -1,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
      -1,    -1,   206,   115,   116,   117,   118,   119,    -1,    -1,
     122,   123,   124,   125,    -1,   127,   128,   129,   130,   131,
      -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,   141,
      -1,    -1,    -1,   145,    21,    22,   115,   116,   117,   118,
     119,    -1,    -1,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,
     139,   140,   141,    -1,    -1,    -1,   145,    -1,   180,    -1,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203,    -1,    -1,   206,    -1,    -1,    -1,    -1,    -1,
      -1,   180,    -1,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,    -1,    -1,   206,   115,   116,
     117,   118,   119,    21,    22,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,   131,    -1,   133,   134,    -1,    -1,
      -1,    -1,   139,   140,   141,    -1,    -1,    -1,   145,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   180,    -1,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,   203,    -1,    -1,   206,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,   116,   117,
     118,   119,    -1,    -1,   122,   123,   124,   125,    -1,   127,
     128,   129,   130,   131,    -1,   133,   134,    -1,    -1,    -1,
      -1,   139,   140,   141,    -1,    -1,    -1,   145,   115,   116,
     117,   118,   119,    21,    22,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,   131,    -1,   133,   134,    -1,    -1,
      -1,    -1,   139,   140,   141,    -1,    -1,    -1,   145,    -1,
      -1,    -1,   180,    -1,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   202,   203,    -1,    -1,   206,    -1,
      -1,    -1,    -1,   180,    -1,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,   203,    -1,    -1,   206,
      -1,    -1,    -1,    -1,    -1,    21,    22,   115,   116,   117,
     118,   119,    -1,    -1,   122,   123,   124,   125,    -1,   127,
     128,   129,   130,   131,    -1,   133,   134,    -1,    -1,    -1,
      -1,   139,   140,   141,    -1,    -1,    -1,   145,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   180,    -1,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   202,   203,    -1,    -1,   206,   115,
     116,   117,   118,   119,    -1,    -1,   122,   123,   124,   125,
      -1,   127,   128,   129,   130,   131,    -1,   133,   134,    -1,
      -1,    -1,    -1,   139,   140,   141,    -1,    -1,    -1,   145,
      21,    22,   115,   116,   117,   118,   119,    -1,    -1,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,   131,    -1,
     133,   134,    -1,    -1,    -1,    -1,   139,   140,   141,    -1,
      -1,    -1,   145,    -1,   180,    -1,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,    -1,    -1,
     206,    -1,    -1,    -1,    -1,    -1,    -1,   180,    -1,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,    -1,    -1,   206,   115,   116,   117,   118,   119,    21,
      22,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,
     141,    -1,    -1,    -1,   145,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   180,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,    -1,    -1,   206,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,   116,   117,   118,   119,    -1,    -1,
     122,   123,   124,   125,    -1,   127,   128,   129,   130,   131,
      -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,   141,
      21,    22,    -1,   145,   115,   116,   117,   118,   119,    -1,
      -1,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,
     141,    21,    22,    -1,   145,    -1,    -1,    -1,   180,    -1,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203,    -1,    -1,   206,    -1,    -1,    -1,    -1,   180,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,   115,   116,   117,   118,   119,    -1,
      -1,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,
     141,    21,    22,    -1,   145,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,
     140,   141,    21,    22,    -1,   145,    -1,    -1,    -1,   180,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   204,    -1,    -1,    -1,    -1,    -1,    -1,
     180,    -1,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,   204,   115,   116,   117,   118,   119,
      -1,    -1,   122,   123,   124,   125,    -1,   127,   128,   129,
     130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,
     140,   141,    21,    22,    -1,   145,   115,   116,   117,   118,
     119,    -1,    -1,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,
     139,   140,   141,    -1,    -1,    -1,   145,    -1,    -1,    -1,
     180,    -1,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,   204,    -1,    -1,    -1,    -1,    -1,
      -1,   180,    -1,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,   204,   115,   116,   117,   118,
     119,    21,    22,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,    -1,   133,   134,    -1,    -1,    -1,    -1,
     139,   140,   141,    -1,    -1,    -1,   145,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   180,    -1,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   202,   203,   204,    -1,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,    21,    22,   139,
     140,   141,   142,   143,   144,   145,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    38,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,
     180,    -1,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   202,   203,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,    -1,    -1,   127,   128,   129,    -1,    -1,   132,   133,
     134,   135,   136,    -1,    -1,   139,   140,   141,   142,   143,
     144,   145,   115,   116,   117,   118,   119,    21,    22,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,   131,    -1,
     133,   134,    -1,    -1,   137,    -1,   139,   140,   141,    -1,
      -1,    -1,   145,    -1,    -1,    -1,   180,    -1,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,    -1,    -1,
     194,   195,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   180,    -1,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
      -1,   115,   116,   117,   118,   119,    -1,    -1,   122,   123,
     124,   125,    -1,   127,   128,   129,   130,   131,    -1,   133,
     134,    -1,    -1,    -1,    -1,   139,   140,   141,    21,    22,
      -1,   145,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   172,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   180,    -1,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,   131,    -1,   133,   134,
      21,    22,    -1,    -1,   139,   140,   141,    -1,    -1,    -1,
     145,    -1,   115,   116,   117,   118,   119,    -1,    -1,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,   131,    -1,
     133,   134,    21,    22,    -1,    -1,   139,   140,   141,    -1,
      -1,    -1,   145,    -1,    -1,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   172,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   180,    -1,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,    -1,
      -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,   202,
     203,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,    -1,   133,   134,    21,    22,    -1,    -1,   139,   140,
     141,    -1,    -1,    -1,   145,    -1,   115,   116,   117,   118,
     119,    -1,    -1,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,    -1,   133,   134,    -1,   168,    21,    22,
     139,   140,   141,    -1,    -1,    -1,   145,    -1,    -1,   180,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   172,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   180,    -1,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,    -1,    -1,    -1,    -1,    -1,   115,   116,
     117,   118,   119,   202,   203,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,   131,    -1,   133,   134,    -1,    -1,
      21,    22,   139,   140,   141,    -1,    -1,    -1,   145,    -1,
      -1,    -1,   115,   116,   117,   118,   119,    -1,    -1,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,   131,    -1,
     133,   134,    -1,    -1,    -1,   172,   139,   140,   141,    -1,
      -1,    -1,   145,   180,    -1,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,   203,    -1,    -1,   172,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   180,    -1,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,    21,
      22,    -1,    -1,    -1,   115,   116,   117,   118,   119,   202,
     203,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,    -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,
     141,    -1,    -1,    -1,   145,    21,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   172,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   180,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   202,   203,   115,   116,   117,   118,   119,    -1,    -1,
     122,   123,   124,   125,    -1,   127,   128,   129,   130,   131,
      -1,   133,   134,    -1,    -1,    -1,    -1,   139,   140,   141,
      -1,    -1,    -1,   145,    21,    22,    -1,    -1,    -1,   115,
     116,   117,   118,   119,    -1,    -1,   122,   123,   124,   125,
      -1,   127,   128,   129,   130,   131,    -1,   133,   134,    -1,
     172,    -1,    -1,   139,   140,   141,    21,    22,   180,   145,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   180,    -1,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,   203,   115,   116,
     117,   118,   119,    -1,    -1,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,   131,    -1,   133,   134,    21,    22,
      -1,    -1,   139,   140,   141,    -1,    -1,    -1,   145,    -1,
     115,   116,   117,   118,   119,    -1,    -1,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,   131,    -1,   133,   134,
      21,    22,    -1,    -1,   139,   140,   141,    -1,    -1,    -1,
      -1,    -1,    -1,   180,    -1,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,   203,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   180,    -1,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,    -1,    -1,    -1,
      -1,    -1,   115,   116,   117,   118,   119,   202,   203,   122,
     123,   124,   125,    -1,   127,   128,   129,   130,   131,    -1,
     133,   134,    21,    22,    -1,    -1,   139,    -1,   141,    -1,
      -1,    -1,    -1,    -1,   115,   116,   117,   118,   119,    -1,
      -1,   122,   123,   124,   125,    -1,   127,   128,   129,   130,
     131,    -1,   133,   134,    21,    22,    -1,    -1,   139,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,
     203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,    -1,    -1,    -1,    -1,    -1,   115,   116,   117,   118,
     119,   202,   203,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,    -1,   133,   134,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,   116,
     117,   118,   119,    -1,    -1,   122,   123,   124,   125,    -1,
     127,   128,   129,   130,   131,    -1,   133,   134,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,    -1,    -1,    -1,    -1,    -1,    19,    -1,
      -1,    -1,    -1,   202,   203,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   183,   184,   185,   186,
     187,   188,   189,   190,   191,    -1,    -1,    -1,    -1,    -1,
     115,   116,   117,   118,   119,   202,   203,   122,   123,   124,
     125,    -1,   127,   128,   129,   130,   131,    -1,   133,   134,
      71,    72,    73,    -1,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    93,    94,    95,    -1,    -1,    98,    99,   100,
     101,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   184,
     185,   186,   187,   188,   189,   190,   191,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,   203,    19,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   153,    -1,    -1,    -1,    -1,    -1,    71,    72,
      73,   162,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      93,    94,    95,    -1,    -1,    98,    99,   100,   101,    -1,
      -1,    71,    72,    73,    -1,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,    -1,    -1,    98,    99,
     100,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     153,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   153,    -1,    35,    -1,    -1,    -1,    71,
      72,    73,   162,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    93,    94,    95,    -1,    -1,    98,    99,   100,   101,
      -1,    71,    -1,    73,    -1,    75,    76,    77,    78,    79,
      -1,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    93,    94,    95,   128,   129,    98,    99,
     100,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,    -1,
      -1,   153,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     162,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   162,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     202,   203
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   211,     0,     7,    30,    32,    34,    40,    50,    56,
      80,   102,   103,   172,   191,   203,   212,   215,   221,   223,
     224,   228,   254,   258,   282,   356,   363,   367,   376,   422,
     426,   430,    19,    20,   162,   246,   247,   248,   155,   229,
     230,   162,   191,   225,   226,    57,    63,   360,   361,   162,
     207,   214,   360,   360,   360,   138,   162,   270,    34,    63,
     131,   195,   205,   250,   251,   252,   253,   270,   172,   172,
       5,     6,     8,    36,   373,    62,   354,   179,   178,   181,
     178,   225,    22,    57,   190,   202,   227,   162,   172,   354,
     162,   162,   162,   162,   138,   222,   252,   252,   252,   205,
     139,   140,   141,   178,   204,    57,    63,   259,   261,    57,
      63,   364,    57,    63,   374,    57,    63,   355,    15,    16,
     155,   160,   162,   163,   205,   217,   247,   155,   230,   162,
     162,   162,   362,    57,    63,   213,   431,   423,   427,   162,
     164,   220,   206,   248,   252,   252,   252,   252,   262,   162,
     365,   377,   357,   164,   165,   166,   216,    15,    16,   155,
     160,   162,   217,   244,   245,   227,   179,   170,   170,   170,
     164,   206,    35,    71,    73,    75,    76,    77,    78,    79,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    93,    94,    95,    98,    99,   100,   101,   117,   118,
     162,   257,   260,   181,   366,   106,   371,   372,   208,   249,
     328,   164,   165,   166,   178,   206,    19,    25,    31,    41,
      49,    64,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   151,   207,   270,   380,   382,
     383,   386,   392,   393,   421,   432,   424,   428,    21,    22,
      38,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   127,
     128,   129,   132,   133,   134,   135,   136,   139,   140,   141,
     142,   143,   144,   145,   180,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   194,   195,   202,   203,    35,
      35,   205,   255,   170,   263,    75,    79,    93,    94,    98,
      99,   100,   101,   381,   170,   162,   378,   247,   208,   162,
     351,   353,   244,   185,   185,   185,   205,   185,   185,   205,
     185,   185,   185,   185,   185,   185,   205,   270,    33,    60,
      61,   123,   127,   180,   184,   187,   203,   209,   391,   182,
     162,   385,   342,   345,   162,   162,   162,   204,    22,   162,
     204,   150,   206,   328,   338,   339,   181,   256,   266,   368,
     181,   370,   170,   375,   247,   178,   181,   184,   349,   394,
     399,   401,     5,     6,    15,    16,    17,    18,    19,    25,
      27,    31,    39,    45,    48,    51,    55,    65,    68,    69,
      80,   102,   103,   104,   117,   118,   146,   147,   148,   149,
     150,   152,   154,   155,   156,   157,   158,   159,   160,   161,
     163,   170,   187,   188,   189,   194,   195,   203,   205,   207,
     208,   219,   221,   264,   270,   275,   287,   294,   297,   300,
     304,   306,   308,   309,   311,   316,   319,   320,   327,   380,
     434,   442,   452,   455,   467,   470,   403,   397,   162,   387,
     405,   407,   409,   411,   413,   415,   417,   419,   320,   185,
     205,    33,   184,    33,   184,   203,   209,   204,   320,   203,
     209,   392,   178,   469,   162,   172,   178,   340,   389,   421,
     425,   162,   343,   389,   429,   162,   132,   205,     7,    50,
     281,   172,   206,   421,     1,     9,    10,    11,    13,    26,
      28,    29,    38,    42,    44,    52,    54,    58,    59,    65,
     105,   171,   172,   231,   232,   235,   237,   238,   239,   240,
     241,   242,   243,   265,   271,   276,   277,   278,   279,   280,
     282,   286,   307,   320,   162,   358,   359,   270,   334,   162,
     392,   126,   132,   179,   348,   421,   421,   390,   421,   185,
     185,   185,   272,   382,   434,   270,   185,     5,   102,   103,
     185,   205,   185,   205,   205,   185,   185,   205,   185,   205,
     185,   205,   185,   185,   205,   185,   185,   320,   320,   205,
     205,   205,   205,   205,   205,   218,    13,   320,   451,   466,
     320,   320,   320,   320,   320,    13,    49,   298,   320,   298,
     208,   205,   254,   170,   208,   300,   305,    21,    22,   115,
     116,   117,   118,   119,   122,   123,   124,   125,   127,   128,
     129,   130,   131,   133,   134,   139,   140,   141,   145,   180,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     202,   203,   206,   205,   421,   421,   206,   162,   384,   421,
     255,   421,   255,   421,   255,   340,   341,   343,   344,   206,
     396,   267,   298,   204,   204,   204,   320,   162,   433,   181,
     389,   171,   181,   389,   171,   320,   147,   162,   347,   379,
     338,   205,   205,   126,   320,   263,    61,   320,   205,   162,
     172,   155,    58,   320,   263,   126,   320,    37,   172,   172,
     205,    10,   172,   172,   172,   172,   172,   172,    66,   283,
     172,   107,   108,   109,   110,   111,   112,   113,   114,   120,
     121,   126,   132,   135,   136,   142,   143,   144,   179,   179,
     178,   469,   171,   254,   335,   172,   348,   320,   186,   186,
     186,   389,   443,   445,   273,   205,   205,   185,   205,   295,
     185,   185,   185,   462,   298,   392,   466,   320,   288,   290,
     320,   292,   320,   464,   298,   449,   453,   298,   447,   392,
     320,   320,   320,   320,   320,   320,   166,   167,   216,   379,
     137,   178,   469,   379,    13,   178,   469,   469,   147,   152,
     185,   270,   310,    70,   153,   162,   203,   206,   298,   435,
     437,     4,   303,   266,   208,   254,    19,   153,   162,   380,
      19,   153,   162,   380,   320,   320,   320,   320,   320,   320,
     162,   320,   153,   162,   320,   320,   320,   380,   320,   320,
     320,   320,   320,   320,    22,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   128,   129,   153,   162,
     202,   203,   317,   380,   320,   206,   298,   186,   186,   172,
     186,   186,   256,   186,   256,   186,   256,   186,   389,   186,
     389,   269,   421,   206,   469,   204,   171,   421,   421,   206,
     205,    43,   126,   178,   179,   181,   184,   346,   320,   379,
     320,    14,   320,   320,   179,   181,   155,   320,   170,   320,
     205,   147,   162,   205,   285,   350,   352,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   358,   369,     8,   328,
     333,   320,   172,   395,   400,   402,   421,   392,   392,   421,
      70,   441,   267,   162,   320,   421,   456,   458,   460,   392,
     469,   186,   389,   469,   206,   392,   392,   206,   392,   206,
     392,   469,   392,   392,   469,   392,   186,   206,   206,   206,
     206,   206,   206,   320,    20,   320,   451,   171,    20,   379,
     320,   204,   206,   205,   205,   312,   314,   162,   206,   437,
     205,   132,   346,   435,   178,   206,   178,   206,   205,   255,
     171,   303,   185,   205,   185,   205,   205,   205,   204,    19,
     153,   162,   380,   181,   153,   162,   320,   205,   205,   153,
     162,   320,     1,   205,   204,   178,   206,   404,   398,   162,
     388,   406,   186,   410,   186,   414,   186,   418,   340,   420,
     343,   186,   389,   320,   162,   162,   421,   320,   206,    20,
     263,   206,   320,   266,   206,   320,   205,    43,   162,   284,
     178,   181,   349,   171,    57,    63,   331,    67,   332,   172,
     172,   186,   186,   186,   206,   437,   206,   186,   389,   206,
     186,   392,   392,   392,   186,   206,   205,   392,   206,   186,
     186,   186,   186,   206,   186,   186,   206,   186,   303,   205,
     168,   298,   298,    20,   320,   320,   392,   255,   206,   320,
     320,   320,   204,   203,   153,   162,   126,   132,   162,   179,
     184,   301,   302,   256,   255,   321,   320,   323,   320,   206,
     298,   320,   185,   205,   320,   205,   204,   320,   206,   298,
     205,   204,   318,   206,   298,   408,   412,   416,   205,   421,
     206,    43,   346,   263,   298,   263,   171,   263,   206,   320,
     162,   178,   206,   162,   392,   348,    47,   332,    46,   106,
     329,   444,   446,   274,   206,   205,   162,   296,   186,   186,
     186,   463,   268,   466,   186,   289,   291,   293,   465,   450,
     454,   448,   205,   263,   206,   298,   172,   172,   298,   206,
     206,   186,   256,   206,   206,   435,   205,   132,   346,   162,
     162,   205,   162,   162,   178,   206,   137,   263,   299,   256,
     392,   206,   421,   206,   206,   206,   325,   320,   320,   206,
     206,   320,   206,   267,   162,   320,   206,    12,    23,    24,
     233,   234,    12,   236,   206,   162,   181,   349,    43,   172,
     348,   320,    33,   330,   329,   331,   205,   205,   320,   186,
     457,   459,   461,   205,   206,   469,   205,   320,   320,   320,
     205,   441,   205,   205,   206,   320,   206,   451,   320,   172,
     313,   186,   132,   346,   204,   320,   320,   320,   162,   301,
     126,   320,   263,   186,   186,   421,   206,   206,   206,   206,
     263,   263,   205,   237,   276,   277,   278,   279,   320,   172,
     392,   348,   162,   320,   172,   336,   330,   347,   441,   441,
     206,   205,   205,   205,   205,   267,   268,   298,   441,   435,
     436,   206,   172,   468,   468,   320,   310,   315,   320,   320,
     206,   206,   206,   320,   322,   324,   186,   320,   348,   320,
     172,   260,   337,   205,   435,   438,   439,   440,   440,   320,
     441,   441,   436,   206,   206,   469,   440,   206,    53,   171,
     204,   468,   310,   132,   346,   326,   206,   320,   172,   172,
     260,   435,   178,   469,   206,   206,   206,   440,   440,   206,
     206,   206,   320,   204,   320,   320,   263,   172,   263,   206,
     205,   206,   206,   234,   435,   206
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   210,   211,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   211,   212,   213,   213,
     213,   214,   214,   215,   216,   216,   216,   216,   217,   217,
     218,   218,   218,   219,   220,   220,   222,   221,   223,   224,
     225,   225,   225,   225,   226,   226,   227,   227,   228,   229,
     229,   230,   230,   231,   232,   232,   233,   233,   234,   234,
     234,   235,   235,   236,   236,   237,   237,   237,   237,   237,
     238,   238,   239,   240,   241,   242,   243,   244,   244,   244,
     244,   244,   244,   245,   245,   246,   246,   246,   247,   247,
     247,   247,   247,   247,   247,   247,   248,   248,   249,   249,
     250,   250,   250,   251,   251,   252,   252,   252,   252,   252,
     252,   252,   253,   253,   254,   254,   255,   255,   255,   256,
     256,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   258,   259,   259,   259,
     260,   262,   261,   263,   263,   264,   265,   265,   265,   265,
     265,   265,   265,   265,   265,   265,   265,   265,   265,   265,
     265,   265,   265,   265,   266,   266,   266,   267,   267,   268,
     268,   269,   269,   270,   270,   270,   271,   271,   273,   274,
     272,   275,   275,   275,   275,   275,   276,   277,   278,   278,
     278,   279,   279,   280,   281,   281,   281,   282,   282,   283,
     283,   284,   284,   285,   285,   286,   286,   288,   289,   287,
     290,   291,   287,   292,   293,   287,   295,   296,   294,   297,
     297,   297,   298,   298,   299,   299,   299,   300,   300,   300,
     301,   301,   301,   301,   301,   302,   302,   303,   303,   304,
     305,   305,   306,   306,   306,   306,   306,   306,   306,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   308,   308,
     309,   309,   310,   310,   311,   312,   313,   311,   314,   315,
     311,   316,   316,   316,   316,   316,   316,   317,   318,   316,
     319,   319,   319,   319,   319,   319,   319,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   321,   322,   320,   320,   320,   320,
     323,   324,   320,   320,   320,   325,   326,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   327,   327,   327,   327,   327,   327,   327,   327,   327,
     327,   327,   327,   327,   327,   327,   327,   328,   328,   329,
     329,   329,   330,   330,   331,   331,   331,   332,   332,   333,
     334,   335,   334,   336,   334,   337,   334,   338,   338,   339,
     339,   340,   340,   341,   341,   342,   342,   342,   343,   344,
     344,   345,   345,   345,   346,   346,   347,   347,   347,   347,
     347,   348,   348,   348,   349,   349,   350,   350,   350,   350,
     350,   351,   351,   352,   352,   352,   353,   353,   353,   354,
     354,   355,   355,   355,   357,   356,   358,   358,   359,   359,
     359,   360,   360,   360,   362,   361,   363,   364,   364,   364,
     365,   366,   366,   368,   369,   367,   370,   370,   371,   371,
     372,   373,   373,   374,   374,   374,   375,   375,   377,   378,
     376,   379,   379,   379,   379,   379,   380,   380,   380,   380,
     380,   380,   380,   380,   380,   380,   380,   380,   380,   380,
     380,   380,   380,   380,   380,   380,   380,   380,   380,   380,
     380,   380,   380,   381,   381,   381,   381,   381,   381,   381,
     381,   382,   383,   383,   383,   384,   384,   385,   385,   385,
     387,   388,   386,   389,   389,   390,   390,   391,   391,   391,
     391,   392,   392,   393,   393,   393,   393,   394,   395,   393,
     393,   393,   396,   393,   393,   393,   393,   393,   393,   393,
     393,   393,   393,   393,   393,   393,   397,   398,   393,   393,
     399,   400,   393,   401,   402,   393,   403,   404,   393,   393,
     405,   406,   393,   407,   408,   393,   393,   409,   410,   393,
     411,   412,   393,   393,   413,   414,   393,   415,   416,   393,
     417,   418,   393,   419,   420,   393,   421,   421,   421,   423,
     424,   425,   422,   427,   428,   429,   426,   431,   432,   433,
     430,   434,   434,   434,   434,   434,   435,   435,   435,   435,
     435,   435,   435,   435,   436,   437,   438,   438,   439,   439,
     440,   440,   441,   441,   443,   444,   442,   445,   446,   442,
     447,   448,   442,   449,   450,   442,   451,   451,   452,   453,
     454,   452,   455,   456,   457,   455,   458,   459,   455,   460,
     461,   455,   455,   462,   463,   455,   455,   464,   465,   455,
     466,   466,   467,   467,   467,   467,   468,   468,   469,   469,
     470,   470,   470
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       3,     2,     2,     2,     2,     2,     2,     2,     0,     1,
       1,     1,     1,     4,     1,     1,     2,     2,     3,     2,
       0,     2,     4,     3,     1,     2,     0,     4,     2,     2,
       1,     2,     3,     3,     2,     4,     0,     1,     2,     1,
       3,     1,     3,     3,     3,     2,     1,     1,     0,     2,
       6,     1,     1,     0,     2,     1,     1,     1,     1,     1,
       6,     7,     7,     2,     5,     5,     4,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     1,     5,     1,     3,     2,     3,
       1,     1,     1,     1,     4,     1,     2,     3,     3,     3,
       3,     2,     1,     3,     0,     3,     0,     2,     3,     0,
       2,     1,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     3,     3,     2,     2,     3,
       4,     3,     2,     2,     2,     2,     2,     3,     3,     3,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     0,     1,     1,
       3,     0,     4,     3,     7,     2,     1,     2,     2,     1,
       1,     1,     1,     2,     1,     2,     2,     2,     2,     1,
       1,     2,     2,     2,     0,     2,     2,     0,     2,     0,
       2,     1,     3,     1,     3,     2,     2,     3,     0,     0,
       5,     2,     5,     5,     6,     2,     1,     1,     1,     2,
       3,     2,     3,     4,     1,     1,     0,     1,     1,     1,
       0,     1,     3,     8,     7,     3,     3,     0,     0,     7,
       0,     0,     7,     0,     0,     7,     0,     0,     6,     5,
       8,    10,     1,     3,     1,     2,     3,     1,     1,     2,
       2,     2,     2,     2,     4,     1,     3,     0,     4,     6,
       6,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     6,     8,
       5,     6,     1,     4,     3,     0,     0,     8,     0,     0,
       9,     3,     4,     5,     6,     5,     6,     0,     0,     5,
       3,     4,     4,     5,     4,     3,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     2,
       2,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     2,     2,     2,     4,     4,     5,
       4,     5,     3,     4,     1,     1,     2,     4,     4,     7,
       8,     6,     3,     5,     0,     0,     8,     3,     3,     3,
       0,     0,     8,     3,     4,     0,     0,     9,     4,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     3,     1,
       4,     4,     4,     4,     4,     4,     1,     6,     7,     6,
       6,     7,     7,     6,     7,     6,     6,     0,     1,     0,
       1,     1,     0,     1,     0,     1,     1,     0,     1,     5,
       0,     0,     4,     0,     9,     0,    10,     3,     4,     1,
       3,     1,     3,     1,     3,     0,     2,     3,     3,     1,
       3,     0,     2,     3,     1,     1,     1,     2,     3,     5,
       3,     1,     1,     1,     0,     1,     1,     4,     3,     3,
       5,     1,     3,     4,     6,     5,     4,     6,     5,     0,
       1,     0,     1,     1,     0,     6,     1,     3,     0,     1,
       3,     0,     1,     1,     0,     5,     3,     0,     1,     1,
       1,     0,     2,     0,     0,    11,     0,     2,     0,     1,
       3,     1,     1,     0,     1,     1,     0,     3,     0,     0,
       7,     1,     4,     3,     3,     5,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     4,     4,     1,     3,     0,     1,     3,
       0,     0,     6,     1,     1,     1,     3,     3,     2,     4,
       3,     1,     2,     1,     1,     1,     1,     0,     0,     6,
       4,     5,     0,     9,     4,     2,     2,     3,     2,     3,
       2,     2,     3,     3,     3,     2,     0,     0,     6,     2,
       0,     0,     6,     0,     0,     6,     0,     0,     6,     1,
       0,     0,     6,     0,     0,     7,     1,     0,     0,     6,
       0,     0,     7,     1,     0,     0,     6,     0,     0,     7,
       0,     0,     6,     0,     0,     6,     1,     3,     3,     0,
       0,     0,     9,     0,     0,     0,     9,     0,     0,     0,
      10,     1,     1,     1,     1,     1,     3,     3,     5,     5,
       6,     6,     8,     8,     1,     1,     3,     5,     1,     2,
       1,     0,     0,     1,     0,     0,    10,     0,     0,    10,
       0,     0,     9,     0,     0,     7,     3,     1,     5,     0,
       0,    10,     4,     0,     0,    11,     0,     0,    11,     0,
       0,    10,     5,     0,     0,     9,     5,     0,     0,    10,
       1,     3,     4,     5,     7,     9,     0,     3,     0,     1,
       9,    10,     9
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

    case YYSYMBOL_global_let_variable_name_with_pos_list: /* global_let_variable_name_with_pos_list  */
            { delete ((*yyvaluep).pNameWithPosList); }
        break;

    case YYSYMBOL_let_variable_declaration: /* let_variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
        break;

    case YYSYMBOL_global_let_variable_declaration: /* global_let_variable_declaration  */
            { delete ((*yyvaluep).pVarDecl); }
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
            das2_yyerror(scanner,"this module already has a name " + yyextra->g_Program->thisModule->name,tokAt(scanner,(yylsp[-2])),
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
        if ( err ) das2_yyerror(scanner,"invalid escape sequence",tokAt(scanner,(yylsp[-1])), CompilationError::invalid_escape_sequence);
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
            das2_yyend_reader(scanner);
        }
    }
    break;

  case 35: /* reader_character_sequence: reader_character_sequence STRING_CHARACTER  */
                                                                {
        if ( !yyextra->g_ReaderMacro->accept(yyextra->g_Program.get(), yyextra->g_Program->thisModule.get(), yyextra->g_ReaderExpr, (yyvsp[0].ch), tokAt(scanner,(yylsp[0]))) ) {
            das2_yyend_reader(scanner);
        }
    }
    break;

  case 36: /* $@1: %empty  */
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

  case 37: /* expr_reader: '%' name_in_namespace $@1 reader_character_sequence  */
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

  case 38: /* options_declaration: "options" annotation_argument_list  */
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

  case 40: /* require_module_name: "name"  */
                   {
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 41: /* require_module_name: '%' require_module_name  */
                                     {
        *(yyvsp[0].s) = "%" + *(yyvsp[0].s);
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 42: /* require_module_name: require_module_name '.' "name"  */
                                                {
        *(yyvsp[-2].s) += ".";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 43: /* require_module_name: require_module_name '/' "name"  */
                                                {
        *(yyvsp[-2].s) += "/";
        *(yyvsp[-2].s) += *(yyvsp[0].s);
        delete (yyvsp[0].s);
        (yyval.s) = (yyvsp[-2].s);
    }
    break;

  case 44: /* require_module: require_module_name is_public_module  */
                                                         {
        ast_requireModule(scanner,(yyvsp[-1].s),nullptr,(yyvsp[0].b),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 45: /* require_module: require_module_name "as" "name" is_public_module  */
                                                                              {
        ast_requireModule(scanner,(yyvsp[-3].s),(yyvsp[-1].s),(yyvsp[0].b),tokAt(scanner,(yylsp[-3])));
    }
    break;

  case 46: /* is_public_module: %empty  */
                    { (yyval.b) = false; }
    break;

  case 47: /* is_public_module: "public"  */
                    { (yyval.b) = true; }
    break;

  case 51: /* expect_error: "integer constant"  */
                   {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[0].i))] ++;
    }
    break;

  case 52: /* expect_error: "integer constant" ':' "integer constant"  */
                                      {
        yyextra->g_Program->expectErrors[CompilationError((yyvsp[-2].i))] += (yyvsp[0].i);
    }
    break;

  case 53: /* expression_label: "label" "integer constant" ':'  */
                                          {
        (yyval.pExpression) = new ExprLabel(tokAt(scanner,(yylsp[-2])),(yyvsp[-1].i));
    }
    break;

  case 54: /* expression_goto: "goto" "label" "integer constant"  */
                                                {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-2])),(yyvsp[0].i));
    }
    break;

  case 55: /* expression_goto: "goto" expr  */
                               {
        (yyval.pExpression) = new ExprGoto(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 56: /* elif_or_static_elif: "elif"  */
                          { (yyval.b) = false; }
    break;

  case 57: /* elif_or_static_elif: "static_elif"  */
                          { (yyval.b) = true; }
    break;

  case 58: /* expression_else: %empty  */
                                                           { (yyval.pExpression) = nullptr; }
    break;

  case 59: /* expression_else: "else" expression_block  */
                                                           { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 60: /* expression_else: elif_or_static_elif '(' expr ')' expression_block expression_else  */
                                                                                                  {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-5])),ExpressionPtr((yyvsp[-3].pExpression)),
            ExpressionPtr((yyvsp[-1].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
        eite->isStatic = (yyvsp[-5].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 61: /* if_or_static_if: "if"  */
                        { (yyval.b) = false; }
    break;

  case 62: /* if_or_static_if: "static_if"  */
                        { (yyval.b) = true; }
    break;

  case 63: /* expression_else_one_liner: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 64: /* expression_else_one_liner: "else" expression_if_one_liner  */
                                                      {
            (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 65: /* expression_if_one_liner: expr  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 66: /* expression_if_one_liner: expression_return  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 67: /* expression_if_one_liner: expression_yield  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 68: /* expression_if_one_liner: expression_break  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 69: /* expression_if_one_liner: expression_continue  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 70: /* expression_if_then_else: if_or_static_if '(' expr ')' expression_block expression_else  */
                                                                                              {
        auto eite = new ExprIfThenElse(tokAt(scanner,(yylsp[-5])),ExpressionPtr((yyvsp[-3].pExpression)),
            ExpressionPtr((yyvsp[-1].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
        eite->isStatic = (yyvsp[-5].b);
        (yyval.pExpression) = eite;
    }
    break;

  case 71: /* expression_if_then_else: expression_if_one_liner "if" '(' expr ')' expression_else_one_liner "end of expression"  */
                                                                                                                {
        (yyval.pExpression) = new ExprIfThenElse(tokAt(scanner,(yylsp[-5])),ExpressionPtr((yyvsp[-3].pExpression)),ExpressionPtr(ast_wrapInBlock((yyvsp[-6].pExpression))),(yyvsp[-1].pExpression) ? ExpressionPtr(ast_wrapInBlock((yyvsp[-1].pExpression))) : nullptr);
    }
    break;

  case 72: /* expression_for_loop: "for" '(' variable_name_with_pos_list "in" expr_list ')' expression_block  */
                                                                                                               {
        (yyval.pExpression) = ast_forLoop(scanner,(yyvsp[-4].pNameWithPosList),(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 73: /* expression_unsafe: "unsafe" expression_block  */
                                                 {
        auto pUnsafe = new ExprUnsafe(tokAt(scanner,(yylsp[-1])));
        pUnsafe->body = ExpressionPtr((yyvsp[0].pExpression));
        (yyval.pExpression) = pUnsafe;
    }
    break;

  case 74: /* expression_while_loop: "while" '(' expr ')' expression_block  */
                                                                       {
        auto pWhile = new ExprWhile(tokAt(scanner,(yylsp[-4])));
        pWhile->cond = ExpressionPtr((yyvsp[-2].pExpression));
        pWhile->body = ExpressionPtr((yyvsp[0].pExpression));
        ((ExprBlock *)(yyvsp[0].pExpression))->inTheLoop = true;
        (yyval.pExpression) = pWhile;
    }
    break;

  case 75: /* expression_with: "with" '(' expr ')' expression_block  */
                                                                 {
        auto pWith = new ExprWith(tokAt(scanner,(yylsp[-4])));
        pWith->with = ExpressionPtr((yyvsp[-2].pExpression));
        pWith->body = ExpressionPtr((yyvsp[0].pExpression));
        (yyval.pExpression) = pWith;
    }
    break;

  case 76: /* expression_with_alias: "assume" "name" '=' expr  */
                                                      {
        (yyval.pExpression) = new ExprAssume(tokAt(scanner,(yylsp[-3])), *(yyvsp[-2].s), (yyvsp[0].pExpression) );
        delete (yyvsp[-2].s);
    }
    break;

  case 77: /* annotation_argument_value: string_constant  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 78: /* annotation_argument_value: "name"  */
                                 { (yyval.aa) = new AnnotationArgument("",*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 79: /* annotation_argument_value: "integer constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",(yyvsp[0].i)); }
    break;

  case 80: /* annotation_argument_value: "floating point constant"  */
                                 { (yyval.aa) = new AnnotationArgument("",float((yyvsp[0].fd))); }
    break;

  case 81: /* annotation_argument_value: "true"  */
                                 { (yyval.aa) = new AnnotationArgument("",true); }
    break;

  case 82: /* annotation_argument_value: "false"  */
                                 { (yyval.aa) = new AnnotationArgument("",false); }
    break;

  case 83: /* annotation_argument_value_list: annotation_argument_value  */
                                       {
        (yyval.aaList) = new AnnotationArgumentList();
        (yyval.aaList)->push_back(*(yyvsp[0].aa));
        delete (yyvsp[0].aa);
    }
    break;

  case 84: /* annotation_argument_value_list: annotation_argument_value_list ',' annotation_argument_value  */
                                                                                {
            (yyval.aaList) = (yyvsp[-2].aaList);
            (yyval.aaList)->push_back(*(yyvsp[0].aa));
            delete (yyvsp[0].aa);
    }
    break;

  case 85: /* annotation_argument_name: "name"  */
                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 86: /* annotation_argument_name: "type"  */
                    { (yyval.s) = new string("type"); }
    break;

  case 87: /* annotation_argument_name: "in"  */
                    { (yyval.s) = new string("in"); }
    break;

  case 88: /* annotation_argument: annotation_argument_name '=' string_constant  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 89: /* annotation_argument: annotation_argument_name '=' "name"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[0].s); delete (yyvsp[-2].s); }
    break;

  case 90: /* annotation_argument: annotation_argument_name '=' "integer constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),(yyvsp[0].i),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 91: /* annotation_argument: annotation_argument_name '=' "floating point constant"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),float((yyvsp[0].fd)),tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 92: /* annotation_argument: annotation_argument_name '=' "true"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),true,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 93: /* annotation_argument: annotation_argument_name '=' "false"  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[-2].s),false,tokAt(scanner,(yylsp[-2]))); delete (yyvsp[-2].s); }
    break;

  case 94: /* annotation_argument: annotation_argument_name  */
                                                                    { (yyval.aa) = new AnnotationArgument(*(yyvsp[0].s),true,tokAt(scanner,(yylsp[0]))); delete (yyvsp[0].s); }
    break;

  case 95: /* annotation_argument: annotation_argument_name '=' '(' annotation_argument_value_list ')'  */
                                                                                          {
        { (yyval.aa) = new AnnotationArgument(*(yyvsp[-4].s),(yyvsp[-1].aaList),tokAt(scanner,(yylsp[-4]))); delete (yyvsp[-4].s); }
    }
    break;

  case 96: /* annotation_argument_list: annotation_argument  */
                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 97: /* annotation_argument_list: annotation_argument_list ',' annotation_argument  */
                                                                    {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 98: /* metadata_argument_list: '@' annotation_argument  */
                                      {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,new AnnotationArgumentList(),(yyvsp[0].aa));
    }
    break;

  case 99: /* metadata_argument_list: metadata_argument_list '@' annotation_argument  */
                                                                  {
        (yyval.aaList) = ast_annotationArgumentListEntry(scanner,(yyvsp[-2].aaList),(yyvsp[0].aa));
    }
    break;

  case 100: /* annotation_declaration_name: name_in_namespace  */
                                    { (yyval.s) = (yyvsp[0].s); }
    break;

  case 101: /* annotation_declaration_name: "require"  */
                                    { (yyval.s) = new string("require"); }
    break;

  case 102: /* annotation_declaration_name: "private"  */
                                    { (yyval.s) = new string("private"); }
    break;

  case 103: /* annotation_declaration_basic: annotation_declaration_name  */
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

  case 104: /* annotation_declaration_basic: annotation_declaration_name '(' annotation_argument_list ')'  */
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

  case 105: /* annotation_declaration: annotation_declaration_basic  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
    }
    break;

  case 106: /* annotation_declaration: '!' annotation_declaration  */
                                              {
        if ( !(yyvsp[0].fa)->annotation->rtti_isFunctionAnnotation() || !((FunctionAnnotation *)((yyvsp[0].fa)->annotation.get()))->isSpecialized() ) {
            das2_yyerror(scanner,"can only run logical operations on contracts", tokAt(scanner, (yylsp[0])),
                CompilationError::invalid_annotation); }
        (yyval.fa) = new AnnotationDeclaration();
        (yyval.fa)->at = tokAt(scanner, (yylsp[-1]));
        (yyval.fa)->annotation = newLogicAnnotation(LogicAnnotationOp::Not,(yyvsp[0].fa),nullptr);
    }
    break;

  case 107: /* annotation_declaration: annotation_declaration "&&" annotation_declaration  */
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

  case 108: /* annotation_declaration: annotation_declaration "||" annotation_declaration  */
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

  case 109: /* annotation_declaration: annotation_declaration "^^" annotation_declaration  */
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

  case 110: /* annotation_declaration: '(' annotation_declaration ')'  */
                                            {
        (yyval.fa) = (yyvsp[-1].fa);
    }
    break;

  case 111: /* annotation_declaration: "|>" annotation_declaration  */
                                          {
        (yyval.fa) = (yyvsp[0].fa);
        (yyvsp[0].fa)->inherited = true;
    }
    break;

  case 112: /* annotation_list: annotation_declaration  */
                                    {
            (yyval.faList) = new AnnotationList();
            (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 113: /* annotation_list: annotation_list ',' annotation_declaration  */
                                                              {
        (yyval.faList) = (yyvsp[-2].faList);
        (yyval.faList)->push_back(AnnotationDeclarationPtr((yyvsp[0].fa)));
    }
    break;

  case 114: /* optional_annotation_list: %empty  */
                                        { (yyval.faList) = nullptr; }
    break;

  case 115: /* optional_annotation_list: '[' annotation_list ']'  */
                                        { (yyval.faList) = (yyvsp[-1].faList); }
    break;

  case 116: /* optional_function_argument_list: %empty  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 117: /* optional_function_argument_list: '(' ')'  */
                                                { (yyval.pVarDeclList) = nullptr; }
    break;

  case 118: /* optional_function_argument_list: '(' function_argument_list ')'  */
                                                { (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList); }
    break;

  case 119: /* optional_function_type: %empty  */
        {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
    }
    break;

  case 120: /* optional_function_type: ':' type_declaration  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 121: /* function_name: "name"  */
                          {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.s) = (yyvsp[0].s);
    }
    break;

  case 122: /* function_name: "operator" '!'  */
                             { (yyval.s) = new string("!"); }
    break;

  case 123: /* function_name: "operator" '~'  */
                             { (yyval.s) = new string("~"); }
    break;

  case 124: /* function_name: "operator" "+="  */
                             { (yyval.s) = new string("+="); }
    break;

  case 125: /* function_name: "operator" "-="  */
                             { (yyval.s) = new string("-="); }
    break;

  case 126: /* function_name: "operator" "*="  */
                             { (yyval.s) = new string("*="); }
    break;

  case 127: /* function_name: "operator" "/="  */
                             { (yyval.s) = new string("/="); }
    break;

  case 128: /* function_name: "operator" "%="  */
                             { (yyval.s) = new string("%="); }
    break;

  case 129: /* function_name: "operator" "&="  */
                             { (yyval.s) = new string("&="); }
    break;

  case 130: /* function_name: "operator" "|="  */
                             { (yyval.s) = new string("|="); }
    break;

  case 131: /* function_name: "operator" "^="  */
                             { (yyval.s) = new string("^="); }
    break;

  case 132: /* function_name: "operator" "&&="  */
                                { (yyval.s) = new string("&&="); }
    break;

  case 133: /* function_name: "operator" "||="  */
                                { (yyval.s) = new string("||="); }
    break;

  case 134: /* function_name: "operator" "^^="  */
                                { (yyval.s) = new string("^^="); }
    break;

  case 135: /* function_name: "operator" "&&"  */
                             { (yyval.s) = new string("&&"); }
    break;

  case 136: /* function_name: "operator" "||"  */
                             { (yyval.s) = new string("||"); }
    break;

  case 137: /* function_name: "operator" "^^"  */
                             { (yyval.s) = new string("^^"); }
    break;

  case 138: /* function_name: "operator" '+'  */
                             { (yyval.s) = new string("+"); }
    break;

  case 139: /* function_name: "operator" '-'  */
                             { (yyval.s) = new string("-"); }
    break;

  case 140: /* function_name: "operator" '*'  */
                             { (yyval.s) = new string("*"); }
    break;

  case 141: /* function_name: "operator" '/'  */
                             { (yyval.s) = new string("/"); }
    break;

  case 142: /* function_name: "operator" '%'  */
                             { (yyval.s) = new string("%"); }
    break;

  case 143: /* function_name: "operator" '<'  */
                             { (yyval.s) = new string("<"); }
    break;

  case 144: /* function_name: "operator" '>'  */
                             { (yyval.s) = new string(">"); }
    break;

  case 145: /* function_name: "operator" ".."  */
                             { (yyval.s) = new string("interval"); }
    break;

  case 146: /* function_name: "operator" "=="  */
                             { (yyval.s) = new string("=="); }
    break;

  case 147: /* function_name: "operator" "!="  */
                             { (yyval.s) = new string("!="); }
    break;

  case 148: /* function_name: "operator" "<="  */
                             { (yyval.s) = new string("<="); }
    break;

  case 149: /* function_name: "operator" ">="  */
                             { (yyval.s) = new string(">="); }
    break;

  case 150: /* function_name: "operator" '&'  */
                             { (yyval.s) = new string("&"); }
    break;

  case 151: /* function_name: "operator" '|'  */
                             { (yyval.s) = new string("|"); }
    break;

  case 152: /* function_name: "operator" '^'  */
                             { (yyval.s) = new string("^"); }
    break;

  case 153: /* function_name: "++" "operator"  */
                             { (yyval.s) = new string("++"); }
    break;

  case 154: /* function_name: "--" "operator"  */
                             { (yyval.s) = new string("--"); }
    break;

  case 155: /* function_name: "operator" "++"  */
                             { (yyval.s) = new string("+++"); }
    break;

  case 156: /* function_name: "operator" "--"  */
                             { (yyval.s) = new string("---"); }
    break;

  case 157: /* function_name: "operator" "<<"  */
                             { (yyval.s) = new string("<<"); }
    break;

  case 158: /* function_name: "operator" ">>"  */
                             { (yyval.s) = new string(">>"); }
    break;

  case 159: /* function_name: "operator" "<<="  */
                             { (yyval.s) = new string("<<="); }
    break;

  case 160: /* function_name: "operator" ">>="  */
                             { (yyval.s) = new string(">>="); }
    break;

  case 161: /* function_name: "operator" "<<<"  */
                             { (yyval.s) = new string("<<<"); }
    break;

  case 162: /* function_name: "operator" ">>>"  */
                             { (yyval.s) = new string(">>>"); }
    break;

  case 163: /* function_name: "operator" "<<<="  */
                             { (yyval.s) = new string("<<<="); }
    break;

  case 164: /* function_name: "operator" ">>>="  */
                             { (yyval.s) = new string(">>>="); }
    break;

  case 165: /* function_name: "operator" '[' ']'  */
                             { (yyval.s) = new string("[]"); }
    break;

  case 166: /* function_name: "operator" "?[" ']'  */
                                { (yyval.s) = new string("?[]"); }
    break;

  case 167: /* function_name: "operator" '.'  */
                             { (yyval.s) = new string("."); }
    break;

  case 168: /* function_name: "operator" "?."  */
                             { (yyval.s) = new string("?."); }
    break;

  case 169: /* function_name: "operator" '.' "name"  */
                                       { (yyval.s) = new string(".`"+*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 170: /* function_name: "operator" '.' "name" ":="  */
                                             { (yyval.s) = new string(".`"+*(yyvsp[-1].s)+"`clone"); delete (yyvsp[-1].s); }
    break;

  case 171: /* function_name: "operator" "?." "name"  */
                                       { (yyval.s) = new string("?.`"+*(yyvsp[0].s)); delete (yyvsp[0].s);}
    break;

  case 172: /* function_name: "operator" ":="  */
                                { (yyval.s) = new string("clone"); }
    break;

  case 173: /* function_name: "operator" "delete"  */
                                { (yyval.s) = new string("finalize"); }
    break;

  case 174: /* function_name: "operator" "??"  */
                           { (yyval.s) = new string("??"); }
    break;

  case 175: /* function_name: "operator" "is"  */
                            { (yyval.s) = new string("`is"); }
    break;

  case 176: /* function_name: "operator" "as"  */
                            { (yyval.s) = new string("`as"); }
    break;

  case 177: /* function_name: "operator" "is" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`is`" + *(yyvsp[0].s); }
    break;

  case 178: /* function_name: "operator" "as" "name"  */
                                       { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "`as`" + *(yyvsp[0].s); }
    break;

  case 179: /* function_name: "operator" '?' "as"  */
                                { (yyval.s) = new string("?as"); }
    break;

  case 180: /* function_name: "operator" '?' "as" "name"  */
                                           { (yyval.s) = (yyvsp[0].s); *(yyvsp[0].s) = "?as`" + *(yyvsp[0].s); }
    break;

  case 181: /* function_name: "bool"  */
                     { (yyval.s) = new string("bool"); }
    break;

  case 182: /* function_name: "string"  */
                     { (yyval.s) = new string("string"); }
    break;

  case 183: /* function_name: "int"  */
                     { (yyval.s) = new string("int"); }
    break;

  case 184: /* function_name: "int2"  */
                     { (yyval.s) = new string("int2"); }
    break;

  case 185: /* function_name: "int3"  */
                     { (yyval.s) = new string("int3"); }
    break;

  case 186: /* function_name: "int4"  */
                     { (yyval.s) = new string("int4"); }
    break;

  case 187: /* function_name: "uint"  */
                     { (yyval.s) = new string("uint"); }
    break;

  case 188: /* function_name: "uint2"  */
                     { (yyval.s) = new string("uint2"); }
    break;

  case 189: /* function_name: "uint3"  */
                     { (yyval.s) = new string("uint3"); }
    break;

  case 190: /* function_name: "uint4"  */
                     { (yyval.s) = new string("uint4"); }
    break;

  case 191: /* function_name: "float"  */
                     { (yyval.s) = new string("float"); }
    break;

  case 192: /* function_name: "float2"  */
                     { (yyval.s) = new string("float2"); }
    break;

  case 193: /* function_name: "float3"  */
                     { (yyval.s) = new string("float3"); }
    break;

  case 194: /* function_name: "float4"  */
                     { (yyval.s) = new string("float4"); }
    break;

  case 195: /* function_name: "range"  */
                     { (yyval.s) = new string("range"); }
    break;

  case 196: /* function_name: "urange"  */
                     { (yyval.s) = new string("urange"); }
    break;

  case 197: /* function_name: "range64"  */
                     { (yyval.s) = new string("range64"); }
    break;

  case 198: /* function_name: "urange64"  */
                     { (yyval.s) = new string("urange64"); }
    break;

  case 199: /* function_name: "int64"  */
                     { (yyval.s) = new string("int64"); }
    break;

  case 200: /* function_name: "uint64"  */
                     { (yyval.s) = new string("uint64"); }
    break;

  case 201: /* function_name: "double"  */
                     { (yyval.s) = new string("double"); }
    break;

  case 202: /* function_name: "int8"  */
                     { (yyval.s) = new string("int8"); }
    break;

  case 203: /* function_name: "uint8"  */
                     { (yyval.s) = new string("uint8"); }
    break;

  case 204: /* function_name: "int16"  */
                     { (yyval.s) = new string("int16"); }
    break;

  case 205: /* function_name: "uint16"  */
                     { (yyval.s) = new string("uint16"); }
    break;

  case 206: /* global_function_declaration: optional_annotation_list "def" function_declaration  */
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

  case 207: /* optional_public_or_private_function: %empty  */
                        { (yyval.b) = yyextra->g_thisStructure ? !yyextra->g_thisStructure->privateStructure : yyextra->g_Program->thisModule->isPublic; }
    break;

  case 208: /* optional_public_or_private_function: "private"  */
                        { (yyval.b) = false; }
    break;

  case 209: /* optional_public_or_private_function: "public"  */
                        { (yyval.b) = true; }
    break;

  case 210: /* function_declaration_header: function_name optional_function_argument_list optional_function_type  */
                                                                                                {
        (yyval.pFuncDecl) = ast_functionDeclarationHeader(scanner,(yyvsp[-2].s),(yyvsp[-1].pVarDeclList),(yyvsp[0].pTypeDecl),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 211: /* $@2: %empty  */
                                                     {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
        }
    }
    break;

  case 212: /* function_declaration: optional_public_or_private_function $@2 function_declaration_header expression_block  */
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

  case 213: /* expression_block: "begin of code block" expressions "end of code block"  */
                                                   {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-2]),(yylsp[0]));
    }
    break;

  case 214: /* expression_block: "begin of code block" expressions "end of code block" "finally" "begin of code block" expressions "end of code block"  */
                                                                                          {
        auto pB = (ExprBlock *) (yyvsp[-5].pExpression);
        auto pF = (ExprBlock *) (yyvsp[-1].pExpression);
        swap ( pB->finalList, pF->list );
        (yyval.pExpression) = (yyvsp[-5].pExpression);
        (yyval.pExpression)->at = tokRangeAt(scanner,(yylsp[-6]),(yylsp[0]));
        delete (yyvsp[-1].pExpression);
    }
    break;

  case 215: /* expr_call_pipe: expr_call expr_full_block_assumed_piped  */
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

  case 216: /* expression_any: "end of expression"  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 217: /* expression_any: expr_assign "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 218: /* expression_any: expression_delete "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 219: /* expression_any: expression_let  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 220: /* expression_any: expression_while_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 221: /* expression_any: expression_unsafe  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 222: /* expression_any: expression_with  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 223: /* expression_any: expression_with_alias "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 224: /* expression_any: expression_for_loop  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 225: /* expression_any: expression_break "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 226: /* expression_any: expression_continue "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 227: /* expression_any: expression_return "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 228: /* expression_any: expression_yield "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 229: /* expression_any: expression_if_then_else  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 230: /* expression_any: expression_try_catch  */
                                            { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 231: /* expression_any: expression_label "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 232: /* expression_any: expression_goto "end of expression"  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 233: /* expression_any: "pass" "end of expression"  */
                                            { (yyval.pExpression) = nullptr; }
    break;

  case 234: /* expressions: %empty  */
        {
        (yyval.pExpression) = new ExprBlock();
        (yyval.pExpression)->at = LineInfo(yyextra->g_FileAccessStack.back(),
            yylloc.first_column,yylloc.first_line,yylloc.last_column,yylloc.last_line);
    }
    break;

  case 235: /* expressions: expressions expression_any  */
                                                        {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
        if ( (yyvsp[0].pExpression) ) {
            static_cast<ExprBlock*>((yyvsp[-1].pExpression))->list.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        }
    }
    break;

  case 236: /* expressions: expressions error  */
                                 {
        delete (yyvsp[-1].pExpression); (yyval.pExpression) = nullptr; YYABORT;
    }
    break;

  case 237: /* optional_expr_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 238: /* optional_expr_list: expr_list optional_comma  */
                                            { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 239: /* optional_expr_map_tuple_list: %empty  */
        { (yyval.pExpression) = nullptr; }
    break;

  case 240: /* optional_expr_map_tuple_list: expr_map_tuple_list optional_comma  */
                                                      { (yyval.pExpression) = (yyvsp[-1].pExpression); }
    break;

  case 241: /* type_declaration_no_options_list: type_declaration  */
                               {
        (yyval.pTypeDeclList) = new vector<Expression *>();
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 242: /* type_declaration_no_options_list: type_declaration_no_options_list c_or_s type_declaration  */
                                                                              {
        (yyval.pTypeDeclList) = (yyvsp[-2].pTypeDeclList);
        (yyval.pTypeDeclList)->push_back(new ExprTypeDecl(tokAt(scanner,(yylsp[0])),(yyvsp[0].pTypeDecl)));
    }
    break;

  case 243: /* name_in_namespace: "name"  */
                                               { (yyval.s) = (yyvsp[0].s); }
    break;

  case 244: /* name_in_namespace: "name" "::" "name"  */
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

  case 245: /* name_in_namespace: "::" "name"  */
                                               { *(yyvsp[0].s) = "::" + *(yyvsp[0].s); (yyval.s) = (yyvsp[0].s); }
    break;

  case 246: /* expression_delete: "delete" expr  */
                                      {
        (yyval.pExpression) = new ExprDelete(tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 247: /* expression_delete: "delete" "explicit" expr  */
                                                   {
        auto delExpr = new ExprDelete(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[0].pExpression)));
        delExpr->native = true;
        (yyval.pExpression) = delExpr;
    }
    break;

  case 248: /* $@3: %empty  */
           { yyextra->das_arrow_depth ++; }
    break;

  case 249: /* $@4: %empty  */
                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 250: /* new_type_declaration: '<' $@3 type_declaration '>' $@4  */
                                                                                                            {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 251: /* expr_new: "new" structure_type_declaration  */
                                                            {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-1])),TypeDeclPtr((yyvsp[0].pTypeDecl)),true);
    }
    break;

  case 252: /* expr_new: "new" structure_type_declaration '(' optional_expr_list ')'  */
                                                                                                   {
        auto pNew = new ExprNew(tokAt(scanner,(yylsp[-4])),TypeDeclPtr((yyvsp[-3].pTypeDecl)),true);
        (yyval.pExpression) = parseFunctionArguments(pNew,(yyvsp[-1].pExpression));
    }
    break;

  case 253: /* expr_new: "new" new_type_declaration '(' use_initializer ')'  */
                                                                                     {
        (yyval.pExpression) = new ExprNew(tokAt(scanner,(yylsp[-4])),TypeDeclPtr((yyvsp[-3].pTypeDecl)),true);
        ((ExprNew *)(yyval.pExpression))->initializer = (yyvsp[-1].b);
    }
    break;

  case 254: /* expr_new: "new" new_type_declaration '(' use_initializer make_struct_single ')'  */
                                                                                                            {
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->at = tokAt(scanner,(yylsp[-4]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = (yyvsp[-4].pTypeDecl);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-5])),ExpressionPtr((yyvsp[-1].pExpression)));
    }
    break;

  case 255: /* expr_new: "new" make_decl  */
                                    {
        (yyval.pExpression) = new ExprAscend(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 256: /* expression_break: "break"  */
                       { (yyval.pExpression) = new ExprBreak(tokAt(scanner,(yylsp[0]))); }
    break;

  case 257: /* expression_continue: "continue"  */
                          { (yyval.pExpression) = new ExprContinue(tokAt(scanner,(yylsp[0]))); }
    break;

  case 258: /* expression_return: "return"  */
                        {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[0])),nullptr);
    }
    break;

  case 259: /* expression_return: "return" expr  */
                                      {
        (yyval.pExpression) = new ExprReturn(tokAt(scanner,(yylsp[-1])),(yyvsp[0].pExpression));
    }
    break;

  case 260: /* expression_return: "return" "<-" expr  */
                                             {
        auto pRet = new ExprReturn(tokAt(scanner,(yylsp[-2])),(yyvsp[0].pExpression));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 261: /* expression_yield: "yield" expr  */
                                     {
        (yyval.pExpression) = new ExprYield(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 262: /* expression_yield: "yield" "<-" expr  */
                                            {
        auto pRet = new ExprYield(tokAt(scanner,(yylsp[-2])),ExpressionPtr((yyvsp[0].pExpression)));
        pRet->moveSemantics = true;
        (yyval.pExpression) = pRet;
    }
    break;

  case 263: /* expression_try_catch: "try" expression_block "recover" expression_block  */
                                                                                       {
        (yyval.pExpression) = new ExprTryCatch(tokAt(scanner,(yylsp[-3])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 264: /* kwd_let_var_or_nothing: "let"  */
                 { (yyval.b) = true; }
    break;

  case 265: /* kwd_let_var_or_nothing: "var"  */
                 { (yyval.b) = false; }
    break;

  case 266: /* kwd_let_var_or_nothing: %empty  */
                    { (yyval.b) = true; }
    break;

  case 267: /* kwd_let: "let"  */
                 { (yyval.b) = true; }
    break;

  case 268: /* kwd_let: "var"  */
                 { (yyval.b) = false; }
    break;

  case 269: /* optional_in_scope: "inscope"  */
                    { (yyval.b) = true; }
    break;

  case 270: /* optional_in_scope: %empty  */
                     { (yyval.b) = false; }
    break;

  case 271: /* tuple_expansion: "name"  */
                    {
        (yyval.pNameList) = new vector<string>();
        (yyval.pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 272: /* tuple_expansion: tuple_expansion ',' "name"  */
                                             {
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        delete (yyvsp[0].s);
        (yyval.pNameList) = (yyvsp[-2].pNameList);
    }
    break;

  case 273: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' ':' type_declaration_no_options copy_or_move_or_clone expr "end of expression"  */
                                                                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-6].pNameList),tokAt(scanner,(yylsp[-6])),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
        (yyval.pVarDecl)->isTupleExpansion = true;
    }
    break;

  case 274: /* tuple_expansion_variable_declaration: '(' tuple_expansion ')' optional_ref copy_or_move_or_clone expr "end of expression"  */
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

  case 275: /* expression_let: kwd_let optional_in_scope let_variable_declaration  */
                                                                 {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 276: /* expression_let: kwd_let optional_in_scope tuple_expansion_variable_declaration  */
                                                                             {
        (yyval.pExpression) = ast_Let(scanner,(yyvsp[-2].b),(yyvsp[-1].b),(yyvsp[0].pVarDecl),tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 277: /* $@5: %empty  */
                          { yyextra->das_arrow_depth ++; }
    break;

  case 278: /* $@6: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 279: /* expr_cast: "cast" '<' $@5 type_declaration_no_options '>' $@6 expr  */
                                                                                                                                                {
        (yyval.pExpression) = new ExprCast(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[0].pExpression)),TypeDeclPtr((yyvsp[-3].pTypeDecl)));
    }
    break;

  case 280: /* $@7: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 281: /* $@8: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 282: /* expr_cast: "upcast" '<' $@7 type_declaration_no_options '>' $@8 expr  */
                                                                                                                                                  {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[0].pExpression)),TypeDeclPtr((yyvsp[-3].pTypeDecl)));
        pCast->upcast = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 283: /* $@9: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 284: /* $@10: %empty  */
                                                                                                        { yyextra->das_arrow_depth --; }
    break;

  case 285: /* expr_cast: "reinterpret" '<' $@9 type_declaration_no_options '>' $@10 expr  */
                                                                                                                                                       {
        auto pCast = new ExprCast(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[0].pExpression)),TypeDeclPtr((yyvsp[-3].pTypeDecl)));
        pCast->reinterpret = true;
        (yyval.pExpression) = pCast;
    }
    break;

  case 286: /* $@11: %empty  */
                         { yyextra->das_arrow_depth ++; }
    break;

  case 287: /* $@12: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 288: /* expr_type_decl: "type" '<' $@11 type_declaration '>' $@12  */
                                                                                                                      {
        (yyval.pExpression) = new ExprTypeDecl(tokAt(scanner,(yylsp[-5])),TypeDeclPtr((yyvsp[-2].pTypeDecl)));
    }
    break;

  case 289: /* expr_type_info: "typeinfo" name_in_namespace '(' expr ')'  */
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

  case 290: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" '>' '(' expr ')'  */
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

  case 291: /* expr_type_info: "typeinfo" name_in_namespace '<' "name" c_or_s "name" '>' '(' expr ')'  */
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

  case 292: /* expr_list: expr  */
                      {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 293: /* expr_list: expr_list ',' expr  */
                                            {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 294: /* block_or_simple_block: expression_block  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 295: /* block_or_simple_block: "=>" expr  */
                                        {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[0].pExpression)));
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-1]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 296: /* block_or_simple_block: "=>" "<-" expr  */
                                               {
            auto retE = make_smart<ExprReturn>(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[0].pExpression)));
            retE->moveSemantics = true;
            auto blkE = new ExprBlock();
            blkE->at = tokAt(scanner,(yylsp[-2]));
            blkE->list.push_back(retE);
            (yyval.pExpression) = blkE;
    }
    break;

  case 297: /* block_or_lambda: '$'  */
                { (yyval.i) = 0;   /* block */  }
    break;

  case 298: /* block_or_lambda: '@'  */
                { (yyval.i) = 1;   /* lambda */ }
    break;

  case 299: /* block_or_lambda: '@' '@'  */
                { (yyval.i) = 2;   /* local function */ }
    break;

  case 300: /* capture_entry: '&' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_reference); delete (yyvsp[0].s); }
    break;

  case 301: /* capture_entry: '=' "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_copy); delete (yyvsp[0].s); }
    break;

  case 302: /* capture_entry: "<-" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_move); delete (yyvsp[0].s); }
    break;

  case 303: /* capture_entry: ":=" "name"  */
                                    { (yyval.pCapt) = new CaptureEntry(*(yyvsp[0].s),CaptureMode::capture_by_clone); delete (yyvsp[0].s); }
    break;

  case 304: /* capture_entry: "name" '(' "name" ')'  */
                                    { (yyval.pCapt) = ast_makeCaptureEntry(scanner,tokAt(scanner,(yylsp[-3])),*(yyvsp[-3].s),*(yyvsp[-1].s)); delete (yyvsp[-3].s); delete (yyvsp[-1].s); }
    break;

  case 305: /* capture_list: capture_entry  */
                         {
        (yyval.pCaptList) = new vector<CaptureEntry>();
        (yyval.pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
    }
    break;

  case 306: /* capture_list: capture_list ',' capture_entry  */
                                               {
        (yyvsp[-2].pCaptList)->push_back(*(yyvsp[0].pCapt));
        delete (yyvsp[0].pCapt);
        (yyval.pCaptList) = (yyvsp[-2].pCaptList);
    }
    break;

  case 307: /* optional_capture_list: %empty  */
        { (yyval.pCaptList) = nullptr; }
    break;

  case 308: /* optional_capture_list: "capture" '(' capture_list ')'  */
                                             { (yyval.pCaptList) = (yyvsp[-1].pCaptList); }
    break;

  case 309: /* expr_full_block: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type block_or_simple_block  */
                                                                                            {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 310: /* expr_full_block_assumed_piped: block_or_lambda optional_annotation_list optional_capture_list optional_function_argument_list optional_function_type expression_block  */
                                                                                       {
        (yyval.pExpression) = ast_makeBlock(scanner,(yyvsp[-5].i),(yyvsp[-4].faList),(yyvsp[-3].pCaptList),(yyvsp[-2].pVarDeclList),(yyvsp[-1].pTypeDecl),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[-4])));
    }
    break;

  case 311: /* expr_full_block_assumed_piped: "begin of code block" expressions "end of code block"  */
                                   {
        (yyval.pExpression) = ast_makeBlock(scanner,0,nullptr,nullptr,nullptr,new TypeDecl(Type::autoinfer),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-1])),tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 312: /* expr_numeric_const: "integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt(tokAt(scanner,(yylsp[0])),(int32_t)(yyvsp[0].i)); }
    break;

  case 313: /* expr_numeric_const: "unsigned integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt(tokAt(scanner,(yylsp[0])),(uint32_t)(yyvsp[0].ui)); }
    break;

  case 314: /* expr_numeric_const: "long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstInt64(tokAt(scanner,(yylsp[0])),(int64_t)(yyvsp[0].i64)); }
    break;

  case 315: /* expr_numeric_const: "unsigned long integer constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt64(tokAt(scanner,(yylsp[0])),(uint64_t)(yyvsp[0].ui64)); }
    break;

  case 316: /* expr_numeric_const: "unsigned int8 constant"  */
                                              { (yyval.pExpression) = new ExprConstUInt8(tokAt(scanner,(yylsp[0])),(uint8_t)(yyvsp[0].ui)); }
    break;

  case 317: /* expr_numeric_const: "floating point constant"  */
                                              { (yyval.pExpression) = new ExprConstFloat(tokAt(scanner,(yylsp[0])),(float)(yyvsp[0].fd)); }
    break;

  case 318: /* expr_numeric_const: "double constant"  */
                                              { (yyval.pExpression) = new ExprConstDouble(tokAt(scanner,(yylsp[0])),(double)(yyvsp[0].d)); }
    break;

  case 319: /* expr_assign: expr  */
                                             { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 320: /* expr_assign: expr '=' expr  */
                                             { (yyval.pExpression) = new ExprCopy(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 321: /* expr_assign: expr "<-" expr  */
                                             { (yyval.pExpression) = new ExprMove(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 322: /* expr_assign: expr ":=" expr  */
                                             { (yyval.pExpression) = new ExprClone(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 323: /* expr_assign: expr "&=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 324: /* expr_assign: expr "|=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 325: /* expr_assign: expr "^=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 326: /* expr_assign: expr "&&=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 327: /* expr_assign: expr "||=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 328: /* expr_assign: expr "^^=" expr  */
                                                { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 329: /* expr_assign: expr "+=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 330: /* expr_assign: expr "-=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 331: /* expr_assign: expr "*=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 332: /* expr_assign: expr "/=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 333: /* expr_assign: expr "%=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 334: /* expr_assign: expr "<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 335: /* expr_assign: expr ">>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 336: /* expr_assign: expr "<<<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 337: /* expr_assign: expr ">>>=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 338: /* expr_named_call: name_in_namespace '(' '[' make_struct_fields ']' ')'  */
                                                                         {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-5])),*(yyvsp[-5].s));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-5].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 339: /* expr_named_call: name_in_namespace '(' expr_list ',' '[' make_struct_fields ']' ')'  */
                                                                                                  {
        auto nc = new ExprNamedCall(tokAt(scanner,(yylsp[-7])),*(yyvsp[-7].s));
        nc->nonNamedArguments = sequenceToList((yyvsp[-5].pExpression));
        nc->arguments = *(yyvsp[-2].pMakeStruct);
        delete (yyvsp[-2].pMakeStruct);
        delete (yyvsp[-7].s);
        (yyval.pExpression) = nc;
    }
    break;

  case 340: /* expr_method_call: expr "->" "name" '(' ')'  */
                                                         {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 341: /* expr_method_call: expr "->" "name" '(' expr_list ')'  */
                                                                              {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 342: /* func_addr_name: name_in_namespace  */
                                    {
        (yyval.pExpression) = new ExprAddr(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 343: /* func_addr_name: "$i" '(' expr ')'  */
                                          {
        auto expr = new ExprAddr(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression), expr, "i");
    }
    break;

  case 344: /* func_addr_expr: '@' '@' func_addr_name  */
                                          {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 345: /* $@13: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 346: /* $@14: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 347: /* func_addr_expr: '@' '@' '<' $@13 type_declaration_no_options '>' $@14 func_addr_name  */
                                                                                                                                                       {
        auto expr = (ExprAddr *) ((yyvsp[0].pExpression)->rtti_isAddr() ? (yyvsp[0].pExpression) : (((ExprTag *) (yyvsp[0].pExpression))->value.get()));
        expr->funcType = TypeDeclPtr((yyvsp[-3].pTypeDecl));
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 348: /* $@15: %empty  */
                    { yyextra->das_arrow_depth ++; }
    break;

  case 349: /* $@16: %empty  */
                                                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 350: /* func_addr_expr: '@' '@' '<' $@15 optional_function_argument_list optional_function_type '>' $@16 func_addr_name  */
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

  case 351: /* expr_field: expr '.' "name"  */
                                              {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), ExpressionPtr((yyvsp[-2].pExpression)), *(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 352: /* expr_field: expr '.' '.' "name"  */
                                                  {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), ExpressionPtr((yyvsp[-3].pExpression)), *(yyvsp[0].s), true);
        delete (yyvsp[0].s);
    }
    break;

  case 353: /* expr_field: expr '.' "name" '(' ')'  */
                                                      {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), *(yyvsp[-2].s));
        delete (yyvsp[-2].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 354: /* expr_field: expr '.' "name" '(' expr_list ')'  */
                                                                           {
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), *(yyvsp[-3].s));
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        delete (yyvsp[-3].s);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 355: /* expr_field: expr '.' basic_type_declaration '(' ')'  */
                                                                        {
        auto method_name = das_to_string((yyvsp[-2].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-3])), (yyvsp[-4].pExpression), method_name);
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 356: /* expr_field: expr '.' basic_type_declaration '(' expr_list ')'  */
                                                                                             {
        auto method_name = das_to_string((yyvsp[-3].type));
        auto pInvoke = makeInvokeMethod(tokAt(scanner,(yylsp[-4])), (yyvsp[-5].pExpression), method_name);
        auto callArgs = sequenceToList((yyvsp[-1].pExpression));
        pInvoke->arguments.insert ( pInvoke->arguments.end(), callArgs.begin(), callArgs.end() );
        (yyval.pExpression) = pInvoke;
    }
    break;

  case 357: /* $@17: %empty  */
                               { yyextra->das_suppress_errors=true; }
    break;

  case 358: /* $@18: %empty  */
                                                                            { yyextra->das_suppress_errors=false; }
    break;

  case 359: /* expr_field: expr '.' $@17 error $@18  */
                                                                                                                    {
        (yyval.pExpression) = new ExprField(tokAt(scanner,(yylsp[-3])), tokAt(scanner,(yylsp[-3])), ExpressionPtr((yyvsp[-4].pExpression)), "");
        yyerrok;
    }
    break;

  case 360: /* expr_call: name_in_namespace '(' ')'  */
                                               {
            (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),*(yyvsp[-2].s));
            delete (yyvsp[-2].s);
    }
    break;

  case 361: /* expr_call: name_in_namespace '(' "uninitialized" ')'  */
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

  case 362: /* expr_call: name_in_namespace '(' make_struct_single ')'  */
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

  case 363: /* expr_call: name_in_namespace '(' "uninitialized" make_struct_single ')'  */
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

  case 364: /* expr_call: name_in_namespace '(' expr_list ')'  */
                                                                    {
            (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),*(yyvsp[-3].s)),(yyvsp[-1].pExpression));
            delete (yyvsp[-3].s);
    }
    break;

  case 365: /* expr_call: basic_type_declaration '(' ')'  */
                                                    {
        (yyval.pExpression) = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-2])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-2].type)));
    }
    break;

  case 366: /* expr_call: basic_type_declaration '(' expr_list ')'  */
                                                                         {
        (yyval.pExpression) = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[-3].type))),(yyvsp[-1].pExpression));
    }
    break;

  case 367: /* expr: "null"  */
                                              { (yyval.pExpression) = new ExprConstPtr(tokAt(scanner,(yylsp[0])),nullptr); }
    break;

  case 368: /* expr: name_in_namespace  */
                                              { (yyval.pExpression) = new ExprVar(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 369: /* expr: expr_numeric_const  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 370: /* expr: expr_reader  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 371: /* expr: string_builder  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 372: /* expr: make_decl  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 373: /* expr: "true"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),true); }
    break;

  case 374: /* expr: "false"  */
                                              { (yyval.pExpression) = new ExprConstBool(tokAt(scanner,(yylsp[0])),false); }
    break;

  case 375: /* expr: expr_field  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 376: /* expr: expr_mtag  */
                                              { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 377: /* expr: '!' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"!",ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 378: /* expr: '~' expr  */
                                              { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"~",ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 379: /* expr: '+' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"+",ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 380: /* expr: '-' expr  */
                                                  { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"-",ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 381: /* expr: expr "<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 382: /* expr: expr ">>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 383: /* expr: expr "<<<" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<<<", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 384: /* expr: expr ">>>" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">>>", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 385: /* expr: expr '+' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"+", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 386: /* expr: expr '-' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"-", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 387: /* expr: expr '*' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"*", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 388: /* expr: expr '/' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"/", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 389: /* expr: expr '%' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"%", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 390: /* expr: expr '<' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 391: /* expr: expr '>' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 392: /* expr: expr "==" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"==", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 393: /* expr: expr "!=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"!=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 394: /* expr: expr "<=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"<=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 395: /* expr: expr ">=" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),">=", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 396: /* expr: expr '&' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 397: /* expr: expr '|' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"|", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 398: /* expr: expr '^' expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 399: /* expr: expr "&&" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"&&", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 400: /* expr: expr "||" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"||", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 401: /* expr: expr "^^" expr  */
                                             { (yyval.pExpression) = new ExprOp2(tokAt(scanner,(yylsp[-1])),"^^", ExpressionPtr((yyvsp[-2].pExpression)), ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 402: /* expr: expr ".." expr  */
                                             {
        auto itv = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-1])),"interval");
        itv->arguments.push_back(ExpressionPtr((yyvsp[-2].pExpression)));
        itv->arguments.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = itv;
    }
    break;

  case 403: /* expr: "++" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"++", ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 404: /* expr: "--" expr  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[-1])),"--", ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 405: /* expr: expr "++"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"+++", ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 406: /* expr: expr "--"  */
                                                 { (yyval.pExpression) = new ExprOp1(tokAt(scanner,(yylsp[0])),"---", ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 407: /* expr: '(' expr_list optional_comma ')'  */
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

  case 408: /* expr: expr '[' expr ']'  */
                                                 { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[-3].pExpression)), ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 409: /* expr: expr '.' '[' expr ']'  */
                                                     { (yyval.pExpression) = new ExprAt(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[-4].pExpression)), ExpressionPtr((yyvsp[-1].pExpression)), true); }
    break;

  case 410: /* expr: expr "?[" expr ']'  */
                                                 { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[-3].pExpression)), ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 411: /* expr: expr '.' "?[" expr ']'  */
                                                     { (yyval.pExpression) = new ExprSafeAt(tokAt(scanner,(yylsp[-2])), ExpressionPtr((yyvsp[-4].pExpression)), ExpressionPtr((yyvsp[-1].pExpression)), true); }
    break;

  case 412: /* expr: expr "?." "name"  */
                                                 { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), ExpressionPtr((yyvsp[-2].pExpression)), *(yyvsp[0].s)); delete (yyvsp[0].s); }
    break;

  case 413: /* expr: expr '.' "?." "name"  */
                                                     { (yyval.pExpression) = new ExprSafeField(tokAt(scanner,(yylsp[-1])), tokAt(scanner,(yylsp[0])), ExpressionPtr((yyvsp[-3].pExpression)), *(yyvsp[0].s), true); delete (yyvsp[0].s); }
    break;

  case 414: /* expr: func_addr_expr  */
                                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 415: /* expr: expr_call  */
                        { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 416: /* expr: '*' expr  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 417: /* expr: "deref" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprPtr2Ref(tokAt(scanner,(yylsp[-3])),ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 418: /* expr: "addr" '(' expr ')'  */
                                                   { (yyval.pExpression) = new ExprRef2Ptr(tokAt(scanner,(yylsp[-3])),ExpressionPtr((yyvsp[-1].pExpression))); }
    break;

  case 419: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' ')'  */
                                                                                                              {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-4].pTypeDecl),(yyvsp[-2].pCaptList),nullptr,tokAt(scanner,(yylsp[-6])));
    }
    break;

  case 420: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list '(' expr ')'  */
                                                                                                                            {
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-5].pTypeDecl),(yyvsp[-3].pCaptList),(yyvsp[-1].pExpression),tokAt(scanner,(yylsp[-7])));
    }
    break;

  case 421: /* expr: "generator" '<' type_declaration_no_options '>' optional_capture_list expression_block  */
                                                                                                                              {
        auto closure = new ExprMakeBlock(tokAt(scanner,(yylsp[0])),ExpressionPtr((yyvsp[0].pExpression)));
        ((ExprBlock *)(yyvsp[0].pExpression))->returnType = make_smart<TypeDecl>(Type::autoinfer);
        (yyval.pExpression) = ast_makeGenerator(scanner,(yyvsp[-3].pTypeDecl),(yyvsp[-1].pCaptList),closure,tokAt(scanner,(yylsp[-5])));
    }
    break;

  case 422: /* expr: expr "??" expr  */
                                                   { (yyval.pExpression) = new ExprNullCoalescing(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression))); }
    break;

  case 423: /* expr: expr '?' expr ':' expr  */
                                                          {
            (yyval.pExpression) = new ExprOp3(tokAt(scanner,(yylsp[-3])),"?",ExpressionPtr((yyvsp[-4].pExpression)),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
        }
    break;

  case 424: /* $@19: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 425: /* $@20: %empty  */
                                                                                                                      { yyextra->das_arrow_depth --; }
    break;

  case 426: /* expr: expr "is" "type" '<' $@19 type_declaration_no_options '>' $@20  */
                                                                                                                                                       {
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[-7].pExpression)),TypeDeclPtr((yyvsp[-2].pTypeDecl)));
    }
    break;

  case 427: /* expr: expr "is" basic_type_declaration  */
                                                               {
        auto vdecl = new TypeDecl((yyvsp[0].type));
        vdecl->at = tokAt(scanner,(yylsp[0]));
        (yyval.pExpression) = new ExprIs(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),vdecl);
    }
    break;

  case 428: /* expr: expr "is" "name"  */
                                              {
        (yyval.pExpression) = new ExprIsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 429: /* expr: expr "as" "name"  */
                                              {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 430: /* $@21: %empty  */
                                               { yyextra->das_arrow_depth ++; }
    break;

  case 431: /* $@22: %empty  */
                                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 432: /* expr: expr "as" "type" '<' $@21 type_declaration '>' $@22  */
                                                                                                                                            {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[-7].pExpression)),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 433: /* expr: expr "as" basic_type_declaration  */
                                                               {
        (yyval.pExpression) = new ExprAsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-2].pExpression)),das_to_string((yyvsp[0].type)));
    }
    break;

  case 434: /* expr: expr '?' "as" "name"  */
                                                  {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-3].pExpression)),*(yyvsp[0].s));
        delete (yyvsp[0].s);
    }
    break;

  case 435: /* $@23: %empty  */
                                                   { yyextra->das_arrow_depth ++; }
    break;

  case 436: /* $@24: %empty  */
                                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 437: /* expr: expr '?' "as" "type" '<' $@23 type_declaration '>' $@24  */
                                                                                                                                                {
        auto vname = (yyvsp[-2].pTypeDecl)->describe();
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-6])),ExpressionPtr((yyvsp[-8].pExpression)),vname);
        delete (yyvsp[-2].pTypeDecl);
    }
    break;

  case 438: /* expr: expr '?' "as" basic_type_declaration  */
                                                                   {
        (yyval.pExpression) = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-3].pExpression)),das_to_string((yyvsp[0].type)));
    }
    break;

  case 439: /* expr: expr_type_info  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 440: /* expr: expr_type_decl  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 441: /* expr: expr_cast  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 442: /* expr: expr_new  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 443: /* expr: expr_method_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 444: /* expr: expr_named_call  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 445: /* expr: expr_full_block  */
                                                { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 446: /* expr: expr "<|" expr  */
                                                { (yyval.pExpression) = ast_lpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 447: /* expr: expr "|>" expr  */
                                                { (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-1]))); }
    break;

  case 448: /* expr: expr "|>" basic_type_declaration  */
                                                          {
        auto fncall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[0])),tokAt(scanner,(yylsp[0])),das_to_string((yyvsp[0].type)));
        (yyval.pExpression) = ast_rpipe(scanner,(yyvsp[-2].pExpression),fncall,tokAt(scanner,(yylsp[-1])));
    }
    break;

  case 449: /* expr: expr_call_pipe  */
                             { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 450: /* expr: "unsafe" '(' expr ')'  */
                                         {
            (yyvsp[-1].pExpression)->alwaysSafe = true;
            (yyvsp[-1].pExpression)->userSaidItsSafe = true;
            (yyval.pExpression) = (yyvsp[-1].pExpression);
        }
    break;

  case 451: /* expr_mtag: "$$" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"e"); }
    break;

  case 452: /* expr_mtag: "$i" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"i"); }
    break;

  case 453: /* expr_mtag: "$v" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"v"); }
    break;

  case 454: /* expr_mtag: "$b" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"b"); }
    break;

  case 455: /* expr_mtag: "$a" '(' expr ')'  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),"a"); }
    break;

  case 456: /* expr_mtag: "..."  */
                                                     { (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[0])),nullptr,"..."); }
    break;

  case 457: /* expr_mtag: "$c" '(' expr ')' '(' ')'  */
                                                            {
            auto ccall = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-5])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``");
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-5])),(yyvsp[-3].pExpression),ccall,"c");
        }
    break;

  case 458: /* expr_mtag: "$c" '(' expr ')' '(' expr_list ')'  */
                                                                                {
            auto ccall = parseFunctionArguments(yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-6])),tokAt(scanner,(yylsp[0])),"``MACRO``TAG``CALL``"),(yyvsp[-1].pExpression));
            (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-6])),(yyvsp[-4].pExpression),ccall,"c");
        }
    break;

  case 459: /* expr_mtag: expr '.' "$f" '(' expr ')'  */
                                                                {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[-5].pExpression)), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 460: /* expr_mtag: expr "?." "$f" '(' expr ')'  */
                                                                 {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[-5].pExpression)), "``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 461: /* expr_mtag: expr '.' '.' "$f" '(' expr ')'  */
                                                                    {
        auto cfield = new ExprField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[-6].pExpression)), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 462: /* expr_mtag: expr '.' "?." "$f" '(' expr ')'  */
                                                                     {
        auto cfield = new ExprSafeField(tokAt(scanner,(yylsp[-4])), tokAt(scanner,(yylsp[-1])), ExpressionPtr((yyvsp[-6].pExpression)), "``MACRO``TAG``FIELD``", true);
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 463: /* expr_mtag: expr "as" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprAsVariant(tokAt(scanner,(yylsp[-4])),ExpressionPtr((yyvsp[-5].pExpression)),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 464: /* expr_mtag: expr '?' "as" "$f" '(' expr ')'  */
                                                                       {
        auto cfield = new ExprSafeAsVariant(tokAt(scanner,(yylsp[-4])),ExpressionPtr((yyvsp[-6].pExpression)),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 465: /* expr_mtag: expr "is" "$f" '(' expr ')'  */
                                                                   {
        auto cfield = new ExprIsVariant(tokAt(scanner,(yylsp[-4])),ExpressionPtr((yyvsp[-5].pExpression)),"``MACRO``TAG``FIELD``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression),cfield,"f");
    }
    break;

  case 466: /* expr_mtag: '@' '@' "$c" '(' expr ')'  */
                                                         {
        auto ccall = new ExprAddr(tokAt(scanner,(yylsp[-4])),"``MACRO``TAG``ADDR``");
        (yyval.pExpression) = new ExprTag(tokAt(scanner,(yylsp[-3])),(yyvsp[-1].pExpression),ccall,"c");
    }
    break;

  case 467: /* optional_field_annotation: %empty  */
                                      { (yyval.aaList) = nullptr; }
    break;

  case 468: /* optional_field_annotation: metadata_argument_list  */
                                      { (yyval.aaList) = (yyvsp[0].aaList); }
    break;

  case 469: /* optional_override: %empty  */
                      { (yyval.i) = OVERRIDE_NONE; }
    break;

  case 470: /* optional_override: "override"  */
                      { (yyval.i) = OVERRIDE_OVERRIDE; }
    break;

  case 471: /* optional_override: "sealed"  */
                      { (yyval.i) = OVERRIDE_SEALED; }
    break;

  case 472: /* optional_constant: %empty  */
                        { (yyval.b) = false; }
    break;

  case 473: /* optional_constant: "const"  */
                        { (yyval.b) = true; }
    break;

  case 474: /* optional_public_or_private_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 475: /* optional_public_or_private_member_variable: "public"  */
                        { (yyval.b) = false; }
    break;

  case 476: /* optional_public_or_private_member_variable: "private"  */
                        { (yyval.b) = true; }
    break;

  case 477: /* optional_static_member_variable: %empty  */
                        { (yyval.b) = false; }
    break;

  case 478: /* optional_static_member_variable: "static"  */
                        { (yyval.b) = true; }
    break;

  case 479: /* structure_variable_declaration: optional_field_annotation optional_static_member_variable optional_override optional_public_or_private_member_variable variable_declaration  */
                                                                                                                                                                                      {
        (yyvsp[0].pVarDecl)->override = (yyvsp[-2].i) == OVERRIDE_OVERRIDE;
        (yyvsp[0].pVarDecl)->sealed = (yyvsp[-2].i) == OVERRIDE_SEALED;
        (yyvsp[0].pVarDecl)->annotation = (yyvsp[-4].aaList);
        (yyvsp[0].pVarDecl)->isPrivate = (yyvsp[-1].b);
        (yyvsp[0].pVarDecl)->isStatic = (yyvsp[-3].b);
        (yyval.pVarDecl) = (yyvsp[0].pVarDecl);
    }
    break;

  case 480: /* struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 481: /* $@25: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructureFields(tak);
        }
    }
    break;

  case 482: /* struct_variable_declaration_list: struct_variable_declaration_list $@25 structure_variable_declaration "end of expression"  */
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

  case 483: /* $@26: %empty  */
                                                                                                                     {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-2]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 484: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable "abstract" optional_constant $@26 function_declaration_header "end of expression"  */
                                                    {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[-1]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyval.pVarDeclList) = ast_structVarDefAbstract(scanner,(yyvsp[-8].pVarDeclList),(yyvsp[-7].faList),(yyvsp[-5].b),(yyvsp[-3].b), (yyvsp[-1].pFuncDecl));
            }
    break;

  case 485: /* $@27: %empty  */
                                                                                                                                                                         {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeFunction(tak);
                }
            }
    break;

  case 486: /* struct_variable_declaration_list: struct_variable_declaration_list optional_annotation_list "def" optional_public_or_private_member_variable optional_static_member_variable optional_override optional_constant $@27 function_declaration_header expression_block  */
                                                                        {
                if ( !yyextra->g_CommentReaders.empty() ) {
                    auto tak = tokAt(scanner,(yylsp[0]));
                    for ( auto & crd : yyextra->g_CommentReaders ) crd->afterFunction((yyvsp[-1].pFuncDecl),tak);
                }
                (yyval.pVarDeclList) = ast_structVarDef(scanner,(yyvsp[-9].pVarDeclList),(yyvsp[-8].faList),(yyvsp[-5].b),(yyvsp[-6].b),(yyvsp[-4].i),(yyvsp[-3].b),(yyvsp[-1].pFuncDecl),(yyvsp[0].pExpression),tokRangeAt(scanner,(yylsp[-7]),(yylsp[0])),tokAt(scanner,(yylsp[-8])));
            }
    break;

  case 487: /* function_argument_declaration: optional_field_annotation kwd_let_var_or_nothing variable_declaration  */
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

  case 488: /* function_argument_declaration: "$a" '(' expr ')'  */
                                     {
            auto na = new vector<VariableNameAndPosition>();
            na->push_back(VariableNameAndPosition{"``MACRO``TAG``","",tokAt(scanner,(yylsp[-1]))});
            auto decl = new VariableDeclaration(na, new TypeDecl(Type::none), (yyvsp[-1].pExpression));
            decl->pTypeDecl->isTag = true;
            (yyval.pVarDecl) = decl;
        }
    break;

  case 489: /* function_argument_list: function_argument_declaration  */
                                                                                 { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 490: /* function_argument_list: function_argument_list "end of expression" function_argument_declaration  */
                                                                                 { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 491: /* tuple_type: type_declaration  */
                                    {
        (yyval.pVarDecl) = new VariableDeclaration(nullptr,(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 492: /* tuple_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition{*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))});
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 493: /* tuple_type_list: tuple_type  */
                                                       { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 494: /* tuple_type_list: tuple_type_list c_or_s tuple_type  */
                                                       { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 495: /* tuple_alias_type_list: %empty  */
      {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 496: /* tuple_alias_type_list: tuple_alias_type_list c_or_s  */
                                         {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 497: /* tuple_alias_type_list: tuple_alias_type_list tuple_type c_or_s  */
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

  case 498: /* variant_type: "name" ':' type_declaration  */
                                                   {
        auto na = new vector<VariableNameAndPosition>();
        na->push_back(VariableNameAndPosition{*(yyvsp[-2].s),"",tokAt(scanner,(yylsp[-2]))});
        (yyval.pVarDecl) = new VariableDeclaration(na,(yyvsp[0].pTypeDecl),nullptr);
        delete (yyvsp[-2].s);
    }
    break;

  case 499: /* variant_type_list: variant_type  */
                                                         { (yyval.pVarDeclList) = new vector<VariableDeclaration*>(); (yyval.pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 500: /* variant_type_list: variant_type_list c_or_s variant_type  */
                                                            { (yyval.pVarDeclList) = (yyvsp[-2].pVarDeclList); (yyvsp[-2].pVarDeclList)->push_back((yyvsp[0].pVarDecl)); }
    break;

  case 501: /* variant_alias_type_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 502: /* variant_alias_type_list: variant_alias_type_list c_or_s  */
                                           {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 503: /* variant_alias_type_list: variant_alias_type_list variant_type c_or_s  */
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

  case 504: /* copy_or_move: '='  */
                    { (yyval.b) = false; }
    break;

  case 505: /* copy_or_move: "<-"  */
                    { (yyval.b) = true; }
    break;

  case 506: /* variable_declaration: variable_name_with_pos_list  */
                                          {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[0]));
        autoT->ref = false;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[0].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 507: /* variable_declaration: variable_name_with_pos_list '&'  */
                                              {
        auto autoT = new TypeDecl(Type::autoinfer);
        autoT->at = tokAt(scanner,(yylsp[-1]));
        autoT->ref = true;
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-1].pNameWithPosList),autoT,nullptr);
    }
    break;

  case 508: /* variable_declaration: variable_name_with_pos_list ':' type_declaration  */
                                                                          {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),(yyvsp[0].pTypeDecl),nullptr);
    }
    break;

  case 509: /* variable_declaration: variable_name_with_pos_list ':' type_declaration copy_or_move expr  */
                                                                                                      {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),(yyvsp[-2].pTypeDecl),(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 510: /* variable_declaration: variable_name_with_pos_list copy_or_move expr  */
                                                                       {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-2]));
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-2].pNameWithPosList),typeDecl,(yyvsp[0].pExpression));
        (yyval.pVarDecl)->init_via_move = (yyvsp[-1].b);
    }
    break;

  case 511: /* copy_or_move_or_clone: '='  */
                    { (yyval.i) = CorM_COPY; }
    break;

  case 512: /* copy_or_move_or_clone: "<-"  */
                    { (yyval.i) = CorM_MOVE; }
    break;

  case 513: /* copy_or_move_or_clone: ":="  */
                    { (yyval.i) = CorM_CLONE; }
    break;

  case 514: /* optional_ref: %empty  */
            { (yyval.b) = false; }
    break;

  case 515: /* optional_ref: '&'  */
            { (yyval.b) = true; }
    break;

  case 516: /* let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 517: /* let_variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{"``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),ExpressionPtr((yyvsp[-1].pExpression))});
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 518: /* let_variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 519: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name"  */
                                                             {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 520: /* let_variable_name_with_pos_list: let_variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition{*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))});
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 521: /* global_let_variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 522: /* global_let_variable_name_with_pos_list: global_let_variable_name_with_pos_list ',' "name"  */
                                                                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 523: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options "end of expression"  */
                                                                                            {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 524: /* let_variable_declaration: let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr "end of expression"  */
                                                                                                                                  {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 525: /* let_variable_declaration: let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr "end of expression"  */
                                                                                                          {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 526: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options "end of expression"  */
                                                                                                   {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-3].pNameWithPosList),(yyvsp[-1].pTypeDecl),nullptr);
    }
    break;

  case 527: /* global_let_variable_declaration: global_let_variable_name_with_pos_list ':' type_declaration_no_options copy_or_move_or_clone expr "end of expression"  */
                                                                                                                                         {
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-5].pNameWithPosList),(yyvsp[-3].pTypeDecl),(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 528: /* global_let_variable_declaration: global_let_variable_name_with_pos_list optional_ref copy_or_move_or_clone expr "end of expression"  */
                                                                                                                 {
        auto typeDecl = new TypeDecl(Type::autoinfer);
        typeDecl->at = tokAt(scanner,(yylsp[-4]));
        typeDecl->ref = (yyvsp[-3].b);
        (yyval.pVarDecl) = new VariableDeclaration((yyvsp[-4].pNameWithPosList),typeDecl,(yyvsp[-1].pExpression));
        (yyval.pVarDecl)->init_via_move  = ((yyvsp[-2].i) & CorM_MOVE) !=0;
        (yyval.pVarDecl)->init_via_clone = ((yyvsp[-2].i) & CorM_CLONE) !=0;
    }
    break;

  case 529: /* optional_shared: %empty  */
                     { (yyval.b) = false; }
    break;

  case 530: /* optional_shared: "shared"  */
                     { (yyval.b) = true; }
    break;

  case 531: /* optional_public_or_private_variable: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 532: /* optional_public_or_private_variable: "private"  */
                     { (yyval.b) = false; }
    break;

  case 533: /* optional_public_or_private_variable: "public"  */
                     { (yyval.b) = true; }
    break;

  case 534: /* $@28: %empty  */
                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeGlobalVariables(tak);
        }
    }
    break;

  case 535: /* global_let: kwd_let optional_shared optional_public_or_private_variable $@28 optional_field_annotation global_let_variable_declaration  */
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

  case 536: /* enum_expression: "name"  */
                   {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 537: /* enum_expression: "name" '=' expr  */
                                   {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        (yyval.pEnumPair) = new EnumPair((yyvsp[-2].s),(yyvsp[0].pExpression),tokAt(scanner,(yylsp[-2])));
    }
    break;

  case 538: /* enum_list: %empty  */
        {
        (yyval.pEnum) = new Enumeration();
    }
    break;

  case 539: /* enum_list: enum_expression  */
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

  case 540: /* enum_list: enum_list ',' enum_expression  */
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

  case 541: /* optional_public_or_private_alias: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 542: /* optional_public_or_private_alias: "private"  */
                     { (yyval.b) = false; }
    break;

  case 543: /* optional_public_or_private_alias: "public"  */
                     { (yyval.b) = true; }
    break;

  case 544: /* $@29: %empty  */
                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeAlias(pubename);
        }
    }
    break;

  case 545: /* single_alias: optional_public_or_private_alias "name" $@29 '=' type_declaration  */
                                  {
        das_checkName(scanner,*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])));
        (yyvsp[0].pTypeDecl)->isPrivateAlias = !(yyvsp[-4].b);
        if ( (yyvsp[0].pTypeDecl)->baseType == Type::alias ) {
            das2_yyerror(scanner,"alias cannot be defined in terms of another alias "+*(yyvsp[-3].s),tokAt(scanner,(yylsp[-3])),
                CompilationError::invalid_type);
        }
        (yyvsp[0].pTypeDecl)->alias = *(yyvsp[-3].s);
        if ( !yyextra->g_Program->addAlias(TypeDeclPtr((yyvsp[0].pTypeDecl))) ) {
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

  case 547: /* optional_public_or_private_enum: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 548: /* optional_public_or_private_enum: "private"  */
                     { (yyval.b) = false; }
    break;

  case 549: /* optional_public_or_private_enum: "public"  */
                     { (yyval.b) = true; }
    break;

  case 550: /* enum_name: "name"  */
                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumeration(pubename);
        }
        (yyval.pEnum) = ast_addEmptyEnum(scanner, (yyvsp[0].s), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 551: /* optional_enum_basic_type_declaration: %empty  */
        {
        (yyval.type) = Type::tInt;
    }
    break;

  case 552: /* optional_enum_basic_type_declaration: ':' enum_basic_type_declaration  */
                                              {
        (yyval.type) = (yyvsp[0].type);
    }
    break;

  case 553: /* $@30: %empty  */
                                                                                                                                                          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeEnumerationEntries(tak);
        }
    }
    break;

  case 554: /* $@31: %empty  */
                                   {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumerationEntries(tak);
        }
    }
    break;

  case 555: /* enum_declaration: optional_annotation_list "enum" optional_public_or_private_enum enum_name optional_enum_basic_type_declaration "begin of code block" $@30 enum_list optional_comma $@31 "end of code block"  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto pubename = tokAt(scanner,(yylsp[-3]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterEnumeration((yyvsp[-7].pEnum)->name.c_str(),pubename);
        }
        ast_enumDeclaration(scanner,(yyvsp[-10].faList),tokAt(scanner,(yylsp[-10])),(yyvsp[-8].b),(yyvsp[-7].pEnum),(yyvsp[-3].pEnum),(yyvsp[-6].type));
    }
    break;

  case 556: /* optional_structure_parent: %empty  */
                                        { (yyval.s) = nullptr; }
    break;

  case 557: /* optional_structure_parent: ':' name_in_namespace  */
                                        { (yyval.s) = (yyvsp[0].s); }
    break;

  case 558: /* optional_sealed: %empty  */
                        { (yyval.b) = false; }
    break;

  case 559: /* optional_sealed: "sealed"  */
                        { (yyval.b) = true; }
    break;

  case 560: /* structure_name: optional_sealed "name" optional_structure_parent  */
                                                                           {
        (yyval.pStructure) = ast_structureName(scanner,(yyvsp[-2].b),(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])),(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
    }
    break;

  case 561: /* class_or_struct: "class"  */
                    { (yyval.b) = true; }
    break;

  case 562: /* class_or_struct: "struct"  */
                    { (yyval.b) = false; }
    break;

  case 563: /* optional_public_or_private_structure: %empty  */
                     { (yyval.b) = yyextra->g_Program->thisModule->isPublic; }
    break;

  case 564: /* optional_public_or_private_structure: "private"  */
                     { (yyval.b) = false; }
    break;

  case 565: /* optional_public_or_private_structure: "public"  */
                     { (yyval.b) = true; }
    break;

  case 566: /* optional_struct_variable_declaration_list: %empty  */
        {
        (yyval.pVarDeclList) = new vector<VariableDeclaration*>();
    }
    break;

  case 567: /* optional_struct_variable_declaration_list: "begin of code block" struct_variable_declaration_list "end of code block"  */
                                                       {
        (yyval.pVarDeclList) = (yyvsp[-1].pVarDeclList);
    }
    break;

  case 568: /* $@32: %empty  */
                                                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto tak = tokAt(scanner,(yylsp[-1]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeStructure(tak);
        }
    }
    break;

  case 569: /* $@33: %empty  */
                         { if ( (yyvsp[0].pStructure) ) { (yyvsp[0].pStructure)->isClass = (yyvsp[-3].b); (yyvsp[0].pStructure)->privateStructure = !(yyvsp[-2].b); } }
    break;

  case 570: /* structure_declaration: optional_annotation_list class_or_struct optional_public_or_private_structure $@32 structure_name $@33 optional_struct_variable_declaration_list  */
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

  case 571: /* variable_name_with_pos_list: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 572: /* variable_name_with_pos_list: "$i" '(' expr ')'  */
                                     {
        auto pSL = new vector<VariableNameAndPosition>();
        pSL->push_back(VariableNameAndPosition{"``MACRO``TAG``","",tokAt(scanner,(yylsp[-1])),(yyvsp[-1].pExpression)});
        (yyval.pNameWithPosList) = pSL;
    }
    break;

  case 573: /* variable_name_with_pos_list: "name" "aka" "name"  */
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

  case 574: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name"  */
                                                         {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameWithPosList)->push_back(VariableNameAndPosition{*(yyvsp[0].s),"",tokAt(scanner,(yylsp[0]))});
        (yyval.pNameWithPosList) = (yyvsp[-2].pNameWithPosList);
        delete (yyvsp[0].s);
    }
    break;

  case 575: /* variable_name_with_pos_list: variable_name_with_pos_list ',' "name" "aka" "name"  */
                                                                               {
        das_checkName(scanner,*(yyvsp[-2].s),tokAt(scanner,(yylsp[-2])));
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-4].pNameWithPosList)->push_back(VariableNameAndPosition{*(yyvsp[-2].s),*(yyvsp[0].s),tokAt(scanner,(yylsp[-2]))});
        (yyval.pNameWithPosList) = (yyvsp[-4].pNameWithPosList);
        delete (yyvsp[-2].s);
        delete (yyvsp[0].s);
    }
    break;

  case 576: /* basic_type_declaration: "bool"  */
                        { (yyval.type) = Type::tBool; }
    break;

  case 577: /* basic_type_declaration: "string"  */
                        { (yyval.type) = Type::tString; }
    break;

  case 578: /* basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 579: /* basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 580: /* basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 581: /* basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 582: /* basic_type_declaration: "int2"  */
                        { (yyval.type) = Type::tInt2; }
    break;

  case 583: /* basic_type_declaration: "int3"  */
                        { (yyval.type) = Type::tInt3; }
    break;

  case 584: /* basic_type_declaration: "int4"  */
                        { (yyval.type) = Type::tInt4; }
    break;

  case 585: /* basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 586: /* basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 587: /* basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 588: /* basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 589: /* basic_type_declaration: "uint2"  */
                        { (yyval.type) = Type::tUInt2; }
    break;

  case 590: /* basic_type_declaration: "uint3"  */
                        { (yyval.type) = Type::tUInt3; }
    break;

  case 591: /* basic_type_declaration: "uint4"  */
                        { (yyval.type) = Type::tUInt4; }
    break;

  case 592: /* basic_type_declaration: "float"  */
                        { (yyval.type) = Type::tFloat; }
    break;

  case 593: /* basic_type_declaration: "float2"  */
                        { (yyval.type) = Type::tFloat2; }
    break;

  case 594: /* basic_type_declaration: "float3"  */
                        { (yyval.type) = Type::tFloat3; }
    break;

  case 595: /* basic_type_declaration: "float4"  */
                        { (yyval.type) = Type::tFloat4; }
    break;

  case 596: /* basic_type_declaration: "void"  */
                        { (yyval.type) = Type::tVoid; }
    break;

  case 597: /* basic_type_declaration: "range"  */
                        { (yyval.type) = Type::tRange; }
    break;

  case 598: /* basic_type_declaration: "urange"  */
                        { (yyval.type) = Type::tURange; }
    break;

  case 599: /* basic_type_declaration: "range64"  */
                        { (yyval.type) = Type::tRange64; }
    break;

  case 600: /* basic_type_declaration: "urange64"  */
                        { (yyval.type) = Type::tURange64; }
    break;

  case 601: /* basic_type_declaration: "double"  */
                        { (yyval.type) = Type::tDouble; }
    break;

  case 602: /* basic_type_declaration: "bitfield"  */
                        { (yyval.type) = Type::tBitfield; }
    break;

  case 603: /* enum_basic_type_declaration: "int"  */
                        { (yyval.type) = Type::tInt; }
    break;

  case 604: /* enum_basic_type_declaration: "int8"  */
                        { (yyval.type) = Type::tInt8; }
    break;

  case 605: /* enum_basic_type_declaration: "int16"  */
                        { (yyval.type) = Type::tInt16; }
    break;

  case 606: /* enum_basic_type_declaration: "uint"  */
                        { (yyval.type) = Type::tUInt; }
    break;

  case 607: /* enum_basic_type_declaration: "uint8"  */
                        { (yyval.type) = Type::tUInt8; }
    break;

  case 608: /* enum_basic_type_declaration: "uint16"  */
                        { (yyval.type) = Type::tUInt16; }
    break;

  case 609: /* enum_basic_type_declaration: "int64"  */
                        { (yyval.type) = Type::tInt64; }
    break;

  case 610: /* enum_basic_type_declaration: "uint64"  */
                        { (yyval.type) = Type::tUInt64; }
    break;

  case 611: /* structure_type_declaration: name_in_namespace  */
                                 {
        (yyval.pTypeDecl) = yyextra->g_Program->makeTypeDeclaration(tokAt(scanner,(yylsp[0])),*(yyvsp[0].s));
        if ( !(yyval.pTypeDecl) ) {
            (yyval.pTypeDecl) = new TypeDecl(Type::tVoid);
            (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
        }
        delete (yyvsp[0].s);
    }
    break;

  case 612: /* auto_type_declaration: "auto"  */
                       {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 613: /* auto_type_declaration: "auto" '(' "name" ')'  */
                                            {
        das_checkName(scanner,*(yyvsp[-1].s),tokAt(scanner,(yylsp[-1])));
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-3]));
        (yyval.pTypeDecl)->alias = *(yyvsp[-1].s);
        delete (yyvsp[-1].s);
    }
    break;

  case 614: /* auto_type_declaration: "$t" '(' expr ')'  */
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

  case 615: /* bitfield_bits: "name"  */
                    {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        auto pSL = new vector<string>();
        pSL->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = pSL;
        delete (yyvsp[0].s);
    }
    break;

  case 616: /* bitfield_bits: bitfield_bits "end of expression" "name"  */
                                           {
        das_checkName(scanner,*(yyvsp[0].s),tokAt(scanner,(yylsp[0])));
        (yyvsp[-2].pNameList)->push_back(*(yyvsp[0].s));
        (yyval.pNameList) = (yyvsp[-2].pNameList);
        delete (yyvsp[0].s);
    }
    break;

  case 617: /* bitfield_alias_bits: %empty  */
        {
        auto pSL = new vector<string>();
        (yyval.pNameList) = pSL;

    }
    break;

  case 618: /* bitfield_alias_bits: "name"  */
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

  case 619: /* bitfield_alias_bits: bitfield_alias_bits ',' "name"  */
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

  case 620: /* $@34: %empty  */
                                     { yyextra->das_arrow_depth ++; }
    break;

  case 621: /* $@35: %empty  */
                                                                                            { yyextra->das_arrow_depth --; }
    break;

  case 622: /* bitfield_type_declaration: "bitfield" '<' $@34 bitfield_bits '>' $@35  */
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

  case 625: /* table_type_pair: type_declaration  */
                                      {
        (yyval.aTypePair).firstType = (yyvsp[0].pTypeDecl);
        (yyval.aTypePair).secondType = new TypeDecl(Type::tVoid);
    }
    break;

  case 626: /* table_type_pair: type_declaration c_or_s type_declaration  */
                                                                             {
        (yyval.aTypePair).firstType = (yyvsp[-2].pTypeDecl);
        (yyval.aTypePair).secondType = (yyvsp[0].pTypeDecl);
    }
    break;

  case 627: /* dim_list: '[' expr ']'  */
                             {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 628: /* dim_list: '[' ']'  */
                {
        (yyval.pTypeDecl) = new TypeDecl(Type::autoinfer);
        appendDimExpr((yyval.pTypeDecl), nullptr);
    }
    break;

  case 629: /* dim_list: dim_list '[' expr ']'  */
                                            {
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), (yyvsp[-1].pExpression));
    }
    break;

  case 630: /* dim_list: dim_list '[' ']'  */
                              {
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
        appendDimExpr((yyval.pTypeDecl), nullptr);
    }
    break;

  case 631: /* type_declaration_no_options: type_declaration_no_options_no_dim  */
                                                     {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 632: /* type_declaration_no_options: type_declaration_no_options_no_dim dim_list  */
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

  case 633: /* type_declaration_no_options_no_dim: basic_type_declaration  */
                                                            { (yyval.pTypeDecl) = new TypeDecl((yyvsp[0].type)); (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0])); }
    break;

  case 634: /* type_declaration_no_options_no_dim: auto_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 635: /* type_declaration_no_options_no_dim: bitfield_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 636: /* type_declaration_no_options_no_dim: structure_type_declaration  */
                                                            { (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl); }
    break;

  case 637: /* $@36: %empty  */
                     { yyextra->das_arrow_depth ++; }
    break;

  case 638: /* $@37: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 639: /* type_declaration_no_options_no_dim: "type" '<' $@36 type_declaration '>' $@37  */
                                                                                                                      {
        (yyvsp[-2].pTypeDecl)->autoToAlias = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 640: /* type_declaration_no_options_no_dim: "typedecl" '(' expr ')'  */
                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeDecl);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]),(yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr.push_back((yyvsp[-1].pExpression));
    }
    break;

  case 641: /* type_declaration_no_options_no_dim: '$' name_in_namespace '(' optional_expr_list ')'  */
                                                                          {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-3]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = sequenceToList((yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-3])), *(yyvsp[-3].s)));
        delete (yyvsp[-3].s);
    }
    break;

  case 642: /* $@38: %empty  */
                                        { yyextra->das_arrow_depth ++; }
    break;

  case 643: /* type_declaration_no_options_no_dim: '$' name_in_namespace '<' $@38 type_declaration_no_options_list '>' '(' optional_expr_list ')'  */
                                                                                                                                                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::typeMacro);
        (yyval.pTypeDecl)->at = tokRangeAt(scanner,(yylsp[-7]), (yylsp[-1]));
        (yyval.pTypeDecl)->dimExpr = typesAndSequenceToList((yyvsp[-4].pTypeDeclList),(yyvsp[-1].pExpression));
        (yyval.pTypeDecl)->dimExpr.insert((yyval.pTypeDecl)->dimExpr.begin(), new ExprConstString(tokAt(scanner,(yylsp[-7])), *(yyvsp[-7].s)));
        delete (yyvsp[-7].s);
    }
    break;

  case 644: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim '-' '[' ']'  */
                                                                 {
        (yyvsp[-3].pTypeDecl)->removeDim = true;
        (yyval.pTypeDecl) = (yyvsp[-3].pTypeDecl);
    }
    break;

  case 645: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim "explicit"  */
                                                                  {
        (yyvsp[-1].pTypeDecl)->isExplicit = true;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 646: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim "const"  */
                                                               {
        (yyvsp[-1].pTypeDecl)->constant = true;
        (yyvsp[-1].pTypeDecl)->removeConstant = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 647: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim '-' "const"  */
                                                                   {
        (yyvsp[-2].pTypeDecl)->constant = false;
        (yyvsp[-2].pTypeDecl)->removeConstant = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 648: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim '&'  */
                                                         {
        (yyvsp[-1].pTypeDecl)->ref = true;
        (yyvsp[-1].pTypeDecl)->removeRef = false;
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
    }
    break;

  case 649: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim '-' '&'  */
                                                             {
        (yyvsp[-2].pTypeDecl)->ref = false;
        (yyvsp[-2].pTypeDecl)->removeRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 650: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim '#'  */
                                                         {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->temporary = true;
    }
    break;

  case 651: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim "implicit"  */
                                                                  {
        (yyval.pTypeDecl) = (yyvsp[-1].pTypeDecl);
        (yyval.pTypeDecl)->implicit = true;
    }
    break;

  case 652: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim '-' '#'  */
                                                             {
        (yyvsp[-2].pTypeDecl)->temporary = false;
        (yyvsp[-2].pTypeDecl)->removeTemporary = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 653: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim "==" "const"  */
                                                                      {
        (yyvsp[-2].pTypeDecl)->explicitConst = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 654: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim "==" '&'  */
                                                                {
        (yyvsp[-2].pTypeDecl)->explicitRef = true;
        (yyval.pTypeDecl) = (yyvsp[-2].pTypeDecl);
    }
    break;

  case 655: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim '?'  */
                                                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-1].pTypeDecl));
    }
    break;

  case 656: /* $@39: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 657: /* $@40: %empty  */
                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 658: /* type_declaration_no_options_no_dim: "smart_ptr" '<' $@39 type_declaration '>' $@40  */
                                                                                                                                {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->smartPtr = true;
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 659: /* type_declaration_no_options_no_dim: type_declaration_no_options_no_dim "??"  */
                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tPointer);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType = make_smart<TypeDecl>(Type::tPointer);
        (yyval.pTypeDecl)->firstType->at = tokAt(scanner,(yylsp[-1]));
        (yyval.pTypeDecl)->firstType->firstType = TypeDeclPtr((yyvsp[-1].pTypeDecl));
    }
    break;

  case 660: /* $@41: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 661: /* $@42: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 662: /* type_declaration_no_options_no_dim: "array" '<' $@41 type_declaration '>' $@42  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tArray);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 663: /* $@43: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 664: /* $@44: %empty  */
                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 665: /* type_declaration_no_options_no_dim: "table" '<' $@43 table_type_pair '>' $@44  */
                                                                                                                      {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTable);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].aTypePair).firstType);
        (yyval.pTypeDecl)->secondType = TypeDeclPtr((yyvsp[-2].aTypePair).secondType);
    }
    break;

  case 666: /* $@45: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 667: /* $@46: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 668: /* type_declaration_no_options_no_dim: "iterator" '<' $@45 type_declaration '>' $@46  */
                                                                                                                                  {
        (yyval.pTypeDecl) = new TypeDecl(Type::tIterator);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 669: /* type_declaration_no_options_no_dim: "block"  */
                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 670: /* $@47: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 671: /* $@48: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 672: /* type_declaration_no_options_no_dim: "block" '<' $@47 type_declaration '>' $@48  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tBlock);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 673: /* $@49: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 674: /* $@50: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 675: /* type_declaration_no_options_no_dim: "block" '<' $@49 optional_function_argument_list optional_function_type '>' $@50  */
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

  case 676: /* type_declaration_no_options_no_dim: "function"  */
                           {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 677: /* $@51: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 678: /* $@52: %empty  */
                                                                                                { yyextra->das_arrow_depth --; }
    break;

  case 679: /* type_declaration_no_options_no_dim: "function" '<' $@51 type_declaration '>' $@52  */
                                                                                                                                 {
        (yyval.pTypeDecl) = new TypeDecl(Type::tFunction);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 680: /* $@53: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 681: /* $@54: %empty  */
                                                                                                                                         { yyextra->das_arrow_depth --; }
    break;

  case 682: /* type_declaration_no_options_no_dim: "function" '<' $@53 optional_function_argument_list optional_function_type '>' $@54  */
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

  case 683: /* type_declaration_no_options_no_dim: "lambda"  */
                         {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[0]));
    }
    break;

  case 684: /* $@55: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 685: /* $@56: %empty  */
                                                                                              { yyextra->das_arrow_depth --; }
    break;

  case 686: /* type_declaration_no_options_no_dim: "lambda" '<' $@55 type_declaration '>' $@56  */
                                                                                                                               {
        (yyval.pTypeDecl) = new TypeDecl(Type::tLambda);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        (yyval.pTypeDecl)->firstType = TypeDeclPtr((yyvsp[-2].pTypeDecl));
    }
    break;

  case 687: /* $@57: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 688: /* $@58: %empty  */
                                                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 689: /* type_declaration_no_options_no_dim: "lambda" '<' $@57 optional_function_argument_list optional_function_type '>' $@58  */
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

  case 690: /* $@59: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 691: /* $@60: %empty  */
                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 692: /* type_declaration_no_options_no_dim: "tuple" '<' $@59 tuple_type_list '>' $@60  */
                                                                                                                        {
        (yyval.pTypeDecl) = new TypeDecl(Type::tTuple);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 693: /* $@61: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 694: /* $@62: %empty  */
                                                                                           { yyextra->das_arrow_depth --; }
    break;

  case 695: /* type_declaration_no_options_no_dim: "variant" '<' $@61 variant_type_list '>' $@62  */
                                                                                                                            {
        (yyval.pTypeDecl) = new TypeDecl(Type::tVariant);
        (yyval.pTypeDecl)->at = tokAt(scanner,(yylsp[-5]));
        varDeclToTypeDecl(scanner, (yyval.pTypeDecl), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
    }
    break;

  case 696: /* type_declaration: type_declaration_no_options  */
                                        {
        (yyval.pTypeDecl) = (yyvsp[0].pTypeDecl);
    }
    break;

  case 697: /* type_declaration: type_declaration '|' type_declaration_no_options  */
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

  case 698: /* type_declaration: type_declaration '|' '#'  */
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

  case 699: /* $@63: %empty  */
                                                                      {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTuple(atvname);
        }
    }
    break;

  case 700: /* $@64: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeTupleEntries(atvname);
        }
    }
    break;

  case 701: /* $@65: %empty  */
                                  {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTupleEntries(atvname);
        }
    }
    break;

  case 702: /* tuple_alias_declaration: "tuple" optional_public_or_private_alias "name" $@63 "begin of code block" $@64 tuple_alias_type_list $@65 "end of code block"  */
          {
        auto vtype = make_smart<TypeDecl>(Type::tTuple);
        vtype->alias = *(yyvsp[-6].s);
        vtype->at = tokAt(scanner,(yylsp[-6]));
        vtype->isPrivateAlias = !(yyvsp[-7].b);
        varDeclToTypeDecl(scanner, vtype.get(), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-6].s),tokAt(scanner,(yylsp[-6])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterTuple((yyvsp[-6].s)->c_str(),atvname);
        }
        delete (yyvsp[-6].s);
    }
    break;

  case 703: /* $@66: %empty  */
                                                                        {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariant(atvname);
        }
    }
    break;

  case 704: /* $@67: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeVariantEntries(atvname);
        }

    }
    break;

  case 705: /* $@68: %empty  */
                                    {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-4]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariantEntries(atvname);
        }
    }
    break;

  case 706: /* variant_alias_declaration: "variant" optional_public_or_private_alias "name" $@66 "begin of code block" $@67 variant_alias_type_list $@68 "end of code block"  */
          {
        auto vtype = make_smart<TypeDecl>(Type::tVariant);
        vtype->alias = *(yyvsp[-6].s);
        vtype->at = tokAt(scanner,(yylsp[-6]));
        vtype->isPrivateAlias = !(yyvsp[-7].b);
        varDeclToTypeDecl(scanner, vtype.get(), (yyvsp[-2].pVarDeclList), true);
        deleteVariableDeclarationList((yyvsp[-2].pVarDeclList));
        if ( !yyextra->g_Program->addAlias(vtype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-6].s),tokAt(scanner,(yylsp[-6])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-6]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterVariant((yyvsp[-6].s)->c_str(),atvname);
        }
        delete (yyvsp[-6].s);
    }
    break;

  case 707: /* $@69: %empty  */
                                                                         {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[0]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfield(atvname);
        }
    }
    break;

  case 708: /* $@70: %empty  */
          {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-2]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->beforeBitfieldEntries(atvname);
        }
    }
    break;

  case 709: /* $@71: %empty  */
                                               {
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-5]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfieldEntries(atvname);
        }
    }
    break;

  case 710: /* bitfield_alias_declaration: "bitfield" optional_public_or_private_alias "name" $@69 "begin of code block" $@70 bitfield_alias_bits optional_comma $@71 "end of code block"  */
          {
        auto btype = make_smart<TypeDecl>(Type::tBitfield);
        btype->alias = *(yyvsp[-7].s);
        btype->at = tokAt(scanner,(yylsp[-7]));
        btype->argNames = *(yyvsp[-3].pNameList);
        btype->isPrivateAlias = !(yyvsp[-8].b);
        if ( btype->argNames.size()>32 ) {
            das2_yyerror(scanner,"only 32 different bits are allowed in a bitfield",tokAt(scanner,(yylsp[-7])),
                CompilationError::invalid_type);
        }
        if ( !yyextra->g_Program->addAlias(btype) ) {
            das2_yyerror(scanner,"type alias is already defined "+*(yyvsp[-7].s),tokAt(scanner,(yylsp[-7])),
                CompilationError::type_alias_already_declared);
        }
        if ( !yyextra->g_CommentReaders.empty() ) {
            auto atvname = tokAt(scanner,(yylsp[-7]));
            for ( auto & crd : yyextra->g_CommentReaders ) crd->afterBitfield((yyvsp[-7].s)->c_str(),atvname);
        }
        delete (yyvsp[-7].s);
        delete (yyvsp[-3].pNameList);
    }
    break;

  case 711: /* make_decl: make_struct_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 712: /* make_decl: make_dim_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 713: /* make_decl: make_table_decl  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 714: /* make_decl: array_comprehension  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 715: /* make_decl: make_tuple_call  */
                                 { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 716: /* make_struct_fields: "name" copy_or_move expr  */
                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),ExpressionPtr((yyvsp[0].pExpression)),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 717: /* make_struct_fields: "name" ":=" expr  */
                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),ExpressionPtr((yyvsp[0].pExpression)),false,true);
        delete (yyvsp[-2].s);
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 718: /* make_struct_fields: make_struct_fields ',' "name" copy_or_move expr  */
                                                                           {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),ExpressionPtr((yyvsp[0].pExpression)),(yyvsp[-1].b),false);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 719: /* make_struct_fields: make_struct_fields ',' "name" ":=" expr  */
                                                                  {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-2])),*(yyvsp[-2].s),ExpressionPtr((yyvsp[0].pExpression)),false,true);
        delete (yyvsp[-2].s);
        ((MakeStruct *)(yyvsp[-4].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-4].pMakeStruct);
    }
    break;

  case 720: /* make_struct_fields: "$f" '(' expr ')' copy_or_move expr  */
                                                                   {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",ExpressionPtr((yyvsp[0].pExpression)),(yyvsp[-1].b),false);
        mfd->tag = ExpressionPtr((yyvsp[-3].pExpression));
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 721: /* make_struct_fields: "$f" '(' expr ')' ":=" expr  */
                                                          {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",ExpressionPtr((yyvsp[0].pExpression)),false,true);
        mfd->tag = ExpressionPtr((yyvsp[-3].pExpression));
        auto msd = new MakeStruct();
        msd->push_back(mfd);
        (yyval.pMakeStruct) = msd;
    }
    break;

  case 722: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' copy_or_move expr  */
                                                                                               {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",ExpressionPtr((yyvsp[0].pExpression)),(yyvsp[-1].b),false);
        mfd->tag = ExpressionPtr((yyvsp[-3].pExpression));
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 723: /* make_struct_fields: make_struct_fields ',' "$f" '(' expr ')' ":=" expr  */
                                                                                      {
        auto mfd = make_smart<MakeFieldDecl>(tokAt(scanner,(yylsp[-3])),"``MACRO``TAG``FIELD``",ExpressionPtr((yyvsp[0].pExpression)),false,true);
        mfd->tag = ExpressionPtr((yyvsp[-3].pExpression));
        ((MakeStruct *)(yyvsp[-7].pMakeStruct))->push_back(mfd);
        (yyval.pMakeStruct) = (yyvsp[-7].pMakeStruct);
    }
    break;

  case 724: /* make_variant_dim: make_struct_fields  */
                                {
        (yyval.pExpression) = ast_makeStructToMakeVariant((yyvsp[0].pMakeStruct), tokAt(scanner,(yylsp[0])));
    }
    break;

  case 725: /* make_struct_single: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 726: /* make_struct_dim_list: '(' make_struct_fields ')'  */
                                        {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 727: /* make_struct_dim_list: make_struct_dim_list ',' '(' make_struct_fields ')'  */
                                                                     {
        ((ExprMakeStruct *) (yyvsp[-4].pExpression))->structs.push_back(MakeStructPtr((yyvsp[-1].pMakeStruct)));
        (yyval.pExpression) = (yyvsp[-4].pExpression);
    }
    break;

  case 728: /* make_struct_dim_decl: make_struct_fields  */
                                {
        auto msd = new ExprMakeStruct();
        msd->structs.push_back(MakeStructPtr((yyvsp[0].pMakeStruct)));
        (yyval.pExpression) = msd;
    }
    break;

  case 729: /* make_struct_dim_decl: make_struct_dim_list optional_comma  */
                                                 {
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 730: /* optional_make_struct_dim_decl: make_struct_dim_decl  */
                                  { (yyval.pExpression) = (yyvsp[0].pExpression);  }
    break;

  case 731: /* optional_make_struct_dim_decl: %empty  */
        {   (yyval.pExpression) = new ExprMakeStruct(); }
    break;

  case 732: /* use_initializer: %empty  */
                            { (yyval.b) = true; }
    break;

  case 733: /* use_initializer: "uninitialized"  */
                            { (yyval.b) = false; }
    break;

  case 734: /* $@72: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 735: /* $@73: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 736: /* make_struct_decl: "struct" '<' $@72 type_declaration_no_options '>' $@73 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                      {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceStruct = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->alwaysUseInitializer = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 737: /* $@74: %empty  */
                            { yyextra->das_arrow_depth ++; }
    break;

  case 738: /* $@75: %empty  */
                                                                                                  { yyextra->das_arrow_depth --; }
    break;

  case 739: /* make_struct_decl: "class" '<' $@74 type_declaration_no_options '>' $@75 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                     {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceClass = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 740: /* $@76: %empty  */
                               { yyextra->das_arrow_depth ++; }
    break;

  case 741: /* $@77: %empty  */
                                                                                                     { yyextra->das_arrow_depth --; }
    break;

  case 742: /* make_struct_decl: "variant" '<' $@76 type_declaration_no_options '>' $@77 '(' make_variant_dim ')'  */
                                                                                                                                                                     {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-8]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-5].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = true;
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceVariant = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 743: /* $@78: %empty  */
                              { yyextra->das_arrow_depth ++; }
    break;

  case 744: /* $@79: %empty  */
                                                                                                    { yyextra->das_arrow_depth --; }
    break;

  case 745: /* make_struct_decl: "default" '<' $@78 type_declaration_no_options '>' $@79 use_initializer  */
                                                                                                                                                           {
        auto msd = new ExprMakeStruct();
        msd->at = tokAt(scanner,(yylsp[-6]));
        msd->makeType = TypeDeclPtr((yyvsp[-3].pTypeDecl));
        msd->useInitializer = (yyvsp[0].b);
        msd->alwaysUseInitializer = true;
        (yyval.pExpression) = msd;
    }
    break;

  case 746: /* make_map_tuple: expr "=>" expr  */
                                         {
        ExprMakeTuple * mt = new ExprMakeTuple(tokAt(scanner,(yylsp[-1])));
        mt->values.push_back(ExpressionPtr((yyvsp[-2].pExpression)));
        mt->values.push_back(ExpressionPtr((yyvsp[0].pExpression)));
        (yyval.pExpression) = mt;
    }
    break;

  case 747: /* make_map_tuple: expr  */
                 {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 748: /* make_tuple_call: "tuple" '(' expr_list optional_comma ')'  */
                                                                    {
        auto mkt = new ExprMakeTuple(tokAt(scanner,(yylsp[-4])));
        mkt->values = sequenceToList((yyvsp[-2].pExpression));
        mkt->makeType = make_smart<TypeDecl>(Type::autoinfer);
        (yyval.pExpression) = mkt;
    }
    break;

  case 749: /* $@80: %empty  */
                             { yyextra->das_arrow_depth ++; }
    break;

  case 750: /* $@81: %empty  */
                                                                                                   { yyextra->das_arrow_depth --; }
    break;

  case 751: /* make_tuple_call: "tuple" '<' $@80 type_declaration_no_options '>' $@81 '(' use_initializer optional_make_struct_dim_decl ')'  */
                                                                                                                                                                                                      {
        (yyvsp[-1].pExpression)->at = tokAt(scanner,(yylsp[-9]));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->useInitializer = (yyvsp[-2].b);
        ((ExprMakeStruct *)(yyvsp[-1].pExpression))->forceTuple = true;
        (yyval.pExpression) = (yyvsp[-1].pExpression);
    }
    break;

  case 752: /* make_dim_decl: '[' expr_list optional_comma ']'  */
                                                          {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_array_move");
        tam->arguments.push_back(mka);
        (yyval.pExpression) = tam;
    }
    break;

  case 753: /* $@82: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 754: /* $@83: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 755: /* make_dim_decl: "array" "struct" '<' $@82 type_declaration_no_options '>' $@83 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 756: /* $@84: %empty  */
                                       { yyextra->das_arrow_depth ++; }
    break;

  case 757: /* $@85: %empty  */
                                                                                                             { yyextra->das_arrow_depth --; }
    break;

  case 758: /* make_dim_decl: "array" "tuple" '<' $@84 type_declaration_no_options '>' $@85 '(' use_initializer optional_make_struct_dim_decl ')'  */
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

  case 759: /* $@86: %empty  */
                                         { yyextra->das_arrow_depth ++; }
    break;

  case 760: /* $@87: %empty  */
                                                                                                               { yyextra->das_arrow_depth --; }
    break;

  case 761: /* make_dim_decl: "array" "variant" '<' $@86 type_declaration_no_options '>' $@87 '(' make_variant_dim ')'  */
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

  case 762: /* make_dim_decl: "array" '(' expr_list optional_comma ')'  */
                                                                   {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        auto tam = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_array_move");
        tam->arguments.push_back(mka);
        (yyval.pExpression) = tam;
    }
    break;

  case 763: /* $@88: %empty  */
                           { yyextra->das_arrow_depth ++; }
    break;

  case 764: /* $@89: %empty  */
                                                                                                 { yyextra->das_arrow_depth --; }
    break;

  case 765: /* make_dim_decl: "array" '<' $@88 type_declaration_no_options '>' $@89 '(' optional_expr_list ')'  */
                                                                                                                                                                        {
        if ( (yyvsp[-1].pExpression) ) {
            auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-8])));
            mka->values = sequenceToList((yyvsp[-1].pExpression));
            mka->makeType = TypeDeclPtr((yyvsp[-5].pTypeDecl));
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

  case 766: /* make_dim_decl: "fixed_array" '(' expr_list optional_comma ')'  */
                                                                         {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        mka->makeType->dim.push_back(TypeDecl::dimAuto);
        (yyval.pExpression) = mka;
    }
    break;

  case 767: /* $@90: %empty  */
                                 { yyextra->das_arrow_depth ++; }
    break;

  case 768: /* $@91: %empty  */
                                                                                                       { yyextra->das_arrow_depth --; }
    break;

  case 769: /* make_dim_decl: "fixed_array" '<' $@90 type_declaration_no_options '>' $@91 '(' expr_list optional_comma ')'  */
                                                                                                                                                                                    {
        auto mka = new ExprMakeArray(tokAt(scanner,(yylsp[-9])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = TypeDeclPtr((yyvsp[-6].pTypeDecl));
        if ( !mka->makeType->dim.size() ) mka->makeType->dim.push_back(TypeDecl::dimAuto);
        (yyval.pExpression) = mka;
    }
    break;

  case 770: /* expr_map_tuple_list: make_map_tuple  */
                                {
        (yyval.pExpression) = (yyvsp[0].pExpression);
    }
    break;

  case 771: /* expr_map_tuple_list: expr_map_tuple_list ',' make_map_tuple  */
                                                                {
            (yyval.pExpression) = new ExprSequence(tokAt(scanner,(yylsp[-2])),ExpressionPtr((yyvsp[-2].pExpression)),ExpressionPtr((yyvsp[0].pExpression)));
    }
    break;

  case 772: /* make_table_decl: "begin of code block" expr_map_tuple_list optional_comma "end of code block"  */
                                                                    {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-3])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-3])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 773: /* make_table_decl: "table" '(' expr_map_tuple_list optional_comma ')'  */
                                                                             {
        auto mka = make_smart<ExprMakeArray>(tokAt(scanner,(yylsp[-4])));
        mka->values = sequenceToList((yyvsp[-2].pExpression));
        mka->makeType = make_smart<TypeDecl>(Type::autoinfer);
        auto ttm = yyextra->g_Program->makeCall(tokAt(scanner,(yylsp[-4])),"to_table_move");
        ttm->arguments.push_back(mka);
        (yyval.pExpression) = ttm;
    }
    break;

  case 774: /* make_table_decl: "table" '<' type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 775: /* make_table_decl: "table" '<' type_declaration_no_options c_or_s type_declaration_no_options '>' '(' optional_expr_map_tuple_list ')'  */
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

  case 776: /* array_comprehension_where: %empty  */
                                    { (yyval.pExpression) = nullptr; }
    break;

  case 777: /* array_comprehension_where: "end of expression" "where" expr  */
                                    { (yyval.pExpression) = (yyvsp[0].pExpression); }
    break;

  case 778: /* optional_comma: %empty  */
                { (yyval.b) = false; }
    break;

  case 779: /* optional_comma: ','  */
                { (yyval.b) = true; }
    break;

  case 780: /* array_comprehension: '[' "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                    {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,false);
    }
    break;

  case 781: /* array_comprehension: '[' "iterator" "for" variable_name_with_pos_list "in" expr_list "end of expression" expr array_comprehension_where ']'  */
                                                                                                                                                                 {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),true,false);
    }
    break;

  case 782: /* array_comprehension: "begin of code block" "for" variable_name_with_pos_list "in" expr_list "end of expression" make_map_tuple array_comprehension_where "end of code block"  */
                                                                                                                                                              {
        (yyval.pExpression) = ast_arrayComprehension(scanner,tokAt(scanner,(yylsp[-7])),(yyvsp[-6].pNameWithPosList),(yyvsp[-4].pExpression),(yyvsp[-2].pExpression),(yyvsp[-1].pExpression),tokRangeAt(scanner,(yylsp[-2]),(yylsp[0])),false,true);
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


