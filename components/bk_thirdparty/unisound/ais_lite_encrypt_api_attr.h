/*
 * Author: Zhu Jian bo
 */


#ifndef AIS_LITE_ENCRYPT_API_ATTR_H_
#define AIS_LITE_ENCRYPT_API_ATTR_H_




#if defined(_WIN32) || defined(_WIN64)
    #define _WINDOWS
#endif

#ifndef _WINDOWS
    #ifndef AIS_LITE_ENCRYPT_API_DEFAULT
        #define AIS_LITE_ENCRYPT_EXPORT __attribute__((visibility("hidden")))
    #else
        #define AIS_LITE_ENCRYPT_EXPORT __attribute__((visibility("default")))
    #endif

#else  // _WINDOWS
    #ifdef DLL_EXPORT
        #define AIS_LITE_ENCRYPT_EXPORT __declspec(dllexport)
    #else  // DLL_EXPORT
        #define AIS_LITE_ENCRYPT_EXPORT __declspec(dllimport)
#endif
#endif





#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif  // AIS_LITE_ENCRYPT_API_ATTR_H_
