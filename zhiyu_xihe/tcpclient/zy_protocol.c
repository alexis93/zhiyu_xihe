/*
 * zy_protocol.c
 * 	Created on:2014年6月16号
 * 		Author:jinhan
 */

#include "zy_common.h"
#include <unistd.h>
#include <string.h>
#include "zy_protocol.h"
#include "zy_tcpclient.h"
#include <signal.h>


int do_protocol(int fd,char *buf,int count)
{
    int res;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}

int do_protocol2(int fd,char *buf,int count)
{
/*
	if(((head_packet_t *)buf)->head_sign == 0x5AA5)
	{
		switch(((head_packet_t *)buf)->ordercode)
		{
			case 0x3001:
				//do_login_mysql_reply(fd,(clock_reply_packet_t*)buf,count);
				break;
			default:
				return 0;
		}
	}else if(((clockdata_t *)buf)->head_data==0xD3)
	{
		senddata(buf,count);
	}
	return 0;
*/
	int res;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}


int do_protocol3(int fd,char *buf,int count)
{
	int res;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}

int do_protocol4(int fd,char *buf,int count)
{
	int res;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}
int do_protocol5(int fd,char *buf,int count)
{
	int res;
	//res=write(fd,(uint8_t*)buf,count);
	if (res<0) {
		puts("write error!");
	}
	return 0;
}





