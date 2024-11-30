#include "daScript/misc/platform.h"

#include "daScript/simulate/runtime_string.h"
#include "daScript/simulate/simulate.h"
#include "daScript/simulate/hash.h"
#include "daScript/simulate/debug_print.h"
#include "daScript/simulate/runtime_string_delete.h"
#include "daScript/simulate/simulate_nodes.h"
#include "daScript/simulate/sim_policy.h"
#include "misc/include_fmt.h"

namespace das
{

    #if (!defined(DAS_ENABLE_EXCEPTIONS)) || (!DAS_ENABLE_EXCEPTIONS)

    DAS_THREAD_LOCAL jmp_buf * g_throwBuf = nullptr;
    DAS_THREAD_LOCAL string g_throwMsg;

    void das_throw(const char * msg) {
        if ( g_throwBuf ) {
            g_throwMsg = msg;
            longjmp(*g_throwBuf,1);
        } else {
            DAS_FATAL_ERROR("unhanded das_throw, %s\n", msg);
        }
    }

    void das_trycatch(callable<void()> tryBody, callable<void(const char * msg)> catchBody) {
        DAS_ASSERTF(g_throwBuf==nullptr, "das_trycatch without g_throwBuf");
        jmp_buf ev;
        g_throwBuf = &ev;
        if ( !setjmp(ev) ) {
            tryBody();
        } else {
            g_throwBuf = nullptr;
            catchBody(g_throwMsg.c_str());
        }
        g_throwBuf = nullptr;
    }
    #endif

    template <typename TT>
    __forceinline StringBuilderWriter & fmt_and_write_T ( StringBuilderWriter & writer, const char * fmt, TT value, Context * context, LineInfoArg * at ) {
        char ffmt[64] = "{";
        char buf[256];
        char * head = ffmt + 1;
        if ( fmt ) {
            char * tail = ffmt + 61;
            while ( head<tail && *fmt ) *head++ = *fmt++;
        }
        *head++ = '}'; *head = 0;
#if DAS_ENABLE_EXCEPTIONS
        try {
            auto result = fmt::format_to(buf, fmt::runtime(ffmt), value);
            *result = 0;
            writer.writeStr(buf, result - buf);
        } catch ( const std::exception & e ) {
            context->throw_error_at(at, "fmt error: %s", e.what());
        }
#else
        das_trycatch([&]{
            auto result = fmt::format_to(buf, fmt::runtime(ffmt), value);
            *result = 0;
            writer.writeStr(buf, result - buf);
        },[&](const char * e){
            context->throw_error_at(at, "fmt error: %s", e);
        });
#endif
        return writer;
    }

    StringBuilderWriter & fmt_and_write_i8 ( StringBuilderWriter & writer, const char * fmt, int8_t value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }
    StringBuilderWriter & fmt_and_write_u8 ( StringBuilderWriter & writer, const char * fmt, uint8_t value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }
    StringBuilderWriter & fmt_and_write_i16 ( StringBuilderWriter & writer, const char * fmt, int16_t value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }
    StringBuilderWriter & fmt_and_write_u16 ( StringBuilderWriter & writer, const char * fmt, uint16_t value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }
    StringBuilderWriter & fmt_and_write_i32 ( StringBuilderWriter & writer, const char * fmt, int32_t value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }
    StringBuilderWriter & fmt_and_write_u32 ( StringBuilderWriter & writer, const char * fmt, uint32_t value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }
    StringBuilderWriter & fmt_and_write_i64 ( StringBuilderWriter & writer, const char * fmt, int64_t value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }
    StringBuilderWriter & fmt_and_write_u64 ( StringBuilderWriter & writer, const char * fmt, uint64_t value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }
    StringBuilderWriter & fmt_and_write_f ( StringBuilderWriter & writer, const char * fmt, float value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }
    StringBuilderWriter & fmt_and_write_d ( StringBuilderWriter & writer, const char * fmt, double value, Context * context, LineInfoArg * at ) {
        return fmt_and_write_T(writer, fmt, value, context, at);
    }

