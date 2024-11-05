// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_mem.h>
#include <util/dag_globDef.h>
#include <EASTL/vector.h>
#include <EASTL/fixed_vector.h>

namespace BaseParNamespace
{
inline IMemAlloc *symbolsmem;
enum
{
  SYMBOL_NONTERMINAL,
  SYMBOL_TERMINAL
};
struct Symbol
{
  DAG_DECLARE_NEW(symbolsmem)

  int file_start, line_start, col_start, file_end, line_end, col_end;

  struct MacroLocation
  {
    const char *name = nullptr;
    int file = 0;
    int line = 0;
  };

  using MacroCallStack = eastl::fixed_vector<MacroLocation, 16, false>;
  MacroCallStack macro_call_stack;
  virtual ~Symbol() {}
  Symbol() {}
  Symbol(int sf, int sl, int sc, int ef, int el, int ec)
  {
    file_start = sf;
    line_start = sl;
    col_start = sc;
    file_end = ef;
    line_end = el;
    col_end = ec;
  }
  int symb_tp;
  bool is_terminal() const { return symb_tp == SYMBOL_TERMINAL; };
  bool is_nonterminal() const { return symb_tp == SYMBOL_NONTERMINAL; };
};
struct Terminal : Symbol
{
  int num;
  // int &file, &line, &column;
  // int file, line, column;
  const char *text;
  // Terminal():file(file_start),line(line_start),column(col_start){symb_tp=SYMBOL_TERMINAL;text=NULL;}
  Terminal()
  {
    symb_tp = SYMBOL_TERMINAL;
    text = NULL;
  }
  Terminal(int sf, int sl, int sc, int ef, int el, int ec) : Symbol(sf, sl, sc, ef, el, ec)
  {
    symb_tp = SYMBOL_TERMINAL;
    text = NULL;
  }
  virtual ~Terminal() {}

  int number() { return num; };
};

struct TerminalEOF : Terminal
{
  static const int number = 0;
  TerminalEOF() : Terminal()
  {
    num = number;
    text = NULL;
  }
};
struct TerminalError : Terminal
{
  static const int number = 1;
  TerminalError() : Terminal()
  {
    num = number;
    text = NULL;
  }

  eastl::vector<Symbol *> garbage;
  int error_position;
};

struct Nonterminal : Symbol
{
  int num;
  Nonterminal()
  {
    symb_tp = SYMBOL_NONTERMINAL;
    num = -1;
  }
  Nonterminal(int sf, int sl, int sc, int ef, int el, int ec) : Symbol(sf, sl, sc, ef, el, ec) { num = -1; }
  int number() { return num; }
};
} // namespace BaseParNamespace
