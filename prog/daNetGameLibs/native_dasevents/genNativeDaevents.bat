%1 ../../prog/scripts/genDasevents.das -main main -- %2 %3 %4 ^
--module ../../prog/daNetGameLibs/native_dasevents/main/native_events.das ^
--cpp_out ../../prog/daNetGame/game ^
--cpp_file dasEvents ^
--globals_out sq_globals ^
--no_sq_stubs ^
--cpp_include \"net/recipientFilters.h\" ^
--config aot_config.blk --verbose