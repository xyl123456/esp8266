/*
 * tcp_conn.h
 *
 *  Created on: 2017Äê10ÔÂ21ÈÕ
 *      Author: Administrator
 */

#ifndef APP_INCLUDE_TCP_CONN_H_
#define APP_INCLUDE_TCP_CONN_H_


extern void connect_to_server_init(uint8 buf[],int port);
struct espconn *temp_user_tcp_conn;

#endif /* APP_INCLUDE_TCP_CONN_H_ */
