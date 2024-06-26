/*
 * Copyright (c) 2023-2025 Davicom Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This DM9051 Driver for LWIP tcp/ip stack
 * First veryification with : AT32F415
 *
 * Author: Joseph CHANG <joseph_chang@davicom.com.tw>
 * Date: 20230411
 * Date: 20230428 (V3)
 * Date: 20240215 (Versions log please refer to 'dm9051_lw/info.txt')
 */

/* SPI master control is essential,
 * Do define spi specific definition, depend on your CPU's board support package library.
 * Here we had the definition header file with : "at32f415_spi.h"
 * for AT32F415 cpu.
 */
#include "dm9051opts.h"
#include "dm9051_lw.h"

//#include "dm9051_lw_debug.h"
//#define printf(fmt, ...) DM9051_DEBUGF(DM9051_TRACE_DEBUG, (fmt, ##__VA_ARGS__))

#include "../freertos_tasks_debug.h" //#include "dm9051_lw_debug.h"
#define printf(fmt, ...) TASK_DM9051_DEBUGF(TASK_SEMAPHORE_MAIN_ON, /*SEMA_OFF*/ SEMA_ON, "[w]", (fmt, ##__VA_ARGS__))

static void dm9051_phycore_on(uint16_t nms, int pin);
static void dm9051_core_reset(int pin);
static void dm9051_set_par(const uint8_t *macadd, int pin);
static void dm9051_set_mar(int pin);
static void dm9051_set_recv(int pin);

#if 0 //[Since lwip_rx_hdlr()/lwip_rx_loop_handler() can do check the link-state in advance, so this logic can then discard!]
static u8 lw_flag[ETHERNET_COUNT];
static u16 unlink_count[ETHERNET_COUNT], unlink_stamp[ETHERNET_COUNT];
#endif

#if  freeRTOS && freeRTOS_LOCK_SPI_MUTEX
	#include "lwip/sys.h"

	// Declare the mutex handle globally so it can be accessed in different tasks
	sys_mutex_t lock_spi_core;
	void dm9051_freertos_init(void)
	{
		// Create a mutex
		if (sys_mutex_new(&lock_spi_core) != ERR_OK) {
			LWIP_ASSERT("failed to create lock_spi_core", 0);
		}
	}

#else
	void dm9051_freertos_init(void) {}
#endif


//#define UNLINK_COUNT_RESET	60000

void dm9051_poweron_rst(void)
{
	LOCK_SPI_CORE();
	DM9051_Poweron_Reset();
	UNLOCK_SPI_CORE();
}

int check_chip_id(uint16_t id) {
	int res = (id == (DM9051_ID >> 16)
			|| id == (DM9052_ID >> 16)
	#if 1
	//_CH390
			|| id == (0x91511c00 >> 16)
			|| id == (0x91001c00 >> 16) /* Test CH390 */
	#endif
		) ? 1 : 0;

	if (id == (0x91001c00 >> 16)) {
		printf("---------------------- \r\n");
		printf("--- warn case %04x --- \r\n", id);
		printf("---------------------- \r\n");
	}

	return res;
}

//static uint16_t read_chip_id(void) {
// uint16_t read_chip_id(void) {
uint16_t read_chip_id(int pin) {
	u8 buff[2];
	LOCK_SPI_CORE();
	cspi_read_regs(DM9051_PIDL, buff, 2, CS_EACH, pin);
	UNLOCK_SPI_CORE();
	return buff[0] | buff[1] << 8;
}

//static void read_chip_revision(u8 *ids, u8 *rev_ad) {
void read_chip_revision(uint8_t *ids, uint8_t *rev_ad, int pin) {
	LOCK_SPI_CORE();
	cspi_read_regs(DM9051_VIDL, ids, 5, OPT_CS_PIN(csmode, pin), pin); //dm9051opts_csmode_tcsmode()
	cspi_read_regs(0x5C, rev_ad, 1, OPT_CS_PIN(csmode, pin), pin); //dm9051opts_csmode_tcsmode()
	UNLOCK_SPI_CORE();
}

uint16_t eeprom_read(uint16_t wordnum, int pin)
{
	u16 uData;
	LOCK_SPI_CORE();
//	uint8_t pin=0;
	do {
		int w = 0;
		cspi_write_reg(DM9051_EPAR, wordnum, pin);
		cspi_write_reg(DM9051_EPCR, 0x4, pin); // chip is reading
		dm_delay_us(1);
		while(cspi_read_reg(DM9051_EPCR, pin) & 0x1) {
			dm_delay_us(1);
			if (++w >= 500) //5
				break;
		} //Wait complete
		cspi_write_reg(DM9051_EPCR, 0x0, pin);
		uData = (cspi_read_reg(DM9051_EPDRH, pin) << 8) | cspi_read_reg(DM9051_EPDRL, pin);
	} while(0);
	UNLOCK_SPI_CORE();
	return uData;
}

