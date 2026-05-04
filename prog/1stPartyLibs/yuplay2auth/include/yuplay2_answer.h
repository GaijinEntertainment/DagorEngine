#ifndef __YUPLAY2_ANSWER__
#define __YUPLAY2_ANSWER__
#pragma once


#include <yuplay2_def.h>


struct IYuplay2AnswerValue
{
  enum
  {
    VALUE_NULL,
    VALUE_STRING,
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_BOOL,
    VALUE_DICT,
    VALUE_ARRAY
  };

  virtual int getType() const = 0;

  virtual const char* getString() const = 0;
  virtual int64_t getInt() const = 0;
  virtual float getFloat() const = 0;
  virtual bool getBool() const = 0;

  virtual IYuplay2AnswerValue* iterateKey(const char** key) const = 0;
  virtual IYuplay2AnswerValue* iterateIdx(unsigned* idx) const = 0;
};


struct IYuplay2Answer
{
  virtual bool YU2VCALL empty() const = 0;

  virtual IYuplay2AnswerValue* iterateKey(const char** key) const = 0;
};


#endif //__YUPLAY2_ANSWER__
