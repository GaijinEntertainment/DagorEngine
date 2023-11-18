#pragma once

namespace dabfg
{

// Prints a raw list of nodes and their declared resources.
// Useful for diagnosing inconsistent graphs when compilation fails.
class IGraphDumper
{
public:
  virtual void dumpRawUserGraph() const = 0;
};

} // namespace dabfg
