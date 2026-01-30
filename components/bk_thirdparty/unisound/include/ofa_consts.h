/**
 * @file ofa_consts.h
 * @brief KWS Engine Constants
 * @details
 * @authors
 * @copyright 2014-2020 Unisound AI Technology Co., Ltd. All rights reserved.
 * @note including:
 * @note    return value definition
 * @note    option id definition
 */

#ifndef KWSLP_BASE_OFA_CONSTS_H_
#define KWSLP_BASE_OFA_CONSTS_H_

/**
 * @brief 识别引擎状态码
 */
typedef enum {
  /** 未实现接口 **/
  ASR_RECOGNIZER_UNIMPLEMENTED_FUCNTION = -10,
  /**错误调用*/
  ASR_RECOGNIZER_WRONG_OPS = -9,
  /**引擎包名不合法**/
  ASR_ENGINE_INVALID_PACKAGE = -4,
  /**Stop接口等待*/
  ASR_STOP_PENDING = -3,
  /**识别错状态*/
  ASR_WRONG_STATUS = -2,
  /**错误*/
  ASR_FATAL_ERROR = -1,
  /**正常工作*/
  ASR_RECOGNIZER_OK = 0,
  /**没有识别结果*/
  ASR_RECOGNIZER_NO_RESULT = 1,
  /**有识别结果*/
  ASR_RECOGNIZER_PARTIAL_RESULT = 2,
  /**唤醒快速响应结果*/
  ASR_RECOGNIZER_QUICK_RESULT = 10,
} EngineStatusCode;

/**
 * @brief 引擎属性ID
 */
enum EngineOptionId {
  /**（Oneshot模式）设置VAD检查最大延迟，单位：毫秒*/
  ASR_VAD_WAV_CHECK_MAX_LATENCY = 22,

  /**当前识别结果的起始时间*/
  ASR_ENGINE_UTTERANCE_START_TIME = 101,
  /**当前识别结果的结束时间*/
  ASR_ENGINE_UTTERANCE_STOP_TIME = 102,

  /**设置计算NN时的bunchsize*/
  KWS_SET_BUNCH_FRAME_NUMBER = 127,
  /**设置统计NN内存时，前向模拟循环次数*/
  KWS_SET_NN_LOOP_COUNT = 128,
  /**设置引擎 silence 奖励的值，程序中真正使用的是这里设置 值 * 0.1 */
  KWS_SET_SIL_PEN = 132,
  /*设置解码Beam(默认10)：数值越小，解码越快，但是识别率会降低*/
  KWS_BEAM_ID = 134,
  /**设置前缀命令词结束最长等待时间,单位为毫秒,默认值为300，表示等待300ms*/
  KWS_SET_PREFIX_COMMAND_END_MAX_WAIT_TIME_ID = 135,
  /**设置输出命令词的最低能量，默认值为0**/
  KWS_SET_OUTPUT_WORD_ENERGY_TH_ID = 136,
  /**设置获取命令词能量**/
  KWS_GET_OUTPUT_WORD_ENERGY_ID = 137,
  /**设置加密验证信息，启用加密模式时，必须设置此选项*/
  KWS_SET_ENCRYPTION_ID = 138,
  /**设置栈空间优化大小,默认2048，若不满足需求，请增大*/
  KWS_SET_HEAP2STACK_SIZE_ID = 139,
  /**设置grammar的tag*/
  KWS_SET_GRAMMAR_TAG_ID = 140,
  /**设置输出结果为json格式*/
  KWS_SET_OUTPUT_JSON_RESULT_ID = 141,
  /**设置语义信息*/
  KWS_SET_NLP_INFO_ID = 142,
  /**设置抑制句子中命令词识别*/
  KWS_SET_DEPRESS_CMD_IN_SENTENCE_ID = 143,
  /**设置当前的语种*/
  KWS_CURRENT_LANGUAGE_ID = 800,
  /**设置Log文件路径*/
  KWS_LOG_PATH_ID = 900,
};

/**
 * @brief 包含的引擎类型
 */
typedef enum {
  /**只有LP引擎*/
  KWS_LP_ENGINE = 0,
} EngineTypeId;

/**
 * @brief Oneshot模式用于获取VAD信息的状态码
 * MAX_LATENCY可通过UalOFASetOptionInt设置,
 * id=ASR_VAD_WAV_CHECK_MAX_LATENCY,
 * 设置范围为100-1000,单位ms，表示唤醒后100ms到1s,
 * 默认设置300,表示300ms
 */
