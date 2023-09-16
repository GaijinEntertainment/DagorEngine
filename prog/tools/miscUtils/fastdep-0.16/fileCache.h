#ifndef FILECACHE_H_
#define FILECACHE_H_

#include <map>
#include <string>
#include <vector>
#include <iostream>

#include "fileStructure.h"
#include "os.h"

class Sequence;

class FileCache
{
public:
  FileCache(const std::string &aBaseDir);
  FileCache(const FileCache &anOther);
  virtual ~FileCache();

  FileCache &operator=(const FileCache &anOther);

  void generate(std::ostream &theStream, const std::string &aDirectory, const std::string &aFilename);
  FileStructure *update(const std::string &aDirectory, const std::string &aFilename, bool isSystem,
    const std::string &aFromFilename = "");

  void addIncludeDir(const std::string &aBaseDir, const std::string &aDir);
  void addIncludeDirFromFile(const std::string &aBaseDir, const std::string &aFilename);
  void addExcludeDir(const std::string &aBaseDir, const std::string &aDir);
  void addAutoInc(const std::string &inc_fname);
  void printAllDependenciesLine(std::ostream *theStream, const std::string &depTarget, int start_idx = 0) const;

  void SetObjectsShouldContainDirectories() { ObjectsContainDirectories = true; };
  void SetObjectsShouldNotContainDirectories() { ObjectsContainDirectories = false; };
  void SetObjectsDir(const std::string &aDir) { ObjectsDir = aDir + sPathSep; }
  void SetObjectsExt(const std::string &aExt) { ObjectsExt = aExt; }

  void SetCompileCommand(const std::string &cmd);
  void SetQuietModeOn() { QuietMode = true; }
  void SetQuietModeOff() { QuietMode = false; }

  void addPreDefine(const std::string &aName);
  void inDebugMode();

private:
  struct FileLocation
  {
    FileLocation(const std::string &aDir, const std::string &aName) : Directory(aDir), Name(aName) {}

    bool operator<(const FileLocation &aLoc) const
    {
      return (Directory < aLoc.Directory) || ((Directory == aLoc.Directory) && (Name < aLoc.Name));
    }
    std::string Directory;
    std::string Name;
  };
  typedef std::map<FileLocation, FileStructure *> FileMap;
  FileMap Files;
  std::vector<std::string> IncludeDirs;
  std::vector<std::string> ExcludeDirs;
  std::vector<std::string> AllDependencies;
  std::vector<std::string> PreDefined;
  std::vector<std::string> PreDefinedVal;
  std::vector<std::string> AutoInc;
  std::string BaseDir;
  std::string ObjectsDir;
  std::string ObjectsExt;
  std::string CompileCommand;
  bool ObjectsContainDirectories;
  static bool QuietMode;
  bool DebugMode;
  bool WarnAboutNonExistingSystemHeaders;
};

#endif
