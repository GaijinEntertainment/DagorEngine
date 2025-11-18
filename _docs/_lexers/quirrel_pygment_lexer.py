"""
    pygments.lexers.quirrel
    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    Lexers for Quirrel.
    :copyright: Copyright 2022 by the Gaijin.
    :license: BSD, see LICENSE for details.
"""

import re

from pygments.lexer import bygroups, combined, default, do_insertions, include, \
    inherit, Lexer, RegexLexer, this, using, words
from pygments.token import Text, Comment, Operator, Keyword, Name, String, \
    Number, Punctuation, Other, Generic, Whitespace
from pygments.util import get_bool_opt
import pygments.unistring as uni
from sphinx.highlighting import lexers

__all__ = ['QuirrelLexer']

SQ_IDENT_START = ('(?:[$_' + uni.combine('Lu', 'Ll', 'Lt', 'Lm', 'Lo', 'Nl') +
                  ']|\\\\u[a-fA-F0-9]{4})')
SQ_IDENT_PART = ('(?:[$' + uni.combine('Lu', 'Ll', 'Lt', 'Lm', 'Lo', 'Nl',
                                       'Mn', 'Mc', 'Nd', 'Pc') +
                 '\u200c\u200d]|\\\\u[a-fA-F0-9]{4})')
SQ_IDENT = SQ_IDENT_START + '(?:' + SQ_IDENT_PART + ')*'

line_re = re.compile('.*?\n')

class QuirrelLexer(RegexLexer):
    """
    For Quirrel source code.
    """

    name = 'Quirrel'
    url = 'https://quirrel.io'
    aliases = ['javascript', 'js']
    filenames = ['*.nut']
    mimetypes = ['application/quirrel', 'application/squirrel']

    flags = re.DOTALL | re.MULTILINE

    tokens = {
        'commentsandwhitespace': [
            (r'\s+', Whitespace),
            (r'<!--', Comment),
            (r'//.*?$', Comment.Single),
            (r'/\*.*?\*/', Comment.Multiline)
        ],
        'slashstartsregex': [
            include('commentsandwhitespace'),
            (r'/(\\.|[^[/\\\n]|\[(\\.|[^\]\\\n])*])+/'
             r'([gimuysd]+\b|\B)', String.Regex, '#pop'),
            (r'(?=/)', Text, ('#pop', 'badregex')),
            default('#pop')
        ],
        'badregex': [
            (r'\n', Whitespace, '#pop')
        ],
        'root': [
            (r'^(?=\s|/|<!--)', Text, 'slashstartsregex'),
            include('commentsandwhitespace'),

            # Lambda shorthand expressions like @() or @(v)
            (r'@(?=\()', Operator),

            # Numeric literals
            (r'0[bB][01]+n?', Number.Bin),
            (r'0[oO]?[0-7]+n?', Number.Oct),  # Browsers support "0o7" and "07" (< ES5) notations
            (r'0[xX][0-9a-fA-F]+n?', Number.Hex),
            (r'[0-9]+n', Number.Integer),  # Javascript BigInt requires an "n" postfix
            # Javascript doesn't have actual integer literals, so every other
            # numeric literal is handled by the regex below (including "normal")
            # integers
            (r'(\.[0-9]+|[0-9]+\.[0-9]*|[0-9]+)([eE][-+]?[0-9]+)?', Number.Float),

            (r'\.\.\.|=>', Punctuation),
            (r'\+\+|--|~|\?\?=?|\?|:|\\(?=\n)|'
             r'(<<|>>>?|==?|!=?|(?:\*\*|\|\||&&|[-<>+*%&|^/]))=?', Operator, 'slashstartsregex'),
            (r'[{(\[;,]', Punctuation, 'slashstartsregex'),
            (r'[})\].]', Punctuation),

            (r'(typeof|instanceof|in|not in|delete)\b', Operator.Word, 'slashstartsregex'),

            # Match stuff like: constructor
            (r'\b(constructor|from|import)\b', Keyword.Reserved),

            (r'(foreach|for|in|while|do|break|return|continue|switch|case|default|if|else|'
             r'throw|try|catch|yield|this|of|static|'
             r'import|extends|base)\b', Keyword, 'slashstartsregex'),
            (r'(local|let|const|function|class)\b', Keyword.Declaration, 'slashstartsregex'),

#            (r'(static)\b', Keyword.Reserved),
            (r'(true|false|null)\b', Keyword.Constant),

            (r'(require|print|println|assert|type)\b', Name.Builtin),

            (r'((?:Eval|Internal|Range|Reference|Syntax|Type|URI)?Error)\b', Name.Exception),

            # Match stuff like: super(argument, list)
#            (r'(super)(\s*)(\([\w,?.$\s]+\s*\))',
#             bygroups(Keyword, Whitespace), 'slashstartsregex'),
            # Match stuff like: function() {...}
            (r'([a-zA-Z_?.$][\w?.$]*)(?=\(\) \{)', Name.Other, 'slashstartsregex'),

            (SQ_IDENT, Name.Other),
            (r'"(\\\\|\\[^\\]|[^"\\])*"', String.Double),
            (r"'(\\\\|\\[^\\]|[^'\\])*'", String.Single),
            (r'`', String.Backtick, 'interp'),
        ],
        'interp': [
            (r'`', String.Backtick, '#pop'),
            (r'\\.', String.Backtick),
            (r'\$\{', String.Interpol, 'interp-inside'),
            (r'\$', String.Backtick),
            (r'[^`\\$]+', String.Backtick),
        ],
        'interp-inside': [
            # TODO: should this include single-line comments and allow nesting strings?
            (r'\}', String.Interpol, '#pop'),
            include('root'),
        ],
    }

lexers['sq'] = QuirrelLexer(startinline=True)
lexers['quirrel'] = QuirrelLexer(startinline=True)

def setup(app):
    return {
        'version': 'builtin',
        'env_version': 1,
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }

