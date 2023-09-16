#pragma once

// Regular Expressions
// Copyright (c) 2000-2001 by Digital Mars
// All Rights Reserved
// Written by Walter Bright
// Ported to Dagor Tech by Nic Tiger


#if _TARGET_PC_WIN | _TARGET_XBOX
#include <tchar.h>
#else
typedef char TCHAR;
#define __T(S) S
#define _tcscpy   strcpy
#define _tcslen   strlen
#define _tcschr   strchr
#define _totupper toupper
#define _totlower tolower
#define _istlower islower
#define _istdigit isdigit
#define _istalpha isalpha
#define _tclen(_pc)           (1)
#define _tccpy(_pc1,_cpc2)    ((*(_pc1) = *(_cpc2)))
#define _tcsdec(_cpc1, _cpc2) ((_cpc1)>=(_cpc2) ? NULL : (_cpc2)-1)
#define _tcsinc(_pc)          ((_pc)+1)
#define _tcsnextc(_cpc)       ((unsigned int) *(const unsigned char *)(_cpc))
#endif

/*
  Escape sequences:

  \nnn starts out a 1, 2 or 3 digit octal sequence,
  where n is an octal digit. If nnn is larger than
  0377, then the 3rd digit is not part of the sequence
  and is not consumed.
  For maximal portability, use exactly 3 digits.

  \xXX starts out a 1 or 2 digit hex sequence. X
  is a hex character. If the first character after the \x
  is not a hex character, the value of the sequence is 'x'
  and the XX are not consumed.
  For maximal portability, use exactly 2 digits.

  \uUUUU is a unicode sequence. There are exactly
  4 hex characters after the \u, if any are not, then
  the value of the sequence is 'u', and the UUUU are not
  consumed.

  Character classes:

  [a-b], where a is greater than b, will produce
  an error.
*/


class RegExp
{
public:
  struct Match
  {
    int rm_so;      // index of start of match
    int rm_eo;      // index past end of match
  };

public:
  RegExp();
  ~RegExp() { reset(); }


  int compile(const TCHAR *pattern, const TCHAR *attributes=NULL, int ref=false);
  int test(const TCHAR *string, int startindex = 0);

  TCHAR *replace(const TCHAR *format);
  TCHAR *replace2(const TCHAR *format);
  static TCHAR *replace3(const TCHAR *format, const TCHAR *input,
    unsigned re_nsub, const Match *pmatch);
  static TCHAR *replace4(const TCHAR *input, const Match *match, const TCHAR *replacement);


  void reset();

public:
  unsigned re_nsub; // number of parenthesized subexpression matches
  Match *pmatch;    // array [re_nsub + 1]

private:
  struct Range;
  struct RegBuffer;

  const TCHAR *src;       // current source pointer
  const TCHAR *src_end;   // source string end pointer
  const TCHAR *src_start; // starting position for match
  const TCHAR *p;         // position of parser in pattern
  const TCHAR *input;     // the string to search

  // match for the entire regular expression (serves as storage for pmatch[0])
  Match match;

  // per instance:

  int ref;              // !=0 means don't make our own copy of pattern
  const TCHAR *pattern; // source text of the regular expression

  TCHAR flags[3 + 1];   // source text of the attributes parameter
                        // (3 TCHARs max plus terminating 0)
  int errors;

  enum
  {
    REAglobal     = 1, // has the g attribute
    REAignoreCase = 2, // has the i attribute
    REAmultiline  = 4, // if treat as multiple lines separated
                       // by newlines, or as a single line
    REAdotmatchlf = 8, // if . matches \n
  };
  unsigned attributes;

  char *program;
  RegBuffer *buf;

  void printProgram(const char *prog);
  int trymatch(const char *prog, const char *progend);
  int parseRegexp();
  int parsePiece();
  int parseAtom();
  int parseRange();
  int escape();
  void error(const char *msg);
  void optimize();
  int startchars(Range *r, char *prog, char *progend);

  // effectively disable copy ctor and assignment operator (since they are not implemented anyway)
  RegExp(const RegExp&);
  RegExp& operator =(const RegExp&);
};
