// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "parser/bparser.h"
#include <debug/dag_log.h>
#include "shsyn.h"
#include "shsem.h"
#include "globVarSem.h"
#include "boolVar.h"
#include "cppStcode.h"
#include "cppStcodeAssembly.h"
using namespace ShaderTerminal;
#include <debug/dag_debug.h>
#include "shLog.h"
#include "shMacro.h"
#include <shaders/dag_shaders.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include "shHardwareOpt.h"
#include "shCompiler.h"
#include <osApiWrappers/dag_direct.h>
#include <memory/dag_regionMemAlloc.h>
#include "cppStcodeUtils.h"
#if _TARGET_PC_WIN
#include <direct.h>
#elif _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_C3
#include <unistd.h>
#endif


IMemAlloc *sh_symbolsmem = NULL;
static Tab<SimpleString> inc_base(inimem);

void reset_shaders_inc_base() { inc_base.clear(); }
void add_shaders_inc_base(const char *base) { inc_base.push_back() = base; }

static char compiler_cwd[DAGOR_MAX_PATH] = {0};
static const char *get_cwd()
{
  if (compiler_cwd[0])
    return compiler_cwd;
  if (getcwd(compiler_cwd, DAGOR_MAX_PATH))
    dd_append_slash_c(compiler_cwd);
  else
    memcpy(compiler_cwd, "./", 3);
  return compiler_cwd;
}

// parse include statement
static bool process_include(String &inc_fpath, const char *inc_fn, int inc_fn_len)
{
  for (int i = 0; i < inc_base.size(); i++)
  {
    inc_fpath.printf(260, "%s%s/%.*s", get_cwd(), inc_base[i].str(), inc_fn_len, inc_fn);
    if (dd_file_exists(inc_fpath))
      return true;
  }
  return false;
}

static bool parseInclude(ShaderLexParser *_this, bool optional = false)
{
  if (_this->get_token() != SHADER_TOKENS::SHTOK_string)
  {
    //_this->make_error("expected include file name");
    sh_debug(SHLOG_FATAL, "expected include file name at line(%d)pos(%d)", _this->get_cur_line(), _this->get_cur_column());
    return false;
  }

  const char *inc_fn = _this->get_lexeme() + 1;
  int inc_fn_len = _this->get_lexeme_length() - 2;
  String inc_fpath;

  if (!process_include(inc_fpath, inc_fn, inc_fn_len))
  {
    char fn[1024];
    ::dd_get_fname_location(fn, _this->__input_stream()->get_filename(_this->get_cur_file()));
    if (!fn[0])
      strcpy(fn, ".");
    ::dd_append_slash_c(fn);
    inc_fpath.printf(260, "%s/%.*s", fn, inc_fn_len, inc_fn);
  }

  ::dd_simplify_fname_c(inc_fpath);
  inc_fpath.resize(strlen(inc_fpath) + 1);

  sh_debug(SHLOG_INFO, "including <%s>", inc_fpath.str());
  _this->__clear_lexeme();
  int fn1 = ((ParserFileInput *)_this->__input_stream())->get_include_file_index(inc_fpath);

  if (fn1 < 0)
  {
    auto incFn = shc::search_include_with_pathes(String(0, "%.*s", inc_fn_len, inc_fn));
    fn1 = ((ParserFileInput *)_this->__input_stream())->get_include_file_index(incFn);
  }
  if (fn1 < 0)
  {
    if (!optional)
    {
      ParserFileInput *pfi = (ParserFileInput *)_this->__input_stream();
      String include_file_stack(0, "can't open file %.*s (or %s)\n", inc_fn_len, inc_fn, inc_fpath);
      for (auto it = pfi->incstk.rbegin(); it != pfi->incstk.rend(); ++it)
      {
        include_file_stack.aprintf(0, "included from '%s'\n", pfi->get_filename(it->file));
      }
      pfi->parser->set_error(include_file_stack);
      sh_debug(SHLOG_FATAL, "can't include shader script '%s'", inc_fpath.str());
      return false;
    }
    return true;
  }
  const char *fnRealName = _this->__input_stream()->get_filename(fn1);
  if (_this->includes.find_as(fnRealName) != _this->includes.end())
    return true;
  _this->includes.emplace(fnRealName);

  if (!((ParserFileInput *)_this->__input_stream())->include(fn1))
  {
    if (!optional)
    {
      ((ParserFileInput *)_this->__input_stream())
        ->parser->set_error(String(0, "can't open file %.*s (or %s)", inc_fn_len, inc_fn, inc_fpath));
      sh_debug(SHLOG_FATAL, "can't include shader script '%s'", inc_fpath.str());
    }
    return false;
  }
  return true;
}

