set(incs)
set(srcs)

if (CONFIG_LWIP_V2_1)
list(APPEND incs
	lwip-2.1.2/src
	lwip-2.1.2/port
	lwip-2.1.2/src/include
	lwip-2.1.2/src/include/netif
	lwip-2.1.2/src/include/lwip
	lwip-2.1.2/src/include/lwip/priv
	)

	list(APPEND srcs
	lwip-2.1.2/port/hwcsum.c
	lwip-2.1.2/port/wlanif.c
	lwip-2.1.2/port/net.c
	lwip-2.1.2/port/sys_arch.c
	lwip-2.1.2/src/api/api_lib.c
	lwip-2.1.2/src/api/api_msg.c
	lwip-2.1.2/src/api/err.c
	lwip-2.1.2/src/api/netbuf.c
	lwip-2.1.2/src/api/netdb.c
	lwip-2.1.2/src/api/netifapi.c
	lwip-2.1.2/src/api/sockets.c
	lwip-2.1.2/src/api/tcpip.c
	lwip-2.1.2/src/apps/mdns/mdns.c
	lwip-2.1.2/src/apps/ifaddrs/lwip_ifaddrs.c
	lwip-2.1.2/src/apps/ping/ping.c
	lwip-2.1.2/src/api/if_api.c
	lwip-2.1.2/src/apps/tftp/tftp_server.c  # Modified by TUYA
	lwip-2.1.2/src/core/def.c
	lwip-2.1.2/src/core/dns.c
	lwip-2.1.2/src/core/inet_chksum.c
	lwip-2.1.2/src/core/init.c
	lwip-2.1.2/src/core/ip.c
	lwip-2.1.2/src/core/ipv4/autoip.c
	lwip-2.1.2/src/core/ipv4/dhcp.c
	lwip-2.1.2/src/core/ipv4/etharp.c
	lwip-2.1.2/src/core/ipv4/icmp.c
	lwip-2.1.2/src/core/ipv4/igmp.c
	lwip-2.1.2/src/core/ipv4/ip4_addr.c
	lwip-2.1.2/src/core/ipv4/ip4.c
	lwip-2.1.2/src/core/ipv4/ip4_frag.c
	lwip-2.1.2/src/core/ipv4/ip4_napt.c
	lwip-2.1.2/src/core/ipv6/dhcp6.c
	lwip-2.1.2/src/core/ipv6/ethip6.c
	lwip-2.1.2/src/core/ipv6/icmp6.c
	lwip-2.1.2/src/core/ipv6/inet6.c
	lwip-2.1.2/src/core/ipv6/ip6_addr.c
	lwip-2.1.2/src/core/ipv6/ip6.c
	lwip-2.1.2/src/core/ipv6/ip6_frag.c
	lwip-2.1.2/src/core/ipv6/mld6.c
	lwip-2.1.2/src/core/ipv6/nd6.c
	lwip-2.1.2/src/core/mem.c
	lwip-2.1.2/src/core/memp.c
	lwip-2.1.2/src/core/netif.c
	lwip-2.1.2/src/core/pbuf.c
	lwip-2.1.2/src/core/raw.c
	lwip-2.1.2/src/core/stats.c
	lwip-2.1.2/src/core/sys.c
	lwip-2.1.2/src/core/tcp.c
	lwip-2.1.2/src/core/tcp_in.c
	lwip-2.1.2/src/core/tcp_out.c
	lwip-2.1.2/src/core/timeouts.c
	lwip-2.1.2/src/core/udp.c
	lwip-2.1.2/src/core/debug.c
	lwip-2.1.2/src/netif/ethernet.c
	dhcpd/dhcp-server.c
	dhcpd/dhcp-server-main.c
	)
# Ethernet
if (CONFIG_ETH)
	list(APPEND srcs
	lwip-2.1.2/port/ethernetif.c
	)
