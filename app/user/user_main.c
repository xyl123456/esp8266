#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "spi_flash.h"
#include "espconn.h"
#include "user_config.h"
#include "smartconfig.h"
#include "user_interface.h"
#include "gpio.h"
#include "mem.h"
#include "state_config.h"
#include "tcp_conn.h"

static struct espconn *pTcpClient;
extern uint8 set_server_ip[6];

os_timer_t timeraa;//wifi检测状态定时器
os_timer_t tcp_conn_timer;//tcp连接定时器


uint8_t g_Current_Remote_Ip[4] = {0};  //用来存放现在连接过来的远端IP
int g_Current_Remote_Port;

struct espconn *ICACHE_FLASH_ATTR
get_trans_conn(void){
	return pTcpClient;
}
void user_rf_pre_init(void){

}

void ICACHE_FLASH_ATTR tcp_start_conn(void){

	struct ip_info ipconfig;
	int esp8266_server_port;
	uint8 esp8266_server_ip[4];
	int i;
	wifi_get_ip_info(STATION_IF, &ipconfig);
	spi_flash_read(0x3C * 4096, (uint32 *)set_server_ip, 6);  //读取server IP port
	os_timer_disarm(&tcp_conn_timer);
	uint8 ap_state = wifi_station_get_connect_status();//获取接口AP的状态

	esp8266_server_port=set_server_ip[4]*256 + set_server_ip[5];
	os_memcpy(esp8266_server_ip,set_server_ip,4);

	if((ap_state==STATION_GOT_IP) && (ipconfig.ip.addr != 0))
	{
#ifdef DNS_ENABLE
		connect_to_server_dns_init();
#else
	   connect_to_server_init(esp8266_server_ip,esp8266_server_port);
#endif
	}else
	{
		os_timer_setfn(&tcp_conn_timer, (os_timer_func_t *)tcp_start_conn, NULL);
		os_timer_arm(&tcp_conn_timer, 10000, 0);
	}
}

void ICACHE_FLASH_ATTR to_scan(void) {

	os_timer_disarm(&timeraa);//关闭定时器，相当于清零计时器计数
	os_timer_setfn(&timeraa, (os_timer_func_t *)state_check, NULL);//初始化定时器
	os_timer_arm(&timeraa, 2000, 1);//开始定时器计数,2000毫秒后，会调用前面的callback函数 （后面的0表示只运行一次 为1表示循环运行）

	//TCP初始化任务
	os_timer_disarm(&tcp_conn_timer);
	os_timer_setfn(&tcp_conn_timer, (os_timer_func_t *)tcp_start_conn, NULL);
	os_timer_arm(&tcp_conn_timer, 10000, 0);
}

void ICACHE_FLASH_ATTR
gpio_init(void){
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);//设置为复位的按键  主要用于重新配置wifi
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);  //第一个灯
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);  //第二个灯
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);  //第四个灯
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);  //第三个灯
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);  //第五个灯
	GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 1);//WIFI模块未配置
}
void user_init(void)
{
	gpio_init();
	system_uart_de_swap();
	uart_init(BIT_RATE_9600, BIT_RATE_9600);

	spi_flash_read(0x3C * 4096, (uint32 *)set_server_ip, 6);  //读取server IP port

	wifi_set_opmode(STATION_MODE);//设置工作模式为 station 模式

	wifi_station_set_auto_connect(1);//设置上电自动连接已经记录的AP信息

	wifi_station_set_reconnect_policy(true);//AP断开重连

	system_init_done_cb(to_scan);
}
