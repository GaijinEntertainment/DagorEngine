// Regular Expressions
// Copyright (c) 2000-2001 by Digital Mars
// All Rights Reserved
// Written by Walter Bright
// www.digitalmars.com

#include <regExp/regExp.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#if _TARGET_PC_WIN|_TARGET_XBOX
#include <malloc.h>
#elif defined(__GNUC__)
#include <stdlib.h>
#define memicmp strnicmp
#endif
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_localConv.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_localConv.h>
#define use_memicmp dd_strnicmp

__forceinline TCHAR use_totupper(TCHAR c)
{
  if (sizeof(TCHAR) == sizeof(char))
    return dd_charupr(c);
  return _totupper(c);
}
__forceinline bool use_istlower(TCHAR c)
{
  if (sizeof(TCHAR) == sizeof(char))
    return dd_charlwr(c) == c;
  return _istlower(c);
}


// Disable debugging printf's
#define printf  1 ||
#define wprintf 1 ||

#define malloc     strmem->alloc
#define realloc    strmem->realloc
#define free       strmem->free

/******************************************/

struct RegExp::RegBuffer
{
  unsigned char *data;
  size_t offset, size;

  RegBuffer();
  ~RegBuffer();

  void reserve(size_t nbytes);
  void write(const void *data, size_t nbytes);
  void writeByte(unsigned b);
  void writeword(unsigned b);
  void writeTCHAR(TCHAR c);
  void write4(unsigned w);
  void write(RegBuffer *buf);
  void fill0(unsigned nbytes);
  void spread(size_t offset, size_t nbytes);
};

/******************************************/

// Opcodes

enum REopcodes
{
  REend,      // end of program
  REchar,     // single character
  REichar,    // single character, case insensitive
  REwchar,    // single wide character
  REiwchar,   // single wide character, case insensitive
  REanychar,  // any character
  REanystar,  // ".*"
  REstring,   // string of characters
  REistring,  // string of characters, case insensitive
  REtestbit,  // any in bitmap, non-consuming
  REbit,      // any in the bit map
  REnotbit,   // any not in the bit map
  RErange,    // any in the string
  REnotrange, // any not in the string
  REor,       // a | b
  REplus,     // 1 or more
  REstar,     // 0 or more
  REquest,    // 0 or 1
  REnm,       // n..m
  REnmq,      // n..m, non-greedy version
  REbol,      // beginning of line
  REeol,      // end of line
  REparen,    // parenthesized subexpression
  REgoto,     // goto offset

  REwordboundary,
  REnotwordboundary,
  REdigit,
  REnotdigit,
  REspace,
  REnotspace,
  REword,
  REnotword,
  REbackref,
};

// BUG: should this include '$'?
static int isword(TCHAR c) { return isalnum(c) || c == '_'; }

static unsigned inf = ~0u;

RegExp::RegExp()
{
  memset(this, 0, sizeof(RegExp));
  pmatch = &match;
}

void RegExp::reset()
{
  if (!ref && pattern)
    free((void*)pattern);
  if (pmatch != &match && pmatch)
    free(pmatch);
  if (program)
    free(program);
  memset(this, 0, sizeof(RegExp));
  pmatch = &match;
}

// Returns 1 on success, 0 error

int RegExp::compile(const TCHAR *pattern, const TCHAR *attributes, int ref)
{
  if (!pattern)
    return false;

  if (!attributes)
    attributes = __T("");
  wprintf(L"RegExp::compile('%s', '%s', %d)\n", pattern, attributes, ref);

  this->attributes = 0;
  if (attributes)
  {
    const TCHAR *p = attributes;

    for ( ; *p; p = _tcsinc(p))
    {
      unsigned att;

      switch (*p)
      {
      case 'g': att = REAglobal;  break;
      case 'i': att = REAignoreCase;  break;
      case 'm': att = REAmultiline; break;
      default:
        return 0;   // unrecognized attribute
      }
      if (this->attributes & att)
        return 0;   // redundant attribute
      this->attributes |= att;
    }
  }

  input = NULL;

  if (!this->ref)
    free((void*)this->pattern);
  this->pattern = ref ? pattern : str_dup(pattern,strmem);
  this->ref = ref;
  _tcscpy(flags, attributes);

  unsigned oldre_nsub = re_nsub;
  re_nsub = 0;
  errors = 0;

  buf = new RegBuffer();
  buf->reserve(_tcslen(pattern) * 8);
  p = this->pattern;
  parseRegexp();
  if (*p)
    error("unmatched ')'");

  optimize();
  program = (char *)buf->data;
  buf->data = NULL;
  delete buf;
  buf = NULL;

  if (re_nsub > oldre_nsub)
  {
    if (pmatch == &match)
      pmatch = NULL;
    pmatch = (Match *)realloc(pmatch, (re_nsub + 1) * sizeof(Match));
  }
  pmatch[0].rm_so = 0;
  pmatch[0].rm_eo = 0;

  return (errors == 0);
}

int RegExp::test(const TCHAR *string, int startindex)
{
  const TCHAR *s;
  const TCHAR *sstart;
  unsigned firstc;

#if _UNICODE
  wprintf(L"RegExp::test(string = '%s', index = %d)\n", string, match.rm_eo);
#else
  //printf("RegExp::test(string = '%s', index = %d)\n", string, match.rm_eo);
#endif
  pmatch[0].rm_so = 0;
  pmatch[0].rm_eo = 0;
  input = string;
  if (startindex < 0 || startindex > _tcslen(string))
    return 0;     // fail
  sstart = string + startindex;

  printProgram(program);

  // First character optimization
  firstc = 0;
  if (program[0] == REchar)
  {
    firstc = *(unsigned char *)(program + 1);
    if (attributes & REAignoreCase && isalpha(firstc))
      firstc = 0;
  }

  src_end = sstart + _tcslen(sstart)+1;
  for (s = sstart; ; s = _tcsinc(s))
  {
    if (firstc && _tcsnextc(s) != (TCHAR)firstc)
    {
      s = _tcsinc(s);
      s = _tcschr(s, firstc);
      if (!s)
        break;
    }
    memset(pmatch, -1, (re_nsub + 1) * sizeof(Match));
    src_start = src = s;
    if (trymatch(program, NULL))
    {
      pmatch[0].rm_so = src_start - input;
      pmatch[0].rm_eo = src - input;
      printf("start = %d, end = %d\n", match.rm_so, match.rm_eo);
      return 1;
    }
    // If possible match must start at beginning, we are done
    if (program[0] == REbol || program[0] == REanystar)
    {
      if (attributes & REAmultiline)
      {
        // Scan for the next \n
        s = _tcschr(s, '\n');
        if (!s)
          break;
      }
      else
        break;
    }
    if (!s || !*s)
      break;
    printf("Starting new try: '%s'\n", s + 1);
  }
  return 0;   // no match
}

