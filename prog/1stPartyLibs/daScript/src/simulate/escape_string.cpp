#include "daScript/misc/platform.h"

#include "daScript/simulate/runtime_string.h"

namespace das {

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
                case 0:     if (das_escape) result.append("\\x00"); else result.append("\\u0000");  break;
                case '{':   if (das_escape) result.append("\\{"); else result.append("{");  break;
                case '}':   if (das_escape) result.append("\\}"); else result.append("}");  break;
                default:
                    if ( ch <= 0x1f ) {
                        result.append("\\u00");
                        const char tohex[] = "0123456789abcdef";
                        result.append(1,tohex[ch>>4]);
                        result.append(1,tohex[ch&15]);
                    } else {
                        result.append(1, ch);
                    }
                    break;
            }
        }
        return result;
    }
}
