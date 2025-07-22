#ifndef AI_IOT_AUTH_CONFIG_H
#define AI_IOT_AUTH_CONFIG_H

struct lingxin_auth_config
{
    char *appId;
    char *appKey;
    /*customer can change sn length by themself*/
    char *sn;
    char *agentCode;
};


#endif