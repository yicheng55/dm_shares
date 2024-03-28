/**
  **************************************************************************
  * @file     _dm9051_lw_logi.h
  * @version  v1.0
  * @date     2023-04-28
  * @brief    header file of dm9051 environment config program.
  **************************************************************************
  */
#ifndef __DM9051_LOGI_H
#define __DM9051_LOGI_H

#include <stdio.h>
#include <string.h>

#define RX_ANY	0
#define RX_MON	1
#define RX_DUMP_NUM	2
						
#define RX_MODLE_DECLARATION \
const pkt_monitor_t rx_modle[RX_DUMP_NUM] = { \
	2, 1, \
}
						
#define TX_MODLE_DECLARATION \
const pkt_monitor_t tx_modle = { \
	16, \
}; \
const pkt_monitor_t tx_all_modle = { \
	2, \
}

#define kkmin(a, b) (a < b) ? a : b

#define SEG_QUAN		16

#define UIP_PROTO_ICMP  			1
#define UIP_PROTO_IGMP_JJ  		2
#define UIP_PROTO_TCP   			6
#define UIP_PROTO_UDP   			17

#define	DEF_MONITOR_NRX				0 //0 //packet number for rx debug display	//[optional define can be eventually permenently removed~]
#define	DEF_MONITOR_NTX				1 //0 //packet number for tx debug display

#define DHCPC_SERVER_PORT			67
#define DHCPC_CLIENT_PORT			68

#define UIP_LLH_LEN				14
#define UIP_UDPH_LEN			8
#define UIP_IPH_LEN				20
#define UIP_IPUDPH_LEN			(UIP_UDPH_LEN + UIP_IPH_LEN)

#define	HTONS(n)				(uint16_t)((((uint16_t) (n)) << 8) | (((uint16_t) (n)) >> 8))

#define BUF					((struct arp_hdr *)buf) //&ptr[0]
#define IPBUF 				((struct ethip_hdr *)buf) //&ptr[0]
#define UDPBUF				((struct uip_udpip_hdr *)&(((uint8_t *)buf)[ UIP_LLH_LEN]))
#define TCPBUF				((struct uip_tcpip_hdr *)&(((uint8_t *)buf)[ UIP_LLH_LEN]))

#define	IS_ARP				(HTONS(BUF->ethhdr.type) == (0x0806))
#define	IS_IP				(HTONS(BUF->ethhdr.type) == (0x0800))

#define	IS_ICMP				(IPBUF->proto == UIP_PROTO_ICMP)
#define	IS_UDP				(IPBUF->proto == UIP_PROTO_UDP)
#define IS_TCP				(IPBUF->proto == UIP_PROTO_TCP)

#define	IS_DHCPRX			(HTONS(UDPBUF->srcport) == DHCPC_SERVER_PORT &&	\
							HTONS(UDPBUF->destport) == DHCPC_CLIENT_PORT)

#define	IS_DHCPTX			(HTONS(UDPBUF->srcport) == DHCPC_CLIENT_PORT &&	\
							HTONS(UDPBUF->destport) == DHCPC_SERVER_PORT)

/*
#define DGROUP_NONE			5*/
/*#define DM_FALSE			0
#define DM_TRUE				1*/

void sprint_hex_dump0(int head_space, int titledn, char *prefix_str,
		    size_t tlen, int rowsize, const void *buf, int seg_start, size_t len, /*int useflg*/ int cast_lf); //, int must, int dgroup
void dm_check_tx(const uint8_t *buf, size_t len);

#define HEAD_SPC	2	// Head Space
//#define dm9051_rx_log	function_monitor_rx	//as a formal function name
//#define dm9051_tx_log	function_monitor_tx	//as a formal function name

#if DM9051_DEBUG_ENABLE

/*
 * RX log is design as: function_monitor_rx()
 *   hdspc: 0 ,must print-out
 *          ~0, only print-out 'rx_modle.allow_num' times.
 * *Will exechange to ~0 must print-out, and others 'rx_modle.allow_num' times.
 */
