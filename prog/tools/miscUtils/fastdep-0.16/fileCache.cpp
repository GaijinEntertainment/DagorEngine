#include "fileCache.h"
#include "fileStructure.h"
#include "compileState.h"
#include "os.h"
#include "mappedFile.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>


#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include <algorithm>

#ifdef WIN32
extern char *realpath(const char *path, char resolved_path[]);
#define PATH_MAX MAX_PATH
#endif

using namespace std;

bool FileCache::QuietMode = false;


FileCache::FileCache(const std::string &aBaseDir) :
  BaseDir(aBaseDir), ObjectsDir(""), ObjectsExt(".obj"), DebugMode(false), WarnAboutNonExistingSystemHeaders(true)
{
  CompileCommand = "";
  SetObjectsShouldContainDirectories();
  IncludeDirs.push_back(".");
}

FileCache::FileCache(const FileCache &anOther) {}

FileCache::~FileCache() {}

FileCache &FileCache::operator=(const FileCache &anOther) { return *this; }

void FileCache::inDebugMode() { DebugMode = true; }

void FileCache::addPreDefine(const std::string &aName)
{
  int pos = aName.find('=');
  PreDefined.push_back(aName.substr(0, pos));
  if (pos != -1)
    PreDefinedVal.push_back(aName.substr(pos + 1, -1));
  else
    PreDefinedVal.push_back("1");
}
void FileCache::addAutoInc(const std::string &inc_fname) { AutoInc.push_back(inc_fname); }

void FileCache::generate(std::ostream &theStream, const std::string &aDirectory, const std::string &aFilename)
{
  CompileState State(aDirectory);
  if (DebugMode)
    State.inDebugMode();

  char buf[260];
  std::string fDir = get_fname_location(buf, aFilename.c_str());
  IncludeDirs.insert(IncludeDirs.begin(), fDir);

  for (unsigned int i = 0; i < PreDefined.size(); ++i)
    State.define(PreDefined[i], PreDefinedVal[i]);

  if (is_path_relative(aFilename.c_str()))
  {
    char buf[512];
    _snprintf(buf, 512, "%s/%s", aDirectory.c_str(), aFilename.c_str());
    buf[511] = 0;

    simplify_fname_c(buf);
    State.addDependencies(buf);
  }
  else
    State.addDependencies(aFilename);

  FileStructure *Structure;

  for (unsigned int i = 0; i < AutoInc.size(); ++i)
  {
    Structure = update(aDirectory, AutoInc[i], false);
    if (Structure)
    {
      State.addDependencies(Structure->getFileName());
      Structure->getDependencies(&State);
      State.mergeDeps(AllDependencies);
    }
  }

  Structure = update(aDirectory, aFilename, false);
  if (Structure)
  {
    Structure->getDependencies(&State);
    //		State.dump();
    State.mergeDeps(AllDependencies);
  }
  IncludeDirs.erase(IncludeDirs.begin());
}

void FileCache::SetCompileCommand(const std::string &cmd) { CompileCommand = cmd; }

void FileCache::addIncludeDir(const std::string &aBaseDir, const std::string &aDir)
{
  if (std::find(IncludeDirs.begin(), IncludeDirs.end(), aDir) == IncludeDirs.end())
    IncludeDirs.push_back(aDir);
}

void FileCache::addExcludeDir(const std::string &aBaseDir, const std::string &aDir)
{
  char buf[512];
  strncpy(buf, aDir.c_str(), 510);
  buf[510] = '\0';
  strcat(buf, "/");
  simplify_fname_c(buf);

  if (std::find(ExcludeDirs.begin(), ExcludeDirs.end(), buf) == ExcludeDirs.end())
    ExcludeDirs.push_back(buf);
}

void FileCache::addIncludeDirFromFile(const std::string &aBaseDir, const std::string &aFilename)
{
  std::ifstream ifile(aFilename.c_str());
  if (!ifile)
  {
    // XXX Error message here
    if (!QuietMode)
    {
      std::cerr << __LINE__ << ":error opening " << aFilename.c_str() << std::endl;
    }
    // exit or return ?
    return;
  }
  while (!ifile.eof())
  {
    std::string aDir;
    std::getline(ifile, aDir);
    aDir = aDir.substr(0, aDir.find('\n'));
    aDir = aDir.substr(0, aDir.find('\r'));
    if (aDir != "")
      addIncludeDir(aBaseDir, aDir);
  }
}


