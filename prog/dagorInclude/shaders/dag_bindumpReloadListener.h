//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class IShaderBindumpReloadListener
{
  IShaderBindumpReloadListener *next = nullptr;

public:
  IShaderBindumpReloadListener(const char *tag_ = nullptr) : next(head), tag(tag_) { head = this; }
  IShaderBindumpReloadListener(const IShaderBindumpReloadListener &other) : next(head), tag(other.tag) { head = this; }
  IShaderBindumpReloadListener &operator=(const IShaderBindumpReloadListener &other)
  {
    tag = other.tag;
    return *this;
  }
  virtual ~IShaderBindumpReloadListener()
  {
    if (head == this) // if we are already in head, which is likely if destructor is called in reversed order
      head = next;
    else
      deleteFromLinkedList();
  }

  const char *getTag() const { return tag; }
  virtual void setEnabled(bool) {}

  static void resolveAll();

  template <typename Cb>
  static void forEach(Cb cb)
  {
    for (auto *i = head; i; i = i->next)
      cb(i);
  }

protected:
  virtual void resolve() = 0;

private:
  const char *tag = nullptr;
  static IShaderBindumpReloadListener *head;

  void deleteFromLinkedList();
};