    template <typename TT>
    __forceinline char * fmt_T ( const char * fmt, TT value, Context * context, LineInfoArg * at ) {
        char ffmt[64] = "{";
        char buf[256];
        char * head = ffmt + 1;
        if ( fmt ) {
            char * tail = ffmt + 61;
            while ( head<tail && *fmt ) *head++ = *fmt++;
        }
        *head++ = '}'; *head = 0;
#if DAS_ENABLE_EXCEPTIONS
        try {
            auto result = fmt::format_to(buf, fmt::runtime(ffmt), value);
            *result= 0;
            return context->allocateString(buf, uint32_t(result-buf), at);
        } catch (const std::exception & e) {
            context->throw_error_at(at, "fmt error: %s", e.what());
            return nullptr;
        }
#else
    char * return_value = nullptr;
    das_trycatch([&]{
        auto result = fmt::format_to(buf, fmt::runtime(ffmt), value);
        *result= 0;
        return_value = context->allocateString(buf, uint32_t(result-buf), at);
    },[&](const char * e){
        context->throw_error_at(at, "fmt error: %s", e);
    });
    return return_value;
#endif
    }

    char * fmt_i8 ( const char * fmt, int8_t value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }
    char * fmt_u8 ( const char * fmt, uint8_t value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }
    char * fmt_i16 ( const char * fmt, int16_t value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }
    char * fmt_u16 ( const char * fmt, uint16_t value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }
    char * fmt_i32 ( const char * fmt, int32_t value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }
    char * fmt_u32 ( const char * fmt, uint32_t value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }
    char * fmt_i64 ( const char * fmt, int64_t value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }
    char * fmt_u64 ( const char * fmt, uint64_t value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }
    char * fmt_f ( const char * fmt, float value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }
    char * fmt_d ( const char * fmt, double value, Context * context, LineInfoArg * at ) {
        return fmt_T(fmt, value, context, at);
    }

    template <typename TT>
    __forceinline char * das_lexical_cast_int_T ( TT x, bool hex, Context * __context__, LineInfoArg * at ) {
        char buffer[128];
        char * result;
        if ( hex ) {
            result = fmt::format_to(buffer,"{:#x}",x);
        } else {
            result = fmt::format_to(buffer,"{}",x);
        }
        *result = 0;
        return __context__->allocateString(buffer,uint32_t(result-buffer),at);
    }

    char * das_lexical_cast_int_i8 ( int8_t x, bool hex, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_int_T(x, hex, __context__, at);
    }
    char * das_lexical_cast_int_u8 ( uint8_t x, bool hex, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_int_T(x, hex, __context__, at);
    }
    char * das_lexical_cast_int_i16 ( int16_t x, bool hex, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_int_T(x, hex, __context__, at);
    }
    char * das_lexical_cast_int_u16 ( uint16_t x, bool hex, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_int_T(x, hex, __context__, at);
    }
    char * das_lexical_cast_int_i32 ( int32_t x, bool hex, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_int_T(x, hex, __context__, at);
    }
    char * das_lexical_cast_int_u32 ( uint32_t x, bool hex, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_int_T(x, hex, __context__, at);
    }
    char * das_lexical_cast_int_i64 ( int64_t x, bool hex, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_int_T(x, hex, __context__, at);
    }
    char * das_lexical_cast_int_u64 ( uint64_t x, bool hex, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_int_T(x, hex, __context__, at);
    }

    template <typename TT>
    __forceinline char * das_lexical_cast_fp_T ( TT x, Context * __context__, LineInfoArg * at ) {
        char buffer[128];
        auto result = fmt::format_to(buffer,"{}",x);
        *result = 0;
        return __context__->allocateString(buffer,uint32_t(result-buffer),at);
    }

    char * das_lexical_cast_fp_f ( float x, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_fp_T(x, __context__, at);
    }
    char * das_lexical_cast_fp_d ( double x, Context * __context__, LineInfoArg * at ) {
        return das_lexical_cast_fp_T(x, __context__, at);
    }

    // string operations

