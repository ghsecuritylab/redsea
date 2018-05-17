/*
 *  Hello World
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h" 

#include "main.h"
#include "main_test.h"
#if CONFIG_WLAN
#include "wifi_conf.h"
#include "wlan_intf.h"
#include "wifi_constants.h"
#endif
#include "lwip_netconf.h"
#include <platform/platform_stdlib.h>

#ifndef CONFIG_INIT_NET
#define CONFIG_INIT_NET             1
#endif
#ifndef CONFIG_INTERACTIVE_MODE
#define CONFIG_INTERACTIVE_MODE     1
#endif

// add by herry
#define STACKSIZE                   512
//#define STACKSIZE                   (512 + 768)

xSemaphoreHandle uart_rx_interrupt_sema = NULL;

void ayla_demo_init(void);
void ayla_wlan_init(void);

void init_thread(void *param)
{
#if CONFIG_INIT_NET
#if CONFIG_LWIP_LAYER
	/* Initilaize the LwIP stack */
	LwIP_Init();
#endif
PRINTF("\n\r[cs] %s %d available heap %d\n\r", __func__, __LINE__, xPortGetFreeHeapSize());
#endif
#if CONFIG_WIFI_IND_USE_THREAD	// 0
	wifi_manager_init();		
#endif
	ayla_wlan_init();

#if CONFIG_WLAN		// 1
	wifi_on(RTW_MODE_STA);
#if CONFIG_AUTO_RECONNECT	// 1
	//setup reconnection flag
	wifi_set_autoreconnect(1);
#endif
	printf("\n\r%s(%d), Available heap %d", __FUNCTION__, __LINE__, xPortGetFreeHeapSize());	
#endif
	
#if CONFIG_INTERACTIVE_MODE
 	/* Initial uart rx swmaphore*/
	vSemaphoreCreateBinary(uart_rx_interrupt_sema);
	xSemaphoreTake(uart_rx_interrupt_sema, 1/portTICK_RATE_MS);
	start_interactive_mode();
#endif	

	PRINTF("\n\r[cs] %s %d available heap %d\n\r", __func__, __LINE__, xPortGetFreeHeapSize());

	ayla_demo_init();
	
	PRINTF("\n\r[cs] %s %d available heap %d\n\r", __func__, __LINE__, xPortGetFreeHeapSize());
	PRINTF("\n\r-------------------------------\n\r\n\r\n\r\n\r");

	/* Kill init thread after all init tasks done */
	vTaskDelete(NULL);
}


void wlan_network()
{
	if(xTaskCreate(init_thread, ((const char*)"init"), STACKSIZE, NULL, tskIDLE_PRIORITY + 3 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);
}