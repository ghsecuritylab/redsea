#include <lwip/sockets.h>
#include <lwip/netif.h>
extern struct netif xnetif[];

/*-----------------------------------------------------------------------
 * Mandatory functions
 *-----------------------------------------------------------------------*/

// Mandatory function for custom initialization
// called when mDNS initialization
void mDNSPlatformCustomInit(void)
{
	xnetif[0].flags |= NETIF_FLAG_IGMP;
}

uint16_t mDNSPlatformHtons(uint16_t hostshort)
{
	return htons(hostshort);
}

uint32_t mDNSPlatformInetAddr(char *cp)
{
	return inet_addr(cp);
}

extern char conf_sys_dev_id[];

// Mandatory function to get hostname
// called when mDNS initialization
char *mDNSPlatformHostname(void)
{
	//add by herry
#if 0
#if LWIP_NETIF_HOSTNAME
	return xnetif[0].hostname;
#else
	return "ameba";
#endif
#endif
    return conf_sys_dev_id;
}

/*-----------------------------------------------------------------------*/
