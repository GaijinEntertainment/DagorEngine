#include "stdafx.h"
#include "yu_answer.h"


//==================================================================================================
bool YuApiAnswer::decode(const YuCharTab& tab)
{
  bool ret = YuJson::decode(tab);

  valIter.assign(val);
  return ret;
}


//==================================================================================================
bool YuApiAnswer::decode(const YuString& str)
{
  bool ret = YuJson::decode(str);

  valIter.assign(val);
  return ret;
}


//==================================================================================================
IYuplay2AnswerValue* YuApiAnswer::iterateKey(const char** key) const
{
  if (val.isDict())
    return valIter.iterateKey(key);

  if (key)
    *key = NULL;

  return NULL;
}





//==================================================================================================
// YuApiAnswer::AnswerValue
//==================================================================================================
YuApiAnswer::AnswerValue::~AnswerValue()
{
  if (iter)
    delete iter;
}


//==================================================================================================
void YuApiAnswer::AnswerValue::assign(const YuParserVal& v)
{
  val = &v;

  if (iter)
  {
    delete iter;
    iter = NULL;
  }
}


//==================================================================================================
int YuApiAnswer::AnswerValue::getType() const
{
  if (val->isString())
    return IYuplay2AnswerValue::VALUE_STRING;
  else if (val->isInt())
    return IYuplay2AnswerValue::VALUE_INT;
  else if (val->isFloat())
    return IYuplay2AnswerValue::VALUE_FLOAT;
  else if (val->isBool())
    return IYuplay2AnswerValue::VALUE_BOOL;
  else if (val->isDict())
    return IYuplay2AnswerValue::VALUE_DICT;
  else if (val->isArray())
    return IYuplay2AnswerValue::VALUE_ARRAY;

  return IYuplay2AnswerValue::VALUE_NULL;
}


//==================================================================================================
const char* YuApiAnswer::AnswerValue::getString() const
{
  strBuf = val->s();

  return strBuf.c_str();
}


//==================================================================================================
IYuplay2AnswerValue* YuApiAnswer::AnswerValue::iterateKey(const char** key) const
{
  if (val->isDict() && !val->d().empty())
  {
    if (!iter)
    {
      iter = new AnswerValue;
      dictIter = val->d().begin();
    }

    if (dictIter != val->d().end())
    {
      if (key)
        *key = dictIter->first.c_str();

      iter->assign(dictIter->second);
      ++dictIter;

      return iter;
    }
  }

  if (key)
    *key = NULL;

  return NULL;
}


//==================================================================================================
IYuplay2AnswerValue* YuApiAnswer::AnswerValue::iterateIdx(unsigned* idx) const
{
  if (val->isArray() && !val->a().empty())
  {
    if (!iter)
    {
      iter = new AnswerValue;
      arrayIdx = 0;
    }

    if (idx)
      *idx = arrayIdx;

    iter->assign(val->a().at(arrayIdx));
    ++arrayIdx;

    return iter;
  }

  if (idx)
    *idx = -1;

  return NULL;
}
