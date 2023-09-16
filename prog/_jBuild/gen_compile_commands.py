#!/usr/bin/python3

import sys
import json
import os

known_compilers = ["g++", "gcc", "clang", "cl.exe", "orbis-clang.exe", "orbis-clang++.exe", "prospero-clang.exe", "clang-cl.exe"]

actions = []

cur_action = []
cur_num_line_breaks = 0
curdir = os.path.realpath(os.path.curdir)

def filter_action(raw_action):
    result = {}
    for line in raw_action:
        words = line.strip().split(" ")
        if len(words) == 0 or not words[0] in ["call", "call_filtered"]:
            continue
        tool = os.path.basename(words[1])
        if not tool in known_compilers:
            continue
        source = os.path.realpath(words[-1])
        words[-1] = source
        result["directory"] = curdir
        result["command"] = " ".join(words[1:])
        result["file"] = source
    return result

for line in sys.stdin:
    if line == "\n":
        cur_num_line_breaks += 1
        if cur_num_line_breaks == 2:
            action = filter_action(cur_action)
            if len(action) != 0:
                actions.append(action)
            cur_action = []
            continue
    else:
        cur_num_line_breaks = 0
        cur_action.append(line)

print(json.dumps(actions, indent=2))

