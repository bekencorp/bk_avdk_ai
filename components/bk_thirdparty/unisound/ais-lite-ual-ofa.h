/*
 * Copyright 2021 Unisound AI Technology Co., Ltd.
 * Author: Zhu Jian Bo
 * All rights reserved.
 */

#ifndef AIS_LIST_UAL_OFA_H_
#define AIS_LIST_UAL_OFA_H_

#include <stdint.h>
#include <stddef.h>

// ais-lite encrypt相关头文件
#include "./ais_lite_encrypt_data_struct.h"
#include "./ais_lite_encrypt_main.h"
#include "./ais_lite_version.h"
#include "./ais_lite_log.h"


#if defined(_WIN32) || defined(_WIN64)
    #define _WINDOWS
#endif

#ifndef _WINDOWS
    #ifdef LIB_KWS_SET_API_HIDDEN
        #define LIB_KWS_API_EXPORT __attribute__((visibility("hidden")))
    #else
        #define LIB_KWS_API_EXPORT __attribute__((visibility("default")))
    #endif

#else
    #ifdef DLL_EXPORT
        #define LIB_KWS_API_EXPORT __declspec(dllexport)
    #else
        #define LIB_KWS_API_EXPORT __declspec(dllimport)
    #endif
#endif


/**
 * @brief UalOFARecognizeAsync回调函数结构体
 */
typedef struct {
  /** 识别状态，与UalOFARecognize返回值一致 */
  int recog_status;
  /** 是否需要新数据 */
  int need_input;
} AisLiteKWSRecognizeCallbackData;


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 创建并从内存初始化 KWS LP 引擎
 *
 * @param am_buffer LP模型在内存中的起始地址,KWS运行期间不允许改变(包括释放)
 * @param grammar_buffer
 * LP语法在内存中的起始地址,KWS运行期间不允许改变(包括释放)
 * @param info:设备信息
 * 
 * @return KWS 引擎句柄
 *
 * @attention 仅适用于LP引擎
 */
LIB_KWS_API_EXPORT void* AisLiteUalOFAInitializeFromBuffer(const char* am_buffer,
                                             const char* grammar_buffer, const char* ai_code_tpye, const char* authCode,
                                             DeviceInfo* info);

/**
 * @brief 设置处于激活状态的引擎的属性值
 *
 * @param handle KWS 引擎句柄
 * @param id 属性id
 * @param value 属性值
 *
 * @return #EngineStatusCode
 *
 * @note 更多细节请参考 wiki 或者 @ref ofa_consts.h
 */
LIB_KWS_API_EXPORT int AisLiteUalOFASetOptionInt(void* handle, int id,
                                               int value);
/**
 * @brief 获取处于激活状态的引擎的属性值
 *
 * @param handle KWS 引擎句柄
 * @param id 属性id
 *
 * @return 属性值
 *    @retval -65535 不支持该属性
 *
 * @note 更多细节请参考 wiki 或者 @ref ofa_consts.h
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAGetOptionInt(void* handle, int id);

/**
 * @brief 设置处于激活状态的引擎的字符串类型属性值
 *
 * @param handle KWS 引擎句柄
 * @param id 属性id
 * @param value 属性值
 *
 * @return #EngineStatusCode
 *
 * @note 更多细节请参考 wiki 或者 @ref ofa_consts.h
 */
LIB_KWS_API_EXPORT int AisLiteUalOFASetOptionString(void* handle, int id,
                                                  const char* value);
/**
 * @brief 获取处于激活状态的引擎的字符串类型属性值
 *
 * @param handle KWS 引擎句柄
 * @param id 属性id
 *
 * @return 属性值
 *    @retval NULL 不支持该属性
 *
 * @note 更多细节请参考 wiki 或者 @ref ofa_consts.h
 */
LIB_KWS_API_EXPORT const char* AisLiteUalOFAGetOptionString(void* handle, int id);

/**
 * @brief 使用当前引擎开始识别
 *
 * @param handle KWS 引擎句柄
 * @param grammar_domain 用到的识别语法的domain
 * @param am_id 声学模型id，具体数值请咨询模型提供方（LP引擎不生效）
 * @param memory_buffer 传入给引擎内存地址,KWS运行期间不允许改变(包括释放)
 * @param memory_size 传入内存空间的大小(bytes), 大小会影响引擎性能
 *                    唤醒, 单唤醒词最小值: 12KiB
 *                    识别, 100命令词最小值: 48KiB
 *                    当词表增大时，请增大memory_size值
 *
 * @return #EngineStatusCode
 *
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAStart(void* handle,
                                        const char* grammar_domain, int am_id,
                                        void* memory_buffer,
                                        int32_t memory_size);
/**
 * @brief 输入数据给当前处于激活状态的引擎
 *
 * @param handle KWS 引擎句柄
 * @param raw_audio 音频数据
 * @param len 音频数据的长度（单位：Bytes），不要超过640 Bytes
 *
 * @return #EngineStatusCode
 *    @retval #ASR_RECOGNIZER_PARTIAL_RESULT 调用 #AisLiteUalOFAGetResult 获取识别结果
 *    @retval #ASR_RECOGNIZER_OK             执行正常，无识别结果
 *    @retval #ASR_FATAL_ERROR    执行出错
 */
