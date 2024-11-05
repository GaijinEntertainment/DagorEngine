// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
// #include <tabsort.h>
#include "str.h"
// #include "memory.h"
// #include "printf.h"
#include "debug.h"

String String::operator+(const String &s) const
{
  String ns(*this);
  if (!ns.Count())
  {
    if (!ns.Resize(1))
      return ns;
    ns.operator[](0) = 0;
  }
  int l = s.length();
  ns.insert(ns.Count() - 1, &s.operator[](0), l);
  return ns;
}

int String::insert(int at, const char *p, int n)
{
  if (!Count())
  {
    if (!Resize(1))
      return -1;
    // ptr[0]=0;
  }
  if (at > Count() - 1)
    at = Count() - 1;
  else if (at < 0)
    at = 0;
  if (n <= 0)
    return at;
  int res = insert(at, p, n);
  if (!p)
    if (res >= 0)
      for (int i = 0; i < n; ++i)
        operator[](res + i) = ' ';
  return res;
}

int String::insert(int at, const char *p)
{
  if (!Count())
  {
    if (!Resize(1))
      return -1;
    // ptr[0]=0;
  }
  if (at > Count() - 1)
    at = Count() - 1;
  else if (at < 0)
    at = 0;
  if (!p)
    return at;
  int l = (int)strlen(p);
  return insert(at, p, l);
}

int String::append(const char *p, int n)
{
  if (!Count())
  {
    if (!Resize(1))
      return -1;
    // ptr[0]=0;
  }
  if (n <= 0)
    return Count() - 1;
  int res = insert(Count() - 1, p, n);
  if (!p)
    if (res >= 0)
      for (int i = 0; i < n; ++i)
        operator[](res + i) = ' ';
  return res;
}
void String::cvtSlashes()
{
  for (int i = length() - 1; i >= 0; i--)
    if (operator[](i) == '\\')
      operator[](i) = '/';
}
void String::cvtSlashesRev()
{
  for (int i = length() - 1; i >= 0; i--)
    if (operator[](i) == '/')
      operator[](i) = '\\';
}

int String::append(const char *p)
{
  if (!Count())
  {
    if (!Resize(1))
      return -1;
    // ptr[0]=0;
  }
  if (!p)
    return Count() - 1;
  int l = (int)strlen(p);
  return insert(Count() - 1, p, l);
}

/*class StringPrintfOutput:public PrintfOutput{
public:
  String &str;
  int start,pos,num;

  StringPrintfOutput(String &s,int at):str(s) {start=pos=at;num=0;}
  void cleanup() {
    if(num) str.erase(start,num);
  }
  virtual int put_str(const char *s,int n) {
    if(str.insert(pos,n,s,1024)<0) {cleanup();return -1;}
    pos+=n;num+=n; return n;
  }
  virtual int fill_chr(char c,int n) {
    if(str.insert(pos,n,NULL,1024)<0) {cleanup();return -1;}
    memset(&str[pos],c,n);
    pos+=n;num+=n; return n;
  }
};*/

int String::vprintf(int sz, char *s, va_list ap)
{
  if (!Resize(1))
    return -1;
  // ptr[0]=0;
  // StringPrintfOutput po(*this,0);
  return 0; // VPrintf(po,s,ap);
  /*
    if(!resize(sz)) return -1;
    int c=_vsnprintf(ptr,sz,s,ap);
    used=c+1;
    shrink();
    return c;
  */
}

int String::printf(int sz, char *s, ...)
{
  va_list ap;
  va_start(ap, s);
  int c = vprintf(sz, s, ap);
  va_end(ap);
  return c;
}

int String::vprintf(int at, int sz, char *s, va_list ap)
{
  if (!Count())
  {
    if (!Resize(1))
      return -1;
    // ptr[0]=0;
  }
  // StringPrintfOutput po(*this,at);
  return 0; // VPrintf(po,s,ap);
  /*
    if(insert(at,sz)<0) return -1;
    int c=_vsnprintf(ptr+at,sz,s,ap);
    if(c<0) {erase(at,sz);return -1;}
    if(c<sz) erase(at+c,sz-c);
    return c;
  */
}

int String::printf(int at, int sz, char *s, ...)
{
  va_list ap;
  va_start(ap, s);
  int c = vprintf(at, sz, s, ap);
  va_end(ap);
  return c;
}

int String::vaprintf(int sz, char *s, va_list ap) { return vprintf(length(), sz, s, ap); }

