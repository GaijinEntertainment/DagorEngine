#pragma once

#include <stdio.h>

class BatchLog
{
public:
  BatchLog() : fp(NULL) {}
  void open(const char *path);
  ~BatchLog();
  void write(const char *text);

private:
  FILE *fp;
};

class BatchLogCB
{
public:
  BatchLogCB(BatchLog *_bl);
  void write(const char *text);

private:
  BatchLog *bl;
};
