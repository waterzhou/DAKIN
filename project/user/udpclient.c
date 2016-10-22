/*************************************************************************
    > File Name: udpclient.c
    > Author: Water Zhou
    > Mail: jianzhengzhou@yandex.com 
    > Created Time: 2016年09月21日 星期三 11时45分20秒
 ************************************************************************/

#include "esp_common.h"
#include "espconn.h"

#define UDP_SERVER_LOCAL_PORT (6666)
#define DEMO_PRODUCT_NAME "DAIKIN"
const uint8 udp_server_ip[4]={255,255,255,255};
os_timer_t time1;
static struct espconn udp_client;

typedef struct s_msg_keepalive {
	uint8 id0;
	uint8 id1;
	uint8 name[9];
	uint8 type;
	uint8 gau8macAddress[6];
}t_msg_keep_alive;

t_msg_keep_alive msg_keep_alive;

void UdpRecvCb(void *arg, char *pdata, unsigned short len)
{
    struct espconn* udp_server_local=arg;

	os_printf("UDP_RECV_CB len:%d ip:%d.%d.%d.%d port:%d\n",len,udp_server_local->proto.tcp->remote_ip[0],
		                                          udp_server_local->proto.tcp->remote_ip[1],
		                                          udp_server_local->proto.tcp->remote_ip[2],
		                                          udp_server_local->proto.tcp->remote_ip[3],
		                                          udp_server_local->proto.tcp->remote_port\
		                                          );
    espconn_send(udp_server_local,pdata,len);
}
void UdpSendCb(void* arg)
{
    struct espconn* udp_server_local=arg;

	/*os_printf("UDP_SEND_CB ip:%d.%d.%d.%d port:%d\n",udp_server_local->proto.tcp->remote_ip[0],
		                                          udp_server_local->proto.tcp->remote_ip[1],
		                                          udp_server_local->proto.tcp->remote_ip[2],
		                                          udp_server_local->proto.tcp->remote_ip[3],
		                                          udp_server_local->proto.tcp->remote_port\
		                                          );*/
}

void t1Callback(void* arg)
{
    //os_printf("t1 callback\n");
	espconn_send(&udp_client,(char *)&msg_keep_alive,sizeof(t_msg_keep_alive));

}
void udpClient(void*arg)
{
   
	static esp_udp udp;
	udp_client.type=ESPCONN_UDP;
	udp_client.proto.udp=&udp;
	udp.remote_port=UDP_SERVER_LOCAL_PORT;
	
    memcpy(udp.remote_ip,udp_server_ip,sizeof(udp_server_ip));
    uint8 i=0;
	os_printf("serve ip:\n");
	for(i=0;i<=3;i++){
        os_printf("%u.",udp_server_ip[i]);
	}
	os_printf("\n remote ip\n");
	for(i=0;i<=3;i++){
        os_printf("%u.",udp.remote_ip[i]);
	}
	os_printf("\n");
	
	memset(&msg_keep_alive, 0, sizeof(msg_keep_alive));
	msg_keep_alive.id0 = 0;
	msg_keep_alive.id1 = 2;
	memcpy(msg_keep_alive.name,DEMO_PRODUCT_NAME, strlen(DEMO_PRODUCT_NAME));
	msg_keep_alive.type = 2;
	if (wifi_get_macaddr(0, msg_keep_alive.gau8macAddress))
	{
		os_printf("get mac addr=%02x:%02x:%02x%02x:%02x:%02x\r\n", MAC2STR(msg_keep_alive.gau8macAddress));
	}

	espconn_regist_recvcb(&udp_client,UdpRecvCb);
	espconn_regist_sentcb(&udp_client,UdpSendCb);
    int8 res=0;
	res=espconn_create(&udp_client);
	
	if(res!=0){
        os_printf("UDP CLIENT CREAT ERR ret:%d\n",res);
	}

	os_timer_disarm(&time1);
	os_timer_setfn(&time1,t1Callback,NULL);
	os_timer_arm(&time1,4000,1);
	
}

