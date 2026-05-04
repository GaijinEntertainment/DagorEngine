#include "sqio.h"
#include <stdarg.h>
#include <assert.h>
#include <cstring>

typedef union {
  uint64_t v;
  uint8_t b[sizeof(uint64_t)];
} U64Conv;

uint64_t InputStream::readVaruint() {
  uint64_t v = 0;
  uint8_t t = 0;

  uint8_t s = 0;

  do {
    t = readByte();
    v |= uint64_t(t & 0x7F) << s;
    s += 7;
  } while (t & 0x80);

  return v;
}

int64_t InputStream::readVarint() {
  union {
    int64_t i;
    uint64_t u;
  } conv;

  conv.u = readVaruint();

  return conv.i;
}

int8_t InputStream::readInt8() {

  union {
    int8_t i;
    uint8_t u;
  } conv;

  conv.u = readByte();
  return conv.i;
}

int16_t InputStream::readInt16() {
  return (int16_t)readVarint();
}

int32_t InputStream::readInt32() {
  return (int32_t)readVarint();
}

int64_t InputStream::readInt64() {
  return (int64_t)readVarint();
}

intptr_t InputStream::readIntptr() {
  return readInt64();
}

uint8_t InputStream::readUInt8() {
  return readByte();
}

uint16_t InputStream::readUInt16() {
  return (uint16_t)readVaruint();
}

uint32_t InputStream::readUInt32() {
  return (uint32_t)readVaruint();
}

uint64_t InputStream::readUInt64() {
  return readVaruint();
}

uintptr_t InputStream::readUIntptr() {
  return readUInt64();
}

uint64_t InputStream::readRawUInt64() {
  U64Conv conv;

  for (int i = 0; i < sizeof conv.b; ++i) {
    conv.b[i] = readByte();
  }

  return conv.v;
}

// ---------------------------------------------

void OutputStream::writeVaruint(uint64_t v) {

  while (v > 0x7F) {
    writeByte(0x80 | uint8_t(v & 0x7F));
    v >>= 7;
  }

  writeByte(uint8_t(v));
}

void OutputStream::writeVarint(int64_t v) {
  union {
    int64_t i;
    uint64_t u;
  } conv;

  conv.i = v;

  writeVaruint(conv.u);
}

void OutputStream::writeInt8(int8_t v) {
  union {
    uint8_t u;
    int8_t i;
  } conv;

  conv.i = v;

  writeByte(conv.u);
}

void OutputStream::writeInt16(int16_t v) {
  writeVarint(v);
}

void OutputStream::writeInt32(int32_t v) {
  writeVarint(v);
}

void OutputStream::writeInt64(int64_t v) {
  writeVarint(v);
}

void OutputStream::writeIntptr(intptr_t v) {
  writeInt64(v);
}

void OutputStream::writeUInt8(uint8_t v) {
  writeByte(v);
}

void OutputStream::writeUInt16(uint16_t v) {
  writeVaruint(v);
}

void OutputStream::writeUInt32(uint32_t v) {
  writeVaruint(v);
}

void OutputStream::writeUInt64(uint64_t v) {
  writeVaruint(v);
}

void OutputStream::writeUIntptr(uintptr_t v) {
  writeUInt64(v);
}

void OutputStream::writeString(const char *s) {
  while (*s) {
    writeByte(*s++);
  }
}

void OutputStream::writeChar(char c) {
  writeInt8(c);
}

void OutputStream::writeRawUInt64(uint64_t v) {
  U64Conv conv;

  conv.v = v;

  for (int i = 0; i < sizeof conv.b; ++i) {
    writeByte(conv.b[i]);
  }
}

//-------------------------------------------------------

uint8_t StdInputStream::readByte() {
  int v = i.get();
  assert(v != EOF);
  return static_cast<uint8_t>(v);
}

size_t StdInputStream::pos() {
  return static_cast<size_t>(i.tellg());
}

void StdInputStream::seek(size_t p) {
  i.seekg(static_cast<std::streampos>(p));
}

FileInputStream::FileInputStream(const char *fileName) {
  file = fopen(fileName, "rb");
}

FileInputStream::~FileInputStream() {
  fclose(file);
}

uint8_t FileInputStream::readByte() {
  int v = fgetc(file);
  assert(v != EOF);
  return static_cast<uint8_t>(v);
}

size_t FileInputStream::pos() {
  return static_cast<size_t>(ftell(file));
}

void FileInputStream::seek(size_t p) {
  fseek(file, p, SEEK_SET);
}

uint8_t MemoryInputStream::readByte() {
  assert(ptr < size);
  return buffer[ptr++];
}

size_t MemoryInputStream::pos() {
  return ptr;
}

void MemoryInputStream::seek(size_t p) {
  assert(p <= size);
  ptr = p;
}

//-----------------------------------------------------------

void StdOutputStream::writeByte(uint8_t v) {
  o.put(v);
}

size_t StdOutputStream::pos() {
  return static_cast<size_t>(o.tellp());
}

void StdOutputStream::seek(size_t p) {
  o.seekp(static_cast<std::streampos>(p));
}

FileOutputStream::FileOutputStream(const char *fileName) {
  file = fopen(fileName, "wb");
  close = true;
}

FileOutputStream::~FileOutputStream() {
  if (close) {
    fclose(file);
  }
}

void FileOutputStream::writeByte(uint8_t v) {
  fputc(v & 0xFF, file);
}

size_t FileOutputStream::pos() {
  return static_cast<size_t>(ftell(file));
}

void FileOutputStream::seek(size_t p) {
  fseek(file, p, SEEK_SET);
}

MemoryOutputStream::~MemoryOutputStream() {
  if (_buffer)
    free(_buffer);
}

void MemoryOutputStream::resize(size_t n) {
  if (n < _size) return;

  uint8_t *newBuffer = (uint8_t*)malloc(n);
  assert(newBuffer);

  if (_buffer) {
    memcpy(newBuffer, _buffer, _size * sizeof(uint8_t)); //-V575
  }

  free(_buffer);
  _buffer = newBuffer;
  _size = n;
}

void MemoryOutputStream::writeByte(uint8_t v) {
  if (ptr >= _size) {
    resize((_size + 512) << 2);
  }

  _buffer[ptr++] = v;
}

size_t MemoryOutputStream::pos() {
  return ptr;
}

void MemoryOutputStream::seek(size_t p) {
  assert(p <= _size);
  ptr = p;
}