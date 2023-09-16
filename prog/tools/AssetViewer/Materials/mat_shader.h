#pragma once


#include <generic/dag_tab.h>
#include <util/dag_string.h>


class DataBlock;


class MatShader
{
public:
  struct ParamRec
  {
    String name;
    bool isBinding;

    ParamRec() : isBinding(false) {}
  };

  MatShader(const DataBlock &blk);

  const char *getName() const { return name; }
  int getParamCount() const { return parameters.size(); }
  const ParamRec &getParam(int idx) const { return parameters[idx]; }

private:
  String name;
  Tab<ParamRec> parameters;
};
