/*************************************************************************
    > File Name: tcpserver.c
    > Author: Water Zhou
    > Mail: jianzhengzhou@yandex.com 
    > Created Time: 2016年09月22日 星期四 11时16分02秒
 ************************************************************************/
 
#include "esp_common.h"
#include "espconn.h"


#define TCP_SERVER_LOCAL_PORT (8899)
#define TCP_SERVER_KEEP_ALIVE_ENABLE (1)
#define TCP_SERVER_KEEP_ALIVE_IDLE_S (100)
#define TCP_SERVER_RETRY_INTVL_S (5)
#define TCP_SERVER_RETRY_CNT     (3)
#define TCP_SERVER_GREETING "Hello!This is a tcp server test\n"
#define TCP_SERVER_MUTI_SEND (0)


static struct espconn esp_conn;
os_timer_t camera_timer_t;
static  esp_tcp tcp;
uint8 client_tcp_connect = 0;
uint8 g_camera_index = 0;
struct espconn* tcp_server_local=NULL;
extern bool b_needresp;
int TcpServerSend2(uint8 *data, uint16 datalen)
{
	int ret = -1;
	struct espconn *pesp_conn = &esp_conn;
	remot_info *premot = NULL;
	uint8 count = 0;
	sint8 value = ESPCONN_OK;
	if (client_tcp_connect == 1) {
		if (espconn_get_connection_info(pesp_conn,&premot,0) == ESPCONN_OK){
			for (count = 0; count < pesp_conn->link_cnt; count ++){
				pesp_conn->proto.tcp->remote_port = premot[count].remote_port;
				pesp_conn->proto.tcp->remote_ip[0] = premot[count].remote_ip[0];
				pesp_conn->proto.tcp->remote_ip[1] = premot[count].remote_ip[1];
				pesp_conn->proto.tcp->remote_ip[2] = premot[count].remote_ip[2];
				pesp_conn->proto.tcp->remote_ip[3] = premot[count].remote_ip[3];
				vTaskDelay(1000/portTICK_RATE_MS);
				os_printf("TcpServerSend:%d\nport:%d\n", datalen,pesp_conn->proto.tcp->remote_port);
				ret = espconn_send(pesp_conn,data,datalen);
			}

		}
	}
	os_printf("send ret=%d\r\n", ret);
	return ret;
}
int TcpServerSend(uint8 *data, uint16 datalen)
{
	int ret = -1;
	struct espconn *pesp_conn = &esp_conn;
	remot_info *premot = NULL;
	uint8 count = 0;
	sint8 value = ESPCONN_OK;
	uint8 *send = malloc(datalen);
	memcpy(send, data, datalen);
	//*(send+datalen) = '\r';
	//*(send+datalen+1) = '\n';
	if (client_tcp_connect == 1) {
#if TCP_SERVER_MUTI_SEND
		if (espconn_get_connection_info(pesp_conn,&premot,0) == ESPCONN_OK){
			for (count = 0; count < pesp_conn->link_cnt; count ++){
				pesp_conn->proto.tcp->remote_port = premot[count].remote_port;
				pesp_conn->proto.tcp->remote_ip[0] = premot[count].remote_ip[0];
				pesp_conn->proto.tcp->remote_ip[1] = premot[count].remote_ip[1];
				pesp_conn->proto.tcp->remote_ip[2] = premot[count].remote_ip[2];
				pesp_conn->proto.tcp->remote_ip[3] = premot[count].remote_ip[3];
				vTaskDelay(1000/portTICK_RATE_MS);
				os_printf("TcpServerSend:%d\nport:%d\n", datalen,pesp_conn->proto.tcp->remote_port);
				ret = espconn_send(pesp_conn,send,datalen);
				//ret = espconn_send(pesp_conn,"hello",strlen("hello"));
				//ret = espconn_send(pesp_conn,"hello daikin\r\n",strlen("hello daikin\r\n"));
			}

		}
#else
		ret = espconn_send(tcp_server_local,send,datalen);
#endif
	}
	os_printf("send ret=%d\r\n", ret);
	free(send);
	return ret;
}