void RegExp::printProgram(const char *prog)
{
#if 1
  prog = 0;     // fix for /W4
#else
  const char *progstart;
  int pc;
  unsigned len;
  unsigned n;
  unsigned m;

  debug("printProgram()");
  progstart = prog;
  for (;;)
  {
    pc = prog - progstart;
    debug("%3d: ", pc);

    switch (*prog)
    {
    case REchar:
      debug("\tREchar '%c'", *(unsigned char *)(prog + 1));
      prog += 1 + sizeof(char);
      break;

    case REichar:
      debug("\tREichar '%c'", *(unsigned char *)(prog + 1));
      prog += 1 + sizeof(char);
      break;

    case REwchar:
      debug("\tREwchar '%c'", *(wchar_t *)(prog + 1));
      prog += 1 + sizeof(wchar_t);
      break;

    case REiwchar:
      debug("\tREiwchar '%c'", *(wchar_t *)(prog + 1));
      prog += 1 + sizeof(wchar_t);
      break;

    case REanychar:
      debug("\tREanychar");
      prog++;
      break;

    case REstring:
      len = *(unsigned *)(prog + 1);
      debug("\tREstring x%x, '%c'", len,
        *(TCHAR *)(prog + 1 + sizeof(unsigned)));
      prog += 1 + sizeof(unsigned) + len * sizeof(TCHAR);
      break;

    case REistring:
      len = *(unsigned *)(prog + 1);
      debug("\tREistring x%x, '%c'", len,
        *(TCHAR *)(prog + 1 + sizeof(unsigned)));
      prog += 1 + sizeof(unsigned) + len * sizeof(TCHAR);
      break;

    case REtestbit:
      debug("\tREtestbit %d, %d",
          ((unsigned short *)(prog + 1))[0],
          ((unsigned short *)(prog + 1))[1]);
      len = ((unsigned short *)(prog + 1))[1];
      prog += 1 + 2 * sizeof(unsigned short) + len;
      break;

    case REbit:
      debug("\tREbit %d, %d",
          ((unsigned short *)(prog + 1))[0],
          ((unsigned short *)(prog + 1))[1]);
      len = ((unsigned short *)(prog + 1))[1];
      prog += 1 + 2 * sizeof(unsigned short) + len;
      break;

    case REnotbit:
      debug("\tREnotbit %d, %d",
          ((unsigned short *)(prog + 1))[0],
          ((unsigned short *)(prog + 1))[1]);
      len = ((unsigned short *)(prog + 1))[1];
      prog += 1 + 2 * sizeof(unsigned short) + len;
      break;

    case RErange:
      debug("\tRErange %d", *(unsigned *)(prog + 1));
      // BUG: REAignoreCase?
      len = *(unsigned *)(prog + 1);
      prog += 1 + sizeof(unsigned) + len;
      break;

    case REnotrange:
      debug("\tREnotrange %d", *(unsigned *)(prog + 1));
      // BUG: REAignoreCase?
      len = *(unsigned *)(prog + 1);
      prog += 1 + sizeof(unsigned) + len;
      break;

    case REbol:
      debug("\tREbol");
      prog++;
      break;

    case REeol:
      debug("\tREeol");
      prog++;
      break;

    case REor:
      len = ((unsigned *)(prog + 1))[0];
      debug("\tREor %d, pc=>%d", len, pc + 1 + sizeof(unsigned) + len);
      prog += 1 + sizeof(unsigned);
      break;

    case REgoto:
      len = ((unsigned *)(prog + 1))[0];
      debug("\tREgoto %d, pc=>%d", len, pc + 1 + sizeof(unsigned) + len);
      prog += 1 + sizeof(unsigned);
      break;

    case REanystar:
      debug("\tREanystar");
      prog++;
      break;

    case REnm:
    case REnmq:
      // len, n, m, ()
      len = ((unsigned *)(prog + 1))[0];
      n = ((unsigned *)(prog + 1))[1];
      m = ((unsigned *)(prog + 1))[2];
      debug("\tREnm%s len=%d, n=%u, m=%u, pc=>%d",
        (*prog == REnmq) ? L"q" : L"", len, n, m, pc + 1 + sizeof(unsigned) * 3 + len);
      prog += 1 + sizeof(unsigned) * 3;
      break;

    case REparen:
      // len, ()
      len = ((unsigned *)(prog + 1))[0];
      n = ((unsigned *)(prog + 1))[1];
      debug("\tREparen len=%d n=%d, pc=>%d", len, n, pc + 1 + sizeof(unsigned) * 2 + len);
      prog += 1 + sizeof(unsigned) * 2;
      break;

    case REend:
      debug("\tREend");
      return;

    case REwordboundary:
      debug("\tREwordboundary");
      prog++;
      break;

    case REnotwordboundary:
      debug("\tREnotwordboundary");
      prog++;
      break;

    case REdigit:
      debug("\tREdigit");
      prog++;
      break;

    case REnotdigit:
      debug("\tREnotdigit");
      prog++;
      break;

    case REspace:
      debug("\tREspace");
      prog++;
      break;

    case REnotspace:
      debug("\tREnotspace");
      prog++;
      break;

    case REword:
      debug("\tREword");
      prog++;
      break;

    case REnotword:
      debug("\tREnotword");
      prog++;
      break;

    case REbackref:
      debug("\tREbackref %d", prog[1]);
      prog += 2;
      break;

    default:
      assert(0);
    }
  }
#endif
}

