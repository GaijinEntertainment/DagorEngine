#include "direct_json.h"
#include <debug/dag_debug.h>

#define assert G_ASSERT //-V1059

#include <squirrel.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqobject.h>
#include <squirrel/sqstring.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqarray.h>
#include <limits>
#include <EASTL/vector.h>
#include <EASTL/sort.h>


struct DirectJsonWriter
{
  HSQUIRRELVM vm;
  bool pretty;
  eastl::string &out;
  int depth;

  DirectJsonWriter(HSQUIRRELVM vm, bool pretty, eastl::string &out) : pretty(pretty), out(out), depth(0), vm(vm) {}

  char toHexDigit(int n)
  {
    if (n < 10)
      return '0' + n;
    return 'A' + n - 10;
  }

  void writeIndent()
  {
    if (!pretty)
      return;
    if (depth > 0)
      out.append(depth * 2, ' ');
  }

  void write(const char *str, int len) { out.append(str, len); }

  void write(const char *str) { out.append(str); }

  void write(char c) { out.push_back(c); }

  void writeNull() { write("null", 4); }

  void writeEscapedString(const char *str, int len)
  {
    bool needEscape = false;
    for (int i = 0; i < len; i++)
      if (uint8_t(str[i]) < 32 || str[i] == '"' || str[i] == '\\')
      {
        needEscape = true;
        break;
      }

    if (!needEscape)
    {
      write('"');
      write(str, len); // 1.5x faster than write symbol by symbol
      write('"');
      return;
    }

    write('"');
    for (int i = 0; i < len; i++)
    {
      char c = str[i];
      switch (c)
      {
        case '"': write("\\\"", 2); break;
        case '\\': write("\\\\", 2); break;
        case '\t': write("\\t", 2); break;
        case '\b': write("\\b", 2); break;
        case '\n': write("\\n", 2); break;
        case '\r': write("\\r", 2); break;
        case '\f': write("\\f", 2); break;
        case '\v': write("\\v", 2); break;
        default:
          if (uint8_t(c) >= 32)
            write(c);
          else
          {
            char buf[6] = {
              '\\',
              'u',
              '0',
              '0',
              toHexDigit(uint8_t(c) >> 4),
              toHexDigit(uint8_t(c) & 0xF),
            };
            write(buf, 6);
          }
      }
    }
    write('"');
  }

  void writeInt(SQInteger val)
  {
    if (val >= 0 && val <= 9)
    {
      write(char(val + '0'));
      return;
    }
    else if (val < 0 && val >= -9)
    {
      write('-');
      write(char(-val + '0'));
      return;
    }
    else if (val == std::numeric_limits<SQInteger>::min())
    {
      char buf[32];
      int len = snprintf(buf, sizeof(buf), "%lld", (long long)(std::numeric_limits<SQInteger>::min()));
      write(buf, len);
      return;
    }
    else if (val < 0)
    {
      write('-');
      val = -val;
    }

    char buf[32];
    int pos = sizeof(buf);

    while (val)
    {
      buf[--pos] = char(val % 10 + '0');
      val /= 10;
    }

    write(buf + pos, sizeof(buf) - pos);
  }

  void writeDouble(double val)
  {
    if (!isfinite(val)) // act like JSON writer in browsers
    {
      writeNull();
      return;
    }

    char buf[32];
    int len = snprintf(buf, sizeof(buf) - 2, "%.17g", val);

    for (int i = len - 2; i >= 0; i--)
      if (buf[i] == '.' || buf[i] == 'e' || buf[i] == 'E')
      {
        write(buf, len);
        return;
      }

    if (buf[len - 1] == '.')
    {
      buf[len++] = '0';
    }
    else
    {
      buf[len++] = '.';
      buf[len++] = '0';
    }
    write(buf, len);
  }

  void writeBool(bool val)
  {
    if (val)
      write("true", 4);
    else
      write("false", 5);
  }

  void writeKey(const char *key, int len)
  {
    writeEscapedString(key, len);
    if (pretty)
      write(": ", 2);
    else
      write(':');
  }

