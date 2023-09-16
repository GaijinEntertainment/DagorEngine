//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <osApiWrappers/dag_miscApi.h>
#include <supp/dag_define_COREIMP.h>
#include <EASTL/utility.h>

#define ACTION_DEBUG_NAMES 1


//! delayed action to be performed on next dagor_work_cycle()
class DelayedAction
{
public:
  virtual bool precondition() { return true; }
  virtual ~DelayedAction() {}

  virtual void performAction() = 0;
};

typedef void (*delayed_callback)(void *);

// Add delayed action object to global list.
// Actions added from another DelayedAction are executed on same cycle
KRNLIMP void add_delayed_action(DelayedAction *action, const char *debug_name = NULL, const char *dummy = NULL);

// Add delayed action object to global list.
// Actions added added from another DelayedAction are executed on next cycle
KRNLIMP void add_delayed_action_buffered(DelayedAction *action, const char *debug_name = NULL, const char *dummy = NULL);

// same as add_delayed_action() but without context
KRNLIMP void add_delayed_callback(delayed_callback cb, void *cb_arg, const char *debug_name = NULL, const char *dummy = NULL);

// same as add_delayed_action_buffered() but without context
KRNLIMP void add_delayed_callback_buffered(delayed_callback cb, void *cb_arg, const char *debug_name = NULL, const char *dummy = NULL);

// Execute delayed action in context of main thread;
// when called from back thread: waits for execution done
// when called from main thread and 'always_in_order'=false: executes action directly
// when called from main thread and 'always_in_order'=true:  executes action via global list (i.e. all preceeding actions will be
// executed first)
KRNLIMP void execute_delayed_action_on_main_thread(DelayedAction *action, bool always_in_order = false, int wait_quant_msec = 10);

// Remove delayed action object from global list. Returns false if passed pointer wasn't found and delete action otherwise.
KRNLIMP bool remove_delayed_action(DelayedAction *action);

// Perform delayed actions and remove them from the global list.
// Actions are performed in the same order they were added.
// This function is called from work_cycle().
KRNLIMP void perform_delayed_actions();

// sets maximum time that perform_delayed_actions() may spend on one work cycle; default is 4000000 (4 sec)
KRNLIMP void set_delayed_action_max_quota(int quota_usec);

KRNLIMP void register_regular_action_to_idle_cycle(delayed_callback cb, void *cb_arg);
KRNLIMP void unregister_regular_action_to_idle_cycle(delayed_callback cb, void *cb_arg);
KRNLIMP void perform_regular_actions_for_idle_cycle();

template <class F>
DelayedAction *make_delayed_action(F &&func)
{
  struct GenDelayedAction : public DelayedAction
  {
    GenDelayedAction(F &&func_) : func(eastl::forward<F>(func_)) {}
    virtual void performAction() { func(); }
    F func;
  };
  return new GenDelayedAction(eastl::forward<F>(func));
};

template <class F>
void delayed_call(F &&func)
{
  add_delayed_action(make_delayed_action(eastl::forward<F>(func)));
}

template <class F>
void run_action_on_main_thread(F &&func)
{
  if (is_main_thread())
    func();
  else
    delayed_call(eastl::forward<F>(func));
}

template <class F>
auto make_delayed_func(F func)
{
  return [func = eastl::move(func)](auto &&...args) {
    delayed_call([func = eastl::move(func), args...]() mutable { func(eastl::move(args)...); });
  };
}

#ifdef ACTION_DEBUG_NAMES
#define ACTION_DEBUG_NAMES_S1(x) #x
#define ACTION_DEBUG_NAMES_S2(x) ACTION_DEBUG_NAMES_S1(x)
#define ACTION_DEBUG_NAMES_LOC   " @\n\t" __FILE__ "(" ACTION_DEBUG_NAMES_S2(__LINE__) "):"

// Custom debug_name can be passed as an optional last arg. This is what the dummy parameter is for.
#define add_delayed_action(...)            add_delayed_action(__VA_ARGS__, "DelayedAction" ACTION_DEBUG_NAMES_LOC)
#define add_delayed_action_buffered(...)   add_delayed_action_buffered(__VA_ARGS__, "DelayedAction(buf)" ACTION_DEBUG_NAMES_LOC)
#define add_delayed_callback(...)          add_delayed_callback(__VA_ARGS__, "DelayedActionCB" ACTION_DEBUG_NAMES_LOC)
#define add_delayed_callback_buffered(...) add_delayed_callback_buffered(__VA_ARGS__, "DelayedActionCB(buf)" ACTION_DEBUG_NAMES_LOC)
#endif

#include <supp/dag_undef_COREIMP.h>
