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
        if ( index<0 ) {
            context->throw_error_at(at, "string character index out of range, %u", uint32_t(index));
        }
        for ( int32_t i = 0; i <= index; ++i ) {
            if ( str[i]==0 ) {
                context->throw_error_at(at, "string character index out of range, %u", uint32_t(index));
            }
        }
        return ((uint8_t *)str)[index];
    }

    int32_t get_first_character ( const char * str, Context * context, LineInfoArg * at ) {
        if ( !str || str[0]==0 ) {
            context->throw_error_at(at, "string is empty");
        }
        return ((uint8_t *)str)[0];
    }

    int32_t get_first_character_ds ( const string & str, Context * context, LineInfoArg * at ) {
        if ( str.empty() ) {
            context->throw_error_at(at, "string is empty");
        }
        return ((uint8_t *)str.c_str())[0];
    }

    void with_das_string ( const TBlock<void,TTemporary<string>> & block, Context * context, LineInfoArg * at ) {
        string tmp;
        vec4f args[1];
        args[0] = cast<string&>::from(tmp);
        context->invoke(block, args, nullptr, at);
    }

    static inline int clamp_int(int v, int minv, int maxv) {
        return (v < minv) ? minv : (v > maxv) ? maxv : v;
    }

    // an empty view has a null data pointer, which the cores would read as "no string at all"
    static __forceinline const char * view_data ( const TArray<uint8_t> & bytes ) {
        return bytes.data ? (const char *) bytes.data : "";
    }

    // every view entry sizes itself here: the find/slice family returns int offsets, so a view
    // past INT_MAX has no representable answer and stops like length() instead of wrapping
    static inline uint32_t view_size ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        if ( bytes.size > uint64_t(INT32_MAX) ) {
            context->throw_error_at(at, "byte view size %llu exceeds INT_MAX; string view operations return int offsets "
                "and are not supported on views this large", (unsigned long long)bytes.size);
        }
        return uint32_t(bytes.size);
    }

    // Every string entry below is stringLength + a length-bounded core, so the same core
    // serves a byte view that carries its own length and never scans for a terminator.
    // A bounded core treats an interior NUL as data.

    static bool ends_with_core ( const char * str, uint32_t strLen, const char * cmp, uint32_t cmpLen ) {
        return cmpLen == 0 || ((cmpLen <= strLen) && memcmp(&str[strLen - cmpLen], cmp, cmpLen) == 0);
    }

    static bool starts_with_core ( const char * str, uint32_t strLen, const char * cmp, uint32_t cmpLen ) {
        return cmpLen == 0 || ((cmpLen <= strLen) && memcmp(str, cmp, cmpLen) == 0);
    }

    static bool starts_with_at_core ( const char * str, uint32_t strLen, int32_t offset, const char * cmp, uint32_t cmpLen ) {
        if ( offset<0 || uint32_t(offset)>=strLen ) return false;
        return starts_with_core(str + offset, strLen - uint32_t(offset), cmp, cmpLen);
    }

    bool builtin_string_endswith ( const char * str, const char * cmp, Context * context ) {
        return ends_with_core(str, stringLength(*context, str), cmp, stringLength(*context, cmp));
    }

    bool builtin_string_startswith ( const char * str, const char * cmp, Context * context ) {
        return starts_with_core(str, stringLength(*context, str), cmp, stringLength(*context, cmp));
    }

    // das_string overload: prefix-test the das_string in place (no allocation),
    // sibling to builtin_string_ends_with. Lets AST/lint passes that hold names as
    // das_string do `name |> starts_with("...")` without materializing a string.
    bool builtin_string_starts_with ( const string & str, const char * cmp, Context * context ) {
        return starts_with_core(str.data(), uint32_t(str.length()), cmp, stringLength(*context, cmp));
    }

    bool builtin_string_startswith2 ( const char * str, const char * cmp, uint32_t cmpLen, Context * context ) {
        return starts_with_core(str, stringLength(*context, str), cmp, min(cmpLen, stringLength(*context, cmp)));
    }

    bool builtin_string_startswith3 ( const char * str, int32_t offset, const char * cmp, Context * context ) {
        return starts_with_at_core(str, stringLength(*context, str), offset, cmp, stringLength(*context, cmp));
    }

    bool builtin_string_startswith4 ( const char * str, int32_t offset, const char * cmp, uint32_t cmpLen, Context * context ) {
        return starts_with_at_core(str, stringLength(*context, str), offset, cmp, min(cmpLen, stringLength(*context, cmp)));
    }

    bool builtin_view_endswith ( const TArray<uint8_t> & bytes, const char * cmp, Context * context, LineInfoArg * at ) {
        return ends_with_core(view_data(bytes), view_size(bytes, context, at), cmp, stringLength(*context, cmp));
    }

    bool builtin_view_startswith ( const TArray<uint8_t> & bytes, const char * cmp, Context * context, LineInfoArg * at ) {
        return starts_with_core(view_data(bytes), view_size(bytes, context, at), cmp, stringLength(*context, cmp));
    }

    bool builtin_view_startswith2 ( const TArray<uint8_t> & bytes, const char * cmp, uint32_t cmpLen, Context * context, LineInfoArg * at ) {
        return starts_with_core(view_data(bytes), view_size(bytes, context, at), cmp, min(cmpLen, stringLength(*context, cmp)));
    }

    bool builtin_view_startswith3 ( const TArray<uint8_t> & bytes, int32_t offset, const char * cmp, Context * context, LineInfoArg * at ) {
        return starts_with_at_core(view_data(bytes), view_size(bytes, context, at), offset, cmp, stringLength(*context, cmp));
    }

    bool builtin_view_startswith4 ( const TArray<uint8_t> & bytes, int32_t offset, const char * cmp, uint32_t cmpLen, Context * context, LineInfoArg * at ) {
        return starts_with_at_core(view_data(bytes), view_size(bytes, context, at), offset, cmp, min(cmpLen, stringLength(*context, cmp)));
    }

    static inline const char* strip_l(const char *str, uint32_t len) {
        const char *t = str, *e = str + len;
        while (t != e && is_white_space(*t))
            t++;
        return t;
    }

    static inline const char* strip_r(const char *str, uint32_t len) {
        if (len == 0)
            return str;
        const char *t = &str[len-1];
        while (t >= str && is_white_space(*t))
            t--;
        return t + 1;
    }

    // the offset form of strip_l: a cursor walk answers "where does the content resume"
    // without materializing the remainder. from clamps into [0,len], so the answer is
    // always a valid offset, and len means "the tail from here is all whitespace"
    static int skip_white_space_core ( const char * str, uint32_t len, int from ) {
        uint32_t i = uint32_t(clamp_int(from, 0, int(len)));
        while ( i!=len && is_white_space(str[i]) ) i++;
        return int(i);
    }

    static char * strip_core ( const char * str, uint32_t strLen, Context * context, LineInfoArg * at ) {
        if (!strLen)
            return nullptr;
        const char *start = strip_l(str, strLen);
        const char *end = strip_r(str, strLen);
        return end > start ? context->allocateString(start, uint32_t(end-start), at) : nullptr;
    }

    static char * strip_left_core ( const char * str, uint32_t strLen, Context * context, LineInfoArg * at ) {
        if (!strLen)
            return nullptr;
        const char *start = strip_l(str, strLen);
        return uint32_t(start-str) < strLen ? context->allocateString(start, strLen-uint32_t(start-str), at) : nullptr;
    }

    static char * strip_right_core ( const char * str, uint32_t strLen, Context * context, LineInfoArg * at ) {
        if (!strLen)
            return nullptr;
        const char *end = strip_r(str, strLen);
        return end != str ? context->allocateString(str, uint32_t(end-str), at) : nullptr;
    }

    char* builtin_string_strip ( const char *str, Context * context, LineInfoArg * at ) {
        return strip_core(str, stringLength(*context, str), context, at);
    }

    char* builtin_string_strip_left ( const char *str, Context * context, LineInfoArg * at ) {
        return strip_left_core(str, stringLength(*context, str), context, at);
    }

    char* builtin_string_strip_right ( const char *str, Context * context, LineInfoArg * at ) {
        return strip_right_core(str, stringLength(*context, str), context, at);
    }

    char* builtin_view_strip ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        return strip_core(view_data(bytes), view_size(bytes, context, at), context, at);
    }

    char* builtin_view_strip_left ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        return strip_left_core(view_data(bytes), view_size(bytes, context, at), context, at);
    }

    char* builtin_view_strip_right ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        return strip_right_core(view_data(bytes), view_size(bytes, context, at), context, at);
    }

    int builtin_string_skip_white_space ( const char * str, int from, Context * context ) {
        return skip_white_space_core(str, stringLength(*context, str), from);
    }

    int builtin_view_skip_white_space ( const TArray<uint8_t> & bytes, int from, Context * context, LineInfoArg * at ) {
        return skip_white_space_core(view_data(bytes), view_size(bytes, context, at), from);
    }

    // memchr narrows to the candidate first byte, then one memcmp confirms the rest
    static int find_sub_core ( const char * str, uint32_t strLen, const char * sub, uint32_t subLen, uint32_t from ) {
        if ( !subLen || subLen>strLen ) return -1;
        const uint32_t last = strLen - subLen;
        if ( from > last ) return -1;
        const char * p = str + from;
        const char * lastP = str + last;
        while ( p <= lastP ) {
            auto f = memchr(p, int(uint8_t(sub[0])), size_t(lastP - p) + 1);
            if ( !f ) return -1;
            p = (const char *) f;
            if ( subLen==1 || memcmp(p+1, sub+1, subLen-1)==0 ) return int(p - str);
            p ++;
        }
        return -1;
    }

    // Ch is matched as an unsigned byte value; anything outside 0..255 is not a byte and
    // matches nothing (memchr alone would match it modulo 256, hence the explicit reject)
    static int find_char_core ( const char * str, uint32_t strLen, int Ch, uint32_t from ) {
        if ( Ch < 0 || Ch > 255 ) return -1;
        if ( from >= strLen ) return -1;
        auto f = memchr(str + from, Ch, strLen - from);
        return f ? int((const char *)f - str) : -1;
    }

    // a negative `from` is -1, not a clamp to 0 - "search backward from before the start" has no answer
    static int rfind_sub_core ( const char * str, uint32_t strLen, const char * sub, uint32_t subLen, int from ) {
        if ( from < 0 ) return -1;
        if ( !subLen || subLen>strLen ) return -1;
        const int last = int(strLen) - int(subLen);
        if ( from > last ) from = last;
        for ( int i = from; i >= 0; --i ) {
            if ( memcmp(str + i, sub, subLen) == 0 )
                return i;
        }
        return -1;
    }

    int builtin_string_find1 ( const char *str, const char *substr, int start, Context * context ) {
        const uint32_t strLen = stringLength ( *context, str );
        return find_sub_core(str, strLen, substr, stringLength ( *context, substr ),
            uint32_t(clamp_int(start, 0, int(strLen))));
    }

    int builtin_string_find2 (const char *str, const char *substr) {
        if (!str || !substr)
            return -1;
        return find_sub_core(str, uint32_t(strlen(str)), substr, uint32_t(strlen(substr)), 0);
    }

    int builtin_string_rfind1 ( const char *str, const char *substr, int start, Context * context ) {
        const uint32_t strLen = stringLength ( *context, str );
        return rfind_sub_core(str, strLen, substr, stringLength ( *context, substr ), start);
    }

    int builtin_string_rfind2 (const char *str, const char *substr) {
        if (!str || !substr)
            return -1;
        const uint32_t strLen = uint32_t(strlen(str));
        return rfind_sub_core(str, strLen, substr, uint32_t(strlen(substr)), int(strLen));
    }

    int builtin_view_find ( const TArray<uint8_t> & bytes, const char * substr, Context * context, LineInfoArg * at ) {
        return find_sub_core(view_data(bytes), view_size(bytes, context, at), substr, stringLength(*context, substr), 0);
    }

    int builtin_view_find_from ( const TArray<uint8_t> & bytes, const char * substr, int start, Context * context, LineInfoArg * at ) {
        const uint32_t len = view_size(bytes, context, at);
        return find_sub_core(view_data(bytes), len, substr, stringLength(*context, substr),
            uint32_t(clamp_int(start, 0, int(len))));
    }

    int builtin_view_rfind ( const TArray<uint8_t> & bytes, const char * substr, Context * context, LineInfoArg * at ) {
        const uint32_t len = view_size(bytes, context, at);
        return rfind_sub_core(view_data(bytes), len, substr, stringLength(*context, substr), int(len));
    }

    int builtin_view_rfind_from ( const TArray<uint8_t> & bytes, const char * substr, int start, Context * context, LineInfoArg * at ) {
        const uint32_t len = view_size(bytes, context, at);
        return rfind_sub_core(view_data(bytes), len, substr, stringLength(*context, substr), start);
    }

    static char * chop_core ( const char * str, uint32_t strLength, int start, int length, Context * context, LineInfoArg * at ) {
        if ( length<=0 ) return nullptr;
        const int32_t strLen = int32_t(strLength);
        if ( start < 0 ) start = 0;
        if ( start >= strLen ) return nullptr;
        if ( length > strLen - start ) length = strLen - start;
        return context->allocateString(str + start, length, at);
    }

    char* builtin_string_chop(const char* str, int start, int length, Context* context, LineInfoArg * at) {
        return chop_core(str, stringLength(*context, str), start, length, context, at);
    }

    char* builtin_view_chop ( const TArray<uint8_t> & bytes, int start, int length, Context* context, LineInfoArg * at ) {
        return chop_core(view_data(bytes), view_size(bytes, context, at), start, length, context, at);
    }

    // negative indices wrap from the end, then both ends clamp into [0,len] - the slice convention
    static inline void slice_range ( uint32_t strLen, int & start, int & end ) {
        start = clamp_int((start < 0) ? (strLen + start) : start, 0, strLen);
        end = clamp_int((end < 0) ? (strLen + end) : end, 0, strLen);
    }

    static char * slice_core ( const char * str, uint32_t strLen, int start, int end, Context * context, LineInfoArg * at ) {
        if (!strLen)
            return nullptr;
        slice_range(strLen, start, end);
        return end > start ? context->allocateString(str + start, uint32_t(end-start), at) : nullptr;
    }

    static char * slice_core ( const char * str, uint32_t strLen, int start, Context * context, LineInfoArg * at ) {
        if (!strLen)
            return nullptr;
        start = clamp_int((start < 0) ? (strLen + start) : start, 0, strLen);
        return strLen > uint32_t(start) ? context->allocateString(str + start, uint32_t(strLen-start), at) : nullptr;
    }

    char* builtin_string_slice1 ( const char *str, int start, int end, Context * context, LineInfoArg * at ) {
        return slice_core(str, stringLength ( *context, str ), start, end, context, at);
    }

    char* builtin_string_slice2 ( const char *str, int start, Context * context, LineInfoArg * at ) {
        return slice_core(str, stringLength ( *context, str ), start, context, at);
    }

    char* builtin_view_slice1 ( const TArray<uint8_t> & bytes, int start, int end, Context * context, LineInfoArg * at ) {
        return slice_core(view_data(bytes), view_size(bytes, context, at), start, end, context, at);
    }

    char* builtin_view_slice2 ( const TArray<uint8_t> & bytes, int start, Context * context, LineInfoArg * at ) {
        return slice_core(view_data(bytes), view_size(bytes, context, at), start, context, at);
    }

    char* builtin_string_reverse ( const char *str, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLength ( *context, str );
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
        const uint32_t strLen = stringLength ( *context, str );
        if (!strLen)
            return nullptr;
        char * ret = context->allocateString(nullptr, strLen, at);
        for (char *d = ret, *end = ret + strLen; d != end; ++str, ++d)
          *d = (char)to_lower(*str);
        context->stringHeap->intern(ret, strLen);
        return ret;
    }

    char* builtin_string_tolower_in_place(char* str) {
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
        const uint32_t strLen = stringLength ( *context, str );
        if (!strLen)
            return nullptr;
        char * ret = context->allocateString(nullptr, strLen, at);
        for (char *d = ret, *end = ret + strLen; d != end; ++str, ++d)
          *d = (char)to_upper(*str);
        context->stringHeap->intern(ret, strLen);
        return ret;
    }

    char* builtin_string_toupper_in_place ( char* str ) {
        char* pch = str;
        for (;;) {
            char ch = *pch;
            if (ch == 0) break;
            else if (ch >= 'a' && ch <= 'z') *pch = ch - 'a' + 'A';
            pch++;
        }
        return str;
    }

    int builtin_string_stricmp( const char *a, const char *b )
    {
        int d;
        for (;; ++a, ++b){
            d = to_lower(*a) - to_lower(*b);
            if ( d ) {
              return d;
            }
            if ( !*a || !*b ) {
              break;
            }
        }
        return d;
    }

    template <typename TT>
    TT string_to_int_number ( const char *str, Context * context, LineInfoArg * at ) {
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

    const char * das_to_cpp_float ( float val, Context * context, LineInfoArg * at ) {
        return context->allocateString(to_cpp_float(val), at);
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

    StringBuilderWriter & builtin_view_write_string ( StringBuilderWriter & writer, const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        const uint32_t len = view_size(bytes, context, at);
        if ( len ) writer.writeStr(view_data(bytes), size_t(len));
        return writer;
    }

    StringBuilderWriter & builtin_view_write_string_range ( StringBuilderWriter & writer, const TArray<uint8_t> & bytes, int start, int end, Context * context, LineInfoArg * at ) {
        const uint32_t len = view_size(bytes, context, at);
        if ( !len ) return writer;
        slice_range(len, start, end);
        if ( end > start ) writer.writeStr(view_data(bytes) + start, size_t(end - start));
        return writer;
    }

    StringBuilderWriter & write_escape_string ( StringBuilderWriter & writer, char * str ) {
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
        uint32_t len = stringLength ( *context, str );
        if ( !len || count<=0 ) return nullptr;
        char * res = context->allocateString(nullptr, uint64_t(len) * uint64_t(count), at);
        for ( char * s = res; count; count--, s+=len ) {
            memcpy ( s, str, len );
        }
        return res;
    }

    DAS_API vector<string> split ( const char * str, const char * delim ) {
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
        vector<const char *> tokens;
        vector<string> words;
        const char * ch = str;
        auto delimLen = stringLength(*context,delim);
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
            auto len = stringLength(*context,str);
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
        array_mark_locked(arr, (char *)tokens.data(), uint32_t(tokens.size()));
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(block, args, nullptr, at);
    }

    void builtin_string_split ( const char * str, const char * delim, const Block & block, Context * context, LineInfoArg * at ) {
        vector<const char *> tokens;
        vector<string> words;
        const char * ch = str;
        auto delimLen = stringLength(*context,delim);
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
            auto len = stringLength(*context,str);
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
        array_mark_locked(arr, (char *)tokens.data(), uint32_t(tokens.size()));
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(block, args, nullptr, at);
    }

    static char * replace_core ( const char * str, uint32_t strLen, const char * toSearch, uint32_t toSearchLen,
            const char * replaceStr, uint32_t replaceStrLen, Context * context, LineInfoArg * at ) {
        if ( !strLen ) return nullptr;
        if ( !toSearchLen ) return context->allocateString(str, strLen, at);
        int pos = find_sub_core(str, strLen, toSearch, toSearchLen, 0);
        if ( pos < 0 ) return context->allocateString(str, strLen, at);
        // append-only rebuild: an in-place replace() shifts the whole tail per occurrence,
        // which is quadratic in occurrence count whenever the replacement size differs
        string data;
        data.reserve(strLen);
        uint32_t begin = 0;
        while ( pos >= 0 ) {
            data.append(str + begin, uint32_t(pos) - begin);
            if ( replaceStrLen ) data.append(replaceStr, replaceStrLen);
            begin = uint32_t(pos) + toSearchLen;
            pos = find_sub_core(str, strLen, toSearch, toSearchLen, begin);
        }
        data.append(str + begin, strLen - begin);
        return context->allocateString(data, at);
    }

    char * builtin_string_replace ( const char * str, const char * toSearch, const char * replaceStr, Context * context, LineInfoArg * at ) {
        return replace_core(str, stringLength(*context, str),
            toSearch, stringLength(*context, toSearch),
            replaceStr, stringLength(*context, replaceStr), context, at);
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
        return context->allocateString(escapeString(str,false), at);
    }

    char * builtin_das_string_escape ( const string & str, Context * context, LineInfoArg * at ) {
        return context->allocateString(escapeString(str,false), at);
    }

    char * builtin_string_unescape ( const char *str, Context * context, LineInfoArg * at ) {
        bool err = false;
        auto estr = unescapeString(str, &err, false);
        if ( err ) context->throw_error_at(at, "invalid escape sequence");
        return context->allocateString(estr,at);
    }

    char * builtin_string_safe_unescape ( const char *str, Context * context, LineInfoArg * at ) {
        bool err = false;
        auto estr = unescapeString(str, &err, false);
        return context->allocateString(estr, at);
    }

    int builtin_find_first_char_of ( const char * str, int Ch, Context * context ) {
        return find_char_core(str, stringLength ( *context, str ), Ch, 0);
    }

    int builtin_find_first_char_of2 ( const char * str, int Ch, int start, Context * context ) {
        uint32_t strLen = stringLength ( *context, str );
        start = clamp_int((start < 0) ? (strLen + start) : start, 0, strLen);
        return find_char_core(str, strLen, Ch, uint32_t(start));
    }

    int builtin_view_find_char_of ( const TArray<uint8_t> & bytes, int Ch, Context * context, LineInfoArg * at ) {
        return find_char_core(view_data(bytes), view_size(bytes, context, at), Ch, 0);
    }

    int builtin_view_find_char_of2 ( const TArray<uint8_t> & bytes, int Ch, int start, Context * context, LineInfoArg * at ) {
        const uint32_t len = view_size(bytes, context, at);
        start = clamp_int((start < 0) ? (len + start) : start, 0, len);
        return find_char_core(view_data(bytes), len, Ch, uint32_t(start));
    }

    char * builtin_string_from_array ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        if ( !bytes.size ) return nullptr;
        return context->allocateString(bytes.data, view_size(bytes, context, at), at);
    }

    // the materializing inverse of string(array<uint8>): a fresh owned copy of the bytes, no
    // terminator appended, so string(to_bytes(s)) round-trips every NUL-free string
    static TArray<uint8_t> bytes_of ( const char * str, uint32_t len, Context * context, LineInfoArg * at ) {
        TArray<uint8_t> bytes;
        das_zero(bytes);
        if ( len ) {
            // exact reserve first: known final size, so the resize never grows - no pow2
            // slack on big strings, and no max_unreserved_size panic
            array_reserve(*context, bytes, len, uint32_t(sizeof(uint8_t)), at);
            array_resize(*context, bytes, len, uint32_t(sizeof(uint8_t)), false, at);
            memcpy(bytes.data, str, len);
        }
        return bytes;
    }

    TArray<uint8_t> builtin_string_to_bytes ( const char * str, Context * context, LineInfoArg * at ) {
        return bytes_of(str, stringLength(*context, str), context, at);
    }

    TArray<uint8_t> builtin_das_string_to_bytes ( const string & str, Context * context, LineInfoArg * at ) {
        return bytes_of(str.data(), uint32_t(str.size()), context, at);
    }

    bool delete_string ( char * & str, Context * context, LineInfoArg * at ) {
        if ( !str ) return false;
        uint32_t len = stringLengthSafe(*context, str);
        if ( context->freeString(str, len, at) ) {
            str = nullptr;
            return true;
        }
        return false;
    }

    void builtin_append_char_to_string(string & str, int32_t Ch) {
        str += char(Ch);
    }

    bool builtin_string_ends_with(const string &str, char * substr, Context * context ) {
        auto sz = str.length();
        auto slen = stringLength(*context,substr);
        if ( slen>sz ) return false;
        return memcmp ( str.data() + sz - slen, substr, slen )==0;
    }

    void builtin_resize_string(string & str, int32_t newLength) {
        str.resize(newLength);
    }

    // TODO: do we need a corresponding delete?
    char * builtin_reserve_string_buffer ( const char * str, int32_t length, Context * context ) {
        auto buf = context->allocate(length);
        if ( str ) {
            auto slen = min ( int32_t(strlen(str)), length-1 );
            memcpy ( buf, str, slen );
            buf[slen] = 0;
        } else {
            buf[0] = 0;
        }
        return buf;
    }

    bool is_char_in_string ( char c, const char * str ) {
        while ( *str ) {
            if ( *str++==c ) return true;
        }
        return false;
    }

    static char * trim_core ( const char * str, uint32_t len, Context * context, LineInfoArg * at ) {
        if ( !len ) return nullptr;
        const char * b = str, * e = str + len;
        while ( b!=e && is_white_space(*b) ) b++;
        while ( e>b && is_white_space(e[-1]) ) e--;
        return e>b ? context->allocateString(b, uint32_t(e-b), at) : nullptr;
    }

    static char * ltrim_core ( const char * str, uint32_t len, Context * context, LineInfoArg * at ) {
        if ( !len ) return nullptr;
        const char * b = str, * e = str + len;
        while ( b!=e && is_white_space(*b) ) b++;
        return e>b ? context->allocateString(b, uint32_t(e-b), at) : nullptr;
    }

    static char * rtrim_core ( const char * str, uint32_t len, Context * context, LineInfoArg * at ) {
        if ( !len ) return nullptr;
        const char * e = str + len;
        while ( e>str && is_white_space(e[-1]) ) e--;
        return e>str ? context->allocateString(str, uint32_t(e-str), at) : nullptr;
    }

    static char * rtrim_chars_core ( const char * str, uint32_t len, const char * chars, Context * context, LineInfoArg * at ) {
        if ( !len ) return nullptr;
        const char * e = str + len;
        while ( e>str && is_char_in_string(e[-1],chars) ) e--;
        return e>str ? context->allocateString(str, uint32_t(e-str), at) : nullptr;
    }

    char * builtin_string_trim ( char* s, Context * context, LineInfoArg * at ) {
        return trim_core(s, stringLength(*context, s), context, at);
    }

    char * builtin_string_ltrim ( char* s, Context * context, LineInfoArg * at ) {
        return ltrim_core(s, stringLength(*context, s), context, at);
    }

    char * builtin_string_rtrim ( char* s, Context * context, LineInfoArg * at ) {
        return rtrim_core(s, stringLength(*context, s), context, at);
    }

    char * builtin_string_rtrim_ts ( char* s, char * ts, Context * context, LineInfoArg * at ) {
        return rtrim_chars_core(s, stringLength(*context, s), ts ? ts : "", context, at);
    }

    char * builtin_view_trim ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        return trim_core(view_data(bytes), view_size(bytes, context, at), context, at);
    }

    char * builtin_view_ltrim ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        return ltrim_core(view_data(bytes), view_size(bytes, context, at), context, at);
    }

    char * builtin_view_rtrim ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at ) {
        return rtrim_core(view_data(bytes), view_size(bytes, context, at), context, at);
    }

    char * builtin_view_rtrim_ts ( const TArray<uint8_t> & bytes, char * ts, Context * context, LineInfoArg * at ) {
        return rtrim_chars_core(view_data(bytes), view_size(bytes, context, at), ts ? ts : "", context, at);
    }

    void builtin_string_peek ( const char * str, const TBlock<void,TTemporary<TArray<uint8_t> const>> & block, Context * context, LineInfoArg * at ) {
        Array arr;
        array_mark_locked(arr, (char *)str, uint32_t(strlen(str)));
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(block, args, nullptr, at);
    }

    char * builtin_string_peek_and_modify ( const char * str, const TBlock<void,TTemporary<TArray<uint8_t>>> & block, Context * context, LineInfoArg * at ) {
        int32_t len = int32_t(strlen(str));
        char * cstr = context->allocateString(str, len, at);
        Array arr;
        array_mark_locked(arr, cstr, uint32_t(len));
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

    // both cores write `result` on success as well as on failure, and leave `offset` at the
    // starting position whenever the parse failed - the string and byte-view entries must agree
    // on both. The string entries always start at 0; a view entry starts wherever its caller's
    // cursor is, which is what makes the view `offset` an in/out parameter.
    template <typename TT>
    TT convert_real_core ( const char * str, uint32_t len, ConversionResult & result, int32_t & offset, uint32_t from = 0 ) {
        offset = int32_t(from);
        if ( !str ) {
            result = ConversionResult::invalid_argument;
            return TT();
        }
        const char * b = str + from, * e = str + len;
        while ( b!=e && is_white_space(*b) ) b++;
        TT value = 0;
        auto res = fast_float::from_chars(b, e, value);
        if (res.ec != std::errc()) {
            result = ConversionResult(res.ec);
            return TT();
        }
        result = ConversionResult::ok;
        offset = int32_t(res.ptr - str);
        return value;
    }

    template <typename TT>
    TT convert_int_core ( const char * str, uint32_t len, ConversionResult & result, int32_t & offset, bool hex, uint32_t from = 0 ) {
        offset = int32_t(from);
        if ( !str ) {
            result = ConversionResult::invalid_argument;
            return TT();
        }
        const char * b = str + from, * e = str + len;
        while ( b!=e && is_white_space(*b) ) b++;
        if ( hex && (e-b)>=2 && b[0]=='0' && (b[1]=='x' || b[1]=='X') ) b += 2;
        TT value = 0;
        auto res = fast_float::from_chars(b, e, value, hex ? 16 : 10);
        if (res.ec != std::errc()) {
            result = ConversionResult(res.ec);
            return TT();
        }
        result = ConversionResult::ok;
        offset = int32_t(res.ptr - str);
        return value;
    }

    template <typename TT>
    TT convert_from_string ( const char * str, ConversionResult & result, int32_t & offset ) {
        return convert_real_core<TT>(str, str ? uint32_t(strlen(str)) : 0, result, offset);
    }

    template <typename TT>
    TT convert_int_from_string ( const char * str, ConversionResult & result, int32_t & offset, bool hex ) {
        return convert_int_core<TT>(str, str ? uint32_t(strlen(str)) : 0, result, offset, hex);
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

    // the view forms take `offset` in and out: in is where to start parsing, out is where the
    // parse stopped. A start outside [0,size] is rejected without touching the cursor.
    template <typename TT>
    TT convert_real_from_view ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, Context * context, LineInfoArg * at ) {
        const uint32_t len = view_size(bytes, context, at);
        if ( offset<0 || uint32_t(offset)>len ) {
            result = ConversionResult::invalid_argument;
            return TT();
        }
        return convert_real_core<TT>(view_data(bytes), len, result, offset, uint32_t(offset));
    }

    template <typename TT>
    TT convert_int_from_view ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, bool hex, Context * context, LineInfoArg * at ) {
        const uint32_t len = view_size(bytes, context, at);
        if ( offset<0 || uint32_t(offset)>len ) {
            result = ConversionResult::invalid_argument;
            return TT();
        }
        return convert_int_core<TT>(view_data(bytes), len, result, offset, hex, uint32_t(offset));
    }

    int8_t convert_from_view_int8 ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, bool hex, Context * context, LineInfoArg * at ) {
        return convert_int_from_view<int8_t>(bytes, result, offset, hex, context, at);
    }

    uint8_t convert_from_view_uint8 ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, bool hex, Context * context, LineInfoArg * at ) {
        return convert_int_from_view<uint8_t>(bytes, result, offset, hex, context, at);
    }

    int16_t convert_from_view_int16 ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, bool hex, Context * context, LineInfoArg * at ) {
        return convert_int_from_view<int16_t>(bytes, result, offset, hex, context, at);
    }

    uint16_t convert_from_view_uint16 ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, bool hex, Context * context, LineInfoArg * at ) {
        return convert_int_from_view<uint16_t>(bytes, result, offset, hex, context, at);
    }

    int32_t convert_from_view_int32 ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, bool hex, Context * context, LineInfoArg * at ) {
        return convert_int_from_view<int32_t>(bytes, result, offset, hex, context, at);
    }

    uint32_t convert_from_view_uint32 ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, bool hex, Context * context, LineInfoArg * at ) {
        return convert_int_from_view<uint32_t>(bytes, result, offset, hex, context, at);
    }

    int64_t convert_from_view_int64 ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, bool hex, Context * context, LineInfoArg * at ) {
        return convert_int_from_view<int64_t>(bytes, result, offset, hex, context, at);
    }

    uint64_t convert_from_view_uint64 ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, bool hex, Context * context, LineInfoArg * at ) {
        return convert_int_from_view<uint64_t>(bytes, result, offset, hex, context, at);
    }

    float convert_from_view_float ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, Context * context, LineInfoArg * at ) {
        return convert_real_from_view<float>(bytes, result, offset, context, at);
    }

    double convert_from_view_double ( const TArray<uint8_t> & bytes, ConversionResult & result, int32_t & offset, Context * context, LineInfoArg * at ) {
        return convert_real_from_view<double>(bytes, result, offset, context, at);
    }

    class Module_Strings : public Module {
    public:
        Module_Strings() : Module("strings") {
            DAS_PROFILE_SECTION("Module_Strings");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            // string builder writer
            addEnumeration(new EnumerationConversionResult());
            addAnnotation(new StringBuilderWriterAnnotation(lib));
            addExternInline<DAS_BIND_FUN(delete_string)>(*this, lib, "delete_string",
                SideEffects::modifyArgumentAndExternal,"delete_string")->args({"str","context","lineinfo"})->unsafeOperation = true;
            addExternInline<DAS_BIND_FUN(builtin_build_string)>(*this, lib, "build_string",
                SideEffects::modifyExternal,"builtin_build_string_T")->args({"block","context","lineinfo"})->setAotTemplate()->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_build_hash)>(*this, lib, "build_hash",
                SideEffects::modifyExternal,"builtin_build_hash_T")->args({"block","context","lineinfo"})->setAotTemplate();
            addExternInline<DAS_BIND_FUN(builtin_string_peek)>(*this, lib, "peek_data",
                SideEffects::modifyExternal,"builtin_string_peek")->args({"str","block","context","lineinfo"});
            addExternInline<DAS_BIND_FUN(builtin_string_peek_and_modify)>(*this, lib, "modify_data",
                SideEffects::modifyExternal,"builtin_string_peek_and_modify")->args({"str","block","context","lineinfo"})->setTempStringResult();
            addInterop<builtin_write_string,StringBuilderWriter &,StringBuilderWriter,vec4f> (*this, lib, "write",
                SideEffects::modifyExternal, "builtin_write_string")->args({"writer","anything"});
            addExtern<DAS_BIND_FUN(write_string_char),SimNode_ExtFuncCallRef>(*this, lib, "write_char",
                SideEffects::modifyExternal, "write_string_char")->args({"writer","ch"});
            addExtern<DAS_BIND_FUN(write_string_chars),SimNode_ExtFuncCallRef>(*this, lib, "write_chars",
                SideEffects::modifyExternal, "write_string_chars")->args({"writer","ch","count"});
            addExtern<DAS_BIND_FUN(write_escape_string),SimNode_ExtFuncCallRef>(*this, lib, "write_escape_string",
                SideEffects::modifyExternal, "write_escape_string")->args({"writer","str"});
            addExtern<DAS_BIND_FUN(builtin_view_write_string),SimNode_ExtFuncCallRef>(*this, lib, "write_string",
                SideEffects::modifyExternal, "builtin_view_write_string")->args({"writer","bytes","context","at"});
            addExtern<DAS_BIND_FUN(builtin_view_write_string_range),SimNode_ExtFuncCallRef>(*this, lib, "write_string",
                SideEffects::modifyExternal, "builtin_view_write_string_range")->args({"writer","bytes","start","end","context","at"});
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
            addExternInline<DAS_BIND_FUN(builtin_string_from_array)>(*this, lib, "string",
                SideEffects::none, "builtin_string_from_array")->args({"bytes","context","at"})->setTempStringResult();
            addExtern<DAS_BIND_FUN(builtin_string_to_bytes),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "to_bytes",
                SideEffects::none, "builtin_string_to_bytes")->args({"str","context","at"});
            addExtern<DAS_BIND_FUN(builtin_das_string_to_bytes),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "to_bytes",
                SideEffects::none, "builtin_das_string_to_bytes")->args({"str","context","at"});
            // dup
            addInterop<builtin_strdup,void,vec4f> (*this, lib, "builtin_strdup",
                SideEffects::modifyArgumentAndExternal, "builtin_strdup")->arg("anything")->unsafeOperation = true;
            // regular string
            addExternInline<DAS_BIND_FUN(get_character_at)>(*this, lib, "character_at",
                SideEffects::none, "get_character_at")->args({"str","idx","context","at"});
            addExternInline<DAS_BIND_FUN(get_character_uat)>(*this, lib, "character_uat",
                SideEffects::none, "get_character_uat")->args({"str","idx"})->unsafeOperation = true;
            addExternInline<DAS_BIND_FUN(get_first_character)>(*this, lib, "first_character",
                SideEffects::none, "get_first_character")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(get_first_character_ds)>(*this, lib, "first_character",
                SideEffects::none, "get_first_character_ds")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(with_das_string)>(*this, lib, "with_das_string",
                SideEffects::invoke, "with_das_string")->args({"block","context","at"});
            addExternInline<DAS_BIND_FUN(string_repeat)>(*this, lib, "repeat",
                SideEffects::none, "string_repeat")->args({"str","count","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(to_string_char)>(*this, lib, "to_char",
                SideEffects::none, "to_string_char")->args({"char","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_endswith)>(*this, lib, "ends_with",
                SideEffects::none, "builtin_string_endswith")->args({"str","cmp","context"});
            addExternInline<DAS_BIND_FUN(builtin_string_ends_with)>(*this, lib, "ends_with",
                SideEffects::none, "builtin_string_ends_with")->args({"str","cmp","context"});
            addExternInline<DAS_BIND_FUN(builtin_string_startswith)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_string_startswith")->args({"str","cmp","context"});
            addExternInline<DAS_BIND_FUN(builtin_string_startswith2)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_string_startswith2")->args({"str","cmp","cmpLen","context"});
            addExternInline<DAS_BIND_FUN(builtin_string_startswith3)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_string_startswith3")->args({"str","offset","cmp","context"});
            addExternInline<DAS_BIND_FUN(builtin_string_startswith4)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_string_startswith4")->args({"str","offset","cmp","cmpLen","context"});
            addExternInline<DAS_BIND_FUN(builtin_string_starts_with)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_string_starts_with")->args({"str","cmp","context"});
            // byte-view twins: the view param is array<uint8> const implicit, so it binds both
            // a plain array<uint8> and the temporary view peek_data yields
            addExternInline<DAS_BIND_FUN(builtin_view_endswith)>(*this, lib, "ends_with",
                SideEffects::none, "builtin_view_endswith")->args({"bytes","cmp","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_view_startswith)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_view_startswith")->args({"bytes","cmp","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_view_startswith2)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_view_startswith2")->args({"bytes","cmp","cmpLen","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_view_startswith3)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_view_startswith3")->args({"bytes","offset","cmp","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_view_startswith4)>(*this, lib, "starts_with",
                SideEffects::none, "builtin_view_startswith4")->args({"bytes","offset","cmp","cmpLen","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_string_strip)>(*this, lib, "strip",
                SideEffects::none, "builtin_string_strip")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_strip_right)>(*this, lib, "strip_right",
                SideEffects::none, "builtin_string_strip_right")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_strip_left)>(*this, lib, "strip_left",
                SideEffects::none, "builtin_string_strip_left")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_strip)>(*this, lib, "strip",
                SideEffects::none, "builtin_view_strip")->args({"bytes","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_strip_right)>(*this, lib, "strip_right",
                SideEffects::none, "builtin_view_strip_right")->args({"bytes","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_strip_left)>(*this, lib, "strip_left",
                SideEffects::none, "builtin_view_strip_left")->args({"bytes","context","at"})->setTempStringResult();
            // strip_left as a cursor: the offset where content resumes, nothing allocated
            addExternInline<DAS_BIND_FUN(builtin_string_skip_white_space)>(*this, lib, "skip_white_space",
                SideEffects::none, "builtin_string_skip_white_space")->args({"str","from","context"});
            addExternInline<DAS_BIND_FUN(builtin_view_skip_white_space)>(*this, lib, "skip_white_space",
                SideEffects::none, "builtin_view_skip_white_space")->args({"bytes","from","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_string_chop)>(*this, lib, "chop",
                SideEffects::none, "builtin_string_chop")->args({"str","start","length","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_chop)>(*this, lib, "chop",
                SideEffects::none, "builtin_view_chop")->args({"bytes","start","length","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_slice1)>(*this, lib, "slice",
                SideEffects::none, "builtin_string_slice1")->args({"str","start","end","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_slice2)>(*this, lib, "slice",
                SideEffects::none, "builtin_string_slice2")->args({"str","start","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_slice1)>(*this, lib, "slice",
                SideEffects::none, "builtin_view_slice1")->args({"bytes","start","end","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_slice2)>(*this, lib, "slice",
                SideEffects::none, "builtin_view_slice2")->args({"bytes","start","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_find1)>(*this, lib, "find",
                SideEffects::none, "builtin_string_find1")->args({"str","substr","start","context"});
            addExternInline<DAS_BIND_FUN(builtin_string_find2)>(*this, lib, "find",
                SideEffects::none, "builtin_string_find2")->args({"str","substr"});
            addExternInline<DAS_BIND_FUN(builtin_find_first_char_of)>(*this, lib, "find",
                SideEffects::none, "builtin_find_first_char_of")->args({"str","substr","context"});
            addExternInline<DAS_BIND_FUN(builtin_find_first_char_of2)>(*this, lib, "find",
                SideEffects::none, "builtin_find_first_char_of2")->args({"str","substr", "start", "context"});
            addExternInline<DAS_BIND_FUN(builtin_view_find)>(*this, lib, "find",
                SideEffects::none, "builtin_view_find")->args({"bytes","substr","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_view_find_from)>(*this, lib, "find",
                SideEffects::none, "builtin_view_find_from")->args({"bytes","substr","start","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_view_find_char_of)>(*this, lib, "find",
                SideEffects::none, "builtin_view_find_char_of")->args({"bytes","substr","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_view_find_char_of2)>(*this, lib, "find",
                SideEffects::none, "builtin_view_find_char_of2")->args({"bytes","substr","start","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_string_rfind1)>(*this, lib, "rfind",
                SideEffects::none, "builtin_string_rfind1")->args({"str","substr","start","context"});
            addExternInline<DAS_BIND_FUN(builtin_string_rfind2)>(*this, lib, "rfind",
                SideEffects::none, "builtin_string_rfind2")->args({"str","substr"});
            addExternInline<DAS_BIND_FUN(builtin_view_rfind)>(*this, lib, "rfind",
                SideEffects::none, "builtin_view_rfind")->args({"bytes","substr","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_view_rfind_from)>(*this, lib, "rfind",
                SideEffects::none, "builtin_view_rfind_from")->args({"bytes","substr","start","context","at"});
            addExternInline<DAS_BIND_FUN(builtin_string_reverse)>(*this, lib, "reverse",
                SideEffects::none, "builtin_string_reverse")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_append_char_to_string)>(*this, lib, "append",
                SideEffects::modifyArgumentAndExternal, "builtin_append_char_to_string")->args({"str","ch"});
            addExternInline<DAS_BIND_FUN(builtin_resize_string)>(*this, lib, "resize",
                SideEffects::modifyArgumentAndExternal, "builtin_resize_string")->args({"str","new_length"});
            addExternInline<DAS_BIND_FUN(builtin_string_toupper)>(*this, lib, "to_upper",
                SideEffects::none, "builtin_string_toupper")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_tolower)>(*this, lib, "to_lower",
                SideEffects::none, "builtin_string_tolower")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_split_by_char)>(*this, lib, "builtin_string_split_by_char",
                SideEffects::modifyExternal, "builtin_string_split_by_char")->args({"str","delimiter","block","context","lineinfo"});
            addExternInline<DAS_BIND_FUN(builtin_string_split)>(*this, lib, "builtin_string_split",
                SideEffects::modifyExternal, "builtin_string_split")->args({"str","delimiter","block","context","lineinfo"});
            // conversion which throws exception on error. detects hex automatically
            addExternInline<DAS_BIND_FUN(string_to_int8)>(*this, lib, "int8",
                SideEffects::none, "string_to_int8")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(string_to_uint8)>(*this, lib, "uint8",
                SideEffects::none, "string_to_uint8")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(string_to_int16)>(*this, lib, "int16",
                SideEffects::none, "string_to_int16")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(string_to_uint16)>(*this, lib, "uint16",
                SideEffects::none, "string_to_uint16")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(string_to_int)>(*this, lib, "int",
                SideEffects::none, "string_to_int")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(string_to_uint)>(*this, lib, "uint",
                SideEffects::none, "string_to_uint")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(string_to_int64)>(*this, lib, "int64",
                SideEffects::none, "string_to_int64")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(string_to_uint64)>(*this, lib, "uint64",
                SideEffects::none, "string_to_uint64")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(string_to_float)>(*this, lib, "float",
                SideEffects::none, "string_to_float")->args({"str","context","at"});
            addExternInline<DAS_BIND_FUN(string_to_double)>(*this, lib, "double",
                SideEffects::none, "string_to_double")->args({"str","context","at"});
            // fast conversion, returns 0 if fails
            addExternInline<DAS_BIND_FUN(fast_to_int8)>(*this, lib, "to_int8",
                SideEffects::none, "fast_to_int8")->args({"value","hex"})->arg_init(1,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(fast_to_uint8)>(*this, lib, "to_uint8",
                SideEffects::none, "fast_to_uint8")->args({"value","hex"})->arg_init(1,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(fast_to_int16)>(*this, lib, "to_int16",
                SideEffects::none, "fast_to_int16")->args({"value","hex"})->arg_init(1,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(fast_to_uint16)>(*this, lib, "to_uint16",
                SideEffects::none, "fast_to_uint16")->args({"value","hex"})->arg_init(1,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(fast_to_int)>(*this, lib, "to_int",
                SideEffects::none, "fast_to_int")->args({"value","hex"})->arg_init(1,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(fast_to_uint)>(*this, lib, "to_uint",
                SideEffects::none, "fast_to_uint")->args({"value","hex"})->arg_init(1,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(fast_to_int64)>(*this, lib, "to_int64",
                SideEffects::none, "fast_to_int64")->args({"value","hex"})->arg_init(1,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(fast_to_uint64)>(*this, lib, "to_uint64",
                SideEffects::none, "fast_to_uint64")->args({"value","hex"})->arg_init(1,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(das_to_cpp_float)>(*this, lib, "to_cpp_float",
                SideEffects::modifyExternal, "das_to_cpp_float")->args({"value","context", "at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(fast_to_float)>(*this, lib, "to_float",
                SideEffects::none, "fast_to_float")->arg("value");
            addExternInline<DAS_BIND_FUN(fast_to_double)>(*this, lib, "to_double",
                SideEffects::none, "fast_to_double")->arg("value");
            // conversion which returns error and offset of the first invalid character
            addExternInline<DAS_BIND_FUN(convert_from_string_int8)>(*this, lib, "int8",
                SideEffects::modifyArgument, "convert_from_string_int8")->args({"str","result","offset","hex"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_string_uint8)>(*this, lib, "uint8",
                SideEffects::modifyArgument, "convert_from_string_uint8")->args({"str","result","offset","hex"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_string_int16)>(*this, lib, "int16",
                SideEffects::modifyArgument, "convert_from_string_int16")->args({"str","result","offset","hex"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_string_uint16)>(*this, lib, "uint16",
                SideEffects::modifyArgument, "convert_from_string_uint16")->args({"str","result","offset","hex"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_string_int32)>(*this, lib, "int",
                SideEffects::modifyArgument, "convert_from_string_int32")->args({"str","result","offset","hex"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_string_uint32)>(*this, lib, "uint",
                SideEffects::modifyArgument, "convert_from_string_uint32")->args({"str","result","offset","hex"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_string_int64)>(*this, lib, "int64",
                SideEffects::modifyArgument, "convert_from_string_int64")->args({"str","result","offset","hex"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_string_uint64)>(*this, lib, "uint64",
                SideEffects::modifyArgument, "convert_from_string_uint64")->args({"str","result","offset","hex"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_string_float)>(*this, lib, "float",
                SideEffects::modifyArgument, "convert_from_string_float")->args({"str","result","offset"});
            addExternInline<DAS_BIND_FUN(convert_from_string_double)>(*this, lib, "double",
                SideEffects::modifyArgument, "convert_from_string_double")->args({"str","result","offset"});
            // byte-view twins of the conversion family - here `offset` is in and out, so the
            // same call can walk a view left to right
            addExternInline<DAS_BIND_FUN(convert_from_view_int8)>(*this, lib, "int8",
                SideEffects::modifyArgument, "convert_from_view_int8")->args({"bytes","result","offset","hex","context","at"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_view_uint8)>(*this, lib, "uint8",
                SideEffects::modifyArgument, "convert_from_view_uint8")->args({"bytes","result","offset","hex","context","at"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_view_int16)>(*this, lib, "int16",
                SideEffects::modifyArgument, "convert_from_view_int16")->args({"bytes","result","offset","hex","context","at"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_view_uint16)>(*this, lib, "uint16",
                SideEffects::modifyArgument, "convert_from_view_uint16")->args({"bytes","result","offset","hex","context","at"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_view_int32)>(*this, lib, "int",
                SideEffects::modifyArgument, "convert_from_view_int32")->args({"bytes","result","offset","hex","context","at"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_view_uint32)>(*this, lib, "uint",
                SideEffects::modifyArgument, "convert_from_view_uint32")->args({"bytes","result","offset","hex","context","at"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_view_int64)>(*this, lib, "int64",
                SideEffects::modifyArgument, "convert_from_view_int64")->args({"bytes","result","offset","hex","context","at"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_view_uint64)>(*this, lib, "uint64",
                SideEffects::modifyArgument, "convert_from_view_uint64")->args({"bytes","result","offset","hex","context","at"})->arg_init(3,new ExprConstBool(false));
            addExternInline<DAS_BIND_FUN(convert_from_view_float)>(*this, lib, "float",
                SideEffects::modifyArgument, "convert_from_view_float")->args({"bytes","result","offset","context","at"});
            addExternInline<DAS_BIND_FUN(convert_from_view_double)>(*this, lib, "double",
                SideEffects::modifyArgument, "convert_from_view_double")->args({"bytes","result","offset","context","at"});
            // escaping etc
            addExternInline<DAS_BIND_FUN(builtin_string_escape)>(*this, lib, "escape",
                SideEffects::none, "builtin_string_escape")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_das_string_escape)>(*this, lib, "escape",
                SideEffects::none, "builtin_das_string_escape")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_unescape)>(*this, lib, "unescape",
                SideEffects::none, "builtin_string_unescape")->args({"str","context", "at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_safe_unescape)>(*this, lib, "safe_unescape",
                SideEffects::none, "builtin_string_safe_unescape")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_replace)>(*this, lib, "replace",
                SideEffects::none, "builtin_string_replace")->args({"str","toSearch","replace","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_rtrim)>(*this, lib, "rtrim",
                SideEffects::none, "builtin_string_rtrim")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_rtrim_ts)>(*this, lib, "rtrim",
                SideEffects::none, "builtin_string_rtrim_ts")->args({"str","chars","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_ltrim)>(*this, lib, "ltrim",
                SideEffects::none, "builtin_string_ltrim")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_string_trim)>(*this, lib, "trim",
                SideEffects::none, "builtin_string_trim")->args({"str","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_rtrim)>(*this, lib, "rtrim",
                SideEffects::none, "builtin_view_rtrim")->args({"bytes","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_rtrim_ts)>(*this, lib, "rtrim",
                SideEffects::none, "builtin_view_rtrim_ts")->args({"bytes","chars","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_ltrim)>(*this, lib, "ltrim",
                SideEffects::none, "builtin_view_ltrim")->args({"bytes","context","at"})->setTempStringResult();
            addExternInline<DAS_BIND_FUN(builtin_view_trim)>(*this, lib, "trim",
                SideEffects::none, "builtin_view_trim")->args({"bytes","context","at"})->setTempStringResult();
            // format (deprecated)
            addExternInline<DAS_BIND_FUN(format<int32_t>)> (*this, lib, "format",
                SideEffects::none, "format<int32_t>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead")->setTempStringResult();
            addExternInline<DAS_BIND_FUN(format<uint32_t>)>(*this, lib, "format",
                SideEffects::none, "format<uint32_t>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead")->setTempStringResult();
            addExternInline<DAS_BIND_FUN(format<int64_t>)> (*this, lib, "format",
                SideEffects::none, "format<int64_t>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead")->setTempStringResult();
            addExternInline<DAS_BIND_FUN(format<uint64_t>)>(*this, lib, "format",
                SideEffects::none, "format<uint64_t>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead")->setTempStringResult();
            addExternInline<DAS_BIND_FUN(format<float>)>   (*this, lib, "format",
                SideEffects::none, "format<float>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead")->setTempStringResult();
            addExternInline<DAS_BIND_FUN(format<double>)>  (*this, lib, "format",
                SideEffects::none, "format<double>")->args({"format","value","context","at"})->setDeprecated("use fmt() instead")->setTempStringResult();
            // queries
            addExternInline<DAS_BIND_FUN(is_alpha)> (*this, lib, "is_alpha",
                SideEffects::none, "is_alpha")->arg("Character");
            addExternInline<DAS_BIND_FUN(is_alnum)> (*this, lib, "is_alnum",
                SideEffects::none, "is_alnum")->arg("Character");
            addExternInline<DAS_BIND_FUN(is_hex)>(*this, lib, "is_hex",
                SideEffects::none, "is_hex")->args({"Character"});
            addExternInline<DAS_BIND_FUN(is_tab_or_space)>(*this, lib, "is_tab_or_space",
                SideEffects::none, "is_tab_or_space")->args({"Character"});
            addExternInline<DAS_BIND_FUN(is_new_line)> (*this, lib, "is_new_line",
                SideEffects::none, "is_new_line")->arg("Character");
            addExternInline<DAS_BIND_FUN(is_white_space)> (*this, lib, "is_white_space",
                SideEffects::none, "is_white_space")->arg("Character");
            addExternInline<DAS_BIND_FUN(is_number)> (*this, lib, "is_number",
                SideEffects::none, "is_number")->arg("Character");
            // bitset helpers
            addExternInline<DAS_BIND_FUN(is_char_in_set)>(*this, lib, "is_char_in_set",
                SideEffects::none,"is_char_in_set")->args({"Character","Charset","Context","At"});
            addExternInline<DAS_BIND_FUN(char_set_total)>(*this, lib, "set_total",
                SideEffects::none,"char_set_total")->arg("Charset");
            addExternInline<DAS_BIND_FUN(char_set_element)>(*this, lib, "set_element",
                SideEffects::none,"char_set_element")->args({"Character","Charset"});
            // case-insensitive comparison
            addExternInline<DAS_BIND_FUN(builtin_string_stricmp)>(*this, lib, "compare_ignore_case",
                SideEffects::none, "builtin_string_stricmp")->args({"a","b"});
            // string buffer
            addExternInline<DAS_BIND_FUN(builtin_reserve_string_buffer)>(*this, lib, "reserve_string_buffer",
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
