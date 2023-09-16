#include <string>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <streambuf>

using namespace std;

static int indent = 2;
static int line_width = 140;
static bool stdinout = false;

static const char * tokens_str[] =
{
  "<=>", "...", ">>>",
  "<<", ">>", "++", "--", "<=", ">=", "!=", "==", "+=", "-=", "*=", "/=", "^=", "&=", "|=", "||", "&&", "::", "<-", ":=",
  "??", "?[", "?(", "?.",
  "/>", "</",
};

struct Token
{
  string text;
  int spaces;
  int newLines;
  int lineInSource;
  int column;

  bool eq(const char * s) const
  {
    return *s == text.c_str()[0] && !strcmp(s, text.c_str());
  }

  bool isComment()
  {
    const char * s = text.c_str();
    return s[0] == '/' && (s[1] == '/' || s[1] == '*');
  }
};

struct Formatter
{
  bool haveUtf8Bom = false;
  int newLineCounter = 0;
  int spaceCounter = 0;
  int srcLine = 1;
  int curColumn = 1;
  bool eof = false;
  const char * p = nullptr;
  const char * curLinePtr = nullptr;
  vector<Token> tokens;

  Formatter() {};
  ~Formatter() {};

  static bool isUtf8Bom(const char * ptr)
  {
    return (uint8_t)ptr[0] == 0xEF && (uint8_t)ptr[1] == 0xBB && (uint8_t)ptr[2] == 0xBF;
  }

  void nextChar()
  {
    if (*p == 0)
    {
      eof = true;
      return;
    }

    if (*p == '\r' && p[1] == '\n')
      p++;

    if (*p == ' ')
      spaceCounter++;

    if (*p == '\t')
      spaceCounter += 4;

    if (*p == '\n')
    {
      p++;
      curLinePtr = p;
      spaceCounter = 0;
      newLineCounter++;
      srcLine++;
      curColumn = 1;
      return;
    }
    else
    {
      p++;
      curColumn++;
      return;
    }
  }

  bool mustHaveSpacesAround(const char * s)
  {
    if (strchr("+-<>=/*%&|?:!", *s))
    {
      if (strcmp(s, "!") == 0)
        return false;
      if (strcmp(s, "?.") == 0)
        return false;
      if (strcmp(s, "?(") == 0)
        return false;
      if (strcmp(s, "?[") == 0)
        return false;
      if (strcmp(s, "++") == 0)
        return false;
      if (strcmp(s, "--") == 0)
        return false;
      if (strcmp(s, "::") == 0)
        return false;

      return true;
    }
    return false;
  }

  bool isUnary(int idx)
  {
    if (tokens[idx].eq("-") || tokens[idx].eq("+"))
    {
      Token & prev = tokens[idx - 1];
      return prev.eq("return") || prev.eq("yield") || prev.eq("(") || prev.eq("[") ||
        prev.eq(",") || mustHaveSpacesAround(prev.text.c_str());
    }
    return false;
  }


  void skipSpaces()
  {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
      nextChar();
  }

