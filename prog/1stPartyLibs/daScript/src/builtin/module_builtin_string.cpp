#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/misc/performance_time.h"
#include "daScript/simulate/aot_builtin_string.h"
#include "daScript/misc/string_writer.h"
#include "daScript/misc/debug_break.h"

#include "daScript/simulate/bind_enum.h"

#include <inttypes.h>
#include <fast_float/fast_float.h>

#if defined(_WIN32) && defined(__clang__)
    #define strdup _strdup
#endif

MAKE_TYPE_FACTORY(StringBuilderWriter, StringBuilderWriter)

DAS_BASE_BIND_ENUM(das::ConversionResult, ConversionResult, ok, invalid_argument, out_of_range)

namespace das
{

    struct StringBuilderWriterAnnotation : ManagedStructureAnnotation <StringBuilderWriter,false> {
        StringBuilderWriterAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("StringBuilderWriter", ml) {
        }
    };

    int32_t get_character_at ( const char * str, int32_t index, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if ( uint32_t(index)>=strLen ) {
            context->throw_error_at(at, "string character index out of range, %u of %u", uint32_t(index), strLen);
        }
        return ((uint8_t *)str)[index];
    }

    bool builtin_string_endswith ( const char * str, const char * cmp, Context * context ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        const uint32_t cmpLen = stringLengthSafe ( *context, cmp );
        return (cmpLen > strLen) ? false : memcmp(&str[strLen - cmpLen], cmp, cmpLen) == 0;
    }

    bool builtin_string_startswith ( const char * str, const char * cmp, Context * context ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        const uint32_t cmpLen = stringLengthSafe ( *context, cmp );
        return (cmpLen > strLen) ? false : memcmp(str, cmp, cmpLen) == 0;
    }

    bool builtin_string_startswith2 ( const char * str, const char * cmp, uint32_t cmpLen, Context * context ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        cmpLen = min(cmpLen, stringLengthSafe ( *context, cmp ));
        return (cmpLen > strLen) ? false : memcmp(str, cmp, cmpLen) == 0;
    }

    bool builtin_string_startswith3 ( const char * str, int32_t offset, const char * cmp, Context * context ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if ( offset<0 || uint32_t(offset)>=strLen ) return  false;
        const uint32_t cmpLen = stringLengthSafe ( *context, cmp );
        return (cmpLen > strLen) ? false : memcmp(str + offset, cmp, cmpLen) == 0;
    }

    bool builtin_string_startswith4 ( const char * str, int32_t offset, const char * cmp, uint32_t cmpLen, Context * context ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if ( offset<0 || uint32_t(offset)>=strLen ) return  false;
        cmpLen = min(cmpLen, stringLengthSafe ( *context, cmp ));
        return (cmpLen > strLen) ? false : memcmp(str + offset, cmp, cmpLen) == 0;
    }

    __forceinline bool is_space ( char c ) {
        return c==' ' || c=='\t' || c=='\r' || c=='\n' || c=='\f' || c=='\v';
    }

    static inline const char* strip_l(const char *str) {
        const char *t = str;
        while (((*t) != '\0') && is_space(*t))
            t++;
        return t;
    }

    static inline const char* strip_r(const char *str, uint32_t len) {
        if (len == 0)
            return str;
        const char *t = &str[len-1];
        while (t >= str && is_space(*t))
            t--;
        return t + 1;
    }

    char* builtin_string_strip ( const char *str, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        const char *start = strip_l(str);
        const char *end = strip_r(str, strLen);
        return end > start ? context->allocateString(start, uint32_t(end-start), at) : nullptr;
    }

    char* builtin_string_strip_left ( const char *str, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        const char *start = strip_l(str);
        return uint32_t(start-str) < strLen ? context->allocateString(start, strLen-uint32_t(start-str), at) : nullptr;
    }

    char* builtin_string_strip_right ( const char *str, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        const char *end = strip_r(str, strLen);
        return end != str ? context->allocateString(str, uint32_t(end-str), at) : nullptr;
    }

    static inline int clamp_int(int v, int minv, int maxv) {
        return (v < minv) ? minv : (v > maxv) ? maxv : v;
    }

    int builtin_string_find1 ( const char *str, const char *substr, int start, Context * context ) {
        if (!str || !substr)
            return -1;
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return -1;
        const char *ret = strstr(&str[clamp_int(start, 0, strLen)], substr);
        return ret ? int(ret-str) : -1;
    }

    int builtin_string_find2 (const char *str, const char *substr) {
        if (!str || !substr)
            return -1;
        const char *ret = strstr(str, substr);
        return ret ? int(ret-str) : -1;
    }

    int builtin_string_length ( const char *str, Context * context ) {
        return stringLengthSafe ( *context, str );
    }

    char* builtin_string_chop(const char* str, int start, int length, Context* context, LineInfoArg * at) {
        if ( !str || length<=0 ) return nullptr;
        return context->allocateString(str + start, length, at);
    }

    char* builtin_string_slice1 ( const char *str, int start, int end, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        start = clamp_int((start < 0) ? (strLen + start) : start, 0, strLen);
        end = clamp_int((end < 0) ? (strLen + end) : end, 0, strLen);
        return end > start ? context->allocateString(str + start, uint32_t(end-start), at) : nullptr;
    }

    char* builtin_string_slice2 ( const char *str, int start, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        start = clamp_int((start < 0) ? (strLen + start) : start, 0, strLen);
        return strLen > uint32_t(start) ? context->allocateString(str + start, uint32_t(strLen-start), at) : nullptr;
    }

    char* builtin_string_reverse ( const char *str, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        char * ret = context->allocateString(str, strLen, at);
        str += strLen-1;
        for (char *d = ret, *end = ret + strLen; d != end; --str, ++d)
          *d = *str;
        return ret;
    }