// static uint16_t phy_read(uint16_t uReg)
uint16_t phy_read(uint16_t uReg, int pin)
{
	int w = 0;
	u16 uData;
	LOCK_SPI_CORE();
#if 1
  //_CH390
  if (DM_GET_FIELD_PIN(uint16_t, read_chip_id, pin) == 0x9151 && uReg == PHY_STATUS_REG)
	dm9051_phycore_on(0, pin); //if (uReg == PHY_STATUS_REG)
#endif

	cspi_write_reg(DM9051_EPAR, DM9051_PHY | uReg, pin);
	cspi_write_reg(DM9051_EPCR, 0xc, pin);
	dm_delay_us(1);
	while(cspi_read_reg(DM9051_EPCR, pin) & 0x1) {
		dm_delay_us(1);
		if (++w >= 500) //5
			break;
	} //Wait complete

	cspi_write_reg(DM9051_EPCR, 0x0, pin);
	uData = (cspi_read_reg(DM9051_EPDRH, pin) << 8) | cspi_read_reg(DM9051_EPDRL, pin);
	UNLOCK_SPI_CORE();

	#if 0
	if (uReg == PHY_STATUS_REG) {
		if (uData  & PHY_LINKED_BIT)
			dm9051_set_flags(lw_flag, DM9051_FLAG_LINK_UP);
		else
			dm9051_clear_flags(lw_flag, DM9051_FLAG_LINK_UP);
	}
	#endif

	return uData;
}

//static ; function "phy_write" was declared but never referenced.
void phy_write(uint16_t reg, uint16_t value, int pin)
{
	int w = 0;

	LOCK_SPI_CORE();
	cspi_write_reg(DM9051_EPAR, DM9051_PHY | reg, pin);
	cspi_write_reg(DM9051_EPDRL, (value & 0xff), pin);
	cspi_write_reg(DM9051_EPDRH, ((value >> 8) & 0xff), pin);
	/* Issue phyxcer write command */
	cspi_write_reg(DM9051_EPCR, 0xa, pin);
	dm_delay_us(1);
	while(cspi_read_reg(DM9051_EPCR, pin) & 0x1){
		dm_delay_us(1);
		if (++w >= 500) //5
			break;
	} //Wait complete
	cspi_write_reg(DM9051_EPCR, 0x0, pin);
	UNLOCK_SPI_CORE();
}

void test_plan_mbndry(int pin)
{
	uint8_t isr0, isr1, mbndry0, mbndry1;
	char *str0, *str1;

	LOCK_SPI_CORE();
	isr0 = cspi_read_reg(DM9051_ISR, pin);

	mbndry0 = OPT_U8_PIN(iomode, pin);
	str0 = (mbndry0 & MBNDRY_BYTE) ? "(8-bit mode)" : "(16-bit mode)";
	printf("  ACTION: Start.s Write the MBNDRY %02x %s\r\n", mbndry0, str0);

	cspi_write_reg(DM9051_MBNDRY, mbndry0, pin);

	mbndry1 = cspi_read_reg(DM9051_MBNDRY, pin);
	str1 = (mbndry1 & MBNDRY_BYTE) ? "(8-bit mode)" : "(16-bit mode)";
	if ((mbndry0 & MBNDRY_BYTE) == (mbndry1 & MBNDRY_BYTE))
		printf("  ACTION: !And.e Read check MBNDRY %02x %s\r\n", mbndry1, str1);
	else
		printf("  ACTION: !But.e Read check MBNDRY %02x %s \"( read different )\"\r\n",
			mbndry1, str1); //"(read diff, be an elder revision chip bit7 can't write)"

	isr1 = DM9051_Read_Reg(DM9051_ISR, pin);
	UNLOCK_SPI_CORE();

	printf("  RESET: ISR.read.s %02x %s\r\n", isr0, isr0 & 0x80 ? "(8-bit mode)" : "(16-bit mode)");
	printf("  RESET: ISR.read.e %02x %s\r\n", isr1, isr1 & 0x80 ? "(8-bit mode)" : "(16-bit mode)");
}

/*void test_plan_100mf(ncrmode_t ncrmode, u8 first_log)
{
	//
	// explicity, phy write bmcr, 0x3300,
	// Since test mode may stop in 10M Half
	// We nee Full duplex for using a loopback terminator
	// for self xmit/receive in starting test item.
	// observ, AN did take some more time to link up state
	// compare to non AN.
	//
	//if (_dm9051opts_ncrforcemode()) {
	  #if 1
		//[dm9052]
		if (ncrmode == NCR_FORCE_100MF) {
			phy_write(27, 0xe000);
			phy_write(0, 0x2100);
			if (first_log) {
				uint16_t bmcr= phy_read(0);
				printf("  RESET: REG27 write : %04x\r\n", 0xe000);
				printf("  RESET: BMCR write/read : %04x/%04x\r\n", 0x2100, bmcr);
				printf("  RESET: _core_reset [set link parameters, Force mode for 100M Full]\r\n");
			}
		}
		//[dm9051c]
		else if (ncrmode == NCR_AUTO_NEG) {
			phy_write(0, 0x3300);
			if (first_log) {
				uint16_t bmcr= phy_read(0);
				printf("  RESET: BMCR write/read : %04x/%04x\r\n", 0x3300, bmcr);
				printf("  RESET: _core_reset [set link parameters, A.N. for 100M Full]\r\n");
			}
		}
	  #endif
	//}
}*/

