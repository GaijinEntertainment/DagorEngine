//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Bitarray - bit array with binary logic
// Bitarray a[(mem)];
// a.bitNot(); better than ~a;(which create a new copy of class)
// a.bitOr(b,c); better than a=b|c;(which create an unnesessary new copy of class)
// all bi-valental operations will work using the minimum of length of two operands (and make others unchanged in applying ops
// (|=,&=))!

// Operators and funcs
//  Logic bi-valental ops and funcs produces the result with size of maximum of two
//  Operators
//  a|b,a&b,a^c;a|=b;a&=b;a^=b; ~a
//  Funcs
//  bitOr(a) bitXor(a) bitAnd(a) <==> (|=,^=,&=)
//  bitOr(a,b) bitXor(a,b) bitAnd(a,b) <==> me=a|b,... but better - unsupported
//  bitNot() <==> a=~a; bitNot(a) <==> me=~a;

//  resize,reserve - just like those in Tab
//  No erase
//  append(int),cut(int)

// Get, set, reset
// get(i) <==> [i]
// set(i),reset(i)      - sets or reset i-th elem
// set(i,val)           - sets i-th elem to val
// set(),reset()        - sets, resets all array
// fill(from,to,val)    - sets all elements from <from> to <to>(inclusive) with val

// Shift - do not modify the length of array!
//  <<=,>>=,>>,<<,shl(),shr()
// shshr() - modifies the length of array!

/// @addtogroup common
/// @{

/** @file
  #Bitarray class
*/


#ifdef __cplusplus
#ifdef _DEBUG_TAB_
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#endif

#include <string.h>

extern "C"
{
#endif

#include <supp/dag_define_KRNLIMP.h>

#ifdef __cplusplus
}

// 32 bits long
typedef unsigned int Bitarraybits;

#define BitarraybitsBITS       32
#define BitarraybitsSHIFT      5
#define BitarraybitsSHIFTMASK  31
#define BitarraybitsbytesSHIFT 2

#define get_size_elems(a) (((a) + (BitarraybitsBITS - 1)) >> BitarraybitsSHIFT)
#define get_size_bytes(a) (get_size_elems(a) << BitarraybitsbytesSHIFT)

/** Bitarray - bit array with binary logic
 * It is better to use function style binary operations instead of operator style.
 * That is avoid constructions like this a=!(b|c) when possible. It is better to re-write as a.bitOr(b,c); a.bitNot(); to get more
 * perfomance.
 * @todo cyclic(rotation) shift operations.
 */
class Bitarray
{
protected:
  /// total in ints
  int total;

  /// used in bits
  int used;

  /// pointer to bits
  Bitarraybits *ptr;

  /// Memory allocator in use
  IMemAlloc *mem;

public:
  /**
   * Default constructor can take memory allocator.
   * @param m Memory allocator to use. Default is tmpmem (not tmpmem_ptr()!).
   */
  explicit Bitarray(IMemAlloc *m = tmpmem)
  {
    ptr = NULL;
    total = used = 0;
    mem = m;
    if (!m)
      mem = tmpmem_ptr();
  }

  /**
   * Copy constructor from int. Will have 32 bits size after constructing.
   * @param a default value.
   * @param m Memory allocator to use. Default is tmpmem (not tmpmem_ptr()!).
   */
  explicit Bitarray(int a, IMemAlloc *m = tmpmem)
  {
    ptr = NULL;
    total = used = 0;
    mem = m;
    if (!m)
      mem = tmpmem_ptr();
    if (resize(32))
      ptr[0] = a;
  }

  /**
   * init function. Clears all states. Warning! If any memory was allocated, it will be lost.
   * Use it only on undefined object, but it is vital to use init in such case.
   * @param m memory allocator to use.
   */
  void init(IMemAlloc *m)
  {
    ptr = NULL;
    total = used = 0;
    mem = m;
  }

