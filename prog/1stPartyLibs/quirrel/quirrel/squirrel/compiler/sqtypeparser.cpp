#include "sqtypeparser.h"
#include <sq_char_class.h>


struct SQRawTypeDecl
{
    const char * names[3]; // only [0] is valid, the rest are synonyms
    SQUnsignedInteger32 typeMask;
};

static SQRawTypeDecl rawTypeDecls[] = {
    { { "bool", "boolean", NULL }, _RT_BOOL },
    { { "number", "num", NULL }, (_RT_FLOAT | _RT_INTEGER) },
    { { "int", "integer", NULL }, _RT_INTEGER },
    { { "float", "double", "real" }, _RT_FLOAT },
    { { "string", "str", NULL }, _RT_STRING },
    { { "table", "dict", "map" }, _RT_TABLE },
    { { "array", "list", "vector" }, _RT_ARRAY },
    { { "userdata", "user", "object" }, _RT_USERDATA },
    { { "function", "func", "closure" }, (_RT_CLOSURE | _RT_NATIVECLOSURE) },
    { { "generator", "gen", "yield" }, _RT_GENERATOR },
    { { "userpointer", "ptr", "pointer" }, _RT_USERPOINTER },
    { { "thread", "coroutine", "fiber" }, _RT_THREAD },
    { { "instance", "inst", "object" }, _RT_INSTANCE },
    { { "class", NULL, NULL }, _RT_CLASS },
    { { "weakref", "reference", "ref" }, _RT_WEAKREF },
    { { "null", "nil", "none" }, _RT_NULL },
    { { "any", NULL, NULL }, ~0u },
    { { NULL, NULL, NULL }, 0 }
};

static const char* skip_spaces(const char* s)
{
    while (*s && sq_isspace(*s))
        s++;
    return s;
}

static bool is_str_equal_ignore_case(const char* str1, const char* str2)
{
    while (*str1 && *str2 && sq_tolower(*str1) == sq_tolower(*str2))
    {
        str1++;
        str2++;
    }
    return *str1 == *str2;
}

static bool parse_identifier(SQVM* vm, const char*& s, SQObjectPtr& res)
{
    const char* p = s;
    if (!sq_isalpha(*p) && *p != '_')
        return false;
    p++;
    while (sq_isalnum(*p) || *p == '_')
        p++;
    res = SQString::Create(_ss(vm), s, p - s);
    s = p;
    return true;
}

bool sq_type_string_to_mask(const char* type_name, SQUnsignedInteger32& mask, const char*& suggestion)
{
    bool found = false;
    mask = 0;
    suggestion = nullptr;
    for (SQRawTypeDecl* decl = rawTypeDecls; decl->names[0]; decl++)
    {
        if (!strcmp(type_name, decl->names[0]))
        {
            mask |= decl->typeMask;
            found = true;
            break;
        }

        for (int syn = 0; syn < 3; syn++)
            if (decl->names[syn] && is_str_equal_ignore_case(type_name, decl->names[syn]))
                suggestion = decl->names[0];
    }
    return found;
}

static bool parse_type_mask(SQVM* vm, const char*& s, SQUnsignedInteger32& mask, SQInteger& error_pos, SQObjectPtr& error_string)
{
    mask = 0;
    const char* p = skip_spaces(s);
    bool hasBrackets = false;

    if (*p == '(')
    {
        hasBrackets = true;
        p++;
        p = skip_spaces(p);
    }

    for (;;)
    {
        SQObjectPtr typeName;
        if (!parse_identifier(vm, p, typeName))
        {
            error_pos = p - s;
            error_string = SQString::Create(_ss(vm), "Expected type name");
            return false;
        }

        const char* suggestion = nullptr;
        SQUnsignedInteger32 currentTypeMask = 0;

        bool found = sq_type_string_to_mask(_stringval(typeName), currentTypeMask, suggestion);

        if (!found)
        {
            error_pos = p - s;
            char buf[256];
            if (suggestion)
                scsprintf(buf, 256, "Invalid type name '%s', did you mean '%s'?", _stringval(typeName), suggestion);
            else
                scsprintf(buf, 256, "Invalid type name '%s'", _stringval(typeName));
            error_string = SQString::Create(_ss(vm), buf);
            return false;
        }

        mask |= currentTypeMask;

        p = skip_spaces(p);
        if (*p == '|')
        {
            p++;
            p = skip_spaces(p);
        }
        else
        {
            break;
        }
    }

    if (hasBrackets)
    {
        if (*p != ')')
        {
            error_pos = p - s;
            error_string = SQString::Create(_ss(vm), "Expected ')' after type list");
            return false;
        }
        p++;
        p = skip_spaces(p);
    }

    s = p;
    return true;
}

