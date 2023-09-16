#include <util/dag_globDef.h>
#include <libTools/util/undo.h>
#include <memory/dag_mem.h>


class UndoRedoHolder : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  Tab<UndoRedoObject *> obj;
  String name;

  UndoRedoHolder() : obj(midmem), name(strmem) {}

  ~UndoRedoHolder()
  {
    for (int i = 0; i < obj.size(); ++i)
      delete (obj[i]);
  }

  void put(UndoRedoObject *o)
  {
    if (!o)
      return;
    obj.push_back(o);
  }

  void clear_after(int p)
  {
    if (p >= obj.size())
      return;
    if (p < 0)
      p = 0;
    for (int i = p; i < obj.size(); ++i)
      delete (obj[i]);
    safe_erase_items(obj, p, obj.size() - p);
  }

  // called to undo changes to database
  // if save_redo_data is true, save data necessary to restore
  //   database to its current state in redo operation
  void restore(bool save)
  {
    for (int i = obj.size() - 1; i >= 0; --i)
      obj[i]->restore(save);
  }

  // redo undone changes
  void redo()
  {
    for (int i = 0; i < obj.size(); ++i)
      obj[i]->redo();
  }

  // get approximate size of this object (bytes)
  // used to keep undo data size under reasonable limit
  size_t size()
  {
    size_t sz = 0;
    for (int i = 0; i < obj.size(); ++i)
      sz += obj[i]->size();
    return sz;
  }

  // called when this object is accepted in UndoSystem
  //   as a result of accept() or cancel()
  void accepted()
  {
    for (int i = 0; i < obj.size(); ++i)
      obj[i]->accepted();
  }

  // for debugging
  void get_description(String &s) { s = name; }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class UndoSystemImpl : public UndoSystem
{
public:
  DAG_DECLARE_NEW(inimem)

  Tab<UndoRedoHolder *> stack;
  String name;
  int curop;
  int max_size;
  IUndoRedoWndClient *wnd;

  bool dirtyFlag;


  UndoSystemImpl(const char *nm, int sz, IUndoRedoWndClient *_wnd) :
    wnd(_wnd), stack(tmpmem), name(inimem), curop(0), max_size(sz), dirtyFlag(false)
  {
    name = nm;
    UndoRedoHolder *h = new UndoRedoHolder;
    stack.push_back(h);
  }

  ~UndoSystemImpl()
  {
    for (int i = stack.size() - 1; i >= 0; --i)
      delete (stack[i]);
  }

  void clear()
  {
    for (int i = stack.size() - 1; i > 0; --i)
      delete (stack[i]);
    safe_erase_items(stack, 1, stack.size() - 1);
    stack[0]->clear_after(0);
    curop = 0;
    if (wnd)
      wnd->updateUndoRedoMenu();
  }

  void set_max_size(int sz) { max_size = sz; }

  int get_max_size() { return max_size; }

  void check_size()
  {
    if (stack.size() != 1)
      return;
    UndoRedoHolder &h = *stack[0];

    // calc size
    size_t sz = 0;
    int i;
    for (i = 0; i < curop - 1; ++i)
      sz += h.obj[i]->size();

    // remove operations until size is ok
    for (i = 0; i < curop - 1 && sz > max_size; ++i)
      sz -= h.obj[i]->size();

    if (i <= 0)
      return;

    // really remove operations
    for (int j = 0; j < i; ++j)
      delete (h.obj[j]);
    erase_items(h.obj, 0, i);
  }

  // call this method to start operation,
  //   then call accept() or cancel() to end it.
  // NOTE: operations can be nested
  void begin()
  {
    UndoRedoHolder *h = new UndoRedoHolder;
    stack.push_back(h);

    // dirtyFlag=true;
  }

  // put object into current operation.
  // if no operation was started (is_holding() is false),
  //   the passed object will be canceled and destroyed.
  void put(UndoRedoObject *o)
  {
    if (!o)
      return;
    if (!is_holding())
    {
      o->accepted();
      o->restore(false);
      delete o;
      return;
    }
    (stack.back())->put(o);
  }

  // accept current operation and leave database in its modified state
  // NOTE: operations can be nested
  void accept(const char *nm)
  {
    if (!nm)
      nm = "(operation)";
    if (stack.size() <= 1)
      fatal("accept '%s' without begin in '%s'", nm, (char *)name);

    UndoRedoHolder *h = stack.back();

    if (!h->obj.size())
    {
      cancel();
      return;
    }

    dirtyFlag = true;

    h->name = nm;
    h->accepted();
    stack.pop_back();

    // clear redo ops if top level!
    if (stack.size() == 1)
      (stack.back())->clear_after(curop);
    (stack.back())->put(h);

    if (stack.size() == 1)
    {
      curop = (stack.back())->obj.size();
      check_size();
    }
    if (wnd)
      wnd->updateUndoRedoMenu();
  }

  // cancel current operation and restore database
  //   to its state before last begin(), destroy UndoRedoObjects
  // NOTE: operations can be nested
  void cancel()
  {
    if (stack.size() <= 1)
      fatal("cancel without begin in '%s'", (char *)name);
    UndoRedoHolder *h = stack.back();
    h->accepted();
    h->restore(false);
    stack.pop_back();
    delete h;
    if (wnd)
      wnd->updateUndoRedoMenu();
  }

  // returns true if system is saving undo data
  // (begin() was called without matching accept() or cancel())
  bool is_holding() { return stack.size() > 1; }


  bool can_undo()
  {
    if (is_holding())
      return false;
    UndoRedoHolder *h = stack.back();
    if (curop > h->obj.size())
      curop = h->obj.size();
    if (curop <= 0)
      return false;
    return true;
  }

  // undo last operation
  void undo()
  {
    if (!can_undo())
      return;
    UndoRedoHolder *h = stack.back();
    dirtyFlag = true;
    h->obj[--curop]->restore(true);
  }

  bool can_redo()
  {
    if (is_holding())
      return false;
    UndoRedoHolder *h = stack.back();
    if (curop >= h->obj.size())
      return false;
    if (curop < 0)
      curop = 0;
    return true;
  }

  // redo last undone operation
  void redo()
  {
    if (!can_redo())
      return;
    UndoRedoHolder *h = stack.back();
    dirtyFlag = true;
    h->obj[curop++]->redo();
  }

  // returns number of (top-level) operations that can be undone
  int undo_level()
  {
    if (is_holding())
      return 0;
    return curop;
  }

  // returns i-th undo operation name (0 is last operation,
  //    1 is operation before it, etc...), or NULL if no such operation
  const char *get_undo_name(int i)
  {
    if (is_holding())
      return NULL;
    if (i < 0)
      i = 0;
    UndoRedoHolder *h = stack.back();
    if (curop - 1 - i < 0)
      return NULL;
    return ((UndoRedoHolder *)h->obj[curop - 1 - i])->name;
  }

  // returns number of (top-level) operations that can be redone
  int redo_level()
  {
    if (is_holding())
      return 0;
    UndoRedoHolder *h = stack.back();
    return h->obj.size() - curop;
  }

  // returns i-th redo operation name (0 is current operation,
  //    1 is operation after it, etc...), or NULL if no such operation
  const char *get_redo_name(int i)
  {
    if (is_holding())
      return NULL;
    if (i < 0)
      i = 0;
    UndoRedoHolder *h = stack.back();
    if (curop + i >= h->obj.size())
      return NULL;
    return ((UndoRedoHolder *)h->obj[curop + i])->name;
  }


  virtual bool isDirty() const { return dirtyFlag; }

  virtual void setDirty(bool dirty) { dirtyFlag = dirty; }
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


UndoSystem *create_undo_system(const char *name, int maxsz, IUndoRedoWndClient *wnd) { return new UndoSystemImpl(name, maxsz, wnd); }
