// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <generic/dag_tab.h>


class String;
class DataBlock;
class ObjectEditor;


namespace mkbindump
{
class BinDumpSaveCB;
}


namespace ScriptHelpers
{
class TunedElement;


class ParamChangeCB
{
public:
  virtual void onScriptHelpersParamChange() = 0;
};


class SaveDataCB
{
public:
  virtual int getRefSlotId(const char *name, bool make_unique = true) = 0;
};


class SaveValuesCB
{
public:
  virtual void addRefSlot(const char *name, const char *type_name, bool make_unique = true) = 0;
};


class Context
{
public:
  Context();
  ~Context();

  void setTunedElement(TunedElement *elem);

  TunedElement *getTunedElement() const { return rootElem; }

  void loadParams(const DataBlock &blk);
  void saveParams(DataBlock &blk, SaveValuesCB *save_cb);

  void saveParamsData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb);

protected:
  TunedElement *rootElem;
};


extern ObjectEditor *obj_editor;

void set_param_change_cb(ParamChangeCB *cb);
void on_param_change();

void set_tuned_element(TunedElement *elem);

// void clear_helpers();
void load_helpers(const DataBlock &blk);
void save_helpers(DataBlock &blk, SaveValuesCB *save_cb);

void save_params_data(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb);
}; // namespace ScriptHelpers