  void parseToken()
  {
    skipSpaces();
    if (eof)
    {
      tokens.emplace_back(Token{string(), spaceCounter, newLineCounter, srcLine, curColumn});
      return;
    }

    const char * from = p;
    int fromColumn = curColumn;

    if (isalpha(*p) || *p == '_')
    {
      while (!eof && (isalnum(*p) || *p == '_'))
        nextChar();

      tokens.emplace_back(Token{string(from, p), spaceCounter, newLineCounter, srcLine, fromColumn});
      spaceCounter = 0;
      newLineCounter = 0;
      return;
    }

    if (isdigit(*p) || (*p == '.' && isdigit(p[1])))
    {
      while (!eof && (isalnum(*p) || *p == '.' || ((*p == 'e' || *p == 'E') && (p[1] == '-' || p[1] == '+' || isdigit(p[1])))))
      {
        if ((*p == 'e' || *p == 'E') && (p[1] == '-' || p[1] == '+'))
          nextChar();

        nextChar();
      }

      tokens.emplace_back(Token{string(from, p), spaceCounter, newLineCounter, srcLine, fromColumn});
      spaceCounter = 0;
      newLineCounter = 0;
      return;
    }

    if (*p == '#')
    {
      while (!eof && !isspace(*p))
        nextChar();

      tokens.emplace_back(Token{string(from, p), spaceCounter, newLineCounter, srcLine, fromColumn});
      spaceCounter = 0;
      newLineCounter = 0;
      return;
    }

    if (*p == '\"' || *p == '\'')
    {
      int savedLine = newLineCounter;
      int savedSpace = spaceCounter;

      char openChar = *p;
      nextChar();
      while (*p != openChar)
      {
        if (*p == '\\')
          nextChar();

        nextChar();
        if (*p == '\r' || *p == '\n' || !*p) // error
          break;
      }
      if (*p == openChar)
        nextChar();

      tokens.emplace_back(Token{string(from, p), savedSpace, savedLine, srcLine, fromColumn});
      spaceCounter = 0;
      newLineCounter = 0;
      return;
    }


    if (*p == '$' && p[1] == '\"')
    {
      int savedLine = newLineCounter;
      int savedSpace = spaceCounter;
      int depth = 0;

      nextChar();
      nextChar();
      while (*p != '\"' || depth > 0)
      {
        if (*p == '{')
          depth++;
        if (*p == '}')
          depth--;
        if (*p == '\\')
          nextChar();

        nextChar();
        if (*p == '\r' || *p == '\n' || !*p) // error
          break;
      }
      if (*p == '\"')
        nextChar();

      tokens.emplace_back(Token{string(from, p), savedSpace, savedLine, srcLine, fromColumn});
      spaceCounter = 0;
      newLineCounter = 0;
      return;
    }


    if (*p == '@' && p[1] == '\"')
    {
      int savedLine = newLineCounter;
      int savedSpace = spaceCounter;

      nextChar();
      nextChar();
      while (*p != '\"')
      {
        nextChar();
        if (*p == '\"' && p[1] == '\"')
        {
          nextChar();
          nextChar();
        }

        if (!*p) // error
          break;
      }
      nextChar();

      tokens.emplace_back(Token{string(from, p), savedSpace, savedLine, srcLine, fromColumn});
      spaceCounter = 0;
      newLineCounter = 0;
      return;
    }

    if (*p == '/' && p[1] == '/')
    {
      int savedLine = newLineCounter;
      int savedSpace = spaceCounter;

      while (*p && *p != '\n' && *p != '\r')
        nextChar();

      tokens.emplace_back(Token{string(from, p), savedSpace, savedLine, srcLine, fromColumn});
      spaceCounter = 0;
      newLineCounter = 0;
      return;
    }

    if (*p == '/' && p[1] == '*')
    {
      int savedLine = newLineCounter;
      int savedSpace = spaceCounter;

      nextChar();
      nextChar();
      while (*p)
      {
        if (*p == '*' && p[1] == '/')
          break;
        nextChar();
      }
      nextChar();
      nextChar();

      tokens.emplace_back(Token{string(from, p), savedSpace, savedLine, srcLine, fromColumn});
      spaceCounter = 0;
      newLineCounter = 0;
      return;
    }

    for (int i = 0; i < sizeof(tokens_str) / sizeof(tokens_str[0]); i++)
      if (tokens_str[i][0] == *p)
      {
        for (int j = 0; j < 4; j++)
          if (p[j] && tokens_str[i][j] == p[j])
          {
            //...
          }
          else if (!tokens_str[i][j])
          {
            p += j - 1;
            nextChar();
            tokens.emplace_back(Token{string(tokens_str[i]), spaceCounter, newLineCounter, srcLine, fromColumn});
            spaceCounter = 0;
            newLineCounter = 0;
            return;
          }
          else
            break;
      }

    nextChar();
    tokens.emplace_back(Token{string(from, p), spaceCounter, newLineCounter, srcLine, fromColumn});
    spaceCounter = 0;
    newLineCounter = 0;
    return;
  }

  int findPairBracket(int idx)
  {
    int count = 0;
    for (int i = idx; i < int(tokens.size()); i++)
    {
      if (tokens[i].eq("(") || tokens[i].eq("{") || tokens[i].eq("[") || tokens[i].eq("?[") || tokens[i].eq("?("))
        count++;
      if (tokens[i].eq(")") || tokens[i].eq("}") || tokens[i].eq("]"))
        count--;
      if (!count)
        return i;
    }

    return idx;
  }

