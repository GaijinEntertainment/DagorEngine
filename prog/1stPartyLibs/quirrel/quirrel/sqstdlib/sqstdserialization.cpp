/* see copyright notice in squirrel.h */
#include <squirrel/sqpcheader.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdblob.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqclass.h>
#include <squirrel/sqarray.h>
#include <squirrel/sqvm.h>
#include "sqstdstream.h"
#include "sqstdblobimpl.h"
#include <unordered_map>
#include <vector>

#define STDLIB std

struct SQStreamSerializer {
    enum DataType {
        TP_NULL = 0 << 4,
        TP_BOOL = 1 << 4,
        TP_INTEGER = 2 << 4,
        TP_FLOAT = 3 << 4,
        TP_STRING = 4 << 4,
        TP_ARRAY = 5 << 4,
        TP_TABLE = 6 << 4,
        TP_INSTANCE = 7 << 4,
    };

    enum {
        START_MARKER = 0xEA,
        END_MARKER = 0xFA,
        MAX_DEPTH = 200,
    };

    struct ClassDesc {
        int index; // -1 if not serialized yet
        const SQChar *className;
    };

    HSQUIRRELVM vm;
    SQStream *stream;
    const SQChar *errorString;
    STDLIB::unordered_map<SQChar *, uint32_t> stringTable; // string -> index mapping
    STDLIB::vector<SQObjectPtr> stringList; // to keep track of strings
    STDLIB::unordered_map<SQClass *, ClassDesc> classTable;
    STDLIB::vector<SQObjectPtr> classList; // to keep track of classes
    SQObjectPtr availableClasses; // null or a table of available classes (class_name -> class)
    SQObjectPtr getstateProcName;
    SQObjectPtr setstateProcName;
    uint32_t stringCounter;
    uint32_t classCounter;
    uint32_t depth;

    SQStreamSerializer() : vm(nullptr), stream(nullptr), errorString(nullptr), stringCounter(0), depth(0),
        classCounter(0) {}

    int getCountOfParams(SQObjectPtr obj) {
        if (sq_isclosure(obj))
            return _closure(obj)->_function->_nparameters;
        else if (sq_isnativeclosure(obj))
            return _nativeclosure(obj)->_nparamscheck;
        else
            return -1;
    }

    bool fillAvailableClassNamesOnDemand() {
        if (!classTable.empty())
            return true;

        if (sq_isnull(availableClasses))
            return true;

        if (!sq_istable(availableClasses)) {
            errorString = _SC("Available classes must be a table (class_name -> class)");
            return false;
        }

        SQTable *tbl = _table(availableClasses);
        SQTable::_HashNode *node = tbl->_nodes;
        uint32_t count = tbl->_numofnodes_minus_one + 1;

        for (uint32_t i = 0; i < count; ++i) {
            SQObjectPtr key = node[i].key;
            if (!sq_isstring(key))
                continue;

            SQObjectPtr value = node[i].val;
            if (!sq_isclass(value))
                continue;

            const SQChar *className = _stringval(key);
            SQClass *cls = _class(value);
            classTable[cls] = { -1, className };
        }

        getstateProcName = SQString::Create(_ss(vm), _SC("__getstate"));
        setstateProcName = SQString::Create(_ss(vm), _SC("__setstate"));
        return true;
    }

