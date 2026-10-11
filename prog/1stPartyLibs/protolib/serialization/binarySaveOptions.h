#pragma once
#ifndef __GAIJIN_PROTOIO_BINARYSAVEOPTIONS__
#define __GAIJIN_PROTOIO_BINARYSAVEOPTIONS__

namespace proto
{
  class BinarySaveOptions
  {
  public:
    explicit BinarySaveOptions(int number_, bool saveExtKey_ = true)
      : number(number_)
      , saveExtKey(saveExtKey_)
    {
    }


    int getNumber() const
    {
      return number;
    }

    bool needSaveExtKey() const
    {
      return saveExtKey;
    }
  private:
    int number;
    bool saveExtKey;
  };
}

#endif // __GAIJIN_PROTOIO_BINARYSAVEOPTIONS__