LIB_KWS_API_EXPORT int AisLiteUalOFARecognize(void* handle,
                                            signed char* raw_audio, int len);

/**
 * @brief 输入数据给当前处于激活状态的引擎 (异步方式)
 *
 * @param handle KWS 引擎句柄
 * @param data 结果结构体
 * @param raw_audio 音频数据
 * @param len 音频数据的长度（单位：Bytes），不要超过640 Bytes
 *
 * @return
 *   @retval #ASR_RECOGNIZER_OK 执行正常
 *   @retval #ASR_FATAL_ERROR 执行错误
 */
LIB_KWS_API_EXPORT int32_t AisLiteUalOFARecognizeAsync(void* handle, signed char* raw_audio,
                                        int len,
                                        AisLiteKWSRecognizeCallbackData* data);
/**
 * @brief 输入数据给当前处于激活状态的引擎 (异步方式)
 *
 * @param handle KWS 引擎句柄
 * @param data 结果结构体
 * @param feat logfbank
 * @param len 特征长度
 *
 * @return
 *   @retval #ASR_RECOGNIZER_OK 执行正常
 *   @retval #ASR_FATAL_ERROR 执行错误
 */
LIB_KWS_API_EXPORT int32_t AisLiteUalOFARecognizeAsyncFeat(void* handle, float* feat, int len,
                                            AisLiteKWSRecognizeCallbackData* data);

/**
 * @brief 获取处于激活状态的引擎的识别结果
 *
 * @param handle KWS 引擎句柄
 * @return 识别结果（C语言字符串）
 *    @retval NULL 如果引擎没有识别结果
 */
LIB_KWS_API_EXPORT const char* AisLiteUalOFAGetResult(void* handle);

/**
 * @brief 判断语音是否结束
 *
 * @param handle KWS 引擎句柄
 * @return #EngineOneshotStatusCode
 *
 * @note 当唤醒之后，判断后面一段时间内是否还有语音
 *
 */
LIB_KWS_API_EXPORT int AisLiteUalOFACheckWavEnd(void* handle);

/**
 * @brief 停止当前引擎的识别
 * @param handle KWS 引擎句柄
 * @details 如果在此之前，引擎没有返回有效的结果，
 *         那么在调用此API后，要调用AisLiteUalOFAGetResult去尝试拿下结果
 *
 * @return #EngineStatusCode
 *    @retval ASR_RECOGNIZER_OK 停止成功
 *    @retval ASR_STOP_PENDING
 *            等待状态，调用者需要反复调用直到返回ASR_RECOGNIZER_OK
 *
 * @note 仅fengniaoL平台可能返回ASR_STOP_PENDING
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAStop(void* handle);

/**
 * @brief 释放所有引擎（STD和LP），回收内存。

 * @param handle KWS 引擎句柄
 * @details 释放后，除非重新初始化，否则引擎将无法继续使用
 *
 * @return void
 */
LIB_KWS_API_EXPORT void AisLiteUalOFARelease(void* handle);

/**
 * @brief 重置当前引擎为初始状态
 *
 * @param handle KWS 引擎句柄
 * @param info:设备信息
 * @return #EngineStatusCode
 * @attention Nothing is done in the implementation
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAReset(void* handle);

/**
 * @brief 获得版本信息(引擎库版本和构建版本)
 *
 * @param handle KWS 引擎句柄
 * @return 描述版本信息的字符串
 */
LIB_KWS_API_EXPORT const char* AisLiteUalOFAGetVersion(void* handle);

/**
 * @brief 获取引擎类型
 *
 * @param handle KWS 引擎句柄
 * @return #EngineTypeId
 *    @retval #KWS_LP_ENGINE 只有LP引擎
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAGetEngineType(void* handle);

/**
 * @brief 输入密码供引擎验证
 *
 * @param handle KWS 引擎句柄
 * @param env 密码字符串(C-string)
 *
 * @return #EngineStatusCode
 *    @retval #ASR_FATAL_ERROR 验证失败
 *    @retval #ASR_RECOGNIZER_OK 验证成功
 *
 * @attention 仅适用于启用加密的STD引擎
 */
LIB_KWS_API_EXPORT int AisLiteUalOFASetEnv(void* handle, void* env);

/**
 * @brief 获取引擎加密方式
 *
 * @param handle KWS 引擎句柄
 * @return EncryptionValue
 *    @retval 0 没有任何加密
 *    @retval 第4个bit位为1 包名限制
 *
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAGetEncryptionScheme(void* handle);

/**
 * @brief 获取当前 active am 的ID
 * @param handle KWS 引擎句柄
 * @return active am id
 *
 */
LIB_KWS_API_EXPORT int32_t AisLiteUalOFAGetActiveAmId(void* handle);

/**
 * @brief 获取当前 active grammar 的 domain
 * @param handle KWS 引擎句柄
 * @return active grammar domain
 *
 */
