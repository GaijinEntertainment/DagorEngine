#ifndef __YUPLAY2_ANSWER_IMPL__
#define __YUPLAY2_ANSWER_IMPL__
#pragma once


#include <yu_json.h>

#include <yuplay2_answer.h>


class YuApiAnswer :
    public IYuplay2Answer,
    public YuJson
{
public:
  bool decode(const YuCharTab& tab);
  bool decode(const YuString& str);

  void clear() { val.clear(); }

private:
  class AnswerValue : public IYuplay2AnswerValue
  {
  public:
    ~AnswerValue();

    void assign(const YuParserVal& v);

    int getType() const override;
    const char* getString() const override;
    int64_t getInt() const override { return val->i(); }
    float getFloat() const override { return val->f(); }
    bool getBool() const override { return val->b(); }
    IYuplay2AnswerValue* iterateKey(const char** key) const override;
    IYuplay2AnswerValue* iterateIdx(unsigned* idx) const override;

  private:
    const YuParserVal* val = NULL;

    mutable AnswerValue* iter = NULL;
    mutable YuParserVal::Dict::const_iterator dictIter = YuParserVal::Dict::const_iterator();
    mutable unsigned arrayIdx = -1;
    mutable YuString strBuf;
  };


  AnswerValue valIter;

  bool YU2VCALL empty() const override { return val.isNull(); }
  IYuplay2AnswerValue* iterateKey(const char** key) const override;
};


#endif //__YUPLAY2_ANSWER_IMPL__