endif()
if (CONFIG_BRIDGE)
	list(APPEND srcs
	lwip-2.1.2/src/netif/bridgeif.c
	lwip-2.1.2/src/netif/bridgeif_fdb.c
	)
endif()
if (CONFIG_PAN)
	list(APPEND srcs
	lwip-2.1.2/port/panif.c
	)
endif()
if (CONFIG_BK_MODEM)
	list(APPEND srcs
	lwip-2.1.2/port/modemif.c
	)
endif()
# lwip apps
if (CONFIG_LWIP_HTTPD)
	list(APPEND srcs
	lwip-2.1.2/src/apps/http/fs.c
	lwip-2.1.2/src/apps/http/fsdata.c
	lwip-2.1.2/src/apps/http/httpd.c
	)
endif()


if (CONFIG_RIO)
       list(APPEND incs
               lwip-2.1.2/port/ip6_route_hook
               )
       list(APPEND srcs
               lwip-2.1.2/port/ip6_route_hook/ip6_route_hook.c
               lwip-2.1.2/port/ip6_route_hook/ip6_route_table.c
               )
endif()

if(CONFIG_LWIP_PPP_SUPPORT)
        list(APPEND srcs
        lwip-2.1.2/src/netif/ppp/auth.c
        lwip-2.1.2/src/netif/ppp/ccp.c
        lwip-2.1.2/src/netif/ppp/chap-md5.c
        lwip-2.1.2/src/netif/ppp/chap-new.c
        lwip-2.1.2/src/netif/ppp/chap_ms.c
        lwip-2.1.2/src/netif/ppp/demand.c
        lwip-2.1.2/src/netif/ppp/eap.c
        lwip-2.1.2/src/netif/ppp/ecp.c
        lwip-2.1.2/src/netif/ppp/eui64.c
        lwip-2.1.2/src/netif/ppp/fsm.c
        lwip-2.1.2/src/netif/ppp/ipcp.c
        lwip-2.1.2/src/netif/ppp/ipv6cp.c
        lwip-2.1.2/src/netif/ppp/lcp.c
        lwip-2.1.2/src/netif/ppp/magic.c
        lwip-2.1.2/src/netif/ppp/mppe.c
        lwip-2.1.2/src/netif/ppp/multilink.c
        lwip-2.1.2/src/netif/ppp/ppp.c
        lwip-2.1.2/src/netif/ppp/pppapi.c
        lwip-2.1.2/src/netif/ppp/pppcrypt.c
        lwip-2.1.2/src/netif/ppp/pppoe.c
        lwip-2.1.2/src/netif/ppp/pppol2tp.c
        lwip-2.1.2/src/netif/ppp/pppos.c
        lwip-2.1.2/src/netif/ppp/upap.c
        lwip-2.1.2/src/netif/ppp/utils.c
        lwip-2.1.2/src/netif/ppp/vj.c
        lwip-2.1.2/src/netif/ppp/polarssl/arc4.c
        lwip-2.1.2/src/netif/ppp/polarssl/des.c
        lwip-2.1.2/src/netif/ppp/polarssl/md4.c
        lwip-2.1.2/src/netif/ppp/polarssl/md5.c
        lwip-2.1.2/src/netif/ppp/polarssl/sha1.c)
endif()

if(CONFIG_LWIP_HOOK_IP4_ROUTE_SRC)
	list(APPEND srcs
	lwip-2.1.2/port/lwip_hooks.c
	)
endif()

else()


list(APPEND incs
	lwip-2.1.2/src
	lwip-2.1.2/port
	lwip-2.1.2/src/include
	lwip-2.1.2/src/include/netif
	lwip-2.1.2/src/include/lwip
	)

list(APPEND srcs
	lwip-2.1.2/src/core/disable_wifi.c
	)
endif()


armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES driver
	PRIV_REQUIRES bk_common bk_wifi bk_rtos os_source bk_wifi_driver
	)
