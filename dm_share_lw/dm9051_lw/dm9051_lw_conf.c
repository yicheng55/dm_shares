/**
  **************************************************************************
  * @file     dm9051_lw_conf.c
  * @file     dm9051_at32_conf.c
  * @file     dm9051_env.c ,dm9051_config.c ,at32_emac.c
  * @version  v1.0
  * @date     2023-04-28, 2023-10-17, 2023-10-24
  * @brief    dm9051 config program
  **************************************************************************
  *                       Copyright notice & Disclaimer
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */
#include "dm9051opts.h"
#include "dm9051_lw.h"
#include "dm9051_lw_conf_types.h"
#include "dm9051_lw_cspi.h"
#include "dm9051_lw_conf_data.h" //1.yicheng
#include "dm9051_lw_debug.h"

//#ifdef AT32F437xx
//#include "dm9051_lw_conf_at437x2spi.h" //1.yicheng
//#else
//#include "dm9051_lw_conf_at437x2spi.h" //or "dm9051_lw_conf_at415x2spi.h"
//#endif

/*
In xxx_int.c, 

To have interrupt mode support function.

	void EXINT9_5_IRQHandler(void) {
	  dm9051_irqlines_proc(); //call the 'dm9051_irqlines_proc()',
	}
*/

//void dm9051_irqlines_proc(void)
//{
////  void xxethernetif_exint_proc(int i);
//  
//	  printf("dm9051_irqlines_proc()-ethernetif _exint _proc()\r\n");
//	  if (!exint_exister()) //[To be enum , e.g. intr_pack[i], if multi-cards]
//		return;
//  
//	  if (exint_flag_get(exint_extline()) != RESET) //if (exint_flag_get(EXINT_LINE_7) != RESET)
//	  {
//		#if ETHERNET_INTERRUPT_MODE
//		#if 1
//		if (exint_extline() == EXINT_LINE_7)
//		{
//			int pin = 0;
//			if (exint_extline() == EXINT_LINE_7)
//				pin = 0;
//			
//			xxethernetif_exint_proc(pin);
//		}
//		#endif
//		#endif
//		exint_flag_clear(exint_extline()); //exint_flag_clear(EXINT_LINE_7);
//	  }
//}

/*********************************
 * dm9051 delay times procedures
 *********************************/

#define	board_printf(format, args...) //int board_printf(const char *format, args...) { return 0; }

void dm_delay_us(uint32_t nus) {
//	void delay_us(uint32_t nus);
	board_printf("test %d ,because rxb %02x (is %d times)\r\n", rstccc, rxbyteee, timesss);
	dmf.dly_us(nus); //delay_us(nus);
}
void dm_delay_ms(uint16_t nms) {
//	void delay_ms(uint16_t nms);
	dmf.dly_ms(nms); //delay_ms(nms);
}

//[Add GPIO]
/*********************************
  * @brief  gpio pin configuration
  * @brief  spi configuration
  * @brief  exint configuration
 *********************************/
static void gpio_pin_config(const gpio_t *gpio, gpio_pull_type gppull) //, gpio_mode_type gpmode
{
  gpio_init_type gpio_init_struct;
  crm_periph_clock_enable(gpio->gpio_crm_clk, TRUE); /* enable the gpioa clock */

  gpio_default_para_init(&gpio_init_struct);
  gpio_init_struct.gpio_out_type  		= GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_drive_strength	= GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_mode			= gpio->gpio_mode; //gpmode; //GPIO_MODE_INPUT;

  gpio_init_struct.gpio_pull			= gppull; //exint_cfg.gpio_pull; //GPIO_PULL_DOWN; GPIO_PULL_UP; //GPIO_PULL_NONE;
  gpio_init_struct.gpio_pins			= gpio->pin;
  gpio_init(gpio->gpport, &gpio_init_struct);

 #ifdef AT32F437xx
  if ((gpio->gpio_mode == GPIO_MODE_MUX) && (gpio->muxsel != GPIO_MUX_NULL))
	gpio_pin_mux_config(gpio->gpport, gpio->pinsrc, gpio->muxsel);
  else {
    // only for intr-gpio
	if (intr_gpio_exister())
		// only for intr-gpio
		printf(": %s, no gpio_pin_mux_config()\r\n",
				intr_gpio_info());
  }
 #endif
}

