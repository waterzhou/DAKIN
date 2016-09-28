/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "user_config.h"
#include "esp_ota.h"

#if USER_UART_CTRL_DEV_EN
#include "user_uart.h" // user uart handler head
#endif
#if USER_PWM_LIGHT_EN
#include "user_light.h"  // user pwm light head
#endif

#if USER_SPI_CTRL_DEV_EN
#include "driver/spi.h"
#endif

LOCAL uint8 device_status;



int ICACHE_FLASH_ATTR readSmartConfigFlag() {
	int res = 0;
	uint32  value = 0;
	res = spi_flash_read(LFILE_START_ADDR+LFILE_SIZE, &value, 4);
	if (res != SPI_FLASH_RESULT_OK) {
		os_printf("read flash data error\n");
		return -1;
	}
	os_printf("read data:0x%2x \n",value);
	return (int)value;
}
// flash写接口需要先擦后写,这里擦了4KB,会导致整个4KB flash存储空间的浪费,可以将这个变量合并保存到用户数据中.
int ICACHE_FLASH_ATTR setSmartConfigFlag(int value) 
{
	int res = 0;	
	uint32 data = (uint32) value;
	spi_flash_erase_sector((LFILE_START_ADDR+LFILE_SIZE)/4096);
	res = spi_flash_write((LFILE_START_ADDR+LFILE_SIZE), &data, 4);
	if (res != SPI_FLASH_RESULT_OK) {
		os_printf("write data error\n");
		return -1;
	}
	os_printf("write flag(%d) success.", value);
	return 0;
}

unsigned portBASE_TYPE ICACHE_FLASH_ATTR stack_free_size() 
{
	xTaskHandle taskhandle;
	unsigned portBASE_TYPE stack_freesize = 0;
	taskhandle = xTaskGetCurrentTaskHandle();
	stack_freesize = uxTaskGetStackHighWaterMark(taskhandle);
	os_printf("stack free size =%u\n", stack_freesize);
	return stack_freesize;
}

void ICACHE_FLASH_ATTR startdemo_task(void *pvParameters) 
{
	os_printf("heap_size %d\n", system_get_free_heap_size());
	stack_free_size();
	while (1) {
		int ret = wifi_station_get_connect_status();   // wait for sys wifi connected OK.
		if (ret == STATION_GOT_IP)
			break;
		vTaskDelay(100 / portTICK_RATE_MS);	 // 100 ms
	}
	//while(1)
	{
		vTaskDelay(100 / portTICK_RATE_MS);
	}
	udpClient();
	TcpLocalServer();
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : smartconfig_done
 * Description  : callback function which be called during the samrtconfig process
 * Parameters   : status -- the samrtconfig status
 *                pdata --
 * Returns      : none
*******************************************************************************/
void  
smartconfig_done(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            printf("SC_STATUS_GETTING_SSID_PSWD\n");          
            sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;
            wifi_station_set_config(sta_conf);
            wifi_station_disconnect();
            wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};
                memcpy(phone_ip, (uint8*)pdata, 4);
                printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            }
            smartconfig_stop();
            device_status = STATION_GOT_IP;
            break;
    }
}

int need_notify_app = 0;
void ICACHE_FLASH_ATTR wificonnect_task(void *pvParameters) 
{
	printf("smartconfig_task start\n");
    smartconfig_start(smartconfig_done);

	//zconfig_start(vendor_callback, vendor_tpsk);   // alink smart config start
	printf("waiting network ready...\r\n");
	while (1) {
		int ret = wifi_station_get_connect_status();
		if (ret == STATION_GOT_IP)
			break;
		vTaskDelay(100 / portTICK_RATE_MS);
	}
	printf("network is ready...\r\n");
	vTaskDelay(100);
	serial_resp_out(CMD_WIFI_MODULE_READY,CMD_SUCCESS);
	need_notify_app = 1;
	vTaskDelete(NULL);
}

static void ICACHE_FLASH_ATTR wifi_event_hand_function(System_Event_t * event) 
{
	os_printf("\n****wifi_event_hand_function %d******\n", event->event_id);
	switch (event->event_id) {
	case EVENT_STAMODE_CONNECTED:
		break;
	case EVENT_STAMODE_DISCONNECTED:
		os_printf(" \n EVENT_STAMODE_DISCONNECTED \n");
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		break;
	case EVENT_STAMODE_GOT_IP:
		os_printf(" \n EVENT_STAMODE_GOT_IP \n 0x%x\n", event->event_info.got_ip.ip.addr);
		
		    //alink_demo(); 
		    break;
	case EVENT_STAMODE_DHCP_TIMEOUT:
		break;
	default:
		break;
	}
}

void ICACHE_FLASH_ATTR wificonnect_test_conn_ap(void) 
{
	signed char ret;
	ESP_DBG(("set a test conn router."));
    wifi_set_opmode(STATION_MODE);
    struct ip_info sta_ipconfig;
    struct station_config config;
    bzero(&config, sizeof(struct station_config));
    sprintf(config.ssid, "IOT_DEMO_TEST");
    sprintf(config.password, "123456789");
    wifi_station_set_config(&config);
    wifi_station_connect();
    wifi_get_ip_info(STATION_IF, &sta_ipconfig);
	return;
}

static void ICACHE_FLASH_ATTR sys_show_rst_info(void)
{
	struct rst_info *rtc_info = system_get_rst_info();	
	os_printf("reset reason: %x\n", rtc_info->reason);	
	if (rtc_info->reason == REASON_WDT_RST ||		
		rtc_info->reason == REASON_EXCEPTION_RST ||		
		rtc_info->reason == REASON_SOFT_WDT_RST) 
	{		
		if (rtc_info->reason == REASON_EXCEPTION_RST) 
		{			
			os_printf("Fatal exception (%d):\n", rtc_info->exccause);		
		}		
		os_printf("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);	
	}

	return;
}

void ICACHE_FLASH_ATTR user_demo(void) 
{
	unsigned int ret = 0;
#if USER_UART_CTRL_DEV_EN
	user_uart_dev_start();
#endif
	printf("SDK version:%s\n", system_get_sdk_version());
	printf("heap_size %d\n", system_get_free_heap_size());

	wifi_set_opmode(STATION_MODE);
	ret = readSmartConfigFlag();// -1 read flash fail!
	printf(" read flag:%d \n", ret);
	if (ret > 0) {		
		setSmartConfigFlag(0);// clear smart config flag
		xTaskCreate(wificonnect_task, "wificonnect_task", 256, NULL, 2, NULL);
		need_notify_app = 1;
	}
	xTaskCreate(startdemo_task, "startdemo_task",(256*4), NULL, 2, NULL);
	

#if USER_SPI_CTRL_DEV_EN
	spi_init(HSPI);
#endif
	return;
}


