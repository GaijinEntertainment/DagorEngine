// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fs_hlp.h"

/* scans an asterisk */
static int asterisk(const char **wildcard, const char **test)
{
  /* Warning: uses multiple returns */
  int fit = 1;

  /* erase the leading asterisk */
  (*wildcard)++;
  while (('\000' != (**test)) && (('?' == **wildcard) || ('*' == **wildcard)))
  {
    if ('?' == **wildcard)
      (*test)++;
    (*wildcard)++;
  }
  /* Now it could be that test is empty and wildcard contains */
  /* aterisks. Then we delete them to get a proper state */
  while ('*' == (**wildcard))
    (*wildcard)++;

  if (('\0' == (**test)) && ('\0' != (**wildcard)))
    fit = 0;
  else if (('\0' == (**test)) && ('\0' == (**wildcard)))
    fit = 1;
  else
  {
    /* Neither test nor wildcard are empty!          */
    /* the first character of wildcard isn't in [*?] */
    if (0 == wildcardfit(*wildcard, (*test)))
    {
      do
      {
        (*test)++;
        /* skip as much characters as possible in the teststring */
        /* stop if a character match occurs */
        while (((**wildcard) != (**test)) && ('[' != (**wildcard)) && ('\0' != (**test)))
          (*test)++;
      } while ((('\0' != **test)) ? (0 == wildcardfit(*wildcard, (*test))) : (0 != (fit = 0)));
    }
    if (('\0' == **test) && ('\0' == **wildcard))
      fit = 1;
  }
  return fit;
}

/* this function implements the UN*X wildcards and returns  */
/* 0  if *wildcard does not match *test                     */
/* 1  if *wildcard matches *test                            */
int wildcardfit(const char *wildcard, const char *test)
{
  int fit = 1;

  for (; ('\000' != *wildcard) && (1 == fit) && ('\000' != *test); wildcard++)
  {
    switch (*wildcard)
    {
      case '?': test++; break;
      case '*':
        fit = asterisk(&wildcard, &test);
        /* the asterisk was skipped by asterisk() but the loop will */
        /* increment by itself. So we have to decrement */
        wildcard--;
        break;
      default: fit = (int)(*wildcard == *test); test++;
    }
  }
  while ((*wildcard == '*') && (1 == fit))
    /* here the teststring is empty otherwise you cannot */
    /* leave the previous loop */
    wildcard++;
  return (int)((1 == fit) && ('\0' == *test) && ('\0' == *wildcard));
}