    __forceinline char to_lower(char ch) {
        return (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
    }

    char* builtin_string_tolower ( const char *str, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        char * ret = context->allocateString(str, strLen, at);
        for (char *d = ret, *end = ret + strLen; d != end; ++str, ++d)
          *d = (char)to_lower(*str);
        return ret;
    }

    char* builtin_string_tolower_in_place(char* str) {
        if (!str) return nullptr;
        char* pch = str;
        for (;;) {
            char ch = *pch;
            if (ch == 0) break;
            else if (ch >= 'A' && ch <= 'Z') *pch = ch - 'A' + 'a';
            pch++;
        }
        return str;
    }

    __forceinline char to_upper(char ch) {
        return (ch >= 'a' && ch <= 'z') ? (ch - 'a' + 'A') : ch;
    }

    char* builtin_string_toupper ( const char *str, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        char * ret = context->allocateString(str, strLen, at);
        for (char *d = ret, *end = ret + strLen; d != end; ++str, ++d)
          *d = (char)to_upper(*str);
        return ret;
    }

    char* builtin_string_toupper_in_place ( char* str ) {
        if (!str) return nullptr;
        char* pch = str;
        for (;;) {
            char ch = *pch;
            if (ch == 0) break;
            else if (ch >= 'a' && ch <= 'z') *pch = ch - 'a' + 'A';
            pch++;
        }
        return str;
    }

    template <typename TT>
    TT string_to_int_number ( const char *str, Context * context, LineInfoArg * at ) {
        if ( !str ) context->throw_error_at(at, "expecting string");
        TT result = 0;
        while ( is_white_space(*str) ) str++;
        bool hex = false;
        if ( str[0]=='0' && (str[1]=='x' || str[1]=='X') ) {
            hex = true;
            str += 2;
        }
        auto res = fast_float::from_chars(str, str+strlen(str), result, hex ? 16 : 10);
        if (res.ec != std::errc()) context->throw_error_at(at, "failed to convert '%s' to number", str);
        return result;
    }

    uint8_t string_to_uint8 ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_int_number<uint8_t>(str, context, at);
    }

