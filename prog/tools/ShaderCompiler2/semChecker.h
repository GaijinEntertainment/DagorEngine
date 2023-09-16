/************************************************************************
  sematic checker for vprog & fshader - base class
/************************************************************************/
#ifndef __SEMCHECKER_H
#define __SEMCHECKER_H

#include "shsyn.h"

/************************************************************************/
/* forwards
/************************************************************************/
namespace ShaderParser
{
class ShaderEvalCB;
};

/*********************************
 *
 * class SemanticChecker
 *
 *********************************/
class SemanticChecker
{
public:
  // ctor/dtor
  SemanticChecker(ShaderParser::ShaderEvalCB *new_owner);
  virtual ~SemanticChecker();

  // set owner of the checker
  inline void setOwner(ShaderParser::ShaderEvalCB *new_owner) { owner = new_owner; };

  // retrun last error message
  inline const String &getLastError() const { return lastError; };

  // retrun error terminal symbol
  inline BaseParNamespace::Terminal *getErrorTerm() const { return errorTerm; };

  // set an error string
  void setError(const char *err_string, BaseParNamespace::Terminal *err_term);

  // return owner
  inline ShaderParser::ShaderEvalCB *getOwner() const { return owner; };

protected:
private:
  String lastError;
  BaseParNamespace::Terminal *errorTerm;
  ShaderParser::ShaderEvalCB *owner;

}; // class SemanticChecker
//

#endif //__SEMCHECKER_H
