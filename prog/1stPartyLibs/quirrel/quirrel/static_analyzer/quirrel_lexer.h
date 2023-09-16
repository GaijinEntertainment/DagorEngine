#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <stdint.h>

class CompilationContext;


#define TOKEN_TYPES \
  TOKEN_TYPE(TK_EMPTY, "") \
  TOKEN_TYPE(TK_EOF, "[END OF FILE]") \
  TOKEN_TYPE(TK_IDENTIFIER, "[IDENTIFIER]") \
  TOKEN_TYPE(TK_STRING_LITERAL, "[STRING LITERAL]") \
  TOKEN_TYPE(TK_INTEGER, "[INTEGER]") \
  TOKEN_TYPE(TK_FLOAT, "[FLOAT]") \
  TOKEN_TYPE(TK_ASSIGN, "=") \
  TOKEN_TYPE(TK_INEXPR_ASSIGNMENT, ":=") \
  TOKEN_TYPE(TK_COMMA, ",") \
  TOKEN_TYPE(TK_DOT, ".") \
  TOKEN_TYPE(TK_PLUS, "+") \
  TOKEN_TYPE(TK_MINUS, "-") \
  TOKEN_TYPE(TK_MUL, "*") \
  TOKEN_TYPE(TK_DIV, "/") \
  TOKEN_TYPE(TK_NOT, "!") \
  TOKEN_TYPE(TK_INV, "~") \
  TOKEN_TYPE(TK_BITXOR, "^") \
  TOKEN_TYPE(TK_BITOR, "|") \
  TOKEN_TYPE(TK_BITAND, "&") \
  TOKEN_TYPE(TK_AT, "@") \
  TOKEN_TYPE(TK_DOUBLE_COLON, "::") \
  TOKEN_TYPE(TK_PLUSPLUS, "++") \
  TOKEN_TYPE(TK_MINUSMINUS, "--") \
  TOKEN_TYPE(TK_LBRACE, "{") \
  TOKEN_TYPE(TK_RBRACE, "}") \
  TOKEN_TYPE(TK_LSQUARE, "[") \
  TOKEN_TYPE(TK_RSQUARE, "]") \
  TOKEN_TYPE(TK_LPAREN, "(") \
  TOKEN_TYPE(TK_RPAREN, ")") \
  TOKEN_TYPE(TK_LS, "<") \
  TOKEN_TYPE(TK_GT, ">") \
  TOKEN_TYPE(TK_EQ, "==") \
  TOKEN_TYPE(TK_NE, "!=") \
  TOKEN_TYPE(TK_LE, "<=") \
  TOKEN_TYPE(TK_GE, ">=") \
  TOKEN_TYPE(TK_AND, "&&") \
  TOKEN_TYPE(TK_OR, "||") \
  TOKEN_TYPE(TK_NEWSLOT, "<-") \
  TOKEN_TYPE(TK_MODULO, "%") \
  TOKEN_TYPE(TK_PLUSEQ, "+=") \
  TOKEN_TYPE(TK_MINUSEQ, "-=") \
  TOKEN_TYPE(TK_MULEQ, "*=") \
  TOKEN_TYPE(TK_DIVEQ, "/=") \
  TOKEN_TYPE(TK_MODEQ, "%=") \
  TOKEN_TYPE(TK_SHIFTL, "<<") \
  TOKEN_TYPE(TK_SHIFTR, ">>") \
  TOKEN_TYPE(TK_USHIFTR, ">>>") \
  TOKEN_TYPE(TK_3WAYSCMP, "<=>") \
  TOKEN_TYPE(TK_VARPARAMS, "...") \
  TOKEN_TYPE(TK_QMARK, "?") \
  TOKEN_TYPE(TK_COLON, ":") \
  TOKEN_TYPE(TK_SEMICOLON, ";") \
  TOKEN_TYPE(TK_NULLCOALESCE, "??") \
  TOKEN_TYPE(TK_NULLGETSTR, "?.") \
  TOKEN_TYPE(TK_NULLGETOBJ, "?[") \
  TOKEN_TYPE(TK_NULLCALL, "?(") \
  TOKEN_TYPE(TK_NULL, "null") \
  TOKEN_TYPE(TK_TRUE, "true") \
  TOKEN_TYPE(TK_FALSE, "false") \
  TOKEN_TYPE(TK_BASE, "base") \
  TOKEN_TYPE(TK_DELETE, "delete") \
  TOKEN_TYPE(TK_SWITCH, "switch") \
  TOKEN_TYPE(TK_IF, "if") \
  TOKEN_TYPE(TK_ELSE, "else") \
  TOKEN_TYPE(TK_FOR, "for") \
  TOKEN_TYPE(TK_FOREACH, "foreach") \
  TOKEN_TYPE(TK_WHILE, "while") \
  TOKEN_TYPE(TK_DO, "do") \
  TOKEN_TYPE(TK_BREAK, "break") \
  TOKEN_TYPE(TK_IN, "in") \
  TOKEN_TYPE(TK_LOCAL, "local") \
  TOKEN_TYPE(TK_LET, "let") \
  TOKEN_TYPE(TK_CLONE, "clone") \
  TOKEN_TYPE(TK_FUNCTION, "function") \
  TOKEN_TYPE(TK_RETURN, "return") \
  TOKEN_TYPE(TK_TYPEOF, "typeof") \
  TOKEN_TYPE(TK_CONTINUE, "continue") \
  TOKEN_TYPE(TK_YIELD, "yield") \
  TOKEN_TYPE(TK_TRY, "try") \
  TOKEN_TYPE(TK_CATCH, "catch") \
  TOKEN_TYPE(TK_THROW, "throw") \
  TOKEN_TYPE(TK_RESUME, "resume") \
  TOKEN_TYPE(TK_CASE, "case") \
  TOKEN_TYPE(TK_DEFAULT, "default") \
  TOKEN_TYPE(TK_THIS, "this") \
  TOKEN_TYPE(TK_CLASS, "class") \
  TOKEN_TYPE(TK_EXTENDS, "extends") \
  TOKEN_TYPE(TK_CONSTRUCTOR, "constructor") \
  TOKEN_TYPE(TK_INSTANCEOF, "instanceof") \
  TOKEN_TYPE(TK___LINE__, "__LINE__") \
  TOKEN_TYPE(TK___FILE__, "__FILE__") \
  TOKEN_TYPE(TK_STATIC, "static") \
  TOKEN_TYPE(TK_ENUM, "enum") \
  TOKEN_TYPE(TK_CONST, "const") \
  TOKEN_TYPE(TK_GLOBAL, "global") \
  TOKEN_TYPE(TK_READER_MACRO, "[READER MACRO]") \
  TOKEN_TYPE(TK_NOTTXT, "not") \
  TOKEN_TYPE(TK_NOTIN, "[NOT IN]") \