  void fmtSpaceAfterKeyword()
  {
    for (int i = 1; i < int(tokens.size()); i++)
      if (tokens[i].eq("("))
      {
        const Token & prev = tokens[i - 1];
        if (prev.eq("if") || prev.eq("while") || prev.eq("switch") || prev.eq("for") || prev.eq("foreach"))
          if (!tokens[i].newLines)
            tokens[i].spaces = 1;
      }
  }

  void removeNewLine(int idx)
  {
    if (idx == 0)
      return;

    if (!tokens[idx - 1].isComment())
    {
      tokens[idx].newLines = 0;
      tokens[idx].spaces = 1;
      return;
    }

    for (int i = idx - 1; i >= 0; i--)
    {
      if (!tokens[i].isComment())
      {
        i++;
        Token tmp = tokens[idx];
        for (int j = idx - 1; j >= i; j--)
          tokens[j + 1] = tokens[j];
        tokens[i] = tmp;
        tokens[i].spaces = 1;
        tokens[i].newLines = 0;
        return;
      }
    }
  }

  void fmtEgyptianBraces()
  {
    for (int i = 0; i < int(tokens.size()) - 2; i++)
    {
      int x = -1;
      if (tokens[i].eq("if") || tokens[i].eq("while") || tokens[i].eq("switch") || tokens[i].eq("for") ||
        tokens[i].eq("foreach"))
      {
        x = findPairBracket(i + 1);
        if (x == i + 1)
          continue;
        x++;
      }

      if (tokens[i].eq("function") || tokens[i].eq("constructor") || tokens[i].eq("catch"))
      {
        for (int j = i + 1; j < int(tokens.size()) - 2; j++)
          if (tokens[j].eq("("))
          {
            x = findPairBracket(j);
            x++;
            break;
          }
      }

      if (tokens[i].eq("enum") || tokens[i].eq("class") || tokens[i].eq("try"))
      {
        for (int j = i + 1; j < int(tokens.size()) - 2; j++)
          if (tokens[j].eq("{"))
          {
            x = j;
            break;
          }
      }

      if (tokens[i].eq("else"))
        x = i + 1;

      if (x >= 0)
      {
        for (; x < int(tokens.size()) - 1; x++) // skip comments
          if (!tokens[x].isComment())
            break;

        if (tokens[x].eq("{") && tokens[x].newLines)
          removeNewLine(x);
      }
    }
  }

  void fmtSpaceBeforeCodeBlock()
  {
    for (int i = 1; i < int(tokens.size()); i++)
      if (tokens[i].eq("{") && tokens[i - 1].eq(")"))
      {
        if (!tokens[i].newLines && !tokens[i].spaces)
          tokens[i].spaces = 1;
      }
  }

  void fmtSpaceAfterComma()
  {
    for (int i = 0; i < int(tokens.size()) - 1; i++)
      if (tokens[i].eq(","))
      {
        Token & next = tokens[i + 1];
        if (!tokens[i].newLines && tokens[i].spaces)  // remove spaces before comma
        {
          if (!next.newLines)
            next.spaces += tokens[i].spaces;
          tokens[i].spaces = 0;
        }

        if (!next.newLines && !next.spaces)
          next.spaces = 1;
      }
  }

  void fmtSpaceAfterSemicolon()
  {
    for (int i = 0; i < int(tokens.size()) - 1; i++)
      if (tokens[i].eq(";"))
      {
        Token & next = tokens[i + 1];
        if (!next.newLines && !next.spaces)
          next.spaces = 1;
      }
  }

  void fmtSpacesAroundAssignment()
  {
    for (int i = 1; i < int(tokens.size()) - 1; i++)
      if (tokens[i].eq("=") || tokens[i].eq("<-"))
      {
        if (!tokens[i].newLines && !tokens[i].spaces)
          tokens[i].spaces = 1;
        if (!tokens[i + 1].newLines && !tokens[i + 1].spaces)
          tokens[i + 1].spaces = 1;
      }
  }

  void fmtRemoveSpacesInsideBrackets()
  {
    for (int i = 1; i < int(tokens.size()) - 1; i++)
    {
      if ((tokens[i].eq("(") || tokens[i].eq("?(")) && !tokens[i + 1].newLines && tokens[i + 1].spaces)
        tokens[i + 1].spaces = 0;
      if (tokens[i].eq(")") && !tokens[i].newLines && tokens[i].spaces)
        tokens[i].spaces = 0;
    }
  }

