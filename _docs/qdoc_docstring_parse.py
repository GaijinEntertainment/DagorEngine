import re

# Typemask map per your definition
_TYPENAME_TO_MASK = {
    'bool': 'b',
    'number': 'n',
    'int': 'i',
    'float': 'f',
    'string': 's',
    'table': 't',
    'array': 'a',
    'userdata': 'u',
    'function': 'c',
    'generator': 'g',
    'userpointer': 'p',
    'thread': 'v',
    'instance': 'x',
    'class': 'y',
    'weakref': 'w',
    'null': 'o',
    'any': '.',
}

def type_to_mask(type_string):
    # Split by '|' for multiple accepted types, ignore spaces
    return '|'.join(
        _TYPENAME_TO_MASK.get(t.strip().lower(), '.')
        for t in type_string.replace(' ', '').split('|')
    )

def parse_docstring(docstring, comments=None):
    """
    Returns: dict {name, rtype, args, paramsnum, typemask, is_pure}
    """
    docstring = docstring.strip()

    # 1. Detect and remove 'pure' with flexible whitespace
    is_pure = False
    m = re.match(r'^pure\s+(.*)', docstring, re.IGNORECASE)
    if m:
        is_pure = True
        docstring = m.group(1).lstrip()

    # 2. Match function signature with flexible spaces:
    #   name ( arg1, [arg2: type = default] ): rtype
    fn_re = re.match(r'^(\w+)\s*\((.*)\)\s*:\s*([\w\.]+)', docstring)
    if not fn_re:
        print("Invalid docstring:", docstring)
        raise ValueError("Invalid docstring: " + docstring)
    name, argstr, rtype = fn_re.groups()

    # 3. Parse arguments:
    args = []
    typemask_parts = []
    paramsnum = 0

    # Robustly split arguments (bracket-safe)
    arg_parts = []
    cur = ''
    level = 0
    for c in argstr:
        if c == '[': level += 1
        if c == ']': level -= 1
        if c == ',' and level == 0:
            arg_parts.append(cur.strip())
            cur = ''
        else:
            cur += c
    if cur.strip():
        arg_parts.append(cur.strip())

    optional_found = False
    for part in arg_parts:
        if not part:
            continue
        part = part.strip()
        # Optional arg: [name: type = default]
        opt_match = re.match(r'^\[?(\w+)\s*:\s*([^\]?=]+)(?:=\s*([^\]?]+))?\]?$', part)
        # Required arg: name: type
        req_match = re.match(r'^(\w+)\s*:\s*([^=]+)$', part)
        if opt_match:
            pname, ptype, defval = opt_match.groups()
            mask = type_to_mask(ptype)
            args.append({
                'name': pname,
                'paramtype': ptype.strip(),
                'optional': True,
                'defvalue': defval.strip() if defval else None
            })
            typemask_parts.append(mask)
            optional_found = True
        elif req_match:
            pname, ptype = req_match.groups()
            mask = type_to_mask(ptype)
            args.append({
                'name': pname,
                'paramtype': ptype.strip(),
                'optional': False
            })
            typemask_parts.append(mask)
            paramsnum += 1
        else:
            # Argument may be name only (no type)
            bare_match = re.match(r'^\[?(\w+)\]?$', part)
            if bare_match:
                pname = bare_match.group(1)
                args.append({
                    'name': pname,
                    'paramtype': '.',
                    'optional': part.startswith('['),
                    'description':None,
                })
                typemask_parts.append('.')
                if not part.startswith('['):
                    paramsnum += 1
                else:
                    optional_found = True
            else:
                print(f"Error parsing: '{docstring}'\n  part:", part)
                raise ValueError(f"Invalid argument in docstring: '{part}'")

    # Typemask is a string like: "ni|p"
    typemask = ''.join(typemask_parts)
    if optional_found:
        paramsnum = -paramsnum

    return {
        "name": name,
        "rtype": rtype,
        "args": args,
        "paramsnum": paramsnum,
        "typemask": typemask,
        "is_pure": is_pure,
        "members": comments.splitlines() if comments else None, #FIXME: parse everything for params and return type and comments
    }
if __name__ == "__main__":
  ds = [
    "pure Color(r: number, g: number, b: number, [alpha: number]): int",
    "pure Color(r: number, g: number, b: number, [alpha: number = 255]): int",
    "pure Color(r: number, g: number, b: number, alpha: number = 255): int",
    "pure sw(percent: number): number",
    "pure sh(percent: number): number",
    "pure flex([weight: number]): userdata",
    "pure pw(percent: number): userdata",
    "pure ph(percent: number): userdata",
    "pure elemw(percent: number): userdata",
    "pure elemh(percent: number): userdata",
    "pure get_font_metrics(fontId: int, [fontHt: number]): table",
  ]
  for s in ds:
    print(parse_docstring(s))