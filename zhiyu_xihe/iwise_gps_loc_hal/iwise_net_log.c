/*-----------------------------------------------------------------
 *	iwise_net_log.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-09-3	created by zhuyong
 *
 *-------------------------------------------------------------------*/

#include "string.h"
#include "iwise_net_log.h"

net_log_t netlog;

int set_net_log_interface(net_log_t *log) {
	netlog.send_log=log->send_log;
	netlog.send_obs=log->send_obs;
	netlog.send_eph=log->send_eph;
	netlog.send_clockorbit=log->send_clockorbit;
	netlog.send_msg=log->send_msg;
	return 1;
}

int send_obs_netlog(iwise_gnss_obs_t *iwise_obs){
	int i;
	char tempbuf[1024];
	int len;
	int pos=0;

	len=sprintf(tempbuf,"/***********obs:%d ************/\r\n",iwise_obs->n);
	netlog.send_obs(tempbuf,len);


	for (i=0;i<iwise_obs->n;i++){
		len=sprintf(tempbuf,"%d,", iwise_obs->data[i].sat);
		netlog.send_obs(tempbuf,len);
	}

	len=sprintf(tempbuf,"\r\n");
	netlog.send_obs(tempbuf,len);
	return 1;
}

int send_obs2eph_netlog(iwise_gnss_obs_t *iwise_obs,iwise_gnss_nav_t *iwise_eph){
	int i,j,n=0;
	char tempbuf[1024];
	int len;
	int pos=0;

	len=sprintf(tempbuf,"/***********obs2eph:%d ************/\r\n",iwise_obs->n);
	netlog.send_obs(tempbuf,len);
	for (i=0;i<iwise_obs->n;i++){
		for (j=0;j<iwise_eph->n;j++){
			if (iwise_eph->eph[j].sat==iwise_obs->data[i].sat) {
				len=sprintf(tempbuf,"%d,", iwise_obs->data[i].sat);
				netlog.send_obs(tempbuf,len);
				n++;
			}
		}
	}

	len=sprintf(tempbuf,"count:%d \r\n",n);
	netlog.send_obs(tempbuf,len);
	return 1;
}

int send_eph_netlog(iwise_gnss_nav_t *iwise_eph){
	int i;
	char tempbuf[1024];
	int len;
	int pos=0;

	len=sprintf(tempbuf,"/***********nav:%d ************/\r\n",iwise_eph->n);
	netlog.send_eph(tempbuf,len);


	for (i=0;i<iwise_eph->n;i++){
		len=sprintf(tempbuf,"%d,", iwise_eph->eph[i].sat);
		netlog.send_eph(tempbuf,len);
	}

	len=sprintf(tempbuf,"\r\n");
	netlog.send_eph(tempbuf,len);
	return 1;
}

int send_obs2eph_netlog_ex(iwise_gnss_obs_t *iwise_obs,iwise_gnss_nav_t *iwise_eph){
	int i,j,n=0;
	char tempbuf[1024];
	int len=0;
	int pos=0;

	len=sprintf(tempbuf,"obs:%d",iwise_obs->n);
	for (i=0;i<iwise_obs->n;i++){
		len=sprintf(tempbuf,"%s,%d",tempbuf,iwise_obs->data[i].sat);
	}
	len=sprintf(tempbuf,"%s\r\n",tempbuf);

	len=sprintf(tempbuf,"%sobs2eph:%d",tempbuf,iwise_obs->n);
	for (i=0;i<iwise_obs->n;i++){
		for (j=0;j<iwise_eph->n;j++){
			if (iwise_eph->eph[j].sat==iwise_obs->data[i].sat) {
				len=sprintf(tempbuf,"%s,%d", tempbuf,iwise_obs->data[i].sat);
				n++;
			}
		}
	}

	len=sprintf(tempbuf,"%s,count:%d \r\n",tempbuf,n);
	netlog.send_obs(tempbuf,len);
	return 1;
}