int get_token(ShaderParser::TokenInfo &tok, ShaderLexParser *__this)
{
  if (ShaderMacroManager::getToken(tok))
  {
    return tok.token;
  }

  tok.isMacro = false;
  tok.token = __this->get_token();
  tok.lexeme = __this->get_lexeme();
  tok.file = __this->get_cur_file();
  tok.line = __this->get_cur_line();
  tok.column = __this->get_cur_column();
  return tok.token;
}
static inline int getToken(ShaderParser::TokenInfo &tok, ShaderLexParser *__this) { return get_token(tok, __this); }

extern bool in_macro_definition;

Terminal *ShaderLexParser::get_terminal()
{
  ShaderParser::TokenInfo tok;
  // debug("get0");
  getToken(tok, this);

  // debug("  tok=%d",tok);
  if (tok.token == SHADER_TOKENS::TerminalEOF)
  {
    // debug("!!!eof");
    if (!input_stream->is_real_eof())
    {
      // debug("!!!eof not real");
      clear_eof();
      getToken(tok, this);
    }
  }

  bool breakCycle = false;
  while (true)
  {
    switch (tok.token)
    {
      case SHADER_TOKENS::SHTOK_include:
      {
        if (!parseInclude(this, false))
          return NULL;
      }
      break;
      case SHADER_TOKENS::SHTOK_include_optional:
      {
        parseInclude(this, true);
      }
      break;
      case SHADER_TOKENS::SHTOK_macro:
      {
        ShaderMacroManager::parseDefinition(this, false);
      }
      break;
      case SHADER_TOKENS::SHTOK_define_macro_if_not_defined:
      {
        ShaderMacroManager::parseDefinition(this, true);
      }
      break;
      case SHADER_TOKENS::SHTOK_ident:
      {
        if (!in_macro_definition)
          ShaderMacroManager::expandMacro(this, tok);
        if (tok.token != -1)
          breakCycle = true;
        // breakCycle = true;
        break;
      }
      case SHADER_TOKENS::TerminalEOF:
        if (input_stream->is_real_eof())
          breakCycle = true;
        break;
      default: breakCycle = true;
    }
    if (breakCycle)
      break;

    // debug("get1");
    getToken(tok, this);
    if (tok.token == SHADER_TOKENS::TerminalEOF)
    {
      // debug("!!!eof");
      if (!input_stream->is_real_eof())
      {
        // debug("!!!eof not real");
        clear_eof();
        getToken(tok, this);
      }
      else
      {
        break;
      }
    }
  }
  // debug("<%s> %d",(char*)tok.lexeme,tok.token);

  Terminal *t = new (sh_symbolsmem) Terminal;
  t->num = tok.token;
  t->file_start = tok.file;
  t->line_start = tok.line;
  t->col_start = tok.column;
  t->text = str_dup(tok.lexeme, sh_symbolsmem);
  if (!tok.isMacro)
    clear_lexeme();
  t->file_end = tok.file;
  t->line_end = tok.line;
  t->col_end = tok.column;
  t->macro_call_stack = tok.macroCallStack;
  return t;
}

