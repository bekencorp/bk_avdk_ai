#ifndef AI_IOT_SDK_TLS_UTIL_H
#define AI_IOT_SDK_TLS_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

    char *generateSignature(const char *sn, const char *appKey, const char *appId,
                            const char *timestamp);
#ifdef __cplusplus
}
#endif
#endif // AI_IOT_SDK_TLS_UTIL_H