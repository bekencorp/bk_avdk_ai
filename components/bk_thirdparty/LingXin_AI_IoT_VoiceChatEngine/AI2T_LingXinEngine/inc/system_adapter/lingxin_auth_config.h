#ifndef LINGXIN_AUTH_CONFIG_H
#define LINGXIN_AUTH_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief appId是企业在灵芯的唯一标识，在灵芯工作台获取，https://eagent.edu-aliyun.com/console/device/#/tenant/developer
   * @return appId
   */
  char *lingxin_auth_appId_get();

  /**
   * @brief appKey是灵芯平台颁发的 License
   * @return appKey
   */
  char *lingxin_auth_appKey_get();

  /**
   * @brief sn是设备在灵芯平台上的唯一 id，一般用设备唯一编号
   * @return sn
   */
  char *lingxin_auth_sn_get();

  /**
   * @brief agentCode是灵芯工作台中创建的 Agent 的唯一标识
   * @return agentCode
   */
  char *lingxin_auth_agentCode_get();

#ifdef __cplusplus
}
#endif

#endif // LINGXIN_AUTH_CONFIG_H