//[Add int]
/**
  * @brief  exint pin configuration.
  * @param  struct exint_st* =
  * 	   struct exint_st {
  * 		struct {
  * 			crm_periph_clock_type crm_clk; //CRM_GPIOC_PERIPH_CLOCK,
  * 			uint32_t extline; //= LINE
  * 			IRQn_Type irqn; //= EXINTn_m
  * 		} extline;
  * 		struct {
  * 			crm_periph_clock_type scfg_clk; //CRM_SCFG_PERIPH_CLOCK
  * 			scfg_port_source_type scfg_port_src; //SCFG_PORT_SOURCE_X
  * 			scfg_pins_source_type scfg_pin_src; //SCFG_PINS_SOURCEX
  * 		} scfg_init;
  * 	   }
  *        exint_polarity_config_type
  *        nvic_priority_group_type
  * @e.g.  exint line7 config. configure pc7 in interrupt pin
  * @retval None
  */
static void exint_config(const struct extscfg_st *pexint_set, exint_polarity_config_type polarity) {
  exint_init_type exint_init_struct;

  printf(": %s :                 exint_config/ %s\r\n", "config", scfg_info());

  crm_periph_clock_enable(scfg_crm(), TRUE); // CRM_SCFG_PERIPH_CLOCK
  crm_periph_clock_enable(exint_crm(), TRUE); // CRM_GPIOC_PERIPH_CLOCK

 #ifndef AT32F437xx
  gpio_exint_line_config(scfg_port(), scfg_pin()); //SCFG_PORT_SOURCE_GPIOA, SCFG_PINS_SOURCE0
 #else
  scfg_exint_line_config(scfg_port(), scfg_pin()); //SCFG_PORT_SOURCE_GPIOC, SCFG_PINS_SOURCE7
//  printf(": %s, of AT32F437\r\n", scfg_info());
//  bannerline_log();
 #endif

  exint_default_para_init(&exint_init_struct);
  exint_init_struct.line_enable = TRUE;

  exint_init_struct.line_mode = EXINT_LINE_INTERRUPUT;
  exint_init_struct.line_select = pexint_set->extline.extline; //line_no;
  exint_init_struct.line_polarity = polarity; //EXINT_TRIGGER_RISING_EDGE/ EXINT_TRIGGER_FALLING_EDGE
  exint_init(&exint_init_struct);
}

//-caller
void exint_mconfig(exint_polarity_config_type polarity)
{
	const struct extscfg_st *pexint_set = (const struct extscfg_st *) exint_scfg_ptr();
	if (pexint_set) {
		exint_config(pexint_set, polarity);
	}
}

//static void config_exint(gpio_pull_type gppull, exint_polarity_config_type polarity)
//{
//  if (intr_gpio_mptr()) {
//	  printf("................................ dm9051 gpio_pin_config(for intr)\r\n");
//	  printf("gpio_pin_config: INTR-gpio\r\n");
//	  gpio_pin_config(intr_gpio_ptr(), gppull);
//  }

//  exint_mconfig(polarity);
//}

void intr_add(void)
{
//  config_exint(GPIO_PULL_UP, EXINT_TRIGGER_FALLING_EDGE);
//  gpio_pull_type gppull = GPIO_PULL_UP;
//  exint_polarity_config_type polarity = EXINT_TRIGGER_FALLING_EDGE;
  struct {
	  gpio_pull_type gppull;
	  exint_polarity_config_type polarity;
  } confi = {
	  GPIO_PULL_UP, EXINT_TRIGGER_FALLING_EDGE,
  };
 
  if (intr_gpio_mptr()) {
	  printf("................................ dm9051 gpio_pin_config(for intr)\r\n");
	  printf("gpio_pin_config: INTR-gpio\r\n");
	  gpio_pin_config(intr_gpio_ptr(), confi.gppull);
  }

  exint_mconfig(confi.polarity);
}

