#include "zy_tcpclient.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>

int zy_tcpclient_config(zy_tcpclient_t* tcpclient,char* name,char* ip,uint16_t port,int (*do_protocol)(int fd,char* buf,int count))
{
	tcpclient->ip = ip;
	tcpclient->port = port;
	tcpclient->do_protocol=do_protocol; //协议处理函数，每个tcpserver需要设置
	sprintf(tcpclient->tcpname,"%s",name);
	return 0;
}

int zy_tcpclient_start(zy_tcpclient_t* tcpclient)
{

	tcpclient->socket_fd=socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr_serv;
	memset(&addr_serv, 0, sizeof(addr_serv));
	addr_serv.sin_family = AF_INET;
	addr_serv.sin_port = htons(tcpclient->port);
	addr_serv.sin_addr.s_addr = inet_addr(tcpclient->ip);
	// 绑定端口和地址
	int ret = connect(tcpclient->socket_fd, (struct sockaddr *)&addr_serv, sizeof(addr_serv));
	if(ret < 0){
		if(errno != EINPROGRESS && errno != EWOULDBLOCK){
		//	GPS_LOGE("connect clockorbit server error [%s : %d]", tcpclient->ip, tcpclient->port);
			return -1;
		}
	}

	int flag = fcntl(tcpclient->socket_fd, F_GETFL, 0);
	fcntl(tcpclient->socket_fd, F_SETFL, flag|O_NONBLOCK);
	return tcpclient->socket_fd;
}

int zy_tcpclient_send(zy_tcpclient_t* tcpclient,char* buf,int len)
{
	int res;	
	res=write(tcpclient->socket_fd,buf,len);
	if (res<0) {
		close(tcpclient->socket_fd);
		res=zy_tcpclient_start(tcpclient);
		if (res<0) {
			return -1;
		}
		res=write(tcpclient->socket_fd,buf,len);		
	}
	return res;
}

int zy_tcpclient_stop(zy_tcpclient_t* tcpclient)
{
	return close(tcpclient->socket_fd);
}