/*void test_plan_rx_checksum_enable(void)
{
	DM9051_Write_Reg(DM9051_RCSSR, RCSSR_RCSEN);
}*/

static void dm9051_delay_in_core_process(uint16_t nms, char *zhead, int pin) //finally, dm9051_lw.c
{
	if (nms)
		printf(": dm9051_driver[%d] ::: %s delay %u ms.. : \r\n", pin, zhead, nms);
	if (nms)
	  dm_delay_ms(nms); //_delay_ms(nms); //from James' advice! to be determined with a reproduced test cases!!
}

static void dm9051_phycore_on(uint16_t nms, int pin) {
	cspi_write_reg(DM9051_GPR, 0x00, pin);		//Power on PHY
	dm9051_delay_in_core_process(nms, "_phycore_on<>", pin);
}

static void dm9051_ncr_reset(uint16_t nms, int pin) {
	cspi_write_reg(DM9051_NCR, DM9051_REG_RESET, pin); //iow(DM9051_NCR, NCR_RST);
	dm9051_delay_in_core_process(nms, "_core_reset<>", pin); //dm_delay_ms(250); //CH390-Est-Extra
}

static void dm9051_core_reset(int pin)
{
	// int pin=0;
	display_identity(mstep_spi_conf_name(pin), 0, NULL, 0, pin); //printf(".(Rst.process[%d])\r\n", pin);

	if (OPT_CONFIRM_PIN(generic_core_rst, pin)){

		#if 0
		printf("-----------------------------------------------------------------------------------------\r\n");
		printf("--------------------- write a long delay type procedure ,for core reset -----------------\r\n");
		printf("-----------------------------------------------------------------------------------------\r\n");
		#endif

		//CH390
		  dm9051_ncr_reset(OPT_U8_PIN(hdir_x2ms, pin)*2, pin);
		  dm9051_phycore_on(250, pin);

		  cspi_write_reg(DM9051_MBNDRY, OPT_U8_PIN(iomode, pin), pin);
		  cspi_write_reg(DM9051_PPCR, PPCR_PAUSE_COUNT, pin); //iow(DM9051_PPCR, PPCR_SETTING); //#define PPCR_SETTING 0x08
		  cspi_write_reg(DM9051_LMCR, LMCR_MODE1, pin);
		  cspi_write_reg(DM9051_INTR, INTR_ACTIVE_LOW, pin); // interrupt active low
	} else {
		  int i = pin;
		  u8 if_log = first_log_get(i); //+first_log_clear(i);

	#if 0
		//DAVICOM
		  printf("  _core_reset[%d]()\r\n", i);
		  dm9051_phycore_on(5);

		  DM9051_Write_Reg(DM9051_NCR, DM9051_REG_RESET); //iow(DM9051_NCR, NCR_RST);
		  dm_delay_ms(0);
	#else
		//CH390
		  printf("  _core_reset[%d]()\r\n", i);
			cspi_write_reg(DM9051_NCR, DM9051_REG_RESET, pin); //iow(DM9051_NCR, NCR_RST);
		  dm_delay_ms(2); //CH390-Est-Extra

		  dm9051_phycore_on(5, pin);
	#endif

		  do {
			if (if_log) {
			  test_plan_mbndry(pin);
			} else
			  cspi_write_reg(DM9051_MBNDRY, OPT_U8_PIN(iomode, pin), pin);
		  } while(0);

		  cspi_write_reg(DM9051_PPCR, PPCR_PAUSE_COUNT, pin); //iow(DM9051_PPCR, PPCR_SETTING); //#define PPCR_SETTING 0x08
		  cspi_write_reg(DM9051_LMCR, LMCR_MODE1, pin);
		  #if 1
		  cspi_write_reg(DM9051_INTR, INTR_ACTIVE_LOW, pin); // interrupt active low
		  #endif

		#if TEST_PLAN_MODE
		if (OPT_CONFIRM_PIN(rxmode_checksum_offload, pin))
			cspi_write_reg(DM9051_RCSSR, RCSSR_RCSEN, pin);
		#else
		/* if (isrxmode_checksum_offload())
				DM9051_Write_Reg(DM9051_RCSSR, RCSSR_RCSEN); */
		#endif

		/* flow_control_config_init */
		if (OPT_CONFIRM_PIN(flowcontrolmode, pin)) {
		  #if TEST_PLAN_MODE == 0
			printf("  core_reset: %s, fcr value %02x\r\n", dm9051opts_pin_desc_flowcontrolmode(pin), FCR_DEFAULT_CONF);
		  #endif
			cspi_write_reg(DM9051_FCR, FCR_DEFAULT_CONF, pin);
		}

	#if TEST_PLAN_MODE
		test_plan_100mf(OPT_BMCR(bmcrmode), if_log); //= dm9051opts_bmcrmode_tbmcrmode()
	#endif
	}
}


