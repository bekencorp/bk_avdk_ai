/*
 * Author: Zhu Jian bo
 */

#ifndef AIS_LITE_ENCRYPT_DATA_STRUCT_H_
#define AIS_LITE_ENCRYPT_DATA_STRUCT_H_


#ifdef __cplusplus
extern "C" {
#endif

/* 
云端返回错误列表:
errorCode    errorMsg                     描述
9000000      请求成功                     请求成功
9000001      必填参数不能为空              请求必填参数为空
9000002      请求参数不合法                请求业务参数不合法
9000003      请求时间戳超出了请求有效期     时间戳过期
9000004      应用不存在                   非paas平台的应用
9000005      签名错误                     签名错误
9000006      超过了激活频率限制            超过了激活频率
9000007      装机量超限                   没有装机量
9000008      激活失败                     授权码生成异常
9009999      未知错误                     未知错误
*/
typedef enum AisLiteStatus
{
  OK = 0, // 正常
  ERR_UNKNOWN, // 未知错误

  // 库内部错误
  ERR_LOCAL_MALLOC,  // 动态内存申请失败
  ERR_LOCAL_CHECK_TYPE,  // 校验方式错误
  ERR_LOCAL_INPUT_PARA_NULL,  // 函数入参为NULL
  ERR_LOCAL_ENGINE_ID,  // 引擎id错误
  ERR_LOCAL_DEVICE_INFO,  // 用户传入数据错误
  ERR_LOCAL_TIMESTAMP,  // 获取时间戳失败,使用time(NULL)获取
  ERR_LOCAL_DEVICE_ACCESS,  // 获取deviceAccess参数失败
  ERR_LOCAL_UPDATE_STR_INFO,  // 更新AisLiteStrInfo数据结构错误(动态申请内存错误)
  ERR_LOCAL_GET_SIGNATURE,  // 计算HTTP请求签名失败
  ERR_LOCAL_CB_FUNC_NULL,  // 需要设置的回调函数为NULL
  ERR_LOCAL_READ_LOCAL_AUTHCODE,  // 读取本地授权码失败
  ERR_LOCAL_WRITE_LOCAL_AUTHCODE,  // 写入本地授权码失败
  ERR_LOCAL_CREATE_SOCKET,  // socket()执行失败
  ERR_LOCAL_GET_IP_ADDR,  // 通过域名获取IP地址失败
  ERR_LOCAL_CREATE_STR_INFO,  // 创建AisLiteStrInfo数据结构错误(动态申请内存错误)
  ERR_LOCAL_LINK_TO_SERVER,  // connect()连接服务器失败
  ERR_LOCAL_SEND_HTTP_MASSAGE,  // 发送http报文失败
  ERR_LOCAL_SOCKET_TIMEOUT,  // 等待接收云端http请求回复超时
  ERR_LOCAL_SOCKET_FUNC,  // select()执行异常
  ERR_LOCAL_RECV_HTTP_MASSAGE,  // 接受云端http返回报文失败
  ERR_LOCAL_RECV_HTTP_MASSAGE_FAIL,  // 云端http返回的报文内容错误
  ERR_LOCAL_RECV_HTTP_MASSAGE_JSON,  // 云端http返回的报文json内容错误

  ERR_LOCAL_GET_USAGE_INFO,  // 获取usageData参数失败
  ERR_LOCAL_CREATE_THREAD,  // 创建线程失败
  ERR_LOCAL_DESTROY_THREAD,  // 销毁线程失败
  ERR_LOCAL_JOIN_THREAD,  // 等待线程退出失败

  ERR_LOCAL_AIS_LITE_HANDLE_NULL,  // ais-lite自身句柄为NULL
  ERR_LOCAL_RELEASE_SELF,  // 释放ais-lite自身资源失败
  ERR_LOCAL_NOT_SUPPORT_ENGINE,  // 引擎初始化接口檢查是否支持當前引擎能力失敗
  ERR_LOCAL_ENCRYPTION_COMPARE_FAIL,  // 本地授權失敗
  ERR_LOCAL_ENGINE_INIT_FAIL,  // 裸引擎初始化失败
  ERR_LOCAL_ENGINE_INIT_SOURCE_IS_NULL,  // 裸引擎初始化资源为NULL



  // 时间限制相关错误
  ERR_LOCAL_DATE_EXPIRE,  // 时间过期
  ERR_LOCAL_ENCRYPT_AUTHMSG,  // 时间戳校验失败

  // 云端返回错误,详见上面注释

  AIS_LITE_STATUS_MAX
}AisLiteStatus;



// 用户传入的设备信息
typedef enum AisLiteDeviceInfo {
  // 云端授权必配项,非云端授权输入NULL即可
  DEVICE_INFO_APP_KEY = 0, // 云端app key
  DEVICE_INFO_APP_SECRET, // 云端app 密钥
  DEVICE_INFO_UNIQUE_ID, // 设备网卡名称或者唯一ID（根据项目指定）
  // 云端授权选填项,非云端授权输入NULL即可
  DEVICE_INFO_IMEI, // 设备的IMEI
  DEVICE_INFO_MAC, // 设备MAC地址
  DEVICE_INFO_REMARK, // 备注信息

  DEVICE_INFO_MAX
} AisLiteDeviceInfo;

// 注:init引擎前,创建了struct DeviceInfo结构数据之后,除非release引擎,否则不要释放和修改该数据
typedef struct DeviceInfo {
  
  char* deviceInfo[DEVICE_INFO_MAX];

  // ais_lite库状态信息,在调用init接口之后,可根据需要,获取该变量的值
  // 错误类型见:enum AisLiteStatus 和 云端返回错误列表
  int ais_lite_status;


  /*
  * @Description: 读取本地授权码(若本地无授权码,则读取函数应输入全0数据)
  * @Input params: aiCodeType: 授权码对应的引擎类型
  *                authCode_buf：云端授权码保存地址
  *                buf_len：云端授权码保存缓冲区长度(单位:字节)
  * @Output params:
  *                无
  * @Return: 成功：0
  *          失败：其他值
  */
  int (*read_authCode_cb)(char* aiCodeType, char* authCode_buf, int buf_len);

  /*
  * @Description: 写入授权码(保存到本地,用于下次初始化时,传入引擎校验)
  * @Input params: aiCodeType: 授权码对应的引擎类型
  *                authCode_buf：云端授权码保存地址
  *                buf_len：云端授权码保存缓冲区长度(单位:字节)
  * @Output params:
  *                无
  * @Return: 成功：0
  *          失败：其他值
  */
  int (*write_authCode_cb)(char* aiCodeType, char* authCode_buf, int authCode_buf_len, char* authMsg_buf, int authMsg_buf_len);


  /*
  * @Description: 获取时间戳(用户时间戳获取实现接口,用于在获取时间戳时，使用用户方实现的接口)
  * @Input params:
  * @Output params:long：对应time_t定义,返回当前日历时间
  *                无
  */
  long (*get_time_cb)(void);


  /*
  * @Description: 获取云端加密报文(用户通过回调接口获得http明文，将其POST云端请求获取云端授权加密报文，并通过回调接口将报文传入aislite)
  * @Input params:  socket_request ailite内部组装后的http报文信息
  * @Output params:  ais_lite_needle_ret云端返回的json串，字符串大小为 3 * 1024
  * @Return: 成功：0
  *          失败：其他值
  *                无
  */
  int (*get_cloud_encryption_cb)(char* socket_request, char* ais_lite_needle_ret);


  // 初始化配置为NULL,如初始化成功,会被库内部赋值,注意不要手动修改
  void* ais_lite_handle;

} DeviceInfo;



#ifdef __cplusplus
}
#endif

#endif  // AIS_LITE_ENCRYPT_DATA_STRUCT_H_