void ICACHE_FLASH_ATTR camera_block_rx()
{
	uint8 uart_get_temblock[]={0x7e, 0x0, 0x02, 0x0c, 0x00,0x8c};
	printf("camera_block_rx..............=%d\r\n", g_camera_index);
	os_timer_disarm(&camera_timer_t);
	uart_get_temblock[4] = g_camera_index;
	uart_get_temblock[5] = 0x8c + g_camera_index;
	uart0_write_data(uart_get_temblock,sizeof(uart_get_temblock));
}

void TcpServerRecvCb(void *arg, char *pdata, unsigned short len)
{
   struct espconn* tcp_server_local=arg;    
   os_printf("Recv tcp client ip:%d.%d.%d.%d port:%d len:%d\n",tcp_server_local->proto.tcp->remote_ip[0],
		                                          tcp_server_local->proto.tcp->remote_ip[1],
		                                          tcp_server_local->proto.tcp->remote_ip[2],
		                                          tcp_server_local->proto.tcp->remote_ip[3],
		                                          tcp_server_local->proto.tcp->remote_port,
		                                          len);
   //espconn_send(tcp_server_local,pdata,len);
   os_printf("recv %s\r\n", pdata);
   //hspi_write_data(pdata, len);
   if(strncmp(&pdata[0], "getpicture", strlen("getpicture")) == 0) {
	   os_printf("=======================picture====================\r\n");
	   char uart_get_picture[]={0x7e, 0x0, 0x01, 0x0B, 0x8a};
	   g_camera_index = 0;
	   uart0_write_data(uart_get_picture,sizeof(uart_get_picture));
	   resetThermoImage();

   }
   if(strncmp(&pdata[0], "gettemperature", strlen("gettemperature")) == 0) {
	   os_printf("=======================temperature====================\r\n");
	   char uart_get_temperature[]={0x7e, 0x0, 0x01, 0x0A, 0x89};
	   uart0_write_data(uart_get_temperature,sizeof(uart_get_temperature));
	   //espconn_send(tcp_server_local,"gettemperature:25\r\n",strlen("gettemperature:25\r\n"));
   }
   if (strncmp(&pdata[0], "ok", strlen("ok")) == 0) {
	   os_printf("=======================ok====================\r\n");
	   if (b_needresp) {
		char uart_res_thermo[]={0x7e, 0x0, 0x01, 0x9a, 0x19};
		uart0_write_data(uart_res_thermo,sizeof(uart_res_thermo));
	   }
   }
   if (strncmp(&pdata[0], "cameraok", strlen("cameraok")) == 0) {
	   os_printf("=======================camera ok %d====================\r\n", g_camera_index);
	   uint8 uart_get_temblock[]={0x7e, 0x0, 0x02, 0x0c, 0x00,0x8c};
	   g_camera_index++;
	   if (g_camera_index > 150)
	   {
		os_printf("one complete picture is ok\r\n");
	   } else {
		uart_get_temblock[4] = g_camera_index;
		uart_get_temblock[5] = 0x8c + g_camera_index;
		uart0_write_data(uart_get_temblock,sizeof(uart_get_temblock));
				
		os_timer_disarm(&camera_timer_t);
		os_timer_setfn(&camera_timer_t, camera_block_rx , NULL);   //a demo to process the data in uart rx buffer
		os_timer_arm(&camera_timer_t,1000,1);
	   }
   }
}

void TcpServerReconnectCb(void *arg, sint8 err)
{
    struct espconn* tcp_server_local=arg;

	os_printf("TcpServerReconnectCb status:%d\n",err);
	os_printf("tcp client ip:%d.%d.%d.%d port:%d\n",tcp_server_local->proto.tcp->remote_ip[0],
		                                          tcp_server_local->proto.tcp->remote_ip[1],
		                                          tcp_server_local->proto.tcp->remote_ip[2],
		                                          tcp_server_local->proto.tcp->remote_ip[3],
		                                          tcp_server_local->proto.tcp->remote_port\
		                                          );
}