struct MyShaderSyntaxParser : public ShaderSyntaxParser
{
  using ShaderSyntaxParser::ShaderSyntaxParser;
  bool shaderDebugModeEnabled = true;
  // clang-format off
  // clang-format breaks alignment
  void add_shader(shader_decl *d)           override { ShaderParser::add_shader(d, *this, shaderDebugModeEnabled); }
  void add_block(block_decl *d)             override { ShaderParser::add_block(d, *this); }
  void add_global_var(global_var_decl *d)   override { ShaderParser::add_global_var(d, *this); }
  void add_sampler(sampler_decl *d)         override { ShaderParser::add_sampler(d, *this); }
  void add_global_interval(interval &i)     override { ShaderParser::add_global_interval(i, *this); }
  void add_global_assume(assume_stat &a)    override { ShaderParser::add_global_assume(a, *this); }
  void add_global_bool(bool_decl &d)        override { ShaderParser::add_global_bool(d, *this); }
  void add_hlsl(hlsl_global_decl_class &d)  override { ShaderParser::add_hlsl(d, *this, shaderDebugModeEnabled); }
  // clang-format on
};

class MyShaderLexParser : public ShaderLexParser
{
public:
  MyShaderLexParser(ParserInputStream *s) : ShaderLexParser(s) {}

  void diag_message(int type, int file, int ln, int col, const char *, int code = -1);
  // called by diag_message(), default is fatal on DIAG_ERROR and DIAG_SYNTAX_ERROR
  void diag_text(int type, const char *, int code);
  const char *get_diag_name(int type);
  void error(const char *msg = NULL) override;
  void warning(const char *) override;
  void set_error(const char *) override;
  void set_error(int file, int ln, int col, const char *) override;
  void set_warning(int file, int ln, int col, const char *) override;
  void register_symbol(int name_id, SymbolType type, Terminal *symbol) override;
  eastl::string get_symbol_location(int name_id, SymbolType type) override;
  void begin_shader() override;
  void end_shader() override;
  const char *get_filename(int f) override;

private:
  struct SymbolLocation
  {
    int file = 0;
    int line = 0;
    int column = 0;
  };

  ska::flat_hash_map<uint64_t, SymbolLocation> mSymbols;
  uint64_t mShaderId = 0;
  uint64_t mCountShaders = 0;
};

void MyShaderLexParser::set_error(const char *txt) { error(txt); }

void MyShaderLexParser::set_error(int file, int ln, int col, const char *txt) { diag_message(DIAG_SYNTAX_ERROR, file, ln, col, txt); }

void MyShaderLexParser::set_warning(int file, int ln, int col, const char *txt) { diag_message(DIAG_WARNING, file, ln, col, txt); }

const char *MyShaderLexParser::get_diag_name(int type)
{
  switch (type)
  {
    case DIAG_SYNTAX_ERROR:
    case DIAG_ERROR: return "Error";
    case DIAG_WARNING: return "Warning";
  }
  return "";
}

void MyShaderLexParser::diag_message(int type, int file, int ln, int col, const char *txt, int code)
{
  const char *fn = get_filename(file);
  String macroDesc = ShaderMacroManager::getCurrentMacroDesc(this);
  String msg((fn ? fn : "<unknown file>"));
  //  msg.aprintf(100,"(%d):%s",ln,get_diag_name(type));
  //  if(code!=-1) msg.aprintf(100," #%d",code);
  msg.aprintf(20, "(%d,%d): ", ln, col);

  bool lf = false;
  if (macroDesc.length())
  {
    msg = macroDesc + "\n" + msg;
    lf = true;
  }

  msg += txt;

  if (lf)
  {
    msg += "\n";
  }

  diag_text(type, msg, code);
}