int RegExp::trymatch(const char *prog, const char *progend)
{
  const TCHAR *srcsave;
  unsigned len;
  unsigned n;
  unsigned m;
  unsigned count;
  const char *pop;
  const TCHAR *ss;
  Match *psave;
  unsigned c1;
  unsigned c2;
  int pc;

  pc = prog - program;
#if _UNICODE
  wprintf(L"RegExp::trymatch(prog[%d], src = '%s', end = [%d])\n",
    pc, src, progend ? progend - program : -1);
#else
  printf("RegExp::trymatch(prog = %p, src = '%s', end = %p)\n", prog, src, progend);
#endif
  srcsave = src;
  psave = NULL;
  for (;;)
  {
    if (prog == progend)    // if done matching
    {
      printf("\tprogend\n");
      return 1;
    }

    //printf("\top = %d\n", *prog);
    switch (*prog)
    {
    case REchar:
      wprintf(L"\tREchar '%c', src = '%c'\n", *(unsigned char *)(prog + 1), _tcsnextc(src));
      if (*(unsigned char *)(prog + 1) != _tcsnextc(src))
        goto Lnomatch;
      src = _tcsinc(src);
      prog += 1 + sizeof(char);
      break;

    case REichar:
      printf("\tREichar '%c', src = '%c'\n", *(unsigned char *)(prog + 1), _tcsnextc(src));
      c1 = *(unsigned char *)(prog + 1);
      c2 = _tcsnextc(src);
      if (c1 != c2)
      {
        if (use_istlower((TCHAR)c2))
          c2 = use_totupper((TCHAR)c2);
        else
          goto Lnomatch;
        if (c1 != c2)
          goto Lnomatch;
      }
      src = _tcsinc(src);
      prog += 1 + sizeof(char);
      break;

    case REwchar:
      printf("\tREwchar '%c', src = '%c'\n", *(wchar_t *)(prog + 1), _tcsnextc(src));
      if (*(wchar_t *)(prog + 1) != _tcsnextc(src))
        goto Lnomatch;
      src = _tcsinc(src);
      prog += 1 + sizeof(wchar_t);
      break;

    case REiwchar:
      printf("\tREiwchar '%c', src = '%c'\n", *(wchar_t *)(prog + 1), _tcsnextc(src));
      c1 = *(wchar_t *)(prog + 1);
      c2 = _tcsnextc(src);
      if (c1 != c2)
      {
        if (use_istlower((TCHAR)c2))
          c2 = use_totupper((TCHAR)c2);
        else
          goto Lnomatch;
        if (c1 != c2)
          goto Lnomatch;
      }
      src = _tcsinc(src);
      prog += 1 + sizeof(wchar_t);
      break;

    case REanychar:
      printf("\tREanychar\n");
      if (!*src)
        goto Lnomatch;
      if (!(attributes & REAdotmatchlf) && *src == '\n')
        goto Lnomatch;
      src = _tcsinc(src);
      prog++;
      break;

    case REstring:
      //wprintf(L"\tREstring x%x, '%c'\n", *(unsigned *)(prog + 1),
      //  *(TCHAR *)(prog + 1 + sizeof(unsigned)));
      len = *(unsigned *)(prog + 1);
      if (src+len > src_end)
        goto Lnomatch;
      if (memcmp(prog + 1 + sizeof(unsigned), src, len * sizeof(TCHAR)))
        goto Lnomatch;
      src += len;
      prog += 1 + sizeof(unsigned) + len * sizeof(TCHAR);
      break;

    case REistring:
      //wprintf(L"\tREstring x%x, '%c'\n", *(unsigned *)(prog + 1),
      //  *(TCHAR *)(prog + 1 + sizeof(unsigned)));
      len = *(unsigned *)(prog + 1);
      if (src+len > src_end)
        goto Lnomatch;
      if (use_memicmp(prog + 1 + sizeof(unsigned), src, len * sizeof(TCHAR)))
        goto Lnomatch;
      src += len;
      prog += 1 + sizeof(unsigned) + len * sizeof(TCHAR);
      break;

    case REtestbit:
      printf("\tREtestbit %d, %d, '%c', x%02x\n",
        ((unsigned short *)(prog + 1))[0],
        ((unsigned short *)(prog + 1))[1], _tcsnextc(src), _tcsnextc(src));
      len = ((unsigned short *)(prog + 1))[1];
      c1 = _tcsnextc(src);
      //printf("[x%02x]=x%02x, x%02x\n", c1 >> 3, ((prog + 1 + 4)[c1 >> 3] ), (1 << (c1 & 7)));
      if (c1 <= ((unsigned short *)(prog + 1))[0] &&
          !((prog + 1 + 4)[c1 >> 3] & (1 << (c1 & 7))))
          goto Lnomatch;
      prog += 1 + 2 * sizeof(unsigned short) + len;
      break;

    case REbit:
      printf("\tREbit %d, %d, '%c'\n",
        ((unsigned short *)(prog + 1))[0],
        ((unsigned short *)(prog + 1))[1], _tcsnextc(src));
      len = ((unsigned short *)(prog + 1))[1];
      c1 = _tcsnextc(src);
      if (c1 > ((unsigned short *)(prog + 1))[0])
        goto Lnomatch;
      if (!((prog + 1 + 4)[c1 >> 3] & (1 << (c1 & 7))))
        goto Lnomatch;
      src = _tcsinc(src);
      prog += 1 + 2 * sizeof(unsigned short) + len;
      break;

    case REnotbit:
      printf("\tREnotbit %d, %d, '%c'\n",
        ((unsigned short *)(prog + 1))[0],
        ((unsigned short *)(prog + 1))[1], _tcsnextc(src));
      len = ((unsigned short *)(prog + 1))[1];
      c1 = _tcsnextc(src);
      if (c1 <= ((unsigned short *)(prog + 1))[0] &&
          ((prog + 1 + 4)[c1 >> 3] & (1 << (c1 & 7))))
          goto Lnomatch;
      src = _tcsinc(src);
      prog += 1 + 2 * sizeof(unsigned short) + len;
      break;

    case RErange:
      printf("\tRErange %d\n", *(unsigned *)(prog + 1));
      // BUG: REAignoreCase?
      len = *(unsigned *)(prog + 1);
      if (memchr(prog + 1 + sizeof(unsigned), _tcsnextc(src), len) == NULL)
        goto Lnomatch;
      src = _tcsinc(src);
      prog += 1 + sizeof(unsigned) + len;
      break;

    case REnotrange:
      printf("\tREnotrange %d\n", *(unsigned *)(prog + 1));
      // BUG: REAignoreCase?
      len = *(unsigned *)(prog + 1);
      if (memchr(prog + 1 + sizeof(unsigned), _tcsnextc(src), len) != NULL)
        goto Lnomatch;
      src = _tcsinc(src);
      prog += 1 + sizeof(unsigned) + len;
      break;

    case REbol:
      printf("\tREbol\n");
      if (src == input)
        ;
      else if (attributes & REAmultiline)
      {
        const TCHAR *p;

        p = _tcsdec(input, src);
        if (_tcsnextc(p) != '\n')
          goto Lnomatch;
      }
      else
        goto Lnomatch;
      prog++;
      break;

    case REeol:
      printf("\tREeol\n");
      if (*src == 0)
        ;
      else if (attributes & REAmultiline && *src == '\n')
        src = _tcsinc(src);
      else
        goto Lnomatch;
      prog++;
      break;

    case REor:
      printf("\tREor %d\n", ((unsigned *)(prog + 1))[0]);
      len = ((unsigned *)(prog + 1))[0];
      pop = prog + 1 + sizeof(unsigned);
      ss = src;
      if (trymatch(pop, progend))
      {
        if (progend)
        {
          const TCHAR *s;

          s = src;
          if (trymatch(progend, NULL))
          {
            printf("\tfirst operand matched\n");
            src = s;
            return 1;
          }
          else
          {
            // If second branch doesn't match to end, take first anyway
            src = ss;
            if (!trymatch(pop + len, NULL))
            {
              printf("\tfirst operand matched\n");
              src = s;
              return 1;
            }
          }
          src = ss;
        }
        else
        {
          printf("\tfirst operand matched\n");
          return 1;
        }
      }
      prog = pop + len; // proceed with 2nd branch
      break;

    case REgoto:
      printf("\tREgoto\n");
      len = ((unsigned *)(prog + 1))[0];
      prog += 1 + sizeof(unsigned) + len;
      break;

    case REanystar:
      printf("\tREanystar\n");
      prog++;
      for (;;)
      {
        const TCHAR *s1;
        const TCHAR *s2;

        s1 = src;
        if (!*src)
          break;
        if (!(attributes & REAdotmatchlf) && *src == '\n')
          break;
        src = _tcsinc(src);
        s2 = src;

        // If no match after consumption, but it
        // did match before, then no match
        if (!trymatch(prog, NULL))
        {
          src = s1;
          // BUG: should we save/restore pmatch[]?
          if (trymatch(prog, NULL))
          {
            src = s1;   // no match
            break;
          }
        }
        src = s2;
      }
      break;

    case REnm:
    case REnmq:
      // len, n, m, ()
      len = ((unsigned *)(prog + 1))[0];
      n = ((unsigned *)(prog + 1))[1];
      m = ((unsigned *)(prog + 1))[2];
      wprintf(L"\tREnm%s len=%d, n=%u, m=%u\n", (*prog == REnmq) ? L"q" : L"", len, n, m);
      pop = prog + 1 + sizeof(unsigned) * 3;
      for (count = 0; count < n; count++)
      {
        if (!trymatch(pop, pop + len))
          goto Lnomatch;
      }
      if (!psave && count < m)
      {
        psave = (Match *)alloca((re_nsub + 1) * sizeof(Match));
      }
      if (*prog == REnmq) // if minimal munch
      {
        for (; count < m; count++)
        {
          const TCHAR *s1;

          memcpy(psave, pmatch, (re_nsub + 1) * sizeof(Match));
          s1 = src;

          if (trymatch(pop + len, NULL))
          {
            src = s1;
            memcpy(pmatch, psave, (re_nsub + 1) * sizeof(Match));
            break;
          }

          if (!trymatch(pop, pop + len))
          {
            printf("\tdoesn't match subexpression\n");
            break;
          }

          // If source is not consumed, don't
          // infinite loop on the match
          if (s1 == src)
          {
            printf("\tsource is not consumed\n");
            break;
          }
        }
      }
      else  // maximal munch
      {
        for (; count < m; count++)
        {
          const TCHAR *s1;
          const TCHAR *s2;

          memcpy(psave, pmatch, (re_nsub + 1) * sizeof(Match));
          s1 = src;
          if (!trymatch(pop, pop + len))
          {
            printf("\tdoesn't match subexpression\n");
            break;
          }
          s2 = src;

          // If source is not consumed, don't
          // infinite loop on the match
          if (s1 == s2)
          {
            printf("\tsource is not consumed\n");
            break;
          }

          // If no match after consumption, but it
          // did match before, then no match
          if (!trymatch(pop + len, NULL))
          {
            src = s1;
            if (trymatch(pop + len, NULL))
            {
              src = s1;   // no match
              memcpy(pmatch, psave, (re_nsub + 1) * sizeof(Match));
              break;
            }
          }
          src = s2;
        }
      }
      printf("\tREnm len=%d, n=%u, m=%u, DONE count=%d\n", len, n, m, count);
      prog = pop + len;
      break;

    case REparen:
      // len, ()
      printf("\tREparen\n");
      len = ((unsigned *)(prog + 1))[0];
      n = ((unsigned *)(prog + 1))[1];
      pop = prog + 1 + sizeof(unsigned) * 2;
      ss = src;
      if (!trymatch(pop, pop + len))
        goto Lnomatch;
      pmatch[n + 1].rm_so = ss - input;
      pmatch[n + 1].rm_eo = src - input;
      prog = pop + len;
      break;

    case REend:
      printf("\tREend\n");
      return 1;   // successful match

    case REwordboundary:
      printf("\tREwordboundary\n");
      if (src != input)
      {
        c1 = _tcsnextc(_tcsdec(input, src));
        c2 = _tcsnextc(src);
        if (!((isword((TCHAR)c1) && !isword((TCHAR)c2)) ||
              (!isword((TCHAR)c1) && isword((TCHAR)c2))))
          goto Lnomatch;
      }
      prog++;
      break;

    case REnotwordboundary:
      printf("\tREnotwordboundary\n");
      if (src == input)
        goto Lnomatch;
      c1 = _tcsnextc(_tcsdec(input, src));
      c2 = _tcsnextc(src);
      if ((isword((TCHAR)c1) && !isword((TCHAR)c2)) ||
          (!isword((TCHAR)c1) && isword((TCHAR)c2)))
        goto Lnomatch;
      prog++;
      break;

    case REdigit:
      printf("\tREdigit\n");
      if (!_istdigit(_tcsnextc(src)))
        goto Lnomatch;
      src = _tcsinc(src);
      prog++;
      break;

    case REnotdigit:
      printf("\tREnotdigit\n");
      c1 = _tcsnextc(src);
      if (!c1 || _istdigit((TCHAR)c1))
        goto Lnomatch;
      src = _tcsinc(src);
      prog++;
      break;

    case REspace:
      printf("\tREspace\n");
      if (!isspace(_tcsnextc(src)))
        goto Lnomatch;
      src = _tcsinc(src);
      prog++;
      break;

    case REnotspace:
      printf("\tREnotspace\n");
      c1 = _tcsnextc(src);
      if (!c1 || isspace(c1))
        goto Lnomatch;
      src = _tcsinc(src);
      prog++;
      break;

    case REword:
      printf("\tREword\n");
      if (!isword(_tcsnextc(src)))
        goto Lnomatch;
      src = _tcsinc(src);
      prog++;
      break;

    case REnotword:
      printf("\tREnotword\n");
      c1 = _tcsnextc(src);
      if (!c1 || isword((TCHAR)c1))
        goto Lnomatch;
      src = _tcsinc(src);
      prog++;
      break;

    case REbackref:
      printf("\tREbackref %d\n", prog[1]);
      n = prog[1];
      len = pmatch[n + 1].rm_eo - pmatch[n + 1].rm_so;
      if (src+len > src_end)
        goto Lnomatch;
      if (attributes & REAignoreCase)
      {
    #if _UNICODE
        if (_wcsnicmp(src, input + pmatch[n + 1].rm_so, len))
    #else
        if (use_memicmp(src, input + pmatch[n + 1].rm_so, len))
    #endif
        goto Lnomatch;
      }
      else if (memcmp(src, input + pmatch[n + 1].rm_so, len * sizeof(TCHAR)))
        goto Lnomatch;
      src += len;
      prog += 2;
      break;

    default:
      assert(0);
    }
  }

Lnomatch:
  wprintf(L"\tnomatch pc=%d\n", pc);
  src = srcsave;
  return 0;
}

