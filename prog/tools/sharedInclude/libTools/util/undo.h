//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>
#include <EASTL/internal/move_help.h>
#include <EASTL/type_traits.h>

// What an UndoRedoObject covers: which slice of which target. Used to find the one object in an
// operation a new one could fold into. Both halves are needed, one frame of a drag covers several
// targets and several slices.
struct UndoMergeKey
{
  unsigned kind = 0;            // one UID per UndoRedoObject class, see UNDO_MERGE_KEY_BY_TARGET
  const void *target = nullptr; // what the state was read from

  bool is_mergeable() const { return kind != 0; }
  bool operator==(const UndoMergeKey &k) const { return kind == k.kind && target == k.target; }
};

// True only when T declares get_merge_key() itself. The address of an inherited member names the class
// that declared it, so a subclass that does not repeat the macro below reads as false.
// One declared with another signature, a missing const say, hides the base rather than overriding it and
// reads as false too, folding nothing and reporting nothing: declare a key only through the macro below.
template <class T>
constexpr bool undo_declares_merge_key()
{
  return eastl::is_same_v<decltype(&T::get_merge_key), UndoMergeKey (T::*)() const>;
}

// Declares the merge key. Implement merge() too, a key alone folds nothing.
//
// `kind_uid` is what makes the key this class's own. Generate it with tools/util/hash.exe from a name for
// this class and write that name in the comment beside it, the way a CID_ is made.
//
// WARNING: THE NAME MUST BE UNIQUE ACROSS EVERY UNDO CLASS. Qualify it with the owner where the class name
// alone is not, as the several UndoPropsChange do: two classes sharing a UID fold into each other, and the
// one that is dropped loses its restore() with nothing to report it.
#define UNDO_MERGE_KEY_BY_TARGET(kind_uid, target_expr) \
  UndoMergeKey get_merge_key() const override { return UndoMergeKey{.kind = (kind_uid), .target = (target_expr)}; }

// For an object that snapshots its target whole and rereads the redo state in restore(), so a later
// snapshot of the same slice adds nothing. One holding both values up front needs its own merge().
#define UNDO_MERGE_SNAPSHOT_BY_TARGET(kind_uid, target_expr) \
  UNDO_MERGE_KEY_BY_TARGET(kind_uid, target_expr)            \
  bool merge(const UndoRedoObject &) override { return true; }

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

  // False for an object that changes what the editor shows and not what the document saves, a
  // selection for example. A save point looks past those, see UndoSystem::set_save_point.
  virtual bool changes_saved_data() const { return true; }

  // get approximate size of this object (bytes)
  // used to keep undo data size under reasonable limit
  virtual size_t size() = 0;

  // see UndoMergeKey, declare it through UNDO_MERGE_KEY_BY_TARGET
  virtual UndoMergeKey get_merge_key() const { return UndoMergeKey(); }

  // Fold `newer`, which has the same key, into this object and return true so it can be dropped; false
  // keeps both, must change nothing, and may be asked again. Both always belong to one operation, so
  // data taken out of `newer` cannot outlive a cancel of it, but never keep a pointer into `newer`:
  // put<T>() moves it away right after. The key names one class, so `newer` casts to it without a check.
  virtual bool merge([[maybe_unused]] const UndoRedoObject &newer) { return false; }

  // True only for the object the undo system groups an operation into, so it can walk one. Do not
  // override it: the undo system casts whatever answers true to that type.
  virtual bool is_operation_group() const { return false; }

  // called when this object is accepted in UndoSystem
  //   as a result of accept() or cancel()
  // Not once per object: one that folds away at put time never hears it, one that folds when its
  // operation is accepted hears it first and is dropped anyway, and one in a nested operation hears it
  // again from each accept around it. So do no work here that any of those would spoil.
  virtual void accepted() = 0;

  // for debugging
  virtual void get_description(String &) = 0;
};

// undo/redo system managing undo/redo for some database
// see create_undo_system()
class UndoSystem
{
public:
  UndoSystem() = default;
  UndoSystem(UndoSystem &&) = default;
  virtual ~UndoSystem() {}

  // call this method to start operation,
  //   then call accept(), or cancel() for one opened with can_cancel, to end it.
  // NOTE: operations can be nested
  // The default promises this operation will only ever be accepted, which lets what is put into it fold
  // into the operation around it as it arrives instead of waiting for the accept. An operation with a
  // path that reaches cancel() has to ask for can_cancel: cancel() on one without it asserts and then
  // closes it without reverting. A nested one holds what is already recorded in the operation around it;
  // a top level one has nothing outside it covering that, so it drops the record and leaves the change
  // applied.
  virtual void begin(bool can_cancel = false) = 0;

  // put object into current operation.
  // if no operation was started (is_holding() is false),
  //   the passed object will be canceled and destroyed.
  // For an object nobody can build in place: a factory result, or one filled in after it is
  // constructed. Anything built from its arguments goes through put<T>() below.
  virtual void put(UndoRedoObject *) = 0;

  // Same, for an object built from `args`. When T declares a merge key, it is built on the caller's stack
  // and reaches the heap only if the current operation has to keep it, so a repeat of what that operation
  // covers costs nothing. Without one, `args` go straight to the heap under an empty key, so nothing is
  // offered to the fold map here: a T that only inherits a key folds at its accept, not as it arrives.
  template <class T, class... Args>
  void put(Args &&...args)
  {
    static_assert(eastl::is_base_of_v<UndoRedoObject, T>, "put<T>() records an UndoRedoObject");
    static_assert(!eastl::is_abstract_v<T>, "put<T>() has to build a T, so it cannot be abstract");
    // T reaches the heap by move, or by copy when a user declared destructor left it no move constructor.
    // One owning a raw resource has to declare one, or the stack object and the heap copy both release it.

    // Only a key T declares itself is worth a probe; an inherited one belongs to the base. Declaring one
    // is on T: the fold at accept() has just a pointer, so nothing here can hold that for it.
    if constexpr (undo_declares_merge_key<T>())
    {
      T probe(eastl::forward<Args>(args)...);
      const UndoMergeKey key = probe.get_merge_key();
      if (!take_probe(probe, key))
        record_put(new T(eastl::move(probe)), key);
    }
    else
    {
      // Nothing it could fold into, so a stack copy would save nothing and only cost a move.
      record_put(new T(eastl::forward<Args>(args)...), UndoMergeKey());
    }
  }