#define TOKEN_TYPE(x, y) x,
enum TokenType
{
  TOKEN_TYPES
  TOKEN_TYPE_COUNT
};
#undef TOKEN_TYPE

extern const char * token_type_names[];


enum TokenFlags
{
  TF_NEXT_EOL = 1 << 0,             // end of line after this token
  TF_NEXT_SPACE = 1 << 1,           // space after this token
  TF_DONT_WARN_UNUSED_VAR = 1 << 2, // don't show warning 'unused variable' for this token
};


struct Token
{
  TokenType type;
  char flags;
  unsigned short column;
  int line;
  union U
  {
    double d;
    uint64_t i;
    const char * s;
  } u;

  bool nextEol() const { return !!(flags & TF_NEXT_EOL); }
  bool nextSpace() const { return !!(flags & TF_NEXT_SPACE); }
  bool dontWarnUnusedVar() const { return !!(flags & TF_DONT_WARN_UNUSED_VAR); }
};


// token index -> comments before this token
typedef std::unordered_map<int, std::vector<std::string>> FileComments;


class Lexer
{
  const std::string & s; // code
  std::unordered_map<const char *, TokenType> tokenIdentStringToType;
  std::vector<std::string> currentComments;

  int curLine;
  int curColumn;
  int index;
  bool insideComment;
  bool insideString;
  bool insideRawString;
  bool isReaderMacro;

  void initializeTokenMaps();
  int nextChar();
  int fetchChar(int offset = 0);
  bool isSpaceOrTab(int c);
  bool isBeginOfIdent(int c);
  bool isContinueOfIdent(int c);
  bool parseStringLiteral(bool raw_string, int open_char);
  void onCompilerDirective(const std::string & directive);
  std::string expandReaderMacro(const char * str, int & out_macro_length);
  void addCurrentComments();

public:
  CompilationContext & ctx;
  std::vector<Token> tokens;
  FileComments comments; // used only when --include-comments specified

  Lexer(CompilationContext & compiler_context);
  Lexer(CompilationContext & compiler_context, const std::string & code);
  bool process();
  void print();
  CompilationContext & getCompilationContext() { return ctx; }
};

extern const char * token_strings[];
