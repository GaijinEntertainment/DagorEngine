#ifdef _MSC_VER
// sscanf is safe enough for this case
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include "compilation_context.h"
#include <limits.h>
#include <string.h>
#include <algorithm>


bool is_utf8_bom(const char * ptr, int i)
{
  return (uint8_t)ptr[i] == 0xEF && (uint8_t)ptr[i + 1] == 0xBB && (uint8_t)ptr[i + 2] == 0xBF;
}


#define STRINGIFY_MACRO(name) #name
#define TOKEN_TYPE(x, y) STRINGIFY_MACRO(x) ,
const char * token_type_names[] =
{
  TOKEN_TYPES
  ""
};
#undef TOKEN_TYPE

#define TOKEN_TYPE(x, y) y,
const char * token_strings[] =
{
  TOKEN_TYPES
};
#undef TOKEN_TYPE


void Lexer::initializeTokenMaps()
{
  if (!tokenIdentStringToType.empty())
    return;

  for (int i = 0; i < int(TOKEN_TYPE_COUNT); i++)
    if ((token_strings[i][0] >= 'a' && token_strings[i][0] <= 'z') || token_strings[i][0] == '_')
    {
      auto it = ctx.stringList.insert(token_strings[i]);
      const char * ptrToTokenStr = it.first->c_str();
      tokenIdentStringToType.insert(std::make_pair(ptrToTokenStr, TokenType(i)));
    }
}


int Lexer::nextChar()
{
  if (index >= int(s.length()))
    return 0;

  if (index > 0 && (s[index - 1] == 0x0a || (s[index - 1] == 0x0d && s[index] != 0x0a)))
  {
    if ((index > 1 && isSpaceOrTab(s[index - 2])) || (index > 2 && s[index - 2] == '\r' && isSpaceOrTab(s[index - 3])))
      if (curLine >= ctx.firstLineAfterImport)
        ctx.warning("space-at-eol", curLine, curColumn - 1);

    curColumn = 1;
    curLine++;
  }
  else
    curColumn++;

  int ch = uint8_t(s[index++]);
  return ch;
}

int Lexer::fetchChar(int offset)
{
  if (index + offset >= int(s.length()) || index + offset < 0)
    return 0;

  int ch = uint8_t(s[index + offset]);
  return ch;
}

bool Lexer::isSpaceOrTab(int c)
{
  return c == ' ' || c == '\t';
}