    bool serializeObject(const SQObject &obj) {
        if (sq_isnull(obj)) {
            uint8_t v = TP_NULL | 1;
            stream->Write(&v, sizeof(v));
        }
        else if (sq_isbool(obj)) {
            uint8_t v = TP_BOOL | (_integer(obj) ? 1 : 0);
            stream->Write(&v, sizeof(v));
        }
        else if (sq_isinteger(obj)) {
            uint8_t v = TP_INTEGER;
            SQInteger iv = _integer(obj);
            if (iv >= -1 && iv <= 2) {
                v |= iv + 1; // map -1, 0, 1, 2 to 0, 1, 2, 3
                stream->Write(&v, sizeof(v));
            }
            else if (iv >= INT8_MIN && iv <= INT8_MAX) {
                v |= 4; // 4 for 1-byte integer
                stream->Write(&v, sizeof(v));
                int8_t iv8 = static_cast<int8_t>(iv);
                stream->Write(&iv8, sizeof(int8_t));
            }
            else if (iv >= INT16_MIN && iv <= INT16_MAX) {
                v |= 5; // 5 for 2-byte integer
                stream->Write(&v, sizeof(v));
                int16_t iv16 = static_cast<int16_t>(iv);
                stream->Write(&iv16, sizeof(int16_t));
            }
            else if (iv >= INT32_MIN && iv <= INT32_MAX) {
                v |= 6; // 6 for 4-byte integer
                stream->Write(&v, sizeof(v));
                int32_t iv32 = static_cast<int32_t>(iv);
                stream->Write(&iv, sizeof(int32_t));
            }
            else {
                v |= 7; // 7 for 8-byte integer
                stream->Write(&v, sizeof(v));
                int64_t iv64 = static_cast<int64_t>(iv);
                stream->Write(&iv64, sizeof(int64_t));
            }
        }
        else if (sq_isfloat(obj)) {
            uint8_t v = TP_FLOAT;
            SQFloat fv = _float(obj);
            if (fv == SQFloat(-1.0)) {
                v |= 0; // -1.0
                stream->Write(&v, sizeof(v));
            }
            else if (fv == SQFloat(-0.5)) {
                v |= 1; // -0.5
                stream->Write(&v, sizeof(v));
            }
            else if (fv == SQFloat(0.0)) {
                v |= 2; // 0.0
                stream->Write(&v, sizeof(v));
            }
            else if (fv == SQFloat(0.5)) {
                v |= 3; // 0.5
                stream->Write(&v, sizeof(v));
            }
            else if (fv == SQFloat(1.0)) {
                v |= 4; // 1.0
                stream->Write(&v, sizeof(v));
            }
            else if (fv == SQFloat(2.0)) {
                v |= 5; // 2.0
                stream->Write(&v, sizeof(v));
            }
            else {
            #ifdef SQUSEDOUBLE
                v |= 7; // 8-byte float
                stream->Write(&v, sizeof(v));
                double dfv = static_cast<double>(fv);
                stream->Write(&dfv, sizeof(double));
            #else
                v |= 6; // 4-byte float
                stream->Write(&v, sizeof(v));
                float ffv = static_cast<float>(fv);
                stream->Write(&ffv, sizeof(float));
            #endif
            }
        }
        else if (sq_isstring(obj)) {
            uint8_t v = TP_STRING;
            SQInteger len = _string(obj)->_len;
            if (len == 0) {
                v |= 0; // empty string
                stream->Write(&v, sizeof(v));
            }
            else {
                // check if the string is new or already in the table
                SQChar *str = _stringval(obj);
                auto it = stringTable.find(str);
                if (it == stringTable.end()) {
                    if (len <= UINT8_MAX) {
                        v |= 1; // 1-byte string
                        stream->Write(&v, sizeof(v));
                        uint8_t index = static_cast<uint8_t>(len);
                        stream->Write(&index, sizeof(uint8_t));
                        stream->Write(str, len * sizeof(SQChar));
                    }
                    else if (len <= UINT16_MAX) {
                        v |= 2; // 2-byte string
                        stream->Write(&v, sizeof(v));
                        uint16_t index = static_cast<uint16_t>(len);
                        stream->Write(&index, sizeof(uint16_t));
                        stream->Write(str, len * sizeof(SQChar));
                    }
                    else {
                        v |= 3; // 4-byte string
                        stream->Write(&v, sizeof(v));
                        uint32_t index = static_cast<uint32_t>(len);
                        stream->Write(&index, sizeof(uint32_t));
                        stream->Write(str, len * sizeof(SQChar));
                    }

                    stringTable[str] = stringCounter++;
                }
                else {
                    uint32_t index = it->second;
                    if (index <= UINT8_MAX) {
                        v |= 4; // 1-byte index
                        stream->Write(&v, sizeof(v));
                        uint8_t idx = static_cast<uint8_t>(index);
                        stream->Write(&idx, sizeof(uint8_t));
                    }
                    else if (index <= UINT16_MAX) {
                        v |= 5; // 2-byte index
                        stream->Write(&v, sizeof(v));
                        uint16_t idx = static_cast<uint16_t>(index);
                        stream->Write(&idx, sizeof(uint16_t));
                    }
                    else {
                        v |= 6; // 4-byte index
                        stream->Write(&v, sizeof(v));
                        uint32_t idx = static_cast<uint32_t>(index);
                        stream->Write(&idx, sizeof(uint32_t));
                    }
                }
            }
        }
        else if (sq_isarray(obj)) {
            depth++;
            if (depth > MAX_DEPTH) {
                errorString = _SC("Maximum serialization depth exceeded");
                return false;
            }

            uint8_t v = TP_ARRAY;
            SQArray *arr = _array(obj);
            SQInteger size = arr->Size();

            if (size == 0) {
                v |= 0; // empty array
                stream->Write(&v, sizeof(v));
            }
            else if (size <= UINT8_MAX) {
                v |= 1; // 1-byte size
                stream->Write(&v, sizeof(v));
                uint8_t size8 = static_cast<uint8_t>(size);
                stream->Write(&size8, sizeof(uint8_t));
            }
            else {
                v |= 2; // 4-byte size
                stream->Write(&v, sizeof(v));
                uint32_t size32 = static_cast<uint32_t>(size);
                stream->Write(&size32, sizeof(uint32_t));
            }

            for (SQInteger i = 0; i < size; ++i)
                if (!serializeObject(arr->_values[i]))
                    return false;

            depth--;
        }
        else if (sq_istable(obj)) {
            depth++;
            if (depth > MAX_DEPTH) {
                errorString = _SC("Maximum serialization depth exceeded");
                return false;
            }

            uint8_t v = TP_TABLE;
            SQTable *tbl = _table(obj);
            SQInteger size = tbl->CountUsed();

            if (size == 0) {
                v |= 0; // empty table
                stream->Write(&v, sizeof(v));
            }
            else if (size <= UINT8_MAX) {
                v |= 1; // 1-byte size
                stream->Write(&v, sizeof(v));
                uint8_t size8 = static_cast<uint8_t>(size);
                stream->Write(&size8, sizeof(uint8_t));
            }
            else {
                v |= 2; // 4-byte size
                stream->Write(&v, sizeof(v));
                uint32_t size32 = static_cast<uint32_t>(size);
                stream->Write(&size32, sizeof(uint32_t));
            }

            SQTable::_HashNode *node = tbl->_nodes;
            uint32_t count = tbl->_numofnodes_minus_one + 1;

            for (uint32_t i = 0; i < count; ++i) {
                if (sq_type(node[i].key) == SQ_FREE_KEY_TYPE)
                    continue;
                if (!serializeObject(node[i].key))
                    return false;
                if (!serializeObject(node[i].val))
                    return false;
            }

            depth--;
        }
        else if (sq_isinstance(obj)) {
            depth++;
            if (depth > MAX_DEPTH) {
                errorString = _SC("Maximum serialization depth exceeded");
                return false;
            }

            SQClass *cls = _instance(obj)->_class;
            if (!fillAvailableClassNamesOnDemand())
                return false;

            auto it = classTable.find(cls);
            if (it == classTable.end()) {
                errorString = _SC("Unsupported class for serialization");
                return false;
            }

            SQObjectPtr closure;
            if (_instance(obj)->Get(getstateProcName, closure)) {
                if (!sq_isclosure(closure) && !sq_isnativeclosure(closure)) {
                    errorString = _SC("Instance method __getstate must be a closure");
                    return false;
                }
            }
            else {
                errorString = _SC("Instance must have __getstate method for serialization");
                return false;
            }

            ClassDesc &classDesc = it->second;

            if (classDesc.index == -1) {
                // New class, write class name
                int classNameLen = strlen(classDesc.className);
                if (classNameLen > UINT8_MAX) {
                    errorString = _SC("Class name too long for serialization");
                    return false;
                }

                uint8_t classNameLen8 = static_cast<uint8_t>(classNameLen);
                uint8_t v = TP_INSTANCE | 0x0F; // 0x0F indicates a new class
                stream->Write(&v, sizeof(v));
                stream->Write(&classNameLen8, sizeof(classNameLen8));
                stream->Write(classDesc.className, classNameLen * sizeof(SQChar));

                classDesc.index = classCounter++;

                if (classCounter >= 0x0EFF) {
                    errorString = _SC("Too many different classes for serialization");
                    return false;
                }
            }
            else {
                // existing class, write index
                assert(classDesc.index >= 0 && classDesc.index < 0x0EFF);
                uint8_t v = TP_INSTANCE | (0x0F & (classDesc.index >> 8));
                uint8_t restOfIndex = static_cast<uint8_t>(classDesc.index & 0xFF);
                stream->Write(&v, sizeof(v));
                stream->Write(&restOfIndex, sizeof(restOfIndex));
            }

            SQInteger referenceStackTop = sq_gettop(vm);


            int countOfParams = getCountOfParams(closure);

            sq_pushobject(vm, closure);
            sq_pushobject(vm, obj); // this
            if (countOfParams > 1)
                sq_pushobject(vm, availableClasses);
            SQInteger top = sq_gettop(vm);
            SQRESULT res = sq_call(vm, countOfParams > 1 ? 2 : 1, SQTrue, SQTrue);
            if (SQ_FAILED(res)) {
                errorString = _SC("Failed to call __getstate method");
                sq_pop(vm, countOfParams > 1 ? 2 : 1); // pop arguments
                return false;
            }

            SQObject state;
            sq_getstackobj(vm, -1, &state);

            if (!serializeObject(state))
                return false;

            sq_pop(vm, 2);

            SQInteger newTop = sq_gettop(vm);
            if (newTop != referenceStackTop) {
                assert(newTop == referenceStackTop);
                errorString = _SC("Unexpected stack size after __getstate call");
                return false;
            }

            depth--;
            return true;
        }
        else {
            errorString = _SC("Unsupported object type for serialization");
            return false;
        }

        return true;
    }