  /// Destructor
  ~Bitarray()
  {
    if (ptr)
      mem->free(ptr);
  }

  /// relocate array to another memory allocator
  int setmem(IMemAlloc *m)
  {
    if (mem == m)
      return 1;
    if (!ptr)
    {
      mem = m;
      return 1;
    }
    Bitarraybits *np;
    size_t s;
    unsigned szb = get_size_bytes(used);
    np = (Bitarraybits *)m->alloc(szb, &s);
    if (!np)
      return 0;
    memcpy(np, ptr, szb);
    mem->free(ptr);
    mem = m;
    ptr = np;
    total = int(s / sizeof(Bitarraybits));
    return 1;
  }

  ///@return memory allocator in use.
  IMemAlloc *getmem() const { return mem; }

  /** Copy constructor will create complete copy of an array.
   * @param a source bit array.
   */
  Bitarray(const Bitarray &a)
  {
    mem = a.mem;
    size_t s;
    unsigned szb = get_size_bytes(a.used);
    total = used = 0;
    ptr = (Bitarraybits *)mem->alloc(szb, &s);
    used = a.used;
    total = int(s / sizeof(Bitarraybits));
    memcpy(ptr, a.ptr, szb);
  }

  /// Move constructor
  Bitarray(Bitarray &&a) : total(a.total), used(a.used), ptr(a.ptr), mem(a.mem) { a.ptr = NULL; }

  /** Operator = create complete copy of an array in current memory alloctor (memory alloctor will not change).
   * @param a source bit array.
   */
  Bitarray &operator=(const Bitarray &a)
  {
    unsigned oldszb = get_size_bytes(used);
    unsigned szb = get_size_bytes(a.used);
    if (oldszb == szb && ptr)
    {
      memcpy(ptr, a.ptr, szb);
      used = a.used;
      return *this;
    }
    if (ptr)
      mem->free(ptr);
    size_t s;
    total = used = 0;
    ptr = (Bitarraybits *)mem->alloc(szb, &s);
    if (!ptr)
      return *this;
    used = a.used;
    total = int(s / sizeof(Bitarraybits));
    memcpy(ptr, a.ptr, szb);
    return *this;
  }

  /// Move assignment operator
  Bitarray &operator=(Bitarray &&a)
  {
    if (this != &a)
    {
      if (ptr)
        mem->free(ptr);
      memcpy(this, &a, sizeof(*this));
      a.ptr = NULL;
    }
    return *this;
  }

  /** Operator = resizes array to 32 bits and fill them from argument.
   * @param a source integer.
   */
  Bitarray &operator=(int a)
  {
    if (resize(32))
      ptr[0] = a;
    return *this;
  }

  // Operators for accessing array elements

  /**
    @return 1 if i-ths bit is on. Otherwise returns zero. The same as Bitarray::get.
    * @param i bit to test.
  */
  int operator[](int i) const
  {
#ifdef _DEBUG_TAB_
    G_ASSERT(i >= 0 && i < used);
#endif
    return (ptr[i >> BitarraybitsSHIFT] >> (i & BitarraybitsSHIFTMASK)) & 1;
  }

  /**
    @return 1 if i-ths bit is on. Otherwise returns zero. The same as Bitarray::get.
    * @param i bit to test.
  */
  int get(int i) const
  {
#ifdef _DEBUG_TAB_
    G_ASSERT(i >= 0 && i < used);
#endif
    return (ptr[i >> BitarraybitsSHIFT] >> (i & BitarraybitsSHIFTMASK)) & 1;
  }

