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

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include <string.h>
#include "set.h"

typedef bool (*BOOLFUNC)(const char ch);

bool isWhiteSpace(const char ch);
bool isNumeric(const char ch);
bool isAlphabetical(const char ch);
bool isNewLine(const char ch);

class Tokenizer
{
private:
  char *str;
  unsigned int length;
  unsigned int start, end;

public:
  Tokenizer();
  ~Tokenizer();

  void setString(const char *string);
  bool setFile(const char *fileName);
  void reset();
  bool next(char *buffer = NULL, BOOLFUNC isAlpha = isAlphabetical);
  bool nextAfterToken(char *buffer, char *token);
  bool nextLine(char *buffer = NULL);
  bool getAllRight(char *buffer);
};

#endif // _TOKENIZER_H_
