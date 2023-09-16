#pragma once

#include "debug_break.h"

namespace das {
    template <typename T, int64_t ref_id = -1>
    struct InstanceDebugger {
        int64_t count = 0;
        void increment() {
            static int64_t COUNTER = 0;
            count = ++COUNTER;
            if (count == ref_id){
                os_debug_break();
            }
        }
        InstanceDebugger() {
            increment();
        }
        InstanceDebugger(const InstanceDebugger& other) {
            increment();
        }
        InstanceDebugger(InstanceDebugger&& other) {
            increment();
        }
        InstanceDebugger& operator=(const InstanceDebugger& other) {
            return *this;
        }
        InstanceDebugger& operator=(InstanceDebugger&& other) {
            return *this;
        }
    };
}