from pygments.lexer import RegexLexer, include, bygroups, words
from pygments.token import *
from pygments.token import (
    Text,
    Comment,
    String,
    Number,
    Name,
    Keyword,
    Operator,
    Punctuation,
    Generic
)

from sphinx.highlighting import lexers

class DascriptLexer(RegexLexer):
    """
    Pygments lexer for DaScript.
    """

    name = "Dascript"
    aliases = ["daslang", "das", "dascript"]
    filenames = ["*.das", "*.da"]

    keywords = (
        "struct", "class", "let", "def", "while", "if",
        "static_if", "else", "for", "recover", "true", "false",
        "new", "typeinfo", "type", "in", "is", "as",
        "elif", "static_elif", "array", "return", "null", "break",
        "try", "options", "table", "expect", "const", "require",
        "operator", "enum", "finally", "delete", "deref", "aka",
        "typedef", "with", "cast", "override", "abstract", "upcast",
        "iterator", "var", "addr", "continue", "where", "pass",
        "reinterpret", "module", "public", "label", "goto", "implicit",
        "shared", "private", "smart_ptr", "generator", "yield", "unsafe",
        "assume", "explicit", "sealed", "static", "inscope", "fixed_array",
        "typedecl", "capture", "default", "uninitialized", "template",
    )

    types = (
        "bool", "void", "string", "auto", "int", "int2",
        "int3", "int4", "uint", "bitfield", "uint2", "uint3",
        "uint4", "float", "float2", "float3", "float4", "range",
        "urange", "block", "int64", "uint64", "double", "function",
        "lambda", "int8", "uint8", "int16", "uint16", "tuple",
        "variant", "range64", "urange64",
    )

    operators = (
        '+=', '-=', '/=', '*=', '%=', '|=', '^=', '<<', '>>',
        '++', '--', '<=', '<<=', '>>=', '>=', '==', '!=',
        '->', '<-', '??', '?.', '?[', '<|', '|>',
        ':=', '<<<', '>>>', '<<<=', '>>>=', '=>', '+', '@@',
        '-', '*', '/', '%', '&', '|', '^', '>', '<', '!', '~',
        '&&', '||', '^^', '&&=', '||=', '^^=',
    )

    tokens = {

        "root": [
            include("whitespace"),
            include("comments"),
            include("strings"),
            include("numbers"),

            # Function definitions
            (r"(def)(\s+)([A-Za-z][A-Za-z0-9_]*)", bygroups(Keyword, Text, Name.Function)),

            # Decorators / attributes
            (r"\[([A-Za-z_][A-Za-z0-9_]*)(\((.*?)\))?\]", Name.Function),

            # Variable/type declarations
            (r"([A-Za-z][A-Za-z0-9_]*)(\s*:\s*)([A-Za-z][A-Za-z0-9_]*)", bygroups(Name.Variable, Punctuation, Keyword.Type)),

            # Keywords and types
            (words(keywords, suffix=r"\b"), Keyword),
            (words(types, suffix=r"\b"), Keyword.Type),

            # Other significant tokens: multi-character tokens
            (r"::|\[\[|\]\]|\[\{|\}\]", Name.Tag),

            # Other significant tokens: single-character tokens
            (r"[{}\[\].,:;'\"@$#=?()]", Name.Tag),

            # Operators
            (words(operators, prefix=r"", suffix=r""), String.Escape),

            # Identifiers
            (r"[A-Za-z][A-Za-z0-9_]*", Name),

        ],

        "whitespace": [
            (r"\s+", Text),
        ],

        "comments": [
            (r"//.*?$", Comment.Single),
            (r"/\*", Comment.Multiline, "comment"),
        ],
        "comment": [
            (r"[^*/]+", Comment.Multiline),
            (r"/\*", Comment.Multiline, "#push"),
            (r"\*/", Comment.Multiline, "#pop"),
            (r"[*/]", Comment.Multiline),
        ],

        "strings": [
            (r'@"[^"]*"', String),            # verbatim string
            (r'"(\\\\|\\"|[^"])*"', String),  # normal string
            (r"'(\\.|[^'])'", String.Char),   # char literal
        ],

        "numbers": [
            (r"0x[0-9A-Fa-f]+(ul|l|u8|u16|u32|u64|d|lf)?", Number.Hex),
            (r"[0-9]+\.[0-9]*(e[+-]?[0-9]+)?(d|lf)?", Number.Float),
            (r"[0-9]+e[+-]?[0-9]+(d|lf)?", Number.Float),
            (r"[0-9]+(ul|l|u8|u16|u32|u64)?", Number.Integer),
        ],
    }

# Register lexer in Sphinx
lexers["das"] = DascriptLexer()
lexers["dascript"] = DascriptLexer()
lexers["daslang"] = DascriptLexer()