  void writeToString(const SQObjectPtr &obj)
  {
    switch (obj._type)
    {
      case OT_STRING: writeEscapedString(_stringval(obj), _string(obj)->_len); break;
      case OT_INTEGER: writeInt(obj._unVal.nInteger); break;
      case OT_FLOAT: writeDouble(obj._unVal.fFloat); break;
      case OT_BOOL: writeBool(obj._unVal.nInteger != 0); break;
      case OT_NULL: writeNull(); break;
      case OT_TABLE:
      {
        SQTable *t = _table(obj);
        int itemsCount = t->CountUsed();
        if (!itemsCount)
        {
          write("{}", 2);
          return;
        }

        SQObjectPtr refidx;
        SQInteger idx;
        SQInteger counter = 0;
        bool allKeysAreStrings = true;

        struct KeyVal
        {
          SQObjectPtr key;
          SQObjectPtr val;
        };

        eastl::vector<KeyVal> tableEntries;
        eastl::vector<int> tableEntriesIndices;
        tableEntries.resize(itemsCount + 1);
        tableEntriesIndices.resize(itemsCount + 1);

        for (int i = 0; i < int(tableEntriesIndices.size()); i++)
          tableEntriesIndices[i] = i;

        for (;;)
        {
          if (counter >= tableEntries.size())
          {
            KeyVal tmp;
            tableEntries.emplace_back(tmp);
            tableEntriesIndices.push_back(counter + 1);
            itemsCount++;
          }

          idx = t->Next(false, refidx, tableEntries[counter].key, tableEntries[counter].val);
          if (idx == -1)
            break;

          if (!sq_isstring(tableEntries[counter].key))
            allKeysAreStrings = false;

          refidx = idx;
          counter++;
        }

        tableEntries.resize(counter);
        tableEntriesIndices.resize(counter);
        itemsCount = counter;

        G_ASSERT(counter == tableEntries.size());

        if (allKeysAreStrings)
        {
          eastl::sort(tableEntriesIndices.begin(), tableEntriesIndices.end(),
            [&tableEntries](int a, int b) { return strcmp(_stringval(tableEntries[a].key), _stringval(tableEntries[b].key)) < 0; });
        }

        bool first = true;
        bool indentChanged = false;
        write('{');

        for (int i = 0; i < itemsCount; i++)
        {
          int index = tableEntriesIndices[i];
          SQObjectPtr &key = tableEntries[index].key;
          SQObjectPtr &val = tableEntries[index].val;

          if (!first)
          {
            write(',');
            if (pretty)
            {
              write('\n');
              writeIndent();
            }
          }
          else
          {
            if (pretty)
            {
              write('\n');
              depth++;
              indentChanged = true;
              writeIndent();
            }
            first = false;
          }

          if (sq_isstring(key))
            writeKey(_stringval(key), _string(key)->_len);
          else
          {
            SQObjectPtr str;
            if (!vm->ToString(key, str))
              write("\"<type conversion error>\"");
            else
              writeEscapedString(_stringval(str), _string(str)->_len);
            write(": ", 2);
          }

          writeToString(val);
        }

        if (indentChanged)
          depth--;

        if (pretty)
        {
          write('\n');
          writeIndent();
        }

        write('}');
      }
      break;

      case OT_ARRAY:
      {
        SQArray *array = _array(obj);
        bool first = true;
        bool indentChanged = false;
        write('[');

        bool eachElementOnNewLine = array->Size() > 5;
        int estimatedLength = 0;

        if (!eachElementOnNewLine && pretty)
          for (SQInteger i = 0; i < array->Size(); i++)
          {
            if (sq_isarray(array->_values[i]) && _array(array->_values[i])->Size() > 0)
            {
              eachElementOnNewLine = true;
              break;
            }
            else if (sq_istable(array->_values[i]) && _table(array->_values[i])->CountUsed() > 0)
            {
              eachElementOnNewLine = true;
              break;
            }
            else if (sq_isstring(array->_values[i]))
              estimatedLength += _string(array->_values[i])->_len;
            else
              estimatedLength += 6;

            if (estimatedLength > 50)
            {
              eachElementOnNewLine = true;
              break;
            }
          }

        for (SQInteger i = 0; i < array->Size(); i++)
        {
          if (!first)
          {
            write(',');
            if (pretty)
            {
              if (eachElementOnNewLine)
              {
                write('\n');
                writeIndent();
              }
              else
                write(' ');
            }
          }
          else
          {
            if (pretty && eachElementOnNewLine)
            {
              write('\n');
              depth++;
              indentChanged = true;
              writeIndent();
            }
            first = false;
          }

          writeToString(array->_values[i]);
        }

        if (indentChanged)
          depth--;

        if (pretty && eachElementOnNewLine)
        {
          write('\n');
          writeIndent();
        }
        write(']');
      }
      break;

      case OT_CLOSURE: write("\"<closure>\""); break;
      case OT_NATIVECLOSURE: write("\"<native closure>\""); break;
      case OT_GENERATOR: write("\"<generator>\""); break;
      case OT_USERDATA: write("\"<userdata>\""); break;
      case OT_USERPOINTER: write("\"<userpointer>\""); break;
      case OT_THREAD: write("\"<thread>\""); break;
      case OT_CLASS: write("\"<class>\""); break;
      case OT_INSTANCE: write("\"<instance>\""); break;
      case OT_WEAKREF: write("\"<weakref>\""); break;
      default: write("\"<unknown>\""); break;
    }
  }
};

eastl::string directly_convert_quirrel_val_to_json_string(HSQUIRRELVM vm, int stack_pos, bool pretty)
{
  eastl::string buf;
  buf.reserve(240);
  DirectJsonWriter writer(vm, pretty, buf);
  const SQObjectPtr &obj = stack_get(vm, stack_pos);
  writer.writeToString(obj);
  return buf;
}
