/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/


#include "tokenizer.h"
#include <stdio.h>
#include "platform.h"

bool isWhiteSpace(const char ch) { return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'); }

bool isNumeric(const char ch) { return (ch >= '0' && ch <= '9'); }

bool isAlphabetical(const char ch) { return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_'); }

bool isNewLine(const char ch) { return (ch == '\r' || ch == '\n'); }


Tokenizer::Tokenizer()
{
  str = NULL;

  reset();
}

Tokenizer::~Tokenizer()
{
  if (str != NULL)
    delete str;
}

void Tokenizer::setString(const char *string)
{
  if (str != NULL)
    delete str;

  length = strlen(string);

  str = new char[(length + 1) * sizeof(char)];
  strcpy(str, string);

  reset();
}

bool Tokenizer::setFile(const char *fileName)
{
  FILE *file;

  if (str != NULL)
    delete str;

  if ((file = fopen(fileName, "rb")) != NULL)
  {
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    str = new char[(length + 1) * sizeof(char)];
    fread(str, length, 1, file);
    str[length] = '\0';

    fclose(file);

    reset();
    return true;
  }
  return false;
}

void Tokenizer::reset() { end = 0; }

bool Tokenizer::next(char *buffer, BOOLFUNC isAlpha)
{
  start = end;

  while (start < length && isWhiteSpace(str[start]))
    start++;
  end = start + 1;

  if (start < length)
  {

    if (isNumeric(str[start]))
    {
      while (isNumeric(str[end]) || str[end] == '.')
        end++;
    }
    else if (isAlpha(str[start]))
    {
      while (isAlpha(str[end]) || isNumeric(str[end]))
        end++;
    }

    if (buffer != NULL)
    {
      strncpy(buffer, str + start, end - start);
      buffer[end - start] = '\0';
    }
    return true;
  }
  return false;
}

bool Tokenizer::nextAfterToken(char *buffer, char *token)
{
  char str[256];

  while (next(str))
  {
    if (stricmp(str, token) == 0)
    {
      return next(buffer);
    }
  }

  return false;
}

bool Tokenizer::nextLine(char *buffer)
{
  if (end < length)
  {
    start = end;

    while (end < length && !isNewLine(str[end]))
      end++;

    if (buffer != NULL)
    {
      strncpy(buffer, str + start, end - start);
      buffer[end - start] = '\0';
    }

    if (isNewLine(str[end + 1]) && str[end] != str[end + 1])
      end += 2;
    else
      end++;
  }
  return false;
}

bool Tokenizer::getAllRight(char *buffer)
{
  if (start < length)
  {
    strcpy(buffer, str + start);
    return true;
  }
  return false;
}
