// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "semChecker.h"


/*********************************
 *
 * class SemanticChecker
 *
 *********************************/
// ctor/dtor
SemanticChecker::SemanticChecker(ShaderParser::ShaderEvalCB *new_owner) : lastError("no error"), errorTerm(NULL), owner(new_owner) {}

SemanticChecker::~SemanticChecker() {}


// set an error string
void SemanticChecker::setError(const char *err_string, BaseParNamespace::Terminal *err_term)
{
  errorTerm = err_term;

  if (!errorTerm || !errorTerm->text)
  {
    lastError = err_string;
    return;
  }

  lastError.printf(512, "%s - (%s)", err_string, errorTerm->text);
}
// class SemanticChecker
//
