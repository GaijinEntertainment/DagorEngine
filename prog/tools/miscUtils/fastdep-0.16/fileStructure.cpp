#include "fileStructure.h"
#include "compileState.h"
#include "define.h"
#include "include.h"
#include "sequence.h"
#include "fileCache.h"
#include "if.h"

#include <iostream>

#define TRACE_PARSER 0

#define GETNEXT            \
  {                        \
    ++Cursor;              \
    if (Cursor >= aLength) \
      break;               \
  }

#define CURRENT (aFile[Cursor])

#define ON(c, label)        \
  {                         \
    if (aFile[Cursor] == c) \
      goto label;           \
  }
#define ONEOL(label) ON('\r', label) ON('\n', label)
#define ONWS(label)  ON(' ', label) ON('\t', label)
#define ONNOT(c, label)     \
  {                         \
    if (aFile[Cursor] != c) \
      goto label;           \
  }

#define SKIPON(c, label)    \
  {                         \
    if (aFile[Cursor] == c) \
    {                       \
      ++Cursor;             \
      goto label;           \
    }                       \
  }

FileStructure::FileStructure(const std::string &aFilename, FileCache *aCache, const char *aFile, unsigned int aLength) :
  ResolvedName(aFilename), Main(0), Current(0), Cache(aCache), PragmaOnce(false), AlreadyIncluded(false)
{
  Current = Main = new Sequence(this);
  std::string IncludeFilename;
  std::string DefineName;
  std::string DefineParam;
  std::string DefineBody;
  std::string Symbol;
  if (aLength == 0)
    return;
  unsigned int Cursor = 0;
  bool line_printed = false;

  for (;;)
  {
    ON('#', begin_preprocessor)
    ONWS(from_begin_of_line)
    ONEOL(from_begin_of_line)
    ON('/', begin_comment_empty_line)
    ON('"', string)
    ON('\'', singlechar)
    goto scan_end_of_line;

#if TRACE_PARSER > 1
  from_begin_of_line_eol:
    line_printed = false;
#else
  from_begin_of_line_eol:
#endif
  from_begin_of_line:
    GETNEXT
#if TRACE_PARSER > 1
    if (!line_printed && aFile[Cursor] != '\n' && aFile[Cursor] != '\r')
    {
      const char *eol = strchr(aFile + Cursor, '\n');
      const char *eol2 = strchr(aFile + Cursor, '\r');
      if (!eol || eol2 < eol)
        eol = eol2;

      if (eol && eol - aFile < aLength)
        printf("%.*s\n", eol - (aFile + Cursor), aFile + Cursor);
      line_printed = true;
    }
#endif
    ONWS(from_begin_of_line)
    ONEOL(from_begin_of_line_eol)
    ON('#', begin_preprocessor)
    ON('/', begin_comment_empty_line)
    ON('"', string)
    ON('\'', singlechar)
    goto scan_end_of_line;

  scan_end_of_line:
    GETNEXT
    ONEOL(from_begin_of_line)
    ON('/', begin_comment)
    // TODO test for begin string and begin quote
    goto scan_end_of_line;

  begin_comment:
    GETNEXT
    ON('*', cstyle_comment)
    ON('/', ccstyle_comment)
    goto scan_end_of_line;

  cstyle_comment:
    GETNEXT
    ONNOT('*', cstyle_comment)
    goto c_style_comment_maybe_end;

  c_style_comment_maybe_end:
    GETNEXT
    ON('*', c_style_comment_maybe_end)
    ONNOT('/', cstyle_comment)
    goto scan_end_of_line;

  ccstyle_comment:
    GETNEXT
    ONEOL(from_begin_of_line)
    goto ccstyle_comment;

  begin_comment_empty_line:
    GETNEXT
    ON('*', cstyle_comment_empty_line)
    ON('/', ccstyle_comment)
    goto scan_end_of_line;

  cstyle_comment_empty_line:
    GETNEXT
    ONNOT('*', cstyle_comment_empty_line)
    goto cstyle_comment_empty_line_maybe_end;

  cstyle_comment_empty_line_maybe_end:
    GETNEXT
    ON('*', c_style_comment_maybe_end)
    ONNOT('/', cstyle_comment_empty_line)
    goto from_begin_of_line;

  string:
    GETNEXT
    SKIPON('\\', string)
    ON('"', scan_end_of_line)
    goto string;

  singlechar:
    GETNEXT
    SKIPON('\\', endsinglechar)
  endsinglechar:
    GETNEXT
    goto scan_end_of_line;

  begin_preprocessor:
    GETNEXT
    ON('i', begin_preprocessor_i)
    ON('d', begin_preprocessor_d)
    ON('e', begin_preprocessor_e)
    ON('p', begin_preprocessor_p)
    ONWS(begin_preprocessor)
    // TODO
    goto scan_end_of_line;

  begin_preprocessor_i:
    GETNEXT
    ON('n', begin_preprocessor_in)
    ON('f', begin_preprocessor_if)
    goto scan_end_of_line;

  begin_preprocessor_in:
    GETNEXT
    ONNOT('c', scan_end_of_line)
    GETNEXT
    ONNOT('l', scan_end_of_line)
    GETNEXT
    ONNOT('u', scan_end_of_line)
    GETNEXT
    ONNOT('d', scan_end_of_line)
    GETNEXT
    ONNOT('e', scan_end_of_line)
    goto look_for_filename;

  look_for_filename:
    GETNEXT
    ONWS(look_for_filename)
    ON('"', local_include)
    ON('<', system_include)
    goto scan_end_of_line;

  local_include:
    GETNEXT
    ON('"', local_include_end)
    IncludeFilename += CURRENT;
    goto local_include;

  local_include_end:
    localInclude(IncludeFilename, aFilename);
    IncludeFilename = "";
    goto scan_end_of_line;

  system_include:
    GETNEXT
    ON('>', system_include_end)
    IncludeFilename += CURRENT;
    goto system_include;

  system_include_end:
    systemInclude(IncludeFilename, aFilename);
    IncludeFilename = "";
    goto scan_end_of_line;

  begin_preprocessor_d:
    GETNEXT
    ONNOT('e', scan_end_of_line)
    GETNEXT
    ONNOT('f', scan_end_of_line)
    GETNEXT
    ONNOT('i', scan_end_of_line)
    GETNEXT
    ONNOT('n', scan_end_of_line)
    GETNEXT
    ONNOT('e', scan_end_of_line)
    goto look_for_define;

  look_for_define:
    GETNEXT
    ONWS(look_for_define)
    ONEOL(from_begin_of_line)
    goto define_name;

  define_name:
    ONWS(define_body_begin)
    ON('(', define_param)
    ONEOL(define_end)
    DefineName += CURRENT;
    GETNEXT
    goto define_name;

  define_param:
    GETNEXT
    ON(')', define_body_begin)
    ONEOL(define_end)
    DefineParam += CURRENT;
    goto define_param;

  define_body_begin:
    GETNEXT
    ONWS(define_body_begin)
    if (aFile[Cursor] == '\\' && aFile[Cursor + 1] != '\\' && aFile[Cursor - 1] != '\\')
      goto define_next_line;
    ONEOL(define_end)
    goto define_body;

  define_body:
    ONEOL(define_end)
    DefineBody += CURRENT;
    GETNEXT
    if (aFile[Cursor] == '\\' && aFile[Cursor + 1] != '\\' && aFile[Cursor - 1] != '\\')
      goto define_next_line;
    goto define_body;

  define_next_line:
    GETNEXT
    if (aFile[Cursor] != '\n' && aFile[Cursor] != '\r')
      goto define_next_line;
    GETNEXT
    if (aFile[Cursor] == '\n' || aFile[Cursor] == '\r')
      goto define_next_line2;
    Cursor--;
    goto define_next_line2;

  define_next_line2:
    GETNEXT
    ONWS(define_next_line2);
    if (aFile[Cursor] == '\\' && aFile[Cursor + 1] != '\\' && aFile[Cursor - 1] != '\\')
      goto define_next_line;
    goto define_body;

  define_end:
    define(DefineName, DefineParam, DefineBody);
    DefineName = "";
    DefineParam = "";
    DefineBody = "";
    goto from_begin_of_line;

  begin_preprocessor_if:
    GETNEXT
    ON('n', begin_preprocessor_ifn);
    ON('d', begin_preprocessor_ifd);
    ONWS(if_begin);
    if (aFile[Cursor] == '(')
    {
      Cursor--;
      goto if_begin;
    }
    goto scan_end_of_line;

  begin_preprocessor_ifn:
    GETNEXT
    ONNOT('d', scan_end_of_line);
    GETNEXT
    ONNOT('e', scan_end_of_line);
    GETNEXT
    ONNOT('f', scan_end_of_line);
    goto ifndef_begin;

  ifndef_begin:
    GETNEXT
    ONWS(ifndef_begin)
    ONEOL(from_begin_of_line)
    goto ifndef;

  ifndef:
    ONWS(ifndef_end)
    ONEOL(ifndef_end)
    Symbol += CURRENT;
    GETNEXT
    goto ifndef;

  ifndef_end:
    ifndef(Symbol);
    Symbol = "";
    ONEOL(from_begin_of_line);
    goto scan_end_of_line;

  begin_preprocessor_ifd:
    GETNEXT
    ONNOT('e', scan_end_of_line);
    GETNEXT
    ONNOT('f', scan_end_of_line);
    goto ifdef_begin;

  ifdef_begin:
    GETNEXT
    ONWS(ifdef_begin)
    ONEOL(from_begin_of_line)
    goto ifdef;

  ifdef:
    ONWS(ifdef_end)
    ONEOL(ifdef_end)
    Symbol += CURRENT;
    GETNEXT
    goto ifdef;

  ifdef_end:
    ifdef(Symbol);
    Symbol = "";
    ONEOL(from_begin_of_line);
    goto scan_end_of_line;

  begin_preprocessor_e:
    GETNEXT
    ON('n', begin_preprocessor_en)
    ON('l', begin_preprocessor_el)
    goto scan_end_of_line;

  begin_preprocessor_p:
    GETNEXT
    ONNOT('r', scan_end_of_line)
    GETNEXT
    ONNOT('a', scan_end_of_line)
    GETNEXT
    ONNOT('g', scan_end_of_line)
    GETNEXT
    ONNOT('m', scan_end_of_line)
    GETNEXT
    ONNOT('a', scan_end_of_line)
    goto begin_pragma;

  begin_pragma:
    GETNEXT
    ONWS(begin_pragma);
    ONEOL(from_begin_of_line);
    ONNOT('o', scan_end_of_line)
    GETNEXT
    ONNOT('n', scan_end_of_line)
    GETNEXT
    ONNOT('c', scan_end_of_line)
    GETNEXT
    ONNOT('e', scan_end_of_line)
#if TRACE_PARSER
    printf("--- pragma once!\n");
#endif
    PragmaOnce = true;
    goto scan_end_of_line;

  begin_preprocessor_en:
    GETNEXT
    ONNOT('d', scan_end_of_line)
    GETNEXT
    ONNOT('i', scan_end_of_line)
    GETNEXT
    ONNOT('f', scan_end_of_line)
    endif();
    goto scan_end_of_line;

  begin_preprocessor_el:
    GETNEXT
    ON('s', begin_preprocessor_els)
    ON('i', begin_preprocessor_eli)
    goto scan_end_of_line;

  begin_preprocessor_els:
    GETNEXT
    ONNOT('e', scan_end_of_line)
    else_();
    goto scan_end_of_line;

  begin_preprocessor_eli:
    GETNEXT
    ONNOT('f', scan_end_of_line)
    goto elif_begin;

  elif_begin:
    GETNEXT
    ONWS(elif_begin);
    Symbol = "";
    ONEOL(elif_end);
    Symbol += CURRENT;
    goto elif_in_progress;

  elif_in_progress:
    GETNEXT
    ONEOL(elif_end);
    Symbol += CURRENT;
    goto elif_in_progress;

  elif_end:
    elif_(Symbol);
    Symbol = "";
    goto from_begin_of_line;

  if_begin:
    GETNEXT
    ONWS(if_begin);
    Symbol = "";
    ONEOL(elif_end);
    Symbol += CURRENT;
    goto if_in_progress;

  if_in_progress:
    GETNEXT
    ONEOL(if_end);
    Symbol += CURRENT;
    goto if_in_progress;

  if_end:
    if_(Symbol);
    Symbol = "";
    goto from_begin_of_line;
  }
}

