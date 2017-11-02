/*
 * tcp_conn.h
 *
 *  Created on: 2017Äê10ÔÂ21ÈÕ
 *      Author: Administrator
 */

#ifndef APP_INCLUDE_TCP_CONN_H_
#define APP_INCLUDE_TCP_CONN_H_


extern void connect_to_server_init(uint8 buf[],int port);
extern void connect_to_server_dns_init(void);

extern void esp8266_socket_send(uint8 buf[],uint8 len);

//#define DNS_ENABLE
#endif /* APP_INCLUDE_TCP_CONN_H_ */