  /**
    fills all bits in segment [from,to] with 1 if v is not 0. Otherwise, fills with 0.
    @param from first bit to set
    @param to last bit to set
    @param v value to set with
  */
  void fill(int from, int to, int v) // inclusive
  {
    if (from > to)
    {
      int a = from;
      from = to;
      to = a;
    }
    ++to; // now exclusive
#ifdef _DEBUG_TAB_
    G_ASSERT(from >= 0 && from <= (total << BitarraybitsSHIFT) && to >= 0 && to <= (total << BitarraybitsSHIFT));
#endif
    if (v)
    {
      if (from & BitarraybitsSHIFTMASK) // head
      {
        int strt = from >> BitarraybitsSHIFT;
        int val = 0;
        for (int us = 1 << (from & BitarraybitsSHIFTMASK); from & BitarraybitsSHIFTMASK && from < to; us <<= 1, from++)
          val |= us;
        ptr[strt] |= val;
      }
      if (from != to && to & BitarraybitsSHIFTMASK) // tail
      {
        int strt = to >> BitarraybitsSHIFT;
        int val = 0;
        for (int us = 1 << ((to - 1) & BitarraybitsSHIFTMASK); to & BitarraybitsSHIFTMASK && to > from; us >>= 1, to--)
          val |= us;
        ptr[strt] |= val;
      }
      if (to != from)
      {
        to = (to - from + 1) >> (BitarraybitsSHIFT - BitarraybitsbytesSHIFT);
        from >>= BitarraybitsSHIFT;
        memset(ptr + from, 0xFF, to);
      }
    }
    else
    {
      if (from != to && from & BitarraybitsSHIFTMASK)
      { // head
        int strt = from >> BitarraybitsSHIFT;
        int val = 0;
        for (int us = 1 << (from & BitarraybitsSHIFTMASK); from & BitarraybitsSHIFTMASK && from < to; us <<= 1, from++)
          val |= us;
        ptr[strt] &= ~val;
      }
      if (to & BitarraybitsSHIFTMASK)
      { // tail
        int strt = to >> BitarraybitsSHIFT;
        int val = 0;
        for (int us = 1 << ((to - 1) & BitarraybitsSHIFTMASK); to & BitarraybitsSHIFTMASK && to > from; us >>= 1, to--)
          val |= us;
        ptr[strt] &= ~val;
      }
      if (to != from)
      {
        to = (to - from + 1) >> (BitarraybitsSHIFT - BitarraybitsbytesSHIFT);
        from >>= BitarraybitsSHIFT;
        memset(ptr + from, 0, to);
      }
    }
  }

  /// sets all array with 1
  void set()
  {
    if (used)
      fill(0, used - 1, 1);
  }

  /// sets all array with 0
  void reset() { memset(ptr, 0, get_size_bytes(used)); }

  /**sets i-th bit with 1
   *@param i bit to set.
   */
  void set(int i)
  {
#ifdef _DEBUG_TAB_
    G_ASSERT(i >= 0 && i < used);
#endif
    ptr[i >> BitarraybitsSHIFT] |= (1 << (i & BitarraybitsSHIFTMASK));
  }

  /** sets i-th bit with 0
   * @param i bit to set.
   */
  void reset(int i)
  {
#ifdef _DEBUG_TAB_
    G_ASSERT(i >= 0 && i < used);
#endif
    ptr[i >> BitarraybitsSHIFT] &= ~(1 << (i & BitarraybitsSHIFTMASK));
  }

  /** sets i-th bit with 1 if v is not zero. Otherwise, sets i-th bit with 0.
   * @param i bit to set with v.
   * @param v value to set in i-th bit.
   */
  void set(int i, unsigned v)
  {
    if (v)
      set(i);
    else
      reset(i);
  }

  /** fills n bits from at with v.
   * same as Bitarray::fill(at,at+n-1,v)
   */
  void set(int at, int n, unsigned v) { fill(at, at + n - 1, v); }
  /*
  // insert n element before at element of array
  // returns number of first added element or -1 at error
  int insert(int at,int n,const Bitarraybits *p=NULL,int step=8){
  }*/

