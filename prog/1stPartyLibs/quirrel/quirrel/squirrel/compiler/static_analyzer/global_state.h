#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "checker_visitor.h"

namespace SQCompilation
{

extern std::unordered_set<std::string> knownBindings;
extern std::unordered_set<std::string> fileNames;
extern std::unordered_map<std::string, std::vector<IdLocation>> declaredGlobals;
extern std::unordered_map<std::string, std::vector<IdLocation>> usedGlobals;

}
