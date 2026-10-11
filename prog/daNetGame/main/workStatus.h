// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

//! Scope calls host callback set/unset with the same string pointer (HIS log-forwarder shape).
//! ``msg`` must outlive the Scope (typically a string literal).
namespace workstatus
{

using Callback = void(__cdecl *)(bool set, const char *text);

void set_callback(Callback cb);

struct Scope
{
  //! ``msg`` is stored by pointer; caller keeps it alive for the Scope lifetime.
  explicit Scope(const char *msg);
  ~Scope();
  Scope(const Scope &) = delete;
  Scope &operator=(const Scope &) = delete;

private:
  const char *ptr = nullptr;
};

} // namespace workstatus
