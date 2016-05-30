/*-----------------------------------------------------------------
 *	iwise_net_log.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-23	
 *-------------------------------------------------------------------*/
#ifndef _IWISE_NET_LOG_H_
#define _IWISE_NET_LOG_H_

typedef void (* send_log_callback)(char* buf,int len);
typedef void (* send_obs_callback)(char* buf,int len);
typedef void (* send_eph_callback)(char* buf,int len);
typedef void (* send_clockorbit_callback)(char* buf,int len);
typedef void (* send_msg_callback)(char* buf,int len);

typedef struct net_log_interface_s {
	send_log_callback send_log;
	send_obs_callback send_obs;
	send_eph_callback send_eph;
	send_clockorbit_callback send_clockorbit;
	send_msg_callback send_msg;
} net_log_t;

extern int set_net_log_interface(net_log_t *log);
extern net_log_t *netlog;

#endif