FileStructure::FileStructure(const FileStructure &anOther) : ModificationTime(anOther.ModificationTime), Main(0)
{
  if (anOther.Main)
    Main = dynamic_cast<Sequence *>(anOther.Main->copy());
}

FileStructure::~FileStructure() { delete Main; }

FileStructure &FileStructure::operator=(const FileStructure &anOther)
{
  if (this != &anOther)
  {
    ModificationTime = anOther.ModificationTime;
    delete Main;
    Main = 0;
    if (anOther.Main)
      Main = dynamic_cast<Sequence *>(anOther.Main->copy());
  }
  return *this;
}

time_t FileStructure::getModificationTime() const { return ModificationTime; }

void FileStructure::setModificationTime(const time_t aTime) { ModificationTime = aTime; }

void FileStructure::localInclude(const std::string &aFile, const std::string &fromFile)
{
#if TRACE_PARSER
  std::cout << "include local file : " << aFile << std::endl;
#endif
  Include *I = new Include(this, aFile, fromFile, false);
  Current->add(I);
}

void FileStructure::systemInclude(const std::string &aFile, const std::string &fromFile)
{
#if TRACE_PARSER
  std::cout << "include system file : " << aFile << std::endl;
#endif
  Include *I = new Include(this, aFile, fromFile, true);
  Current->add(I);
}

