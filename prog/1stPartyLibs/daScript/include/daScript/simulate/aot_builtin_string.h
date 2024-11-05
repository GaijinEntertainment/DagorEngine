#pragma once

#include "daScript/ast/ast_typefactory.h"

namespace das {

    enum class ConversionResult {
        ok
    ,   invalid_argument = int(std::errc::invalid_argument)  // fast_float.h
    ,   out_of_range = int(std::errc::result_out_of_range)   // fast_float.h
    };

    void delete_string ( char * & str, Context * context, LineInfoArg * at );

    char * builtin_build_string ( const TBlock<void,StringBuilderWriter> & block, Context * context, LineInfoArg * lineinfo );
    uint64_t builtin_build_hash ( const TBlock<void,StringBuilderWriter> & block, Context * context, LineInfoArg * at );
    vec4f builtin_write_string ( Context & context, SimNode_CallBase * call, vec4f * args );

    void builtin_string_peek ( const char * str, const TBlock<void,TTemporary<TArray<uint8_t> const>> & block, Context * context, LineInfoArg * lineinfo );
    char * builtin_string_peek_and_modify ( const char * str, const TBlock<void,TTemporary<TArray<uint8_t>>> & block, Context * context, LineInfoArg * lineinfo );

    char * builtin_string_escape ( const char *str, Context * context, LineInfoArg * at );
    char * builtin_string_unescape ( const char *str, Context * context, LineInfoArg * at );
    char * builtin_string_safe_unescape ( const char *str, Context * context, LineInfoArg * at );

    vec4f builtin_strdup ( Context &, SimNode_CallBase * call, vec4f * args );

    int32_t get_character_at ( const char * str, int32_t index, Context * context, LineInfoArg * at );

    bool builtin_string_endswith ( const char * str, const char * cmp, Context * context );
    bool builtin_string_startswith ( const char * str, const char * cmp, Context * context );
    bool builtin_string_startswith2 ( const char * str, const char * cmp, uint32_t cmpLen, Context * context );
    bool builtin_string_startswith3 ( const char * str, int32_t offset, const char * cmp, Context * context );
    bool builtin_string_startswith4 ( const char * str, int32_t offset, const char * cmp, uint32_t cmpLen, Context * context );
    char* builtin_string_strip ( const char *str, Context * context, LineInfoArg * at );
    char* builtin_string_strip_left ( const char *str, Context * context, LineInfoArg * at );
    char* builtin_string_strip_right ( const char *str, Context * context, LineInfoArg * at );
    int builtin_string_find1 ( const char *str, const char *substr, int start, Context * context );
    int builtin_string_find2 (const char *str, const char *substr);
    int builtin_find_first_of ( const char * str, const char * substr, Context * context );
    int builtin_find_first_char_of ( const char * str, int Ch, Context * context );
    int builtin_find_first_char_of2 ( const char * str, int Ch, int start, Context * context );
    int builtin_string_length ( const char *str, Context * context );
    char* builtin_string_slice1 ( const char *str, int start, int end, Context * context, LineInfoArg * at );
    char* builtin_string_slice2 ( const char *str, int start, Context * context, LineInfoArg * at );
    char* builtin_string_reverse ( const char *str, Context * context, LineInfoArg * at );
    char* builtin_string_tolower ( const char *str, Context * context, LineInfoArg * at );
    char* builtin_string_tolower_in_place ( char* str );
    char* builtin_string_toupper ( const char *str, Context * context, LineInfoArg * at );
    char* builtin_string_toupper_in_place ( char* str );
    char* builtin_string_chop( const char * str, int start, int length, Context * context, LineInfoArg * at );

