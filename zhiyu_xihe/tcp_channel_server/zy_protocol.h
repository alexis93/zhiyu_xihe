/*
 * zy_protocol.h
 *
 * 	Created on:2014年6月16号
 * 		Author:jinhan
 */

#ifndef ZY_PROTOCOL_H_
#define ZY_PROTOCOL_H_

#include <stdint.h>
#include <sys/socket.h>
//#include "zy_net.h"

int do_rcvserver0_protocol(int fd,char *buf,int count);
int do_sendserver0_protocol(int fd,char *buf,int count);

int do_rcvserver1_protocol(int fd,char *buf,int count);
int do_sendserver1_protocol(int fd,char *buf,int count);

int do_rcvserver2_protocol(int fd,char *buf,int count);
int do_sendserver2_protocol(int fd,char *buf,int count);

int do_rcvserver3_protocol(int fd,char *buf,int count);
int do_sendserver3_protocol(int fd,char *buf,int count);

int do_rcvserver4_protocol(int fd,char *buf,int count);
int do_sendserver4_protocol(int fd,char *buf,int count);

int do_rcvserver5_protocol(int fd,char *buf,int count);
int do_sendserver5_protocol(int fd,char *buf,int count);
#endif
