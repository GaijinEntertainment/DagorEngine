#ifndef _SQASTIO_H_
#define _SQASTIO_H_ 1

#include <stdint.h>
#include <iostream>
#include <stdio.h>

#include "squirrel.h"

class InputStream {
protected:
  virtual uint8_t readByte() = 0;

  int64_t readVarint();
  uint64_t readVaruint();
public:

  virtual ~InputStream() {}

  virtual size_t pos() = 0;
  virtual void seek(size_t) = 0;

  int8_t readInt8();
  int16_t readInt16();
  int32_t readInt32();
  int64_t readInt64();
  intptr_t readIntptr();

  uint8_t readUInt8();
  uint16_t readUInt16();
  uint32_t readUInt32();
  uint64_t readUInt64();
  uintptr_t readUIntptr();

  SQInteger readSQInteger() {
#ifdef _SQ64
    return readInt64();
#else
    return readIntptr();
#endif // _SQ64
  }

  SQUnsignedInteger readSQUnsignedInteger() {
#ifdef _SQ64
    return readUInt64();
#else
    return readUIntptr();
#endif // _SQ64
  }

  SQFloat readSQFloat() {

#ifdef SQUSEDOUBLE
    int32_t t = readInt32();
    return *(SQFloat *)&t;
#else
    int64_t t = readInt64();
    return *(SQFloat *)&t;
#endif // SQUSEDOUBLE
  }

  uint64_t readRawUInt64();
};

class StdInputStream : public InputStream {
  std::istream &i;
public:

  StdInputStream(std::istream &s) : i(s) {}

  uint8_t readByte();
  size_t pos();
  void seek(size_t);
};

class FileInputStream : public InputStream {
  FILE *file;
public:

  FileInputStream(const char *fileName);
  ~FileInputStream();

  bool valid() const { return file != NULL; }

  uint8_t readByte();
  size_t pos();
  void seek(size_t);
};

class MemoryInputStream : public InputStream {

  const uint8_t *buffer;
  const size_t size;
  size_t ptr;

public:

  MemoryInputStream(const uint8_t *b, const size_t s) : buffer(b), size(s), ptr(0) {}

  uint8_t readByte();
  size_t pos();
  void seek(size_t);
};


class OutputStream {
protected:
  virtual void writeByte(uint8_t) = 0;

  void writeVarint(int64_t);
  void writeVaruint(uint64_t);

public:

  virtual ~OutputStream() {}

  virtual size_t pos() = 0;
  virtual void seek(size_t pos) = 0;

  void writeInt8(int8_t);
  void writeInt16(int16_t);
  void writeInt32(int32_t);
  void writeInt64(int64_t);
  void writeIntptr(intptr_t);

  void writeUInt8(uint8_t);
  void writeUInt16(uint16_t);
  void writeUInt32(uint32_t);
  void writeUInt64(uint64_t);
  void writeUIntptr(uintptr_t);

  void writeSQInteger(SQInteger i) {
#ifdef _SQ64
    writeInt64(i);
#else
    writeIntptr(i);
#endif // _SQ64
  }

  void writeSQUnsignedInteger(SQUnsignedInteger u) {
#ifdef _SQ64
    writeUInt64(u);
#else
    writeUIntptr(u);
#endif // _SQ64
  }

  void writeSQFloat(SQFloat f) {
#ifdef SQUSEDOUBLE
    writeInt32(*((int32_t*)&f));
#else
    writeInt64(*((int64_t*)&f));
#endif // SQUSEDOUBLE
  }

  void writeString(const char *s);
  void writeChar(char c);

  void writeRawUInt64(uint64_t v);
};

class StdOutputStream : public OutputStream {
  std::ostream &o;
public:

  StdOutputStream(std::ostream &s) : o(s) {}

  void writeByte(uint8_t);
  size_t pos();
  void seek(size_t);
};

class FileOutputStream : public OutputStream {
  FILE *file;
  bool close;
public:

  FileOutputStream(FILE *f) : file(f), close(false) {}
  FileOutputStream(const char *filename);
  ~FileOutputStream();

  bool valid() const { return file != NULL; }

  void writeByte(uint8_t);
  size_t pos();
  void seek(size_t);
};

class MemoryOutputStream : public OutputStream {
  uint8_t *_buffer;
  size_t _size;
  size_t ptr;

  void resize(size_t);

public:

  MemoryOutputStream() : _buffer(NULL), _size(0), ptr(0) {}
  ~MemoryOutputStream();

  void writeByte(uint8_t);
  size_t pos();
  void seek(size_t);

  const uint8_t *buffer() const { return _buffer; }
};

#endif // !_SQASTIO_H_
