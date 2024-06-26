//mqttc
linking...
//[dm9051_lw_conf.o]
/*(main.o)*/  dm9051_boards_initialize (referred from main.o).
/* mqtt_client.o */ mstep_set_net_index (referred from mqtt_client.o).
/* netconf_b.o */ mstep_get_net_index (referred from netconf_b.o).
/* stack_init.o */ mstep_eth_mac (referred from stack_init.o).
/* developer_conf.o */ mstep_eth_gw (referred from developer_conf.o).
/* developer_conf.o */ mstep_eth_ip (referred from developer_conf.o).
/* developer_conf.o */ mstep_eth_mask (referred from developer_conf.o).
//[dm9051opts.o]
/*(mqtt_main.o)*/  ETH_COUNT_VOIDFN (referred from mqtt_main.o).
/* ethernetif.o */ ETH_COUNT_VOIDTX (referred from ethernetif.o).
//[dm9051_lw.o]
/* ethernetif.o */ dm9051_init (referred from ethernetif.o).
/* ethernetif.o */ dm9051_rx (referred from ethernetif.o).
/* ethernetif.o */ dm9051_tx (referred from ethernetif.o).
/* ethernetif.o */ check_chip_id (referred from ethernetif.o).
/* netconf_b.o */ dm9051_bmsr_update (referred from netconf_b.o).
//[dm9051_lw_log.o]
/* ethernetif.o */ dm9051_rxlog_ack (referred from ethernetif.o).
/* ethernetif.o */ dm9051_rxlog_arp (referred from ethernetif.o).
/* ethernetif.o */ dm9051_rxlog_monitor_rx_all (referred from ethernetif.o).
/* ethernetif.o */ dm9051_txlog_disp (referred from ethernetif.o).
/* ethernetif.o */ dm9051_txlog_monitor_tx (referred from ethernetif.o).
//[dmtypes.h]
#if DM_TYPE == 0
/* netif.o */ dm9051opts_enable_ttest_plan_log //rtype dm9051opts_##mtype##field(void); //DM_GET_FIELD()
/* netif.o */ (dm9051opts_desctest_plan_log (referred from netif.o).) //char *dm9051opts_desc##field(void); //DM_GET_DESC
/* linkhdlr_timeouts.o */ dm9051opts_descbmcrmode (referred from linkhdlr_timeouts.o). //char *dm9051opts_desc##field(void); //DM_GET_DESC()
#endif

/* linkhdlr_timeouts.o */  dm9051_bmcr_update (referred from linkhdlr_timeouts.o).
/* ethernetif.o */ cspi_read_reg (referred from ethernetif.o).
/* comm.o */ mstep_next_net_index (referred from netconf - comm.o).
/* ethernetif.o */ mstep_spi_conf_name (referred from ethernetif.o).
/* log.o */ GpioDisplay (referred from netcon2 - log.o).

//X2Web
//[dm9051_lw.o]
#if DM9051OPTS_API
/*(proc_main.o)*/ uint16_t dm9051_init(const uint8_t *adr);
/*(netconf.o)*/ uint16_t dm9051_rx(uint8_t *buff);
/*(ethernetif.o)*/ void dm9051_tx(uint8_t *buf, uint16_t len);
/*(ethernetif.o)*/ void hdlr_reset_process(enable_t en);
/*(proc_main.o)*/ void dm9051_mac_adr(const uint8_t *macadd);
/*(proc_main.o)*/ uint16_t dm9051_bmsr_update(void);
/*(ethernetif.o)*/ int check_chip_id(uint16_t id);
#endif
/*(netconf.o)*/ uint16_t dm9051_phy_read(uint32_t reg);
/*(proc_main.o)*/ uint16_t dm9051_bmcr_update(void);
/*(proc_main.o)*/ uint16_t dm9051_rx_dump(uint8_t *buff);
/*(proc_main.o)*/ char *display_identity_bannerline_title;

