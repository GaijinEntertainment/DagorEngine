#ifndef __MAPPEDFILE_H__
#define __MAPPEDFILE_H__

#include <string>

#ifdef WIN32

#include <windows.h>

struct MapFileData
{
  HANDLE hFile;
  HANDLE hFileMapping;
};

#else

struct MapFileData
{
  int mapsize;
  int fd;
};

#endif

/** Class MappedFile creates memory file mapping.
 *
 *  Works with Windows Mingw GCC, VC++.NET and on Unix
 */
class MappedFile : MapFileData
{
  bool opened_;
  long file_size;
  time_t last_change;
  char *map_;

public:
  /** C'tor
   *
   */
  MappedFile() : opened_(false), file_size(0) {}

  /** D'tor
   *
   */
  virtual ~MappedFile();

  /** Size of the opened file.
   *
   */
  long size() const { return file_size; }

  /** Modification time of the opened file.
   *
   */
  time_t time() const { return last_change; }

  /** Opens a file for memory mapping
   *
   */
  bool open(const std::string &name);

  /** Maps the file into memory
   *
   */
  char *map();
};

#endif // __MAPPEDFILE_H__

// vim:ts=4:nu
//
