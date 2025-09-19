#ifndef LINGXIN_TLS_UTIL_H
#define LINGXIN_TLS_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

    char *generateSignature(const char *sn, const char *appKey, const char *appId,
                            const char *timestamp);
#ifdef __cplusplus
}
#endif
#endif // LINGXIN_TLS_UTIL_H