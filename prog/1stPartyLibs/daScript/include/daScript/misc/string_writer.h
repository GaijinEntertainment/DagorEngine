#pragma once

namespace das {

    class Context;
    struct LineInfo;

    // todo: support hex
    struct StringWriterTag {};
    extern StringWriterTag HEX;
    extern StringWriterTag DEC;
    extern StringWriterTag FIXEDFP;
    extern StringWriterTag SCIENTIFIC;

    class StringWriter {
    public:
        virtual ~StringWriter() {}
        virtual string str() const = 0;
        virtual uint64_t tellp() const = 0;
        virtual char * allocate (int l) = 0;
        virtual void append(const char * s, int l) = 0;
        virtual void output() = 0;
    public:
        template <typename TT> StringWriter & format(const char * format, TT value);
        StringWriter & writeStr(const char * st, size_t len);
        StringWriter & writeChars(char ch, size_t len);
        StringWriter & write(const char * stst);
        StringWriter & operator << (const StringWriterTag & v );
        StringWriter & operator << (char v);
        StringWriter & operator << (unsigned char v);
        StringWriter & operator << (bool v);
        StringWriter & operator << (int v);
        StringWriter & operator << (long v);
        StringWriter & operator << (long long v);
        StringWriter & operator << (unsigned v);
        StringWriter & operator << (unsigned long v);
        StringWriter & operator << (unsigned long long v);
        StringWriter & operator << (char * v);
        StringWriter & operator << (const char * v);
        StringWriter & operator << (const string & v);
        StringWriter & operator << (float v);
        StringWriter & operator << (double v);
    protected:
        bool hex = false;
        bool fixed = false;
    };

    #ifndef DAS_SMALL_BUFFER_SIZE
    #define DAS_SMALL_BUFFER_SIZE   4096
    #endif

    class FixedBufferTextWriter : public StringWriter {
    public:
        virtual string str() const override;
        virtual uint64_t tellp() const override;
        virtual void append(const char * s, int l) override;
        virtual char * allocate (int l) override;
        virtual void output() override;
    protected:
        char    data[DAS_SMALL_BUFFER_SIZE];
        int32_t size = 0;
    };

    #ifndef DAS_STRING_BUILDER_BUFFER_SIZE
    #define DAS_STRING_BUILDER_BUFFER_SIZE   256
    #endif

    class TextWriter : public StringWriter {
    public:
        TextWriter() {}
        virtual ~TextWriter();
        virtual string str() const override;
        virtual uint64_t tellp() const override;
        bool empty() const;
        char * c_str();
        char * data();
        void clear();
        virtual void output() override;
    protected:
        virtual void append(const char * s, int l)  override;
        virtual char * allocate (int l) override;
    protected:
        char    fixedBuffer[DAS_STRING_BUILDER_BUFFER_SIZE];
        char *  largeBuffer = fixedBuffer;
        int32_t size = 0;
        int32_t capacity = DAS_STRING_BUILDER_BUFFER_SIZE;
    };

    class TextPrinter : public TextWriter {
    public:
        TextPrinter() {}
        virtual void output() override;
    protected:
        uint64_t pos = 0;
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
            auto newPos = tellp();
            if (newPos != pos) {
                string st(data() + pos, newPos - pos);
                logger(logLevel, useMarker ? getLogMarker(logLevel) : "", st.c_str(), /*ctx*/nullptr, /*at*/nullptr);
                useMarker = false;
                clear();
                pos = newPos = 0;
            }
        }
    protected:
        uint64_t pos = 0;
        int logLevel = LogLevel::debug;
        bool useMarker = true;
    };
}