  /**
   * Appends n bits into the end of array.
   * @param n bits to append
   * @return index of first added bit (Bitarray::count before adding). -1 on error.
   */
  int append(int n)
  {
    if (n <= 0)
    {
      return used - 1;
    }
    if ((used >> BitarraybitsSHIFT) == ((used + n) >> BitarraybitsSHIFT))
    {
      used = used + n;
      return used - 1;
    }
    if (!resize(used + n))
      return -1;
    return used - n;
  }

  /// @return ptr to buffer (serialization)
  const Bitarraybits *getPtr() const { return ptr; }

  /// copy ptr to array (deserializatio)
  /// @return false if there is no enough memory
  bool setPtr(const Bitarraybits *p, int sz)
  {
    if (!resize(sz))
      return false;
    memcpy(ptr, p, dataSize());
    return true;
  }

  /// @return ptr to buffer (deserialization)
  Bitarraybits *deserializePtr() const { return ptr; }


  /**
   * Erases n bits from end.
   * @param n bits to erase.
   */
  void cut(int n)
  {
    if (n <= 0)
    {
      return;
    }
    if ((used >> BitarraybitsSHIFT) == ((used + n) >> BitarraybitsSHIFT))
    {
      used = used - n;
      return;
    }
    resize(used - n);
  }
  /*// erase n elements starting from at
  // do not free memory - use shrink()
  void erase(int at,int n)
  {
  }
  */
  /// Clears all array, freeing memory.
  void clear(void)
  {
    if (ptr)
      mem->free(ptr);
    ptr = NULL;
    total = used = 0;
  }

  /// Erase all array, not freeing memory. The resulting count is 0.
  void eraseall() { used = 0; }

  /** reserve n elements
   * @return false if there is not enough memory.
   */
  bool reserve(int n)
  {
    if (n <= 0 || n + used < (total << BitarraybitsSHIFT))
      return true;
    size_t s = 0;
    Bitarraybits *p = (Bitarraybits *)mem->realloc(ptr, get_size_bytes(n + used), &s);
    if (p)
    {
      total = int(s / sizeof(Bitarraybits));
      ptr = p;
#ifdef _DEBUG_TAB_
      G_ASSERT(total >= used);
#endif
      fill(used, (total << BitarraybitsSHIFT) - 1, 0);
      return true;
    }
    return false;
  }

  /** resize bit array, preserving old memory contents.
   * That is, even if you enlarging array the first bits will be the same to old one.
   * @param c new bits count.
   * @return false if there is not enough memory.
   */
  bool resize(int c)
  {
    if ((total << BitarraybitsSHIFT) >= c)
    {
      if (used < c)
      {
        int ou = used;
        used = c;
        fill(ou, used - 1, 0);
        return true;
      }
      else if (used > c)
      {
        int ou = used;
        used = c;
        fill(used, ou - 1, 0);
        return true;
      }
      else
        return true;
    }
    size_t s = 0;
    Bitarraybits *p = (Bitarraybits *)mem->realloc(ptr, get_size_bytes(c), &s);
    if (p)
    {
      int ou = -1;
      if (used < c)
      {
        ou = used;
      }
      used = c;
      total = int(s / sizeof(Bitarraybits));
      ptr = p;
#ifdef _DEBUG_TAB_
      G_ASSERT((total << BitarraybitsSHIFT) >= used);
#endif
      if (ou >= 0)
      {
        fill(ou, (total << BitarraybitsSHIFT) - 1, 0);
        return true;
      }
      return true;
    }
    return false;
  }

  /// @return total count of bits of memory allocated
  unsigned capacity() const { return total * sizeof(Bitarraybits); }

  /// @return count of bits in use
  unsigned size() const { return used; }

  /// @return count of bytes in use
  int dataSize() const { return get_size_bytes(used); }

  /// Shrinks unused memory
  void shrink(void)
  {
    size_t s;
    ptr = (Bitarraybits *)mem->realloc(ptr, get_size_bytes(used), &s);
    total = int(s / sizeof(Bitarraybits));
  }

  // Logical operators and functions

