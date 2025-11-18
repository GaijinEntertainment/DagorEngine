from pygments.lexer import RegexLexer, bygroups
from pygments.token import (
    Text,
    Comment,
    String,
    Number,
    Name,
    Keyword,
    Literal,
    Generic,
    Error,
    Operator,
    Punctuation,
)

from sphinx.highlighting import lexers

class BlkLexer(RegexLexer):
    """
    Advanced lexer for .blk configuration files.

    Features:
      - Distinguishes between block entities (node{...}) and directives (include "...").
      - Supports typed attributes: :t=, :p4=, :b=, :r=.
      - Supports both single-line (//) and multi-line (/* ... */) comments.
      - Matrix definitions [[ ... ]].
      - '=' appears only inside the four typed suffixes.
    """

    name = "blk"
    aliases = ["blk"]
    filenames = ["*.blk"]

    tokens = {
        "root": [
            # --- COMMENTS ---
            (r"//.*$", Generic.Emph),
            (r"/\*[\s\S]*?\*/", Generic.Emph),

            # --- WHITESPACE ---
            (r"\s+", Text),

            # --- DIRECTIVES ---
            (r"\b(?:include|override|delete)\b", Literal.String.Escape),

            # --- BLOCK NAME '{' ---
            (r"([a-zA-Z_][a-zA-Z0-9_]*)\s*(?=\{)", String.Escape),

            # --- BOOLEAN VALUES ---
            (r"\b(?:yes|no|true|false|on|off)\b", String.Interpol),

            # --- TYPES (:t=, :p4=, etc.) ---
            (r":(?:t|b|c|r|m|p2|p3|p4|i|ip2|ip3|<type>|type)=", Name.Tag),

            # --- PARAMETER NAME ---
            (r"[a-zA-Z_][a-zA-Z0-9_]*", Name.Constant),

            # --- BRACES ---
            (r"[{}]", Name.Entity),
            (r"\[\[", Name.Entity),
            (r"[\[\]]", Name.Entity),

            # --- STRINGS ---
            (r'"[^"\\]*(?:\\.[^"\\]*)*"', Name.Function),
            (r"'[^'\\]*(?:\\.[^'\\]*)*'", Name.Function),

            # --- NUMBERS ---
            (r"-?\d+\.\d+", Number),
            (r"-?\d+", Number),

            # --- PUNCTUATION ---
            (r"[;,]", Generic.Prompt),

            # --- EVERYTHING ELSE ---
            (r"[^{}\[\];,\n]+", Text),
        ],

        # --- MATRIX CONTEXT [[ ... ]] ---
        "matrix": [
            (r"\]\]", Keyword.Type, "#pop"),
            (r"[\[\],;]", Punctuation),
            (r"-?\d+\.\d+", Number),
            (r"-?\d+", Number),
            (r"\s+", Text),
        ],
    }

lexers["blk"] = BlkLexer()
