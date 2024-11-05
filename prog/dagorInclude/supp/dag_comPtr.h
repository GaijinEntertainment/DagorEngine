//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_PC_WIN || _TARGET_XBOX

#include <Guiddef.h>
#include <unknwn.h>
#include <EASTL/type_traits.h>
#include <util/dag_globDef.h>

// copy paste (with some small changes) from wrl/client.h
// don't blame me for the stupid shit of this...
template <typename T> // T should be the ComPtr<T> or a derived type of it, not just the interface
class ComPtrRefBase
{
public:
  typedef typename T::InterfaceType InterfaceType;

  operator IUnknown **() const throw()
  {
    static_assert(__is_base_of(IUnknown, InterfaceType), "Invalid cast: InterfaceType does not "
                                                         "derive from IUnknown");
    return reinterpret_cast<IUnknown **>(ptr_->ReleaseAndGetAddressOf());
  }

protected:
  T *ptr_;
};

template <typename T>
class ComPtrRef : public ComPtrRefBase<T> // T should be the ComPtr<T> or a derived type of it, not
                                          // just the interface
{
  using Super = ComPtrRefBase<T>;
  using InterfaceType = typename Super::InterfaceType;

public:
  ComPtrRef(_In_opt_ T *ptr) throw() { this->ptr_ = ptr; }

  // Conversion operators
  operator void **() const throw() { return reinterpret_cast<void **>(this->ptr_->ReleaseAndGetAddressOf()); }

  // This is our operator ComPtr<U> (or the latest derived class from ComPtr (e.g. WeakRef))
  operator T *() throw()
  {
    *this->ptr_ = nullptr;
    return this->ptr_;
  }

  // We define operator InterfaceType**() here instead of on ComPtrRefBase<T>, since
  // if InterfaceType is IUnknown or IInspectable, having it on the base will collide.
  operator InterfaceType **() throw() { return this->ptr_->ReleaseAndGetAddressOf(); }

  // This is used for COM_ARGS in order to do __uuidof(**(ppType)).
  // It does not need to clear  ptr_ at this point, it is done at COM_ARGS_Helper(ComPtrRef&)
  // later in this file.
  InterfaceType *operator*() throw() { return this->ptr_->Get(); }

  // Explicit functions
  InterfaceType *const *GetAddressOf() const throw() { return this->ptr_->GetAddressOf(); }

  InterfaceType **ReleaseAndGetAddressOf() throw() { return this->ptr_->ReleaseAndGetAddressOf(); }
};
template <typename T>
class ComPtr
{
public:
  typedef T InterfaceType;

protected:
  InterfaceType *ptr_;
  template <class U>
  friend class ComPtr;

  void InternalAddRef() const throw()
  {
    if (ptr_ != nullptr)
    {
      ptr_->AddRef();
    }
  }

  unsigned long InternalRelease() throw()
  {
    unsigned long ref = 0;
    T *temp = ptr_;

    if (temp != nullptr)
    {
      ptr_ = nullptr;
      ref = temp->Release();
    }

    return ref;
  }

public:
#pragma region constructors
  ComPtr() throw() : ptr_(nullptr) {}

  ComPtr(decltype(__nullptr)) throw() : ptr_(nullptr) {}

  template <class U>
  ComPtr(_In_opt_ U *other) throw() : ptr_(other)
  {
    InternalAddRef();
  }

  ComPtr(const ComPtr &other) throw() : ptr_(other.ptr_) { InternalAddRef(); }

  // copy constructor that allows to instantiate class when U* is convertible to T*
  template <class U>
  ComPtr(const ComPtr<U> &other, typename eastl::enable_if<__is_convertible_to(U *, T *), void *>::type * = 0) throw() :
    ptr_(other.ptr_)
  {
    InternalAddRef();
  }

  ComPtr(_Inout_ ComPtr &&other) throw() : ptr_(nullptr)
  {
    if (this != reinterpret_cast<ComPtr *>(&reinterpret_cast<unsigned char &>(other)))
    {
      Swap(other);
    }
  }

  // Move constructor that allows instantiation of a class when U* is convertible to T*
  template <class U>
  ComPtr(_Inout_ ComPtr<U> &&other, typename eastl::enable_if<__is_convertible_to(U *, T *), void *>::type * = 0) throw() :
    ptr_(other.ptr_)
  {
    other.ptr_ = nullptr;
  }
#pragma endregion

#pragma region destructor
  ~ComPtr() throw() { InternalRelease(); }
#pragma endregion

#pragma region assignment
  ComPtr &operator=(decltype(__nullptr)) throw()
  {
    InternalRelease();
    return *this;
  }

  ComPtr &operator=(_In_opt_ T *other) throw()
  {
    if (ptr_ != other)
    {
      ComPtr(other).Swap(*this);
    }
    return *this;
  }

  template <typename U>
  ComPtr &operator=(_In_opt_ U *other) throw()
  {
    ComPtr(other).Swap(*this);
    return *this;
  }

  ComPtr &operator=(const ComPtr &other) throw()
  {
    if (ptr_ != other.ptr_)
    {
      ComPtr(other).Swap(*this);
    }
    return *this;
  }

  template <class U>
  ComPtr &operator=(const ComPtr<U> &other) throw()
  {
    ComPtr(other).Swap(*this);
    return *this;
  }

  ComPtr &operator=(_Inout_ ComPtr &&other) throw()
  {
    ComPtr(static_cast<ComPtr &&>(other)).Swap(*this);
    return *this;
  }