FileStructure *FileCache::update(const std::string &aDirectory, const std::string &aFilename, bool isSystem,
  const std::string &aFromFilename)
{
  FileMap::iterator iti = Files.find(FileLocation(aDirectory, aFilename));
  if (iti != Files.end())
  {
    /*		if(!iti->second && !QuietMode && (!isSystem || WarnAboutNonExistingSystemHeaders))
        {
          std::cerr << __LINE__	<< ":error opening " << aFilename.c_str() << std::endl;
        } */

    return iti->second;
  }

  std::unique_ptr<MappedFile> mfile(new MappedFile);

  if (DebugMode)
    std::cout << "[DEBUG] FileCache::update(" << aDirectory << "," << aFilename << "," << isSystem << ");" << std::endl;
  char ResolvedBuffer[PATH_MAX + 1];
  bool test_near_path = false;

  if (!isSystem)
  {
    std::string theFilename;
    if (is_path_relative(aDirectory.c_str()))
      theFilename = BaseDir + cPathSep + aDirectory + cPathSep + aFilename;
    else
      theFilename = aDirectory + cPathSep + aFilename;

    if (DebugMode)
      std::cout << "[DEBUG] realpath(" << theFilename << ",buf);" << std::endl;
    if (realpath(theFilename.c_str(), ResolvedBuffer))
      test_near_path = true;
  }

  if (test_near_path && mfile->open(ResolvedBuffer))
    ; // file found
  else
  {
    unsigned int i;
    for (i = 0; i < IncludeDirs.size(); ++i)
    {
      std::string theFilename = IncludeDirs[i] + cPathSep + aFilename;
      if (is_path_relative(IncludeDirs[i].c_str()))
        theFilename = BaseDir + cPathSep + theFilename;
      if (DebugMode)
        std::cout << "[DEBUG] realpath(" << theFilename << ",buf);" << std::endl;
      if (!realpath(theFilename.c_str(), ResolvedBuffer))
        continue;

      if (mfile->open(ResolvedBuffer))
        break;
    }

    if (i == IncludeDirs.size())
    {
      std::cout << "[DEBUG] not found: " << ResolvedBuffer << ", " << aFilename << std::endl;
      std::string theFilename = aDirectory + cPathSep + aFilename;
      if (aFilename[0] == cPathSep)
      {
        theFilename = aFilename;
      }
      if (!realpath(theFilename.c_str(), ResolvedBuffer))
      {
        if (!QuietMode && (!isSystem || WarnAboutNonExistingSystemHeaders))
        {
          if (aFromFilename != "")
            std::cerr << "including from " << aFromFilename << ":\n";
          std::cerr << __LINE__ << ":error opening " << aFilename.c_str() << std::endl;
        }
        Files[FileLocation(aDirectory, aFilename)] = 0;
        return 0;
      }
      if (!mfile->open(ResolvedBuffer))
      {
        if (!QuietMode && (!isSystem || WarnAboutNonExistingSystemHeaders))
        {
          if (aFromFilename != "")
            std::cerr << "including from " << aFromFilename << ":\n";
          std::cerr << __LINE__ << ":error opening " << aFilename.c_str() << std::endl;
        }
        Files[FileLocation(aDirectory, aFilename)] = 0;
        return 0;
      }
    }

    int rb_len = strlen(ResolvedBuffer);
    for (i = 0; i < ExcludeDirs.size(); ++i)
    {
      if (rb_len > ExcludeDirs[i].length() && strnicmp(ResolvedBuffer, ExcludeDirs[i].c_str(), ExcludeDirs[i].length()) == 0)
      {
        if (DebugMode)
          std::cout << "[DEBUG] excluding : " << ResolvedBuffer << std::endl;
        Files[FileLocation(aDirectory, aFilename)] = 0;
        return 0;
      }
    }
  }
  std::string ResolvedName(ResolvedBuffer);

  long file_size = mfile->size();
  time_t last_change = mfile->time();

  if (file_size == 0)
  {
    std::cerr << ResolvedName << " is empty." << std::endl;
    return 0;
  }
  char *map = 0;
  try
  {
    map = mfile->map();
  }
  catch (string s)
  {
    std::cerr << s << std::endl;
    exit(1);
  }

  FileStructure *S = new FileStructure(ResolvedName, this, map, file_size);
  S->setModificationTime(last_change);
  Files.insert(std::make_pair(FileLocation(aDirectory, aFilename), S));

  return S;
}

void FileCache::printAllDependenciesLine(std::ostream *theStream, const std::string &depTarget, int start_idx) const
{
  int p = depTarget.rfind('/');
  int p2 = depTarget.rfind('\\');
  if (p == -1 || p2 > p)
    p = p2;

  std::string basename(depTarget, p + 1, -1);
  std::string dep(basename, 0, basename.rfind("."));
  dep += ObjectsExt;

  for (unsigned int i = start_idx; i < AllDependencies.size(); ++i)
    *theStream << dep << ": " << AllDependencies[i] << std::endl;
}

// vim:ts=4:nu
//
