//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_string.h>

// object that holds undo/redo data
// derive your objects from this one
class UndoRedoObject
{
public:
  virtual ~UndoRedoObject() {}

  // called to undo changes to database
  // if save_redo_data is true, save data necessary to restore
  //   database to its current state in redo operation
  virtual void restore(bool save_redo_data) = 0;

  // redo undone changes
  virtual void redo() = 0;

  // get approximate size of this object (bytes)
  // used to keep undo data size under reasonable limit
  virtual size_t size() = 0;

  // called when this object is accepted in UndoSystem
  //   as a result of accept() or cancel()
  virtual void accepted() = 0;

  // for debugging
  virtual void get_description(String &) = 0;
};

// undo/redo system managing undo/redo for some database
// see create_undo_system()
class UndoSystem
{
public:
  virtual ~UndoSystem() {}

  // call this method to start operation,
  //   then call accept() or cancel() to end it.
  // NOTE: operations can be nested
  virtual void begin() = 0;

  // put object into current operation.
  // if no operation was started (is_holding() is false),
  //   the passed object will be canceled and destroyed.
  virtual void put(UndoRedoObject *) = 0;

  // accept current operation and leave database in its modified state
  // NOTE: operations can be nested
  virtual void accept(const char *operation_name) = 0;

  // cancel current operation and restore database
  //   to its state before last begin(), destroy UndoRedoObjects
  // NOTE: operations can be nested
  virtual void cancel() = 0;

  // returns true if system is saving undo data
  // (begin() was called without matching accept() or cancel())
  virtual bool is_holding() = 0;


  // returns true if undo is possible
  virtual bool can_undo() = 0;

  // returns true if redo is possible
  virtual bool can_redo() = 0;

  // undo last operation
  virtual void undo() = 0;

  // redo last undone operation
  virtual void redo() = 0;

  // returns number of (top-level) operations that can be undone
  virtual int undo_level() = 0;

  // returns i-th undo operation name (0 is last operation,
  //    1 is operation before it, etc...), or NULL if no such operation
  virtual const char *get_undo_name(int i) = 0;

  // returns number of (top-level) operations that can be redone
  virtual int redo_level() = 0;

  // returns i-th redo operation name (0 is current operation,
  //    1 is operation after it, etc...), or NULL if no such operation
  virtual const char *get_redo_name(int i) = 0;

  // set maximum undo size (bytes)
  virtual void set_max_size(int) = 0;

  // get maximum undo size (bytes)
  virtual int get_max_size() = 0;

  // remove all undo/redo operations
  virtual void clear() = 0;

  // "Dirty" flag is set when undo system is modified in some way, or when setDirty() is called.
  virtual bool isDirty() const = 0;

  virtual void setDirty(bool dirty = true) = 0;
};


class IUndoRedoWndClient
{
public:
  virtual void updateUndoRedoMenu() = 0;
};

// create UndoSystem object
UndoSystem *create_undo_system(const char *name, int max_size = 10 << 20, IUndoRedoWndClient *wnd = NULL);
