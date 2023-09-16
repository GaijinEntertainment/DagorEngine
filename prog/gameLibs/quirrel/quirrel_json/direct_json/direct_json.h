#include <EASTL/string.h>
typedef struct SQVM *HSQUIRRELVM;

bool parse_json_directly_to_quirrel(HSQUIRRELVM vm, const char *start, const char *end, eastl::string &error_string);
eastl::string directly_convert_quirrel_val_to_json_string(HSQUIRRELVM vm, int stack_pos, bool pretty);
