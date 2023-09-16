#include "shmem.h"
#include <assert.h>

#include <windows.h>
#include <Rpc.h>
#include <time.h>

#include "../unicodeString.h"


namespace ipc
{
using UnicodeString = webbrowser::UnicodeString;

struct SharedMemory::Handle : public webbrowser::EnableLogging
{
  Handle(webbrowser::ILogger& l) : EnableLogging(l), file(NULL), mutex(NULL), view(NULL) { /* VOID */ }
  ~Handle() { this->close(); }

  HANDLE file, mutex;
  SharedMemory::data_type *view;


  bool create(const std::string& key, size_t size)
  {
    UnicodeString utf8Key(createFullKey(key).c_str());
    std::wstring k = utf8Key.wide();
    file = CreateFileMappingW(INVALID_HANDLE_VALUE, // use paging file
                              NULL,                 // default security
                              PAGE_READWRITE,       // read/write access
                              0,                    // maximum object size (high-order DWORD)
                              size,                 // maximum object size (low-order DWORD)
                              k.c_str());           // name of mapping object
    if (file != INVALID_HANDLE_VALUE)
      view = (SharedMemory::data_type*)MapViewOfFile(file,
                                                     FILE_MAP_ALL_ACCESS,
                                                     0,
                                                     0,
                                                     size);
    else
    {
      ERR("%s: Could not create file mapping for %s: %d", __FUNCTION__, utf8Key.utf8(), GetLastError());
      return false;
    }

    if (!view)
    {
      ERR("%s: Could not create view for %s: %d", __FUNCTION__, utf8Key.utf8(), GetLastError());
      CloseHandle(file);
    }
    else
      mutex = CreateMutexW(NULL, FALSE, (k+L"-lock").c_str());

    return view && mutex;
  }

  bool attach(const std::string& key, size_t size)
  {
    UnicodeString utf8Key(createFullKey(key).c_str());
    std::wstring k = utf8Key.wide();
    file = OpenFileMappingW(FILE_MAP_ALL_ACCESS, false, k.c_str());
    if (file)
      view = (SharedMemory::data_type*)MapViewOfFile(file,
                                                     FILE_MAP_ALL_ACCESS,
                                                     0,
                                                     0,
                                                     size);
    else
    {
      ERR("%s: could not open file mapping %s: %d", __FUNCTION__, utf8Key.utf8(), GetLastError());
      return false;
    }

    if (!view)
    {
      ERR("%s: could not map view for %s: %d", __FUNCTION__, utf8Key.utf8(), GetLastError());
      CloseHandle(file);
    }
    else
      mutex = OpenMutexW(SYNCHRONIZE, FALSE, (k+L"-lock").c_str());

    return view && mutex;
  }

  void close()
  {
    if (view)
      UnmapViewOfFile(view);
    if (file != INVALID_HANDLE_VALUE)
      CloseHandle(file);
    if (mutex)
      CloseHandle(mutex);
    file = INVALID_HANDLE_VALUE;
    mutex = view = NULL;
  }

  bool lock()
  {
    DWORD r = WaitForSingleObject(mutex, 5);
    if (r != WAIT_OBJECT_0 && r != WAIT_TIMEOUT)
    {
      int err = GetLastError();
      WRN("%s: could not lock (%x - %d)", __FUNCTION__, r, err);
    }

    return r == WAIT_OBJECT_0;
  }

  void unlock()
  {
    if (!ReleaseMutex(mutex))
    {
      int err = GetLastError();
      ERR("%s: could not unlock (%d)", __FUNCTION__, err);
    }
  }

private:
  std::string createFullKey(const std::string& k) { return "Local\\" + k; }
}; // struct SharedMemory::Handle


SharedMemory::SharedMemory(webbrowser::ILogger& l) : EnableLogging(l), size(0)
{
  this->handle = new SharedMemory::Handle(l);
}


SharedMemory::~SharedMemory()
{
  this->close();
  delete this->handle;
}


bool SharedMemory::create(const std::string& k, size_t sz)
{
  assert(this->key.empty());
  assert(!this->size);
  assert(sz > 0);

  this->key = k;
  if (!this->handle->create(this->key.c_str(), sz))
  {
    this->key.clear();
    this->key.shrink_to_fit();
    return false;
  }

  this->size = sz;
  return true;
}

static void gen_random_key(char *out, size_t len)
{
  static const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyaABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static const char magic[] = "dacef3";
  static const size_t magicLen = sizeof(magic) - 1;

  srand(time(NULL));
  size_t keyStartPos = magicLen < len ? magicLen : 0;
  if (keyStartPos)
    memcpy(out, magic, magicLen);

  for (int i=keyStartPos; i < len; ++i)
    out[i] = charset[rand() % (sizeof(charset) - 1)];

  out[len] = '\0';
}

bool SharedMemory::create(size_t sz)
{
  UUID uuid = { 0 };
  char uuidStr[64] = { 0 };
  RPC_STATUS e = UuidCreate(&uuid);
  if (e == RPC_S_OK || e == RPC_S_UUID_LOCAL_ONLY)
    snprintf(uuidStr, sizeof(uuidStr), "%.8lX%.4X%.4X"
                                       "%.2X%.2X%.2X%.2X"
                                       "%.2X%.2X%.2X%.2X",
                                       uuid.Data1, uuid.Data2, uuid.Data3,
                                       uuid.Data4[0], uuid.Data4[1], uuid.Data4[2], uuid.Data4[3],
                                       uuid.Data4[4], uuid.Data4[5], uuid.Data4[6], uuid.Data4[7]);
  else
  {
    WRN("%s: Could not create UUID (err = %d), falling back to random string", __FUNCTION__, e);
    gen_random_key(uuidStr, sizeof(uuidStr));
  }

  return this->create(uuidStr, sz);
}


bool SharedMemory::attach(const std::string& k, size_t sz)
{
  assert(this-key.empty());
  if (this->handle->attach(k, sz))
  {
    this->key = k;
    this->size = sz;
  }

  return !this->key.empty();
}


void SharedMemory::close()
{
  this->handle->close();
  this->size = 0;
  this->key.clear();
  this->key.shrink_to_fit();
}


bool SharedMemory::resize(size_t sz)
{
  if (sz <= this->size)
    return true;

  this->close();
  return this->create(sz);
}


size_t SharedMemory::write(const SharedMemory::data_type *data, size_t sz)
{
  assert(sz <= this->size);
  if (!this->handle->lock())
    return 0;

  memcpy(this->handle->view, data, sz);
  this->handle->unlock();
  return sz;
}


size_t SharedMemory::read(SharedMemory::data_type *data, size_t sz)
{
  assert(sz <= this->size);
  if (!this->handle->lock())
    return 0;

  memcpy(data, this->handle->view, sz);
  this->handle->unlock();
  return sz;
}

} // namespace ipc
