#pragma once

class DagorAsset;
class AScene;
class Node;
class ILogWriter;

bool load_skeleton(DagorAsset &a, const char *suffix_to_remove, int flags, ILogWriter &log, AScene &scene);

bool combine_skeleton(DagorAsset &a, int flags, ILogWriter &log, Node *root);

bool auto_complete_skeleton(DagorAsset &a, const char *suffix_to_remove, int flags, ILogWriter &log, Node *root);