  void fmtSpacesInsideCurlyBraces()
  {
    for (int i = 1; i < int(tokens.size()) - 1; i++)
    {
      if (tokens[i].eq("{") && !tokens[i + 1].newLines && !tokens[i + 1].spaces && !tokens[i + 1].eq("}"))
        tokens[i + 1].spaces = 1;
      if (tokens[i].eq("}") && !tokens[i].newLines && !tokens[i].spaces && !tokens[i - 1].eq("{"))
        tokens[i].spaces = 1;
    }
  }

  void fmtElse()
  {
    for (int i = 1; i < int(tokens.size()) - 1; i++)
    {
      if (tokens[i].eq("else") && tokens[i - 1].eq("}") && tokens[i - 1].newLines && !tokens[i].newLines)
      {
        tokens[i].newLines = 1;
        tokens[i].spaces = tokens[i - 1].spaces;
      }
      if (tokens[i].eq("else") && tokens[i + 1].eq("if") && tokens[i + 1].newLines)
      {
        removeNewLine(i + 1);
      }
    }
  }

  void fmtSpacesAroundOperators()
  {
    bool insideCase = false;

    for (int i = 1; i < int(tokens.size()) - 1; i++)
    {
      if (tokens[i].eq("case") || tokens[i].eq("default"))
        insideCase = tokens[i].lineInSource;

      if (mustHaveSpacesAround(tokens[i].text.c_str()))
      {
        if (tokens[i].eq(":") && insideCase)
        {
          insideCase = false;
          continue;
        }

        if (isUnary(i))
        {
          if (!tokens[i + 1].newLines && tokens[i + 1].spaces)
            tokens[i + 1].spaces = 0;
        }
        else
        {
          if (!tokens[i].newLines && !tokens[i].spaces)
            tokens[i].spaces = 1;
          if (!tokens[i + 1].newLines && !tokens[i + 1].spaces)
            tokens[i + 1].spaces = 1;
        }
      }
    }
  }

  void fmtNewLineAfterIfElse()
  {
    for (int i = 1; i < int(tokens.size()) - 2; i++)
    {
      if (tokens[i].newLines && (tokens[i].eq("if") || tokens[i].eq("while") || tokens[i].eq("for") || tokens[i].eq("foreach") ||
        tokens[i].eq("else")))
      {
        int statement = i + 1;
        if (tokens[statement].eq("if"))
          statement++;

        if (tokens[statement].eq("("))
          statement = findPairBracket(statement) + 1;
        if (statement < int(tokens.size()))
        {
          if (tokens[statement].eq("if"))
            continue;

          int endOfStatement = statement + 1;
          if (tokens[statement].eq("{"))
          {
            int lastBracket = findPairBracket(statement);
            if (lastBracket < int(tokens.size()) - 1 && !tokens[lastBracket + 1].newLines && !tokens[lastBracket].eq("{") &&
              !tokens[lastBracket].isComment())
            {
              tokens[lastBracket].newLines = 1;
              tokens[lastBracket].spaces = tokens[i].spaces;
            }
            statement++;
            endOfStatement = lastBracket + 1;
          }

          int elsePos = -1;
          if (tokens[i].eq("if"))
            for (int j = endOfStatement; j < int(tokens.size()); j++)
            {
              if (tokens[j].newLines)
                break;
              if (tokens[j].eq("else"))
              {
                elsePos = j;
                break;
              }
              if (tokens[j].eq("if"))
                break;
              if (tokens[j].eq("}"))
                break;
            }

          if (elsePos != -1 && !tokens[elsePos].newLines && !tokens[elsePos].isComment())
          {
            tokens[elsePos].newLines = 1;
            tokens[elsePos].spaces = tokens[i].spaces;

            if (elsePos < int(tokens.size()) - 2 && tokens[elsePos + 1].eq("{"))
            {
              int lastBracket = findPairBracket(elsePos + 1);
              if (lastBracket > elsePos + 1 && !tokens[lastBracket].eq("{") && !tokens[lastBracket].isComment())
              {
                tokens[lastBracket].newLines = 1;
                tokens[lastBracket].spaces = tokens[i].spaces;
              }
            }
          }

          if (!tokens[statement].newLines && !tokens[statement].eq("}") && !tokens[statement].eq("{") && !tokens[statement].isComment())
          {
            tokens[statement].newLines = 1;
            tokens[statement].spaces = tokens[i].spaces + indent;
          }
        }
      }
    }
  }