bool Lexer::isBeginOfIdent(int c)
{
  return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Lexer::isContinueOfIdent(int c)
{
  return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}


Lexer::Lexer(CompilationContext & compiler_context) :
  ctx(compiler_context),
  isReaderMacro(false),
  s(compiler_context.code)
{
  initializeTokenMaps();
}

Lexer::Lexer(CompilationContext & compiler_context, const std::string & code) :
  ctx(compiler_context),
  isReaderMacro(false),
  s(code)
{
  initializeTokenMaps();
}

void Lexer::print()
{
  for (int i = 0; i < int(tokens.size()); i++)
  {
    int line = 0;
    int col = 0;
   // offsetToLineAndCol(tokens[i].offset, line, col);
    const char * s = nullptr;
    char buf[64];
    if (tokens[i].type == TK_INTEGER)
      snprintf(buf, 64, "%llu", (long long unsigned int)tokens[i].u.i);
    if (tokens[i].type == TK_FLOAT)
      snprintf(buf, 64, "%g", tokens[i].u.d);

    fprintf(out_stream, "%d:%d %s = '%s' \n", line, col, token_type_names[int(tokens[i].type)],
      tokens[i].type == TK_INTEGER || tokens[i].type == TK_FLOAT ? buf : tokens[i].u.s);
  }
}

void Lexer::addCurrentComments()
{
  if (!currentComments.empty())
    comments.emplace(std::make_pair(int(tokens.size() - 1), currentComments));
  currentComments.clear();
}


std::string Lexer::expandReaderMacro(const char * str, int & out_macro_length)
{
  const char * start = str;
  char prevChar = '"';
  char curChar = 0;
  out_macro_length = 0;

  std::string macroParams;
  std::string macroStr;

  macroStr.push_back('"');

  int depth = 0;
  int braceCount = 0;
  int paramCount = 0;
  bool insideStr1 = false;
  bool insideStr2 = false;

  while (*str)
  {
    prevChar = curChar;
    curChar = *str;
    str++;

    if (curChar == '\n' || curChar == '\r')
    {
      ctx.error(162, "new line inside interpolated string", curLine, curColumn + int(str - start) + 1);
      return std::string();
    }

    if (prevChar != '\\')
    {
      if (!insideStr1 && !insideStr2)
      {
        if (depth > 0 && prevChar == '/' && (curChar == '/' || curChar == '*'))
        {
          ctx.error(160, "comments inside interpolated string are not supported", curLine, curColumn + int(str - start));
          return std::string();
        }

        if (curChar == '{')
        {
          if (!depth)
          {
            if (macroParams.size())
              macroParams.push_back(',');

            macroParams.push_back('(');
            macroParams += "#apos:";
            macroParams += std::to_string(int(str - start) + 1);
            macroParams += " ";
            depth++;
            continue;
          }
          depth++;
        }

        if (curChar == '}')
        {
          depth--;

          if ((braceCount != 0 && depth == 0) || depth < 0)
          {
            ctx.error(161, "error in brace order", curLine, curColumn + int(str - start));
            break;
          }

          if (!depth)
          {
            macroParams.push_back(')');
            macroStr.push_back('{');
            char buf[16] = { 0 };
            snprintf(buf, sizeof(buf), "%d", paramCount);
            macroStr += buf;
            paramCount++;
          }
        }

        if (depth > 0)
        {
          if (curChar == '(' || curChar == '[')
            braceCount++;
          if (curChar == ')' || curChar == ']')
            braceCount--;
        }
      }

      if (depth > 0)
      {
        if (curChar == '"' && !insideStr1)
          insideStr2 = !insideStr2;

        if (curChar == '\'' && !insideStr2)
          insideStr1 = !insideStr1;
      }
    }
    else //  prevChar == '\'
    {
      if (curChar == '{' || curChar == '}')
      {
        if (depth == 0)
          macroStr.pop_back();
        else
          macroParams.pop_back();
      }
    }

    if (depth == 0)
      macroStr.push_back(curChar);
    else
      macroParams.push_back(curChar);

    if (depth <= 0 && curChar == '"' && prevChar != '\\' && !insideStr1 && !insideStr2)
      break;

    if (curChar == '\\' && prevChar == '\\')
      curChar = '\0';
  }

  if (macroParams.size() != 0)
  {
    macroStr += ".subst(";

    for (size_t i = 0; i < macroParams.size(); i++)
      macroStr.push_back(macroParams[i]);

    macroStr.push_back(')');
  }
  macroStr += "#apos:";
  macroStr += std::to_string(curColumn + int(str - start));
  macroStr += " ";

  curColumn = curColumn + int(str - start);
  out_macro_length = int(str - start);

  return macroStr;
}

bool Lexer::parseStringLiteral(bool raw_string, int open_char)
{
  std::string tok;
  insideString = true;
  int beginColumn = curColumn;
  int beginLine = curLine;
  int ch = 0;

  for (ch = nextChar(); ch > 0 && !ctx.isError; ch = nextChar())
  {
    if (ch == open_char)
    {
      if (raw_string && fetchChar() == '\"')
      {
        ch = nextChar();
      }
      else
      {
        insideString = false;
        break;
      }
    }
    else if (ch == 0x0d || ch == 0x0a)
    {
      if (!raw_string)
      {
        ctx.error(101, "end of line inside string literal", curLine, curColumn);
        insideString = false;
        return false;
      }

      if (ch == 0x0d && fetchChar() == 0x0a)
      {
        nextChar();
        ch = '\n';
      }
    }
    else if (ch == '\\' && !raw_string)
    {
      ch = nextChar();
      switch (ch)
      {
      case '\\': ch = '\\'; break;
      case 'a': ch = '\a'; break;
      case 'b': ch = '\b'; break;
      case 'n': ch = '\n'; break;
      case 'r': ch = '\r'; break;
      case 'v': ch = '\v'; break;
      case 't': ch = '\t'; break;
      case 'f': ch = '\f'; break;
      case '0': ch = '\0'; break;
      case '\"': ch = '\"'; break;
      case '\'': ch = '\''; break;
      case 'x':
      case 'u':
      case 'U':
      {
        int digitsLimit = INT_MAX;
        if (ch == 'u')
          digitsLimit = 4;
        if (ch == 'U')
          digitsLimit = 8;

        std::string hex;

        for (ch = fetchChar(); ch > 0; ch = fetchChar())
        {
          if (!isxdigit(ch))
            break;
          hex += char(ch);
          nextChar();

          digitsLimit--;
          if (digitsLimit <= 0)
            break;
        }

        if (hex.empty())
        {
          ctx.error(102, "hexadecimal number expected", curLine, curColumn);
          break;
        }

        ch = (int)strtoull(hex.c_str(), nullptr, 16);
      }
      break;

      default:
        ctx.error(103, "unrecognised escaper char", curLine, curColumn);
        return false;
      }
    }

    tok += char(ch);
  }

  if (open_char == '\'')
  {
    if (tok.empty())
    {
      ctx.error(104, "empty constant", curLine, curColumn);
      return false;
    }

    if (tok.length() > 1)
    {
      ctx.error(105, "constant is too long", curLine, curColumn);
      return false;
    }

    Token::U u;
    u.i = (unsigned char)tok[0];
    tokens.emplace_back(Token{ (TokenType)TK_INTEGER, 0, (unsigned short)beginColumn, beginLine, u });
    addCurrentComments();
  }
  else
  {
    auto listIt = ctx.stringList.emplace(tok);
    Token::U u;
    u.s = listIt.first->c_str();
    tokens.emplace_back(Token{ (TokenType)TK_STRING_LITERAL, 0, (unsigned short)beginColumn, beginLine, u });
    addCurrentComments();
  }

  return true;
}


void Lexer::onCompilerDirective(const std::string & directive)
{
  const char * s = directive.c_str();
  if (strstr(s, "#apos:") == s)
  {
    s += 6;
    sscanf(s, "%d", &curColumn);
    curColumn--;
  }

  unsigned setFlags = 0;
  unsigned clearFlags = 0;
  bool applyToDefault = false;
  s++; // skip '#'
  if (strncmp(s, "default:", 8) == 0)
  {
    applyToDefault = true;
    s += 8;
  }

  if (strcmp(s, "strict") == 0)
    setFlags = LF_STRICT;
  else if (strcmp(s, "relaxed") == 0)
    clearFlags = LF_STRICT;
  else if (strcmp(s, "strict-bool") == 0)
    setFlags = LF_STRICT_BOOL;
  else if (strcmp(s, "relaxed-bool") == 0)
    clearFlags = LF_STRICT_BOOL;
  else if (strcmp(s, "no-plus-concat") == 0)
    setFlags = LF_NO_PLUS_CONCAT;
  else if (strcmp(s, "allow-plus-concat") == 0)
    clearFlags = LF_NO_PLUS_CONCAT;
  else if (strcmp(s, "forbid-root-table") == 0)
    setFlags = LF_FORBID_ROOT_TABLE;
  else if (strcmp(s, "allow-root-table") == 0)
    clearFlags = LF_FORBID_ROOT_TABLE;
  else if (strcmp(s, "disable-optimizer") == 0)
    setFlags = LF_DISABLE_OPTIMIZER;
  else if (strcmp(s, "enable-optimizer") == 0)
    clearFlags = LF_DISABLE_OPTIMIZER;

  ctx.langFeatures = (ctx.langFeatures | setFlags) & ~clearFlags;
  if (applyToDefault)
    CompilationContext::defaultLangFeatures = (CompilationContext::defaultLangFeatures | setFlags) & ~clearFlags;
}

bool Lexer::process()
{
  tokens.clear();
  tokens.reserve(s.length() / 6 + 4);
  currentComments.clear();
  index = is_utf8_bom(s.c_str(), 0) ? 3 : 0;

  curLine = 1;
  curColumn = 0;

  insideComment = false;
  insideString = false;
  insideRawString = false;

  #define PUSH_TOKEN(tk) \
    { \
      Token::U u; u.s = token_strings[tk]; tokens.emplace_back( Token \
      { (TokenType)(tk), 0, \
        (unsigned short)curColumn, curLine, u \
      }); \
      addCurrentComments(); \
    }

  for (int ch = nextChar(); ch > 0 && !ctx.isError; ch = nextChar())
    switch (ch)
    {
    case ' ':
    case '\t':
      if (!tokens.empty())
        tokens.back().flags |= TF_NEXT_SPACE;
      break;

    case 0x0d:
    case 0x0a:
      if (!tokens.empty())
        tokens.back().flags |= TF_NEXT_EOL;
      break;

    case '/':
      if (fetchChar() == '/')
      {
        if (ctx.includeComments)
        {
          std::string comment;
          nextChar();
          for (ch = nextChar(); ch > 0 && ch != '\n'; ch = nextChar())
            comment.push_back(ch);
          currentComments.emplace_back(comment);
        }
        else
        {
          for (ch = nextChar(); ch > 0 && ch != '\n'; ch = nextChar())
            ;
        }
        if (!tokens.empty())
          tokens.back().flags |= TF_NEXT_EOL;
      }
      else if (fetchChar() == '*')
      {
        nextChar();
        std::string comment;
        insideComment = true;
        bool eol = false;
        for (ch = nextChar(); ch > 0; ch = nextChar())
        {
          if (ch == '\n')
            eol = true;
          if (ch == '*' && fetchChar() == '/')
          {
            if (ctx.includeComments)
              currentComments.emplace_back(comment);
            nextChar();
            insideComment = false;
            break;
          }
          if (ctx.includeComments)
            comment.push_back(ch);
        }

        if (eol)
          if (!tokens.empty())
            tokens.back().flags |= TF_NEXT_EOL;
      }
      else if (fetchChar() == '=')
      {
        PUSH_TOKEN(TK_DIVEQ);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_DIV);
      }
      break;

    case '#':
      {
        std::string tok;
        tok += char(ch);
        for (ch = fetchChar(); ch > 0; ch = fetchChar())
        {
          if (isalnum(ch) || ch == '_' || ch == '-' || ch == ':')
          {
            tok += char(ch);
            nextChar();
          }
          else
            break;
        }

        onCompilerDirective(tok);
      }
      break;

    case '@':
      if (fetchChar() == '\"')
      {
        nextChar();
        parseStringLiteral(true, '\"');
      }
      else
        PUSH_TOKEN(TK_AT);
      break;

    case '\'':
      parseStringLiteral(false, '\'');
      break;

    case '\"':
      if (tokens.size() > 0 && tokens.back().type == TK_READER_MACRO)
      {
        int macroLength = 0;
        std::string macro = expandReaderMacro(s.c_str() + index, macroLength);
        index += macroLength;
        int baseColumn = tokens.back().column;
        int line = tokens.back().line;
        Lexer * macroLex = new Lexer(ctx, macro);
        macroLex->isReaderMacro = true;
        if (macroLex->process() && !macroLex->tokens.empty())
        {
          for (auto & t : macroLex->tokens)
          {
            t.column += baseColumn;
            t.line = line;
          }

          tokens.erase(tokens.end() - 1, tokens.end());
          tokens.insert(tokens.end(), macroLex->tokens.begin(), macroLex->tokens.end() - 1);
          tokens.back().flags &= ~TF_NEXT_EOL;
        }
        delete macroLex;
      }
      else
        parseStringLiteral(false, '\"');

      break;

    case '-':
      if (fetchChar() == '-')
      {
        PUSH_TOKEN(TK_MINUSMINUS);
        nextChar();
      }
      else if (fetchChar() == '=')
      {
        PUSH_TOKEN(TK_MINUSEQ);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_MINUS);
      }
      break;

    case '+':
      if (fetchChar() == '+')
      {
        PUSH_TOKEN(TK_PLUSPLUS);
        nextChar();
      }
      else if (fetchChar() == '=')
      {
        PUSH_TOKEN(TK_PLUSEQ);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_PLUS);
      }
      break;

    case '!':
      if (fetchChar() == '=')
      {
        PUSH_TOKEN(TK_NE);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_NOT);
      }
      break;

    case '%':
      if (fetchChar() == '=')
      {
        PUSH_TOKEN(TK_MODEQ);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_MODULO);
      }
      break;

    case '&':
      if (fetchChar() == '&')
      {
        PUSH_TOKEN(TK_AND);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_BITAND);
      }
      break;

    case '=':
      if (fetchChar() == '=')
      {
        PUSH_TOKEN(TK_EQ);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_ASSIGN);
      }
      break;

    case '(':
      PUSH_TOKEN(TK_LPAREN);
      break;

    case ')':
      PUSH_TOKEN(TK_RPAREN);
      break;

    case ',':
      PUSH_TOKEN(TK_COMMA);
      break;

    case '[':
      PUSH_TOKEN(TK_LSQUARE);
      break;

    case ']':
      PUSH_TOKEN(TK_RSQUARE);
      break;

    case '{':
      PUSH_TOKEN(TK_LBRACE);
      break;

    case '}':
      PUSH_TOKEN(TK_RBRACE);
      break;

    case '^':
      PUSH_TOKEN(TK_BITXOR);
      break;

    case '~':
      PUSH_TOKEN(TK_INV);
      break;

    case '$':
      PUSH_TOKEN(TK_READER_MACRO);
      break;

    case ';':
      PUSH_TOKEN(TK_SEMICOLON);
      break;

    case '*':
      if (fetchChar() == '=')
      {
        PUSH_TOKEN(TK_MULEQ);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_MUL);
      }
      break;

    case '.':
      if (fetchChar() == '.' && fetchChar(1) != '.')
      {
        ctx.error(106, "invalid token '..'", curLine, curColumn);
      }
      else if (fetchChar() == '.' && fetchChar(1) == '.')
      {
        PUSH_TOKEN(TK_VARPARAMS);
        nextChar();
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_DOT);
      }
      break;

    case ':':
      if (fetchChar() == ':')
      {
        PUSH_TOKEN(TK_DOUBLE_COLON);
        nextChar();
      }
      else if (fetchChar() == '=')
      {
        PUSH_TOKEN(TK_INEXPR_ASSIGNMENT);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_COLON);
      }
      break;

    case '?':
      if (fetchChar() == '(')
      {
        PUSH_TOKEN(TK_NULLCALL);
        nextChar();
      }
      else if (fetchChar() == '?')
      {
        PUSH_TOKEN(TK_NULLCOALESCE);
        nextChar();
      }
      else if (fetchChar() == '[')
      {
        PUSH_TOKEN(TK_NULLGETOBJ);
        nextChar();
      }
      else if (fetchChar() == '(')
      {
        PUSH_TOKEN(TK_NULLCALL);
        nextChar();
      }
      else if (fetchChar() == '.')
      {
        PUSH_TOKEN(TK_NULLGETSTR);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_QMARK);
      }
      break;;

    case '|':
      if (fetchChar() == '|')
      {
        PUSH_TOKEN(TK_OR);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_BITOR);
      }
      break;

    case '<':
      if (fetchChar() == '=')
      {
        if (fetchChar(1) == '>')
        {
          PUSH_TOKEN(TK_3WAYSCMP);
          nextChar();
        }
        else
        {
          PUSH_TOKEN(TK_LE);
        }
        nextChar();
      }
      else if (fetchChar() == '-')
      {
        PUSH_TOKEN(TK_NEWSLOT);
        nextChar();
      }
      else if (fetchChar() == '<')
      {
        PUSH_TOKEN(TK_SHIFTL);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_LS);
      }
      break;

    case '>':
      if (fetchChar() == '>')
      {
        if (fetchChar(1) == '>')
        {
          PUSH_TOKEN(TK_USHIFTR);
          nextChar();
        }
        else
        {
          PUSH_TOKEN(TK_SHIFTR);
        }
        nextChar();
      }
      else if (fetchChar() == '=')
      {
        PUSH_TOKEN(TK_GE);
        nextChar();
      }
      else
      {
        PUSH_TOKEN(TK_GT);
      }
      break;

      default:
        if (isBeginOfIdent(ch))
        {
          int beginLine = curLine;
          int beginColumn = curColumn;

          int from = index - 1;
          while (isContinueOfIdent(ch = fetchChar()))
            nextChar();
          ;

          auto listIt = ctx.stringList.emplace(std::string(s, from, index - from));
          Token::U u;
          u.s = listIt.first->c_str();

          auto it = tokenIdentStringToType.find(u.s);
          if (it != tokenIdentStringToType.end())
          {
            if (it->second == TK_IN && !tokens.empty() && tokens.back().type == TK_NOTTXT)
              tokens.back().type = TK_NOTIN;
            else if (it->second == TK___FILE__)
            {
              auto fileIt = ctx.stringList.insert(ctx.fileName);
              Token::U ufile;
              ufile.s = fileIt.first->c_str();
              tokens.emplace_back(Token{ (TokenType)TK_STRING_LITERAL, 0, (unsigned short)beginColumn, beginLine, ufile });
              addCurrentComments();
            }
            else if (it->second == TK___LINE__)
            {
              Token::U uline;
              uline.i = beginLine;
              tokens.emplace_back(Token{ (TokenType)TK_INTEGER, 0, (unsigned short)beginColumn, beginLine, uline });
              addCurrentComments();
            }
            else
            {
              tokens.emplace_back(Token{ it->second, 0, (unsigned short)beginColumn, beginLine, u });
              addCurrentComments();
            }
          }
          else
          {
            // HACK: just make code "x.$y" compilable
            if (tokens.size() > 0 && tokens.back().type == TK_READER_MACRO)
              tokens.erase(tokens.end() - 1, tokens.end());

            tokens.emplace_back(Token{ (TokenType)TK_IDENTIFIER, 0, (unsigned short)beginColumn, beginLine, u });
            addCurrentComments();
          }
        }
        else if (isdigit(ch))
        {
          std::string tok;
          tok += char(ch);
          int beginLine = curLine;
          int beginColumn = curColumn;
          bool hasPoint = false;
          bool hasExp = false;

          Token::U u;
          u.i = 0;

          if (ch == '0' && isdigit(fetchChar()))
          {
            ctx.error(107, "octal numbers are not supported", curLine, curColumn);
            break;
          }
          else if (ch == '0' && toupper(fetchChar()) == 'X')
          {
            tok += nextChar();
            for (ch = fetchChar(); ch > 0; ch = fetchChar())
            {
              if (isxdigit(ch) || ch=='_')
              {
                tok += char(ch);
                nextChar();
              }
              else
                break;
            }
          }
          else
          {
            for (ch = fetchChar(); ch > 0; ch = fetchChar())
            {
              if (isdigit(ch) || ch == '.' || ch == 'e' || ch == 'E' || ch == '_')
              {
                nextChar();
                tok += char(ch);
                if (ch == '.')
                {
                  if (hasPoint || hasExp)
                  {
                    ctx.error(108, "error in number", curLine, curColumn);
                    break;
                  }
                  hasPoint = true;
                }
                else if (ch == 'e' || ch == 'E')
                {
                  if (hasExp)
                  {
                    ctx.error(108, "error in number", curLine, curColumn);
                    break;
                  }
                  hasExp = true;
                  ch = nextChar();
                  tok += char(ch);
                  if (ch != '+' && ch != '-' && !isdigit(ch))
                  {
                    ctx.error(108, "error in number", curLine, curColumn);
                    break;
                  }

                  if (!isdigit(ch))
                  {
                    ch = nextChar();
                    tok += char(ch);
                    if (!isdigit(ch))
                    {
                      ctx.error(108, "error in number", curLine, curColumn);
                      break;
                    }
                  }
                }
              }
              else
                break;
            }
          }

          if (isBeginOfIdent(ch))
          {
            ctx.error(108, "error in number", curLine, curColumn);
            break;
          }

          tok.erase(std::remove(tok.begin(), tok.end(), '_'), tok.end()); // filter out underscores

          if (hasExp || hasPoint)
          {
            u.d = strtod(tok.c_str(), nullptr);
            tokens.emplace_back(Token{ TK_FLOAT, 0, (unsigned short)beginColumn, beginLine, u });
            addCurrentComments();
          }
          else
          {
            if (tok.length() >= 2 && toupper(tok[1]) == 'X')
            {
              if (tok.length() == 2)
              {
                ctx.error(109, "expected hex number", curLine, curColumn);
                break;
              }

              u.i = strtoull(tok.c_str() + 2, nullptr, 16);
              if (tok.length() > 16 + 2)
              {
                ctx.error(110, "too many digits for a hex number", curLine, curColumn);
                break;
              }
            }
            else
            {
              u.i = strtoull(tok.c_str(), nullptr, 10);
              if (u.i > LLONG_MAX)
              {
                ctx.error(111, "integer number is too big", curLine, curColumn);
              }
            }
            tokens.emplace_back(Token{ (TokenType)TK_INTEGER, 0, (unsigned short)beginColumn, beginLine, u });
            addCurrentComments();
          }
        }
        else
        {
          ctx.error(112, "syntax error", curLine, curColumn);
        }
      break;

    } // switch


  if (!ctx.isError)
  {
    if (insideComment)
      ctx.error(113, "unexpected end of file inside comment", curLine, curColumn);
    else if (insideString)
      ctx.error(114, "unexpected end of file inside string", curLine, curColumn);
    else if (insideRawString)
      ctx.error(115, "unexpected end of file inside raw string", curLine, curColumn);

    if (!isReaderMacro)
    {
      if ((s.length() > 0 && isSpaceOrTab(s[s.length() - 1])))
        ctx.warning("space-at-eol", std::max(curLine, 1), curColumn);

      if ((s.length() > 1 && s[s.length() - 1] == '\n' && isSpaceOrTab(s[s.length() - 2])) ||
        (s.length() > 2 && s[s.length() - 1] == '\n' && s[s.length() - 2] == '\r' && isSpaceOrTab(s[s.length() - 3])))
      {
        ctx.warning("space-at-eol", std::max(curLine - 1, 1), curColumn - 1);
      }
    }
  }

  if (!ctx.isError)
  {
    if (!tokens.empty())
      tokens.back().flags |= TF_NEXT_EOL;

    PUSH_TOKEN(TK_EOF);
  }

  return !ctx.isError;
}


