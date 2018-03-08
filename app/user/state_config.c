/*
 * state_config.c
 *
 *  Created on: 2017��10��21��
 *      Author: Administrator
 */
#include "state_config.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "smartconfig.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "spi_flash.h"
#include "espconn.h"
#include "user_config.h"
#include "mem.h"
#include "gpio.h"
#include "string.h"
#include "tcp_conn.h"


extern uint8 set_server_ip[6];

uint8 wifi_rssi;

#define configtime 				60   //30*2
//����״̬
#define NO_CONFIG 				0
#define IN_CONFIG 				1
#define COMPLETE_CONFIG 		2

int current_set_Port;
uint8 esp8266_state;//ESP8266 ����״̬
uint8 config_state;//smartconfig ����״̬
uint8 config_time;   //����ʱ��

int8_t ipconfig_cnt=0;
int8_t portconfig_cnt=0;


struct espconn phone_udp_conn;
esp_udp phone_udp;
void ICACHE_FLASH_ATTR udp_process_command(char *pdata, unsigned short len);

int udp_Current_Remote_Port;//UDP���ӵ�Զ�̶˿�
uint8_t udp_Current_Remote_Ip[4] = {0};  //��������������ӹ�����Զ��IP

void ICACHE_FLASH_ATTR
udp_process_command(char *pdata, unsigned short len){
		char data_buf[64];
		int i;
		uint8_t ret=0;
		uint8_t index = 0;
		uint32 ret_port=0;
		os_memcpy(data_buf,pdata,len);
		if(!strncmp(data_buf,"SERVER IP:",strlen("SERVER IP:"))){
			ipconfig_cnt=1;
			for(i = 0; data_buf[i+10] != ':' && i < 15; i++){
				if(data_buf[i+10] != '.'){
				ret = ret*10 + data_buf[i+10] - '0';
				set_server_ip[index] = ret;
				}else{
					ret = 0;
					index++;
					}
				}
			memset(data_buf,0,sizeof(data_buf));
		}else if(!strncmp(data_buf,"SERVER PORT:",strlen("SERVER PORT:"))){
			portconfig_cnt=1;
			for(i = 0; data_buf[i+12] != ':' && i < 5; i++)
				{
				ret_port = ret_port*10 + data_buf[i+12] - '0';
				}

			current_set_Port=ret_port;
			set_server_ip[4] = current_set_Port/256;
			set_server_ip[5] = current_set_Port%256;
			memset(data_buf,0,sizeof(data_buf));
		}
		if((ipconfig_cnt==1) && (portconfig_cnt==1)){
			ipconfig_cnt=0;
			portconfig_cnt=0;
			spi_flash_erase_sector(0x3C);
			spi_flash_write(0x3C * 4096, (uint32 *)set_server_ip, 6);
			system_restart();
		}
}
void ICACHE_FLASH_ATTR
udpdata_recv(void *arg, char *pdata, unsigned short len){
	struct espconn *pespconnte = (struct espconn *)arg;

	//��ȡԶ�����ӵ�UDP�Ķ˿ں�IP��ַ,Ҳ���������ֻ��Ķ˿ں�IP��ַ
	udp_Current_Remote_Port =  pespconnte->proto.udp->remote_port;
	os_memcpy(udp_Current_Remote_Ip,pespconnte->proto.udp->remote_ip,4);

	udp_process_command(pdata,len);
}

