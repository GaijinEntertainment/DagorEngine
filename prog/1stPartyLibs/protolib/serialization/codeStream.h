#pragma once
#ifndef __GAIJIN_PROTOIO_CODESTREAM__
#define __GAIJIN_PROTOIO_CODESTREAM__

namespace proto
{
  namespace io
  {
    typedef unsigned int TNumber;
    struct StreamTag
    {
      enum Constants
      {
        TAG_BITS_COUNT = 3,
        TAG_MASK = (1 << 3) - 1,
        SHORT_NUMBER_LIMIT = 32
      };

      enum Type
      {
        EMPTY         = 0,
        EMPTY2        = 1,
        RAW32         = 2,
        VAR_INT       = 3,
        VAR_INT_MINUS = 4,
        VAR_LENGTH    = 5,
        BLOCK_BEGIN   = 6,
        SPECIAL       = 7,
      };

      enum Specials
      {
        SPECIAL_BLOCK_END = 1,
        TAG_BLOCK_END = SPECIAL | (SPECIAL_BLOCK_END << TAG_BITS_COUNT),
      };

      TNumber number;
      Type    type;

      bool isBlockEnded() const
      {
        return type == SPECIAL && number == SPECIAL_BLOCK_END;
      }
    };
  }
}

#endif // __GAIJIN_PROTOIO_CODESTREAM__
