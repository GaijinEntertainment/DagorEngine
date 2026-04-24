#include <assert.h>
#include "global_state.h"

namespace SQCompilation
{

std::unordered_set<std::string> knownBindings;
std::unordered_set<std::string> fileNames;
std::unordered_map<std::string, std::vector<IdLocation>> declaredGlobals;
std::unordered_map<std::string, std::vector<IdLocation>> usedGlobals;

}
