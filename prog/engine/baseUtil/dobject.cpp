#include <generic/dag_DObject.h>
#include <generic/dag_tab.h>
#include <debug/dag_except.h>
#include <osApiWrappers/dag_stackHlp.h>

#include <debug/dag_debug.h>

#if _TARGET_PC
#define CHK_LOST_DOBJ
#endif


#define DOBJFLG_DEBUG 1


#ifdef CHK_LOST_DOBJ
static Tab<DObject *> dobj(inimem_ptr());


void check_lost_dobjects()
{
  int i, num, n;
  for (i = 0, num = 0; i < dobj.size(); ++i)
    if (dobj[i])
      ++num;
  if (num)
    debug("\n%d lost object%s!", num, num == 1 ? "" : "s");
  for (i = 0, n = 0; i < dobj.size(); ++i)
    if (dobj[i])
    {
      debug("%5d/%d: %s %p rc=%d", ++n, num, dobj[i]->class_name(), dobj[i], dobj[i]->getRefCount());
    }
}
#endif


#ifdef DEBUG_DOBJECTS
DObject::DObject()
{
  ref_count = 0;
  dobject_flags = 0;
#ifdef CHK_LOST_DOBJ
  dobj.setmem(inimem_ptr()); // to ensure it is initialized

  int i;
  for (i = 0; i < dobj.size(); ++i)
    if (!dobj[i])
      break;
  if (i >= dobj.size())
  {
    DObject *p = this;
    dobj.push_back(p);
  }
  else
    dobj[i] = this;
#endif
  // debug_ctx("DObject(%X)", this);
}


DObject::~DObject()
{
  if (dobject_flags & DOBJFLG_DEBUG)
    debug("DObject::dtr %X", this);
#ifdef CHK_LOST_DOBJ
  for (int i = 0; i < dobj.size(); ++i)
    if (dobj[i] == this)
    {
      dobj[i] = NULL;
      break;
    }
#endif
  if (ref_count != 0)
  {
    debug("Attempt to destroy DObject(%X) with ref_count=%d", this, ref_count);
    DAG_FATAL("Attempt to destroy DObject(%X) with ref_count=%d", this, ref_count);
  }
}


static String stackInfo()
{
  stackhelp::CallStackCaptureStore<32> stack;
  stackhelp::ext::CallStackCaptureStore extStack;
  stack.capture();
  extStack.capture();
  return get_call_stack_str(stack, extStack);
}


void DObject::addRef()
{
  if (dobject_flags & DOBJFLG_DEBUG)
    debug("ref %d++ %s %X\n%s", ref_count, class_name(), this, stackInfo());

  ++ref_count;
}


void DObject::delRef()
{
  if (ref_count == 0)
  {
    debug("ref_count already 0 %X", this);
    const char *cn = "<unknown>";
    DAGOR_TRY { cn = class_name(); }
    DAGOR_CATCH(DagorException) {}
    debug("ref_count already 0 %s %X", cn, this);
    DAG_FATAL("ref_count already 0 %s %X", cn, this);
  }

  if (dobject_flags & DOBJFLG_DEBUG)
    debug("ref %d-- %s %X\n%s", ref_count, class_name(), this, stackInfo());

  if (--ref_count <= 0)
  {
    if (dobject_flags & DOBJFLG_DEBUG)
      debug("delref0 %s %X", class_name(), this);
    delete this;
  }
}


void DObject::destroy() {}


void DObject::startdebug()
{
  if (!(dobject_flags & DOBJFLG_DEBUG))
    debug("startdebug %s %X", class_name(), this);
  dobject_flags |= DOBJFLG_DEBUG;
}


void DObject::stopdebug()
{
  if (dobject_flags & DOBJFLG_DEBUG)
    debug("stopdebug %s %X", class_name(), this);
  dobject_flags &= ~DOBJFLG_DEBUG;
}
#endif


const char *DObject::class_name() const { return "DObject"; }


bool DObject::isSubOf(DClassID id) { return id == DObjectCID; }


void *DObject::get_interface(int /*id*/) { return NULL; }

#define EXPORT_PULL dll_pull_baseutil_dobject
#include <supp/exportPull.h>
