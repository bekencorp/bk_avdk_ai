#include "lingxin_auth_config.h"

static AuthAppIdGetFunc inner_lingxin_auth_app_id_get = NULL;
static AuthAppKeyGetFunc inner_lingxin_auth_app_key_get = NULL;
static AuthSnGetFunc inner_lingxin_auth_sn_get = NULL;
static AuthAgentCodeGetFunc inner_lingxin_auth_agent_code_get = NULL;

void inner_register_auth_get_func(
    AuthAppIdGetFunc auth_app_id_get_func, 
    AuthAppKeyGetFunc auth_app_key_get_func, 
    AuthSnGetFunc auth_sn_get_func, 
    AuthAgentCodeGetFunc auth_agent_code_get_func
) {
    inner_lingxin_auth_app_id_get = auth_app_id_get_func;
    inner_lingxin_auth_app_key_get = auth_app_key_get_func;
    inner_lingxin_auth_sn_get = auth_sn_get_func;
    inner_lingxin_auth_agent_code_get = auth_agent_code_get_func;
}

AuthAppIdGetFunc lingxin_auth_appId_get() {
    if (inner_lingxin_auth_app_id_get) {
        return inner_lingxin_auth_app_id_get;
    } else {
        return NULL;
    }
    
}
AuthAppKeyGetFunc lingxin_auth_appKey_get() {
    if (inner_lingxin_auth_app_key_get) {
        return inner_lingxin_auth_app_key_get;
    } else {
        return NULL;
    }
}
AuthSnGetFunc lingxin_auth_sn_get() {
    if (inner_lingxin_auth_sn_get) {
        return inner_lingxin_auth_sn_get;
    } else {
        return NULL;
    }
}
AuthAgentCodeGetFunc lingxin_auth_agentCode_get() {
    if (inner_lingxin_auth_agent_code_get) {
        return inner_lingxin_auth_agent_code_get;
    } else {
        return NULL;
    }
}