static void dm9051_show_rxbstatistic(u8 *htc, int n)
{
	int i, j;

	//dm9051_show_id();
	printf("SHW rxbStatistic, 254 wrngs\r\n");
	for (i = 0 ; i < (n+2); i++) {
		if (!(i%32) && i) printf("\r\n");
		if (!(i%32) || !(i%16)) printf("%02x:", i);
		if (!(i%8)) printf(" ");
		if (i==0 || i==1) {
			printf("  ");
			continue;
		}
		j = i - 2;
		printf("%d ", htc[j]);
	}
	printf("\r\n");
}

void dm9051_mac_adr(const uint8_t *macadd, int pin)
{
	dm9051_set_par(macadd, pin);
	//show_par();
}
void dm9051_rx_mode(int pin)
{
	dm9051_set_mar(pin);
	//show_mar();

	dm9051_set_recv(pin);
}

static void dm9051_set_par(const u8 *macadd, int pin)
{
	int i;
	for (i=0; i<6; i++)
		cspi_write_reg(DM9051_PAR+i, macadd[i], pin);
}
static void dm9051_set_mar(int pin)
{
	int i;
	for (i=0; i<8; i++)
		cspi_write_reg(DM9051_MAR+i, (i == 7) ? 0x80 : 0x00, pin);
}

static void dm9051_set_recv(int pin)
{
	cspi_write_reg(DM9051_IMR, IMR_PAR | IMR_PRM, pin); //(IMR_PAR | IMR_PTM | IMR_PRM);

	if (OPT_U8_PIN(promismode, pin)) {
		printf("SETRECV: ::: test rx_promiscous (write_reg, 0x05, (1 << 2))\r\n");
		cspi_write_reg(DM9051_RCR, RCR_DEFAULT | RCR_PRMSC | RCR_RXEN, pin); //promiscous
	}
	else
		cspi_write_reg(DM9051_RCR, RCR_DEFAULT | RCR_RXEN, pin); //dm9051_fifo_RX_enable();
}

char *display_identity_bannerline_title = NULL;
char *display_mac_bannerline_defaultN = ": Display Bare device";
char *display_identity_bannerline_defaultN = ": Bare device";
char *display_identity_bannerline_default =  ": Read device";

uint16_t psh_id[ETHERNET_COUNT];
uint8_t psh_ids[ETHERNET_COUNT][5], psh_id_adv[ETHERNET_COUNT];

static int display_identity(char *spiname, uint16_t id, uint8_t *ids, uint8_t id_adv, int pin) {
	if (ids) {
		psh_id[pin] = id;
		memcpy(&psh_ids[pin][0], ids, 5);
		psh_id_adv[pin] = id_adv;
	} else {
		 id = psh_id[pin];
		 memcpy(ids, &psh_ids[pin][0], 5);
		 id_adv = psh_id_adv[pin];
	}

	printf("%s[%d] ::: ids %02x %02x %02x %02x %02x (%s) chip rev %02x, Chip ID %02x (CS_EACH_MODE)%s\r\n",
		display_identity_bannerline_title ? display_identity_bannerline_title : display_identity_bannerline_default,
		pin, ids[0], ids[1], ids[2], ids[3], ids[4], dm9051opts_pin_desc_csmode(pin), id_adv, id,
		ids ? "" : ".(Rst.process)");
	return 0;
}

static char *bare_mac_tbl[2] = {
	": rd-bare device",
	": wr-bare device",
};

static void display_mac_action(char *head, const uint8_t *adr, int pin) {
	display_mac_bannerline_defaultN = head; // ": rd-bare device";/ ": wr-bare device";
	printf("%s[%d] ::: chip-mac %02x%02x%02x%02x%02x%02x\r\n",
				display_identity_bannerline_title ? display_identity_bannerline_title : display_mac_bannerline_defaultN,
				pin, adr[0], adr[1], adr[2], adr[3], adr[4], adr[5]);
}

static void display_baremac(int pin){
	uint8_t buf[6];

	cspi_read_regs(DM9051_PAR, buf, 6, OPT_CS_PIN(csmode, pin), pin); //dm9051opts_csmode_tcsmode()
	display_mac_action(bare_mac_tbl[0], buf, pin); //[0]= ": rd-bare device"
}

 #define	DM9051_Read_Rxb	DM9051_Read_Mem2X

#define	TIMES_TO_RST	10

void hdlr_reset_process(enable_t en, int pin)
{
	dm9051_core_reset(pin);

  #if 1
    #if 1 //or [B].AS dm9051_init's whole operations. Only for _CH390
	if (en) {
		u16 rwpa_w, mdra_ingress;
		exint_menable(NVIC_PRIORITY_GROUP_0); //dm9051_board_irq_enable();
		dm9051_start(mstep_eth_mac(pin), pin);

		rwpa_w = (uint32_t)DM9051_Read_Reg(0x24, pin) | (uint32_t)DM9051_Read_Reg(0x25, pin) << 8; //DM9051_RWPAL

		mdra_ingress = (uint32_t)DM9051_Read_Reg(0x74, pin) | (uint32_t)DM9051_Read_Reg(0x75, pin) << 8; //DM9051_MRRL;
		printf("dm9051_start(pin = %d).e rwpa %04x / ingress %04x\r\n", pin, rwpa_w, mdra_ingress);
	}
    #endif
  #endif
}