void MyShaderLexParser::diag_text(int type, const char *txt, int code)
{
  //== write some script log file
  ShLogMode mode = SHLOG_NORMAL;
  switch (type)
  {
    case DIAG_ERROR:
    case DIAG_SYNTAX_ERROR: mode = SHLOG_ERROR; break;
    case DIAG_WARNING: mode = SHLOG_WARNING; break;
  }

  sh_debug(mode, "%s", txt);
}


void MyShaderLexParser::error(const char *txt)
{
  diag_message(DIAG_ERROR, get_cur_file(), get_cur_line(), get_cur_column(), txt ? txt : "syntax error");
}

void MyShaderLexParser::warning(const char *txt)
{
  diag_message(DIAG_WARNING, get_cur_file(), get_cur_line(), get_cur_column(), txt ? txt : "syntax error");
}

static uint64_t make_symbol_id(uint64_t name_id, SymbolType type, uint64_t shader_id)
{
  G_ASSERT(name_id <= 0xffffffff);
  G_ASSERT((int)type <= 15);
  G_ASSERT(shader_id <= 0x0fffffff);
  return name_id | (static_cast<uint64_t>(type) << 32) | (shader_id << (32 + 4));
}

void MyShaderLexParser::register_symbol(int name_id, SymbolType type, Terminal *symbol)
{
  auto pair = mSymbols.emplace(make_symbol_id(name_id, type, mShaderId),
    SymbolLocation{symbol->file_start, symbol->line_start, symbol->col_start});
  bool is_inserted = pair.second;
  if (is_inserted)
    return;

  const SymbolLocation &existed = pair.first->second;
  bool is_equal = symbol->file_start == existed.file && symbol->line_start == existed.line && symbol->col_start == existed.column;
  if (is_equal)
    return;

  eastl::string existed_loc = get_symbol_location(name_id, type);
  eastl::string err(eastl::string::CtorSprintf{}, "Attempt to register another symbol '%s' with an existed name_id: %s", symbol->text,
    existed_loc.c_str());
  set_error(symbol->file_start, symbol->line_start, symbol->col_start, err.c_str());
}

eastl::string MyShaderLexParser::get_symbol_location(int name_id, SymbolType type)
{
  auto found = mSymbols.find(make_symbol_id(name_id, type, mShaderId));
  if (found == mSymbols.end())
  {
    auto found = mSymbols.find(make_symbol_id(name_id, type, 0));
    if (found == mSymbols.end())
      return "<unknown file>";
  }

  const char *file_name = get_filename(found->second.file);
  return eastl::string({}, "%s(%i,%i)", file_name, found->second.line, found->second.column);
}

void MyShaderLexParser::begin_shader()
{
  mCountShaders++;
  mShaderId = mCountShaders;
}

void MyShaderLexParser::end_shader()
{
  mShaderId = 0;
  mSymbols.clear();
}

const char *MyShaderLexParser::get_filename(int f) { return input_stream->get_filename(f); }

namespace ShaderParser
{
ShHardwareOptions currentShaderOptions(d3d::smAny);
}

static ParserFileInput *current_inp = NULL;

int get_parsed_filename_index(const char *fname)
{
  if (!current_inp || !fname)
    return -1;
  return current_inp->get_include_file_index(fname);
}

