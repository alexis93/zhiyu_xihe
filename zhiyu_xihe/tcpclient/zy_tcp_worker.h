/*
 * zy_tcp_worker.h
 *
 *  Created on: Jul 26, 2015
 *      Author: root
 */

#ifndef ZY_TCP_WORKER_H_
#define ZY_TCP_WORKER_H_

//#include "zy_tcpserver.h"

//extern zy_tcpserver_t nmea_tcpserver;
//extern zy_tcpserver_t gnss_tcpserver;
//extern zy_tcpserver_t ins_tcpserver;

int tcp_client_init(void);

extern int send_nmea(char* buf,int len);
extern int send_gnss(char* buf,int len);
extern int send_gnss_nmea(char* buf,int len);
extern int send_ins(char* buf,int len);
extern int send_ins_nmea(char* buf,int len);
extern int send_log(char* buf,int len);
extern int send_obs_log(char* buf,int len);
extern int send_eph_log(char* buf,int len);
extern int send_clockorbit_log(char* buf,int len);
extern int send_msg_log(char* buf,int len);
#endif /* ZY_TCP_WORKER_H_ */
