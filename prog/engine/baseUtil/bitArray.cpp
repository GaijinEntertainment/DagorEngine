// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <string.h>
#define __BITARRAYCPP__
#include <util/dag_bitArray.h>
// #include <debug/dag_debug.h>

unsigned Bitarray::numberset() const
{
  static char bitsperhex[] = "\0\1\1\2\1\2\2\3\1\2\2\3\2\3\3\4";
  unsigned val = 0;
  Bitarraybits *__restrict ptr_used = ptr, *__restrict ptr_end = ptr + get_size_elems(used);
  for (; ptr_used != ptr_end; ptr_used++)
    for (unsigned wordVal = *ptr_used; wordVal != 0; wordVal >>= 4)
      val += bitsperhex[wordVal & 0xF];
  return val;
}

void Bitarray::shr(unsigned a)
{ // shift right by _Pos
  const int wordshift = (a >> BitarraybitsSHIFT);
  if (wordshift != 0)
  {
    int words = get_size_elems(used);
    for (int wpos = 0; wpos < words; ++wpos) // shift by words
      ptr[wpos] = (wordshift < words - wpos) ? ptr[wpos + wordshift] : 0;
  }

  if (a %= BitarraybitsBITS)
  { // 0 < _Pos < _Bitsperword, shift by bits
    int words = get_size_elems(used);
    if (words)
    {
      for (int wpos = 0; wpos < words - 1; ++wpos)
        ptr[wpos] = ((ptr[wpos] >> a) | (ptr[wpos + 1] << (BitarraybitsBITS - a)));
      ptr[words - 1] >>= a;
    }
  }
}

/*
#pragma warning( default : 177 )
#pragma warning( default : 1011 )

void Bitarray::shr(int a){
  int sz=get_size_elems(used);
  if(a>=used){
    memset(ptr,0,sz<<BitarraybitsbytesSHIFT);
    return;
  }
  if(a>BitarraybitsSHIFTMASK) {
    int mv=(a>>BitarraybitsSHIFT);
    sz-=mv;
    memmove(ptr,ptr+mv,sz<<BitarraybitsbytesSHIFT);
    memset(ptr+sz,0,mv<<BitarraybitsbytesSHIFT);
  }
  int ma = a&BitarraybitsSHIFTMASK;
  if (ma){
    _asm{
      mov  ecx,ma
      mov  eax,sz
      test  eax,eax
      jz    calc_end
      dec eax

      mov  ebx,this
      mov  ebx,[ebx].ptr

      cycle_body:
      test  eax,eax
      jz    cycle_end

      mov   edx,DWORD PTR [ebx+4]

      shrd  DWORD PTR [ebx],edx,cl

      add   ebx,4
      dec   eax

      jmp   cycle_body
      cycle_end:

      shr  DWORD PTR [ebx],cl

      calc_end:
    }

  }
}

void Bitarray::shl(int a){
  int sz=get_size_elems(used);
  if(a>=used){
    memset(ptr,0,sz<<BitarraybitsbytesSHIFT);
    return;
  }
  if(a>BitarraybitsSHIFTMASK) {
    int mv=(a>>BitarraybitsSHIFT);
    memmove(ptr+mv,ptr,(sz-mv)<<BitarraybitsbytesSHIFT);
    memset(ptr,0,mv<<BitarraybitsbytesSHIFT);
  }
  int ma=a&BitarraybitsSHIFTMASK;
  if(ma){
    _asm{
      mov  eax,sz

      mov  ebx,eax
      dec  ebx
      shl  ebx,2
      mov  ecx,this
      add  ebx,[ecx].ptr

      test  eax,eax
      jz    calc_end
      dec eax

      mov  ecx,ma


      cycle_body:
      test  eax,eax
      jz    cycle_end

      mov   edx,DWORD PTR [ebx-4]

      shld  DWORD PTR [ebx],edx,cl

      sub   ebx,4
      dec   eax

      jmp   cycle_body
      cycle_end:

      shl  DWORD PTR [ebx],cl

      mov  edx,this
      mov  ecx,[edx].used
      and  ecx,1Fh
      test ecx,ecx
      jz   calc_end

      bts  eax,ecx
      dec  eax

      mov  edx,[edx].ptr
      mov  ecx,sz
      dec  ecx
      shl  ecx,2
      add  edx,ecx
      and  DWORD PTR [edx],eax


      calc_end:

    }
  }
}
*/

#define EXPORT_PULL dll_pull_baseutil_bitArray
#include <supp/exportPull.h>
