// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>


class BaseLexicalAnalyzer
{
protected:
  int tab_size;
  bool got_creturn;

public:
  BaseLexicalAnalyzer()
  {
    tab_size = 8;
    got_creturn = false;
  }

  virtual int get_token() = 0;
  // Warning! This pointer is valid only before next get_token called (or analyzer is dead)
  virtual const char *get_lexeme() const = 0;
  virtual int get_lexeme_length() const = 0;
  virtual const eastl::string &get_lexeme_str() const = 0;
  // virtual char *get_token_string()=0;//Returns NULL if no token string currently present

  virtual void set_error(const char *text) = 0;
  virtual void set_error(int file, int line, int col, const char *text) = 0;
  virtual void set_warning(const char *text) = 0;
  virtual void set_warning(int file, int line, int col, const char *text) = 0;
  virtual void set_message(const char *text) = 0;
  virtual void set_message(int file, int line, int col, const char *text) = 0;
  virtual void set_debug_message(const char *text) = 0;
  virtual void set_debug_message(int file, int line, int col, const char *text) = 0;

  virtual void set_cur_line(int) = 0;
  virtual void set_cur_column(int) = 0;
  virtual void set_cur_offset(int) = 0; // offset in symbols from bof,\t is 1,not tab-size
  virtual void set_cur_file(int) = 0;   // (char*)?
  virtual void set_tab_size(int a) { tab_size = a; }

  virtual const char *get_filename(int file) { return nullptr; }
  virtual int get_cur_line() const = 0;
  virtual int get_cur_column() const = 0;
  virtual int get_cur_offset() const = 0; // offset in symbols from bof,\t is 1,not tab-size
  virtual int get_cur_file() const = 0;   // (char*)?
  virtual int get_tab_size() const { return tab_size; }
  virtual void internal_position_counter(char c)
  {
    if (c == '\n' || got_creturn)
    {
      set_cur_line(get_cur_line() + 1);
      set_cur_column(1);
      got_creturn = false;
    }
    if (c == '\n')
      ; // no-op
    else if (c == '\r')
      got_creturn = true;
    else if (c == '\t')
    {
      set_cur_column(get_cur_column() + (get_tab_size() - (get_cur_column() - 1) % get_tab_size()));
      // set_cur_column(get_cur_column()+get_tab_size());
    }
    else
      set_cur_column(get_cur_column() + 1);
    set_cur_offset(get_cur_offset() + 1);
  }

  virtual void clear_eof() = 0;
  virtual int buffer_size() = 0;
  virtual void clear_buffer() = 0;
};
