#include "daScript/misc/platform.h"

#include "daScript/simulate/runtime_string.h"
#include "daScript/simulate/simulate.h"
#include "daScript/simulate/hash.h"
#include "daScript/simulate/debug_print.h"
#include "daScript/simulate/runtime_string_delete.h"
#include "daScript/simulate/simulate_nodes.h"
#include "daScript/simulate/sim_policy.h"

namespace das
{
    // string operations

    vec4f SimPolicy_String::Add ( vec4f a, vec4f b, Context & context, LineInfo * at ) {
        const char *  sA = to_rts(a);
        auto la = stringLength(context, sA);
        const char *  sB = to_rts(b);
        auto lb = stringLength(context, sB);
        uint32_t commonLength = la + lb;
        if ( !commonLength ) {
            return v_zero();
        } else if ( char * sAB = (char * ) context.stringHeap->allocateString(nullptr, commonLength) ) {
            memcpy ( sAB, sA, la );
            memcpy ( sAB+la, sB, lb+1 );
            context.stringHeap->recognize(sAB);
            return cast<char *>::from(sAB);
        } else {
            context.throw_error_at(at, "can't add two strings, out of heap");
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
        } else if ( char * sAB = (char * ) context.stringHeap->allocateString(nullptr, commonLength) ) {
            memcpy ( sAB, sA, la );
            memcpy ( sAB+la, sB, lb+1 );
            *pA = sAB;
            context.stringHeap->recognize(sAB);
        } else {
            context.throw_error_at(at, "can't add two strings, out of heap");
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
        text << string(das::max(LCOL-COL+1,1),'^') << "\n";
        text << ROW << ":" << COL << " - " << LROW << ":" << LCOL << "\n";
        return text.str();
    }

    string to_string_ex ( double dnum ) {
        char buffer[32];
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%.17g", dnum);
        string stst(buffer);
        if ( stst.find_first_of(".e")==string::npos )
            stst += ".";
        return stst;
    }

    string to_string_ex ( float dnum ) {
        char buffer[32];
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%.9g", dnum);
        string stst(buffer);
        if ( stst.find_first_of(".e")==string::npos )
            stst += ".";
        return stst;
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
        if ( row ) {
            auto text = st ? getFewLines(st, stlen, row, col, lrow, lcol, tabSize) : "";
            ssw << fileName << ":" << row << ":" << col << ":\n" << text;
            if ( erc != CompilationError::unspecified ) ssw << int(erc) << ": ";
            ssw << message << "\n";
        } else {
            if ( erc != CompilationError::unspecified ) ssw << int(erc) << ": ";
            ssw << "error, " << message << "\n";
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
        int length = writer.tellp();
        if ( length ) {
            auto pStr = context.stringHeap->allocateString(writer.c_str(), length);
            if ( !pStr  ) {
                context.throw_error_at(debugInfo, "can't allocate string builder result, out of heap");
            }
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
        context.heap->free((char *)this, sizeof(StringIterator));
    }

    vec4f SimNode_StringIterator::eval ( Context & context ) {
        DAS_PROFILE_NODE
        vec4f ll = source->eval(context);
        char * str = cast<char *>::to(ll);
        char * iter = context.heap->allocate(sizeof(StringIterator));
        context.heap->mark_comment(iter,"string iterator");
        context.heap->mark_location(iter,&debugInfo);
        new (iter) StringIterator(str);
        return cast<char *>::from(iter);
    }
}
