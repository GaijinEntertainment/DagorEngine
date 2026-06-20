#pragma once


#include "daScript/misc/platform.h"
namespace das {

    struct LineInfoArg;

    DAS_MOD_API void StdDlgInit();

    // shared impl
    DAS_MOD_API bool GetOkCancelFromUser(const char * caption, const char * body);
    DAS_MOD_API bool GetOkFromUser(const char * caption, const char * body);

    // C++ impl
	string GetSaveFileFromUser ( const char * initialFileName , const char * initialPath, const char * filter );
	string GetOpenFileFromUser ( const char * initialPath, const char * filter );

    // and das bindings
    class Context;
    DAS_MOD_API char * GetSaveFileDlg ( const char * initialFileName , const char * initialPath, const char * filter, Context * ctx, LineInfoArg * at );
    DAS_MOD_API char * GetOpenFileDlg ( const char * initialPath, const char * filter, Context * ctx, LineInfoArg * at );
}