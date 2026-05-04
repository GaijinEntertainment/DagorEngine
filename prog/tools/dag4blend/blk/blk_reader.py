from .datablock             import datablock
from .blk_remove_comments   import text_remove_comments

def read_blk_file(filepath):
    with open(filepath, "r") as blk:
        raw_blk_lines = blk.readlines()
    blk_no_comments = text_remove_comments(raw_blk_lines)
    return blk_no_comments


def parse_blk_line(line, current_block, unprocessed_line):
#include
    include_start = line.find('include')
    if include_start > -1:
        include = line[include_start + 7:]
        include = include.replace('"', "")
        include = include.replace(' ', "")
        current_block.includes.append(include)
        return [current_block, '']
#parameter
    param_value_split = line.find('=')
    if param_value_split > -1:
        param, value = line.split("=")
        #value = value.replace(' ', '')
        if value.find('"') > -1:
            try:  # trying to remove quotes
                value = value[value.find('"')+1:]
                value = value[:value.find('"')]
            except:
                pass
        try:
            name, type = param.split(':')
        except:
            name = param
            type = None
        name = name.replace(" ", "")
        current_block.parameters[name] = {"type": type, "value": value}
        return [current_block, '']
#new datablock
    block_start = line.find('{')
    if block_start > -1:
        new_block = datablock()
        current_block.datablocks.append(new_block)
        new_block.parent = current_block
        block_type = line[:line.find('{')]
        block_type = unprocessed_line + block_type
        block_type = block_type.replace(" ", "")
        new_block.type = block_type
        return [new_block, '']
#databloclk closed
    block_end = line.find('}')
    if block_end > -1:
        return [current_block.parent, '']
# something else
    unprocessed_line += line
    return [current_block, unprocessed_line]


def parse_blk(filepath):
    blk_lines = read_blk_file(filepath)
    blk_lines = blk_lines.replace("\t", "  ")
    blk_lines = blk_lines.replace("{", "{\n")  # to properly process "block{param:type..."
    blk_lines = blk_lines.replace("}", "\n}\n")  # to properly process "...=value}"
    blk_lines = blk_lines.replace(";", "\n")  # to properly process "param:type=value; param:type=value"
    blk_lines = blk_lines.split("\n")
    root_block = datablock()
    current_block = root_block
    unprocessed_line = ""
    for line in blk_lines:
        current_block, unprocessed_line = parse_blk_line(line, current_block, unprocessed_line)
    if current_block != root_block:
        pass  # TODO: add errormessage
    return current_block  # must be root block
