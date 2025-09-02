// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shsyn.h"
#include "shMacro.h"
#include "commonUtils.h"

namespace shc
{
class TargetContext;
}


class Lexer : public GeneratedLexer
{
public:
  Lexer(InputStream *s, ShaderMacroManager &macro_mgr) : GeneratedLexer(s), macroMgr{macro_mgr} {}

  void diag_message(int type, int file, int ln, int col, const char *, int code = -1);
  // called by diag_message(), default is fatal on DIAG_ERROR and DIAG_SYNTAX_ERROR
  void diag_text(int type, const char *, int code);
  const char *get_diag_name(int type);
  void error(const char *msg = NULL) override;
  void warning(const char *) override;
  void message(const char *) override;
  void debug_message(const char *) override;
  void set_error(const char *) override;
  void set_error(int file, int ln, int col, const char *) override;
  void set_warning(const char *text) override;
  void set_warning(int file, int ln, int col, const char *) override;
  void set_message(const char *text) override;
  void set_message(int file, int line, int col, const char *text) override;
  void set_debug_message(const char *text) override;
  void set_debug_message(int file, int line, int col, const char *text) override;
  void register_symbol(int name_id, SymbolType type, Terminal *symbol);
  eastl::string get_symbol_location(int name_id, SymbolType type);
  void begin_shader();
  void begin_block();
  void end_shader() { end_lexed_entity(); }
  void end_block() { end_lexed_entity(); }
  const char *get_filename(int f) override;
  String build_current_include_stack() const;

  ShaderMacroManager &get_macro_mgr() { return macroMgr; }
  const ShaderMacroManager &get_macro_mgr() const { return macroMgr; }

private:
  struct SymbolLocation
  {
    int file = 0;
    int line = 0;
    int column = 0;
  };
  enum class LexedEntityType
  {
    GLOBAL,
    SHADER,
    BLOCK
  };

  void end_lexed_entity();

  static uint64_t makeSymbolId(uint64_t name_id, SymbolType type, uint64_t entity_id, LexedEntityType entity_type);

  ShaderMacroManager &macroMgr;

  ska::flat_hash_map<uint64_t, SymbolLocation> mSymbols;
  uint64_t mCountShaders = 0;
  uint64_t mCountBlocks = 0;
  uint64_t mLexedEntityId = 0;
  LexedEntityType mContextType = LexedEntityType::GLOBAL;
};

struct Parser : public ShaderTerminal::GeneratedParser
{
  using GeneratedParser::GeneratedParser;

  shc::TargetContext &ctx;

  Parser(Lexer &lexer, shc::TargetContext &ctx) : GeneratedParser{lexer}, ctx{ctx} {}

  // Concrete callbacks
  void add_shader(ShaderTerminal::shader_decl *d) override;
  void add_block(ShaderTerminal::block_decl *d) override;
  void add_global_var(ShaderTerminal::global_var_decl *d) override;
  void add_sampler(ShaderTerminal::sampler_decl *d) override;
  void add_global_interval(ShaderTerminal::interval &i) override;
  void add_global_assume(ShaderTerminal::assume_stat &a) override;
  void add_global_assume_if_not_assumed(ShaderTerminal::assume_if_not_assumed_stat &a) override;
  void add_global_bool(ShaderTerminal::bool_decl &d) override;
  void add_hlsl(ShaderTerminal::hlsl_global_decl_class &d) override;

  // Shadows base class method providing concrete implementation
  Lexer &get_lexer() { return (Lexer &)lexical_analyzer; }
};

// Joint lifetime for everithing related to the 'file on disk -> ast' transformation
struct SourceFileParseState
{
  InputFile input;
  ShaderMacroManager macroMgr;
  Lexer lexer;
  Parser parser;

  SourceFileParseState(const char *fn, shc::TargetContext &a_ctx);

  PINNED_TYPE(SourceFileParseState)
};