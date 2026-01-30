/**
 * @file ual_ofa.h
 * @brief KWS Engine C/C++ API Header
 * @details
 * @authors
 * @copyright 2014-2020 Unisound AI Technology Co., Ltd. All rights reserved.
 */

#ifndef KWSLP_BASE_UAL_OFA_H_
#define KWSLP_BASE_UAL_OFA_H_

#include <stdint.h>
#include "./ofa_consts.h"

/** KWS 引擎句柄类型 */
typedef void* HANDLE;

/**
 * @def KWS_EXPORT
 * Cross Platform Export Macro
 */
#if defined(_WIN32) || defined(_WIN64)
#define _WINDOWS
#endif

#ifndef _WINDOWS
#ifdef KWS_API_HIDDEN
#define KWS_EXPORT __attribute__((visibility("hidden")))
#else
#define KWS_EXPORT __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
#else   // _WINDOWS
#ifdef DLL_EXPORT
#ifdef __cplusplus
#define KWS_EXPORT extern "C" __declspec(dllexport)
#else
#define KWS_EXPORT __declspec(dllexport)
#endif  // __cplusplus
#else   // DLL_EXPORT
#ifdef __cplusplus
#define KWS_EXPORT extern "C" __declspec(dllimport)
#else
#define KWS_EXPORT __declspec(dllimport)
#endif
#endif  // DLL_EXPORT
#endif  // _WINDOWS
/**
 * @brief 创建并从文件初始化 KWS STD 引擎
 *
 * @param path_of_am 放置模型文件（asrfix.dat）的目录
 * @param path_of_grammar 语法文件的路径
 *                        如果有多个语法文件，用';'分开。
 *
 * @return  KWS 引擎句柄
 *
 *
 * @note 不同语法文件的 domain 必须唯一。
 *       假如有两个相同的 domain 的语法，后面加载的语法会把前面的语法覆盖掉
 *
 * @attention 废弃接口
 */
KWS_EXPORT HANDLE UalOFAInitialize(const char* path_of_am,
                                   const char* path_of_grammar);

/**
 * @brief 创建 KWS 引擎句柄
 *
 * @return KWS 引擎句柄
 *
 */
KWS_EXPORT HANDLE UalOFAInitializeHandle();

/**
 * @brief 内存初始化 KWS 引擎
 *
 * @param handle KWS 引擎句柄
 * @param am_buffer LP模型在内存中的起始地址,KWS运行期间不允许改变(包括释放)
 * @param grammar_buffer
 * LP语法在内存中的起始地址,KWS运行期间不允许改变(包括释放)
 *
 * @return #EngineStatusCode
 */
KWS_EXPORT EngineStatusCode UalOFALoadFromBuffer(HANDLE handle,
                                                 const char* am_buffer,
                                                 const char* grammar_buffer);

/**
 * @brief 创建并从内存初始化 KWS LP 引擎
 *        该接口等效于顺序调用 UalOFAInitializeHandle UalOFALoadFromBuffer
 *
 * @param am_buffer LP模型在内存中的起始地址,KWS运行期间不允许改变(包括释放)
 * @param grammar_buffer
 * LP语法在内存中的起始地址,KWS运行期间不允许改变(包括释放)
 *
 * @return KWS 引擎句柄
 *
 */
KWS_EXPORT HANDLE UalOFAInitializeFromBuffer(const char* am_buffer,
                                             const char* grammar_buffer);

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
KWS_EXPORT EngineStatusCode UalOFASetOptionInt(HANDLE handle, int id,
                                               int value);
/**
 * @brief 获取处于激活状态的引擎的属性值
 *
 * @param handle KWS 引擎句柄
 * @param id 属性id
 *
 * @return 属性值
 *    @retval -1 不支持该属性
 *
 * @note 更多细节请参考 wiki 或者 @ref ofa_consts.h
 */
