#include "lingxin_tls_utils.h"
#include "lingxin_common.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include <ctype.h>
#include <stddef.h>

// 实现 HMAC-SHA1 加密逻辑
static char *sdk_hmac_sha1(const char *key, const char *data) {
  unsigned char output[20]; // SHA1 的输出长度为 20 字节
  size_t output_len = sizeof(output);

  // 执行 HMAC-SHA1 加密
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t *md_info;
  int ret = 0;

  // 初始化 MD 上下文
  mbedtls_md_init(&ctx);

  // 获取 SHA1 的 MD 信息
  md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  if (md_info == NULL) {
    logPrintf("无法获取 SHA1 MD 信息\n");
    mbedtls_md_free(&ctx);
    return NULL;
  }

  // 设置 HMAC 上下文
  if ((ret = mbedtls_md_setup(&ctx, md_info, 1)) != 0) {
    logPrintf("HMAC 上下文设置失败，错误码: %d", ret);
    mbedtls_md_free(&ctx);
    return NULL;
  }

  // 设置 HMAC 密钥
  if ((ret = mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key,
                                    strlen(key))) != 0) {
    logPrintf("设置 HMAC 密钥失败，错误码: %d", ret);
    mbedtls_md_free(&ctx);
    return NULL;
  }

  // 输入数据
  if ((ret = mbedtls_md_hmac_update(&ctx, (const unsigned char *)data,
                                    strlen(data))) != 0) {
    logPrintf("HMAC 数据更新失败，错误码: %d", ret);
    mbedtls_md_free(&ctx);
    return NULL;
  }

  // 生成 HMAC-SHA1 输出
  if ((ret = mbedtls_md_hmac_finish(&ctx, output)) != 0) {
    logPrintf("HMAC 计算失败，错误码: %d", ret);
    mbedtls_md_free(&ctx);
    return NULL;
  }

  // 清理上下文
  mbedtls_md_free(&ctx);

  // 使用 mbedtls 的 Base64 编码
  char base64_output[64]; // Base64 编码长度取决于输入长度，20 字节的输出需要 28
                          // 字节
  size_t base64_len = 0;
  ret = mbedtls_base64_encode((unsigned char *)base64_output,
                              sizeof(base64_output), &base64_len, output,
                              output_len);
  if (ret != 0) {
    logPrintf("Base64 编码失败，错误码: %d", ret);
    return NULL;
  }

  // 分配存储结果字符串的内存
  char *base64_result = malloc(base64_len + 1);
  if (!base64_result) {
    logPrintf("内存分配失败\n");
    return NULL;
  }
  // 复制 Base64 编码结果
  memcpy(base64_result, base64_output, base64_len);
  base64_result[base64_len] = '\0'; // 添加字符串终止符

  return base64_result;
}

/**
 * Helper function to check if a character is unreserved in URL encoding.
 * Unreserved characters are alphanumeric or one of "-_.~".
 */
static int isUnreserved(unsigned char c) {
  return (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~');
}

/**
 * Helper function to convert a byte to a hex string.
 * @param dest Destination buffer to store the hex representation (must have at
 * least 3 bytes).
 * @param byte The byte to convert.
 */
static void byteToHex(char *dest, unsigned char byte) {
  const char *hex_digits = "0123456789ABCDEF";
  dest[0] = '%';
  dest[1] = hex_digits[(byte >> 4) & 0xF]; // High nibble
  dest[2] = hex_digits[byte & 0xF];        // Low nibble
}

/**
 * URL-encodes the given input string.
 * @param input The input string to encode.
 * @param length The length of the input string. If set to -1, the function will
 * use strlen(input).
 * @return A newly allocated string containing the URL-encoded result, or NULL
 * on error. The caller is responsible for freeing the returned string.
 */
static char *urlEncode(const char *input, int length) {
  if (!input) {
    return NULL;
  }

  // Allocate the output buffer (worst case: every character is encoded as %XX)
  size_t output_size = length * 3 + 1; // +1 for null terminator
  char *output = (char *)malloc(output_size);
  if (!output) {
    return NULL; // Memory allocation failed
  }

  // Perform URL encoding
  char *dest = output;
  for (size_t i = 0; i < length; i++) {
    unsigned char c = (unsigned char)input[i];
    if (isUnreserved(c)) {
      *dest++ = c; // Copy unreserved characters as-is
    } else {
      byteToHex(dest, c); // Encode reserved characters as %XX
      dest += 3;          // Advance by 3 characters (%XX)
    }
  }

  *dest = '\0'; // Null-terminate the resulting string
  return output;
}

char *generateSignature(const char *sn, const char *appKey, const char *appId,
                        const char *timestamp) {
  const char *snEncode = urlEncode(sn, strlen(sn));
  if (!snEncode) {
    snEncode = sn;
  }
  const char *appIdEncode = urlEncode(appId, strlen(appId));
  if(!appIdEncode) {
    appIdEncode = appId;
  }
  char *hmacValue = snprintfWithMalloc("app_id=%s&sn=%s&timestamp=%s", appIdEncode,
                                       snEncode, timestamp);
  logPrintf("hmacValue:%s", hmacValue);
  logPrintf("appKey:%s", appKey);

  char *result = sdk_hmac_sha1(appKey, hmacValue);
  logPrintf("signature:%s", result);

  if (!result) {
    logPrintf("Signature generatte fail");
    return NULL;
  }
  free(hmacValue);
  return result;
}