//[Enable int]
static void exint_enable(const struct extscfg_st *pexint_set) {
	
//	printf(": %s :                 exint-enable/ %s\r\n", "config", exint_enable_info()); //pexint_set
	bannerline_log();
	printf("irq_priority: priority = 0x%x !\r\n", pexint_set->extline.priority); //or "NVIC_PRIORITY_GROUP_0"
	printf("irq_enable: %s\r\n", exint_enable_info()); //pexint_set
	bannerline_log();

//NVIC_PRIORITY_GROUP_0/NVIC_PRIORITY_GROUP_4, //nvic_priority_group_type priority
  nvic_priority_group_config(pexint_set->extline.priority);
  nvic_irq_enable(pexint_set->extline.irqn, 1, 0); //nvic_irq_enable(EXINT9_5_IRQn, 1, 0); //i.e.= //_misc
}

void dm9051_board_irq_enable(void)
{
	const struct extscfg_st *pexint_set = (const struct extscfg_st *) exint_scfg_ptr();
	if (pexint_set) {
		exint_enable(pexint_set);
	}
}


/**
  * @brief  exint line0 config. configure pa0 in interrupt mode
  * @param  None
  * @retval None
  */
//void exint_line0_config(void)
//{
//  exint_init_type exint_init_struct;

//  crm_periph_clock_enable(CRM_SCFG_PERIPH_CLOCK, TRUE);
//  crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);

//  scfg_exint_line_config(SCFG_PORT_SOURCE_GPIOA, SCFG_PINS_SOURCE0);

//  exint_default_para_init(&exint_init_struct);
//  exint_init_struct.line_enable = TRUE;
//  
//  exint_init_struct.line_mode = EXINT_LINE_INTERRUPUT;
//  exint_init_struct.line_select = EXINT_LINE_0;
//  exint_init_struct.line_polarity = EXINT_TRIGGER_FALLING_EDGE; //EXINT_TRIGGER_RISING_EDGE;
//  exint_init(&exint_init_struct);

//  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
//  nvic_irq_enable(EXINT0_IRQn, 1, 0);
//}
//void exint_line4_config(void)
//{
//  exint_init_type exint_init_struct;
//	nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);

//  crm_periph_clock_enable(CRM_SCFG_PERIPH_CLOCK, TRUE);

//  exint_default_para_init(&exint_init_struct);
//  exint_init_struct.line_enable = TRUE;
//  exint_init_struct.line_mode = EXINT_LINE_INTERRUPUT;
//  exint_init_struct.line_select = EXINT_LINE_4;
//  exint_init_struct.line_polarity = EXINT_TRIGGER_RISING_EDGE;
//  exint_init(&exint_init_struct);
//  exint_flag_clear(EXINT_LINE_4);
//  nvic_irq_enable(EXINT4_IRQn, 1, 0);
//}

//[spi configuration]
/**
  * @brief  spi configuration.
  * @param  spi_inf_t* =
  *         struct {
  *         	spi_type *num;        			//= SPIPORT;
  *         	crm_periph_clock_type spi_crm_clk;	//= SPI_CRM_CLK;
  *         } spi_inf_t;
  * @retval None
  */
static void spi_config(void)
{
  spi_init_type spi_init_struct;
  crm_periph_clock_enable(spi_crm(), TRUE); //crm_spi_clk(_pinCode), CRM_SPI3_PERIPH_CLOCK/CRM_SPI2_PERIPH_CLOCK

  spi_default_para_init(&spi_init_struct);
  spi_init_struct.transmission_mode = SPI_TRANSMIT_FULL_DUPLEX;
  spi_init_struct.master_slave_mode = SPI_MODE_MASTER;
  spi_init_struct.mclk_freq_division = SPI_MCLK_DIV_8;
  //spi_init_struct.first_bit_transmission = SPI_FIRST_BIT_LSB;
  spi_init_struct.first_bit_transmission = SPI_FIRST_BIT_MSB;
  spi_init_struct.frame_bit_num = SPI_FRAME_8BIT;
  spi_init_struct.clock_polarity = SPI_CLOCK_POLARITY_LOW;
  //spi_init_struct.clock_phase = SPI_CLOCK_PHASE_2EDGE;
  spi_init_struct.clock_phase = SPI_CLOCK_PHASE_1EDGE;
  spi_init_struct.cs_mode_selection = SPI_CS_SOFTWARE_MODE;
  spi_init(spi_number(), &spi_init_struct); //SPI2, _spi_num(_pinCode)
  spi_enable(spi_number(), TRUE); //SPI2, _spi_num(_pinCode)
}

