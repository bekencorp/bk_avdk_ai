#ifndef AI_IOT_SDK_GENERATE_BY_LLM_H
#define AI_IOT_SDK_GENERATE_BY_LLM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
  STYLE_DEFAULT,           // 默认，
  STYLE_3D_CARTOON,        // 3D卡通
  STYLE_ANIME,             // 动漫
  STYLE_OIL_PAINTING,      // 油画
  STYLE_WATER_COLOR,       // 水彩
  STYLE_SKETCH,            // 素描
  STYLE_CHINESE_PAINTINGS, // 中国画
  STYLE_FLAT_ILLUSTRATION, // 扁平插画
} ImageStyle;

typedef void (*GenerateTextRequestCallback)(char *contents, int finish);

void generateText(const char *sn, const char *appKey, bool showLog,
                  const char *input, GenerateTextRequestCallback callback);

void generateImage(const char *sn, const char *appKey, bool showLog,
                   const char *prompt, int width, int height, ImageStyle style,
                   char **response);

void queryGenerateImageResult(const char *sn, const char *appKey, bool showLog,
                              const char *taskId, char **response);

#ifdef __cplusplus
}
#endif

#endif // AI_IOT_SDK_GENERATE_BY_LLM_H