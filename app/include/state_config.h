/*
 * state_config.h
 *
 *  Created on: 2017��10��21��
 *      Author: Administrator
 */
#include "c_types.h"
#ifndef APP_INCLUDE_STATE_CONFIG_H_
#define APP_INCLUDE_STATE_CONFIG_H_

extern void state_check(void);
extern void tcp_start_conn(void);

extern void tcp_re_start_conn(void);

extern void udpdata_connect(void);

extern uint8 wifi_rssi;

uint8 set_server_ip[6];

uint8 timer_cnt;

uint8 dev_id[4];



#endif /* APP_INCLUDE_STATE_CONFIG_H_ */
