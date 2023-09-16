#pragma once

#include <sqrat.h>

namespace darg
{
class Element;
}

void darg_immediate_error(HSQUIRRELVM vm, const char *error); // error will be shown immediately, but without squirrel callstack
void darg_assert_trace_var(const char *message, const Sqrat::Object &container, const Sqrat::Object &key);
void darg_assert_trace_var(const char *message, const Sqrat::Object &container, int key);

void darg_report_unrebuildable_element(const darg::Element *elem, const char *main_message);
