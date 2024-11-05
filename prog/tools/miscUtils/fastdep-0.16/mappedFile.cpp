// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifdef WIN32

#include <direct.h>
#include <io.h>
#define PATH_MAX MAX_PATH
#else
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#endif

#include <memory>

#include "mappedFile.h"

#ifdef WIN32
// http://dotnet.di.unipi.it/Content/sscli/docs/doxygen/pal/filetime_8c-source.html
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600;
static const __int64 SECS_TO_100NS = 10000000; /* 10^7 */


time_t FILEFileTimeToUnixTime(FILETIME FileTime, long *nsec)
{
  __int64 UnixTime;

  /* get the full win32 value, in 100ns */
  UnixTime = ((__int64)FileTime.dwHighDateTime << 32) + FileTime.dwLowDateTime;

  /* convert to the Unix epoch */
  UnixTime -= (SECS_BETWEEN_EPOCHS * SECS_TO_100NS);

  //     TRACE("nsec=%p\n", nsec);

  if (nsec)
  {
    /* get the number of 100ns, convert to ns */
    *nsec = (UnixTime % SECS_TO_100NS) * 100;
  }

  UnixTime /= SECS_TO_100NS; /* now convert to seconds */

  if ((time_t)UnixTime != UnixTime)
  {
    //         WARN("Resulting value is too big for a time_t value\n");
  }

  /*
TRACE("Win32 FILETIME = [%#x:%#x] converts to Unix time = [%ld.%09ld]\n",
        FileTime.dwHighDateTime, FileTime.dwLowDateTime ,(long) UnixTime,
        nsec?*nsec:0L);
*/
  return (time_t)UnixTime;
}

MappedFile::~MappedFile()
{
  UnmapViewOfFile(map_);
  CloseHandle(hFileMapping);
  CloseHandle(hFile);
}

bool MappedFile::open(const std::string &name)
{
  hFile = CreateFile(name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  opened_ = hFile == INVALID_HANDLE_VALUE ? false : true;

  if (opened_)
  {
    file_size = GetFileSize(hFile, NULL);
    FILETIME fTime;
    GetFileTime(hFile, NULL, NULL, &fTime);
    long nsec;
    last_change = FILEFileTimeToUnixTime(fTime, &nsec);
  }
  return opened_;
}

char *MappedFile::map()
{
  hFileMapping = CreateFileMapping(hFile, NULL, PAGE_WRITECOPY, 0, 0, NULL);
  return map_ = (char *)MapViewOfFile(hFileMapping, FILE_MAP_COPY, 0, 0, 0);
};

#else

MappedFile::~MappedFile()
{
  munmap(map_, mapsize);
  close(fd);
}


bool MappedFile::open(const std::string &name)
{
  fd = ::open(name.c_str(), O_RDONLY);
  opened_ = fd >= 0 ? true : false;
  if (opened_)
  {
    struct stat st;
    fstat(fd, &st);
    file_size = st.st_size;
    last_change = st.st_mtime;
    mapsize = st.st_size;
  }
  return opened_;
}

char *MappedFile::map() throw(std::string)
{
  int pagesizem1 = getpagesize() - 1;

  mapsize = (mapsize + pagesizem1) & ~pagesizem1;
  map_ = (char *)mmap(NULL, mapsize, PROT_READ, MAP_PRIVATE, fd, 0);
  if ((long)map_ == -1)
  {
    perror("mkdep: mmap");
    close(fd);
    return 0;
  }
  if ((unsigned long)map_ % sizeof(unsigned long) != 0)
  {
    throw std::string("do_depend: map not aligned");
  }
  return map_;
};

#endif

// vim:ts=4:nu
//