u8 ret_fire_time(u8 *histc, int csize, int i, u8 rxb, int pin)
{
	u16 rwpa_w, mdra_ingress;
	u8 times = (histc[i] >= TIMES_TO_RST) ? histc[i] : 0;

	rwpa_w = (uint32_t)cspi_read_reg(0x24, pin) | (uint32_t)cspi_read_reg(0x25, pin) << 8; //DM9051_RWPAL

	mdra_ingress = (uint32_t)cspi_read_reg(0x74, pin) | (uint32_t)cspi_read_reg(0x75, pin) << 8; //DM9051_MRRL;
  printf("%2d. rwpa %04x / ingress %04x, histc[rxb %02xh], times= %d\r\n",
		 histc[i], rwpa_w, mdra_ingress, rxb, times);

	if (times) { //if (histc[i] >= TIMES_TO_RST)
		dm9051_show_rxbstatistic(histc, csize);
		histc[i] = 1;
	}
	return times; //0;
}

static u16 err_hdlr(char *errstr, u32 invalue, u8 zerochk, int pin)
{
	if (zerochk && invalue == 0)
		return 0; //.printf(": NoError as %u\r\n", valuecode);

	printf(errstr, invalue); //or "0x%02x"

	hdlr_reset_process(OPT_CONFIRM_PIN(hdlr_confrecv, pin), pin);    //pin = 0
	return 0;
}

static u16 ev_rxb(uint8_t rxb, int pin)
{
	int i;
	static u8 histc[254] = { 0 }; //static int rff_c = 0 ...;
	u8 times = 1;

	for (i = 0 ; i < sizeof(histc); i++) {
		if (rxb == (i+2)) {
			histc[i]++;
			times = ret_fire_time(histc, sizeof(histc), i, rxb, pin);
			return err_hdlr(" : rxbErr %u times :: dm9051_core_reset()\r\n", times, 1, pin);
			                //: Read device[0] :::
		}
	}
	return err_hdlr(" _dm9051f rxb error times (No way!) : %u\r\n", times, 0, pin);//As: Hdlr (times : 1, zerochk : 0)
}

static u16 ev_status(uint8_t rx_status, int pin)
{
	bannerline_log();
	printf(".(Err.status%2x) _dm9051f:", rx_status);
	if (rx_status & RSR_RF) printf(" runt-frame");

	if (rx_status & RSR_LCS) printf(" late-collision");
	if (rx_status & RSR_RWTO) printf(" watchdog-timeout");
	if (rx_status & RSR_PLE) printf(" physical-layer-err");
	if (rx_status & RSR_AE) printf(" alignment-err");
	if (rx_status & RSR_CE) printf(" crc-err");
	if (rx_status & RSR_FOE) printf(" rx-memory-overflow-err");
	bannerline_log();
	return err_hdlr("_dm9051f rx_status error : 0x%02x\r\n", rx_status, 0, pin);
}

/* if "expression" is true, then execute "handler" expression */
#define DM9051_RX_RERXB(expression, handler) do { if ((expression)) { \
  handler;}} while(0)
#define DM9051_RX_BREAK(expression, handler) do { if ((expression)) { \
  handler;}} while(0)
#define DM9051_TX_DELAY(expression, handler) do { if ((expression)) { \
  handler;}} while(0)

u8 checksum_re_rxb(u8 rxbyte, int pin) {
	if (OPT_CONFIRM_PIN(rxmode_checksum_offload, pin))
		rxbyte &= 0x03;
	return rxbyte;
}
uint16_t dm9051_rx_dump(uint8_t *buff, int pin)
{
	// Because we encounter 16-bit mode fail, so went to try above!
	// Since target dm9051a IS only 8-bit mode (working)
	u8 rxbyte, rx_status;
	u8 ReceiveData[4];
	u16 rx_len;

	rxbyte = DM9051_Read_Rxb(pin);
	rxbyte = checksum_re_rxb(rxbyte, pin); //This is because checksum-offload enable

	DM9051_RX_BREAK((rxbyte != 0x01 && rxbyte != 0), return ev_rxb(rxbyte, pin));
	DM9051_RX_BREAK((rxbyte == 0), return 0);

	DM9051_Read_Mem(ReceiveData, 4, pin);
	DM9051_Write_Reg(DM9051_ISR, 0x80, pin);

	rx_status = ReceiveData[1];
	rx_len = ReceiveData[2] + (ReceiveData[3] << 8);

	DM9051_RX_BREAK((rx_status & 0xbf), return ev_status(rx_status, pin)); //_err_hdlr("_dm9051f rx_status error : 0x%02x\r\n", rx_status, 0)
	DM9051_RX_BREAK((rx_len > RX_POOL_BUFSIZE), return err_hdlr("_dm9051f rx_len error : %u\r\n", rx_len, 0, pin));

	DM9051_Read_Mem(buff, rx_len, pin);
	DM9051_Write_Reg(DM9051_ISR, 0x80, pin);
	return rx_len;
}

 #if TEST_PLAN_MODE