    SQRESULT serialize(HSQUIRRELVM vm_, SQStream *dest_, SQObjectPtr obj, SQObjectPtr available_classes) {
        vm = vm_;
        stream = dest_;
        errorString = nullptr;
        stringCounter = 0;
        classCounter = 0;
        stringTable.clear();
        classTable.clear();
        stringList.clear();
        classList.clear();
        availableClasses = available_classes;
        depth = 0;

        uint8_t startMarker = START_MARKER;
        stream->Write(&startMarker, sizeof(startMarker));

        if (!serializeObject(obj))
            return sq_throwerror(vm, errorString ? errorString : _SC("Serialization failed"));

        uint8_t endMarker = END_MARKER;
        stream->Write(&endMarker, sizeof(endMarker));
        return SQ_OK;
    }

    bool unexpectedEndOfData()
    {
        errorString = _SC("Unexpected end of data during deserialization");
        return false;
    }

    bool deserializeObject()
    {
        uint8_t t = 0;
        if (stream->Read(&t, sizeof(t)) != sizeof(t))
            return unexpectedEndOfData();

        unsigned v = t & 0x0F;

        switch (t & 0xF0) {
            case TP_NULL:
                assert(v == 1);
                sq_pushnull(vm);
                return true;

            case TP_BOOL: {
                assert(v <= 1);
                sq_pushbool(vm, v);
                return true;
            }

            case TP_INTEGER: {
                switch (v) {
                    case 0: sq_pushinteger(vm, -1); return true;
                    case 1: sq_pushinteger(vm, 0); return true;
                    case 2: sq_pushinteger(vm, 1); return true;
                    case 3: sq_pushinteger(vm, 2); return true;
                    case 4: {
                        int8_t iv8 = 0;
                        if (stream->Read(&iv8, sizeof(iv8)) != sizeof(iv8))
                            return unexpectedEndOfData();
                        sq_pushinteger(vm, iv8);
                        return true;
                    }
                    case 5: {
                        int16_t iv16 = 0;
                        if (stream->Read(&iv16, sizeof(iv16)) != sizeof(iv16))
                            return unexpectedEndOfData();
                        sq_pushinteger(vm, iv16);
                        return true;
                    }
                    case 6: {
                        int32_t iv32 = 0;
                        if (stream->Read(&iv32, sizeof(iv32)) != sizeof(iv32))
                            return unexpectedEndOfData();
                        sq_pushinteger(vm, iv32);
                        return true;
                    }
                    case 7: {
                        int64_t iv64 = 0;
                        if (stream->Read(&iv64, sizeof(iv64)) != sizeof(iv64))
                            return unexpectedEndOfData();
                        sq_pushinteger(vm, iv64);
                        return true;
                    }
                    default:
                        errorString = _SC("Invalid integer type during deserialization");
                        return false;
                }
                break;
            }

            case TP_FLOAT: {
                switch (v) {
                    case 0: sq_pushfloat(vm, -1.0f); return true;
                    case 1: sq_pushfloat(vm, -0.5f); return true;
                    case 2: sq_pushfloat(vm, 0.0f); return true;
                    case 3: sq_pushfloat(vm, 0.5f); return true;
                    case 4: sq_pushfloat(vm, 1.0f); return true;
                    case 5: sq_pushfloat(vm, 2.0f); return true;
                    case 6: {
                        float ffv = 0.0f;
                        if (stream->Read(&ffv, sizeof(ffv)) != sizeof(ffv))
                            return unexpectedEndOfData();
                        sq_pushfloat(vm, static_cast<SQFloat>(ffv));
                        return true;
                    }
                    case 7: {
                        double dfv = 0.0;
                        if (stream->Read(&dfv, sizeof(dfv)) != sizeof(dfv))
                            return unexpectedEndOfData();
                        sq_pushfloat(vm, static_cast<SQFloat>(dfv));
                        return true;
                    }
                    default:
                        errorString = _SC("Invalid float type during deserialization");
                        return false;
                }
                break;
            }

            case TP_STRING: {
                uint32_t len = 0;
                const SQChar *data = nullptr;

                if (v < 4) {
                    switch (v) {
                        case 0: sq_pushstring(vm, _SC(""), 0); return true; // Empty string
                        case 1: {
                            uint8_t len8 = 0;
                            if (stream->Read(&len8, sizeof(len8)) != sizeof(len8))
                                return unexpectedEndOfData();
                            len = static_cast<uint32_t>(len8);
                        }
                        break;
                        case 2: {
                            uint16_t len16 = 0;
                            if (stream->Read(&len16, sizeof(len16)) != sizeof(len16))
                                return unexpectedEndOfData();
                            len = static_cast<uint32_t>(len16);
                        }
                        break;
                        case 3: {
                            uint32_t len32 = 0;
                            if (stream->Read(&len32, sizeof(len32)) != sizeof(len32))
                                return unexpectedEndOfData();
                            len = len32;
                        }
                        break;
                    }
                    assert(len < 80 * 1024 * 1024); // 80 Mb string is too much
                    data = sq_getscratchpad(vm, len * sizeof(SQChar));
                    if (stream->Read((void *)data, len * sizeof(SQChar)) != len * sizeof(SQChar))
                        return unexpectedEndOfData();

                    SQObjectPtr s(SQString::Create(_ss(vm), data, len));
                    stringList.push_back(s);
                    vm->Push(s);

                    return true;
                }
                else // v >= 4
                {
                    uint32_t index = 0;
                    if (v == 4) {
                        uint8_t idx8 = 0;
                        if (stream->Read(&idx8, sizeof(idx8)) != sizeof(idx8))
                            return unexpectedEndOfData();
                        index = static_cast<uint32_t>(idx8);
                    }
                    else if (v == 5) {
                        uint16_t idx16 = 0;
                        if (stream->Read(&idx16, sizeof(idx16)) != sizeof(idx16))
                            return unexpectedEndOfData();
                        index = static_cast<uint32_t>(idx16);
                    }
                    else if (v == 6) {
                        if (stream->Read(&index, sizeof(index)) != sizeof(index))
                            return unexpectedEndOfData();
                    }
                    else {
                        errorString = _SC("Invalid string type during deserialization");
                        return false;
                    }

                    if (index >= stringList.size()) {
                        errorString = _SC("String index out of bounds during deserialization");
                        return false;
                    }

                    vm->Push(stringList[index]);
                    return true;
                }
            }

            case TP_ARRAY: {
                uint32_t size = 0;
                if (v == 0) {
                    sq_newarray(vm, 0); // empty array
                    return true;
                }
                else if (v == 1) {
                    uint8_t size8 = 0;
                    if (stream->Read(&size8, sizeof(size8)) != sizeof(size8))
                        return unexpectedEndOfData();
                    size = static_cast<uint32_t>(size8);
                }
                else if (v == 2) {
                    uint32_t size32 = 0;
                    if (stream->Read(&size32, sizeof(size32)) != sizeof(size32))
                        return unexpectedEndOfData();
                    size = size32;
                }
                else {
                    errorString = _SC("Invalid array type during deserialization");
                    return false;
                }

                assert(size < 80 * 1024 * 1024);

                sq_newarray(vm, size);
                for (uint32_t i = 0; i < size; ++i) {
                    sq_pushinteger(vm, i);
                    if (!deserializeObject()) {
                        sq_pop(vm, 2);
                        return false;
                    }
                    sq_rawset(vm, -3);
                }
                return true;
            }

            case TP_TABLE: {
                uint32_t size = 0;
                if (v == 0) {
                    sq_newtable(vm); // empty table
                    return true;
                }
                else if (v == 1) {
                    uint8_t size8 = 0;
                    if (stream->Read(&size8, sizeof(size8)) != sizeof(size8))
                        return unexpectedEndOfData();
                    size = static_cast<uint32_t>(size8);
                }
                else if (v == 2) {
                    uint32_t size32 = 0;
                    if (stream->Read(&size32, sizeof(size32)) != sizeof(size32))
                        return unexpectedEndOfData();
                    size = size32;
                }
                else {
                    errorString = _SC("Invalid table type during deserialization");
                    return false;
                }

                assert(size < 80 * 1024 * 1024);

                sq_newtableex(vm, size);
                for (uint32_t i = 0; i < size; ++i) {
                    if (!deserializeObject()) { // key
                        sq_pop(vm, 1);
                        return false;
                    }

                    if (!deserializeObject()) { // value
                        sq_pop(vm, 2);
                        return false;
                    }

                    sq_rawset(vm, -3);
                }
                return true;
            }

            case TP_INSTANCE: {
                fillAvailableClassNamesOnDemand();
                uint32_t index = 0;
                if (v == 0x0F) {
                    // New class, read class name
                    uint8_t classNameLen8 = 0;
                    if (stream->Read(&classNameLen8, sizeof(classNameLen8)) != sizeof(classNameLen8))
                        return unexpectedEndOfData();
                    uint32_t classNameLen = static_cast<uint32_t>(classNameLen8);
                    SQChar *className = sq_getscratchpad(vm, classNameLen * sizeof(SQChar));
                    if (stream->Read(className, classNameLen * sizeof(SQChar)) != classNameLen * sizeof(SQChar))
                        return unexpectedEndOfData();

                    SQString *classNameStr = SQString::Create(_ss(vm), className, classNameLen);

                    if (!sq_istable(availableClasses)) {
                        errorString = _SC("Instance found during deserialization, but available classes not set or not a table");
                        return false;
                    }

                    SQTable *tbl = _table(availableClasses);
                    SQObjectPtr classObj;
                    if (!tbl->Get(SQObjectPtr(classNameStr), classObj)) {
                        errorString = _SC("Class not found in available classes during deserialization");
                        return false;
                    }

                    assert(sq_isclass(classObj)); // must be already checked in fillAvailableClassNamesOnDemand()

                    classList.push_back(classObj);
                    index = classList.size() - 1;

                    // check number of arguments required by the class constructor (must accept 1 argument - the instance itself)
                    SQObjectPtr constructor;
                    if (_class(classObj)->GetConstructor(constructor)) {
                      if (!isClassConstructorValid(constructor))
                        return false;
                    }
                }
                else {
                    uint8_t restOfIndex = 0;
                    if (stream->Read(&restOfIndex, sizeof(restOfIndex)) != sizeof(restOfIndex))
                        return unexpectedEndOfData();
                    index = ((v & 0x0F) << 8) | static_cast<uint32_t>(restOfIndex);
                    if (index >= classList.size()) {
                        errorString = _SC("Class index out of bounds during deserialization");
                        return false;
                    }
                }

                SQObjectPtr classObj = classList[index];

                SQObjectPtr setStateClosure;
                if (_class(classObj)->Get(setstateProcName, setStateClosure)) {
                    if (!sq_isclosure(setStateClosure) && !sq_isnativeclosure(setStateClosure)) {
                        errorString = _SC("Class method __setstate must be a closure");
                        return false;
                    }
                }
                else {
                    errorString = _SC("Class must have __setstate method for deserialization");
                    return false;
                }

                // create instance

                SQInteger stackbase = sq_gettop(vm);

                sq_pushobject(vm, classObj);
                sq_pushnull(vm); // environment
                if (SQ_FAILED(sq_call(vm, 1, SQTrue, SQTrue))) {
                    errorString = _SC("Failed to instantiate class during deserialization");
                    sq_settop(vm, stackbase);
                    return false;
                }

                sq_remove(vm, -2); // remove class from stack, instance stays on stack

                sq_pushobject(vm, setStateClosure);
                sq_push(vm, -2); // instance

                if (!deserializeObject()) {
                    sq_pop(vm, 2);
                    return false;
                }

                int countOfSetStateParams = getCountOfParams(setStateClosure);
                if (countOfSetStateParams > 2)
                    sq_pushobject(vm, availableClasses);

                SQInteger top = sq_gettop(vm);
                SQRESULT res = sq_call(vm, countOfSetStateParams > 2 ? 3 : 2, SQTrue, SQTrue);
                if (SQ_FAILED(res)) {
                    errorString = _SC("Failed to call __setstate method");
                    sq_settop(vm, stackbase); // restore stack
                    return false;
                }

                sq_settop(vm, stackbase + 1); // instance is left on the stack
                return true;
            }

            default:
                errorString = _SC("Unsupported object type during deserialization");
                return false;
        }
    }

