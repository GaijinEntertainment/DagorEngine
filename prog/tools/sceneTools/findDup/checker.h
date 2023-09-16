#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>


static const int MD5_LEN = 16;

struct FileInfo
{
  FileInfo(const char *file) : filePath(file){};

  SimpleString filePath;
  unsigned char hash[MD5_LEN];
};


struct FilePair
{
  FilePair(int i, int j) : f1(i), f2(j){};

  int f1, f2;
};


class FileChecker
{
public:
  FileChecker(bool all_files);
  ~FileChecker();

  void gatherFileNames(const char *dir_path);
  void checkForDuplicates();

  Tab<FileInfo *> files;
  Tab<FilePair> fileDups;
  bool allFiles;

private:
  void checkDir(const char *dir_name, const char *parent_dir);
  void processFile(const char *file_path);
  void calculateHash(unsigned char *md5_hash, unsigned char *data, int len);
};
