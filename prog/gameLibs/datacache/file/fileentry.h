#pragma once
#include <util/dag_stdint.h>
#include <datacache/datacache.h>
#include <util/dag_simpleString.h>
#include <debug/dag_assert.h>

class IGenLoad;
class IGenSave;

namespace datacache
{

class FileBackend;

class FileEntry : public Entry
{
public:
  int refCount;

  enum OpType
  {
    OT_NONE,
    OT_MMAP,
    OT_READ_STREAM,
    OT_WRITE_STREAM
  };
  volatile OpType opType;
  union
  {
    const void *mmapPtr;
    IGenLoad *readStream;
    IGenSave *writeStream;
  };
  SimpleString tempFileName;
  void *file_ptr;
  bool delOnFree;
  bool flushFileTimes;
  bool readOnly;

  int64_t lastUsed;
  int64_t lastModified;
  int dataSize, dataWritten;
  int sizeDelta;
  FileBackend *backend; // backref, can be NULL if entry "unlinked" from backend
  const char *key;      // points to the middle of filePath
  char filePath[1];     // varlen (must be last member)

  FileEntry(FileBackend *back);
  ~FileEntry();
  static FileEntry *create(FileBackend *bck, const char *mnt, const char *key, int key_len);

  FileEntry *get(bool file_time = true);
  bool isOpened() const { return opType != OT_NONE; }
  void freeInternal();

  void addRef() { refCount++; }
  bool delRef()
  {
    if (--refCount == 0)
    {
      delete this;
      return true;
    }
    return false;
  }

  void detach(FileBackend *back)
  {
    (void)back;
    G_ASSERT(back == backend);
    backend = NULL;
    delRef();
  }
  bool remove()
  {
    bool ret = refCount == 1;
    del();
    freeInternal();
    return ret;
  } // return true if entry was actually deleted (i.e. resources was freed)

  //
  // Entry interface
  //
  virtual const char *getKey() const { return key; }
  virtual const char *getPath() const { return filePath; }
  virtual int64_t getLastUsed() const { return lastUsed; }
  virtual int64_t getLastModified() const { return lastModified; }
  virtual void setLastModified(int64_t);
  virtual int getDataSize() const { return dataSize; }

  virtual dag::ConstSpan<uint8_t> getData();
  virtual IGenLoad *getReadStream();
  virtual IGenSave *getWriteStream();
  virtual void closeStream();

  virtual void del();
  virtual void free();
};

}; // namespace datacache
