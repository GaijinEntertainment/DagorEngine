//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/propPanel.h>
#include <util/dag_simpleString.h>

namespace PropPanel
{

class IconWithName
{
public:
  void setFileName(const char *icon_file_name)
  {
    fileName = icon_file_name;
    id = IconId::Invalid;
  }

  IconId getIconId()
  {
    if (fileName.empty())
      return id;

    id = load_icon(fileName);
    fileName.clear();
    return id;
  }

private:
  SimpleString fileName;
  IconId id = IconId::Invalid;
};

class IconWithNameAndSize
{
public:
  void setFileName(const char *icon_file_name)
  {
    fileName = icon_file_name;
    id = IconId::Invalid;
    loadedSize = 0;
  }

  IconId getIconId(int size)
  {
    G_ASSERT(size > 0);
    if (size == loadedSize)
      return id;

    loadedSize = size;
    id = load_icon(fileName, size);
    return id;
  }

private:
  SimpleString fileName;
  IconId id = IconId::Invalid;
  int loadedSize = 0;
};

} // namespace PropPanel