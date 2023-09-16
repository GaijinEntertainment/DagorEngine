#include "lottiekeypath.h"

V_BEGIN_NAMESPACE

LOTKeyPath::LOTKeyPath(const std::string &keyPath)
{
#ifdef LOTTIE_NOSTDSTREAM_SUPPORT
    int start = 0, i = 0;
    for (; i < keyPath.size(); i++)
      if (keyPath[i] == '.')
      {
        mKeys.push_back(keyPath.substr(start, i-start));
        i++;
        start = i;
      }
    mKeys.push_back(keyPath.substr(start, i-start));
#else
    std::stringstream ss(keyPath);
    std::string       item;

    while (getline(ss, item, '.')) {
        mKeys.push_back(item);
    }
#endif
}

bool LOTKeyPath::matches(const std::string &key, uint depth)
{
    if (skip(key)) {
        // This is an object we programatically create.
        return true;
    }
    if (depth > size()) {
        return false;
    }
    if ((mKeys[depth] == key) || (mKeys[depth] == "*") ||
        (mKeys[depth] == "**")) {
        return true;
    }
    return false;
}

uint LOTKeyPath::nextDepth(const std::string key, uint depth)
{
    if (skip(key)) {
        // If it's a container then we added programatically and it isn't a part
        // of the keypath.
        return depth;
    }
    if (mKeys[depth] != "**") {
        // If it's not a globstar then it is part of the keypath.
        return depth + 1;
    }
    if (depth == size()) {
        // The last key is a globstar.
        return depth;
    }
    if (mKeys[depth + 1] == key) {
        // We are a globstar and the next key is our current key so consume
        // both.
        return depth + 2;
    }
    return depth;
}

bool LOTKeyPath::fullyResolvesTo(const std::string key, uint depth)
{
    if (depth > mKeys.size()) {
        return false;
    }

    bool isLastDepth = (depth == size());

    if (!isGlobstar(depth)) {
        bool matches = (mKeys[depth] == key) || isGlob(depth);
        return (isLastDepth || (depth == size() - 1 && endsWithGlobstar())) &&
               matches;
    }

    bool isGlobstarButNextKeyMatches = !isLastDepth && mKeys[depth + 1] == key;
    if (isGlobstarButNextKeyMatches) {
        return depth == size() - 1 ||
               (depth == size() - 2 && endsWithGlobstar());
    }

    if (isLastDepth) {
        return true;
    }

    if (depth + 1 < size()) {
        // We are a globstar but there is more than 1 key after the globstar we
        // we can't fully match.
        return false;
    }
    // Return whether the next key (which we now know is the last one) is the
    // same as the current key.
    return mKeys[depth + 1] == key;
}

V_END_NAMESPACE