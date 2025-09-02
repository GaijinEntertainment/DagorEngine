// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <util/dag_preprocessor.h>

namespace memoryreport
{

struct Entry
{
  size_t memory_allocated{};
  int64_t allocation_diff{};

  int64_t used_device_local_vram{};
  int64_t used_shared_vram{};
};

class IBackend
{
public:
  virtual void onReport(const Entry &entry) = 0;

  virtual void onStart() = 0;
  virtual void onStop() = 0;
};

struct BackendList
{
  BackendList(eastl::string name, IBackend *backend) : name(name), backend(backend)
  {
    next = head;
    head = this;
  }

  BackendList *next = nullptr;
  static BackendList *head;

  eastl::string name;
  IBackend *backend;
};

} // namespace memoryreport

#define MEMORYREPORT_PULL_VAR_NAME(name) DAG_CONCAT(memory_report_pull_, name)

#define PULL_MEMORYREPORT_BACKEND(name)        \
  namespace memoryreport                       \
  {                                            \
  extern int MEMORYREPORT_PULL_VAR_NAME(name); \
  }                                            \
  void pull_memoryreport_##name() { memoryreport::MEMORYREPORT_PULL_VAR_NAME(name) = 1; }

#define REGISTER_MEMORYREPORT_BACKEND(name, backend) \
  namespace memoryreport                             \
  {                                                  \
  int MEMORYREPORT_PULL_VAR_NAME(name) = 1;          \
  }                                                  \
  static memoryreport::BackendList backend_registration(#name, &backend);
