#include "volc_network.h"

#include "sdkconfig.h"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include <esp_log.h>
#include <esp_netif.h>

#include "volc_type.h"

#ifndef EAI_SYSTEM
#define EAI_SYSTEM -11
#endif

uint32_t _volc_ip_address_from_string(volc_ip_addr_t* p_ip_address, char* p_ip_address_string){
    int port = 0;
    sscanf(p_ip_address_string,"%d.%d.%d.%d:%d",&p_ip_address->address[0],&p_ip_address->address[1],&p_ip_address->address[2],&p_ip_address->address[3],&port);
    p_ip_address->family = VOLC_IP_FAMILY_TYPE_IPV4;
    p_ip_address->port = port; 
    return VOLC_STATUS_SUCCESS;
};

uint32_t volc_get_local_ip(volc_ip_addr_t* dest_ip_list, uint32_t* p_dest_ip_list_len, volc_network_callback_t callback, uint64_t custom_data) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    uint32_t ip_count = 0;
    // iterate over active interfaces, and print out IPs of "our" netifs
    esp_netif_t *netif = NULL;
    while ((netif = esp_netif_next_unsafe(netif)) != NULL) {
        {
#if CONFIG_LWIP_IPV4
            esp_netif_ip_info_t ip;
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));
            char ipv4[64] = {0};
            snprintf(ipv4,64,"%d.%d.%d.%d:0",esp_ip4_addr1_16(&ip.ip),esp_ip4_addr2_16(&ip.ip),esp_ip4_addr3_16(&ip.ip),esp_ip4_addr4_16(&ip.ip));
            if(_volc_ip_address_from_string(&dest_ip_list[ip_count],ipv4) == VOLC_STATUS_SUCCESS) {
                ip_count++;
            }
#endif
        }
    }
    *p_dest_ip_list_len = ip_count;
    return ret;
}

// getIpWithHostName
uint32_t volc_get_ip_with_host_name(const char* hostname, volc_ip_addr_t* dest_ip) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    int32_t err_code;
    char* err_str;
    struct addrinfo *res, *rp;
    bool resolved = false;
    struct sockaddr_in* ipv4Addr;
    struct sockaddr_in6* ipv6Addr;
    VOLC_CHK(hostname != NULL, VOLC_STATUS_NULL_ARG);
    err_code = getaddrinfo(hostname, NULL, NULL, &res);
    if (err_code != 0) {
        err_str = err_code == EAI_SYSTEM ? strerror(errno) : (char *) gai_strerror(err_code);
        ret = VOLC_STATUS_RESOLVE_HOSTNAME_FAILED;
        goto err_out_label;
    }

    for (rp = res; rp != NULL && !resolved; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            ipv4Addr = (struct sockaddr_in*) rp->ai_addr;
            dest_ip->family = VOLC_IP_FAMILY_TYPE_IPV4;
            memcpy(dest_ip->address, &ipv4Addr->sin_addr, VOLC_IPV4_ADDRESS_LENGTH);
            resolved = true;
        } else if (rp->ai_family == AF_INET6) {
            ipv6Addr = (struct sockaddr_in6*) rp->ai_addr;
            dest_ip->family = VOLC_IP_FAMILY_TYPE_IPV6;
            memcpy(dest_ip->address, &ipv6Addr->sin6_addr, VOLC_IPV6_ADDRESS_LENGTH);
            resolved = true;
        }
    }

    freeaddrinfo(res);
    if (!resolved) {
        ret = VOLC_STATUS_HOSTNAME_NOT_FOUND;
    }
err_out_label:
    return ret;
}
int volc_inet_pton (int __af, const char *__restrict __cp,void *__restrict __buf) {
    int af = AF_INET;
    if(__af == VOLC_IP_FAMILY_TYPE_IPV6 ) {
        af = AF_INET6;
    }
    return inet_pton(af,__cp,__buf);
};

 uint16_t volc_htons(uint16_t hostshort){
    return htons(hostshort);
 };
uint16_t volc_ntohs(uint16_t netshort){
    return ntohs(netshort);
}
