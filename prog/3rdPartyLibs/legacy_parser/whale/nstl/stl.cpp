
#include "stl_format.h"


const int multiline_flag_index = std::ios_base::xalloc();

#ifndef NO_CPP_LOCALE

// FIXME: lookup in the standard

template<class charT>
std::locale::id stl_format<charT>::id;

template
std::locale::id stl_format<char>::id;
#else
const int stl_format_index = std::ios_base::xalloc();
#endif
