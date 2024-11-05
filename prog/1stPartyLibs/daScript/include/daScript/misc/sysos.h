#pragma once

namespace das {

    #define DAS_MAX_HW_BREAKPOINTS  16

    enum HwBpType {
        HwBp_Execute =      0,
        HwBp_Write =        1,
        HwBp_ReadWrite =    3,
    };

    enum HwBpSize {
        HwBp_1 =    0,
        HwBp_2 =    1,
        HwBp_4 =    3,  // yes 3
        HwBp_8 =    2,
    };

    string getExecutableFileName ( void );
    string getDasRoot ( void );
    void setDasRoot ( const string & dr );

    string normalizeFileName ( const char * fileName );

    string get_prefix ( const string & req );   // blah.... \ foo.bar - returns blah....
    string get_suffix ( const string & req );   // blah.... \ foo.bar - returns foo.bar

    void * loadDynamicLibrary ( const char * fileName );
    void * getFunctionAddress ( void * module, const char * func );
    void * getLibraryHandle ( const char * moduleName );
    bool closeLibrary ( void * module );


    void hwSetBreakpointHandler ( void (* handler ) ( int, void * ) );
    int hwBreakpointSet ( void * address, int len, int when );
    bool hwBreakpointClear ( int bp_index );
}