void FileStructure::define(const std::string &aName, const std::string &aParam, const std::string &aBody)
{
#if TRACE_PARSER
  std::cout << "define : " << aName << "(" << aParam << ") " << aBody << std::endl;
#endif
  Define *D = new Define(this, aName, aBody);
  Current->add(D);
}

void FileStructure::ifndef(const std::string &aName)
{
#if TRACE_PARSER
  std::cout << "--- ifndef : " << aName << "-(" << Scopes.size() + 1 << ")" << std::endl;
#endif
  If *I = new If(this);
  Current->add(I);
  Current = I->addIf(new If::IfNDef(aName));
  Scopes.push_back(I);
}

void FileStructure::ifdef(const std::string &aName)
{
#if TRACE_PARSER
  std::cout << "--- ifdef : " << aName << "-(" << Scopes.size() + 1 << ")" << std::endl;
#endif
  If *I = new If(this);
  Current->add(I);
  Current = I->addIf(new If::IfDef(aName));
  Scopes.push_back(I);
}

void FileStructure::if_(const std::string &aName)
{
#if TRACE_PARSER
  std::cout << "--- if " << aName << "-(" << Scopes.size() + 1 << ")" << std::endl;
#endif
  If *I = new If(this);
  Current->add(I);
  Current = I->addIf(new If::IfTest(aName));
  Scopes.push_back(I);
}

