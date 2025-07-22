#include "auth_config.h"

#if !CONFIG_ENABLE_LINGXIN_COSTOM_AUTH
struct lingxin_auth_config auth_config = {0};

char *lingxin_auth_appId_get()
{
    return auth_config.appId;
}
char *lingxin_auth_appKey_get()
{
    return auth_config.appKey;
}
char *lingxin_auth_sn_get()
{
    return auth_config.sn;
}
char *lingxin_auth_agentCode_get()
{
    return auth_config.agentCode;
}
#else
/*customers need to config appid, appkey, sn, agentcode follow Lingxin Docs*/
char *lingxin_auth_appId_get()
{
    return "xxx";
}
char *lingxin_auth_appKey_get()
{
    return "xxx";
}
char *lingxin_auth_sn_get()
{
    return "xxx";
}
char *lingxin_auth_agentCode_get()
{
    return "xxx";
}
#endif