void TcpServerClientDisConnect(void* arg)
{
    struct espconn* tcp_server_local=arg;
	os_printf("TCP server DISCONNECT \r\n");
	os_printf("tcp client ip:%d.%d.%d.%d port:%d\n",tcp_server_local->proto.tcp->remote_ip[0],
		                                          tcp_server_local->proto.tcp->remote_ip[1],
		                                          tcp_server_local->proto.tcp->remote_ip[2],
		                                          tcp_server_local->proto.tcp->remote_ip[3],
		                                          tcp_server_local->proto.tcp->remote_port
		                                          );
	client_tcp_connect = 0;
}

void TcpServerClienSendCb(void* arg)
{
    struct espconn* tcp_server_local=arg;
	os_printf("TCP server SendCb\r\n");
	/*os_printf("tcp client ip:%d.%d.%d.%d port:%d\n",tcp_server_local->proto.tcp->remote_ip[0],
		                                          tcp_server_local->proto.tcp->remote_ip[1],
		                                          tcp_server_local->proto.tcp->remote_ip[2],
		                                          tcp_server_local->proto.tcp->remote_ip[3],
		                                          tcp_server_local->proto.tcp->remote_port
		                                          );*/
	// just test send continuely
	//espconn_send(tcp_server_local,"gettemperature:25\r\n",strlen("gettemperature:25\r\n"));
}

void TcpServerClientConnect(void*arg)
{
	tcp_server_local=arg;
	int ret;
#if TCP_SERVER_KEEP_ALIVE_ENABLE
	espconn_set_opt(tcp_server_local,BIT(3));//enable keep alive ,this api must call in connect callback

	uint32 keep_alvie=0;
	keep_alvie=TCP_SERVER_KEEP_ALIVE_IDLE_S;
	espconn_set_keepalive(tcp_server_local,ESPCONN_KEEPIDLE,&keep_alvie);
	keep_alvie=TCP_SERVER_RETRY_INTVL_S;
	espconn_set_keepalive(tcp_server_local,ESPCONN_KEEPINTVL,&keep_alvie);
	keep_alvie=keep_alvie=TCP_SERVER_RETRY_INTVL_S;
	espconn_set_keepalive(tcp_server_local,ESPCONN_KEEPCNT,&keep_alvie);
	os_printf("keep alive enable.......................\r\n");
#endif
	client_tcp_connect = 1;
	//Here can disable time1
	//os_timer_disarm(&time1);
	os_printf("tcp client ip:%d.%d.%d.%d port:%d\r\n",tcp_server_local->proto.tcp->remote_ip[0],
		                                          tcp_server_local->proto.tcp->remote_ip[1],
		                                          tcp_server_local->proto.tcp->remote_ip[2],
		                                          tcp_server_local->proto.tcp->remote_ip[3],
		                                          tcp_server_local->proto.tcp->remote_port
		                                          );
	espconn_regist_recvcb(tcp_server_local,TcpServerRecvCb);
	espconn_regist_reconcb(tcp_server_local,TcpServerReconnectCb);
	espconn_regist_disconcb(tcp_server_local,TcpServerClientDisConnect);
	espconn_regist_sentcb(tcp_server_local,TcpServerClienSendCb);
												  
	ret= espconn_send(tcp_server_local,TCP_SERVER_GREETING,strlen(TCP_SERVER_GREETING));
	os_printf("send ret=%d\r\n", ret);
}

void TcpLocalServer(void* arg)
{
	esp_conn.type=ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp=&tcp;
	tcp.local_port=TCP_SERVER_LOCAL_PORT;
	client_tcp_connect = 0;
	espconn_regist_connectcb(&esp_conn,TcpServerClientConnect);
	espconn_accept(&esp_conn);
	espconn_regist_time(&esp_conn,0,0);
}

