#include "daScript/misc/platform.h"

#include "daScript/simulate/debug_print.h"
#include "daScript/simulate/debug_info.h"
#include "daScript/simulate/aot_builtin.h"
#include "daScript/misc/arraytype.h"
#include "daScript/simulate/runtime_table.h"
#include "daScript/ast/ast.h"

namespace das {

    // ---- TypeInfo-driven single-pass JSON scanner ----
    // No intermediate DOM. The JSON cursor and TypeInfo drive parsing together.
    // At each point we know what type we expect, parse the JSON token directly
    // into the target memory, and advance the cursor.

    struct JsonScanner {
        const char * cur;
        const char * end;
        Context & ctx;
        LineInfo * at;

        JsonScanner(const char * str, uint32_t len, Context & _ctx, LineInfo * _at)
            : cur(str), end(str + len), ctx(_ctx), at(_at) {}

        // ---- whitespace & helpers ----

        void skipWS() {
            while (cur < end && (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r'))
                cur++;
        }

        bool expect(char ch) {
            skipWS();
            if (cur < end && *cur == ch) { cur++; return true; }
            return false;
        }

        char peek() {
            skipWS();
            return cur < end ? *cur : 0;
        }

        // ---- skip any JSON value (for unknown keys) ----

        bool skipValue() {
            skipWS();
            if (cur >= end) return false;
            char ch = *cur;
            if (ch == '"') return skipString();
            if (ch == '{') return skipObject();
            if (ch == '[') return skipArray();
            if (ch == 't') { if (end - cur >= 4 && memcmp(cur,"true",4)==0) { cur+=4; return true; } return false; }
            if (ch == 'f') { if (end - cur >= 5 && memcmp(cur,"false",5)==0) { cur+=5; return true; } return false; }
            if (ch == 'n') { if (end - cur >= 4 && memcmp(cur,"null",4)==0) { cur+=4; return true; } return false; }
            if (ch == '-' || (ch >= '0' && ch <= '9')) return skipNumber();
            return false;
        }

        bool skipString() {
            if (cur >= end || *cur != '"') return false;
            cur++;
            while (cur < end) {
                char ch = *cur++;
                if (ch == '"') return true;
                if (ch == '\\') { if (cur >= end) return false; cur++; }
            }
            return false;
        }

        bool skipNumber() {
            if (cur < end && *cur == '-') cur++;
            if (cur >= end || (*cur < '0' || *cur > '9')) return false;
            while (cur < end && *cur >= '0' && *cur <= '9') cur++;
            if (cur < end && *cur == '.') {
                cur++;
                while (cur < end && *cur >= '0' && *cur <= '9') cur++;
            }
            if (cur < end && (*cur == 'e' || *cur == 'E')) {
                cur++;
                if (cur < end && (*cur == '+' || *cur == '-')) cur++;
                while (cur < end && *cur >= '0' && *cur <= '9') cur++;
            }
            return true;
        }

        bool skipArray() {
            if (!expect('[')) return false;
            if (peek() == ']') { cur++; return true; }
            while (true) {
                if (!skipValue()) return false;
                skipWS();
                if (cur >= end) return false;
                if (*cur == ']') { cur++; return true; }
                if (*cur != ',') return false;
                cur++;
            }
        }

        bool skipObject() {
            if (!expect('{')) return false;
            if (peek() == '}') { cur++; return true; }
            while (true) {
                skipWS();
                if (!skipString()) return false;
                if (!expect(':')) return false;
                if (!skipValue()) return false;
                skipWS();
                if (cur >= end) return false;
                if (*cur == '}') { cur++; return true; }
                if (*cur != ',') return false;
                cur++;
            }
        }

        // ---- parse JSON string into std::string ----

        bool readString(string & out) {
            skipWS();
            if (cur >= end || *cur != '"') return false;
            cur++;
            out.clear();
            while (cur < end) {
                char ch = *cur++;
                if (ch == '"') return true;
                if (ch == '\\') {
                    if (cur >= end) return false;
                    ch = *cur++;
                    switch (ch) {
                        case '"':  out += '"'; break;
                        case '\\': out += '\\'; break;
                        case '/':  out += '/'; break;
                        case 'b':  out += '\b'; break;
                        case 'f':  out += '\f'; break;
                        case 'n':  out += '\n'; break;
                        case 'r':  out += '\r'; break;
                        case 't':  out += '\t'; break;
                        case 'u': {
                            uint32_t cp = 0;
                            for (int i = 0; i < 4; i++) {
                                if (cur >= end) return false;
                                cp <<= 4;
                                ch = *cur++;
                                if (ch >= '0' && ch <= '9') cp |= uint32_t(ch - '0');
                                else if (ch >= 'a' && ch <= 'f') cp |= uint32_t(ch - 'a' + 10);
                                else if (ch >= 'A' && ch <= 'F') cp |= uint32_t(ch - 'A' + 10);
                                else return false;
                            }
                            if (cp >= 0xD800 && cp <= 0xDBFF) {
                                if (cur + 1 < end && cur[0] == '\\' && cur[1] == 'u') {
                                    cur += 2;
                                    uint32_t lo = 0;
                                    for (int i = 0; i < 4; i++) {
                                        if (cur >= end) return false;
                                        lo <<= 4;
                                        ch = *cur++;
                                        if (ch >= '0' && ch <= '9') lo |= uint32_t(ch - '0');
                                        else if (ch >= 'a' && ch <= 'f') lo |= uint32_t(ch - 'a' + 10);
                                        else if (ch >= 'A' && ch <= 'F') lo |= uint32_t(ch - 'A' + 10);
                                        else return false;
                                    }
                                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                                }
                            }
                            if (cp < 0x80) {
                                out += char(cp);
                            } else if (cp < 0x800) {
                                out += char(0xC0 | (cp >> 6));
                                out += char(0x80 | (cp & 0x3F));
                            } else if (cp < 0x10000) {
                                out += char(0xE0 | (cp >> 12));
                                out += char(0x80 | ((cp >> 6) & 0x3F));
                                out += char(0x80 | (cp & 0x3F));
                            } else {
                                out += char(0xF0 | (cp >> 18));
                                out += char(0x80 | ((cp >> 12) & 0x3F));
                                out += char(0x80 | ((cp >> 6) & 0x3F));
                                out += char(0x80 | (cp & 0x3F));
                            }
                            break;
                        }
                        default: return false;
                    }
                } else {
                    out += ch;
                }
            }
            return false;
        }

        // ---- parse number directly to int64 or double ----

        bool readInt64(int64_t & out) {
            skipWS();
            if (cur >= end) return false;
            const char * start = cur;
            if (*cur == '-') cur++;
            if (cur >= end || *cur < '0' || *cur > '9') { cur = start; return false; }
            while (cur < end && *cur >= '0' && *cur <= '9') cur++;
            // if there's a dot or exponent, it's a float — parse as double and convert
            if (cur < end && (*cur == '.' || *cur == 'e' || *cur == 'E')) {
                double d;
                cur = start;
                if (!readDouble(d)) return false;
                out = int64_t(d);
                return true;
            }
            string numStr(start, cur);
            out = atoll(numStr.c_str());
            return true;
        }

        bool readDouble(double & out) {
            skipWS();
            if (cur >= end) return false;
            const char * start = cur;
            if (*cur == '-') cur++;
            if (cur >= end || *cur < '0' || *cur > '9') { cur = start; return false; }
            while (cur < end && *cur >= '0' && *cur <= '9') cur++;
            if (cur < end && *cur == '.') {
                cur++;
                while (cur < end && *cur >= '0' && *cur <= '9') cur++;
            }
            if (cur < end && (*cur == 'e' || *cur == 'E')) {
                cur++;
                if (cur < end && (*cur == '+' || *cur == '-')) cur++;
                while (cur < end && *cur >= '0' && *cur <= '9') cur++;
            }
            string numStr(start, cur);
            out = atof(numStr.c_str());
            return true;
        }

        bool readBool(bool & out) {
            skipWS();
            if (end - cur >= 4 && memcmp(cur, "true", 4) == 0) {
                cur += 4; out = true; return true;
            }
            if (end - cur >= 5 && memcmp(cur, "false", 5) == 0) {
                cur += 5; out = false; return true;
            }
            return false;
        }

        bool isNull() {
            skipWS();
            if (end - cur >= 4 && memcmp(cur, "null", 4) == 0) {
                cur += 4; return true;
            }
            return false;
        }

        // ---- get effective field name (handle @rename) ----

        static string getFieldName(VarInfo * vi) {
            string name = vi->name ? vi->name : "";
            if (vi->annotation_arguments) {
                auto aa = (AnnotationArguments *)vi->annotation_arguments;
                for (auto & arg : *aa) {
                    if (arg.name == "rename") {
                        if (arg.type == Type::tString) {
                            name = arg.sValue;
                        } else if (arg.type == Type::tBool && !name.empty() && name[0] == '_') {
                            // @rename _field — strip leading underscore
                            name = name.substr(1);
                        }
                        break;
                    }
                }
            }
            return name;
        }

        // ---- find field by JSON key ----

        static VarInfo * findField(StructInfo * si, const string & key) {
            for (uint32_t f = 0; f < si->count; f++) {
                VarInfo * vi = si->fields[f];
                string fname = getFieldName(vi);
                if (fname == key) return vi;
            }
            return nullptr;
        }

        // ---- main: scan JSON value guided by TypeInfo ----

        bool scanValue(char * dst, TypeInfo * ti) {
            // null is valid for any type — leave default
            if (isNull()) return true;
            // handle fixed-size arrays (dim > 0)
            if (ti->dimSize > 0) {
                return scanFixedArray(dst, ti);
            }
            switch (ti->type) {
                case Type::tBool: {
                    bool v;
                    if (!readBool(v)) return false;
                    *(bool*)dst = v;
                    return true;
                }
                case Type::tInt8: case Type::tUInt8:
                case Type::tInt16: case Type::tUInt16:
                case Type::tInt: case Type::tUInt:
                case Type::tInt64: case Type::tUInt64: {
                    int64_t v;
                    if (!readInt64(v)) return false;
                    switch (ti->type) {
                        case Type::tInt8:   *(int8_t*)dst = int8_t(v); break;
                        case Type::tUInt8:  *(uint8_t*)dst = uint8_t(v); break;
                        case Type::tInt16:  *(int16_t*)dst = int16_t(v); break;
                        case Type::tUInt16: *(uint16_t*)dst = uint16_t(v); break;
                        case Type::tInt:    *(int32_t*)dst = int32_t(v); break;
                        case Type::tUInt:   *(uint32_t*)dst = uint32_t(v); break;
                        case Type::tInt64:  *(int64_t*)dst = v; break;
                        case Type::tUInt64: *(uint64_t*)dst = uint64_t(v); break;
                        default: break;
                    }
                    return true;
                }
                case Type::tFloat: {
                    double v;
                    if (!readDouble(v)) return false;
                    *(float*)dst = float(v);
                    return true;
                }
                case Type::tDouble: {
                    double v;
                    if (!readDouble(v)) return false;
                    *(double*)dst = v;
                    return true;
                }
                case Type::tString: {
                    string v;
                    if (!readString(v)) return false;
                    *(char**)dst = ctx.allocateString(v, at);
                    return true;
                }
                case Type::tStructure:
                    return ti->structType ? scanStruct(dst, ti->structType) : false;
                case Type::tPointer:
                    return scanPointer(dst, ti);
                case Type::tArray:
                    return ti->firstType ? scanArray(dst, ti->firstType) : false;
                case Type::tEnumeration:
                case Type::tEnumeration8:
                case Type::tEnumeration16:
                case Type::tEnumeration64:
                    return scanEnum(dst, ti);
                case Type::tTuple:
                    return scanTuple(dst, ti);
                case Type::tVariant:
                    return scanVariant(dst, ti);
                case Type::tTable:
                    return scanTable(dst, ti);
                // vector types — JSON arrays
                case Type::tInt2:   return scanVec<int32_t>(dst, 2);
                case Type::tInt3:   return scanVec<int32_t>(dst, 3);
                case Type::tInt4:   return scanVec<int32_t>(dst, 4);
                case Type::tUInt2:  return scanVec<uint32_t>(dst, 2);
                case Type::tUInt3:  return scanVec<uint32_t>(dst, 3);
                case Type::tUInt4:  return scanVec<uint32_t>(dst, 4);
                case Type::tFloat2: return scanVecF(dst, 2);
                case Type::tFloat3: return scanVecF(dst, 3);
                case Type::tFloat4: return scanVecF(dst, 4);
                case Type::tRange:    return scanRange<int32_t>(dst);
                case Type::tURange:   return scanRange<uint32_t>(dst);
                case Type::tRange64:  return scanRange<int64_t>(dst);
                case Type::tURange64: return scanRange<uint64_t>(dst);
                // bitfields — just integers
                case Type::tBitfield: {
                    int64_t v; if (!readInt64(v)) return false;
                    *(uint32_t*)dst = uint32_t(v); return true;
                }
                case Type::tBitfield8: {
                    int64_t v; if (!readInt64(v)) return false;
                    *(uint8_t*)dst = uint8_t(v); return true;
                }
                case Type::tBitfield16: {
                    int64_t v; if (!readInt64(v)) return false;
                    *(uint16_t*)dst = uint16_t(v); return true;
                }
                case Type::tBitfield64: {
                    int64_t v; if (!readInt64(v)) return false;
                    *(uint64_t*)dst = uint64_t(v); return true;
                }
                // void pointer — only null (already handled above)
                case Type::tVoid:
                    return skipValue();
                case Type::tHandle:
                    ctx.throw_error_at(at, "sscan_json: handled types are not supported");
                    return false;
                default:
                    // unsupported type — skip the JSON value
                    return skipValue();
            }
        }

        // ---- scan JSON object into struct ----

        bool scanStruct(char * dst, StructInfo * si) {
            if (!expect('{')) return false;
            if (peek() == '}') { cur++; return true; }
            while (true) {
                // read key
                string key;
                if (!readString(key)) return false;
                if (!expect(':')) return false;
                // find matching field
                VarInfo * vi = findField(si, key);
                if (vi) {
                    // skip class internals
                    string vname = vi->name ? vi->name : "";
                    if ((si->flags & StructInfo::flag_class) && (vname == "__rtti" || vname == "__finalize")) {
                        if (!skipValue()) return false;
                    } else {
                        char * pf = dst + vi->offset;
                        if (!scanValue(pf, vi)) return false;
                    }
                } else {
                    // unknown key — skip value
                    if (!skipValue()) return false;
                }
                skipWS();
                if (cur >= end) return false;
                if (*cur == '}') { cur++; return true; }
                if (*cur != ',') return false;
                cur++;
            }
        }

        // ---- scan pointer (struct pointer) ----

        bool scanPointer(char * dst, TypeInfo * ti) {
            if (isNull()) {
                *(char**)dst = nullptr;
                return true;
            }
            if (ti->firstType && ti->firstType->type == Type::tStructure && ti->firstType->structType) {
                auto si = ti->firstType->structType;
                char * ptr = ctx.allocate(si->size, at);
                if (!ptr) return false;
                memset(ptr, 0, si->size);
                if (!scanStruct(ptr, si)) return false;
                *(char**)dst = ptr;
                return true;
            }
            // unsupported pointer type — skip
            return skipValue();
        }

        // ---- scan dynamic array ----

        bool scanArray(char * dst, TypeInfo * elemTi) {
            if (!expect('[')) return false;
            Array * arr = (Array*)dst;
            int stride = getTypeSize(elemTi);
            // clear existing content
            if (arr->size) {
                array_resize(ctx, *arr, 0, stride, false, at);
            }
            if (peek() == ']') { cur++; return true; }
            while (true) {
                // grow array by one, zero-initialized
                uint32_t idx = builtin_array_push_back_zero(*arr, stride, &ctx, (LineInfoArg*)at);
                char * elem = arr->data + idx * stride;
                if (!scanValue(elem, elemTi)) return false;
                skipWS();
                if (cur >= end) return false;
                if (*cur == ']') { cur++; return true; }
                if (*cur != ',') return false;
                cur++;
            }
        }

        // ---- scan fixed-size array ----

        bool scanFixedArray(char * dst, TypeInfo * ti) {
            if (!expect('[')) return false;
            int elemSize = getTypeBaseSize(ti);
            uint32_t maxCount = ti->dim[0];
            uint32_t idx = 0;
            if (peek() == ']') { cur++; return true; }
            while (true) {
                if (idx < maxCount) {
                    if (!scanValue(dst + idx * elemSize, ti)) return false;
                } else {
                    if (!skipValue()) return false;
                }
                idx++;
                skipWS();
                if (cur >= end) return false;
                if (*cur == ']') { cur++; return true; }
                if (*cur != ',') return false;
                cur++;
            }
        }

        // ---- scan enumeration ----

        bool scanEnum(char * dst, TypeInfo * ti) {
            if (!ti->enumType) return skipValue();
            int64_t eVal = 0;
            skipWS();
            if (cur < end && *cur == '"') {
                // string enum value — match by name
                string name;
                if (!readString(name)) return false;
                for (uint32_t e = 0; e < ti->enumType->count; e++) {
                    if (ti->enumType->fields[e]->name && name == ti->enumType->fields[e]->name) {
                        eVal = ti->enumType->fields[e]->value;
                        break;
                    }
                }
            } else {
                // numeric enum value
                if (!readInt64(eVal)) return false;
            }
            switch (ti->type) {
                case Type::tEnumeration:   *(int32_t*)dst = int32_t(eVal); break;
                case Type::tEnumeration8:  *(int8_t*)dst = int8_t(eVal); break;
                case Type::tEnumeration16: *(int16_t*)dst = int16_t(eVal); break;
                case Type::tEnumeration64: *(int64_t*)dst = eVal; break;
                default: break;
            }
            return true;
        }

        // ---- scan tuple: {"_0":val, "_1":val, ...} ----

        bool scanTuple(char * dst, TypeInfo * ti) {
            if (!expect('{')) return false;
            if (peek() == '}') { cur++; return true; }
            while (true) {
                string key;
                if (!readString(key)) return false;
                if (!expect(':')) return false;
                // key is "_0", "_1", etc.
                int idx = -1;
                if (key.length() >= 2 && key[0] == '_') {
                    idx = atoi(key.c_str() + 1);
                }
                if (idx >= 0 && uint32_t(idx) < ti->argCount) {
                    int offset = getTupleFieldOffset(ti, idx);
                    if (!scanValue(dst + offset, ti->argTypes[idx])) return false;
                } else {
                    if (!skipValue()) return false;
                }
                skipWS();
                if (cur >= end) return false;
                if (*cur == '}') { cur++; return true; }
                if (*cur != ',') return false;
                cur++;
            }
        }

        // ---- scan variant: {"fieldName":val} (single key) ----

        bool scanVariant(char * dst, TypeInfo * ti) {
            if (!expect('{')) return false;
            if (peek() == '}') { cur++; return true; }
            // read the single key
            string key;
            if (!readString(key)) return false;
            if (!expect(':')) return false;
            // find matching variant field
            int fidx = -1;
            for (uint32_t i = 0; i < ti->argCount; i++) {
                if (ti->argNames[i] && key == ti->argNames[i]) {
                    fidx = int(i);
                    break;
                }
            }
            if (fidx >= 0) {
                // set variant index
                *(int32_t*)dst = fidx;
                int offset = getVariantFieldOffset(ti, fidx);
                if (!scanValue(dst + offset, ti->argTypes[fidx])) return false;
            } else {
                if (!skipValue()) return false;
            }
            // consume remaining entries (should be just the closing brace)
            skipWS();
            if (cur < end && *cur == ',') {
                cur++;
                // skip any extra key-value pairs
                while (true) {
                    if (!skipString()) return false;
                    if (!expect(':')) return false;
                    if (!skipValue()) return false;
                    skipWS();
                    if (cur >= end) return false;
                    if (*cur == '}') { cur++; return true; }
                    if (*cur != ',') return false;
                    cur++;
                }
            }
            if (!expect('}')) return false;
            return true;
        }

        // ---- scan table: {"key":val, ...} ----

        // Parse a value from a string by temporarily redirecting the scanner
        bool scanFromStr(const string & str, char * dst, TypeInfo * ti) {
            const char * saveCur = cur;
            const char * saveEnd = end;
            cur = str.c_str();
            end = cur + str.size();
            bool ok = scanValue(dst, ti);
            cur = saveCur;
            end = saveEnd;
            return ok;
        }

        // Reserve a key in the table and return the index
        template <typename KeyT>
        int tableReserve(Table * tab, const KeyT & key, uint32_t valueTypeSize) {
            auto hfn = hash_function(ctx, key);
            TableHash<KeyT> thh(&ctx, valueTypeSize);
            return thh.reserve(*tab, key, hfn, at);
        }

        // Parse key from string and reserve in table (for compound key types)
        template <typename KeyT>
        bool scanKeyAndReserve(Table * tab, const string & keyStr, TypeInfo * keyTi,
                               uint32_t valueTypeSize, int & index) {
            KeyT key;
            memset(&key, 0, sizeof(key));
            if (!scanFromStr(keyStr, (char*)&key, keyTi)) return false;
            index = tableReserve<KeyT>(tab, key, valueTypeSize);
            return true;
        }

        bool scanTable(char * dst, TypeInfo * ti) {
            if (!ti->firstType || !ti->secondType) return skipValue();
            if (!expect('{')) return false;
            Table * tab = (Table*)dst;
            uint32_t valueTypeSize = ti->secondType->size;
            // clear existing content
            if (tab->size) {
                table_clear(ctx, *tab, at);
            }
            if (peek() == '}') { cur++; return true; }
            while (true) {
                // read key as string (JSON keys are always strings)
                string keyStr;
                if (!readString(keyStr)) return false;
                if (!expect(':')) return false;
                // insert key into table based on key type
                int index = -1;
                bool keyOk = true;
                switch (ti->firstType->type) {
                    // string key
                    case Type::tString: {
                        char * key = ctx.allocateString(keyStr, at);
                        index = tableReserve<char*>(tab, key, valueTypeSize);
                        break;
                    }
                    // integer key types — parse from string
                    case Type::tBool: {
                        bool key = keyStr == "true";
                        index = tableReserve<bool>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tInt8: {
                        int8_t key = int8_t(atoll(keyStr.c_str()));
                        index = tableReserve<int8_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tUInt8: {
                        uint8_t key = uint8_t(strtoull(keyStr.c_str(), nullptr, 10));
                        index = tableReserve<uint8_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tInt16: {
                        int16_t key = int16_t(atoll(keyStr.c_str()));
                        index = tableReserve<int16_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tUInt16: {
                        uint16_t key = uint16_t(strtoull(keyStr.c_str(), nullptr, 10));
                        index = tableReserve<uint16_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tInt: {
                        int32_t key = int32_t(atoll(keyStr.c_str()));
                        index = tableReserve<int32_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tUInt:
                    case Type::tBitfield: {
                        uint32_t key = uint32_t(strtoull(keyStr.c_str(), nullptr, 10));
                        index = tableReserve<uint32_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tInt64: {
                        int64_t key = atoll(keyStr.c_str());
                        index = tableReserve<int64_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tUInt64:
                    case Type::tBitfield64: {
                        uint64_t key = strtoull(keyStr.c_str(), nullptr, 10);
                        index = tableReserve<uint64_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tBitfield8: {
                        uint8_t key = uint8_t(strtoull(keyStr.c_str(), nullptr, 10));
                        index = tableReserve<uint8_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tBitfield16: {
                        uint16_t key = uint16_t(strtoull(keyStr.c_str(), nullptr, 10));
                        index = tableReserve<uint16_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tFloat: {
                        float key = float(strtod(keyStr.c_str(), nullptr));
                        index = tableReserve<float>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tDouble: {
                        double key = strtod(keyStr.c_str(), nullptr);
                        index = tableReserve<double>(tab, key, valueTypeSize);
                        break;
                    }
                    // enumeration key types — parse as integer (sprint_json with enumAsInt)
                    case Type::tEnumeration: {
                        int32_t key = int32_t(atoll(keyStr.c_str()));
                        index = tableReserve<int32_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tEnumeration8: {
                        int8_t key = int8_t(atoll(keyStr.c_str()));
                        index = tableReserve<int8_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tEnumeration16: {
                        int16_t key = int16_t(atoll(keyStr.c_str()));
                        index = tableReserve<int16_t>(tab, key, valueTypeSize);
                        break;
                    }
                    case Type::tEnumeration64: {
                        int64_t key = atoll(keyStr.c_str());
                        index = tableReserve<int64_t>(tab, key, valueTypeSize);
                        break;
                    }
                    // vector key types — parse "[x,y,...]" from string
                    case Type::tInt2:   keyOk = scanKeyAndReserve<int2>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tInt3:   keyOk = scanKeyAndReserve<int3>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tInt4:   keyOk = scanKeyAndReserve<int4>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tUInt2:  keyOk = scanKeyAndReserve<uint2>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tUInt3:  keyOk = scanKeyAndReserve<uint3>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tUInt4:  keyOk = scanKeyAndReserve<uint4>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tFloat2: keyOk = scanKeyAndReserve<float2>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tFloat3: keyOk = scanKeyAndReserve<float3>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tFloat4: keyOk = scanKeyAndReserve<float4>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    // range key types — parse "[x,y]" from string
                    case Type::tRange:    keyOk = scanKeyAndReserve<range>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tURange:   keyOk = scanKeyAndReserve<urange>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tRange64:  keyOk = scanKeyAndReserve<range64>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    case Type::tURange64: keyOk = scanKeyAndReserve<urange64>(tab, keyStr, ti->firstType, valueTypeSize, index); break;
                    // pointer key — always null
                    case Type::tPointer: {
                        void * key = nullptr;
                        index = tableReserve<void*>(tab, key, valueTypeSize);
                        break;
                    }
                    default:
                        keyOk = false;
                        break;
                }
                if (!keyOk) {
                    // key parse failed — skip the value
                    if (!skipValue()) return false;
                } else if (index >= 0) {
                    char * val = tab->data + index * valueTypeSize;
                    memset(val, 0, valueTypeSize);
                    if (!scanValue(val, ti->secondType)) return false;
                } else {
                    if (!skipValue()) return false;
                }
                skipWS();
                if (cur >= end) return false;
                if (*cur == '}') { cur++; return true; }
                if (*cur != ',') return false;
                cur++;
            }
        }

        // ---- scan vector types: [x,y], [x,y,z], [x,y,z,w] ----

        template <typename T>
        bool scanVec(char * dst, int count) {
            if (!expect('[')) return false;
            T * vals = (T*)dst;
            for (int i = 0; i < count; i++) {
                if (i > 0 && !expect(',')) return false;
                int64_t v;
                if (!readInt64(v)) return false;
                vals[i] = T(v);
            }
            return expect(']');
        }

        bool scanVecF(char * dst, int count) {
            if (!expect('[')) return false;
            float * vals = (float*)dst;
            for (int i = 0; i < count; i++) {
                if (i > 0 && !expect(',')) return false;
                double v;
                if (!readDouble(v)) return false;
                vals[i] = float(v);
            }
            return expect(']');
        }

        // ---- scan range types: [x,y] ----

        template <typename T>
        bool scanRange(char * dst) {
            if (!expect('[')) return false;
            T * vals = (T*)dst;
            int64_t v;
            if (!readInt64(v)) return false;
            vals[0] = T(v);
            if (!expect(',')) return false;
            if (!readInt64(v)) return false;
            vals[1] = T(v);
            return expect(']');
        }
    };

    // ---- Entry point ----

    bool debug_json_scan(Context & ctx, char * dst, TypeInfo * info, const char * json, uint32_t jsonLen, LineInfo * at) {
        if (!json || jsonLen == 0) return false;
        JsonScanner scanner(json, jsonLen, ctx, at);
        return scanner.scanValue(dst, info);
    }
}
