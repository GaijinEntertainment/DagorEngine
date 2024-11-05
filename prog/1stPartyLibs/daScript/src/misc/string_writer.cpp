#include "daScript/misc/platform.h"
#include "daScript/misc/string_writer.h"
#include "misc/include_fmt.h"

namespace das {
    StringWriterTag HEX;
    StringWriterTag DEC;
    StringWriterTag FIXEDFP;
    StringWriterTag SCIENTIFIC;

    mutex TextPrinter::pmut;

    // string writer

    template <typename TT>
    StringWriter & StringWriter::format(const char * format, TT value) {
        char buf[128];
        auto tail = fmt::format_to(buf, fmt::runtime(format), value);
        auto realL = tail - buf;
        if ( auto at = this->allocate(int(realL)) ) {
            memcpy(at, buf, realL);
            this->output();
        }
        return *this;
    }

    StringWriter & StringWriter::writeStr(const char * st, size_t len) {
        this->append(st, int(len));
        this->output();
        return *this;
    }
    StringWriter & StringWriter::writeChars(char ch, size_t len) {
        if ( auto at = this->allocate(int(len)) ) {
            memset(at, ch, len);
            this->output();
        }
        return *this;
    }
    StringWriter & StringWriter::write(const char * stst) {
        if ( stst ) {
            return writeStr(stst, strlen(stst));
        } else {
            return *this;
        }
    }
    StringWriter & StringWriter::operator << (const StringWriterTag & v ) {
        if (&v == &HEX) hex = true;
        else if (&v == &DEC) hex = false;
        else if (&v == &FIXEDFP) fixed = true;
        else if (&v == &SCIENTIFIC) fixed = false;
        return *this;
    }
    StringWriter & StringWriter::operator << (char v)                 { return format("{}", v); }
    StringWriter & StringWriter::operator << (unsigned char v)        { return format("{}", v); }
    StringWriter & StringWriter::operator << (bool v)                 { return write(v ? "true" : "false"); }
    StringWriter & StringWriter::operator << (int v)                  { return hex ? format("{:x}", v) : format("{}", v); }
    StringWriter & StringWriter::operator << (long v)                 { return hex ? format("{:x}", v) : format("{}", v); }
    StringWriter & StringWriter::operator << (long long v)            { return hex ? format("{:x}", v) : format("{}", v); }
    StringWriter & StringWriter::operator << (unsigned v)             { return hex ? format("{:x}", v) : format("{}", v); }
    StringWriter & StringWriter::operator << (unsigned long v)        { return hex ? format("{:x}", v) : format("{}", v); }
    StringWriter & StringWriter::operator << (unsigned long long v)   { return hex ? format("{:x}", v) : format("{}", v); }
    StringWriter & StringWriter::operator << (char * v)               { return write(v ? (const char*)v : ""); }
    StringWriter & StringWriter::operator << (const char * v)         { return write(v ? v : ""); }
    StringWriter & StringWriter::operator << (const string & v)       { return v.length() ? writeStr(v.c_str(), v.length()) : *this; }
    StringWriter & StringWriter::operator << (float v)                { return fixed ? format("{:.9}", v) : format("{}", v); }
    StringWriter & StringWriter::operator << (double v)               { return fixed ? format("{:.17}", v) : format("{}", v); }

    // fixed buffer string writer

    string FixedBufferTextWriter::str() const {
        DAS_VERIFY(size <= DAS_SMALL_BUFFER_SIZE);
        return string(data, size);
    }

    uint64_t FixedBufferTextWriter::tellp() const {
        return uint64_t(size);
    }

    void FixedBufferTextWriter::append(const char * s, int l) {
        if ( size + l <= DAS_SMALL_BUFFER_SIZE ) {
            memcpy ( data+size, s, l );
            size += l;
        } else {
            DAS_FATAL_ERROR("DAS_SMALL_BUFFER_SIZE overflow");
        }
    }

    char * FixedBufferTextWriter::allocate (int l) {
        if ( size + l <= DAS_SMALL_BUFFER_SIZE ) {
            char * res = data + size;
            size += l;
            return res;
        } else {
            DAS_FATAL_ERROR("DAS_SMALL_BUFFER_SIZE overflow");
            return nullptr;
        }
    }

    void FixedBufferTextWriter::output() {
        // nothing to do
    }

    // text writer

    TextWriter::~TextWriter() {
        if ( largeBuffer != fixedBuffer ) {
            das_aligned_free16(largeBuffer);
        }
    }

    string TextWriter::str() const {            // todo: replace via stringview
        return string(largeBuffer, size);
    }

    uint64_t TextWriter::tellp() const {
        return uint64_t(size);
    }

    bool TextWriter::empty() const {
        return size == 0;
    }

    char * TextWriter::data() {
        return largeBuffer;
    }

    void TextWriter::clear() {
        size = 0;
    }

    void TextWriter::output() {
        // nothing to do
    }

    void TextWriter::append(const char * s, int l) {
        char * at = allocate(l);
        memcpy(at, s, l);
    }

    char * TextWriter::c_str() {
        if ( size < capacity ) {
            largeBuffer[size] = 0;
            return largeBuffer;
        } else {
            char * newBuffer = (char *) das_aligned_alloc16(size + 1);
            memcpy(newBuffer, largeBuffer, size);
            newBuffer[size] = 0;
            if ( largeBuffer != fixedBuffer ) {
                das_aligned_free16(largeBuffer);
            }
            largeBuffer = newBuffer;
            capacity = size + 1;
            return largeBuffer;
        }
    }

    char * TextWriter::allocate (int l) {
        // keep data in the fixed buffer, if it fits
        if ( size + l <= capacity ) {
            char * res = largeBuffer + size;
            size += l;
            return res;
        } else {
            // if it does not fit, allocate a new buffer
            int32_t newCapacity = capacity * 2;
            if ( newCapacity < size + l ) {
                newCapacity = size + l;
            }
            char * newBuffer = (char *) das_aligned_alloc16(newCapacity);
            if ( largeBuffer != fixedBuffer ) {
                memcpy(newBuffer, largeBuffer, size);
                das_aligned_free16(largeBuffer);
            } else {
                memcpy(newBuffer, fixedBuffer, size);
            }
            largeBuffer = newBuffer;
            capacity = newCapacity;
            char * res = largeBuffer + size;
            size += l;
            return res;
        }
    }

    // TextPrinter

    void TextPrinter::output() {
        lock_guard<mutex> guard(pmut);
        uint64_t newPos = tellp();
        if (newPos != pos) {
            string st(data() + pos, newPos - pos);
            printf("%s", st.c_str());
            fflush(stdout);
            pos = newPos;
        }
    }
}