/* =================== Compiler ================== */

int RegExp::parseRegexp()
{
  size_t offset;
  size_t gotooffset;
  size_t len1;
  size_t len2;

  //wprintf(L"parseRegexp() '%s'\n", p);
  offset = buf->offset;
  for (;;)
  {
    switch (*p)
    {
    case ')':
      return 1;

    case 0:
      buf->writeByte(REend);
      return 1;

    case '|':
      p++;
      gotooffset = buf->offset;
      buf->writeByte(REgoto);
      buf->write4(0);
      len1 = buf->offset - offset;
      buf->spread(offset, 1 + sizeof(unsigned));
      gotooffset += 1 + sizeof(unsigned);
      parseRegexp();
      len2 = buf->offset - (gotooffset + 1 + sizeof(unsigned));
      buf->data[offset] = REor;
      ((unsigned *)(buf->data + offset + 1))[0] = (int)len1;
      ((unsigned *)(buf->data + gotooffset + 1))[0] = (int)len2;
      break;

    default:
      parsePiece();
      break;
    }
  }
}

int RegExp::parsePiece()
{
  size_t offset;
  size_t len;
  unsigned n;
  unsigned m;
  char op;

  //wprintf(L"parsePiece() '%s'\n", p);
  offset = buf->offset;
  parseAtom();
  switch (*p)
  {
  case '*':
    // Special optimization: replace .* with REanystar
    if (buf->offset - offset == 1 &&
        buf->data[offset] == REanychar &&
        p[1] != '?')
    {
      buf->data[offset] = REanystar;
      p++;
      break;
    }

    n = 0;
    m = inf;
    goto Lnm;

  case '+':
    n = 1;
    m = inf;
    goto Lnm;

  case '?':
    n = 0;
    m = 1;
    goto Lnm;

  case '{': // {n} {n,} {n,m}
    p++;
    if (!_istdigit(*p))
      goto Lerr;
    n = 0;
    do
    {
      // BUG: handle overflow
      n = n * 10 + *p - '0';
      p++;
    } while (_istdigit(*p));
    if (*p == '}')    // {n}
    {
      m = n;
      goto Lnm;
    }
    if (*p != ',')
      goto Lerr;
    p++;
    if (*p == '}')    // {n,}
    {
      m = inf;
      goto Lnm;
    }
    if (!_istdigit(*p))
      goto Lerr;
    m = 0;      // {n,m}
    do
    {
      // BUG: handle overflow
      m = m * 10 + *p - '0';
      p++;
    } while (_istdigit(*p));
    if (*p != '}')
      goto Lerr;
    goto Lnm;

  Lnm:
    p++;
    op = REnm;
    if (*p == '?')
    {
      op = REnmq; // minimal munch version
      p++;
    }
    len = buf->offset - offset;
    buf->spread(offset, 1 + sizeof(unsigned) * 3);
    buf->data[offset] = op;
    ((unsigned *)(buf->data + offset + 1))[0] = (int)len;
    ((unsigned *)(buf->data + offset + 1))[1] = n;
    ((unsigned *)(buf->data + offset + 1))[2] = m;
    break;
  }
  return 1;

Lerr:
  error("badly formed {n,m}");
  return 0;
}

