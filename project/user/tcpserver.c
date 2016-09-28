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

void TcpServerClientConnect(void*arg)
{
    struct espconn* tcp_server_local=arg;
#if TCP_SERVER_KEEP_ALIVE_ENABLE
	espconn_set_opt(tcp_server_local,BIT(3));//enable keep alive ,this api must call in connect callback

	uint32 keep_alvie=0;
	keep_alvie=TCP_SERVER_KEEP_ALIVE_IDLE_S;
	espconn_set_keepalive(tcp_server_local,ESPCONN_KEEPIDLE,&keep_alvie);
	keep_alvie=TCP_SERVER_RETRY_INTVL_S;
	espconn_set_keepalive(tcp_server_local,ESPCONN_KEEPINTVL,&keep_alvie);
	keep_alvie=keep_alvie=TCP_SERVER_RETRY_INTVL_S;
	espconn_set_keepalive(tcp_server_local,ESPCONN_KEEPCNT,&keep_alvie);
	os_printf("keep alive enable\n");
#endif

	os_printf("tcp client ip:%d.%d.%d.%d port:%d",tcp_server_local->proto.tcp->remote_ip[0],
		                                          tcp_server_local->proto.tcp->remote_ip[1],
		                                          tcp_server_local->proto.tcp->remote_ip[2],
		                                          tcp_server_local->proto.tcp->remote_ip[3],
		                                          tcp_server_local->proto.tcp->remote_port
		                                          );
	espconn_send(tcp_server_local,TCP_SERVER_GREETING,strlen(TCP_SERVER_GREETING));
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
   }
   if(strncmp(&pdata[0], "gettemperature", strlen("gettemperature")) == 0) {
	   os_printf("=======================temperature====================\r\n");
	   char uart_get_temperature[]={0x7e, 0x0, 0x01, 0x0A, 0x89};
	   uart0_write_data(uart_get_temperature,sizeof(uart_get_temperature));
	   //espconn_send(tcp_server_local,"gettemperature:25\r\n",strlen("gettemperature:25\r\n"));
   }
}

void TcpServerReconnectCb(void *arg, sint8 err)
{
    struct espconn* tcp_server_local=arg;

	os_printf("status:%d\n",err);
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
}

void TcpServerClienSendCb(void* arg)
{
    struct espconn* tcp_server_local=arg;
	os_printf("TCP server SendCb\r\n");
	os_printf("tcp client ip:%d.%d.%d.%d port:%d\n",tcp_server_local->proto.tcp->remote_ip[0],
		                                          tcp_server_local->proto.tcp->remote_ip[1],
		                                          tcp_server_local->proto.tcp->remote_ip[2],
		                                          tcp_server_local->proto.tcp->remote_ip[3],
		                                          tcp_server_local->proto.tcp->remote_port
		                                          );
}

void TcpLocalServer(void* arg)
{
   
	static struct espconn tcp_server_local;
	static esp_tcp tcp;
	tcp_server_local.type=ESPCONN_TCP;
	tcp_server_local.proto.tcp=&tcp;
	tcp.local_port=TCP_SERVER_LOCAL_PORT;

	espconn_regist_connectcb(&tcp_server_local,TcpServerClientConnect);
	espconn_regist_recvcb(&tcp_server_local,TcpServerRecvCb);
	espconn_regist_reconcb(&tcp_server_local,TcpServerReconnectCb);
	espconn_regist_disconcb(&tcp_server_local,TcpServerClientDisConnect);
	espconn_regist_sentcb(&tcp_server_local,TcpServerClienSendCb);
	
	espconn_accept(&tcp_server_local);

}