  void fmtNewLineBeforeClosingBracket()
  {
    for (int i = 0; i < int(tokens.size()) - 3; i++)
      if (tokens[i].newLines)
      {
        int spaces = tokens[i].spaces;
        if (tokens[i].eq("if") || tokens[i].eq("while") || tokens[i].eq("for") || tokens[i].eq("foreach") || tokens[i].eq("else"))
        {
          if (tokens[i].eq("else") && tokens[i + 1].eq("if"))
            i++;

          if (tokens[i + 1].eq("("))
            i = findPairBracket(i + 1);
          i++;
        }

        if (i >= int(tokens.size()) - 2)
          break;

        if (tokens[i].eq("{") && tokens[i + 1].newLines && (tokens[i - 1].eq(")") || tokens[i - 1].eq("else")))
        {
          int closingBraket = findPairBracket(i);
          if (closingBraket > i && !tokens[i].newLines)
          {
            tokens[closingBraket].newLines = 1;
            tokens[closingBraket].spaces = spaces;
          }
        }
      }
  }

  void fmtLineWidth()
  {
    int curIndent = 0;
    int accum = 0;
    int lineBeginTok = 0;
    bool ignoreThisString = false;

    for (int i = 0; i < int(tokens.size()); i++)
    {
      if (tokens[i].newLines)
      {
        curIndent = tokens[i].spaces;
        accum = curIndent;
        lineBeginTok = i;
        ignoreThisString = false;
      }

      if (int(tokens[i].text.length()) >= line_width)
        ignoreThisString = true;

      if (tokens[i].text.c_str()[0] == '/' && tokens[i].text.c_str()[1] == '/')
        ignoreThisString = true;

      if (ignoreThisString)
        continue;

      if (accum + tokens[i].spaces + int(tokens[i].text.length()) >= line_width)
      {
        int breakPos = 0;
        int breakIndent = curIndent + 2 * indent;

        for (int j = i; j > lineBeginTok; j--)
          if (tokens[j - 1].eq("||") || tokens[j - 1].eq("&&") || tokens[j - 1].eq(":") || tokens[j - 1].eq("?"))
          {
            if (tokens[j].column > breakIndent + 2)
              breakPos = j;
            break;
          }

        if (!breakPos)
          for (int j = i; j > lineBeginTok; j--)
            if (tokens[j - 1].eq("+") || tokens[j - 1].eq("-"))
              if (tokens[j - 1].spaces && tokens[j].spaces)
              {
                if (tokens[j].column > breakIndent + 2)
                  breakPos = j;
                break;
              }

        if (!breakPos)
          for (int j = i; j > lineBeginTok; j--)
            if (!tokens[j].eq("{") && tokens[j].spaces)
            {
              if (tokens[j].column > breakIndent + 2)
                breakPos = j;
              break;
            }

        if (!breakPos)
          for (int j = i; j > lineBeginTok; j--)
            if (tokens[j - 1].eq(".") || tokens[j - 1].eq("?."))
            {
              if (tokens[j].column > breakIndent + 2)
                breakPos = j;
              break;
            }

        if (!breakPos)
          for (int j = i; j > lineBeginTok; j--)
            if (tokens[j - 1].eq("("))
            {
              breakPos = j;
              break;
            }

        if (breakPos)
        {
          tokens[breakPos].newLines = 1;
          tokens[breakPos].spaces = breakIndent;
          i = breakPos;

          accum = curIndent + int(tokens[breakPos].text.length());
          lineBeginTok = i;
          ignoreThisString = false;

          continue;
        }
      }

      accum += tokens[i].spaces + int(tokens[i].text.length());
    }
  }

