#ifndef FILESTRUCTURE_H_
#define FILESTRUCTURE_H_

#include <string>

#include <time.h>

#include "if.h"

class CompileState;
class FileCache;
class Sequence;

class FileStructure
{
public:
  FileStructure(const std::string &aFilename, FileCache *aCache, const char *aFile, unsigned int aLength);
  FileStructure(const FileStructure &anOther);
  virtual ~FileStructure();

  FileStructure &operator=(const FileStructure &anOther);

  void localInclude(const std::string &aFile, const std::string &fromFile);
  void systemInclude(const std::string &aFile, const std::string &fromFile);
  void define(const std::string &aName, const std::string &aParam, const std::string &aBody);
  void ifndef(const std::string &aName);
  void ifdef(const std::string &aName);
  void endif();
  void else_();
  void if_(const std::string &aName);
  void elif_(const std::string &aName);

  time_t getModificationTime() const;
  void setModificationTime(const time_t aTime);
  void getDependencies(CompileState *aState);

  FileCache *getCache();

  std::string getPath() const;
  std::string getFileName() const;

private:
  time_t ModificationTime;
  std::string ResolvedName;

  Sequence *Main;
  Sequence *Current;
  FileCache *Cache;
  std::vector<If *> Scopes;
  bool PragmaOnce, AlreadyIncluded;
};

#endif