// -

/**
  * @brief  gpio configuration.
  * @brief  spi configuration.
  * @brief  exint configuration.
  */
static void spi_add(void) //=== pins_config(); //total_eth_count++;
{
//.#ifndef AT32F437xx
  //Setting of Non-f437
  if (spi_number() == SPI1) {
	  if  (spi_iomux() & IO_CRM_CLOCK) {

		#ifndef AT32F437xx //.
	    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE); //Non-f437,iomux-config
	    gpio_pin_remap_config(SWJTAG_GMUX_010, TRUE); //Non-f437,iomux-config
		#endif //.
	  }
	  if  (spi_iomux() & IO_MUX_PINREMAP) {

		#ifndef AT32F437xx //.
		  gpio_pin_remap_config(SPI1_MUX_01, TRUE); //Non-f437,remap
		#endif //.
	  }
  }
//.#endif
  bannerline_log();
  printf("gpio_pin_config: SCK-gpio\r\n");
  gpio_pin_config(&gpio_wire_sck(), GPIO_PULL_NONE); //,GPIO_MODE_MUX
  printf("gpio_pin_config: MI-gpio\r\n");
  gpio_pin_config(&gpio_wire_mi(), GPIO_PULL_NONE); //,GPIO_MODE_MUX
  printf("gpio_pin_config: MO-gpio\r\n");
  gpio_pin_config(&gpio_wire_mo(), GPIO_PULL_NONE); //,GPIO_MODE_MUX //GPIO_PULL_UP; //test ffff
  spi_config(); //(spi_port_ptr(_pinCode));
  printf("gpio_pin_config: CS-gpio\r\n");
  gpio_pin_config(&gpio_cs(), GPIO_PULL_NONE); //,GPIO_MODE_OUTPUT
}

void rst_add(void)
{
  if (rst_pin_mexist()) {
	printf("gpio_pin_config: RST-gpio\r\n");
	gpio_pin_config(rst_gpio_ptr(), GPIO_PULL_UP); //=(rst_gpio_ptr(_pinCode), GPIO_PULL_UP); //,GPIO_MODE_OUTPUT
  }
}

void interface_all_add(int pin)
{
	DM_UNUSED_ARG(pin);

	spi_add();
	rst_add();
	intr_add();
//	exint_line0_config();
	
//	exint_line4_config();
}

/*
 * pDevObj[] is
 * waiting to be used!
 */
//const spi_dev_t *pDevObj[BOARD_SPI_COUNT]; //To be used, in case 'pin_code' can be eliminated mass-quantity.

//void DevObj_store(int pin) {
//	pDevObj[pin] = PTR_SPIDEV(pin); //'pin_code'
//}

/*
 * DataObj.devconf[] and DataObj.intrconf[] are
 * waiting to be used!
 */
typedef struct {
	const spi_dev_t *devconf[BOARD_SPI_COUNT];
	const struct modscfg_st *intrconf[BOARD_SPI_COUNT];
} conf_list_t;

conf_list_t DataObj;

//Example:
// = {
//	{
//		&devconf[0], //devconf_at437_spi4("AT32F437", "sck/mi/mo/ pe2/pe5/pe6", "cs/ pe4"),
//		&devconf[1], //devconf_at437_spi2("AT32F437", "sck/mi/mo/ pd1/pc2/pd4", "cs/ pd0"),
//	},
//	{
//		&devconf_at437_intr_c7,
//		&devconf_at437_intr_a0,
//	},
//};
void DataObj_store(int pin) {
	DataObj.devconf[pin] = &devconf[pin]; //PTR_SPIDEV(pin); //'pin_code'
	if (intr_packPT)
	  DataObj.intrconf[pin] = ((const struct modscfg_st **)intr_packPT)[pin]; //Can it in case NULL ok ?
//	  DataObj.intrconf[pin] = &((struct modscfg_st *)intr_packPT)[pin];
}