void FileStructure::endif()
{
#if TRACE_PARSER
  std::cout << "--- endif(" << Scopes.size() << ")" << std::endl;
#endif
  if (Scopes.size() > 0)
  {
    Scopes.pop_back();
    if (Scopes.size() == 0)
      Current = Main;
    else
      Current = Scopes.back()->getLastScope();
  }
  else
    std::cout << ResolvedName << ": Too much endifs for me" << std::endl;
}

void FileStructure::else_()
{
#if TRACE_PARSER
  std::cout << "--- else (" << Scopes.size() << ")" << std::endl;
#endif
  if (Scopes.size() > 0)
    Current = Scopes.back()->addElse();
  else
    std::cerr << "Spurious else without if" << std::endl;
}

void FileStructure::elif_(const std::string &aName)
{
#if TRACE_PARSER
  std::cout << "--- elif (" << Scopes.size() << ")" << aName << std::endl;
#endif
  if (Scopes.size() > 0)
    Current = Scopes.back()->addIf(new If::IfTest(aName));
  else
    std::cerr << "Spurious elsif without if" << std::endl;
}

void FileStructure::getDependencies(CompileState *aState)
{
  if (PragmaOnce && AlreadyIncluded)
  {
#if TRACE_PARSER
    std::cout << "ignoring second inclusion (#pragma once): " << ResolvedName << std::endl;
#endif
    return;
  }
  AlreadyIncluded = true;
  Main->getDependencies(aState);
}

FileCache *FileStructure::getCache() { return Cache; }

std::string FileStructure::getPath() const
{
  int p = ResolvedName.rfind('/');
  int p2 = ResolvedName.rfind('\\');
  if (p == -1 || p2 > p)
    p = p2;

  if (p == -1)
    p = 0;

  char buf[512];
  _snprintf(buf, 512, "%.*s", p, ResolvedName.c_str());
  buf[511] = 0;
  simplify_fname_c(buf);

  return std::string(buf);
}

std::string FileStructure::getFileName() const { return ResolvedName; }
