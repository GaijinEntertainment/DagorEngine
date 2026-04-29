#pragma once
#ifndef _SQSOURCELOC_H_
#define _SQSOURCELOC_H_

#include <cstdint>
#include <algorithm>

namespace SQCompilation {

struct SourceLoc {
    int32_t line;    // 1-based
    int32_t column;  // 0-based

    static SourceLoc invalid() { return {-1, -1}; }
    bool isValid() const { return line >= 0; }

    bool operator==(const SourceLoc &other) const {
        return line == other.line && column == other.column;
    }

    bool operator!=(const SourceLoc &other) const {
        return !(*this == other);
    }
};

struct SourceSpan {
    SourceLoc start;
    SourceLoc end;  // Exclusive (one past last char)

    static SourceSpan invalid() { return {SourceLoc::invalid(), SourceLoc::invalid()}; }
    bool isValid() const { return start.isValid() && end.isValid(); }

    static SourceSpan merge(const SourceSpan &a, const SourceSpan &b) {
        return {a.start, b.end};
    }

    int32_t textWidth() const {
        if (start.line != end.line) return 1;
        return std::max(1, end.column - start.column);
    }

    bool operator==(const SourceSpan &other) const {
        return start == other.start && end == other.end;
    }

    bool operator!=(const SourceSpan &other) const {
        return !(*this == other);
    }
};

} // namespace SQCompilation

#endif // _SQSOURCELOC_H_
