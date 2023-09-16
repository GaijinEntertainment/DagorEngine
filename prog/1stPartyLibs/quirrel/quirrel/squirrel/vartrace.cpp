#include "sqpcheader.h"
#include <sqconfig.h>

#if SQ_VAR_TRACE_ENABLED == 1

#include "squtils.h"
#include "sqvm.h"
#include "vartrace.h"

#define VT_FLAG_STRING (1<<0)
#define VT_FLAG_ELLIPSIS (1<<1)


bool VarTrace::isStacksEqual(int a, int b)
{
  for (int i = 0; i < VAR_TRACE_STACK_DEPTH; i++)
  {
    if (history[a].stack[i].line == STACK_NOT_INITIALIZED && history[b].stack[i].line == STACK_NOT_INITIALIZED)
      return true;

    if (history[a].stack[i].line != history[b].stack[i].line ||
      history[a].stack[i].fileName != history[b].stack[i].fileName)
    {
      return false;
    }
  }

  return true;
}

void VarTrace::saveStack(const SQObject & value, const SQObject &vm_ref)
{
  // NULL vm may happen for objects created during SQSharedState initialization
  assert(sq_isthread(vm_ref) || sq_isnull(vm_ref));

  if (!sq_isthread(vm_ref))
    return;

  HSQUIRRELVM vm = static_cast<HSQUIRRELVM>(vm_ref._unVal.pRefCounted);
  saveStack(value, vm);
}


void VarTrace::saveStack(const SQObject & value, HSQUIRRELVM vm)
{
  if (!_ss(vm)->_varTraceEnabled)
    return;

  setCnt++;

  SQStackInfos si;
  SQInteger level = 0;

#if VAR_TRACE_SAVE_VALUES != 0
  history[pos].count = 1;
  const int valSize = sizeof(history[pos].val);
  history[pos].val[valSize - 2] = 0;

  SQObjectPtr obj(value);
  SQObjectPtr res;
  if (vm->ToString(obj, res))
  {
    const SQChar *valueAsString = sq_objtostring(&res);
    if (valueAsString)
    {
      strncpy(history[pos].val, valueAsString, valSize);

      history[pos].flags = (history[pos].val[valSize - 2] != 0 ? VT_FLAG_ELLIPSIS : 0) |
        (sq_isstring(value) ? VT_FLAG_STRING : 0);

      history[pos].val[valSize - 1] = 0;
    }
  }
#endif

  int count = 0;
  while (SQ_SUCCEEDED(sq_stackinfos(vm, level, &si)))
  {
    const SQChar *src = _SC("unknown");
    if (si.source)
      src = si.source;
    history[pos].stack[count].fileName = src;
    history[pos].stack[count].line = int(si.line);
    if (int(si.line) != -1)
      count++;

    if (count >= VAR_TRACE_STACK_DEPTH)
      break;

    level++;
  }

  if (count < VAR_TRACE_STACK_DEPTH)
    history[pos].stack[count].line = STACK_NOT_INITIALIZED;

  int prevPos = (pos - 1 + VAR_TRACE_STACK_DEPTH) % VAR_TRACE_STACK_DEPTH;

#if VAR_TRACE_SAVE_VALUES != 0
  if (memcmp(&history[pos].val, &history[prevPos].val, sizeof(history[prevPos].val)) == 0 &&
      isStacksEqual(pos, prevPos))
  {
    history[pos].stack[0].line = STACK_NOT_INITIALIZED;
    history[prevPos].count++;
  }
  else
  {
    pos++;
    if (pos >= VAR_TRACE_STACK_HISTORY)
      pos = 0;
  }
#else
  pos++;
  if (pos >= VAR_TRACE_STACK_HISTORY)
    pos = 0;
#endif
}

#define VT_MAX(a, b) ((a) > (b) ? (a) : (b))
#define VT_APRINTF(...) { written += scsprintf(buf + written, VT_MAX(size - written, 0), __VA_ARGS__); }

void VarTrace::printStack(SQChar * buf, int size)
{
  size--;
  int written = 0;
  *buf = 0;

  for (int h = 0; h < VAR_TRACE_STACK_HISTORY; h++)
  {
    int historyPos = (-h + pos + VAR_TRACE_STACK_HISTORY * 2 - 1) % VAR_TRACE_STACK_HISTORY;
    HistoryRecord & hist = history[historyPos];

    bool stackPresent = hist.stack[0].line != STACK_NOT_INITIALIZED;
    if (size > written)
      VT_APRINTF(_SC("%d:"), h);

#if VAR_TRACE_SAVE_VALUES != 0
    bool showCount = history[historyPos].count > 1 && stackPresent;

    if (showCount)
      VT_APRINTF(_SC(" x%d"), history[historyPos].count);

    if (stackPresent)
      VT_APRINTF((history[historyPos].flags & VT_FLAG_STRING) ? _SC(" value='%s%s'") : _SC(" value=%s%s"),
        history[historyPos].val,
        (history[historyPos].flags & VT_FLAG_ELLIPSIS) ? _SC("...") : _SC(""));

    VT_APRINTF(_SC("\n"));
#endif
    VT_APRINTF(_SC("\n"));

    for (int i = 0; i < VAR_TRACE_STACK_DEPTH; i++)
    {
      if (hist.stack[i].line == STACK_NOT_INITIALIZED)
        break;

      if (size > written)
        VT_APRINTF(_SC("  %s:%d\n"), hist.stack[i].fileName, hist.stack[i].line);
    }

    VT_APRINTF(_SC("\n"));
  }

  VT_APRINTF(_SC("set counter = %d\n"), setCnt);
}

#endif