    uint8_t string_to_uint8 ( const char *str, Context * context, LineInfoArg * at );
    int8_t string_to_int8 ( const char *str, Context * context, LineInfoArg * at );
    uint16_t string_to_uint16 ( const char *str, Context * context, LineInfoArg * at );
    int16_t string_to_int16 ( const char *str, Context * context, LineInfoArg * at );
    uint32_t string_to_uint ( const char *str, Context * context, LineInfoArg * at );
    int32_t string_to_int ( const char *str, Context * context, LineInfoArg * at );
    uint64_t string_to_uint64 ( const char *str, Context * context, LineInfoArg * at );
    int64_t string_to_int64 ( const char *str, Context * context, LineInfoArg * at );
    float string_to_float ( const char *str, Context * context, LineInfoArg * at );
    double string_to_double ( const char *str, Context * context, LineInfoArg * at );
    float fast_to_float ( const char *str );
    double fast_to_double ( const char *str );
    int8_t fast_to_int8 ( const char *str, bool hex );
    uint8_t fast_to_uint8 ( const char *str, bool hex );
    int16_t fast_to_int16 ( const char *str, bool hex );
    uint16_t fast_to_uint16 ( const char *str, bool hex );
    int32_t fast_to_int ( const char *str, bool hex );
    uint32_t fast_to_uint ( const char *str, bool hex );
    int64_t fast_to_int64 ( const char *str, bool hex );
    uint64_t fast_to_uint64 ( const char *str, bool hex );
    void builtin_append_char_to_string(string & str, int32_t Ch);
    bool builtin_string_ends_with(const string &str, char * substr, Context * context);
    int32_t builtin_ext_string_length(const string & str);
    void builtin_resize_string(string & str, int32_t newLength);
    char * string_repeat ( const char * str, int count, Context * context, LineInfoArg * at );
    char * to_string_char(int ch, Context * context, LineInfoArg * at);
    StringBuilderWriter & write_string_char(StringBuilderWriter & writer, int32_t ch);
    StringBuilderWriter & write_string_chars(StringBuilderWriter & writer, int32_t ch, int32_t count);
    StringBuilderWriter & write_escape_string ( StringBuilderWriter & writer, char * str );
    void builtin_string_split_by_char ( const char * str, const char * delim, const Block & sblk, Context * context, LineInfoArg * lineinfo );
    void builtin_string_split ( const char * str, const char * delim, const Block & sblk, Context * context, LineInfoArg * lineinfo );
    char * builtin_string_from_array ( const TArray<uint8_t> & bytes, Context * context, LineInfoArg * at );
    char * builtin_string_replace ( const char * str, const char * toSearch, const char * replaceStr, Context * context, LineInfoArg * at );
    char * builtin_string_rtrim ( char* s, Context * context, LineInfoArg * at );
    char * builtin_string_rtrim_ts ( char* s, char * ts, Context * context, LineInfoArg * at );
    char * builtin_string_ltrim ( char* s, Context * context, LineInfoArg * at );
    char * builtin_string_trim ( char* s, Context * context, LineInfoArg * at );

    char * builtin_reserve_string_buffer ( const char * str, int32_t length, Context * context );

    template <typename TT>
    __forceinline char * format ( const char * fmt, TT value, Context * context, LineInfoArg * at ) {
        char buf[256];
        snprintf(buf, 256, fmt ? fmt : "", value);
        return context->allocateString(buf, uint32_t(strlen(buf)), at);
    }

    char * fmt_i8 ( const char * fmt, int8_t value, Context * context, LineInfoArg * at );
    char * fmt_u8 ( const char * fmt, uint8_t value, Context * context, LineInfoArg * at );
    char * fmt_i16 ( const char * fmt, int16_t value, Context * context, LineInfoArg * at );
    char * fmt_u16 ( const char * fmt, uint16_t value, Context * context, LineInfoArg * at );
    char * fmt_i32 ( const char * fmt, int32_t value, Context * context, LineInfoArg * at );
    char * fmt_u32 ( const char * fmt, uint32_t value, Context * context, LineInfoArg * at );
    char * fmt_i64 ( const char * fmt, int64_t value, Context * context, LineInfoArg * at );
    char * fmt_u64 ( const char * fmt, uint64_t value, Context * context, LineInfoArg * at );
    char * fmt_f ( const char * fmt, float value, Context * context, LineInfoArg * at );
    char * fmt_d ( const char * fmt, double value, Context * context, LineInfoArg * at );

    template <typename TT>
    StringBuilderWriter & format_and_write ( StringBuilderWriter & writer, const char * fmt, TT value  ) {
        char buf[256];
        snprintf(buf, 256, fmt ? fmt : "", value);
        writer.writeStr(buf, strlen(buf));
        return writer;
    }

