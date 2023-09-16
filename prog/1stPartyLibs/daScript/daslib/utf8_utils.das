options indenting = 4
options strict_smart_pointers = true

module utf8_utils shared

require daslib/strings_boost
require math

def public utf16_to_utf32 ( high,low:uint )
	if high >= 0xD800 && high <= 0xDBFF
		return ((high - 0xD800) << 10u) + (low - 0xDC00) + 0x10000
	else
		return high

def public utf8_encode(var dest_array: array<uint8> &; ch: uint)
    //! Converts a codepoint (UTF-32 symbol) to UTF-8 and appends it to the UTF-8 byte array
    if ch < 0x80
        dest_array |> push(uint8(ch))
    elif ch < 0x800
        dest_array |> push(uint8(0xC0 + (ch >> 6u)))
        dest_array |> push(uint8(0x80 + (ch & 0x3F)))
    elif (ch < 0x10000)
        dest_array |> push(uint8(0xE0 + (ch >> 12u)))
        dest_array |> push(uint8(0x80 + ((ch >> 6u) & 0x3F)))
        dest_array |> push(uint8(0x80 + (ch & 0x3F)))
    else
        dest_array |> push(uint8(0xF0 + (ch >> 18u)))
        dest_array |> push(uint8(0x80 + ((ch >> 12u) & 0x3F)))
        dest_array |> push(uint8(0x80 + ((ch >> 6u) & 0x3F)))
        dest_array |> push(uint8(0x80 + (ch & 0x3F)))


def public utf8_encode(ch: uint): array<uint8>
    //! Converts a codepoint (UTF-32 symbol) to the UTF-8 byte array
    if ch < 0x80
        return <- [{ uint8[] uint8(ch) }]
    elif ch < 0x800
        return <- [{ uint8[]
            uint8(0xC0 + (ch >> 6u));
            uint8(0x80 + (ch & 0x3F)) }]
    elif ch < 0x10000
        return <- [{ uint8[]
            uint8(0xE0 + (ch >> 12u));
            uint8(0x80 + ((ch >> 6u) & 0x3F));
            uint8(0x80 + (ch & 0x3F)) }]
    else
        return <- [{ uint8[]
            uint8(0xF0 + (ch >> 18u));
            uint8(0x80 + ((ch >> 12u) & 0x3F));
            uint8(0x80 + ((ch >> 6u) & 0x3F));
            uint8(0x80 + (ch & 0x3F)) }]


def public utf8_encode(var dest_array: array<uint8> &; source_utf32_string: array<uint> const implicit)
    //! Converts UTF-32 string to UTF-8 and appends it to the UTF-8 byte array
    for ch in source_utf32_string
        dest_array |> utf8_encode(ch)


def public utf8_encode(source_utf32_string: array<uint> const implicit): array<uint8>
    //! Converts UTF-32 string to UTF-8 and returns it as a UTF-8 byte array
    var dest_array: array<uint8>
    dest_array |> reserve(length(source_utf32_string))
    for ch in source_utf32_string
        dest_array |> utf8_encode(ch)
    return <- dest_array


def public utf8_length(utf8_string: array<uint8> const implicit): int
    //! Returns the number of characters in the UTF-8 string
    var length = 0
    for ch in utf8_string
        if (uint(ch) & 0xC0) != 0x80
            length++
    return length


def public utf8_length(utf8_string: string const): int
    //! Returns the number of characters in the UTF-8 string
    var length = 0
    for ch in utf8_string
        if (uint(ch) & 0xC0) != 0x80
            length++
    return length


def public is_first_byte_of_utf8_char(ch: uint8): bool
    let x = uint(ch)
    return (x > 0u) && (x < 128u || (x & 0xC0) == 0xC0)


def public contains_utf8_bom(utf8_string: array<uint8> const implicit): bool
    return (length(utf8_string) >= 3 &&
          uint(utf8_string[0]) == 0xEF && uint(utf8_string[1]) == 0xBB && uint(utf8_string[2]) == 0xBF)


def public contains_utf8_bom(utf8_string: string const): bool
    unsafe
        return (!empty(utf8_string) &&
              uint(utf8_string |> character_uat(0)) == 0xEF &&
              uint(utf8_string |> character_uat(1)) == 0xBB &&
              uint(utf8_string |> character_uat(2)) == 0xBF)



// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