  template <class U>
  ComPtr &operator=(_Inout_ ComPtr<U> &&other) throw()
  {
    ComPtr(static_cast<ComPtr<U> &&>(other)).Swap(*this);
    return *this;
  }
#pragma endregion

#pragma region modifiers
  void Swap(_Inout_ ComPtr &&r) throw()
  {
    T *tmp = ptr_;
    ptr_ = r.ptr_;
    r.ptr_ = tmp;
  }

  void Swap(_Inout_ ComPtr &r) throw()
  {
    T *tmp = ptr_;
    ptr_ = r.ptr_;
    r.ptr_ = tmp;
  }
#pragma endregion

  explicit operator bool() const throw() { return Get() != nullptr; }

  T *Get() const throw() { return ptr_; }

  InterfaceType *operator->() const throw() { return ptr_; }

  ComPtrRef<ComPtr<T>> operator&() throw() { return ComPtrRef<ComPtr<T>>(this); }

  const ComPtrRef<const ComPtr<T>> operator&() const throw() { return ComPtrRef<const ComPtr<T>>(this); }

  T *const *GetAddressOf() const throw() { return &ptr_; }

  T **GetAddressOf() throw() { return &ptr_; }

  T **ReleaseAndGetAddressOf() throw()
  {
    InternalRelease();
    return &ptr_;
  }

  T *Detach() throw()
  {
    T *ptr = ptr_;
    ptr_ = nullptr;
    return ptr;
  }

  void Attach(_In_opt_ InterfaceType *other) throw()
  {
    if (ptr_ != nullptr)
    {
      auto ref = ptr_->Release();
      G_UNUSED(ref);
      // Attaching to the same object only works if duplicate references are being coalesced.
      // Otherwise re-attaching will cause the pointer to be released and may cause a crash on a
      // subsequent dereference.
      G_ASSERT(ref != 0 || ptr_ != other);
    }

    ptr_ = other;
  }

  unsigned long Reset() { return InternalRelease(); }

  // Previously, unsafe behavior could be triggered when 'this' is ComPtr<IInspectable> or
  // ComPtr<IUnknown> and CopyTo is used to copy to another type U. The user will use operator& to
  // convert the destination into a ComPtrRef, which can then implicit cast to IInspectable** and
  // IUnknown**. If this overload of CopyTo is not present, it will implicitly cast to IInspectable
  // or IUnknown and match CopyTo(InterfaceType**) instead. A valid polymoprhic downcast requires
  // run-time type checking via QueryInterface, so CopyTo(InterfaceType**) will break type safety.
  // This overload matches ComPtrRef before the implicit cast takes place, preventing the unsafe
  // downcast.
  template <typename U>
  HRESULT CopyTo(ComPtrRef<ComPtr<U>> ptr,
    typename eastl::enable_if<(eastl::is_same<T, IUnknown>::value) && !eastl::is_same<U *, T *>::value, void *>::type * = 0) const
    throw()
  {
    return ptr_->QueryInterface(__uuidof(U), ptr);
  }

  HRESULT CopyTo(_Outptr_result_maybenull_ InterfaceType **ptr) const throw()
  {
    InternalAddRef();
    *ptr = ptr_;
    return S_OK;
  }

  HRESULT CopyTo(REFIID riid, _Outptr_result_nullonfailure_ void **ptr) const throw() { return ptr_->QueryInterface(riid, ptr); }

  template <typename U>
  HRESULT CopyTo(_Outptr_result_nullonfailure_ U **ptr) const throw()
  {
    return ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void **>(ptr));
  }

  // query for U interface
  template <typename U>
  HRESULT As(_Inout_ ComPtrRef<ComPtr<U>> p) const throw()
  {
    return ptr_->QueryInterface(__uuidof(U), p);
  }

  // query for U interface
  template <typename U>
  HRESULT As(_Out_ ComPtr<U> *p) const throw()
  {
    return ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void **>(p->ReleaseAndGetAddressOf()));
  }

  // query for riid interface and return as IUnknown
  HRESULT AsIID(REFIID riid, _Out_ ComPtr<IUnknown> *p) const throw()
  {
    return ptr_->QueryInterface(riid, reinterpret_cast<void **>(p->ReleaseAndGetAddressOf()));
  }
  /*
      HRESULT AsWeak(_Out_ WeakRef* pWeakRef) const throw()
      {
          return ::Microsoft::WRL::AsWeak(ptr_, pWeakRef);
      }

  #if (NTDDI_VERSION >= NTDDI_WINBLUE)
      HRESULT AsAgile(_Out_ AgileRef* pAgile) const throw()
      {
          return ::Microsoft::WRL::AsAgile(ptr_, pAgile);
      }
  #endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
  */
}; // ComPtr

#define COM_ARGS(ppType) __uuidof(**(ppType)), COM_ARGS_Helper(ppType)

template <typename T>
void **COM_ARGS_Helper(_Inout_ ComPtrRef<T> pp) throw()
{
#if _TARGET_PC_WIN
  static_assert(__is_base_of(IUnknown, typename T::InterfaceType), "T has to derive from IUnknown");
#elif _TARGET_XBOX
  static_assert(__is_base_of(IGraphicsUnknown, typename T::InterfaceType), "T has to derive from IGraphicsUnknown");
#endif
  return pp;
}

template <typename T>
void **IID_PPV_ARGS_Helper(_Inout_ ComPtrRef<T> pp) throw()
{
  static_assert(__is_base_of(IUnknown, typename T::InterfaceType), "T has to derive from IUnknown");
  return pp;
}

#endif
