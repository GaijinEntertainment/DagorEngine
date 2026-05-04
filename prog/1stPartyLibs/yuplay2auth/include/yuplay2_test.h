#ifndef __YUPLAY2_TEST__
#define __YUPLAY2_TEST__
#pragma once


class YuHttpTest
{
public:
  bool testSimpleGet();
  bool testGetParams();
  bool testHttpsGet();

  bool testSimplePost();
  bool testPostParams();
  bool testHttpsPost();
};


#endif //__YUPLAY2_TEST__
