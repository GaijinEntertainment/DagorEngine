#pragma once

namespace das {

    class Context;
    struct LineInfo;

    #define DAS_SMALL_BUFFER_SIZE   4096

    class SmallBufferPolicy {
    public:
        string str() const {            // todo: replace via stringview
            DAS_VERIFY(size <= DAS_SMALL_BUFFER_SIZE);
            return string(data, size);
        }
        int tellp() const {
            return size;
        }
    protected:
        __forceinline void append(const char * s, int l) {
            if ( size + l <= DAS_SMALL_BUFFER_SIZE ) {
                memcpy ( data+size, s, l );
                size += l;
            } else {
                DAS_FATAL_ERROR("DAS_SMALL_BUFFER_SIZE overflow");
            }
        }
        __forceinline char * allocate (int l) {
            if ( size + l <= DAS_SMALL_BUFFER_SIZE ) {
                char * res = data + size;
                size += l;
                return res;
            } else {
                DAS_FATAL_ERROR("DAS_SMALL_BUFFER_SIZE overflow");
                return nullptr;
            }
        }
        __forceinline void output() {}
    protected:
        char    data[DAS_SMALL_BUFFER_SIZE];
        int32_t size = 0;
    };

    class VectorAllocationPolicy {
    public:
        virtual ~VectorAllocationPolicy() {}

        string str() const {            // todo: replace via stringview
            return string(data.data(), data.size());
        }
        int tellp() const {
            return int(data.size());
        }
    protected:
        __forceinline void append(const char * s, int l) {
            data.insert(data.end(), s, s + l);
        }
        __forceinline char * allocate (int l) {
            data.resize(data.size() + l);
            return data.data() + data.size() - l;
        }
        virtual void output() {}
        vector<char> data;
    };

    // todo: support hex
    struct StringWriterTag {};
    extern StringWriterTag HEX;
    extern StringWriterTag DEC;
    extern StringWriterTag FIXEDFP;
    extern StringWriterTag SCIENTIFIC;

    template <typename AllocationPolicy>
    class StringWriter : public AllocationPolicy {
    public:
        virtual ~StringWriter() {}
        template <typename TT>
        StringWriter & write(const char * format, TT value) {
            char buf[128];
            int realL = snprintf(buf, sizeof(buf), format, value);
            if ( auto at = this->allocate(realL) ) {
                memcpy(at, buf, realL);
                this->output();
            }
            return *this;
        }
        StringWriter & writeStr(const char * st, size_t len) {
            this->append(st, int(len));
            this->output();
            return *this;
        }
        StringWriter & writeChars(char ch, size_t len) {
            if ( auto at = this->allocate(int(len)) ) {
                memset(at, ch, len);
                this->output();
            }
            return *this;
        }
        StringWriter & write(const char * stst) {
            if ( stst ) {
                return writeStr(stst, strlen(stst));
            } else {
                return *this;
            }
        }
        StringWriter & operator << (const StringWriterTag & v ) {
            if (&v == &HEX) hex = true;
            else if (&v == &DEC) hex = false;
            else if (&v == &FIXEDFP) fixed = true;
            else if (&v == &SCIENTIFIC) fixed = false;
            return *this;
        }
        StringWriter & operator << (char v)                 { return write("%c", v); }
        StringWriter & operator << (unsigned char v)        { return write("%c", v); }
        StringWriter & operator << (bool v)                 { return write(v ? "true" : "false"); }
        StringWriter & operator << (int v)                  { return write(hex ? "%x" : "%d", v); }
        StringWriter & operator << (long v)                 { return write(hex ? "%lx" : "%ld", v); }
        StringWriter & operator << (long long v)            { return write(hex ? "%llx" : "%lld", v); }
        StringWriter & operator << (unsigned v)             { return write(hex ? "%x" : "%u", v); }
        StringWriter & operator << (unsigned long v)        { return write(hex ? "%lx" : "%lu", v); }
        StringWriter & operator << (unsigned long long v)   { return write(hex ? "%llx" : "%llu", v); }
        StringWriter & operator << (float v)                { return write(fixed ? "%.9f" : "%g", v); }
        StringWriter & operator << (double v)               { return write(fixed ? "%.17f" : "%g", v); }
        StringWriter & operator << (char * v)               { return write(v ? (const char*)v : ""); }
        StringWriter & operator << (const char * v)         { return write(v ? v : ""); }
        StringWriter & operator << (const string & v)       { return v.length() ? writeStr(v.c_str(), v.length()) : *this; }
    protected:
        bool hex = false;
        bool fixed = false;
    };

    typedef StringWriter<VectorAllocationPolicy> TextWriter;

    typedef StringWriter<SmallBufferPolicy> FixedBufferTextWriter;

    class TextPrinter : public TextWriter {
    public:
        virtual void output() override;
    protected:
        int pos = 0;
        static mutex pmut;
    };




    enum LogLevel {

        // CRITICAL - application can no longer perform its functions, or further execution will cause data corruption.
        // For example: array overrun, os_debug_break, assertion failure inside compiler.
        critical    = 50000,

        // ERROR - Error events of considerable importance that will prevent normal program execution,
        // but might still allow the application to continue running.
        // For example: an error when opening a file, wrong settings for texture conversion, assertion failure in the script.
        error       = 40000,

        // WARNING - the error is handled locally, something may cause unexpected behavior of the application.
        // For example: not all characters have been extracted from keystroke buffer, MSAA level was too high.
        warning     = 30000,

        // INFO - highlight the progress of the application.
        // General system start/stop messages, device configuration information.
        info        = 20000,

        // DEBUG - information useful for application developers. (DEBUG is defaut level)
        debug       = 10000,

        // TRACE - information useful for developers of subsystems, libraries, daScript etc.
        // For example: application activation/deactivation, key pressed, texture loaded from a file, sound file loaded.
        trace       = 0,
    };

    const char * getLogMarker(int level);
    void logger ( int level, const char *marker, const char * text, Context * context, LineInfo * at );

    class LOG : public TextWriter {
    public:
        LOG ( int level = LogLevel::debug ) : logLevel(level) {}
        virtual void output() override {
            int newPos = tellp();
            if (newPos != pos) {
                string st(data.data() + pos, newPos - pos);
                logger(logLevel, useMarker ? getLogMarker(logLevel) : "", st.c_str(), /*ctx*/nullptr, /*at*/nullptr);
                useMarker = false;
                data.clear();
                pos = newPos = 0;
            }
        }
    protected:
        int pos = 0;
        int logLevel = LogLevel::debug;
        bool useMarker = true;
    };
}