enum EngineOneshotStatusCode {
  /**UalOFACheckWavEnd 返回值状态**/
  /**初始状态，即第一次唤醒有结果之前状态*/
  ASR_VAD_INIT = 1,
  /**唤醒之后,MAX_LATENCY后仍没有检测到语音
   （到下次唤醒结果之前都一直返回该数值）*/
  ASR_VAD_TIMEOUT = 2,
  /**唤醒之后,MAX_LATENCY内尚未检测到语音*/
  ASR_VAD_DETECT_SILENCE = 3,
  /**唤醒之后，MAX_LATENCY内检测到语音
   （到下次唤醒结果之前都一直返回该数值）*/
  ASR_VAD_DETECT_SPEECH = 4,
};

/**
 * @brief 前端相关设置
 */
enum EngineFrontendId {
  /**引擎前端采样率设置（KHz）
   * @note 仅支持8和16(默认)
   */
  ASR_SAMPLE_RATE_KHZ = 600,

  /**前端特征类型*/
  ASR_FRONTEND_FEATURE_TYPE = 602,
};

/**
 * @brief VAD功能相关选项
 */
enum EngineVadId {
  /**VAD功能开关*/
  ASR_ENGINE_ENABLE_VAD_ID = 702,
};

/**
 * @brief acoustic status
 */
enum {
  ASR_ACOUSTIC_OK = 0,
  ASR_ACOUSTIC_ERROR = -1,
};

/**
 * @brief acoustic async status
 */
enum {
  ACOUSTIC_ASYNC_OK = 0x1,
  ACOUSTIC_ASYNC_NN_NEED_DATA = 0x2,
  ACOUSTIC_ASYNC_NN_BUSY = 0x4,
  ACOUSTIC_ASYNC_NN_RESIDUAL = 0x8,
};

/**
 * @brief engine error code
 * [-50, -99]
 */
enum {
  ASR_ACOUSTIC_VERSION_NULL = -50,
  ASR_ACOUSTIC_VERSION_WRONG = -51,
  ASR_ACOUSTIC_VERSION_UNSUPPORT = -52,
  ASR_ACOUSTIC_GRAMMAR_LANGUAGE_UNMATCH = -53,
  ASR_ACOUSTIC_GRAMMAR_TREE_UNMATCH = -54,
  ASR_ACOUSTIC_GRAMMAR_LANGUAGE_NULL = -55,
  ASR_ACOUSTIC_GRAMMAR_TREE_NULL = -56,
  KWS_NULL = -57,
  ASR_LANGUAGE_UNSUPPORT = -58,
};

/**
 * @brief acoustic error code
 * [-100, -200)
 * */
enum {
  ASR_ACOUSTIC_INIT_ERROR = -101,
  ASR_ACOUSTIC_FORWARD_ERROR = -102,
  ASR_ACOUSTIC_RESET_ERROR = -103,
  ASR_ACOUSTIC_BUFFER_NULL = -104,
  ASR_ACOUSTIC_HANDLE_NULL = -105,
  ASR_ACOUSTIC_NET_WRONG = -106,
  ASR_ACOUSTIC_ID_WRONG = -107,
  /**NN 内存不足*/
  ASR_ACOUSTIC_NN_OOM = -108,
};

/**
 * @brief grammar error code
 * [-200, -300)
 * */
typedef enum {
  ASR_GRAMMAR_INIT_ERROR = -200,
  ASR_GRAMMAR_RESET_ERROR = -201,
  ASR_GRAMMAR_BUFFER_NULL = -203,
  ASR_GRAMMAR_HANDLE_NULL = -204,
  ASR_GRAMMAR_NET_WRONG = -205,
  ASR_GRAMMAR_CHECK_WRONG = -206,
  ASR_GRAMMAR_UNLOAD_ERROR = -207,
  ASR_GRAMMAR_LOAD_ERROR = -208,
  ASR_GRAMMAR_IS_OK = 0,
} GrammarStatusCode;

/**
 * @brief decoder error code
 * [-300, -400)
 * */
enum {
  ASR_DECODER_INIT_ERROR = -300,
  ASR_DECODER_RESET_ERROR = -301,
};

/**
 * @brief UalOFARecognizeAsync回调函数结构体
 */
typedef struct {
  /** 识别状态，与UalOFARecognize返回值一致 */
  int recog_status;
  /** 是否需要新数据 */
  int need_input;
} KWSRecognizeCallbackData;

/**
 * @brief 特征类型
 */
typedef enum {
  /** Unisound 特征 (默认) */
  FEATURE_TYPE_GENERAL = 0,
  /** fengniaoL 特征 */
  FEATURE_TYPE_FENGNIAOL = 1,
} FeatureType;

/**
 * @brief JSON结果错误码
 */
typedef enum {
  /**  JSON结果正常 */
  JSON_RESULT_NORMAL = 0,
  /** JSON结果缺失意图 */
  JSON_RESULT_NO_INTENT = -1,
  /** JSON结果缺失语义映射 */
  JSON_RESULT_NO_NLPINFO = -2,
} JsonResultType;

#endif  // KWSLP_BASE_OFA_CONSTS_H_
