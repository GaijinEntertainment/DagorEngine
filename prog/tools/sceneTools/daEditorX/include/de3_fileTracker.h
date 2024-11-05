//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class IFileChangedNotify;

class IFileChangeTracker
{
public:
  static constexpr unsigned HUID = 0xFAA4F86Bu; // IFileChangeTracker

  virtual int addFileChangeTracking(const char *fname) = 0;
  virtual void subscribeUpdateNotify(int file_name_id, IFileChangedNotify *notify) = 0;
  virtual void unsubscribeUpdateNotify(int file_name_id, IFileChangedNotify *notify) = 0;

  virtual const char *getFileName(int file_name_id) = 0;

  virtual void lazyCheckOnAct() = 0;
  virtual void immediateCheck() = 0;
};

class IFileChangedNotify
{
public:
  virtual void onFileChanged(int file_name_id) = 0;
};