LIB_KWS_API_EXPORT const char* AisLiteUalOFAGetActiveGrammarDomain(void* handle);

/**
 * @brief 打印所有资源文件(包括AM和Grammar)信息
 * @param handle KWS 引擎句柄
 *
 * @return void
 *
 */
LIB_KWS_API_EXPORT void AisLiteUalOFAPrintResourceInfo(void* handle);

/**
 * @brief 获取所有AM Info字符串
 * @param handle KWS 引擎句柄
 * @param buffer 外部传入的 AM Info 存储地址(每个AM大约需要32 byte)
 *               格式：Language0 AMID0;Language1 AMID1;...
 *
 * @return 字符串长度
 *
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAGetAmInfo(void* handle, char* buffer);

/**
 * @brief 获取所有Grammar Info字符串
 * @param handle KWS 引擎句柄
 * @param buffer 外部传入的 Grammar Info 存储地址(每个Grammar大约需要32 byte)
 *               格式：Language0 Domain0;Language1 Domain1;...
 *
 * @return 字符串长度
 *
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAGetGrammarInfo(void* handle, char* buffer);

/**
 * @brief 加载新语法
 * @param handle KWS 引擎句柄
 * @param grammar_buffer
 * 语法在内存中的起始地址,KWS运行期间不允许改变(包括释放)
 *
 * @return 语法状态码 #GrammarStatusCode
 *
 * @note 若新语法domain与初始语法domain相同，会覆盖初始语法
 */
LIB_KWS_API_EXPORT int AisLiteUalOFALoadGrammar(void* handle,
                                               const char* grammar_buffer);

/**
 * @brief 卸载语法
 * @param handle KWS 引擎句柄
 * @param grammar_domain 语法domain
 *
 * @return 语法状态码 #GrammarStatusCode
 *
 * @note 只卸载当前语种的语法，若要卸载其他语种的语法，需要先切换至目标语种
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAUnloadGrammar(void* handle,
                                                 const char* grammar_domain);
/**
 * @brief 获取识别结果对应的临时语法
 * @param handle KWS 引擎句柄
 * @param partial_grammar 保存临时语法的地址
 * @param max_grammar_size 存储临时语法的内存空间大小(当前版本需要356字节)
 *
 * @return 临时语法大小 (-1 表示错误 )
 *
 * @note 临时语法不可直接用于AisLiteUalOFAStart接口
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAGetResGrammar(void* handle, void* partial_grammar,
                                   int max_grammar_size);

/**
 * @brief 合并初始语法和临时语法
 * @param handle KWS 引擎句柄
 * @param base_grammar 初始语法，由domain、language确定具体要合并的语法
 * @param domain 初始语法domain
 * @param language 初始语法language
 * @param partial_grammar 临时语法
 * @param merged_grammar 合并后语法存储地址
 * @param max_merged_grammar_size 存储合并后语法的内存空间大小
 *        需要bytes: 2*(初始语法大小+临时语法大小)
 *
 * @return 合并后语法大小 (-1 表示错误 )
 *
 * @note 将多个不同partial_grammar与base_grammar合并，可通过多次调用本接口实现
 *
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAMergeGrammar(void* handle, void* base_grammar,
                                  const char* domain, const char* language,
                                  void* partial_grammar, void* merged_grammar,
                                  int max_merged_grammar_size);
/**
 * @brief 比较临时语法相似度
 * @param handle KWS 引擎句柄
 * @param partial_grammar1 临时语法
 * @param partial_grammar2 临时语法
 *
 * @return 临时语法相似度
 *
 */
LIB_KWS_API_EXPORT int AisLiteUalOFAGetGrammarSimilarity(void* handle, void* partial_grammar1,
                                          void* partial_grammar2);

/**
 * @brief 根据拼音/音标序列编译临时语法
 * @param handle KWS 引擎句柄
 * @param compile_resource_buffer 语法编译资源地址(需外部加载至内存或Flash)
 * @param input 拼音/音标序列，需要保证为映射表中的有效拼写
 *    普通话/粤语：每个字一个拼音，空格隔开。例如：
 *        你好魔方：ni3 hao3 mo2 fang1
 *    英语：每个单词一个序列，空格隔开，每个单词中的音标用","隔开。例如：
 *        hey siri: hh,ey s,ih,r,ih
 * @param partial_grammar 保存临时语法的地址
 * @param max_grammar_size 存储临时语法的内存空间大小(当前版本需要356字节)
 *
 * @return 临时语法相大小
 *
 * @note
 *    1. 临时语法需要与初始语法合并(AisLiteUalOFAMergeGrammar)之后才能使用
 *    2. 临时语法的识别结果与编译时的input完全一致，可以按需进行进一步的映射
 */
LIB_KWS_API_EXPORT int AisLiteUalOFACompileGrammar(void* handle,
                                    const char* compile_resource_buffer,
                                    const char* input,
                                    uint32_t max_grammar_size,
                                    void* partial_grammar);

#ifdef __cplusplus
}
#endif

#endif  // AIS_LIST_UAL_OFA_H_









