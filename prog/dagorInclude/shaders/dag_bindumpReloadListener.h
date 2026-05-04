//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class IShaderBindumpReloadListener
{
  IShaderBindumpReloadListener *next = nullptr;

public:
  IShaderBindumpReloadListener() : next(head) { head = this; }
  IShaderBindumpReloadListener(const IShaderBindumpReloadListener &) : next(head) { head = this; }
  IShaderBindumpReloadListener &operator=(const IShaderBindumpReloadListener &) { return *this; }
  virtual ~IShaderBindumpReloadListener()
  {
    if (head == this) // if we are already in head, which is likely if destructor is called in reversed order
      head = next;
    else
      deleteFromLinkedList();
  }

  static void resolveAll();
  static void reportStaticInitDone();

protected:
  virtual void resolve() = 0;
  static bool staticInitDone;

private:
  static IShaderBindumpReloadListener *head;

  void deleteFromLinkedList();
};
