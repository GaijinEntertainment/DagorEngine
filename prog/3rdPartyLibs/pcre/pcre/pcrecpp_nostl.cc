// A very ugly hack. The PCRE library is itself very nice, but Google's pcrecpp wrapper
// is hiding too much from us and basically does not allow to do multiple searches
// over a single string

namespace pcrecpp
{
  class RE2;
}

#include <string>
#define private friend class ::pcrecpp::RE2; private
#include "pcrecpp.h"
#undef private

#include "pcrecpp_nostl.h"

// Simple interface for regular expression matching without STL.

namespace pcrecpp 
{

bool RE2::FullMatch ( const char * text ) const
{
  return re.FullMatch ( text );
}


bool RE2::PartialMatch ( const char * text ) const
{
  return re.PartialMatch ( text );
}


const char * RE2::Replace ( const char * rewrite, const char * str )
{
  result = str;  
  re.Replace ( rewrite, & result );
  return result.c_str ();
}


const char * RE2::GlobalReplace ( const char * rewrite, const char * str )
{
  result = str;  
  re.GlobalReplace ( rewrite, & result );
  return result.c_str ();
}


const char * RE2::Extract ( const char * rewrite, const char * text )
{
  re.Extract ( rewrite, text, & result );
  return result.c_str ();
}

void RE2::MultiExtractInit(MultiExtractState * state, const char * text)
{
  state->searchStr = text;
  state->offset = 0;
  state->previousWasEmpty = false;
}

const char * RE2::MultiExtract(MultiExtractState * state, const char * rewrite)
{
  const int VECTOR_SIZE = 20; // must be a multiple of 2; up to 20/2 = (1 +) 9 capture groups
  int vec[VECTOR_SIZE];
  RE::Anchor anchor = state->previousWasEmpty ? RE::ANCHOR_START : RE::UNANCHORED;
  bool emptyIsOk = state->previousWasEmpty ? false : true;
  int matches = re.TryMatch(state->searchStr, state->offset, anchor, emptyIsOk, vec, VECTOR_SIZE);
  if (matches == 0)
    return nullptr;
  result.erase();
  re.Rewrite(&result, rewrite, state->searchStr, vec, matches);
  state->offset = vec[1];
  state->previousWasEmpty = (vec[0] == vec[1]);
  return result.c_str();
}

const char * RE2::pattern()
{
  return re.pattern().c_str();
}

};