  // accept current operation and leave database in its modified state
  // NOTE: operations can be nested
  virtual void accept(const char *operation_name) = 0;

  // cancel current operation and restore database
  //   to its state before last begin(), destroy UndoRedoObjects
  // NOTE: operations can be nested
  // Only for an operation opened with can_cancel. One opened without it is closed and not reverted, and
  // the assert that reports it is compiled out of release, so ask for can_cancel wherever this is reached.
  virtual void cancel() = 0;

  // returns true if system is saving undo data
  // (begin() was called without matching accept() or cancel())
  virtual bool is_holding() = 0;

  // How many operations are open. Whoever opens one records this straight after its begin() and closes
  // it only while the count is still at least that, which is_holding() cannot tell it: one opened
  // inside this one reads as holding too. It does not separate an operation of its own from one another
  // owner opened after a clear() dropped it, because both read the same count.
  virtual int open_operation_count() = 0;


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

  // Remove all undo/redo operations. What they changed stays applied, see set_save_point.
  virtual void clear() = 0;

  // Per-operation owner token. The undo system never dereferences it -- the owner
  // is opaque and interpreted by the caller (the editor app uses it as the active
  // plugin). The current owner is stamped onto every operation committed by
  // accept(); set_op_owner is normally called when the active plugin changes.
  // Default owner is NULL.
  virtual void set_op_owner(void *owner) = 0;

  // Owner of the operation that the next undo() / redo() would act on, or NULL when
  // there is no such operation. Lets the caller switch to the owning context before
  // applying the change.
  virtual void *get_undo_owner() = 0;
  virtual void *get_redo_owner() = 0;

  // Remove every top-level operation stamped with `owner` (both undo and redo sides),
  // adjusting the current position so the surviving operations keep their order. Used
  // to drop one owner's history without touching others -- e.g. a plugin reloading its
  // document. Drops that owner's save point too. No-op while an operation is being recorded
  // (is_holding()).
  virtual void remove_ops_by_owner(void *owner) = 0;

  // "Dirty" flag is set when undo system is modified in some way, or when setDirty() is called.
  virtual bool isDirty() const = 0;

  virtual void setDirty(bool dirty = true) = 0;

  // Call when `owner` saves its document, with no operation open. Records the newest applied
  // operation of `owner` that changes saved data (see UndoRedoObject::changes_saved_data)
  virtual void set_save_point(void *owner) = 0;

  // True while what `owner` holds is what it saved (undo/redo was triggered to the last save point),
  // and for an owner that never recorded anything.
  virtual bool is_at_save_point(void *owner) = 0;

protected:
  // put<T>() offers what it built here first, under the key it worked out for T. True means it is dealt
  // with and must not be kept: either no operation is open, so it was applied and reverted, or it folded
  // into one this operation covers.
  virtual bool take_probe(UndoRedoObject &probe, const UndoMergeKey &key) = 0;

  // Records an object put<T>() built on the heap, under the key it worked out for T. An empty key keeps
  // `o` out of this operation's fold map; the fold at accept() asks the object for its key instead, so a
  // key it inherited still folds there. Applies and reverts `o` when no operation is open, as the raw put
  // does.
  virtual void record_put(UndoRedoObject *o, const UndoMergeKey &key) = 0;
};


// Groups the per-change operations of one continuous edit -- a dragged slider, a held spin button, an
// open color picker -- into a single operation, so undo reverts the whole edit instead of its last
// change. The owner calls begin() on every change and end() on the matching change finish; `edit_id`
// tells one edit from the next and is whatever the owner uses to name what is being edited.
// An open gesture keeps the system it was begun on, so end() resolves none: a destructor can close it
// without reaching for an editor that may already be gone.
class UndoEditGesture
{
public:
  bool isOpenFor(int edit_id) const { return undoSystem && gestureId == edit_id; }

  // Opens a gesture, or keeps the one already open for `edit_id`. `operation_name` has to outlive the
  // gesture (a literal); it names the operation the matching end() accepts.
  void begin(UndoSystem &undo, int edit_id, const char *operation_name);

  // Safe to call with no gesture open. Returns false when it left the gesture open because an operation
  // opened inside it is not closed yet.
  bool end();

  // Closes only a gesture begun for `edit_id`, so a change finish closes the edit it belongs to and
  // leaves one another control opened in between alone. True when no gesture of that edit is open.
  bool end(int edit_id) { return isOpenFor(edit_id) ? end() : true; }

private:
  UndoSystem *undoSystem = nullptr; // the system the open gesture was begun on, null when none is open
  int gestureId = -1;
  int gestureDepth = 0;
  const char *gestureName = nullptr;
};


class IUndoRedoWndClient
{
public:
  virtual void updateUndoRedoMenu() = 0;
  virtual void onUndoRedo([[maybe_unused]] const char *nm, [[maybe_unused]] bool wasUndo) {}
};

// create UndoSystem object
UndoSystem *create_undo_system(const char *name, int max_size = 10 << 20, IUndoRedoWndClient *wnd = NULL);
