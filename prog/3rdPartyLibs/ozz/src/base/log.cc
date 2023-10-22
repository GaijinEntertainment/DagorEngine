//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "ozz/base/log.h"

#include <iomanip>
#include <sstream>

#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace log {

// Default log level initialization.
namespace {
Level log_level = kStandard;
}

Level SetLevel(Level _level) {
  const Level previous_level = log_level;
  log_level = _level;
  return previous_level;
}

Level GetLevel() { return log_level; }

LogV::LogV() : Logger(std::clog, kVerbose) {}

Log::Log() : Logger(std::clog, kStandard) {}

Out::Out() : Logger(std::cout, kStandard) {}

Err::Err() : Logger(std::cerr, kStandard) {}

Logger::Logger(std::ostream& _stream, Level _level)
    : stream_(_level <= GetLevel() ? _stream : *ozz::New<std::ostringstream>()),
      local_stream_(&stream_ != &_stream) {}
Logger::~Logger() {
  if (local_stream_) {
      ozz::Delete(&stream_);
  }
}

FloatPrecision::FloatPrecision(const Logger& _logger, int _precision)
    : precision_(_logger.stream().precision(_precision)),
      format_(_logger.stream().setf(std::ios_base::fixed,
                                    std::ios_base::floatfield)),
      stream_(_logger.stream()) {}
FloatPrecision::~FloatPrecision() {
  stream_.precision(precision_);
  stream_.setf(format_, std::ios_base::floatfield);
}

}  // namespace log
}  // namespace ozz