uint32_t DataObj_EXINT_extline(int pin)
{
	return DataObj.intrconf[pin]->extend->extline.extline;
}

void LIST_EXTLINE(uint32_t exint_line)
{
	int pin;
	if (exint_line)
		printf("Enter exint_line 0x%06x\r\n", exint_line);
	else
		printf("Enter EXINT_LINE_NONE 0x%06x\r\n", exint_line);
		
	for (pin = 0; pin < ETHERNET_COUNT; pin++)
		printf("List%d = 0x%06x\r\n", pin, DataObj.intrconf[pin]->extend->extline.extline);
		
	if (exint_line)
		printf("Exit exint_line 0x%06x NOT found!\r\n", exint_line);
	else
		printf("Exit EXINT_LINE_NONE 0x%06x\r\n", exint_line);
}

int DataObj_EXINT_Pin(uint32_t exint_line)
{
	int pin;
	for (pin = 0; pin < ETHERNET_COUNT; pin++) {
		if (DataObj.intrconf[pin]->extend->extline.extline == exint_line)
			return pin;
	}
	
//	printf("Enter exint_line 0x%06x\r\n", exint_line);
//	for (pin = 0; pin < ETHERNET_COUNT; pin++)
//		printf("List%d = 0x%06x\r\n", pin, DataObj.intrconf[pin]->extend->extline.extline);
//	printf("Exit exint_line 0x%06x NOT found!\r\n", exint_line);
	LIST_EXTLINE(exint_line);

	return 0;
}

void board_conf_configuration(void)
{
  /*for (i = 0; i < n; i++) { //get_eth_interfaces()
	mstep_set_net_index(i);
	interface_all_add(i);
  }*/

//  ETH_COUNT_VOIDFN(DevObj_store);
  ETH_COUNT_VOIDFN(DataObj_store);
  LIST_EXTLINE(EXINT_LINE_NONE);

  ETH_COUNT_VOIDFN(interface_all_add); //voidfn_dual

  cpin_poweron_reset();
  dmf.dly_ms(30);
}

//void ethernet_interfaces_initialize(void)
//{
//  int i;
//  for (i = 0; i < _ETHERNET_COUNT; i++) {
//	mstep_set_net_index(i); //+
//	dm9051_init(_get_eth_mac());
//  }
//}

static void rst_pin_pulse(void) {
	gpio_bits_reset(rst_gpio_ptr()->gpport, rst_gpio_ptr()->pin); //rstpin_lo();
	dm_delay_ms(1);
	gpio_bits_set(rst_gpio_ptr()->gpport, rst_gpio_ptr()->pin); //rstpin_hi();
}

void cpin_poweron_reset(void)
{
	if (rst_pin_exister())
		rst_pin_pulse(); //.dm9051_if->rstb_pulse()
}

/*
 * Include user defined options first. Anything not defined in these files
 * will be set to standard values. Override anything you don't like!
 */
#if 0 //only for Lwip
#include "lwipopts.h"
#endif

#if freeRTOS_CONF && freeRTOS_ENABLE_MUTEX
int cspiSemaphoreDoOwn(int pntlog, char *headstr, SemaphoreHandle_t semaphore_hdlr);
void cspiSemaphoreDoYield(int pntlog, char *headstr, SemaphoreHandle_t semaphore_hdlr);

#define INIT_CSPI_CORE() \
	int rc;
	
#define LOCK_CSPI_CORE(log) \
	if (freeRTOS_ENABLE_MUTEX) \
		rc = cspiSemaphoreDoOwn(log, "empty", NULL);
#define UNLOCK_CSPI_CORE() \
	if (freeRTOS_ENABLE_MUTEX) \
		if (rc) cspiSemaphoreDoYield(0, "empty", NULL);
#else
#define INIT_CSPI_CORE()
#define LOCK_CSPI_CORE(log)
#define UNLOCK_CSPI_CORE()
#endif