int RegExp::parseAtom()
{
  char op;
  size_t offset;
  unsigned c;

  //wprintf(L"parseAtom() '%s'\n", p);
  c = *p;
  switch (c)
  {
  case '*':
  case '+':
  case '?':
    error("*+? not allowed in atom");
    p++;
    return 0;

  case '(':
    p++;
    buf->writeByte(REparen);
    offset = buf->offset;
    buf->write4(0);   // reserve space for length
    buf->write4(re_nsub);
    re_nsub++;
    parseRegexp();
    *(unsigned *)(buf->data + offset) =
      (int)(buf->offset - (offset + sizeof(unsigned) * 2));
    if (*p != ')')
    {
      error("')' expected");
      return 0;
    }
    p++;
    break;

  case '[':
    if (!parseRange())
      return 0;
    break;

  case '.':
    p++;
    buf->writeByte(REanychar);
    break;

  case '^':
    p++;
    buf->writeByte(REbol);
    break;

  case '$':
    p++;
    buf->writeByte(REeol);
    break;

  case 0:
    break;

  case '\\':
    p++;
    switch (*p)
    {
    case 0:
      error("no character past '\\'");
      return 0;

    case 'b':    op = REwordboundary; goto Lop;
    case 'B':    op = REnotwordboundary;  goto Lop;
    case 'd':    op = REdigit;    goto Lop;
    case 'D':    op = REnotdigit;   goto Lop;
    case 's':    op = REspace;    goto Lop;
    case 'S':    op = REnotspace;   goto Lop;
    case 'w':    op = REword;   goto Lop;
    case 'W':    op = REnotword;    goto Lop;

    Lop:
      buf->writeByte(op);
      p++;
      break;

    case 'f':
    case 'n':
    case 'r':
    case 't':
    case 'v':
    case 'c':
    case 'x':
    case 'u':
    case '0':
      c = escape();
      goto Lbyte;

    case '1': case '2': case '3':
    case '4': case '5': case '6':
    case '7': case '8': case '9':
      c = *p - '1';
      if (c < re_nsub)
      {
        buf->writeByte(REbackref);
        buf->writeByte(c);
      }
      else
      {
        error("no matching back reference");
        return 0;
      }
      p++;
      break;

    default:
      c = _tcsnextc(p);
      p = _tcsinc(p);
      goto Lbyte;
    }
    break;

  default:
    c = _tcsnextc(p);
    p = _tcsinc(p);

  Lbyte:
    op = REchar;
    if (attributes & REAignoreCase)
    {
      if (_istalpha((TCHAR)c))
      {
        op = REichar;
        c = use_totupper((TCHAR)c);
      }
    }
    if (op == REchar && c <= 0xFF)
    {
      // Look ahead and see if we can make this into
      // an REstring
      const TCHAR *q;
      int len;

      for (q = p; ; q = _tcsinc(q))
      {
        TCHAR qc = (TCHAR)_tcsnextc(q);

        switch (qc)
        {
        case '{':
        case '*':
        case '+':
        case '?':
          if (q == p)
            goto Lchar;
          q--;
          break;

        case '(': case ')':
        case '|':
        case '[': case ']':
        case '.': case '^':
        case '$': case '\\':
        case '}': case 0:
          break;

        default:
          continue;
        }
        break;
      }
      len = q - p;
      if (len > 0)
      {
        wprintf(L"writing string len %d, c = '%c', *p = '%c'\n", len+1, c, *p);
        buf->reserve(5 + (1 + len) * sizeof(TCHAR));
        buf->writeByte((attributes & REAignoreCase) ? REistring : REstring);
        buf->write4(len + 1);
        buf->writeTCHAR(c);
        buf->write(p, len * sizeof(TCHAR));
        p = q;
        break;
      }
    }
    if (c & ~0xFF)
    {
      // Convert to 16 bit opcode
      op = (char)((op == REchar) ? REwchar : REiwchar);
      buf->writeByte(op);
      buf->writeword(c);
    }
    else
    {
    Lchar:
      printf("It's an REchar '%c'\n", c);
      buf->writeByte(op);
      buf->writeByte(c);
    }
    break;
  }
  return 1;
}

#if 1

struct RegExp::Range
{
  unsigned maxc;
  unsigned maxb;
  RegExp::RegBuffer *buf;
  unsigned char *base;

  Range(RegBuffer *buf)
  {
    this->buf = buf;
    this->base = &buf->data[buf->offset];
    maxc = 0;
    maxb = 0;
  }

  void setbitmax(unsigned u)
  {
    unsigned b;

    if (u > maxc)
    {
      maxc = u;
      b = u >> 3;
      if (b >= maxb)
      {
        unsigned u;

        u = base - buf->data;
        buf->fill0(b - maxb + 1);
        base = buf->data + u;
        maxb = b + 1;
      }
    }
  }

  void setbit(unsigned u)
  {
    //printf("setbit x%02x\n", 1 << (u & 7));
    base[u >> 3] |= 1 << (u & 7);
  }

  void setbit2(unsigned u)
  {
    setbitmax(u + 1);
    //printf("setbit2 [x%02x] |= x%02x\n", u >> 3, 1 << (u & 7));
    base[u >> 3] |= 1 << (u & 7);
  }

  int testbit(unsigned u)
  {
    return base[u >> 3] & (1 << (u & 7));
  }

  void negate()
  {
    for (unsigned b = 0; b < maxb; b++)
    {
      base[b] = (unsigned char)~base[b];
    }
  }
};

