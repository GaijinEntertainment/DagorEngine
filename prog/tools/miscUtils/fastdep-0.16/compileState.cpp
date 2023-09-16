#include "compileState.h"
#include "os.h"

#include <algorithm>
#include <iostream>

CompileState::CompileState(const std::string &aBase) : Basedir(aBase), DebugMode(false) {}

CompileState::CompileState(const CompileState &anOther) {}

CompileState::~CompileState() {}

CompileState &CompileState::operator=(const CompileState &anOther)
{
  // if (this != &anOther) {}
  return *this;
}

void CompileState::inDebugMode() { DebugMode = true; }

void CompileState::addDependencies(const std::string &aString)
{
  char buf[512];
  _snprintf(buf, 512, aString.c_str());
  buf[511] = '\0';
  simplify_fname_c(buf);

  std::string Beef(buf);

  // if (std::string(Beef, 0, Basedir.length() + 1) == Basedir + "/")
  //	 Beef = std::string(aString, Basedir.length() + 1, aString.length());
  if (std::find(Dependencies.begin(), Dependencies.end(), Beef) == Dependencies.end())
    Dependencies.push_back(Beef);
}

std::string CompileState::getDependenciesLine() const
{
  std::string Result;
  for (unsigned int i = 0; i < Dependencies.size(); ++i)
    Result += " \\\n\t" + Dependencies[i];
  return Result;
}

void CompileState::define(const std::string &aString, const std::string &aContent)
{
  if (DebugMode)
    std::cout << "[DEBUG] define " << aString << " as " << aContent << ";" << std::endl;
  if (std::find(Defines.begin(), Defines.end(), aString) == Defines.end())
  {
    Defines.push_back(aString);
    Contents.push_back(aContent);
  }
}

void CompileState::undef(const std::string &aString)
{
  if (DebugMode)
    std::cout << "[DEBUG] undef " << aString << std::endl;
  std::vector<std::string>::iterator i = std::find(Defines.begin(), Defines.end(), aString);
  while (i != Defines.end())
  {
    unsigned int Pos = i - Defines.begin();
    Defines.erase(i);
    Contents.erase(Contents.begin() + Pos);
    i = std::find(Defines.begin(), Defines.end(), aString);
  }
}

std::string CompileState::getContent(const std::string &aName) const
{
  std::vector<std::string>::const_iterator i = std::find(Defines.begin(), Defines.end(), aName);
  if (i != Defines.end())
  {
    unsigned int Pos = i - Defines.begin();
    return Contents[Pos];
  }
  return "";
}

bool CompileState::isDefined(const std::string &aString) const
{
  bool Result = std::find(Defines.begin(), Defines.end(), aString) != Defines.end();
  if (DebugMode)
    std::cout << "[DEBUG] isdefined " << aString << " : " << Result << ";" << std::endl;
  return Result;
}

void CompileState::dump() const
{
  std::cout << "defined:" << std::endl;
  for (unsigned int i = 0; i < Defines.size(); ++i)
    std::cout << " " << Defines[i] << std::endl;
}

void CompileState::mergeDeps(std::vector<std::string> &aDepLine)
{
  for (unsigned int i = 0; i < Dependencies.size(); ++i)
    if (std::find(aDepLine.begin(), aDepLine.end(), Dependencies[i]) == aDepLine.end())
      aDepLine.push_back(Dependencies[i]);
}