    bool isClassConstructorValid(SQObjectPtr &constructor)
    {
      if (sq_isclosure(constructor))
      {
        SQFunctionProto *func = _closure(constructor)->_function;
        SQInteger paramssize = func->_nparameters;
        SQInteger ndef = func->_ndefaultparams;
        SQInteger nargs = 1;
        if (func->_varparams)
          paramssize--;

        if (paramssize != nargs && !(ndef && nargs < paramssize && (paramssize - nargs) <= ndef))
        {
          errorString = _SC("Class constructor must accept call with all default arguments during deserialization");
          return false;
        }
        return true;
      }
      else if (sq_isnativeclosure(constructor))
      {
        SQNativeClosure *nativeClosure = _nativeclosure(constructor);
        if (nativeClosure->_nparamscheck > 1)
        {
          errorString = _SC("Class constructor must accept call with all default arguments during deserialization");
          return false;
        }
        return true;
      }

      errorString = _SC("Class constructor must be a closure or native closure during deserialization");
      return false;
    }

    SQRESULT deserialize(HSQUIRRELVM vm_, SQStream *src_, SQObjectPtr available_classes) {  // parsed value is left on the stack
        vm = vm_;
        stream = src_;
        errorString = nullptr;
        stringCounter = 0;
        classCounter = 0;
        stringTable.clear();
        classTable.clear();
        stringList.clear();
        classList.clear();
        availableClasses = available_classes;
        depth = 0;

        uint8_t startMarker = 0;
        if (stream->Read(&startMarker, sizeof(startMarker)) != sizeof(startMarker))
            return sq_throwerror(vm, _SC("Unexpected end of data during deserialization"));
        if (startMarker != START_MARKER)
            return sq_throwerror(vm, _SC("Invalid start marker during deserialization"));

        if (!deserializeObject())
            return sq_throwerror(vm, errorString ? errorString : _SC("Deserialization failed"));

        uint8_t endMarker = 0;
        if (stream->Read(&endMarker, sizeof(endMarker)) != sizeof(endMarker))
            return sq_throwerror(vm, _SC("Unexpected end of data during deserialization"));
        if (endMarker != END_MARKER)
            return sq_throwerror(vm, _SC("Invalid end marker during deserialization"));

        return SQ_OK;
    }
};


SQRESULT sqstd_serialize_object_to_stream(HSQUIRRELVM vm, SQStream *dest, SQObjectPtr obj, SQObjectPtr available_classes) {
    SQStreamSerializer serializer;
    return serializer.serialize(vm, dest, obj, available_classes);
}

SQRESULT sqstd_deserialize_object_from_stream(HSQUIRRELVM vm, SQStream *src, SQObjectPtr available_classes) {
    int prevTop = sq_gettop(vm);
    SQStreamSerializer serializer;
    SQRESULT res = serializer.deserialize(vm, src, available_classes);
    sq_settop(vm, prevTop + 1);
    return res;
}

