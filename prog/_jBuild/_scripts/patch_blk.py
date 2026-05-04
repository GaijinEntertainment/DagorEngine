import os
import re
import sys

dagor = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "../../.."))

sys.path.append(os.path.join(dagor, "prog/tools"))

from pythonCommon import datablock


def get_or_create_block_by_path(blk, path):
    for part in path.split("/"):
        child = blk.getBlockWithoutIncludes(part)
        if child is None:
            child = blk.createBlock(part)
        blk = child
    return blk


def parse_param_string(param_str):
    """Parse 'name:type=value' with a regex to avoid DataBlock grammar ambiguity.

    The DataBlock grammar tries vectorVal (real numbers) before stringVal, so an
    unquoted value like '39UjwB0z...' gets truncated to '39'. Direct regex parsing
    avoids this problem entirely.
    """
    m = re.match(r'^([\w.-]+)(?::(\w+))?=(.+)$', param_str, re.DOTALL)
    if not m:
        raise ValueError(f"Cannot parse param: {param_str!r}")
    name, typ, raw_value = m.group(1), m.group(2) or 't', m.group(3)

    if raw_value.startswith('"') and raw_value.endswith('"'):
        value = raw_value[1:-1]
    elif typ in ('i', 'i64'):
        value = int(raw_value)
    elif typ == 'r':
        value = float(raw_value)
    elif typ == 'b':
        value = raw_value.lower() in ('yes', 'true', 'on', '1')
    else:
        value = raw_value

    return name, typ, value


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <blk_file> [path/to/block/param:t=value ...]")
        sys.exit(1)

    blk_path = sys.argv[1]
    patch_strings = sys.argv[2:]

    blk = datablock.DataBlock(blk_path, preserve_formating=True)

    for patch_str in patch_strings:
        slash = patch_str.rfind("/")
        if slash == -1:
            block_path, param_str = None, patch_str
        else:
            block_path, param_str = patch_str[:slash], patch_str[slash + 1:]

        target = get_or_create_block_by_path(blk, block_path) if block_path else blk

        name, typ, value = parse_param_string(param_str)
        target.setParam(name, typ, value)

    blk.saveFile(blk_path)


if __name__ == "__main__":
    main()