  /// Not operator. It is preferenced to use x.bitNot() instead of x=!x, construction, because of avoiding useless memory operations.
  void bitNot()
  {
    for (int i = get_size_elems(used) - 1; i >= 0; --i)
      ptr[i] = ~ptr[i];
  }

  ///|= operator. The same as Bitarray::bitOr. Resizes array to maximum of sizes of arguments.
  const Bitarray &operator|=(const Bitarray &a)
  {
    bitOr(a);
    return *this;
  }

  ///&= operator. The same as Bitarray::bitAnd. Resizes array to maximum of sizes of arguments.
  const Bitarray &operator&=(const Bitarray &a)
  {
    bitAnd(a);
    return *this;
  }

  ///^= operator. The same as Bitarray::bitXor. Resizes array to maximum of sizes of arguments.
  const Bitarray &operator^=(const Bitarray &a)
  {
    bitXor(a);
    return *this;
  }

  /** Resizes array to maximum of sizes of arguments.
   * per bit | operation
   * @return false if there is not enough memory
   */
  bool bitOr(const Bitarray &a)
  {
    int sz1;
    if (used <= a.used)
    {
      if (used < a.used)
        if (!resize(a.used))
          return false;
      sz1 = used;
    }
    else
    {
      sz1 = a.used;
    }
    sz1 = get_size_elems(sz1) - 1;
    for (int i = sz1; i >= 0; --i)
      ptr[i] = ptr[i] | a.ptr[i];
    return true;
  }

  /** Resizes array to maximum of sizes of arguments.
   * per bit & operation
   * @return false if there is not enough memory
   */
  bool bitAnd(const Bitarray &a)
  {
    int sz1;
    if (used <= a.used)
    {
      if (used < a.used)
        if (!resize(a.used))
          return false;
      sz1 = a.used;
    }
    else
    {
      sz1 = a.used;
      fill(a.used, used - 1, 0);
    }
    sz1 = get_size_elems(sz1) - 1;

    for (int i = sz1; i >= 0; --i)
      ptr[i] = ptr[i] & a.ptr[i];
    return true;
  }

  /** Resizes array to maximum of sizes of arguments.
   * per bit ^ operation
   * @return false if there is not enough memory
   */
  bool bitXor(const Bitarray &a)
  {
    int sz1;
    if (used <= a.used)
    {
      if (used < a.used)
        if (!resize(a.used))
          return false;
      sz1 = a.used;
    }
    else
    {
      sz1 = a.used;
      fill(a.used, used - 1, 0);
    }
    sz1 = get_size_elems(sz1) - 1;
    for (int i = sz1; i >= 0; --i)
      ptr[i] = ptr[i] ^ a.ptr[i];
    return true;
  }

  /** Resizes array to maximum of (32,size()).
   * per bit | operation
   * @return false if there is not enough memory
   */
  bool bitOr(int a)
  {
    if (used < 32)
      if (!resize(32))
        return false;
    ptr[0] |= a;
    return true;
  }

  /** Resizes array to maximum of (32,size()).
   * per bit & operation
   * @return false if there is not enough memory
   */
  bool bitAnd(int a)
  {
    if (used < 32)
      if (!resize(32))
        return false;
    ptr[0] &= a;
    memset(ptr + 1, 0, get_size_bytes(used) - 1);
    return true;
  }

  /** Resizes array to maximum of (32,size()).
   * per bit ^ operation
   * @return false if there is not enough memory
   */
  bool bitXor(int a)
  {
    if (used < 32)
      if (!resize(32))
        return false;
    ptr[0] ^= a;
    return true;
  }

  const Bitarray &operator|=(int a)
  {
    bitOr(a);
    return *this;
  }
  const Bitarray &operator&=(int a)
  {
    bitAnd(a);
    return *this;
  }

  const Bitarray &operator^=(int a)
  {
    bitXor(a);
    return *this;
  }

