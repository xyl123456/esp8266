/*
 * tcp_conn.h
 *
 *  Created on: 2017年10月21日
 *      Author: Administrator
 */

#ifndef APP_INCLUDE_TCP_CONN_H_
#define APP_INCLUDE_TCP_CONN_H_
void user_dns_found(const char *name,ip_addr_t *ipaddr,void *arg);
extern void connect_to_server_init(uint8 buf[],int port);
extern void connect_to_server_dns_init(void);

extern void esp8266_socket_send(uint8 buf[],uint8 len);
void process_command(char *pusrdata, unsigned short length);

extern uint8 log_server;
extern void tcp_delete_conn(void);

uint16_t SumCheck(unsigned char *pData, unsigned char pLength);
void espconn_init_sent(struct espconn *espconn);

#define DNS_ENABLE  1

//#define UDP_ENABLE  1
//#define DEBUG_MODE  1

typedef union tcp_init
{
	uint8_t data_buf[22];
	struct tcp_data_t
	{
		uint8_t head[2];//EB 90
		uint8_t length[2];//00 14
		uint8_t data_type;//02
		uint8_t dev_id[4];
		uint8_t sof_dev[3];//02 00 00
		uint8_t hard_dev[3];//01 00 00
		uint8_t heat_time[3];//00 00 1E
		//uint8_t rssi;//信号强度
		uint8_t crc_data[2];
		uint8_t tail_data[2];//OD OA
	}data_core;
}TCP_INIT_DATA;

#endif /* APP_INCLUDE_TCP_CONN_H_ */