    int8_t string_to_int8 ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_int_number<int8_t>(str, context, at);
    }

    uint16_t string_to_uint16 ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_int_number<uint16_t>(str, context, at);
    }

    int16_t string_to_int16 ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_int_number<int16_t>(str, context, at);
    }

    uint32_t string_to_uint ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_int_number<uint32_t>(str, context, at);
    }

    int32_t string_to_int ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_int_number<int32_t>(str, context, at);
    }

    uint64_t string_to_uint64 ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_int_number<uint64_t>(str, context, at);
    }

    int64_t string_to_int64 ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_int_number<int64_t>(str, context, at);
    }

    template <typename TT>
    TT string_to_real_number ( const char *str, Context * context, LineInfoArg * at ) {
        if ( !str ) context->throw_error_at(at, "expecting string");
        TT result = 0;
        while ( is_white_space(*str) ) str++;
        auto res = fast_float::from_chars(str, str+strlen(str), result);
        if (res.ec != std::errc()) context->throw_error_at(at, "failed to convert '%s' to number", str);
        return result;
    }

    float string_to_float ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_real_number<float>(str, context, at);
    }

    double string_to_double ( const char *str, Context * context, LineInfoArg * at ) {
        return string_to_real_number<double>(str, context, at);
    }

    template <typename TT>
    TT fast_to_real ( const char *str ) {
        if ( !str ) return 0;
        TT result = 0;
        while ( is_white_space(*str) ) str++;
        auto res = fast_float::from_chars(str, str+strlen(str), result);
        return (res.ec == std::errc()) ? result : 0;
    }

    float fast_to_float ( const char *str ) {
        return fast_to_real<float>(str);
    }

    double fast_to_double ( const char *str ) {
        return fast_to_real<double>(str);
    }

    template <typename TT>
    TT fast_to_int_TT ( const char *str, bool hex ) {
        if ( !str ) return 0;
        TT result = 0;
        while ( is_white_space(*str) ) str++;
        if ( hex && str[0]=='0' && (str[1]=='x' || str[1]=='X') ) str += 2;
        auto res = fast_float::from_chars(str, str+strlen(str), result, hex ? 16 : 10);
        return (res.ec == std::errc()) ? result : 0;
    }

    int8_t fast_to_int8 ( const char *str, bool hex ) {
        return fast_to_int_TT<int8_t>(str, hex);
    }

    uint8_t fast_to_uint8 ( const char *str, bool hex ) {
        return fast_to_int_TT<uint8_t>(str, hex);
    }

    int16_t fast_to_int16 ( const char *str, bool hex ) {
        return fast_to_int_TT<int16_t>(str, hex);
    }

    uint16_t fast_to_uint16 ( const char *str, bool hex ) {
        return fast_to_int_TT<uint16_t>(str, hex);
    }

    int32_t fast_to_int ( const char *str, bool hex ) {
        return fast_to_int_TT<int32_t>(str, hex);
    }

    uint32_t fast_to_uint ( const char *str, bool hex ) {
        return fast_to_int_TT<uint32_t>(str, hex);
    }

    int64_t fast_to_int64 ( const char *str, bool hex ) {
        return fast_to_int_TT<int64_t>(str, hex);
    }

    uint64_t fast_to_uint64 ( const char *str, bool hex ) {
        return fast_to_int_TT<uint64_t>(str, hex);
    }

    char * builtin_build_string ( const TBlock<void,StringBuilderWriter> & block, Context * context, LineInfoArg * at ) {
        StringBuilderWriter writer;
        vec4f args[1];
        args[0] = cast<StringBuilderWriter *>::from(&writer);
        context->invoke(block, args, nullptr, at);
        auto length = writer.tellp();
        if ( length ) {
            return context->allocateString(writer.c_str(), uint32_t(length),at);
        } else {
            return nullptr;
        }
    }

    vec4f builtin_write_string ( Context &, SimNode_CallBase * call, vec4f * args ) {
        StringBuilderWriter * writer = cast<StringBuilderWriter *>::to(args[0]);
        DebugDataWalker<StringBuilderWriter> walker(*writer, PrintFlags::string_builder);
        walker.walk(args[1], call->types[1]);
        return cast<StringBuilderWriter *>::from(writer);
    }

    StringBuilderWriter & write_string_char ( StringBuilderWriter & writer, int32_t ch ) {
        char buf[2];
        buf[0] = char(ch);
        buf[1] = 0;
        writer.writeStr(buf, 1);
        return writer;
    }

    StringBuilderWriter & write_string_chars ( StringBuilderWriter & writer, int32_t ch, int32_t count ) {
        if ( count >0 ) writer.writeChars(char(ch), count);
        return writer;
    }

    StringBuilderWriter & write_escape_string ( StringBuilderWriter & writer, char * str ) {
        if ( !str ) return writer;
        auto estr = escapeString(str,false);
        writer.writeStr(estr.c_str(), estr.length());
        return writer;
    }

    char * to_string_char ( int ch, Context * context, LineInfoArg * at ) {
        auto st = context->allocateString(nullptr, 1, at);
        *st = char(ch);
        return st;
    }

    char * string_repeat ( const char * str, int count, Context * context, LineInfoArg * at ) {
        uint32_t len = stringLengthSafe ( *context, str );
        if ( !len || count<=0 ) return nullptr;
        char * res = context->allocateString(nullptr, len * count, at);
        for ( char * s = res; count; count--, s+=len ) {
            memcpy ( s, str, len );
        }
        return res;
    }

    vector<string> split ( const char * str, const char * delim ) {
        if ( !str ) str = "";
        if ( !delim ) delim = "";
        vector<const char *> tokens;
        vector<string> words;
        const char * ch = str;
        auto delimLen = strlen(delim);
        if ( delimLen ) {
            while ( *ch ) {
                const char * tok = ch;
                while ( *ch && !strchr(delim,*ch) ) ch++;
                words.push_back(string(tok,ch-tok));
                if ( !*ch ) break;
                if ( strchr(delim,*ch) ) ch++;
                if ( !*ch ) words.push_back("");
            }
        } else {
            auto len = strlen(str);
            words.reserve(len);
            while ( *ch ) {
                words.push_back(string(1,*ch));
                ch ++;
            }
        }
        return words;
    }

    void builtin_string_split_by_char ( const char * str, const char * delim, const Block & block, Context * context, LineInfoArg * at ) {
        if ( !str ) str = "";
        if ( !delim ) delim = "";
        vector<const char *> tokens;
        vector<string> words;
        const char * ch = str;
        auto delimLen = stringLengthSafe(*context,delim);
        if ( delimLen ) {
            while ( *ch ) {
                const char * tok = ch;
                while ( *ch && !strchr(delim,*ch) ) ch++;
                words.push_back(string(tok,ch-tok));
                if ( !*ch ) break;
                if ( strchr(delim,*ch) ) ch++;
                if ( !*ch ) words.push_back("");
            }
        } else {
            auto len = stringLengthSafe(*context,str);
            words.reserve(len);
            while ( *ch ) {
                words.push_back(string(1,*ch));
                ch ++;
            }
        }
        tokens.reserve(words.size());
        for ( auto & tok : words ) {
            tokens.push_back(tok.c_str());
        }
        if ( tokens.empty() ) tokens.push_back("");
        Array arr;
        arr.data = (char *) tokens.data();
        arr.capacity = arr.size = uint32_t(tokens.size());
        arr.lock = 1;
        arr.flags = 0;
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(block, args, nullptr, at);
    }

    void builtin_string_split ( const char * str, const char * delim, const Block & block, Context * context, LineInfoArg * at ) {
        if ( !str ) str = "";
        if ( !delim ) delim = "";
        vector<const char *> tokens;
        vector<string> words;
        const char * ch = str;
        auto delimLen = stringLengthSafe(*context,delim);
        if ( delimLen ) {
            while ( *ch ) {
                const char * tok = ch;
                while ( *ch && strncmp(delim,ch,delimLen)!=0 ) ch++;
                words.push_back(string(tok,ch-tok));
                if ( !*ch ) break;
                if ( strncmp(delim,ch,delimLen)==0 ) ch+=delimLen;
                if ( !*ch ) words.push_back("");
            }
        } else {
            auto len = stringLengthSafe(*context,str);
            words.reserve(len);
            while ( *ch ) {
                words.push_back(string(1,*ch));
                ch ++;
            }
        }
        tokens.reserve(words.size());
        for ( auto & tok : words ) {
            tokens.push_back(tok.c_str());
        }
        if ( tokens.empty() ) tokens.push_back("");
        Array arr;
        arr.data = (char *) tokens.data();
        arr.capacity = arr.size = uint32_t(tokens.size());
        arr.lock = 1;
        arr.flags = 0;
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(block, args, nullptr, at);
    }

    char * builtin_string_replace ( const char * str, const char * toSearch, const char * replaceStr, Context * context, LineInfoArg * at ) {
        auto toSearchSize = stringLengthSafe(*context, toSearch);
        if ( !toSearchSize ) return (char *) str;
        string data = str ? str : "";
        auto replaceStrSize = stringLengthSafe(*context,replaceStr);
        const char * repl = replaceStr ? replaceStr : "";
        const char * toss = toSearch ? toSearch : "";
        size_t pos = data.find(toss);
        while ( pos != string::npos ) {
            data.replace(pos, toSearchSize, repl);
            pos = data.find(toss, pos + replaceStrSize);
        }
        return context->allocateString(data, at);
    }

    class StrdupDataWalker : public DataWalker {
        virtual void String ( char * & str ) {
            if (str) str = strdup(str);
        }
    };

    vec4f builtin_strdup ( Context &, SimNode_CallBase * call, vec4f * args ) {
        StrdupDataWalker walker;
        walker.walk(args[0], call->types[0]);
        return v_zero();
    }

    char * builtin_string_escape ( const char *str, Context * context, LineInfoArg * at ) {
        if ( !str ) return nullptr;
        return context->allocateString(escapeString(str,false), at);
    }

    char * builtin_string_unescape ( const char *str, Context * context, LineInfoArg * at ) {
        if ( !str ) return nullptr;
        bool err = false;
        auto estr = unescapeString(str, &err, false);
        if ( err ) context->throw_error_at(at, "invalid escape sequence");
        return context->allocateString(estr,at);
    }

    char * builtin_string_safe_unescape ( const char *str, Context * context, LineInfoArg * at ) {
        if ( !str ) return nullptr;
        bool err = false;
        auto estr = unescapeString(str, &err, false);
        return context->allocateString(estr, at);
    }

    int builtin_find_first_char_of ( const char * str, int Ch, Context * context ) {
        uint32_t strlen = stringLengthSafe ( *context, str );
        for ( uint32_t o=0; o!=strlen; ++o ) {
            if ( str[o]==Ch ) {
                return o;
            }
        }
        return -1;
    }

    int builtin_find_first_char_of2 ( const char * str, int Ch, int start, Context * context ) {
        uint32_t strlen = stringLengthSafe ( *context, str );
        start = clamp_int((start < 0) ? (strlen + start) : start, 0, strlen);
        for ( uint32_t o=start; o!=strlen; ++o ) {
            if ( str[o]==Ch ) {
                return o;
            }
        }
        return -1;
    }

    char * builtin_string_from_array ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        if ( !bytes.size ) return nullptr;
        return context->allocateString(bytes.data, bytes.size, at);
    }

    void delete_string ( char * & str, Context * context, LineInfoArg * at ) {
        if ( !str ) return;
        uint32_t len = stringLengthSafe(*context, str);
        context->freeString(str, len, at);
        str = nullptr;
    }

    void builtin_append_char_to_string(string & str, int32_t Ch) {
        str += char(Ch);
    }

    bool builtin_string_ends_with(const string &str, char * substr, Context * context ) {
        if ( substr==nullptr ) return false;
        auto sz = str.length();
        auto slen = stringLengthSafe(*context,substr);
        if ( slen>sz ) return false;
        return memcmp ( str.data() + sz - slen, substr, slen )==0;
    }

    int32_t builtin_ext_string_length(const string & str) {
        return int32_t(str.length());
    }

    void builtin_resize_string(string & str, int32_t newLength) {
        str.resize(newLength);
    }

    // TODO: do we need a corresponding delete?
    char * builtin_reserve_string_buffer ( const char * str, int32_t length, Context * context ) {
        auto buf = context->allocate(length);
        if ( !buf ) context->throw_out_of_memory(false, length);
        if ( str ) {
            auto slen = min ( int32_t(strlen(str)), length-1 );
            memcpy ( buf, str, slen );
            buf[slen] = 0;
        } else {
            buf[0] = 0;
        }
        return buf;
    }

    char * builtin_string_trim ( char* s, Context * context, LineInfoArg * at ) {
        if ( !s ) return nullptr;
        while ( is_white_space(*s) ) s++;
        if ( *s ) return builtin_string_rtrim(s, context, at);
        return nullptr;
    }

    char * builtin_string_ltrim ( char* s, Context * context, LineInfoArg * at ) {
        if ( !s ) return nullptr;
        while ( is_white_space(*s) ) s++;
        if ( *s ) {
            return context->allocateString(s, uint32_t(strlen(s)), at);
        } else  {
            return nullptr;
        }
    }

    char * builtin_string_rtrim ( char* s, Context * context, LineInfoArg * at ) {
        if ( !s ) return nullptr;
        char * str_end_o = s + strlen(s);
        char * str_end = str_end_o;
        while ( str_end > s && is_white_space(str_end[-1]) ) str_end--;
        if ( str_end==s ) {
            return nullptr;
        } else if ( str_end!=str_end_o ) {
            auto len = str_end - s;
            char * res = context->allocateString(nullptr, int32_t(len), at);
            memcpy ( res, s, len );
            res[len] = 0;
            return res;
        } else {
            return s;
        }
    }

    bool is_char_in_string ( char c, const char * str ) {
        while ( *str ) {
            if ( *str++==c ) return true;
        }
        return false;
    }

    char * builtin_string_rtrim_ts ( char* s, char * ts, Context * context, LineInfoArg * at ) {
        if ( !s ) return nullptr;
        if ( !ts ) return s;
        char * str_end_o = s + strlen(s);
        char * str_end = str_end_o;
        while ( str_end > s && is_char_in_string(str_end[-1],ts) ) str_end--;
        if ( str_end==s ) {
            return nullptr;
        } else if ( str_end!=str_end_o ) {
            auto len = str_end - s;
            char * res = context->allocateString(nullptr, int32_t(len), at);
            memcpy ( res, s, len );
            res[len] = 0;
            return res;
        } else {
            return s;
        }
    }

    void builtin_string_peek ( const char * str, const TBlock<void,TTemporary<TArray<uint8_t> const>> & block, Context * context, LineInfoArg * at ) {
        if ( !str ) return;
        Array arr;
        arr.data = (char *) str;
        arr.capacity = arr.size = uint32_t(strlen(str));
        arr.lock = 1;
        arr.flags = 0;
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(block, args, nullptr, at);
    }

    char * builtin_string_peek_and_modify ( const char * str, const TBlock<void,TTemporary<TArray<uint8_t>>> & block, Context * context, LineInfoArg * at ) {
        if ( !str ) return nullptr;
        int32_t len = int32_t(strlen(str));
        char * cstr = context->allocateString(str, len, at);
        memcpy(cstr, str, len);
        Array arr;
        arr.data = cstr;
        arr.capacity = arr.size = len;
        arr.lock = 1;
        arr.flags = 0;
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(block, args, nullptr, at);
        return cstr;
    }

    uint64_t builtin_build_hash ( const TBlock<void,StringBuilderWriter> & block, Context * context, LineInfoArg * at ) {
        StringBuilderWriter writer;
        vec4f args[1];
        args[0] = cast<StringBuilderWriter *>::from(&writer);
        context->invoke(block, args, nullptr, at);
        return hash_blockz64((const uint8_t *)writer.c_str());
    }

    template <typename TT>
    TT convert_from_string ( const char * str, ConversionResult & result, int32_t & offset ) {
        offset = 0;
        if ( !str ) {
            result = ConversionResult::invalid_argument;
            return TT();
        }
        TT value = 0;
        while ( is_white_space(str[offset]) ) offset++;
        auto res = fast_float::from_chars(str+offset, str+strlen(str), value);
        if (res.ec != std::errc()) {
            result = ConversionResult(res.ec);
            return TT();
        }
        offset = int32_t(res.ptr - str);
        return value;
    }

    template <typename TT>
    TT convert_int_from_string ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        offset = 0;
        if ( !str ) {
            result = ConversionResult::invalid_argument;
            return TT();
        }
        TT value = 0;
        while ( is_white_space(str[offset]) ) offset++;
        if ( hex && str[0]=='0' && (str[1]=='x' || str[1]=='X') ) str += 2;
        auto res = fast_float::from_chars(str+offset, str+strlen(str), value, hex ? 16 : 10);
        if (res.ec != std::errc()) {
            result = ConversionResult(res.ec);
            return TT();
        }
        offset = int32_t(res.ptr - str);
        return value;
    }

    int8_t convert_from_string_int8 ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        return convert_int_from_string<int8_t>(str, result, offset, hex);
    }

    uint8_t convert_from_string_uint8 ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        return convert_int_from_string<uint8_t>(str, result, offset, hex);
    }

    int16_t convert_from_string_int16 ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        return convert_int_from_string<int16_t>(str, result, offset, hex);
    }

    uint16_t convert_from_string_uint16 ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        return convert_int_from_string<uint16_t>(str, result, offset, hex);
    }

    int32_t convert_from_string_int32 ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        return convert_int_from_string<int32_t>(str, result, offset, hex);
    }

    uint32_t convert_from_string_uint32 ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        return convert_int_from_string<uint32_t>(str, result, offset, hex);
    }

    int64_t convert_from_string_int64 ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        return convert_int_from_string<int64_t>(str, result, offset, hex);
    }

    uint64_t convert_from_string_uint64 ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        return convert_int_from_string<uint64_t>(str, result, offset, hex);
    }

    float convert_from_string_float ( const char * str, ConversionResult & result, int32_t & offset ) {
        return convert_from_string<float>(str, result, offset);
    }

    double convert_from_string_double ( const char * str, ConversionResult & result, int32_t & offset ) {
        return convert_from_string<double>(str, result, offset);
    }

    class Module_Strings : public Module {
    public:
        Module_Strings() : Module("strings") {
            DAS_PROFILE_SECTION("Module_Strings");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            // string builder writer
            addEnumeration(make_smart<EnumerationConversionResult>());
            addAnnotation(make_smart<StringBuilderWriterAnnotation>(lib));
            addExtern<DAS_BIND_FUN(delete_string)>(*this, lib, "delete_string",
                SideEffects::modifyArgumentAndExternal,"delete_string")->args({"str","context","lineinfo"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_build_string)>(*this, lib, "build_string",
                SideEffects::modifyExternal,"builtin_build_string_T")->args({"block","context","lineinfo"})->setAotTemplate();
            addExtern<DAS_BIND_FUN(builtin_build_hash)>(*this, lib, "build_hash",
                SideEffects::modifyExternal,"builtin_build_hash_T")->args({"block","context","lineinfo"})->setAotTemplate();
            addExtern<DAS_BIND_FUN(builtin_string_peek)>(*this, lib, "peek_data",
                SideEffects::modifyExternal,"builtin_string_peek")->args({"str","block","context","lineinfo"});
            addExtern<DAS_BIND_FUN(builtin_string_peek_and_modify)>(*this, lib, "modify_data",
                SideEffects::modifyExternal,"builtin_string_peek_and_modify")->args({"str","block","context","lineinfo"});
            addInterop<builtin_write_string,StringBuilderWriter &,StringBuilderWriter,vec4f> (*this, lib, "write",
                SideEffects::modifyExternal, "builtin_write_string")->args({"writer","anything"});
            addExtern<DAS_BIND_FUN(write_string_char),SimNode_ExtFuncCallRef>(*this, lib, "write_char",
                SideEffects::modifyExternal, "write_string_char")->args({"writer","ch"});
            addExtern<DAS_BIND_FUN(write_string_chars),SimNode_ExtFuncCallRef>(*this, lib, "write_chars",
                SideEffects::modifyExternal, "write_string_chars")->args({"writer","ch","count"});
            addExtern<DAS_BIND_FUN(write_escape_string),SimNode_ExtFuncCallRef>(*this, lib, "write_escape_string",
                SideEffects::modifyExternal, "write_escape_string")->args({"writer","str"});
            // fmt
            addExtern<DAS_BIND_FUN(fmt_and_write_i8),SimNode_ExtFuncCallRef> (*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_i8")->args({"writer","format","value","context","lineinfo"});
            addExtern<DAS_BIND_FUN(fmt_and_write_u8),SimNode_ExtFuncCallRef>(*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_u8")->args({"writer","format","value","context","lineinfo"});
            addExtern<DAS_BIND_FUN(fmt_and_write_i16),SimNode_ExtFuncCallRef> (*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_i16")->args({"writer","format","value","context","lineinfo"});
            addExtern<DAS_BIND_FUN(fmt_and_write_u16),SimNode_ExtFuncCallRef>(*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_u16")->args({"writer","format","value","context","lineinfo"});
            addExtern<DAS_BIND_FUN(fmt_and_write_i32),SimNode_ExtFuncCallRef> (*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_i32")->args({"writer","format","value","context","lineinfo"});
            addExtern<DAS_BIND_FUN(fmt_and_write_u32),SimNode_ExtFuncCallRef>(*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_u32")->args({"writer","format","value","context","lineinfo"});
            addExtern<DAS_BIND_FUN(fmt_and_write_i64),SimNode_ExtFuncCallRef> (*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_i64")->args({"writer","format","value","context","lineinfo"});
            addExtern<DAS_BIND_FUN(fmt_and_write_u64),SimNode_ExtFuncCallRef>(*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_u64")->args({"writer","format","value","context","lineinfo"});
            addExtern<DAS_BIND_FUN(fmt_and_write_f),SimNode_ExtFuncCallRef>   (*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_f")->args({"writer","format","value","context","lineinfo"});
            addExtern<DAS_BIND_FUN(fmt_and_write_d),SimNode_ExtFuncCallRef>  (*this, lib, "fmt",
                SideEffects::modifyExternal, "fmt_and_write_d")->args({"writer","format","value","context","lineinfo"});
            // format (deprecated)
            addExtern<DAS_BIND_FUN(format_and_write<int32_t>),SimNode_ExtFuncCallRef> (*this, lib, "format",
                SideEffects::modifyExternal, "format_and_write<int32_t>")->args({"writer","format","value"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format_and_write<uint32_t>),SimNode_ExtFuncCallRef>(*this, lib, "format",
                SideEffects::modifyExternal, "format_and_write<uint32_t>")->args({"writer","format","value"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format_and_write<int64_t>),SimNode_ExtFuncCallRef> (*this, lib, "format",
                SideEffects::modifyExternal, "format_and_write<int64_t>")->args({"writer","format","value"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format_and_write<uint64_t>),SimNode_ExtFuncCallRef>(*this, lib, "format",
                SideEffects::modifyExternal, "format_and_write<uint64_t>")->args({"writer","format","value"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format_and_write<float>),SimNode_ExtFuncCallRef>   (*this, lib, "format",
                SideEffects::modifyExternal, "format_and_write<float>")->args({"writer","format","value"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format_and_write<double>),SimNode_ExtFuncCallRef>  (*this, lib, "format",
                SideEffects::modifyExternal, "format_and_write<double>")->args({"writer","format","value"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(builtin_string_from_array)>(*this, lib, "string",
                SideEffects::none, "builtin_string_from_array")->args({"bytes","context","at"});
            // dup
            addInterop<builtin_strdup,void,vec4f> (*this, lib, "builtin_strdup",
                SideEffects::modifyArgumentAndExternal, "builtin_strdup")->arg("anything")->unsafeOperation = true;
            // regular string
            addExtern<DAS_BIND_FUN(get_character_at)>(*this, lib, "character_at",
                SideEffects::none, "get_character_at")->args({"str","idx","context","at"});
            addExtern<DAS_BIND_FUN(get_character_uat)>(*this, lib, "character_uat",
                SideEffects::none, "get_character_uat")->args({"str","idx"})->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(string_repeat)>(*this, lib, "repeat",
                SideEffects::none, "string_repeat")->args({"str","count","context","at"});
            addExtern<DAS_BIND_FUN(to_string_char)>(*this, lib, "to_char",
                SideEffects::none, "to_string_char")->args({"char","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_endswith)>(*this, lib, "ends_with",
                SideEffects::none, "builtin_string_endswith")->args({"str","cmp","context"});
            addExtern<DAS_BIND_FUN(builtin_string_ends_with)>(*this, lib, "ends_with",
                SideEffects::none, "builtin_string_ends_with")->args({"str","cmp","context"});
            addExtern<DAS_BIND_FUN(builtin_string_startswith)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_string_startswith")->args({"str","cmp","context"});
            addExtern<DAS_BIND_FUN(builtin_string_startswith2)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_string_startswith2")->args({"str","cmp","cmpLen","context"});
            addExtern<DAS_BIND_FUN(builtin_string_startswith3)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_string_startswith3")->args({"str","offset","cmp","context"});
            addExtern<DAS_BIND_FUN(builtin_string_startswith4)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_string_startswith4")->args({"str","offset","cmp","cmpLen","context"});
            addExtern<DAS_BIND_FUN(builtin_string_strip)>(*this, lib, "strip",
                SideEffects::none, "builtin_string_strip")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_strip_right)>(*this, lib, "strip_right",
                SideEffects::none, "builtin_string_strip_right")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_strip_left)>(*this, lib, "strip_left",
                SideEffects::none, "builtin_string_strip_left")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_chop)>(*this, lib, "chop",
                SideEffects::none, "builtin_string_chop")->args({"str","start","length","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_slice1)>(*this, lib, "slice",
                SideEffects::none, "builtin_string_slice1")->args({"str","start","end","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_slice2)>(*this, lib, "slice",
                SideEffects::none, "builtin_string_slice2")->args({"str","start","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_find1)>(*this, lib, "find",
                SideEffects::none, "builtin_string_find1")->args({"str","substr","start","context"});
            addExtern<DAS_BIND_FUN(builtin_string_find2)>(*this, lib, "find",
                SideEffects::none, "builtin_string_find2")->args({"str","substr"});
            addExtern<DAS_BIND_FUN(builtin_find_first_char_of)>(*this, lib, "find",
                SideEffects::none, "builtin_find_first_char_of")->args({"str","substr","context"});
            addExtern<DAS_BIND_FUN(builtin_find_first_char_of2)>(*this, lib, "find",
                SideEffects::none, "builtin_find_first_char_of2")->args({"str","substr", "start", "context"});
            addExtern<DAS_BIND_FUN(builtin_string_length)>(*this, lib, "length",
                SideEffects::none, "builtin_string_length")->args({"str","context"});
            addExtern<DAS_BIND_FUN(builtin_string_reverse)>(*this, lib, "reverse",
                SideEffects::none, "builtin_string_reverse")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_append_char_to_string)>(*this, lib, "append",
                SideEffects::modifyArgumentAndExternal, "builtin_append_char_to_string")->args({"str","ch"});
            addExtern<DAS_BIND_FUN(builtin_resize_string)>(*this, lib, "resize",
                SideEffects::modifyArgumentAndExternal, "builtin_resize_string")->args({"str","new_length"});
            addExtern<DAS_BIND_FUN(builtin_ext_string_length)>(*this, lib, "length",
                SideEffects::none, "builtin_ext_string_length")->arg("str");
            addExtern<DAS_BIND_FUN(builtin_string_toupper)>(*this, lib, "to_upper",
                SideEffects::none, "builtin_string_toupper")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_tolower)>(*this, lib, "to_lower",
                SideEffects::none, "builtin_string_tolower")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_tolower_in_place)>(*this, lib, "to_lower_in_place",
                SideEffects::none, "builtin_string_tolower_in_place")->arg("str")->setCaptureString()->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_string_toupper_in_place)>(*this, lib, "to_upper_in_place",
                SideEffects::none, "builtin_string_toupper_in_place")->arg("str")->setCaptureString()->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(builtin_string_split_by_char)>(*this, lib, "builtin_string_split_by_char",
                SideEffects::modifyExternal, "builtin_string_split_by_char")->args({"str","delimiter","block","context","lineinfo"});
            addExtern<DAS_BIND_FUN(builtin_string_split)>(*this, lib, "builtin_string_split",
                SideEffects::modifyExternal, "builtin_string_split")->args({"str","delimiter","block","context","lineinfo"});
            // conversion which throws exception on error. detects hex automatically
            addExtern<DAS_BIND_FUN(string_to_int8)>(*this, lib, "int8",
                SideEffects::none, "string_to_int8")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(string_to_uint8)>(*this, lib, "uint8",
                SideEffects::none, "string_to_uint8")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(string_to_int16)>(*this, lib, "int16",
                SideEffects::none, "string_to_int16")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(string_to_uint16)>(*this, lib, "uint16",
                SideEffects::none, "string_to_uint16")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(string_to_int)>(*this, lib, "int",
                SideEffects::none, "string_to_int")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(string_to_uint)>(*this, lib, "uint",
                SideEffects::none, "string_to_uint")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(string_to_int64)>(*this, lib, "int64",
                SideEffects::none, "string_to_int64")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(string_to_uint64)>(*this, lib, "uint64",
                SideEffects::none, "string_to_uint64")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(string_to_float)>(*this, lib, "float",
                SideEffects::none, "string_to_float")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(string_to_double)>(*this, lib, "double",
                SideEffects::none, "string_to_double")->args({"str","context","at"});
            // fast conversion, returns 0 if fails
            addExtern<DAS_BIND_FUN(fast_to_int8)>(*this, lib, "to_int8",
                SideEffects::none, "fast_to_int8")->args({"value","hex"})->arg_init(1,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(fast_to_uint8)>(*this, lib, "to_uint8",
                SideEffects::none, "fast_to_uint8")->args({"value","hex"})->arg_init(1,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(fast_to_int16)>(*this, lib, "to_int16",
                SideEffects::none, "fast_to_int16")->args({"value","hex"})->arg_init(1,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(fast_to_int)>(*this, lib, "to_int",
                SideEffects::none, "fast_to_int")->args({"value","hex"})->arg_init(1,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(fast_to_uint)>(*this, lib, "to_uint",
                SideEffects::none, "fast_to_uint")->args({"value","hex"})->arg_init(1,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(fast_to_int64)>(*this, lib, "to_int64",
                SideEffects::none, "fast_to_int64")->args({"value","hex"})->arg_init(1,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(fast_to_uint64)>(*this, lib, "to_uint64",
                SideEffects::none, "fast_to_uint64")->args({"value","hex"})->arg_init(1,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(fast_to_float)>(*this, lib, "to_float",
                SideEffects::none, "fast_to_float")->arg("value");
            addExtern<DAS_BIND_FUN(fast_to_double)>(*this, lib, "to_double",
                SideEffects::none, "fast_to_double")->arg("value");
            // conversion which returns error and offset of the first invalid character
            addExtern<DAS_BIND_FUN(convert_from_string_int8)>(*this, lib, "int8",
                SideEffects::modifyArgument, "convert_from_string_int8")->args({"str","result","offset","hex"})->arg_init(3,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(convert_from_string_uint8)>(*this, lib, "uint8",
                SideEffects::modifyArgument, "convert_from_string_uint8")->args({"str","result","offset","hex"})->arg_init(3,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(convert_from_string_int16)>(*this, lib, "int16",
                SideEffects::modifyArgument, "convert_from_string_int16")->args({"str","result","offset","hex"})->arg_init(3,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(convert_from_string_uint16)>(*this, lib, "uint16",
                SideEffects::modifyArgument, "convert_from_string_uint16")->args({"str","result","offset","hex"})->arg_init(3,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(convert_from_string_int32)>(*this, lib, "int",
                SideEffects::modifyArgument, "convert_from_string_int32")->args({"str","result","offset","hex"})->arg_init(3,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(convert_from_string_uint32)>(*this, lib, "uint",
                SideEffects::modifyArgument, "convert_from_string_uint32")->args({"str","result","offset","hex"})->arg_init(3,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(convert_from_string_int64)>(*this, lib, "int64",
                SideEffects::modifyArgument, "convert_from_string_int64")->args({"str","result","offset","hex"})->arg_init(3,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(convert_from_string_uint64)>(*this, lib, "uint64",
                SideEffects::modifyArgument, "convert_from_string_uint64")->args({"str","result","offset","hex"})->arg_init(3,make_smart<ExprConstBool>(false));
            addExtern<DAS_BIND_FUN(convert_from_string_float)>(*this, lib, "float",
                SideEffects::modifyArgument, "convert_from_string_float")->args({"str","result","offset"});
            addExtern<DAS_BIND_FUN(convert_from_string_double)>(*this, lib, "double",
                SideEffects::modifyArgument, "convert_from_string_double")->args({"str","result","offset"});
            // escaping etc
            addExtern<DAS_BIND_FUN(builtin_string_escape)>(*this, lib, "escape",
                SideEffects::none, "builtin_string_escape")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_unescape)>(*this, lib, "unescape",
                SideEffects::none, "builtin_string_unescape")->args({"str","context", "at"});
            addExtern<DAS_BIND_FUN(builtin_string_safe_unescape)>(*this, lib, "safe_unescape",
                SideEffects::none, "builtin_string_safe_unescape")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_replace)>(*this, lib, "replace",
                SideEffects::none, "builtin_string_replace")->args({"str","toSearch","replace","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_rtrim)>(*this, lib, "rtrim",
                SideEffects::none, "builtin_string_rtrim")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_rtrim_ts)>(*this, lib, "rtrim",
                SideEffects::none, "builtin_string_rtrim_ts")->args({"str","chars","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_ltrim)>(*this, lib, "ltrim",
                SideEffects::none, "builtin_string_ltrim")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_string_trim)>(*this, lib, "trim",
                SideEffects::none, "builtin_string_trim")->args({"str","context","at"});
            // fmt
            addExtern<DAS_BIND_FUN(fmt_i8)>(*this, lib, "fmt",
                SideEffects::none, "fmt_i8")->args({"format","value","context","at"});
            addExtern<DAS_BIND_FUN(fmt_u8)>(*this, lib, "fmt",
                SideEffects::none, "fmt_u8")->args({"format","value","context","at"});
            addExtern<DAS_BIND_FUN(fmt_i16)>(*this, lib, "fmt",
                SideEffects::none, "fmt_i16")->args({"format","value","context","at"});
            addExtern<DAS_BIND_FUN(fmt_u16)>(*this, lib, "fmt",
                SideEffects::none, "fmt_u16")->args({"format","value","context","at"});
            addExtern<DAS_BIND_FUN(fmt_i32)>(*this, lib, "fmt",
                SideEffects::none, "fmt_i32")->args({"format","value","context","at"});
            addExtern<DAS_BIND_FUN(fmt_u32)>(*this, lib, "fmt",
                SideEffects::none, "fmt_u32")->args({"format","value","context","at"});
            addExtern<DAS_BIND_FUN(fmt_i64)>(*this, lib, "fmt",
                SideEffects::none, "fmt_i64")->args({"format","value","context","at"});
            addExtern<DAS_BIND_FUN(fmt_u64)>(*this, lib, "fmt",
                SideEffects::none, "fmt_u64")->args({"format","value","context","at"});
            addExtern<DAS_BIND_FUN(fmt_f)>(*this, lib, "fmt",
                SideEffects::none, "fmt_f")->args({"format","value","context","at"});
            addExtern<DAS_BIND_FUN(fmt_d)>(*this, lib, "fmt",
                SideEffects::none, "fmt_d")->args({"format","value","context","at"});
            // format (deprecated)
            addExtern<DAS_BIND_FUN(format<int32_t>)> (*this, lib, "format",
                SideEffects::none, "format<int32_t>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format<uint32_t>)>(*this, lib, "format",
                SideEffects::none, "format<uint32_t>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format<int64_t>)> (*this, lib, "format",
                SideEffects::none, "format<int64_t>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format<uint64_t>)>(*this, lib, "format",
                SideEffects::none, "format<uint64_t>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format<float>)>   (*this, lib, "format",
                SideEffects::none, "format<float>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead");
            addExtern<DAS_BIND_FUN(format<double>)>  (*this, lib, "format",
                SideEffects::none, "format<double>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead");
            // queries
            addExtern<DAS_BIND_FUN(is_alpha)> (*this, lib, "is_alpha",
                SideEffects::none, "is_alpha")->arg("Character");
            addExtern<DAS_BIND_FUN(is_new_line)> (*this, lib, "is_new_line",
                SideEffects::none, "is_new_line")->arg("Character");
            addExtern<DAS_BIND_FUN(is_white_space)> (*this, lib, "is_white_space",
                SideEffects::none, "is_white_space")->arg("Character");
            addExtern<DAS_BIND_FUN(is_number)> (*this, lib, "is_number",
                SideEffects::none, "is_number")->arg("Character");
            // bitset helpers
            addExtern<DAS_BIND_FUN(is_char_in_set)>(*this, lib, "is_char_in_set",
                SideEffects::none,"is_char_in_set")->args({"Character","Charset","Context","At"});
            addExtern<DAS_BIND_FUN(char_set_total)>(*this, lib, "set_total",
                SideEffects::none,"char_set_total")->arg("Charset");
            addExtern<DAS_BIND_FUN(char_set_element)>(*this, lib, "set_element",
                SideEffects::none,"char_set_element")->args({"Character","Charset"});
            // string buffer
            addExtern<DAS_BIND_FUN(builtin_reserve_string_buffer)>(*this, lib, "reserve_string_buffer",
                SideEffects::none,"builtin_reserve_string_buffer")->args({"str","length","context"});

            // lets make sure its all aot ready
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_string.h\"\n";
            return ModuleAotType::cpp;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Strings,das);
