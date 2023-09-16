#pragma once
#include <datacache/datacache.h>
#include <EASTL/hash_map.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_direct.h>

class DataBlock;

namespace datacache
{

class FileEntry;
struct FindFilesAsyncJob;

class FileBackend : public Backend
{
public:
  typedef eastl::hash_map<size_t, FileEntry *> EntriesMap;

  FileBackend();
  ~FileBackend();
  static Backend *create(const FileBackendConfig &config);

  virtual Entry *get(const char *key, ErrorCode *error, completion_cb_t, void *);
  virtual Entry *set(const char *key, int64_t modtime);
  virtual bool del(const char *key);
  virtual void delAll();

  virtual void control(int opcode, void *p1, void *)
  {
    if (opcode == _MAKE4C('CS'))
      csMgr = (WinCritSec *)p1;
  }

  virtual Entry *nextEntry(void **iter);
  virtual void endEnumeration(void **iter);

  virtual int getEntriesCount();

  virtual void poll() {}

  virtual bool hasFreeSpace() const override { return !manualEviction || curSize < maxSize; }

public:
  void doPopulate();
  void onSizeChange(int delta);
  void onFoundFiles(dag::ConstSpan<Tab<alefind_t>> fnd_results);
  void doEviction();
  EntriesMap::iterator getInternal(const char *key);
  EntriesMap::iterator createNewEntry(const char *mnt, const char *key, int key_len, size_t hash, int size, int64_t atime,
    int64_t mtime);

  static const char *tmpFilePref;

public:
  uint64_t maxSize;
  int64_t curSize;
  bool manualEviction; // do not delete files automatic, user can do it
  int numOpened;
  enum PStatus
  {
    NOT_POPULATED,
    POPULATION_IN_PROGRESS,
    POPULATED
  };
  PStatus populateStatus;
  FindFilesAsyncJob *ffJob;
  int aioJobId;
  EntriesMap entries;
  const char *roMountPath;
  WinCritSec *csMgr = NULL;
  char mountPath[1]; // varlen (must be last member)
};

}; // namespace datacache
