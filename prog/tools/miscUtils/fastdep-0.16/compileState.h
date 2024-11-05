// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string>
#include <vector>

class CompileState
{
public:
  CompileState(const std::string &aBase);
  CompileState(const CompileState &anOther);
  virtual ~CompileState();

  CompileState &operator=(const CompileState &anOther);

  void addDependencies(const std::string &aString);
  std::string getDependenciesLine() const;

  void define(const std::string &aString, const std::string &aContent);
  void undef(const std::string &aString);
  bool isDefined(const std::string &aString) const;
  std::string getContent(const std::string &aName) const;

  void dump() const;

  void mergeDeps(std::vector<std::string> &aDepLine);
  void inDebugMode();

private:
  std::vector<std::string> Dependencies;
  std::string Basedir;
  std::vector<std::string> Defines;
  std::vector<std::string> Contents;
  bool DebugMode;
};
