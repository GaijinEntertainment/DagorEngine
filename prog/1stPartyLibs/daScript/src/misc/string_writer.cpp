#include "daScript/misc/platform.h"

#include "daScript/misc/string_writer.h"

namespace das {
    StringWriterTag HEX;
    StringWriterTag DEC;
    StringWriterTag FIXEDFP;
    StringWriterTag SCIENTIFIC;

    mutex TextPrinter::pmut;

    void TextPrinter::output() {
        lock_guard<mutex> guard(pmut);
        int newPos = tellp();
        if (newPos != pos) {
            string st(data.data() + pos, newPos - pos);
            printf("%s", st.c_str());
            fflush(stdout);
            pos = newPos;
        }
    }
}