int RegExp::parseRange()
{
  char op;
  int c = -1;   // initialize to keep /W4 happy
  int c2 = -1;  // initialize to keep /W4 happy
  unsigned i;
  unsigned cmax;
  size_t offset;

  cmax = 0x7F;
  p++;
  op = REbit;
  if (*p == '^')
  {
    p++;
    op = REnotbit;
  }
  buf->writeByte(op);
  offset = buf->offset;
  buf->write4(0);   // reserve space for length
  buf->reserve(128 / 8);
  Range r(buf);
  if (op == REnotbit)
    r.setbit2(0);
  switch (*p)
  {
  case ']':
  case '-':
    c = *p;
    p = _tcsinc(p);
    r.setbit2(c);
    break;
  }

  enum RangeState { RSstart, RSrliteral, RSdash };
  RangeState rs;

  rs = RSstart;
  for (;;)
  {
    switch (*p)
    {
    case ']':
      switch (rs)
      {
        case RSdash:
          r.setbit2('-');
        case RSrliteral:
          r.setbit2(c);
        case RSstart:
        break;
      }
      p++;
      break;

    case '\\':
      p++;
      r.setbitmax(cmax);
      switch (*p)
      {
      case 'd':
        for (i = '0'; i <= '9'; i++)
          r.setbit(i);
        goto Lrs;

      case 'D':
        for (i = 1; i < '0'; i++)
          r.setbit(i);
        for (i = '9' + 1; i <= cmax; i++)
          r.setbit(i);
        goto Lrs;

      case 's':
        for (i = 0; i <= cmax; i++)
          if (isspace(i))
        r.setbit(i);
        goto Lrs;

      case 'S':
        for (i = 1; i <= cmax; i++)
          if (!isspace(i))
        r.setbit(i);
        goto Lrs;

      case 'w':
        for (i = 0; i <= cmax; i++)
          if (isword((TCHAR)i))
        r.setbit(i);
        goto Lrs;

      case 'W':
        for (i = 1; i <= cmax; i++)
          if (!isword((TCHAR)i))
        r.setbit(i);
        goto Lrs;

      Lrs:
        switch (rs)
        {
          case RSdash:
            r.setbit2('-');
          case RSrliteral:
            r.setbit2(c);
          case RSstart:
            break;
        }
        rs = RSstart;
        continue;
      }
      c2 = escape();
      goto Lrange;

    case '-':
      p++;
      if (rs == RSstart)
        goto Lrange;
      else if (rs == RSrliteral)
        rs = RSdash;
      else if (rs == RSdash)
      {
        r.setbit2(c);
        r.setbit2('-');
        rs = RSstart;
      }
      continue;

    case 0:
      error("']' expected");
      return 0;

    default:
      c2 = _tcsnextc(p);
      p = _tcsinc(p);

    Lrange:
      switch (rs)
      {
      case RSrliteral:
        r.setbit2(c);
      case RSstart:
        c = c2;
        rs = RSrliteral;
        break;

      case RSdash:
        if (c > c2)
        {
          error("inverted range in character class");
          return 0;
        }
        r.setbitmax(c2);
        //printf("c = %x, c2 = %x\n",c,c2);
        for (; c <= c2; c++)
          r.setbit(c);
        rs = RSstart;
        break;
      }
      continue;
    }
    break;
  }
  //printf("maxc = %d, maxb = %d\n",r.maxc,r.maxb);
  ((unsigned short *)(buf->data + offset))[0] = (unsigned short)r.maxc;
  ((unsigned short *)(buf->data + offset))[1] = (unsigned short)r.maxb;
  if (attributes & REAignoreCase)
  {
    // BUG: what about unicode?
    r.setbitmax(0x7F);
    for (c = 'a'; c <= 'z'; c++)
    {
      if (r.testbit(c))
        r.setbit(c + 'A' - 'a');
      else if (r.testbit(c + 'A' - 'a'))
        r.setbit(c);
    }
  }
  return 1;
}
#else
int RegExp::parseRange()
{
  char op;
  unsigned offset;
  TCHAR c;
  TCHAR c1;
  TCHAR c2;

  p++;
  op = RErange;
  if (*p == '^')
  {
    p++;
    op = REnotrange;
  }
  buf->writeByte(op);
  offset = buf->offset;
  buf->write4(0);   // reserve space for length
  switch (*p)
  {
  case ']':
  case '-':
    buf->writeByte(*p++);
    break;
  }
  for (;;)
  {
    switch (*p)
    {
    case ']':
      p++;
      break;

    case '-':
      p++;
      c2 = p[0];
      if (c2 == ']')    // if "-]"
      {
        buf->writeByte('-');
        continue;
      }
      c1 = p[-2];
      if (c1 > c2)
      {
        error("range not in order");
        return 0;
      }
      for (c1++; c1 <= c2; c1++)
        buf->writeByte(c1);
      p++;
      continue;

    case '\\':
      p++;
    #if 0 // BUG: should handle these cases
      switch (*p)
      {
        case 'd':    op = REdigit;
        case 'D':    op = REnotdigit;
        case 's':    op = REspace;
        case 'S':    op = REnotspace;
        case 'w':    op = REword;
        case 'W':    op = REnotword;
      }
    #endif
      c = escape();
      buf->writeByte(c);
      break;

    case 0:
      error("']' expected");
      return 0;

    default:
      buf->writeByte(*p++);
      continue;
    }
    break;
  }

  *(unsigned *)(buf->data + offset) = buf->offset - (offset + sizeof(unsigned));
  return 1;
}
#endif

void RegExp::error(const char *msg)
{
  (void) msg;     // satisfy /W4
  p += _tcslen(p);  // advance to terminating 0
  debug("RegExp.compile() error: %s\n", msg);
  errors++;
}