int String::aprintf(int sz, char *s, ...)
{
  va_list ap;
  va_start(ap, s);
  int c = vprintf(length(), sz, s, ap);
  va_end(ap);
  return c;
}

/*
String::String(int sz,char *s,va_list ap):Tab<char>(tmpmem) {
  if(!resize(sz)) return;
  int c=_vsnprintf(ptr,sz,s,ap);
  used=c+1;
  shrink();
}
*/

/*
String::String(int sz,char *s,...):
    Tab<char>(tmpmem) {
  if(!resize(1)) return;
  ptr[0]=0;
  StringPrintfOutput po(*this,0);
  va_list ap;
  va_start(ap,s);
  VPrintf(po,s,ap);
  va_end(ap);
}
*/


//================================================================================
String &String::toLower()
{
  char *p = begin();
  for (int i = length(); --i >= 0; p++)
    if (*p >= 'A' && *p <= 'Z')
      *p += 'a' - 'A';
  return *this;
}


//================================================================================
String &String::toUpper()
{
  char *p = begin();
  for (int i = length(); --i >= 0; p++)
    if (*p >= 'a' && *p <= 'z')
      *p += 'A' - 'a';
  return *this;
}


//=======================================================================================
bool String::prefix(const char *pref) const
{
  const char *str = begin();

  if (!str)
    return false;

  while (*pref)
    if (*(str++) != *(pref++))
      return false;
  return true;
}


//=======================================================================================
bool String::suffix(const char *suff) const
{
  const char *str = begin();

  if (!str)
    return false;

  if (strlen(str) < strlen(suff))
    return false;

  return String(str + strlen(str) - strlen(suff)) == suff;
}


//==========================================================================================================
String &String::replace(const String &old_str, const String &new_str)
{
  String dst;
  const char *first = begin(), *last = begin(), *p = old_str;

  /*while (*last)
  {
    if (*p==0)
    {
      dst+=String(strlen(last)-old_str.length(), first);
      dst+=new_str;
      first=last;
      p=old_str;
    }
    else if (*last!=*p)
      p=old_str;
    else
      p++;
    last++;
  }

  if (*p==0)
  {
    dst+=String(first, last-old_str.length());
    dst+=new_str;
  }
  else
    dst+=String(first, last);*/

  return (*this) = dst;
}


//==========================================================================================================
const char *String::find(const char *simple, const char *pos, bool forward) const
{
  // G_ASSERT(forward==true);
  return strstr(pos ? pos : begin(), simple);
  /*
    if (!simple)
      return NULL;

    if (forward)
    {
      if (pos==NULL)
        pos=begin();

      const char  *first=pos,
                  *last=pos,
                  *p=simple;

      while (*p)
      {
        if (*last==0)
          return NULL;
        else if (*last!=*p)
        {
          first=last+1;
          p=simple;
        }
        else
          p++;
        last++;
      }
      return first;
    }
    else
      G_ASSERT(0);
    return NULL;*/
}


//==========================================================================================================
const char *String::find(char ch, const char *pos, bool forward) const
{
  if (forward)
  {
    if (pos == NULL)
      pos = begin();
    for (; pos < end(); pos++)
      if (*pos == ch)
        return pos;
  }
  else
  {
    if (pos == NULL)
      pos = end();
    for (; --pos >= begin();)
      if (*pos == ch)
        return pos;
  }
  return NULL;
}


