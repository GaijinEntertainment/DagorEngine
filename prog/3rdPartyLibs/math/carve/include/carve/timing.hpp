// Copyright 2006-2015 Tobias Sargeant (tobias.sargeant@gmail.com).
//
// This file is part of the Carve CSG Library (http://carve-csg.com/)
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <carve/carve.hpp>

#ifndef CARVE_USE_TIMINGS
#define CARVE_USE_TIMINGS 0
#endif

namespace carve {

#if CARVE_USE_TIMINGS

class TimingName {
 public:
  TimingName(const char* name);
  int id;
};

class TimingBlock {
 public:
  /**
   * Starts timing at the end of this constructor, using the given ID. To
   * associate an ID with a textual name, use Timing::registerID.
   */
  TimingBlock(int id);
  TimingBlock(const TimingName& name);
  ~TimingBlock();
};

class Timing {
 public:
  /**
   * Starts timing against a particular ID.
   */
  static void start(int id);

  static void start(const TimingName& id) { start(id.id); }

  /**
   * Stops the most recent timing block.
   */
  static double stop();

  /**
   * This will print out the current state of recorded time blocks. It will
   * display the tree of timings, as well as the summaries down the bottom.
   */
  static void printTimings();

  /**
   * Associates a particular ID with a text string. This is used when
   * printing out the timings.
   */
  static void registerID(int id, const char* name);
};

#else

struct TimingName {
  TimingName(const char*) {}
};
struct TimingBlock {
  TimingBlock(int /* id */) {}
  TimingBlock(const TimingName& /* name */) {}
};
struct Timing {
  static void start(int /* id */) {}
  static void start(const TimingName& /* id */) {}
  static double stop() { return 0; }
  static void printTimings() {}
  static void registerID(int /* id */, const char* /* name */) {}
};

#endif
}  // namespace carve