// p is following the \ char
int RegExp::escape()
{
  int c;
  int i;

  c = *p;   // none of the cases are multibyte
  switch (c)
  {
  case 'b':    c = '\b';  break;
  case 'f':    c = '\f';  break;
  case 'n':    c = '\n';  break;
  case 'r':    c = '\r';  break;
  case 't':    c = '\t';  break;
  case 'v':    c = '\v';  break;

  // BUG: Perl does \a and \e too, should we?

  case 'c':
    p = _tcsinc(p);
    c = _tcsnextc(p);
    // Note: we are deliberately not allowing Unicode letters
    if (!(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')))
    {
      error("letter expected following \\c");
      return 0;
    }
    c &= 0x1F;
    break;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
    c -= '0';
    for (i = 0; i < 2; i++)
    {
      p++;
      if ('0' <= *p && *p <= '7')
      {
        c = c * 8 + (*p - '0');
        // Treat overflow as if last
        // digit was not an octal digit
        if (c >= 0xFF)
        {
          c >>= 3;
          return c;
        }
      }
      else
        return c;
    }
    break;

  case 'x':
    c = 0;
    for (i = 0; i < 2; i++)
    {
      p++;
      if ('0' <= *p && *p <= '9')
        c = c * 16 + (*p - '0');
      else if ('a' <= *p && *p <= 'f')
        c = c * 16 + (*p - 'a' + 10);
      else if ('A' <= *p && *p <= 'F')
        c = c * 16 + (*p - 'A' + 10);
      else if (i == 0)  // if no hex digits after \x
      {
        // Not a valid \xXX sequence
        return 'x';
      }
      else
        return c;
    }
    break;

  case 'u':
    c = 0;
    for (i = 0; i < 4; i++)
    {
      p++;
      if ('0' <= *p && *p <= '9')
        c = c * 16 + (*p - '0');
      else if ('a' <= *p && *p <= 'f')
        c = c * 16 + (*p - 'a' + 10);
      else if ('A' <= *p && *p <= 'F')
        c = c * 16 + (*p - 'A' + 10);
      else
      {
          // Not a valid \uXXXX sequence
          p -= i;
          return 'u';
      }
    }
    break;

  default:
    c = _tcsnextc(p);
    p = _tcsinc(p);
    return c;
  }

  p++;
  return c;
}

/* ==================== optimizer ======================= */

void RegExp::optimize()
{
  char *prog;

  printf("RegExp::optimize()\n");
  prog = (char *)buf->data;
  for (;;)
  {
    switch (*prog)
    {
    case REend:
    case REanychar:
    case REanystar:
    case REbackref:
    case REeol:
    case REchar:
    case REichar:
    case REwchar:
    case REiwchar:
    case REstring:
    case REistring:
    case REtestbit:
    case REbit:
    case REnotbit:
    case RErange:
    case REnotrange:
    case REwordboundary:
    case REnotwordboundary:
    case REdigit:
    case REnotdigit:
    case REspace:
    case REnotspace:
    case REword:
    case REnotword:
      return;

    case REbol:
      prog++;
      continue;

    case REor:
    case REnm:
    case REnmq:
    case REparen:
    case REgoto:
      {
        RegBuffer bitbuf;
        unsigned offset;
        Range r(&bitbuf);

        offset = prog - (char *)buf->data;
        if (startchars(&r, prog, NULL))
        {
          printf("\tfilter built\n");
          buf->spread(offset, 1 + 4 + r.maxb);
          buf->data[offset] = REtestbit;
          ((unsigned short *)(buf->data + offset + 1))[0] = (unsigned short)r.maxc;
          ((unsigned short *)(buf->data + offset + 1))[1] = (unsigned short)r.maxb;
          memcpy(buf->data + offset + 1 + 4, r.base, r.maxb);
        }
      }
      return;

    default:
      assert(0);
    }
  }
}

/////////////////////////////////////////
// OR the leading character bits into r.
// Limit the character range from 0..7F,
// trymatch() will allow through anything over maxc.
// Return 1 if success, 0 if we can't build a filter or
// if there is no point to one.

int RegExp::startchars(Range *r, char *prog, char *progend)
{
  unsigned c;
  unsigned maxc;
  unsigned maxb;
  unsigned len;
  unsigned b;
  unsigned n;
  unsigned m;
  char *pop;

  //printf("RegExp::startchars(prog = %p, progend = %p)\n", prog, progend);
  for (;;)
  {
    if (prog == progend)
      return 1;
    switch (*prog)
    {
      case REchar:
      c = *(unsigned char *)(prog + 1);
      if (c <= 0x7F)
        r->setbit2(c);
      return 1;

    case REichar:
      c = *(unsigned char *)(prog + 1);
      if (c <= 0x7F)
      {
        r->setbit2(c);
        r->setbit2(_totlower((TCHAR)c));
      }
      return 1;

    case REwchar:
    case REiwchar:
      return 1;

    case REanychar:
      return 0;   // no point

    case REstring:
      printf("\tREstring %d, '%c'\n", *(unsigned *)(prog + 1),
        *(TCHAR *)(prog + 1 + sizeof(unsigned)));
      len = *(unsigned *)(prog + 1);
      assert(len);
      c = *(TCHAR *)(prog + 1 + sizeof(unsigned));
      if (c <= 0x7F)
        r->setbit2(c);
      return 1;

    case REistring:
      printf("\tREistring %d, '%c'\n", *(unsigned *)(prog + 1),
        *(TCHAR *)(prog + 1 + sizeof(unsigned)));
      len = *(unsigned *)(prog + 1);
      assert(len);
      c = *(TCHAR *)(prog + 1 + sizeof(unsigned));
      if (c <= 0x7F)
        r->setbit2(c);
      return 1;

    case REtestbit:
    case REbit:
      maxc = ((unsigned short *)(prog + 1))[0];
      maxb = ((unsigned short *)(prog + 1))[1];
      if (maxc <= 0x7F)
        r->setbitmax(maxc);
      else
        maxb = r->maxb;
      for (b = 0; b < maxb; b++)
        r->base[b] |= *(char *)(prog + 1 + 4 + b);
      return 1;

    case REnotbit:
      maxc = ((unsigned short *)(prog + 1))[0];
      maxb = ((unsigned short *)(prog + 1))[1];
      if (maxc <= 0x7F)
        r->setbitmax(maxc);
      else
        maxb = r->maxb;
      for (b = 0; b < maxb; b++)
        r->base[b] |= ~*(char *)(prog + 1 + 4 + b);
      return 1;

    case REbol:
    case REeol:
      return 0;

    case REor:
      len = ((unsigned *)(prog + 1))[0];
      pop = prog + 1 + sizeof(unsigned);
      return startchars(r, pop, progend) && startchars(r, pop + len, progend);

    case REgoto:
      len = ((unsigned *)(prog + 1))[0];
      prog += 1 + sizeof(unsigned) + len;
      break;

    case REanystar:
      return 0;

    case REnm:
    case REnmq:
      // len, n, m, ()
      len = ((unsigned *)(prog + 1))[0];
      n = ((unsigned *)(prog + 1))[1];
      m = ((unsigned *)(prog + 1))[2];
      pop = prog + 1 + sizeof(unsigned) * 3;
      if (!startchars(r, pop, pop + len))
        return 0;
      if (n)
        return 1;
      prog = pop + len;
      break;

    case REparen:
      // len, ()
      len = ((unsigned *)(prog + 1))[0];
      n = ((unsigned *)(prog + 1))[1];
      pop = prog + 1 + sizeof(unsigned) * 2;
      return startchars(r, pop, pop + len);

    case REend:
      return 0;

    case REwordboundary:
    case REnotwordboundary:
      return 0;

    case REdigit:
      r->setbitmax('9');
      for (c = '0'; c <= '9'; c++)
        r->setbit(c);
      return 1;

    case REnotdigit:
      r->setbitmax(0x7F);
      for (c = 0; c <= '0'; c++)
        r->setbit(c);
      for (c = '9' + 1; c <= r->maxc; c++)
        r->setbit(c);
      return 1;

    case REspace:
      r->setbitmax(0x7F);
      for (c = 0; c <= r->maxc; c++)
        if (isspace(c))
      r->setbit(c);
      return 1;

    case REnotspace:
      r->setbitmax(0x7F);
      for (c = 0; c <= r->maxc; c++)
        if (!isspace(c))
      r->setbit(c);
      return 1;

    case REword:
      r->setbitmax(0x7F);
      for (c = 0; c <= r->maxc; c++)
        if (isword((TCHAR)c))
      r->setbit(c);
      return 1;

    case REnotword:
      r->setbitmax(0x7F);
      for (c = 0; c <= r->maxc; c++)
        if (!isword((TCHAR)c))
      r->setbit(c);
      return 1;

    case REbackref:
      return 0;

    default:
      assert(0);
    }
  }
}

/* ==================== replace ======================= */

// This version of replace() uses:
//  & replace with the match
//  \n  replace with the nth parenthesized match, n is 1..9
//  \c  replace with char c

TCHAR *RegExp::replace(const TCHAR *format)
{
  RegBuffer buf;
  TCHAR *result;
  unsigned c;

  buf.reserve((_tcslen(format) + 1) * sizeof(TCHAR));
  for (; ; format = _tcsinc(format))
  {
    c = _tcsnextc(format);
    switch (c)
    {
    case 0:
      break;

    case '&':
      buf.write(input + pmatch[0].rm_so, (pmatch[0].rm_eo - pmatch[0].rm_so) * sizeof(TCHAR));
      continue;

    case '\\':
      format = _tcsinc(format);
      c = _tcsnextc(format);
      if (c >= '1' && c <= '9')
      {
        unsigned i;

        i = c - '0';
        if (i <= re_nsub)
          buf.write(input + pmatch[i].rm_so, (pmatch[i].rm_eo - pmatch[i].rm_so) * sizeof(TCHAR));
      }
      else if (!c)
        break;
      else
        buf.writeTCHAR(c);
      continue;

    default:
      buf.writeTCHAR(c);
      continue;
    }
    break;
  }

  buf.writeTCHAR(0);
  result = (TCHAR *)buf.data;
  buf.data = NULL;
  return result;
}

// This version of replace uses:
//  $$  $
//  $&  The matched substring.
//  $`  The portion of string that precedes the matched substring.
//  $'  The portion of string that follows the matched substring.
//  $n  The nth capture, where n is a single digit 1-9
//    and $n is not followed by a decimal digit.
//    If n<=m and the nth capture is undefined, use the empty
//    string instead. If n>m, the result is implementation-
//    defined.
//  $nn The nnth capture, where nn is a two-digit decimal
//    number 01-99.
//    If n<=m and the nth capture is undefined, use the empty
//    string instead. If n>m, the result is implementation-
//    defined.
//
//  Any other $ are left as is.

TCHAR *RegExp::replace2(const TCHAR *format)
{
  return replace3(format, input, re_nsub, pmatch);
}

// Static version that doesn't require a RegExp object to be created

TCHAR *RegExp::replace3(const TCHAR *format, const TCHAR *input,
  unsigned re_nsub, const RegExp::Match *pmatch)
{
  RegBuffer buf;
  TCHAR *result;
  unsigned c;
  unsigned c2;
  int rm_so;
  int rm_eo;
  int i;

  //wprintf(L"replace3(format = '%s', input = '%s', match = %p, re_nsub = %d, pmatch = %p\n",
  //  format, input, match, re_nsub, pmatch);
  buf.reserve((_tcslen(format) + 1) * sizeof(TCHAR));
  for (; ; format = _tcsinc(format))
  {
    c = _tcsnextc(format);
  
  L1:
    if (c == 0)
      break;
    if (c != '$')
    {
      buf.writeTCHAR(c);
      continue;
    }
    format = _tcsinc(format);
    c = _tcsnextc(format);

    switch (c)
    {
    case 0:
      buf.writeTCHAR('$');
      break;

    case '&':
      rm_so = pmatch[0].rm_so;
      rm_eo = pmatch[0].rm_eo;
      goto Lstring;

    case '`':
      rm_so = 0;
      rm_eo = pmatch[0].rm_so;
      goto Lstring;

    case '\'':
      rm_so = pmatch[0].rm_eo;
      rm_eo = (int)_tcslen(input);
      goto Lstring;

    Lstring:
      buf.write(input + rm_so,
        (rm_eo - rm_so) * sizeof(TCHAR));
      continue;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      c2 = format[1];
      if (c2 >= '0' && c2 <= '9')
      {
        i = (c - '0') * 10 + (c2 - '0');
        format = _tcsinc(format);
      }
      else
        i = c - '0';
      if (i == 0)
      {
        buf.writeTCHAR('$');
        buf.writeTCHAR(c);
        c = c2;
        goto L1;
      }

      if (i <= (int)re_nsub)
      {
        rm_so = pmatch[i].rm_so;
        rm_eo = pmatch[i].rm_eo;
        goto Lstring;
      }
      continue;

    default:
      buf.writeTCHAR('$');
      buf.writeTCHAR(c);
      continue;
    }
    break;
  }

  buf.writeTCHAR(0);    // terminate string
  result = (TCHAR *)buf.data;
  buf.data = NULL;
  return result;
}

////////////////////////////////////////////////////////
// Return a string that is [input] with [match] replaced by [replacement].

TCHAR *RegExp::replace4(const TCHAR *input, const RegExp::Match *match, const TCHAR *replacement)
{
  int input_len;
  int replacement_len;
  int result_len;
  TCHAR *result;

  input_len = (int)_tcslen(input);
  replacement_len = (int)_tcslen(replacement);
  result_len = input_len - (match->rm_eo - match->rm_so) + replacement_len;
  result = (TCHAR *)malloc((result_len + 1) * sizeof(TCHAR));
  memcpy(result, input, match->rm_so * sizeof(TCHAR));
  memcpy(result + match->rm_so, replacement, replacement_len * sizeof(TCHAR));
  memcpy(result + match->rm_so + replacement_len, input + match->rm_eo,
    (input_len - match->rm_eo) * sizeof(TCHAR));
  result[result_len] = 0;
  return result;
}

/************************** RegBuffer ***********************/

RegExp::RegBuffer::RegBuffer()
{
  data = NULL;
  offset = 0;
  size = 0;
}

RegExp::RegBuffer::~RegBuffer()
{
  free(data);
  data = NULL;
}

void RegExp::RegBuffer::reserve(size_t nbytes)
{
  //printf("RegBuffer::reserve: size = %d, offset = %d, nbytes = %d\n", size, offset, nbytes);
  if (size - offset < nbytes)
  {
    size = (offset + nbytes) * 2;
    data = (unsigned char *)realloc(data, size);
  }
}

void RegExp::RegBuffer::write(const void *data, size_t nbytes)
{
  reserve(nbytes);
  memcpy(this->data + offset, data, nbytes);
  offset += nbytes;
}

void RegExp::RegBuffer::writeByte(unsigned b)
{
  reserve(1);
  this->data[offset] = (unsigned char)b;
  offset++;
}

void RegExp::RegBuffer::writeword(unsigned w)
{
  reserve(2);
  *(unsigned short *)(this->data + offset) = (unsigned short)w;
  offset += 2;
}

void RegExp::RegBuffer::writeTCHAR(TCHAR b)
{
  TCHAR *p;
  size_t len;

  len = _tclen(&b) * sizeof(TCHAR);
  reserve(len);
  p = (TCHAR *)(this->data + offset);
  _tccpy(p, &b);
  offset += len;
}

void RegExp::RegBuffer::write4(unsigned w)
{
  reserve(4);
  *(unsigned long *)(this->data + offset) = w;
  offset += 4;
}

void RegExp::RegBuffer::write(RegBuffer *buf)
{
  if (buf)
  {
    reserve(buf->offset);
    memcpy(data + offset, buf->data, buf->offset);
    offset += buf->offset;
  }
}

void RegExp::RegBuffer::fill0(unsigned nbytes)
{
  reserve(nbytes);
  memset(data + offset,0,nbytes);
  offset += nbytes;
}

void RegExp::RegBuffer::spread(size_t offset, size_t nbytes)
{
  reserve(nbytes);
  memmove(data + offset + nbytes, data + offset, this->offset - offset);
  this->offset += nbytes;
}