let shared s_utf8d <- [[ uint[]
	// The first part of the table maps bytes to character classes that
	// to reduce the size of the transition table and create bitmasks.
   0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;  0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;
   0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;  0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;
   0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;  0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;
   0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;  0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;0u;
   1u;1u;1u;1u;1u;1u;1u;1u;1u;1u;1u;1u;1u;1u;1u;1u;  9u;9u;9u;9u;9u;9u;9u;9u;9u;9u;9u;9u;9u;9u;9u;9u;
   7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;  7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;7u;
   8u;8u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;  2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;2u;
  10u;3u;3u;3u;3u;3u;3u;3u;3u;3u;3u;3u;3u;4u;3u;3u; 11u;6u;6u;6u;5u;8u;8u;8u;8u;8u;8u;8u;8u;8u;8u;8u;

  // The second part is a transition table that maps a combination
  // of a state of the automaton and a character class to a state.
   0u;12u;24u;36u;60u;96u;84u;12u;12u;12u;48u;72u; 12u;12u;12u;12u;12u;12u;12u;12u;12u;12u;12u;12u;
  12u; 0u;12u;12u;12u;12u;12u; 0u;12u; 0u;12u;12u; 12u;24u;12u;12u;12u;12u;12u;24u;12u;24u;12u;12u;
  12u;12u;12u;12u;12u;12u;12u;24u;12u;12u;12u;12u; 12u;24u;12u;12u;12u;12u;12u;12u;12u;24u;12u;12u;
  12u;12u;12u;12u;12u;12u;12u;36u;12u;36u;12u;12u; 12u;36u;12u;12u;12u;12u;12u;36u;12u;36u;12u;12u;
  12u;36u;12u;12u;12u;12u;12u;12u;12u;12u;12u;12u
]]


let UTF8_ACCEPT = 0u

def public is_utf8_string_valid(utf8_string: array<uint8> const implicit): bool
    var codepoint = 0u
    var state = 0u
    for ch in utf8_string
        let byte = uint(ch)
        let type_ = s_utf8d[byte]
        codepoint = (state != UTF8_ACCEPT) ? (byte & 0x3F) | (codepoint << 6u) : (0xFF >> type_) & byte
        state = s_utf8d[256u + state + type_]
    return state == UTF8_ACCEPT


def public is_utf8_string_valid(utf8_string: string const): bool
    var codepoint = 0u
    var state = 0u
    for ch in utf8_string
        let byte = uint(ch)
        let type_ = s_utf8d[byte]
        codepoint = (state != UTF8_ACCEPT) ? (byte & 0x3F) | (codepoint << 6u) : (0xFF >> type_) & byte
        state = s_utf8d[256u + state + type_]
    return state == UTF8_ACCEPT


def public utf8_decode(var dest_utf32_string: array<uint> &; source_utf8_string: array<uint8> const implicit)
    //! Converts UTF-8 string to UTF-32 and appends it to the array of codepoints (UTF-32 string)
    var codepoint = 0u
    var state = 0u
    let length = length(source_utf8_string)
    var i = contains_utf8_bom(source_utf8_string) ? 3 : 0

    while i < length
        let byte = uint(source_utf8_string[i++])
        let type_ = s_utf8d[byte]
      	codepoint = (state != UTF8_ACCEPT) ? (byte & 0x3F) | (codepoint << 6u) : (0xFF >> type_) & byte
        state = s_utf8d[256u + state + type_]
        if state == UTF8_ACCEPT
            dest_utf32_string |> push(codepoint)


def public utf8_decode(source_utf8_string: array<uint8> const implicit): array<uint>
    //! Converts UTF-8 string to UTF-32 and returns it as an array of codepoints (UTF-32 string)
    var dest_utf32_string: array<uint>
    dest_utf32_string |> utf8_decode(source_utf8_string)
    return <- dest_utf32_string


def public utf8_decode(source_utf8_string: string const): array<uint>
    //! Converts UTF-8 string to UTF-32 and returns it as an array of codepoints (UTF-32 string)
    var dest_utf32_string: array<uint>
    peek_data(source_utf8_string) <| $(arr)
        dest_utf32_string |> utf8_decode(arr)
    return <- dest_utf32_string


def public utf8_decode(var dest_utf32_string: array<uint>; source_utf8_string: string const)
    //! Converts UTF-8 string to UTF-32 and appends it to the array of codepoints (UTF-32 string)
    peek_data(source_utf8_string) <| $(arr)
        dest_utf32_string |> utf8_decode(arr)

def decode_unicode_escape ( str:string )
    return build_string <| $ ( writer )
        peek_data(str) <| $ ( p )
            var i = 0
            let llen = p |> length
            var bang : array<uint8>
            bang |> reserve(8)
            var high_surrogate = 0u
            while i < llen
                if p[i]==0x5cu8
                    var j = i + 1
                    var jlen = min(llen, j+8)
                    bang |> resize(0)
                    while j<jlen && is_hex(int(p[j]))
                        bang |> push(p[j])
                        j ++
                    if length(bang) == 4
                        let utf32 = to_uint(string(bang), true)
                        if utf32>=0xD800 && utf32<=0xDFFF
                            if high_surrogate == 0u
                                high_surrogate = utf32
                            else
                                let low_surrogate = utf32
                                let pair = 0x10000 + ((high_surrogate - 0xD800) << 10u) + (low_surrogate - 0xDC00)
                                high_surrogate = 0u
                                writer |> write(string(utf8_encode(pair)))
                        else
                            writer |> write(string(utf8_encode(utf32)))
                        i = j
                        continue
                writer |> write_char(int(p[i]))
                i += 1
