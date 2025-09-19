// 当前该配置仅启用HMAC-SHA1算法，后续可根据业务增删
// 启用 SHA1 算法
#define MBEDTLS_SHA1_C

// 启用 HMAC 功能
#define MBEDTLS_MD_C
#define MBEDTLS_MD5_C // 如果需要其他哈希算法，可以启用相关算法
#define MBEDTLS_SHA256_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_BASE64_C

// 禁用所有与 X.509 和 TLS 相关的功能
#undef MBEDTLS_X509_USE_C
#undef MBEDTLS_X509_CRT_PARSE_C
#undef MBEDTLS_SSL_TLS_C
#undef MBEDTLS_SSL_PROTO_TLS1_2
#undef MBEDTLS_SSL_SRV_C
#undef MBEDTLS_SSL_CLI_C

#include "mbedtls/check_config.h"
