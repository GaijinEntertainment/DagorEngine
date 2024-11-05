//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resMgr.h>

class SbufferIDPair
{
protected:
  Sbuffer *buf;
  D3DRESID bufId;

public:
  SbufferIDPair(Sbuffer *t = NULL, D3DRESID id = BAD_D3DRESID) : buf(t), bufId(id) {}

  D3DRESID getId() const { return bufId; }
  Sbuffer *getBuf() const { return buf; }
};

class SbufferIDHolder : public SbufferIDPair
{
public:
  SbufferIDHolder() = default;
  SbufferIDHolder(SbufferIDHolder &&rhs)
  {
    buf = rhs.buf;
    bufId = rhs.bufId;
    rhs.buf = NULL;
    rhs.bufId = BAD_D3DRESID;
  }
  ~SbufferIDHolder() { close(); }

  SbufferIDHolder &operator=(SbufferIDHolder &&rhs)
  {
    close();
    buf = rhs.buf;
    bufId = rhs.bufId;
    rhs.buf = NULL;
    rhs.bufId = BAD_D3DRESID;
    return *this;
  }

  void setRaw(Sbuffer *buf_, D3DRESID texId_)
  {
    buf = buf_;
    bufId = texId_;
  }
  void set(Sbuffer *buf_, D3DRESID texId_)
  {
    close();
    setRaw(buf_, texId_);
  }
  void set(Sbuffer *buf_, const char *name);
  void close();

private:
  SbufferIDHolder(const SbufferIDHolder &) = delete; // Avoid removing the copy of the texture.
};

class SbufferIDHolderWithVar : public SbufferIDPair
{
  int varId = -1;

public:
  SbufferIDHolderWithVar() = default;
  SbufferIDHolderWithVar(SbufferIDHolderWithVar &&rhs)
  {
    buf = rhs.buf;
    bufId = rhs.bufId;
    varId = rhs.varId;
    rhs.buf = NULL;
    rhs.bufId = BAD_D3DRESID;
    rhs.varId = -1;
  }
  ~SbufferIDHolderWithVar() { close(); }
  void setVarId(int id) { varId = id; }
  int getVarId() const { return varId; }

  SbufferIDHolderWithVar &operator=(SbufferIDHolderWithVar &&rhs)
  {
    close();
    buf = rhs.buf;
    bufId = rhs.bufId;
    varId = rhs.varId;
    rhs.buf = NULL;
    rhs.bufId = BAD_D3DRESID;
    rhs.varId = -1;
    return *this;
  }

  void setRaw(Sbuffer *buf_, D3DRESID texId_)
  {
    buf = buf_;
    bufId = texId_;
  }
  void set(Sbuffer *buf_, D3DRESID texId_)
  {
    close();
    setRaw(buf_, texId_);
  }
  void set(Sbuffer *buf_, const char *name);
  void setVar() const;
  void close();

private:
  SbufferIDHolderWithVar(const SbufferIDHolderWithVar &) = delete; // Avoid removing the copy of the texture.
};