//[dm9051_lw_conf.o]
//init
#if DM9051OPTS_API
/*(main.o)*/ void dm9051_boards_initialize(int n);
/*(stack_init.o)*/ uint8_t *identify_eth_mac(uint8_t *macadr)
/*(stack_init.o)*/ uint8_t *identify_tcpip_ip(uint8_t *ip4adr);
/*(stack_init.o)*/ uint8_t *identify_tcpip_gw(uint8_t *ip4adr);
/*(stack_init.o)*/ uint8_t *identify_tcpip_mask(uint8_t *ip4adr)
/*(main.o)*/ void mstep_set_net_index(int i);
/*(proc_main.o)*/ int mstep_get_net_index(void);
/*(at32f435_437_int.o)*/ void dm9051_irqlines_proc(void);
#endif
/*(proc_main.o)*/ void dm_delay_ms(uint16_t nms);
/*(proc_main.o)*/ uint8_t *mstep_eth_mac(void);
/*(developer_conf.o)*/ uint8_t *mstep_eth_ip(void);
/*(developer_conf.o)*/ uint8_t *mstep_eth_gw(void);
/*(developer_conf.o)*/ uint8_t *mstep_eth_mask(void);
/*(proc_main.o)*/ cspi_read_reg
/*(proc_main.o)*/ cspi_read_regs
/*(proc_main.o)*/ cspi_write_reg

//[dm9051_lw_log.o]
#if DM9051OPTS_API
#if DM9051_DEBUG_ENABLE == 1
/*(ooooooo.o)*/ #define bannerline_log()   printf("\r\n")
/*(netifo.o)*/ void dm9051_txlog_monitor_tx(int hdspc, const uint8_t *buffer, size_t len);
/*(netconf.o)*/ void dm9051_rxlog_monitor_rx_all(int hdspc, const uint8_t *buffer, size_t len);
/*(xxxxxxx.o)*/ void dm9051_rxlog_monitor_rx(int hdspc, const uint8_t *buffer, size_t len);
/*(ethernetif.o)*/ void dm9051_txlog_monitor_tx_all(int hdspc, const uint8_t *buffer, size_t len);
/*(ethernetif.o)*/ void dm9051_txlog_disp(uint8_t *buffer, int len);
/*(ethernetif.o)*/ void dm9051_rxlog_ack(void *payload, uint16_t tot_len, char *result_str);
/*(ethernetif.o)*/ void dm9051_rxlog_arp(void *payload, uint16_t tot_len, uint16_t len);
#endif
#endif
//[dm9051opts.o]
#if DM9051OPTS_API
//.int TRANS_DUAL(trn_conn_t f); //return : found id number
/*(main.o)*/ void dm9051_opts_display(void);
#endif
#if DM9051OPTS_LOG_ENABLE
/*(main.o)*/ void dm9051_opts_iomod_etc(void);
#endif
/*(log.o)*/ void ethcnt_ifdiplay(void);
//[dmtypes.h]
#if DM_TYPE == 0
// - type 0, extern API
/*(main.o)*/ dm9051opts_enable_trxtypemode //rtype dm9051opts_##mtype##field(void);
/*(main.o)*/ dm9051opts_descrxtypemode //char *dm9051opts_desc##field(void);
/*(proc_main.o)*/ dm9051opts_enable_ttest_plan_log //rtype dm9051opts_##mtype##field(void);
/*(proc_main.o)*/ dm9051opts_csmode_tcsmode //rtype dm9051opts_##mtype##field(void);
/*(proc_main.o)*/ dm9051opts_enable_tflowcontrolmode //rtype dm9051opts_##mtype##field(void);
/*(proc_main.o)*/ dm9051opts_descflowcontrolmode //rtype dm9051opts_##mtype##field(void);
/*(netconf.o)*/ dm9051opts_bmcrmode_tbmcrmode //rtype dm9051opts_##mtype##field(void);
/*(ethernetif.o)*/ dm9051opts_enable_ttx_endbit //rtype dm9051opts_##mtype##field(void);
#endif
