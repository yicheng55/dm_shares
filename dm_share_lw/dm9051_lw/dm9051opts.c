//
// dm9051opts.c
//
#include "dm9051opts.h"
#include "dm9051_lw.h"

u8 gfirst_log[ETHERNET_COUNT];

void GpioDisplay(void) {
  int i;
  for (i = 0; i < ETHERNET_COUNT; i++) {
//	mstep_set_net_index(i);
	printf("@%s, %s, %s\r\n", mstep_conf_info(i), mstep_conf_cpu_spi_ethernet(i), mstep_conf_cpu_cs_ethernet(i));
  }
}

void dm9051_opts_display(void)
{
	int i;
	GpioDisplay();
	for (i = 0; i< ETHERNET_COUNT; i++) {
//		mstep_set_net_index(i);
		bannerline_log();
		printf("dm9051[%d]_options_display:\r\n", i);
		printf(" - core rst mode, %s\r\n", dm9051opts_pin_desc_generic_core_rst(i));
		printf(" - tx_endbit mode, %s\r\n", dm9051opts_pin_desc_tx_endbit(i));
		//..
	}
}

#if DM9051OPTS_LOG_ENABLE
	void dm9051_opts_iomod_etc(void)
	{
		int i;
		// int pin;
		GpioDisplay();
		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("iomode[%d] %s / value %02x\r\n", i, dm9051opts_pin_desc_iomode(i), OPT_U8_PIN(iomode, i)); //dm9051opts_iomode()
		}

		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("csmode[%d] %s\r\n", i, dm9051opts_pin_desc_csmode(i));
		}

		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("bmcmod[%d] %s\r\n", i, dm9051opts_pin_desc_bmcrmode(i));
		}

		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("rx's mode[%d] %s\r\n", i, dm9051opts_pin_desc_promismode(i));
		}

		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("rxtype[%d] %s\r\n", i, dm9051opts_pin_desc_rxtypemode(i));
		}

		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("chksum[%d] %s / value %02x %s\r\n", i, dm9051opts_pin_desc_rxmode_checksum_offload(i), //~dm9051opts_desc_rxchecksum_offload(),
					OPT_CONFIRM_PIN(rxmode_checksum_offload, i) ? RCSSR_RCSEN : 0,
					OPT_CONFIRM_PIN(rxmode_checksum_offload, i) ? "(RCSSR_RCSEN)" : " "); //is_dm9051opts_rxmode_checksum_offload ~is_dm9051opts_rxchecksum_offload
		}

		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("fcrmod[%d] %s / value %02x\r\n", i, dm9051opts_pin_desc_flowcontrolmode(i),
					OPT_CONFIRM_PIN(flowcontrolmode, i) ? FCR_DEFAULT_CONF : 0);
		}

		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("iomode[%d] %s %s\r\n", i, //"device: ", dm9051opts_desconlybytemode()
					OPT_CONFIRM_PIN(onlybytemode, i) ? "device: " : "set-to: ",
					OPT_CONFIRM_PIN(onlybytemode, i) ? dm9051opts_pin_desc_onlybytemode(i) : dm9051opts_pin_desc_iomode(i));
		}

		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("chip[%d]: 0x%x\r\n", i, dm9051_init_setup(i));
		}

	#if TO_ADD_CODE_LATER_BACK
		for (i = 0; i< ETHERNET_COUNT; i++) {
			uint8_t *macaddr;
			macaddr = mstep_eth_mac(i);
			printf("config tobe mac[%d] %02x%02x%02x%02x%02x%02x\r\n", i, macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
		}
	#endif
	#if TO_ADD_CODE_LATER_BACK
		for (i = 0; i< ETHERNET_COUNT; i++) {
			EepromDisplay(i);
		}
	#endif
	#if TO_ADD_CODE_LATER_BACK
		for (i = 0; i< ETHERNET_COUNT; i++) {
			printf("ip[%d] %"U16_F".%"U16_F".%"U16_F".%"U16_F"\r\n", i,
			  ip4_addr1_16(netif_ip4_addr(&xnetif[i])),
			  ip4_addr2_16(netif_ip4_addr(&xnetif[i])),
			  ip4_addr3_16(netif_ip4_addr(&xnetif[i])),
			  ip4_addr4_16(netif_ip4_addr(&xnetif[i])));
		}
	#endif

	}

	void ethcnt_ifdiplay(void)
	{
	#if 0
		int i;
		for (i = 0; i< _ETHERNET_COUNT; i++) {
			NetifDisplay(i);
			EepromDisplay(i);
		}
	#endif
	}
#endif

void first_log_init(void)
{
	int i;
	for (i = 0; i < ETHERNET_COUNT; i++)
		gfirst_log[i] = 1; //Can know the first time reset the dm9051 chip!
}

u8 first_log_get(int i)
{
	u8 if_log =
		gfirst_log[i];
	gfirst_log[i] = 0; //first_log_clear(i);
	return if_log;
}

//void first_log_clear(int i)
//{
//	gfirst_log[i] = 0;
//}

/* , uint8_t trans_type
 * == MULTI_TRANS,
 *  return : found id number
 */
//int TRANS_DUAL(trn_conn_t f) {
//  int i;
//  int nID = 0;
//  uint16_t id;
//  for (i = 0; i < ETHERNET_COUNT; i++) {
	//.mstep_set_net_index(i); + //dm9051_init(_mstep_eth_mac());
//	id = f(i); //= drviver_init(i)
//	if (check_chip_id(id))
//	 nID++;
//  }
//return nID;
//}

//voidfn_dual
void ETH_COUNT_VOIDFN(voidpin_t f) {
  int i;
  for (i = 0; i < ETHERNET_COUNT; i++) {
//	  mstep_set_net_index(i);
	  f(i);
  }
}

//voidtx_dual
void ETH_COUNT_VOIDTX(voidtx_t pinfunc, uint8_t *buf, uint16_t len) {
  int i;
  for (i = 0; i < ETHERNET_COUNT; i++) {
//	  mstep_set_net_index(i);
	  pinfunc(i, buf, len);
  }
}
