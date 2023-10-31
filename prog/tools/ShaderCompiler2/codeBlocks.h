#pragma once

#include "shsem.h"
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>
#include "nameMap.h"

struct CryptoHash;

class CodeSourceBlocks
{
public:
  struct ParserContext;
  struct Fragment;
  struct Condition;
  struct Unconditional;

  enum class ExprValue
  {
    NonConst,
    ConstFalse,
    ConstTrue,
  };

  struct CondIf
  {
    ShaderTerminal::bool_expr *expr = nullptr;
    ExprValue constExprVal = ExprValue::NonConst;
    Tab<Fragment> onTrue;
  };

  struct Condition
  {
    CondIf onIf;
    //! optional list of elif's
    Tab<CondIf> onElif;
    //! code fragments for false branche of condition
    Tab<Fragment> onElse;
  };

  struct Unconditional
  {
    //! when simple uncoditional fragment, codeId contains index of code text with fnameId/line pointing to code text origin
    int codeId, fnameId, line;

    struct Decl
    {
      int identId, identValue;
    };
    Decl decl;

    struct DeclBool
    {
      const char *name = nullptr;
      ShaderTerminal::bool_expr *expr = nullptr;
    };
    DeclBool declBool;

    bool isDecl() const { return codeId == -1; }
    bool isDeclBool() const { return codeId == -2; }
  };

  struct Fragment
  {
    //! when cond!=NULL, fragment is condition block
    eastl::unique_ptr<Condition> cond;

    //! when cond==NULL, fragment is code block with uncondCodeId referencing text fragment
    Unconditional uncond;

    bool isCond() const { return cond != NULL; }
    bool isDecl() const { return uncond.isDecl(); }
    bool isDeclBool() const { return uncond.isDeclBool(); }
    Unconditional::Decl &decl() { return uncond.decl; }
    Unconditional::DeclBool &declBool() { return uncond.declBool; }
  };

public:
  CodeSourceBlocks() : blocks(midmem), declCount(0) {}

  //! resets data structure
  void reset()
  {
    blocks.clear();
    declCount = 0;
  }

  //! parse source text into code fragments
  bool parseSourceCode(const char *stage, const char *src, ShaderParser::ShaderBoolEvalCB &cb);

  //! returns code unique identifier for specific variant [returns reference to highly temporary storage]
  dag::ConstSpan<Unconditional *> getPreprocessedCode(ShaderParser::ShaderBoolEvalCB &cb);

  //! builds and returns code digest for given code [returns reference to highly temporary storage]
  CryptoHash getCodeDigest(dag::ConstSpan<Unconditional *> code) const;

  //! returns code text for given code [returns reference to highly temporary storage]
  dag::ConstSpan<char> buildSourceCode(dag::ConstSpan<Unconditional *> code);

public:
  //! removes all unconditional code fragments (used between shaders compilation)
  static void resetCompilation();

public:
  Tab<Fragment> blocks;
  int declCount;

  static SCFastNameMap incFiles;

protected:
  void ppSrcCode(char *s, int len, int fnameId, int line, ParserContext &ctx, char *st_comment, char *end_comment);
  bool ppCheckDirective(char *s, ParserContext &ctx);
  bool ppDirective(char *s, int len, char *dtext, int fnameId, int line, ParserContext &ctx);
  bool ppDoInclude(const char *incl_fn, Tab<char> &out_text, const char *src_fn, int src_ln, ParserContext &ctx);
};
DAG_DECLARE_RELOCATABLE(CodeSourceBlocks::Fragment);
