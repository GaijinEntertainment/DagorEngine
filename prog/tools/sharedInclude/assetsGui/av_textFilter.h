// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <libTools/util/strUtil.h>
#include <util/dag_string.h>
#include <util/dag_strUtil.h>

class AssetsGuiTextFilter
{
public:
  void setSearchText(const char *text)
  {
    searchWords.clear();

    // Some asset names contain spaces by mistake. Allow searching for those.
    if (strcmp(text, " ") == 0)
    {
      SearchWord searchWord;
      searchWord.word = " ";
      searchWords.push_back(searchWord);
      return;
    }

    str_split(text, ' ', [this](const char *word_start, int word_length) {
      SearchWord searchWord;
      searchWord.word.setStr(word_start, word_length);
      trim(searchWord.word);
      if (!searchWord.word.empty())
      {
        searchWord.word.toLower();
        searchWord.hasWildcardCharacter = str_has_wildcard_character(searchWord.word);
        searchWords.push_back(searchWord);
      }

      return true;
    });
  }

  bool isSet() const { return !searchWords.empty(); }

  bool matchesFilter(const char *text) const
  {
    if (searchWords.empty())
      return true;

    buffer = text;
    buffer.toLower();

    for (const SearchWord &searchWord : searchWords)
    {
      if (searchWord.hasWildcardCharacter)
      {
        if (!str_matches_wildcard_pattern(buffer, searchWord.word))
          return false;
      }
      else
      {
        if (!strstr(buffer, searchWord.word))
          return false;
      }
    }

    return true;
  }

private:
  struct SearchWord
  {
    String word;
    bool hasWildcardCharacter = false;
  };

  dag::Vector<SearchWord> searchWords;
  mutable String buffer;
};
