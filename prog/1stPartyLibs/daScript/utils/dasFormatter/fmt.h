#pragma once

#include <string>
#include "formatter.h"


namespace das::format {

    struct Result {
        struct ErrorInfo {
            string content;
            string what;
        };
        std::optional<string> ok;
        std::optional<ErrorInfo> error;
    };

    /**
     * Run formatter on single file and it's content
     */
    Result transform_syntax(const string &filename, const string content, format::FormatOptions options = {});

    /**
     * Run formatter on following files
     * @return 0 if everything is fine
     */
    int run(FormatOptions opt, const vector<string> &files);
};
