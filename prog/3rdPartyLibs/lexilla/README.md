# Lexilla

Lexilla is the lexer library for the Scintilla editing component, providing
syntax highlighting lexers used together with the vendored `scintilla`
library (see `prog/3rdPartyLibs/scintilla`).

Only the public API headers (`Lexilla.h`, `SciLexer.h`) are vendored here;
Dagor links against the Lexilla lexer library at runtime/build time.

Upstream: Lexilla, https://github.com/ScintillaOrg/lexilla
Vendored version: approximately 5.4.6 (SciLexer.h includes SCLEX_SINEX,
the last lexer added in that release).
