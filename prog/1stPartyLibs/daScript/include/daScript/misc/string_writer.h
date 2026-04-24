#pragma once

#include "daScript/misc/platform.h"
namespace das {

    class Context;
    struct LineInfo;

    // todo: support hex
    struct StringWriterTag {
        StringWriterTag() = default;
        StringWriterTag(const StringWriterTag&) = delete;
        StringWriterTag& operator=(const StringWriterTag&) = delete;
        StringWriterTag(StringWriterTag&&) = delete;
        StringWriterTag& operator=(StringWriterTag&&) = delete;
    };
    DAS_API extern StringWriterTag HEX;
    DAS_API extern StringWriterTag DEC;
    DAS_API extern StringWriterTag FIXEDFP;
    DAS_API extern StringWriterTag SCIENTIFIC;

    class DAS_API StringWriter {
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

    class DAS_API FixedBufferTextWriter : public StringWriter {
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

    class DAS_API TextWriter : public StringWriter {
    public:
        TextWriter() {}

        /*
         * Copy and move constructors are not supported
         * This helps prevent issues with `largeBuffer` ownership
         * and keeps the code cleaner and simpler
         */
        TextWriter(const TextWriter&) = delete;
        TextWriter(TextWriter&&) = delete;
        TextWriter& operator=(const TextWriter&) = delete;
        TextWriter& operator=(TextWriter&&) = delete;

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

    class DAS_API TextPrinter : public TextWriter {
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
        defaultPrint = debug,

        // TRACE - information useful for developers of subsystems, libraries, daslang etc.
        // For example: application activation/deactivation, key pressed, texture loaded from a file, sound file loaded.
        trace       = 0,
    };

    const char * getLogMarker(int level);

    class DAS_API LOG : public TextWriter {
    public:
        LOG ( int level = LogLevel::debug ) : logLevel(level) {}
        virtual void output() override;
    protected:
        uint64_t pos = 0;
        int logLevel = LogLevel::debug;
        bool useMarker = true;
    };
}
