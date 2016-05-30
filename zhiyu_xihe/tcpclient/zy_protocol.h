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

int do_protocol(int fd,char *buf,int count);
int do_protocol2(int fd,char *buf,int count);
int do_protocol3(int fd,char *buf,int count);
int do_protocol4(int fd,char *buf,int count);
int do_protocol5(int fd,char *buf,int count);
#endif
