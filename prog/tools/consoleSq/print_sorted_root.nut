local output = [];

local function string_hash(s)
{
  return "hash" + s.len();
}

local function param_to_string(v)
{
  local t = type(v);
  if (t == "null")
    return "null";

  if (t != "integer" && t != "bool" && t != "float" && t != "string")
    return "(" + t + ")";

  return (type(v) == "string") ? "\"" + v + "\"" : v;
}

local function function_params(func)
{
  local suffix = "(";
  local infos = func.getfuncinfos();
  if (infos.native)
  {
    local absParamsCheck = infos.paramscheck > 0 ? infos.paramscheck : -infos.paramscheck;
    for (local i = 1; i < absParamsCheck; i++)
    {
      if (i > 1)
        suffix += ",";
      suffix += "*";
    }

    if (infos.paramscheck < -1)
      suffix += ", ...";
    else if (infos.paramscheck < 0)
      suffix += "...";
  }
  else
  {
    local delim = "";
    for (local i = 0; i < infos.parameters.len(); i++)
    {
      if (i == 0 && infos.parameters[i] == "this")
        continue;

      suffix += delim + infos.parameters[i];

      local defOffset = infos.parameters.len() - infos.defparams.len();
      if (i >= defOffset)
        suffix += "=" + param_to_string(infos.defparams[i - defOffset]);

      delim = ", ";
    }
  }
  suffix += ")";

  return suffix;
}

local function dump_keys(table, prefix, depth)
{
  if (depth == 1 && prefix == "globals.")
    return;

  foreach (k, v in table)
  {
    local value = (type(v) == "string") ? string_hash(v) : v;
    local s = prefix + k + "|" + value;
    if (type(v) == "function")
      s += "|params:" + function_params(v);

    output.append(s);

    if (depth < 1 && (type(v) == "table" || type(v) == "class"))
      callee()(v, prefix + k + ".", depth + 1);
  }
}


local types = ["table", "array", "string", "integer", "generator", "closure", "thread", "class", "instance", "weakref"];
foreach (t in types)
  dump_keys(__get_type_delegates(t), "[type " + t + "].", 0);

dump_keys(::getroottable(), "", 0)
dump_keys(::getconsttable(), "", 0)
output.sort();
foreach (s in output) print(s + "\n");