#define check_get()				1
#define check_get_check_done()	0
#define	check_decr_to_done()
#define	check_set_done()
#define	check_set_new()

 #define tp_all_done()	1

 //#ifndef rxrp_dump_print_init_show
 static void rxrp_dump_print_init_show(void) {
	uint16_t rxrp;
	rxrp = cspi_read_reg(DM9051_MRRL);
	rxrp |= cspi_read_reg(DM9051_MRRH) << 8;
	printf(" %4x", rxrp);
 }
 //#endif
 #endif

uint16_t dm9051_rx_lock(uint8_t *buff, int pin)
{
	u8 rxbyte, rx_status;
	u8 ReceiveData[4];
	u16 rx_len;

	#if TEST_PLAN_MODE
	u8 rxbyte_flg;
	int ipf = 0, udpf = 0, tcpf = 0;
	#endif

	rxbyte = DM9051_Read_Rxb(pin); //DM9051_Read_Reg(DM9051_MRCMDX);

	#if TEST_PLAN_MODE //TestMode.RX
	if (check_get() && !check_get_check_done()) { //(checkrxbcount > 1)
		if (rxbyte & RXB_ERR) { //(checksum_re_rxb(rxbyte) != 0x01 && checksum_re_rxb(rxbyte) != 0)
				check_decr_to_done();
				bannerline_log();
				printf(".(Err.rxb%2x)", rxbyte);
				if (check_get_check_done()) {
					printf("\r\n");
					printf("---------------------- on (Err.rxb == %2x) ,check_get_check_done() ----------------------\r\n", rxbyte);
					printf("-------------------------- DM9051_RX_RERXB will process follow -------------------------\r\n");
				}
		}
		else {
			if (rxbyte == 0) {

			  check_decr_to_done(); //need ?
			#if 0
			  printf(".(rxb%2x)", rxbyte);
			#endif
			  if (check_get_check_done()) {
				bannerline_log();
				printf("-------------------------- (rxb == 0) ,check_get_check_done() --------------------------\r\n");
				printf("-------------------------- On ChecksumOffload, DM9051_RX_RERXB will process follow -------------------------\r\n");
			  }
			}
			//else

			/*if (checksum_re_rxb(rxbyte) == DM9051_PKT_RDY)
			{
				//.printf("[rx %d]\r\n", get_testing_rx_count()); //testing_rx_count
				if (!tp_all_done()) {
					printf("[rx %d]\r\n", get_testing_rx_count());
					display_state();
				}
			}*/

			if (rxbyte != 0x01 && rxbyte != 0) {
				if (rxbyte == 0x40)
					;
				else {
					//Normal- case
					//printf(".ReceivePacket: rxb=%2x (processed) to %02x\r\n", rxbyte, rxbyte & 0x03);

					#if 1 //Extra- work around
					if (rxbyte & RCSSR_TCPS) {
						if (!(rxbyte & RCSSR_TCPP)) {
							rxbyte &= ~RCSSR_TCPS;
							bannerline_log();
							printf("Warn.case.ReceivePacket: rxb=%2x (step1) to %02x, mdra_ingress", rxbyte, rxbyte);
							rxrp_dump_print_init_show();
							bannerline_log();
						}
					}
					#endif
				}

				if (get_testplanlog(test_plan_log, pin)) { //get_dm9051opts_testplanlog()
					if ((rxbyte & RCSSR_IPP)) {
						ipf = 1;
						if (!tp_all_done())
							printf("DM9RX: found IP, checksum %s\r\n", (rxbyte & RCSSR_IPS) ? "fail" : "ok");
					}
					if ((rxbyte & RCSSR_UDPP)) {
						udpf = 1;
						if (!tp_all_done())
							printf("DM9RX: found UDP, checksum %s\r\n", (rxbyte & RCSSR_UDPS) ? "fail" : "ok");
					}
					if ((rxbyte & RCSSR_TCPP)) {
						tcpf = 1;
						if (!tp_all_done())
							printf("DM9RX: found TCP, checksum %s\r\n", (rxbyte & RCSSR_TCPS) ? "fail" : "ok");
					}
				}
				rxbyte = checksum_re_rxb(rxbyte);
			}

			//.if (rxbyte != 0x01 && rxbyte != 0) {
			//.} else
			if (rxbyte == 1) {
			 #if 1
			  check_set_done(); //checkrxbcount = 1;
			  check_set_new(); //SET NEW AGAIN, we do continue testing...
			 #endif
			  //POST output, printf(".rxb%2x\r\n", rxbyte);
			}
		}
	}
	else
		DM9051_RX_RERXB((rxbyte != 0x01 && rxbyte != 0), rxbyte = checksum_re_rxb(rxbyte));
	#endif

	DM9051_RX_BREAK((rxbyte != 0x01 && rxbyte != 0), return ev_rxb(rxbyte, pin));
	DM9051_RX_BREAK((rxbyte == 0), return 0);

	DM9051_Read_Mem(ReceiveData, 4, pin);
	DM9051_Write_Reg(DM9051_ISR, 0x80, pin);

	rx_status = ReceiveData[1];
	rx_len = ReceiveData[2] + (ReceiveData[3] << 8);

  //printf(" :drv.rx %02x %02x %02x %02x (len %u)\r\n",
  //		ReceiveData[0], ReceiveData[1], ReceiveData[2], ReceiveData[3], rx_len);

	#if 0
	if (rx_len != LEN64)
		printf(":drv.rx %02x %02x %02x %02x (len %u)\r\n",
			ReceiveData[0], ReceiveData[1], ReceiveData[2], ReceiveData[3], rx_len);
	#endif

	#if 0
	do {
		static int dump_odd_c = 0;
		if (rx_len & 1) {
		  printf("\r\n+ drv +oddfound %d (len %d)\r\n", ++dump_odd_c, rx_len);
		}
	} while(0);
	#endif

	DM9051_RX_BREAK((rx_status & 0xbf), return ev_status(rx_status, pin)); //_err_hdlr("_dm9051f rx_status error : 0x%02x\r\n", rx_status, 0)
	DM9051_RX_BREAK((rx_len > RX_POOL_BUFSIZE), return err_hdlr("_dm9051f rx_len error : %u\r\n", rx_len, 0, pin));

	DM9051_Read_Mem(buff, rx_len, pin);
	DM9051_Write_Reg(DM9051_ISR, 0x80, pin);

	if (!OPT_CONFIRM_PIN(tx_endbit, pin)) //CH390
		dm_delay_ms(1);

	#if 	0 	//test.
	dm9051_rxlog_monitor_rx(2, buff, rx_len); //HEAD_SPC
	#endif

	#if TEST_PLAN_MODE
	if (get_testplanlog(test_plan_log, pin)) { //get_dm9051opts_testplanlog()
		if (ipf && !tp_all_done())
			printf("DM9RX: IP, checksum %02x %02x\r\n", buff[24], buff[25]);
		if (udpf && !tp_all_done())
			printf("DM9RX: UDP, checksum %02x %02x\r\n", buff[40], buff[41]);
		if (tcpf && !tp_all_done())
			printf("DM9RX: TCP, checksum %02x %02x\r\n", buff[50], buff[51]); //TBD

		rxbyte_flg = ReceiveData[0] & ~(0x03);

		if (!tp_all_done())
			printf("DM9RX[%d]: drv.rx %02x %02x %02x %02x (len %u), rxb %02x | %x\r\n",
				mstep_get_net_index(), ReceiveData[0], ReceiveData[1], ReceiveData[2], ReceiveData[3], rx_len, rxbyte_flg, rxbyte);
	}
	#endif

#if DM9051_DEBUG_ENABLE == 1
	/* An assurence */
	if (dm9051_log_rx(buff, rx_len, pin)) { //ok. only 1st-pbuf
		dm9051_log_rx_inc_count();
		hdlr_reset_process(OPT_CONFIRM_PIN(hdlr_confrecv, pin), pin);    //pin = 0
		return 0;
	}
#endif
	return rx_len;
}