KWS_EXPORT int UalOFAGetOptionInt(HANDLE handle, int id);

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
KWS_EXPORT EngineStatusCode UalOFASetOptionString(HANDLE handle, int id,
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
KWS_EXPORT const char* UalOFAGetOptionString(HANDLE handle, int id);

/**
 * @brief 使用当前引擎开始识别
 *
 * @param handle KWS 引擎句柄
 * @param grammar_domain 用到的识别语法的domain : 7.5.1以后支持多个domain,
 *                          中间可用空格或者分号隔开，如"wakeup;ivm"
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
KWS_EXPORT EngineStatusCode UalOFAStart(HANDLE handle,
                                        const char* grammar_domain, int am_id,
                                        HANDLE memory_buffer,
                                        int32_t memory_size);
/**
 * @brief 输入数据给当前处于激活状态的引擎
 *
 * @param handle KWS 引擎句柄
 * @param raw_audio 音频数据
 * @param len 音频数据的长度（单位：Bytes），不要超过640 Bytes
 *
 * @return #EngineStatusCode
 *    @retval #ASR_RECOGNIZER_PARTIAL_RESULT 调用 #UalOFAGetResult 获取识别结果
 *    @retval #ASR_RECOGNIZER_OK             执行正常，无识别结果
 *    @retval #ASR_FATAL_ERROR    执行出错
 */
KWS_EXPORT EngineStatusCode UalOFARecognize(HANDLE handle,
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
KWS_EXPORT int32_t UalOFARecognizeAsync(HANDLE handle, signed char* raw_audio,
                                        int len,
                                        KWSRecognizeCallbackData* data);
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
KWS_EXPORT int32_t UalOFARecognizeAsyncFeat(HANDLE handle, float* feat, int len,
                                            KWSRecognizeCallbackData* data);

/**
 * @brief 获取处于激活状态的引擎的识别结果
 *
 * @param handle KWS 引擎句柄
 * @return 识别结果（C语言字符串）
 *    @retval NULL 如果引擎没有识别结果
 */
KWS_EXPORT const char* UalOFAGetResult(HANDLE handle);

/**
 * @brief 判断语音是否结束
 *
 * @param handle KWS 引擎句柄
 * @return #EngineOneshotStatusCode
 *
 * @note 当唤醒之后，判断后面一段时间内是否还有语音
 *
 */
KWS_EXPORT int UalOFACheckWavEnd(HANDLE handle);

/**
 * @brief 停止当前引擎的识别
 * @param handle KWS 引擎句柄
 * @details 如果在此之前，引擎没有返回有效的结果，
 *         那么在调用此API后，要调用UalOFAGetResult去尝试拿下结果
 *
 * @return #EngineStatusCode
 *    @retval ASR_RECOGNIZER_OK 停止成功
 *    @retval ASR_STOP_PENDING
 *            等待状态，调用者需要反复调用直到返回ASR_RECOGNIZER_OK
 *
 * @note 仅fengniaoL平台可能返回ASR_STOP_PENDING
 */
KWS_EXPORT EngineStatusCode UalOFAStop(HANDLE handle);

/**
 * @brief 释放所有引擎（STD和LP），回收内存。

 * @param handle KWS 引擎句柄
 * @details 释放后，除非重新初始化，否则引擎将无法继续使用
 *
 * @return void
 */
KWS_EXPORT void UalOFARelease(HANDLE handle);

/**
 * @brief 重置当前引擎为初始状态
 *
 * @param handle KWS 引擎句柄
 * @return #EngineStatusCode
 * @attention Nothing is done in the implementation
 */
KWS_EXPORT EngineStatusCode UalOFAReset(HANDLE handle);

/**
 * @brief 获得版本信息(引擎库版本和构建版本)
 *
 * @param handle KWS 引擎句柄
 * @return 描述版本信息的字符串
 */
KWS_EXPORT const char* UalOFAGetVersion(HANDLE handle);

/**
 * @brief 获取引擎类型
 *
 * @param handle KWS 引擎句柄
 * @return #EngineTypeId
 *    @retval #KWS_LP_ENGINE 只有LP引擎
 */
KWS_EXPORT EngineTypeId UalOFAGetEngineType(HANDLE handle);

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
KWS_EXPORT EngineStatusCode UalOFASetEnv(HANDLE handle, HANDLE env);

/**
 * @brief 获取引擎加密方式
 *
 * @param handle KWS 引擎句柄
 * @return EncryptionValue
 *    @retval 0 没有任何加密
 *    @retval 第4个bit位为1 包名限制
 *    @retval 第5个bit位为1 ais 校验加密
 *
 */
KWS_EXPORT int UalOFAGetEncryptionScheme(HANDLE handle);

/**
 * @brief 获取当前 active am 的ID
 * @param handle KWS 引擎句柄
 * @return active am id
 *
 */
KWS_EXPORT int32_t UalOFAGetActiveAmId(HANDLE handle);

/**
 * @brief 获取当前 active grammar 的 domain
 * @param handle KWS 引擎句柄
 * @return active grammar domain;7.5.1以后支持多domain，存在返回
 *                        多个激活domain的情况，中间用分号隔开，
 *                        如"wakeup;ivm"
 *
 */
KWS_EXPORT const char* UalOFAGetActiveGrammarDomain(HANDLE handle);

/**
 * @brief 打印所有资源文件(包括AM和Grammar)信息
 * @param handle KWS 引擎句柄
 *
 * @return void
 *
 */
KWS_EXPORT void UalOFAPrintResourceInfo(HANDLE handle);

/**
 * @brief 获取所有AM Info字符串
 * @param handle KWS 引擎句柄
 * @param buffer 外部传入的 AM Info 存储地址(每个AM大约需要32 byte)
 *               格式：Language0 AMID0;Language1 AMID1;...
 *
 * @return 字符串长度
 *
 */
KWS_EXPORT int UalOFAGetAmInfo(HANDLE handle, char* buffer);

/**
 * @brief 获取所有Grammar Info字符串
 * @param handle KWS 引擎句柄
 * @param buffer 外部传入的 Grammar Info 存储地址(每个Grammar大约需要32 byte)
 *               格式：Language0 Domain0;Language1 Domain1;...
 *
 * @return 字符串长度
 *
 */
KWS_EXPORT int UalOFAGetGrammarInfo(HANDLE handle, char* buffer);

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
KWS_EXPORT GrammarStatusCode UalOFALoadGrammar(HANDLE handle,
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
KWS_EXPORT GrammarStatusCode UalOFAUnloadGrammar(HANDLE handle,
                                                 const char* grammar_domain);
/**
 * @brief 获取识别结果对应的临时语法
 * @param handle KWS 引擎句柄
 * @param partial_grammar 保存临时语法的地址
 * @param max_grammar_size
 * 存储临时语法的内存空间大小(当前版本当KWS_LARGE_GRAMMAR_TYPE=1时需要556字节，否则需要356字节)
 *
 * @return 临时语法大小 (-1 表示错误 )
 *
 * @note 临时语法不可直接用于UalOFAStart接口
 */
KWS_EXPORT int UalOFAGetResGrammar(HANDLE handle, void* partial_grammar,
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
KWS_EXPORT int UalOFAMergeGrammar(HANDLE handle, void* base_grammar,
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
KWS_EXPORT int UalOFAGetGrammarSimilarity(HANDLE handle, void* partial_grammar1,
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
 * @param max_grammar_size
 * 存储临时语法的内存空间大小(当前版本当KWS_LARGE_GRAMMAR_TYPE=1时需要556字节，否则需要356字节)
 *
 * @return 临时语法相大小
 *
 * @note
 *    1. 临时语法需要与初始语法合并(UalOFAMergeGrammar)之后才能使用
 *    2. 临时语法的识别结果与编译时的input完全一致，可以按需进行进一步的映射
 */
KWS_EXPORT int UalOFACompileGrammar(HANDLE handle,
                                    const char* compile_resource_buffer,
                                    const char* input,
                                    uint32_t max_grammar_size,
                                    void* partial_grammar);
#ifdef JIELI_CHIP_ENCRYPTION
/**
 * @brief 获取杰里平台kws信息
 *
 * @param infos:杰里平台kws信息
 *
 * @return void
 *
 * @note
 *    获取值存放在变量 infos 里面
 */
KWS_EXPORT void UalOFAGetJIELIKWSINFOS(char* infos);
#endif

#ifndef _WIN32
#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // _WIN32
#endif  // KWSLP_BASE_UAL_OFA_H_
