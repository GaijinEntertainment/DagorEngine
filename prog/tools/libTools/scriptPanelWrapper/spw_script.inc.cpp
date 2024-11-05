// Copyright (C) Gaijin Games KFT.  All rights reserved.

static const char *defScript = "\
::getControl <- function getControl(full_name) \r\n\
{ \r\n\
  local function nextWordEnd(text, word_begin_index) \r\n\
  { \r\n\
    return text.indexof(\"/\", word_begin_index); \r\n\
  } \r\n\
   \r\n\
  local function getControlByName(object, name) \r\n\
  { \r\n\
    if (object.rawin(\"controls\") && ((typeof object.controls) == \"array\")) \r\n\
      foreach(key, control in object.controls) \r\n\
        if ( control.rawin(\"name\") && (control.name == name) ) \r\n\
          return control \r\n\
    return null \r\n\
  } \r\n\
   \r\n\
  if (!full_name.len()) \r\n\
    return null  \r\n\
   \r\n\
  local cur_start  = 0 \r\n\
  local cur_end    = cur_start + 1 \r\n\
  local cur_object = null \r\n\
   \r\n\
  local new_end = nextWordEnd(full_name, cur_start) \r\n\
  cur_end = (new_end) ? new_end : full_name.len() \r\n\
  local sch_name = full_name.slice(cur_start, cur_end) \r\n\
  cur_start      = cur_end + 1 \r\n\
  if (::getroottable().rawin(sch_name)) \r\n\
    cur_object = ::getroottable()[sch_name] \r\n\
   \r\n\
  while ( cur_object && (cur_start < full_name.len()) ) \r\n\
  { \r\n\
    new_end = nextWordEnd(full_name, cur_start) \r\n\
    cur_end = (new_end) ? new_end : full_name.len() \r\n\
    local cur_name = full_name.slice(cur_start, cur_end) \r\n\
    cur_start      = cur_end + 1 \r\n\
    cur_object = getControlByName(cur_object, cur_name); \r\n\
     \r\n\
    if (cur_object && (cur_start > full_name.len())) \r\n\
      return cur_object; \r\n\
  } \r\n\
  return null \r\n\
} \r\n\
";