    StringBuilderWriter & fmt_and_write_i8 ( StringBuilderWriter & writer, const char * fmt, int8_t value, Context * context, LineInfoArg * at );
    StringBuilderWriter & fmt_and_write_u8 ( StringBuilderWriter & writer, const char * fmt, uint8_t value, Context * context, LineInfoArg * at );
    StringBuilderWriter & fmt_and_write_i16 ( StringBuilderWriter & writer, const char * fmt, int16_t value, Context * context, LineInfoArg * at );
    StringBuilderWriter & fmt_and_write_u16 ( StringBuilderWriter & writer, const char * fmt, uint16_t value, Context * context, LineInfoArg * at );
    StringBuilderWriter & fmt_and_write_i32 ( StringBuilderWriter & writer, const char * fmt, int32_t value, Context * context, LineInfoArg * at );
    StringBuilderWriter & fmt_and_write_u32 ( StringBuilderWriter & writer, const char * fmt, uint32_t value, Context * context, LineInfoArg * at );
    StringBuilderWriter & fmt_and_write_i64 ( StringBuilderWriter & writer, const char * fmt, int64_t value, Context * context, LineInfoArg * at );
    StringBuilderWriter & fmt_and_write_u64 ( StringBuilderWriter & writer, const char * fmt, uint64_t value, Context * context, LineInfoArg * at );
    StringBuilderWriter & fmt_and_write_f ( StringBuilderWriter & writer, const char * fmt, float value, Context * context, LineInfoArg * at );
    StringBuilderWriter & fmt_and_write_d ( StringBuilderWriter & writer, const char * fmt, double value, Context * context, LineInfoArg * at );

    template <typename TT>
    char * builtin_build_string_T ( TT && block, Context * context, LineInfoArg * at ) {
        StringBuilderWriter writer;
        block(writer);
        auto length = writer.tellp();
        if ( length ) {
            return context->allocateString(writer.c_str(), length, at);
        } else {
            return nullptr;
        }
    }

    template <typename TT>
    uint64_t builtin_build_hash_T ( TT && block, Context * context, LineInfoArg * at ) {
        StringBuilderWriter writer;
        block(writer);
        return hash_block64((const uint8_t *)writer.c_str(),writer.tellp());
    }

    __forceinline int32_t get_character_uat ( const char * str, int32_t index ) { return ((uint8_t *)str)[index]; }

    __forceinline bool is_alpha ( int32_t ch ) { return (ch>='a' && ch<='z') || (ch>='A' && ch<='Z'); }
    __forceinline bool is_white_space ( int32_t ch ) { return  ch==' ' || ch=='\n' || ch=='\r' || ch=='\t'; }
    __forceinline bool is_number ( int32_t ch ) { return (ch>='0' && ch<='9'); }
    __forceinline bool is_new_line ( int32_t ch ) { return ch=='\n' || ch=='\r'; }

    int8_t convert_from_string_int8 ( const char * str, ConversionResult & result, int32_t & offset, bool hex );
    uint8_t convert_from_string_uint8 ( const char * str, ConversionResult & result, int32_t & offset, bool hex );
    int16_t convert_from_string_int16 ( const char * str, ConversionResult & result, int32_t & offset, bool hex );
    uint16_t convert_from_string_uint16 ( const char * str, ConversionResult & result, int32_t & offset, bool hex );
    int32_t convert_from_string_int32 ( const char * str, ConversionResult & result, int32_t & offset, bool hex );
    uint32_t convert_from_string_uint32 ( const char * str, ConversionResult & result, int32_t & offset, bool hex );
    int64_t convert_from_string_int64 ( const char * str, ConversionResult & result, int32_t & offset, bool hex );
    uint64_t convert_from_string_uint64 ( const char * str, ConversionResult & result, int32_t & offset, bool hex );
    float convert_from_string_float ( const char * str, ConversionResult & result, int32_t & offset );
    double convert_from_string_double ( const char * str, ConversionResult & result, int32_t & offset );
}