void parse_shader_script(const char *fn, const ShHardwareOptions &opt, Tab<SimpleString> *out_filenames)
{
  RegionMemAlloc rm_alloc(4 << 20, 4 << 20);

  g_cppstcode.reset();
  g_cppstcode.shaderName = stcode::extract_shader_name_from_path(fn);

  {
    ParserFileInput inp;
    String inc_fpath;
    if (dd_file_exists(fn))
      inc_fpath.setStrCat(get_cwd(), fn);
    else if (!process_include(inc_fpath, fn, i_strlen(fn)))
      inc_fpath = fn;

    if (!inp.include_alefile(inc_fpath))
    {
      sh_debug(SHLOG_FATAL, "can't read shader script '%s'", fn);
    }
    current_inp = &inp;

    sh_symbolsmem = &rm_alloc;

    BaseParNamespace::symbolsmem = sh_symbolsmem;

    //    sh_symbolsmem->enableStackInfo(false);
    //    sh_symbolsmem->enableOutCheck(false);
    ShaderParser::currentShaderOptions = opt;

    MyShaderLexParser lex(&inp);
    sh_process_errors();
    sh_debug(SHLOG_INFO, "parsing shader script %s", fn);
    MyShaderSyntaxParser syn(lex);
    NonterminalS *s = syn.parse();
    if (!s)
      sh_debug(SHLOG_ERROR, "Unexpected EOF at '%s'", fn);
    sh_debug(SHLOG_INFO, "parsed shader script %s", fn);
    sh_dump_warn_info();
    sh_process_errors();
    ShaderMacroManager::clearMacros();
    BoolVar::clear();

    if (out_filenames)
      for (int i = 0; i < inp.incfile.size(); i++)
        out_filenames->push_back() = inp.incfile[i].fn;
    current_inp = NULL;
  }

  DEBUG_CTX("sh_symbolsmem used %dK (of %dK allocated in %d pools)", rm_alloc.getPoolUsedSize() >> 10,
    rm_alloc.getPoolAllocatedSize() >> 10, rm_alloc.getPoolsCount());
  sh_symbolsmem = NULL;
}

NonterminalS *parse_shader_string(const String &source, const char *file_name, int line, IMemAlloc *alloc)
{
  struct ParserInputString : public ParserInputStream
  {
    const char *begin = nullptr;
    const char *end = nullptr;
    const char *file = nullptr;

    ParserInputString(const String &source, const char *file_name) :
      begin(source.data()), end(source.data() + source.size()), file(file_name)
    {}

    bool eof() override { return begin == end; }
    char get() override { return *begin++; }
    void stream_set(BaseLexicalAnalyzer &) override {}
    bool is_real_eof() override { return eof(); }
    const char *get_filename(int) override { return file; }
  };
  struct StringLexParser : public MyShaderLexParser
  {
    StringLexParser(ParserInputString *s, const char *file, int line) : MyShaderLexParser(s)
    {
      ShaderLexParser::current_line = line;
      ShaderLexParser::current_file = file ? get_parsed_filename_index(file) : -1;
    }
  };

  IMemAlloc *prev_alloc = sh_symbolsmem;
  if (alloc)
  {
    sh_symbolsmem = alloc;
    BaseParNamespace::symbolsmem = sh_symbolsmem;
  }

  ParserInputString inp(source, file_name);
  StringLexParser lex(&inp, file_name, line);
  ShaderSyntaxParser syn(lex);
  NonterminalS *result = syn.parse();

  if (alloc)
  {
    sh_symbolsmem = prev_alloc;
    BaseParNamespace::symbolsmem = sh_symbolsmem;
  }
  return result;
}

namespace ShaderParser
{
bool_expr *parse_condition(const char *cond_str, int cond_str_len, const char *file_name, int line)
{
  String str(0, "bool _ = (%.*s);", cond_str_len, cond_str);
  NonterminalS *result = parse_shader_string(str, file_name, line, nullptr);
  if (!result || result->decl.size() != 1 || !result->decl[0]->global_bool_decl)
    return nullptr;

  return result->decl[0]->global_bool_decl->expr;
}

shader_stat *parse_shader_stat(const char *stat_str, int stat_str_len, IMemAlloc *alloc)
{
  String str(0, "shader _ { %.*s }", stat_str_len, stat_str);
  NonterminalS *result = parse_shader_string(str, nullptr, 1, alloc);
  if (!result || result->decl.size() != 1 || !result->decl[0]->shader)
    return nullptr;
  if (result->decl[0]->shader->stat.size() != 1)
    return nullptr;

  return result->decl[0]->shader->stat[0];
}
} // namespace ShaderParser
