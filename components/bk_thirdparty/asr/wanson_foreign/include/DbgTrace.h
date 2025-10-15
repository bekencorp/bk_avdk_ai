#ifndef _DBG_TRACE_H
#define _DBG_TRACE_H

#include <common/bk_include.h>

// Define one of following option.
#define SYS_LOG
//#define PRT_LOG
//#define NO_LOG

#if defined(SYS_LOG)
    #define DBG_TRACE          bk_printf
#elif defined(PRT_LOG)
    #define DBG_TRACE          printf
#elif  defined(NO_LOG)
    #define DBG_TRACE          1 ? (void)0 : EmptyTrace
#endif

#ifdef __cplusplus
extern "C" {
#endif

inline void EmptyTrace(const char *lpszFormat, ...) {(void)(lpszFormat);}

#ifdef __cplusplus
}
#endif

#endif //_DBG_TRACE_H
