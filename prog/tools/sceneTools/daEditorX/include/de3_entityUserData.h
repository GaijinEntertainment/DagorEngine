//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class IObjEntityUserDataHolder
{
public:
  static constexpr unsigned HUID = 0x5B740457u; // IObjEntityUserDataHolder

  virtual DataBlock *getUserDataBlock(bool create_if_not_exist) = 0;
  virtual void resetUserDataBlock() = 0;
  virtual void setSuperEntityRef(const char *ref) {}

  virtual bool canBeParentForAttach() { return false; }
  virtual bool canBeAttached() { return false; }
  virtual void attachTo(IObjEntity * /*e*/, const TMatrix & /*local_tm*/) {}
};