#define	function_monitor_rx(hdspc, buffer, len) \
	do { \
		if (hdspc == 0) \
			sprint_hex_dump0(rx_modle_count[RX_MON].allow_num, 0, "dm9 monitor <<rx  ", len, 32, buffer, 0, (len < 70) ? len : 70, /*DM_FALSE*/ DM_TRUE); \
		else if (rx_modle_count[RX_MON].allow_num < rx_modle[RX_MON].allow_num) { \
			rx_modle_count[RX_MON].allow_num++; \
			sprint_hex_dump0(rx_modle_count[RX_MON].allow_num, 0, "dm9 monitor <<rx  ", len, 32, buffer, 0, (len < 70) ? len : 70, /*DM_FALSE*/ DM_TRUE); /*, DM_TRUE, DGROUP_NONE */ \
			/* dm_check_rx(buffer, len); */ \
		} \
	} while(0)

#define	function_monitor_rx_allbx(headstr, buffer, len, hdspc) \
	  sprint_hex_dump0(hdspc, 0, headstr, len, 32, buffer, 0, len, DM_TRUE)

#define	function_monitor_rx_allb(headstr, buffer, len, hdspc) \
	  do { \
		if (headstr) \
			function_monitor_rx_allbx(headstr, buffer, len, hdspc); \
		else \
			function_monitor_rx_allbx("dm9 monitor <<rx_allb  ", buffer, len, hdspc); \
		/*dm_check_rx(buffer, len); */ \
	  } while (0)

#define	function_monitor_rx_all(hdspc, headstr, buffer, len) \
	do { \
		if (headstr || (len < 38)) \
			function_monitor_rx_allb(headstr, buffer, len, hdspc); \
		else { \
			if (len > 38) len = 38; \
			function_monitor_rx_allb( \
				/*hdspc,*/ /*0,*/ \
				headstr, /*len, 32,*/ \
				buffer, /*0,*/ \
				len, /* DM_TRUE,*/ \
				hdspc \
				); \
		} \
	} while(0)

#define function_monitor_tx(hdspc, ttn, headstr, buffer, len) \
	do { \
	dm_check_tx(buffer, len); \
	if (headstr) \
	  sprint_hex_dump0(hdspc, ttn, headstr, len, 32, buffer, 0, (len < 66) ? len : 66, DM_FALSE); /*, DM_TRUE, DGROUP_NONE */ \
	else \
	  sprint_hex_dump0(hdspc, ttn, "dm9 monitor   tx>>", len, 32, buffer, 0, (len < 66) ? len : 66, DM_FALSE); /*, DM_TRUE, DGROUP_NONE */ \
	/*dm_check_tx(buffer, len);*/ \
	} while(0)

#define TX_ALL_LEN	32 //50
#define function_monitor_tx_all(hdspc, ttn, heads, buffer, len) \
	do { \
	dm_check_tx(buffer, len); \
	if (heads) \
		sprint_hex_dump0(hdspc, ttn, heads, len, 32, buffer, 0, (len < TX_ALL_LEN) ? len : TX_ALL_LEN, DM_TRUE); /*, DM_TRUE, DGROUP_NONE */ \
	else \
		sprint_hex_dump0(hdspc, ttn, "dm9 monitor-all   tx>>", len, 32, buffer, 0, (len < TX_ALL_LEN) ? len : TX_ALL_LEN, DM_TRUE); /*, DM_TRUE, DGROUP_NONE */ \
	} while(0)
#else

//#undef _function_monitor_rx
//#undef _function_monitor_rx_all
//#undef _function_monitor_tx
//#undef _function_monitor_tx_all

#define function_monitor_rx(hdspc, buffer, len)
#define function_monitor_rx_all(hdspc, headstr, buffer, len)
#define function_monitor_tx(hdspc, ttn, headstr, buffer, len)
#define function_monitor_tx_all(hdspc, ttn, heads, buffer, len)

#endif

/* dm9051_lw_log-X0.h */

int DBG_IS_ARP(void *dataload);
int DBG_IS_TCP(void *dataload);


#endif //__DM9051_LOGI_H
