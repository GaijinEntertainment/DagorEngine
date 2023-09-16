#pragma once


/************************************************************************
  variable names map for shaders
/************************************************************************/
namespace VarMap
{
static const int BAD_ID = -1;

// ctor/dtor
void init();
void release();
void clear();

// get ID by variable name - return VarMap::BAD_ID if fail
int getVarId(const char *var_name);

// add new id if not exists
int addVarId(const char *var_name);

int getIdCount();
const char *getName(int id);
}; // namespace VarMap
