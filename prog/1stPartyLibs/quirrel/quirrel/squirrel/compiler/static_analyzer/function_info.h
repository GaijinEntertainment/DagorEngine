#pragma once

namespace SQCompilation
{

struct FunctionInfo {

  FunctionInfo(const FunctionExpr *d, const FunctionExpr *o) : declaration(d), owner(o) {}

  ~FunctionInfo() = default;

  struct Modifiable {
    const FunctionExpr *owner;
    const char *name;
  };

  const FunctionExpr *owner;
  std::vector<Modifiable> modifiable;
  const FunctionExpr *declaration;
  std::vector<const char *> parameters;

  void joinModifiable(const FunctionInfo *other);
  void addModifiable(const char *name, const FunctionExpr *o);

};

}