/*int TABinsert(void*&ptr,int &total,int &used,IMemAlloc *mem,
int at,int n,int sz,const void *p,int STEP) {
  if(n<=0) return at;
  if(STEP<=0) STEP=1;
  if(at>used) at=used;
  int nn=used+n;
  if(nn>total) {
#ifdef DEBUG_TAB
    if ( STEP <= 0 )
      fatal("append with step=%d", STEP );
#endif
    nn = ((nn+STEP-1)/STEP)*STEP;

    unsigned asz;
    void *nt=mem->alloc(nn*sz,&asz);
    if(!nt) return -1;
    if(at) memcpy(nt,ptr,at*sz);
    if(p) memcpy((char*)nt+at*sz,p,n*sz);
    if(used-at) memcpy((char*)nt+(at+n)*sz,(char*)ptr+at*sz,(used-at)*sz);
    if(ptr) mem->free(ptr);
    ptr=nt;
    total=asz/sz;used+=n;
  }else{
    if(used-at) memmove((char*)ptr+(at+n)*sz,(char*)ptr+at*sz,(used-at)*sz);
    if(p) memcpy((char*)ptr+at*sz,p,n*sz);
    used+=n;
  }
  return at;
}

int TABSORTEDinsert(void*&ptr,int &total,int &used,IMemAlloc *mem,
int sz,void *e,int STEP,int (*sf) (const void*,const void*)) {
  if(!ptr || used<=0 || !sf || !e)
    return TABinsert(ptr,total,used,mem,used,1,sz,e,STEP);
  int a=0,b=used-1;
  char *p=(char*)ptr;
  if(sf(e,p)<=0) return TABinsert(ptr,total,used,mem,0,1,sz,e,STEP);
  if(sf(p+(used-1)*sz,e)<=0) return TABinsert(ptr,total,used,mem,used,1,sz,e,STEP);
  while(a<b) {
    int c=(a+b)/2;
    if(c==a) return TABinsert(ptr,total,used,mem,a+1,1,sz,e,STEP);
    int v=sf(e,p+c*sz);
    if(v==0) return TABinsert(ptr,total,used,mem,c,1,sz,e,STEP);
    else if(v<0) b=c; else a=c;
  }
  return TABinsert(ptr,total,used,mem,a+1,1,sz,e,STEP);
}

static void *simplesearch(const void *k,const void *ptr,int n,int w) {
  char* p=(char* )ptr;
  for(int i=0;i<n;++i,p+=w) if(memcmp(k,p,w)==0) return p;
  return NULL;
}

void *binsearch(const void *k,const void *ptr,int n,int w,
int (*f) (const void*,const void*)) {
  if(!ptr || !k || n<=0) return NULL;
  else if(!f) return simplesearch(k,ptr,n,w);
  int a=0,b=n-1;
  const char *p=(const char*)ptr;
  int v=f(k,p);
  if(v==0) return (void*)p; else if(v<0) return NULL;
  v=f(k,p+b*w);
  if(v==0) return (void*)(p+b*w); else if(v>0) return NULL;
  while(a<=b) {
    int c=(a+b)/2;
    if(c==a) {
      if(f(k,p+a*w)==0) return (void*)(p+a*w);
      return NULL;
    }
    v=f(k,p+c*w);
    if(v==0) return (void*)(p+c*w);
    else if(v<0) b=c; else a=c;
  }
  return NULL;
}

static int __sort_w;
static int (*__sort_f) (const void*,const void*);
static void *__sort_x=NULL,*__sort_y=NULL;
static int __sort_alloced=0;//half of real alloced mem

static void __sort(char *a, int n)
{
  if (n <= 1) return;

  #define SWAP(l, r) \
    memcpy(__sort_y, a + l * __sort_w, __sort_w); \
    memcpy(a + l * __sort_w, a + r * __sort_w, __sort_w); \
    memcpy(a + r * __sort_w, __sort_y, __sort_w);

  int l = 1, r = n;
  SWAP(0, (n>>1))

  const void *pivot=a;

  while (l < r)
  {
    if (__sort_f(a + l * __sort_w, pivot) <= 0)
      ++l;
    else
    {
      for (--r; l < r && __sort_f(a + r * __sort_w, pivot) >= 0; --r);

      SWAP(l, r)
    }
  }

  --l;
  SWAP(0, l)

  if (l <= 1)
    __sort(a + r * __sort_w, n - r);
  else if (n - r <= 1)
    __sort(a, l);
  else
  {
    __sort(a, l);
    __sort(a + r * __sort_w, n - r);
  }

//  __sort(a, l);
//  __sort(a + r * __sort_w, n-r);

  #undef SWAP
}


extern "C" void qusort(void *p,int n,int w,int (*f) (const void*,const void*)) {
  if(!p || n<2 || !w || !f) return;
  __sort_w=w;__sort_f=f;
  if(w>__sort_alloced){
    if(__sort_x)memfree(__sort_x,inimem);
    if(!(__sort_x=memalloc((__sort_alloced=w)<<1,inimem))){
      __sort_alloced=0;
    }
  }
  __sort_y=(char*)__sort_x+w;
  __sort((char*)p,n);
}

void qusort_init(){
  __sort_x=NULL;__sort_y=NULL;
  __sort_alloced=0;
}

void qusort_close(){
  if(__sort_x)memfree(__sort_x,inimem);
  __sort_x=NULL;__sort_y=NULL;
  __sort_alloced=0;
}*/