uint16_t dm9051_rx(uint8_t *buff, int pin)
{
	u16 rx_len;
	LOCK_SPI_CORE();
	rx_len = dm9051_rx_lock(buff, pin);
	UNLOCK_SPI_CORE();
	return rx_len;
}

void dm9051_tx(uint8_t *buf, uint16_t len, int pin)
{
	LOCK_SPI_CORE();
	DM9051_Write_Reg(DM9051_TXPLL, len & 0xff, pin);
	DM9051_Write_Reg(DM9051_TXPLH, (len >> 8) & 0xff, pin);
	DM9051_Write_Mem(buf, len, pin);
	DM9051_Write_Reg(DM9051_TCR, TCR_TXREQ, pin); /* Cleared after TX complete */

	if (OPT_CONFIRM_PIN(tx_endbit, pin))
		DM9051_TX_DELAY((DM9051_Read_Reg(DM9051_TCR, pin) & TCR_TXREQ), dm_delay_us(5));
	else
		dm_delay_ms(1); //CH390

	UNLOCK_SPI_CORE();
}

void display_chipmac(int pin)
{
		uint8_t buf[6];
		display_identity_bannerline_defaultN = ": rd-bare device";
		// cspi_read_regs(DM9051_PAR, buf, 6, OPT_CS(csmode)); //dm9051opts_csmode_tcsmode()
		cspi_read_regs(DM9051_PAR, buf, 6, CS_EACH, pin);
		printf("%s[%d] ::: chip-mac %02x%02x%02x%02x%02x%02x\r\n",
			display_identity_bannerline_title ? display_identity_bannerline_title : display_identity_bannerline_defaultN,
			pin, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
}

#if 0
int display_verify_chipid(char *str, char *spiname, uint16_t id) {
	if (_check_chip_id(id)) { //(id == (DM9051_ID/DM9052_ID >> 16))
		chipid_on_pin_code_log_s(spiname, str, id);
	//#if NON_CHIPID_STOP == 1
	//	printf("Chip ID CHECK experiment! Succeed OK!!\r\n");
	//	while(1) ; //Feature attribute experiment!!
	//#endif
		return 1;
	} //else {
		chipid_on_pin_code_log_err(spiname, str, id);
		return 0;
	//#if NON_CHIPID_STOP == 1
	//	while(1) ;
	//#endif
	//}
}
#endif

uint16_t dm9051_init_setup(int pin)
{
	uint16_t id;
	uint8_t ids[5], id_adv;

	id = read_chip_id(pin);
	read_chip_revision(ids, &id_adv, pin);

	display_identity(mstep_spi_conf_name(pin), id, ids, id_adv, pin);

	DM_SET_FIELD_PIN(uint16_t, read_chip_id, id, pin); //store into dm9051optsex[i].read_chip_id
	return id;
}

void dm9051_start(const uint8_t *adr, int pin)
{
	display_baremac(pin);
	display_mac_action(bare_mac_tbl[1], adr, pin); //[1]= ": wr-bare device"

	dm9051_mac_adr(adr, pin);
	dm9051_rx_mode(pin);
}

uint16_t dm9051_init(const uint8_t *adr, int pin)
{
	uint16_t id;

	DM_UNUSED_ARG(adr);
	first_log_init();
	printf("dm9051_init, %s, device[%d] %s, %s, to set mac/ %02x%02x%02x%02x%02x%02x\r\n",
			mstep_conf_info(pin),
			pin,
			mstep_conf_cpu_spi_ethernet(pin),
			mstep_conf_cpu_cs_ethernet(pin),
			adr[0],
			adr[1],
			adr[2],
			adr[3],
			adr[4],
			adr[5]);
	id = dm9051_init_setup(pin);
	if (!check_chip_id(id))
		return id;

	hdlr_reset_process(DM_TRUE, pin);
	return id;
}

uint16_t dm9051_bmcr_update(int pin) {
  // return phy_read(PHY_CONTROL_REG);
	return phy_read(PHY_CONTROL_REG, pin);
}
uint16_t dm9051_bmsr_update(int pin) {
  // return phy_read(PHY_STATUS_REG);
	return phy_read(PHY_STATUS_REG, pin);
}
uint16_t dm9051_link_update(int pin) {
  // return phy_read(PHY_STATUS_REG);
	return phy_read(PHY_STATUS_REG, pin);
}
uint16_t dm9051_phy_read(uint32_t reg, int pin) {
  // return phy_read(reg);
	return phy_read(reg, pin);
}
void dm9051_phy_write(uint32_t reg, uint16_t value, int pin) {
  phy_write(reg, value, pin);
}
uint16_t dm9051_eeprom_read(uint16_t word, int pin) {
  return eeprom_read(word, pin);
}

static uint16_t bityes(uint8_t *hist) {
	uint16_t i;
	for (i = 0; i< 16; i++)
		if (hist[i])
			return 1;
	return 0;
}

static uint16_t link_show(int pin) {
	u8 n = 0, i, histnsr[16] = { 0, }, histlnk[16] = { 0, };
	u8 val;
	u16 lnk;
	u16 rwpa_w, mdra_ingress;

	do {
		n++;
		for (i= 0; i< 16; i++) {
			val = DM9051_Read_Reg(DM9051_NSR, pin);
			lnk = phy_read(PHY_STATUS_REG, pin);
			histnsr[i] += (val & 0x40) ? 1 : 0;
			histlnk[i] += (lnk & 0x04) ? 1 : 0;
		}
	} while(n < 20 && !bityes(histnsr) && !bityes(histlnk)); // 20 times for 2 seconds

	rwpa_w = (uint32_t)DM9051_Read_Reg(0x24, pin) | (uint32_t)DM9051_Read_Reg(0x25, pin) << 8; //DM9051_RWPAL
	mdra_ingress = (uint32_t)DM9051_Read_Reg(0x74, pin) | (uint32_t)DM9051_Read_Reg(0x75, pin) << 8; //DM9051_MRRL;

	printf("(SHW timelink, 20 detects) det %d\r\n", n);
	for (i= 8; i< 16; i++)
		printf(" %s%d", (i==8) ? ".nsr " : "", histnsr[i]);
	for (i= 8; i< 16; i++)
		printf(" %s%d", (i==8) ? ".bmsr. " : "", histlnk[i]);
	printf(" (rrp %x rwp %x)\r\n", mdra_ingress, rwpa_w);
	return bityes(histnsr) && bityes(histlnk);
}

uint16_t dm9051_link_show(int pin)
{
	return link_show(pin);    // pin = 0
}