bool sq_parse_function_type_string(SQVM* vm, const char* s, SQFunctionType& res, SQInteger& error_pos, SQObjectPtr& error_string)
{
    if (!s || !*s)
    {
        error_pos = 1;
        error_string = SQString::Create(_ss(vm), "Empty function type string");
        return false;
    }

    const char* p = skip_spaces(s);
    res.objectTypeMask = ~0u;
    res.returnTypeMask = _RT_NULL;
    res.requiredArgs = 0;
    res.ellipsisArgTypeMask = 0;
    res.argNames.clear();
    res.argTypeMask.clear();

    for (;;)
    {
        if (strncmp(p, "pure ", 5) == 0)
        {
            res.pure = true;
            p = skip_spaces(p + 5);
        }
        else if (strncmp(p, "nodiscard ", 10) == 0)
        {
            res.nodiscard = true;
            p = skip_spaces(p + 10);
        }
        else
            break;
    }

    if (*p == '(')
    {
        if (!parse_type_mask(vm, p, res.objectTypeMask, error_pos, error_string))
        {
            error_pos += p - s + 1;
            return false;
        }
        p = skip_spaces(p);
        if (*p != '.')
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), "Expected '.' after object type");
            return false;
        }
        p++;
        p = skip_spaces(p);
        if (!parse_identifier(vm, p, res.functionName))
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), "Expected function name after '.'");
            return false;
        }
    }
    else
    {
        SQObjectPtr identifier1;
        const char* p_initial = p;
        if (!parse_identifier(vm, p, identifier1))
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), "Expected function name");
            return false;
        }

        if (*p == '.')
        {
            p++;
            p = skip_spaces(p);
            if (!parse_identifier(vm, p, res.functionName))
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Expected function name after '.'");
                return false;
            }

            const char* typeStr = _stringval(identifier1);
            const char* typeStrPtr = typeStr;
            SQUnsignedInteger32 objectTypeMask;
            SQInteger error_pos_local;
            SQObjectPtr error_string_local;

            if (!parse_type_mask(vm, typeStrPtr, objectTypeMask, error_pos_local, error_string_local))
            {
                error_pos = (p_initial - s) + error_pos_local;
                error_string = error_string_local;
                return false;
            }

            if (*typeStrPtr != '\0')
            {
                error_pos = (p_initial - s) + (typeStrPtr - typeStr);
                error_string = SQString::Create(_ss(vm), "Invalid object type");
                return false;
            }

            res.objectTypeMask = objectTypeMask;
        }
        else
        {
            res.functionName = identifier1;
        }
    }

    p = skip_spaces(p);

    if (*p != '(')
    {
        error_pos = p - s + 1;
        error_string = SQString::Create(_ss(vm), "Expected '(' after function name");
        return false;
    }

    p++;
    p = skip_spaces(p);

    bool insideString = false;
    bool insideOptional = false;
    bool optionalBlockFinished = false;
    bool argumentProcessed = false;
    while (*p != ')')
    {
        p = skip_spaces(p);
        if (*p == '\0')
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), "Unterminated argument list");
            return false;
        }

        if (*p == '[')
        {
            if (insideOptional)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Nested optional blocks are not allowed");
                return false;
            }
            if (optionalBlockFinished)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Optional block must be the last argument");
                return false;
            }
            insideOptional = true;
            p++;
            p = skip_spaces(p);
            continue;
        }

        if (*p == ']')
        {
            if (!insideOptional)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Unmatched ']'");
                return false;
            }
            insideOptional = false;
            optionalBlockFinished = true;
            p++;
            p = skip_spaces(p);
            continue;
        }

        if (*p == ',')
        {
            if (!argumentProcessed)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Argument expected before ','");
                return false;
            }
            p++;
            p = skip_spaces(p);
            argumentProcessed = false;
            continue;
        }

        if (strncmp(p, "...", 3) == 0)
        {
            argumentProcessed = true;

            if (res.ellipsisArgTypeMask != 0)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Multiple ellipsis arguments");
                return false;
            }

            p += 3;
            p = skip_spaces(p);

            if (*p == ':')
            {
                p++;
                p = skip_spaces(p);
                if (!parse_type_mask(vm, p, res.ellipsisArgTypeMask, error_pos, error_string))
                {
                    error_pos += p - s + 1;
                    return false;
                }
            }
            else
            {
                res.ellipsisArgTypeMask = ~0u;
            }

            p = skip_spaces(p);

            if (*p != ')' && *p != ']')
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Expected ')' after ellipsis argument");
                return false;
            }

            continue;
        }

        if (optionalBlockFinished)
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), "Argument after optional block");
            return false;
        }

        SQObjectPtr argName;
        if (!parse_identifier(vm, p, argName))
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), "Expected argument name");
            return false;
        }

        argumentProcessed = true;

        SQUnsignedInteger32 argTypeMask = ~0u;
        p = skip_spaces(p);
        if (*p == ':')
        {
            p++;
            p = skip_spaces(p);
            if (!parse_type_mask(vm, p, argTypeMask, error_pos, error_string))
            {
                error_pos += p - s + 1;
                return false;
            }
        }
        else if (*p != ',' && *p != ')' && *p != ']' && *p != '[' && *p != '=')
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), "Expected ':' after argument name");
            return false;
        }

        SQObjectPtr defaultValue;
        char stringOpener = '\0';

        if (*p == '=')
        {
            p++;
            p = skip_spaces(p);

            char stack[40];
            int stackIndex = 0;

            const char * startDefaultValue = p;
            while (*p != '\0')
            {
                if (insideString)
                {
                    if (*p == stringOpener)
                    {
                        insideString = false;
                        p++;
                        continue;
                    }
                    else if (*p == '\\')
                    {
                        p++;
                        if (*p == '\0')
                        {
                            error_pos = p - s + 1;
                            error_string = SQString::Create(_ss(vm), "Unterminated string in default value");
                            return false;
                        }
                    }

                    p++;
                    continue;
                }
                else if (*p == '"' || *p == '\'')
                {
                    insideString = true;
                    stringOpener = *p;
                    p++;
                    continue;
                }

                if (stackIndex >= sizeof(stack) - 1)
                {
                    error_pos = p - s + 1;
                    error_string = SQString::Create(_ss(vm), "Default value too complex. Too many nested structures");
                    return false;
                }

                if (*p == '(')
                    stack[stackIndex++] = '(';
                else if (*p == ')')
                {
                    if (stackIndex == 0)
                        break; // end of default value

                    if (stack[stackIndex - 1] != '(')
                    {
                        error_pos = p - s + 1;
                        error_string = SQString::Create(_ss(vm), "Unmatched ')' in default value");
                        return false;
                    }
                    stackIndex--;
                }
                else if (*p == '[')
                    stack[stackIndex++] = '[';
                else if (*p == ']')
                {
                    if (stackIndex == 0)
                        break; // end of default value

                    if (stack[stackIndex - 1] != '[')
                    {
                        error_pos = p - s + 1;
                        error_string = SQString::Create(_ss(vm), "Unmatched ']' in default value");
                        return false;
                    }
                    stackIndex--;
                }
                else if (*p == '{')
                    stack[stackIndex++] = '{';
                else if (*p == '}')
                {
                    if (stackIndex == 0 || stack[stackIndex - 1] != '{')
                    {
                        error_pos = p - s + 1;
                        error_string = SQString::Create(_ss(vm), "Unmatched '}' in default value");
                        return false;
                    }
                    stackIndex--;
                }
                else if (*p == ',' && stackIndex == 0)
                {
                    break; // end of default value
                }

                p++;
                p = skip_spaces(p);
            }

            if (stackIndex > 0)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Unfinished default value, unmatched brackets");
                return false;
            }

            if (*p == '\0')
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Unterminated function type string");
                return false;
            }

            SQInteger defaultValueLength = p - startDefaultValue;
            if (defaultValueLength > 0)
                defaultValue = SQString::Create(_ss(vm), startDefaultValue, defaultValueLength);
            else
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Expected default value after '='");
                return false;
            }
        }
        else // have no default value
        {
            if (!insideOptional && res.defaultValues.size() > 0 && !sq_isnull(res.defaultValues.back()))
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), "Default value expected after optional argument");
                return false;
            }
        }

        res.argNames.push_back(argName);
        res.argTypeMask.push_back(argTypeMask);
        res.defaultValues.push_back(defaultValue);

        if (!insideOptional && sq_isnull(defaultValue))
        {
            res.requiredArgs++;
        }

        p = skip_spaces(p);
        if (*p == ',')
        {
            p++;
            p = skip_spaces(p);
        }
        else if (*p != ')' && *p != ']' && *p != '[')
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), "Expected ',' or ')'");
            return false;
        }
    }

    // *p == ')'

    if (insideOptional)
    {
        error_pos = p - s + 1;
        error_string = SQString::Create(_ss(vm), "Unmatched '['");
        return false;
    }

    if (insideString)
    {
        error_pos = p - s + 1;
        error_string = SQString::Create(_ss(vm), "Unterminated string in function type");
        return false;
    }

    p++;
    p = skip_spaces(p);

    if (*p == ':')
    {
        p++;
        p = skip_spaces(p);
        if (!parse_type_mask(vm, p, res.returnTypeMask, error_pos, error_string))
        {
            error_pos += p - s + 1;
            return false;
        }
    }

    p = skip_spaces(p);
    if (*p != '\0')
    {
        error_pos = p - s + 1;
        error_string = SQString::Create(_ss(vm), "Unexpected characters after function type string");
        return false;
    }

    return true;
}


