#pragma once


#define DAS_COMMENT(...)

#define DAS_STRINGIFY(x) #x
#define DAS_TOSTRING(x) DAS_STRINGIFY(x)
#define DAS_FILE_LINE_NAME(a,b) a " at line " DAS_TOSTRING(b)
#define DAS_FILE_LINE   DAS_FILE_LINE_NAME(__FILE__,__LINE__)