  string format(const char * code)
  {
    if (!code || !*code)
      return string();

    p = code;
    if (isUtf8Bom(p))
    {
      p += 3;
      haveUtf8Bom = true;
    }

    eof = false;
    curLinePtr = p;

    while (!eof)
    {
      parseToken();
      //Token & t = tokens.back();
      //printf("s:%d n:%d \"%s\"\n", t.spaces, t.newLines, t.text.c_str());
    }

    fmtNewLineAfterIfElse();
    fmtSpaceAfterKeyword();
    fmtEgyptianBraces();
    fmtSpaceAfterComma();
    fmtSpaceAfterSemicolon();
    fmtSpaceBeforeCodeBlock();
    fmtSpacesAroundAssignment();
    fmtRemoveSpacesInsideBrackets();
    fmtSpacesInsideCurlyBraces();
    fmtElse();
    fmtSpacesAroundOperators();
    fmtLineWidth();
    fmtNewLineBeforeClosingBracket();

    return generateCode();
  }

  void debugPrint()
  {
    for (Token & t : tokens)
      printf("s:%d n:%d \"%s\"\n", t.spaces, t.newLines, t.text.c_str());
  }

  string generateCode()
  {
    string res;
    res.reserve(256000);
    if (haveUtf8Bom)
    {
      res += (char)0xEF;
      res += (char)0xBB;
      res += (char)0xBF;
    }

    for (Token & t : tokens)
    {
      for (int i = 0; i < t.newLines; i++)
        res += "\n";
      for (int i = 0; i < t.spaces; i++)
        res += " ";
      res += t.text;
    }
    return res;
  }
};

void print_usage()
{
  printf("\nUsage:\n");
  printf("  nut_formatter [options] <file_name.nut>\n");
  printf("  or\n");
  printf("  nut_formatter [options] --files:<files-list.txt>\n\n");
  printf("  --no-backup - don't create backup files.\n");
  printf("  --line-width:110 - fit to line width (140 by default).\n");
  printf("  --std - get source from stdin, write result to stdout.\n");
}

int main(int argc, char ** argv)
{
  if (argc <= 1 || !strcmp(argv[1], "--help"))
  {
    print_usage();
    return 0;
  }

  vector<string> fileList;

  bool createBackup = true;

  for (int i = 1; i < argc; i++)
  {
    const char * arg = argv[i];
    if (arg[0] != '-')
    {
      fileList.push_back(string(arg));
      break;
    }

    if (!strcmp(arg, "--no-backup"))
      createBackup = false;

    if (!strcmp(arg, "--std"))
      stdinout = true;

    if (!strncmp(arg, "--line-width:", 13))
    {
      const char * s = arg + 13;
      line_width = atoi(s);
      if (line_width <= indent)
        line_width = 999999;
    }

    if (!strncmp(arg, "--files:", 8))
    {
      const char * fn = arg + 8;
      ifstream tmp(fn);
      if (tmp.fail())
      {
        printf("\nERROR: Cannot open files list '%s'\n\n", fn);
        return 1;
      }

      string filesListString((std::istreambuf_iterator<char>(tmp)), std::istreambuf_iterator<char>());
      filesListString += "\n";
      string nutName;
      for (size_t j = 0; j < filesListString.size(); j++)
      {
        if (filesListString[j] >= ' ')
          nutName += filesListString[j];
        else if (!nutName.empty())
        {
          fileList.push_back(nutName);
          nutName.clear();
        }
      }
    }
  }


  if (!fileList.empty() && stdinout)
  {
    printf("\nERROR: stdin/stdout and file input were set at the same time\n\n");
    return 1;
  }


  if (stdinout)
  {
    Formatter f;
    istreambuf_iterator<char> begin(cin), end;
    string code(begin, end);
    string formatted = f.format(code.c_str());
    cout << formatted;
    return 0;
  }


  for (const string & fileName : fileList)
  {
    Formatter f;

    ifstream tmp(fileName);
    if (tmp.fail())
    {
      printf("\nERROR: Cannot open file '%s'\n\n", fileName.c_str());
      return 1;
    }
    string code = string((std::istreambuf_iterator<char>(tmp)), std::istreambuf_iterator<char>());
    string formatted = f.format(code.c_str());
    if (formatted != code)
    {
      if (createBackup)
      {
        std::ofstream bak(fileName + ".bak");
        bak << code;
        bak.close();
      }

      std::ofstream fmt(fileName);
      if (fmt.fail())
      {
        printf("\nERROR: Cannot open file for write '%s'\n\n", fileName.c_str());
        return 1;
      }
      fmt << formatted;
      fmt.close();
    }
  }

  return 0;
}
