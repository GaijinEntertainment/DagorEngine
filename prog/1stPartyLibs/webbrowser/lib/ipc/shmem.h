// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string>
#include "../log.h"


namespace ipc
{

class SharedMemory : public webbrowser::EnableLogging
{
  public:
    typedef uint8_t data_type;
  public:
    SharedMemory(webbrowser::ILogger& l);
    virtual ~SharedMemory();

  public:
    bool create(size_t sz);
    bool create(const std::string& key, size_t sz);
    bool resize(size_t sz);
    bool attach(const std::string& key, size_t sz);
    void close();
    const char* getKey() { return this->key.c_str(); }
    size_t getSize() { return this->size; }

    size_t write(const data_type *src, size_t sz);
    size_t read(data_type *dst, size_t max_sz);

  private:
    std::string key;
    size_t size;

  private:
    struct Handle;
    Handle *handle;
}; // class SharedMemory

} // namespace ipc
