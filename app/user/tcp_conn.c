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
#include "gpio.h"
#include "tcp_conn.h"
#include "state_config.h"


//监测仪6000
//四面出风7000
//净化器7001
//空调7002
//地暖7003

//美丽妈妈  1883

#define NET_DOMAIN "ibms.env365.cn"
//#define NET_DOMAIN "api.env365.cn"
//#define NET_DOMAIN "xylvip.top"
//#define NET_DOMAIN "pub.env365.cn"
//#define NET_DOMAIN "1498kn1392.imwork.net"
#define DNS_PORT   6000

uint8 log_server=0;

uint8 head_line[2]={0xEB,0x90};
uint8 tcp_init_length[2]={0x00,0x14};
uint8 soft_dev[3]={0x20,0x00,0x00};
uint8 hard_dev[3]={0x10,0x00,0x00};
uint8 heat_time[3]={0x00,0x00,0x1E};
uint8 tail_data[2]={0x0D,0x0A};

uint8 send_registdata[13]={0xEB,0x90,0x00,0x0B,0x0F,0x00,0x00,0x00,0x00,0x00,0x1A,0x0D,0x0A};
TCP_INIT_DATA  tcp_init_data;

struct espconn *temp_user_tcp_conn;

struct espconn esp8266_tcp_conn;
struct _esp_tcp user_tcp;
os_timer_t tcp_reconn_timer;
os_timer_t dns_timer;//重新连接DNS时间



ip_addr_t tcp_server_dns_ip;

uint16_t ICACHE_FLASH_ATTR
SumCheck(unsigned char *pData, unsigned char pLength){
	uint16_t result = 0;
	unsigned char i = 0;
	for(i = 0; i < pLength; i++)
	{
		result = result + pData[i];
	}

	return result;

}


void ICACHE_FLASH_ATTR
esp8266_socket_send(uint8 buf[],uint8 len){
	uint8_t wifi_reconn_state;
	wifi_reconn_state=wifi_station_get_connect_status();
	if(wifi_reconn_state==STATION_GOT_IP){
    	if(temp_user_tcp_conn->proto.tcp->remote_ip!=0)
    	{
    		espconn_sent(temp_user_tcp_conn, buf, len);
    	}
	}
}

void ICACHE_FLASH_ATTR
esp8266_reconn_cb(void){
	uint8_t reconn_server_ip[6];
	uint8_t esp8266_reconn_ip[4];
	uint16 esp8266_reconn_port;
	os_timer_disarm(&tcp_reconn_timer);

	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);

	uint8_t wifi_reconn_state;
	wifi_reconn_state=wifi_station_get_connect_status();
    if (wifi_reconn_state== STATION_GOT_IP)
	{
    	//uart0_sendStr("connect to tcp server\r\n");
    	//GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 0);//WIFI模块配置
    	spi_flash_read(0x3C * 4096, (uint32 *)reconn_server_ip, 6);
    	os_memcpy(esp8266_reconn_ip,reconn_server_ip,4);
    	esp8266_reconn_port=reconn_server_ip[4]*256 + reconn_server_ip[5];

    	connect_to_server_init(esp8266_reconn_ip,esp8266_reconn_port);

    }
	else
	{
		//uart0_sendStr("reconnect to tcp server\r\n");
		//os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)esp8266_reconn_cb, NULL);
		//os_timer_arm(&tcp_reconn_timer, 10000, 0);

    }
}
//TCP连接中DNS重新配置连接
void ICACHE_FLASH_ATTR
esp8266_reconn_dns_cb(void){
	struct station_config *re_dsn;
	os_timer_disarm(&tcp_reconn_timer);
	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);

	uint8_t wifi_reconn_state;
		wifi_reconn_state=wifi_station_get_connect_status();
	    if (wifi_reconn_state== STATION_GOT_IP)
		{
	    	//重新分配解析DNS
	    	connect_to_server_dns_init();
	    	/*esp8266_tcp_conn.proto.tcp=&user_tcp;
	    	esp8266_tcp_conn.type=ESPCONN_TCP;
	    	esp8266_tcp_conn.state=ESPCONN_NONE;
	    	//connect to tcp server as NET_DOMAIN
	    	tcp_server_dns_ip.addr = 0;
	    	espconn_gethostbyname(&esp8266_tcp_conn,NET_DOMAIN,&tcp_server_dns_ip,user_dns_found);
	    	*/
		}else{
			//uart0_sendStr("reconnect to tcp server\r\n");
			//os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)esp8266_reconn_dns_cb, NULL);
			//os_timer_arm(&tcp_reconn_timer, 60000, 0);
		}
}
//路由断开后，重新连接
void ICACHE_FLASH_ATTR
esp8266_tcp_recon_cb(void *arg, sint8 err){

	os_timer_disarm(&tcp_reconn_timer);
#ifdef DEBUG_MODE
	uart0_tx_buffer("tcp_reeconnect\n", 15);
#endif
	espconn_disconnect(arg);

#ifdef DNS_ENABLE
	os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)esp8266_reconn_dns_cb, NULL);
