/*
 * tcp_conn.c
 *
 *  Created on: 2017年10月21日
 *      Author: Administrator
 */
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "spi_flash.h"
#include "espconn.h"
#include "user_config.h"
#include "smartconfig.h"
#include "mem.h"
#include "user_interface.h"
#include "gpio.h"
#include "tcp_conn.h"

struct espconn esp8266_tcp_conn;
struct _esp_tcp user_tcp;
os_timer_t tcp_reconn_timer;

//struct espconn *temp_user_tcp_conn;//用于存储连接设备FD，串口透传用

void ICACHE_FLASH_ATTR
esp8266_reconn_cb(void){

	uint8_t reconn_server_ip[6];
	uint8_t esp8266_reconn_ip[4];
	uint16 esp8266_reconn_port;
	os_timer_disarm(&tcp_reconn_timer);
	//get ip info of ESP8266 station
	//wifi_get_ip_info(STATION_IF, &ipconfig);
	uint8_t wifi_reconn_state;
	wifi_reconn_state=wifi_station_get_connect_status();
    if (wifi_reconn_state== STATION_GOT_IP)
	{
    	GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 0);//WIFI模块配置
    	spi_flash_read(0x3C * 4096, (uint32 *)reconn_server_ip, 6);
    	os_memcpy(esp8266_reconn_ip,reconn_server_ip,4);
    	esp8266_reconn_port=reconn_server_ip[4]*256 + reconn_server_ip[5];

    	connect_to_server_init(esp8266_reconn_ip,esp8266_reconn_port);

#ifdef DNS_ENABLE
       tcp_server_ip.addr = 0;
       espconn_gethostbyname(&user_tcp_conn, NET_DOMAIN, &tcp_server_ip, user_dns_found); // DNS function

       os_timer_setfn(&test_timer, (os_timer_func_t *)user_dns_check_cb, &user_tcp_conn);
       os_timer_arm(&test_timer, 1000, 0);
#endif
    }
	else
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 1);//WIFI模块未配置
		os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)esp8266_reconn_cb, NULL);
		os_timer_arm(&tcp_reconn_timer, 10000, 0);

    }
}

//TCP 重新连接
LOCAL void ICACHE_FLASH_ATTR
esp8266_tcp_recon_cb(void *arg, sint8 err){

	os_timer_disarm(&tcp_reconn_timer);
	os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)esp8266_reconn_cb, NULL);
	os_timer_arm(&tcp_reconn_timer, 10000, 0);
}
/******************************************************************************
 * FunctionName : user_tcp_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
esp8266_tcp_discon_cb(void *arg){


	os_timer_disarm(&tcp_reconn_timer);
	os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)esp8266_reconn_cb, NULL);
	os_timer_arm(&tcp_reconn_timer, 10000, 0);
}

LOCAL void ICACHE_FLASH_ATTR
esp8266_tcp_sent_cb(void *arg){
   //data sent successfully

}

LOCAL void ICACHE_FLASH_ATTR
esp8266_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length){

	uart0_tx_buffer(pusrdata,  length);
	//process_command(pusrdata, length);
}


LOCAL void ICACHE_FLASH_ATTR
esp8266_tcp_connect_cb(void *arg){

    struct espconn *pespconn = arg;
    espconn_regist_recvcb(pespconn, esp8266_tcp_recv_cb);
    espconn_regist_sentcb(pespconn, esp8266_tcp_sent_cb);
    espconn_regist_disconcb(pespconn, esp8266_tcp_discon_cb);

	temp_user_tcp_conn = arg;

}

void ICACHE_FLASH_ATTR
connect_to_server_init(uint8 buf[],int port){

	esp8266_tcp_conn.proto.tcp=&user_tcp;
	esp8266_tcp_conn.type=ESPCONN_TCP;
	esp8266_tcp_conn.state=ESPCONN_NONE;

	os_memcpy(esp8266_tcp_conn.proto.tcp->remote_ip,buf,4);

	esp8266_tcp_conn.proto.tcp->remote_port=port;
	esp8266_tcp_conn.proto.tcp->local_port = espconn_port(); //local port of ESP8266

    espconn_regist_connectcb(&esp8266_tcp_conn, esp8266_tcp_connect_cb); // register connect callback
    espconn_regist_reconcb(&esp8266_tcp_conn, esp8266_tcp_recon_cb);     // register reconnect callback as error handler

    espconn_connect(&esp8266_tcp_conn);

}