void ICACHE_FLASH_ATTR
udpdata_connect(void){
	uint8 udp_ip_remote[4]={255,255,255,255};
	phone_udp_conn.type = ESPCONN_UDP;
	phone_udp_conn.proto.udp = &phone_udp;
	phone_udp_conn.proto.udp->local_port = 2002;//���ض˿�
	phone_udp_conn.proto.udp->remote_port = 2002;//Զ�̶˿�

	//os_memcpy(phone_udp_conn.proto.udp->local_ip,udp_ip_local,4);
	os_memcpy(phone_udp_conn.proto.udp->remote_ip,udp_ip_remote,4);
	//���ڽ��ն˿�IP��ַ������Ϣ
	espconn_regist_recvcb(&phone_udp_conn, udpdata_recv); // ע��һ��UDP���ݰ����ջص�
	espconn_create(&phone_udp_conn);//���� UDP ����
}
void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata){
    switch(status) {
        case SC_STATUS_WAIT:
            break;
        case SC_STATUS_FIND_CHANNEL:

            break;
        case SC_STATUS_GETTING_SSID_PSWD:
        {
        	sc_type *type = pdata;
			config_time = configtime;
            if (*type == SC_TYPE_ESPTOUCH)
            {
				//uart0_sendStr("SC_TYPE:SC_TYPE_ESPTOUCH\n");
			}
            else
            {
				//uart0_sendStr("SC_TYPE:SC_TYPE_AIRKISS\n");
			}
        }
            break;
        case SC_STATUS_LINK:
        {
        	struct station_config *sta_conf = pdata;
	        	wifi_station_set_config(sta_conf);
	        	wifi_station_disconnect();
	        	wifi_station_connect();
        }
            break;
        case SC_STATUS_LINK_OVER:
            //uart0_sendStr("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL)
            {
                uint8 phone_ip[4] = {0};
                os_memcpy(phone_ip, (uint8*)pdata, 4);
                //os_printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
			}
            smartconfig_stop();
            break;
    }
}
void ICACHE_FLASH_ATTR state_check(void){
	  uint8_t wifi_conn_state;
	  wifi_conn_state = wifi_station_get_connect_status();//��ȡ�ӿ�AP��״̬

	  //��������wifi-smartconfig����״̬
	  if(!GPIO_INPUT_GET(GPIO_ID_PIN(0)))
	  {
		  esp8266_state = 2;
		  config_state = NO_CONFIG;
		  config_time = configtime;
	  }


	  if(esp8266_state==2 || esp8266_state==3)
	  {
		  switch(config_state)//�ж�һ������״̬
		  {
		  	  case NO_CONFIG://δ���ù�
		  		  smartconfig_start(smartconfig_done);//����һ�����Ӽ���
				  //GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 1);//GPIO14����ߵ�ƽ
		  		  config_state=IN_CONFIG;
		  		  break;
		  	  case IN_CONFIG://��������,60S����ʱ��
		  		  config_time--;
		  		  if(config_time<=0)
		  		  {
		  			  smartconfig_stop();//ֹͣ����
		  			  wifi_station_connect();//�����Ѿ����ù���AP
		  			  config_state=COMPLETE_CONFIG;//�������
		  		  }
		  		  break;
		  	  case COMPLETE_CONFIG://�����ù���ֱ�ӽ����ж�
		  		config_state=COMPLETE_CONFIG;

		  		  break;
		  	  default:
		  		  break;
		  }

	  }
	  switch(wifi_conn_state)
	  {
	  	  case 0:
	  		esp8266_state=2;
	  		break;
	  	  case STATION_CONNECTING:
	  		  break;
	  	  case STATION_WRONG_PASSWORD:
	  		  if(esp8266_state!=2){
	  			  esp8266_state=2;
	  			  smartconfig_stop();//ֹͣ����
	  		  }
	  		esp8266_state=2;
	  		config_state = NO_CONFIG;
	  		  break;
	  	  case STATION_NO_AP_FOUND:
	  		  if(esp8266_state!=3){
	  			esp8266_state=3;
	  		  }
	  		GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 1);//WIFIģ��δ����
	  		esp8266_state=3;
	  		config_state = NO_CONFIG;
	  		  break;
	  	  case STATION_CONNECT_FAIL:
	  		  if(esp8266_state!=4){
	  			esp8266_state=4;
	  		  }
		  	GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 1);//WIFIģ��δ����
	  		esp8266_state=4;
	  		  break;
	  	  case STATION_GOT_IP:
	  	  {
	  		  if(esp8266_state!=5){
				#ifdef DNS_ENABLE
				#else
		  			udpdata_connect();
				#endif
	  			esp8266_state=5;
	  		  }
	  		GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 0);//WIFIģ���ȡ��IP�������ӳɹ�
	  		wifi_rssi=(wifi_station_get_rssi())&0x7F;
	  		esp8266_state=5;
	  	  }
	  		  break;
	  	  default:
	  		  break;
	  }
}