#else
	os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)esp8266_reconn_cb, NULL);
#endif
	os_timer_arm(&tcp_reconn_timer, 10000, 0);
}
/******************************************************************************
 * FunctionName : user_tcp_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
esp8266_tcp_discon_cb(void *arg){

	os_timer_disarm(&tcp_reconn_timer);
#ifdef DEBUG_MODE
	uart0_tx_buffer("tcp_disconnect\n", 15);
#endif

#ifdef DNS_ENABLE
	os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)esp8266_reconn_dns_cb, NULL);
#else
	os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)esp8266_reconn_cb, NULL);
#endif
	os_timer_arm(&tcp_reconn_timer, 10000, 0);
}

void ICACHE_FLASH_ATTR
esp8266_tcp_sent_cb(void *arg){
   //data sent successfully

}
void ICACHE_FLASH_ATTR
process_command(char *pusrdata, unsigned short length)
{
	char re_buf[100];
	os_memcpy(re_buf,pusrdata,length);
	if(re_buf[4]==0x01){
		log_server=1;
	}else{
		uart0_tx_buffer(pusrdata,length);
	}
	os_memset(re_buf,0,100);
}
void ICACHE_FLASH_ATTR
esp8266_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length){

	process_command(pusrdata, length);
	os_memset(pusrdata,0,length);
	length=0;
}

void ICACHE_FLASH_ATTR
espconn_init_sent(struct espconn *espconn)
{
	uint16_t sum_data;
	uint8 buf[2];
	spi_flash_read(0x3D * 4096, (uint32 *)dev_id, 4);  //读取设备ID

	os_memcpy(tcp_init_data.data_core.head,head_line,2);
	os_memcpy(tcp_init_data.data_core.length,tcp_init_length,2);
	tcp_init_data.data_core.data_type=0x02;
	os_memcpy(tcp_init_data.data_core.dev_id,dev_id,4);
	os_memcpy(tcp_init_data.data_core.sof_dev,soft_dev,3);
	os_memcpy(tcp_init_data.data_core.hard_dev,hard_dev,3);
	os_memcpy(tcp_init_data.data_core.heat_time,heat_time,3);
	//tcp_init_data.data_core.rssi=wifi_rssi;

	sum_data=SumCheck(tcp_init_data.data_buf+2,16);
	buf[0]=(sum_data>>8)&0xFF;
	buf[1]=sum_data&0xFF;
	os_memcpy(tcp_init_data.data_core.crc_data,buf,2);
	os_memcpy(tcp_init_data.data_core.tail_data,tail_data,2);
	espconn_sent(espconn,tcp_init_data.data_buf,22);
}
void ICACHE_FLASH_ATTR
esp8266_tcp_connect_cb(void *arg){

    struct espconn *pespconn = arg;
	temp_user_tcp_conn = arg;//串口发送的TCP连接
#ifdef DEBUG_MODE
	uart0_tx_buffer("tcp_success\n", 12);
#endif
	//espconn_set_opt(pespconn,0x01);

    espconn_regist_recvcb(pespconn, esp8266_tcp_recv_cb);
    espconn_regist_sentcb(pespconn, esp8266_tcp_sent_cb);
    espconn_regist_disconcb(pespconn, esp8266_tcp_discon_cb);

    uart0_tx_buffer(send_registdata, 13); //发送注册信息

    //espconn_init_sent(pespconn);//tcp_init,发送注册信息
}
void ICACHE_FLASH_ATTR
user_dns_found(const char *name,ip_addr_t *ipaddr,void *arg){
	struct espconn *pespconn = (struct espconn *)arg;
	if(ipaddr == NULL){
		return;
	}
	if(tcp_server_dns_ip.addr == 0 && ipaddr->addr !=0){
		//停止周期检测dns  timer
#ifdef DEBUG_MODE
		uart0_tx_buffer("dns_success\n", 12);
		uart0_tx_buffer(pespconn->proto.tcp->remote_ip, 4);
#endif
		//os_timer_disarm(&dns_timer);
		tcp_server_dns_ip.addr=ipaddr->addr;
		os_memcpy(pespconn->proto.tcp->remote_ip,&ipaddr->addr,4);
		pespconn->proto.tcp->remote_port = DNS_PORT;
		pespconn->proto.tcp->local_port = espconn_port();
        //注册TCP回调函数，连接成功后回调
		espconn_regist_connectcb(pespconn,esp8266_tcp_connect_cb);//register connect callback
		//网络断开后
		espconn_regist_reconcb(pespconn,esp8266_tcp_recon_cb);//register reconnect callback as error handler
		espconn_connect(pespconn);
	}

}

void ICACHE_FLASH_ATTR
connect_to_server_init(uint8 buf[],int port){

	esp8266_tcp_conn.proto.tcp=&user_tcp;
	esp8266_tcp_conn.type=ESPCONN_TCP;
	esp8266_tcp_conn.state=ESPCONN_NONE;
//设置ip
	os_memcpy(esp8266_tcp_conn.proto.tcp->remote_ip,buf,4);

	esp8266_tcp_conn.proto.tcp->remote_port=port;
	esp8266_tcp_conn.proto.tcp->local_port = espconn_port(); //local port of ESP8266

    espconn_regist_connectcb(&esp8266_tcp_conn, esp8266_tcp_connect_cb); // register connect callback
    espconn_regist_reconcb(&esp8266_tcp_conn, esp8266_tcp_recon_cb);     // register reconnect callback as error handler

    espconn_connect(&esp8266_tcp_conn);

}


void ICACHE_FLASH_ATTR
tcp_delete_conn(void)
{
#ifdef DEBUG_MODE
	uart0_tx_buffer("delete_tcpconn\n", 15);
#endif
	espconn_set_opt(&esp8266_tcp_conn,0);
}
void ICACHE_FLASH_ATTR
user_dns_check_cb(void *arg)
{
	struct espconn *pespconn = arg;
#ifdef DEBUG_MODE
	uart0_tx_buffer("dns_reconnect\n", 14);
#endif
	espconn_gethostbyname(pespconn,NET_DOMAIN,&tcp_server_dns_ip,user_dns_found);
	os_timer_arm(&dns_timer, 5000, 0);
}

void ICACHE_FLASH_ATTR
connect_to_server_dns_init(void){
	esp8266_tcp_conn.proto.tcp=&user_tcp;
	esp8266_tcp_conn.type=ESPCONN_TCP;
	esp8266_tcp_conn.state=ESPCONN_NONE;
	//connect to tcp server as NET_DOMAIN
#ifdef DEBUG_MODE
	uart0_tx_buffer("dns_init\n", 9);
#endif
	tcp_server_dns_ip.addr = 0;
	espconn_gethostbyname(&esp8266_tcp_conn,NET_DOMAIN,&tcp_server_dns_ip,user_dns_found);
}


