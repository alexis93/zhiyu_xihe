/*
 * zy_protocol.c
 * 	Created on:2014年6月16号
 * 		Author:jinhan
 */

#include "zy_common.h"
#include <unistd.h>
#include <string.h>
#include "zy_protocol.h"
//#include "zy_log.h"
//#include "gnss_mysql.h"
//#include "crc24q.h"
#include "zy_mempool.h"
#include "zy_tcpserver.h"
//#include <pthread.h>
#include <signal.h>
#include "zy_tcp_worker.h"




int send_all_tcpclient_msg(zy_tcpserver_t* tcpserver,char* buf,int len) {

	int usenode_num,res,i;
		zy_tcpserver_peer_mempool_t* zy_tcpserver_peer;
		zy_tcpserver_peer_mempool_t* zy_tcpserver_errpeer;
		if (len>0) {
			printf("msg%d:'%s'",len,buf);
			usenode_num=tcpserver->mempool.usecount;
			zy_tcpserver_peer=(zy_tcpserver_peer_mempool_t*)(tcpserver->mempool.uselist);

			for (i=0;i<usenode_num;i++){
				res=write(zy_tcpserver_peer->zy_tcpserver_peer.socket_fd,buf,len);

				if (res<1) {
					close(zy_tcpserver_peer->zy_tcpserver_peer.socket_fd);
					zy_tcpserver_peer->zy_tcpserver_peer.socket_fd=-1;
					//send_log("tcp send error!\r\n",17);
					puts("write tcp peer error!");
					zy_tcpserver_errpeer=zy_tcpserver_peer;
					pthread_mutex_lock(&(tcpserver->status_mutex));
					zy_tcpserver_peer=(zy_tcpserver_peer_mempool_t*)(zy_tcpserver_peer->memnode.next);
					zy_mempool_return_node(&(tcpserver->mempool),zy_tcpserver_errpeer);
					pthread_mutex_unlock(&(tcpserver->status_mutex));
					//return -1;
				} else {
					pthread_mutex_lock(&(tcpserver->status_mutex));
					zy_tcpserver_peer=(zy_tcpserver_peer_mempool_t*)(zy_tcpserver_peer->memnode.next);
					pthread_mutex_unlock(&(tcpserver->status_mutex));
				}

			}
		}
	return 0;
}

int do_rcvserver0_protocol(int fd,char *buf,int count)
{
    int res;
    res=send_all_tcpclient_msg(&(channels[0].send_tcpserver),buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}

int do_rcvserver1_protocol(int fd,char *buf,int count)
{
    int res;
    res=send_all_tcpclient_msg(&(channels[1].send_tcpserver),buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}

int do_rcvserver2_protocol(int fd,char *buf,int count)
{
    int res;
    res=send_all_tcpclient_msg(&(channels[2].send_tcpserver),buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}

int do_rcvserver3_protocol(int fd,char *buf,int count)
{
    int res;
    res=send_all_tcpclient_msg(&(channels[3].send_tcpserver),buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}

int do_rcvserver4_protocol(int fd,char *buf,int count)
{
    int res;
    res=send_all_tcpclient_msg(&(channels[4].send_tcpserver),buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}

int do_rcvserver5_protocol(int fd,char *buf,int count)
{
    int res;
    res=send_all_tcpclient_msg(&(channels[5].send_tcpserver),buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}


int do_sendserver0_protocol(int fd,char *buf,int count)
{
    int res=0;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}

int do_sendserver1_protocol(int fd,char *buf,int count)
{
    int res=0;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}
int do_sendserver2_protocol(int fd,char *buf,int count)
{
    int res=0;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}
int do_sendserver3_protocol(int fd,char *buf,int count)
{
    int res=0;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}
int do_sendserver4_protocol(int fd,char *buf,int count)
{
    int res=0;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}

int do_sendserver5_protocol(int fd,char *buf,int count)
{
    int res=0;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}