void sq_stringify_type_mask(char* buffer, int buffer_length, SQUnsignedInteger32 mask)
{
    assert(buffer_length > 128);

    if ((mask & _RT_CLOSURE) || (mask & _RT_NATIVECLOSURE))
        mask |= _RT_CLOSURE | _RT_NATIVECLOSURE;

    char* p = buffer;
    char* end = buffer + buffer_length - 1;

    auto append = [&](const char* str)
    {
        while (*str && p < end)
            *p++ = *str++;
    };

    if (mask == ~0u)
    {
        append("any");
        *p = '\0';
        return;
    }

    bool first = true;
    for (const SQRawTypeDecl* decl = rawTypeDecls; decl->names[0]; decl++)
        if ((mask & decl->typeMask) == decl->typeMask)
        {
            if (!first)
                append("|");

            append(decl->names[0]);
            first = false;
            mask &= ~decl->typeMask;
        }

    *p = '\0';
}


SQObjectPtr sq_stringify_function_type(SQVM* vm, const SQFunctionType& ft)
{
    const SQInteger bufferSize = 2048; // 2KB buffer
    char buffer[bufferSize];
    char* p = buffer;
    char* end = buffer + bufferSize - 1;

    if (sq_isnull(ft.functionName))
    {
        SQObjectPtr tmp;
        return tmp;
    }

    auto append = [&](const char* str)
    {
        while (*str && p < end)
        {
            *p++ = *str++;
        }
    };

    auto appendTypeMask = [&](SQUnsignedInteger32 typeMask) {
        if (typeMask == ~0u)
        {
            append("any");
            return;
        }

        bool first = true;
        for (const SQRawTypeDecl* decl = rawTypeDecls; decl->names[0]; decl++)
        {
            if ((typeMask & decl->typeMask) == decl->typeMask)
            {
                if (!first)
                {
                    append("|");
                }
                append(decl->names[0]);
                first = false;
                typeMask &= ~decl->typeMask;
            }
        }
    };

    if (ft.pure)
        append("pure ");

    if (ft.nodiscard)
        append("nodiscard ");

    if (ft.objectTypeMask != ~0u)
    {
        bool isComplexType = true;
        for (const SQRawTypeDecl* decl = rawTypeDecls; decl->names[0]; decl++)
            if (ft.objectTypeMask == decl->typeMask)
            {
                isComplexType = false;
                break;
            }

        if (isComplexType)
            append("(");

        appendTypeMask(ft.objectTypeMask);

        if (isComplexType)
            append(")");

        append(".");
    }

    append(_stringval(ft.functionName));

    append("(");
    for (SQInteger i = 0; i < ft.argNames.size(); i++)
    {
        if (i > 0)
            append(", ");

        if (i == ft.requiredArgs)
            append("[");

        append(_stringval(ft.argNames[i]));

        if (ft.argTypeMask[i] != ~0u)
        {
            append(": ");
            appendTypeMask(ft.argTypeMask[i]);
        }

        if (!sq_isnull(ft.defaultValues[i]))
        {
            append(" = ");
            if (sq_isstring(ft.defaultValues[i]))
                append(_stringval(ft.defaultValues[i]));
            else
                append("<default value>");
        }
    }

    if (ft.argNames.size() > ft.requiredArgs)
        append("]");

    if (ft.ellipsisArgTypeMask != 0)
    {
        if (ft.argNames.size() > 0)
        {
            append(", ");
        }
        append("...");
        if (ft.ellipsisArgTypeMask != ~0u)
        {
            append(": ");
            appendTypeMask(ft.ellipsisArgTypeMask);
        }
    }

    append(")");

    if (ft.returnTypeMask != _RT_NULL)
    {
        append(": ");
        appendTypeMask(ft.returnTypeMask);
    }

    *p = '\0';

    return SQObjectPtr(SQString::Create(_ss(vm), buffer, p - buffer));
}
