
local dag_system = require_optional("dagor.system")
local argv = dag_system?.argv ?? ::__argv;
local print_table = true;
foreach (a in argv)
  if (a == "--dont-print-table")
    print_table = false;

::print("\n\n")

local output = []

local function dump_keys(table, prefix, depth)
{
  if (depth == 1 && prefix == "globals.")
    return;

  foreach (k, v in table)
  {
    local s = prefix + k;

    output.append(s);

    if (depth < 1 && (::type(v) == "table" || ::type(v) == "class"))
      ::callee()(v, prefix + k + ".", depth + 1);
  }
}

dump_keys(::getroottable(), "", 0);
dump_keys(::getconsttable(), "", 0);
output.sort();

if (print_table)
  foreach (s in output) ::print(".R. " + s + "\n");

try
{
  ::print(dump_table + "\n");
}
catch (e)
{
  return;
}

try
{
  local prevOutput = clone output;
  output = [];
  local table = dump_table();
  dump_keys(::getroottable(), "", 0)
  dump_keys(::getconsttable(), "", 0)
  output.sort();

  foreach (s in output)
    if (prevOutput.indexof(s) == null)
      if (print_table)
        ::print(".A. " + s + "\n");

  output = [];
  dump_keys(table, "", 0);

  foreach (s in output)
    if (print_table)
      ::print(".M. " + s + "\n");
}
catch (e)
{
  if (print_table)
    ::print(".E. fail to require\n");
}