    vec4f SimPolicy_String::Add ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
        const char *  sA = to_rts(a);
        auto la = stringLength(context, sA);
        const char *  sB = to_rts(b);
        auto lb = stringLength(context, sB);
        uint32_t commonLength = la + lb;
        if ( !commonLength ) {
            return v_zero();
        } else if ( char * sAB = (char * ) context.allocateString(nullptr, commonLength, at) ) {
            memcpy ( sAB, sA, la );
            memcpy ( sAB+la, sB, lb+1 );
            context.stringHeap->recognize(sAB);
            return cast<char *>::from(sAB);
        } else {
            context.throw_out_of_memory(true, commonLength, at);
            return v_zero();
        }
    }

    void SimPolicy_String::SetAdd ( char * a, vec4f b, Context & context, LineInfo * at ) {
        char ** pA = (char **)a;
        const char *  sA = *pA ? *pA : rts_null;
        auto la = stringLength(context, sA);
        const char *  sB = to_rts(b);
        auto lb = stringLength(context, sB);
        uint32_t commonLength = la + lb;
        if ( !commonLength ) {
            // *pA = nullptr; is unnecessary, because its already nullptr
            return;
        } else if ( char * sAB = (char * ) context.allocateString(nullptr, commonLength, at) ) {
            memcpy ( sAB, sA, la );
            memcpy ( sAB+la, sB, lb+1 );
            *pA = sAB;
            context.stringHeap->recognize(sAB);
        } else {
            context.throw_out_of_memory(true, commonLength, at);
        }
    }

    // helper functions

    const char * rts_null = "";

    int hexChar ( char ch ) {
        if ( ch>='a' && ch<='f' ) {
            return ch - 'a' + 10;
        } else if ( ch>='A' && ch<='F' ) {
            return ch - 'A' + 10;
        } else if ( ch>='0' && ch<='9' ) {
            return ch - '0';
        } else {
            return -1;
        }
    }


    bool encodeUtf8Char(uint32_t ch, char * result) {

      if (ch <= 0x7F) {
          result[0] = char(ch);
          result[1] = 0;
          return true;
      }

      if (ch <= 0x7FF) {
          result[0] = char((ch >> 6) | 0xC0);
          result[1] = char((ch & 0x3F) | 0x80);
          result[2] = 0;
          return true;
      }

      if (ch <= 0xFFFF) {
          result[0] = char((ch >> 12) | 0xE0);
          result[1] = char(((ch >> 6) & 0x3F) | 0x80);
          result[2] = char(((ch >> 0) & 0x3F) | 0x80);
          result[3] = 0;
          return true;
      }

      if (ch <= 0x1FFFFF) {
          result[0] = char((ch >> 18) | 0xF0);
          result[1] = char(((ch >> 12) & 0x3F) | 0x80);
          result[2] = char(((ch >>  6) & 0x3F) | 0x80);
          result[3] = char(((ch >>  0) & 0x3F) | 0x80);
          result[4] = 0;
          return true;
      }

      if (ch <= 0x3FFFFFF) {
          result[0] = char((ch >> 24) | 0xF8);
          result[1] = char(((ch >> 18) & 0x3F) | 0x80);
          result[2] = char(((ch >> 12) & 0x3F) | 0x80);
          result[3] = char(((ch >>  6) & 0x3F) | 0x80);
          result[4] = char(((ch >>  0) & 0x3F) | 0x80);
          result[5] = 0;
          return true;
      }

      if (ch <= 0x7FFFFFFF) {
          result[0] = char((ch >> 30) | 0xFC);
          result[1] = char(((ch >> 24) & 0x3F) | 0x80);
          result[2] = char(((ch >> 18) & 0x3F) | 0x80);
          result[3] = char(((ch >> 12) & 0x3F) | 0x80);
          result[4] = char(((ch >>  6) & 0x3F) | 0x80);
          result[5] = char(((ch >>  0) & 0x3F) | 0x80);
          result[6] = 0;
          return true;
      }

      result[0] = '?';
      result[1] = 0;

      return false;
    }

    string unescapeString ( const string & input, bool * error, bool ) {
        if ( error ) *error = false;
        const char* str = input.c_str();
        const char* strEnd = str + input.length();
        string result;
        result.reserve(input.size());
        for( ; str < strEnd; ++str ) {
            if ( *str=='\\' ) {
                ++str;
                if ( str == strEnd ) {
                    if ( error ) *error = true;
                    return result; // invalid escape sequence
                }
                switch ( *str ) {
                    case '"':
                    case '/':
                    case '\\':  result += *str;    break;
                    case 'b':   result += '\b';    break;
                    case 'f':   result += '\f';    break;
                    case 'n':   result += '\n';    break;
                    case 'r':   result += '\r';    break;
                    case 't':   result += '\t';    break;
                    case 'v':   result += '\v';    break;
                    case '\n':  break;  // skip LF
                    case '{':   result += '{';    break;
                    case '}':   result += '}';    break;
                    case '\r':  if ( str+1!=strEnd && str[1]=='\n' ) str++; break;  // skip CR LF or just CR
                    case 'x':
                    case 'u':
                    case 'U': {
                            int symbols = (*str == 'x') ? 2 : (*str == 'u') ? 4 : 8;
                            uint32_t charCode = 0;
                            int x = 0;
                            str++;
                            for (; x < symbols && str != strEnd; x++, str++) {
                                int h = hexChar(*str);
                                if (h < 0)
                                    break;
                                charCode = charCode * 16 + h;
                            }
                            str--;

                            if (symbols == 2) { // \xNN
                                if (!x) {
                                    if (error) *error = true;
                                }
                                else
                                    result += char(charCode);
                            }
                            else if (x != symbols) { // \u \U requires exactly 4 or 8 hex symbols
                                if (error) *error = true;
                            }
                            else {
                                char buf[8];
                                if (!encodeUtf8Char(charCode, buf) && error)
                                    *error = true;
                                result += buf;
                            }
                        }
                        break;
                    default:    result += *str; if ( error ) *error = true; break;  // invalid escape character
                }
            } else
                result += *str;
        }
        return result;
    }

    string escapeString ( const string & input, bool das_escape ) {
        const char* str = input.c_str();
        const char* strEnd = str + input.length();
        string result;
        result.reserve(input.size());
        for( ; str < strEnd; ++str ) {
            auto ch = uint8_t(*str);
            switch ( ch ) {
                case '\"':  result.append("\\\"");  break;
                case '\\':  result.append("\\\\");  break;
                case '\b':  result.append("\\b");   break;
                case '\v':  result.append("\\v");   break;
                case '\f':  result.append("\\f");   break;
                case '\n':  result.append("\\n");   break;
                case '\r':  result.append("\\r");   break;
                case '\t':  result.append("\\t");   break;
                case '{':   if (das_escape) result.append("\\{"); else result.append("{");  break;
                case '}':   if (das_escape) result.append("\\}"); else result.append("}");  break;
                default:
                    if ( ch <= 0x1f ) {
                        result.append("\\u00");
                        result.append(1,(ch>>4)+'0');
                        result.append(1,(ch&15)+'0');
                    } else {
                        result.append(1, ch);
                    }
                    break;
            }
        }
        return result;
    }

    string getFewLines ( const char* st, uint32_t stlen, int ROW, int COL, int LROW, int LCOL, int TAB ) {
        TextWriter text;
        int col=0, row=1;
        auto it = st;
        auto itend = st + stlen;
        if ( ROW>1 ) {
            while ( *it && it!=itend ) {
                auto CH = *it++;
                if ( CH=='\n' ) {
                    row++;
                    col=0;
                    if ( row==ROW ) break;
                }
            }
        }
        if ( row!=ROW ) return "";
        auto beginOfLine = it;
        for (;;) {
            if (*it == 0 || it == itend)
            {
                text << "\n";
                break;
            }
            auto CH = *it++;
            if ( CH=='\t' ) {
                int tcol = (col + TAB) & ~(TAB-1);
                while ( col < tcol ) {
                    text << " ";
                    col ++;
                }
                continue;
            } else if ( CH=='\n' ) {
                row++;
                col=0;
                text << "\n";
                break;
            } else {
                text << CH;
            }
            col ++;
        }
        it = beginOfLine;
        const char * tail = it + COL;
        while ( *it && it!=tail && it!=itend ) {
            auto CH = *it++;
            if ( CH=='\t' ) {
                int tcol = (col + TAB) & ~(TAB-1);
                while ( col < tcol ) {
                    text << " ";
                    col ++;
                }
                continue;
            } else if ( CH=='\n' ) {
                break;
            } else {
                text << " ";
            }
            col ++;
        }
        text << string(das::max(LCOL-COL,1),'^') << "\n";
        return text.str();
    }

    string to_cpp_double ( double val ) {
        if ( val==DBL_MIN ) return "DBL_MIN";
        else if ( val==-DBL_MIN ) return "(-DBL_MIN)";
        else if ( val==DBL_MAX ) return "DBL_MAX";
        else if ( val==-DBL_MAX ) return "(-DBL_MAX)";
        else {
            char buf[256];
            auto result = fmt::format_to(buf, "{:e}", val);
            *result = 0;
            return buf;
        }
    }

    int32_t levenshtein_distance ( const char * s1, const char * s2 ) {
        int len1 = int(strlen(s1));
        int len2 = int(strlen(s2));
        if ( len1==0 ) return len2;
        if ( len2==0 ) return len1;
        int * v0 = (int *) alloca(sizeof(int)*(len2+1));
        int * v1 = (int *) alloca(sizeof(int)*(len2+1));
        for ( int i=0; i<=len2; ++i ) v0[i] = i;
        for ( int i=0; i<len1; ++i ) {
            v1[0] = i+1;
            for ( int j=0; j<len2; ++j ) {
                int cost = s1[i]==s2[j] ? 0 : 1;
                v1[j+1] = min(v1[j]+1, min(v0[j+1]+1, v0[j]+cost));
            }
            for ( int j=0; j<=len2; ++j ) v0[j] = v1[j];
        }
        return v1[len2];
    }

    string to_cpp_float ( float val ) {
        if ( val==FLT_MIN ) return "FLT_MIN";
        else if ( val==-FLT_MIN ) return "(-FLT_MIN)";
        else if ( val==FLT_MAX ) return "FLT_MAX";
        else if ( val==-FLT_MAX ) return "(-FLT_MAX)";
        else {
            char buf[256];
            auto result = fmt::format_to(buf, "{:e}f", val);
            *result = 0;
            return buf;
        }
    }

    string reportError(const struct LineInfo & at, const string & message,
        const string & extra, const string & fixme, CompilationError erc) {
        const char * src = nullptr;
        uint32_t len = 0;
        if ( at.fileInfo ) at.fileInfo->getSourceAndLength(src, len);
        return reportError(
                src, len,
                at.fileInfo ? at.fileInfo->name.c_str() : nullptr,
                at.line, at.column, at.last_line, at.last_column,
                at.fileInfo ? at.fileInfo->tabSize : 4,
                message, extra, fixme, erc );
    }

    string reportError ( const char * st, uint32_t stlen, const char * fileName,
        int row, int col, int lrow, int lcol, int tabSize, const string & message,
        const string & extra, const string & fixme, CompilationError erc ) {
        TextWriter ssw;
        if (erc != CompilationError::unspecified)
            ssw << "error[" << int(erc) << "]: ";
        else
            ssw << "error: ";
        if ( row ) {
            auto text = st ? getFewLines(st, stlen, row, col, lrow, lcol, tabSize) : "";
            ssw << message << "\n" << fileName << ":" << row << ":" << col << "\n" << text;
        } else {
            ssw << message << "\n";
        }
        if (!extra.empty()) ssw << extra << "\n";
        if (!fixme.empty()) ssw << "\t" << fixme << "\n";
        return ssw.str();
    }

    vec4f SimNode_StringBuilder::eval ( Context & context ) {
        DAS_PROFILE_NODE
        vec4f * argValues = (vec4f *)(alloca(nArguments * sizeof(vec4f)));
        evalArgs(context, argValues);
        StringBuilderWriter writer;
        DebugDataWalker<StringBuilderWriter> walker(writer, PrintFlags::string_builder);
        for ( int i=0, is=nArguments; i!=is; ++i ) {
            walker.walk(argValues[i], types[i]);
        }
        uint64_t length = writer.tellp();
        if ( length ) {
            auto pStr = context.allocateString(writer.c_str(), uint32_t(length), &debugInfo);
            if ( !pStr  ) {
                context.throw_out_of_memory(true, uint32_t(length), &debugInfo);
            }
            if ( isTempString ) context.freeTempString(pStr, &debugInfo);
            return cast<char *>::from(pStr);
        } else {
            return v_zero();
        }
    }

    // string iteration

    bool StringIterator::first ( Context &, char * _value )  {
        if ( str==nullptr || *str==0 ) return false;
        int32_t * value = (int32_t *) _value;
        *value = uint8_t(*str++);
        return true;
    }

    bool StringIterator::next  ( Context &, char * _value )  {
        int32_t * value = (int32_t *) _value;
        *value = uint8_t(*str++);
        return *value != 0;
    }

    void StringIterator::close ( Context & context, char * _value )  {
        if ( _value ) {
            int32_t * value = (int32_t *) _value;
            *value = 0;
        }
        context.freeIterator((char *)this, debugInfo);
    }

    vec4f SimNode_StringIterator::eval ( Context & context ) {
        DAS_PROFILE_NODE
        vec4f ll = source->eval(context);
        char * str = cast<char *>::to(ll);
        char * iter = context.allocateIterator(sizeof(StringIterator),"string iterator", &debugInfo);
        if ( !iter ) context.throw_out_of_memory(false, sizeof(StringIterator)+16, &debugInfo);
        new (iter) StringIterator(str, &debugInfo);
        return cast<char *>::from(iter);
    }
}
