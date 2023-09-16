#ifndef _PCRECPP_NOSTL_H
#define _PCRECPP_NOSTL_H

#include "pcrecpp.h"

namespace pcrecpp 
{

// Simple interface for regular expression matching without STL.
class PCRECPP_EXP_DEFN RE2
{
public:
  RE2 ( const char * pat ) : re ( pat ) {}
  RE2 ( const char * pat, const RE_Options & option ) : re ( pat, option ) {}

  bool FullMatch    ( const char * text ) const;
  bool PartialMatch ( const char * text ) const;

  const char * Replace       ( const char * rewrite, const char * str  );
  const char * GlobalReplace ( const char * rewrite, const char * str  );
  const char * Extract       ( const char * rewrite, const char * text );

  struct MultiExtractState
  {
    const char * searchStr = nullptr;
    int offset = 0;
    bool previousWasEmpty = false;
  };

  void MultiExtractInit(MultiExtractState * state, const char * text);
  const char * MultiExtract(MultiExtractState * state, const char * rewrite);

  const char * pattern();

private:
  RE          re;
  std::string result;
};

};

#endif