  /// Sets array as per-bit !argument
  void bitNot(const Bitarray &a)
  {
    resize(a.size());
    for (int i = get_size_elems(used) - 1; i >= 0; --i)
      ptr[i] = ~a.ptr[i];
  }

  /// Per-bit not operator.
  Bitarray operator~()
  {
    Bitarray a(*this);
    a.bitNot();
    return a;
  }

  /// Per-bit or operator
  Bitarray operator|(const Bitarray &b)
  {
    Bitarray a(*this);
    a.bitOr(b);
    return a;
  }

  /// Per-bit and operator
  Bitarray operator&(const Bitarray &b)
  {
    Bitarray a(*this);
    a.bitAnd(b);
    return a;
  }

  /// Per-bit xor operator
  Bitarray operator^(const Bitarray &b)
  {
    Bitarray a(*this);
    a.bitXor(b);
    return a;
  }

  /// Per bit or operator. Array will have per-bit b|c value.
  bool bitOr(const Bitarray &a, const Bitarray &b)
  {
    if (!resize(a.used))
      return false;
    if (used)
      memmove(ptr, a.ptr, get_size_bytes(used));
    return bitOr(b);
  }

  /// Per bit and operator. Array will have per-bit b|c value.
  bool bitAnd(const Bitarray &a, const Bitarray &b)
  {
    if (!resize(a.used))
      return false;
    if (used)
      memmove(ptr, a.ptr, get_size_bytes(used));
    return bitAnd(b);
  }

  /// Per bit xor operator. Array will have per-bit b|c value.
  bool bitXor(const Bitarray &a, const Bitarray &b)
  {
    if (!resize(a.used))
      return false;
    if (used)
      memmove(ptr, a.ptr, get_size_bytes(used));
    return bitXor(b);
  }

  // Shift-operators do not reduce size of array, but set to 0 unused or new elements

  /// Per bit shift right operator. Array will have the same size as before. Undefined bits will set to 0.
  KRNLIMP void shr(unsigned a);

  /// Per bit shift left operator. Array will have the same size as before. Undefined bits will set to 0.
  KRNLIMP void shl(unsigned a);

  /// Per bit shift right operator. Array will have the same size as before. Undefined bits will set to 0.
  Bitarray operator>>(int a)
  {
    Bitarray r(*this);
    r >>= a;
    return r;
  }

  /// Per bit shift left operator. Array will have the same size as before. Undefined bits will set to 0.
  Bitarray operator<<(int a)
  {
    Bitarray r(*this);
    r <<= a;
    return r;
  }

  /// Per bit shift right operator. Array will have the same size as before. Undefined bits will set to 0.
  const Bitarray &operator>>=(int a)
  {
    shr(a);
    return *this;
  }

  /// Per bit shift left operator. Array will have the same size as before. Undefined bits will set to 0.
  const Bitarray &operator<<=(int a)
  {
    shl(a);
    return *this;
  }
  void shshr(int a)
  {
    shr(a);
    used -= a;
  }

  bool operator==(const Bitarray &a) const
  {
    if (used != a.used)
      return false;

    const int usedFoolBytes = used / 8;

    if (usedFoolBytes && memcmp(ptr, a.ptr, usedFoolBytes) != 0)
      return false;

    for (int i = usedFoolBytes * 8; i < used; ++i)
      if (bool(get(i)) != bool(a.get(i)))
        return false;

    return true;
  }

  bool operator!=(const Bitarray &a) const { return !(*this == a); }

  //@return number of bits that are on
  KRNLIMP unsigned numberset() const;
};


#ifndef __BITARRAYCPP__
#undef Bitarraybits
#undef BitarraybitsBITS
#undef BitarraybitsSHIFT
#undef BitarraybitsSHIFTMASK
#undef BitarraybitsbytesSHIFT

#undef get_size_elems
#undef get_size_bytes
#endif

#endif

#include <supp/dag_undef_KRNLIMP.h>

static inline unsigned data_size(const Bitarray &b) { return b.dataSize(); }

///@}
